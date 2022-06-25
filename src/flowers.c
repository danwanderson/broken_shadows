///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2022 by Daniel Anderson
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
 * If you choose to use this code, please retain my name in this file and  *
 * send me an email (dwa1844@rit.edu) saying you are using it. Suggestions *
 * for improvement are welcome.   -- Rahl (Daniel Anderson)                *
 ***************************************************************************/

 /**************************************************************************
  * This code is based on something I saw on Aardwolf. It looked kinda     *
  * cool so I thought I'd try putting it on Broken Shadows :)              *
  * This code allows you to send flowers to someone online, along with a   *
  * small (or long) message on the card. (Thanks to Mekare for pointing it *
  * out to me :)                                                           *
  **************************************************************************/

 /**************************************************************************
  * To install, add spec_flower_shop (or whatever you want to call it) to  *
  * special.c, #define all the OBJ_VNUM stuff in merc.h, add a mob with    *
  * the spec function to your MUD, create all the various objects in       *
  * the area of your choice (I use limbo.are), add flowers.o to your       *
  * makefile, add do_purchase to interp.c and interp.h, and enjoy.         *
  **************************************************************************/

 /**************************************************************************
  0 PURCHASE~
  Syntax: purchase <person> <item>
          purchase <person> <item> <message>

  You can buy flowers and such for other players for various occasions. To 
  do this, you can use either of the above syntaxes. If the message is 
  omitted, it will place a simple message stating who the flowers are from.
  If you add a message, the card will contain that message.

  Currently, you can buy:
        a red rose          (RED)       200 gold
        a white rose        (WHITE)     200 gold
        a black rose        (BLACK)     200 gold
        a pink rosebud      (PINK)      200 gold
        a dozen red roses   (DOZEN)     500 gold
        a corsage           (CORSAGE)   300 gold

  When buying a selection, use the keywords in parenthesis () as the <item>.
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"
#include "interp.h"

void do_purchase( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim, *shopkeeper;
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];    /* person */
    char arg2[MAX_INPUT_LENGTH];    /* object */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf2 = buffer_new( MAX_INPUT_LENGTH );
    char *oldname;
  
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( ch ) )
    {
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }
    
    for ( shopkeeper = ch->in_room->people; shopkeeper != NULL; shopkeeper = shopkeeper->next_in_room )
    {
        if ( !IS_NPC( shopkeeper ) )
            continue;
        if ( shopkeeper->spec_fun == spec_lookup( "spec_flower_shop" ) )
            break;
    }

    if ( shopkeeper == NULL || shopkeeper->spec_fun != spec_lookup( "spec_flower_shop" ) )
    {
        send_to_char("You can't do that here.\n\r",ch);
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( shopkeeper->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Send what to whom?\n\r", ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( !str_cmp( arg2, "black" ) )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_BLACK_ROSE ), 0 );
        if ( ch->gold < obj->cost )
        {
            send_to_char( "You can't afford that.\n\r", ch );
            buffer_free( buf );
            buffer_free( buf2 );
            extract_obj( obj );
            return;
        }
        ch->gold -= obj->cost;
    }
    else if ( !str_cmp( arg2, "white" ) )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_WHITE_ROSE ), 0 );
        if ( ch->gold < obj->cost )
        {
            send_to_char( "You can't afford that.\n\r", ch );
            buffer_free( buf );
            buffer_free( buf2 );
            extract_obj( obj );
            return;
        }
        ch->gold -= obj->cost;
    }
    else if ( !str_cmp( arg2, "red" ) )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_RED_ROSE ), 0 );
        if ( ch->gold < obj->cost )
        {
            send_to_char( "You can't afford that.\n\r", ch );
            buffer_free( buf );
            buffer_free( buf2 );
            extract_obj( obj );
            return;
        }
        ch->gold -= obj->cost;
    }
    else if ( !str_cmp( arg2, "dozen" ) )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_DOZEN_ROSES ), 0 );
        if ( ch->gold < obj->cost )
        {
            send_to_char( "You can't afford that.\n\r", ch );
            buffer_free( buf );
            buffer_free( buf2 );
            extract_obj( obj );
            return;
        }
        ch->gold -= obj->cost;
    }
    else if ( !str_cmp( arg2, "pink" ) )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_PINK_ROSE ), 0 );
        if ( ch->gold < obj->cost )
        {
            send_to_char( "You can't afford that.\n\r", ch );
            buffer_free( buf );
            buffer_free( buf2 );
            extract_obj( obj );
            return;
        }
        ch->gold -= obj->cost;
    }
    else if ( !str_cmp( arg2, "corsage" ) )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_CORSAGE ), 0 );
        if ( ch->gold < obj->cost )
        {
            send_to_char( "You can't afford that.\n\r", ch );
            buffer_free( buf );
            buffer_free( buf2 );
            extract_obj( obj );
            return;
        }
        ch->gold -= obj->cost;
    }
    else /* oops! not on list */
    {
        send_to_char( "Sorry, but we don't sell that.\n\r", ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }
    
    oldname = str_dup( obj->name );
    free_string( obj->name );
    obj->name = str_dup( "tempobjname" );

    if ( argument[0] == '\0' )
    {
        bprintf( buf, "A gift from %s.", ch->name );
        bprintf( buf2, "obj %s extended card %s", obj->name, buf );
        send_to_char( "Your card says: ", ch );
        send_to_char( buf->data, ch );
        send_to_char( "\n\r", ch );
        do_string( ch, buf2->data );
    }
    else
    {
        bprintf( buf2, "obj %s extended card %s -- %s", obj->name,
            argument, ch->name );
        do_string( ch, buf2->data );
        send_to_char( "Your card says: ", ch );
        bprintf( buf2, "%s -- %s\n\r", argument, ch->name );
        send_to_char( buf2->data, ch );
    }

    free_string( obj->name );
    obj->name = str_dup( oldname );
    free_string( oldname );
    buffer_free( buf );
    buffer_free( buf2 );

    act( "A messenger runs out of the room.", ch, NULL, NULL, TO_ALL );   
    act( "A messenger runs by and pauses to hand you $p.", victim, obj, NULL, TO_CHAR );
    act( "A messenger runs by and pauses to hand $p to $n.", victim, obj, NULL, TO_ROOM );
    obj_to_char( obj, victim );
}
