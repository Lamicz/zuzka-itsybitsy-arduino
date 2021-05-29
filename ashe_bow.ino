#include <Adafruit_NeoPixel.h>
#include <APA102.h>
#include <arduino-timer.h>

#define STRIP_LENGTH 20
#define TIMER1_UPDATE 15
#define TIMER2_UPDATE 10
#define EXTRACT_RED(color)  (((color) >> 16) & 0xff)
#define EXTRACT_GREEN(color)  (((color) >> 8) & 0xff)
#define EXTRACT_BLUE(color)  ((color) & 0xff)

// pixelsMode0Max + pixelsMiddleCnt;
#define PIXELS_CNT 16

APA102<41, 40> onboardLed;
rgb_color colors[1];
Adafruit_NeoPixel strip(STRIP_LENGTH, 5, NEO_GRBW + NEO_KHZ800);
auto timer = timer_create_default();

typedef struct Pixel
{
  byte mode = 0;
  byte status = 0;
  byte timer = 0;
  byte wait = 0;
  int position = 0;
  int maxBrightness = 0;
  int minBrightness = 0;
  float brightnessStep = 0.0;
  float currentBrightness = 0.0;

  // Mode 0
  byte timerWhiteBlink = 0;
  byte waitWhiteBlink = 0;

  // Mode 2
  float currentBrightnessWhite = 0.0;
  bool dim = false;  
} Pixel;

Pixel pixels[PIXELS_CNT];
int i = 0;
int x = 0;
int stripFreePositions[STRIP_LENGTH];
byte pixelsMode0Current = 11;
int pixelsMode0CurrentPixels = 0;
byte pixelsWhiteBlinkCurrent = 3;
int pixelsWhiteBlinkCurrentPixels = 0;

const byte pixelsMiddle[] = {8, 9, 10, 11};
const byte pixelMiddle = 10;
const byte pixelsMiddleCnt = 4;
const byte pixelsMiddleMinBrightnessPool[] = {75, 110};
const byte pixelsMode0[] = {5, 13};
const byte pixelsMode0Max = 12;
const word pixelMode0MaxBrightness[] = {85, 256};
const byte pixelStepChangeBrightnessMode0[] = {20, 40}; // /100
const byte pixelStepChangeBrightnessMode2[] = {10, 20}; // /100
const byte pixelColor[] = {123, 254, 255};
const byte pixelsWaitWhiteBlinkMaxPool[] = {8, 51};
const byte pixelWaitCyclesMode2 = 40;
const byte pixelWaitCyclesMode0 = 10;

void adjustValByStep(float* varToAdjust, const byte minMaxArr[], float adjustByVar = 10000, byte adjustStep = 3)
{
  if (varToAdjust == 0) {
    *varToAdjust = random(minMaxArr[0], minMaxArr[1]);
    return;
  }
  byte currentStep = random(adjustStep);
  if (random(11) > 5) {
    *varToAdjust = (adjustByVar == 10000)
                  ? *varToAdjust + currentStep
                  : adjustByVar + currentStep;
  } else {
    *varToAdjust = (adjustByVar == 10000)
                  ? *varToAdjust - currentStep
                  : adjustByVar - currentStep;
  }
  if (*varToAdjust > minMaxArr[1]) {
    *varToAdjust = minMaxArr[1];
    return;
  }
  if (*varToAdjust < minMaxArr[0]) {
    *varToAdjust = minMaxArr[0];
    return;
  }
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
      pixel.currentBrightnessWhite -= pixel.brightnessStep;

      if (pixel.currentBrightness >= pixel.maxBrightness) {
        pixel.timer = 1;
        pixel.status = 3;
        pixel.currentBrightness = pixel.maxBrightness;
      }

      if(pixel.currentBrightnessWhite < 0){
        pixel.currentBrightnessWhite = 0;
      }

    } else {

      pixel.currentBrightness -= pixel.brightnessStep;
      pixel.currentBrightnessWhite += pixel.brightnessStep;

      if (pixel.currentBrightness < 0) {
        pixel.currentBrightness = 0;
      }

      if(pixel.currentBrightnessWhite >= pixel.maxBrightness){
        pixel.currentBrightnessWhite = pixel.maxBrightness;
      }
    }

    if (pixel.mode == 2) {

      strip.setPixelColor(
        pixel.position,
        strip.Color(
          pixelColor[0] * pixel.currentBrightness / 255,
          pixelColor[1] * pixel.currentBrightness / 255,
          pixelColor[2] * pixel.currentBrightness / 255,
          (pixel.dim == false) ? pixel.currentBrightness : pixel.currentBrightnessWhite
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

      if (pixel.waitWhiteBlink > 0 && pixel.status == 2) {

        uint32_t color = strip.getPixelColor(pixel.position);

        if (pixel.timerWhiteBlink < pixel.waitWhiteBlink) {

          strip.setPixelColor(
            pixel.position,
            strip.Color(EXTRACT_RED(color), EXTRACT_GREEN(color), EXTRACT_BLUE(color), 255)
          );

          pixel.timerWhiteBlink++;

        } else {

          strip.setPixelColor(
            pixel.position,
            strip.Color(EXTRACT_RED(color), EXTRACT_GREEN(color), EXTRACT_BLUE(color), 0)
          );
        }
      }
    }
  }

  if ((pixel.currentBrightness <= pixel.minBrightness) && (pixel.status == 2)) {

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

void pixelCreate(Pixel &pixel)
{
  bool isValid = true;

  if (pixel.mode == 0) {
    isValid = pixelsMode0CurrentPixels < pixelsMode0Current;
  }

  if (isValid) {

    if (pixel.mode == 0) {

      pixelsMode0CurrentPixels++;

      do {
        pixel.position = stripFreePositions[random(STRIP_LENGTH)];
      } while (pixel.position == -2 || pixel.position == -1);

      stripFreePositions[pixel.position] = -1;

      if (pixelsWhiteBlinkCurrentPixels < pixelsWhiteBlinkCurrent) {

        pixelsWhiteBlinkCurrentPixels++;

        pixel.timerWhiteBlink = 1;
        pixel.waitWhiteBlink = random(pixelsWaitWhiteBlinkMaxPool[0], pixelsWaitWhiteBlinkMaxPool[1]);
      }

      pixel.brightnessStep = random(pixelStepChangeBrightnessMode0[0], pixelStepChangeBrightnessMode0[1]) / 100.0;
      pixel.maxBrightness = random(pixelMode0MaxBrightness[0], pixelMode0MaxBrightness[1]);
    }

    if (pixel.mode == 2) {

      adjustValByStep(&pixel.currentBrightness, pixelsMiddleMinBrightnessPool, pixel.minBrightness);      
      pixel.minBrightness = pixel.currentBrightness;      
      pixel.brightnessStep = random(pixelStepChangeBrightnessMode2[0], pixelStepChangeBrightnessMode2[1]) / 100.0;      
      
      pixel.dim = random(20) == 10;
      if (pixel.dim) {
        adjustValByStep(&pixel.currentBrightnessWhite, pixelsMiddleMinBrightnessPool, pixel.minBrightness);
      }
      
      pixel.maxBrightness = pixel.minBrightness * 2;
      if(pixel.maxBrightness > 255){
        pixel.maxBrightness = 255;
      }     
    }
    pixel.status = 1;
  }
}

bool eventTimer1(void *)
{
  float pixelsMode0CurrentF = (float) pixelsMode0Current;
  adjustValByStep(&pixelsMode0CurrentF, pixelsMode0);
  pixelsWhiteBlinkCurrent = random(round(pixelsMode0Current / 2));
  return true;
}

void stripReset()
{
  strip.clear();

  // fade mode
  i = x = 0;
  while (x < pixelsMode0Max) {

    pixels[i] = Pixel();
    pixels[i].mode = 0;
    pixels[i].timer = pixelWaitCyclesMode0;

    i++;
    x++;
  }

  // middle pixels
  x = 0;  
  while (x < pixelsMiddleCnt) {

    pixels[i] = Pixel();
    pixels[i].mode = 2;
    pixels[i].timer = pixelWaitCyclesMode2;
    pixels[i].position = pixelsMiddle[x];

    i++;
    x++;
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

  colors[0].red = 0;
  colors[0].green = 65;
  colors[0].blue = 0;
  onboardLed.write(colors, 1, 1);

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
