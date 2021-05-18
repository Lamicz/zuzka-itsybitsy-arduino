#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#include <arduino-timer.h>

#define STRIP_LENGTH 20
#define TIMER1_UPDATE 15
#define TIMER2_UPDATE 10

// pixelsMode0Max + pixelsMiddleCnt;
#define PIXELS_CNT 16

Adafruit_DotStar onboardLed(1, 41, 40, DOTSTAR_BGR);
Adafruit_NeoPixel strip(STRIP_LENGTH, 5, NEO_GRBW + NEO_KHZ800);
auto timer = timer_create_default();

const int pixelWaitCyclesMode2 = 40;
const int pixelWaitCyclesMode0 = 10;

class Pixel
{
  public:
    int timer = 0;
    int timerWhiteBlink = 0;
    int status = 0;
    int mode = 0; // 0 - RGB fade with blink, 2 - middle fade
    int position = 0;
    int wait = 0;
    int waitWhiteBlink = 0;
    int currentBrightnessImmutable = 0;
    float brightnessStep = 0;
    float currentBrightness = 0;
    int maxBrightness = 0;

    Pixel()
    {
    }

    Pixel(int m, int p = 0)
    {
      mode = m;
      position = p;
      wait = (m == 0) ? pixelWaitCyclesMode0 : pixelWaitCyclesMode2;
    }
};

Pixel pixels[PIXELS_CNT];
int i = 0;
int x = 0;
bool isValid = false;
uint32_t color;
int stripFreePositions[STRIP_LENGTH];
int pixelsMode0Current = 11;
int pixelsMode0CurrentPixels = 0;
int pixelsWhiteBlinkCurrent = 3;
int pixelsWhiteBlinkCurrentPixels = 0;

const int pixelsMiddle[] = {8, 9, 10, 11};
const int pixelMiddle = 10;
const int pixelsMiddleCnt = 4;
const int pixelsMiddleMinBrightnessPool[] = {75, 110};
const int pixelsMode0[] = {5, 13};
const int pixelsMode0Max = 12;
const int pixelMode0MaxBrightness[] = {85, 256};
const int pixelStepChangeBrightnessMode02[] = {30, 50}; // *100
const int pixelColor[] = {123, 254, 255};
const int pixelsWaitWhiteBlinkMaxPool[] = {8, 51};

uint8_t getRed(uint32_t color)
{
  return (color >> 16) & 0xFF;
}

uint8_t getGreen(uint32_t color)
{
  return (color >> 8) & 0xFF;
}

uint8_t getBlue(uint32_t color)
{
  return color & 0xFF;
}

void pixelProcess(Pixel &pixel) {

  if (pixel.timer > 0) {

    if (pixel.timer > pixel.wait) {
      pixel.status = 2;
      pixel.timer = 0;
    } else {
      pixel.timer++;
    }
  }

  if (pixel.status < 3) {

    if (pixel.status == 1) {

      pixel.currentBrightness += pixel.brightnessStep;

      if (pixel.currentBrightness >= pixel.maxBrightness) {
        pixel.timer = 1;
        pixel.status = 3;
        pixel.currentBrightness = pixel.maxBrightness;
      }

    } else {

      pixel.currentBrightness -= pixel.brightnessStep;

      if (pixel.currentBrightness < 0) {
        pixel.currentBrightness = 0;
      }
    }

    if (pixel.mode == 2) {

      strip.setPixelColor(
        pixel.position,
        strip.Color(
          pixelColor[0] * pixel.currentBrightness / 255,
          pixelColor[1] * pixel.currentBrightness / 255,
          pixelColor[2] * pixel.currentBrightness / 255,
          (pixel.waitWhiteBlink == 1 && pixel.currentBrightness > 40) ? pixel.currentBrightness - 15 : pixel.currentBrightness
        )
      );
    }

    if (pixel.mode == 0) {

      strip.setPixelColor(
        pixel.position,
        strip.Color(pixelColor[0] * pixel.currentBrightness / 255,
                    pixelColor[1] * pixel.currentBrightness / 255,
                    pixelColor[2] * pixel.currentBrightness / 255
                   )
      );
    }

    if (pixel.waitWhiteBlink > 0 && pixel.status == 2) {

      color = strip.getPixelColor(pixel.position);

      if (pixel.timerWhiteBlink < pixel.waitWhiteBlink) {

        strip.setPixelColor(
          pixel.position,
          strip.Color(getRed(color), getGreen(color), getBlue(color), 255)
        );

        pixel.timerWhiteBlink++;

      } else {

        strip.setPixelColor(
          pixel.position,
          strip.Color(getRed(color), getGreen(color), getBlue(color), 0)
        );
      }
    }

    if ((pixel.mode == 2) && (pixel.status == 2) && (pixel.currentBrightness <= pixel.currentBrightnessImmutable)) {
      pixel.currentBrightness = 0;
    }
  }

  if ((pixel.currentBrightness == 0) && (pixel.status == 2)) {

    if (pixel.mode == 0) {

      stripFreePositions[pixel.position] = pixel.position;

      pixelsMode0CurrentPixels--;

      if (pixel.waitWhiteBlink > 0) {

        pixelsWhiteBlinkCurrentPixels--;

        pixel.waitWhiteBlink = 0;
        pixel.timerWhiteBlink = 0;
      }
    }

    pixel.status = 0;
  }
}

void pixelCreate(Pixel &pixel) {

  isValid = true;
  int posRnd;

  if (pixel.mode == 0) {
    isValid = pixelsMode0CurrentPixels < pixelsMode0Current;
  }

  if (isValid) {

    if (pixel.mode == 0) {

      pixelsMode0CurrentPixels++;

      do {
        posRnd = random(STRIP_LENGTH);
        pixel.position = stripFreePositions[posRnd];
      } while (pixel.position == -2 || pixel.position == -1);

      stripFreePositions[pixel.position] = -1;

      if (pixelsWhiteBlinkCurrentPixels < pixelsWhiteBlinkCurrent) {

        pixelsWhiteBlinkCurrentPixels++;

        pixel.timerWhiteBlink = 1;
        pixel.waitWhiteBlink = random(pixelsWaitWhiteBlinkMaxPool[0], pixelsWaitWhiteBlinkMaxPool[1]);
      }

      pixel.brightnessStep = random(pixelStepChangeBrightnessMode02[0], pixelStepChangeBrightnessMode02[1]) / 100.0;
      pixel.maxBrightness = random(pixelMode0MaxBrightness[0], pixelMode0MaxBrightness[1]);
    }

    if (pixel.mode == 2) {

      pixel.brightnessStep = random(pixelStepChangeBrightnessMode02[0], pixelStepChangeBrightnessMode02[1]) / 100.0;
      pixel.currentBrightness = random(pixelsMiddleMinBrightnessPool[0], pixelsMiddleMinBrightnessPool[1]);
      pixel.currentBrightnessImmutable = pixel.currentBrightness;
      pixel.waitWhiteBlink = (random(10) > 8) ? 1 : 0;
      pixel.maxBrightness = 255;
    }

    pixel.status = 1;
  }
}

bool eventTimer1(void *)
{
  pixelsMode0Current = random(pixelsMode0[0], pixelsMode0[1]);
  pixelsWhiteBlinkCurrent = random(pixelsMode0Current / 2);
  return true;
}

void stripReset() {

  strip.clear();

  // fade mode
  i = x = 0;
  while (x < pixelsMode0Max) {

    pixels[i] = Pixel(0);

    i++;
    x++;
  }

  // middle pixels
  for (x = 0; x < pixelsMiddleCnt; x++) {

    pixels[i] = Pixel(2, pixelsMiddle[x]);

    i++;
  }

  // free positions
  x = 0;
  while (x < STRIP_LENGTH) {

    stripFreePositions[x] = x;

    for (i = 0; i < pixelsMiddleCnt; i++) {

      if (pixelsMiddle[i] == x) {

        stripFreePositions[x] = -2;
        continue;
      }
    }
    x++;
  }
}

void setup()
{
  randomSeed(analogRead(A0));

  timer.every(TIMER1_UPDATE * 1000, eventTimer1);

  onboardLed.begin();
  onboardLed.setBrightness(30);
  onboardLed.setPixelColor(0, onboardLed.Color(0, 255, 0));
  onboardLed.show();

  strip.begin();
  strip.show();
  strip.setBrightness(255);

  stripReset();
}

void loop()
{
  timer.tick();

  for (int cnt = 0; cnt < PIXELS_CNT; cnt++) {

    if (pixels[cnt].status > 0) {

      pixelProcess(pixels[cnt]);

    } else {

      pixelCreate(pixels[cnt]);
    }
  }

  strip.show();
}
