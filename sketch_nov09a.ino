// -*- mode: C++; compile-command: "g++ -DPC_TESTING -x c++ ./sketch_nov09a.ino -o sketch" -*-
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
  class _serial {
      public:
          void println(const std::string& s) { std::cout << s << "\n"; }
          void begin(int unknown) { }
  };
  class Adafruit_NeoPixel {
      public:
          Adafruit_NeoPixel(int, int, int) { }
          void begin() { }
          void show() { }
          void setPixelColor(int position, int r, int g, int b) { }
  };
  const int NEO_GRB = 0;
  const int NEO_KHZ800 = 0;
  typedef bool boolean;
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

#define PIN 6

// Forward declarations.
void sendToStrip(int begin, int end, boolean reversed);
void addColorStop(int r_, int g_, int b_, int location_);
int linterp(int val, int smin, int smax, int min, int max);
void setup();
void loop();

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(147, PIN, NEO_GRB + NEO_KHZ800);

int colorStopIndex = 0; // max 255
int reds [255];
int greens [255];
int blues [255];
int locations[255];

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  addColorStop(129, 255, 64, 0);
  addColorStop(255, 244, 43, 89);
  addColorStop(255, 177, 64, 153);
  addColorStop(255, 7, 99, 255);

  Serial.begin(9600);

  Serial.println("Color stops:");
  for (int i = 0; i < colorStopIndex; i++) {
    Serial.println(String(i) + " at " + String(locations[i]) + ": " + String(reds[i]) + ", " + String(greens[i]) + ", " + String(blues[i]));
  }
  Serial.println("~");
  Serial.println("");

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  updatePixels(10);
}

void loop() {

}



// void sendToStrip(int begin, int end, boolean reversed) {
//   int length = end - begin;
//   int r[length];
//   int g[length];
//   int b[length];
//
//   int ibegin;
//   int iend;
//
//   if (!reversed) {
//     ibegin = 0;
//     iend = colorStopIndex;
//   } else {
//     ibegin = colorStopIndex;
//     iend = 0;
//   }
//
//   for (int i = ibegin; i < iend - 1; i++) {
//     int loc = linterp(locations[i], 0, 255, 0, length);
//     int nextLoc = linterp(locations[i + 1], 0, 255, 0, length);
//     Serial.println(String(loc) + " to " + String(nextLoc));
//     r[loc] = reds[i];
//     g[loc] = greens[i];
//     b[loc] = blues[i];
//
//     Serial.println(String(i) + ": " + String(r[loc]) + ", " + String(g[loc]) + ", " + String(b[loc]));
//
//     for (int j = loc; j < nextLoc; ++j) {
//       r[j] = linterp(j, loc, nextLoc, r[loc], reds[i + 1]);
//       g[j] = linterp(j, loc, nextLoc, g[loc], greens[i + 1]);
//       b[j] = linterp(j, loc, nextLoc, b[loc], blues[i + 1]);
//
//       Serial.println(String(i) + ": " + String(r[j]) + ", " + String(g[j]) + ", " + String(b[j]));
//     }
//   }
//
//   for (int i = begin; i <= end; i++) {
//     int loc = i - begin;
//     strip.setPixelColor(i, r[loc], g[loc], b[loc]);
//   }
// }

void updatePixels(boolean reversed) {
  const int MAX_OUTPUT_LIGHTS = 255; // the maximum PWM input will be 255, so the maximum amount of steps can only be up to 100% of 255

  int r[MAX_OUTPUT_LIGHTS];
  int g[MAX_OUTPUT_LIGHTS];
  int b[MAX_OUTPUT_LIGHTS];

  int outputLightIndex = 0;

  for (int i = 0; i < colorStopIndex - 1; i++) {
    float steps = locations[i + 1] - locations[i];
    for (float u = 0; u <= 1; u += 1 / steps) {
      float red = linterp(u, reds[i], reds[i + 1]);
      float green = linterp(u, greens[i], greens[i + 1]);
      float blue = linterp(u, blues[i], blues[i + 1]);

      r[outputLightIndex] = int(red);
      g[outputLightIndex] = int(green);
      b[outputLightIndex] = int(blue);
      outputLightIndex++;
    }
  }

  for (int i = 0; i < outputLightIndex; i++) {
    int a;
    if (!reversed) {
      a = i;
    } else {
      a = outputLightIndex - 1 - i;
    }

    strip.setPixelColor(a, r[a], g[a], b[a]);
    Serial.println("Pixel " + String(a) + ": " + String(r[a]) + ", " + String(g[a]) + "," + String(b[a]));
  }
}

void addColorStop(int r_, int g_, int b_, int location_) {
  reds[colorStopIndex] = r_;
  greens[colorStopIndex] = g_;
  blues[colorStopIndex] = b_;
  locations[colorStopIndex] = location_;
  colorStopIndex++;
}

float linterp(float val, float start, float end) {
  return(start + val * (end -  start));
}
