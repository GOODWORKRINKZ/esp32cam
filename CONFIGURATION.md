# Configuration Guide

This document explains all configurable parameters in the ESP32-CAM line detection system.

## WiFi Configuration

### Access Point Mode (Default)
```cpp
const char* ssid = "ESP32-CAM-LineBot";
const char* password = "linedetect123";
```

The ESP32-CAM creates its own WiFi network. Devices connect directly to it.

**Pros:**
- No existing WiFi needed
- Direct connection
- Portable

**Cons:**
- Limited range
- No internet access
- Must switch networks

### Station Mode (Connect to Existing WiFi)
```cpp
// Replace WiFi.softAP() with:
WiFi.begin("YourWiFiSSID", "YourWiFiPassword");
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
}
Serial.println(WiFi.localIP());
```

**Pros:**
- Use existing network
- Internet access available
- Better range (depends on router)

**Cons:**
- Requires WiFi network
- Need to know network credentials

## Camera Resolution

Configured in `initCamera()`:

```cpp
config.frame_size = FRAMESIZE_QVGA;
```

### Available Options

| Frame Size | Resolution | RAM Usage | Speed | Best For |
|------------|-----------|-----------|-------|----------|
| FRAMESIZE_QQVGA | 160x120 | Low | Fast | Basic line detection, high speed |
| FRAMESIZE_QCIF | 176x144 | Low | Fast | Basic line detection |
| FRAMESIZE_HQVGA | 240x176 | Medium | Medium | Balanced |
| FRAMESIZE_QVGA | 320x240 | Medium | Medium | **Default - good balance** |
| FRAMESIZE_CIF | 400x296 | High | Slow | Detailed detection |
| FRAMESIZE_VGA | 640x480 | High | Slow | Maximum detail (if PSRAM) |

**Recommendation**: QVGA (320x240) for most applications, QQVGA for faster robot response.

## Line Detection Parameters

### Threshold Value
```cpp
#define LINE_THRESHOLD 128
```

Controls what is considered "dark" for line detection.

- **Range**: 0-255
- **Lower values** (0-100): Detect very dark lines only
- **Medium values** (100-150): Balanced detection
- **Higher values** (150-255): Detect lighter lines

**How to adjust**:
1. Capture image of your line
2. Check pixel values in Serial Monitor
3. Set threshold between line and background values

### Minimum Line Width
```cpp
#define MIN_LINE_WIDTH 10
```

Minimum width in pixels to be considered a valid line.

- **Small values** (5-10): Detect thin lines, may have false positives
- **Medium values** (10-20): Good balance
- **Large values** (20-40): Only detect wide lines, fewer false positives

**Calculation**:
```
MIN_LINE_WIDTH = (actual_line_width_cm / field_of_view_cm) * image_width_pixels
```

Example: 2cm line, 30cm FOV, 320px width:
```
MIN_LINE_WIDTH = (2 / 30) * 320 = 21 pixels
```

## Camera Settings Presets

### Preset 0: Bright Lighting
```cpp
.brightness = 0,
.contrast = 1,
.saturation = -1,
.exposure_ctrl = 1,
.aec_value = 300,
.agc_gain = 0,
```

### Preset 1: Low Light
```cpp
.brightness = 1,
.contrast = 2,
.saturation = -2,
.exposure_ctrl = 1,
.aec_value = 600,
.agc_gain = 10,
```

### Preset 2: High Contrast (Line Detection)
```cpp
.brightness = 0,
.contrast = 2,
.saturation = -2,
.exposure_ctrl = 1,
.aec_value = 400,
.agc_gain = 5,
```

### Creating Custom Preset

Add to `presets[]` array:

```cpp
CameraSettings presets[4] = {
    // ... existing presets ...
    // Custom preset 3
    {
        .brightness = 0,      // Adjust for your lighting
        .contrast = 2,        // Keep high for line detection
        .saturation = -2,     // Keep low for grayscale
        .sharpness = 2,       // Keep high for edges
        .quality = 10,
        .whitebal = 0,
        .awb_gain = 0,
        .wb_mode = 0,
        .exposure_ctrl = 1,
        .aec2 = 0,
        .ae_level = 0,        // Adjust for brightness
        .aec_value = 350,     // Adjust for lighting
        .gain_ctrl = 1,
        .agc_gain = 7,        // Adjust for sensitivity
        .gainceiling = 3,
        .bpc = 1,
        .wpc = 1,
        .raw_gma = 1,
        .lenc = 1,
        .hmirror = 0,         // Flip if needed
        .vflip = 0,           // Flip if needed
        .dcw = 1,
        .colorbar = 0
    }
};
```

## JPEG Quality

```cpp
config.jpeg_quality = 10;  // Lower = better quality
```

- **Range**: 10-63
- **Low (10-15)**: Best quality, larger files, slower
- **Medium (15-30)**: Good balance
- **High (30-63)**: Compressed, smaller files, faster

For line detection: Use 10-12 to preserve edge detail.

## Frame Buffer Count

```cpp
config.fb_count = 2;  // If PSRAM available
```

- **1**: Single buffer (slower, but works without PSRAM)
- **2**: Double buffer (smoother, requires PSRAM)

Check PSRAM availability:
```cpp
if(psramFound()){
    config.fb_count = 2;
} else {
    config.fb_count = 1;
}
```

## Loop Timing

```cpp
delay(100);  // In loop()
```

Controls how often line detection runs.

- **Fast (50ms)**: 20 FPS, responsive, higher CPU usage
- **Medium (100ms)**: 10 FPS, good balance
- **Slow (200ms)**: 5 FPS, lower CPU usage

For robot control: 50-100ms recommended.

## Web Server Port

```cpp
httpd_config_t config = HTTPD_DEFAULT_CONFIG();
config.server_port = 80;  // Default HTTP port
```

Change if needed:
```cpp
config.server_port = 8080;  // Custom port
```

Access via: `http://192.168.4.1:8080`

## Serial Baud Rate

```cpp
Serial.begin(115200);
```

Standard rates: 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600

For debugging: 115200 (good balance)
For robot control: 115200 or higher

## Clock Frequency

```cpp
config.xclk_freq_hz = 20000000;  // 20MHz
```

- **10MHz**: Lower power, slower
- **20MHz**: Standard, good balance
- **Higher**: Not recommended for stability

## Power Management

### Brownout Detector
```cpp
WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable
```

**Disabled** (0): No brownout protection, useful for testing
**Enabled** (1): Resets on low voltage (default)

Only disable if you have stable power supply.

### Camera Power Down Pin
```cpp
#define PWDN_GPIO_NUM 32
```

Set to -1 to disable power down control:
```cpp
#define PWDN_GPIO_NUM -1
```

## Motor Control Parameters (for Robot Integration)

### Base Speed
```cpp
#define BASE_SPEED 150  // Range: 0-255
```

Starting speed for both motors.

- **Low (50-100)**: Slow, precise
- **Medium (100-200)**: Good balance
- **High (200-255)**: Fast

### PID Constants
```cpp
float Kp = 2.0;   // Proportional
float Ki = 0.0;   // Integral
float Kd = 1.0;   // Derivative
```

Tuning guide:
1. Start with Kp only (Ki=0, Kd=0)
2. Increase Kp until oscillation
3. Add Kd to dampen oscillation
4. Add Ki if steady-state error exists (usually not needed)

### Speed Limits
```cpp
#define MAX_SPEED 255
#define MIN_SPEED 50
```

Prevents motors from stalling or going too fast.

## Advanced Configuration

### Image Format
```cpp
config.pixel_format = PIXFORMAT_JPEG;    // JPEG compression
config.pixel_format = PIXFORMAT_GRAYSCALE;  // Raw grayscale
config.pixel_format = PIXFORMAT_RGB565;  // Raw color
```

For line detection: GRAYSCALE is most efficient

### JPEG Subsampling
```cpp
s->set_colorbar(s, 0);  // 0=off, 1=on (test pattern)
```

### Special Effect
```cpp
s->set_special_effect(s, 0);  // 0=none, 2=grayscale
```

Effects: 0=None, 1=Negative, 2=Grayscale, 3=Red Tint, 4=Green Tint, 5=Blue Tint, 6=Sepia

## Environment-Specific Settings

### Outdoor Setting
```cpp
currentSettings.brightness = -1;    // Reduce brightness
currentSettings.aec_value = 200;    // Lower exposure
currentSettings.agc_gain = 0;       // Minimal gain
currentSettings.ae_level = -2;      // Underexpose
```

### Indoor LED Lighting
```cpp
currentSettings.brightness = 0;
currentSettings.aec_value = 400;
currentSettings.agc_gain = 5;
currentSettings.ae_level = 0;
```

### Fluorescent Lighting
```cpp
currentSettings.aec2 = 1;          // Use AEC2
currentSettings.aec_value = 500;
```

### Incandescent/Warm Light
```cpp
currentSettings.whitebal = 1;      // Enable white balance
currentSettings.wb_mode = 3;       // Office mode
```

## Debug Output Configuration

### Verbose Mode
```cpp
Serial.setDebugOutput(true);  // Enable ESP32 debug
```

### Custom Debug Output
```cpp
// Add to loop():
Serial.printf("Pos: %d, Width: %d, Conf: %d\n",
              lastResult.linePosition,
              lastResult.lineWidth,
              lastResult.confidence);
```

### JSON Output for External Processing
```cpp
Serial.printf("{\"pos\":%d,\"width\":%d,\"conf\":%d}\n",
              lastResult.linePosition,
              lastResult.lineWidth,
              lastResult.confidence);
```

## Performance Optimization

### Maximum Speed Configuration
```cpp
config.frame_size = FRAMESIZE_QQVGA;  // Smallest size
config.fb_count = 1;                   // Single buffer
config.pixel_format = PIXFORMAT_GRAYSCALE;  // No color
s->set_quality(s, 30);                 // Lower quality
delay(20);                             // 50 FPS
```

### Maximum Quality Configuration
```cpp
config.frame_size = FRAMESIZE_QVGA;   // Good detail
config.fb_count = 2;                   // Double buffer
config.pixel_format = PIXFORMAT_JPEG;
s->set_quality(s, 10);                 // Best quality
delay(100);                            // 10 FPS
```

## Testing Configuration Changes

1. Change one parameter at a time
2. Test in your actual environment
3. Document what works best
4. Create custom preset for your setup

## Configuration Template

Create `config.h` with your settings:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// WiFi Settings
#define WIFI_SSID "ESP32-CAM-LineBot"
#define WIFI_PASSWORD "linedetect123"

// Camera Settings
#define CAMERA_FRAME_SIZE FRAMESIZE_QVGA
#define CAMERA_QUALITY 10

// Line Detection
#define LINE_THRESHOLD 128
#define MIN_LINE_WIDTH 10

// Motor Control
#define BASE_SPEED 150
#define PID_KP 2.0
#define PID_KI 0.0
#define PID_KD 1.0

// Timing
#define LOOP_DELAY_MS 100

#endif
```

Then include in your sketch:
```cpp
#include "config.h"
```

## Troubleshooting Configuration

### Image Too Dark
- Increase `brightness` (0 to 2)
- Increase `aec_value` (300 to 800)
- Increase `agc_gain` (0 to 15)

### Image Too Bright
- Decrease `brightness` (0 to -2)
- Decrease `aec_value` (100 to 300)
- Decrease `ae_level` (0 to -2)

### Low Contrast
- Increase `contrast` (to 2)
- Decrease `saturation` (to -2)
- Enable `raw_gma` (1)

### Blurry Image
- Increase `sharpness` (to 2)
- Decrease `aec_value`
- Check camera focus

### False Line Detection
- Increase `MIN_LINE_WIDTH`
- Adjust `LINE_THRESHOLD`
- Enable `bpc` and `wpc`

### No Line Detection
- Decrease `LINE_THRESHOLD`
- Decrease `MIN_LINE_WIDTH`
- Check camera angle and height
- Verify line contrast
