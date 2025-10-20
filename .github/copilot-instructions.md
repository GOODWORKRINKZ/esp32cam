# Copilot Instructions for ESP32-CAM Line Detection Project

## Project Overview

This is an ESP32-CAM based line detection system designed for line-following robots. The project provides a web interface for real-time camera parameter configuration optimized for line detection in robotics competitions.

**Target Audience**: Robotics enthusiasts, competition teams, and embedded systems developers
**Primary Language**: C++ with Arduino framework
**Documentation Language**: Russian (Русский)

## Technology Stack

### Hardware
- **Microcontroller**: ESP32-CAM (AI-Thinker module with PSRAM)
- **Camera**: OV2640 integrated camera module
- **Board Model**: ESP32 Wrover Module

### Software & Frameworks
- **Platform**: PlatformIO (espressif32 platform)
- **Framework**: Arduino for ESP32
- **Web Server**: ESPAsyncWebServer library
- **Async TCP**: AsyncTCP library
- **Build System**: PlatformIO
- **Monitor Speed**: 115200 baud
- **Upload Speed**: 921600 baud

### Key Libraries
- `esp_camera.h` - ESP32 camera driver
- `WiFi.h` - WiFi connectivity
- `ESPAsyncWebServer` - Asynchronous web server
- `AsyncTCP` - Asynchronous TCP library

## Project Structure

```
esp32cam/
├── .github/
│   └── copilot-instructions.md  # This file
├── src/
│   └── main.cpp                 # Main application code
├── examples/                    # Example code (if any)
├── platformio.ini              # PlatformIO configuration
├── esp32cam_line_detection.ino # Legacy Arduino sketch
├── README.md                   # Main documentation (Russian)
├── QUICKSTART.md              # Quick start guide
├── SETUP.md                   # Setup instructions
├── CONFIGURATION.md           # Configuration details
├── HARDWARE_SETUP.md          # Hardware setup guide
├── WIRING.md                  # Wiring diagrams
├── CAMERA_SETTINGS.md         # Camera settings reference
├── CAMERA_PARAMETERS.md       # Camera parameters documentation
└── LINE_DETECTION.md          # Line detection optimization guide
```

## Coding Guidelines

### General Principles
1. **Follow Arduino/ESP32 conventions**: Use Arduino-style function names and structure
2. **Memory efficiency**: ESP32-CAM has limited RAM; be mindful of memory usage
3. **Real-time performance**: Code should maintain smooth video streaming (target 10-30 FPS)
4. **Hardware-specific**: All code must be compatible with AI-Thinker ESP32-CAM pinout

### Code Style
- Use descriptive variable names in English
- Use camelCase for variables and functions (Arduino convention)
- Use UPPER_CASE for pin definitions and constants
- Add comments for complex camera parameter logic
- Keep functions focused and modular

### Camera Pin Definitions (AI-Thinker ESP32-CAM)
Always use these exact pin assignments:
```cpp
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
```

### WiFi Configuration
- Default AP mode: SSID `ESP32-CAM-LineDetector`, password `12345678`
- Default IP: `192.168.4.1`
- For network changes, modify the `ssid` and `password` constants

### Camera Settings for Line Detection
Optimal settings for line detection:
- **Special Effect**: Grayscale (value: 2) - mandatory for line detection
- **Saturation**: -2 (minimum for better B&W contrast)
- **Contrast**: 0-1 (helps highlight the line)
- **Frame Size**: VGA (640x480) - balance between speed and detail
- **Quality**: 10-15 - sufficient for detection, fast processing

### Performance Considerations
1. **Frame buffer management**: Use single frame buffer (`fb_count = 1`)
2. **JPEG quality**: Higher values (15-20) = faster but lower quality
3. **Resolution trade-offs**: QVGA (320x240) for speed, VGA (640x480) for accuracy
4. **Disable features**: Turn off denoise, BPC if not needed for performance

## Build and Development Workflow

### Building the Project
```bash
# Navigate to project directory
cd esp32cam

# Build
pio run

# Upload to ESP32-CAM
pio run --target upload

# Monitor serial output
pio device monitor
```

### Testing
1. **Visual verification**: Access web interface at `http://192.168.4.1`
2. **Camera feed check**: Verify video stream is working
3. **Parameter testing**: Test all preset modes (High Quality, Balanced, High Speed, Indoor, Outdoor)
4. **Line detection test**: Use black line on white background
5. **Performance monitoring**: Check FPS in serial console

### Debugging
- Use Serial Monitor at 115200 baud
- Check camera initialization errors
- Monitor WiFi connection status
- Verify web server responses

## Important Constraints

1. **No dynamic memory allocation during streaming**: Pre-allocate buffers
2. **PSRAM required**: Project uses PSRAM for frame buffers
3. **Partition scheme**: Uses `huge_app.csv` for larger application size
4. **No HTTPS**: Uses plain HTTP for web interface (performance reasons)
5. **Single client**: Web server handles one client at a time for streaming

## Common Tasks

### Adding New Camera Parameters
1. Add parameter to `CameraSettings` struct in `main.cpp`
2. Update `applyCameraSettings()` function to apply the parameter
3. Add web API endpoint in server route handlers
4. Update HTML interface to show the control

### Creating New Presets
1. Define preset values in a function (e.g., `applyHighQualityPreset()`)
2. Set all relevant camera parameters
3. Call `applyCameraSettings()` to apply
4. Add preset button in web interface

### Modifying WiFi Behavior
- For AP mode: Modify `ssid` and `password` constants
- For Station mode: Replace `WiFi.softAP()` with `WiFi.begin()`
- Update IP address handling accordingly

## Documentation Standards

- Primary documentation is in Russian
- Code comments should be in English
- Variable names in English
- User-facing strings in Russian
- Keep documentation files updated when adding features

## Security Considerations

1. **Default credentials**: Users should change default WiFi password
2. **No authentication**: Web interface has no authentication (local network only)
3. **Plain text**: Credentials stored in plain text (acceptable for AP mode)
4. **Network exposure**: Device creates open WiFi AP - document this clearly

## Testing Checklist

Before submitting code:
- [ ] Code compiles without errors or warnings
- [ ] Serial monitor shows successful initialization
- [ ] WiFi AP/Station connects successfully
- [ ] Web interface loads properly
- [ ] Video stream displays correctly
- [ ] All camera parameters respond to changes
- [ ] Preset modes work as expected
- [ ] No memory leaks during extended operation
- [ ] Performance is acceptable (check FPS)
- [ ] Documentation is updated if needed
