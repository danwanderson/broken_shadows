/***************************************************************************
 *          This snippet was written by Donut for the Khrooon Mud.         *
 *            Original Coded by Yago Diaz <yago@cerberus.uab.es>           *
 *              (C) June 1997                                              *
 *              (C) Last Modification October 1997                         *
 ***************************************************************************/

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1996 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@efn.org)                                  *
*           Gabrielle Taylor                                               *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

#define ROOM_VNUM_OFCOL          601
#define ROOM_VNUM_NEW_THALOS    9506

DECLARE_DO_FUN( do_say  );
DECLARE_DO_FUN( do_look );

CHAR_DATA * find_summoner       args( ( CHAR_DATA *ch ) );

CHAR_DATA *find_summoner ( CHAR_DATA *ch )
{
    CHAR_DATA * summoner;

    for ( summoner = ch->in_room->people; summoner != NULL; summoner = summoner->next_in_room )
    {
        if (!IS_NPC(summoner))
            continue;

        if (summoner->spec_fun == spec_lookup( "spec_summoner" ))
            return summoner;
    }

   if (summoner == NULL || summoner->spec_fun != spec_lookup( "spec_summoner" ))
   {
        send_to_char("You can't do that here.\n\r",ch);
        return NULL;
   }

   if ( summoner->fighting != NULL)
   {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        return NULL;
   }

   return NULL;
}


void do_travel(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *summoner;
    CHAR_DATA *pet;
    char  buf[MAX_STRING_LENGTH];
    char  arg[MAX_STRING_LENGTH];
    char  arg1[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg );
    argument = one_argument(argument, arg1);

    summoner = find_summoner (ch);

    if (!summoner)
        return;

    if (arg[0] == '\0')
    {
        sprintf(buf, "You must tell me what travel you want to do\n\r\tTRAVEL list         Shows the possible locations to travel to.\n\r\tTRAVEL buy <name> To travel to the selected location.");
        act("$N says '$t'.", ch, buf, summoner, TO_CHAR);
        return;
    }

    if (!strcmp( arg, "list"))
    {
        sprintf(buf, "Possible travels are:\n\r\n\r\tOfcol\t\t500 gold coins\n\r\tNew Thalos\t750 gold coins");
        act("$N says '$t'.", ch, buf, summoner, TO_CHAR);
        return;
    }

    if (!strcmp( arg, "buy"))
    {
        if (arg1[0] == '\0')
        {
            sprintf(buf, "You must tell me what travel do you want to do");
            act("$N says '$t'.", ch, buf, summoner, TO_CHAR);
            return;
        }

        if (is_name(arg1, "ofcol"))
        {
            if (ch->gold < 500)
            {
                sprintf(buf, "You don't have enough gold for my services");
                act("$N says '$t'.", ch, buf, summoner, TO_CHAR);
                return;
            }

            pet = ch->pet;              
            if ( ch->pet != NULL && ch->pet->in_room == ch->in_room)
            {
                char_from_room( pet );
                char_to_room( pet, get_room_index(ROOM_VNUM_OFCOL) );
            }

            char_from_room( ch );
            char_to_room( ch, get_room_index(ROOM_VNUM_OFCOL) );
            ch->gold -= 500;
            summoner->gold += 750;
            sprintf(buf, "%s utters the words 'hasidsindsad'\n\rYou are surrounded by a violet fog.\n\r", summoner->short_descr);
            send_to_char(buf, ch);
            do_look (ch, "");
            return;
        }

        else if (is_name(arg1, "new thalos"))
        {
            if (ch->gold < 750)
            {
                sprintf(buf, "You don't have enough gold for my services");
                act("$N says '$t'.", ch, buf, summoner, TO_CHAR);
                return;
            }

            pet = ch->pet;              
            if ( ch->pet != NULL && ch->pet->in_room == ch->in_room)
            {
                char_from_room( pet );
                char_to_room( pet, get_room_index(ROOM_VNUM_NEW_THALOS) );
            }

            char_from_room( ch );
            char_to_room( ch, get_room_index(ROOM_VNUM_NEW_THALOS) );
            ch->gold -= 750;
            summoner->gold += 750;
            sprintf(buf, "%s utters the words 'hasidsindsad'\n\rYou are surrounded by a violet fog.\n\r", summoner->short_descr);
            send_to_char(buf, ch);
            do_look (ch, "");
            return;
        }

        else
        {
            sprintf(buf, "This travel is not on the list");
            act("$N says '$t'.", ch, buf, summoner, TO_CHAR);
            return;
        }
    }
}
