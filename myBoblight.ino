/* Modified and commented by Fra.par
  myBoblight 1.1
  12/08/2016
  Addded - Change background color by Sony Bravia remote control
    RED button = increase/decrease RED in RGB color
    GREEN button = increase/decrease GREEN in RGB color
    BLUE button = increase/decrease BLUE in RGB color
    YELLOW button = toggle Boblight vs static color enabled/static color/dimmed color

  myBoblight is free software and can be distributed and/or modified
  freely as long as the copyright notice remains in place.
  Nobody is allowed to charge you for this code.
  Use of this code is entirely at your own risk.
*/

#include "Adafruit_NeoPixel.h"
#include "IRremote.h"

// #define DEBUG 1 //uncomment if you want to enable the debug mode
// DEFINITIONS

int R = 0, G = 0, B = 125; //define variables to store RGB color values

#define DATAPIN    6      // Datapin for WS2812B LED strip
#define LEDCOUNT   59     // Number of LEDs used for boblight left 16, top 27, right 16
#define SHOWDELAY  200    // Delay in micro seconds before showing default 200
#define BAUDRATE   115200 // Serial port speed
#define BRIGHTNESS 100    // Max. brightness in %

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
//---------------------------------------------------------------------------------------------
#define BOBLIGHT_ENABLED  true    // BOBLIGHT ENABLED, reveive data from serial
#define BOBLIGHT_DISABLED false   // BOBLIGHT DISABLED, set the static color
#define COLOR_INCREASE false
#define COLOR_DECREASE true
#define COLOR_CHANGE_INTERVAL 10

int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
boolean mainStatus      = BOBLIGHT_DISABLED; //Set the initial value
boolean REDDirection    = COLOR_INCREASE;
boolean GREENDirection  = COLOR_INCREASE;
boolean BLUEDirection   = COLOR_INCREASE;

void setColor(uint32_t color);

// Init LED strand, WS2811/WS2912 specific
// NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDCOUNT, DATAPIN, NEO_GRB + NEO_KHZ800);

int state;                   // Define current state
#define STATE_WAITING   1    // - Waiting for prefix
#define STATE_DO_PREFIX 2    // - Processing prefix
#define STATE_DO_DATA   3    // - Handling incoming LED colors

int readSerial;           // Read Serial data (1)
int currentLED;           // Needed for assigning the color to the right LED

void setup()
{
  irrecv.enableIRIn(); // Start the receiver
  strip.begin();            // Init LED strand, set all black, then all to startcolor
  strip.setBrightness( (255 / 100) * BRIGHTNESS );
  Serial.begin(BAUDRATE);   // Init serial speed

  //set initial color
  if (mainStatus   == BOBLIGHT_DISABLED) {
    setColor(YELLOW); 
  }
  else {
    setAllLEDs(0x000000, 0)
  }
  
  state = STATE_WAITING;    // Initial state: Waiting for prefix
}

void loop()
{

  if (irrecv.decode(&results)) //check if there is a command from the remote control and it is true analyze the result
  {
    storeCode(&results);
    irrecv.resume(); // resume receiver
  }

  if (mainStatus == BOBLIGHT_ENABLED) // if boblight is enabled read data from serial
  {
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

          for ( int Counter = 0; Counter < sizeof(prefix) - 1; Counter++)
          {
            if ( buffer[Counter] == prefix[Counter + 1] )
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


      case STATE_DO_DATA:                  // *** Process incoming color data ***
        if ( Serial.available() > 2 )      // if we receive more than 2 chars
        {
          Serial.readBytes( buffer, 3 );   // Abuse buffer to temp store 3 charaters
          strip.setPixelColor( currentLED++, buffer[0], buffer[1], buffer[2]);  // and assing to LEDs
        }

        if ( currentLED > LEDCOUNT )       // Reached the last LED? Display it!
        {
          strip.show();                  // Make colors visible
          delayMicroseconds(SHOWDELAY);  // Wait a few micro seconds

          state = STATE_WAITING;         // Reset to waiting ...
          currentLED = 0;                // and go to LED one

          break;                         // and exit ... and do it all over again
        }
        break;
    } // switch(state)
  }
} // loop


// Sets the color of all LEDs in the strand to 'color'
// If 'wait'>0 then it will show a swipe from start to end
void setAllLEDs(uint32_t color, int wait)
{
  for ( int Counter = 0; Counter < LEDCOUNT; Counter++ )    // For each LED
  {
    strip.setPixelColor( Counter, color );      // .. set the color

    if ( wait > 0 )                       // if a wait time was set then
    {
      strip.show();                     // Show the LED color
      delay(wait);                      // and wait before we do the next LED
    } // if wait

  } // for Counter
  strip.show();                         // Show all LEDs
} // setAllLEDs


// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
  codeType = results->decode_type;
  codeValue = results->value;
  if (codeType == REMOTE_TYPE) {
    if (codeValue == YELLOW) { //toggle boblight
      if (mainStatus == BOBLIGHT_ENABLED) {
        mainStatus = BOBLIGHT_DISABLED;
        setColor(codeValue);
      } else if (mainStatus == BOBLIGHT_DISABLED) {
        mainStatus = BOBLIGHT_ENABLED;

      }
      prt(mainStatus);
    }
    else if (codeValue == RED || codeValue == GREEN || codeValue == BLUE) {
      setColor(codeValue);
    }
  }
}

void setColor(uint32_t color) {
  if (mainStatus == BOBLIGHT_DISABLED) {
    switch (color)
    {
      case RED:
        if (REDDirection == COLOR_INCREASE) {
          R = R + COLOR_CHANGE_INTERVAL;
          if (R > 255) {
            R = 255;
            REDDirection = COLOR_DECREASE;
          }
        }
        else if (REDDirection == COLOR_DECREASE) {
          R = R - COLOR_CHANGE_INTERVAL;
          if (R < 0) {
            R = 0;
            REDDirection = COLOR_INCREASE;
          }
        }
        prt(R);
        break;
      case GREEN:
        if (GREENDirection == COLOR_INCREASE) {
          G = G + COLOR_CHANGE_INTERVAL;
          if (G > 255) {
            G = 255;
            GREENDirection = COLOR_DECREASE;
          }
        }
        else if (GREENDirection == COLOR_DECREASE) {
          G = G - COLOR_CHANGE_INTERVAL;
          if (G < 0) {
            G = 0;
            GREENDirection = COLOR_INCREASE;
          }
        }
        prt(G);
        break;
      case BLUE:
        if (BLUEDirection == COLOR_INCREASE) {
          B = B + COLOR_CHANGE_INTERVAL;
          if (B > 255) {
            B = 255;
            BLUEDirection = COLOR_DECREASE;
          }
        }
        else if (BLUEDirection == COLOR_DECREASE) {
          B = B - COLOR_CHANGE_INTERVAL;
          if (B < 0) {
            B = 0;
            BLUEDirection = COLOR_INCREASE;
          }
        }
        prt(B);
        break;
    }
  }
  setAllLEDs(strip.Color(R, G, B), 10);
}

void prt(int str)
{
#ifdef DEBUG
  //Serial.println(str);
#endif
}

