# ESP32-CAM Camera Settings Guide for Line Detection

## Overview
This document explains the optimal camera settings for line detection with ESP32-CAM on a linear robot.

## Camera Settings Parameters

### Basic Image Adjustments

#### Brightness (-2 to 2)
- **Description**: Controls the overall brightness of the image
- **For Line Detection**: 
  - Use `0` for normal lighting
  - Use `1` for low light conditions
  - Avoid high values as they can wash out line contrast

#### Contrast (-2 to 2)
- **Description**: Controls the difference between light and dark areas
- **For Line Detection**: 
  - Use `2` (maximum) for best line distinction
  - High contrast makes lines more visible against background
  - **Recommended: 2** for optimal line detection

#### Saturation (-2 to 2)
- **Description**: Controls color intensity
- **For Line Detection**:
  - Use `-2` (minimum) to reduce color information
  - Black and white images are better for line detection
  - Reduces processing complexity
  - **Recommended: -2** for grayscale-like output

#### Sharpness (-2 to 2)
- **Description**: Enhances edge definition
- **For Line Detection**:
  - Use `2` (maximum) for crisp line edges
  - Helps distinguish line boundaries
  - **Recommended: 2** for clear edges

### Quality Settings

#### JPEG Quality (10 to 63)
- **Description**: Lower values = better quality, higher values = more compression
- **For Line Detection**:
  - Use `10-12` for minimal compression artifacts
  - Better quality preserves line details
  - **Recommended: 10-12**

### White Balance

#### White Balance (0 or 1)
- **Description**: Automatic white balance adjustment
- **For Line Detection**:
  - Use `0` (off) for consistent colors under constant lighting
  - Use `1` (on) if lighting conditions vary
  - **Recommended: 0** for controlled environments

#### AWB Gain (0 or 1)
- **Description**: Automatic white balance gain
- **For Line Detection**:
  - Use `0` (off) with white balance off
  - **Recommended: 0** for consistency

#### WB Mode (0 to 4)
- **Description**: White balance mode (Auto, Sunny, Cloudy, Office, Home)
- **For Line Detection**:
  - Use `0` (Auto) if enabled
  - **Recommended: 0**

### Exposure Control

#### Exposure Control (0 or 1)
- **Description**: Automatic exposure control
- **For Line Detection**:
  - Use `1` (on) to adapt to lighting changes
  - **Recommended: 1**

#### AEC2 (0 or 1)
- **Description**: Automatic exposure control algorithm version 2
- **For Line Detection**:
  - Use `0` for bright conditions
  - Use `1` for low light
  - **Recommended: 0** for normal lighting

#### AE Level (-2 to 2)
- **Description**: Exposure compensation level
- **For Line Detection**:
  - Use `-1` to slightly underexpose
  - Prevents line from appearing washed out
  - **Recommended: -1 to 0**

#### AEC Value (0 to 1200)
- **Description**: Manual exposure value
- **For Line Detection**:
  - Use `300-400` for bright indoor
  - Use `600-800` for low light
  - **Recommended: 400** for optimal contrast

### Gain Control

#### Gain Control (0 or 1)
- **Description**: Automatic gain control
- **For Line Detection**:
  - Use `1` (on) to adjust sensitivity
  - **Recommended: 1**

#### AGC Gain (0 to 30)
- **Description**: Manual gain value
- **For Line Detection**:
  - Use `0-5` for bright conditions
  - Use `10-15` for low light
  - **Recommended: 5** for balanced sensitivity

#### Gain Ceiling (0 to 6)
- **Description**: Maximum gain limit (2x, 4x, 8x, 16x, 32x, 64x, 128x)
- **For Line Detection**:
  - Use `2-3` (8x-16x) for normal conditions
  - Use `4-5` (32x-64x) for low light
  - **Recommended: 3** for good noise/sensitivity balance

### Image Correction

#### BPC - Black Pixel Correction (0 or 1)
- **Description**: Corrects dead pixels
- **For Line Detection**:
  - Use `1` (on) to reduce noise
  - **Recommended: 1**

#### WPC - White Pixel Correction (0 or 1)
- **Description**: Corrects hot pixels
- **For Line Detection**:
  - Use `1` (on) to reduce noise
  - **Recommended: 1**

#### Raw GMA - Gamma Correction (0 or 1)
- **Description**: Applies gamma curve
- **For Line Detection**:
  - Use `1` (on) for better dynamic range
  - **Recommended: 1**

#### LENC - Lens Correction (0 or 1)
- **Description**: Corrects lens distortion
- **For Line Detection**:
  - Use `1` (on) to reduce edge distortion
  - Important for accurate line positioning
  - **Recommended: 1**

### Image Orientation

#### H-Mirror (0 or 1)
- **Description**: Horizontal flip
- **For Line Detection**:
  - Use based on camera mounting
  - **Recommended: 0** (adjust as needed)

#### V-Flip (0 or 1)
- **Description**: Vertical flip
- **For Line Detection**:
  - Use based on camera mounting
  - **Recommended: 0** (adjust as needed)

### Advanced Settings

#### DCW - Downsize Enable (0 or 1)
- **Description**: Enable downscaling
- **For Line Detection**:
  - Use `1` (on) for faster processing
  - **Recommended: 1**

#### Color Bar (0 or 1)
- **Description**: Test pattern
- **For Line Detection**:
  - Use `0` (off) for normal operation
  - Use `1` for camera testing
  - **Recommended: 0**

## Preset Configurations

### Preset 0: Bright Lighting
Best for outdoor or well-lit indoor environments.
```
brightness: 0
contrast: 1
saturation: -1
sharpness: 1
exposure: 300
gain: 0
```

### Preset 1: Low Light
Optimized for dim lighting conditions.
```
brightness: 1
contrast: 2
saturation: -2
sharpness: 2
exposure: 600
gain: 10
```

### Preset 2: High Contrast (Optimal for Line Detection)
Maximum contrast for clear line distinction.
```
brightness: 0
contrast: 2
saturation: -2
sharpness: 2
exposure: 400
gain: 5
```

## Frame Size Considerations

For line detection, recommended frame sizes:
- **QVGA (320x240)**: Best balance of speed and detail
- **QQVGA (160x120)**: Faster processing, sufficient for line detection
- **CIF (400x296)**: More detail, slower processing

## Lighting Recommendations

1. **Consistent Lighting**: Use consistent lighting to avoid frequent adjustments
2. **Diffused Light**: Avoid harsh shadows that can interfere with detection
3. **Contrast Background**: Use high-contrast line color against background
4. **Avoid Glare**: Minimize reflective surfaces that can create false lines

## Testing Your Configuration

1. Start with Preset 2 (High Contrast)
2. Adjust brightness based on your lighting
3. Fine-tune exposure if line appears too bright or dark
4. Test at different positions and angles
5. Verify consistent detection across the track

## Troubleshooting

### Line Not Detected
- Increase contrast
- Adjust exposure (try lower values)
- Check if line width is sufficient
- Verify lighting is consistent

### False Positives
- Increase threshold value
- Reduce gain to decrease noise
- Enable BPC and WPC
- Check for shadows or reflections

### Inconsistent Detection
- Enable lens correction
- Use fixed white balance
- Stabilize lighting conditions
- Reduce saturation further

## Performance Optimization

For best frame rate while maintaining quality:
1. Use QVGA resolution
2. Enable downsize (DCW)
3. Set JPEG quality to 12
4. Use frame buffer count of 2 (if PSRAM available)

## Advanced Line Detection Tips

1. **Region of Interest**: Focus on bottom center of image where line appears
2. **Adaptive Thresholding**: Adjust threshold based on average brightness
3. **Edge Detection**: Use Sobel or Canny edge detection for better line isolation
4. **Hough Transform**: For detecting straight lines at various angles
5. **Filtering**: Apply median filter to reduce noise before detection
