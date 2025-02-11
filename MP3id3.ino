/*

  Open a MP3 file and read the Tags

*/

#include <SD.h>
#include "MP3id3.h"

File mp3track;
MP3_Id3 tags;
// the setup routine runs once when you press reset:
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  
  SD.begin(BUILTIN_SDCARD);  // initialise the SD card

  mp3track = SD.open("file.mp3");
  tags.read(mp3track);


  Serial.print(F("\nArtist\t"));
  Serial.println(tags.artist());

  Serial.print(F("Album\t"));
  Serial.println(tags.getAlbum());

  Serial.print(F("Title (char)\t"));
  Serial.println(tags.title());
  
  mp3track.close();
}

void loop() {}