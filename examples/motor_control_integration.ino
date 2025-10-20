/*
 * ESP32-CAM Line Detection with Motor Control Integration
 * 
 * Complete example showing line detection integrated with motor control
 * for a 2-wheel line-following robot.
 * 
 * Hardware connections:
 * - ESP32-CAM camera module
 * - L298N Motor Driver
 *   GPIO12 -> IN1 (Left motor)
 *   GPIO13 -> IN2 (Left motor)
 *   GPIO14 -> IN3 (Right motor)
 *   GPIO15 -> IN4 (Right motor)
 */

#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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

// Line detection parameters
#define LINE_THRESHOLD    128
#define MIN_LINE_WIDTH    10

// Motor control parameters
#define BASE_SPEED        150    // Base PWM speed (0-255)
#define MAX_SPEED         255
#define MIN_SPEED         50

// PID constants
float Kp = 2.0;   // Proportional gain
float Ki = 0.0;   // Integral gain (usually 0 for line following)
float Kd = 1.0;   // Derivative gain

// PID variables
float previousError = 0;
float integral = 0;

// Robot states
enum RobotState {
    FOLLOWING,
    SEARCHING_LEFT,
    SEARCHING_RIGHT,
    STOPPED
};

RobotState state = STOPPED;
unsigned long searchStartTime = 0;

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    Serial.begin(115200);
    Serial.println("\nESP32-CAM Line Following Robot");
    Serial.println("================================");
    
    // Initialize motor pins
    pinMode(MOTOR_LEFT_FWD, OUTPUT);
    pinMode(MOTOR_LEFT_BWD, OUTPUT);
    pinMode(MOTOR_RIGHT_FWD, OUTPUT);
    pinMode(MOTOR_RIGHT_BWD, OUTPUT);
    
    // Stop motors initially
    stopMotors();
    
    // Initialize camera
    if (!initCamera()) {
        Serial.println("Camera init failed!");
        while(1);
    }
    
    Serial.println("System ready!");
    Serial.println("Place robot on line and it will start following...");
    
    delay(2000);  // Give time to place robot
    state = FOLLOWING;
}

void loop() {
    // Get camera frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Capture failed");
        stopMotors();
        delay(100);
        return;
    }
    
    // Detect line position
    int position = detectLine(fb);
    esp_camera_fb_return(fb);
    
    // Update robot behavior based on detection
    if (position >= 0) {
        // Line detected
        if (state != FOLLOWING) {
            Serial.println("Line found! Resuming following...");
            state = FOLLOWING;
        }
        
        // Calculate error (deviation from center)
        float error = position - 50.0;  // Center is at 50%
        
        // PID control
        float control = calculatePID(error);
        
        // Apply motor control
        setMotorSpeeds(control);
        
        Serial.printf("Pos: %d%%, Error: %.1f, Control: %.1f\n", 
                      position, error, control);
    } else {
        // Line lost - enter search mode
        handleLineLost();
    }
    
    delay(50);  // Adjust for desired loop rate
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
    
    if(psramFound()){
        config.frame_size = FRAMESIZE_QQVGA;  // Faster for motor control
        config.jpeg_quality = 12;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QQVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera error: 0x%x\n", err);
        return false;
    }
    
    // Optimal settings for line detection
    sensor_t * s = esp_camera_sensor_get();
    s->set_brightness(s, 0);
    s->set_contrast(s, 2);
    s->set_saturation(s, -2);
    s->set_sharpness(s, 2);
    s->set_exposure_ctrl(s, 1);
    s->set_aec_value(s, 400);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 5);
    s->set_lenc(s, 1);
    
    return true;
}

int detectLine(camera_fb_t* fb) {
    if (!fb || fb->len == 0) return -1;
    
    uint8_t* pixels = fb->buf;
    int width = fb->width;
    int height = fb->height;
    
    // Multi-row scanning for more stable detection
    int totalPosition = 0;
    int detectionCount = 0;
    
    // Scan middle third of image for better line detection
    int startRow = height / 3;
    int endRow = (2 * height) / 3;
    
    for (int row = startRow; row < endRow; row += 5) {
        int darkStart = -1;
        int darkEnd = -1;
        bool inDarkLine = false;
        
        for (int x = 0; x < width; x++) {
            int idx = row * width + x;
            
            if (!inDarkLine && pixels[idx] < LINE_THRESHOLD) {
                darkStart = x;
                inDarkLine = true;
            } else if (inDarkLine && pixels[idx] >= LINE_THRESHOLD) {
                darkEnd = x - 1;
                break;
            }
        }
        
        // If still in line at end of row
        if (inDarkLine && darkEnd == -1) {
            darkEnd = width - 1;
        }
        
        // Validate line width
        if (darkStart != -1 && darkEnd != -1 && (darkEnd - darkStart) >= MIN_LINE_WIDTH) {
            int lineCenter = (darkStart + darkEnd) / 2;
            totalPosition += (lineCenter * 100) / width;
            detectionCount++;
        }
    }
    
    if (detectionCount > 0) {
        return totalPosition / detectionCount;
    }
    
    return -1;
}

float calculatePID(float error) {
    // Proportional term
    float P = Kp * error;
    
    // Integral term (usually not needed for line following)
    integral += error;
    float I = Ki * integral;
    
    // Derivative term
    float D = Kd * (error - previousError);
    
    // Calculate control output
    float control = P + I + D;
    
    // Update previous error
    previousError = error;
    
    // Limit control output
    if (control > 100) control = 100;
    if (control < -100) control = -100;
    
    return control;
}

void setMotorSpeeds(float control) {
    // Calculate motor speeds based on control value
    int leftSpeed = BASE_SPEED + control;
    int rightSpeed = BASE_SPEED - control;
    
    // Constrain speeds to valid range
    leftSpeed = constrain(leftSpeed, MIN_SPEED, MAX_SPEED);
    rightSpeed = constrain(rightSpeed, MIN_SPEED, MAX_SPEED);
    
    // Set motor directions and speeds
    if (leftSpeed >= 0) {
        analogWrite(MOTOR_LEFT_FWD, leftSpeed);
        analogWrite(MOTOR_LEFT_BWD, 0);
    } else {
        analogWrite(MOTOR_LEFT_FWD, 0);
        analogWrite(MOTOR_LEFT_BWD, -leftSpeed);
    }
    
    if (rightSpeed >= 0) {
        analogWrite(MOTOR_RIGHT_FWD, rightSpeed);
        analogWrite(MOTOR_RIGHT_BWD, 0);
    } else {
        analogWrite(MOTOR_RIGHT_FWD, 0);
        analogWrite(MOTOR_RIGHT_BWD, -rightSpeed);
    }
}

void handleLineLost() {
    switch (state) {
        case FOLLOWING:
            Serial.println("Line lost! Searching left...");
            state = SEARCHING_LEFT;
            searchStartTime = millis();
            turnLeft();
            break;
            
        case SEARCHING_LEFT:
            if (millis() - searchStartTime > 1000) {
                Serial.println("Searching right...");
                state = SEARCHING_RIGHT;
                searchStartTime = millis();
                turnRight();
            }
            break;
            
        case SEARCHING_RIGHT:
            if (millis() - searchStartTime > 1000) {
                Serial.println("Line not found. Stopping.");
                state = STOPPED;
                stopMotors();
            }
            break;
            
        case STOPPED:
            stopMotors();
            break;
    }
}

void stopMotors() {
    analogWrite(MOTOR_LEFT_FWD, 0);
    analogWrite(MOTOR_LEFT_BWD, 0);
    analogWrite(MOTOR_RIGHT_FWD, 0);
    analogWrite(MOTOR_RIGHT_BWD, 0);
}

void turnLeft() {
    analogWrite(MOTOR_LEFT_FWD, 0);
    analogWrite(MOTOR_LEFT_BWD, BASE_SPEED / 2);
    analogWrite(MOTOR_RIGHT_FWD, BASE_SPEED / 2);
    analogWrite(MOTOR_RIGHT_BWD, 0);
}

void turnRight() {
    analogWrite(MOTOR_LEFT_FWD, BASE_SPEED / 2);
    analogWrite(MOTOR_LEFT_BWD, 0);
    analogWrite(MOTOR_RIGHT_FWD, 0);
    analogWrite(MOTOR_RIGHT_BWD, BASE_SPEED / 2);
}
