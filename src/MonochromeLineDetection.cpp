/*
 * Monochrome (1-bit) Line Detection Library Implementation
 */

#include "MonochromeLineDetection.h"

MonochromeLineDetection::MonochromeLineDetection(uint8_t threshold)
    : _threshold(threshold)
    , _minLineWidth(MONO_MIN_LINE_WIDTH)
    , _debug(false)
{
}

MonochromeLineDetection::~MonochromeLineDetection() {
}

void MonochromeLineDetection::setThreshold(uint8_t threshold) {
    _threshold = threshold;
}

void MonochromeLineDetection::setMinLineWidth(int width) {
    _minLineWidth = width;
}

MonoLineResult MonochromeLineDetection::detectLine(camera_fb_t* fb) {
    MonoLineResult result = {false, 0, 0, 0, 0};
    
    if (!fb || fb->len == 0) {
        if (_debug) Serial.println("Invalid frame buffer");
        return result;
    }
    
    // Check if format is grayscale
    if (fb->format != PIXFORMAT_GRAYSCALE && fb->format != PIXFORMAT_RGB565) {
        if (_debug) Serial.println("Unsupported pixel format");
        return result;
    }
    
    uint8_t* pixels = fb->buf;
    int width = fb->width;
    int height = fb->height;
    
    if (_debug) {
        Serial.printf("Image size: %dx%d, format: %d\n", width, height, fb->format);
    }
    
    // Multi-row scanning for stability
    int roiStart = (int)(height * MONO_ROI_START);
    int totalPosition = 0;
    int totalWidth = 0;
    int detectionCount = 0;
    int scanStep = (height - roiStart) / MONO_SCAN_ROWS;
    if (scanStep < 1) scanStep = 1;
    
    for (int row = roiStart; row < height; row += scanStep) {
        int linePos = scanRowForLine(pixels, width, row, height);
        
        if (linePos >= 0) {
            totalPosition += linePos;
            detectionCount++;
            
            // Calculate approximate line width (simplified)
            int darkPixels = 0;
            for (int x = 0; x < width; x++) {
                int idx = row * width + x;
                if (pixels[idx] < _threshold) {
                    darkPixels++;
                }
            }
            totalWidth += darkPixels;
        }
    }
    
    if (detectionCount > 0) {
        result.detected = true;
        result.position = totalPosition / detectionCount;
        result.width = totalWidth / detectionCount;
        result.confidence = calculateConfidence(result.width, detectionCount, MONO_SCAN_ROWS);
        result.deviation = result.position - 50;
        
        if (_debug) {
            Serial.printf("Line detected: pos=%d%%, width=%dpx, conf=%d%%, dev=%+d\n",
                         result.position, result.width, result.confidence, result.deviation);
        }
    } else {
        if (_debug) {
            Serial.println("No line detected");
        }
    }
    
    return result;
}

void MonochromeLineDetection::convertToBinary(const uint8_t* grayscale, int width, int height, uint8_t* output) {
    int outputSize = (width * height + 7) / 8;  // Number of bytes needed
    memset(output, 0, outputSize);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pixelIdx = y * width + x;
            uint8_t value = grayscale[pixelIdx];
            
            // Convert to binary: 1 if above threshold (white), 0 if below (black)
            if (value >= _threshold) {
                int bitIdx = pixelIdx;
                int byteIdx = bitIdx / 8;
                int bitPos = 7 - (bitIdx % 8);  // MSB first
                output[byteIdx] |= (1 << bitPos);
            }
        }
    }
}

MonoLineResult MonochromeLineDetection::detectLineInBinary(const uint8_t* binary, int width, int height) {
    MonoLineResult result = {false, 0, 0, 0, 0};
    
    int roiStart = (int)(height * MONO_ROI_START);
    int totalPosition = 0;
    int totalWidth = 0;
    int detectionCount = 0;
    int scanStep = (height - roiStart) / MONO_SCAN_ROWS;
    if (scanStep < 1) scanStep = 1;
    
    for (int row = roiStart; row < height; row += scanStep) {
        int darkStart = -1;
        int darkEnd = -1;
        int darkPixels = 0;
        
        for (int x = 0; x < width; x++) {
            int pixelIdx = row * width + x;
            int byteIdx = pixelIdx / 8;
            int bitPos = 7 - (pixelIdx % 8);
            bool isWhite = (binary[byteIdx] >> bitPos) & 1;
            
            if (!isWhite) {  // Black pixel (line)
                if (darkStart == -1) darkStart = x;
                darkEnd = x;
                darkPixels++;
            }
        }
        
        if (darkStart != -1 && (darkEnd - darkStart + 1) >= _minLineWidth) {
            int lineCenter = (darkStart + darkEnd) / 2;
            int linePosition = (lineCenter * 100) / width;
            totalPosition += linePosition;
            totalWidth += darkPixels;
            detectionCount++;
        }
    }
    
    if (detectionCount > 0) {
        result.detected = true;
        result.position = totalPosition / detectionCount;
        result.width = totalWidth / detectionCount;
        result.confidence = calculateConfidence(result.width, detectionCount, MONO_SCAN_ROWS);
        result.deviation = result.position - 50;
    }
    
    return result;
}

int MonochromeLineDetection::scanRowForLine(const uint8_t* pixels, int width, int row, int height) {
    int darkStart = -1;
    int darkEnd = -1;
    
    for (int x = 0; x < width; x++) {
        int idx = row * width + x;
        
        if (pixels[idx] < _threshold) {
            if (darkStart == -1) darkStart = x;
            darkEnd = x;
        }
    }
    
    // Check if line is wide enough
    if (darkStart != -1 && (darkEnd - darkStart + 1) >= _minLineWidth) {
        int lineCenter = (darkStart + darkEnd) / 2;
        return (lineCenter * 100) / width;  // Return position as percentage
    }
    
    return -1;  // No line found in this row
}

int MonochromeLineDetection::calculateConfidence(int width, int detectionCount, int totalScans) {
    // Confidence based on:
    // - Detection consistency across rows (0-70%)
    // - Line width within expected range (0-30%)
    
    int consistencyScore = (detectionCount * 70) / totalScans;
    
    int widthScore = 0;
    if (width >= _minLineWidth && width <= 50) {
        widthScore = 30;
    } else if (width > 50) {
        widthScore = 15;  // Too wide, might be noise
    }
    
    return consistencyScore + widthScore;
}
