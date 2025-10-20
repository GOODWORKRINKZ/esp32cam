#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// WiFi credentials - update these for your network
const char* ssid = "ESP32-CAM-LineDetector";
const char* password = "12345678";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

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

// Camera configuration
camera_config_t camera_config;
sensor_t * s = NULL;

// Current camera settings for line detection
struct CameraSettings {
  int framesize = 7;  // FRAMESIZE_VGA (640x480) - good balance
  int quality = 10;    // 0-63, lower is better quality
  int brightness = 0;  // -2 to 2
  int contrast = 0;    // -2 to 2
  int saturation = -2; // -2 to 2, lower for better B&W line detection
  int sharpness = 0;   // -2 to 2
  int denoise = 0;     // 0-8
  int special_effect = 2; // 0: No effect, 2: Grayscale (best for line detection)
  int wb_mode = 0;     // 0: Auto, 1: Sunny, 2: Cloudy, 3: Office, 4: Home
  int awb = 1;         // Auto white balance
  int awb_gain = 1;    // Auto white balance gain
  int aec = 1;         // Auto exposure control
  int aec2 = 1;        // Auto exposure control 2
  int ae_level = 0;    // -2 to 2
  int aec_value = 300; // 0-1200
  int agc = 1;         // Auto gain control
  int agc_gain = 0;    // 0-30
  int gainceiling = 0; // 0-6
  int bpc = 0;         // Black pixel correction
  int wpc = 1;         // White pixel correction
  int raw_gma = 1;     // Gamma correction
  int lenc = 1;        // Lens correction
  int hmirror = 0;     // Horizontal mirror
  int vflip = 0;       // Vertical flip
  int dcw = 1;         // Downsize enable
  int colorbar = 0;    // Color bar test pattern
} settings;

void initCamera() {
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d0 = Y2_GPIO_NUM;
  camera_config.pin_d1 = Y3_GPIO_NUM;
  camera_config.pin_d2 = Y4_GPIO_NUM;
  camera_config.pin_d3 = Y5_GPIO_NUM;
  camera_config.pin_d4 = Y6_GPIO_NUM;
  camera_config.pin_d5 = Y7_GPIO_NUM;
  camera_config.pin_d6 = Y8_GPIO_NUM;
  camera_config.pin_d7 = Y9_GPIO_NUM;
  camera_config.pin_xclk = XCLK_GPIO_NUM;
  camera_config.pin_pclk = PCLK_GPIO_NUM;
  camera_config.pin_vsync = VSYNC_GPIO_NUM;
  camera_config.pin_href = HREF_GPIO_NUM;
  camera_config.pin_sscb_sda = SIOD_GPIO_NUM;
  camera_config.pin_sscb_scl = SIOC_GPIO_NUM;
  camera_config.pin_pwdn = PWDN_GPIO_NUM;
  camera_config.pin_reset = RESET_GPIO_NUM;
  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = PIXFORMAT_JPEG;
  camera_config.frame_size = (framesize_t)settings.framesize;
  camera_config.jpeg_quality = settings.quality;
  camera_config.fb_count = 1;
  camera_config.grab_mode = CAMERA_GRAB_LATEST;

  // Init Camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("Failed to get camera sensor");
    return;
  }

  applyCameraSettings();
}

void applyCameraSettings() {
  if (s == NULL) return;

  s->set_framesize(s, (framesize_t)settings.framesize);
  s->set_quality(s, settings.quality);
  s->set_brightness(s, settings.brightness);
  s->set_contrast(s, settings.contrast);
  s->set_saturation(s, settings.saturation);
  s->set_sharpness(s, settings.sharpness);
  s->set_denoise(s, settings.denoise);
  s->set_special_effect(s, settings.special_effect);
  s->set_wb_mode(s, settings.wb_mode);
  s->set_whitebal(s, settings.awb);
  s->set_awb_gain(s, settings.awb_gain);
  s->set_exposure_ctrl(s, settings.aec);
  s->set_aec2(s, settings.aec2);
  s->set_ae_level(s, settings.ae_level);
  s->set_aec_value(s, settings.aec_value);
  s->set_gain_ctrl(s, settings.agc);
  s->set_agc_gain(s, settings.agc_gain);
  s->set_gainceiling(s, (gainceiling_t)settings.gainceiling);
  s->set_bpc(s, settings.bpc);
  s->set_wpc(s, settings.wpc);
  s->set_raw_gma(s, settings.raw_gma);
  s->set_lenc(s, settings.lenc);
  s->set_hmirror(s, settings.hmirror);
  s->set_vflip(s, settings.vflip);
  s->set_dcw(s, settings.dcw);
  s->set_colorbar(s, settings.colorbar);
}

String getMainPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-CAM Line Detector</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 { font-size: 2em; margin-bottom: 10px; }
        .header p { opacity: 0.9; }
        .content {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            padding: 30px;
        }
        .camera-view {
            grid-column: 1 / -1;
            text-align: center;
        }
        .camera-view img {
            width: 100%;
            max-width: 640px;
            border-radius: 10px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .controls {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
        }
        .control-group {
            margin-bottom: 20px;
        }
        .control-group label {
            display: block;
            font-weight: bold;
            margin-bottom: 8px;
            color: #333;
        }
        .control-group input[type="range"] {
            width: 100%;
            height: 6px;
            border-radius: 3px;
            background: #ddd;
            outline: none;
        }
        .control-group input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
        }
        .control-group select {
            width: 100%;
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 14px;
        }
        .value-display {
            display: inline-block;
            float: right;
            background: #667eea;
            color: white;
            padding: 2px 10px;
            border-radius: 12px;
            font-size: 12px;
        }
        .presets {
            grid-column: 1 / -1;
            display: flex;
            gap: 10px;
            justify-content: center;
            flex-wrap: wrap;
        }
        .preset-btn {
            padding: 12px 24px;
            border: none;
            border-radius: 25px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            cursor: pointer;
            font-size: 14px;
            font-weight: bold;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        .preset-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.3);
        }
        .info {
            grid-column: 1 / -1;
            background: #fff3cd;
            padding: 15px;
            border-radius: 10px;
            border-left: 5px solid #ffc107;
        }
        .info h3 { margin-bottom: 10px; color: #856404; }
        .info ul { margin-left: 20px; color: #856404; }
        .info li { margin-bottom: 5px; }
        @media (max-width: 768px) {
            .content { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🤖 ESP32-CAM Line Detector</h1>
            <p>Настройка параметров камеры для распознавания линии</p>
        </div>
        <div class="content">
            <div class="camera-view">
                <img id="stream" src="/stream">
            </div>
            
            <div class="info">
                <h3>💡 Рекомендации для детектирования линии:</h3>
                <ul>
                    <li><strong>Grayscale Effect:</strong> Лучше всего для черно-белых линий</li>
                    <li><strong>Низкая насыщенность:</strong> Уменьшает цветовые помехи</li>
                    <li><strong>Контраст:</strong> Повышайте для четкости линии на фоне</li>
                    <li><strong>Разрешение:</strong> VGA (640x480) - баланс скорости и качества</li>
                    <li><strong>Качество:</strong> 10-15 для быстрой обработки</li>
                </ul>
            </div>

            <div class="presets">
                <button class="preset-btn" onclick="setPreset('highQuality')">Высокое качество</button>
                <button class="preset-btn" onclick="setPreset('balanced')">Сбалансированный</button>
                <button class="preset-btn" onclick="setPreset('highSpeed')">Высокая скорость</button>
                <button class="preset-btn" onclick="setPreset('indoor')">В помещении</button>
                <button class="preset-btn" onclick="setPreset('outdoor')">На улице</button>
            </div>

            <div class="controls">
                <h3 style="margin-bottom: 15px;">Основные параметры</h3>
                
                <div class="control-group">
                    <label>Разрешение <span class="value-display" id="framesize-val">VGA</span></label>
                    <select id="framesize" onchange="updateSetting('framesize', this.value)">
                        <option value="5">QVGA (320x240)</option>
                        <option value="6">CIF (400x296)</option>
                        <option value="7" selected>VGA (640x480)</option>
                        <option value="8">SVGA (800x600)</option>
                        <option value="9">XGA (1024x768)</option>
                        <option value="10">HD (1280x720)</option>
                        <option value="11">SXGA (1280x1024)</option>
                        <option value="12">UXGA (1600x1200)</option>
                    </select>
                </div>

                <div class="control-group">
                    <label>Качество (0-63) <span class="value-display" id="quality-val">10</span></label>
                    <input type="range" id="quality" min="0" max="63" value="10" 
                           onchange="updateSetting('quality', this.value)">
                </div>

                <div class="control-group">
                    <label>Яркость (-2 to 2) <span class="value-display" id="brightness-val">0</span></label>
                    <input type="range" id="brightness" min="-2" max="2" value="0" 
                           onchange="updateSetting('brightness', this.value)">
                </div>

                <div class="control-group">
                    <label>Контраст (-2 to 2) <span class="value-display" id="contrast-val">0</span></label>
                    <input type="range" id="contrast" min="-2" max="2" value="0" 
                           onchange="updateSetting('contrast', this.value)">
                </div>

                <div class="control-group">
                    <label>Насыщенность (-2 to 2) <span class="value-display" id="saturation-val">-2</span></label>
                    <input type="range" id="saturation" min="-2" max="2" value="-2" 
                           onchange="updateSetting('saturation', this.value)">
                </div>

                <div class="control-group">
                    <label>Эффект <span class="value-display" id="effect-val">Grayscale</span></label>
                    <select id="special_effect" onchange="updateSetting('special_effect', this.value)">
                        <option value="0">No Effect</option>
                        <option value="1">Negative</option>
                        <option value="2" selected>Grayscale</option>
                        <option value="3">Red Tint</option>
                        <option value="4">Green Tint</option>
                        <option value="5">Blue Tint</option>
                        <option value="6">Sepia</option>
                    </select>
                </div>
            </div>

            <div class="controls">
                <h3 style="margin-bottom: 15px;">Расширенные параметры</h3>
                
                <div class="control-group">
                    <label>Резкость (-2 to 2) <span class="value-display" id="sharpness-val">0</span></label>
                    <input type="range" id="sharpness" min="-2" max="2" value="0" 
                           onchange="updateSetting('sharpness', this.value)">
                </div>

                <div class="control-group">
                    <label>Шумоподавление (0-8) <span class="value-display" id="denoise-val">0</span></label>
                    <input type="range" id="denoise" min="0" max="8" value="0" 
                           onchange="updateSetting('denoise', this.value)">
                </div>

                <div class="control-group">
                    <label>AE Level (-2 to 2) <span class="value-display" id="ae_level-val">0</span></label>
                    <input type="range" id="ae_level" min="-2" max="2" value="0" 
                           onchange="updateSetting('ae_level', this.value)">
                </div>

                <div class="control-group">
                    <label>AGC Gain (0-30) <span class="value-display" id="agc_gain-val">0</span></label>
                    <input type="range" id="agc_gain" min="0" max="30" value="0" 
                           onchange="updateSetting('agc_gain', this.value)">
                </div>

                <div class="control-group">
                    <label>Gain Ceiling <span class="value-display" id="gainceiling-val">0</span></label>
                    <select id="gainceiling" onchange="updateSetting('gainceiling', this.value)">
                        <option value="0" selected>2x</option>
                        <option value="1">4x</option>
                        <option value="2">8x</option>
                        <option value="3">16x</option>
                        <option value="4">32x</option>
                        <option value="5">64x</option>
                        <option value="6">128x</option>
                    </select>
                </div>

                <div class="control-group">
                    <label>
                        <input type="checkbox" id="hmirror" onchange="updateSetting('hmirror', this.checked ? 1 : 0)">
                        Горизонтальное зеркало
                    </label>
                </div>

                <div class="control-group">
                    <label>
                        <input type="checkbox" id="vflip" onchange="updateSetting('vflip', this.checked ? 1 : 0)">
                        Вертикальное отражение
                    </label>
                </div>

                <div class="control-group">
                    <label>
                        <input type="checkbox" id="awb" checked onchange="updateSetting('awb', this.checked ? 1 : 0)">
                        Auto White Balance
                    </label>
                </div>

                <div class="control-group">
                    <label>
                        <input type="checkbox" id="aec" checked onchange="updateSetting('aec', this.checked ? 1 : 0)">
                        Auto Exposure Control
                    </label>
                </div>

                <div class="control-group">
                    <label>
                        <input type="checkbox" id="agc" checked onchange="updateSetting('agc', this.checked ? 1 : 0)">
                        Auto Gain Control
                    </label>
                </div>
            </div>
        </div>
    </div>

    <script>
        function updateSetting(param, value) {
            fetch('/set?' + param + '=' + value)
                .then(response => response.text())
                .then(data => {
                    console.log('Updated ' + param + ' to ' + value);
                    // Update display value
                    var display = document.getElementById(param + '-val');
                    if (display) {
                        if (param === 'framesize') {
                            var sizes = ['', '', '', '', '', 'QVGA', 'CIF', 'VGA', 'SVGA', 'XGA', 'HD', 'SXGA', 'UXGA'];
                            display.textContent = sizes[value];
                        } else if (param === 'special_effect') {
                            var effects = ['No Effect', 'Negative', 'Grayscale', 'Red', 'Green', 'Blue', 'Sepia'];
                            display.textContent = effects[value];
                        } else if (param === 'gainceiling') {
                            display.textContent = (2 << parseInt(value)) + 'x';
                        } else {
                            display.textContent = value;
                        }
                    }
                })
                .catch(error => console.error('Error:', error));
        }

        function setPreset(preset) {
            fetch('/preset?name=' + preset)
                .then(response => response.json())
                .then(settings => {
                    console.log('Applied preset: ' + preset);
                    // Update UI to reflect new settings
                    Object.keys(settings).forEach(key => {
                        var element = document.getElementById(key);
                        if (element) {
                            if (element.type === 'checkbox') {
                                element.checked = settings[key] === 1;
                            } else {
                                element.value = settings[key];
                            }
                            // Update display
                            var display = document.getElementById(key + '-val');
                            if (display) {
                                if (key === 'framesize') {
                                    var sizes = ['', '', '', '', '', 'QVGA', 'CIF', 'VGA', 'SVGA', 'XGA', 'HD', 'SXGA', 'UXGA'];
                                    display.textContent = sizes[settings[key]];
                                } else if (key === 'special_effect') {
                                    var effects = ['No Effect', 'Negative', 'Grayscale', 'Red', 'Green', 'Blue', 'Sepia'];
                                    display.textContent = effects[settings[key]];
                                } else if (key === 'gainceiling') {
                                    display.textContent = (2 << parseInt(settings[key])) + 'x';
                                } else {
                                    display.textContent = settings[key];
                                }
                            }
                        }
                    });
                })
                .catch(error => console.error('Error:', error));
        }

        // Reload stream image periodically
        setInterval(function() {
            document.getElementById('stream').src = '/stream?' + new Date().getTime();
        }, 100);
    </script>
</body>
</html>
)rawliteral";
  return html;
}

void setupRoutes() {
  // Main page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getMainPage());
  });

  // Camera stream
  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      request->send(500, "text/plain", "Camera capture failed");
      return;
    }
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg", fb->buf, fb->len);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    esp_camera_fb_return(fb);
  });

  // Set camera parameter
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("framesize")) {
      settings.framesize = request->getParam("framesize")->value().toInt();
    }
    if (request->hasParam("quality")) {
      settings.quality = request->getParam("quality")->value().toInt();
    }
    if (request->hasParam("brightness")) {
      settings.brightness = request->getParam("brightness")->value().toInt();
    }
    if (request->hasParam("contrast")) {
      settings.contrast = request->getParam("contrast")->value().toInt();
    }
    if (request->hasParam("saturation")) {
      settings.saturation = request->getParam("saturation")->value().toInt();
    }
    if (request->hasParam("sharpness")) {
      settings.sharpness = request->getParam("sharpness")->value().toInt();
    }
    if (request->hasParam("denoise")) {
      settings.denoise = request->getParam("denoise")->value().toInt();
    }
    if (request->hasParam("special_effect")) {
      settings.special_effect = request->getParam("special_effect")->value().toInt();
    }
    if (request->hasParam("wb_mode")) {
      settings.wb_mode = request->getParam("wb_mode")->value().toInt();
    }
    if (request->hasParam("awb")) {
      settings.awb = request->getParam("awb")->value().toInt();
    }
    if (request->hasParam("awb_gain")) {
      settings.awb_gain = request->getParam("awb_gain")->value().toInt();
    }
    if (request->hasParam("aec")) {
      settings.aec = request->getParam("aec")->value().toInt();
    }
    if (request->hasParam("aec2")) {
      settings.aec2 = request->getParam("aec2")->value().toInt();
    }
    if (request->hasParam("ae_level")) {
      settings.ae_level = request->getParam("ae_level")->value().toInt();
    }
    if (request->hasParam("aec_value")) {
      settings.aec_value = request->getParam("aec_value")->value().toInt();
    }
    if (request->hasParam("agc")) {
      settings.agc = request->getParam("agc")->value().toInt();
    }
    if (request->hasParam("agc_gain")) {
      settings.agc_gain = request->getParam("agc_gain")->value().toInt();
    }
    if (request->hasParam("gainceiling")) {
      settings.gainceiling = request->getParam("gainceiling")->value().toInt();
    }
    if (request->hasParam("bpc")) {
      settings.bpc = request->getParam("bpc")->value().toInt();
    }
    if (request->hasParam("wpc")) {
      settings.wpc = request->getParam("wpc")->value().toInt();
    }
    if (request->hasParam("raw_gma")) {
      settings.raw_gma = request->getParam("raw_gma")->value().toInt();
    }
    if (request->hasParam("lenc")) {
      settings.lenc = request->getParam("lenc")->value().toInt();
    }
    if (request->hasParam("hmirror")) {
      settings.hmirror = request->getParam("hmirror")->value().toInt();
    }
    if (request->hasParam("vflip")) {
      settings.vflip = request->getParam("vflip")->value().toInt();
    }
    if (request->hasParam("dcw")) {
      settings.dcw = request->getParam("dcw")->value().toInt();
    }
    if (request->hasParam("colorbar")) {
      settings.colorbar = request->getParam("colorbar")->value().toInt();
    }
    
    applyCameraSettings();
    request->send(200, "text/plain", "OK");
  });

  // Apply preset configurations
  server.on("/preset", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
      String preset = request->getParam("name")->value();
      
      if (preset == "highQuality") {
        // High quality, slower
        settings.framesize = 7;  // VGA
        settings.quality = 5;
        settings.brightness = 0;
        settings.contrast = 1;
        settings.saturation = -2;
        settings.special_effect = 2; // Grayscale
        settings.sharpness = 1;
        settings.denoise = 2;
      } else if (preset == "balanced") {
        // Balanced settings (default)
        settings.framesize = 7;  // VGA
        settings.quality = 10;
        settings.brightness = 0;
        settings.contrast = 0;
        settings.saturation = -2;
        settings.special_effect = 2; // Grayscale
        settings.sharpness = 0;
        settings.denoise = 0;
      } else if (preset == "highSpeed") {
        // High speed, lower quality
        settings.framesize = 5;  // QVGA
        settings.quality = 20;
        settings.brightness = 0;
        settings.contrast = 0;
        settings.saturation = -2;
        settings.special_effect = 2; // Grayscale
        settings.sharpness = 0;
        settings.denoise = 0;
      } else if (preset == "indoor") {
        // Indoor lighting optimized
        settings.framesize = 7;  // VGA
        settings.quality = 10;
        settings.brightness = 1;
        settings.contrast = 1;
        settings.saturation = -2;
        settings.special_effect = 2; // Grayscale
        settings.ae_level = 1;
      } else if (preset == "outdoor") {
        // Outdoor lighting optimized
        settings.framesize = 7;  // VGA
        settings.quality = 10;
        settings.brightness = -1;
        settings.contrast = 1;
        settings.saturation = -2;
        settings.special_effect = 2; // Grayscale
        settings.ae_level = -1;
      }
      
      applyCameraSettings();
      
      // Return current settings as JSON
      String json = "{";
      json += "\"framesize\":" + String(settings.framesize) + ",";
      json += "\"quality\":" + String(settings.quality) + ",";
      json += "\"brightness\":" + String(settings.brightness) + ",";
      json += "\"contrast\":" + String(settings.contrast) + ",";
      json += "\"saturation\":" + String(settings.saturation) + ",";
      json += "\"sharpness\":" + String(settings.sharpness) + ",";
      json += "\"denoise\":" + String(settings.denoise) + ",";
      json += "\"special_effect\":" + String(settings.special_effect) + ",";
      json += "\"ae_level\":" + String(settings.ae_level) + ",";
      json += "\"agc_gain\":" + String(settings.agc_gain) + ",";
      json += "\"gainceiling\":" + String(settings.gainceiling);
      json += "}";
      
      request->send(200, "application/json", json);
    } else {
      request->send(400, "text/plain", "Missing preset name");
    }
  });
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
  
  Serial.begin(115200);
  Serial.println("\n\nESP32-CAM Line Detector Starting...");

  // Initialize camera
  initCamera();
  Serial.println("Camera initialized");

  // Start WiFi as Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Setup web server routes
  setupRoutes();
  
  // Start server
  server.begin();
  Serial.println("Web server started");
  Serial.println("Connect to WiFi: " + String(ssid));
  Serial.println("Open browser at: http://" + IP.toString());
}

void loop() {
  // Nothing to do here, server handles everything
  delay(10);
}
