# myBoblight

This is an enanhancement of the project described here:

[http://www.tweaking4all.com/home-theatre/xbmc/xbmc-boblight-openelec-ws2811-ws2812/](http://www.tweaking4all.com/home-theatre/xbmc/xbmc-boblight-openelec-ws2811-ws2812/)

Arduino sketch to drive a **WS2812B** LEDs strip with Boblight or selected static colors.
As additional feature it integrates a IR Receiver in order to change the backlight configuration by a TV remote control. 

You can toggle (enable/disable) Boblight by a Sony Bravia Remote Control, set a static color or a color pattern.

Need a IR receiver like a **TSOP31238** to enable this feature.

###Technical overview

This sketch has been tested on Arduino Uno and Aduino 101. 
It should work on others models, but you have to change the ir receiver pin, according to the interrupt configuration.
The current sketch uses the pin 2 and the interrupt 0.

It needs the **FASTLED** and **IRemote** libraries.

####Pin configuration

Pin 2 = ir receiver
Pin 6 = Led strip data

![](https://raw.githubusercontent.com/theflorianmaas/myBoblight/master/images/myBoblight_bb.png)

If the number of leds is huge the +5v output on Arduino is not able to supply enought current. In this case an external power source is connected in parallel.

The used remote control is a Sony. You can try with different brands changing the brand name and the key codes. See the IRremote library for further details.

- "#define REMOTE_TYPE	SONY"
- "#define RED     		0x52E9 // red button"
- "#define GREEN   		0x32E9 // green button"
- "#define YELLOW  		0x72E9 // yellow button"
- "#define BLUE    		0x12E9 // blue button"


### Usage

![](https://github.com/theflorianmaas/myBoblight/blob/master/images/remotecontrol.png?raw=true)

- **RED** 		On/Off  Off=all leds off
- **GREEN**		Change the static color when the Static mode is selected
- **YELLOW**	Mode selector. Static Color/Boblight/Pattern 
- **BLUE**		Change the pattern when the Pattern mode is selected


Static mode
![](https://github.com/theflorianmaas/myBoblight/blob/master/images/static.png?raw=true)

Boblight mode
![](https://github.com/theflorianmaas/myBoblight/blob/master/images/boblight.png?raw=true)

Pattern mode
![](https://github.com/theflorianmaas/myBoblight/blob/master/images/pattern.png?raw=true)

### Arduino 101
With Arduino 101 use this IRremote library https://github.com/jimaobian/Arduino-IRremote

Replace Fastled with Adafruit Neopixel library


