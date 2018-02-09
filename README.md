# blinkyzoom
Arduino code to control NeoPixel strips using a set color gradient and a PWM input representing robot speed

You can download the AdaFruit NeoPixel dependency library using the Arduino IDE:
* Does Sketch → Import Library → Library Manager exist?  If so:
  1. Open that dialog.
  2. Type "neopixel" into the search box and select the AdaFruit NeoPixel library. 
* If not, you need to (install the library)[http://arduino.cc/en/Guide/Libraries
] by hand:
  1. git clone -v https://github.com/adafruit/Adafruit_NeoPixel.git
  2. Move the `Adafruit_NeoPixel` folder into ~/sketchbook/libraries (on Linux.)
     * Note that using Sketch → Import Library → Add Library and browsing to the  `Adafruit_NeoPixel` folder accomplishes the same thing.

# Compiling without Arduino

The code can compile and run just fine on its own using the standard
g++ compiler.  Just define the `PC_TESTING` macro to stub out the
Arduino functions and classes:

```
g++ -DPC_TESTING -x c++ ./sketch_nov09a.ino -o sketch
```

You should still be able to use `Serial.println()` to debug color values.
