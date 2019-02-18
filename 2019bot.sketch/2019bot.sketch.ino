#include <Adafruit_NeoPixel.h>
#include <math.h>

// The PWM pin on the Arduino that's sending (non-pwm) signals to the light strips.
#define ARDUINO_DIGITAL_INPUT_PIN 8

// The number of WS2812B lights on the light strip that we're driving.
//
// You can use a number smaller than the actual number.  Values above 255 are ignored.
#define NUMBER_OF_LIGHTS 19

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_LIGHTS, ARDUINO_DIGITAL_INPUT_PIN, NEO_GRB + NEO_KHZ800);

struct RGB {
  unsigned int r, g, b;
  RGB() : r(0), g(0), b(0) { }
  RGB(int red, int green, int blue) : r(red), g(green), b(blue) { }
  friend RGB hsv_to_rgb(float hue, float saturation, float value);

  friend void rgb_to_hsv(RGB rgb, float& hue, float& saturation, float& value) {
    float r = rgb.r / 255, g = rgb.g / 255, b = rgb.b / 255;
    float largest = max(r, max(g, b));
    float smallest = min(r, min(g, b));
    if (largest == smallest) {

      // Neutral axis.  Hue is technically undefined.
      hue = 0;
      saturation = 0;

    } else {

      const float angle = 2 * PI / 6;
      if (largest == r) {
        hue = angle * (0 + (g - b) / (largest - smallest));
      } else if (largest == g) {
        hue = angle * (2 + (b - r) / (largest - smallest));
      } else if (largest == b) {
        hue = angle * (4 + (r - g) / (largest - smallest));
      }

      if (hue < 0) {
        hue += (2 * PI);
      }

      saturation = (largest - smallest) / largest;
    }

    value = largest;
  }
};

RGB hsv_to_rgb(float hue, float saturation, float value) {
  int i = floor(hue * 6);
  float f = hue * 6 - i;
  float p = value * (1 - saturation);
  float q = value * (1 - f * saturation);
  float t = value * (1 - (1 - f) * saturation);
  float r = 0, g = 0, b = 0;
  switch (i % 6) {
    case 0: r = value, g = t, b = p; break;
    case 1: r = q, g = value, b = p; break;
    case 2: r = p, g = value, b = t; break;
    case 3: r = p, g = q, b = value; break;
    case 4: r = t, g = p, b = value; break;
    case 5: r = value, g = p, b = q; break;
  }
  return RGB(round(r * 255), round(g * 255), round(b * 255));
}


void rainbow_pattern(long start, long end, float saturation = 1.0, float value = 1.0) {

  strip.clear();

  // It's okay for end to be less than start; that just means we'll wrap around.
  //
  // It's not okay for start or end to exceed the number of lights, though.
  start = start % NUMBER_OF_LIGHTS;
  end = end % NUMBER_OF_LIGHTS;

  const float hueStart = 0, hueEnd = 1;
  float hue = hueStart;
  float hueIncrement = 0;
  if (start != end) {
    // If end == start + 1, the increment should be 1.
    // If end == start + 2, the increment should be 1/2.
    // If end == start + N, the increment should be 1/N.  This wraps around, so:
    // If end == start - 1, the increment should be 1/(NUMBER_OF_LIGHTS - 1).
    // If end == start - 2, the increment should be 1/(NUMBER_OF_LIGHTS - 2).
    // If end == start - N, the increment should be 1/(NUMBER_OF_LIGHTS - N).
    hueIncrement = (hueEnd - hueStart) / ((NUMBER_OF_LIGHTS + (end - start)) % NUMBER_OF_LIGHTS);
  }
  long index = start;
  while (true) {
    RGB color = hsv_to_rgb(hue, saturation, value);
    strip.setPixelColor(index, color.r, color.g, color.b);
    if (index == end) {
      break;
    }
    hue += hueIncrement;
    index = (index + 1) % NUMBER_OF_LIGHTS;
  }
  strip.show(); // Update the lights.
}

int lastUpdateMilliseconds = 0;
const int DELAY_MILLISECONDS = 500;
long index = 0;

// This code runs once, as the Arduino starts up.
void setup() {
  strip.begin();
  strip.clear();
  strip.show();
}

// This code runs continuously.
void loop() {

  //rainbow_pattern(18, 0, 1.0, 0.6);
  if (millis() - lastUpdateMilliseconds > DELAY_MILLISECONDS) {
    rainbow_pattern(index + 10, index, 1.0, 0.25);
    index++;
    lastUpdateMilliseconds = millis();
  }
}
