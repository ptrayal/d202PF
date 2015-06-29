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

C_FILES = ${wildcard *.c}

OBJFILES = $(patsubst %.c, %.o, ${C_FILES})

CXREF_FILES = \
	account.c act.comm.c act.informative.c act.item.c act.movement.c act.offensive.c \
	act.other.c act.social.c act.wizard.c aedit.c alias.c asciimap.c \
	assedit.c assemblies.c auction.c auction_house.c ban.c boards.c \
	bsd-snprintf.c cedit.c char_descs.c clan.c clanedit.c class.c \
	comm.c config.c constants.c context_help.c db.c deities.c \
	dg_comm.c dg_db_scripts.c dg_event.c dg_handler.c dg_misc.c dg_mobcmd.c \
	dg_objcmd.c dg_olc.c dg_scripts.c dg_triggers.c dg_variables.c dg_wldcmd.c \
	feats.c fight.c gedit.c gengld.c genmob.c genobj.c \
	genolc.c genqst.c genshp.c genwld.c genzon.c graph.c \
	grid.c guild.c handler.c hedit.c hints.c house.c \
	htree.c improved-edit.c interpreter.c limits.c magic.c mail.c \
	medit.c memorization.c mobact.c modify.c modules.c new_mail.c \
	note.c oasis.c oasis_copy.c oasis_delete.c oasis_list.c objsave.c \
	oedit.c olc.c pets.c player_guilds.c players.c polls.c \
	protocol.c qedit.c quest.c races.c random.c redit.c \
	sedit.c shop.c spec_assign.c spec_procs.c spell_parser.c spells.c \
	statedit.c tedit.c transport.c treasure.c utils.c vehicles.c \
	weather.c zedit.c

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


# Not sure this is really used.  Just leaving this at the bottom of the code.
ref:
#
# Create the cross reference files
# Note, this is not meant to be used unless you've installed cxref...
#
	@for file in $(CXREF_FILES) ; do \
	  echo Cross referencing $$file ; \
	  $(CXREF) -D__CXREF__ -xref -Odoc -Ncircle $$file ; \
	done
#
# Create the source files using cxref
#
	@for file in $(CXREF_FILES) ; do \
	   echo Documenting $$file ; \
	   ( cd . ; $(CXREF) -D__CXREF__ -warn-xref -xref -Odoc -Ncircle -html $$file ) ; \
	   rm -f $(DOCS) ; \
	done
#
# Create the index using cxref
#
	@echo Indexing
	@( cd . ; $(CXREF) -D__CXREF__ -index-all -Odoc -Ncircle -html )
	@rm -f $(DOCS)
#
# Make html files for the .h files
#
	@echo Creating .h.html files...
	@for file in *.h ; do \
	  echo $$file ; \
	  cat htmlh-head $$file htmlh-tail > doc/$$file.html ; \
	done
# Copy over to the html directory
	#cp doc/*.html $(HOME)/www/cxref
	#chmod 644 $(HOME)/www/cxref/*.html

# Dependencies for the object files (automagically generated with
# gcc -MM)

depend:
	$(CC) -MM *.c > depend

-include depend
