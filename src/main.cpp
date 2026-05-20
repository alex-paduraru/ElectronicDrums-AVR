#include <Arduino.h>

#define LED_METRONOME PD2

void setup() {
    DDRD |= (1 << LED_METRONOME);
}

void loop() {

    // Dummy Code
    PORTD |= (1 << LED_METRONOME);
    delay(100);
    PORTD &= ~(1 << LED_METRONOME);
    delay(1000);
}