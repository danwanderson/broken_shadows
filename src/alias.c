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
 ***************************************************************************/
 
/***************************************************************************
*       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@pacinfo.com)                              *
*           Gabrielle Taylor (gtaylor@pacinfo.com)                         *
*           Brian Moore (rom@rom.efn.org)                                  *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"

/* does aliasing and other fun stuff */
void substitute_alias(DESCRIPTOR_DATA *d, char *argument)
{
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH],name[MAX_INPUT_LENGTH];
    char *point;
    int alias;
    char prefix[MAX_INPUT_LENGTH];

    ch = d->original ? d->original : d->character;

    /* check for prefix */
    if (ch->prefix[0] != '\0' && str_prefix("prefix",argument))
    {
        if (strlen(ch->prefix) + strlen(argument) > MAX_INPUT_LENGTH)
            send_to_char("Line to long, prefix not processed.\r\n",ch);
        else
        {
            sprintf(prefix,"%s %s",ch->prefix,argument);
            argument = prefix;
        }
    }

    if (IS_NPC(ch) || ch->pcdata->alias[0] == NULL
    ||  !str_prefix("alias",argument) || !str_prefix("una",argument) ) 
    {
        interpret(d->character,argument);
        return;
    }

    strcpy(buf,argument);

    for (alias = 0; alias < MAX_ALIAS; alias++)  /* go through the aliases */
    {
        if (ch->pcdata->alias[alias] == NULL)
            break;

        if (!str_prefix(ch->pcdata->alias[alias],argument))
        {
            point = one_argument(argument,name);
            if (!strcmp(ch->pcdata->alias[alias],name))
            {
                buf[0] = '\0';
                strcat(buf,ch->pcdata->alias_sub[alias]);
                strcat(buf," ");
                strcat(buf,point);

                if (strlen(buf) > MAX_INPUT_LENGTH)
                {
                    send_to_char("Alias substitution too long."
                        " Truncated.\r\n",ch);
                    buf[MAX_INPUT_LENGTH -1] = '\0';
                }
                break;
            }
        }
    }
    interpret(d->character,buf);
}

void do_alia(CHAR_DATA *ch, char *argument)
{
    send_to_char("I'm sorry, alias must be entered in full.\n\r",ch);
    return;
}

void do_alias(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH ); 
    int pos;

    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
    {
        buffer_free( buf );
        return;
    }

    argument = one_argument(argument,arg);
    smash_tilde(argument);

    if (arg[0] == '\0')
    {

        if (rch->pcdata->alias[0] == NULL)
        {
            send_to_char("You have no aliases defined.\n\r",ch);
            buffer_free( buf );
            return;
        }
        send_to_char("Your current aliases are:\n\r",ch);

        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (rch->pcdata->alias[pos] == NULL
            ||  rch->pcdata->alias_sub[pos] == NULL)
                break;

            bprintf(buf,"    %s:  %s\n\r",rch->pcdata->alias[pos],
                    rch->pcdata->alias_sub[pos]);
            send_to_char(buf->data,ch);
        }
        buffer_free( buf );
        return;
    }

    if (!str_prefix("una",arg) || !str_cmp("alias",arg))
    {
        send_to_char("Sorry, that word is reserved.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if (argument[0] == '\0')
    {
        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (rch->pcdata->alias[pos] == NULL
            ||  rch->pcdata->alias_sub[pos] == NULL)
                break;

            if (!str_cmp(arg,rch->pcdata->alias[pos]))
            {
                bprintf(buf,"%s aliases to '%s'.\n\r",
                    rch->pcdata->alias[pos],
                    rch->pcdata->alias_sub[pos]);
                send_to_char(buf->data,ch);
                buffer_free( buf );
                return;
            }
        }

        send_to_char("That alias is not defined.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if (!str_prefix(argument,"delete") )
    {
        send_to_char("That shall not be done!\n\r",ch);
        buffer_free( buf );
        return;
    }

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
        if (rch->pcdata->alias[pos] == NULL)
            break;

        if (!str_cmp(arg,rch->pcdata->alias[pos])) /* redefine an alias */
        {
            free_string(rch->pcdata->alias_sub[pos]);
            rch->pcdata->alias_sub[pos] = str_dup(argument);
            bprintf(buf,"%s is now realiased to '%s'.\n\r",
                arg, argument);
            send_to_char(buf->data,ch);
            buffer_free( buf );
            return;
        }
     }

     if (pos >= MAX_ALIAS)
     {
        send_to_char("Sorry, you have reached the alias limit.\n\r",ch);
        buffer_free( buf );
        return;
     }
  
     /* make a new alias */
     rch->pcdata->alias[pos]            = str_dup(arg);
     rch->pcdata->alias_sub[pos]        = str_dup(argument);
     bprintf(buf,"%s is now aliased to '%s'.\n\r",arg,argument);
     send_to_char(buf->data,ch);
     buffer_free( buf );
     return;
}


void do_unalias(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH];
    int pos;
    bool found = FALSE;
 
    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;
 
    if (IS_NPC(rch))
        return;
 
    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        send_to_char("Unalias what?\n\r",ch);
        return;
    }

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
        if (rch->pcdata->alias[pos] == NULL)
            break;

        if (found)
        {
            rch->pcdata->alias[pos-1]           = rch->pcdata->alias[pos];
            rch->pcdata->alias_sub[pos-1]       = rch->pcdata->alias_sub[pos];
            rch->pcdata->alias[pos]             = NULL;
            rch->pcdata->alias_sub[pos]         = NULL;
            continue;
        }

        if(!strcmp(arg,rch->pcdata->alias[pos]))
        {
            send_to_char("Alias removed.\n\r",ch);
            free_string(rch->pcdata->alias[pos]);
            free_string(rch->pcdata->alias_sub[pos]);
            rch->pcdata->alias[pos] = NULL;
            rch->pcdata->alias_sub[pos] = NULL;
            found = TRUE;
        }
    }

    if (!found)
        send_to_char("No alias of that name to remove.\n\r",ch);
}

