
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <ctype.h>

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

char *strele(int num,char *delimiter,char *string) {
	static char return_info[1024];
	register char *src,*dptr,*eptr,*cptr;
	register char *dest, qc;
	register int count;

#if DEBUG
	printf("Element: %d, delimiter: %s, string: %s\n",num,delimiter,string);
#endif

	eptr = string;
	dptr = delimiter;
	dest = return_info;
	count = 0;
	qc = 0;
	for(src = string; *src != '\0'; src++) {
#if DEBUG
		printf("src: %d, qc: %d\n", *src, qc);
#endif
		if (qc) {
			if (*src == qc) qc = 0;
			continue;
		} else {
			if (*src == '\"' || *src == '\'')  {
				qc = *src;
				cptr++;
			}
		}
		if (isspace(*src)) *src = 32;
#if DEBUG
		if (*src)
			printf("src: %c == ",*src);
		else
			printf("src: (null) == ");
		if (*dptr != 32)
			printf("dptr: %c\n",*dptr);
		else if (*dptr == 32)
			printf("dptr: (space)\n");
		else
			printf("dptr: (null)\n");
#endif
		if (*src == *dptr) {
			cptr = src+1;
			dptr++;
#if DEBUG
			if (*cptr != '\0')
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			while(*cptr == *dptr && *cptr != '\0' && *dptr != '\0') {
				cptr++;
				dptr++;
#if DEBUG
				if (*cptr != '\0')
					printf("cptr: %c == ",*cptr);
				else
					printf("cptr: (null) == ");
				if (*dptr != '\0')
					printf("dptr: %c\n",*dptr);
				else
					printf("dptr: (null)\n");
#endif
				if (*dptr == '\0' || *cptr == '\0') {
#if DEBUG
					printf("Breaking...\n");
#endif
					break;
				}
/*
				dptr++;
				if (*dptr == '\0') break;
				cptr++;
				if (*cptr == '\0') break;
*/
			}
#if DEBUG
			if (*cptr != '\0')
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			if (*dptr == '\0') {
#if DEBUG
				printf("Count: %d, num: %d\n",count,num);
#endif
				if (count == num) break;
				if (cptr > src+1) src = cptr-1;
				eptr = src+1;
				count++;
//				printf("eptr[0]: %c\n", eptr[0]);
				if (*eptr == '\"' || *eptr == '\'') eptr++;
#if DEBUG
				printf("eptr: %s, src: %s\n",eptr,src+1);
#endif
			}
			dptr = delimiter;
		}
	}
#if DEBUG
	printf("Count: %d, num: %d\n",count,num);
#endif
	if (count == num) {
#if DEBUG
		printf("eptr: %s\n",eptr);
		printf("src: %s\n",src);
#endif
		while(eptr < src) {
			if (*eptr == '\"' || *eptr == '\'') break;
			*dest++ = *eptr++;
		}
	}
	*dest = '\0';
#if DEBUG
	printf("Returning: %s\n",return_info);
#endif
	return(return_info);
}
