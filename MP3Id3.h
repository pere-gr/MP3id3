/*
  Based on the code found here https://forum.pjrc.com/index.php?threads/read-id3-tags-from-audio-files.69298/
*/

#include <cstdint>
#include <SD.h>

// https://id3.org/id3v2.4.0-structure

typedef struct
{
  char id[3];          //"ID3"
  uint8_t version[2];  //04 00 Version 4
  uint8_t flag_unsynchronization : 1;
  uint8_t flag_extendedheader : 1;
  uint8_t flag_experimental : 1;
  uint8_t flag_footerpresent : 1;
  uint8_t flags_zero : 4;
  uint32_t size;
} __attribute__((packed)) ID3TAG;

typedef struct
{
  uint32_t size;
  uint8_t numFlagBytes;
  uint8_t flag_zero1 : 1;
  uint8_t flag_update : 1;
  uint8_t flag_crc : 1;
  uint8_t flag_restrictions : 1;
  uint8_t flag_zero2 : 4;
} __attribute__((packed)) ID3TAG_EXTHEADER;

typedef struct
{
  char id[4];  //Tag ID
  uint32_t size;
  uint8_t flags[2];  //Several bits.. not needed here
} __attribute__((packed)) ID3FRAME;

struct TAGdata {
  char tag[3];
  char title[30];
  char artist[30];
  char album[30];
  char year[4];
  char comment[30];
  int genre;
};



/* ----------------------------------------------- MP3id3.h -----------------------------------------------*/

class MP3_Id3 {
public:
  bool read(File& file);
  bool read(char *fileName);
  char *album();
  char *artist();
  char *genre();
  char *title();

private:
  char *charUTF16UTF8(const char *buf, const uint32_t len);
  char* getTagData(char *bufData, unsigned int len);
  bool isFrameId(const char *id, ID3FRAME frame);
  bool isV2(ID3TAG header);
  char *readFrameData(File file, unsigned int size);
  ID3TAG readHeaderV2(File& file);
  bool readV1(File& file);
  bool readV2(File& file, ID3TAG id3);  
  char* tagAlbum;
  char* tagArtist;
  char* tagTitle;
  char* tagGenre;
  long lenRead = 0;
  ID3TAG Tag;
  ID3TAG_EXTHEADER ExtHeader;
  ID3FRAME Frame;
};