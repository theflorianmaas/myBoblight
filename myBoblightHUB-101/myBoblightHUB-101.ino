/* Modified and commented by Fra.par
  myBoblightHUB 101
  04/01/2018
  Addded - Change background color by a Sony Bravia remote control
    RED button = on/off off=all leds off
    GREEN button = change RGB color preset
    YELLOW button = toggle static / boblight / preset mode
    BLUE button = change pattern preset

  STATIC    set a static color
  BOBLIGHT  enable Boblight, color changed based on the screen color
  PATTERN   set a color animation (not used)

  myBoblightHUB101 is free software and can be distributed and/or modified
  freely as long as the copyright notice remains in place.
  Nobody is allowed to charge you for this code.
  Use of this code is entirely at your own risk.
*/


#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include "IRremote.h"
#include <XBee.h>
#include <SoftwareSerial.h>

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

#define RECV_PIN            2      // ir led
#define DATAPIN             6      // Datapin for WS2812B LED strip
#define LEDCOUNT            60     // Number of LEDs used for boblight left 16, top 27, right 16
#define BAUDRATE            115200 // Serial port speed
#define BRIGHTNESS          100

const char prefix[] = {0x41, 0x64, 0x61, 0x00, 0x18, 0x4D};  // Start prefix ADA
char buffer[sizeof(prefix)]; // Temporary buffer for receiving prefix data
uint8_t sizeOfPrefix = sizeof(prefix);

// Remote control Definitions
// Change these values if you a different remote control. See the IRemote library for reference
//---------------------------------------------------------------------------------------------
#define REMOTE_TYPE SONY
#define RED      0x52E9 // red button
#define GREEN    0x32E9 // green button
#define YELLOW   0x72E9 // yellow button
#define BLUE     0x12E9 // blue button
#define DEFAULTP 0x0    // default color preset
//---------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------
#define ON        1    // main Status ON
#define OFF       0   // main Status OFF
#define STATIC    10       // Static color mode  
#define BOBLIGHT  20       // Boblight mode
#
//---------------------------------------------------------------------------------------------
// IR definition
//---------------------------------------------------------------------------------------------
IRrecv irrecv(RECV_PIN);
decode_results results;
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
int mainStatus  = ON; //Set the initial value
int modeStatus  = STATIC; // STATIC //Set the initial mode
volatile boolean irstate = false;
#define RECEIVED   true    // - ir data received
#define IDLE       false   // - ir idle

void setColor(uint32_t color);

uint8_t state;               // - Define current status
#define STATE_WAITING   1    // - Waiting for prefix
#define STATE_DO_PREFIX 2    // - Processing prefix
#define STATE_DO_DATA   3    // - Handling incoming LED colors

uint8_t readSerial;           // Read Serial data (1)
uint8_t currentLED;           // Needed for assigning the color to the right LED

// Satellite config
// Define SoftSerial TX/RX pins
uint8_t ssRX = 5; //TX of usb-serial device
uint8_t ssTX = 4; //RX of usb-serial device
// Remember to connect all devices to a common Ground: XBee, Arduino and USB-Serial device
SoftwareSerial XbeeSerial(ssRX, ssTX);
int16_t xbeeData[6]; //array data to transmit RGB to the satellite
XBee xbee = XBee();
uint8_t payload[6];
// 16-bit addressing: Enter address of remote XBee, typically the coordinator
Tx16Request tx = Tx16Request(0xFFFF, payload, sizeof(payload));
TxStatusResponse txStatus = TxStatusResponse();

Adafruit_NeoPixel LEDS = Adafruit_NeoPixel(LEDCOUNT, DATAPIN);

void setup()
{
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(RECV_PIN, INPUT);
  //attachInterrupt(digitalPinToInterrupt(RECV_PIN), irdata, CHANGE);
  irrecv.enableIRIn(); // Start the receiver
  irrecv.blink13(true);
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code
  LEDS.begin(); // This initializes the NeoPixel library.
  LEDS.setBrightness(BRIGHTNESS);
  LEDS.show();

  Serial.begin(BAUDRATE);   // Init serial speed
  xbee.setSerial(XbeeSerial);
  XbeeSerial.begin(57600);
  xbee.begin(XbeeSerial);

  //set initial color
  if (mainStatus == ON && modeStatus == STATIC) {
    setColor(DEFAULTP); //set the default static color
    sendSatellite (DEFAULTP, DEFAULTP, 0xFF);
  }
  else if (mainStatus == ON && modeStatus == BOBLIGHT) {
    setAllLEDs(0x0, 0x0, 0x0, 0); //turn off all leds
    sendSatellite (0x0, 0x0, 0x0);
  }
  state = STATE_WAITING;    // Initial state: Waiting for prefix
}

void loop()
{

  //check if there is a command from the remote control and it is true analyze the result
  if (irrecv.decode(&results))
  {
    storeCode(&results);
    irrecv.resume(); // resume receiver
  }

  if (mainStatus == ON) {
    if (modeStatus == BOBLIGHT) {
      // if BOBLIGHT mode is selected data from serial and update leds
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
          if ( Serial.available() > sizeOfPrefix - 2 )
          {
            Serial.readBytes(buffer, sizeOfPrefix - 1);
            for ( uint8_t counter = 0; counter < sizeOfPrefix - 1; counter++)
            {
              if ( buffer[counter] == prefix[counter + 1] )
              {
                state = STATE_DO_DATA;     // Received character is in prefix, continue
                currentLED = 0;            // Set current LED to the first one
              }
              else
              {
                state = STATE_WAITING;     // Crap, one of the received chars is NOT in the prefix
              } // end if buffer
            } // end for Counter
          } // end if Serial
          break;
        case STATE_DO_DATA: // *** Process incoming color data ***
          if ( Serial.available() > 2 )      // if I receive more than 2 chars
          {
            Serial.readBytes( buffer, 3 );   // Abuse buffer to temp store 3 charaters
            if (mainStatus == ON && modeStatus == BOBLIGHT)
              setPixel( currentLED++, buffer[0], buffer[1], buffer[2]);  // and assing to LEDs
          }

          if (currentLED == LEDCOUNT) { //if it is the last LED send RGB to the satellite
            digitalWrite(12, HIGH);
            sendSatellite (buffer[0], buffer[1], buffer[2]);
            digitalWrite(12, LOW);
          }

          if ( currentLED > LEDCOUNT )       // Reached the last LED? Display it!
          {
            if (mainStatus == ON && modeStatus == BOBLIGHT) {
              showStrip();
            }
            state = STATE_WAITING;         // Reset to waiting ...
            currentLED = 0;                // and go to LED one
          }
          break;
      } // switch(state)
    }
  }
} // loop


// Sets the color of all LEDs in the strand to 'color'
// If 'wait'>0 then it will show a swipe from start to end
void setAllLEDs(byte r, byte g, byte b, int wait)
{
  for ( int counter = 0; counter < LEDCOUNT; counter++ )  // For each LED
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

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results * results) {
  codeType = results->decode_type;
  codeValue = results->value;
  //Serial.println(codeValue, HEX);
  if (codeType == REMOTE_TYPE) {
    switch (codeValue) {
      case YELLOW: //toggle boblight
        if (mainStatus == ON) {
          if (modeStatus == STATIC)
          {
            LEDS.clear();
            modeStatus = BOBLIGHT;
          }
          else if (modeStatus == BOBLIGHT)
          {
            modeStatus = STATIC;
            state = STATE_WAITING;
            LEDS.clear();
            setColor(DEFAULTP); //set the default static color
            sendSatellite (DEFAULTP, DEFAULTP, 0xFF);
          }
        }
        break;
      case RED: // ON/OFF
        if (mainStatus == ON) {
          mainStatus = OFF;
          setAllLEDs(0x0, 0x0, 0x0, 0);
          sendSatellite (0x0, 0x0, 0x0);
        }
        else if (mainStatus == OFF) {
          mainStatus = ON;
          setColor(DEFAULTP);
          sendSatellite (DEFAULTP, DEFAULTP, 0xFF);
        }
        break;
      case GREEN:
        if (mainStatus == ON) {
          if (modeStatus == STATIC) {
            setColor(codeValue);
          }
        }
        break;
        /*
          case BLUE:
          switch (satelliteStatus) {
            case ON:
              satelliteStatus = OFF;
              break;
            case OFF:
              satelliteStatus = ON;
              break;
          }
          break;
        */
    }
  }
  delay(1); //delay(5);
}

void setColor(uint32_t color = DEFAULTP) {
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
  LEDS.show();
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  LEDS.setPixelColor(Pixel, LEDS.Color(red, green, blue));
}

void sendSatellite (byte red, byte green, byte blue) {

  payload[0] = red >> 8 & 0xff; // High byte - shift bits 8 places, 0xff masks off the upper 8 bits
  payload[1] = red & 0xff;  // low byte, just mask off the upper 8 bits
  payload[2] = green >> 8 & 0xff; // High byte - shift bits 8 places, 0xff masks off the upper 8 bits
  payload[3] = green & 0xff;  // low byte, just mask off the upper 8 bits
  payload[4] = blue >> 8 & 0xff; // High byte - shift bits 8 places, 0xff masks off the upper 8 bits
  payload[5] = blue & 0xff;  // low byte, just mask off the upper 8 bits
  xbee.send(tx);

}


