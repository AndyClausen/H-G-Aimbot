#include "selfdel.h"
#include <time.h>
#include <stdio.h>
#include <cstdio>
#include <Windows.h>

#define newFilenameLen 7

char *RandomString() {
	srand ((unsigned)time(NULL));
	char str[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	char ret[newFilenameLen +5];

	for (int i = 0; i < newFilenameLen; i++) {
		ret[i] = str[rand() % 62];
	} ret[newFilenameLen] = '.'; ret[newFilenameLen +1] = 'e'; ret[newFilenameLen +2] = 'x'; ret[newFilenameLen +3] = 'e'; ret[newFilenameLen +4] = 0x00;
	return ret;
}

unsigned int getLen(char *chr) {
	unsigned int i;
	for (i = 0; i < 255; i++)
		if (chr[i] == 0x00) break;
	return i;
}

void newFile(char *argv0) {
	char filename[255];

	unsigned int filenameLen = getLen(argv0);
	unsigned int i;
	for (i = filenameLen; i > 0; i--)
		if (argv0[i] == '\\') break;
	i++;

	memcpy(&filename, argv0 + i, filenameLen - i);
	filename[filenameLen - i] = 0x00;

	char buf[BUFSIZ];
	size_t size;

	FILE* source = fopen(filename, "rb");
	FILE* dest = fopen(RandomString(), "wb");

	unsigned short ush = 0;
	srand ((unsigned)time(NULL));
	ush = rand() % 16;

	while (size_t sizeo = fread(buf, 1, BUFSIZ -1, source)) {
		size = sizeo;
		fwrite(buf, 1, size, dest);
	}

	char buf2[16];
	ush = rand() % 16;
	fwrite(buf2, 1, ush, dest);

	fclose(source);
	fclose(dest);
}