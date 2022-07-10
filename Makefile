
DEBUG=yes
BLUETOOTH=yes
MQTT=yes

ifeq ($(TARGET),win32)
	CC = /usr/bin/i686-w64-mingw32-gcc
	CFLAGS+=-D_WIN32 -DWINDOWS
	WINDOWS=yes
	EXT=.exe
else ifeq ($(TARGET),win64)
	CC = /usr/bin/x86_64-w64-mingw32-gcc
	CFLAGS+=-D_WIN64 -DWINDOWS
	WINDOWS=yes
	EXT=.exe
else ifeq ($(TARGET),pi)
	CC = /usr/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
	LINUX=yes
else
	CC = gcc
	LINUX=yes
endif

PROG=jbdtool$(EXT)
SRCS=main.c jbd_info.c jbd.c parson.c list.c utils.c cfg.c daemon.c module.c ip.c serial.c bt.c can.c 

ifeq ($(DEBUG),yes)
CFLAGS+=-Wall -g -DDEBUG=1
else
CFLAGS+=-Wall -O2 -pipe
endif
#LIBS=-ldl

STATIC=no
ifeq ($(MQTT),yes)
	SRCS+=mqtt.c
	CFLAGS+=-DMQTT
	ifeq ($(WINDOWS),yes)
		ifeq ($(STATIC),yes)
			LIBS+=-lpaho-mqtt3c-static
		else
			LIBS+=-lpaho-mqtt3c
		endif
		LIBS+=-lws2_32
		ifeq ($(STATIC),yes)
			LIBS+=-lgdi32 -lcrypt32 -lrpcrt4 -lkernel32
		endif
	else
		LIBS+=-lpaho-mqtt3c
	endif
endif

ifneq ($(WINDOWS),yes)
ifeq ($(BLUETOOTH),yes)
	CFLAGS+=-DBLUETOOTH
	LIBS+=-lgattlib -lgio-2.0 -lgobject-2.0 -lgmodule-2.0 -lglib-2.0
	# XXX in order to static link on my RPI4 64-bit debian 11 box
	# download https://github.com/util-linux/util-linux
	# ./configure --disable-shared --enable-static
	# make
	# cp ./.libs/libmount.a /usr/lib/aarch64-linux-gnu/
	ifeq ($(STATIC),yes)
		LIBS+=-lffi -lpcre -lpthread -lmount -ldl -lblkid -lresolv -lz -lselinux
	endif
endif
endif

LIBS+=-lpthread
#LDFLAGS+=-rdynamic
OBJS=$(SRCS:.c=.o)

ifeq ($(STATIC),yes)
	LDFLAGS+=-static
endif

.PHONY: all
all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

#$(OBJS): Makefile

DEPDIR := .deps
CLEANFILES+=.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

main.o: $(SRCS)
	@mkdir -p .deps
	@echo "#define BUILD $$(date '+%Y%m%d%H%M')LL" > build.h
	$(COMPILE.c) main.c

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))

debug: $(PROG)
	gdb ./$(PROG)

install: $(PROG)
	sudo install -m 755 -o bin -g bin $(PROG) /usr/bin/$(PROG)

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES) build.h

zip: $(PROG)
	rm -f jbdtool_$(TARGET)_static.zip
	zip jbdtool_$(TARGET)_static.zip $(PROG)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull


