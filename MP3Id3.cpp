#include "MP3Id3.h"
#include <cstdint>
#include <SD.h>

const char *MP3Id3_genres[148] = {
    "Blues", "Classic Rock", "Country", "Dance",
    "Disco", "Funk", "Grunge", "Hip-Hop",
    "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap",
    "Reggae", "Rock", "Techno", "Industrial",
    "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
    "Vocal", "Jazz+Funk", "Fusion", "Trance",
    "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise",
    "AlternRock", "Bass", "Soul", "Punk",
    "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
    "Electronic", "Pop-Folk", "Eurodance", "Dream",
    "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
    "Native American", "Cabaret", "New Wave", "Psychadelic",
    "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka",
    "Retro", "Musical", "Rock & Roll", "Hard Rock",
    "Folk", "Folk/Rock", "National folk", "Swing",
    "Fast-fusion", "Bebob", "Latin", "Revival",
    "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
    "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
    "Big Band", "Chorus", "Easy Listening", "Acoustic",
    "Humour", "Speech", "Chanson", "Opera",
    "Chamber Music", "Sonata", "Symphony", "Booty Bass",
    "Primus", "Porn Groove", "Satire", "Slow Jam",
    "Club", "Tango", "Samba", "Folklore",
    "Ballad", "Powder Ballad", "Rhythmic Soul", "Freestyle",
    "Duet", "Punk Rock", "Drum Solo", "A Capella",
    "Euro-House", "Dance Hall", "Goa", "Drum & Bass",
    "Club House", "Hardcore", "Terror", "Indie",
    "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover",
    "Contemporary C", "Christian Rock", "Merengue", "Salsa",
    "Thrash Metal", "Anime", "JPop", "SynthPop"
};

// -------------------------------- PUBLIC
bool MP3Id3::read(File &file) {
  lenRead = 0;
  ID3TAG id3 = readHeaderV2(file);

  if (isV2(id3)) {
    return readV2(file, id3); // Read ID3 v2 frames at the beginning of the file.
  }
  return readV1(file); // Read ID3 v1 data at the (end - 128) of the file.
}

bool MP3Id3::read(char *fileName) {
  File fileTrack = SD.open(fileName);
  if (!fileTrack) { return false; }

  return read(fileTrack); // Read ID3 v1 data at the (end - 128) of the file.
}

char *MP3Id3::album() {
  return tagAlbum;
}

char *MP3Id3::artist() {
  return tagArtist;
}

char *MP3Id3::genre() {
  return tagGenre;
}

char *MP3Id3::title() {
  return tagTitle;
}

char *MP3Id3::track() {
  return tagTrack;
}

char *MP3Id3::year() {
  return tagYear;
}
// -------------------------------- PRIVATE

// converts unicode in UTF-8, buff contains the string to be converted up to len
char *MP3Id3::charUTF16UTF8(const char *buf, const uint32_t len) {
  char b[len + 1];
  b[len] = 0;

  auto *tmpbuf = (uint8_t *)malloc(len + 1);

  if (!tmpbuf)
    return { char(0) };  // out of memory;

  auto *t = tmpbuf;
  auto bitorder = false;  //Default to BE
  auto *p = (uint16_t *)buf;
  const auto *pe = (uint16_t *)&b[len];
  auto code = *p;

  if (code == 0xFEFF) {
    bitorder = false;
    p++;
  }  // LSB/MSB
  else if (code == 0xFFFE) {
    bitorder = true;
    p++;
  }  // MSB/LSB

  while (p < pe) {
    code = *p++;
    if (bitorder == true)
      code = __builtin_bswap16(code);

    if (code < 0X80) {
      *t++ = code & 0xff;
    } else if (code < 0X800) {
      *t++ = ((code >> 6) | 0XC0);
      *t++ = ((code & 0X3F) | 0X80);
    } else {
      *t++ = ((code >> 12) | 0XE0);
      *t++ = (((code >> 6) & 0X3F) | 0X80);
      *t++ = ((code & 0X3F) | 0X80);
    }
  }

  *t = 0;

  for (int i = 0; i < len; i++) {
    b[i] = tmpbuf[i];
  }

  free(tmpbuf);
  return b;
}

/*
  tagData
  The first byte tells the encoding:
    $00   ISO-8859-1 [ISO-8859-1]. Terminated with $00.
    $01   UTF-16 [UTF-16] encoded Unicode [UNICODE] with BOM. All
        strings in the same frame SHALL have the same byteorder.
        Terminated with $00 00.
    $02   UTF-16BE [UTF-16] encoded Unicode [UNICODE] without BOM.
        Terminated with $00 00.
    $03   UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated with $00.
*/
char *MP3Id3::getTagData(char *bufData, unsigned int len) {
  const auto encoding = bufData[0];
  auto *s = &bufData[1];
  len--;
  switch (encoding) {
    case 0x00:
      //Serial.println("Encoding is ISO-8859-1");
      break;
    //          return latin1UTF8(s);
    case 0x01:
      s = charUTF16UTF8(s, len);
      return s;
    case 0x02:
      s = charUTF16UTF8(s, len);
      return s;
    case 0x03:
      //Serial.println("Encoding is UTF-8");
      break;
  }

  return s;
}

bool MP3Id3::isFrameId(const char *id, ID3FRAME frame) {
  return (strncmp(id, frame.id, 4) == 0);
}

bool MP3Id3::isV2(ID3TAG header){
  return (strncmp(header.id, "ID3", 3) == 0);
}

char *MP3Id3::readFrameData(File file, unsigned int size) {
  char *buf = (char *)malloc(size + 1);
  file.readBytes(buf, size);
  buf[size] = 0;
  return buf;
}

ID3TAG MP3Id3::readHeaderV2(File &file) {
  ID3TAG id3;
  
  lenRead += file.readBytes((char *)&id3, sizeof(id3));

  if (strncmp(id3.id, "ID3", 3) != 0) {  // ID3v2 is there...
    file.seek(lenRead); // ... then jump to the position of the first v2 TAG frame.
  };
  return id3;
}

bool MP3Id3::readV1(File &file) {
  //v1
  TAGdata tagStruct = { 0 };
  file.seek(file.size() - 128);
  if (file.seek(file.size() - 128)) {
    if (file.read(reinterpret_cast<char *>(&tagStruct), 128) >= 0) {
      tagArtist = tagStruct.artist;
      tagAlbum = tagStruct.album;
      tagTitle = tagStruct.title;
      tagGenre = tagStruct.genre >= 0 ? MP3Id3_genres[tagStruct.genre] : 0;
      return true;
    }
  }
  return false;  //Identifier v1
}

bool MP3Id3::readV2(File &file, ID3TAG id3) {
  //size is a "syncsafe" integer
  id3.size = __builtin_bswap32(id3.size) + 10;

  if (id3.version[0] < 2 || id3.version[0] == 0xff || id3.version[1] == 0xff || id3.flags_zero != 0) {
    Serial.println("Abort!");
    return false;
  }

  if (id3.flag_extendedheader) {
    Serial.println("Has extended header. Skipping.");
    ID3TAG_EXTHEADER extheader;
    lenRead += file.readBytes((char *)&extheader, sizeof(extheader));
    extheader.size = __builtin_bswap32(extheader.size);
    if (extheader.size < 6) goto end;  //error
    lenRead += extheader.size;

    file.seek(lenRead);  //Skip the extendedHeader
  }

  // Search for ID3v2 frames
  do {
    ID3FRAME frameheader;
    lenRead += file.readBytes((char *)&frameheader, sizeof(frameheader));

    frameheader.size = __builtin_bswap32(frameheader.size); 

    //Serial.printf("ID3 Frame: %c%c%c%c size:%u\n", frameheader.id[0], frameheader.id[1], frameheader.id[2] , frameheader.id[3], frameheader.size);

    if (frameheader.id[0] == 0 || frameheader.id[1] == 0 || frameheader.id[2] == 0) {
      //Serial.println("----> Blank data, break");
      break;
    }

    if (lenRead + frameheader.size >= id3.size) {
      //Serial.println("----> End of ID3, break");
      break;
    }

    char *bufData = readFrameData(file, frameheader.size);
    char *tag = getTagData(bufData, frameheader.size);

    if (isFrameId("TIT2", frameheader)) {
      tagTitle = tag ;
    } else if (isFrameId("TPE1", frameheader)) {
      tagArtist = tag;
    } else if (isFrameId("TALB", frameheader)) {
      tagAlbum = tag;
    } else if (strncmp("TCON", frameheader.id, 4) == 0) {
      tagGenre = tag;
    } else if (strncmp("TLEN", frameheader.id, 4) == 0) {
      //Serial.print("Found TLEN - ");
    } else if (strncmp("TRCK", frameheader.id, 4) == 0) {
      tagTrack = tag;
    } else if (strncmp("TYER", frameheader.id, 4) == 0) {
      tagYear = tag;
    }

    lenRead += frameheader.size;

    file.seek(lenRead);
  } while (true);

  return true;

end:
  //Skip to first mp3/aac frame:
  file.seek(id3.size);
  return true;
}
