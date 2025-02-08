#include "MP3Id3.h"
#include <cstdint>
#include <SD.h>

bool MP3_Id3::read(File &file) {
  bool ID3present = false;
  uint8_t numTagstoFind = 5;

  ID3TAG id3;
  auto lenRead = 0;
  lenRead += file.readBytes((char *)&id3, sizeof(id3));

  if (strncmp(id3.id, "ID3", 3) != 0) {
    //Serial.println("No ID3 header found");
    //v1
    int fileNameLength = 1024;
    int mp3TagSize = 128;
    artist = "";
    TAGdata tagStruct = { 0 };
    file.seek(file.size() - 128);
    if (file.seek(file.size() - 128)) {
      file.read(reinterpret_cast<char *>(&tagStruct), 128);
      //if(!file.fail()){
      artist = tagStruct.artist;
      album = tagStruct.album;
      title = tagStruct.title;
      genre = tagStruct.genre;
      //}
    }
    return true;  //Identifier v1
  }

  //size is a "syncsafe" integer
  id3.size = __builtin_bswap32(id3.size) + 10;
  //Serial.print("id3.size");
  //Serial.println(id3.size);

  // log_e("ID3 size: %u", id3.size);
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
  }

  //ID3 Frames:
  file.seek(lenRead);

  //Serial.println("Searching for frames");
  do {
    ID3FRAME frameheader;
    lenRead += file.readBytes((char *)&frameheader, sizeof(frameheader));
    frameheader.size = __builtin_bswap32(frameheader.size);

    if (frameheader.id[0] == 0 || frameheader.id[1] == 0 || frameheader.id[2] == 0) {
      //Serial.println("Blank data, break");
      numTagstoFind = 0;
      break;
    }

    //Serial.printf("ID3 Frame: %c%c%c%c size:%u\n", frameheader.id[0], frameheader.id[1], frameheader.id[2] , frameheader.id[3], frameheader.size);

    if (lenRead + frameheader.size >= id3.size) {
      Serial.println("End of ID3, break");
      numTagstoFind = 0;
      break;
    }

    //Serial.printf("ID3 Frame: %c%c%c%c size:%u\n", frameheader.id[0], frameheader.id[1], frameheader.id[2] , frameheader.id[3], frameheader.size);

    if (strncmp("TIT2", frameheader.id, 4) == 0) {
      //Serial.print("Found TIT2 - ");
      title = id3_getString(file, frameheader.size);
      //Serial.print("Title ");
      //Serial.println(title);
      numTagstoFind--;
    } else if (strncmp("TPE1", frameheader.id, 4) == 0) {
      artist = id3_getString(file, frameheader.size);
      //numTagstoFind--;
    } else if (strncmp("TALB", frameheader.id, 4) == 0) {
      //Serial.print("Found TALB - ");
      album = id3_getString(file, frameheader.size);
      //Serial.print("album ");
      //Serial.println(album);
      numTagstoFind--;
    } else if (strncmp("TCON", frameheader.id, 4) == 0) {
      //Serial.print("Found TLEN - ");
      genre = id3_getString(file, frameheader.size);
      numTagstoFind--;
    } else if (strncmp("TLEN", frameheader.id, 4) == 0) {
      Serial.print("Found TLEN - ");
      tracklen = id3_getString(file, frameheader.size);
      numTagstoFind--;
    }

    if (numTagstoFind == 0) {
      Serial.println("Found all searched tags");
      break;
    }

    lenRead += frameheader.size;
    file.seek(lenRead);
  } while (true);  //numTagstoFind > 0);
  return true;

end:
  //Skip to first mp3/aac frame:
  file.seek(id3.size);
  return true;
}

String MP3_Id3::getArtist() {
  return artist;
}

String MP3_Id3::getAlbum() {
  return album;
}

String MP3_Id3::getGenre() {
  return genre;
}

String MP3_Id3::getTitle() {
  return title;
}

/*
  id3_getString
  The first byte tells the encoding:
    $00   ISO-8859-1 [ISO-8859-1]. Terminated with $00.
    $01   UTF-16 [UTF-16] encoded Unicode [UNICODE] with BOM. All
        strings in the same frame SHALL have the same byteorder.
        Terminated with $00 00.
    $02   UTF-16BE [UTF-16] encoded Unicode [UNICODE] without BOM.
        Terminated with $00 00.
    $03   UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated with $00.
*/
static String MP3_Id3::id3_getString(File file, unsigned int len) {
  if (len > 127) len = 127;
  char buf[len + 1];
  file.readBytes(buf, len);
  buf[len] = 0;
  const auto encoding = buf[0];
  auto *s = &buf[1];
  len--;
  switch (encoding) {
    case 0x00:
      //Serial.println("Encoding is ISO-8859-1");
      break;
    //          return latin1UTF8(s);
    case 0x01:
      //Serial.println("Encoding is UTF-16 with BOM");
      return UTF16UTF8(s, len);
    case 0x02:
      //Serial.println("Encoding is UTF-16BE without BOM");
      return UTF16UTF8(s, len);
    case 0x03:
      //Serial.println("Encoding is UTF-8");
      break;
      //          return String(s);
  }

  return String(s);
}

String MP3_Id3::UTF16UTF8(const char *buf, const uint32_t len) {
  // converts unicode in UTF-8, buff contains the string to be converted up to len
  // range U+1 ... U+FFFF


  //if no BOM found, BE is default
  String out;
  out.reserve(len);


  auto *tmpbuf = (uint8_t *)malloc(len + 1);

  if (!tmpbuf)
    return String();  // out of memory;


  auto *t = tmpbuf;


  auto bitorder = false;  //Default to BE
  auto *p = (uint16_t *)buf;
  const auto *pe = (uint16_t *)&buf[len];
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
  out = (char *)tmpbuf;
  free(tmpbuf);
  return out;
}