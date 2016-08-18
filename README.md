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




