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

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"



/********************************************************************
* This is the code for gocials, or socials over the gossip channel. *
* It is fairly long and complicated and took me a whole hour and a  *
* half to write up. I finally did get it working though, but I am   *
* sure there are neater ways to code it...the code towards the      *
* end where it tries to do the replacing done normally by acts      *
* so that it can get the victim's name/etc in and then send it to   *
* another victim with the act command. Feel free to edit it if you  *
* can do a quick re-write using much neater code. This should work  *
* for all possible socials that normally work in the mud.           *
********************************************************************/

/********************************************************************
* I ask for no credit on the mud for this, nor do i ask for any-    *
* thing in exchange. I believe in the philosophy of giving what you *
* can to the mud community and then taking what others offer. Use   *
* this, copy this, give it to friends...whatever you like.          *
*                                                 -Bangle           *
********************************************************************/



void do_gocial(CHAR_DATA *ch, char *argument)
{
    char command[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int cmd;
    bool found;
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    int counter;
    int count;
    char buf2[MAX_STRING_LENGTH];

    argument = one_argument(argument,command);

    buf[0] = '\0';
    buf2[0] = '\0';

    if (command[0] == '\0')
    {
        send_to_char("What do you wish to gocial?\n\r",ch);
        return;
    }

    found = FALSE;
    for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++)
    {
        if (command[0] == social_table[cmd].name[0]
        && !str_prefix( command,social_table[cmd].name ) )
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        send_to_char("What kind of social is that?!?!\n\r",ch);
        return;
    }

    if (!IS_NPC(ch) && IS_SET(ch->comm,   COMM_QUIET))
    {
        send_to_char("You must turn off quiet mode first.\n\r",ch);
        return;
    }

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOGOSSIP))
    {
        send_to_char("But you have the gossip channel turned off!\n\r",ch);
        return;
    }

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOCHANNELS))
    {
        send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
        return;
    }

    switch (ch->position)
    {
        case POS_DEAD:
            send_to_char("Lie still; you are DEAD!\n\r",ch);
            return;
        case POS_INCAP:
        case POS_MORTAL:
            send_to_char("You are hurt far too bad for that.\n\r",ch);
            return;
        case POS_STUNNED:
            send_to_char("You are too stunned for that.\n\r",ch);
            return;
    }

    one_argument(argument,arg);
    victim = NULL;
    if (arg[0] == '\0')
    {
        sprintf(buf, "`gGocial:`w %s`w", social_table[cmd].char_no_arg );
        act_new(buf,ch,NULL,NULL,TO_CHAR,POS_DEAD);
        buf[0] = '\0';
        sprintf(buf, "`gGocial:`w %s`w", social_table[cmd].others_no_arg );
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *vch;
            vch = d->original ? d->original : d->character;
            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(vch->comm,COMM_NOGOSSIP) &&
                !IS_SET(vch->comm,COMM_QUIET))
            {
                act_new(buf,ch,NULL,vch,TO_VICT,POS_DEAD);
            }
        }
    }
    else if ((victim = get_char_world(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }
    else if (victim == ch)
    {
        buf[0] = '\0';
        sprintf(buf,"`gGocial:`w %s`w", social_table[cmd].char_auto);
        act_new(buf,ch,NULL,NULL,TO_CHAR,POS_DEAD);
        buf[0] = '\0';
        sprintf(buf,"`gGocial:`w %s`w", social_table[cmd].others_auto);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *vch;
            vch = d->original ? d->original : d->character;
            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(vch->comm,COMM_NOGOSSIP) &&
                !IS_SET(vch->comm,COMM_QUIET))
            {
                act_new(buf,ch,NULL,vch,TO_VICT,POS_DEAD);
            }
        }
    }               
    else
    {
        buf[0] = '\0';
        sprintf(buf,"`gGocial:`w %s`w", social_table[cmd].char_found);
        act_new(buf,ch,NULL,victim,TO_CHAR,POS_DEAD);
        buf[0] = '\0';
        sprintf(buf,"`gGocial:`w %s`w", social_table[cmd].vict_found);

        if ( !IS_SET( victim->comm, COMM_QUIET ) &&
             !IS_SET( victim->comm, COMM_NOGOSSIP ) )
            act_new(buf,ch,NULL,victim,TO_VICT,POS_DEAD);

        buf[0] = '\0';
        sprintf(buf,"`gGocial:`w %s`w", social_table[cmd].others_found);
        for (counter = 0; buf[counter+1] != '\0'; counter++)
        {
            if (buf[counter] == '$' && buf[counter + 1] == 'N')
            {
                strcpy(buf2,buf);
                buf2[counter] = '\0';
                strcat(buf2,victim->name);
                for (count = 0; buf[count] != '\0'; count++)
                {
                    buf[count] = buf[count+counter+2];
                }
                strcat(buf2,buf);
                strcpy(buf,buf2);
                continue;
            }
            else if (buf[counter] == '$' && buf[counter + 1] == 'E')
            {
                switch (victim->sex)
                {
                    default:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"it");
                        for (count = 0; buf[count] != '\0'; count ++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                    case 1:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"it");
                        for (count = 0; buf[count] != '\0'; count++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                    case 2:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"it");
                        for (count = 0; buf[count] != '\0'; count++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                }
                continue;
            }    
            else if (buf[counter] == '$' && buf[counter + 1] == 'M')
            {
                buf[counter] = '%';
                buf[counter + 1] = 's';
                switch (victim->sex)
                {
                    default:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"it");
                        for (count = 0; buf[count] != '\0'; count++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                    case 1:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"him");
                        for (count = 0; buf[count] != '\0'; count++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                    case 2:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"her");
                        for (count = 0; buf[count] != '\0'; count++);
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                }
                continue;
            }
            else if (buf[counter] == '$' && buf[counter + 1] == 'S')
            {
                switch (victim->sex)
                {
                    default:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"its");
                        for (count = 0;buf[count] != '\0'; count++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                    case 1:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"his");
                        for (count = 0; buf[count] != '\0'; count++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                    case 2:
                        strcpy(buf2,buf);
                        buf2[counter] = '\0';
                        strcat(buf2,"her");
                        for (count = 0; buf[count] != '\0'; count++)
                        {
                            buf[count] = buf[count+counter+2];
                        }
                        strcat(buf2,buf);
                        strcpy(buf,buf2);
                        break;
                }
                continue;
            }

        }
        for (d=descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *vch;
            vch = d->original ? d->original : d->character;
            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                d->character != victim &&
                !IS_SET(vch->comm, COMM_NOGOSSIP) &&
                !IS_SET(vch->comm,COMM_QUIET))
            {
                act_new(buf,ch,NULL,vch,TO_VICT,POS_DEAD);
            }
        }
    }
    return;
}
