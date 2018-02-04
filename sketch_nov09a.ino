// -*- c-basic-offset: 2; rainbow-html-colors: t; mode: C++; compile-command: "g++ -DPC_TESTING -x c++ ./sketch_nov09a.ino -o sketch" -*-

// Forward declarations.
struct colorStop;
void updatePixels(float rangeStart, float rangeEnd, float amountVisible,
                  float brightness, bool reversed);
void addColorStop(colorStop stopSet[], int r, int g, int b, float loc);
float linterp(float val, float start, float end);
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
    String(int i) : s() { std::stringstream stream; stream << i; s = stream.str(); }
    String(float f) : s() { std::stringstream stream; stream << f; s = stream.str(); }

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
// Rounds the input to the nearest whole number.  In the real environment, this is a macro.
int round(float f) {
  return (f >= 0 ? int(f + 0.5f) : int(f - 0.5f));
}
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
#define NUMBER_OF_PIXELS 90        // 92 on each robot side

// These _ADJUST variables allow for adjustment of how brightly each color is
// shown; adjust for color accuracy

#define RED_ADJUST   1.0           // 0 to 1
#define GREEN_ADJUST 0.55          // 0 to 1
#define BLUE_ADJUST  0.45          // 0 to 1

// The maximum PWM input will be 255, so the maximum amount of LEDs can only
// be up to 100% of 255
//
// IMPORTANT: The NUMBER_OF_PIXELS macro may not exceed this value.
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

// A minor wrapper to keep related data together (namely, the array of color
// stops, its current length, and the RGB array corresponding to each pixel.)
class colorStopList {
  private:
    int length_;
    colorStop errorValue;
    colorStop* colorStops;
  public:
    colorStopList(int maxColorStops) : errorValue(6, 6, 6, 0.666), colorStops(new colorStop[NUMBER_OF_PIXELS]), length_(0) { }
    colorStopList(const colorStopList& other) : errorValue(6, 6, 6, 0.666), colorStops(new colorStop[NUMBER_OF_PIXELS]), length_(other.length_) {
      for (unsigned int i = 0; i < length_; ++i) {
        colorStops[i] = other.colorStops[i];
      }
    }
    colorStopList& operator= (const colorStopList& other) {
      if (&other != this) {
        length_ = other.length_;
	delete[] colorStops;
        colorStops = new colorStop[NUMBER_OF_PIXELS];
        for (unsigned int i = 0; i < length_; ++i) {
	  colorStops[i] = other.colorStops[i];
        }
      }
      return *this;
    }
    ~colorStopList() { delete[] colorStops; }
   
      void clear() { length_ = 0; }
      colorStop& operator[] (int index) { return (index >= 0 && index < length_ ? colorStops[index] : errorValue); }
      int length() const { return length_; }
      int size() const { return length_; }
      void add(int r, int g, int b, float location) { add(colorStop(r, g, b, location)); }
      void add(const colorStop& c) {
        if (length_ < NUMBER_OF_PIXELS) {
          colorStops[length_++] = c;

          // Sort the list by location.
          //
          // For a small dataset, the performance should be adequate.  We can
          // have up to 256 color stops, though (which would be a pathological
          // dataset for us.)
          int i = 1;
          while (i < length_) {
            colorStop x = colorStops[i];
            int j = i - 1;
            while (j >= 0 && colorStops[j].location > x.location) {
              colorStops[j + 1] = colorStops[j];
              j -= 1;
            }
            colorStops[j + 1] = x;
            i += 1;
          } // end (insertion sort)
        } // end (if we inserted a color stop)
      }
};


colorStopList colorStops(NUMBER_OF_PIXELS);

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

  // colorStops.add(110, 0, 240, 1.0);   // rgb(110, 0, 240)
  // colorStops.add(255, 7, 99, 0.0);    // rgb(255, 7, 99)
  // colorStops.add(255, 255, 255, 0.5); // rgb(0, 0, 255)
  colorStops.add(255, 0, 0,   0.0); 
  // colorStops.add(255, 128, 0, 0.366);
  colorStops.add(255, 255, 0, .9);
  colorStops.add(0, 128,0,   0.66);  
  colorStops.add(0, 0, 128,   0.4);  
  colorStops.add(128, 0, 128,   1.0);  

  Serial.begin(9600);

  Serial.println("Color stops:");
  for (int i = 0; i < colorStops.length(); i++) {
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

  const float BRIGHTNESS = 0.5;
  const float AMOUNT_VISIBLE = 1.0;
  const bool reversed = false;
  updatePixels(0.0, 0.50, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
  updatePixels(0.50, 1.0, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
}

void loop() {

}

// Renders the given portion of the light strip.
//
// @param rangeStart:    Where shall we begin the rendering?  The values range
//                       from 0.0 (the first light in the strip) to 1.0 (the
//                       last light in the strip.)
// @param rangeStart:    Where shall we stop the rendering?  The values here also
//                       range from 0.0 to 1.0.
// @param amountVisible: What percentage of the range should we render?  1.0
//                       means to render 100% of the range (that is, from
//                       rangeStart to rangeEnd), while a value like 0.37
//                       would render 37% of the range (from rangeStart to
//                       rangeStart + 0.37 * (rangeEnd - rangeStart).)
// @param brightness:    A constant multiplier used to reduce the intensity of
//                       the LEDs.  A value of 1.0 uses the original color
//                       values, while a value of 0.5 would divide the output
//                       R, G, and B values by 2.
// @param reversed:      If true, the final color values will be reversed
//                       before being added to the lights.
void updatePixels(float rangeStart, float rangeEnd, float amountVisible=1.0,
                  float brightness=1.0, bool reversed=false) {

  // Sanity checks.
  if (colorStops.length() <= 0) {
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
  if (brightness < 0.0)    { brightness = 0.0; }
  if (brightness > 1.0)    { brightness = 1.0; }


  int startIndex  = round(rangeStart * (NUMBER_OF_PIXELS - 1));
  int endIndex    = round(rangeEnd   * (NUMBER_OF_PIXELS - 1));
  int cutoffIndex = round(linterp(amountVisible, startIndex, endIndex));

  Serial.println("Range: " + String(startIndex) + " to " + String(endIndex) + ", cutoff at " + String(cutoffIndex));

  // The actual parallel arrays of color values to be sent to the strip.
  int r[NUMBER_OF_PIXELS];
  int g[NUMBER_OF_PIXELS];
  int b[NUMBER_OF_PIXELS];

  int outputLightIndex = 0;

  // Act as if we're going to fill the entire light strip with color, from the
  // first color stop to the last.
  for (int i = 0; i < colorStops.length(); ++i) {

    colorStop previousColorStop, nextColorStop;
    if (colorStops.length() == 1) {
      // Pathological case: one color stop means constant color.
      previousColorStop = nextColorStop = colorStops[i];
      previousColorStop.location = 0.0;
      nextColorStop.location = 1.0;      
    } else {
      previousColorStop = colorStops[i];
      nextColorStop = colorStops[i + 1];
    }    
    
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

        float v = float(index - startIndex) / (cutoffIndex - startIndex);
        int reversedIndex = round(linterp(v, cutoffIndex, startIndex));
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

// Returns a floating-point value that is val% of the way between start and
// end.
float linterp(float val, float start, float end) {
  return(start + val * (end -  start));
}
