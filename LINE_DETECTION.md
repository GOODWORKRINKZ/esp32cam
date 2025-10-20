# Line Detection Algorithm for ESP32-CAM

## Overview
This document describes the line detection algorithm implementation for a linear robot using ESP32-CAM.

## Basic Concept
Line detection involves identifying a continuous dark or light line against a contrasting background. For a line-following robot, this typically means detecting a black line on a white surface or vice versa.

## Algorithm Approach

### 1. Image Acquisition
- Capture frame from ESP32-CAM at 320x240 resolution (QVGA)
- Use JPEG format for efficient transmission
- Maintain consistent frame rate (5-10 FPS for line following)

### 2. Preprocessing
```
Steps:
1. Convert to grayscale (if not already)
2. Apply Gaussian blur to reduce noise (optional)
3. Apply binary threshold to separate line from background
4. Optional: Morphological operations (erosion/dilation) to clean up image
```

### 3. Line Detection Methods

#### Method A: Horizontal Line Scan (Simple)
Most efficient for ESP32-CAM with limited processing power.

**Algorithm:**
```
1. Select region of interest (bottom 1/3 of image)
2. For each row in ROI:
   - Scan pixels from left to right
   - Count consecutive dark pixels
   - If count > MIN_LINE_WIDTH, mark as potential line
3. Calculate line center position
4. Return position relative to image center
```

**Advantages:**
- Fast execution
- Low memory usage
- Suitable for real-time operation

**Best For:**
- Straight or slightly curved tracks
- Well-defined lines
- Controlled environments

#### Method B: Edge Detection (Medium Complexity)
Better for various line types and lighting conditions.

**Algorithm:**
```
1. Apply Sobel or Canny edge detection
2. Find vertical edges (line boundaries)
3. Group nearby edges
4. Identify line as region between two edges
5. Calculate line center and width
```

**Advantages:**
- More robust to lighting changes
- Can detect lines with texture
- Works with various line widths

**Best For:**
- Varying lighting conditions
- Textured or faded lines
- Complex backgrounds

#### Method C: Blob Detection (Advanced)
Most robust but requires more processing.

**Algorithm:**
```
1. Apply threshold to create binary image
2. Find connected components (blobs)
3. Filter blobs by size, aspect ratio
4. Identify blob most likely to be line
5. Calculate centroid and orientation
```

**Advantages:**
- Most robust to noise
- Can handle broken lines
- Detects line orientation

**Best For:**
- Noisy environments
- Broken or dashed lines
- Need for orientation information

### 4. Position Calculation

**Output Format:**
```c
typedef struct {
    bool lineDetected;      // Whether a line was found
    int linePosition;       // Position (0-100%, 50% = center)
    int lineWidth;          // Width in pixels
    int confidence;         // Detection confidence (0-100%)
    int orientation;        // Line angle (optional)
} LineDetectionResult;
```

**Position Calculation:**
```c
// For horizontal scan method
linePosition = (lineCenter / imageWidth) * 100;

// Deviation from center (for robot control)
deviation = linePosition - 50; // -50 to +50
```

### 5. Filtering and Validation

**Confidence Scoring:**
```
Factors:
- Line width within expected range: +30%
- Clear contrast with background: +30%
- Consistent with previous frames: +20%
- Line straightness: +20%

Total confidence = sum of factors
```

**Temporal Filtering:**
```c
// Moving average for smoother control
filteredPosition = (currentPosition * alpha) + (previousPosition * (1-alpha));
// alpha = 0.3 to 0.5 for balance between responsiveness and stability
```

## Implementation Examples

### Simple Threshold-Based Detection
```cpp
LineDetectionResult detectLineSimple(camera_fb_t* fb) {
    LineDetectionResult result = {false, 0, 0, 0};
    
    // Assume grayscale image
    uint8_t* pixels = fb->buf;
    int width = 320;
    int height = 240;
    
    // Scan middle row
    int row = height * 2 / 3; // Bottom third
    int startX = -1, endX = -1;
    
    for (int x = 0; x < width; x++) {
        int idx = row * width + x;
        if (pixels[idx] < LINE_THRESHOLD) {
            if (startX == -1) startX = x;
            endX = x;
        }
    }
    
    if (startX != -1 && (endX - startX) >= MIN_LINE_WIDTH) {
        result.lineDetected = true;
        result.lineWidth = endX - startX;
        int lineCenter = (startX + endX) / 2;
        result.linePosition = (lineCenter * 100) / width;
        result.confidence = calculateConfidence(result.lineWidth);
    }
    
    return result;
}
```

### Multi-Row Scanning for Better Accuracy
```cpp
LineDetectionResult detectLineMultiRow(camera_fb_t* fb) {
    LineDetectionResult result = {false, 0, 0, 0};
    
    int width = 320;
    int height = 240;
    uint8_t* pixels = fb->buf;
    
    int detectionCount = 0;
    int totalPosition = 0;
    int totalWidth = 0;
    
    // Scan multiple rows in bottom third
    for (int row = height * 2/3; row < height; row += 10) {
        int startX = -1, endX = -1;
        
        for (int x = 0; x < width; x++) {
            int idx = row * width + x;
            if (pixels[idx] < LINE_THRESHOLD) {
                if (startX == -1) startX = x;
                endX = x;
            }
        }
        
        if (startX != -1 && (endX - startX) >= MIN_LINE_WIDTH) {
            detectionCount++;
            int lineCenter = (startX + endX) / 2;
            totalPosition += (lineCenter * 100) / width;
            totalWidth += (endX - startX);
        }
    }
    
    if (detectionCount > 0) {
        result.lineDetected = true;
        result.linePosition = totalPosition / detectionCount;
        result.lineWidth = totalWidth / detectionCount;
        result.confidence = (detectionCount * 100) / ((height / 3) / 10);
    }
    
    return result;
}
```

## Optimization Tips for ESP32

### Memory Optimization
1. **Use streaming**: Don't store full uncompressed image
2. **Process on-the-fly**: Analyze pixels as they're decoded
3. **Subsample**: Process every Nth pixel for faster operation
4. **ROI only**: Only decode/process region of interest

### Processing Optimization
1. **Integer math**: Avoid floating-point operations
2. **Lookup tables**: Pre-calculate threshold values
3. **Early exit**: Stop scanning once line is confidently detected
4. **Frame skipping**: Process every Nth frame if CPU is overloaded

### Code Example: Optimized Scanning
```cpp
// Process only bottom region, every 2nd pixel
for (int row = height * 2/3; row < height; row += 5) {
    for (int x = 0; x < width; x += 2) {  // Skip every other pixel
        int idx = row * width + x;
        if (pixels[idx] < THRESHOLD) {
            // Line pixel detected
        }
    }
}
```

## Robot Control Integration

### PID Controller Setup
```cpp
// PID constants for line following
float Kp = 1.0;  // Proportional gain
float Ki = 0.0;  // Integral gain
float Kd = 0.5;  // Derivative gain

float previousError = 0;
float integral = 0;

// Calculate error from line position
float error = linePosition - 50;  // Deviation from center

// PID calculation
integral += error;
float derivative = error - previousError;
float control = (Kp * error) + (Ki * integral) + (Kd * derivative);

// Update motor speeds based on control value
leftMotorSpeed = baseSpeed + control;
rightMotorSpeed = baseSpeed - control;

previousError = error;
```

### State Machine for Robust Control
```cpp
enum RobotState {
    LINE_FOLLOWING,
    SEARCHING_LEFT,
    SEARCHING_RIGHT,
    STOPPED
};

RobotState currentState = LINE_FOLLOWING;

void updateRobotState(LineDetectionResult result) {
    switch (currentState) {
        case LINE_FOLLOWING:
            if (!result.lineDetected) {
                currentState = SEARCHING_LEFT;
                searchStartTime = millis();
            }
            break;
            
        case SEARCHING_LEFT:
            if (result.lineDetected) {
                currentState = LINE_FOLLOWING;
            } else if (millis() - searchStartTime > 1000) {
                currentState = SEARCHING_RIGHT;
                searchStartTime = millis();
            }
            break;
            
        case SEARCHING_RIGHT:
            if (result.lineDetected) {
                currentState = LINE_FOLLOWING;
            } else if (millis() - searchStartTime > 1000) {
                currentState = STOPPED;
            }
            break;
            
        case STOPPED:
            // Wait for manual intervention
            break;
    }
}
```

## Testing and Calibration

### Calibration Steps
1. **Threshold Calibration**: 
   - Place robot on track
   - Capture sample images
   - Analyze pixel intensities
   - Set threshold between line and background values

2. **Position Calibration**:
   - Center robot on line
   - Verify reported position is ~50%
   - Adjust position calculation if needed

3. **Width Verification**:
   - Measure actual line width
   - Compare with detected width
   - Adjust MIN_LINE_WIDTH threshold

### Test Cases
1. **Straight Line**: Verify consistent center tracking
2. **Curved Line**: Check response to gradual changes
3. **Sharp Turn**: Test maximum detection angle
4. **Line Gap**: Verify recovery from brief interruptions
5. **Lighting Change**: Test adaptation to different brightness

## Performance Metrics

Target performance for line-following robot:
- **Detection Rate**: >95% when line is in view
- **Position Accuracy**: ±5% of image width
- **Processing Time**: <100ms per frame
- **False Positive Rate**: <5%
- **Recovery Time**: <500ms after losing line

## Common Issues and Solutions

### Issue: Line not detected in bright light
**Solution**: 
- Lower exposure value
- Increase contrast setting
- Adjust threshold value

### Issue: Too many false positives
**Solution**:
- Increase MIN_LINE_WIDTH
- Add confidence threshold
- Enable pixel correction (BPC/WPC)

### Issue: Slow detection rate
**Solution**:
- Reduce resolution
- Process every Nth frame
- Simplify detection algorithm
- Use DCW (downsize) setting

### Issue: Jittery line position
**Solution**:
- Apply moving average filter
- Increase alpha in temporal filter
- Use multi-row scanning

## Advanced Features

### Line Intersection Detection
Useful for navigation markers or decision points.
```cpp
bool detectIntersection() {
    // Scan multiple rows
    // If line width suddenly increases significantly
    // Likely at an intersection or marker
    return (currentWidth > normalWidth * 1.5);
}
```

### Line Color Detection
For colored line following.
```cpp
// Instead of grayscale threshold, use color matching
bool isLineColor(uint8_t r, uint8_t g, uint8_t b) {
    // Define target color (e.g., red line)
    return (r > 150 && g < 100 && b < 100);
}
```

### Predictive Line Position
For higher speeds, predict where line will be.
```cpp
int predictedPosition = currentPosition + (velocity * lineAngle);
// Search in predicted area first
```

## Conclusion

The line detection algorithm should be chosen based on:
- Track complexity
- Lighting conditions
- Processing power available
- Required speed/accuracy tradeoff

Start with the simple horizontal scan method and increase complexity only if needed for your specific application.
