CC      = gcc
PROF    = -DLINUX 
NOCRYPT =
C_FLAGS = -Wall -O2 $(PROF) $(NOCRYPT)
L_FLAGS = -O2 $(PROF) -lcrypt

O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
	alias.o ban.o bit.o board.o bonus_update.o buffer.o clans.o comm.o \
	const.o db.o dice_games.o engrave.o effects.o fight.o flowers.o \
	gocial.o handle_con.o handler.o healer.o interp.o magic.o marry.o \
	mem.o music.o olc.o \
	olc_act.o olc_save.o quest.o rename.o save.o skills.o \
	special.o string.o update.o  

all: shadows

shadows: $(O_FILES)
	rm -f shadows
	$(CC) $(L_FLAGS) -o shadows $(O_FILES) -lm

.c.o: merc.h
	$(CC) -c -ggdb $(C_FLAGS) $<

clean:
	-rm -f *.o
	-rm -f make.out

src:
	-rm -f src.*.tar*
	-ctags *.[ch]
	tar cvzf src.`date +%m%d%Y_%H%M`.tar.gz Makefile *.[ch] tags
