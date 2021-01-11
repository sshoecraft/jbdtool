
PROG=jbdtool
MYBMM_SRC=../mybmm
MODULES=$(shell cat $(MYBMM_SRC)/Makefile | grep ^MODULES | awk -F= '{ print $$2 }')
SRCS=main.c module.c jbd.c parson.c list.c utils.c $(MODULES)
OBJS=$(SRCS:.c=.o)
CFLAGS=-DJBDTOOL -I$(MYBMM_SRC)
CFLAGS+=-Wall -O2 -pipe
#CFLAGS+=-Wall -g -DDEBUG
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
	sudo install -m 755 -o bin -g bin $(PROG) /usr/bin/$(PROG)

clean:
	rm -rf $(PROG) $(OBJS) $(CLEANFILES)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull
