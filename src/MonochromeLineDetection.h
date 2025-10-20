/*
 * Monochrome (1-bit) Line Detection Library for ESP32-CAM
 * 
 * This library provides ultra-fast line detection using 1-bit (binary) image processing.
 * Inspired by retro 1-bit cameras for maximum speed on ESP32.
 * 
 * Benefits of 1-bit processing:
 * - 8x less memory usage than grayscale
 * - Much faster processing (simple boolean operations)
 * - Suitable for high-speed line following robots
 * - Better for high-contrast black/white lines
 * 
 * Author: ESP32-CAM Line Detection Project
 * License: MIT
 */

#ifndef MONOCHROME_LINE_DETECTION_H
#define MONOCHROME_LINE_DETECTION_H

#include <Arduino.h>
#include "esp_camera.h"

// Configuration for 1-bit processing
#define MONO_THRESHOLD 128        // Threshold for binary conversion (0-255)
#define MONO_MIN_LINE_WIDTH 5     // Minimum line width in pixels for detection
#define MONO_SCAN_ROWS 8          // Number of rows to scan in ROI
#define MONO_ROI_START 0.6        // Start scanning at 60% of image height

/**
 * Structure to hold line detection results
 */
struct MonoLineResult {
    bool detected;                // True if line was found
    int position;                 // Line center position (0-100%)
    int width;                    // Line width in pixels
    int confidence;               // Detection confidence (0-100%)
    int deviation;                // Deviation from center (-50 to +50)
};

/**
 * Monochrome Line Detection Class
 * 
 * Provides methods for converting grayscale images to 1-bit binary
 * and detecting lines using fast boolean operations.
 */
class MonochromeLineDetection {
public:
    MonochromeLineDetection(uint8_t threshold = MONO_THRESHOLD);
    ~MonochromeLineDetection();
    
    /**
     * Detect line in a camera frame buffer
     * @param fb Camera frame buffer (GRAYSCALE format)
     * @return MonoLineResult with detection information
     */
    MonoLineResult detectLine(camera_fb_t* fb);
    
    /**
     * Set threshold for binary conversion
     * @param threshold Value 0-255 (pixels below this are black)
     */
    void setThreshold(uint8_t threshold);
    
    /**
     * Get current threshold value
     */
    uint8_t getThreshold() const { return _threshold; }
    
    /**
     * Set minimum line width for detection
     * @param width Minimum width in pixels
     */
    void setMinLineWidth(int width);
    
    /**
     * Get current minimum line width
     */
    int getMinLineWidth() const { return _minLineWidth; }
    
    /**
     * Convert grayscale image to 1-bit binary
     * @param grayscale Input grayscale buffer
     * @param width Image width
     * @param height Image height
     * @param output Output binary buffer (must be allocated by caller)
     * 
     * Output format: Each byte contains 8 pixels (MSB first)
     * 1 = white/bright, 0 = black/dark
     */
    void convertToBinary(const uint8_t* grayscale, int width, int height, uint8_t* output);
    
    /**
     * Detect line in binary (1-bit) image
     * Fast detection using packed binary data
     */
    MonoLineResult detectLineInBinary(const uint8_t* binary, int width, int height);
    
    /**
     * Enable/disable debug output
     */
    void setDebug(bool enable) { _debug = enable; }

private:
    uint8_t _threshold;
    int _minLineWidth;
    bool _debug;
    
    // Helper functions
    int scanRowForLine(const uint8_t* pixels, int width, int row, int height);
    int calculateConfidence(int width, int detectionCount, int totalScans);
};

#endif // MONOCHROME_LINE_DETECTION_H
