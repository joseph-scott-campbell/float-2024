# Firmware Documentation

# Goals

The float needs to be able to do the following tasks
- [x] Complete profiles on command (control motor driver)
- [ ] Measure depth of the bottom of the float accurately
- [x] Communicate depth/time back to surface station after each profile
- [x] Communicate information about the team and the current UTC time to surface

# Libraries Being Used

## [ArduinoWebsockets](https://www.arduino.cc/reference/en/libraries/websockets/)

This is being used for communication between the float and the surface station. I did some surface level auditing of this library and it is very well written.

The surface station is using the Python WebSockets library to communicate.

## [WiFi](https://www.arduino.cc/reference/en/libraries/wifi/)

This is the default WiFi library, it is a dependency of ArudinoWebsockets. It is unfortunately no longer maintained, and with enough time we will hopefully be able to move away from it.

## [Adafruit_Neopixel](https://github.com/adafruit/Adafruit_NeoPixel)

This is a library to control Neopixel LEDs. The indicator LED on the Adafruit Py QT board we are using is a Neopixel, allowing us to set different colors for the LED.

# Interrupt Routines

## record_depth()

This function takes the depth from the depth sensor and mutates a global array at the next index with the current depth.

This ISR is triggered every 5 seconds.

## hall_effect()

This function writes the `ENABLE` pin to `LOW` and sets a global flag saying the hall effect sensor has been triggered.

This stops the motor from running immediately, and allows other code doing a vertical profile know when it is time to switch directions.

# Structure of Code

At the top of the file preprocessing directives are global variables are declared.

The functions are written in a chronological order, with ISR functions are put in the beginning of the file because those can happen at any time.
