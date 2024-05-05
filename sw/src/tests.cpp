#include <Arduino.h>
#include <MD_MAX72xx.h>

#include "main.h"

void test_blinking(MD_MAX72XX mx)
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
