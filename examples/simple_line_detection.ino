/*
 * Simple Line Detection Example
 * 
 * A minimal example showing basic line detection without web interface.
 * Useful for integrating with robot control code.
 * 
 * This example:
 * - Initializes ESP32-CAM
 * - Detects line position
 * - Outputs position via Serial
 * - No WiFi or web interface
 */

#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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

// Line detection settings
#define LINE_THRESHOLD 128
#define MIN_LINE_WIDTH 10

// Curve detection variables
int lineCenterTop = -1;
int lineCenterMiddle = -1;
int lineCenterBottom = -1;
float curveAngle = 0.0;
bool sharpTurnDetected = false;

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    Serial.begin(115200);
    Serial.println("\nSimple Line Detection Starting...");
    
    if (!initCamera()) {
        Serial.println("Camera init failed!");
        while(1);
    }
    
    Serial.println("Camera ready! Starting line detection...");
}

void loop() {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Capture failed");
        delay(100);
        return;
    }
    
    // Detect line with multi-region scanning
    detectLineMultiRegion(fb);
    
    // Return frame buffer
    esp_camera_fb_return(fb);
    
    // Output position and curve information
    if (lineCenterBottom >= 0 || lineCenterMiddle >= 0 || lineCenterTop >= 0) {
        int mainPosition = lineCenterBottom >= 0 ? lineCenterBottom : (lineCenterMiddle >= 0 ? lineCenterMiddle : lineCenterTop);
        int positionPercent = (mainPosition * 100) / fb->width;
        int deviation = positionPercent - 50;  // -50 to +50 from center
        
        Serial.printf("Line: %d%% (dev: %+d), Angle: %.1f°", positionPercent, deviation, curveAngle);
        
        if (sharpTurnDetected) {
            Serial.print(" SHARP TURN!");
        }
        
        if (abs(curveAngle) > 5) {
            Serial.printf(" -> %s", curveAngle > 0 ? "RIGHT" : "LEFT");
        }
        
        Serial.println();
    } else {
        Serial.println("No line detected");
    }
    
    delay(100);
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
    config.pixel_format = PIXFORMAT_GRAYSCALE;  // Use grayscale for line detection
    
    if(psramFound()){
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QQVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init error: 0x%x\n", err);
        return false;
    }
    
    // Apply optimal settings for line detection
    sensor_t * s = esp_camera_sensor_get();
    s->set_brightness(s, 0);
    s->set_contrast(s, 2);
    s->set_saturation(s, -2);
    s->set_sharpness(s, 2);
    s->set_exposure_ctrl(s, 1);
    s->set_aec_value(s, 400);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 5);
    s->set_gainceiling(s, (gainceiling_t)3);
    s->set_lenc(s, 1);
    
    return true;
}

int detectLine(camera_fb_t* fb) {
    if (!fb || fb->len == 0) return -1;
    
    uint8_t* pixels = fb->buf;
    int width = fb->width;
    int height = fb->height;
    
    // Scan multiple rows in the middle third for more robust detection
    int startRow = height / 3;
    int endRow = (2 * height) / 3;
    int rowStep = 5; // Sample every 5th row for efficiency
    
    int totalDarkStart = 0;
    int totalDarkEnd = 0;
    int detectionCount = 0;
    
    // Scan multiple rows
    for (int row = startRow; row < endRow; row += rowStep) {
        int darkStart = -1;
        int darkEnd = -1;
        bool inDarkLine = false;
        
        // Find dark line in this row
        for (int x = 0; x < width; x++) {
            int idx = row * width + x;
            
            if (!inDarkLine && pixels[idx] < LINE_THRESHOLD) {
                darkStart = x;
                inDarkLine = true;
            } else if (inDarkLine && pixels[idx] >= LINE_THRESHOLD) {
                darkEnd = x - 1;
                break;
            }
        }
        
        // If still in line at end of row, set end to last pixel
        if (inDarkLine && darkEnd == -1) {
            darkEnd = width - 1;
        }
        
        // Validate line width (at least MIN_LINE_WIDTH pixels)
        if (darkStart != -1 && darkEnd != -1 && (darkEnd - darkStart) >= MIN_LINE_WIDTH) {
            totalDarkStart += darkStart;
            totalDarkEnd += darkEnd;
            detectionCount++;
        }
    }
    
    // Calculate average center from all detections
    if (detectionCount > 0) {
        int avgDarkStart = totalDarkStart / detectionCount;
        int avgDarkEnd = totalDarkEnd / detectionCount;
        int lineCenter = (avgDarkStart + avgDarkEnd) / 2;
        return (lineCenter * 100) / width;  // Convert to percentage
    }
    
    return -1;  // No line detected
}

// Enhanced multi-region line detection for curves and sharp turns
void detectLineInRegion(camera_fb_t* fb, int startRow, int endRow, int& centerX) {
    uint8_t* pixels = fb->buf;
    int width = fb->width;
    int rowStep = 3;
    
    int totalDarkStart = 0;
    int totalDarkEnd = 0;
    int detectionCount = 0;
    
    for (int row = startRow; row < endRow; row += rowStep) {
        int darkStart = -1;
        int darkEnd = -1;
        bool inDarkLine = false;
        
        for (int x = 0; x < width; x++) {
            int idx = row * width + x;
            
            if (!inDarkLine && pixels[idx] < LINE_THRESHOLD) {
                darkStart = x;
                inDarkLine = true;
            } else if (inDarkLine && pixels[idx] >= LINE_THRESHOLD) {
                darkEnd = x - 1;
                break;
            }
        }
        
        if (inDarkLine && darkEnd == -1) {
            darkEnd = width - 1;
        }
        
        if (darkStart != -1 && darkEnd != -1 && (darkEnd - darkStart) >= MIN_LINE_WIDTH) {
            totalDarkStart += darkStart;
            totalDarkEnd += darkEnd;
            detectionCount++;
        }
    }
    
    if (detectionCount > 0) {
        int avgDarkStart = totalDarkStart / detectionCount;
        int avgDarkEnd = totalDarkEnd / detectionCount;
        centerX = (avgDarkStart + avgDarkEnd) / 2;
    } else {
        centerX = -1;
    }
}

void detectLineMultiRegion(camera_fb_t* fb) {
    int height = fb->height;
    int width = fb->width;
    
    // Detect line in three regions
    int topStart = height / 6;
    int topEnd = height / 3;
    detectLineInRegion(fb, topStart, topEnd, lineCenterTop);
    
    int middleStart = height / 3;
    int middleEnd = (2 * height) / 3;
    detectLineInRegion(fb, middleStart, middleEnd, lineCenterMiddle);
    
    int bottomStart = (2 * height) / 3;
    int bottomEnd = (5 * height) / 6;
    detectLineInRegion(fb, bottomStart, bottomEnd, lineCenterBottom);
    
    // Calculate curve angle
    curveAngle = 0.0;
    sharpTurnDetected = false;
    
    if (lineCenterBottom >= 0 && lineCenterTop >= 0) {
        float displacement = lineCenterBottom - lineCenterTop;
        float verticalDistance = width * 0.4;
        curveAngle = atan(displacement / verticalDistance) * 180.0 / 3.14159;
        sharpTurnDetected = (abs(curveAngle) > 30.0);
    } else if (lineCenterBottom >= 0 && lineCenterMiddle >= 0) {
        float displacement = lineCenterBottom - lineCenterMiddle;
        float verticalDistance = width * 0.3;
        curveAngle = atan(displacement / verticalDistance) * 180.0 / 3.14159;
        sharpTurnDetected = (abs(curveAngle) > 30.0);
    }
}
