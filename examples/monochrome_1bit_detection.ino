/*
 * ESP32-CAM 1-Bit Monochrome Line Detection Example
 * 
 * This example demonstrates ultra-fast line detection using 1-bit (binary)
 * image processing with minimal resolution for maximum speed.
 * 
 * Inspired by retro 1-bit cameras - perfect for line-following robots!
 * 
 * Features:
 * - Uses smallest resolution (QQVGA 160x120) for speed
 * - 1-bit binary processing (8x less memory than grayscale)
 * - Optimized for ESP32-CAM with limited resources
 * - Fast enough for high-speed robot control
 * 
 * Hardware: AI-Thinker ESP32-CAM
 * 
 * Performance:
 * - Frame rate: 15-30 FPS
 * - Processing time: < 10ms per frame
 * - Memory usage: ~2.5KB per frame (vs 19KB for grayscale)
 */

#include <Arduino.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Include our 1-bit monochrome detection library
#include "MonochromeLineDetection.h"

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

// Create monochrome line detector instance
// Threshold 128 works well for black lines on white background
MonochromeLineDetection detector(128);

// Performance tracking
unsigned long lastFrameTime = 0;
int frameCount = 0;
float avgFps = 0;

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable brownout detector
    
    Serial.begin(115200);
    Serial.println("\n" + String('=').repeat(50));
    Serial.println("ESP32-CAM 1-Bit Monochrome Line Detection");
    Serial.println("Retro-style binary image processing for speed!");
    Serial.println(String('=').repeat(50) + "\n");
    
    // Initialize camera with minimal resolution
    if (!initCamera()) {
        Serial.println("❌ Camera initialization failed!");
        while(1) delay(1000);
    }
    
    // Configure detector
    detector.setThreshold(128);         // Adjust based on your lighting
    detector.setMinLineWidth(5);        // Minimum line width in pixels
    detector.setDebug(false);           // Set true for detailed output
    
    Serial.println("✓ Camera initialized successfully!");
    Serial.println("✓ Using QQVGA resolution (160x120) for maximum speed");
    Serial.println("✓ 1-bit processing: 8x less memory than grayscale\n");
    Serial.println("Starting line detection...\n");
    
    delay(1000);
}

void loop() {
    unsigned long frameStart = millis();
    
    // Capture frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("⚠ Camera capture failed");
        delay(100);
        return;
    }
    
    unsigned long captureTime = millis() - frameStart;
    
    // Detect line using 1-bit processing
    unsigned long detectStart = millis();
    MonoLineResult result = detector.detectLine(fb);
    unsigned long detectTime = millis() - detectStart;
    
    // Return frame buffer
    esp_camera_fb_return(fb);
    
    // Calculate FPS
    unsigned long frameTime = millis() - frameStart;
    float fps = 1000.0 / frameTime;
    avgFps = (avgFps * frameCount + fps) / (frameCount + 1);
    frameCount++;
    
    // Print results
    if (result.detected) {
        Serial.printf("🎯 LINE FOUND | Pos: %3d%% | Dev: %+3d | Width: %2dpx | Conf: %3d%% | ",
                     result.position, result.deviation, result.width, result.confidence);
        
        // Visual position indicator
        int barPos = result.position / 5;  // Scale to 0-20
        Serial.print("[");
        for (int i = 0; i < 20; i++) {
            if (i == barPos) Serial.print("█");
            else if (i == 10) Serial.print("|");  // Center mark
            else Serial.print("·");
        }
        Serial.print("]");
    } else {
        Serial.printf("⚠ NO LINE    |                                                  ");
    }
    
    // Performance info
    Serial.printf(" | Cap: %2lums | Proc: %2lums | FPS: %4.1f (avg: %4.1f)\n",
                 captureTime, detectTime, fps, avgFps);
    
    // Adaptive delay based on performance
    if (frameTime < 50) {
        delay(50 - frameTime);  // Target ~20 FPS for stability
    }
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
    
    // IMPORTANT: Use GRAYSCALE format for 1-bit processing
    // It's faster than JPEG and uses less memory
    config.pixel_format = PIXFORMAT_GRAYSCALE;
    
    // Use minimal resolution for maximum speed
    // QQVGA (160x120) is perfect for line detection
    config.frame_size = FRAMESIZE_QQVGA;  // 160x120 pixels
    config.jpeg_quality = 12;              // Not used for grayscale
    config.fb_count = 2;                   // Double buffering for smoother operation
    config.grab_mode = CAMERA_GRAB_LATEST; // Always get latest frame
    
    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }
    
    // Get camera sensor
    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        Serial.println("Failed to get camera sensor");
        return false;
    }
    
    // Apply optimal settings for 1-bit line detection
    // These settings maximize contrast for binary conversion
    s->set_brightness(s, 0);       // Neutral brightness
    s->set_contrast(s, 2);         // Maximum contrast for clear black/white
    s->set_saturation(s, -2);      // Minimum saturation (grayscale)
    s->set_sharpness(s, 2);        // Maximum sharpness for clear edges
    s->set_denoise(s, 0);          // No denoise for speed
    s->set_special_effect(s, 0);   // No special effects
    
    // Exposure and gain settings for consistent lighting
    s->set_exposure_ctrl(s, 1);    // Enable auto exposure
    s->set_aec2(s, 0);             // Disable AEC2 for faster response
    s->set_ae_level(s, 0);         // Neutral AE level
    s->set_aec_value(s, 400);      // Medium exposure value
    
    s->set_gain_ctrl(s, 1);        // Enable auto gain
    s->set_agc_gain(s, 5);         // Medium gain
    s->set_gainceiling(s, (gainceiling_t)3);  // 16x gain ceiling
    
    // Image corrections for better binary conversion
    s->set_bpc(s, 1);              // Enable black pixel correction
    s->set_wpc(s, 1);              // Enable white pixel correction
    s->set_raw_gma(s, 1);          // Enable gamma correction
    s->set_lenc(s, 1);             // Enable lens correction
    
    // Mirror/flip if needed
    s->set_hmirror(s, 0);          // No horizontal mirror
    s->set_vflip(s, 0);            // No vertical flip
    
    s->set_whitebal(s, 1);         // Enable white balance
    s->set_awb_gain(s, 1);         // Enable AWB gain
    s->set_wb_mode(s, 0);          // Auto white balance mode
    
    Serial.println("\nCamera Configuration:");
    Serial.println("  Resolution: QQVGA (160x120)");
    Serial.println("  Format: GRAYSCALE (8-bit)");
    Serial.println("  Processing: 1-bit binary");
    Serial.println("  Memory per frame: ~2.5KB (1-bit) vs ~19KB (grayscale)");
    Serial.println("  Expected FPS: 15-30");
    
    return true;
}
