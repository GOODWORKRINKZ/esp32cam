# Quick Start Guide

Get your ESP32-CAM line detection system running in 15 minutes!

## 🆕 Choose Your Mode

### Option A: Ultra-Fast 1-Bit Mode (Recommended for Robots)

**Best for:** High-speed robots, minimal memory, black/white lines
- **Speed:** 30+ FPS  
- **Memory:** 2.4KB per frame
- **File:** `examples/monochrome_1bit_robot.ino`

### Option B: Web Interface Mode (Recommended for Setup)

**Best for:** Calibration, testing, parameter tuning
- **Speed:** 10-15 FPS
- **Features:** Live video stream, web controls
- **File:** `src/main.cpp` (requires PlatformIO)

### Option C: Simple Grayscale Mode

**Best for:** Learning, basic detection
- **Speed:** 12-15 FPS
- **File:** `examples/simple_line_detection.ino`

---

## What You Need

- ESP32-CAM module
- USB-to-Serial adapter (FTDI or CP2102)
- 5V power supply
- 4 jumper wires
- Computer with Arduino IDE
- Black line on white background (or vice versa)

## Step 1: Install Arduino IDE (5 minutes)

1. Download Arduino IDE from https://www.arduino.cc/en/software
2. Install and open Arduino IDE
3. Go to **File → Preferences**
4. In "Additional Board Manager URLs", paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
5. Click OK
6. Go to **Tools → Board → Boards Manager**
7. Search for "esp32"
8. Install "esp32 by Espressif Systems"
9. Wait for installation to complete

## Step 2: Wire ESP32-CAM (2 minutes)

Connect your USB-to-Serial adapter to ESP32-CAM:

```
USB Adapter          ESP32-CAM
──────────           ─────────
5V        ────────►  5V
GND       ────────►  GND
TX        ────────►  U0R (GPIO3)
RX        ────────►  U0T (GPIO1)

IMPORTANT: Connect GPIO0 to GND (for programming mode)
```

## Step 3: Upload Code (5 minutes)

1. Download `esp32cam_line_detection.ino` from this repository
2. Open it in Arduino IDE
3. Configure settings:
   - **Tools → Board**: "AI Thinker ESP32-CAM"
   - **Tools → Port**: Select your USB adapter's port
   - **Tools → Upload Speed**: 115200
4. **Important**: Ensure GPIO0 is connected to GND
5. Click **Upload** button
6. Wait for "Connecting..." message
7. If it fails to connect, press RESET button on ESP32-CAM
8. Wait for "Leaving... Hard resetting via RTS pin..."
9. **Disconnect GPIO0 from GND**
10. Press RESET button

## Step 4: Connect to WiFi (2 minutes)

1. Open Serial Monitor (Tools → Serial Monitor)
2. Set baud rate to 115200
3. You should see:
   ```
   =================================
   ESP32-CAM Line Detection System
   =================================
   
   WiFi AP Started!
   AP SSID: ESP32-CAM-LineBot
   AP Password: linedetect123
   AP IP address: 192.168.4.1
   ```

4. On your phone or computer:
   - Connect to WiFi: **ESP32-CAM-LineBot**
   - Password: **linedetect123**

## Step 5: View Camera Feed (1 minute)

1. Open web browser
2. Go to: `http://192.168.4.1`
3. You should see:
   - Live camera stream
   - Control buttons
   - Line detection status

Congratulations! Your system is now running! 🎉

## Quick Test

1. Place ESP32-CAM 10-15cm above a black line on white paper
2. Tilt camera at 30-45° angle pointing downward
3. Click **High Contrast** preset button
4. Watch "Line Detection Status" section
5. Move the line left/right and observe position changes

## Troubleshooting

### Can't Upload Code
- ✓ Check GPIO0 is connected to GND during upload
- ✓ Verify TX→RX and RX→TX are correct (crossed)
- ✓ Try pressing RESET button when "Connecting..." appears
- ✓ Check USB cable and adapter

### Can't See WiFi Network
- ✓ Press RESET button after upload
- ✓ Check Serial Monitor for boot messages
- ✓ Ensure GPIO0 is DISCONNECTED from GND
- ✓ Power cycle the ESP32-CAM

### No Camera Stream
- ✓ Ensure you're connected to ESP32-CAM-LineBot WiFi
- ✓ Try http://192.168.4.1 (not https)
- ✓ Clear browser cache
- ✓ Check Serial Monitor for errors

### Line Not Detected
- ✓ Ensure good contrast (black on white or vice versa)
- ✓ Adjust camera height (10-15cm optimal)
- ✓ Try different presets
- ✓ Check lighting - avoid harsh shadows

## Next Steps

### Customize WiFi Settings

Edit in the code:
```cpp
const char* ssid = "YourWiFiName";        // Change this
const char* password = "YourPassword";     // Change this
```

### Adjust Line Detection

Edit these values based on your setup:
```cpp
#define LINE_THRESHOLD 128     // Lower = darker lines detected
#define MIN_LINE_WIDTH 10      // Minimum line width in pixels
```

### Test Different Camera Settings

Try the three presets via web interface:
- **Preset 0**: Bright lighting (outdoor/well-lit)
- **Preset 1**: Low light conditions
- **Preset 2**: High contrast (best for line detection)

### Access Detection Data

View line detection via API:
```
http://192.168.4.1/detect
```

Response:
```json
{
  "detected": true,
  "position": 45,
  "width": 25,
  "confidence": 85
}
```

## Common Camera Settings

### Too Bright
- Lower **Brightness** slider
- Decrease **Exposure** value
- Use **Preset 0** (Bright Lighting)

### Too Dark
- Raise **Brightness** slider
- Increase **Exposure** value
- Use **Preset 1** (Low Light)

### Can't See Line Clearly
- Maximize **Contrast** slider
- Minimize **Saturation** slider
- Use **Preset 2** (High Contrast)

## API Quick Reference

| Endpoint | Description | Example |
|----------|-------------|---------|
| `/` | Web interface | http://192.168.4.1/ |
| `/stream` | MJPEG stream | http://192.168.4.1/stream |
| `/control?preset=2` | Load preset | High contrast mode |
| `/detect` | Detection JSON | Current line data |

## Tips for Best Results

1. **Lighting**: Use consistent, diffused lighting
2. **Contrast**: Black line on white paper works best
3. **Height**: 10-15cm above the surface
4. **Angle**: 30-45° downward tilt
5. **Line Width**: 1-3cm width recommended
6. **Speed**: Start slow, increase as system stabilizes

## Integration Example

Read detection data in Arduino:
```cpp
if (lastResult.lineDetected) {
    int position = lastResult.linePosition;  // 0-100%
    int deviation = position - 50;           // -50 to +50
    
    // Use for motor control
    Serial.printf("Line at %d%%, deviation: %d\n", position, deviation);
}
```

## Power Options

### For Testing
- USB power bank (easiest)
- USB cable to computer
- 5V wall adapter

### For Robot
- 5V regulated from main battery
- Separate USB power bank
- Buck converter from 7-12V

**Important**: Ensure minimum 500mA current capacity!

## Performance Expectations

With default settings (QVGA resolution):
- **Frame Rate**: 5-10 FPS
- **Detection Latency**: 100-200ms
- **Detection Range**: Line within bottom 2/3 of image
- **Position Accuracy**: ±5% of image width
- **Battery Life**: 6-10 hours (2000mAh battery)

## Need More Help?

1. Check [README.md](README.md) for detailed information
2. Review [CAMERA_SETTINGS.md](CAMERA_SETTINGS.md) for camera parameters
3. Read [LINE_DETECTION.md](LINE_DETECTION.md) for algorithm details
4. See [HARDWARE_SETUP.md](HARDWARE_SETUP.md) for wiring help

## Success Checklist

- [ ] Arduino IDE installed with ESP32 support
- [ ] Code uploaded successfully
- [ ] ESP32-CAM powers on (red LED lit)
- [ ] Can connect to WiFi AP
- [ ] Web interface loads
- [ ] Camera stream visible
- [ ] Line detection working
- [ ] Position updates as line moves

If you've checked all items, you're ready to build your line-following robot! 🤖

## Quick Reference Card

```
╔══════════════════════════════════════╗
║   ESP32-CAM Line Detection          ║
╠══════════════════════════════════════╣
║ WiFi SSID: ESP32-CAM-LineBot        ║
║ Password:  linedetect123            ║
║ IP:        192.168.4.1              ║
║ Baud:      115200                   ║
╠══════════════════════════════════════╣
║ Camera Height: 10-15cm              ║
║ Camera Angle:  30-45°               ║
║ Line Width:    1-3cm                ║
║ Best Preset:   #2 (High Contrast)  ║
╠══════════════════════════════════════╣
║ Upload Mode:   GPIO0 → GND          ║
║ Run Mode:      GPIO0 disconnected   ║
╚══════════════════════════════════════╝
```

Print this and keep it handy! 📋
