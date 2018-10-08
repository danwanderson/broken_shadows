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
 * If you choose to use this code, please retain my name in this file and  *
 * send me an email (dwa1844@rit.edu) saying you are using it. Suggestions *
 * for improvement are welcome.   -- Rahl (Daniel Anderson)                *
 ***************************************************************************/

 /**************************************************************************
  * To install, add spec_engraver (or whatever you want to call it) to     *
  * special.c, add a mob with the spec function to your MUD, add           * 
  * engraver.o to your makefile, add do_engrave to interp.c and interp.h,  *
  * and enjoy.                                                             *
  **************************************************************************/

 /**************************************************************************
  0 ENGRAVE~
  Syntax: engrave <item> <location> <message>
          
  For a small fee, a specialized engraver will customize any of your 
  equipment. 

  Valid locations are "short", "long", "extended", and "name"

  For the "extended" location, the first word is the keyword (ie, for
  "examine <keyword>")

  If an immortal or another player has a problem with your engraving, you 
  may be asked to change it.

 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"
#include "interp.h"

void do_engrave( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *shopkeeper;
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];    /* object */
    char arg2[MAX_INPUT_LENGTH];    /* location */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf2 = buffer_new( MAX_INPUT_LENGTH );
    char *oldname;
    int fee = 10000;
  
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
        if ( shopkeeper->spec_fun == spec_lookup( "spec_engraver" ) )
            break;
    }

    if ( shopkeeper == NULL || shopkeeper->spec_fun != spec_lookup( "spec_engraver" ) )
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
        send_to_char( "Engrave what where?\n\r", ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Engrave it to what?\n\r", ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( ch->gold < fee )
    {
        bprintf( buf, "I'm sorry, %s, but you don't have enough money for"
            " my services.", ch->name );
        do_say( shopkeeper, buf->data );
        bprintf( buf, "My fee is %d gold.", fee );
        do_say( shopkeeper, buf->data );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
    {
        do_say( shopkeeper, "You aren't carrying that." );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    oldname = str_dup( obj->name );
    free_string( obj->name );
    obj->name = str_dup( "tempobj2name" );

    if ( !str_cmp( arg2, "extended" ) )
    {
        bprintf( buf, "obj %s extended %s", obj->name, argument );
        do_string( ch, buf->data );
        send_to_char( argument, ch );
        send_to_char( "\n\r", ch );
        do_say( shopkeeper, "The first word is the keyword. The rest is"
            " the descrption" );
        free_string( obj->name );
        obj->name = str_dup( oldname );
        ch->gold -= fee;
    }

    else if ( !str_cmp( arg2, "short" ) )
    {
        if ( strlen( argument ) > 42 )
        {
            send_to_char( "This is the SHORT description, remember?\n\r", ch );
            buffer_free( buf );
            buffer_free( buf2 );
            return;
        } 
        bprintf( buf, "obj %s short %s", obj->name, argument );
        do_string( ch, buf->data );
        bprintf( buf2, "The short description of %s has been changed to %s", 
            oldname, argument );
        do_say( shopkeeper, buf2->data );
        free_string( obj->name );
        obj->name = str_dup( oldname );
        ch->gold -= fee;
    }

    else if ( !str_cmp( arg2, "long" ) )
    {
        bprintf( buf, "obj %s long %s", obj->name, argument );
        do_string( ch, buf->data );
        bprintf( buf2, "The long description of %s has been changed to %s", 
            oldname, argument );
        do_say( shopkeeper, buf2->data );
        free_string( obj->name );
        obj->name = str_dup( oldname );
        ch->gold -= fee;
    }

    else if ( !str_cmp( arg2, "name" ) )
    {
        bprintf( buf, "obj %s name %s", obj->name, argument );
        do_string( ch, buf->data );
        bprintf( buf2, "The name of %s has been changed to %s", 
            oldname, argument );
        do_say( shopkeeper, buf2->data );
        ch->gold -= fee;
    }
 
    else
    {
        do_say( shopkeeper, "I'm afraid I don't understand what you're asking me to do." );
        free_string( obj->name );
        obj->name = str_dup( oldname );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }
    
    free_string( oldname );
    buffer_free( buf );
    buffer_free( buf2 );
    return;
}
