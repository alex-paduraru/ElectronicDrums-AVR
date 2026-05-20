#include <Arduino.h>
#include <TM1637Display.h>

// Inputs
#define BTN_BPM_UP           PB0
#define BTN_TOGGLE_METRONOME PB1
#define BTN_BPM_DOWN         PB2
#define BTN_CHANGE_PRESET    PB7

// Outputs
#define LED_CHANGE_PRESET PB5
#define LED_METRONOME     PD2
#define BUZZER            PD5
#define DISPLAY_CLK       PD6
#define DISPLAY_DIO       PD7



// TM1637 - 7 segments display
TM1637Display display(DISPLAY_CLK, DISPLAY_DIO);



// Metronome and Presets variables
uint16_t bpm = 120;
bool metronomeRunning = false;
uint8_t kitPreset = 1;

const uint16_t MAX_BPM = 300;
const uint16_t MIN_BPM = 30;
const uint8_t PRESETS_NR = 4;



// Buttons logic and debounce variables
uint8_t lastButtons = 0xFF;
uint32_t lastDebounceTime = 0;
uint32_t lastBpmRepeatTime = 0;

const uint16_t DEBOUNCE_MS = 200;
const uint16_t BPM_REPEAT_START_MS = 1000;
const uint16_t BPM_REPEAT_MS = 50;



void setup() {
    // Inputs
    DDRB &= ~(1 << BTN_BPM_UP);
    DDRB &= ~(1 << BTN_TOGGLE_METRONOME);
    DDRB &= ~(1 << BTN_BPM_DOWN);
    DDRB &= ~(1 << BTN_CHANGE_PRESET);

    // Inputs internal pull-ups
    PORTB |= (1 << BTN_BPM_UP);
    PORTB |= (1 << BTN_TOGGLE_METRONOME);
    PORTB |= (1 << BTN_BPM_DOWN);
    PORTB |= (1 << BTN_CHANGE_PRESET);

    lastButtons = PINB;

    // Outputs
    DDRB |= (1 << LED_CHANGE_PRESET);
    DDRD |= (1 << LED_METRONOME);
    DDRD |= (1 << BUZZER);
    DDRD |= (1 << DISPLAY_CLK);
    DDRD |= (1 << DISPLAY_DIO);

    // LEDs and Buzzer off
    PORTB &= ~(1 << LED_CHANGE_PRESET);
    PORTD &= ~(1 << LED_METRONOME);
    PORTD |= (1 << BUZZER);

    // Display on
    display.setBrightness(0);
    display.showNumberDec(bpm, false);
}



void loop() {

    // ===== BUTTONS READING =====

    uint32_t now = millis();

    uint8_t buttons = PINB;
    uint8_t pressed = (~buttons) & lastButtons;
    uint8_t held = ~buttons;

    if ((now - lastDebounceTime) > DEBOUNCE_MS) {
        // Toggle Metronome
        if (pressed & (1 << BTN_TOGGLE_METRONOME)) {
            metronomeRunning = !metronomeRunning;
            display.setBrightness(metronomeRunning ? 7 : 0);
            display.showNumberDec(bpm, false);

            lastDebounceTime = now;
        }

        // Change Preset
        if (pressed & (1 << BTN_CHANGE_PRESET)) {
            kitPreset++;
            kitPreset = kitPreset > PRESETS_NR ? 1 : kitPreset;
            for (int i = 0; i < kitPreset; i++) {
                PORTB |= (1 << LED_CHANGE_PRESET);
                delay(100);
                PORTB &= ~(1 << LED_CHANGE_PRESET);
                delay(150);
            }

            lastDebounceTime = now;
        }
    }



    // BPM Up
    if (pressed & (1 << BTN_BPM_UP)) {
        if (bpm < MAX_BPM) {
            bpm++;
            display.showNumberDec(bpm, false);
        }
        lastBpmRepeatTime = now + BPM_REPEAT_START_MS;
    }

    // Hold for faster BPM Up
    if ((held & (1 << BTN_BPM_UP)) && ((int32_t)(now - lastBpmRepeatTime) >= 0)) {
        if (bpm < MAX_BPM) {
            bpm++;
            display.showNumberDec(bpm, false);
        }
        lastBpmRepeatTime = now + BPM_REPEAT_MS;
    }

    // BPM Down
    if (pressed & (1 << BTN_BPM_DOWN)) {
        if (bpm > MIN_BPM) {
            bpm--;
            display.showNumberDec(bpm, false);
        }
        lastBpmRepeatTime = now + BPM_REPEAT_START_MS;
    }

    // Hold for faster BPM Down
    if ((held & (1 << BTN_BPM_DOWN)) && ((int32_t)(now - lastBpmRepeatTime) >= 0)) {
        if (bpm > MIN_BPM) {
            bpm--;
            display.showNumberDec(bpm, false);
        }
        lastBpmRepeatTime = now + BPM_REPEAT_MS;
    }

    lastButtons = buttons;





    // ===== PIEZO READING =====
}