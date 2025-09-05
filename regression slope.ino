#include <Arduino.h>
#include <U8x8lib.h>
#include "Seeed_Arduino_mmWave.h"

#ifdef ESP32
#  include <HardwareSerial.h>
HardwareSerial mmWaveSerial(0);
#else
#  define mmWaveSerial Serial1
#endif

// OLED initialization: using software I2C for Seeed Studio XIAO
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(
    /* clock=*/D0, /* data=*/D10,
    /* reset=*/U8X8_PIN_NONE);  // OLEDs without Reset of the Display

typedef enum { LABEL_BREATH, LABEL_HEART, LABEL_DISTANCE } Label;

void updateDisplay(Label label, float value) {
  static float last_breath_rate = -1.0;
  static float last_heart_rate  = -1.0;
  static float last_distance    = -1.0;

  switch (label) {
    case LABEL_BREATH:
      if (value == last_breath_rate) break;
      u8x8.setCursor(11, 3);
      u8x8.print(value);
      last_breath_rate = value;
      break;

    case LABEL_HEART:
      if (value == last_heart_rate) break;
      u8x8.setCursor(11, 5);
      u8x8.print(value);
      last_heart_rate = value;
      break;

    case LABEL_DISTANCE:
      if (value == last_distance) break;
      u8x8.setCursor(11, 7);
      u8x8.print(value);
      last_distance = value;
      break;

    default:
      break;
  }
}

SEEED_MR60BHA2 mmWave;

static const char* TAG_Breath   = "BreathRate";
static const char* TAG_Heart    = "HeartRate";
static const char* TAG_Distance = "Distance";

void setup() {
  Serial.begin(115200);
  mmWave.begin(&mmWaveSerial);
  Serial.println("mmWave Heart Rate Monitor Initialized");

  u8x8.begin();
  u8x8.setFlipMode(3);  // Flip screen 180 degrees if needed
  u8x8.clearDisplay();
  u8x8.setFont(u8x8_font_victoriamedium8_r);

  u8x8.setCursor(1, 0);
  u8x8.print("Vitals Monitor");

  u8x8.setCursor(0, 3);
  u8x8.print(TAG_Breath);

  u8x8.setCursor(0, 5);
  u8x8.print(TAG_Heart);

  u8x8.setCursor(0, 7);
  u8x8.print(TAG_Distance);

  u8x8.setFont(u8x8_font_chroma48medium8_n);
}

void loop() {
  if (mmWave.update(100)) {
    float breath_rate = 0;
    float heart_rate  = 0;
    float distance    = 0;

    if (mmWave.getBreathRate(breath_rate)) {
      updateDisplay(LABEL_BREATH, breath_rate);
      Serial.printf("Breath Rate   : %.2f\n", breath_rate);
    }

    if (mmWave.getHeartRate(heart_rate)) {
      // Apply regression-based correction
      // Correction model derived from linear regression on 20 paired readings (4 barn owls):
      // Actual Heart Rate (BPM) = 1.38 × Sensor Output + 20.78
      float corrected_heart_rate = (1.38 * heart_rate) + 20.78;

      updateDisplay(LABEL_HEART, corrected_heart_rate);
      Serial.printf("Raw Heart Rate      : %.2f\n", heart_rate);
      Serial.printf("Corrected Heart Rate: %.2f\n", corrected_heart_rate);
    }

    if (mmWave.getDistance(distance)) {
      updateDisplay(LABEL_DISTANCE, distance);
      Serial.printf("Distance      : %.2f cm\n", distance);
    }

    Serial.println("----------------------------");
    delay(1000);  // Wait before next read
  }
}
