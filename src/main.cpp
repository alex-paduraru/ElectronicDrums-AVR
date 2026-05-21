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
uint16_t bpm = 120;                         // Current metronome speed
volatile bool metronomeRunning = false;     // Metronome ON/OFF state
volatile bool metronomeTick = false;        // Set by Timer1 ISR on every beat
bool metronomePulseActive = false;          // LED and Buzzer pulse currently active
uint32_t metronomePulseEnd = 0;             // Timestamp for end of LED and Buzzer pulste
uint8_t kitPreset = 1;                      // Current sound preset

const uint8_t METRONOME_PULSE_MS = 50;      // Duration of LED and Buzzer pulse
const uint16_t MAX_BPM = 300;               // MAX speed of metronome
const uint16_t MIN_BPM = 30;                // MIN speed of metronome
const uint8_t PRESETS_NR = 4;               // Number of sound presets



// Buttons logic and debounce variables
uint8_t lastButtons = 0xFF;                 // PORTB previous state
uint32_t lastDebounceTime = 0;              // Timestamp of the last debounce
uint32_t nextBpmRepeatTime = 0;             // Timestamp of the next for BPM UP/DOWN step

const uint16_t DEBOUNCE_MS = 200;           // Debounce time for toggle and preset buttons
const uint16_t BPM_REPEAT_MS = 50;          // Repeat interval for holding BPM UP/DOWN
const uint16_t BPM_REPEAT_START_MS = 1000;  // Hold duration for start of fast BPM UP/DOWN




// Metronome Intrerrupt Routine
ISR(TIMER1_COMPA_vect) {
    if (metronomeRunning) {
        // LED and Buzzer ON
        PORTD |= (1 << LED_METRONOME);
        PORTD &= ~(1 << BUZZER);

        // Signal the pulse globaly
        metronomeTick = true;
    }
}



// Sets timer depending on BPM
void updateMetronomeTimer() {
    uint32_t compareValue;

    // ticks_per_beat = (CPU_frequency / Prescaler) * 60 / BPM
    compareValue = (F_CPU / 1024UL);
    compareValue = (compareValue * 60UL) / bpm;
    compareValue--;

    // Temporarily disable intrrupts
    uint8_t oldSREG = SREG;
    cli();

    // Set new timer compare value
    OCR1A = (uint16_t)compareValue;
    
    // Enable intrerrupts and restore registers
    SREG = oldSREG;
}



void setup() {

    // ===== I/O =====

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

    // LEDs and Buzzer OFF
    PORTB &= ~(1 << LED_CHANGE_PRESET);
    PORTD &= ~(1 << LED_METRONOME);
    PORTD |= (1 << BUZZER);

    // Display on
    display.setBrightness(0);
    display.showNumberDec(bpm, false);



    // ===== TIMER AND INTRERRPUTS =====

    // Reset Timer1 configuration registers
    TCCR1A = 0;
    TCCR1B = 0;

    // Reset Timer1 counter value
    TCNT1 = 0;

    // Sets initial timer
    updateMetronomeTimer();

    // Enable CTC Mode
    TCCR1B |= (1 << WGM12);

    // Set prescaler to 1024
    TCCR1B |= (1 << CS12) | (1 << CS10);

    // Disable Timer1 Compare Match A intrerrupt (Metronome starts as OFF)
    TIMSK1 &= ~(1 << OCIE1A);

    // Enable global intrerrupts
    sei();
}



void loop() {

    // ===== BUTTONS READING =====

    uint32_t now = millis();

    uint8_t buttons = PINB;                         // Hardware PINB values 
    uint8_t held = ~buttons;                        // Logic PINB values
    uint8_t pressed = (~buttons) & lastButtons;     // Newly pressed buttons
    

    if ((now - lastDebounceTime) > DEBOUNCE_MS) {
        // Toggle Metronome
        if (pressed & (1 << BTN_TOGGLE_METRONOME)) {
            metronomeRunning = !metronomeRunning;

            // Set display brightness depending on status
            display.setBrightness(metronomeRunning ? 7 : 0);
            display.showNumberDec(bpm, false);

            if (metronomeRunning) {
                // Enable Timer1 intrerrupts and reset timer
                uint8_t oldSREG = SREG;
                cli();
                TCNT1 = 0;
                TIMSK1 |= (1 << OCIE1A);
                SREG = oldSREG;
            } else {
                // Disable Timer1 intrerrupts
                TIMSK1 &= ~(1 << OCIE1A);
            }

            lastDebounceTime = now;
        }

        // Change Preset
        if (pressed & (1 << BTN_CHANGE_PRESET)) {
            kitPreset++;
            kitPreset = kitPreset > PRESETS_NR ? 1 : kitPreset;

            // Blink LED depending on preset
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
            updateMetronomeTimer();
            display.showNumberDec(bpm, false);
        }
        nextBpmRepeatTime = now + BPM_REPEAT_START_MS;
    }

    // Hold for faster BPM Up
    if ((held & (1 << BTN_BPM_UP)) && ((int32_t)(now - nextBpmRepeatTime) >= 0)) {
        if (bpm < MAX_BPM) {
            bpm++;
            updateMetronomeTimer();
            display.showNumberDec(bpm, false);
        }
        nextBpmRepeatTime = now + BPM_REPEAT_MS;
    }

    // BPM Down
    if (pressed & (1 << BTN_BPM_DOWN)) {
        if (bpm > MIN_BPM) {
            bpm--;
            updateMetronomeTimer();
            display.showNumberDec(bpm, false);
        }
        nextBpmRepeatTime = now + BPM_REPEAT_START_MS;
    }

    // Hold for faster BPM Down
    if ((held & (1 << BTN_BPM_DOWN)) && ((int32_t)(now - nextBpmRepeatTime) >= 0)) {
        if (bpm > MIN_BPM) {
            bpm--;
            updateMetronomeTimer();
            display.showNumberDec(bpm, false);
        }
        nextBpmRepeatTime = now + BPM_REPEAT_MS;
    }

    lastButtons = buttons;







    // ===== METRONOME =====

    // Marks the tick as read and decides the end timestamp of the pulse
    if (metronomeTick) {
        metronomeTick = false;
        metronomePulseActive = true;
        metronomePulseEnd = now + METRONOME_PULSE_MS;
    }

    // Disables the pulse if enough time has passed
    if (metronomePulseActive && ((int32_t)(now - metronomePulseEnd) >= 0)) {
        PORTD &= ~(1 << LED_METRONOME);
        PORTD |= (1 << BUZZER);

        metronomePulseActive = false;
    }







    // ===== PIEZO READING =====
}