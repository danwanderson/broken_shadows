# Makefile for Broken Shadows unit tests
#
# Produces five independent test binaries:
#   test_buffer     — tests for buffer.c  (find_mem_size, buffer_new/strcat/…)
#   test_bit        — tests for bit.c     (is_stat, flag_lookup, flag_string, flag_value)
#   test_handler    — tests for handler.c (get_date, get_time, lookups, check_immune)
#   test_flags      — tests for flags_util.c (flag_convert, print_flags, round-trip)
#   test_yaml_clans — tests for yaml_area.c clan functions (load/save/round-trip)
#
# Design notes:
#   • Each binary links only the module(s) being tested plus stubs.c.
#   • -ffunction-sections + -Wl,--gc-sections prunes dead code so the linker
#     requires stubs only for the symbols actually reachable from each test's
#     main(), not for every function in the compiled module.
#   • Compiled objects use a _t suffix to avoid clobbering parent build artefacts
#     if someone runs `make test` from the parent src/ directory.
#   • Include path -I.. resolves merc.h, buffer.h, recycle.h, interp.h etc.
#
# Usage:
#   make          — build all test binaries
#   make run      — build and run all tests; exits non-zero on any failure
#   make clean    — remove build artefacts

CC      = clang
PROF    = -DLINUX
CFLAGS  = -Wall -O2 $(PROF) -I.. -ffunction-sections -fPIE
LDFLAGS = -O2 $(PROF) -Wl,--gc-sections -lm -pie

# ------------------------------------------------------------------ #
#  Object file rules                                                   #
# ------------------------------------------------------------------ #

# Shared stubs (always needed)
stubs.o: stubs.c
	$(CC) $(CFLAGS) -c stubs.c -o stubs.o

# Parent module objects compiled with test flags (suffix _t to avoid conflicts)
buffer_t.o: ../buffer.c ../buffer.h ../merc.h
	$(CC) $(CFLAGS) -c ../buffer.c -o buffer_t.o

bit_t.o: ../bit.c ../merc.h
	$(CC) $(CFLAGS) -c ../bit.c -o bit_t.o

handler_t.o: ../handler.c ../merc.h ../recycle.h ../interp.h
	$(CC) $(CFLAGS) -c ../handler.c -o handler_t.o

flags_util_t.o: ../flags_util.c ../flags_util.h
	$(CC) $(CFLAGS) -c ../flags_util.c -o flags_util_t.o

yaml_area_t.o: ../yaml_area.c ../yaml_area.h ../merc.h ../db.h
	$(CC) $(CFLAGS) -c ../yaml_area.c -o yaml_area_t.o

# Test driver objects
test_buffer.o: test_buffer.c test_framework.h ../merc.h ../buffer.h
	$(CC) $(CFLAGS) -c test_buffer.c -o test_buffer.o

test_bit.o: test_bit.c test_framework.h ../merc.h
	$(CC) $(CFLAGS) -c test_bit.c -o test_bit.o

test_handler.o: test_handler.c test_framework.h ../merc.h
	$(CC) $(CFLAGS) -c test_handler.c -o test_handler.o

test_flags.o: test_flags.c test_framework.h ../merc.h ../flags_util.h
	$(CC) $(CFLAGS) -c test_flags.c -o test_flags.o

test_yaml_clans.o: test_yaml_clans.c test_framework.h ../merc.h ../yaml_area.h
	$(CC) $(CFLAGS) -c test_yaml_clans.c -o test_yaml_clans.o

# ------------------------------------------------------------------ #
#  Test binary link rules                                              #
# ------------------------------------------------------------------ #

test_buffer: test_buffer.o buffer_t.o stubs.o
	$(CC) $(LDFLAGS) -o test_buffer test_buffer.o buffer_t.o stubs.o

test_bit: test_bit.o bit_t.o stubs.o
	$(CC) $(LDFLAGS) -o test_bit test_bit.o bit_t.o stubs.o

# test_handler links bit_t.o to supply material_type (defined in bit.c,
# not const.c) which is referenced by handler.c's material_lookup.
test_handler: test_handler.o handler_t.o bit_t.o stubs.o
	$(CC) $(LDFLAGS) -o test_handler test_handler.o handler_t.o bit_t.o stubs.o

# test_flags: flags_util has zero external deps; stubs.o only needed to
# satisfy merc.h globals brought in transitively.
test_flags: test_flags.o flags_util_t.o stubs.o
	$(CC) $(LDFLAGS) -o test_flags test_flags.o flags_util_t.o stubs.o

# test_yaml_clans: yaml_area_t.o compiled with -ffunction-sections so
# --gc-sections prunes all area/mob/room code; only clan-path symbols
# (alloc_perm, str_dup, flag_convert, print_flags, bug, clan globals)
# are required.  -lyaml supplies libyaml.
test_yaml_clans: test_yaml_clans.o yaml_area_t.o flags_util_t.o stubs.o
	$(CC) $(LDFLAGS) -lyaml -o test_yaml_clans \
		test_yaml_clans.o yaml_area_t.o flags_util_t.o stubs.o

# ------------------------------------------------------------------ #
#  Convenience targets                                                 #
# ------------------------------------------------------------------ #

.PHONY: all run clean

all: test_buffer test_bit test_handler test_flags test_yaml_clans

run: all
	@echo ""
	@echo "=============================="
	@echo " Running: test_buffer"
	@echo "=============================="
	./test_buffer
	@echo ""
	@echo "=============================="
	@echo " Running: test_bit"
	@echo "=============================="
	./test_bit
	@echo ""
	@echo "=============================="
	@echo " Running: test_handler"
	@echo "=============================="
	./test_handler
	@echo ""
	@echo "=============================="
	@echo " Running: test_flags"
	@echo "=============================="
	./test_flags
	@echo ""
	@echo "=============================="
	@echo " Running: test_yaml_clans"
	@echo "=============================="
	./test_yaml_clans
	@echo ""
	@echo "All test suites passed."

clean:
	-rm -f *.o test_buffer test_bit test_handler test_flags test_yaml_clans
