
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>
#include <FastLED.h>

#define VS1053_RESET -1

#define VS1053_CS 6     // VS1053 chip select pin (output)
#define VS1053_DCS 10     // VS1053 Data/command select pin (output)
#define CARDCS 5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ 9     // VS1053 Data request, ideally an Interrupt pin

#define  SWITCH1  18  // PA01
#define  SWITCH2  19 // PA02
#define  SWITCH3  20 // PA03
#define  SWITCH4  2 // PA06
#define  PWM1  12  // PC00
#define  PWM2  13 // PC01
#define  PWM3  0 // PA04
#define  PWM4  1 // PA05

#define LED_PIN 12
#define LED_COUNT 22

#define ON_TIME_MS 500
#define DELAY_MS 10

enum button_t {
  B_1 = 1,
  B_2 = 2,
  B_3 = 3,
  B_4 = 4,
};

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

Adafruit_seesaw ss;

CRGB leds[LED_COUNT];

bool ready_led = false;
bool brighten = true;
bool success_led = false;
bool fail_led = false;
bool play_beep = false;
bool play_error = false;
bool play_success_print = false;
bool play_error_print = false;
bool playing = false;
uint8_t ready_led_brightnesss = 0;
uint8_t input_seq_idx = 0;
uint8_t delay_count = 0;
button_t input_seq[3];

const button_t send_print_seq[3] = {B_3, B_2, B_1};

void setup() {
  Serial.begin(115200);

  delay(500);
  Serial.println("\n\nAdafruit VS1053 Feather Test");

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));

  if (! ss.begin(0x3A)) {
     Serial.println(F("Couldn't find seesaw, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("seesaw found"));

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);

  Serial.println(F("seesaw started OK!"));
  ss.pinMode(SWITCH1, INPUT_PULLUP);
  ss.pinMode(SWITCH2, INPUT_PULLUP);
  ss.pinMode(SWITCH3, INPUT_PULLUP);
  ss.pinMode(SWITCH4, INPUT_PULLUP);
  ss.analogWrite(PWM1, 0);
  ss.analogWrite(PWM2, 0);
  ss.analogWrite(PWM3, 0);
  ss.analogWrite(PWM4, 0);

  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working

  if (!SD.begin(CARDCS)) {
    while (1) {
      Serial.println(F("SD failed, or not present"));
      delay(1);
    };  // don't do anything more
  }
  Serial.println("SD OK!");
  printDirectory(SD.open("/"), 0);

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(1,1);

  // If DREQ is on an interrupt pin we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  FastLED.clear(true);
  FastLED.show();
}

void loop() {
  if (musicPlayer.stopped()) {
    playing = false;
    if (fail_led) {
      for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds[i] = CHSV(HUE_RED, 255, 200);
      }
      delay_count = 0;
      FastLED.show();
      fail_led = false;
    } else if (success_led) {
      for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds[i] = CHSV(HUE_GREEN, 255, 200);
      }
      delay_count = 0;
      FastLED.show();
      success_led = false;
    }
    if (delay_count >= (ON_TIME_MS / DELAY_MS)) {
      FastLED.clear(true);
      FastLED.show();
    }
  }

  if (! ss.digitalRead(SWITCH1)) {
    FastLED.delay(DELAY_MS);
    delay_count++;
    ss.analogWrite(PWM1, 200);
    if ((! ss.digitalRead(SWITCH1)) && (!playing)) {
      play_beep = true;
      input_seq[input_seq_idx] = B_1;
      input_seq_idx++;
    }
  } else {
    ss.analogWrite(PWM1, 0);
  }

  if (! ss.digitalRead(SWITCH2)) {
    FastLED.delay(DELAY_MS);
    delay_count++;
    ss.analogWrite(PWM2, 200);
    if ((! ss.digitalRead(SWITCH2)) && (!playing)) {
      play_beep = true;
      input_seq[input_seq_idx] = B_2;
      input_seq_idx++;
    }
  } else {
    ss.analogWrite(PWM2, 0);
  }

  if (! ss.digitalRead(SWITCH3)) {
    FastLED.delay(DELAY_MS);
    delay_count++;
    ss.analogWrite(PWM3, 200);
    if ((! ss.digitalRead(SWITCH3)) && (!playing)) {
      play_beep = true;
      input_seq[input_seq_idx] = B_3;
      input_seq_idx++;
    }
  } else {
    ss.analogWrite(PWM3, 0);
  }

  if (! ss.digitalRead(SWITCH4)) {
    FastLED.delay(DELAY_MS);
    delay_count++;
    ss.analogWrite(PWM4, 200);
    if ((! ss.digitalRead(SWITCH4)) && (!playing)) {
      if (input_seq_idx >= 3) {
        if ((input_seq[0] == send_print_seq[0])
         && (input_seq[1] == send_print_seq[1])
         && (input_seq[2] == send_print_seq[2])
        ) {
          play_success_print = true;
        } else {
          play_error_print = true;
        }
        input_seq_idx = 0;
      } else {
        play_error = true;
      }
    }
  } else {
    if (ready_led) {
      if (ready_led_brightnesss > 248) {
        brighten = false;
      } else if (ready_led_brightnesss < 16) {
        brighten = true;
      }
      if (brighten) {
        ready_led_brightnesss += 5;
      } else {
        ready_led_brightnesss -= 5;
      }
    } else {
      ready_led_brightnesss = 0;
    }
    ss.analogWrite(PWM4, ready_led_brightnesss);
  }

  if (musicPlayer.stopped()) {
      if (play_beep) {
        musicPlayer.startPlayingFile("/beep001.mp3");
        play_beep = false;
        playing = true;
      } else if (play_error) {
        musicPlayer.startPlayingFile("/beep002.mp3");
        play_error = false;
        playing = true;
      } else if (play_error_print) {
        musicPlayer.startPlayingFile("/error002.mp3");
        play_error_print = false;
        fail_led = true;
        playing = true;
      } else if (play_success_print) {
        musicPlayer.startPlayingFile("/print001.mp3");
        play_success_print = false;
        success_led = true;
        playing = true;
      }
  }

  if (input_seq_idx == 3) {
    ready_led = true;
  } else if (input_seq_idx > 3) {
    input_seq_idx = 0;
    ready_led = false;
  } else {
    ready_led = false;
  }

  FastLED.delay(DELAY_MS);
  delay_count++;
}

/// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {

     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}
