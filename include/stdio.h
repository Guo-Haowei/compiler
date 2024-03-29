#ifndef __STDIO_H__
#define __STDIO_H__

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0

typedef struct {
    void* _Placeholder;
    char* _ptr;
    int _cnt;
    char* _base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char* _tmpfname;
} FILE;

int printf(char* fmt, ...);
int fprintf(void* stream, char* fmt, ...);
int sprintf(char* buf, char* fmt, ...);
int snprintf(char* buf, int n, char* fmt, ...);

FILE* fopen(char* filename, char* mode);
int fseek(FILE* file, long offset, int origin);
long ftell(FILE* file);
int fread(void* dest, int size, int n, FILE* f);
int fclose(FILE* file);
int remove(char* _FileName);

#endif