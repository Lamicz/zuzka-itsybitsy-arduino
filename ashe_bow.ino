#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>

#define STRIP_LENGTH 20
#define TIMER1_UPDATE 15
#define TIMER2_UPDATE 10

// pixelsMode0Max + pixelsMode1Max + pixelsMiddleCnt;
#define PIXELS_CNT 23

Adafruit_DotStar onboardLed(1, 41, 40, DOTSTAR_BRG);
Adafruit_NeoPixel strip(STRIP_LENGTH, 5, NEO_GRBW + NEO_KHZ800);

int i = 0;
int x = 0;
int y = 0;
bool isValid = false;

unsigned long timer1LastUpdated = millis();
unsigned long timer2LastUpdated = millis();

int stripFreePositions[STRIP_LENGTH];
int pixelsMode0Current = 11;
int pixelsMode0CurrentPixels = 0;
int pixelsMode1Current = 5;
int pixelsMode1CurrentPixels = 0;
int pixelsMiddle[] = {8, 9, 10, 11};
int pixelsMiddleCnt = 4;
int pixelsMiddleMinBrightnessPool[] = {75, 110};
int pixelsMode0[] = {8, 13};
int pixelsMode0Max = 13;
int pixelMode0MaxBrightness[] = {85, 255};
int pixelsMode1[] = {1, 6};
int pixelsMode1Max = 6;
int pixelWaitCyclesMode1Pool[] = {20, 50};
int pixelStepChangeBrightnessMode02[] = {4, 8};
int pixelColor[] = {154, 248, 251};
int pixelWaitCyclesMode1 = 40;
int pixelWaitCyclesMode0 = 4;

class Pixel
{
  public:
    int timer = 0;
    int status = 0;
    int mode = 0; // 0 - RGB fade, 1 - white blink, 2 - middle fade
    int position = 0;
    int wait = 0;
    int currentBrightness = 0;
    int currentBrightnessImmutable = 0;
    int brightnessStep = 0;
    int maxBrightness = 0;

    Pixel()
    {
    }

    Pixel(int m, int p = 0)
    {
      mode = m;
      position = p;
      wait = (m == 0) ? pixelWaitCyclesMode0 : pixelWaitCyclesMode1;
    }
};

Pixel pixels[PIXELS_CNT];

//strip.setBuffered(true)
//stripTest()

void pixelProcess(Pixel &pixel) {

  if (pixel.timer > 0) {

    if (pixel.timer > pixel.wait) {
      pixel.status = 2;
      pixel.timer = 0;
    } else {
      pixel.timer++;
    }
  }

  if ((pixel.mode == 0) || (pixel.mode == 2)) {

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

        strip.setPixelColor(pixel.position, strip.Color(0, 0, 0, strip.gamma8(pixel.currentBrightness)));

      }

      strip.setPixelColor(
        pixel.position,
        strip.Color(pixelColor[0] * pixel.currentBrightness / 255, pixelColor[1] * pixel.currentBrightness / 255, pixelColor[2] * pixel.currentBrightness / 255)
      );

      if ((pixel.mode == 2) && (pixel.status == 2) && (pixel.currentBrightness <= pixel.currentBrightnessImmutable)) {
        pixel.currentBrightness = 0;
      }
    }
  }

  if (pixel.mode == 1) {

    if (pixel.status == 1) {

      pixel.timer = 1;
      pixel.status = 3;
    }

    if (pixel.status == 2) {

      if (pixel.currentBrightness == 0) {

        strip.setPixelColor(pixel.position, strip.Color(0, 0, 0, 255));

        pixel.currentBrightness = 255;

      } else {

        strip.setPixelColor(pixel.position, strip.Color(0, 0, 0, 0));

        pixel.currentBrightness = 0;
      }
    }
  }

  if ((pixel.currentBrightness == 0) && (pixel.status == 2)) {

    pixel.status = 0;

    if (pixel.mode == 0) {

      stripFreePositions[pixel.position] = pixel.position;

      pixelsMode0CurrentPixels--;
    }

    if (pixel.mode == 1) {

      pixelsMode1CurrentPixels--;
    }
  }
}

void pixelCreate(Pixel &pixel) {

  isValid = true;

  if (pixel.mode == 0) {
    isValid = pixelsMode0CurrentPixels < pixelsMode0Current;
  }

  if (pixel.mode == 1) {
    isValid = random(0, 10) > 5 && pixelsMode1CurrentPixels < pixelsMode1Current;
  }

  if (isValid) {

    if (pixel.mode == 0) {

      pixelsMode0CurrentPixels++;

      do {
        int freePosRndKey = random(0, STRIP_LENGTH);
        pixel.position = stripFreePositions[freePosRndKey];
      } while (pixel.position == -2 || pixel.position == -1);

      stripFreePositions[pixel.position] = -1;

      pixel.brightnessStep = random(pixelStepChangeBrightnessMode02[0], pixelStepChangeBrightnessMode02[1]);
      pixel.maxBrightness = random(pixelMode0MaxBrightness[0], pixelMode0MaxBrightness[1]);
    }

    if (pixel.mode == 1) {

      pixelsMode1CurrentPixels++;

      do {
        int freePosRndKey = random(0, STRIP_LENGTH);
        pixel.position = stripFreePositions[freePosRndKey];
      } while (pixel.position == -2);
    }

    if (pixel.mode == 2) {

      pixel.brightnessStep = random(pixelStepChangeBrightnessMode02[0], pixelStepChangeBrightnessMode02[1]);
      pixel.currentBrightness = random(pixelsMiddleMinBrightnessPool[0], pixelsMiddleMinBrightnessPool[1]);
      pixel.currentBrightnessImmutable = pixel.currentBrightness;
      pixel.maxBrightness = 255;
    }

    pixel.status = 1;
  }
}

void stripReset() {

  strip.clear();

  stripFreePositions[STRIP_LENGTH];

  // fade mode
  i = x = 0;
  while (x < pixelsMode0Max) {

    pixels[i] = Pixel(0);

    i++;
    x++;
  }

  // white blink
  x = 0;
  while (x < pixelsMode1Max) {

    pixels[i] = Pixel(1);

    i++;
    x++;
  }

  // middle pixels
  for (x = 0; x < pixelsMiddleCnt; x++) {

    pixels[i] = Pixel(2, pixelsMiddle[x]);

    i++;
  }

  //
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
  onboardLed.begin();
  onboardLed.setBrightness(30);
  onboardLed.setPixelColor(0, 0x00FF0000);
  onboardLed.show();
  
  strip.begin();
  strip.show();
  strip.setBrightness(255);

  stripReset();
}

void loop()
{
  if ((millis() - timer1LastUpdated) > TIMER1_UPDATE) {

    timer1LastUpdated = millis();

    pixelsMode0Current = random(pixelsMode0[0], pixelsMode0[1]);
  }

  if ((millis() - timer2LastUpdated) > TIMER2_UPDATE) {

    timer2LastUpdated = millis();

    pixelsMode1Current = random(pixelsMode1[0], pixelsMode1[1]);
    pixelWaitCyclesMode1 = random(pixelWaitCyclesMode1Pool[0], pixelWaitCyclesMode1Pool[1]);
  }

  for (int cnt = 0; cnt < PIXELS_CNT; cnt++) {

    if (pixels[cnt].status > 0) {

      pixelProcess(pixels[cnt]);

    } else {

      pixelCreate(pixels[cnt]);
    }
  }

  strip.show();

  delay(30);
}
