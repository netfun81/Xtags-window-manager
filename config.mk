#
# XTags build configuration
#
VERSION=0.5

PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

INCS = -I. -I/usr/include -I/usr/local/include 

LDFLAGS = -lX11

# NetBSD
#INCS += -I/usr/pkg/include
#LDFLAGS += -L/usr/pkg/lib -R/usr/pkg/lib

CFLAGS = -fcommon -Os ${INCS} -DVERSION=\"${VERSION}\"
DEBUG_CFLAGS =	${CFLAGS} -Wpointer-arith -Wcast-align -Wwrite-strings		\
		-Wstrict-prototypes -Wmissing-prototypes -Wnested-externs	\
		-Winline -Wdisabled-optimization -Wundef -Wendif-labels		\
		-Wshadow -pedantic -g -ggdb -W -Wall

CC = cc

