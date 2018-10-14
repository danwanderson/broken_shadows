///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2018 by Daniel Anderson
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
*       ROM 2.4 is copyright 1993-1996 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@efn.org)                                  *
*           Gabrielle Taylor                                               *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/
/***************************************************************************
*       MARRY.C written by Ryouga for Vilaross Mud (baby.indstate.edu 4000)*
*       Please leave this and all other credit include in this package.    *
*       Email questions/comments to ryouga@jessi.indstate.edu              *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"

void do_marry( CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( char_getImmRank( ch ) >= HERO ) { 
        if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
            send_to_char( "Syntax: marry <char1> <char2>\n\r",ch);
            buffer_free( buf );
            return;
        }

        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
            send_to_char( "The first person mentioned isn't playing.\n\r", ch );
            buffer_free( buf );
            return;
        }
        
        if ( ( victim2 = get_char_world( ch, arg2 ) ) == NULL ) {
            send_to_char( "The second person mentioned isn't playing.\n\r", ch);
            buffer_free( buf );
            return;
        }
        
        if ( IS_NPC(victim) || IS_NPC(victim2)) {
         	send_to_char("I don't think they want to be Married to the Mob.\n\r", ch);
         	buffer_free( buf );
         	return;
        }        
        
        if (!IS_SET(victim->act, PLR_CONSENT) || !IS_SET(victim2->act, PLR_CONSENT)) {
         	send_to_char( "They have not given consent.\n\r", ch);
         	buffer_free( buf );
         	return;
        }

        if ( victim == victim2 ) {
            send_to_char( "You can't do that!\n\r", ch );
            buffer_free( buf );
            return;
        }
        
        if ( victim->pcdata->spouse != NULL || 
            victim2->pcdata->spouse != NULL ) {
           send_to_char( "They are already married! \n\r", ch);
           buffer_free( buf );
           return;
        }
       
        bprintf( buf, "You pronounce %s and %s man and wife!\n\r",
            victim->name, victim2->name ); 
        send_to_char( buf->data, ch );
        bprintf( buf, "You say the big 'I do' to %s.\n\r", victim2->name);
        send_to_char( buf->data, victim );
        bprintf( buf, "You say the big 'I do' to %s.\n\r", victim->name);
        send_to_char( buf->data, victim2 );
        REMOVE_BIT( victim->act, PLR_CONSENT );
        REMOVE_BIT( victim2->act, PLR_CONSENT );
        free_string( victim->pcdata->spouse );
        free_string( victim2->pcdata->spouse );
        victim->pcdata->spouse = str_dup( victim2->name );
        victim2->pcdata->spouse = str_dup( victim->name );
        buffer_free( buf );
        return;

    } else {
    	send_to_char( "You do not have the ability to perform the ceremony.\n\r", ch);
    	buffer_free( buf );
    	return;
    }
    buffer_free( buf );
    return;
}

void do_divorce( CHAR_DATA *ch, char *argument)
{

    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH ); 
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( char_getImmRank( ch ) >= HERO ) { 
        /* print syntax if arguments are not specified */
        if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
            send_to_char( "Syntax: divorce <char1> <char2>\n\r",ch);
            buffer_free( buf );
            return;
        }

        /* Player1 has to be online */
        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
            send_to_char( "The first person mentioned isn't playing.\n\r", ch );
            buffer_free( buf );
            return;
        }
        
        /* so does player 2 */
        if ( ( victim2 = get_char_world( ch, arg2 ) ) == NULL ) {
            send_to_char( "The second person mentioned isn't playing.\n\r", ch);
            buffer_free( buf );
            return;
        }
        
        /* Can't marry an NPC */
        if ( IS_NPC(victim) || IS_NPC(victim2)) {
            send_to_char("I don't think they're Married to the Mob...\n\r", ch);
            buffer_free( buf );
            return;
        }
         
        /* Both players must consent first */       
        if (!IS_SET(victim->act, PLR_CONSENT) || !IS_SET(victim2->act, PLR_CONSENT)) {
            send_to_char( "They have not consented.\n\r", ch);
            buffer_free( buf );
            return;
        }

        /* can't marry people to themselves */
        if ( victim == victim2 ) {
            send_to_char( "You can't do that!\n\r", ch );
            buffer_free( buf );
            return;
        }

        /* can't divorce a couple that's not married */
        if (str_cmp( victim->pcdata->spouse, victim2->name ) ) {
            send_to_char( "They aren't even married!!\n\r",ch);
            buffer_free( buf );
            return;
        }
           
        bprintf( buf, "You hand %s and %s their divorce papers.\n\r", 
            victim->name, victim2->name );
        send_to_char( buf->data, ch );
        send_to_char( "Your divorce is final.\n\r", victim);
        send_to_char( "Your divorce is final.\n\r", victim2);
        REMOVE_BIT( victim->act, PLR_CONSENT );
        REMOVE_BIT( victim2->act, PLR_CONSENT );
        free_string( victim->pcdata->spouse );
        free_string( victim2->pcdata->spouse );
        victim->pcdata->spouse = NULL;
        victim2->pcdata->spouse = NULL;
        buffer_free( buf );
        return;

    } else {
        send_to_char( "Hire an attorney.\n\r", ch);
        buffer_free( buf );
        return;
   }
   buffer_free( buf );
   return;
}

void do_consent( CHAR_DATA *ch )
{
    /* NPCs can't get married */
    if (IS_NPC(ch)) {
        return;
    }
    
    /* If you've already consented, revoke it */
    if ( IS_SET(ch->act, PLR_CONSENT) ) {
        send_to_char( "You no longer give consent.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_CONSENT);
        return;
    }
                           
    send_to_char( "You now give consent to married!\n\r", ch);
    SET_BIT(ch->act, PLR_CONSENT);
    return;
}


void do_spousetalk( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    DESCRIPTOR_DATA *d;
    int found = FALSE;
 
    if ( ch->pcdata->spouse == NULL ) {
        send_to_char( "You aren't married.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if (argument[0] == '\0' ) {
        send_to_char("What do you wish to tell your other half?\n\r", ch);
        buffer_free( buf );
        return;
    } else { /* message sent */
        bprintf( buf, "`MYou say to %s, '%s`M'\n\r`w", ch->pcdata->spouse, argument );
        send_to_char( buf->data, ch );
        for ( d = descriptor_list; d != NULL; d = d->next ) {
            CHAR_DATA *victim;
  
            victim = d->original ? d->original : d->character;
 
            if ( d->connected == CON_PLAYING &&
                /*  d->character != ch && */
                 !str_cmp( d->character->name, ch->pcdata->spouse) ) {
                act_new( "`M$n says to you, '$t`M'`w", 
                       ch, argument, d->character, TO_VICT, POS_SLEEPING );
               found = TRUE;
            }
        }

        if ( !found ) {
            bprintf( buf, "%s isn't here.\n\r", ch->pcdata->spouse );
            send_to_char( buf->data, ch );
        }
    }
    buffer_free( buf );
    return;
}

