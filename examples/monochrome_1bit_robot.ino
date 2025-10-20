/*
 * ESP32-CAM 1-Bit Monochrome Line Following Robot
 * 
 * Complete line-following robot implementation using ultra-fast 1-bit
 * monochrome image processing with minimal resolution.
 * 
 * Features:
 * - QQVGA resolution (160x120) for maximum speed
 * - 1-bit binary processing (retro-style)
 * - Fast PID control for smooth following
 * - Motor control with L298N driver
 * - State machine for robust operation
 * 
 * Hardware connections:
 * ESP32-CAM:
 *   - Standard AI-Thinker pinout
 * 
 * L298N Motor Driver:
 *   GPIO12 -> IN1 (Left motor forward)
 *   GPIO13 -> IN2 (Left motor backward)
 *   GPIO14 -> IN3 (Right motor forward)
 *   GPIO15 -> IN4 (Right motor backward)
 *   
 * Power:
 *   - ESP32-CAM: 5V (separate from motors)
 *   - Motors: 6-12V (through L298N)
 *   - Common ground between ESP32 and L298N
 */

#include <Arduino.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "MonochromeLineDetection.h"

// Camera pins (AI-Thinker ESP32-CAM)
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

// Motor control pins
#define MOTOR_LEFT_FWD    12
#define MOTOR_LEFT_BWD    13
#define MOTOR_RIGHT_FWD   14
#define MOTOR_RIGHT_BWD   15

// Motor control parameters
#define BASE_SPEED        180    // Base motor speed (0-255)
#define MAX_SPEED         255    // Maximum motor speed
#define MIN_SPEED         80     // Minimum motor speed
#define TURN_SPEED        120    // Speed for searching turns

// PID constants - tuned for 1-bit fast processing
#define PID_KP            3.0    // Proportional gain (higher = more responsive)
#define PID_KI            0.0    // Integral gain (usually 0 for line following)
#define PID_KD            1.5    // Derivative gain (helps with stability)

// Detection parameters
#define DETECTION_THRESHOLD  128  // Binary threshold
#define MIN_LINE_WIDTH       5    // Minimum line width in pixels
#define CONFIDENCE_MIN       40   // Minimum confidence to trust detection

// Create monochrome detector
MonochromeLineDetection detector(DETECTION_THRESHOLD);

// PID variables
float previousError = 0.0;
float integral = 0.0;

// Robot states
enum RobotState {
    STOPPED,
    CALIBRATING,
    FOLLOWING,
    SEARCHING_LEFT,
    SEARCHING_RIGHT,
    LINE_LOST
};

RobotState currentState = STOPPED;
unsigned long searchStartTime = 0;
unsigned long lastDetectionTime = 0;

// Performance tracking
unsigned long loopCount = 0;
unsigned long totalProcessTime = 0;

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    Serial.begin(115200);
    Serial.println("\n" + String('=').repeat(60));
    Serial.println("ESP32-CAM 1-BIT MONOCHROME LINE FOLLOWING ROBOT");
    Serial.println("Retro-style binary image processing for maximum speed!");
    Serial.println(String('=').repeat(60) + "\n");
    
    // Initialize motor pins
    pinMode(MOTOR_LEFT_FWD, OUTPUT);
    pinMode(MOTOR_LEFT_BWD, OUTPUT);
    pinMode(MOTOR_RIGHT_FWD, OUTPUT);
    pinMode(MOTOR_RIGHT_BWD, OUTPUT);
    
    // Ensure motors are stopped
    stopMotors();
    Serial.println("✓ Motor pins initialized");
    
    // Initialize camera
    if (!initCamera()) {
        Serial.println("❌ Camera initialization failed!");
        while(1) delay(1000);
    }
    Serial.println("✓ Camera initialized (QQVGA 160x120, 1-bit mode)");
    
    // Configure detector
    detector.setThreshold(DETECTION_THRESHOLD);
    detector.setMinLineWidth(MIN_LINE_WIDTH);
    detector.setDebug(false);
    Serial.println("✓ 1-bit monochrome detector configured");
    
    Serial.println("\nRobot Configuration:");
    Serial.println("  Base Speed: " + String(BASE_SPEED));
    Serial.println("  PID: Kp=" + String(PID_KP) + " Ki=" + String(PID_KI) + " Kd=" + String(PID_KD));
    Serial.println("  Binary Threshold: " + String(DETECTION_THRESHOLD));
    Serial.println("  Min Line Width: " + String(MIN_LINE_WIDTH) + "px");
    
    Serial.println("\n" + String('=').repeat(60));
    Serial.println("Place robot on line. Starting in 3 seconds...");
    Serial.println(String('=').repeat(60) + "\n");
    
    delay(3000);
    currentState = FOLLOWING;
    Serial.println("🚀 ROBOT STARTED - FOLLOWING LINE\n");
}

void loop() {
    unsigned long loopStart = millis();
    
    // Capture frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("⚠ Capture failed");
        stopMotors();
        delay(100);
        return;
    }
    
    // Detect line using 1-bit processing
    MonoLineResult result = detector.detectLine(fb);
    esp_camera_fb_return(fb);
    
    unsigned long processTime = millis() - loopStart;
    totalProcessTime += processTime;
    loopCount++;
    
    // Update robot state and control
    if (result.detected && result.confidence >= CONFIDENCE_MIN) {
        handleLineDetected(result, processTime);
    } else {
        handleLineNotDetected(processTime);
    }
    
    // Print stats every 50 loops
    if (loopCount % 50 == 0) {
        float avgTime = (float)totalProcessTime / loopCount;
        float avgFps = 1000.0 / avgTime;
        Serial.printf("\n📊 Stats: Avg cycle time: %.1fms | Avg FPS: %.1f | Loops: %lu\n\n",
                     avgTime, avgFps, loopCount);
    }
    
    delay(20);  // Small delay for stability
}

void handleLineDetected(MonoLineResult result, unsigned long processTime) {
    lastDetectionTime = millis();
    
    // Update state if we were searching
    if (currentState != FOLLOWING) {
        Serial.println("✓ Line reacquired! Resuming following...");
        currentState = FOLLOWING;
        previousError = 0;  // Reset PID
        integral = 0;
    }
    
    // Calculate PID control
    float error = result.deviation;  // -50 to +50
    float control = calculatePID(error);
    
    // Apply motor control
    setMotorSpeeds(control);
    
    // Print status with visual indicator
    Serial.printf("🎯 FOLLOWING | Pos: %3d%% | Dev: %+3d | Width: %2dpx | Conf: %3d%% | ",
                 result.position, result.deviation, result.width, result.confidence);
    
    // Visual position bar
    printPositionBar(result.position);
    Serial.printf(" | Ctrl: %+5.1f | Time: %2lums\n", control, processTime);
}

void handleLineNotDetected(unsigned long processTime) {
    unsigned long timeSinceDetection = millis() - lastDetectionTime;
    
    switch (currentState) {
        case FOLLOWING:
            if (timeSinceDetection > 300) {  // Lost for 300ms
                Serial.println("\n⚠ LINE LOST! Starting search pattern...");
                currentState = SEARCHING_LEFT;
                searchStartTime = millis();
                turnLeft();
            } else {
                // Keep going straight briefly
                moveForward(BASE_SPEED);
            }
            break;
            
        case SEARCHING_LEFT:
            if (millis() - searchStartTime > 800) {
                Serial.println("⚠ Searching right...");
                currentState = SEARCHING_RIGHT;
                searchStartTime = millis();
                turnRight();
            }
            break;
            
        case SEARCHING_RIGHT:
            if (millis() - searchStartTime > 1500) {
                Serial.println("❌ LINE NOT FOUND! Stopping robot.");
                currentState = LINE_LOST;
                stopMotors();
            }
            break;
            
        case LINE_LOST:
            stopMotors();
            Serial.printf("🛑 STOPPED | No line | Time: %2lums\n", processTime);
            break;
            
        default:
            stopMotors();
            break;
    }
}

float calculatePID(float error) {
    // P term
    float P = PID_KP * error;
    
    // I term (accumulate error)
    integral += error;
    // Anti-windup: limit integral
    if (integral > 50) integral = 50;
    if (integral < -50) integral = -50;
    float I = PID_KI * integral;
    
    // D term (rate of change)
    float D = PID_KD * (error - previousError);
    previousError = error;
    
    // Calculate total control
    float control = P + I + D;
    
    // Limit control output
    if (control > 100) control = 100;
    if (control < -100) control = -100;
    
    return control;
}

void setMotorSpeeds(float control) {
    // Calculate differential speeds
    int leftSpeed = BASE_SPEED + control;
    int rightSpeed = BASE_SPEED - control;
    
    // Constrain to valid range
    leftSpeed = constrain(leftSpeed, MIN_SPEED, MAX_SPEED);
    rightSpeed = constrain(rightSpeed, MIN_SPEED, MAX_SPEED);
    
    // Apply to motors
    analogWrite(MOTOR_LEFT_FWD, leftSpeed);
    analogWrite(MOTOR_LEFT_BWD, 0);
    analogWrite(MOTOR_RIGHT_FWD, rightSpeed);
    analogWrite(MOTOR_RIGHT_BWD, 0);
}

void moveForward(int speed) {
    analogWrite(MOTOR_LEFT_FWD, speed);
    analogWrite(MOTOR_LEFT_BWD, 0);
    analogWrite(MOTOR_RIGHT_FWD, speed);
    analogWrite(MOTOR_RIGHT_BWD, 0);
}

void stopMotors() {
    analogWrite(MOTOR_LEFT_FWD, 0);
    analogWrite(MOTOR_LEFT_BWD, 0);
    analogWrite(MOTOR_RIGHT_FWD, 0);
    analogWrite(MOTOR_RIGHT_BWD, 0);
}

void turnLeft() {
    analogWrite(MOTOR_LEFT_FWD, 0);
    analogWrite(MOTOR_LEFT_BWD, TURN_SPEED);
    analogWrite(MOTOR_RIGHT_FWD, TURN_SPEED);
    analogWrite(MOTOR_RIGHT_BWD, 0);
}

void turnRight() {
    analogWrite(MOTOR_LEFT_FWD, TURN_SPEED);
    analogWrite(MOTOR_LEFT_BWD, 0);
    analogWrite(MOTOR_RIGHT_FWD, 0);
    analogWrite(MOTOR_RIGHT_BWD, TURN_SPEED);
}

void printPositionBar(int position) {
    Serial.print("[");
    int barPos = position / 5;  // Scale to 0-20
    for (int i = 0; i < 20; i++) {
        if (i == barPos) Serial.print("█");
        else if (i == 10) Serial.print("|");  // Center mark
        else Serial.print("·");
    }
    Serial.print("]");
}

bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.frame_size = FRAMESIZE_QQVGA;  // 160x120 for speed
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init error: 0x%x\n", err);
        return false;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    if (!s) return false;
    
    // Optimize for 1-bit line detection
    s->set_brightness(s, 0);
    s->set_contrast(s, 2);         // Max contrast
    s->set_saturation(s, -2);
    s->set_sharpness(s, 2);        // Max sharpness
    s->set_denoise(s, 0);          // No denoise for speed
    s->set_exposure_ctrl(s, 1);
    s->set_aec_value(s, 400);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 5);
    s->set_gainceiling(s, (gainceiling_t)3);
    s->set_bpc(s, 1);
    s->set_wpc(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    
    return true;
}
