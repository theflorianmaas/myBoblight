/* Modified and commented by theflorianmaas
  myHyperionHUB 101
  10/10/2018
  Version 1.0
  Boblight (Momo) and Hyperion (Adalight) support
  Addded - Change background color by a Sony Bravia remote control
    RED button = on/off off=all leds off
    GREEN button = change RGB color preset
    YELLOW button = toggle static / DYNAMIC
    BLUE button = Turn ON/OFF. For Udoo X86 only

  STATIC    set a static color
  DYNAMIC  enable DYNAMIC, color changed based on the screen color

  Supported boards
  Arduino UNO and derivates
  Arduino 101

  Hardware support
  Adafruit Neopixel or WS2812B led strip
  TSOP38283 IR Receiver or similar

  This software is free and can be distributed and/or modified
  freely as long as the copyright notice remains in place.
  Nobody is allowed to charge you for this code.
  Use of this code is entirely at your own risk.
*/

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include "IRremote.h" //with Arduino 101 use this version https://github.com/jimaobian/Arduino-IRremote
#include <XBee.h>

#if !defined (__arc__) //if it is not Arduino 101 use software serial
#include <SoftwareSerial.h>
#endif

// Color preset
//---------------------------------------------------------------------------------------------
byte aRGB[] = { 0, 0, 125 }; //define variables to store RGB color values
//define variables for the RGB color values
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
//---------------------------------------------------------------------------------------------

// change values in this section to configure the sketch
#define RECVPIN             2      // ir led pin
#define DATAPIN             6      // Datapin for WS2812B LED strip
#define RESETPIN            9      // Triggers the power signal. UDOO X86 only
#define LEDCOUNT            75     // SET the number of leds in the strip 
#define BAUDRATE            460800 // Serial port speed
// end section -------------------------------------------

#define BRIGHTNESS          170     // level 0-255 of led brightness
#define PULSETIME           8

/// The prefix for start sending led-data
static const uint8_t prefix[] = {'A', 'd', 'a'};  // Start prefix ADA
char buffer[sizeof(prefix)]; // Temporary buffer for receiving prefix data
uint8_t sizeOfPrefix = sizeof(prefix);

// Remote control Definitions
// Change these values if you have a different remote control. See the IRemote library for reference
//---------------------------------------------------------------------------------------------
#define REMOTE_TYPE SONY
#define RED      0x52E9 // red button
#define GREEN    0x32E9 // green button
#define YELLOW   0x72E9 // yellow button
#define BLUE     0x12E9 // blue button
#define DEFAULTP 0x0    // default color preset
//---------------------------------------------------------------------------------------------

// Remote control Definitions
//---------------------------------------------------------------------------------------------
#define ON        1   // main Status ON
#define OFF       0   // main Status OFF
#define STATIC    10  // Static color mode  
#define DYNAMIC   20  // DYNAMIC mode
//---------------------------------------------------------------------------------------------
// IR definition
//---------------------------------------------------------------------------------------------
IRrecv irrecv(RECVPIN); //init IR receiver
decode_results results;
int codeType = -1; // The type of IR received code
unsigned long codeValue; // The code value if not raw
int mainStatus  = ON; //Set the initial value
int modeStatus  = STATIC; //Set the initial mode
int showDelayMicros = 200; //delay after LEDS.show(), give the time to re-enable the interrupts. Min 50ms to run properly
unsigned long LastTimeIR = millis(); //time of the last received IR signal

//---------------------------------------------------------------------------------------------
// Software serial
//---------------------------------------------------------------------------------------------
#if !defined (__arc__)  //if it is not Arduino 101 use softwareSerial, otherwise Hardware serial1
uint8_t ssRX = 5; //TX of usb-serial device
uint8_t ssTX = 4; //RX of usb-serial device
SoftwareSerial Serial1(ssRX, ssTX);
#endif

void setColor(uint32_t color);

uint8_t state;               // - Define current status
#define STATE_WAITING   1    // - Waiting for prefix
#define STATE_DO_PREFIX 2    // - Processing prefix
#define STATE_DO_DATA   3    // - Handling incoming LED colors

uint8_t currentLED = 0;           // Needed for assigning the color to the right LED

int16_t xbeeData[6]; //array data to transmit RGB to the satellite
XBee xbee = XBee();
uint8_t payload[6];
// 16-bit addressing: Enter address of remote XBee, typically the coordinator
Tx16Request tx = Tx16Request(0xFFFF, payload, sizeof(payload));
TxStatusResponse txStatus = TxStatusResponse();

Adafruit_NeoPixel LEDS = Adafruit_NeoPixel(LEDCOUNT, DATAPIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  pinMode(12, OUTPUT);
  pinMode(RECVPIN, INPUT);
  pinMode(RESETPIN, OUTPUT);
  initIR(); // Start the receiver
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code
  LEDS.begin(); // This initializes the NeoPixel library.
  LEDS.setBrightness(BRIGHTNESS);
  LEDS.show();

  //set initial color
  if (mainStatus == ON && modeStatus == STATIC) {
    setColor(DEFAULTP); //set the default static color
    sendSatellite (0x0, 0x0, 0xFF);
  }
  else if (mainStatus == ON && modeStatus == DYNAMIC) {
    setAllLEDs(0x0, 0x0, 0x0, 0); //turn off all leds
    sendSatellite (0x0, 0x0, 0x0);
  }
  state = STATE_WAITING;    // Initial state: Waiting for prefix

  Serial.begin(BAUDRATE);   // Init serial speed
  Serial1.begin(57600);
  xbee.setSerial(Serial1);
  xbee.begin(Serial1);
  // Send 'Ada' string to host
  Serial.print("Ada\n");
}

void loop()
{
  //check if there is a command from the remote control and it is true analyze the result
  if (irrecv.decode(&results))
  {
    LastTimeIR = millis();
    storeCode(&results);
    irrecv.resume(); // resume receiver
    if (modeStatus == DYNAMIC) {
      showDelayMicros = 50000; //set the delay to re-nable interrupts
    }
    else if (modeStatus == STATIC) {
      showDelayMicros = 200; //set the delay to the minimum
    }
  }

  if (LastTimeIR <= millis() - 2000) //reset the delay after 2 seconds from the last IR received
    showDelayMicros = 200; //set the delay to the minimum

  switch (state)
  {
    case STATE_WAITING:                  // *** Waiting for prefix ***
      // Wait for first byte of 'Ada'
      if ( Serial.available() > 2 )      // if I receive more than 2 chars
      {
        for (int i = 0; i < sizeof(prefix); ++i)
        {
          if (prefix[i] != Serial.read())
            return;
        }
        state = STATE_DO_PREFIX; //if Ada prefix received then next step
      }
      break;

    case STATE_DO_PREFIX:                // *** Processing Prefix ***
      // Wait for highByte, lowByte, Checksum to be available
      if ( Serial.available() > 2 )      // if I receive more than 2 chars
      {
        // Read high and low byte and the checksum
        Serial.readBytes( buffer, 3); //read 3 bytes from serial highByte,lowByte,checksum
        if (buffer[2] != (buffer[0] ^ buffer[1] ^ 0x55)) //check checksum
        {
          // Checksum check failed
          state = STATE_WAITING;
          break;
        }
        else { //checksum OK
          //ledCount = ((highByte & 0x00FF) << 8 | (lowByte & 0x00FF) ) + 1;
          state = STATE_DO_DATA;
        }
      }
      break;

    case STATE_DO_DATA: // *** Process incoming color data ***
      if ( Serial.available() > 2 )      // if I receive more than 2 chars
      {
        Serial.readBytes( buffer, 3);   // Abuse buffer to temp store 3 charaters
        if (currentLED != LEDCOUNT) { //it is NOT the last LED, send RGB to current led
          if (mainStatus == ON && modeStatus == DYNAMIC) {
            setPixel( currentLED++, buffer[0], buffer[1], buffer[2]);  // and assing to LEDs
          }
        }
        else if (currentLED == LEDCOUNT) { //it is the last LED, send RGB to the satellite and show leds
        {
            if (mainStatus == ON && modeStatus == DYNAMIC) 
            {
             showStrip(); // Reached the last LED? Display it!
             satelliteLed(HIGH);
             sendSatellite (buffer[0], buffer[1], buffer[2]);
             satelliteLed(LOW);
            }
            state = STATE_WAITING;         // Reset to waiting ...
            currentLED = 0;   // and go to LED one
        }
      }

  }
  break;

} // switch(state)

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
  sendSatellite (r, g, b);
} // setAllLEDs

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results * results) {
  codeType = results->decode_type;
  codeValue = results->value;
  //Serial.println(codeValue, HEX);
  if (codeType == REMOTE_TYPE) {
    switch (codeValue) {
      case YELLOW: //toggle DYNAMIC
        if (mainStatus == ON) {
          if (modeStatus == STATIC)
          {
            LEDS.clear();
            modeStatus = DYNAMIC;
            showDelayMicros = 200; //DYNAMIC enabled, set the minimum delay
          }
          else if (modeStatus == DYNAMIC)
          {
            LEDS.clear();
            modeStatus = STATIC;
            showDelayMicros = 200; //DYNAMIC enabled, set the minimum delay
            state = STATE_WAITING;
            setColor(DEFAULTP); //set the default static color
            sendSatellite (0x0, 0x0, 0xFF);
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
          setColor(codeValue);
          showDelayMicros = 200; //set the minimum delay
        }
        break;
      case GREEN:
        if (mainStatus == ON) {
          if (modeStatus == STATIC) {
            setColor(codeValue);
          }
        }
        break;
      case BLUE: //TURN ON, TURN OFF. ONLY for UDOO X86
        showDelayMicros = 200; //set the minimum delay
        for (int i = 0; i < 5; i++) {
          digitalWrite(RESETPIN, LOW);
          delay(PULSETIME); /* Reset pin goes LOW for 8ms */
          digitalWrite(RESETPIN, HIGH);
          delay(PULSETIME); /* Reset pin goes HIGH for 8ms */
        }
        break;
    }
  }
}

void setColor(uint32_t color = DEFAULTP) {
  // if a valid command from remote control is received then execute the command, with other values (DEFAULT) set the last selected preset color
  // if the Status=ON and DYNAMIC=Disabled
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
    //show the color
    setAllLEDs(aRGB[0], aRGB[1], aRGB[2], 5);
  }
}

void showStrip() {
  LEDS.show();
  delayMicroseconds(showDelayMicros);  // Wait a few micro seconds
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  LEDS.setPixelColor(Pixel, LEDS.Color(red, green, blue));
}

void sendSatellite (byte red, byte green, byte blue) {
  //send the color to the satellite by xbee
  payload[0] = red >> 8 & 0xff; // High byte - shift bits 8 places, 0xff masks off the upper 8 bits
  payload[1] = red & 0xff;  // low byte, just mask off the upper 8 bits
  payload[2] = green >> 8 & 0xff; // High byte - shift bits 8 places, 0xff masks off the upper 8 bits
  payload[3] = green & 0xff;  // low byte, just mask off the upper 8 bits
  payload[4] = blue >> 8 & 0xff; // High byte - shift bits 8 places, 0xff masks off the upper 8 bits
  payload[5] = blue & 0xff;  // low byte, just mask off the upper 8 bits
  xbee.send(tx);
}

void initIR()
{
  irrecv.enableIRIn(); // Start the receiver
  irrecv.blink13(true);
}

void satelliteLed(int sts) {
  digitalWrite(12, sts);
}


