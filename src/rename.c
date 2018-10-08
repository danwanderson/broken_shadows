///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvements copyright (C) 1992, 1993 by Michael         *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefiting.  We hope that you share your changes too.  What goes       *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <form.h>
#include "merc.h"


/*
===========================================================================
This snippet was written by Erwin S. Andreasen, erwin@pip.dknet.dk. You may
use this code freely, as long as you retain my name in all of the files. You
also have to mail me telling that you are using it. I am giving this,
hopefully useful, piece of source code to you for free, and all I require
from you is some feedback.
Please mail me if you find any bugs or have any new ideas or just comments.

===========================================================================
*/

/**************************************************************************
 * Added by Rahl to Broken Shadows. Modified by Rahl
 *************************************************************************/

/*
 * do_renam by Rahl
 */
void do_renam( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to RENAME someone, you have to type it out!\n\r", ch );
    return;
}

/*
 * do_rename renames a player to another name.
 * PCs only. Previous file is deleted, if it exists.
 * Char is then saved to new file.
 * New name is checked against std. checks, existing offline players and
 * online players.
 * .gz files are checked for too, just in case.
 */

bool check_parse_name (char* name);  /* comm.c */
char *initial( const char *str );    /* comm.c */

void do_rename (CHAR_DATA* ch, char* argument)
{
    char old_name[MAX_INPUT_LENGTH],
    new_name[MAX_INPUT_LENGTH],
    strsave [MAX_INPUT_LENGTH];

    CHAR_DATA* victim;
    FILE* file;

    argument = one_argument(argument, old_name); /* find new/old name */
    one_argument (argument, new_name);

    /* Trivial checks */
    if (!old_name[0])
    {
        send_to_char ("Rename who?\n\r",ch);
        return;
    }

    victim = get_char_world (ch, old_name);

    if (!victim)
    {
        send_to_char ("There is no such a person online.\n\r",ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char ("You cannot use rename on NPCs.\n\r",ch);
        return;
    }

    /* allow rename self new_name,but otherwise only lower level */
    if ( (victim != ch) && (char_getImmRank(victim) >= char_getImmRank (ch)) )
    {
        send_to_char ("You failed.\n\r",ch);
        return;
    }

    if (!victim->desc || (victim->desc->connected != CON_PLAYING) )
    {
        send_to_char ("This player has lost his link or something.\n\r",ch);
        return;
    }

    if (!new_name[0])
    {
        send_to_char ("Rename to what new name?\n\r",ch);
        return;
    }

    /* Insert check for clan here!! */


    if (victim->pcdata->clan)
    {
        send_to_char ("This player is member of a clan, remove them from the clan first.\n\r",ch);
        return;
    }


    if (!check_parse_name(new_name))
    {
        send_to_char ("The new name is illegal.\n\r",ch);
        return;
    }

    /* First, check if there is a player named that off-line */
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( new_name ) );

    fclose (fpReserve); /* close the reserve file */
    file = fopen (strsave, "r"); /* attempt to to open pfile */
    if (file)
    {
        send_to_char ("A player with that name already exists!\n\r",ch);
        fclose (file);
        fpReserve = fopen( NULL_FILE, "r" ); /* is this really necessary these days? */
        return;
    }
    fpReserve = fopen( NULL_FILE, "r" );  /* reopen the extra file */

    /* Check .gz file ! */
    sprintf( strsave, "%s%s.gz", PLAYER_DIR, capitalize( new_name ) );
    fclose (fpReserve); /* close the reserve file */
    file = fopen (strsave, "r"); /* attempt to to open pfile */
    if (file)
    {
        send_to_char ("A player with that name already exists in a compressed file!\n\r",ch);
        fclose (file);
        fpReserve = fopen( NULL_FILE, "r" );
        return;
    }
    fpReserve = fopen( NULL_FILE, "r" );  /* reopen the extra file */

    if (get_char_world(ch,new_name)) /* check for playing level-1 non-saved */
    {
        send_to_char ("A player with the name you specified already exists!\n\r",ch);
        return;
    }

    /* Save the filename of the old name */
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( victim->name ) );

    /* Rename the character and save him to a new file */
    /* NOTE: Players who are level 1 do NOT get saved under a new name */

    free_string (victim->name);
    victim->name = str_dup (capitalize(new_name));

    save_char_obj (victim);

    /* unlink the old file */
    unlink (strsave); /* unlink does return a value.. but we do not care */

    /* That's it! */

    send_to_char ("Character renamed.\n\r",ch);

    victim->position = POS_STANDING; /* I am laaazy */
    act ("$n has renamed you to $N!",ch,NULL,victim,TO_VICT);

} /* do_rename */


