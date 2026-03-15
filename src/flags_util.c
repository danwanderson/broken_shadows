/*
 * flags_util.c — flag_convert() and print_flags()
 *
 * Extracted from db.c and save.c so these pure utility functions can be
 * unit-tested without linking the full game server.
 *
 * flag_convert: ROM OLC letter → bitmask  (A=1, B=2, … Z=2^25, a=2^26, …)
 * print_flags:  bitmask → letter string   (inverse; '0' for zero)
 */

#include "flags_util.h"

long
flag_convert( char letter )
{
    long bitsum = 0;
    char i;

    if ( 'A' <= letter && letter <= 'Z' )
    {
        bitsum = 1;
        for ( i = letter; i > 'A'; i-- )
            bitsum *= 2;
    }
    else if ( 'a' <= letter && letter <= 'z' )
    {
        bitsum = 67108864; /* 2^26 */
        for ( i = letter; i > 'a'; i-- )
            bitsum *= 2;
    }

    return bitsum;
}

char *
print_flags( int flag )
{
    int count, pos = 0;
    static char buf[52];

    for ( count = 0; count < 32; count++ )
    {
        if ( (flag) & (1 << count) )
        {
            if ( count < 26 )
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + (count - 26);
            pos++;
        }
    }

    if ( pos == 0 )
    {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';
    return buf;
}
