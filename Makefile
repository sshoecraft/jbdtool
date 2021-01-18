
PROG=jbdtool
MYBMM_SRC=../mybmm
TRANSPORTS=$(shell cat $(MYBMM_SRC)/Makefile | grep ^TRANSPORTS | awk -F= '{ print $$2 }')
SRCS=main.c module.c jbd.c parson.c list.c utils.c $(TRANSPORTS)
OBJS=$(SRCS:.c=.o)
CFLAGS=-DJBDTOOL -I$(MYBMM_SRC)
#CFLAGS+=-Wall -O2 -pipe
CFLAGS+=-Wall -g -DDEBUG
LIBS+=-ldl -lgattlib -lglib-2.0 -lpthread
LDFLAGS+=-rdynamic

vpath %.c $(MYBMM_SRC)

.PHONY: all
all: $(PROG)

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

#$(OBJS): Makefile *.h

debug: $(PROG)
	gdb ./$(PROG)

install: $(PROG)
	install -m 755 -o bin -g bin $(PROG) /usr/bin/$(PROG)

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

.PHONY: gitpush
gitpush:
	./gitpush $(MYBMM_SRC) $(TRANSPORTS)
