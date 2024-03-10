/**
 * Hermit Retro ZXUSB Firmware
 *
 * Copyright (c)2021 Hermit Retro Products <https://hermitretro.com>
 *
 * Shift - applies shift + key to be interpreted locally
 * Symbol Shift = CTRL
 * Symbol Shift + Shift = limited remapping
 * EXT1 = Alt
 * EXT2 = Option
 *
 * Todo:
 * - Add "classic" Spectrum layout mode possibly switched in at boot time by holding down
 *   EXT1
 */

#include "Keyboard.h"

#define NUM_ROWS 8
#define NUM_COLS 5

/** Unshifted keymap */
byte keyMap[NUM_ROWS][NUM_COLS] = {
  { '5', '4', '3', '2', '1' }, 
  { 't', 'r', 'e', 'w', 'q' }, 
  { 'g', 'f', 'd', 's', 'a' }, 
  { '6', '7', '8', '9', '0' }, 
  { 'y', 'u', 'i', 'o', 'p' },
  { 'v', 'c', 'x', 'z', 0 }, 
  { 'h', 'j', 'k', 'l', KEY_RETURN },
  { 'b', 'n', 'm', '.', ' ' }
};

/** SHIFT keymap */
byte keyMapShifted[NUM_ROWS][NUM_COLS] = {
  { KEY_LEFT_ARROW, '$', '\\', '@', KEY_ESC }, 
  { 'T', 'R', 'E', 'W', 'Q' }, 
  { 'G', 'F', 'D', 'S', 'A' }, 
  { KEY_DOWN_ARROW, KEY_UP_ARROW, KEY_RIGHT_ARROW, '!', KEY_BACKSPACE }, 
  { 'Y', 'U', 'I', 'O', 'P' },
  { 'V', 'C', 'X', 'Z', 0 }, 
  { 'H', 'J', 'K', 'L', KEY_F5 },
  { 'B', 'N', 'M', ',', '#' }
};

/** SYMBOL SHIFT keymap */
byte keyMapSymbolShifted[NUM_ROWS][NUM_COLS] = {
  { '%', '$', '#', '@', '!' }, 
  { '>', '<', 'E', 'W', 'Q' }, 
  { 'G', 'F', 'D', 'S', 'A' }, 
  { '&', '`', '(', ')', '_' }, 
  { 'Y', 'U', 'I', ';', '\"' },
  { '/', '?', '#', ':', 0 }, 
  { '^', '-', '+', '=', KEY_F5 },
  { '*', '\'', '.', 0, '#' }
};

// define the row and column pins
uint8_t colPins[NUM_COLS] = {
  A0, 15, 14, 16, 10
};

uint8_t rowPins[NUM_ROWS] = {
  9,8, 7, 6, 5, 4, 3, 2 };

#define EXT1_PIN A1
#define EXT2_PIN A2
#define ZELUX_PWR_PIN A3

/** Shift position */
#define SHIFT_COL 4
#define SHIFT_ROW 5

/** Symbol shift position */
#define SYMBOL_SHIFT_ROW 7
#define SYMBOL_SHIFT_COL 3

#define SYMBOLSHIFT_ACTION KEY_LEFT_CTRL    /** Ctrl */
#define EXT1_ACTION KEY_LEFT_GUI    /** Option */
#define EXT2_ACTION KEY_LEFT_ALT    /** Alt */

/** Pressed keys matrix to speed up unpresses... */
unsigned char keysPressed[NUM_ROWS][NUM_COLS];
bool shiftPressed = false;
bool symbolShiftPressed = false;
bool ext1Pressed = false;
bool ext2Pressed = false;

/** Debouncing */
#define GPIO_MEMBRANE_DEBOUNCE_IN_MS 50L
unsigned long lastEventTime = 0;

#define ZXUSB_VERSION_MAJOR 0
#define ZXUSB_VERSION_MINOR 0
#define ZXUSB_VERSION_PATCH 0

bool debounceEvent() {
    return (millis() - lastEventTime) < GPIO_MEMBRANE_DEBOUNCE_IN_MS;
}

/** Press the key */
void pressKey( uint8_t r, uint8_t c, bool shifted, bool symbolShifted ) {  

    // byte key = shifted ? keyMapShifted[r][c] : (symbolShifted ? keyMapSymbolShifted[r][c] : keyMap[r][c]);    keysPressed[r][c] = 1;
    byte key = keyMap[r][c];

    keysPressed[r][c] = 1;
    if ( key == '0' && symbolShiftPressed ) {
        /** Map to backspace */
        Keyboard.release( SYMBOLSHIFT_ACTION );
        Keyboard.press( KEY_BACKSPACE );
        Keyboard.press( SYMBOLSHIFT_ACTION );
        return;
    }

    Keyboard.press( key );
}

/** Unpress the key */
void unpressKey( uint8_t r, uint8_t c, bool shifted, bool symbolShifted ) {  

    // byte key = shifted ? keyMapShifted[r][c] : (symbolShifted ? keyMapSymbolShifted[r][c] : keyMap[r][c]);
    byte key = keyMap[r][c];

    if ( key == '0' && symbolShiftPressed ) {
        /** Map to backspace */
        Keyboard.release( KEY_BACKSPACE );
        return;
    }

    Keyboard.release( key );
    keysPressed[r][c] = 0;
}

void setup() {

    Serial1.begin( 9600 );
    while ( !Serial1 ) {}

    Serial1.print( F("Hermit Retro Products ZXUSB (") );
    Serial1.print( ZXUSB_VERSION_MAJOR );
    Serial1.print( F(".") );
    Serial1.print( ZXUSB_VERSION_MINOR );
    Serial1.print( F(".") );
    Serial1.print( ZXUSB_VERSION_PATCH );
    Serial1.println( F(")") );
    Serial1.flush();

    pinMode( ZELUX_PWR_PIN, OUTPUT );
    digitalWrite( ZELUX_PWR_PIN, LOW );

    pinMode( EXT1_PIN, INPUT );
    pinMode( EXT2_PIN, INPUT );

    for ( int c = 0 ; c < NUM_COLS ; c++ ) {
        pinMode( colPins[c], INPUT );
        digitalWrite( colPins[c], HIGH );

        for (byte r = 0; r < NUM_ROWS; r++) {
            pinMode( rowPins[r], INPUT );
            keysPressed[r][c] = 0;
        }
    }

    Keyboard.begin();
}

void loop() {

    /** Debounce */
    if ( debounceEvent() ) {
        return;
    }

    /** Check for SHIFT */
    pinMode( rowPins[SHIFT_ROW], OUTPUT );
    bool lshiftPressed = (digitalRead( colPins[SHIFT_COL]) == LOW );
    pinMode( rowPins[SHIFT_ROW], INPUT );
    if ( lshiftPressed ) {
        Keyboard.press( KEY_LEFT_SHIFT );
    } else {
        if ( shiftPressed ) {
            Keyboard.release( KEY_LEFT_SHIFT );
        }
    }
    shiftPressed = lshiftPressed;

    /** Check for SYMBOL SHIFT */
    pinMode( rowPins[SYMBOL_SHIFT_ROW], OUTPUT );
    bool lsymbolShiftPressed = (digitalRead( colPins[SYMBOL_SHIFT_COL]) == LOW );
    pinMode( rowPins[SYMBOL_SHIFT_ROW], INPUT );

    if ( lsymbolShiftPressed ) {
        Keyboard.press( SYMBOLSHIFT_ACTION );
    } else {
        if ( symbolShiftPressed ) {
            Keyboard.release( SYMBOLSHIFT_ACTION );
        }
    }
    symbolShiftPressed = lsymbolShiftPressed;

    /** Check for EXT1 */
    bool lext1Pressed = (digitalRead( EXT1_PIN ) == LOW );
    if ( lext1Pressed ) {
        Keyboard.press( EXT1_ACTION );
    } else {
        if ( ext1Pressed ) {
            Keyboard.release( EXT1_ACTION );
        }
    }
    ext1Pressed = lext1Pressed;

    /** Check for EXT2 */
    bool lext2Pressed = (digitalRead( EXT2_PIN ) == LOW );
    if ( lext2Pressed ) {
        Keyboard.press( EXT2_ACTION );
    } else {
        if ( ext2Pressed ) {
            Keyboard.release( EXT2_ACTION );
        }
    }
    ext2Pressed = lext2Pressed;

    /**
     * Scan the keyboard matrix by enabling each row one-by-one and testing each column
     */
    for ( uint8_t r = 0 ; r < NUM_ROWS ; r++ ) {
        pinMode( rowPins[r], OUTPUT );
        digitalWrite( rowPins[r], LOW );

        for ( uint8_t c = 0 ; c < NUM_COLS ; c++ ) {

            /** We've already processed CAPS SHIFT and SYMBOL SHIFT so skip them here */
            if ( (r == SHIFT_ROW && c == SHIFT_COL) || (r == SYMBOL_SHIFT_ROW && c == SYMBOL_SHIFT_COL) ) {
              continue;
            }

            if ( digitalRead( colPins[c] ) == LOW ) {
                pressKey( r, c, shiftPressed, symbolShiftPressed );
            } else {
                if ( keysPressed[r][c] ) {
                    unpressKey( r, c, shiftPressed, symbolShiftPressed );
                }
                keysPressed[r][c] = 0;
            }
        }

        pinMode( rowPins[r], INPUT );
    }

    digitalWrite( rowPins[SHIFT_ROW], LOW );
}
