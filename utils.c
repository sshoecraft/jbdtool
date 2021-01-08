
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void _bindump(long offset,void *bptr,int len) {
	char line[128], *buf = bptr;
	int end;
	register char *ptr;
	register int x,y;

//	printf("buf: %p, len: %d\n", buf, len);
#ifdef __WIN32__
	if (buf == (void *)0xBAADF00D) return;
#endif

	for(x=y=0; x < len; x += 16) {
		sprintf(line,"%04lX: ",offset);
		ptr = line + strlen(line);
		end=(x+16 >= len ? len : x+16);
		for(y=x; y < end; y++) {
			sprintf(ptr,"%02X ",buf[y]);
			ptr += 3;
		}
		for(y=end; y < x+17; y++) {
			sprintf(ptr,"   ");
			ptr += 3;
		}
		for(y=x; y < end; y++) {
			if (buf[y] > 31 && buf[y] < 127)
				*ptr++ = buf[y];
			else
				*ptr++ = '.';
		}
		for(y=end; y < x+16; y++) *ptr++ = ' ';
		*ptr = 0;
		printf("%s\n",line);
		offset += 16;
	}
}

void bindump(char *label,void *bptr,int len) {
	printf("%s(%d):\n",label,len);
	_bindump(0,bptr,len);
}

char *trim(char *string) {
	register char *src,*dest;

	/* If string is empty, just return it */
	if (*string == '\0') return string;

	/* Trim the front */
	src = string;
//	while(isspace((int)*src) && *src != '\t') src++;
	while(isspace((int)*src) || (*src > 0 && *src < 32)) src++;
	dest = string;
	while(*src != '\0') *dest++ = *src++;

	/* Trim the back */
	*dest-- = '\0';
	while((dest >= string) && isspace((int)*dest)) dest--;
	*(dest+1) = '\0';

	return string;
}
