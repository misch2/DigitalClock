// clang-format off
#define LOCAL_DEBUG
#include "common/prolog.h"
// clang-format on

#include "intensity.h"

#include <BH1750.h>
#include <Wire.h>
#include <movingAvg.h>

BH1750 lightMeter;
movingAvg intensityAvg(10);  // use 10 data points for the moving average

long map_and_clamp(long x, long in_min, long in_max, long out_min, long out_max) {
  return constrain(map(x, in_min, in_max, out_min, out_max), out_min, out_max);
}

void initLightSensor() {
  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  // On esp8266 devices you can select SCL and SDA pins using Wire.begin(D4, D3);
  DEBUG_PRINT("Initializing light sensor");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2);
  intensityAvg.begin();
}

int last_intensity = -1;
void setAutoIntensity(tm rtcTime) {
  float lux = lightMeter.readLightLevel();
  // Serial.printf("Light: %.2f lux\n", lux);

  if (lux < 0) {
    Serial.println(F("Error reading light intensity - no sensor"));
  } else {
    if (lux > 40000.0) {
      // reduce measurement time - needed in direct sun light
      if (lightMeter.setMTreg(32)) {
        // Serial.println(F("Setting MTReg to low value for high light environment"));
      } else {
        Serial.println(F("Error setting MTReg to low value for high light environment"));
      }
    } else {
      if (lux > 10.0) {
        // typical light environment
        if (lightMeter.setMTreg(69)) {
          // Serial.println(F("Setting MTReg to default value for normal light environment"));
        } else {
          Serial.println(
              F("Error setting MTReg to default value for normal "
                "light environment"));
        }
      } else {
        if (lux <= 10.0) {
          // very low light environment
          if (lightMeter.setMTreg(138)) {
            // Serial.println(F("Setting MTReg to high value for low light environment"));
          } else {
            Serial.println(
                F("Error setting MTReg to high value for low "
                  "light environment"));
          }
        }
      }
    }
  };

  int log_corrected_lux = 0;
  if (lux > 0) {
    log_corrected_lux = 1000 * log10(lux);
  };
  int current_intensity = map_and_clamp(log_corrected_lux, 0, 1000, 0, MAX_INTENSITY);

  int final_intensity = intensityAvg.reading(current_intensity);

  mx.control(MD_MAX72XX::INTENSITY, final_intensity);
  if (final_intensity != last_intensity) {
    // DEBUG_PRINT("Light intensity: %.1f lux (log_corrected=%d) -> movavg intensity %d", lux, log_corrected_lux, final_intensity);
    last_intensity = final_intensity;
  }
}
