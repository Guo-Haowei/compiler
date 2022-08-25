#ifndef __STRING_H__
#define __STRING_H__

void* memcpy(void* dest, void* src, int n);
void* memset(void* dest, int val, int n);

int strcmp(char* str1, char* str2);
int strncmp(char* str1, char* str2, int n);

int strlen(char* str);

char* strcpy(char* dest, char* src);
char* strncpy(char* dest, char* src, int n);
char* strchr(char* str, int _Val);
char* strrchr(char* str, int _Val);

char* strdup(char* src);

#endif