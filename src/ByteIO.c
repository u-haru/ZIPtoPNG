#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

bool big_endian;

uint32_t mask(unsigned int byte) {
    if (byte >= 4) return 0xffffffff;
    return (uint32_t)((1 << (long long)(byte * 8)) - 1);
}

uint32_t swap(uint32_t dat, size_t size) {
    char *out = (char *)malloc(size);
    char *p = (char *)&dat;
    for (unsigned int i = size; i > 0; i--) out[i - 1] = p[size - i];
    uint32_t val = *(uint32_t *)out & mask(size);
    free(out);
    return val;
}

unsigned char *memmem(unsigned char *buf_src, unsigned char *buf_find,
                      size_t size_src, size_t size_find) {
    for (unsigned char *search = buf_src; search < buf_src + size_src;
         search++) {
        search = memchr(search, buf_find[0], buf_src - search + size_src);
        if (search == NULL) return NULL;
        if (memcmp(search, buf_find, size_find) == 0) {
            return (unsigned char *)search;
        }
    }
    return NULL;
}

unsigned int readInt(unsigned char *src, size_t size, unsigned char end) {
    bool big = 0;
    unsigned int val;
    if (end == 'B' || end == 'b') big = 1;
    if (big ^ big_endian) {
        memcpy(&val, src, size);
        val = swap(val, size);
    } else {
        memcpy(&val, src, size);
    }
    return val;
}

void writeInt(unsigned char *dst, unsigned int val, size_t size,
              unsigned char end) {
    bool big = 0;
    if (end == 'B' || end == 'b') big = 1;
    if (big ^ big_endian) {
        val = swap(val, size);
        memcpy(dst, &val, size);
    } else {
        memcpy(dst, &val, size);
    }
}

unsigned int freadInt(FILE *fp, size_t size, char end) {
    bool big = 0;
    unsigned int val;
    if (end == 'B' || end == 'b') big = 1;
    if (big ^ big_endian) {
        fread(&val, size, 1, fp);
        val = swap(val, size);
    } else {
        fread(&val, size, 1, fp);
    }
    return val;
}

void fwriteInt(FILE *fp, unsigned int val, size_t size, char end) {
    bool big = 0;
    if (end == 'B' || end == 'b') big = 1;
    if (big ^ big_endian) {
        val = swap(val, size);
        fwrite(&val, size, 1, fp);
    } else {
        fwrite(&val, size, 1, fp);
    }
}

void set_sysendian() {
    int x = 1;
    if (*(char *)&x) {
        big_endian = 0;
    } else {
        big_endian = 1;
    }
}

unsigned char *readFile(char file[], unsigned int *Content_len) {
    FILE *fp;
    if ((fp = fopen(file, "rb")) == NULL) {
        printf("fopen error");
        exit(1);
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("fseek error");
        exit(1);
    }
    *Content_len = ftell(fp);
    unsigned char *Content_dat = (unsigned char *)malloc((size_t)Content_len);
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
    fread(Content_dat, 1, (size_t)Content_len, fp);
    fclose(fp);
    return Content_dat;
}