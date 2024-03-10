/**
 * Hermit Retro ZXUSB Firmware
 *
 * Copyright (c)2021 Hermit Retro Products <https://hermitretro.com>
 *
 * Shift - Generally shifted characters except for 5, 6, 7, 8 which act as cursor keys and 0 is DELETE
 * Symbol Shift - Generally the red characters on each key, except for some cases where the red legend beneath the key is used, e.g., ~
 * Symbol Shift + Shift = Ctrl on Mac, Option (maybe Command?) on PC
 * EXT1 = Option on Mac, Ctrl on PC
 * EXT2 = Alt
 * Symbol Shift + RETURN = ESCAPE
 * Shift + RETURN = Tab
 *
 * If EXT1 is held down at connection of the ZXUSB, it will boot into PC mappings, Mac is default.
 * You can change the default behaviour around line 96
 *
 */

#include "Keyboard.h"

#define NUM_ROWS 8
#define NUM_COLS 5

#define SYMBOLSHIFT_ENHANCED 0x00
#define NULL_KEY 0xFF

/** Unshifted keymap */
byte keyMap[NUM_ROWS][NUM_COLS] = {
    { '5', '4', '3', '2', '1' }, 
    { 't', 'r', 'e', 'w', 'q' }, 
    { 'g', 'f', 'd', 's', 'a' }, 
    { '6', '7', '8', '9', '0' }, 
    { 'y', 'u', 'i', 'o', 'p' },
    { 'v', 'c', 'x', 'z', NULL_KEY }, 
    { 'h', 'j', 'k', 'l', KEY_RETURN },
    { 'b', 'n', 'm', '.', ' ' }
};

/** Classic SHIFT keymap */
byte keyMapShiftedSpectrum[NUM_ROWS][NUM_COLS] = {
    { KEY_LEFT_ARROW, '$', '#', '@', KEY_ESC }, 
    { 'T', 'R', 'E', 'W', 'Q' }, 
    { 'G', 'F', 'D', 'S', 'A' }, 
    { KEY_DOWN_ARROW, KEY_UP_ARROW, KEY_RIGHT_ARROW, ')', KEY_BACKSPACE }, 
    { 'Y', 'U', 'I', 'O', 'P' },
    { 'V', 'C', 'X', 'Z', NULL_KEY }, 
    { 'H', 'J', 'K', 'L', KEY_TAB },
    { 'B', 'N', 'M', ',', '#' }
};

/** Classic SYMBOL SHIFT keymap */
byte keyMapSymbolShiftedSpectrum[NUM_ROWS][NUM_COLS] = {
    { '%', '$', '#', '@', '!' }, 
    { '>', '<', SYMBOLSHIFT_ENHANCED, SYMBOLSHIFT_ENHANCED, SYMBOLSHIFT_ENHANCED }, 
    { '}', '{', '\\', '|', '~' }, 
    { '&', '`', '(', ')', '_' }, 
    { '[', ']', SYMBOLSHIFT_ENHANCED, ';', '\"' },
    { '/', '?', '#', ':', NULL_KEY }, 
    { '^', '-', '+', '=', KEY_ESC },
    { '*', ',', '.', NULL_KEY, '#' }
};

/** Classic SYMBOL SHIFT keymap -- enhanced */
String keyMapSymbolShiftedEnhancedSpectrum[NUM_ROWS][NUM_COLS] = {
    { "", "", "", "", "" }, 
    { "", "", ">=", "<>", "<=" }, 
    { "", "", "", "", "" }, 
    { "", "", "", "", "" }, 
    { "", "", "(C)", "", "" }, 
    { "", "", "", "", "" }, 
    { "", "", "", "", "" }, 
    { "", "", "", "", "" }
};

// define the row and column pins
uint8_t colPins[NUM_COLS] = {
    A0, 15, 14, 16, 10
};

uint8_t rowPins[NUM_ROWS] = {
    9,8, 7, 6, 5, 4, 3, 2
};

#define EXT1_PIN A1
#define EXT2_PIN A2
#define ZELUX_PWR_PIN A3

/** Shift position */
#define SHIFT_COL 4
#define SHIFT_ROW 5

/** Symbol shift position */
#define SYMBOL_SHIFT_ROW 7
#define SYMBOL_SHIFT_COL 3

/** Mac style */
typedef enum { VARIANT_PC, VARIANT_MAC } ModifierVariant;
ModifierVariant variant = VARIANT_MAC;      /** Change this if you don't want to have to press EXT2 at each use */
#define SHIFT_ACTION KEY_LEFT_SHIFT         /** Shift */
uint8_t SHIFTSYMSHIFT_ACTION = KEY_LEFT_CTRL;
uint8_t EXT1_ACTION = KEY_LEFT_GUI;
uint8_t EXT2_ACTION = KEY_LEFT_ALT;

/** Pressed keys matrix to speed up unpresses... */
bool keysPressed[NUM_ROWS][NUM_COLS];
bool shiftPressed = false;
bool symbolShiftPressed = false;
bool ext1Pressed = false;
bool ext2Pressed = false;

/** Debouncing */
#define GPIO_MEMBRANE_DEBOUNCE_IN_MS 50L
unsigned long lastEventTime = 0;

#define ZXUSB_VERSION_MAJOR 1
#define ZXUSB_VERSION_MINOR 0
#define ZXUSB_VERSION_PATCH 0

bool debounceEvent() {
    return (millis() - lastEventTime) < GPIO_MEMBRANE_DEBOUNCE_IN_MS;
}

void setVariant() {

    switch ( variant ) {
        case VARIANT_MAC: {
            SHIFTSYMSHIFT_ACTION = KEY_LEFT_CTRL;
            EXT1_ACTION = KEY_LEFT_GUI;
            EXT2_ACTION = KEY_LEFT_ALT;
            break;
        }
        case VARIANT_PC: {
            SHIFTSYMSHIFT_ACTION = KEY_LEFT_GUI;
            EXT1_ACTION = KEY_LEFT_CTRL;
            EXT2_ACTION = KEY_LEFT_ALT;
            break;
        }
    }
}

void _handleKey( bool doPress, uint8_t r, uint8_t c ) {

    uint8_t key = 0xFF;
    String enhancedKey = "";

    if ( shiftPressed ) {
        if ( symbolShiftPressed ) {

            /** Shift + Symbol Shift */
            key = keyMap[r][c];
            enhancedKey = "";

            Keyboard.press( SHIFTSYMSHIFT_ACTION );
        } else {
            /** Only shifted */
            key = keyMapShiftedSpectrum[r][c];                  
        }
    } else {
        /** Not shifted */
        if ( symbolShiftPressed ) {
            key = keyMapSymbolShiftedSpectrum[r][c];
            if ( key == SYMBOLSHIFT_ENHANCED ) {
                enhancedKey = keyMapSymbolShiftedEnhancedSpectrum[r][c];
            }
        } else {
            /** Completely unmodified (unless EXT1 or EXT2 has been pressed) */
            key = keyMap[r][c];                          
        }
    }

    if ( key == NULL_KEY ) {
        return;
    }

    if ( enhancedKey.length() > 0 ) {
        for ( size_t i = 0 ; i < enhancedKey.length() ; i++ ) {
            if ( doPress ) {
                Keyboard.press( enhancedKey[i] );        
            } else {
                Keyboard.release( enhancedKey[i] );
            }
            delay( 25 );
        }
    } else {
        if ( doPress ) {
            Keyboard.press( key );
        } else {
            Keyboard.release( key );
        }
    }
}

/** Press the key */
void pressKey( uint8_t r, uint8_t c ) {  

    keysPressed[r][c] = true;

    _handleKey( true, r, c );
}

/** Unpress the key */
void unpressKey( uint8_t r, uint8_t c ) {  

    _handleKey( false, r, c );

    keysPressed[r][c] = false;
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

    pinMode( EXT1_PIN, INPUT_PULLUP );
    pinMode( EXT2_PIN, INPUT_PULLUP );

    /** Are we using Mac modifiers or PC modifiers? (Default is Mac) */
    if ( digitalRead( EXT1_PIN) == LOW ) {
        variant = VARIANT_PC;
    }
    Serial1.println( "" );
    Serial1.print( F("Using variant: ") );
    Serial1.println( variant == VARIANT_PC ? F("PC") : F("MacOS") );

    setVariant();

    /** Setup default membrane pin states */
    for ( int c = 0 ; c < NUM_COLS ; c++ ) {
        pinMode( colPins[c], INPUT );
        digitalWrite( colPins[c], HIGH );

        for (byte r = 0; r < NUM_ROWS; r++) {
            pinMode( rowPins[r], INPUT );
            keysPressed[r][c] = false;
        }
    }

    Keyboard.begin();
}

void loop() {

    /** Debounce */
    if ( debounceEvent() ) {
        return;
    }

    /** Handle SHIFT/SYM SHIFT here in case we're pressing both to go into a modifier mode */
    bool shouldReleaseShift = false;
    bool shouldReleaseSymshift = false;

    /** Check for SHIFT */
    pinMode( rowPins[SHIFT_ROW], OUTPUT );
    bool lshiftPressed = (digitalRead( colPins[SHIFT_COL]) == LOW );
    pinMode( rowPins[SHIFT_ROW], INPUT );
    if ( !lshiftPressed ) {
        if ( shiftPressed ) {
            shouldReleaseShift = true;
        }
    }
    shiftPressed = lshiftPressed;

    /** Check for SYMBOL SHIFT */
    pinMode( rowPins[SYMBOL_SHIFT_ROW], OUTPUT );
    bool lsymbolShiftPressed = (digitalRead( colPins[SYMBOL_SHIFT_COL]) == LOW );
    pinMode( rowPins[SYMBOL_SHIFT_ROW], INPUT );
    if ( !lsymbolShiftPressed ) {
        if ( symbolShiftPressed ) {
            shouldReleaseSymshift = true;
        }
    }
    symbolShiftPressed = lsymbolShiftPressed;

    if ( shouldReleaseShift || shouldReleaseSymshift ) {
        Keyboard.release( SHIFTSYMSHIFT_ACTION );
    }

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
                pressKey( r, c );
            } else {
                if ( keysPressed[r][c] ) {
                    unpressKey( r, c );
                }
                keysPressed[r][c] = false;
            }
        }

        pinMode( rowPins[r], INPUT );
    }

    digitalWrite( rowPins[SHIFT_ROW], LOW );
}
