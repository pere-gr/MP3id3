# MP3Id3 
Library to retrieve id3 info from a mp3 File 

Tags available right now:
- Album
- Artist
- Genre
- Title
- Track
- Year

## Tested?
Just for my project (Arduino) with a Teensy4.1 and several Iron Maiden, Korn, Metallica, Anthrax and Rammstein MP3 files ;)

## How to use it?
Be sure to include SD.h and MP3id3.h and define a File variable for the file and a MP3id3 variable for this class.

Check the .ino example.

### Open a file and read its tags ...
  mp3track = SD.open("file.mp3");
  tags.read(mp3track);

### ... or just use the fullpath of the file
  tags.read("folder/file.mp3");
