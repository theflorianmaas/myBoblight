/* Modified and commented by Fra.par
  myBoblightFLED 3
  18/08/2016
  Addded - Change background color by a Sony Bravia remote control
    RED button = on/off off=all leds off
    GREEN button = change RGB color preset
    YELLOW button = toggle static / boblight / preset mode
    BLUE button = change pattern preset

  STATIC    set a static color
  BOBLIGHT  enable Boblight, color changed based on the screen color
  PATTERN   set a color animation

  myBoblightFLEDir is free software and can be distributed and/or modified
  freely as long as the copyright notice remains in place.
  Nobody is allowed to charge you for this code.
  Use of this code is entirely at your own risk.
*/

#include "FastLED.h"                                          // FastLED library.
#include "IRremote.h"

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

//DEFINITIONS

byte aRGB[] = { 0, 0, 125 }; //define variables to store RGB color values
//define variables to store RGB color values
#define MAX_PRESET_INDEX 7 //# of RBG color presets -1
byte RGBPresets[][3] = {
  { 0,    0,    0x7D }, //blue
  { 0xFF, 0,    0    }, //red
  { 0,    0x80, 0    }, //green
  { 0x64, 0,    0x64 }, //purple
  { 0xFF, 0x78, 0    }, //orange
  { 0x0A, 0,    0x64 }, //dark blue
  { 0xE2, 0x17, 0xE2 }, //magenta
  { 0xFF, 0xFF, 0    }, //yellow
};
int RGBPresetsIndex = 0; //current RGB color preset

#define DATAPIN             6      // Datapin for WS2812B LED strip
#define LEDCOUNT            59     // Number of LEDs used for boblight left 16, top 27, right 16
#define SHOWDELAY           200    // Delay in micro seconds before showing default 200
#define BAUDRATE            57600 //115200 // Serial port speed
#define LEDTYPE             WS2812B
#define COLORORDER          GRB
#define BRIGHTNESS          100
#define FRAMES_PER_SECOND   120

struct CRGB leds[LEDCOUNT];                                   // Initializxe our array

const char prefix[] = {0x41, 0x64, 0x61, 0x00, 0x18, 0x4D};  // Start prefix ADA
char buffer[sizeof(prefix)]; // Temporary buffer for receiving prefix data

// Remote control Definitions
// Change these values if you a different remote control. See the IRemote library for reference
//---------------------------------------------------------------------------------------------
#define REMOTE_TYPE SONY
#define RED     0x52E9 // red button
#define GREEN   0x32E9 // green button
#define YELLOW  0x72E9 // yellow button
#define BLUE    0x12E9 // blue button
#define DEFAULT 0x0    // default value
//---------------------------------------------------------------------------------------------
#define ON        1    // main Status ON
#define OFF       0   // main Status OFF
#define STATIC    10       // Static color mode  
#define BOBLIGHT  20       // Boblight mode
#define PATTERN   30       // Pattern mode

int RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
decode_results results;
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
int mainStatus  = ON; //Set the initial value
int modeStatus  = STATIC; //Set the initial mode
volatile boolean irstate = false;
#define RECEIVED   true    // - ir data received
#define IDLE       false   // - ir idle

// Declare patter functions
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();

#define NUMPATTERNS 4

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void setColor(uint32_t color);

int state;                   // Define current state
#define STATE_WAITING   1    // - Waiting for prefix
#define STATE_DO_PREFIX 2    // - Processing prefix
#define STATE_DO_DATA   3    // - Handling incoming LED colors

int readSerial;           // Read Serial data (1)
int currentLED;           // Needed for assigning the color to the right LED

void setup()
{
  pinMode(13, OUTPUT);
  attachInterrupt(0, irdata, CHANGE);
  irrecv.enableIRIn(); // Start the receiver
  irrecv.blink13(true);
  FastLED.addLeds<LEDTYPE, DATAPIN, COLORORDER>(leds, LEDCOUNT).setCorrection(TypicalLEDStrip);  // Use this for WS2812B
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  Serial.begin(BAUDRATE);   // Init serial speed

  //set initial color
  if (mainStatus == ON && modeStatus == STATIC) {
    setColor(DEFAULT); //set the default static color
  }
  else if (mainStatus == ON && modeStatus == BOBLIGHT) {
    setAllLEDs(0x0, 0x0, 0x0, 0); //turn off all leds
  }
  state = STATE_WAITING;    // Initial state: Waiting for prefix
}

void loop()
{
  if (irstate == RECEIVED) {
    //check if there is a command from the remote control and it is true analyze the result
    if (irrecv.decode(&results))
    {
      irrecv.resume(); // resume receiver
      storeCode(&results);
    }
    irstate == IDLE;
  }

  switch (mainStatus) {
    case ON:
      switch (modeStatus) {
        case PATTERN:
          // do some periodic updates
          EVERY_N_MILLISECONDS( 20 ) {
            gHue++;  // slowly cycle the "base color" through the rainbow
          }

          EVERY_N_MILLISECONDS(100) {         // FastLED based non-blocking delay to update/display the sequence.
            showPattern();
          }
          break;

        // if BOBLIGHT mode is selected data from serial and update leds
        case BOBLIGHT:
          switch (state)
          {
            case STATE_WAITING:                  // *** Waiting for prefix ***
              if ( Serial.available() > 0 )
              {
                readSerial = Serial.read();      // Read one character

                if ( readSerial == prefix[0] )   // if this character is 1st prefix char
                {
                  state = STATE_DO_PREFIX;  // then set state to handle prefix
                }
              }
              break;

            case STATE_DO_PREFIX:                // *** Processing Prefix ***
              if ( Serial.available() > sizeof(prefix) - 2 )
              {
                Serial.readBytes(buffer, sizeof(prefix) - 1);

                for ( int counter = 0; counter < sizeof(prefix) - 1; counter++)
                {
                  if ( buffer[counter] == prefix[counter + 1] )
                  {
                    state = STATE_DO_DATA;     // Received character is in prefix, continue
                    currentLED = 0;            // Set current LED to the first one
                  }
                  else
                  {
                    state = STATE_WAITING;     // Crap, one of the received chars is NOT in the prefix
                    break;                     // Exit, to go back to waiting for the prefix
                  } // end if buffer
                } // end for Counter
              } // end if Serial
              break;

            case STATE_DO_DATA: // *** Process incoming color data ***
              if ( Serial.available() > 2 )      // if we receive more than 2 chars
              {
                Serial.readBytes( buffer, 3 );   // Abuse buffer to temp store 3 charaters
                if (mainStatus == ON && modeStatus == BOBLIGHT)
                  setPixel( currentLED++, buffer[0], buffer[1], buffer[2]);  // and assing to LEDs
              }

              if ( currentLED > LEDCOUNT )       // Reached the last LED? Display it!
              {
                if (mainStatus == ON && modeStatus == BOBLIGHT)
                  showStrip();
                state = STATE_WAITING;         // Reset to waiting ...
                currentLED = 0;                // and go to LED one
                break;                         // and exit ... and do it all over again
              }
              break;
          } // switch(state)
          break;
      }
      break;
  }
} // loop


void irdata() {
  irstate = RECEIVED;
}

// Sets the color of all LEDs in the strand to 'color'
// If 'wait'>0 then it will show a swipe from start to end
void setAllLEDs(byte r, byte g, byte b, int wait)
{
  for ( int counter = 0; counter < LEDCOUNT; counter++ )    // For each LED
  {
    setPixel(counter, r, g, b);  // .. set the color
    if ( wait > 0 )                       // if a wait time was set then
    {
      // send the 'leds' array out to the actual LED strip
      showStrip();    // Show the LED color
      delay(wait);    // and wait before we do the next LED
    } // if wait

  } // for Counter
  showStrip();    // Show the LED color
} // setAllLEDs

void nextPattern()
{
  //add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = gCurrentPatternNumber + 1;
  if (gCurrentPatternNumber == NUMPATTERNS)
    gCurrentPatternNumber = 0;
}

void showPattern() {

  switch (gCurrentPatternNumber) {
    case 0:
      rainbow();
      break;
    case 1:
      rainbowWithGlitter();
      break;
    case 2:
      confetti();
      break;
    case 3:
      bpm();
      break;
  }
  showStrip();
}

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results * results) {
  codeType = results->decode_type;     
  codeValue = results->value;
  if (codeType == REMOTE_TYPE) {
    switch (codeValue) {
      case YELLOW: //toggle boblight
        switch (mainStatus) {
          case ON:
            switch (modeStatus) {
              case STATIC:
                FastLED.clear();
                modeStatus = BOBLIGHT;
                break;
              case BOBLIGHT:
                modeStatus = PATTERN;
                state = STATE_WAITING;
                FastLED.clear();
                showPattern();
                break;
              case PATTERN:
                modeStatus = STATIC;
                setColor(codeValue);
                break;
            }
            break;
        }
        break;
      case RED: // ON/OFF
        switch (mainStatus) {
          case ON:
            mainStatus = OFF;
            setAllLEDs(DEFAULT, DEFAULT, DEFAULT, 0);
            break;
          case OFF:
            mainStatus = ON;
            setColor(DEFAULT);
            break;
        }
        break;
      case GREEN:
        switch (mainStatus) {
          case ON:
            switch (modeStatus) {
              case STATIC:
                setColor(codeValue);
                break;
            }
            break;
        }
        break;
      case BLUE:   
        switch (mainStatus) {
          case ON:
            switch (modeStatus) {
              case PATTERN:
                nextPattern();
                FastLED.clear();
                showPattern();
                break;
            }
            break;
        }
        break;
    }
  }
  delay(10);
}

void setColor(uint32_t color = DEFAULT) {
  // if a valid command from remote control is received then execute the command, with other values (DEFAULT) set the last selected preset color
  // if the Status=ON and Boblight=Disabled
  if (mainStatus == ON && modeStatus == STATIC) {
    if (color == GREEN) { //change the RGB preset color
      if (++RGBPresetsIndex > MAX_PRESET_INDEX) {
        RGBPresetsIndex = 0;
      }
      //set the current RGB colors from the select preset color
      aRGB[0] = RGBPresets[RGBPresetsIndex][0];
      aRGB[1] = RGBPresets[RGBPresetsIndex][1];
      aRGB[2] = RGBPresets[RGBPresetsIndex][2];
    }

  }
  //show the color
  setAllLEDs(aRGB[0], aRGB[1], aRGB[2], 10);
}

void showStrip() {
  //if (!irrecv.isIdle()) //wait for the irrceiver has nothing to receive
    //irrecv.resume(); // resume receiver
  FastLED.show();
  delayMicroseconds(SHOWDELAY);  // Wait a few micro seconds
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}

// ----------------------------------------------------------------------------------
// Pattern n
// ----------------------------------------------------------------------------------
void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, LEDCOUNT, gHue, 7);
  //showStrip();
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(LEDCOUNT) ] += CRGB::White;
  }
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, LEDCOUNT, 10);
  int pos = random16(LEDCOUNT);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < LEDCOUNT; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}






