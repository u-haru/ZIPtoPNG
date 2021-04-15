#include <inttypes.h>
#include <stdio.h>
#include <memory.h>
#include "ByteIO.h"

unsigned char sig_CEN[4] = "PK\x01\x02";
unsigned char sig_EOCD[4] = "PK\x05\x06";

#define SIZE_ZIP_CEN 46 //CENの固定長部分のサイズ
#define ZIP_CENNAM 28   //CEN内のファイル名サイズのoffset
#define ZIP_CENEXT 30   //CEN内の拡張情報サイズのoffset
#define ZIP_CENCOM 32   //CEN内のコメントサイズのoffset
#define ZIP_CENOFF 42   //CEN内のLOCのoffset
#define ZIP_ENDOFF 16   //EOCD内のCENのオフセットのoffset

void FixupZIP(unsigned char *Content_dat,unsigned int Content_len,unsigned int offset){
  unsigned char *pos_cen = memmem(Content_dat, sig_CEN, Content_len, sizeof(sig_CEN));
  unsigned char *pos_eocd = memmem(Content_dat, sig_EOCD, Content_len, sizeof(sig_EOCD));
  if(pos_eocd == NULL || pos_eocd <=pos_cen){
    printf("this isn't zip?");
    return;
  }
  unsigned char buf[4];
  memcpy(buf, pos_cen, 4);
  if(memcmp(sig_CEN,buf,4)){
    printf("not match\n%s\n%s\nthis isn't zip?",sig_CEN,buf);
    return;
  }
  uint32_t size_cen = readInt(pos_eocd+12,4,'L');
  
  
  unsigned int size = 0;
  while(size < size_cen){
    unsigned char *position = pos_cen + size;
    uint32_t LOC = readInt(position+ZIP_CENOFF,4,'L');
    LOC += offset;
    // printf("LOC:%x\n",LOC);
    writeInt(position + ZIP_CENOFF,LOC,4,'L');

    size += SIZE_ZIP_CEN + readInt(position+ZIP_CENNAM,2,'L') + readInt(position+ZIP_CENEXT,2,'L') + readInt(position+ZIP_CENCOM,2,'L');
  }
  uint32_t zip_offset = pos_cen - Content_dat + offset;
  writeInt(pos_eocd + ZIP_ENDOFF,zip_offset,4,'L');
}