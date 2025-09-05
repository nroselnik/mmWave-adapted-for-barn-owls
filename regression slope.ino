#include <Arduino.h>
#include <U8x8lib.h>
#include "Seeed_Arduino_mmWave.h"

#ifdef ESP32
#  include <HardwareSerial.h>
HardwareSerial mmWaveSerial(0);
#else
#  define mmWaveSerial Serial1
#endif

// OLED initialization: software I2C (adjust pins for your board if needed)
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(
    /* clock=*/D0, /* data=*/D10,
    /* reset=*/U8X8_PIN_NONE);

typedef enum { LABEL_BREATH, LABEL_HEART, LABEL_DISTANCE } Label;

// ===== Calibration model (standardized dataset: 5 measurements per owl, 4 owls; n = 20) =====
// Linear regression (Actual BPM) = slope * (Sensor BPM) + intercept
// Derived from paired stethoscope vs mmWave readings.
static const float HR_SLOPE     = 1.45819f;   // from regression
static const float HR_INTERCEPT = 21.95453f;  // from regression
// If you prefer a simple offset model, the mean bias on this dataset was +56.8 BPM.
// Replace the line applying the regression with: corrected = heart_rate + 56.8f;

void updateDisplay(Label label, float value) {
  static float last_breath_rate = -1.0;
  static float last_heart_rate  = -1.0;
  static float last_distance    = -1.0;

  switch (label) {
    case LABEL_BREATH:
      if (value == last_breath_rate) break;
      u8x8.setCursor(11, 3);
      u8x8.print(value, 1);
      last_breath_rate = value;
      break;

    case LABEL_HEART:
      if (value == last_heart_rate) break;
      u8x8.setCursor(11, 5);
      u8x8.print(value, 1);
      last_heart_rate = value;
      break;

    case LABEL_DISTANCE:
      if (value == last_distance) break;
      u8x8.setCursor(11, 7);
      u8x8.print(value, 1);
      last_distance = value;
      break;
  }
}

SEEED_MR60BHA2 mmWave;

void setup() {
  Serial.begin(115200);
  mmWave.begin(&mmWaveSerial);
  Serial.println("mmWave Heart Rate Monitor (Barn Owl calibration)");

  u8x8.begin();
  u8x8.setFlipMode(3);  // 180° flip if needed
  u8x8.clearDisplay();
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.setCursor(1, 0);
  u8x8.print("Vitals Monitor");

  u8x8.setCursor(0, 3); u8x8.print("BreathRate");
  u8x8.setCursor(0, 5); u8x8.print("HeartRate");
  u8x8.setCursor(0, 7); u8x8.print("Distance");
  u8x8.setFont(u8x8_font_chroma48medium8_n);
}

void loop() {
  if (mmWave.update(100)) {
    float breath_rate = NAN;
    float heart_rate  = NAN;
    float distance    = NAN;

    if (mmWave.getBreathRate(breath_rate)) {
      updateDisplay(LABEL_BREATH, breath_rate);
      Serial.printf("Breath Rate   : %.2f bpm\n", breath_rate);
    }

    if (mmWave.getHeartRate(heart_rate)) {
      // Apply regression-based correction:
      // Actual BPM = HR_SLOPE * (Sensor BPM) + HR_INTERCEPT
      float corrected_heart_rate = HR_SLOPE * heart_rate + HR_INTERCEPT;

      updateDisplay(LABEL_HEART, corrected_heart_rate);
      Serial.printf("Raw Heart Rate      : %.2f bpm\n", heart_rate);
      Serial.printf("Corrected Heart Rate: %.2f bpm\n", corrected_heart_rate);
    }

    if (mmWave.getDistance(distance)) {
      updateDisplay(LABEL_DISTANCE, distance);
      Serial.printf("Distance      : %.2f cm\n", distance);
    }

    Serial.println("----------------------------");
    delay(1000);
  }
}
