# CircleMUD Makefile.in - Makefile template used by 'configure'
#
# $CVSHeader: cwg/rasputin/src/Makefile.in,v 1.3 2004/12/21 06:15:40 fnord Exp $

# C compiler to use
CC = gcc

# Path to cxref utility
CXREF = cxref

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

ENGINE = circle

CFLAGS = -ggdb -O2 $(MYFLAGS) $(PROFILE)

LIBRARIES =  -lcrypt -lz  -lmysqlclient

SRC_FILES = ${wildcard *.c}

OBJFILES = $(patsubst %.c, %.o, ${SRC_FILES})

# How to do the Make

default: all

all: .accepted
	$(MAKE) $(BINDIR)/${ENGINE}
	$(MAKE) utils

.accepted: licheck
	@./licheck less

licheck: licheck.in
	@cp licheck.in licheck
	@chmod 755 licheck

utils: .accepted
	(cd util; $(MAKE) all)

circle:
	$(MAKE) $(BINDIR)/${ENGINE}

$(BINDIR)/${ENGINE} : $(OBJFILES)
	$(CC) $(CFLAGS) -o $(BINDIR)/${ENGINE} $(PROFILE) $(OBJFILES) $(LIBRARIES)

clean:
	rm -f *.o


# Dependencies for the object files (automagically generated with
# gcc -MM)

depend:
	$(CC) -MM *.c > depend

-include depend
