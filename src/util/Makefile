# CircleMUD Makefile.in - Makefile template used by 'configure'
# for the 'util' directory

# C compiler to use
CC = C:/cygwin64/bin/gcc.exe

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -Wno-char-subscripts -DCIRCLE_UTIL

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

# binary destination directory
BINDIR = ../../bin
# location of Circle include files
INCDIR = ..

CFLAGS = -g3 -gdwarf-2 $(MYFLAGS) $(PROFILE) -I$(INCDIR)
LIBS = -lcrypt

default: all

all: $(BINDIR)/asciipasswd $(BINDIR)/autowiz $(BINDIR)/delobjs \
	$(BINDIR)/play2to3 \
	$(BINDIR)/shopconv \
	$(BINDIR)/showplay $(BINDIR)/sign $(BINDIR)/split $(BINDIR)/wld2html

asciipasswd: $(BINDIR)/asciipasswd

autowiz: $(BINDIR)/autowiz

delobjs: $(BINDIR)/delobjs

listrent: $(BINDIR)/listrent

play2to3: $(BINDIR)/play2to3

plrtoascii: $(BINDIR)/plrtoascii

purgeplay: $(BINDIR)/purgeplay

shopconv: $(BINDIR)/shopconv

showplay: $(BINDIR)/showplay

sign: $(BINDIR)/sign

split: $(BINDIR)/split

wld2html: $(BINDIR)/wld2html

$(BINDIR)/asciipasswd: asciipasswd.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h $(INCDIR)/utils.h
	$(CC) $(CFLAGS) -o $(BINDIR)/asciipasswd asciipasswd.c $(LIBS)

$(BINDIR)/autowiz: autowiz.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h $(INCDIR)/utils.h $(INCDIR)/db.h
	$(CC) $(CFLAGS) -o $(BINDIR)/autowiz autowiz.c $(LIBS)

$(BINDIR)/delobjs: delobjs.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h $(INCDIR)/utils.h
	$(CC) $(CFLAGS) -o $(BINDIR)/delobjs delobjs.c $(LIBS)

$(BINDIR)/listrent: listrent.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h
	$(CC) $(CFLAGS) -o $(BINDIR)/listrent listrent.c $(LIBS)

$(BINDIR)/mudpasswd: mudpasswd.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h $(INCDIR)/utils.h
	$(CC) $(CFLAGS) -o $(BINDIR)/mudpasswd mudpasswd.c $(LIBS)

$(BINDIR)/play2to3: play2to3.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h
	$(CC) $(CFLAGS) -o $(BINDIR)/play2to3 play2to3.c $(LIBS)

$(BINDIR)/plrtoascii: plrtoascii.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/db.h $(INCDIR)/pfdefaults.h
	$(CC) $(CFLAGS) -o $(BINDIR)/plrtoascii plrtoascii.c $(LIBS)

$(BINDIR)/purgeplay: purgeplay.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h $(INCDIR)/utils.h
	$(CC) $(CFLAGS) -o $(BINDIR)/purgeplay purgeplay.c $(LIBS)

$(BINDIR)/shopconv: shopconv.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h $(INCDIR)/db.h $(INCDIR)/utils.h $(INCDIR)/shop.h
	$(CC) $(CFLAGS) -o $(BINDIR)/shopconv shopconv.c $(LIBS)

$(BINDIR)/showplay: showplay.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h \
	$(INCDIR)/structs.h
	$(CC) $(CFLAGS) -o $(BINDIR)/showplay showplay.c $(LIBS)

$(BINDIR)/sign: sign.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h
	$(CC) $(CFLAGS) -o $(BINDIR)/sign sign.c $(LIBS)

$(BINDIR)/split: split.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h
	$(CC) $(CFLAGS) -o $(BINDIR)/split split.c $(LIBS)

$(BINDIR)/wld2html: wld2html.c $(INCDIR)/conf.h $(INCDIR)/sysdep.h
	$(CC) $(CFLAGS) -o $(BINDIR)/wld2html wld2html.c $(LIBS)
