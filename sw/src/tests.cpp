// clang-format off
#define LOCAL_DEBUG
#include "common/prolog.h"
// clang-format on

#include "tests.h"

void test_blinking()
// Uses the test function of the MAX72xx to blink the display on and off.
{
  int nDelay = 1000;

  Serial.println("\nBlinking");

  while (nDelay > 0) {
    Serial.printf("Blinking delay: %d\n", nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
    delay(nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    delay(nDelay);

    nDelay -= 500;
  }

  mx.control(0, MD_MAX72XX::TEST, MD_MAX72XX::ON);
  delay(1000);
  mx.control(1, MD_MAX72XX::TEST, MD_MAX72XX::ON);
  delay(1000);
  mx.control(0, MD_MAX72XX::TEST, MD_MAX72XX::OFF);
  delay(1000);
  mx.control(1, MD_MAX72XX::TEST, MD_MAX72XX::OFF);
  delay(1000);

  Serial.println("Stopped blinking");
};

void test_all_on() {
  Serial.println("\nAll on");

  // mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY);
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY / 2);  // To avoid burning the LEDs and to see PWM on a scope
  mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
};

void test_off() {
  Serial.println("\nTestmode off");
  mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
};