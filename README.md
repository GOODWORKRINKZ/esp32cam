# ESP32-CAM Line Detection System for Linear Robot

A comprehensive ESP32-CAM solution for line detection optimized for linear robot applications. This project provides camera configuration experiments and optimized settings for reliable line detection under various lighting conditions.

## Features

- **Optimized Camera Settings**: Pre-configured presets for different lighting conditions
- **Real-time Line Detection**: Fast and efficient line detection algorithm
- **Web Interface**: Live camera stream and settings adjustment via web browser
- **Multiple Presets**: Bright lighting, low light, and high contrast configurations
- **Adjustable Parameters**: Fine-tune camera settings for your specific environment
- **Line Position Tracking**: Accurate line position and width detection
- **RESTful API**: Easy integration with robot control systems

## Hardware Requirements

- ESP32-CAM (AI-Thinker module recommended)
- USB-to-Serial adapter (for programming)
- Power supply (5V recommended)
- Linear robot chassis (optional, for testing)

## Software Requirements

- Arduino IDE 1.8.x or 2.x
- ESP32 Board Support Package
- Required libraries (included with ESP32 package):
  - esp_camera
  - WiFi
  - esp_http_server

## Installation

### 1. Arduino IDE Setup

1. Install Arduino IDE from [arduino.cc](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Open File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Open Tools → Board → Boards Manager
   - Search for "esp32" and install "esp32 by Espressif Systems"

### 2. Upload the Code

1. Open `esp32cam_line_detection.ino` in Arduino IDE
2. Configure board settings:
   - Board: "AI Thinker ESP32-CAM"
   - Upload Speed: 115200
   - Flash Frequency: 80MHz
   - Flash Mode: QIO
   - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
3. Connect ESP32-CAM to computer via USB-to-Serial adapter:
   - ESP32-CAM GND → Adapter GND
   - ESP32-CAM 5V → Adapter 5V
   - ESP32-CAM U0R → Adapter TX
   - ESP32-CAM U0T → Adapter RX
   - ESP32-CAM GPIO0 → GND (for upload mode)
4. Select the correct COM port in Tools → Port
5. Click Upload
6. After upload, disconnect GPIO0 from GND and press reset

### 3. WiFi Configuration

By default, the ESP32-CAM creates a WiFi access point:
- SSID: `ESP32-CAM-LineBot`
- Password: `linedetect123`

To change these, edit the code:
```cpp
const char* ssid = "YourSSID";
const char* password = "YourPassword";
```

## Usage

### 1. Power On and Connect

1. Power the ESP32-CAM (5V)
2. Wait for the system to start (LED should blink)
3. Connect to the WiFi network "ESP32-CAM-LineBot"
4. Open a browser and navigate to `http://192.168.4.1`

### 2. Web Interface

The web interface provides:
- **Live Camera Stream**: Real-time video feed
- **Preset Buttons**: Quick access to optimized configurations
- **Camera Controls**: Manual adjustment of brightness, contrast, saturation
- **Line Detection Status**: Current line position, width, and confidence

### 3. Camera Presets

#### Preset 0: Bright Lighting
Optimized for outdoor or well-lit indoor environments:
- Standard brightness and contrast
- Reduced saturation for better edge detection
- Lower exposure to prevent overexposure

#### Preset 1: Low Light
Optimized for dim lighting conditions:
- Increased brightness and gain
- Maximum contrast
- Higher exposure value
- Enhanced noise reduction

#### Preset 2: High Contrast (Recommended for Line Detection)
Maximum contrast for clear line distinction:
- Maximum contrast and sharpness
- Minimum saturation (near grayscale)
- Balanced exposure
- Optimal for black lines on white background

### 4. API Endpoints

The system provides RESTful API endpoints:

- `GET /` - Web interface
- `GET /stream` - MJPEG video stream
- `GET /control?preset=<0-2>` - Load preset configuration
- `GET /control?<parameter>=<value>` - Adjust individual settings
- `GET /detect` - Get line detection status (JSON)

Example detection response:
```json
{
  "detected": true,
  "position": 45,
  "width": 25,
  "confidence": 85
}
```

## Camera Settings Guide

See [CAMERA_SETTINGS.md](CAMERA_SETTINGS.md) for detailed information about:
- Camera parameter explanations
- Optimal settings for line detection
- Lighting recommendations
- Troubleshooting tips

## Line Detection Algorithm

See [LINE_DETECTION.md](LINE_DETECTION.md) for:
- Algorithm implementation details
- Different detection methods
- Robot control integration
- Performance optimization tips

## Customization

### Adjusting WiFi Mode

To use station mode (connect to existing WiFi) instead of AP mode:

```cpp
// Replace in setup():
WiFi.begin("YourWiFiSSID", "YourWiFiPassword");
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
}
Serial.println(WiFi.localIP());
```

### Modifying Line Detection Parameters

```cpp
#define LINE_THRESHOLD 128     // Adjust for your line darkness
#define MIN_LINE_WIDTH 10      // Minimum pixels to consider as line
```

### Changing Frame Size

In `initCamera()`:
```cpp
config.frame_size = FRAMESIZE_QVGA;  // Options: QQVGA, QVGA, CIF, VGA
```

Recommendations:
- **QQVGA (160x120)**: Fastest, sufficient for basic line detection
- **QVGA (320x240)**: Good balance (default)
- **CIF (400x296)**: More detail, slower processing

## Troubleshooting

### Camera Not Initializing
- Check all pin connections
- Verify power supply is adequate (5V, >500mA)
- Try pressing the reset button
- Check serial monitor for error messages

### Cannot Connect to WiFi
- Verify SSID and password in code
- Check that WiFi is enabled on your device
- Try moving closer to ESP32-CAM
- Reset the ESP32-CAM

### Line Not Detected
- Ensure good contrast between line and background
- Adjust camera height (10-15cm recommended)
- Try different presets (especially Preset 2)
- Check lighting conditions
- Adjust LINE_THRESHOLD value

### Poor Image Quality
- Clean camera lens
- Adjust focus (if available)
- Try lower JPEG quality setting (10-12)
- Check for adequate lighting

### Web Interface Not Loading
- Verify you're connected to the correct WiFi network
- Try http://192.168.4.1 directly
- Check serial monitor for actual IP address
- Clear browser cache

## Performance Tips

1. **Frame Rate**: Adjust detection frequency in loop()
   ```cpp
   delay(100); // Process 10 times per second
   ```

2. **Processing Speed**: Use QQVGA for faster operation
   ```cpp
   s->set_framesize(s, FRAMESIZE_QQVGA);
   ```

3. **Memory Usage**: Reduce JPEG quality for less memory
   ```cpp
   s->set_quality(s, 15);
   ```

4. **Battery Life**: Reduce clock frequency
   ```cpp
   config.xclk_freq_hz = 10000000; // 10MHz instead of 20MHz
   ```

## Integration with Robot

### Reading Detection Data

Access `lastResult` structure:
```cpp
if (lastResult.lineDetected) {
    int position = lastResult.linePosition;  // 0-100
    int deviation = position - 50;           // -50 to +50
    
    // Use deviation for motor control
    leftMotor = baseSpeed + deviation;
    rightMotor = baseSpeed - deviation;
}
```

### Serial Communication

Add to loop() to send data via serial:
```cpp
Serial.printf("L:%d,P:%d,W:%d,C:%d\n", 
              lastResult.lineDetected,
              lastResult.linePosition,
              lastResult.lineWidth,
              lastResult.confidence);
```

## Project Structure

```
esp32cam/
├── esp32cam_line_detection.ino    # Main Arduino sketch
├── CAMERA_SETTINGS.md             # Camera settings documentation
├── LINE_DETECTION.md              # Line detection algorithm guide
└── README.md                      # This file
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for:
- Bug fixes
- Performance improvements
- New features
- Documentation improvements

## License

This project is open source and available for educational and commercial use.

## Acknowledgments

- ESP32-CAM community for hardware support
- Espressif Systems for ESP32 platform
- Arduino community for development tools

## References

- [ESP32-CAM Getting Started Guide](https://randomnerdtutorials.com/esp32-cam-video-streaming-face-recognition-arduino-ide/)
- [ESP32 Camera Driver Documentation](https://github.com/espressif/esp32-camera)
- [Line Following Robot Algorithms](https://www.robotshop.com/community/tutorials/show/line-following-robot-algorithm)

## Support

For questions or issues:
1. Check the [Troubleshooting](#troubleshooting) section
2. Review [CAMERA_SETTINGS.md](CAMERA_SETTINGS.md) and [LINE_DETECTION.md](LINE_DETECTION.md)
3. Open an issue on GitHub

## Version History

- **v1.0.0** - Initial release with camera settings optimization and line detection