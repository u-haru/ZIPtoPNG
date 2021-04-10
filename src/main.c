#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>


unsigned char sig_CEN[4] = "PK\x01\x02";
unsigned char sig_EOCD[4] = "PK\x05\x06";

bool big_endian;

#define SIZE_ZIP_CEN 46 //CENの固定長部分のサイズ
#define ZIP_CENNAM 28   //CEN内のファイル名サイズのoffset
#define ZIP_CENEXT 30   //CEN内の拡張情報サイズのoffset
#define ZIP_CENCOM 32   //CEN内のコメントサイズのoffset
#define ZIP_CENOFF 42   //CEN内のLOCのoffset
#define ZIP_ENDOFF 16   //EOCD内のCENのオフセットのoffset

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

uint32_t masc(unsigned int byte){
  if(byte >= 4) return 0xffffffff;
  return (uint32_t)((1<<(long long)(byte*8))-1);
}

uint32_t swap(uint32_t dat,size_t size){
	char *out = (char*) malloc(size);
  char *p = (char*)&dat;
  for(unsigned int i = size;i>0;i--)
	  out[i-1] = p[size-i];
  uint32_t val = *(uint32_t*)out & masc(size);
  free(out);
	return val;
}

unsigned char *memmem(unsigned char *buf_src,unsigned char *buf_find,size_t size_src,size_t size_find){
  for (unsigned char *search = buf_src; search < buf_src+size_src; search++){
    search = memchr(search, buf_find[0],buf_src-search+size_src);
    if (search == NULL)return NULL;
    if(memcmp(search,buf_find,size_find) == 0){
      return (unsigned char *)search;
    }
  }
  return NULL;
}

unsigned int readInt(unsigned char *src,size_t size,unsigned char end){
  bool big = 0;
  unsigned int val;
  if(end == 'B'||end == 'b') big=1;
  if(big^big_endian){
    memcpy(&val,src,size);
    val = swap(val,size);
  }else{
    memcpy(&val,src,size);
  }
  return val;
}
void writeInt(unsigned char *dst,unsigned int val,size_t size,unsigned char end){
  bool big = 0;
  if(end == 'B'||end == 'b') big=1;
  if(big^big_endian){
    val = swap(val,size);
    memcpy(dst,&val,size);
  }else{
    memcpy(dst,&val,size);
  }
}
unsigned int freadInt(FILE *fp,size_t size,char end){
  bool big = 0;
  unsigned int val;
  if(end == 'B'||end == 'b') big=1;
  if(big^big_endian){
    fread(&val, size, 1, fp);
    val = swap(val,size);
  }else{
    fread(&val, size, 1, fp);
  }
  return val;
}
void fwriteInt(FILE *fp,unsigned int val,size_t size,char end){
  bool big = 0;
  if(end == 'B'||end == 'b') big=1;
  if(big^big_endian){
    val = swap(val,size);
    fwrite(&val, size, 1, fp);
  }else{
    fwrite(&val, size, 1, fp);
  }
}
void set_sysendian(){
  int x = 1;
  if( *(char *)&x ){
    big_endian = 0;
  }else{
    big_endian=1;
  }
}

void readDat(FILE *fp){
  if (fseek(fp, 0, SEEK_END) != 0) {
    printf("fseek error");
    exit(1);
  }
  Content_len = ftell(fp);
  Content_dat = (unsigned char*) malloc(Content_len);
  if (Content_dat == NULL) {
    printf("malloc error");
    exit(1);
  }
  if (fseek(fp, 0L, SEEK_SET) != 0) {
    printf("fseek error");
    exit(1);
  }
  if (fseek(fp, 0L, SEEK_SET) != 0) {
    printf("fseek error");
    exit(1);
  }
  fread(Content_dat, 1, Content_len, fp);
}
void FixupZIP(unsigned int offset){
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
    LOC += offset + 8;
    // printf("LOC:%x\n",LOC);
    writeInt(position + ZIP_CENOFF,LOC,4,'L');

    size += SIZE_ZIP_CEN + readInt(position+ZIP_CENNAM,2,'L') + readInt(position+ZIP_CENEXT,2,'L') + readInt(position+ZIP_CENCOM,2,'L');
  }
  uint32_t zip_offset = pos_cen - Content_dat + offset + 8;
  writeInt(pos_eocd + ZIP_ENDOFF,zip_offset,4,'L');
}
int main(int argc,char* argv[]){
  FILE *fp;
  FILE *PNGfp;
  FILE *DATfp;
  set_sysendian();
  if(argc<3){
    printf("usage\n%s file.png hide.zip [output.zip.png]",argv[0]);
    return 0;
  }
  
	if ((PNGfp = fopen(argv[1], "rb")) == NULL) {
    printf("fopen error");
    return -1;
  }

	if ((DATfp = fopen(argv[2], "rb")) == NULL) {
    printf("fopen error");
    return -1;
  }
  readDat(DATfp);
  fclose(DATfp);
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
        FixupZIP(start_offset);
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