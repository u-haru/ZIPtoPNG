uint32_t mask(unsigned int byte);

uint32_t swap(uint32_t dat, size_t size);

unsigned char *memmem(unsigned char *buf_src, unsigned char *buf_find,size_t size_src, size_t size_find);

unsigned int readInt(unsigned char *src, size_t size, unsigned char end);

void writeInt(unsigned char *dst, unsigned int val, size_t size,unsigned char end);

unsigned int freadInt(FILE *fp, size_t size, char end);

void fwriteInt(FILE *fp, unsigned int val, size_t size, char end);

void set_sysendian();

unsigned char *readFile(char *file, unsigned int *Content_len);

