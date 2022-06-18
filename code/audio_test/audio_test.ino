
#include <SPI.h>
#include <Adafruit_VS1053.h>

#define VS1053_RESET -1

#define VS1053_CS 6     // VS1053 chip select pin (output)
#define VS1053_DCS 10     // VS1053 Data/command select pin (output)
#define CARDCS 5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ 9     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

bool file_a = true;

void setup() {
  Serial.begin(115200);

  // Wait for serial port to be opened, remove this line for 'standalone' operation
  while (!Serial) { delay(1); }
  delay(500);
  Serial.println("\n\nAdafruit VS1053 Feather Test");

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }

  Serial.println(F("VS1053 found"));

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
  musicPlayer.playFullFile("/print001.mp3");
}

void loop() {
    // File is playing in the background
    if (musicPlayer.stopped()) {
        if (file_a) {
            musicPlayer.startPlayingFile("/beep001.mp3");
            file_a = false;
        } else {
            musicPlayer.startPlayingFile("/beep002.mp3");
            file_a = true;
        }
    }
    Serial.println("Can Play music???");
    delay(100);
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
