#CC      = g++-4
CC      = g++

#Used to get a profile of the code.
#Uncomment and run the code long enough to get a good profile and do a normal ingame shutdown.
#Go to the "LOP/" directory.
#Then do a "prof lop > prof.out"
#PROF    = -p
#Used to get an advanced profile of the code.
#Do same as above one only use "gprof lop > gprof.out"
#Then just look at "prof.out/gprof.out" in an editor to see the profile.
#PROF    = -pg

#Uncomment to compile in Cygwin
CYGWIN = -DCYGWIN

#Uncomment the line below if you are getting undefined references to dlsym, dlopen, and dlclose.
#Comment it out if you get errors about ldl not being found.
NEED_DL = -ldl

#Some systems need this for dynamic linking to work.
EXPORT_SYMBOLS = -export-dynamic

# Uncomment the two lines below if compiling on a Solaris box
#SOLARIS_FLAG = -Dsun -DSYSV
#SOLARIS_LINK = -lnsl -lsocket

#IMC2 - Comment out to disable IMC2 support
IMC = 1

# Comment out if it complains about it
WSTRINGS = -Wwrite-strings

W_FLAGS = -Wall -Werror -Wshadow -Wformat-security -Wpointer-arith -Wcast-align -Wredundant-decls $(WSTRINGS)

C_FLAGS = -g2 -O $(W_FLAGS) $(SOLARIS_FLAG) $(PROF) $(EXPORT_SYMBOLS)
L_FLAGS = $(PROF) $(SOLARIS_LINK) -lz $(NEED_DL)

C_FILES = act_comm.c act_info.c act_move.c act_obj.c act_wiz.c affects.c areas.c auth.c \
          ban.c bank.c boards.c bti.c build.c \
          channels.c clans.c classes.c color.c comm.c commands.c const.c \
          db.c deity.c \
          exp.c \
          fight.c fish.c friends.c \
          grub.c \
          handler.c hashstr.c helps.c highscore.c hint.c host.c hostlist.c hotboot.c \
          interp.c \
          lockers.c \
          magic.c makeobjs.c mapout.c mccp.c misc.c mpxset.c mssp.c mud_comm.c mud_prog.c mwresets.c \
          news.c \
          player.c polymorph.c \
          quest.c \
          races.c reserved.c reset.c rubiks.c \
          save.c sha256.c shops.c skills.c socials.c special.c storages.c sudoku.c sysdata.c \
          tables.c timeinfo.c track.c transfer.c trivia.c \
          update.c \
          wilderness.c

ifdef IMC
   C_FILES := imc.c $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DIMC -DIMCSMAUG
endif

O_FILES := $(patsubst %.c,o/%.o,$(C_FILES))

H_FILES = $(wildcard h/*.h) 

all:
	$(MAKE) -s lop

SPEC = specials/
ifdef CYGWIN
LOP = LOP.exe
LDEF = $(SPEC)LOP.def
LEXP = $(SPEC)LOP.exp
else
LOP = LOP
endif
DEP = $(SPEC)dependencies.d

# pull in dependency info for *existing* .o files
-include $(DEP)

dependency:
	echo "Generating dependency file...";
	$(CC) -MM $(C_FLAGS) $(C_FILES) > $(DEP)
	perl -pi -e 's.^([a-z]).o/$$1.g' $(DEP)

lop: $(O_FILES)
	rm -f ../$(LOP)
ifdef CYGWIN
	dlltool --export-all --output-def $(LDEF) $(O_FILES)
	dlltool --dllname $(LOP) --output-exp $(LEXP) --def $(LDEF)
	$(CC) -o $(LOP) $(O_FILES) $(LEXP) $(L_FLAGS)
else
	$(CC) -export-dynamic -o $(LOP) $(O_FILES) $(L_FLAGS)
endif
	$(MAKE) -s dependency
	echo "Done compiling mud.";
	chmod g+w $(LOP)
	chmod a+x $(LOP)
	mv $(LOP) ../$(LOP)
	chmod g+w $(O_FILES)

purge:
	rm -f o/*.o ../$(LOP) *~ $(DEP)
	touch $(DEP)

clean:
	$(MAKE) -s purge
	$(MAKE) -s lop

indent:
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(C_FILES)
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(H_FILES)

indentclean:
	rm *.c~ *h/.h~

o/%.o: %.c
	echo "  Compiling $@....";
	$(CC) -c $(C_FLAGS) $< -o $@

.c.o: h/mud.h
	$(CC) -c $(C_FLAGS) $<
