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

// LED Flash pin for ESP32-CAM
#define LED_FLASH 4

// Calibration and line detection parameters
int binaryThreshold = 128; // Auto-calibrated threshold for 1-bit conversion
bool invertColors = false; // false = black line on white, true = white line on black
int lineCenterX = -1; // Detected line center X position (-1 if not detected)

// Curve and turn detection parameters
int lineCenterTop = -1;    // Line position in top region
int lineCenterMiddle = -1; // Line position in middle region
int lineCenterBottom = -1; // Line position in bottom region
float curveAngle = 0.0;    // Estimated curve angle in degrees
bool sharpTurnDetected = false; // True if sharp turn (>45°) detected
String turnDirection = "straight"; // "left", "right", or "straight"

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

// Simple camera settings for minimal grayscale capture
// FIXED: Settings with fixed/manual values to prevent auto-adjustment
struct CameraSettings {
  int framesize = 5;   // FRAMESIZE_QVGA (320x240) - minimal for fast processing
  int brightness = 0;  // -2 to 2
  int contrast = 2;    // -2 to 2, maximum contrast for line detection
  int saturation = -2; // -2 to 2, lowest for B&W
  int sharpness = 2;   // -2 to 2, maximum sharpness
  int ae_level = 0;    // -2 to 2 (not used when AEC disabled)
  int agc_gain = 5;    // 0-30, fixed manual gain value
  int gainceiling = 2; // 0-6
} settings;

void applyCameraSettings();
void calibrateCamera();
void detectLineCenter(uint8_t* grayscale_buf, size_t width, size_t height);
void detectLineCenterInRegion(uint8_t* grayscale_buf, size_t width, size_t height, int startRow, int endRow, int& centerX);
void detectCurveAndTurn(size_t width);
void convertTo1Bit(uint8_t* grayscale_buf, size_t len);

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
  camera_config.pin_sccb_sda = SIOD_GPIO_NUM;
  camera_config.pin_sccb_scl = SIOC_GPIO_NUM;
  camera_config.pin_pwdn = PWDN_GPIO_NUM;
  camera_config.pin_reset = RESET_GPIO_NUM;
  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = PIXFORMAT_GRAYSCALE; // Use grayscale for minimal processing
  camera_config.frame_size = (framesize_t)settings.framesize;
  camera_config.jpeg_quality = 12; // Not used for grayscale but needs to be set
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
  
  // FIXED: Initialize LED flash pin and keep it OFF (disabled for now)
  pinMode(LED_FLASH, OUTPUT);
  digitalWrite(LED_FLASH, LOW); // Keep LED off - not needed
}

void applyCameraSettings() {
  if (s == NULL) return;

  s->set_framesize(s, (framesize_t)settings.framesize);
  s->set_brightness(s, settings.brightness);
  s->set_contrast(s, settings.contrast);
  s->set_saturation(s, settings.saturation);
  s->set_sharpness(s, settings.sharpness);
  s->set_ae_level(s, settings.ae_level);
  s->set_agc_gain(s, settings.agc_gain);
  s->set_gainceiling(s, (gainceiling_t)settings.gainceiling);
  
  // FIXED: Disable auto-adjustments to prevent settings from changing automatically
  s->set_whitebal(s, 0);      // Disable auto white balance
  s->set_awb_gain(s, 0);      // Disable AWB gain
  s->set_exposure_ctrl(s, 0); // Disable auto exposure control - IMPORTANT FIX
  s->set_aec2(s, 0);          // Disable AEC2
  s->set_aec_value(s, 300);   // Set fixed manual exposure value
  s->set_gain_ctrl(s, 0);     // Disable auto gain control - IMPORTANT FIX
  s->set_bpc(s, 1);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  s->set_dcw(s, 1);
  s->set_colorbar(s, 0);
}

// Convert grayscale image to 1-bit (binary) using threshold
void convertTo1Bit(uint8_t* grayscale_buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (invertColors) {
      // White line on black field
      grayscale_buf[i] = (grayscale_buf[i] < binaryThreshold) ? 0 : 255;
    } else {
      // Black line on white field
      grayscale_buf[i] = (grayscale_buf[i] < binaryThreshold) ? 0 : 255;
    }
  }
}

// Auto-calibrate camera threshold by analyzing the histogram
void calibrateCamera() {
  Serial.println("Starting calibration...");
  
  // FIXED: LED flash disabled - not needed for now
  // digitalWrite(LED_FLASH, HIGH);
  // delay(100); // Let LED stabilize
  
  // Capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed during calibration");
    // digitalWrite(LED_FLASH, LOW);
    return;
  }
  
  if (fb->format != PIXFORMAT_GRAYSCALE) {
    Serial.println("Expected grayscale format");
    esp_camera_fb_return(fb);
    // digitalWrite(LED_FLASH, LOW);
    return;
  }
  
  // Build histogram
  int histogram[256] = {0};
  for (size_t i = 0; i < fb->len; i++) {
    histogram[fb->buf[i]]++;
  }
  
  // Find peaks in histogram (bimodal distribution for line and field)
  int maxPeak1 = 0, maxPeak2 = 0;
  int peak1Idx = 0, peak2Idx = 0;
  
  // Find first peak (darker values)
  for (int i = 0; i < 128; i++) {
    if (histogram[i] > maxPeak1) {
      maxPeak1 = histogram[i];
      peak1Idx = i;
    }
  }
  
  // Find second peak (brighter values)
  for (int i = 128; i < 256; i++) {
    if (histogram[i] > maxPeak2) {
      maxPeak2 = histogram[i];
      peak2Idx = i;
    }
  }
  
  // Calculate threshold as midpoint between peaks
  if (maxPeak1 > 0 && maxPeak2 > 0) {
    binaryThreshold = (peak1Idx + peak2Idx) / 2;
    
    // Determine if we have black on white or white on black
    // Count pixels near edges of frame
    int edgeSum = 0;
    int edgeCount = 0;
    int width = fb->width;
    int height = fb->height;
    
    // Sample top and bottom rows
    for (int x = 0; x < width; x++) {
      edgeSum += fb->buf[x]; // Top row
      edgeSum += fb->buf[(height-1) * width + x]; // Bottom row
      edgeCount += 2;
    }
    
    // Sample left and right columns
    for (int y = 1; y < height-1; y++) {
      edgeSum += fb->buf[y * width]; // Left column
      edgeSum += fb->buf[y * width + (width-1)]; // Right column
      edgeCount += 2;
    }
    
    int edgeAvg = edgeSum / edgeCount;
    
    // If edges are dark, field is black (invert colors)
    invertColors = (edgeAvg < binaryThreshold);
    
    Serial.printf("Calibration complete: threshold=%d, peak1=%d, peak2=%d, invertColors=%d\n", 
                  binaryThreshold, peak1Idx, peak2Idx, invertColors);
  } else {
    Serial.println("Calibration failed: could not find two peaks");
  }
  
  esp_camera_fb_return(fb);
  // digitalWrite(LED_FLASH, LOW);
}

// Detect line center in a specific region of the image
void detectLineCenterInRegion(uint8_t* grayscale_buf, size_t width, size_t height, int startRow, int endRow, int& centerX) {
  int rowStep = 3; // Sample every 3rd row for efficiency
  
  uint8_t lineColor = invertColors ? 255 : 0; // What color the line should be
  uint8_t fieldColor = invertColors ? 0 : 255; // What color the field should be
  
  int totalLeftEdge = 0, totalRightEdge = 0;
  int detectionCount = 0;
  
  // Scan multiple rows in this region
  for (int row = startRow; row < endRow; row += rowStep) {
    int leftEdge = -1, rightEdge = -1;
    bool inLine = false;
    
    // Scan from left to find line start (transition from field to line)
    for (int x = 0; x < width; x++) {
      uint8_t pixel = grayscale_buf[row * width + x];
      if (!inLine && pixel == lineColor) {
        leftEdge = x;
        inLine = true;
      } else if (inLine && pixel == fieldColor) {
        // Found end of line
        rightEdge = x - 1;
        break;
      }
    }
    
    // If we didn't find the end, scan from right
    if (inLine && rightEdge == -1) {
      for (int x = width - 1; x >= 0; x--) {
        if (grayscale_buf[row * width + x] == lineColor) {
          rightEdge = x;
          break;
        }
      }
    }
    
    // Validate detection (line should be at least 5 pixels wide)
    if (leftEdge != -1 && rightEdge != -1 && (rightEdge - leftEdge) >= 5) {
      totalLeftEdge += leftEdge;
      totalRightEdge += rightEdge;
      detectionCount++;
    }
  }
  
  // Calculate average center from all detections
  if (detectionCount > 0) {
    int avgLeftEdge = totalLeftEdge / detectionCount;
    int avgRightEdge = totalRightEdge / detectionCount;
    centerX = (avgLeftEdge + avgRightEdge) / 2;
  } else {
    centerX = -1;
  }
}

// Enhanced line detection with multi-region scanning for curves and turns
void detectLineCenter(uint8_t* grayscale_buf, size_t width, size_t height) {
  // Divide image into three regions: top (far), middle, and bottom (near)
  // This allows detection of curves by comparing line position across regions
  
  int topStart = height / 6;           // Top region: 1/6 to 1/3 of height (far ahead)
  int topEnd = height / 3;
  
  int middleStart = height / 3;        // Middle region: 1/3 to 2/3 of height
  int middleEnd = (2 * height) / 3;
  
  int bottomStart = (2 * height) / 3;  // Bottom region: 2/3 to 5/6 of height (close to robot)
  int bottomEnd = (5 * height) / 6;
  
  // Detect line in each region
  detectLineCenterInRegion(grayscale_buf, width, height, topStart, topEnd, lineCenterTop);
  detectLineCenterInRegion(grayscale_buf, width, height, middleStart, middleEnd, lineCenterMiddle);
  detectLineCenterInRegion(grayscale_buf, width, height, bottomStart, bottomEnd, lineCenterBottom);
  
  // Use bottom region as primary detection (closest to robot, most reliable)
  // Fall back to middle, then top if bottom is not detected
  if (lineCenterBottom >= 0) {
    lineCenterX = lineCenterBottom;
  } else if (lineCenterMiddle >= 0) {
    lineCenterX = lineCenterMiddle;
  } else if (lineCenterTop >= 0) {
    lineCenterX = lineCenterTop;
  } else {
    lineCenterX = -1;
  }
  
  // Detect curves and turns based on multi-region data
  detectCurveAndTurn(width);
  
  // Debug output
  if (lineCenterX >= 0) {
    Serial.printf("Line detected: center=%d (T:%d M:%d B:%d), angle=%.1f°, turn=%s\n", 
                  lineCenterX, lineCenterTop, lineCenterMiddle, lineCenterBottom,
                  curveAngle, turnDirection.c_str());
  } else {
    Serial.println("No line detected in any region");
  }
}

// Analyze line positions across regions to detect curves and calculate turn angle
void detectCurveAndTurn(size_t width) {
  // Reset detection state
  curveAngle = 0.0;
  sharpTurnDetected = false;
  turnDirection = "straight";
  
  // Need at least 2 regions detected to calculate curve
  int regionsDetected = 0;
  if (lineCenterTop >= 0) regionsDetected++;
  if (lineCenterMiddle >= 0) regionsDetected++;
  if (lineCenterBottom >= 0) regionsDetected++;
  
  if (regionsDetected < 2) {
    // Not enough data to determine curve
    return;
  }
  
  // Calculate horizontal displacement between regions
  // Positive displacement = line curves to the right
  // Negative displacement = line curves to the left
  float displacement = 0.0;
  int validComparisons = 0;
  
  // Compare bottom to middle
  if (lineCenterBottom >= 0 && lineCenterMiddle >= 0) {
    displacement += (lineCenterTop >= 0 ? lineCenterMiddle : lineCenterBottom) - lineCenterMiddle;
    validComparisons++;
  }
  
  // Compare middle to top
  if (lineCenterMiddle >= 0 && lineCenterTop >= 0) {
    displacement += lineCenterMiddle - lineCenterTop;
    validComparisons++;
  }
  
  // Compare bottom to top (for sharp turns)
  if (lineCenterBottom >= 0 && lineCenterTop >= 0) {
    displacement += (lineCenterBottom - lineCenterTop) * 0.5; // Weight this less
    validComparisons++;
  }
  
  if (validComparisons > 0) {
    displacement /= validComparisons;
    
    // Calculate approximate angle
    // Assuming vertical distance between regions is ~40% of image height
    // angle ≈ arctan(horizontal_displacement / vertical_distance)
    float verticalDistance = width * 0.4; // Approximation based on typical FOV
    curveAngle = atan(displacement / verticalDistance) * 180.0 / 3.14159;
    
    // Determine turn direction
    if (abs(displacement) < width * 0.05) {
      // Less than 5% displacement = straight
      turnDirection = "straight";
      sharpTurnDetected = false;
    } else if (displacement > 0) {
      // Line curves to the right
      turnDirection = "right";
      sharpTurnDetected = (abs(curveAngle) > 30.0); // Sharp turn if > 30°
    } else {
      // Line curves to the left
      turnDirection = "left";
      sharpTurnDetected = (abs(curveAngle) > 30.0); // Sharp turn if > 30°
    }
  }
}

String getMainPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-CAM 1-Bit Line Detector</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #222;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            background: #333;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            overflow: hidden;
        }
        .header {
            background: #111;
            color: white;
            padding: 20px;
            text-align: center;
        }
        .header h1 { font-size: 1.8em; margin-bottom: 5px; }
        .header p { opacity: 0.7; font-size: 0.9em; }
        .camera-view {
            background: #000;
            padding: 20px;
            text-align: center;
        }
        .camera-view canvas {
            max-width: 100%;
            border: 2px solid #555;
            image-rendering: pixelated;
            image-rendering: crisp-edges;
        }
        .controls {
            padding: 20px;
        }
        .control-group {
            margin: 15px 0;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        .control-group label {
            color: #fff;
            font-size: 14px;
            min-width: 120px;
        }
        .control-group input[type="range"] {
            flex: 1;
            margin: 0 15px;
        }
        .control-group .value {
            color: #4CAF50;
            font-weight: bold;
            min-width: 50px;
            text-align: right;
        }
        .status {
            margin-top: 15px;
            padding: 10px;
            background: #444;
            color: #fff;
            border-radius: 5px;
            font-family: monospace;
            font-size: 14px;
        }
        .status-item {
            margin: 5px 0;
        }
        .line-indicator {
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 5px;
        }
        .line-detected { background: #4CAF50; }
        .line-not-detected { background: #f44336; }
        button {
            background: #4CAF50;
            color: white;
            border: none;
            padding: 12px 24px;
            font-size: 16px;
            border-radius: 5px;
            cursor: pointer;
            width: 100%;
            margin: 10px 0;
            transition: background 0.3s;
        }
        button:hover {
            background: #45a049;
        }
        button:active {
            background: #3d8b40;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>⚫⚪ 1-Bit Line Detector</h1>
            <p>ESP32-CAM Binary Line Tracking</p>
        </div>
        <div class="camera-view">
            <canvas id="canvas" width="320" height="240"></canvas>
        </div>
        <div class="controls">
            <button onclick="calibrate()">🎯 КАЛИБРОВКА</button>
            <div class="control-group">
                <label>Порог (Threshold):</label>
                <input type="range" id="threshold" min="0" max="255" value="128" oninput="updateControl('threshold', this.value)">
                <span class="value" id="thresholdValue">128</span>
            </div>
            <div class="control-group">
                <label>Яркость (Brightness):</label>
                <input type="range" id="brightness" min="-2" max="2" value="0" oninput="updateControl('brightness', this.value)">
                <span class="value" id="brightnessValue">0</span>
            </div>
            <div class="control-group">
                <label>Контраст (Contrast):</label>
                <input type="range" id="contrast" min="-2" max="2" value="2" oninput="updateControl('contrast', this.value)">
                <span class="value" id="contrastValue">2</span>
            </div>
            <div class="status">
                <div class="status-item">
                    <span class="line-indicator" id="lineIndicator"></span>
                    <span id="lineStatus">Ожидание...</span>
                </div>
                <div class="status-item" id="positionStatus">Позиция: ---</div>
                <div class="status-item" id="curveStatus">Поворот: ---</div>
                <div class="status-item" id="angleStatus">Угол: ---</div>
            </div>
        </div>
    </div>

    <script>
        const canvas = document.getElementById('canvas');
        const ctx = canvas.getContext('2d');
        let updateInterval;

        function calibrate() {
            document.getElementById('lineStatus').textContent = 'Калибровка...';
            fetch('/calibrate')
                .then(response => response.text())
                .then(data => {
                    console.log('Calibration complete');
                    setTimeout(updateStatus, 1000); // Update status after calibration
                })
                .catch(error => {
                    console.error('Calibration error:', error);
                    document.getElementById('lineStatus').textContent = 'Ошибка калибровки';
                });
        }

        function updateControl(control, value) {
            document.getElementById(control + 'Value').textContent = value;
            fetch('/control?name=' + control + '&value=' + value)
                .then(response => response.text())
                .then(data => {
                    console.log(control + ' set to ' + value);
                })
                .catch(error => {
                    console.error('Control update error:', error);
                });
        }

        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    const indicator = document.getElementById('lineIndicator');
                    const lineStatus = document.getElementById('lineStatus');
                    const positionStatus = document.getElementById('positionStatus');
                    const curveStatus = document.getElementById('curveStatus');
                    const angleStatus = document.getElementById('angleStatus');
                    
                    if (data.lineDetected) {
                        indicator.className = 'line-indicator line-detected';
                        lineStatus.textContent = 'Линия обнаружена!';
                        positionStatus.textContent = 'Позиция: ' + data.lineCenterX + ' px';
                        
                        // Display turn information
                        let turnText = 'прямо';
                        if (data.turnDirection === 'left') {
                            turnText = '⬅️ влево';
                        } else if (data.turnDirection === 'right') {
                            turnText = '➡️ вправо';
                        }
                        
                        if (data.sharpTurn) {
                            turnText += ' (резкий!)';
                        }
                        
                        curveStatus.textContent = 'Поворот: ' + turnText;
                        angleStatus.textContent = 'Угол: ' + data.curveAngle + '°';
                    } else {
                        indicator.className = 'line-indicator line-not-detected';
                        lineStatus.textContent = 'Линия не обнаружена';
                        positionStatus.textContent = 'Позиция: ---';
                        curveStatus.textContent = 'Поворот: ---';
                        angleStatus.textContent = 'Угол: ---';
                    }
                    
                    // Update control values from server
                    if (data.threshold !== undefined) {
                        document.getElementById('threshold').value = data.threshold;
                        document.getElementById('thresholdValue').textContent = data.threshold;
                    }
                    if (data.brightness !== undefined) {
                        document.getElementById('brightness').value = data.brightness;
                        document.getElementById('brightnessValue').textContent = data.brightness;
                    }
                    if (data.contrast !== undefined) {
                        document.getElementById('contrast').value = data.contrast;
                        document.getElementById('contrastValue').textContent = data.contrast;
                    }
                })
                .catch(error => console.error('Status error:', error));
        }

        function updateImage() {
            fetch('/stream')
                .then(response => response.blob())
                .then(blob => {
                    const img = new Image();
                    img.onload = function() {
                        ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
                    };
                    img.src = URL.createObjectURL(blob);
                })
                .catch(error => console.error('Stream error:', error));
        }

        // Update status every 500ms
        setInterval(updateStatus, 500);
        
        // Update image every 100ms
        setInterval(updateImage, 100);
        
        // Initial update
        setTimeout(() => {
            updateStatus();
            updateImage();
        }, 500);
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

  // Camera stream - returns 1-bit processed image as JPEG
  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    // FIXED: LED flash disabled - not needed for now
    // digitalWrite(LED_FLASH, HIGH);
    // delay(10);
    
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      // digitalWrite(LED_FLASH, LOW);
      request->send(500, "text/plain", "Camera capture failed");
      return;
    }
    
    if (fb->format != PIXFORMAT_GRAYSCALE) {
      esp_camera_fb_return(fb);
      // digitalWrite(LED_FLASH, LOW);
      request->send(500, "text/plain", "Expected grayscale format");
      return;
    }
    
    // Convert to 1-bit
    convertTo1Bit(fb->buf, fb->len);
    
    // Detect line center
    detectLineCenter(fb->buf, fb->width, fb->height);
    
    // Draw detection indicators for all three regions if detected
    // Bottom region (close) - red/inverted line
    if (lineCenterBottom >= 0 && lineCenterBottom < fb->width) {
      int bottomRow = (2 * fb->height) / 3;
      for (int y = bottomRow; y < (5 * fb->height) / 6; y++) {
        for (int dx = -2; dx <= 2; dx++) {
          int x = lineCenterBottom + dx;
          if (x >= 0 && x < fb->width) {
            int idx = y * fb->width + x;
            fb->buf[idx] = (fb->buf[idx] == 0) ? 255 : 0;
          }
        }
      }
    }
    
    // Middle region - red/inverted line
    if (lineCenterMiddle >= 0 && lineCenterMiddle < fb->width) {
      int middleRow = fb->height / 2;
      for (int y = fb->height / 3; y < (2 * fb->height) / 3; y++) {
        for (int dx = -1; dx <= 1; dx++) {
          int x = lineCenterMiddle + dx;
          if (x >= 0 && x < fb->width) {
            int idx = y * fb->width + x;
            fb->buf[idx] = (fb->buf[idx] == 0) ? 255 : 0;
          }
        }
      }
    }
    
    // Top region (far ahead) - red/inverted line
    if (lineCenterTop >= 0 && lineCenterTop < fb->width) {
      for (int y = fb->height / 6; y < fb->height / 3; y++) {
        for (int dx = -1; dx <= 1; dx++) {
          int x = lineCenterTop + dx;
          if (x >= 0 && x < fb->width) {
            int idx = y * fb->width + x;
            fb->buf[idx] = (fb->buf[idx] == 0) ? 255 : 0;
          }
        }
      }
    }
    
    // Draw connecting line between detected regions to visualize curve
    if (lineCenterBottom >= 0 && lineCenterTop >= 0) {
      // Simple line drawing between bottom and top
      int startY = (5 * fb->height) / 6;
      int endY = fb->height / 6;
      int startX = lineCenterBottom;
      int endX = lineCenterTop;
      
      for (int y = endY; y < startY; y += 3) {
        float t = (float)(y - endY) / (startY - endY);
        int x = startX + (int)(t * (endX - startX));
        if (x >= 0 && x < fb->width && y >= 0 && y < fb->height) {
          int idx = y * fb->width + x;
          fb->buf[idx] = (fb->buf[idx] == 0) ? 255 : 0;
        }
      }
    }
    
    // Convert 1-bit grayscale to JPEG for transmission
    uint8_t * out_jpg = NULL;
    size_t out_jpg_len = 0;
    if (frame2jpg(fb, 80, &out_jpg, &out_jpg_len)) {
      AsyncWebServerResponse *response = request->beginResponse(200, "image/jpeg", out_jpg, out_jpg_len);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
      free(out_jpg);
    } else {
      request->send(500, "text/plain", "JPEG conversion failed");
    }
    
    esp_camera_fb_return(fb);
    // digitalWrite(LED_FLASH, LOW);
  });

  // Control endpoint - handles slider updates
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("name") && request->hasParam("value")) {
      String name = request->getParam("name")->value();
      int value = request->getParam("value")->value().toInt();
      
      if (name == "threshold") {
        binaryThreshold = constrain(value, 0, 255);
        Serial.printf("Threshold updated to: %d\n", binaryThreshold);
      } else if (name == "brightness") {
        settings.brightness = constrain(value, -2, 2);
        applyCameraSettings();
        Serial.printf("Brightness updated to: %d\n", settings.brightness);
      } else if (name == "contrast") {
        settings.contrast = constrain(value, -2, 2);
        applyCameraSettings();
        Serial.printf("Contrast updated to: %d\n", settings.contrast);
      }
      
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });

  // Calibration endpoint
  server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
    calibrateCamera();
    request->send(200, "text/plain", "Calibration complete");
  });
  
  // Status endpoint - returns current detection status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"threshold\":" + String(binaryThreshold) + ",";
    json += "\"brightness\":" + String(settings.brightness) + ",";
    json += "\"contrast\":" + String(settings.contrast) + ",";
    json += "\"invertColors\":" + String(invertColors ? "true" : "false") + ",";
    json += "\"lineDetected\":" + String(lineCenterX >= 0 ? "true" : "false") + ",";
    json += "\"lineCenterX\":" + String(lineCenterX) + ",";
    json += "\"lineCenterTop\":" + String(lineCenterTop) + ",";
    json += "\"lineCenterMiddle\":" + String(lineCenterMiddle) + ",";
    json += "\"lineCenterBottom\":" + String(lineCenterBottom) + ",";
    json += "\"curveAngle\":" + String(curveAngle, 1) + ",";
    json += "\"sharpTurn\":" + String(sharpTurnDetected ? "true" : "false") + ",";
    json += "\"turnDirection\":\"" + turnDirection + "\"";
    json += "}";
    request->send(200, "application/json", json);
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
