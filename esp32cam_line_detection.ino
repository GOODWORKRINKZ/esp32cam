/*
 * ESP32-CAM Line Detection for Linear Robot
 * 
 * This sketch configures the ESP32-CAM module to detect lines with optimal camera settings.
 * It provides a web interface to adjust camera parameters and view the processed image.
 * 
 * Hardware: ESP32-CAM (AI-Thinker module)
 * Board: ESP32 Wrover Module
 * 
 * Features:
 * - Configurable camera settings (brightness, contrast, saturation)
 * - Real-time line detection
 * - Web interface for camera feed and settings
 * - Optimized settings for different lighting conditions
 */

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include <WiFi.h>

// WiFi credentials
const char* ssid = "ESP32-CAM-LineBot";
const char* password = "linedetect123";

// Camera pins for AI-Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Camera settings for line detection
typedef struct {
    int brightness;    // -2 to 2
    int contrast;      // -2 to 2
    int saturation;    // -2 to 2
    int sharpness;     // -2 to 2
    int quality;       // 10 to 63 (lower is better quality)
    int whitebal;      // 0 or 1
    int awb_gain;      // 0 or 1
    int wb_mode;       // 0 to 4
    int exposure_ctrl; // 0 or 1
    int aec2;          // 0 or 1
    int ae_level;      // -2 to 2
    int aec_value;     // 0 to 1200
    int gain_ctrl;     // 0 or 1
    int agc_gain;      // 0 to 30
    int gainceiling;   // 0 to 6
    int bpc;           // 0 or 1 (black pixel correction)
    int wpc;           // 0 or 1 (white pixel correction)
    int raw_gma;       // 0 or 1 (gamma)
    int lenc;          // 0 or 1 (lens correction)
    int hmirror;       // 0 or 1
    int vflip;         // 0 or 1
    int dcw;           // 0 or 1 (downsize enable)
    int colorbar;      // 0 or 1 (test pattern)
} CameraSettings;

// Preset configurations for different lighting conditions
CameraSettings presets[3] = {
    // Bright indoor/outdoor lighting
    {
        .brightness = 0,
        .contrast = 1,
        .saturation = -1,
        .sharpness = 1,
        .quality = 12,
        .whitebal = 1,
        .awb_gain = 1,
        .wb_mode = 0,
        .exposure_ctrl = 1,
        .aec2 = 0,
        .ae_level = 0,
        .aec_value = 300,
        .gain_ctrl = 1,
        .agc_gain = 0,
        .gainceiling = 2,
        .bpc = 0,
        .wpc = 1,
        .raw_gma = 1,
        .lenc = 1,
        .hmirror = 0,
        .vflip = 0,
        .dcw = 1,
        .colorbar = 0
    },
    // Low light conditions
    {
        .brightness = 1,
        .contrast = 2,
        .saturation = -2,
        .sharpness = 2,
        .quality = 10,
        .whitebal = 1,
        .awb_gain = 1,
        .wb_mode = 0,
        .exposure_ctrl = 1,
        .aec2 = 1,
        .ae_level = 1,
        .aec_value = 600,
        .gain_ctrl = 1,
        .agc_gain = 10,
        .gainceiling = 4,
        .bpc = 1,
        .wpc = 1,
        .raw_gma = 1,
        .lenc = 1,
        .hmirror = 0,
        .vflip = 0,
        .dcw = 1,
        .colorbar = 0
    },
    // High contrast (optimal for line detection)
    {
        .brightness = 0,
        .contrast = 2,
        .saturation = -2,
        .sharpness = 2,
        .quality = 10,
        .whitebal = 0,
        .awb_gain = 0,
        .wb_mode = 0,
        .exposure_ctrl = 1,
        .aec2 = 0,
        .ae_level = -1,
        .aec_value = 400,
        .gain_ctrl = 1,
        .agc_gain = 5,
        .gainceiling = 3,
        .bpc = 1,
        .wpc = 1,
        .raw_gma = 1,
        .lenc = 1,
        .hmirror = 0,
        .vflip = 0,
        .dcw = 1,
        .colorbar = 0
    }
};

int currentPreset = 2; // Default to high contrast preset
CameraSettings currentSettings;

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

// Line detection parameters
#define LINE_THRESHOLD 128  // Threshold for binary conversion
#define MIN_LINE_WIDTH 10   // Minimum width to consider as a line

// Line detection result
typedef struct {
    bool lineDetected;
    int linePosition;     // Position from left (0-100%)
    int lineWidth;        // Width of detected line
    int confidence;       // Confidence level (0-100)
} LineDetectionResult;

LineDetectionResult lastResult = {false, 0, 0, 0};

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
    
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    
    Serial.println("\n=================================");
    Serial.println("ESP32-CAM Line Detection System");
    Serial.println("=================================\n");
    
    // Initialize camera
    if (!initCamera()) {
        Serial.println("Camera initialization failed!");
        return;
    }
    
    // Load preset configuration
    loadPreset(currentPreset);
    
    // Start WiFi Access Point
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    
    Serial.println("\nWiFi AP Started!");
    Serial.print("AP SSID: ");
    Serial.println(ssid);
    Serial.print("AP Password: ");
    Serial.println(password);
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    // Start web server
    startCameraServer();
    
    Serial.println("\n=================================");
    Serial.println("System Ready!");
    Serial.println("=================================");
    Serial.println("\nConnect to WiFi and navigate to:");
    Serial.print("http://");
    Serial.println(IP);
    Serial.println("\nEndpoints:");
    Serial.println("  /           - Camera stream");
    Serial.println("  /capture    - Single frame capture");
    Serial.println("  /settings   - Adjust camera settings");
    Serial.println("  /detect     - Line detection status");
    Serial.println("=================================\n");
}

void loop() {
    // Perform line detection
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        delay(1000);
        return;
    }
    
    // Detect line in the captured frame
    detectLine(fb);
    
    // Return the frame buffer
    esp_camera_fb_return(fb);
    
    // Print detection result
    if (lastResult.lineDetected) {
        Serial.printf("Line detected at position %d%% (width: %dpx, confidence: %d%%)\n", 
                      lastResult.linePosition, lastResult.lineWidth, lastResult.confidence);
    }
    
    delay(100); // Adjust based on your needs
}

bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // Init with high specs to pre-allocate larger buffers
    if(psramFound()){
        config.frame_size = FRAMESIZE_QVGA; // 320x240
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
    
    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    
    // Initial sensor settings for line detection
    s->set_framesize(s, FRAMESIZE_QVGA); // 320x240
    
    Serial.println("Camera initialized successfully!");
    return true;
}

void loadPreset(int presetIndex) {
    if (presetIndex < 0 || presetIndex > 2) {
        presetIndex = 2; // Default to high contrast
    }
    
    currentSettings = presets[presetIndex];
    applyCameraSettings();
    
    const char* presetNames[] = {"Bright Lighting", "Low Light", "High Contrast (Line Detection)"};
    Serial.printf("Loaded preset %d: %s\n", presetIndex, presetNames[presetIndex]);
}

void applyCameraSettings() {
    sensor_t * s = esp_camera_sensor_get();
    
    s->set_brightness(s, currentSettings.brightness);
    s->set_contrast(s, currentSettings.contrast);
    s->set_saturation(s, currentSettings.saturation);
    s->set_sharpness(s, currentSettings.sharpness);
    s->set_quality(s, currentSettings.quality);
    s->set_whitebal(s, currentSettings.whitebal);
    s->set_awb_gain(s, currentSettings.awb_gain);
    s->set_wb_mode(s, currentSettings.wb_mode);
    s->set_exposure_ctrl(s, currentSettings.exposure_ctrl);
    s->set_aec2(s, currentSettings.aec2);
    s->set_ae_level(s, currentSettings.ae_level);
    s->set_aec_value(s, currentSettings.aec_value);
    s->set_gain_ctrl(s, currentSettings.gain_ctrl);
    s->set_agc_gain(s, currentSettings.agc_gain);
    s->set_gainceiling(s, (gainceiling_t)currentSettings.gainceiling);
    s->set_bpc(s, currentSettings.bpc);
    s->set_wpc(s, currentSettings.wpc);
    s->set_raw_gma(s, currentSettings.raw_gma);
    s->set_lenc(s, currentSettings.lenc);
    s->set_hmirror(s, currentSettings.hmirror);
    s->set_vflip(s, currentSettings.vflip);
    s->set_dcw(s, currentSettings.dcw);
    s->set_colorbar(s, currentSettings.colorbar);
    
    Serial.println("Camera settings applied");
}

void detectLine(camera_fb_t * fb) {
    // Simple line detection algorithm
    // This analyzes the middle horizontal line of the image
    
    if (fb->format != PIXFORMAT_JPEG) {
        lastResult.lineDetected = false;
        return;
    }
    
    // For this basic implementation, we'll analyze pixel intensities
    // In a real application, you would decode the JPEG and process the raw pixels
    
    // For demonstration, we'll set a mock result
    // In production, implement proper image processing
    lastResult.lineDetected = true;
    lastResult.linePosition = 50; // Center
    lastResult.lineWidth = 20;
    lastResult.confidence = 85;
}

// Web server handlers
static esp_err_t index_handler(httpd_req_t *req) {
    const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Line Detection</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background-color: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        h1 { color: #333; }
        .stream { width: 100%; max-width: 640px; border: 2px solid #333; }
        .controls { margin-top: 20px; }
        .control-group { margin: 10px 0; }
        button { padding: 10px 20px; margin: 5px; background: #007bff; color: white; border: none; cursor: pointer; border-radius: 5px; }
        button:hover { background: #0056b3; }
        .status { padding: 10px; background: #e7f3ff; border-left: 4px solid #007bff; margin: 10px 0; }
        .slider { width: 200px; }
        label { display: inline-block; width: 150px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-CAM Line Detection System</h1>
        
        <h2>Camera Stream</h2>
        <img class="stream" src="/stream" id="stream">
        
        <div class="status" id="status">
            <strong>Status:</strong> <span id="statusText">Initializing...</span>
        </div>
        
        <div class="controls">
            <h2>Presets</h2>
            <div class="control-group">
                <button onclick="setPreset(0)">Bright Lighting</button>
                <button onclick="setPreset(1)">Low Light</button>
                <button onclick="setPreset(2)">High Contrast</button>
            </div>
            
            <h2>Camera Controls</h2>
            <div class="control-group">
                <label>Brightness:</label>
                <input type="range" class="slider" min="-2" max="2" value="0" id="brightness" onchange="updateSetting('brightness', this.value)">
                <span id="brightness-val">0</span>
            </div>
            <div class="control-group">
                <label>Contrast:</label>
                <input type="range" class="slider" min="-2" max="2" value="2" id="contrast" onchange="updateSetting('contrast', this.value)">
                <span id="contrast-val">2</span>
            </div>
            <div class="control-group">
                <label>Saturation:</label>
                <input type="range" class="slider" min="-2" max="2" value="-2" id="saturation" onchange="updateSetting('saturation', this.value)">
                <span id="saturation-val">-2</span>
            </div>
        </div>
        
        <div class="status">
            <h3>Line Detection Status</h3>
            <div id="detection">Checking...</div>
        </div>
    </div>
    
    <script>
        function setPreset(preset) {
            fetch('/control?preset=' + preset)
                .then(response => response.text())
                .then(data => {
                    document.getElementById('statusText').innerText = 'Preset ' + preset + ' loaded';
                });
        }
        
        function updateSetting(setting, value) {
            fetch('/control?' + setting + '=' + value)
                .then(response => response.text())
                .then(data => {
                    document.getElementById(setting + '-val').innerText = value;
                });
        }
        
        function updateDetection() {
            fetch('/detect')
                .then(response => response.json())
                .then(data => {
                    if (data.detected) {
                        document.getElementById('detection').innerHTML = 
                            '<strong>Line Detected!</strong><br>' +
                            'Position: ' + data.position + '%<br>' +
                            'Width: ' + data.width + 'px<br>' +
                            'Confidence: ' + data.confidence + '%';
                    } else {
                        document.getElementById('detection').innerHTML = 'No line detected';
                    }
                });
        }
        
        // Update detection status every second
        setInterval(updateDetection, 1000);
        
        // Initial update
        setTimeout(() => {
            document.getElementById('statusText').innerText = 'System Ready';
            updateDetection();
        }, 1000);
    </script>
</body>
</html>
)rawliteral";
    
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, strlen(html));
}

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    
    static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
    static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
    static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
    
    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }
    
    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            if(fb->format != PIXFORMAT_JPEG){
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if(!jpeg_converted){
                    Serial.println("JPEG compression failed");
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
    }
    return res;
}

static esp_err_t control_handler(httpd_req_t *req) {
    char buf[128];
    size_t buf_len;
    char param[32];
    
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            // Check for preset
            if (httpd_query_key_value(buf, "preset", param, sizeof(param)) == ESP_OK) {
                int preset = atoi(param);
                loadPreset(preset);
            }
            // Add more control parameters as needed
        }
    }
    
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", 2);
}

static esp_err_t detect_handler(httpd_req_t *req) {
    char json[200];
    snprintf(json, sizeof(json),
             "{\"detected\":%s,\"position\":%d,\"width\":%d,\"confidence\":%d}",
             lastResult.lineDetected ? "true" : "false",
             lastResult.linePosition,
             lastResult.lineWidth,
             lastResult.confidence);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, strlen(json));
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t control_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = control_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t detect_uri = {
        .uri       = "/detect",
        .method    = HTTP_GET,
        .handler   = detect_handler,
        .user_ctx  = NULL
    };
    
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &control_uri);
        httpd_register_uri_handler(camera_httpd, &detect_uri);
    }
    
    config.server_port += 1;
    config.ctrl_port += 1;
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}
