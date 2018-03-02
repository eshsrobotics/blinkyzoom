// -*- mode: C++; compile-command: "g++ -DPC_TESTING -x c++ ./sketch_nov09a.ino -o sketch" -*-

// Forward declarations.
struct colorStop;
void updatePixels(float rangeStart, float rangeEnd, float amountVisible,
		  float brightness, bool reversed);
void addColorStop(colorStop stopSet[], int r, int g, int b, float loc);
void drawPixels(int r[], int g[], int b[], int cutoffIndex, bool reversed, int outputLightIndex);
float linterp(float val, float start, float end);
void setup();
void loop();

void swap(int& a, int& b) {
	int temp = a;
	a = b;
	b = temp;
}


#ifdef PC_TESTING
// Simulate the Arduino libraries we're using.
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
  int r;
  int g;
  int b;
};
class _serial {
public:
  void println(const std::string& s) { std::cout << s << "\n"; }
  void begin(int baud) { }
};
// This doesn't light up any light or actually anything--it just makes the compile happy.
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
// This is the code that the Arduino IDE actually uses.
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#endif // #ifdef PC_TESTING

// TUNABLES:
#define PIN 10 // digital pin number
#define NUMBER_OF_PIXELS 10 // 92 on each robot side
// These _ADJUST variables allow for adjustment of how brightly each color is shown; adjust for color accuracy
#define RED_ADJUST 1.0 // 0 to 1
#define GREEN_ADJUST 0.55 // 0 to 1
#define BLUE_ADJUST 0.45 // 0 to 1


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
int r[MAX_OUTPUT_LIGHTS];
int g[MAX_OUTPUT_LIGHTS];
int b[MAX_OUTPUT_LIGHTS];

unsigned long initMillis;
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
  addColorStop(colorStops, 0, 0, 255, 1.0); // rgb(0, 0, 255)

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
	rotatePixels(r, g, b, 0,  NUMBER_OF_PIXELS - 1, 3);
	drawPixels(r, g, b, NUMBER_OF_PIXELS, false, NUMBER_OF_PIXELS);
}

void loop() {

}

// rangeStart and rangeEnd: the portion of the strip on which to display the gradient; max 0 to 1
// amountVisible: the amount of the gradient to display; where to cut the gradient so that the remainder of the range is blank; max 255; just the PWM input value in this case
void updatePixels(float rangeStart, float rangeEnd, float amountVisible, float brightness, bool reversed) {

  // sanity checks
  // TODO: complete these
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

  int startIndex = int(rangeStart * NUMBER_OF_PIXELS);
  int endIndex = int(rangeEnd * NUMBER_OF_PIXELS);
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

// updatePixels() and drawPixels() are separate to allow pixels
// to be calculated without immediately sending them to the strip.
void drawPixels(int r[], int g[], int b[], int cutoffIndex, bool reversed, int numberOfPixels) {
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

void rotatePixels(int r[], int g[], int b[], int startIndex, int endIndex, int offset) {
	for (int i = startIndex; i <= ceil((startIndex + endIndex) / 2.0); ++i) {
		int targetIndex = i + offset;
		if (targetIndex < startIndex) {
			targetIndex += (endIndex - startIndex);
		}
		if (targetIndex > endIndex) {
			targetIndex -= (endIndex - startIndex);
		}
		swap(r[i], r[targetIndex]);
		swap(g[i], g[targetIndex]);
		swap(b[i], b[targetIndex]);
	}
}

void addColorStop(colorStop stopSet[], int r, int g, int b, float loc) {
  colorStop c(r, g, b, loc);
  stopSet[colorStopIndex++] = c;
}

float linterp(float val, float start, float end) {
  return(start + val * (end -  start));
}

// float abs(float a) {
//   if (a < 0) {
//     return -a;
//   } else {
//     return a;
//   }
// }
