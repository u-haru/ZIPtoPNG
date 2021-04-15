#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "FixupZIP.h"
#include "ByteIO.h"


unsigned int Content_len;
unsigned char Content_type[4];
unsigned char *Content_dat;
uint32_t Content_crc32;

uint32_t crc32(unsigned char *dat,size_t size,uint32_t crc) {
  //https://qiita.com/mikecat_mixc/items/e5d236e3a3803ef7d3c5
  uint32_t magic = UINT32_C(0xEDB88320); /* 反転したマジックナンバー */
  uint32_t table[256];                   /* 下位8ビットに対応する値を入れるテーブル */
  int i, j;

  /* テーブルを作成する */
  for (i = 0; i < 256; i++) {      /* 下位8ビットそれぞれについて計算する */
    uint32_t table_value = i;      /* 下位8ビットを添え字に、上位24ビットを0に初期化する */
    for (j = 0; j < 8; j++) {
      int b = (table_value & 1);   /* 上(反転したので下)から1があふれるかをチェックする */
      table_value >>= 1;           /* シフトする */
      if (b) table_value ^= magic; /* 1があふれたらマジックナンバーをXORする */
    }
    table[i] = table_value;        /* 計算した値をテーブルに格納する */
  }

  /* テーブルを用いてCRC32を計算する */
  for (i = 0; i<size; i++) {
    crc = table[(crc ^ dat[i]) & 0xff] ^ (crc >> 8); /* 1バイト投入して更新する */
  }
  return ~crc;
}

int main(int argc,char* argv[]){
  FILE *fp;
  FILE *PNGfp;
  set_sysendian();
  if(argc<3){
    printf("usage\n%s file.png hide.zip [output.zip.png]",argv[0]);
    return 0;
  }
  
	if ((PNGfp = fopen(argv[1], "rb")) == NULL) {
    printf("fopen error");
    return -1;
  }

  Content_dat = readFile(argv[2],&Content_len);

  if(argc>3){
    if ((fp = fopen(argv[3], "wb")) == NULL) {
      printf("fopen error");
      return -1;
    }
  }else if ((fp = fopen("output.zip.png", "wb")) == NULL) {
    printf("fopen error");
    return -1;
  }
  fwrite("\x89PNG\r\n\x1a\n", 8, 1, fp);
  fseek(PNGfp, 8, SEEK_SET);
  uint32_t chunk_len;
  uint32_t buf_len = 0x100000;
  char chunk_type[4];
	char *chunk_data = (char*) malloc(0x100000);
  uint32_t chunk_crc32;

  while(1){
    chunk_len = freadInt(PNGfp,4,'B');
    fread(chunk_type,4,1,PNGfp);
    if(buf_len<chunk_len){//バッファ足りなければリサイズ
      char *tmp = (char *)realloc( chunk_data, chunk_len );
      if( tmp == NULL ){
        printf("メモリ確保に失敗しました\n");
        free(chunk_data);
        free(Content_dat);
        return -1;
      }
      else{
        chunk_data = tmp;
        buf_len = chunk_len;
      }
    }
    fread(chunk_data,chunk_len,1,PNGfp);
	  chunk_crc32 = freadInt(PNGfp,4,'B');

    if(memcmp(chunk_type,"IEND",4)== 0){
      uint32_t start_offset = ftell(PNGfp)-4-chunk_len-4-4;//now-crc-data-type-len
      // printf("offset:%x\n",start_offset);
      memcpy(Content_type,"IDAT",4);

      if(strstr(argv[2],".zip")!=NULL){
        FixupZIP(Content_dat,Content_len,start_offset);
        // memcpy(Content_type,"ziPc",4);
      }

      fwriteInt(fp,Content_len,4,'B');
      fwrite(Content_type,4,1,fp);
      fwrite(Content_dat,Content_len,1,fp);

      Content_crc32 = crc32(Content_type,4,UINT32_C(0xFFFFFFFF));
      Content_crc32 = crc32(Content_dat,Content_len,~Content_crc32);

      // printf("CRC:%x\n",Content_crc32);
      fwriteInt(fp,Content_crc32,4,'B');
    }

    fwriteInt(fp,chunk_len,4,'B');
    fwrite(chunk_type,4,1,fp);
    fwrite(chunk_data,chunk_len,1,fp);
    fwriteInt(fp,chunk_crc32,4,'B');
    
    if(memcmp(chunk_type,"IEND",4) == 0) break;
  }
  
  free(chunk_data);
  free(Content_dat);
  fclose(PNGfp);
  fclose(fp);
  return 0;
}