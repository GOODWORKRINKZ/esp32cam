# ESP32-CAM Hardware Setup Guide

## Overview
This guide provides detailed instructions for setting up the ESP32-CAM hardware for line detection on a linear robot.

## Components Required

### Essential Components
1. **ESP32-CAM Module** (AI-Thinker recommended)
   - Built-in camera
   - WiFi capability
   - MicroSD card slot (optional)

2. **USB-to-Serial Adapter** (FTDI or CP2102)
   - For programming the ESP32-CAM
   - 5V/3.3V compatible

3. **Power Supply**
   - 5V power source
   - Minimum 500mA current capacity
   - Recommended: 1A for stable operation

4. **Jumper Wires**
   - For connections during programming
   - Male-to-female recommended

### Optional Components
- **External LED** (for status indication)
- **Push Buttons** (for manual control)
- **Motor Driver Module** (L298N or TB6612FNG)
- **DC Motors** (for robot movement)
- **Robot Chassis** (for mounting)
- **Battery Pack** (for mobile operation)

## ESP32-CAM Pinout

### Camera Module Pins
```
                    [USB Port Side]
                    
    5V    [ ]  [ ] GND
   GPIO16 [ ]  [ ] GPIO0
   GPIO14 [ ]  [ ] GPIO15
   GPIO13 [ ]  [ ] GPIO12
   GPIO3  [ ]  [ ] GPIO1 (TX)
   GPIO2  [ ]  [ ] GPIO4
                    
                [Camera Side]
```

### Pin Functions

#### Power Pins
- **5V**: Main power input (4.5V - 5.5V)
- **GND**: Ground (multiple pins available)
- **3.3V**: Output from onboard regulator (limited current)

#### Serial Communication
- **GPIO1 (U0T)**: UART TX (connect to RX of USB adapter)
- **GPIO3 (U0R)**: UART RX (connect to TX of USB adapter)

#### GPIO Pins (Available for Robot Control)
- **GPIO0**: Boot mode select (pulled high by default)
- **GPIO2**: General purpose (can be used for LED)
- **GPIO4**: General purpose (Flash LED on some modules)
- **GPIO12**: General purpose
- **GPIO13**: General purpose
- **GPIO14**: General purpose
- **GPIO15**: General purpose
- **GPIO16**: General purpose (UART2 RX)

#### Reserved Pins (Used by Camera)
These pins are used by the camera and should not be used for other purposes:
- GPIO0 (XCLK)
- GPIO5, GPIO18, GPIO19, GPIO21 (Data pins)
- GPIO22 (PCLK)
- GPIO23 (HREF)
- GPIO25 (VSYNC)
- GPIO26, GPIO27 (I2C for camera control)
- GPIO32 (PWDN)
- GPIO34, GPIO35, GPIO36, GPIO39 (Data pins)

## Programming Setup

### Wiring for Programming

Connect USB-to-Serial adapter to ESP32-CAM:

```
USB-Serial Adapter          ESP32-CAM
─────────────────          ─────────
5V (or 3.3V)      ───────► 5V
GND               ───────► GND
TX                ───────► U0R (GPIO3)
RX                ───────► U0T (GPIO1)
```

**Important**: For upload mode, connect GPIO0 to GND before powering on.

### Programming Steps

1. **Enter Flash Mode**:
   - Connect GPIO0 to GND
   - Power on the ESP32-CAM (or press reset)
   - GPIO0 to GND makes the chip enter bootloader mode

2. **Upload Code**:
   - Select correct board and port in Arduino IDE
   - Click Upload
   - Wait for "Connecting..." message
   - Press reset button if needed

3. **Run Mode**:
   - Disconnect GPIO0 from GND
   - Press reset button or power cycle
   - Device will now run the uploaded program

### Common Programming Issues

**Problem**: "Failed to connect"
- **Solution**: Ensure GPIO0 is connected to GND before powering on
- **Solution**: Try pressing reset button during "Connecting..." message
- **Solution**: Check TX/RX are not swapped

**Problem**: "Invalid header"
- **Solution**: Select correct board (AI Thinker ESP32-CAM)
- **Solution**: Try different upload speed (115200 or 921600)

**Problem**: "Brown-out detector"
- **Solution**: Use better power supply (higher current capacity)
- **Solution**: Use shorter USB cable
- **Solution**: Add 100µF capacitor between 5V and GND

## Robot Integration

### Basic Robot Wiring

For a simple 2-wheel robot with motor driver:

```
ESP32-CAM                Motor Driver (L298N)
─────────                ─────────────────────
5V              ───────► 5V (or separate power)
GND             ───────► GND (common ground)
GPIO14          ───────► IN1 (Left motor control)
GPIO15          ───────► IN2 (Left motor control)
GPIO13          ───────► IN3 (Right motor control)
GPIO12          ───────► IN4 (Right motor control)

Motor Driver            Motors
─────────────           ──────
OUT1            ───────► Left Motor +
OUT2            ───────► Left Motor -
OUT3            ───────► Right Motor +
OUT4            ───────► Right Motor -

Power Supply
────────────
Battery Pack +  ───────► Motor Driver 12V
Battery Pack -  ───────► Motor Driver GND
                         ESP32-CAM GND (common)
```

**Important**: Use separate power for motors (7V-12V) and ESP32-CAM (5V).

### Camera Mounting

#### Height Recommendations
- **Optimal Height**: 10-15cm above the surface
- **Minimum Height**: 8cm
- **Maximum Height**: 20cm

#### Angle Recommendations
- **Optimal Angle**: 30-45° downward tilt
- **Field of View**: Should include 20-30cm ahead of robot

#### Mounting Tips
1. **Stability**: Secure mount to prevent vibration blur
2. **Adjustability**: Use adjustable mount for fine-tuning
3. **Protection**: Consider protective case for outdoor use
4. **Cable Management**: Secure wires to prevent disconnection

### Lighting Considerations

#### Indoor Setup
- Overhead lighting works best
- Avoid harsh shadows
- Use diffused light sources
- Consider ambient light consistency

#### Outdoor Setup
- Prefer cloudy days for consistent lighting
- Use shade or diffuser in bright sun
- Test at different times of day
- Consider battery life impact of high gain settings

## Power Supply Options

### USB Power
**Pros**:
- Clean power supply
- Easy for testing
- No battery needed

**Cons**:
- Tethered operation
- Not suitable for mobile robot

**Usage**: Development and testing

### Battery Power

#### Option 1: USB Power Bank
**Pros**:
- 5V output, perfect for ESP32-CAM
- Built-in voltage regulation
- Easy to recharge

**Cons**:
- May be bulky
- Some auto-shutoff with low current

**Recommended**: 5000mAh or larger

#### Option 2: LiPo Battery with Regulator
**Pros**:
- Compact
- High current capability
- Rechargeable

**Cons**:
- Requires voltage regulator (3.7V→5V)
- Need charging circuit

**Recommended**: 7.4V (2S) LiPo with 5V buck converter

#### Option 3: AA Battery Pack
**Pros**:
- Easy to replace
- Readily available
- Simple setup

**Cons**:
- Higher cost per charge
- May need voltage regulation

**Recommended**: 4x AA (6V) with LDO regulator to 5V

### Power Consumption

Typical current draw:
- **Idle (WiFi off)**: 30-50mA
- **WiFi Active**: 100-150mA
- **Camera + WiFi + Streaming**: 200-300mA
- **Peak (WiFi transmission)**: up to 500mA

Battery life estimation:
```
Battery Life (hours) = Battery Capacity (mAh) / Average Current (mA)

Example: 2000mAh battery with 250mA average
Battery Life = 2000 / 250 = 8 hours
```

## Testing the Hardware

### Step 1: Power Test
1. Connect only power (5V and GND)
2. Red LED should light up
3. Camera should be warm to touch (normal)

### Step 2: Serial Communication Test
1. Connect USB-to-Serial adapter
2. Open serial monitor (115200 baud)
3. Press reset button
4. Should see boot messages

### Step 3: Camera Test
1. Upload test sketch
2. Power on in run mode
3. Check serial monitor for camera init message
4. Connect to WiFi
5. View camera stream

### Step 4: Line Detection Test
1. Place robot over line
2. Check serial monitor for detection messages
3. Verify position accuracy
4. Test at different line positions

## Troubleshooting Hardware Issues

### Camera Not Working
**Check**:
- Camera ribbon cable is fully inserted
- Camera connector is not damaged
- No bent pins on connector

**Fix**:
- Reseat ribbon cable
- Try different camera module
- Check for firmware compatibility

### WiFi Issues
**Check**:
- Antenna is present (PCB antenna)
- No interference from metal chassis
- Power supply is adequate

**Fix**:
- Move away from metal objects
- Use external antenna (if supported)
- Ensure adequate power

### Unreliable Operation
**Check**:
- Power supply current capacity
- Wire connections are secure
- No loose connections

**Fix**:
- Use larger power supply
- Add capacitor (100µF) near ESP32-CAM
- Reseat all connections
- Check for cold solder joints

### Overheating
**Check**:
- Adequate ventilation
- Power supply voltage (should be 5V, not higher)
- Continuous operation time

**Fix**:
- Add heat sink to voltage regulator
- Reduce camera resolution
- Add breaks in operation
- Ensure proper voltage

## Safety Considerations

1. **Electrical Safety**:
   - Never exceed 5.5V on 5V pin
   - Ensure proper polarity
   - Use fuses for battery power
   - Avoid short circuits

2. **Thermal Safety**:
   - Monitor device temperature
   - Avoid covering ventilation
   - Don't operate above 85°C

3. **Mechanical Safety**:
   - Secure all moving parts
   - Protect wires from wheels
   - Use appropriate mounting hardware

## Recommended Hardware Configurations

### Configuration 1: Development Setup
- ESP32-CAM on breadboard
- USB-to-Serial adapter for programming
- USB power bank (tethered)
- Simple mount for camera testing

### Configuration 2: Basic Robot
- ESP32-CAM mounted on robot chassis
- L298N motor driver
- 2x DC motors
- 7.4V LiPo for motors + 5V regulator for ESP32
- Basic chassis with wheels

### Configuration 3: Advanced Robot
- ESP32-CAM with external antenna
- TB6612FNG motor driver (more efficient)
- 2x DC motors with encoders
- Separate power regulation
- Custom adjustable camera mount
- Status LEDs and buttons
- Protective case

## Maintenance

### Regular Checks
1. **Weekly**:
   - Clean camera lens
   - Check wire connections
   - Test camera functionality

2. **Monthly**:
   - Inspect for damage
   - Check battery condition
   - Clean dust from components

3. **As Needed**:
   - Replace damaged wires
   - Update firmware
   - Recalibrate camera settings

### Storage
- Store in cool, dry place
- Remove batteries if not used for extended period
- Protect from static electricity
- Keep in anti-static bag

## Resources

### Datasheets
- [ESP32-CAM Datasheet](https://github.com/SeeedDocument/forum_doc/raw/master/reg/ESP32_CAM_V1.6.pdf)
- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

### Tools
- Arduino IDE: https://www.arduino.cc/
- ESPTool: https://github.com/espressif/esptool
- Serial Monitor: PuTTY, CoolTerm, or Arduino Serial Monitor

### Communities
- ESP32 Forum: https://www.esp32.com/
- Arduino Forum: https://forum.arduino.cc/
- Reddit: r/esp32 and r/arduino
