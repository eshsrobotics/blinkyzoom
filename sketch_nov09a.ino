// -*- c-basic-offset: 2; rainbow-html-colors: t; mode: C++; compile-command: "g++ -DPC_TESTING -x c++ ./sketch_nov09a.ino -o sketch" -*-

// Forward declarations.
struct colorStop;
void updatePixels(float rangeStart, float rangeEnd, float amountVisible,
                  float brightness, bool reversed);
void addColorStop(colorStop stopSet[], int r, int g, int b, float loc);
float linterp(float val, float start, float end);
int round(float f);
void setup();
void loop();


#ifdef PC_TESTING
/////////////////////////////////////////////////
// Simulate the Arduino libraries we're using. //
/////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <string>
class String {
  public:
    String() : s() {}
    String(const std::string& s_) : s(s_) { }
    String(int i) : s() {
      std::stringstream stream;
      stream << i;
      s = stream.str();
    }
    operator std::string() const { return s; }
    friend std::string operator+ (const String& left, const std::string& right) { return left.s + right; }
    friend std::string operator+ (const std::string& left, const String& right) { return left + right.s; }

  private:
    std::string s;
};
struct color {
    color(int r_, int g_, int b_) : r(r_), g(g_), b(b_) { }
    int r, g, b;
};
class _serial {
  public:
    void println(const std::string& s) { std::cout << s << "\n"; }
    void begin(int baud) { }
};
// This doesn't light up any light or actually anything--it just makes the compiler happy.
class Adafruit_NeoPixel {
  public:
    Adafruit_NeoPixel(int, int, int) { }
    void begin() { }
    void show() { }
    void setPixelColor(int position, const color& c) { }
    color Color(int r, int g, int b) { return color(r, g, b); }
};
const int NEO_GRB = 0;
const int NEO_KHZ800 = 0;
_serial Serial;

int main() {
  setup();
  // We should probably call loop() at some point.
  return 0;
}
#else
  //////////////////////////////////////////////////////////
  // This is the code that the Arduino IDE actually uses. //
  //////////////////////////////////////////////////////////

  #include <Adafruit_NeoPixel.h>
  #ifdef __AVR__
    #include <avr/power.h>
  #endif
#endif // #ifdef PC_TESTING

//////////////
// TUNABLES //
//////////////

#define PIN 10                     // digital pin number
#define NUMBER_OF_PIXELS 10        // 92 on each robot side

// These _ADJUST variables allow for adjustment of how brightly each color is
// shown; adjust for color accuracy

#define RED_ADJUST 1.0             // 0 to 1
#define GREEN_ADJUST 1.0//0.55          // 0 to 1
#define BLUE_ADJUST 1.0//0.45           // 0 to 1

// The maximum PWM input will be 255, so the maximum amount of LEDs can only
// be up to 100% of 255
const int MAX_OUTPUT_LIGHTS = 255;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

int colorStopIndex = 0; // max 255

struct colorStop {
  int red;
  int green;
  int blue;
  float location; // 0.0 to 1.0
  colorStop():
    red(0),
    green(0),
    blue(0),
    location(0.0) {}
  colorStop(int r, int g, int b, float loc):
    red(r),
    green(g),
    blue(b),
    location(loc) {}
};

colorStop colorStops[255];

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  // // gradient for two sections of 73 LEDs
  // addColorStop(gradient, 129, 255, 64, 0);  // rgb(129, 255, 64)
  // addColorStop(gradient, 255, 244, 43, 18); // rgb(255, 244, 43)
  // addColorStop(gradient, 255, 177, 64, 27); // rgb(255, 177, 64)
  // // addColorStop(gradient, 255, 7, 99, 45); // rgb(255, 7, 99)
  // addColorStop(gradient, 110, 0, 240, 45);  // rgb(110, 0, 240)

  // // gradient to span all 147 LEDs
  // addColorStop(129, 255, 64, 0);  // rgb(129, 255, 64)
  // addColorStop(255, 244, 43, 59); // rgb(255, 244, 43)
  // addColorStop(255, 177, 64, 88); // rgb(255, 177, 64)
  // addColorStop(255, 7, 99, 146);  // rgb(255, 7, 99)

  // // test gradient for 10 LEDs
  // addColorStop(129, 255, 64, 0); // rgb(129, 255, 64)
  // addColorStop(255, 244, 43, 3); // rgb(255, 244, 43)
  // addColorStop(255, 177, 64, 5); // rgb(255, 177, 64)
  // addColorStop(255, 7, 99, 8);   // rgb(255, 7, 99)

  // test gradient for sections of 5 LEDs
  addColorStop(colorStops, 255, 7, 99, 0.0);   // rgb(255, 7, 99)
  addColorStop(colorStops, 110, 0, 240, 1.0); // rgb(110, 0, 240)

  Serial.begin(9600);

  Serial.println("Color stops:");
  for (int i = 0; i < colorStopIndex; i++) {
    Serial.println("#" + String(i) + " at " +
                   String(colorStops[i].location * 100) + "%: rgb(" +
                   String(colorStops[i].red) + ", " +
                   String(colorStops[i].green) + ", " +
                   String(colorStops[i].blue) + ")");
  }

  Serial.println("~");
  Serial.println("");

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  const float BRIGHTNESS = 1.0;//0.5;
  const float AMOUNT_VISIBLE = 1.0;
  const bool reversed = false;
  updatePixels(0.0, 0.50, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
  updatePixels(0.50, 1.0, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
}

void loop() {

}

// rangeStart and rangeEnd: the portion of the strip on which to display the gradient; max 0 to 1
// amountVisible: the amount of the gradient to display; where to cut the gradient so that the
// remaining of the range is blank; max 255; just the PWM input value in this case
void updatePixels(float rangeStart, float rangeEnd, float amountVisible,
                  float brightness, bool reversed) {

  // sanity checks
  if (colorStopIndex <= 0) {
    Serial.println("You need to add at least one color stop!");
    return;
  }
  if (rangeStart > rangeEnd) { // switch values if supplied in the wrong order
    float temp = rangeStart;
    rangeStart = rangeEnd;
    rangeEnd = temp;
  }
  if (rangeStart < 0.0)    { rangeStart = 0.0; }
  if (rangeEnd > 1.0)      { rangeEnd = 1.0; }
  if (amountVisible < 0.0) { amountVisible = 0.0; }
  if (amountVisible > 1.0) { amountVisible = 1.0; }

  int startIndex  = round(rangeStart * (NUMBER_OF_PIXELS - 1));
  int endIndex    = round(rangeEnd   * (NUMBER_OF_PIXELS - 1));
  int cutoffIndex = round(linterp(amountVisible, startIndex, endIndex));

  Serial.println("Range: " + String(startIndex) + " to " + String(endIndex) + ", cutoff at " + String(cutoffIndex));

  // actual parallel arrays of color values to be sent to the strip
  int r[MAX_OUTPUT_LIGHTS];
  int g[MAX_OUTPUT_LIGHTS];
  int b[MAX_OUTPUT_LIGHTS];

  int outputLightIndex = 0;

  // Act as if we're going to fill the entire light strip with color, from the
  // first color stop to the last.
  for (int i = 0; i < colorStopIndex; ++i) {

    colorStop previousColorStop = colorStops[i];
    colorStop nextColorStop     = colorStops[colorStopIndex == 1 ? i : i + 1];

    // Remember that colorStop locations are specified using normalized values
    // between 0 and 1 (so this algorithm works regardless of the actual
    // number of lights in the strip, and it doesn't matter whether the color
    // stops are evenly distributed.)
    for (int start = round(previousColorStop.location * (NUMBER_OF_PIXELS - 1)),
             end   = round(nextColorStop.location     * (NUMBER_OF_PIXELS - 1)),
             index = start;
         index <= end;
         ++index) {

      // Only fill those lights that the caller actually wanted to fill.
      if (index < startIndex || index > cutoffIndex) {
        continue;
      }

      // Determine the percentage of the way we are along the current gradient.
      float u = float(index - start) / (end - start);

      // A simple linear interpolation gets the color.
      float red   = linterp(u, previousColorStop.red,   nextColorStop.red)   * brightness * RED_ADJUST;
      float green = linterp(u, previousColorStop.green, nextColorStop.green) * brightness * GREEN_ADJUST;
      float blue  = linterp(u, previousColorStop.blue,  nextColorStop.blue)  * brightness * BLUE_ADJUST;

      if (reversed) {

        int reversedIndex = round(linterp(u, cutoffIndex, startIndex));
        r[reversedIndex] = round(red);
        g[reversedIndex] = round(green);
        b[reversedIndex] = round(blue);

      } else {

        r[index] = round(red);
        g[index] = round(green);
        b[index] = round(blue);

      } // end (if not reversed)
    } // end (for each index between the current pair of color stops)
  } // end (for each pair of color stops in sequence)

  for (int i = startIndex; i <= cutoffIndex; i++) {
    strip.setPixelColor(i, strip.Color(r[i], g[i], b[i]));
    Serial.println("Pixel " + String(i) + ": rgb(" + String(r[i]) + ", " + String(g[i]) + ", " + String(b[i]) + ")");
  }
  strip.show();
}

void addColorStop(colorStop stopSet[], int r, int g, int b, float loc) {
  colorStop c(r, g, b, loc);
  stopSet[colorStopIndex++] = c;
}

// Returns a floating-point value that is val% of the way between start and
// end.
float linterp(float val, float start, float end) {
  return(start + val * (end -  start));
}

// Rounds the input to the nearest whole number,
int round(float f) {
  return (f >= 0 ? int(f + 0.5f) : int(f - 0.5f));
}
