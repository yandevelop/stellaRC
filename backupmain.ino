#include <FastLED.h>
#include "Button.h"

#define LED_DT 4
#define LED_TYPE WS2812B
#define NUM_LEDS 300
#define MIC_PIN 3

//Ripple
uint8_t colour;                                               // Ripple colour is randomized.
int center = 0;                                               // Center of the current ripple.
int step = -1;                                                // -1 is the initializing step.
uint8_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.


struct CRGB leds[NUM_LEDS];

//getSample Variables
float micLev = 0;
uint8_t maxVol = 12;
uint8_t squelch = 7;

int sample;

float sampleAvg = 0;
bool samplePeak = 0;

int sampleAgc, multAgc;

//Brightness
uint8_t smoothVal;

uint8_t crMode; //MODE SWITCH

//Speed
uint8_t spSmooth;

#define MAX_BRIGHTNESS 128

// Color palettes
DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
    0, 120,  0,  0,
   22, 179, 22,  0,
   51, 255,104,  0,
   85, 167, 22, 18,
  135, 100,  0,103,
  198,  16,  0,130,
  255,   0,  0,160
};

DEFINE_GRADIENT_PALETTE( bhw1_06_gp ) {
    0, 184,  1,128,
  160,   1,193,182,
  219, 153,227,190,
  255, 255,255,255
};

CRGBPalette16 purplePalette = CRGBPalette16 (
    CRGB::DarkViolet,
    CRGB::DarkViolet,
    CRGB::DarkViolet,
    CRGB::DarkViolet,
    
    CRGB::Magenta,
    CRGB::Magenta,
    CRGB::Linen,
    CRGB::Linen,
    
    CRGB::Magenta,
    CRGB::Magenta,
    CRGB::DarkViolet,
    CRGB::DarkViolet,

    CRGB::DarkViolet,
    CRGB::DarkViolet,
    CRGB::Linen,
    CRGB::Linen
);

DEFINE_GRADIENT_PALETTE( OS_WAT_Mars_gp ) {
    0, 194,229,212,
    5, 194,229,207,
   11, 197,227,205,
   17, 197,227,203,
   22, 199,227,205,
   28, 199,227,205,
   34, 201,229,203,
   40, 206,231,203,
   45, 210,233,205,
   51, 213,233,203,
   57, 215,233,199,
   63, 220,235,199,
   68, 224,233,199,
   74, 232,237,199,
   80, 237,237,197,
   85, 237,237,197,
   91, 234,237,194,
   97, 234,237,194,
  103, 234,237,194,
  108, 232,235,190,
  114, 232,229,182,
  120, 229,223,176,
  126, 227,219,170,
  131, 229,215,170,
  137, 232,211,168,
  143, 234,207,164,
  148, 232,197,158,
  154, 224,187,149,
  160, 217,178,140,
  166, 208,169,132,
  171, 201,164,127,
  177, 197,157,120,
  183, 194,154,115,
  189, 182,141,106,
  194, 161,125, 93,
  200, 144,111, 84,
  206, 130, 99, 74,
  211, 115, 88, 66,
  217, 100, 77, 57,
  223,  92, 75, 57,
  229, 109, 93, 77,
  234, 132,114, 98,
  240, 155,136,122,
  246, 182,161,149,
  252, 213,189,182,
  255, 227,207,199
};

CRGBPalette16 sunset = Sunset_Real_gp;
CRGBPalette16 pink = bhw1_06_gp;
CRGBPalette16 mars = OS_WAT_Mars_gp;
CRGBPalette16 firePalette = HeatColors_p;

CRGBPalette16 currentPalette;
TBlendType currentBlending = NOBLEND;
uint8_t whichPalette = 0;

Button buttonM1(0); //Strobe
Button buttonM2(1); //Rainbow
Button buttonM3(2); // 0 is reserved for Strip
Button buttonM4(3);
Button buttonM5(5); //Sound_ripple;
Button buttonM6(6);
Button buttonM7(7);
Button buttonM8(8);
Button buttonC9(9);
Button buttonC10(10);
Button buttonC11(11);
Button buttonC12(12);
Button buttonC13(13); 
//Button buttonC14(19);

#define dbTime 50

void setup() {
  buttonM1.begin();
  buttonM2.begin();
  buttonM3.begin();
  buttonM4.begin();
  buttonM5.begin();
  buttonM6.begin();
  buttonM7.begin();
  buttonM8.begin();
  buttonC9.begin();
  buttonC10.begin();
  buttonC11.begin();
  buttonC12.begin();
  //buttonC13.begin();
  //buttonC14.begin();
  delay(3000);
  LEDS.addLeds<LED_TYPE, LED_DT, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);
  //cylon();
}

void loop() {
  getMode();
  setBrightness();
  getSpeed();
  getColor();
  FastLED.show();
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }
void cylon(){
    static uint8_t hue = 0;
  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }

  // Now go in the other direction.  
  for(int i = (NUM_LEDS)-1; i >= 0; i--) {
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  
  }
void setBrightness() {
  uint8_t oldVal;
  uint8_t bright = map(analogRead(A1), 0, 1023, 0, MAX_BRIGHTNESS);
  smoothVal = 0.5 * smoothVal + 0.5 * bright;
  if (oldVal != smoothVal) {
        FastLED.setBrightness(smoothVal);
    }
  oldVal = smoothVal;
}

void getSpeed() {
    uint8_t oldsVal;
    uint8_t speedVal = map(analogRead(A2), 0, 1023, 6, 255);
    spSmooth = 0.4 * spSmooth + 0.6 * speedVal;
    if (oldsVal != spSmooth) {
      }
    oldsVal = spSmooth;
}

void getColor() {
    switch(whichPalette) {
        case 0:
          currentPalette = OceanColors_p;
          break;
        case 1:
          currentPalette = firePalette;
          break;
        case 2:
          currentPalette = purplePalette;
          break;
        case 3:
          currentPalette = ForestColors_p;
          break;
        case 4:
          currentPalette = RainbowColors_p;
          break;
        case 5:
          currentPalette = mars;
          break;
      }


    if (buttonC9.pressed()) {
        whichPalette = 0;
      }
    if (buttonC10.pressed()) {  
        whichPalette = 1;
      }
    if (buttonC11.pressed()) {
        whichPalette = 2;
      }
    if (buttonC12.pressed()) {
        whichPalette = 3;
      }
  /*  if (buttonC13.pressed()) {
        whichPalette = 4;
      }
    if (buttonC14.pressed()) {
        whichPalette = 5;
      }*/
}

void getMode() {
    if (buttonM1.pressed()) {
        crMode = 0;
      }
    if (buttonM2.pressed()) {  
        crMode = 1;
      }
    if (buttonM3.pressed()) {
        crMode = 2;
      }
    if (buttonM4.pressed()) {
        crMode = 3;
      }
    if (buttonM5.pressed()) {
        crMode = 4;
      }
    if (buttonM6.pressed()) {
        crMode = 5;
      }
    if (buttonM7.pressed()) {  
        crMode = 6;
      }
    if (buttonM8.pressed()) {
        crMode = 7;
      }
     switch (crMode) {
        case 0:
          strobe();
          break;
        case 1:
          rainbow();
          break;
        case 2:
          fire();
          break;
        case 3:
          confetti(); //sound_pal;
          break;
        case 4:
          sound_ripple(); //sound_dots
          break;
        case 5:                       //Non-Sound-Reactive
          sound_noise();
          break;
        case 6:
          sound_pal();
          break;
        case 7:
          sound_wave();
          break;
        default:
          break;
          //rainbow, strobe, strobeR, fire, addGlitter, confetti, purple
    }
  }


// Audio Processing

void getSample() {
  int16_t micIn;                                              // Current sample starts with negative values and large values, which is why it's 16 bit signed.
  static long peakTime;
  
  micIn = analogRead(MIC_PIN);                                // Poor man's analog Read.
  micLev = ((micLev * 31) + micIn) / 32;                      // Smooth it out over the last 32 samples for automatic centering.
  micIn -= micLev;                                            // Let's center it to 0 now.
  micIn = abs(micIn);                                         // And get the absolute value of each sample.
  sample = (micIn <= squelch) ? 0 : (sample + micIn) / 2;     // Using a ternary operator, the resultant sample is either 0 or it's a bit smoothed out with the last sample.
  sampleAvg = ((sampleAvg * 31) + sample) / 32;               // Smooth it out over the last 32 samples.

  if (sample > (sampleAvg+maxVol) && millis() > (peakTime + 50)) {    // Poor man's beat detection by seeing if sample > Average + some value.
    samplePeak = 1;                                                   // Then we got a peak, else we don't. Display routines need to reset the samplepeak value in case they miss the trigger.
    peakTime=millis();                
  }                                                          
}

void agcAvg() {                                                   // A simple averaging multiplier to automatically adjust sound sensitivity.
  uint8_t targetAgc = 60;         //Normal value = 60;
  multAgc = (sampleAvg < 1) ? targetAgc : targetAgc / sampleAvg;  // Make the multiplier so that sampleAvg * multiplier = setpoint
  sampleAgc = sample * multAgc;
  if (sampleAgc > 255) sampleAgc = 255;
}


// Modes
void sound_ripple() {
  getSample();
  EVERY_N_MILLISECONDS(20) {
     ripple();
   }
   FastLED.show();
}

void sound_wave() {
  EVERY_N_MILLISECONDS(100) {                                 // AWESOME palette blending capability once they do change.
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, currentPalette, maxChanges);
  }

  EVERY_N_MILLIS_I(thistimer,20) {                            // For fun, let's make the animation have a variable rate.
    uint8_t timeval = beatsin8(10,20,50);                     // Use a sinewave for the line below. Could also use peak/beat detection.
    thistimer.setPeriod(timeval);                             // Allows you to change how often this routine runs.
    fadeToBlackBy(leds, NUM_LEDS, 16);                        // 1 = slow, 255 = fast fade. Depending on the faderate, the LED's further away will fade out.
    getSample();
    agcAvg();
    sndwave();
  }
  //FastLED.show();
}


void sound_noise() {
  //uint8_t maxChanges = 24;                                      // Value for blending between palettes.
  EVERY_N_MILLISECONDS(10) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, currentPalette, maxChanges);   // AWESOME palette blending capability.
    fillnoise8();                                             // Update the LED array with noise based on sound input
    fadeToBlackBy(leds, NUM_LEDS, 32);                         // 8 bit, 1 = slow, 255 = fast
  }
  getSample();
}

void sound_pal() {
   EVERY_N_SECONDS(5) {                                        // Change the target palette to a random one every 5 seconds.
    static uint8_t baseC = random8(32);                         // You can use this as a baseline colour if you want similar hues in the next line.
    //for (int i = 0; i < 16; i++) currentPalette[i] = CHSV(random8()+baseC, 255, 255);
  }      

  EVERY_N_MILLISECONDS(100) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, currentPalette, maxChanges);   // AWESOME palette blending capability.
  }
  
  EVERY_N_MILLISECONDS(20) {                           // FastLED based non-blocking delay to update/display the sequence.
    getSample();
    agcAvg();
    propPal();
  }
}

void sound_dots() {
    EVERY_N_MILLIS(10) {
    fadeToBlackBy(leds, NUM_LEDS, 4);                         // 8 bit, 1 = slow, 255 = fast
    fadeToBlackBy(leds, 1, 32);
  }
  
  getSample();                                                // Sample the microphone.
  agcAvg();                                                   // Calculate the adjusted value as sampleAvg.
  ledShow();
}

void ripple() {
  // Ripple variables
  if (samplePeak == 1) {step = -1; samplePeak = 0; }          // If we have a peak, let's reset our ripple.

  fadeToBlackBy(leds, NUM_LEDS, 64);                          // 8 bit, 1 = slow, 255 = fast
  switch (step) {

    case -1:                                                  // Initialize ripple variables.
      center = random(NUM_LEDS);
      colour = ColorFromPalette(currentPalette, sampleAgc, sampleAgc, currentBlending);//random8();                                     // More peaks/s = higher the hue colour.
      step = 0;
      break;

    case 0:
      leds[center] = ColorFromPalette(currentPalette, sampleAgc, sampleAgc, currentBlending);
      //leds[center] = CHSV(colour, 255, 255);                  // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                            // At the end of the ripples.
      // step = -1;
      break;

    default:                                                  // Middle of the ripples.
      leds[(center + step + NUM_LEDS) % NUM_LEDS] += CHSV(currentPalette, 255, myfade/step*2);       // Simple wrap.
      leds[(center - step + NUM_LEDS) % NUM_LEDS] += CHSV(currentPalette, 255, myfade/step*2);
      step ++;                                                // Next step.
      break;  
  } // switch step
}

void sndwave() {
  leds[NUM_LEDS/2] = ColorFromPalette(currentPalette, sampleAgc, sampleAgc, currentBlending);

  for (int i = NUM_LEDS - 1; i > NUM_LEDS/2; i--) {       //move to the left      // Copy to the left, and let the fade do the rest.
    leds[i] = leds[i - 1];
  }

  for (int i = 0; i < NUM_LEDS/2; i++) {                  // move to the right    // Copy to the right, and let the fade to the rest.
    leds[i] = leds[i + 1];
  }
}

void fillnoise8() {                                                       // Add Perlin noise with modifiers from the soundmems routine.
   // Noise Variables
  static int16_t xdist;                                          // A random number for our noise generator.
  static int16_t ydist;
  uint16_t xscale = 30;                                         // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
  uint16_t yscale = 30;                                         // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
  int maxLen = sampleAvg;
  if (sampleAvg >NUM_LEDS) maxLen = NUM_LEDS;
  for (int i = (NUM_LEDS-maxLen)/2; i <(NUM_LEDS+maxLen+1)/2; i++) {      // The louder the sound, the wider the soundbar.
    uint8_t index = inoise8(i*sampleAvg+xdist, ydist+i*sampleAvg);        // Get a value from the noise function. I'm using both x and y axis.
    //uint8_t index = inoise8(xdist+i*xscale, ydist+i*yscale) % 255;        // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(currentPalette, index, smoothVal, LINEARBLEND);  // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }

  xdist=xdist+beatsin8(5,0,10);
  ydist=ydist+beatsin8(4,0,10);                                                                      
}

void propPal() {

  leds[0] = ColorFromPalette(currentPalette, sampleAgc, sampleAgc, currentBlending);
  
  for (int i = NUM_LEDS-1; i>0; i--) {                        // Propagate up the strand.
    leds[i] = leds[i-1];
  }
}

void ledShow() {
  if (samplePeak == 1) { leds[0] = CRGB::Gray; samplePeak = 0;}
  leds[(millis() % (NUM_LEDS-1)) +1] = ColorFromPalette(currentPalette, sampleAgc, sampleAgc, currentBlending);//CHSV(sampleAgc, 255, sampleAgc);
}

void strobeR() {
  uint8_t strobe1;
  EVERY_N_MILLISECONDS(500) {
      strobe1 = spSmooth;
    }
  if ((millis() / strobe1) % 2) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::White);
  }
}

void strobe() {
    if ((millis() / 50) % 2) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
      }
      else {
          fill_solid(leds, NUM_LEDS, CRGB::White);
        }
  }

void rainbow() {
    uint8_t deltaHue= 10;
    uint8_t thisHue = beat8(spSmooth, 50);
    fill_rainbow(leds, NUM_LEDS, thisHue, deltaHue);
  }

void fire() {
    EVERY_N_MILLISECONDS(5) {
      int  a = millis();
  
    for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t noise = inoise8 (0 , i * 60 + a , a / 3);
      uint8_t math = abs8(i - (NUM_LEDS-1)) * 255 / (NUM_LEDS-1);
      uint8_t index = qsub8 (noise, math);
      leds[i] = ColorFromPalette (currentPalette, index, 255);    
    }  
  }
}

void confetti() 
{
  uint8_t gHue = 0;
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}
