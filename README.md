# myBoblight
Arduino sketch to drive a WS2812B LEDs strip with Boblight and static colors.

It uses the FASTLED and IRemote libraries.

You can toggle (enable/disable) Boblight by a Sony Bravia Remote Control, set a static color or a color pattern.

Need a IR receiver like a TSOP31238 to enable this feature.

###Technical overview

This sketch has been tested on Arduino Uno and Aduino Duemilanove. 
It should work on others models, but you have to change the ir receiver pin, according to the interrupt configuration.

The current sketch uses the pin 2 and the interrupt 0.

Pin 2 = ir receiver
Pin 6 = Led strip data

![]({{site.baseurl}}//myBoblightIr3.png)

If the number of leds is huge the +5v output on Arduino is not able to supply enought current. In this case an external power source is connected in parallel.

The used remote control is a Sony. You can try with different brands changing the brand name and the key codes. See the IRremote library for further details.

#define REMOTETYPE SONY
#define RED     	0x52E9 // red button
#define GREEN   	0x32E9 // green button
#define YELLOW  	0x72E9 // yellow button
#define BLUE    	0x12E9 // blue button_


### Usage

![]({{site.baseurl}}//remotecontrol.png)

RED 	On/Off  Off=all leds off
GREEN	Change the static color when the Static function is selected
YELLOW	Function selector. Static Color/Boblight/Pattern 
BLUE	Change the pattern when the Pattern function is selected


