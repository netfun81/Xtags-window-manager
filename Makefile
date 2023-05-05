#
# XTags - Makefile
#
include config.mk

PROG=xtags
SRC += ${PROG}.c client.c layout.c
OBJ = ${SRC:.c=.o} 

all: clean options ${PROG}

options:
	@echo Build options for XTags:
	@echo "CC       = ${CC}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

${PROG}: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

debug: clean
	@make CFLAGS='${DEBUG_CFLAGS}'

clean:
	@echo Cleaning for XTags..
	rm -f ${PROG} ${PROG}.core *.o

install: ${PROG}
	@echo Stripping the executable file..
	@strip -s ${PROG}
	@echo Installing to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f ${PROG} ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/${PROG}
	@echo Installing the manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@cp -f ${PROG}.1 ${DESTDIR}${MANPREFIX}/man1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/${PROG}.1
	@echo done.

uninstall:
	@echo Uninstalling ${PROG}..
	@echo Removing files from ${DESTDIR}${PREFIX}/bin..
	@rm -f ${DESTDIR}${PREFIX}/bin/${PROG}
	@echo Removing manual page from ${DESTDIR}${MANPREFIX}/man1..
	@rm -f ${DESTDIR}${MANPREFIX}/man1/${PROG}.1
	@echo done.

.PHONY: all options clean install uninstall

