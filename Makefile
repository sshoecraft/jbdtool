
DEBUG=yes
BLUETOOTH=yes
MQTT=yes
PI=$(shell test $$(cat /proc/cpuinfo  | grep ^model | grep -c ARM) -gt 0 && echo yes)

PROG=jbdtool
MYBMM_SRC=../mybmm
TRANSPORTS=$(shell cat $(MYBMM_SRC)/Makefile | grep ^TRANSPORTS | head -1 | awk -F= '{ print $$2 }')
ifneq ($(BLUETOOTH),yes)
_TMPVAR := $(TRANSPORTS)
TRANSPORTS = $(filter-out bt.c, $(_TMPVAR))
endif
SRCS=main.c module.c jbd_info.c jbd.c parson.c list.c utils.c cfg.c daemon.c $(TRANSPORTS)
CFLAGS=-DJBDTOOL -I$(MYBMM_SRC) -DSTATIC_MODULES

ifeq ($(DEBUG),yes)
CFLAGS+=-Wall -g -DDEBUG=1
else
CFLAGS+=-Wall -O2 -pipe
endif
LIBS=-ldl

ifeq ($(MQTT),yes)
SRCS+=mqtt.c
CFLAGS+=-DMQTT
ifeq ($(PI),yes)
LIBS+=./libpaho-mqtt3c.a
DEPS=./libpaho-mqtt3c.a
else
LIBS+=-lpaho-mqtt3c
endif
endif

ifeq ($(BLUETOOTH),yes)
CFLAGS+=-DBLUETOOTH
ifeq ($(PI),yes)
LIBS+=./libgattlib.a -lglib-2.0
DEPS=./libgattlib.a
else
LIBS+=-lgattlib -lglib-2.0
endif
endif
LIBS+=-lpthread
LDFLAGS+=-rdynamic
OBJS=$(SRCS:.c=.o)

ifeq ($(PI),yes)
LDFLAGS+=-static
endif

vpath %.c $(MYBMM_SRC)

.PHONY: all
all: $(PROG)

# Set CC to gcc if not set
CC ?= gcc

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

#$(OBJS): Makefile

include $(MYBMM_SRC)/Makefile.dep

debug: $(PROG)
	gdb ./$(PROG)

install: $(PROG)
	sudo install -m 755 -o bin -g bin $(PROG) /usr/bin/$(PROG)

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

zip: $(PROG)
	rm -f jbdtool_pi_static.zip
	zip jbdtool_pi_static.zip $(PROG)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull
