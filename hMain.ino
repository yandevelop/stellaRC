#include <FastLED.h>
#include <ezButton.h>

#define LED_DT 4
#define LED_TYPE WS2812
#define NUM_LEDS 300

struct CRGB leds[NUM_LEDS]; 

#define MIC_PIN 5
uint8_t squelch = 7;
int sample;
float sampleAvg = 0;
float micLev = 0;
uint8_t maxVol = 11;
bool samplePeak = 0;

int sampleAgc, multAgc;
uint8_t targetAgc = 60;

bool randomPalette = false;

int crMode; //MODE SWITCH


// Ripple variables
uint8_t colour;                                               // Ripple colour is randomized.
int center = 0;                                               // Center of the current ripple.
int step = -1;                                                // -1 is the initializing step.
uint8_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.


// Noise Variables
static int16_t xdist;                                          // A random number for our noise generator.
static int16_t ydist;
uint16_t xscale = 30;                                         // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint16_t yscale = 30;                                         // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 24;                                      // Value for blending between palettes.
#define MIN_BRIGHTNESS 0
#define MAX_BRIGHTNESS 100


//Button Var's
ezButton button1(2);
ezButton button2(6);
ezButton button3(5);

// Color palettes
CRGBPalette16 currentPalette = OceanColors_p;
CRGBPalette16 targetPalette = OceanColors_p;
TBlendType    currentBlending = NOBLEND;

void setup() {
  button1.setDebounceTime(50);
  button2.setDebounceTime(50);
  button3.setDebounceTime(50);
  Serial.begin(9600);
  delay(3000);
  LEDS.addLeds<LED_TYPE, LED_DT, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);
}

void loop() {
  getMode();
  FastLED.show();
}

void getMode() {
    button1.loop();
    button2.loop();
    button3.loop();

    if (button1.isPressed()) {
        crMode = 0;
      }

    if (button2.isPressed()) {
        crMode = 1;
      }

    if (button3.isPressed()) {
        crMode = 2;
      }
     switch (crMode) {
        case 0:
          //Serial.println("ripple");
          sound_ripple();
          break;
        case 1:
          sound_wave();
          //Serial.println("wave");
          break;
        case 2:
          sound_noise();
           //Serial.println("noise");
          break;
        case 3:
          sound_pal();
          //Serial.println("pal");
          break;
        case 4:
          sound_dots();
          //Serial.println("agc");
          break;
        case 5:                       //Non-Sound-Reactive
          rainbow();
          //Serial.println("RAINBOW!");
          break;
        default:
          break;
          
    }
  }

void sound_ripple() {
  getSample();
  EVERY_N_MILLISECONDS(20) {
     ripple();
   }
}

void sound_wave() {
  if (!randomPalette) {
    EVERY_N_SECONDS(5) {                                        // Change the palette every 5 seconds.
      for (int i = 0; i < 16; i++) {
        targetPalette[i] = CHSV(random8(), 255, 255);
      }
    }
  }
  
  EVERY_N_MILLISECONDS(100) {                                 // AWESOME palette blending capability once they do change.
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  }


  EVERY_N_MILLIS_I(thistimer,20) {                            // For fun, let's make the animation have a variable rate.
    uint8_t timeval = beatsin8(10,20,50);                     // Use a sinewave for the line below. Could also use peak/beat detection.
    thistimer.setPeriod(timeval);                             // Allows you to change how often this routine runs.
    fadeToBlackBy(leds, NUM_LEDS, 16);                        // 1 = slow, 255 = fast fade. Depending on the faderate, the LED's further away will fade out.
    getSample();
    agcAvg();
    sndwave();
  }
}

void sound_noise() {
  EVERY_N_MILLISECONDS(10) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);   // AWESOME palette blending capability.
    fillnoise8();                                             // Update the LED array with noise based on sound input
    fadeToBlackBy(leds, NUM_LEDS, 32);                         // 8 bit, 1 = slow, 255 = fast
  }

  if (!randomPalette) {
    EVERY_N_SECONDS(5) {                                        // Change the target palette to a random one every 5 seconds.
      targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 192, random8(128,255)), CHSV(random8(), 255, random8(128,255)));
    }
  }
  
  getSample();
}

void sound_pal() {
   EVERY_N_SECONDS(5) {                                        // Change the target palette to a random one every 5 seconds.
    static uint8_t baseC = random8(32);                         // You can use this as a baseline colour if you want similar hues in the next line.
    for (int i = 0; i < 16; i++) targetPalette[i] = CHSV(random8()+baseC, 255, 255);
  }      

  EVERY_N_MILLISECONDS(100) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);   // AWESOME palette blending capability.
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
  
  multAgc = (sampleAvg < 1) ? targetAgc : targetAgc / sampleAvg;  // Make the multiplier so that sampleAvg * multiplier = setpoint
  sampleAgc = sample * multAgc;
  if (sampleAgc > 255) sampleAgc = 255;
}

void ripple() {

  if (samplePeak == 1) {step = -1; samplePeak = 0; }          // If we have a peak, let's reset our ripple.

  fadeToBlackBy(leds, NUM_LEDS, 64);                          // 8 bit, 1 = slow, 255 = fast
  
  switch (step) {

    case -1:                                                  // Initialize ripple variables.
      center = random(NUM_LEDS);
      colour = random8();                                     // More peaks/s = higher the hue colour.
      step = 0;
      break;

    case 0:
      leds[center] = CHSV(colour, 255, 255);                  // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                            // At the end of the ripples.
      // step = -1;
      break;

    default:                                                  // Middle of the ripples.
      leds[(center + step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade/step*2);       // Simple wrap.
      leds[(center - step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade/step*2);
      step ++;                                                // Next step.
      break;  
  } // switch step
  
}

void sndwave() {
  
  leds[NUM_LEDS/2] = ColorFromPalette(currentPalette, sampleAgc, sampleAgc, currentBlending); // Put the sample into the center
  
  for (int i = NUM_LEDS - 1; i > NUM_LEDS/2; i--) {       //move to the left      // Copy to the left, and let the fade do the rest.
    leds[i] = leds[i - 1];
  }

  for (int i = 0; i < NUM_LEDS/2; i++) {                  // move to the right    // Copy to the right, and let the fade to the rest.
    leds[i] = leds[i + 1];
  }
  
}

void fillnoise8() {                                                       // Add Perlin noise with modifiers from the soundmems routine.
  int maxLen = sampleAvg;
  if (sampleAvg >NUM_LEDS) maxLen = NUM_LEDS;

  for (int i = (NUM_LEDS-maxLen)/2; i <(NUM_LEDS+maxLen+1)/2; i++) {      // The louder the sound, the wider the soundbar.
    uint8_t index = inoise8(i*sampleAvg+xdist, ydist+i*sampleAvg);        // Get a value from the noise function. I'm using both x and y axis.
  //  uint8_t index = inoise8(xdist+i*xscale, ydist+i*yscale) % 255;        // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);  // With that value, look up the 8 bit colour palette value and assign it to the current LED.
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
  leds[(millis() % (NUM_LEDS-1)) +1] = CHSV(sampleAgc, 255, sampleAgc);
 
}

void rainbow() {
    uint8_t thisSpeed = 10;
    uint8_t deltaHue= 10;
    uint8_t thisHue = beat8(thisSpeed,50); 
    fill_rainbow(leds, NUM_LEDS, thisHue, deltaHue);
  }
