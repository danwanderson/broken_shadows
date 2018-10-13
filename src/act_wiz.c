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

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"
#include "olc.h"
#include "interp.h"

/* global constants */
extern AFFECT_DATA *affect_free;

/*
 * Local functions.
 */
ROOM_INDEX_DATA *       find_location   ( CHAR_DATA *ch, char *arg );

/* equips a character */
void do_outfit ( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    int i, sn, vnum;

    if (ch->level > 5 || IS_NPC(ch))
    {
        send_to_char("Find it yourself!\n\r",ch);
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_LIGHT );
    }
 
    if ( ( obj = get_eq_char( ch, WEAR_BODY ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_BODY );
    }

    if ( ( obj = get_eq_char( ch, WEAR_SHIELD ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_SHIELD );
    }

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
    { 
     /*
        obj = create_object( get_obj_index(class_table[ch->ch_class].weapon),0);
      */

        /* junk for weapon choice - Rahl */
        sn = 0;
        vnum = OBJ_VNUM_SCHOOL_SWORD;
        
        for ( i = 0; weapon_table[i].name != NULL; i++ )
        {
            if (ch->pcdata->learned[sn] <
                ch->pcdata->learned[*weapon_table[i].gsn])
            {
                sn = *weapon_table[i].gsn;
                vnum = weapon_table[i].vnum;
            }
        }

        obj = create_object(get_obj_index( vnum ), 0 );
        /* end junk for weapon choice - Rahl */
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_WIELD );
    }

    send_to_char("You have been equipped by the gods.\n\r",ch);
}

     
/* RT nochannels command, for those spammers */
void do_nochannels( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH ); 

    one_argument( argument, arg );
 
    if ( arg[0] == '\0' )
    {
        send_to_char( "Nochannel whom?", ch );
        buffer_free( buf );
        return;
    }
 
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }
 
    if ( char_getImmRank( victim ) >= char_getImmRank( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        buffer_free( buf );
        return;
    }
 
    if ( IS_SET(victim->comm, COMM_NOCHANNELS) )
    {
        REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "The gods have restored your channel priviliges.\n\r", 
                      victim );
        send_to_char( "NOCHANNELS removed.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N restores channels to %s", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 ); 
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "The gods have revoked your channel priviliges.\n\r", 
                       victim );
        send_to_char( "NOCHANNELS set.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N removes %s's channels", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
    }
 
    buffer_free( buf );
    return;
}

void do_poofin( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( !IS_NPC(ch) )
    {
        smash_tilde( argument );

        if (argument[0] == '\0')
        {
            bprintf(buf,"Your poofin is %s\n\r`w",ch->pcdata->bamfin);
            send_to_char(buf->data,ch);
            buffer_free( buf );
            return;
        }

        if ( strstr(argument,ch->name) == NULL)
        {
            send_to_char("You must include your name.\n\r",ch);
            buffer_free( buf );
            return;
        }
             
        free_string( ch->pcdata->bamfin );
        ch->pcdata->bamfin = str_dup( argument );
        strcat(ch->pcdata->bamfin, "`w");

        bprintf(buf,"Your poofin is now %s\n\r`w",ch->pcdata->bamfin);
        send_to_char(buf->data,ch);
    }
    buffer_free( buf );
    return;
}



void do_poofout( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
 
    if ( !IS_NPC(ch) )
    {
        smash_tilde( argument );
 
        if (argument[0] == '\0')
        {
            bprintf(buf,"Your poofout is %s\n\r`w",ch->pcdata->bamfout);
            send_to_char(buf->data,ch);
            buffer_free( buf );
            return;
        }
 
        if ( strstr(argument,ch->name) == NULL)
        {
            send_to_char("You must include your name.\n\r",ch);
            buffer_free( buf );
            return;
        }
 
        free_string( ch->pcdata->bamfout );
        ch->pcdata->bamfout = str_dup( argument );
        strcat(ch->pcdata->bamfout, "`w");
 
        bprintf(buf,"Your poofout is now %s\n\r`w",ch->pcdata->bamfout);
        send_to_char(buf->data,ch);
    }
    buffer_free( buf );
    return;
}



void do_deny( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Deny whom?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( char_getImmRank( victim ) >= char_getImmRank( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        buffer_free( buf );
        return;
    }

    SET_BIT(victim->act, PLR_DENY);
    send_to_char( "You are denied access!\n\r", victim );
    /* added by Rahl */
    bprintf( buf, "$N denies access to %s", victim->name );
    wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );

    send_to_char( "OK.\n\r", ch );
    save_char_obj(victim);
    do_quit( victim, "" );

    buffer_free( buf );
    return;
}



void do_disconnect( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Disconnect whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( victim->desc == NULL )
    {
        act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
        return;
    }

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d == victim->desc )
        {
            close_socket( d );
            send_to_char( "Ok.\n\r", ch );
            return;
        }
    }

    bug( "Do_disconnect: desc not found.", 0 );
   send_to_char( "Descriptor not found!\n\r", ch );
   return;
}

void do_new_discon( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   DESCRIPTOR_DATA *d;
   int sock_num;
   
   one_argument( argument, arg );
   if ( arg[0] == '\0' )
     {
        send_to_char( "Disconnect whom?\n\r", ch );
        return;
     }
   
   if ( !is_number(arg) )
     {
        send_to_char("Argument must be numeric, use sockets to find number.\n\r", ch);
        return;
     }
   sock_num = atoi(arg);
   
   for ( d = descriptor_list; d != NULL; d = d->next )
     {
        if ( d->descriptor == sock_num )
          {
             close_socket( d );
             send_to_char( "Ok.\n\r", ch );
             return;
          }
     }
   
   send_to_char( "Descriptor not on list, use sockets.\n\r", ch );
   return;
}


void do_pardon( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "killer" ) || !str_cmp( arg2, "pk" ) )
    {
        if ( IS_SET(victim->act, PLR_KILLER) )
        {
            REMOVE_BIT( victim->act, PLR_KILLER );
            send_to_char( "`RPlayer Killer`w flag removed.\n\r", ch );
            send_to_char( "You are no longer a `RPlayer Killer`w.\n\r",
                victim );
            bprintf( buf, "$N removes %s's Player Killer flag.",
                victim->name );
            wiznet( buf->data, ch, NULL, WIZ_FLAGS, 0, 0 );
        }
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "thief" ) )
    {
        if ( IS_SET(victim->act, PLR_THIEF) )
        {
            REMOVE_BIT( victim->act, PLR_THIEF );
            send_to_char( "Thief flag removed.\n\r", ch );
            send_to_char( "You are no longer a `KTHIEF`w.\n\r", victim );
            bprintf( buf, "$N removes %s's `KTHIEF`w flag.",
                victim->name);
            wiznet( buf->data, ch, NULL, WIZ_FLAGS, 0, 0 );
        }
        buffer_free( buf );
        return;
    }

    send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
    buffer_free( buf );
    return;
}



void do_echo( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Global echo what?\n\r", ch );
        return;
    }
    
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->connected == CON_PLAYING )
        {
            if (char_getImmRank(d->character) >= char_getImmRank(ch))
                send_to_char( "global> ",d->character);
            send_to_char( "`w", d->character );
            send_to_char( argument, d->character );
            send_to_char( "\n\r`w",   d->character );
        }
    }

    return;
}



void do_recho( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Local echo what?\n\r", ch );

        return;
    }

    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->connected == CON_PLAYING
        &&   d->character->in_room == ch->in_room )
        {
            if (char_getImmRank(d->character) >= char_getImmRank(ch))
                send_to_char( "local> ",d->character);
            send_to_char( argument, d->character );
            send_to_char( "\n\r`w",   d->character );
        }
    }

    return;
}

void do_pecho( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);
 
    if ( argument[0] == '\0' || arg[0] == '\0' )
    {
        send_to_char("Personal echo what?\n\r", ch); 
        return;
    }
   
    if  ( (victim = get_char_world(ch, arg) ) == NULL )
    {
        send_to_char("Target not found.\n\r",ch);
        return;
    }

    if (char_getImmRank(victim) >= char_getImmRank(ch) 
	&& char_getImmRank(ch) != MAX_RANK )
        send_to_char( "personal> ",victim);

    send_to_char(argument,victim);
    send_to_char("\n\r`w",victim);
    send_to_char( "personal> ",ch);
    send_to_char(argument,ch);
    send_to_char("\n\r`w",ch);
}


ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if ( is_number(arg) )
        return get_room_index( atoi( arg ) );

    if ( ( victim = get_char_world( ch, arg ) ) != NULL )
        return victim->in_room;

    if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
        return obj->in_room;

    return NULL;
}



void do_transfer( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Transfer whom (and where)?\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->connected == CON_PLAYING
            &&   d->character != ch
            &&   d->character->in_room != NULL
            &&   can_see( ch, d->character ) 
            /* added by Rahl so you can't transfer higher level chars
               when you "transfer all" */
            &&   d->character->level <= ch->level )
            {
                BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
                bprintf( buf, "%s %s", d->character->name, arg2 );
                do_transfer( ch, buf->data );
                buffer_free( buf );
            }
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
        location = ch->in_room;
    }
    else
    {
        if ( ( location = find_location( ch, arg2 ) ) == NULL )
        {
            send_to_char( "No such location.\n\r", ch );
            return;
        }

        if ( !is_room_owner(ch,location) && room_is_private( location ) 
        && char_getImmRank(ch) < MAX_RANK )
        {
            send_to_char( "That room is private right now.\n\r", ch );
            return;
        }
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( victim->in_room == NULL )
    {
        send_to_char( "They are in limbo.\n\r", ch );
        return;
    }

    /* added by Rahl so you can't transfer higher level chars */
    if ( !IS_NPC( victim ) && ( victim->level > ch->level  ) )
    {
        send_to_char( "Maybe you'd better ask them to come to you...\n\r", ch );
        return;
    }

    if ( victim->fighting != NULL )
        stop_fighting( victim, TRUE );
    act( "$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, location );
    act( "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
    if ( ch != victim )
        act( "$n has transferred you.", ch, NULL, victim, TO_VICT );
    do_look( victim, "auto" );
    send_to_char( "Ok.\n\r", ch );
}



void do_at( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    CHAR_DATA *wch;
    
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "At where what?\n\r", ch );
        return;
    }

    if ( ( location = find_location( ch, arg ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }

    if ( !is_room_owner(ch,location) && room_is_private( location ) 
    && char_getImmRank(ch) < MAX_RANK )
    {
        send_to_char( "That room is private right now.\n\r", ch );
        return;
    }

    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
        if ( wch == ch )
        {
            char_from_room( ch );
            char_to_room( ch, original );
            break;
        }
    }

    return;
}

void do_repop( CHAR_DATA *ch, char *argument )
{
    AREA_DATA *pArea;
    int vnum;
    BUFFER *buf = buffer_new( 200 );
    bool found_area=FALSE;
    
    if ( argument[0] == 0 ) {
        reset_area(ch->in_room->area);
        send_to_char("Area repop!\n\r", ch);
    }
    if ( is_number(argument) ) {
        vnum=atoi(argument);
        for (pArea = area_first; pArea; pArea = pArea->next )
        {
            if (pArea->vnum == vnum) {
                /* was name[8] --Rahl */
                bprintf(buf, "%s has been repoped!\n\r", &pArea->name[16]);
                send_to_char(buf->data, ch);
                found_area=TRUE;
                reset_area(pArea);
            }
        }
        if (!found_area) send_to_char("No such area!\n\r",ch);
    }
    if ( !strcmp(argument, "all") ||!strcmp( argument, "world" ) ) {
        for (pArea = area_first; pArea; pArea = pArea->next )
        {
            reset_area(pArea);
        }
        send_to_char("World repop!\n\r", ch);
    }
    buffer_free( buf );
    return;
}


void do_goto( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    int count = 0;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Goto where?\n\r", ch );
        return;
    }

    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }

    count = 0;
    for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
        count++;

    if ( !is_room_owner(ch,location) && room_is_private( location ) 
    && (count > 1 || char_getImmRank(ch) < MAX_RANK ) )
    {
        send_to_char( "That room is private right now.\n\r", ch );
        return;
    }

    if ( ch->fighting != NULL )
        stop_fighting( ch, TRUE );

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (char_getImmRank(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
            else
                act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    char_from_room( ch );
    char_to_room( ch, location );


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (char_getImmRank(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    do_look( ch, "auto" );
    return;
}

/* RT to replace the 3 stat commands */

void do_stat ( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char *string;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim;

   string = one_argument(argument, arg);
   if ( arg[0] == '\0')
   {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  stat <name>\n\r",ch);
        send_to_char("  stat obj <name>\n\r",ch);
        send_to_char("  stat mob <name>\n\r",ch);
        send_to_char("  stat room <number>\n\r",ch);
        return;
   }

   if (!str_cmp(arg,"room"))
   {
        do_rstat(ch,string);
        return;
   }
  
   if (!str_cmp(arg,"obj"))
   {
        do_ostat(ch,string);
        return;
   }

   if(!str_cmp(arg,"char")  || !str_cmp(arg,"mob"))
   {
        do_mstat(ch,string);
        return;
   }
   
   /* do it the old way */

   obj = get_obj_world(ch,argument);
   if (obj != NULL)
   {
     do_ostat(ch,argument);
     return;
   }

  victim = get_char_world(ch,argument);
  if (victim != NULL)
  {
    do_mstat(ch,argument);
    return;
  }

  location = find_location(ch,argument);
  if (location != NULL)
  {
    do_rstat(ch,argument);
    return;
  }

  send_to_char("Nothing by that name found anywhere.\n\r",ch);
}

   

void do_rstat( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *output = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door;

    one_argument( argument, arg );
    location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
    if ( location == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    if ( !is_room_owner(ch,location) && ch->in_room != location 
    && room_is_private( location ) && char_getImmRank(ch) < MAX_RANK )
    {
        send_to_char( "That room is private right now.\n\r", ch );
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    bprintf( buf, "Name: '%s.'\n\rArea: '%s'.\n\r",
        location->name,
        location->area->name );
  /*  send_to_char( buf, ch ); */
    buffer_strcat( output, buf->data );
/*
    bprintf( buf,
        "Vnum: %d.  Sector: %d.  Light: %d.\n\r",
        location->vnum,
        location->sector_type,
        location->light );
*/
  /*  send_to_char( buf, ch ); */
    bprintf( buf,
        "Vnum: %d.  Sector: %s.  Light: %d.",
        location->vnum,
        flag_string( sector_flags, location->sector_type ),
        location->light );
    buffer_strcat( output, buf->data );

    bprintf( buf, "  Timer: %d.\n\r", location->timer );
  /*  send_to_char( buf, ch ); */
    buffer_strcat( output, buf->data );

/*
    bprintf( buf,
        "Room flags: %d.\n\rDescription:\n\r%s",
        location->room_flags,
        location->description );
*/
  /*  send_to_char( buf, ch ); */
    bprintf( buf,
        "Room flags: %s.\n\rDescription:\n\r%s",
        flag_string( room_flags, location->room_flags ),
        location->description );
    buffer_strcat( output, buf->data );

    if ( location->extra_descr != NULL )
    {
        EXTRA_DESCR_DATA *ed;

        buffer_strcat( output, "Extra description keywords: '" );
        for ( ed = location->extra_descr; ed; ed = ed->next )
        {
            buffer_strcat( output, ed->keyword );
            if ( ed->next != NULL )
            {
                buffer_strcat( output, " " );
            }
        }
        buffer_strcat( output, ".\n\r" );
    }

    buffer_strcat( output, "Characters:" );
    for ( rch = location->people; rch; rch = rch->next_in_room )
    {
        if (can_see(ch,rch))
        {
          /*  send_to_char( " ", ch ); */
            buffer_strcat( output, " " );
            one_argument( rch->name, buf->data );
            buffer_strcat( output, buf->data );
          /*  send_to_char( buf, ch ); */
        }
    }

    buffer_strcat( output, ".\n\rObjects:   "  );
  /*  send_to_char( ".\n\rObjects:   ", ch ); */
    for ( obj = location->contents; obj; obj = obj->next_content )
    {
        buffer_strcat( output, " " );
        one_argument( obj->name, buf->data );
        buffer_strcat( output, buf->data );
    }
    buffer_strcat( output, ".\n\r" );
  /*  send_to_char( ".\n\r", ch ); */

    for ( door = 0; door <= 5; door++ )
    {
        EXIT_DATA *pexit;

        if ( ( pexit = location->exit[door] ) != NULL )
        {
        /*
            bprintf( buf,
                "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\n\rKeyword: '%s'.  Description: %s",
        */

            bprintf( buf,
                "Door: %d.  To: %d.  Key: %d.  Exit flags: %s.\n\r"
                "   Keyword: '%s'.  Description: %s",

                door,
                (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
                pexit->key,
            /*  pexit->exit_info, */
                flag_string( exit_flags, pexit->exit_info ),
                pexit->keyword,
                pexit->description[0] != '\0'
                    ? pexit->description : "(none).\n\r" );
            buffer_strcat( output, buf->data );
          /*  send_to_char( buf, ch ); */
        }
    }

    page_to_char( output->data, ch );

    buffer_free( buf );
    buffer_free( output );
    return;
}



void do_ostat( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *output = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Stat what?\n\r", ch );
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    if ( ( obj = get_obj_world( ch, argument ) ) == NULL )
    {
        send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    bprintf( buf, "Name(s): %s\n\r",
        obj->name );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Vnum: %d  Type: %s  Resets: %d\n\r",
        obj->pIndexData->vnum, 
        item_type_name(obj), obj->pIndexData->reset_num );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Short description: %s\n\rLong description: %s\n\r",
        obj->short_descr, obj->description );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Wear bits: %s\n\rExtra bits: %s\n\r",
        wear_bit_name(obj->wear_flags), extra_bit_name( obj->extra_flags ) );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Number: %d/%d  Weight: %d/%d\n\r",
        1,           get_obj_number( obj ),
        obj->weight, get_obj_weight( obj ) );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Level: %d  Cost: %d  Timer: %d\n\r",
        obj->level, obj->cost, obj->timer );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Material: %s\n\r", material_name( obj->material ) );
    buffer_strcat( output, buf->data );

    bprintf( buf,
        "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
        obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
        obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
        obj->carried_by == NULL    ? "(none)" : 
            can_see(ch,obj->carried_by) ? obj->carried_by->name
                                        : "someone",
        obj->wear_loc );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Values: %d %d %d %d %d\n\r",
        obj->value[0], obj->value[1], obj->value[2], obj->value[3],
        obj->value[4] );
    buffer_strcat( output, buf->data );
    
    /* now give out vital statistics as per identify */
    
    switch ( obj->item_type )
    {
        case ITEM_SCROLL: 
        case ITEM_POTION:
        case ITEM_PILL:
            bprintf( buf, "Level %d spells of:", obj->value[0] );
            buffer_strcat( output, buf->data );

            if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
            {
               /* 
                * send_to_char( " '", ch ); 
                * send_to_char( skill_table[obj->value[1]].name, ch ); 
                * send_to_char( "'", ch ); 
                */
                buffer_strcat( output, " '" );
                buffer_strcat( output, skill_table[obj->value[1]].name );
                buffer_strcat( output, "'" );
            }

            if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
            {
               /*
                * send_to_char( " '", ch );
                * send_to_char( skill_table[obj->value[2]].name, ch );
                * send_to_char( "'", ch );
                */
                buffer_strcat( output, " '" );
                buffer_strcat( output, skill_table[obj->value[2]].name );
                buffer_strcat( output, "'" );
            }

            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
               /*
                * send_to_char( " '", ch );
                * send_to_char( skill_table[obj->value[3]].name, ch );
                * send_to_char( "'", ch );
                */
                buffer_strcat( output, " '" );
                buffer_strcat( output, skill_table[obj->value[3]].name );
                buffer_strcat( output, "'" );
            }

            buffer_strcat( output, ".\n\r" );
            break;

        case ITEM_WAND: 
        case ITEM_STAFF: 
            bprintf( buf, "Has %d(%d) charges of level %d",
                obj->value[1], obj->value[2], obj->value[0] );
            buffer_strcat( output, buf->data );
      
            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
               /*
                * send_to_char( " '", ch );
                * send_to_char( skill_table[obj->value[3]].name, ch );
                * send_to_char( "'", ch );
                */
                buffer_strcat( output, " '" );
                buffer_strcat( output, skill_table[obj->value[3]].name );
                buffer_strcat( output, "'" );
            }

            buffer_strcat( output, ".\n\r" );
        break;
      
        case ITEM_WEAPON:
          /*  send_to_char("Weapon type is ",ch); */
            buffer_strcat( output, "Weapon type is " );
            switch (obj->value[0])
            {
                case(WEAPON_EXOTIC) : 
                    buffer_strcat( output, "exotic\n\r" );
                    break;
                case(WEAPON_SWORD)  : 
                    buffer_strcat( output, "sword\n\r" );
                    break;      
                case(WEAPON_DAGGER) : 
                    buffer_strcat( output, "dagger\n\r" );
                    break;
                case(WEAPON_SPEAR): 
                    buffer_strcat( output, "spear/staff\n\r" );
                    break;
                case(WEAPON_MACE) : 
                    buffer_strcat( output, "mace/club\n\r" );
                    break;
                case(WEAPON_AXE):
                    buffer_strcat( output, "axe\n\r" );
                    break;
                case(WEAPON_FLAIL): 
                    buffer_strcat( output, "flail\n\r" );
                    break;
                case(WEAPON_WHIP): 
                    buffer_strcat( output, "whip\n\r" );
                    break;
                case(WEAPON_POLEARM): 
                    buffer_strcat( output, "polearm\n\r" );
                    break;
                default : 
                    buffer_strcat( output, "unknown\n\r" );
                    break;
            }
                bprintf(buf,"Damage is %dd%d (average %d)\n\r",
                    obj->value[1],obj->value[2],
                    (1 + obj->value[2]) * obj->value[1] / 2);
            buffer_strcat( output, buf->data );
            
            if (obj->value[4])  /* weapon flags */
            {
                bprintf(buf,"Weapons flags: %s\n\r",
                    weapon_bit_name(obj->value[4]));
                buffer_strcat( output, buf->data );
            }
        break;

        case ITEM_ARMOR:
            bprintf( buf, 
                "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r", 
                obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
            buffer_strcat( output, buf->data );
        break;
    }


    if ( obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL )
    {
        EXTRA_DESCR_DATA *ed;

        buffer_strcat( output, "Extra description keywords: '" );

        for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
        {
            buffer_strcat( output, ed->keyword );
            if ( ed->next != NULL )
            {
                buffer_strcat( output, " " );
            }
        }

        for ( ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next )
        {
            buffer_strcat( output, ed->keyword );
            if ( ed->next != NULL )
            {
                buffer_strcat( output, " " );
            }
        }

        buffer_strcat( output, "'\n\r" );
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        bprintf( buf, "Affects %s by %d, level %d",
            affect_loc_name( paf->location ), paf->modifier,paf->level );
        buffer_strcat( output, buf->data );
        /* added by Rahl */
        if ( paf->duration > -1 )
            bprintf( buf, ", %d hours.\n\r", paf->duration );
        else
            bprintf( buf, ".\n\r" );
        buffer_strcat( output, buf->data );
        if ( paf->bitvector )
        {
            switch ( paf->where )
            {
                case TO_AFFECTS:
                    bprintf( buf, "Adds %s affect.\n", 
                        affect_bit_name( paf->bitvector ) );
                    break;
                case TO_AFFECTS2:
                    bprintf( buf, "Adds %s affect.\n",
                        affect2_bit_name( paf->bitvector ) );
                    break;
                case TO_WEAPON:
                    bprintf( buf, "Adds %s weapon flags.\n",
                        weapon_bit_name( paf->bitvector ) );
                    break;
                case TO_OBJECT:
                    bprintf( buf, "Adds %s object flag.\n",
                        extra_bit_name( paf->bitvector ) );
                    break;
                case TO_IMMUNE:
                    bprintf( buf, "Adds immunity to %s.\n",
                        imm_bit_name( paf->bitvector ) );
                    break;
                case TO_RESIST:
                    bprintf( buf, "Adds resistance to %s.\n",
                        imm_bit_name( paf->bitvector ) );
                    break;
                case TO_VULN:
                    bprintf( buf, "Adds vulnerability to %s.\n\r",
                        imm_bit_name( paf->bitvector ) );
                    break;
                default:
                    bprintf( buf, "Unknown bit %d %d\n\r",
                        paf->where, paf->bitvector );
                    break;
                }
                buffer_strcat( output, buf->data );
        }  
    }

    if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
        bprintf( buf, "Affects %s by %d, level %d.\n\r",
            affect_loc_name( paf->location ), paf->modifier,paf->level );
        buffer_strcat( output, buf->data );
        if ( paf->bitvector )
        {
            switch ( paf->where )
            {
                case TO_AFFECTS:
                    bprintf( buf, "Adds %s affect.\n", 
                        affect_bit_name( paf->bitvector ) );
                    break;
                case TO_AFFECTS2:
                    bprintf( buf, "Adds %s affect.\n",
                        affect2_bit_name( paf->bitvector ) );
                    break;
                case TO_WEAPON:
                    bprintf( buf, "Adds %s weapon flags.\n",
                        weapon_bit_name( paf->bitvector ) );
                    break;
                case TO_OBJECT:
                    bprintf( buf, "Adds %s object flag.\n",
                        extra_bit_name( paf->bitvector ) );
                    break;
                case TO_IMMUNE:
                    bprintf( buf, "Adds immunity to %s.\n",
                        imm_bit_name( paf->bitvector ) );
                    break;
                case TO_RESIST:
                    bprintf( buf, "Adds resistance to %s.\n",
                        imm_bit_name( paf->bitvector ) );
                    break;
                case TO_VULN:
                    bprintf( buf, "Adds vulnerability to %s.\n\r",
                        imm_bit_name( paf->bitvector ) );
                    break;
                default:
                    bprintf( buf, "Unknown bit %d %d\n\r",
                        paf->where, paf->bitvector );
                    break;
                }
                buffer_strcat( output, buf->data );
        }  

    }

    page_to_char( output->data, ch );

    buffer_free( buf );
    buffer_free( output );
    return;
}

void do_mstat( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *output = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Stat whom?\n\r", ch );
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    bprintf( buf, "Name: %s.\n\r",
        victim->name );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Vnum: %d  Race: %s  Sex: %s  Room: %d\n\r",
        IS_NPC(victim) ? victim->pIndexData->vnum : 0,
        race_table[victim->race].name,
        victim->sex == SEX_MALE    ? "male"   :
        victim->sex == SEX_FEMALE  ? "female" : "neutral",
        victim->in_room == NULL    ?        0 : victim->in_room->vnum
        );
    buffer_strcat( output, buf->data );

    if (IS_NPC(victim))
    {
        bprintf(buf,"Count: %d  Killed: %d\n\r",
            victim->pIndexData->count,victim->pIndexData->killed);
        buffer_strcat( output, buf->data );
    }

    bprintf( buf, 
        "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
        victim->perm_stat[STAT_STR],
        get_curr_stat(victim,STAT_STR),
        victim->perm_stat[STAT_INT],
        get_curr_stat(victim,STAT_INT),
        victim->perm_stat[STAT_WIS],
        get_curr_stat(victim,STAT_WIS),
        victim->perm_stat[STAT_DEX],
        get_curr_stat(victim,STAT_DEX),
        victim->perm_stat[STAT_CON],
        get_curr_stat(victim,STAT_CON) );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Practices: %d\n\r",
        victim->hit,         victim->max_hit,
        victim->mana,        victim->max_mana,
        victim->move,        victim->max_move,
        IS_NPC(ch) ? 0 : victim->practice );
   /*  send_to_char( buf, ch ); */
    buffer_strcat( output, buf->data );

/* new stuff added by Rahl */
	if ( !IS_NPC( victim ) ) {
    	bprintf( buf, "Trains: %d  Quest Points: %d  Quest Timer: %d"
			"  Bonus Points: %d\n\r", victim->train, 
        	victim->questpoints, victim->nextquest, victim->bonusPoints );
    	buffer_strcat( output, buf->data );
	}
        
    bprintf( buf,
        "Lv: %d  Class: %s  Align: %d  Gold: %ld  Exp: %ld\n\r",
        victim->level,       
        IS_NPC(victim) ? "mobile" : class_table[victim->ch_class].name,            
        victim->alignment,
        victim->gold,         victim->exp );
    buffer_strcat( output, buf->data );

    bprintf( buf, "Bank: %ld\n\r", victim->bank );
    buffer_strcat( output, buf->data );

    if ( !IS_NPC( victim ) )
    {
        bprintf( buf, "Killed: %d  Pkills: %d  Pkilled: %d\n\r",
            victim->pcdata->killed, 
            victim->pcdata->pkills, victim->pcdata->pkilled );
        buffer_strcat( output, buf->data );
    }

    bprintf(buf,"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
            GET_AC(victim,AC_PIERCE), GET_AC(victim,AC_BASH),
            GET_AC(victim,AC_SLASH),  GET_AC(victim,AC_EXOTIC));
    buffer_strcat( output, buf->data );

    bprintf( buf, "Hit: %d  Dam: %d  Saves: %d  Position: %d  Wimpy: %d\n\r",
        GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw,
        victim->position,    victim->wimpy );
    buffer_strcat( output, buf->data );

    if (IS_NPC(victim) )
    {
        bprintf(buf, "Damage: %dd%d  Message:  %s\n\r",
            victim->damage[DICE_NUMBER],victim->damage[DICE_TYPE],
            attack_table[victim->dam_type].name);
        buffer_strcat( output, buf->data );
    }

    bprintf( buf, "Fighting: %s\n\r",
        victim->fighting ? victim->fighting->name : "(none)" );
    buffer_strcat( output, buf->data );

    if ( !IS_NPC(victim) )
    {
        bprintf( buf,
            "Thirst: %d  Full: %d  Drunk: %d\n\r",
            victim->pcdata->condition[COND_THIRST],
            victim->pcdata->condition[COND_FULL],
            victim->pcdata->condition[COND_DRUNK] );
        buffer_strcat( output, buf->data );
    }

    bprintf( buf, "Carry number: %d  Carry weight: %d\n\r",
        victim->carry_number, victim->carry_weight );
    buffer_strcat( output, buf->data );


    if (!IS_NPC(victim))
    {
        bprintf( buf, 
            "Age: %d  Played: %d  Timer: %d\n\r",
            get_age(victim), 
            (int) (victim->played + current_time - victim->logon) / 3600, 
            victim->timer);
        buffer_strcat( output, buf->data );
    }

    if (!IS_NPC(victim))
    {
        bprintf( buf, "Recall: %d\n\r",
            victim->pcdata->recall_room->vnum);
        buffer_strcat( output, buf->data );
    }
            
    bprintf(buf, "Clan: %s`w\n\r", IS_NPC(victim) ? 
        vis_clan(victim->pIndexData->clan) : vis_clan(victim->pcdata->clan));
    buffer_strcat( output, buf->data );

    if ( !IS_NPC( victim ) )
    {
        bprintf( buf, "Email: %s\n\r`w", victim->pcdata->email ); 
        buffer_strcat( output, buf->data );

        bprintf( buf, "Comment: %s`w\n\r", victim->pcdata->comment );
        buffer_strcat( output, buf->data );
    }

    if ( IS_NPC( victim ) )
    {
        bprintf( buf, "Material: %s\n\r", 
            material_name( victim->material ) );
        buffer_strcat( output, buf->data );
    }

    bprintf(buf, "Act: %s\n\r",act_bit_name(victim->act));
    buffer_strcat( output, buf->data );
    
    if (victim->comm)
    {
        bprintf(buf,"Comm: %s\n\r",comm_bit_name(victim->comm));
        buffer_strcat( output, buf->data );
    }

    if (IS_NPC(victim) && victim->off_flags)
    {
        bprintf(buf, "Offense: %s\n\r",off_bit_name(victim->off_flags));
        buffer_strcat( output, buf->data );
    }

    if (victim->imm_flags)
    {
        bprintf(buf, "Immune: %s\n\r",imm_bit_name(victim->imm_flags));
        buffer_strcat( output, buf->data );
    }
 
    if (victim->res_flags)
    {
        bprintf(buf, "Resist: %s\n\r", imm_bit_name(victim->res_flags));
        buffer_strcat( output, buf->data );
    }

    if (victim->vuln_flags)
    {
        bprintf(buf, "Vulnerable: %s\n\r",
            imm_bit_name(victim->vuln_flags));
        buffer_strcat( output, buf->data );
    }

    bprintf(buf, "Form: %s\n\rParts: %s\n\r", 
        form_bit_name(victim->form), part_bit_name(victim->parts));
    buffer_strcat( output, buf->data );

    if (victim->affected_by)
    {
        bprintf(buf, "Affected by %s\n\r", 
            affect_bit_name(victim->affected_by));
        buffer_strcat( output, buf->data );
    }

    if ( victim->affected2_by )
    {
        bprintf( buf, "Also affected by %s\n\r",
            affect2_bit_name( victim->affected2_by ) );
        buffer_strcat( output, buf->data );
    }

    bprintf( buf, "Master: %s  Leader: %s  Pet: %s\n\r",
        victim->master      ? victim->master->name   : "(none)",
        victim->leader      ? victim->leader->name   : "(none)",
        victim->pet         ? victim->pet->name      : "(none)");
    buffer_strcat( output, buf->data );

   if ( !IS_NPC(victim))
   {
        /* OLC */
        /* this WAS ch->pcdata->security. dunno. why. -Rahl */
        bprintf( buf, "Security: %d.\n\r", victim->pcdata->security );
        buffer_strcat( output, buf->data );
   }
   
    bprintf( buf, "Short description: %s\n\rLong  description: %s",
        victim->short_descr,
        victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n\r" );
    buffer_strcat( output, buf->data );

    if ( IS_NPC(victim) && victim->spec_fun != 0 )
    {
        buffer_strcat( output, "Mobile has special procedure.\n\r" );
    }


    for ( paf = victim->affected; paf != NULL; paf = paf->next )
    {
        bprintf( buf,
            "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n\r",
            skill_table[(int) paf->type].name,
            affect_loc_name( paf->location ),
            paf->modifier,
            paf->duration,
            ( paf->where == TO_AFFECTS ) ? affect_bit_name( paf->bitvector )
                : affect2_bit_name( paf->bitvector ),
            paf->level
            );
        buffer_strcat( output, buf->data );
    }

    page_to_char( output->data, ch );

    buffer_free( buf );
    buffer_free( output );

    return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument(argument,arg);
 
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  vnum obj <name>\n\r",ch);
        send_to_char("  vnum mob <name>\n\r",ch);
        send_to_char("  vnum skill <skill or spell>\n\r",ch);
        return;
    }

    if (!str_cmp(arg,"obj"))
    {
        do_ofind(ch,string);
        return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    { 
        do_mfind(ch,string);
        return;
    }

    if (!str_cmp(arg,"skill") || !str_cmp(arg,"spell"))
    {
        do_slookup(ch,string);
        return;
    }
    /* do both */
    do_mfind(ch,argument);
    do_ofind(ch,argument);
}


void do_mfind( CHAR_DATA *ch, char *argument )
{
    extern int top_mob_index;
    BUFFER *buffer = buffer_new(MAX_INPUT_LENGTH);
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Find whom?\n\r", ch );
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    fAll        = FALSE; /* !str_cmp( arg, "all" ); */
    found       = FALSE;
    nMatch      = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_mob_index; vnum++ )
    {
        if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
        {
            nMatch++;
            if ( fAll || is_name( argument, pMobIndex->player_name ) )
            {
                found = TRUE;
                /* %3d added by Rahl to show levels */
                bprintf( buf, "`W[%5d]`w `C(%3d)`w %s\n\r",
                    pMobIndex->vnum, pMobIndex->level, pMobIndex->short_descr );
                buffer_strcat( buffer, buf->data );
            }
        }
    }
    page_to_char( buffer->data, ch );

    buffer_free( buffer );
    buffer_free( buf );

    if ( !found )
        send_to_char( "No mobiles by that name.\n\r", ch );

    return;
}



void do_ofind( CHAR_DATA *ch, char *argument )
{
    extern int top_obj_index;
    BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Find what?\n\r", ch );
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    fAll        = FALSE; /* !str_cmp( arg, "all" ); */
    found       = FALSE;
    nMatch      = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            nMatch++;
            if ( fAll || is_name( argument, pObjIndex->name ) )
            {
                found = TRUE;
                /* %3d added by Rahl to show levels */
                bprintf( buf, "`W[%5d]`w `C(%3d)`w %s\n\r",
                    pObjIndex->vnum, pObjIndex->level, pObjIndex->short_descr );
                buffer_strcat( buffer, buf->data );
            }
        }
    }
    page_to_char( buffer->data, ch );

    buffer_free( buffer );
    buffer_free( buf );

    if ( !found )
        send_to_char( "No objects by that name.\n\r", ch );

    return;
}



void do_mwhere( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH ); 
    BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *victim;
    bool found;
    int count = 0;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Use mwhere <name> to find a specific target.\n\r", ch );
        buffer_free( buf );
        buffer_free( buffer );
        do_where( ch, "" );
        return;
    }

    found = FALSE;
    for ( victim = char_list; victim != NULL; victim = victim->next )
    {
        if ( IS_NPC(victim)
        &&   victim->in_room != NULL
        &&   is_name( argument, victim->name ) )
        {
            found = TRUE;
            count++;
            bprintf( buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
                victim->pIndexData->vnum,
                victim->short_descr,
                victim->in_room->vnum,
                victim->in_room->name );
            buffer_strcat( buffer, buf->data );
        }
    }

    if ( !found )
        act( "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
    else
        page_to_char(buffer->data,ch);

    buffer_free( buffer );
    buffer_free( buf );
    
    return;
}



void do_reboo( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to REBOOT, spell it out.\n\r", ch );
    return;
}


void do_reboot( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;

    if (chaos)
    {
     send_to_char("Please remove `rC`RH`YA`RO`rS`w before rebooting.\n\r", ch);
     buffer_free( buf );
     return;
    }

    if ( IS_NPC( ch ) )
    {
        send_to_char( "Try again when you aren't switched.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if (!IS_SET(ch->act,PLR_WIZINVIS) && !IS_SET(ch->act,PLR_INCOGNITO))
    {
        bprintf( buf, "Reboot by %s.", ch->name );
        do_echo( ch, buf->data );
    }

    do_force ( ch, "all save");
    do_save (ch, "");
    do_asave (ch, "changed");
    merc_down = TRUE;
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
        d_next = d->next;
        close_socket(d);
    }

    buffer_free( buf );    
    return;
}


void do_shutdow( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SHUTDOWN, spell it out.\n\r", ch );
    return;
}



void do_shutdown( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;

    if (chaos)
    {
     send_to_char("Please remove `rC`RH`YA`RO`rS`w before shutting down.\n\r", ch);
     buffer_free( buf );
     return;
    }

    if ( IS_NPC( ch ) )
    {
        send_to_char( "Try again when you aren't switched.\n\r", ch );
        buffer_free( buf );
        return;
    }

/*
 * I got sick of the fact that if you were wizi and went to shutdown, it
 * just rebooted so I wrote the "else" below, but I decided it was too
 * cheesy. Instead, I'm just gonna have it echo the character's name.
 * Who cares if they know you're on when you're shutting it down?
 * -Rahl
 */
/*
    if (!IS_SET(ch->act,PLR_WIZINVIS) && !IS_SET( ch->act, PLR_INCOGNITO))
    {
        bprintf( buf, "Shutdown by %s.", ch->name );
    }
    else
    {
        bprintf( buf, "Shutdown by someone." );
    }
*/
    bprintf( buf, "Shutdown by %s.", ch->name );
    /* 
     * I haven't ever actually seen a shutdown.txt file, but, what
     * the hell. I'll leave this line in here. - Rahl
     */

    append_file( ch, SHUTDOWN_FILE, buf->data );
    buffer_strcat( buf, "\n\r" );
    do_echo( ch, buf->data );
    do_force ( ch, "all save");
    do_save (ch, "");
    do_asave (ch, "changed");
    merc_down = TRUE;
    for ( d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        close_socket(d);
    }

    buffer_free( buf );
    return;
}



void do_snoop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Snoop whom?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim->desc == NULL )
    {
        send_to_char( "No descriptor to snoop.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "Cancelling all snoops.\n\r", ch );
        /* added by Rahl */
        wiznet( "$N stops being such a snoop.", ch, NULL, WIZ_SNOOPS,
                WIZ_SECURE, char_getImmRank( ch ) );
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->snoop_by == ch->desc )
                d->snoop_by = NULL;
        }
        buffer_free( buf );
        return;
    }

    if ( victim->desc->snoop_by != NULL )
    {
        send_to_char( "Busy already.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( char_getImmRank( victim ) >= char_getImmRank( ch ) 
    ||   IS_SET( victim->comm,COMM_SNOOP_PROOF ) )
    {
        send_to_char( "You failed.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->desc != NULL )
    {
        for ( d = ch->desc->snoop_by; d != NULL; d = d->snoop_by )
        {
            if ( d->character == victim || d->original == victim )
            {
                send_to_char( "No snoop loops.\n\r", ch );
                buffer_free( buf );
                return;
            }
        }
    }

    victim->desc->snoop_by = ch->desc;
    /* added by Rahl */
    bprintf( buf, "$N snoops on %s", ( IS_NPC( ch ) ? victim->short_descr
                : victim->name ) );
    wiznet( buf->data, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, char_getImmRank( ch ) );

    send_to_char( "Ok.\n\r", ch );

    buffer_free( buf );
    return;
}



void do_switch( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Switch into whom?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->desc == NULL )
    {
        buffer_free( buf );
        return;
    }
    
    if ( ch->desc->original != NULL )
    {
        send_to_char( "You are already switched.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "Ok.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if (!IS_NPC(victim))
    {
        send_to_char("You can only switch into mobiles.\n\r",ch);
        buffer_free( buf );
        return;
    }


    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( victim->desc != NULL )
    {
        send_to_char( "Character in use.\n\r", ch );
        buffer_free( buf );
        return;
    }

    /* added by Rahl */
    bprintf( buf, "$N switches into %s", victim->short_descr );
    wiznet( buf->data, ch, NULL, WIZ_SWITCHES, WIZ_SECURE, char_getImmRank( ch ) );

    ch->desc->character = victim;
    ch->desc->original  = ch;
    victim->desc        = ch->desc;
    ch->desc            = NULL;
    /* change communications to match */
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    char_setImmRank( victim, char_getImmRank( ch ) );
    send_to_char( "Ok.\n\r", victim );
    buffer_free( buf );
    return;
}



void do_return( CHAR_DATA *ch, char *argument )
{
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( ch->desc == NULL )
    {
        buffer_free( buf );
        return;
    }

    if ( ch->desc->original == NULL )
    {
        send_to_char( "You aren't switched.\n\r", ch );
        buffer_free( buf );
        return;
    }

    send_to_char( "You return to your original body.\n\r", ch );

    if ( ch->desc->original->pcdata->message_ctr )
    {
        bprintf( buf, "You have %d messages waiting.\n\r",
            ch->desc->original->pcdata->message_ctr );
        send_to_char( buf->data, ch );
    }

    /* added by Rahl */
    bprintf( buf, "$N returns from %s", ch->short_descr );
    wiznet( buf->data, ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE, 
        char_getImmRank( ch->desc->original ) );

	char_setImmRank( ch, 0 );

    ch->desc->character       = ch->desc->original;
    ch->desc->original        = NULL;
    ch->desc->character->desc = ch->desc; 
    ch->desc                  = NULL;
  
    buffer_free( buf );

    return;
}

/* trust levels for load and clone */
bool obj_check (CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (IS_TRUSTED(ch,GOD)
        || (IS_TRUSTED(ch,IMMORTAL) && obj->level <= 50 && obj->cost <= 1000)
        || (IS_TRUSTED(ch,DEMI)     && obj->level <= 30 && obj->cost <= 500)
        || (IS_TRUSTED(ch,ANGEL)    && obj->level <= 10 && obj->cost <= 250)
        || (IS_TRUSTED(ch,AVATAR)   && obj->level ==  0 && obj->cost <= 100))
        return TRUE;
    else
        return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
    OBJ_DATA *c_obj, *t_obj;


    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
    {
        if (obj_check(ch,c_obj))
        {
            t_obj = create_object(c_obj->pIndexData,0);
            clone_object(c_obj,t_obj);
            obj_to_obj(t_obj,clone);
            recursive_clone(ch,c_obj,t_obj);
        }
    }
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    CHAR_DATA *mob;
    OBJ_DATA  *obj;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    rest = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        send_to_char("Clone what?\n\r",ch);
        buffer_free( buf );
        return;
    }

    if (!str_prefix(arg,"object"))
    {
        mob = NULL;
        obj = get_obj_here(ch,rest);
        if (obj == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            buffer_free( buf );
            return;
        }
    }
    else if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
        obj = NULL;
        mob = get_char_room(ch,rest);
        if (mob == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            buffer_free( buf );
            return;
        }
    }
    else /* find both */
    {
        mob = get_char_room(ch,argument);
        obj = get_obj_here(ch,argument);
        if (mob == NULL && obj == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            buffer_free( buf );
            return;
        }
    }

    /* clone an object */
    if (obj != NULL)
    {
        OBJ_DATA *clone;

        if (!obj_check(ch,obj))
        {
            send_to_char(
                "Your powers are not great enough for such a task.\n\r",ch);
            buffer_free( buf );
            return;
        }

        clone = create_object(obj->pIndexData,0); 
        clone_object(obj,clone);
        if (obj->carried_by != NULL)
            obj_to_char(clone,ch);
        else
            obj_to_room(clone,ch->in_room);
        recursive_clone(ch,obj,clone);

        act("$n has created $p.",ch,clone,NULL,TO_ROOM);
        act("You clone $p.",ch,clone,NULL,TO_CHAR);
        /* added by Rahl */
        wiznet( "$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE,char_getImmRank(ch));
        buffer_free( buf );
        return;
    }
    else if (mob != NULL)
    {
        CHAR_DATA *clone;
        OBJ_DATA *new_obj;

        if (!IS_NPC(mob))
        {
            send_to_char("You can only clone mobiles.\n\r",ch);
            buffer_free( buf );
            return;
        }

        if ((mob->level > 20 && !IS_TRUSTED(ch,GOD))
        ||  (mob->level > 10 && !IS_TRUSTED(ch,IMMORTAL))
        ||  (mob->level >  5 && !IS_TRUSTED(ch,DEMI))
        ||  (mob->level >  0 && !IS_TRUSTED(ch,ANGEL))
        ||  !IS_TRUSTED(ch,AVATAR))
        {
            send_to_char(
                "Your powers are not great enough for such a task.\n\r",ch);
            buffer_free( buf );
            return;
        }

        clone = create_mobile(mob->pIndexData);
      
        while ( clone->affected != NULL )
            affect_strip( clone, clone->affected->type );

        clone_mobile(mob,clone); 
        
        for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
        {
            if (obj_check(ch,obj))
            {
                new_obj = create_object(obj->pIndexData,0);
                clone_object(obj,new_obj);
                recursive_clone(ch,obj,new_obj);
                obj_to_char(new_obj,clone);
                new_obj->wear_loc = obj->wear_loc;
            }
        }
        char_to_room(clone,ch->in_room);
        act("$n has created $N.",ch,NULL,clone,TO_ROOM);
        act("You clone $N.",ch,NULL,clone,TO_CHAR);

        /* added by Rahl */
        bprintf( buf, "$N clones %s.", clone->short_descr );
        wiznet( buf->data, ch, NULL, WIZ_LOAD, WIZ_SECURE, char_getImmRank(ch));

        buffer_free( buf );
        return;
    }
    buffer_free( buf );
    return;
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  load mob <vnum>\n\r",ch);
        send_to_char("  load obj <vnum> <level>\n\r",ch);
        return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    {
        do_mload(ch,argument);
        return;
    }

    if (!str_cmp(arg,"obj"))
    {
        do_oload(ch,argument);
        return;
    }
    /* echo syntax */
    do_load(ch,"");
}


void do_mload( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    
    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number(arg) )
    {
        send_to_char( "Syntax: load mob <vnum>.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
    {
        send_to_char( "No mob has that vnum.\n\r", ch );
        buffer_free( buf );
        return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    act( "$n has created $N!", ch, NULL, victim, TO_ROOM );
    /* added by Rahl */
    bprintf( buf, "$n loads %s", victim->short_descr );
    wiznet( buf->data, ch, NULL, WIZ_LOAD, WIZ_SECURE, char_getImmRank( ch ) );

    send_to_char( "Ok.\n\r", ch );
    buffer_free( buf );
    return;
}



void do_oload( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    int level;
    
    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number(arg1))
    {
        send_to_char( "Syntax: load obj <vnum> <level>.\n\r", ch );
        return;
    }
    
    level = char_getImmRank(ch);  /* default */
  
    if ( arg2[0] != '\0')  /* load with a level */
    {
        if (!is_number(arg2))
        {
          send_to_char( "Syntax: oload <vnum> <level>.\n\r", ch );
          return;
        }
        level = atoi(arg2);
        if (level < 0 || level > char_getImmRank(ch))
        {
          send_to_char( "Level must be be between 0 and your level.\n\r",ch);
          return;
        }
    }

    if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL )
    {
        send_to_char( "No object has that vnum.\n\r", ch );
        return;
    }

    if ( pObjIndex->level > char_getImmRank( ch ) )
    {
        send_to_char( "Loading that object is beyond your powers.\n\r", ch );
        return;
    }

    if ( arg2[0] == '\0' )
        level = pObjIndex->level;

    obj = create_object( pObjIndex, level );
    if ( CAN_WEAR(obj, ITEM_TAKE) )
        obj_to_char( obj, ch );
    else
        obj_to_room( obj, ch->in_room );
    act( "$n has created $p!", ch, obj, NULL, TO_ROOM );
    /* added by Rahl */
    wiznet( "$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, char_getImmRank( ch ));

    send_to_char( "Ok.\n\r", ch );
    return;
}



void do_purge( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( 100 );
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    DESCRIPTOR_DATA *d;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        /* 'purge' */
        CHAR_DATA *vnext;
        OBJ_DATA  *obj_next;

        for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
        {
            vnext = victim->next_in_room;
            if ( IS_NPC(victim) && !IS_SET(victim->act,ACT_NOPURGE) 
            &&   victim != ch /* safety precaution */ )
                extract_char( victim, TRUE );
        }

        for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if (!IS_OBJ_STAT(obj,ITEM_NOPURGE))
              extract_obj( obj );
        }

        act( "`mYou are surrounded by purple flames as $n purges the room.`w", ch, NULL, NULL, TO_ROOM);
        send_to_char( "`mYou are surrounded in all consuming flames.\n\r`w", ch );
        buffer_free( buf );
        return;
    }

    if ( (obj = get_obj_list( ch, arg, ch->in_room->contents ) ) != NULL)
    {
        extract_obj( obj );
        act( "You disintegrate $p.", ch, obj, NULL, TO_CHAR);
        act( "$n disintegrates $p.", ch, obj, NULL, TO_ROOM);
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "Nothing like that in heaven or hell.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !IS_NPC(victim) )
    {

        if (ch == victim)
        {
          send_to_char("Ho ho ho.\n\r",ch);
          buffer_free( buf );
          return;
        }

        if (char_getImmRank(ch) <= char_getImmRank(victim))
        {
          send_to_char("Maybe that wasn't a good idea...\n\r",ch);
          bprintf(buf,"%s tried to purge you!\n\r",ch->name);
          send_to_char(buf->data,victim);
          buffer_free( buf );
          return;
        }

        act("$n disintegrates $N.",ch,0,victim,TO_NOTVICT);
        act( "You disintegrate $N.", ch, 0,victim, TO_CHAR);

        if (victim->level > 1)
            save_char_obj( victim );
        d = victim->desc;
        extract_char( victim, TRUE );
        if ( d != NULL )
          close_socket( d );

        buffer_free( buf );
        return;
    }

    act( "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    act( "You disintegrate $N.", ch, 0,victim, TO_CHAR);
    extract_char( victim, TRUE );

    buffer_free( buf );
    return;
}



void do_promote( CHAR_DATA *ch, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int rank;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) ) {
        send_to_char( "Syntax: promote <char> <rank>.\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "That player is not here.\n\r", ch);
        return;
    }

    if ( IS_NPC(victim) ) {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }

	if ( victim == ch ) {
		send_to_char( "Why would you want to do that to yourself?\n\r", ch );
		return;
	}

    if ( ( rank = atoi( arg2 ) ) < 0 || ( rank > MAX_RANK ) ) {
        BUFFER *buf = buffer_new( 100 );
        bprintf( buf, "Rank must be 0 to %d.\n\r", MAX_RANK );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    if ( rank > char_getImmRank( ch ) ) {
        send_to_char( "Limited to your rank or below.\n\r", ch );
        return;
    }

    /*
     *   Reset to rank 0.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if ( rank <= char_getImmRank( victim ) ) {
        send_to_char( "Lowering a player's rank!\n\r", ch );
		send_to_char( "You have been demoted!\n\r", victim );
       	char_setImmRank( victim, rank );

		if ( rank == 0 ) {
        	victim->exp      = 0;
        	victim->max_hit  = 10;
        	victim->max_mana = 100;
        	victim->max_move = 100;
        	victim->practice = 0;
        	victim->train    = 0; 
        	victim->hit      = victim->max_hit;
        	victim->mana     = victim->max_mana;
        	victim->move     = victim->max_move;
		}
    } else {
        send_to_char( "Raising a player's rank!\n\r", ch );
        send_to_char( "You raise a rank!!", victim );
        char_setImmRank( victim, rank );
    }
    victim->exp   = 0;
    save_char_obj(victim);
    return;
}



void do_restore( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    one_argument( argument, arg );
    if (arg[0] == '\0' || !str_cmp(arg,"room"))
    {
    /* cure room */
        
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            affect_strip(vch,gsn_plague);
            affect_strip(vch,gsn_poison);
            affect_strip(vch,gsn_blindness);
            affect_strip(vch,gsn_sleep);
            affect_strip(vch,gsn_curse);
            
            vch->hit    = vch->max_hit;
            vch->mana   = vch->max_mana;
            vch->move   = vch->max_move;
            update_pos( vch);
            act("$n has restored you.",ch,NULL,vch,TO_VICT);
        }
        
        /* added by Rahl */
        bprintf( buf, "$N restored room %d.", ch->in_room->vnum );
        wiznet( buf->data, ch, NULL, WIZ_RESTORE, WIZ_SECURE, 
            char_getImmRank( ch )); 

        send_to_char("Room restored.\n\r",ch);
        buffer_free( buf );
        return;

    }
    
    if ( char_getImmRank(ch) >=  MAX_RANK && !str_cmp(arg,"all"))
    {
    /* cure all */
        
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            victim = d->character;

            if (victim == NULL || IS_NPC(victim))
                continue;
                
            affect_strip(victim,gsn_plague);
            affect_strip(victim,gsn_poison);
            affect_strip(victim,gsn_blindness);
            affect_strip(victim,gsn_sleep);
            affect_strip(victim,gsn_curse);
            
            victim->hit         = victim->max_hit;
            victim->mana        = victim->max_mana;
            victim->move        = victim->max_move;
            update_pos( victim);
            if (victim->in_room != NULL)
                act("$n has restored you.",ch,NULL,victim,TO_VICT);
        }
        send_to_char("All active players restored.\n\r",ch);
        bprintf( buf, "$N restored everyone." );
        wiznet( buf->data, ch, NULL, WIZ_RESTORE, WIZ_SECURE, 
            char_getImmRank( ch ) );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    affect_strip(victim,gsn_plague);
    affect_strip(victim,gsn_poison);
    affect_strip(victim,gsn_blindness);
    affect_strip(victim,gsn_sleep);
    affect_strip(victim,gsn_curse);
    victim->hit  = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos( victim );
    act( "$n has restored you.", ch, NULL, victim, TO_VICT );

    /* added by Rahl */
    bprintf( buf, "$N restored %s.", IS_NPC( victim ) ?
                victim->short_descr : victim->name );
    wiznet( buf->data, ch, NULL, WIZ_RESTORE, WIZ_SECURE, 
        char_getImmRank( ch ) );

    send_to_char( "Ok.\n\r", ch );
    buffer_free( buf );
    return;
}


void do_freeze( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );


    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Freeze whom?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( char_getImmRank( victim ) >= char_getImmRank( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET(victim->act, PLR_FREEZE) )
    {
        REMOVE_BIT(victim->act, PLR_FREEZE);
        send_to_char( "You can play again.\n\r", victim );
        send_to_char( "FREEZE removed.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N thaws %s.", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
    }
    else
    {
        SET_BIT(victim->act, PLR_FREEZE);
        send_to_char( "You can't do ANYthing!\n\r", victim );
        send_to_char( "FREEZE set.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N freezes %s.", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
    }

    save_char_obj( victim );
    buffer_free( buf );

    return;
}



void do_log( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Log whom?\n\r", ch );
        return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        if ( fLogAll )
        {
            fLogAll = FALSE;
            send_to_char( "Log ALL off.\n\r", ch );
        }
        else
        {
            fLogAll = TRUE;
            send_to_char( "Log ALL on.\n\r", ch );
        }
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if ( IS_SET(victim->act, PLR_LOG) )
    {
        REMOVE_BIT(victim->act, PLR_LOG);
        send_to_char( "LOG removed.\n\r", ch );
    }
    else
    {
        SET_BIT(victim->act, PLR_LOG);
        send_to_char( "LOG set.\n\r", ch );
    }

    return;
}



void do_noemote( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );


    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Noemote whom?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }


    if ( char_getImmRank( victim ) >= char_getImmRank( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET(victim->comm, COMM_NOEMOTE) )
    {
        REMOVE_BIT(victim->comm, COMM_NOEMOTE);
        send_to_char( "You can emote again.\n\r", victim );
        send_to_char( "NOEMOTE removed.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N restores %s's emotions.", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOEMOTE);
        send_to_char( "You can't emote!\n\r", victim );
        send_to_char( "NOEMOTE set.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N revokes %s's emotions.", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
    }

    buffer_free( buf );
    return;
}



void do_notell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Notell whom?", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( char_getImmRank( victim ) >= char_getImmRank( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET(victim->comm, COMM_NOTELL) )
    {
        REMOVE_BIT(victim->comm, COMM_NOTELL);
        send_to_char( "You can tell again.\n\r", victim );
        send_to_char( "NOTELL removed.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N restores %s's ability to tell.", victim->name);
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOTELL);
        send_to_char( "You can't tell!\n\r", victim );
        send_to_char( "NOTELL set.\n\r", ch );
        /* added by Rahl */
        bprintf( buf, "$N revokes %s's ability to tell.", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 ); 
   }

    buffer_free( buf );
    return;
}



void do_peace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *rch;

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( rch->fighting != NULL )
            stop_fighting( rch, TRUE );
        if (IS_NPC(rch) && IS_SET(rch->act,ACT_AGGRESSIVE))
            REMOVE_BIT(rch->act,ACT_AGGRESSIVE);
    }

    send_to_char( "Ok.\n\r", ch );
    return;
}


void do_wizlock( CHAR_DATA *ch, char *argument )
{
    wizlock = !wizlock;

    if ( wizlock )
    {
        send_to_char( "Game wizlocked.\n\r", ch );
        /* added by Rahl */
        wiznet( "$N has wizlocked the game.", ch, NULL, 0, 0, 0 );
    }
    else
    {
        send_to_char( "Game un-wizlocked.\n\r", ch );
        /* added by Rahl */
        wiznet( "$N removes wizlock.", ch, NULL, 0, 0, 0 );
    }
    return;
}

/* RT anti-newbie code */

void do_newlock( CHAR_DATA *ch, char *argument )
{
    newlock = !newlock;
 
    if ( newlock )
    {
        send_to_char( "New characters have been locked out.\n\r", ch );
        /* added by Rahl */
        wiznet( "$N newlocks the game.", ch, NULL, 0, 0, 0 );
    }
    else
    {
        send_to_char( "New characters are now allowed.\n\r", ch );
        /* added by Rahl */
        wiznet( "$N allows new characters back in.", ch, NULL, 0, 0, 0 );
    }
    return;
}


void do_slookup( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Lookup which skill or spell?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name == NULL )
                break;
            bprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
                sn, skill_table[sn].slot, skill_table[sn].name );
            send_to_char( buf->data, ch );
        }
    }
    else
    {
        if ( ( sn = skill_lookup( arg ) ) < 0 )
        {
            send_to_char( "No such skill or spell.\n\r", ch );
            buffer_free( buf );
            return;
        }

        bprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
            sn, skill_table[sn].slot, skill_table[sn].name );
        send_to_char( buf->data, ch );
    }

    buffer_free( buf );
    return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  set mob   <name> <field> <value>\n\r",ch);
        send_to_char("  set obj   <name> <field> <value>\n\r",ch);
        send_to_char("  set room  <room> <field> <value>\n\r",ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r",ch);
        return;
    }

    if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
        do_mset(ch,argument);
        return;
    }

    if (!str_prefix(arg,"skill") || !str_prefix(arg,"spell"))
    {
        do_sset(ch,argument);
        return;
    }

    if (!str_prefix(arg,"object"))
    {
        do_oset(ch,argument);
        return;
    }

    if (!str_prefix(arg,"room"))
    {
        do_rset(ch,argument);
        return;
    }
    /* echo syntax */
    do_set(ch,"");
}


void do_sset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char( "Syntax:\n\r",ch);
        send_to_char( "  set skill <name> <spell or skill> <value>\n\r", ch);
        send_to_char( "  set skill <name> all <value>\n\r",ch);  
        send_to_char("   (use the name of the skill, not the number)\n\r",ch);
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }

    fAll = !str_cmp( arg2, "all" );
    sn   = 0;
    if ( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
    {
        send_to_char( "No such skill or spell.\n\r", ch );
        return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
        send_to_char( "Value must be numeric.\n\r", ch );
        return;
    }

    value = atoi( arg3 );
    if ( value < 0 || value > 100 )
    {
        send_to_char( "Value range is 0 to 100.\n\r", ch );
        return;
    }

    if ( fAll )
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name != NULL )
                victim->pcdata->learned[sn]     = value;
        }
    }
    else
    {
        victim->pcdata->learned[sn] = value;
    }

    return;
}



void do_mset( CHAR_DATA *ch, char *argument )
{
   char arg1 [MAX_INPUT_LENGTH];
   char arg2 [MAX_INPUT_LENGTH];
   char arg3 [MAX_INPUT_LENGTH];
   BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
   CHAR_DATA *victim;
   CLAN_DATA *Clan;
   CLAN_DATA *Clan2;
   ROOM_INDEX_DATA *location;
   int value;
/*   char *tmpmem[MAX_CLAN_MEMBERS]; */
/*   int tmpcount, tmpcount2=0; */
   
   
   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   strcpy( arg3, argument );
   
    if (( arg1[0] == '\0') || (arg2[0] == '\0') || (arg3[0] == '\0'))
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  set char <name> <field> <value>\n\r",ch); 
        send_to_char( "  Field being one of:\n\r",                      ch );
        send_to_char( "    str int wis dex con sex class rank\n\r",    ch );
        send_to_char( "    race gold hp mana move practice align\n\r",  ch );
        send_to_char( "    train thirst drunk full security recall\n\r",ch );
        send_to_char( "    clan questpoints flag clan_leader\n\r",ch );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    /* clear zones for mobs */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number( arg3 ) ? atoi( arg3 ) : -1;

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "str" ) )
    {
        if ( value < 3 || value > get_max_train(victim,STAT_STR) )
        {
            bprintf(buf,
                "Strength range is 3 to %d.\n\r",
                get_max_train(victim,STAT_STR));
            send_to_char(buf->data,ch);
            buffer_free( buf );
            return;
        }

        victim->perm_stat[STAT_STR] = value;
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "int" ) )
    {
        if ( value < 3 || value > get_max_train(victim,STAT_INT) )
        {
            bprintf(buf,
                "Intelligence range is 3 to %d.\n\r",
                get_max_train(victim,STAT_INT));
            send_to_char(buf->data,ch);
            buffer_free( buf );
            return;
        }
 
        victim->perm_stat[STAT_INT] = value;
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "wis" ) )
    {
        if ( value < 3 || value > get_max_train(victim,STAT_WIS) )
        {
            bprintf(buf,
                "Wisdom range is 3 to %d.\n\r",
                    get_max_train(victim,STAT_WIS));
            send_to_char( buf->data, ch );
            buffer_free( buf );
            return;
        }

        victim->perm_stat[STAT_WIS] = value;
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "dex" ) )
    {
        if ( value < 3 || value > get_max_train(victim,STAT_DEX) )
        {
            bprintf(buf,
                "Dexterity ranges is 3 to %d.\n\r",
                get_max_train(victim,STAT_DEX));
            send_to_char( buf->data, ch );
            buffer_free( buf );
            return;
        }

        victim->perm_stat[STAT_DEX] = value;
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "con" ) )
    {
        if ( value < 3 || value > get_max_train(victim,STAT_CON) )
        {
            bprintf(buf,
                "Constitution range is 3 to %d.\n\r",
                get_max_train(victim,STAT_CON));
            send_to_char( buf->data, ch );
            buffer_free( buf );
            return;
        }

        victim->perm_stat[STAT_CON] = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "sex" ) )
    {
        if ( value < 0 || value > 2 )
        {
            send_to_char( "Sex range is 0 to 2.\n\r", ch );
            buffer_free( buf );
            return;
        }
        victim->sex = value;
        if (!IS_NPC(victim))
            victim->pcdata->true_sex = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "class" ) )
    {
        int class;

        if (IS_NPC(victim))
        {
            send_to_char("Mobiles have no class.\n\r",ch);
            buffer_free( buf );
            return;
        }

        class = class_lookup(arg3);
        if ( class == -1 )
        {
                bprintf( buf, "Possible classes are: " );
                for ( class = 0; class < MAX_CLASS; class++ )
                {
                    if ( class > 0 )
                        buffer_strcat( buf, " " );
                    buffer_strcat( buf, class_table[class].name );
                }
            buffer_strcat( buf, ".\n\r" );

            send_to_char(buf->data,ch);
            buffer_free( buf );
            return;
        }

        victim->ch_class = class;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "rank" ) )
    {
        if ( !IS_NPC(victim) )
        {
            send_to_char( "Not on PC's.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( value < 0 || value > MAX_RANK )
        {
            bprintf( buf, "Rank range is 0 to %d.\n\r", MAX_RANK );
            send_to_char( buf->data, ch );
            buffer_free( buf );
            return;
        }
        char_setImmRank( victim, value );
        buffer_free( buf );
        return;
    }
    if ( !str_prefix( arg2, "clan" ) )
    {
        if ( IS_NPC(victim) )
        {
            send_to_char( "Not on mobiles (yet).\n\r", ch );
            buffer_free( buf );
            return;
        }

        
       if ( !str_prefix( arg3, "none" ))
         {
            if (victim->pcdata->clan <= 0)
              {
                 bprintf(buf,"%s is not a member of a clan!\n\r",
                    victim->name);
                 send_to_char(buf->data,ch);
                 buffer_free( buf );
                 return;
              }
            bprintf(buf,"%d",victim->pcdata->clan);
            Clan=find_clan(buf->data);
/*
            for (tmpcount=0 ;tmpcount < Clan->num_members ; tmpcount++)
              {
                 if (Clan->members[tmpcount]!=victim->name)
                   {
                      tmpmem[tmpcount2]=Clan->members[tmpcount];
                      tmpcount2++;
                   }
              }
*/
            Clan->num_members = Clan->num_members-1;
/*
            for (tmpcount=0 ; tmpcount < Clan->num_members ; tmpcount++)
              {
                 Clan->members[tmpcount] = tmpmem[tmpcount];
              }
            Clan->members[Clan->num_members+1]=NULL;
*/
            bprintf(buf,"%s is no longer a member of a clan.\n\r",
                victim->name);
            send_to_char(buf->data, ch);
            if ( is_clan_leader( victim, clan_lookup( victim->pcdata->clan ) ) )
                victim->pcdata->clan_leader = 0;        
            victim->pcdata->clan = 0;
            do_asave(ch, "clans");
            buffer_free( buf );
            return;
         }
       Clan=find_clan(arg3);
       if (Clan==NULL) 
         {
            send_to_char( "No such clan.\n\r", ch);
            buffer_free( buf );
            return;
         }
       else 
         {
            bprintf(buf,"%d",victim->pcdata->clan);
            Clan2=find_clan(buf->data);
            if (Clan2!=NULL)
              {
/*
                 for (tmpcount=0 ; tmpcount < Clan2->num_members ; tmpcount++)
                   {
                      if (Clan2->members[tmpcount]!=victim->name)
                        {
                           tmpmem[tmpcount2]=Clan2->members[tmpcount];
                           tmpcount2++;
                        }
                   }
*/
                 bprintf(buf,"%s is no longer a member of clan %s.\n\r",
                     victim->name,Clan2->visible);
                 send_to_char( buf->data, ch);
                 Clan2->num_members = Clan2->num_members-1;
              }
/*
            Clan->members[Clan->num_members]=str_dup(victim->name);
*/
            Clan->num_members = Clan->num_members+1;
            bprintf(buf,"%s is now a member of clan %s.\n\r",
                victim->name,Clan->visible);
            send_to_char( buf->data, ch);
            victim->pcdata->clan = Clan->number;
            do_asave(ch, "clans");
            buffer_free( buf );
            return;
         }  
       buffer_free( buf );
       return;
    }
   
   if ( !str_prefix( arg2, "gold" ) )
     {
        victim->gold = value;
        buffer_free( buf );
        return;
    }

    /* added by Rahl */
    if (!str_prefix( arg2, "questpoints" ) )
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPCs.\n\r", ch );
            buffer_free( buf );
            return;
        }
        else
        {
            victim->questpoints = value;
            buffer_free( buf );
            return;
        }
    }

    /* added by Rahl */
    if (!str_prefix( arg2, "clan_leader" ) )
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPCs.\n\r", ch );
            buffer_free( buf );
            return;
        }
        if ( value < 0 || value > 1 )
        {
            send_to_char( "Values are either 0 or 1\n\r", ch );
            buffer_free( buf );
            return;
        }
        if ( victim->pcdata->clan == 0 )
        {
            send_to_char( "They aren't in a clan.\n\r", ch );
            buffer_free( buf );
            return;
        }
        else
        {
            victim->pcdata->clan_leader = value;
            if ( value )
            {
                send_to_char( "You are now a clan leader.\n\r", victim );
                bprintf( buf, "%s is now a clan leader.\n\r", victim->name );
                send_to_char( buf->data, ch );
            }
            else
            {
                send_to_char( "You are no longer a clan leader.\n\r", victim );
                bprintf( buf, "%s no longer a clan leader.\n\r",
                    victim->name );
                send_to_char( buf->data, ch );
            }
            buffer_free( buf );
            return;
        }
    }

    if ( !str_prefix( arg2, "hp" ) )
    {
        if ( value < -10 || value > 30000 )
        {
            send_to_char( "Hp range is -10 to 30,000 hit points.\n\r", ch );
            buffer_free( buf );
            return;
        }
        victim->max_hit = value;
        if (!IS_NPC(victim))
            victim->pcdata->perm_hit = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "mana" ) )
    {
        if ( value < 0 || value > 30000 )
        {
            send_to_char( "Mana range is 0 to 30,000 mana points.\n\r", ch );
            buffer_free( buf );
            return;
        }
        victim->max_mana = value;
        if (!IS_NPC(victim))
            victim->pcdata->perm_mana = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "move" ) )
    {
        if ( value < 0 || value > 30000 )
        {
            send_to_char( "Move range is 0 to 30,000 move points.\n\r", ch );
            buffer_free( buf );
            return;
        }
        victim->max_move = value;
        if (!IS_NPC(victim))
            victim->pcdata->perm_move = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "practice" ) )
    {
        if ( value < 0 || value > 250 )
        {
            send_to_char( "Practice range is 0 to 250 sessions.\n\r", ch );
            buffer_free( buf );
            return;
        }
        victim->practice = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "train" ))
    {
        if (value < 0 || value > 50 )
        {
            send_to_char("Training session range is 0 to 50 sessions.\n\r",ch);
            buffer_free( buf );
            return;
        }
        victim->train = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "align" ) )
    {
        if ( value < -1000 || value > 1000 )
        {
            send_to_char( "Alignment range is -1000 to 1000.\n\r", ch );
            buffer_free( buf );
            return;
        }
        victim->alignment = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "thirst" ) )
    {
        if ( IS_NPC(victim) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            send_to_char( "Thirst range is -1 to 100.\n\r", ch );
            buffer_free( buf );
            return;
        }

        victim->pcdata->condition[COND_THIRST] = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "drunk" ) )
    {
        if ( IS_NPC(victim) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            send_to_char( "Drunk range is -1 to 100.\n\r", ch );
            buffer_free( buf );
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "full" ) )
    {
        if ( IS_NPC(victim) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            send_to_char( "Full range is -1 to 100.\n\r", ch );
            buffer_free( buf );
            return;
        }

        victim->pcdata->condition[COND_FULL] = value;
        buffer_free( buf );
        return;
    }

    if (!str_prefix( arg2, "race" ) )
    {
        int race;

        race = race_lookup(arg3);

        if ( race == 0)
        {
            send_to_char("That is not a valid race.\n\r",ch);
            buffer_free( buf );
            return;
        }

        if (!IS_NPC(victim) && !race_table[race].pc_race)
        {
            send_to_char("That is not a valid player race.\n\r",ch);
            buffer_free( buf );
            return;
        }

        victim->race = race;
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "security" ) ) /* OLC */
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( value > ch->pcdata->security || value < 0 )
        {
			/*
			 * Maybe make it so that only IMPs can set
			 * security?
			 */
            if ( ch->pcdata->security != 0 )
            {
                bprintf( buf, "Valid security is 0-%d.\n\r",
                    ch->pcdata->security );
                send_to_char( buf->data, ch );
            }
            else
            {
                send_to_char( "Valid security is 0 only.\n\r", ch );
            }
            buffer_free( buf );
            return;
        }
        victim->pcdata->security = value;
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg2, "recall" ) )   /* Thexder */
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            buffer_free( buf );
            return;
        }
        if ( ( location = find_location( ch, arg3 ) ) == NULL )
        {
            send_to_char( "That is an invalid location.\n\r", ch );
            buffer_free( buf );
            return;
        }
        else 
        {
            victim->pcdata->recall_room = location;
            send_to_char( "Recall set.\n\r", ch);
            buffer_free( buf );
            return;
        }
    }

    /* flag by Rahl */
    if ( !str_prefix( arg2, "flag" ) )
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPCs.\n\r", ch );
            buffer_free( buf );
            return;
        }       

        if ( arg3[0] == '\0' )
        {
            send_to_char( "Flags are: killer, thief, bountyhunter\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( !str_cmp( arg3, "killer" ) )
        {
            if ( IS_SET( victim->act, PLR_KILLER ) )
            {
                REMOVE_BIT( victim->act, PLR_KILLER );
                bprintf( buf, "%s is no longer a player killer.\n\r",
                    victim->name );
                send_to_char( buf->data, ch );
                send_to_char( "You are no longer a player killer.\n\r", 
                    victim );
            }
            else
            {
                SET_BIT( victim->act, PLR_KILLER );
                bprintf( buf, "%s is now a player killer.\n\r", 
                    victim->name );
                send_to_char( buf->data, ch );
                send_to_char( "You are now a player killer.\n\r",       
                    victim );
            }
        }

        if ( !str_cmp( arg3, "thief" ) )
        {
            if ( IS_SET( victim->act, PLR_THIEF ) )
            {
                REMOVE_BIT( victim->act, PLR_THIEF );
                bprintf( buf, "%s is no longer a thief.\n\r",
                    victim->name );
                send_to_char( buf->data, ch );
                send_to_char( "You are no longer a thief.\n\r", 
                    victim );
            }
            else
            {
                SET_BIT( victim->act, PLR_THIEF );
                bprintf( buf, "%s is now a thief.\n\r", 
                    victim->name );
                send_to_char( buf->data, ch );
                send_to_char( "You are now a thief.\n\r",       
                    victim );
            }
        }

        if ( !str_cmp( arg3, "bountyhunter" ) )
        {
            if ( IS_SET( victim->act, PLR_BOUNTY_HUNTER ) )
            {
                REMOVE_BIT( victim->act, PLR_BOUNTY_HUNTER );
                bprintf( buf, "%s is no longer a bounty hunter.\n\r",
                    victim->name );
                send_to_char( buf->data, ch );
                send_to_char( "You are no longer a bounty hunter.\n\r", 
                    victim );
            }
            else
            {
                SET_BIT( victim->act, PLR_BOUNTY_HUNTER );
                bprintf( buf, "%s is now a bounty hunter.\n\r", 
                    victim->name );
                send_to_char( buf->data, ch );
                send_to_char( "You are now a bounty hunter.\n\r",       
                    victim );
            }
        }
        buffer_free( buf );
        return;
    }


    /*
     * Generate usage message.
     */
    do_mset( ch, "" );
    buffer_free( buf );
    return;
}

void do_string( CHAR_DATA *ch, char *argument )
{
    char type [MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    smash_tilde( argument );
    argument = one_argument( argument, type );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  string char <name> <field> <string>\n\r",ch);
        send_to_char("    fields: name short long desc title spec\n\r",ch);
        send_to_char("  string obj  <name> <field> <string>\n\r",ch);
        send_to_char("    fields: name short long extended\n\r",ch);
        return;
    }
    
    if (!str_prefix(type,"character") || !str_prefix(type,"mobile"))
    {
        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
        {
            send_to_char( "They aren't here.\n\r", ch );
            return;
        }

        /* clear zone for mobs */
        victim->zone = NULL;

        /* string something */

        if ( !str_prefix( arg2, "name" ) )
        {
            if ( !IS_NPC(victim) )
            {
                send_to_char( "Not on PC's.\n\r", ch );
                return;
            }

            free_string( victim->name );
            victim->name = str_dup( arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "description" ) )
        {
            free_string(victim->description);
            string_append( ch, &victim->description );
            return;
        }

        if ( !str_prefix( arg2, "short" ) )
        {
            free_string( victim->short_descr );
            victim->short_descr = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "long" ) )
        {
            free_string( victim->long_descr );
            strcat(arg3,"\n\r");
            victim->long_descr = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "title" ) )
        {
            if ( IS_NPC(victim) )
            {
                send_to_char( "Not on NPC's.\n\r", ch );
                return;
            }

            set_title( victim, arg3 );
            return;
        }

        if ( !str_prefix( arg2, "spec" ) )
        {
            if ( !IS_NPC(victim) )
            {
                send_to_char( "Not on PC's.\n\r", ch );
                return;
            }

            if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
            {
                send_to_char( "No such spec fun.\n\r", ch );
                return;
            }

            return;
        }
    }
    
    if (!str_prefix(type,"object"))
    {
        /* string an obj */
        
        if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
        {
            send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
            return;
        }
        
        if ( !str_prefix( arg2, "name" ) )
        {
            free_string( obj->name );
            obj->name = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "short" ) )
        {
            free_string( obj->short_descr );
            obj->short_descr = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "long" ) )
        {
            free_string( obj->description );
            obj->description = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "ed" ) || !str_prefix( arg2, "extended"))
        {
            EXTRA_DESCR_DATA *ed;

            argument = one_argument( argument, arg3 );
            if ( argument == NULL )
            {
                send_to_char( "Syntax: oset <object> ed <keyword> <string>\n\r",
                    ch );
                return;
            }

            strcat(argument,"\n\r");

            if ( extra_descr_free == NULL )
            {
                ed                      = alloc_perm( sizeof(*ed) );
            }
            else
            {
                ed                      = extra_descr_free;
                extra_descr_free        = ed->next;
            }

            ed->keyword         = str_dup( arg3     );
            ed->description     = str_dup( argument );
            ed->next            = obj->extra_descr;
            obj->extra_descr    = ed;
            return;
        }
    }
    
        
    /* echo bad use message */
    do_string(ch,"");
}



void do_oset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  set obj <object> <field> <value>\n\r",ch);
        send_to_char("  Field being one of:\n\r",                               ch );
        send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r",      ch );
        send_to_char("    extra wear level weight cost timer\n\r",              ch );
        return;
    }

    if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number( arg3) ? atoi( arg3 ) : -1;

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
    {
        obj->value[0] = UMIN(50,value);
        return;
    }

    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
    {
        obj->value[1] = value;
        return;
    }

    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
    {
        obj->value[2] = value;
        return;
    }

    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
    {
        obj->value[3] = value;
        return;
    }

    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
    {
        obj->value[3] = value;
        return;
    }

    if ( !str_prefix( arg2, "extra" ) )
    {
        obj->extra_flags = value;
        return;
    }

    if ( !str_prefix( arg2, "wear" ) )
    {
        obj->wear_flags = value;
        return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
        obj->level = value;
        return;
    }
        
    if ( !str_prefix( arg2, "weight" ) )
    {
        obj->weight = value;
        return;
    }

    if ( !str_prefix( arg2, "cost" ) )
    {
        obj->cost = value;
        return;
    }

    if ( !str_prefix( arg2, "timer" ) )
    {
        obj->timer = value;
        return;
    }
        
    /*
     * Generate usage message.
     */
    do_oset( ch, "" );
    return;
}



void do_rset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char( "Syntax:\n\r",ch);
        send_to_char( "  set room <location> <field> <value>\n\r",ch);
        send_to_char( "  Field being one of:\n\r",                      ch );
        send_to_char( "    flags sector\n\r",                           ch );
        return;
    }

    if ( ( location = find_location( ch, arg1 ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }

    if (!is_room_owner(ch,location) && ch->in_room != location
    &&  room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That room is private right now.\n\r",ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
        send_to_char( "Value must be numeric.\n\r", ch );
        return;
    }
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_prefix( arg2, "flags" ) )
    {
        location->room_flags    = value;
        return;
    }

    if ( !str_prefix( arg2, "sector" ) )
    {
        location->sector_type   = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_rset( ch, "" );
    return;
}



void do_sockets( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    int count = 0;

    one_argument(argument,arg);
    send_to_char( "\r", ch );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->character != NULL && can_see( ch, d->character ) 
        && (arg[0] == '\0' || is_name(arg,d->character->name)
                           || (d->original && is_name(arg,d->original->name))))
        {
            count++;
            bprintf( buf, "[%3d %2d] %-12s %-39s Idle: %3d ticks\n\r",
                d->descriptor,
                d->connected,
                d->original  ? d->original->name  :
                d->character ? d->character->name : "(none)",
                d->host,
                d->character->timer
                );
            buffer_strcat( buffer, buf->data );
        }
    }
    if (count == 0)
    {
        send_to_char("No one by that name is connected.\n\r",ch);
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }
    
    bprintf( buf, "%d user%s\n\r", count, count == 1 ? "" : "s" );
    buffer_strcat( buffer, buf->data );
    page_to_char( buffer->data, ch );
    buffer_free( buffer );
    buffer_free( buf );
    return;
}




/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "Force whom to do what?\n\r", ch );
        buffer_free( buf );
        return;
    }

    one_argument(argument,arg2);
  
    if (!str_cmp(arg2,"delete") ) 
    {
        send_to_char("That will NOT be done.\n\r",ch);
        buffer_free( buf );
        return;
    }


    bprintf( buf, "$n forces you to '%s'.", argument );

    if ( !str_cmp( arg, "all" ) )
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (char_getImmRank(ch) < MAX_RANK - 3)
        {
            send_to_char("Not at your level!\n\r",ch);
            buffer_free( buf );
            return;
        }

        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;

            if ( !IS_NPC(vch) && char_getImmRank( vch ) < char_getImmRank( ch ) )
            {
                act( buf->data, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else if (!str_cmp(arg,"players"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
 
        if (char_getImmRank(ch) < MAX_RANK - 2)
        {
            send_to_char("Not at your level!\n\r",ch);
            buffer_free( buf );
            return;
        }
 
        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;
 
            if ( !IS_NPC(vch) && char_getImmRank( vch ) < char_getImmRank( ch ) 
            &&   char_getImmRank( vch ) < 1 ) // Lowest Rank
            {
                act( buf->data, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else if (!str_cmp(arg,"gods") || !str_cmp( arg, "immortals" ))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
 
        if (char_getImmRank(ch) < MAX_RANK - 2)
        {
            send_to_char("Not at your level!\n\r",ch);
            buffer_free( buf );
            return;
        }
 
        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;
 
            if ( !IS_NPC(vch) && char_getImmRank( vch ) < char_getImmRank( ch )
            &&  IS_IMMORTAL( vch ) ) 
            {
                act( buf->data, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else
    {
        CHAR_DATA *victim;

        if ( ( victim = get_char_world( ch, arg ) ) == NULL )
        {
            send_to_char( "They aren't here.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( victim == ch )
        {
            send_to_char( "Aye aye, right away!\n\r", ch );
            buffer_free( buf );
            return;
        }


        if (!is_room_owner(ch,victim->in_room)
        &&  ch->in_room != victim->in_room
        &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
        {
            send_to_char("That character is in a private room.\n\r",ch);
            buffer_free( buf );
            return;
        }

        if ( char_getImmRank( victim ) >= char_getImmRank( ch ) )
        {
            send_to_char( "Do it yourself!\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( !IS_NPC(victim) && char_getImmRank(ch) < MAX_RANK -3)
        {
            send_to_char("Not at your level!\n\r",ch);
            buffer_free( buf );
            return;
        }

        act( buf->data, ch, NULL, victim, TO_VICT );
        interpret( victim, argument );
    }

    send_to_char( "Ok.\n\r", ch );
    buffer_free( buf );
    return;
}



/*
 * New routines by Dionysos.
 */
void do_invis( CHAR_DATA *ch, char *argument )
{
    int rank;
    char arg[MAX_INPUT_LENGTH];

    if ( IS_NPC(ch) ) {
        return;
	}

    /* RT code for taking a level argument */
    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
    /* take the default path */

      if ( IS_SET(ch->act, PLR_WIZINVIS) ) {
          REMOVE_BIT(ch->act, PLR_WIZINVIS);
          ch->invis_level = 0;
          act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You slowly fade back into existence.\n\r", ch );
      } else {
          SET_BIT(ch->act, PLR_WIZINVIS);
          ch->invis_level = char_getImmRank(ch);
          act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You slowly vanish into thin air.\n\r", ch );
      }
    } else { /* do the level thing */
      rank = atoi(arg);
      if (rank < 1 || rank > char_getImmRank(ch)) {
        send_to_char("Invis level must be between 1 and your rank.\n\r",ch);
        return;
      } else {
          ch->reply = NULL;
          SET_BIT(ch->act, PLR_WIZINVIS);
          ch->invis_level = rank;
          act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You slowly vanish into thin air.\n\r", ch );
      }
    }

    return;
}



void do_holylight( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) ) {
        return;
	}

    if ( IS_SET(ch->act, PLR_HOLYLIGHT) ) {
        REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char( "Holy light mode off.\n\r", ch );
    } else {
        SET_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char( "Holy light mode on.\n\r", ch );
    }

    return;
}

void do_chaos( CHAR_DATA *ch, char *argument )
{
    if (!chaos)
    {
        do_force ( ch, "all save");
        do_save (ch, "");
    }

    chaos = !chaos;

    if ( chaos )
    {
        send_to_char( "Chaos now set.\n\r", ch );
        do_sendinfo(ch , "`rC`RH`YA`RO`rS`R has begun!");
    }
    else
    {
        send_to_char( "Chaos cancelled.\n\r", ch );
        do_sendinfo(ch, "`rC`RH`YA`RO`rS`R has been cancelled.");
    }

    return;
}

void do_rlist(CHAR_DATA *ch, char *argument)
{
    /* buffer sizes increased by Rahl */

    ROOM_INDEX_DATA     *pRoomIndex;
    AREA_DATA           *pArea;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf1 = buffer_new( MAX_INPUT_LENGTH );
    char                arg  [ MAX_INPUT_LENGTH    ];
    bool found;
    int vnum;
    int avnum;
    int  col = 0;

    one_argument( argument, arg );
    found   = FALSE;

    if (arg[0]=='\0')
    {
       pArea = ch->in_room->area;

       for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ )
         {
           if ( ( pRoomIndex = get_room_index( vnum ) ) )
             {
               found = TRUE;
               bprintf( buf, "[%5d] %-32.31s`w",
               pRoomIndex->vnum, capitalize( pRoomIndex->name ) );
               buffer_strcat( buf1, buf->data );
               if ( ++col % 2 == 0 )
                   buffer_strcat( buf1, "\n\r" );
             }
         }

      if ( !found )
      {
         send_to_char( "No rooms found in this area.\n\r", ch);
         buffer_free( buf );
         buffer_free( buf1 );
         return;
      }

        if ( col % 2 != 0 )
            buffer_strcat( buf1, "\n\r" );

        page_to_char( buf1->data, ch );
    
        buffer_free( buf );
        buffer_free( buf1 );
        return;
    }

 else if ( is_number(argument) ) 

    {
      avnum=atoi(argument);
        for (pArea = area_first; pArea; pArea = pArea->next )
          {
             if (pArea->vnum == avnum) 
                {
                   found=TRUE;
                   break;
                }
          }

      if (!found) 
        {
           send_to_char("No such area!\n\r",ch);
           buffer_free( buf );
           buffer_free( buf1 );
           return;
        }

      found=FALSE;

      for ( vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++ )
         {
           if ( ( pRoomIndex = get_room_index( vnum ) ) )
             {
               found = TRUE;
               bprintf( buf, "[%5d] %-32.31s`w",
               pRoomIndex->vnum, capitalize( pRoomIndex->name ) );
               buffer_strcat( buf1, buf->data );
               if ( ++col % 2 == 0 )
                   buffer_strcat( buf1, "\n\r" );
             }
         }

     if ( col % 2 != 0 )
        buffer_strcat( buf1, "\n\r" );

      page_to_char( buf1->data, ch );
      buffer_free( buf );
      buffer_free( buf1 );
      return;
   }
   buffer_free( buf );
   buffer_free( buf1 );
   return;
}


void do_test ( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   argument = one_argument( argument, arg1 );
   strcpy(arg2, argument);

   if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
     {
        send_to_char("That player either doesn't exist, or is not currently logged in.\n\r", ch);
        send_to_char("You may currently only test players who are online.\n\r", ch);
        return;
     }
   if ( IS_NPC (victim) )
     {
        send_to_char("Mobiles don't need to be tested to join a clan, silly!\n\r", ch);
        return;
     }
   if (clan_accept(victim, arg2) == 0)
     {
        send_to_char("That player would be accepted by that clan.\n\r", ch);
        return;
     }
   if (clan_accept(victim, arg2) == 1)
     {
        send_to_char("That clan doesn't exist (clan_accept failure #1).\n\r", ch);
     }
   if (clan_accept(victim, arg2) == 2)
     {
        send_to_char("That clan has reached max members! (clan_accept failure #2).\n\r", ch);
     }
   if (clan_accept(victim, arg2) == 3)
     {
        send_to_char("That player isn't high enough level. (clan_accept failure #3).\n\r", ch);
     }
   if (clan_accept(victim, arg2) == 4)
     {
        send_to_char("That clan requires PK and that player is not. (clan_accept failure #4).\n\r", ch);
     }
   if (clan_accept(victim, arg2) == 5)
     {
        send_to_char("That player's class isn't accepted in that clan. (clan_accept failure #5).\n\r", ch);
     }
   if (clan_accept(victim, arg2) == 6)
     {
        send_to_char("That player's race isn't accepted in that clan. (clan_accept failure #6).\n\r", ch);
     }
   if (clan_accept(victim, arg2) == 7)
     {
        send_to_char("That player is already a member of that clan!\n\r", ch);
     }
   return;
}


void do_status( CHAR_DATA *ch, char *argument )
{
    extern bool fLogAll;

    if (chaos) {
        send_to_char( "`rC`RH`YA`RO`rS`w is ACTIVE.\n\r", ch );
    } else {
        send_to_char( "`rC`RH`YA`RO`rS`w is NOT ACTIVE.\n\r", ch );
    }

    if (newlock) {
        send_to_char( "New characters are locked out.\n\r", ch );
    } else {
        send_to_char( "New characters are NOT locked out.\n\r", ch );
    }

    if (wizlock) {
        send_to_char( "Only Immortals are allowed.\n\r", ch );
    } else {
        send_to_char( "All players are allowed.\n\r", ch );
    }

    if ( fLogAll ) {
        send_to_char( "All players are being logged.\n\r", ch );
    } else {
        send_to_char( "Log all is OFF.\n\r", ch );
    }

	if ( DEBUG ) {
		send_to_char( "Debug mode is ON.\n\r", ch );
	} else {
		send_to_char( "Debug mode is OFF.\n\r", ch );
	}        

    return;
}

/* Wizslap found on the MUDConnector (http://www.mudconnector.com) by
 * Rahl. I forgot who posted it :(, but
 * it looks kinda neat, so I thought I'd put it in. :)
 * A few minor changes by Rahl.
 */
void do_wizslap( CHAR_DATA *ch, char *argument )
{ 
     char arg[MAX_INPUT_LENGTH]; 
     CHAR_DATA *victim;
     ROOM_INDEX_DATA *pRoomIndex; 
     AFFECT_DATA af;

     one_argument( argument, arg );

     if ( arg[0] == '\0' )
     { 
        send_to_char( "WizSlap whom?\n\r",ch); 
        return; 
     }

     if ( ( victim = get_char_world( ch, arg ) ) == NULL ) 
     {
        send_to_char( "They aren't here.\n\r", ch ); 
        return;
     }

     if ( IS_NPC(victim) ) 
     { 
        send_to_char( "Not on NPC's.\n\r", ch ); 
        return;
     }

     if ( victim == ch )
     {
        send_to_char( "Why would you want to do that?\n\r", ch );
        return;
     }

     if ( victim->level >= ch->level ) 
     {  
        send_to_char( "Yeah, right!\n\r", ch ); 
        act( "$n attempts to slap you!", ch, NULL, victim, TO_VICT );
/*      send_to_char( "If you say so...\n\r", ch ); */
        return; 
     } 

   /*  pRoomIndex = get_random_room(victim); */
    for ( ; ; )
    {
        pRoomIndex = get_room_index( number_range( 0, 65535 ) );
        if ( pRoomIndex != NULL )
        if ( can_see_room(ch,pRoomIndex)
        &&   !IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
        &&   !IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
        &&   !IS_SET(pRoomIndex->room_flags, ROOM_NOTELEPORT) )
            break;
    }

     act( "$n slaps you, sending you reeling through time and space!",
        ch, NULL, victim, TO_VICT); 
     act( "$n slaps $N, sending $M reeling through time and space!",
        ch, NULL, victim, TO_NOTVICT ); 
     act( "You send $N reeling through time and space!", ch, NULL,
        victim, TO_CHAR ); 
     char_from_room( victim ); 
     char_to_room( victim, pRoomIndex );
     act( "$n crashes to the ground!", victim, NULL, NULL, TO_ROOM );      

     af.where = TO_AFFECTS;  
     af.type = skill_lookup("weaken"); 
     af.level = ch->level; 
     af.duration = 5; 
     af.location = APPLY_STR; 
     af.modifier = -1 * (ch->level / 5);
     af.bitvector = AFF_WEAKEN; affect_to_char( victim, &af );
     send_to_char( "You feel your strength slip away.\n\r", victim );
     do_look( victim, "auto" ); 
     
     af.where = TO_AFFECTS; 
     af.type = skill_lookup("curse"); 
     af.level = ch->level;
     af.duration = 5; 
     af.location = APPLY_CON; 
     af.modifier = -1 * (ch->level / 5); 
     af.bitvector = AFF_CURSE; 
     affect_to_char( victim, &af ); 
     return; 
} 
        
/*
 * Bonus command implemented and modified slightly by Rahl.
 * original author unknown
 */
void do_bonus( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    long value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ||
        !is_number( arg3 ) )
    {
        send_to_char( "Syntax: bonus <char> <type> <amount>.\n\r", ch );
        send_to_char( "  Types are: experience, questpoints,"
            " gold, practices, trains\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "That player is not here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        send_to_char( "Not on NPCs.\n\r", ch );
        buffer_free( buf );
        return;
    }
 
    if ( ch == victim )
    {
        send_to_char( "You may not bonus yourself.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_IMMORTAL( victim ) )
    {
        send_to_char( "You cannot bonus immortals.\n\r", ch );
        buffer_free( buf );
        return;
    }

    value = atoi( arg3 );

    if ( value == 0 )
    {
        send_to_char( "What the heck are you thinking?\n\r", ch );
        buffer_free( buf );
        return;
    }
  
    if ( !str_prefix( arg2, "experience" ) || !str_cmp( arg2, "xp" ) )
    {
        if ( value < -10000000 || value > 10000000 )
        {
            send_to_char( "Value range is -10000000 to 10000000.\n\r", ch );
            buffer_free( buf );
            return;
        }

        gain_exp( victim, value );

        if ( value > 0 )
        {
            bprintf( buf, "You grant %s %ld experience points.\n\r",
                victim->name, value );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been granted %ld experience points.\n\r", 
                value );
            send_to_char( buf->data, victim );
        }
        else
        {
            bprintf( buf, "You penalize %s %ld experience points.\n\r",
                victim->name, labs( value ) );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been penalized %ld experience points.\n\r",
                labs( value ) );
            send_to_char( buf->data, victim );
        }
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "questpoints" ) ) 
    {
        if ( value < -1000 || value > 1000 )
        {
            send_to_char( "Value range is -1000 to 1000.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( value > 0 )
        {
            bprintf( buf, "You grant %s %ld quest points.\n\r",
                victim->name, value );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been granted %ld quest points.\n\r",
                value );
            send_to_char( buf->data, victim );
            victim->questpoints += value;
        }
        else
        {
            bprintf( buf, "You penalize %s %ld quest points.\n\r",
                victim->name, labs( value ) );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been penalized %ld quest points.\n\r",
                labs( value ) );
            send_to_char( buf->data, victim );
            victim->questpoints += value;
        }
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "gold" ) ) 
    {
        if ( value < -100000000 || value > 100000000 )
        {
            send_to_char( "Value range is -100000000 to 100000000.\n\r",
                ch );
            buffer_free( buf );
            return;
        }

        if ( value > 0 )
        {
            bprintf( buf, "You grant %s %ld gold.\n\r",
                victim->name, value );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been granted %ld gold.\n\r",
                value );
            send_to_char( buf->data, victim );
            victim->gold += value;
        }
        else
        {
            bprintf( buf, "You penalize %s %ld gold.\n\r",
                victim->name, labs( value ) );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been penalized %ld gold.\n\r",
                labs( value ) );
            send_to_char( buf->data, victim );
            victim->gold += value;
        }
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "practices" ) ) 
    {
        if ( value < -100 || value > 100 )
        {
            send_to_char( "Value range is -100 to 100.\n\r",
                ch );
            buffer_free( buf );
            return;
        }

        if ( value > 0 )
        {
            bprintf( buf, "You grant %s %ld practices.\n\r",
                victim->name, value );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been granted %ld practices.\n\r",
                value );
            send_to_char( buf->data, victim );
            victim->practice += value;
        }
        else
        {
            bprintf( buf, "You penalize %s %ld practices.\n\r",
                victim->name, labs( value ) );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been penalized %ld practices.\n\r",
                labs( value ) );
            send_to_char( buf->data, victim );
            victim->practice += value;
        }
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg2, "trains" ) ) 
    {
        if ( value < -100 || value > 100 )
        {
            send_to_char( "Value range is -100 to 100.\n\r",
                ch );
            buffer_free( buf );
            return;
        }

        if ( value > 0 )
        {
            bprintf( buf, "You grant %s %ld trains.\n\r",
                victim->name, value );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been granted %ld trains.\n\r",
                value );
            send_to_char( buf->data, victim );
            victim->train += value;
        }
        else
        {
            bprintf( buf, "You penalize %s %ld trains.\n\r",
                victim->name, labs( value ) );
            send_to_char( buf->data, ch );
            bprintf( buf, "You have been penalized %ld trains.\n\r",
                labs( value ) );
            send_to_char( buf->data, victim );
            victim->train += value;
        }
        buffer_free( buf );
        return;
    }

    send_to_char( "You can't bonus someone that!\n\r", ch );

    buffer_free( buf );
    return;
}

/* wiznet added by Rahl */
void do_wiznet( CHAR_DATA *ch, char *argument )
{
    int flag;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( argument[0] == '\0' )
    {
        if ( IS_SET(ch->wiznet, WIZ_ON ) )
        {
            send_to_char( "Signing off Wiznet.\n\r", ch );
            REMOVE_BIT( ch->wiznet, WIZ_ON );
        }
        else
        {
            send_to_char( "Welcome to Wiznet!\n\r", ch );
            SET_BIT( ch->wiznet, WIZ_ON );
        }
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( argument, "on" ) )
    {
        send_to_char( "Welcome to Wiznet!\n\r", ch );
        SET_BIT( ch->wiznet, WIZ_ON );
    }

    if ( !str_prefix( argument, "off" ) )
    {
        send_to_char( "Signing off Wiznet.\r\n", ch );
        REMOVE_BIT( ch->wiznet, WIZ_ON );
    }

    /* show wiznet status */
    if ( !str_prefix( argument, "status" ) )
    {
        if ( !IS_SET( ch->wiznet, WIZ_ON ) )
            buffer_strcat( buf, "off " );

        for ( flag = 0; wiznet_table[flag].name != NULL; flag++ )
        {
            if ( IS_SET( ch->wiznet, wiznet_table[flag].flag ) )
            {
                buffer_strcat( buf, wiznet_table[flag].name );
                buffer_strcat( buf, " " );
            }
        }

        buffer_strcat( buf, "\n\r" );

        send_to_char( "Wiznet status:\n\r", ch );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    /* show all the wiznet options */
    if ( !str_prefix( argument, "show" ) )
    {
        for ( flag = 0; wiznet_table[flag].name != NULL; flag++ )
        {
            if ( wiznet_table[flag].level <= char_getImmRank( ch ) )
            {
                buffer_strcat( buf, wiznet_table[flag].name );
                buffer_strcat( buf, " " );
            }
        }
        
        buffer_strcat( buf, "\n\r" );

        send_to_char( "Wiznet options available to you are:\n\r", ch );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    flag = wiznet_lookup( argument );

    if ( flag == -1 || char_getImmRank( ch ) < wiznet_table[flag].level )
    {
        send_to_char( "No such option.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET( ch->wiznet, wiznet_table[flag].flag ) )
    {
        bprintf( buf, "You will no longer see %s on wiznet.\n\r",
                wiznet_table[flag].name );
        send_to_char( buf->data, ch );
        REMOVE_BIT( ch->wiznet, wiznet_table[flag].flag );
        buffer_free( buf );
        return;
    }
    else
    {
        bprintf( buf, "You will now see %s on wiznet.\n\r",
                wiznet_table[flag].name );
        send_to_char( buf->data, ch );
        SET_BIT( ch->wiznet, wiznet_table[flag].flag );
        buffer_free( buf );
        return;
    }
    buffer_free( buf );
    return;
}

/* more wiznet stuff by Rahl */
void wiznet( char *string, CHAR_DATA *ch, OBJ_DATA *obj, long flag, long
            flag_skip, int min_level )
{
    DESCRIPTOR_DATA *d;

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->connected == CON_PLAYING
        && IS_IMMORTAL( d->character )
        && IS_SET( d->character->wiznet, WIZ_ON )
        && ( !flag || IS_SET( d->character->wiznet, flag ) )
        && ( !flag_skip || !IS_SET( d->character->wiznet, flag_skip ) )
        && char_getImmRank( d->character ) >= min_level
        && d->character != ch )
        {
            if ( IS_SET( d->character->wiznet, WIZ_PREFIX ) )
                send_to_char( "--> ", d->character );
            act_new( string, d->character, obj, ch, TO_CHAR, POS_DEAD );
        }
    }
    return;
}

/* incognito added by Rahl */
void do_incognito( CHAR_DATA *ch, char *argument )
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    /* take the default path */
    {
        if ( IS_SET( ch->act, PLR_INCOGNITO ) && ch->incog_level > 0 )
        {
            ch->incog_level = 0;
            REMOVE_BIT( ch->act, PLR_INCOGNITO );
            act( "$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You are no longer cloaked.\n\r", ch );
        }
        else
        {
            ch->incog_level = char_getImmRank( ch );
            SET_BIT( ch->act, PLR_INCOGNITO );
            act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You cloak your presence.\n\r", ch );
        }
    }
    else
    /* do the level thing */
    {
        level = atoi( arg );
        if ( level < 1 || level > char_getImmRank( ch ) )
        {
            send_to_char( "Incog level must be between 2 and your level.\n\r", ch );
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->incog_level = level;
            SET_BIT( ch->act, PLR_INCOGNITO );
            act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You cloak your presence.\n\r", ch );
        }
    }

    return;
}

/*
 * page added by Rahl based on code by Judson Knott
 */
void do_page( CHAR_DATA *ch, char *argument ) {
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( IS_NPC( ch ) ) {
        send_to_char( "NPC's can't page!\n\r", ch );
        buffer_free( buf );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "Syntax: page <victim> <message>\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "You can't page NPCs.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Now why would you want to do that?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !IS_IMMORTAL( victim ) ) {
        bprintf( buf, "You can't page %s.\n\r", victim->name );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    bprintf( buf, "\a`WYou page %s.\n\r`w", victim->name );
    send_to_char( buf->data, ch );
    bprintf( buf, "`RYou tell %s '%s`R'`w\n\r", victim->name, argument );
    send_to_char( buf->data, ch );

    bprintf( buf, "\a`W%s has paged you.\n\r`w", ch->name );
    send_to_char( buf->data, victim );
    bprintf( buf, "`R%s tells you '%s`R'`w\n\r", ch->name, argument );
    send_to_char( buf->data, victim );

    buffer_free( buf );
    return;
}


void do_addapply(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  AFFECT_DATA *paf,*af_new;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  int affect_modify = 0, bit = 0, enchant_type = 0, pos, i;

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  argument = one_argument( argument, arg3 );

  if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
  {
    send_to_char("Syntax for applies: addapply <object> <apply type> <value>\n\r",ch);
    send_to_char("Apply Types: hp str dex int wis con sex mana\n\r", ch);
    send_to_char("             ac move hitroll damroll saves\n\r\n\r", ch);
    send_to_char("Syntax for affects: addapply <object> affect <affect name>\n\r",ch);
    send_to_char("Affect Names: blind invisible detect_evil detect_invis detect_magic\n\r",ch);
    send_to_char("              detect_hidden detect_good sanctuary faerie_fire infrared\n\r",ch);
    send_to_char("              curse poison protect_evil protect_good sneak hide sleep charm\n\r", ch);
    send_to_char("              flying pass_door haste calm plague weaken dark_vision berserk\n\r", ch);
    send_to_char("              swim regeneration slow\n\r", ch);
    send_to_char("Affects availible include the ones you add too!\n\r",
ch);
    return;
  }

  obj = get_obj_world(ch,arg1);

  if (obj == NULL)
  {
    send_to_char("No such object exists!\n\r",ch);
    return;
  }

       if (!str_prefix(arg2,"hp"))
        enchant_type=APPLY_HIT;
  else if (!str_prefix(arg2,"str"))
        enchant_type=APPLY_STR;
  else if (!str_prefix(arg2,"dex"))
        enchant_type=APPLY_DEX;
  else if (!str_prefix(arg2,"int"))
        enchant_type=APPLY_INT;
  else if (!str_prefix(arg2,"wis"))
        enchant_type=APPLY_WIS;
  else if (!str_prefix(arg2,"con"))
        enchant_type=APPLY_CON;
/*  else if (!str_prefix(arg2,"sex"))
        enchant_type=APPLY_SEX;
*/
  else if (!str_prefix(arg2,"mana"))
        enchant_type=APPLY_MANA;
  else if (!str_prefix(arg2,"move"))
        enchant_type=APPLY_MOVE;
  else if (!str_prefix(arg2,"ac"))
        enchant_type=APPLY_AC;
  else if (!str_prefix(arg2,"hitroll"))
        enchant_type=APPLY_HITROLL;
  else if (!str_prefix(arg2,"damroll"))
        enchant_type=APPLY_DAMROLL;
  else if (!str_prefix(arg2,"saves"))
        enchant_type=APPLY_SAVING_SPELL;
  else if (!str_prefix(arg2,"affect"))
        enchant_type=APPLY_SPELL_AFFECT;
  else
  {
    send_to_char("That apply is not possible!\n\r",ch);
    return;
  }

  if (enchant_type==APPLY_SPELL_AFFECT)
  {
    for (pos = 0; !str_cmp(affect_flags[pos].name, "" ); pos++)
        if (!str_cmp(arg3,affect_flags[pos].name))
            bit = affect_flags[pos].bit;
    for ( pos = 0; !str_cmp( affect_flags[pos].name, "" ); pos++ )
        if ( !str_cmp(arg3, affect2_flags[pos].name ) )
            bit = affect2_flags[pos].bit;
  }
  else
  {
    if ( is_number(arg3) )
        affect_modify=atoi(arg3);       
    else
    {
        send_to_char("Applies require a value.\n\r", ch);
        return;
    }
  }

    if (!obj->enchanted)
    {
      obj->enchanted = TRUE;

      for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
      {
        if (affect_free == NULL)
        af_new = alloc_perm(sizeof(*af_new));
        else
        {
          af_new = affect_free;
          affect_free = affect_free->next;
        }

      af_new->next = obj->affected;
      obj->affected = af_new;
      af_new->type        = UMAX(0,paf->type);
      af_new->level       = paf->level;
      af_new->duration    = paf->duration;
      af_new->location    = paf->location;
      af_new->modifier    = paf->modifier;
      af_new->bitvector   = paf->bitvector;
      }
    }

  if (affect_free == NULL)
    paf = alloc_perm(sizeof(*paf));
  else
  {
    paf = affect_free;
    affect_free = affect_free->next;
  }

        paf->type       = -1;
        paf->level      = ch->level;
        paf->duration   = -1;
        paf->location   = enchant_type;
        paf->modifier   = affect_modify;
        paf->bitvector  = bit;

        if (enchant_type==APPLY_SPELL_AFFECT)
        {
            /* Quick hack to make table compatible with skill_lookup */

            for ( i=0 ; arg3[i] != '\0'; i++ )
            {
                if ( arg3[i] == '_' )
                    arg3[i] = ' ';
            }

            paf->type           = skill_lookup(arg3);
            paf->where          = TO_AFFECTS;
            paf->modifier       = 0;    
        }

        paf->next       = obj->affected;
        obj->affected   = paf;

        send_to_char("Ok.\n\r", ch);
}

/* 
 * grab from somewhere. Dunno who the author is
 *   -- Rahl
 */
void do_grab (CHAR_DATA *ch, char *argument)
{
    CHAR_DATA  *victim;
    OBJ_DATA   *obj;
    char        arg1 [ MAX_INPUT_LENGTH ];
    char        arg2 [ MAX_INPUT_LENGTH ];
    char        arg3 [ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( ( arg1[0] == '\0' ) || ( arg2[0] == '\0' ) )
    {
        send_to_char( "Syntax : grab <object> <player>\n\r", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, arg2 ) ) )
    {
        send_to_char( "They are not here!\n\r", ch );
        return;
    }

    if ( !( obj = get_obj_list( ch, arg1, victim->carrying ) ) )
    {
        send_to_char( "They do not have that item.\n\r", ch );
        return;
    }

        /* was ch->level -- Rahl */
    if ( char_getImmRank( victim ) >= char_getImmRank( ch ) )
    {
        send_to_char( "I wouldn't try that if I were you...\r\n", ch );
        return;
    }

    if ( obj->wear_loc != WEAR_NONE )
        unequip_char( victim, obj );

    obj_from_char( obj );
    obj_to_char( obj, ch );

    act( "You grab $p from $N.", ch, obj, victim, TO_CHAR );
    if ( arg3[0] == '\0' 
        || !str_cmp( arg3, "yes" ) || !str_cmp( arg3, "true" ) )                
           act( "You no longer own $p.", ch, obj, victim, TO_VICT );

    return;
}


/* 
 * owhere added by Rahl from Rom2.4b4
 */
void do_owhere(CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 200;

    if (argument[0] == '\0')
    {
        send_to_char("Find what?\n\r",ch);
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( argument, obj->name )
        ||   ch->level < obj->level)
            continue;

        found = TRUE;
        number++;

        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;

        if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by)
        &&   in_obj->carried_by->in_room != NULL)
            bprintf( buf, "%3d) %s is carried by %s [Room %d]\n\r",
                number, obj->short_descr,PERS(in_obj->carried_by, ch),
                in_obj->carried_by->in_room->vnum );
        else if (in_obj->in_room != NULL && can_see_room(ch,in_obj->in_room))
            bprintf( buf, "%3d) %s is in %s [Room %d]\n\r",
                number, obj->short_descr,in_obj->in_room->name,
                in_obj->in_room->vnum);
        else
            bprintf( buf, "%3d) %s is somewhere\n\r", number,
                obj->short_descr);

        buf->data[0] = UPPER(buf->data[0]);
        buffer_strcat( buffer, buf->data );

        if (number >= max_found)
            break;
    }

    if ( !found )
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
    else
        page_to_char( buffer->data,ch);

    buffer_free( buffer );
    buffer_free( buf );
}

/*
 * add ROOM_VNUM_JAIL to merc.h - written by Rahl
 */
void do_jail( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *jail;
    ROOM_INDEX_DATA *temple;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    jail = get_room_index( ROOM_VNUM_JAIL );
    temple = get_room_index( ROOM_VNUM_TEMPLE );

    if ( argument[0] == '\0' )
    {
        send_to_char( "Jail whom?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        send_to_char( "You can't jail NPCs.\n\r", ch );
        buffer_free( buf );
        return;
    }
     
    if ( ch == victim )
    {
        send_to_char( "Why would you want to do that?\n\r", ch );
        buffer_free( buf );
        return;
    }
       
    if ( char_getImmRank( ch ) < char_getImmRank( victim ) )
    {
        send_to_char( "That's NOT a good idea.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET( victim->act, PLR_FREEZE ) && victim->in_room == jail )
    {
        do_freeze( ch, victim->name );
        char_from_room( victim );
        char_to_room( victim, temple );
        if ( victim->pet != NULL )
            char_to_room( victim->pet, temple );
        send_to_char( "You've been released from jail.\n\r", victim );
        bprintf( buf, "%s has been released from jail.\n\r", victim->name );
        wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    do_freeze( ch, victim->name );
    char_from_room( victim );
    char_to_room( victim, jail );
    if ( victim->pet != NULL )
        char_to_room( victim->pet, jail );
    send_to_char( "You've been JAILED!\n\r", victim );
    bprintf( buf, "%s has been jailed.\n\r", victim->name );
    wiznet( buf->data, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0 );
    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}

void do_states( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf2 = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf3 = buffer_new( 100 );
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    int count;

    count       = 0;

    one_argument(argument,arg);
    
    bprintf( buf, "Connected State               Name\n\r" );
    buffer_strcat( buf, "----------------------------------\n\r" );

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->character != NULL && can_see( ch, d->character ) 
        && (arg[0] == '\0' || is_name(arg,d->character->name)
                           || (d->original && is_name(arg,d->original->name))))
        {
            count++;
            switch( d->connected )
            {
                default:
                    bprintf( buf3, "ERROR" );
                    break;
                case CON_PLAYING:
                    bprintf( buf3, "Playing" );
                    break;
                case CON_GET_NAME:
                    bprintf( buf3, "Get Name" );
                    break;
                case CON_GET_OLD_PASSWORD:
                    bprintf( buf3, "Get Old Password" );
                    break;
                case CON_CONFIRM_NEW_NAME:
                    bprintf( buf3, "Confirm New Name" );
                    break;
                case CON_GET_NEW_PASSWORD:
                    bprintf( buf3, "Get New Password" );
                    break;
                case CON_CONFIRM_NEW_PASSWORD:
                    bprintf( buf3, "Confirm New Password" );
                    break;
                case CON_GET_NEW_RACE:
                    bprintf( buf3, "Get New Race" );
                    break;
                case CON_GET_NEW_SEX:
                    bprintf( buf3, "Get New Sex" );
                    break;
                case CON_GET_NEW_CLASS:
                    bprintf( buf3, "Get New Class" );
                    break;
                case CON_GET_ALIGNMENT:
                    bprintf( buf3, "Get Alignment" );
                    break;
                case CON_DEFAULT_CHOICE:
                    bprintf( buf3, "Default Skill Choice" );
                    break;
                case CON_GEN_GROUPS:
                    bprintf( buf3, "Picking Skills" );
                    break;
                case CON_PICK_WEAPON:
                    bprintf( buf3, "Pick Weapon" );
                    break;
                case CON_READ_IMOTD:
                    bprintf( buf3, "Read IMOTD" );
                    break;
                case CON_READ_MOTD:
                    bprintf( buf3, "Read MOTD" );
                    break;
                case CON_BREAK_CONNECT:
                    bprintf( buf3, "Linkdead" );
                    break;
                case CON_GET_STATS:
                    bprintf( buf3, "Get Stats" );
                    break;
                case CON_NOTE_TO:
                    bprintf( buf3, "Note To" );
                    break;
                case CON_NOTE_SUBJECT:
                    bprintf( buf3, "Note Subject" );
                    break;
                case CON_NOTE_EXPIRE:
                    bprintf( buf3, "Note Expire" );
                    break;
                case CON_NOTE_TEXT:
                    bprintf( buf3, "Note Text" );
                    break;
                case CON_NOTE_FINISH:
                    bprintf( buf3, "Note Finish" );
                    break;
                case CON_COPYOVER_RECOVER:
                    bprintf( buf3, "Copyover Recover" );
                    break;
                case CON_ANSI:
                    bprintf( buf3, "ANSI Color Prompt" );
                    break;
            }
            bprintf( buf2, "[%2d -- %-20s]  %-12s\n\r",
                d->connected,
                buf3->data,
                d->original  ? d->original->name  :
                d->character ? d->character->name : "(none)"
                );
            buffer_strcat( buf, buf2->data );
        }
    }
    if (count == 0)
    {
        send_to_char("No one by that name is connected.\n\r",ch);
        buffer_free( buf );
        buffer_free( buf2 );
        buffer_free( buf3 );
        return;
    }
    
    bprintf( buf2, "%d user%s\n\r", count, count == 1 ? "" : "s" );
    buffer_strcat( buf, buf2->data );
    page_to_char( buf->data, ch );
    buffer_free( buf );
    buffer_free( buf2 );
    buffer_free( buf3 );
    return;
}


void do_protect( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    if (argument[0] == '\0')
    {
        send_to_char("Protect whom from snooping?\n\r",ch);
        return;
    }

    if ((victim = get_char_world(ch,argument)) == NULL)
    {
        send_to_char("You can't find them.\n\r",ch);
        return;
    }

    if (IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
        act_new("$N is no longer snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
        send_to_char("Your snoop-proofing was just removed.\n\r",victim);
        REMOVE_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
    else
    {
        act_new("$N is now snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
        send_to_char("You are now immune to snooping.\n\r",victim);
        SET_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
}

void do_violate( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Goto where?\n\r", ch );
        return;
    }

    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }

    if (!room_is_private( location ))
    {
        send_to_char( "That room isn't private, use goto.\n\r", ch );
        return;
    }

    if ( ch->fighting != NULL )
        stop_fighting( ch, TRUE );

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (char_getImmRank(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
            else
                act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    char_from_room( ch );
    char_to_room( ch, location );


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (char_getImmRank(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    do_look( ch, "auto" );
    return;
}

void do_smote(CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }

    if (strstr(argument,ch->name) == NULL)
    {
        send_to_char("You must include your name in an smote.\n\r",ch);
        return;
    }

    send_to_char(argument,ch);
    send_to_char("\n\r",ch);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr(argument,vch->name)) == NULL)
        {
            send_to_char(argument,vch);
            send_to_char("\n\r",vch);
            continue;
        }

        strcpy(temp,argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = vch->name;

        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen(vch->name))
            {
                strcat(temp,"r");
                continue;
            }

            if (*letter == 's' && matches == strlen(vch->name))
            {
                matches = 0;
                continue;
            }

            if (matches == strlen(vch->name))
            {
                matches = 0;
            }

            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen(vch->name))
                {
                    strcat(temp,"you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat(last,letter,1);
                continue;
            }

            matches = 0;
            strcat(temp,last);
            strncat(temp,letter,1);
            strncat(temp,letter,1);
            last[0] = '\0';
            name = vch->name;
        }

        send_to_char(temp,vch);
        send_to_char("\n\r",vch);
    }

    return;
}

void do_zecho(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char("Zone echo what?\n\r",ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING
        &&  d->character->in_room != NULL && ch->in_room != NULL
        &&  d->character->in_room->area == ch->in_room->area)
        {
            if (char_getImmRank(d->character) >= char_getImmRank(ch))
                send_to_char("zone> ",d->character);
            send_to_char(argument,d->character);
            send_to_char("\n\r",d->character);
        }
    }
}

/* prefix command: it will put the string typed on each line typed */

void do_prefi (CHAR_DATA *ch, char *argument)
{
    send_to_char("You cannot abbreviate the prefix command.\r\n",ch);
    return;
}

void do_prefix (CHAR_DATA *ch, char *argument)
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( IS_NPC( ch ) )
    {
        send_to_char( "You'll have to type it all out. :(\n\r", ch );
        buffer_free( buf );
        return;
    }

    if (argument[0] == '\0')
    {
        if (ch->prefix[0] == '\0')
        {
            send_to_char("You have no prefix to clear.\r\n",ch);
            buffer_free( buf );
            return;
        }

        send_to_char("Prefix removed.\r\n",ch);
        free_string(ch->prefix);
        ch->prefix = str_dup("");
        buffer_free( buf );
        return;
    }

    if (ch->prefix[0] != '\0')
    {
        bprintf(buf,"Prefix changed to %s.\r\n",argument);
        free_string(ch->prefix);
    }
    else
    {
        bprintf(buf,"Prefix set to %s.\r\n",argument);
    }

    send_to_char( buf->data, ch );
    ch->prefix = str_dup(argument);

    buffer_free( buf );
    return;
}

/* written by Rahl */
void do_permit( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool old;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax: permit [player]\n\r", ch );
        buffer_free( buf );
        return;
    }
    else if ( ( victim = get_char_world( ch, argument ) ) != NULL )
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Umm.. why?\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( IS_SET( victim->act, PLR_PERMIT ) )
        {
            REMOVE_BIT( victim->act, PLR_PERMIT );
            bprintf( buf, "%s is no longer permitted to play around bans.\n\r", 
                victim->name );
            send_to_char( buf->data, ch );
        }
        else
        {
            SET_BIT( victim->act, PLR_PERMIT );
            bprintf( buf, "%s is now permitted to play around bans.\n\r", 
                victim->name );
            send_to_char( buf->data, ch );
        }
        buffer_free( buf );
        return;
    }
    else
    {       
        d = alloc_perm (sizeof(DESCRIPTOR_DATA)); 
        old = load_char_obj( d, argument );
        if ( !old )
        {
            send_to_char( "No such character.\n\r", ch );
            buffer_free( buf );
            return;
        }
        victim = d->character;

        if ( IS_SET( victim->act, PLR_PERMIT ) )
        {
            REMOVE_BIT( victim->act, PLR_PERMIT );
            bprintf( buf, "%s is no longer permitted to play around bans.\n\r", 
                victim->name );
            send_to_char( buf->data, ch );
        }
        else
        {
            SET_BIT( victim->act, PLR_PERMIT );
            bprintf( buf, "%s is now permitted to play around bans.\n\r", 
                victim->name );
            send_to_char( buf->data, ch );
        }

        save_char_obj( victim );
        free_char( victim );
        d = descriptor_free;
        buffer_free( buf );
        return;
    }
    send_to_char( "No such character.\n\r", ch );
    buffer_free( buf );
    return;
}

void do_debug( CHAR_DATA *ch, char *argument ) {
	if ( DEBUG ) {
		DEBUG = FALSE;
		send_to_char( "Debug mode OFF.\n\r", ch );
		log_string( " ***** Debug mode OFF. ***** " );
	} else {
		DEBUG = TRUE;
		send_to_char( "Debug mode ON.\n\r", ch );
		log_string( " ***** Debug mode ON. ***** " );
	}
}

int char_getImmRank( CHAR_DATA *ch ) {
	if ( ch == NULL ) {
		return 0;
	}

	if ( ( ch->immRank < 0 ) || ( ch->immRank > 10 ) ) {
		ch->immRank = 0;
	}

	return ch->immRank;
}

void char_setImmRank( CHAR_DATA *ch, int newRank ) {
	if ( ch == NULL ) {
		return;
	}

	if ( ( newRank < 0 ) || ( newRank > 10 ) ) {
		newRank = 0;
	}
	
	ch->immRank = newRank;

	return;
}
