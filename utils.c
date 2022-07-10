
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1
#include "utils.h"
#include "debug.h"

void _bindump(long offset,void *bptr,int len) {
	char line[128];
	unsigned char *buf = bptr;
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

char *strele(int num,char *delimiter,char *string) {
	static char return_info[1024];
	register char *src,*dptr,*eptr,*cptr;
	register char *dest, qc;
	register int count;

#ifdef DEBUG_STRELE
	printf("Element: %d, delimiter: %s, string: %s\n",num,delimiter,string);
#endif

	eptr = string;
	dptr = delimiter;
	dest = return_info;
	count = 0;
	qc = 0;
	for(src = string; *src != '\0'; src++) {
#ifdef DEBUG_STRELE
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
#ifdef DEBUG_STRELE
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
#ifdef DEBUG_STRELE
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
#ifdef DEBUG_STRELE
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
#ifdef DEBUG_STRELE
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
#ifdef DEBUG_STRELE
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
#ifdef DEBUG_STRELE
				printf("Count: %d, num: %d\n",count,num);
#endif
				if (count == num) break;
				if (cptr > src+1) src = cptr-1;
				eptr = src+1;
				count++;
//				printf("eptr[0]: %c\n", eptr[0]);
				if (*eptr == '\"' || *eptr == '\'') eptr++;
#ifdef DEBUG_STRELE
				printf("eptr: %s, src: %s\n",eptr,src+1);
#endif
			}
			dptr = delimiter;
		}
	}
#ifdef DEBUG_STRELE
	printf("Count: %d, num: %d\n",count,num);
#endif
	if (count == num) {
#ifdef DEBUG_STRELE
		printf("eptr: %s\n",eptr);
		printf("src: %s\n",src);
#endif
		while(eptr < src) {
			if (*eptr == '\"' || *eptr == '\'') break;
			*dest++ = *eptr++;
		}
	}
	*dest = '\0';
#ifdef DEBUG_STRELE
	printf("Returning: %s\n",return_info);
#endif
	return(return_info);
}

int is_ip(char *string) {
	register char *ptr;
	int dots,digits;

	dprintf(7,"string: %s\n", string);

	digits = dots = 0;
	for(ptr=string; *ptr; ptr++) {
		if (*ptr == '.') {
			if (!digits) goto is_ip_error;
			if (++dots > 4) goto is_ip_error;
			digits = 0;
		} else if (isdigit((int)*ptr)) {
			if (++digits > 3) goto is_ip_error;
		}
	}
	dprintf(7,"dots: %d\n", dots);

	return (dots == 4 ? 1 : 0);
is_ip_error:
	return 0;
}

FILE *logfp = (FILE *) 0;
static int logopts;

int log_open(char *ident,char *filename,int opts) {
	DPRINTF("filename: %s\n",filename);
	if (filename) {
		char *op;

		/* Open the file */
		op = (opts & LOG_CREATE ? "w+" : "a+");
		logfp = fopen(filename,op);
		if (!logfp) {
			perror("log_open: unable to create logfile");
			return 1;
		}
	} else if (opts & LOG_STDERR) {
		DPRINTF("logging to stderr\n");
		logfp = stderr;
	} else {
		DPRINTF("logging to stdout\n");
		logfp = stdout;
	}
	logopts = opts;

	DPRINTF("log is opened.\n");
	return 0;
}

int get_timestamp(char *ts, int tslen, int local) {
	struct tm *tptr;
	time_t t;
	char temp[32];
	int len;

	if (!ts || !tslen) return 1;

	/* Fill the tm struct */
	t = time(NULL);
	tptr = 0;
	DPRINTF("local: %d\n", local);
	if (local) tptr = localtime(&t);
	else tptr = gmtime(&t);
	if (!tptr) {
		DPRINTF("unable to get %s!\n",local ? "localtime" : "gmtime");
		return 1;
	}

	/* Set month to 1 if month is out of range */
	if (tptr->tm_mon < 0 || tptr->tm_mon > 11) tptr->tm_mon = 0;

	/* Fill string with yyyymmddhhmmss */
	sprintf(temp,"%04d-%02d-%02d %02d:%02d:%02d",
		1900+tptr->tm_year,tptr->tm_mon+1,tptr->tm_mday,
		tptr->tm_hour,tptr->tm_min,tptr->tm_sec);

	ts[0] = 0;
	len = tslen < strlen(temp)-1 ? strlen(temp)-1 : tslen; 
	DPRINTF("returning: %s\n", ts);
	strncat(ts,temp,len);
	return 0;
}

static char message[32767];

int log_write(int type,char *format,...) {
	va_list ap;
	char dt[32],error[128];
	register char *ptr;

	/* Make sure log_open was called */
	if (!logfp) log_open("",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);

	/* Do we even log this type? */
	DPRINTF("logopts: %0x, type: %0x\n",logopts,type);
	if ( (logopts | type) != logopts) return 0;

	/* get the error text asap before it's gone */
	if (type & LOG_SYSERR) {
		error[0] = 0;
		strncat(error,strerror(errno),sizeof(error));
	}

	/* Prepend the time? */
	ptr = message;
	if (logopts & LOG_TIME || type & LOG_TIME) {
//		struct tm *tptr;
//		time_t t;

		DPRINTF("prepending time...\n");
		get_timestamp(dt,sizeof(dt),1);
#if 0
		/* Fill the tm struct */
		t = time(NULL);
		tptr = 0;
		DPRINTF("getting localtime\n");
		tptr = localtime(&t);
		if (!tptr) {
			DPRINTF("unable to get localtime!\n");
			return 1;
		}

		/* Set month to 1 if month is out of range */
		if (tptr->tm_mon < 0 || tptr->tm_mon > 11) tptr->tm_mon = 0;

		/* Fill string with yyyymmddhhmmss */
		sprintf(dt,"%04d-%02d-%02d %02d:%02d:%02d",
			1900+tptr->tm_year,tptr->tm_mon+1,tptr->tm_mday,
			tptr->tm_hour,tptr->tm_min,tptr->tm_sec);
#endif

		strcat(dt,"  ");
		ptr += sprintf(ptr,"%s",dt);
	}

	/* If it's a warning, prepend warning: */
	if (type & LOG_WARNING) {
		DPRINTF("prepending warning...\n");
		sprintf(ptr,"warning: ");
		ptr += strlen(ptr);
	}

	/* If it's an error, prepend error: */
	else if ((type & LOG_ERROR) || (type & LOG_SYSERR)) {
		DPRINTF("prepending error...\n");
		sprintf(ptr,"error: ");
		ptr += strlen(ptr);
	}

	/* Build the rest of the message */
	DPRINTF("adding message...\n");
	va_start(ap,format);
	vsprintf(ptr,format,ap);
	va_end(ap);

	/* Trim */
	trim(message);

	/* If it's a system error, concat the system message */
	if (type & LOG_SYSERR) {
		DPRINTF("adding error text...\n");
		strcat(message,": ");
		strcat(message, error);
	}

	/* Strip all CRs and LFs */
	DPRINTF("stripping newlines...\n");
	for(ptr = message; *ptr; ptr++) {
		if (*ptr == '\n' || *ptr == '\r')
			strcpy(ptr,ptr+1);
	}

	/* Write the message */
	DPRINTF("message: %s\n",message);
	fprintf(logfp,"%s\n",message);
	fflush(logfp);
	return 0;
}

#ifndef WINDOWS
int lock_file(char *path, int wait) {
	struct flock fl;
	int fd,op;

	fd = open(path, O_CREAT|O_RDWR, 0666);
	if (fd < 0) {
		log_syserror("lock_file: open(%s)",path);
		return -1;
	}

	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 1;
	op = (wait ? F_SETLKW : F_SETLK);
	if (fcntl(fd,op,&fl) < 0) {
		log_syserror("lock_file: fcntl");
		close(fd);
		return -1;
	}
	return fd;
}

void unlock_file(int fd) {
	close(fd);
	return;
}
#endif
