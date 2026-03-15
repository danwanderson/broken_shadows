#ifndef FLAGS_UTIL_H
#define FLAGS_UTIL_H

/*
 * flag_convert / print_flags — isolated here so they can be unit-tested
 * without pulling in all of db.c or save.c.
 *
 * Both functions are also declared in merc.h; this header exists solely
 * so that flags_util.c (and the test binary) can compile without merc.h.
 */

long  flag_convert ( char letter );
char *print_flags  ( int  flag   );

#endif /* FLAGS_UTIL_H */
