
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "mybmm.h"

struct serial_info {
	int fd;
	char device[MYBMM_TARGET_LEN+1];
	int baud,parity,stop;
};
typedef struct serial_info serial_info_t;

static int set_interface_attribs (int fd, int speed, int data, int parity, int stop, int vmin, int vtime) {
        struct termios tty;
        if (tcgetattr (fd, &tty) != 0) {
                printf("error %d from tcgetattr\n", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = vmin;		// min num of chars before returning
        tty.c_cc[VTIME] = vtime;	// useconds to wait for a char

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;		/* No stop bits */
        tty.c_cflag |= stop;
        tty.c_cflag &= ~CRTSCTS; /* No CTS */

        if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		printf("error %d from tcsetattr\n", errno);
                return -1;
        }
        return 0;
}

static int serial_init(mybmm_config_t *conf) {
	return 0;
}

static void *serial_new(mybmm_config_t *conf, ...) {
	serial_info_t *s;
	char device[MYBMM_TARGET_LEN+1],*opts;
	int baud,parity,stop;
#ifndef JBDTOOL
        struct cfg_proctab serialtab[] = {
		{ device, "data", "Data bits", DATA_TYPE_INT,&stop,0,"8" },
		{ device, "sync", "Sync bits", DATA_TYPE_INT,&sync,0,"0" },
		{ device, "parity", "Parity bits", DATA_TYPE_INT,&parity,0,"1" },
		{ device, "baud", "Baud rate", DATA_TYPE_INT,&baud,0,"9600" },
		CFG_PROCTAB_END
	};
#endif
	va_list(ap);
	char *target;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	opts = va_arg(ap,char *);
	va_end(ap);
	dprintf(1,"target: %s, opts: %s\n", target, opts);

	device[0] = 0;
	strncat(device,target,MYBMM_TARGET_LEN);
#ifndef JBDTOOL
	cfg_get_tab(conf->cfg,serialtab);
        if (debug >= 0) cfg_disp_tab(serialtab,0,1);
#else
	baud=atoi(opts);
	stop=8;
	parity=1;
#endif

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("serial_new: malloc");
		return 0;
	}
	s->fd = -1;
	strcpy(s->device,device);
	s->baud = baud;
	s->stop = stop;
	s->parity = parity;

	return s;
}

static int serial_open(void *handle) {
	serial_info_t *s = handle;
	char path[256];

	if ((access(s->device,0)) == 0) {
		if (strncmp(s->device,"/dev",4) != 0) {
			sprintf(path,"/dev/%s",s->device);
			dprintf(1,"path: %s\n", path);
			s->fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
		} else {
			s->fd = open(s->device,O_RDWR | O_NOCTTY | O_SYNC);
		}
	} else {
		s->fd = open(s->device,O_RDWR | O_NOCTTY | O_SYNC);
	}
	if (s->fd < 0) {
		printf("error %d opening %s: %s", errno, path, strerror(errno));
		return 1;
	}
	set_interface_attribs(s->fd, B9600, 8, 1, 0, 1, 5);

	return 0;
}

static int serial_read(void *handle, ...) {
	serial_info_t *s = handle;
	uint8_t *buf, ch;
	int buflen, bytes, bidx, num;
	struct timeval tv;
	va_list ap;
	fd_set rdset;

	tv.tv_usec = 0;
	tv.tv_sec = 1;

        /* Do a select on the socket */
//        numset = select(set->maxfd+1,(fd_set *)&set->fdset,(fd_set *)0, (fd_set *)0,tptr);

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);
	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	FD_ZERO(&rdset);
	dprintf(5,"reading...\n");
	bidx=0;
	while(1) {
		FD_SET(s->fd,&rdset);
		num = select(s->fd+1,&rdset,0,0,&tv);
		dprintf(5,"num: %d\n", num);
		if (!num) break;
		bytes = read(s->fd, &ch, 1);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) {
			bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
			dprintf(5,"ch: %02x\n", ch);
			buf[bidx++] = ch;
			if (bidx >= buflen) break;
		}
	}
#if 0
	bidx=0;
	while(1) {
		bytes = read(s->fd, &ch, 1);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) {
			bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
			dprintf(5,"ch: %02x\n", ch);
			buf[bidx++] = ch;
			if (bidx >= buflen) break;
		}
		usleep(100);
	}
#endif

//	bindump("serial",buf,bidx);
	dprintf(1,"returning: %d\n", bidx);
	return bidx;
}

static int serial_write(void *handle, ...) {
	serial_info_t *s = handle;
	uint8_t *buf;
	int bytes,buflen;
	va_list ap;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(1,"writing...\n");
	bytes = write(s->fd,buf,buflen);
	dprintf(1,"bytes: %d\n", bytes);
	usleep ((buflen + 25) * 100);
	return bytes;
}

static int serial_close(void *handle) {
	serial_info_t *s = handle;

	close(s->fd);
	return 0;
}

mybmm_module_t serial_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"serial",
	0,
	serial_init,
	serial_new,
	serial_open,
	serial_read,
	serial_write,
	serial_close
};
