# Firmware Documentation

# Goals

The float needs to be able to do the following tasks
- [ ] Complete profiles on command (control motor driver)
- [ ] Measure depth of the bottom of the float accurately
- [ ] Communicate depth/time back to surface station after each profile
- [ ] Communicate information about the team and the current UTC time to surface

# Libraries Being Used

## [ArduinoWebsockets](https://www.arduino.cc/reference/en/libraries/websockets/)

This is being used for communication between the float and the surface station. I did some surface level auditing of this library and it is very well written.

The surface station is using the Python WebSockets library to communicate.

## [WiFi](https://www.arduino.cc/reference/en/libraries/wifi/)

This is the default WiFi library, it is a dependency of ArudinoWebsockets. It is unfortunately no longer maintained, and with enough time we will hopefully be able to move away from it.

## [Adafruit_Neopixel](https://github.com/adafruit/Adafruit_NeoPixel)

This is a library to control Neopixel LEDs. The indicator LED on the Adafruit Py QT board we are using is a Neopixel, allowing us to set different colors for the LED.
