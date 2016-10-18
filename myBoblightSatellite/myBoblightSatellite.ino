/* Modified and commented by Fra.par
  myBoblightSatellite
  17/10/2016

  BOBLIGHT  enable Boblight, color changed based on the screen color

  myBoblightSatellite is free software and can be distributed and/or modified
  freely as long as the copyright notice remains in place.
  Nobody is allowed to charge you for this code.
  Use of this code is entirely at your own risk.
*/

#include "FastLED.h"                                          // FastLED library.
#include <XBee.h>
#include <SoftwareSerial.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

//DEFINITIONS

byte aRGB[] = { 0, 0, 125 }; //define variables to store RGB color values

#define DATAPIN             6      // Datapin for WS2812B LED strip
#define LEDCOUNT            50     // Number of LEDs used for boblight left 16, top 27, right 16
#define SHOWDELAY           200    // Delay in micro seconds before showing default 200
#define BAUDRATE            38400 //115200 // Serial port speed
#define LEDTYPE             WS2812B
#define COLORORDER          GRB
#define BRIGHTNESS          100
#define FRAMES_PER_SECOND   120

struct CRGB leds[LEDCOUNT];

void setColor(uint32_t color);
int currentLED;           // Needed for assigning the color to the right LED

// Satellite config
// Define SoftSerial TX/RX pins
uint8_t ssRX = 5; //TX of usb-serial device
uint8_t ssTX = 4; //RX of usb-serial device
// Remember to connect all devices to a common Ground: XBee, Arduino and USB-Serial device
SoftwareSerial XbeeSerial(ssRX, ssTX);
int16_t xbeeData[6]; //array data to transmit RGB to the satellite
XBee xbee = XBee();
uint8_t payload[6];
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();
uint8_t option = 0;

void setup()
{

  FastLED.addLeds<LEDTYPE, DATAPIN, COLORORDER>(leds, LEDCOUNT).setCorrection(TypicalLEDStrip);  // Use this for WS2812B
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  Serial.begin(9600);   // Init serial speed
  xbee.setSerial(XbeeSerial);
  XbeeSerial.begin(BAUDRATE);
  xbee.begin(XbeeSerial);

}

void loop()
{

  xbee.readPacket();
  if (xbee.getResponse().isAvailable()) {
    // got something
    Serial.println("Ricevuto qualcosa");
    if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) {
      // got a rx packet
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
        xbee.getResponse().getRx16Response(rx16);
        option = rx16.getOption();
        //data = rx16.getData(0);
        int dataLength = rx16.getDataLength();
        for (int i = 0; i < dataLength; i = i + 2)
        {
          aRGB[i / 2]  = rx16.getData(i) << 8;
          aRGB[i / 2] |= rx16.getData(i + 1);

        }
        setAllLEDs(aRGB[0], aRGB[1], aRGB[2]);
      }
    }
  }
} // loop


// Sets the color of all LEDs in the strand to 'color'
// If 'wait'>0 then it will show a swipe from start to end
void setAllLEDs(byte r, byte g, byte b)
{
  for ( int counter = 0; counter < LEDCOUNT; counter++ )    // For each LED
  {
    setPixel(counter, r, g, b);  // .. set the color
  } // for Counter
  showStrip();    // Show the LED color
} // setAllLEDs


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






