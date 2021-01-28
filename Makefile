
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
SRCS=main.c module.c jbd_info.c jbd.c parson.c list.c utils.c $(TRANSPORTS)
OBJS=$(SRCS:.c=.o)
CFLAGS=-I$(MYBMM_SRC)
#CFLAGS+=-Wall -O2 -pipe
CFLAGS+=-Wall -g -DDEBUG=1
LIBS=-ldl
ifeq ($(MQTT),yes)
CFLAGS+=-DMQTT
ifeq ($(PI),yes)
LIBS+=./libpaho-mqtt3c.a
else
LIBS+=-lpaho-mqtt3c
endif
endif
ifeq ($(BLUETOOTH),yes)
CFLAGS+=-DBLUETOOTH
ifeq ($(PI),yes)
LIBS+=./libgattlib.a -lglib-2.0
else
LIBS+=-lgattlib -lglib-2.0
endif
endif
LIBS+=-lpthread
LDFLAGS+=-rdynamic

vpath %.c $(MYBMM_SRC)

.PHONY: all
all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

#$(OBJS): Makefile

include $(MYBMM_SRC)/Makefile.dep

debug: $(PROG)
	gdb ./$(PROG)

install: $(PROG)
	install -m 755 -o bin -g bin $(PROG) /usr/bin/$(PROG)

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull
