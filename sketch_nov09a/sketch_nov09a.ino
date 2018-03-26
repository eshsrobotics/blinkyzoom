// -*- mode: C++; c-basic-offset: 2; rainbow-html-colors: t; compile-command: "g++ -DPC_TESTING -x c++ ./sketch_nov09a.ino -o sketch" -*-

// Forward declarations.
struct colorStop;
void updatePixels(float rangeStart, float rangeEnd, float amountVisible,
                  float brightness, bool reversed);
void addColorStop(colorStop stopSet[], int r, int g, int b, float loc);
void drawPixels(unsigned char r[], unsigned char g[], unsigned char b[],
                int cutoffIndex, bool reversed, int outputLightIndex);
void rotatePixels(unsigned char r[], unsigned char g[], unsigned char b[],
                  int startIndex, int endIndex, int offset);
float linterp(float val, float start, float end);
void setup();
void loop();
void removeAllColorStops();

void swap(int& a, int& b) {
  int temp = a;
  a = b;
  b = temp;
}


#ifdef PC_TESTING

  /////////////////////////////////////////////////
  // Simulate the Arduino libraries we're using. //
  /////////////////////////////////////////////////

  #include <iostream>
  #include <ostream>
  #include <sstream>
  #include <string>
  #include <cmath>
  #include <ctime>
  using std::ceil;

  // Returns the number of milliseconds since the program began, just like the
  // real millis() function does (see
  // https://www.arduino.cc/reference/en/language/functions/time/millis/).
  //
  // This function isn't thread-safe, though it could easily be made to be
  // with a state argument.
  double millis() {
    static clock_t lastClockTicks = 0;
    if (lastClockTicks == 0) {
      return 0;
    }
    double oldClockTicks = lastClockTicks;
    lastClockTicks = clock();
    return (lastClockTicks - oldClockTicks) / (1000.0 * CLOCKS_PER_SEC);
  }

  class String {
    public:
      String() : s() {}
      String(const std::string& s_) : s(s_) { }
      String(int i) : s() { std::stringstream stream; stream << i; s = stream.str(); }
      String(float f) : s() { std::stringstream stream; stream << f; s = stream.str(); }
      String(unsigned long l) : s() { std::stringstream stream; stream << l; s = stream.str(); }

      operator std::string() const { return s; }
      friend std::string operator+ (const String& left, const std::string& right) { return left.s + right; }
      friend std::string operator+ (const std::string& left, const String& right) { return left + right.s; }

    private:
      std::string s;
  };
  struct color {
      color(int r_, int g_, int b_) : r(r_), g(g_), b(b_) { }
      int r;
      int g;
      int b;
  };
  class _serial {
    public:
      void println(const std::string& s) { std::cout << s << std::endl; }
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
#define PIN 10 // digital pin number
#define NUMBER_OF_PIXELS 20 // 92 on each robot side
// These _ADJUST variables allow for adjustment of how brightly each color is shown; adjust for color accuracy
#define RED_ADJUST 1.0 // 0 to 1
#define GREEN_ADJUST 0.55 // 0 to 1
#define BLUE_ADJUST 0.45 // 0 to 1
#define STATE_STANDARD_ANIMATION_START 1
#define STATE_ANIMATE 2
#define STATE_DELAY 3

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

const int MAX_OUTPUT_LIGHTS = 255; // the maximum PWM input will be 255, so the maximum amount of LEDs can only be up to 100% of 255; we'd like to avoid scaling the gradient up, only scale it down

// actual parallel arrays of color values to be sent to the strip
// hoisted to be global so that they can also be utilized by functions other than updatePixels
unsigned char r[MAX_OUTPUT_LIGHTS];
unsigned char g[MAX_OUTPUT_LIGHTS];
unsigned char b[MAX_OUTPUT_LIGHTS];

unsigned long initMillis;
// This variable tracks our current state in the state machine that controls
// animations during the loop function.
int state;

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
// colorStop rightGradient[255];
// colorStop leftGradient[255];

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  initMillis = millis();

  // cycler
  int cyclerIteration = 0;


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

  // test gradient for 2 sections of 5 LEDs
  addColorStop(colorStops, 255, 0, 0, 0.0); // rgb(255, 0, 0)
  addColorStop(colorStops, 0, 255, 0, 0.5); // rgb(0, 255, 0)
  addColorStop(colorStops, 0, 0, 255, 0.7); // rgb(0, 0, 255)
  addColorStop(colorStops, 255, 255, 0, 1.0); // rgb(255, 255,0)

  Serial.begin(9600);

  Serial.println("Color stops:");
  for (int i = 0; i < colorStopIndex; i++) {
    Serial.println("#" + String(i) + " at " + String(colorStops[i].location) + ": rgb(" + String(colorStops[i].red) + ", " + String(colorStops[i].green) + ", " + String(colorStops[i].blue) + ")");
  }

  Serial.println("~");
  Serial.println("");

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'


  // updatePixels(0.0, 1.0, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
  // updatePixels(0.0, 0.50, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
  // updatePixels(0.50, 1.0, AMOUNT_VISIBLE, BRIGHTNESS, reversed);

  const float BRIGHTNESS = 1.0;
  const float AMOUNT_VISIBLE = 1.0;
  const bool reversed = false;

  updatePixels(0.0, 1.0, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
  Serial.println("Rotating.");
  rotatePixels(r, g, b, 0,  NUMBER_OF_PIXELS - 1, 3);
  drawPixels(r, g, b, NUMBER_OF_PIXELS, false, NUMBER_OF_PIXELS);
  state = STATE_STANDARD_ANIMATION_START;
}

void loop() {
  const float BRIGHTNESS = 1.0;
  const float AMOUNT_VISIBLE = 1.0;
  const bool reversed = false;

  switch (state) {
    // This State's purpose is to initialise a colorful rainbow gradient.
    case STATE_STANDARD_ANIMATION_START:
      removeAllColorStops();
      addColorStop(colorStops, 255, 60, 0, 0.0); // rgb(255, 60, 0)
      addColorStop(colorStops, 0, 255, 0, 0.33); // rgb(0, 255, 0)
      addColorStop(colorStops, 0, 100, 255, 0.66); // rgb(0, 100, 255)
      addColorStop(colorStops, 255, 255, 0, 1.0); // rgb(255, 255,0)
      updatePixels(0.0, 1.0, AMOUNT_VISIBLE, BRIGHTNESS, reversed);
      state = STATE_ANIMATE;
      break;

      // This state is responsible for taking these color values,
      // sending them to the Arduino and rotating them.
    case STATE_ANIMATE:
      rotatePixels(r, g, b, 0,  NUMBER_OF_PIXELS - 1, 1);
      drawPixels(r, g, b, NUMBER_OF_PIXELS, false, NUMBER_OF_PIXELS);
      state = STATE_DELAY;
      break;

      // The purpose of this state is to wait
    case STATE_DELAY:

      break;

  }
}


// Determines the colors for all of the gradients between our registered
// colorstops.  The RGB values are stored in ::r[], ::g[], and ::b[] for
// safekeeping (and for later calls to updatePixels().)
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

void updatePixels(float rangeStart, float rangeEnd, float amountVisible,
                  float brightness, bool reversed) {

  // sanity checks
  if (rangeStart > rangeEnd) { // switch values if supplied in the wrong order
    float temp = rangeStart;
    rangeStart = rangeEnd;
    rangeEnd = temp;
  }
  if (rangeStart < 0.0) {
    rangeStart = 0.0;
  }
  if (rangeEnd > 1.0) {
    rangeEnd = 1.0;
  }
  if (amountVisible < 0.0) {
    amountVisible = 0.0;
  }
  if (amountVisible > 1.0) {
    amountVisible = 1.0;
  }

  int startIndex  = int(rangeStart * NUMBER_OF_PIXELS);
  int endIndex    = int(rangeEnd   * NUMBER_OF_PIXELS);
  int cutoffIndex = int(linterp(amountVisible, startIndex, endIndex));

  Serial.println("Range: " + String(startIndex) + " to " + String(endIndex) + ", cutoff at " + String(cutoffIndex));

  int outputLightIndex = 0;

  for (int i = startIndex; i <= endIndex; i++) {
    int deltaIndex = endIndex - startIndex;
    if (deltaIndex < 0) {
      deltaIndex = -deltaIndex;
    }
    float deltaLocation = colorStops[i + 1].location - colorStops[i].location;
    if (deltaLocation < 0) {
      deltaLocation = -deltaLocation;
    }
    float steps = deltaLocation * (endIndex);
    if (steps == 0) {
      // if steps is zero, there is no change in location and the gradient has a duplicate stop
      break;
    }

    // Serial.println("steps = " + String(steps));
    // Serial.println("-> " + String(1.0 / steps));
    // delay(100000);

    for (float u = 0.0; u <= 1.0; u += 1.0 / steps) {
      float red = linterp(u, colorStops[i].red, colorStops[i + 1].red);
      float green = linterp(u, colorStops[i].green, colorStops[i + 1].green);
      float blue = linterp(u, colorStops[i].blue, colorStops[i + 1].blue);

      if (outputLightIndex + startIndex < MAX_OUTPUT_LIGHTS) {
        int index = outputLightIndex + startIndex;
        r[index] = int(red * brightness  * RED_ADJUST);
        g[index] = int(green * brightness * GREEN_ADJUST);
        b[index] = int(blue * brightness * BLUE_ADJUST);
        outputLightIndex++;

        // Serial.println("Pixel " + String(index) + "before:: r:" + String(r[index]) + ", " + String(g[index]) + ", " + String(b[index]));
      }

      // Serial.println("outputLightIndex = " + String(outputLightIndex) + "; u = " + String(u));
    } // end (for each pixel in the gradient)
  } // (end for each color stop)

  drawPixels(r, g, b, cutoffIndex, reversed, outputLightIndex);
}

// Renders the RGB values from the given parallel arrays onto the actual light
// strip.
//
// There's a reason this function is separate from updatePixels(): sometimes,
// we just want the RGB values so that we can rotate or otherwise animate them
// before rendering.
//
// @param r              An array of MAX_OUTPUT_LIGHTS bytes representing the
//                       current red values that we want to render on the
//                       light strip.
// @param g              Ditto for green.
// @param b              Ditto for blue.
// @param cutoffIndex    drawPixels() will only touch the lights from 0 to
//                       cutoffIndex - 1, so you can use this to cut the
//                       gradients short.  (TODO: instead of just having a
//                       cutoffIndex, which doesn't make much sense [why must
//                       we always start at 0?], have both a startIndex and a
//                       count.
// @param reversed       If true, the order of rendered pixels will be
//                       reversed.
// @param numberOfPixels A not-so-useful parameter that is only used when
//                       reversed==true.  For now, you should always set this
//                       to NUMBER_OF_PIXELS.)

void drawPixels(unsigned char r[], unsigned char g[], unsigned char b[], int cutoffIndex, bool reversed, int numberOfPixels) {
  for (int i = 0; i < cutoffIndex; i++) {
    int a;
    if (!reversed) {
      a = i;
    } else {
      a = numberOfPixels - 1 - i;
    }

    strip.setPixelColor(a, strip.Color(r[i], g[i], b[i]));
    Serial.println("Pixel " + String(a) + ": rgb(" + String(r[i]) + ", " + String(g[i]) + ", " + String(b[i]) + ")");
  }

  strip.show();
}

// Shifts the R, G, and B values within the given range of the given parallel
// arrays by the given offset.
//
// @param r          An array of MAX_OUTPUT_LIGHTS bytes representing the
//                   current red values that we want to render on the
//                   light strip.
// @param g          Ditto for green.
// @param b          Ditto for blue.
// @param startIndex The start of the range that our shifting should affect,
//                   inclusive.
// @param endIndex   The end of the range that our shifting should affect,
//                   inclusive.
// @param offset     The value to shift the lights by.  This can be positive,
//                   negative, or 0.

void rotatePixels(unsigned char r[], unsigned char g[], unsigned char b[],
                  int startIndex, int endIndex, int offset) {
  // To save space, rotate one color channel at a time.
  unsigned char buffer[MAX_OUTPUT_LIGHTS];
  for (int i = 0; i<MAX_OUTPUT_LIGHTS; ++i) { buffer[i]=99; }

  for (int channel = 0; channel < 3; ++channel) {
    unsigned char* source = 0;
    switch (channel) {
      case 0:
        source = r; break;
      case 1:
        source = g; break;
      case 2:
        source = b; break;
    }

    // Place all the pixels in the appropriate locations within the temporary buffer.
    for (int i = startIndex; i <= endIndex; ++i) {

      // The target index wraps around, whether the offset is
      // positive or negative.
      int targetIndex = i + offset;
      if (targetIndex < startIndex) {
        targetIndex += (endIndex - startIndex + 1);
      }
      if (targetIndex > endIndex) {
        targetIndex -= (endIndex - startIndex + 1);
      }
      // Serial.println("i = " + String(i) + ", offset = " + String(offset) + ", targetIndex = " + String(targetIndex));
      buffer[targetIndex] = source[i];
    }

    // Copy the temporary buffer back to the source.
    for (int i = startIndex; i <= endIndex; ++i) {
      source[i] = buffer[i];
    }
  }
}

void addColorStop(colorStop stopSet[], int r, int g, int b, float loc) {
  colorStop c(r, g, b, loc);
  stopSet[colorStopIndex++] = c;
}

void removeAllColorStops() {
  colorStopIndex = 0;
}

float linterp(float val, float start, float end) {
  return(start + val * (end -  start));
}
