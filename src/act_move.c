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
#include <form.h>
#include "merc.h"
#include "interp.h"

char *  const   dir_name        []              =
{
    "north", "east", "south", "west", "up", "down"
};

const   sh_int  rev_dir         []              =
{
    2, 3, 0, 1, 5, 4
};

const   sh_int  movement_loss   [SECT_MAX]      =
{
    1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6, 4
};



/*
 * Local functions.
 */
int     find_door       ( CHAR_DATA *ch, char *arg );
bool    char_has_key    ( CHAR_DATA *ch, int key ); 
bool    check_web       ( CHAR_DATA *ch );


void move_char( CHAR_DATA *ch, int door, bool follow )
{
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    BUFFER *buf = buffer_new( 200 );
    int chance;

    if ( door < 0 || door > 5 )
    {
        bug( "Do_move: bad door %d.", door );
        buffer_free( buf );
        return;
    }

    if (IS_AFFECTED(ch, AFF_WEB)) 
        {
        if (check_web(ch))
            {
             act( "You break free of the webs and fall down from the effort.", ch, NULL, NULL, TO_CHAR );
             act( "$n breaks free of the webs and falls downs from the effort.", ch, NULL,NULL,TO_ROOM);
             WAIT_STATE( ch, 5 );
             ch->move -= 5;
             affect_strip(ch,skill_lookup("web"));
             REMOVE_BIT( ch->affected_by, AFF_WEB);
             ch->position = POS_RESTING;
             buffer_free( buf );
             return;
            }
        else
            {
             act( "You struggle against the webs.", ch, NULL, NULL, TO_CHAR );
             act( "$n struggles weakly in the webs.", ch, NULL,NULL,TO_ROOM);
             WAIT_STATE( ch, 5 );
             ch->move -= 5;
             buffer_free( buf );
             return;
            }
        /* The end? */
        }

    in_room = ch->in_room;
    if ( ( pexit   = in_room->exit[door] ) == NULL
    ||   ( to_room = pexit->u1.to_room   ) == NULL 
    ||   !can_see_room(ch,pexit->u1.to_room))
    {
        send_to_char( "Alas, you cannot go that way.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET(pexit->exit_info, EX_CLOSED)
        && IS_SET(pexit->exit_info, EX_HIDDEN))
        {
        send_to_char( "Alas, you cannot go that way.\n\r", ch );
        buffer_free( buf );
        return;
        }

    if ( IS_SET(pexit->exit_info, EX_CLOSED)
    &&   !IS_AFFECTED(ch, AFF_PASS_DOOR) 
    /* below line allows imms to walk through doors */
    && !IS_IMMORTAL( ch ) )
    {
        act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
        act( "$n tries unsuccessfully to walk through the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        buffer_free( buf );
        return;
    }

    if ( IS_SET(pexit->exit_info, EX_CLOSED)
        && IS_SET(pexit->exit_info, EX_PASSPROOF)
        && IS_AFFECTED(ch, AFF_PASS_DOOR) )

        {
        act( "A magical force prevents you from passing through the $d.", ch, NULL, pexit->keyword, TO_CHAR );
        act( "$n looks puzzled by not passing through the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        buffer_free( buf );
        return;
        }

    if ( IS_AFFECTED(ch, AFF_CHARM)
    &&   ch->master != NULL
    &&   in_room == ch->master->in_room )
    {
        send_to_char( "What?  And leave your beloved master?\n\r", ch );
        buffer_free( buf );
        return;
    }

    /* caved in rooms  - added by Rahl */
    if ( to_room->status >= 1 )
    {
        send_to_char( "A wall of stone prevents you from entering that room at this time.\n\r",
            ch );
        buffer_free( buf );
        return;
    }

    if ( chaos && IS_SET( to_room->room_flags, ROOM_SAFE ) &&
        !IS_IMMORTAL( ch ) )
    {
        send_to_char( "You can't enter that room while `rC`RH`YA`RO`rS`w"
            " is active.\n\r", ch );
        buffer_free( buf );
        return;
    } 

    if ( room_is_private( to_room ) )
    {
        send_to_char( "That room is private right now.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !IS_NPC(ch) )
    {
        int iClass, iGuild;
        int move;

        for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        {
            for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)    
            {
                if ( iClass != ch->ch_class
                &&   to_room->vnum == class_table[iClass].guild[iGuild] )
                {
                    send_to_char( "You aren't allowed in there.\n\r", ch );
                    buffer_free( buf );
                    return;
                }
            }
        }

        /* caved in rooms - the cavein part  - added by Rahl */
        if ( to_room->sector_type == SECT_UNDERGROUND 
        || IS_SET( to_room->room_flags, ROOM_UNDERGROUND ) )
        {
            chance = 15;

            if ( number_percent( ) < chance )
            {
                to_room->status += 1;
                send_to_char( "Darkness surrounds you as the walls cave in.\n\r",
                    ch );
                char_from_room( ch );
                char_to_room( ch, get_room_index( ROOM_VNUM_ALTAR ) );
                do_look( ch, "" );
                ch->hit -= ( ch->hit -1 );
                buffer_free( buf );
                return;
            }
        }

        if ( in_room->sector_type == SECT_AIR
        ||   to_room->sector_type == SECT_AIR )
        {
            if ( !IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
            {
                send_to_char( "You can't fly.\n\r", ch );
                buffer_free( buf );
                return;
            }
        }

        if (( in_room->sector_type == SECT_WATER_NOSWIM
        ||    to_room->sector_type == SECT_WATER_NOSWIM )
        &&    !IS_AFFECTED(ch,AFF_FLYING))
        {
            OBJ_DATA *obj;
            bool found;

            /*
             * Look for a boat.
             */
            found = FALSE;

            if (IS_IMMORTAL(ch))
                found = TRUE;

            for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
            {
                if ( obj->item_type == ITEM_BOAT )
                {
                    found = TRUE;
                    break;
                }
            }
            if ( !found )
            {
                send_to_char( "You need a boat to go there.\n\r", ch );
                buffer_free( buf );
                return;
            }
        }

        move = movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)]
             + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)]
             ;

        move /= 2;  /* i.e. the average */

        /* conditional effects - added by Rahl */
        if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_HASTE ) )
            move /= 2;

        if ( IS_AFFECTED( ch, AFF_SLOW ) )
           move *= 2;

        if ( ch->move < move )
        {
            send_to_char( "You are too exhausted.\n\r", ch );
            buffer_free( buf );
            return;
        }

        WAIT_STATE( ch, 1 );
        ch->move -= move;
    }

    if ( !IS_AFFECTED(ch, AFF_SNEAK)
    && ( IS_NPC(ch) || !IS_SET(ch->act, PLR_WIZINVIS) ) )
        act( "$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM );


    char_from_room( ch );
    char_to_room( ch, to_room );
    if ( !IS_AFFECTED(ch, AFF_SNEAK)
    && ( IS_NPC(ch) || !IS_SET(ch->act, PLR_WIZINVIS) ) )
        act( "$n has arrived.", ch, NULL, NULL, TO_ROOM );

    do_look( ch, "auto" );

    if (in_room == to_room) /* no circular follows */
    {
        buffer_free( buf );
        return;
    }

    for ( fch = in_room->people; fch != NULL; fch = fch_next )
    {
        fch_next = fch->next_in_room;

        if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) 
        &&   fch->position < POS_STANDING)
            do_stand(fch,"");

        if ( fch->master == ch && fch->position == POS_STANDING )
        {

            if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
            &&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE)))
            {
                act("You can't bring $N into the city.",
                    ch,NULL,fch,TO_CHAR);
                act("You aren't allowed in the city.",
                    fch,NULL,NULL,TO_CHAR);
                buffer_free( buf );
                return;
            }

            act( "You follow $N.", fch, NULL, ch, TO_CHAR );
            move_char( fch, door, TRUE );
        }
    }
    buffer_free( buf );
    return;
}



void do_north( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_NORTH, FALSE );
    return;
}



void do_east( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_EAST, FALSE );
    return;
}



void do_south( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_SOUTH, FALSE );
    return;
}



void do_west( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_WEST, FALSE );
    return;
}



void do_up( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_UP, FALSE );
    return;
}



void do_down( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_DOWN, FALSE );
    return;
}



int find_door( CHAR_DATA *ch, char *arg )
{
    EXIT_DATA *pexit;
    int door;

         if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"  ) ) door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"  ) ) door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"    ) ) door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"  ) ) door = 5;
    else
    {
        for ( door = 0; door <= 5; door++ )
        {
            if ( ( pexit = ch->in_room->exit[door] ) != NULL
            &&   IS_SET(pexit->exit_info, EX_ISDOOR)
            &&   pexit->keyword != NULL
            &&   is_name( arg, pexit->keyword ) )
                return door;
        }
        act( "I see no $T here.", ch, NULL, arg, TO_CHAR );
        return -1;
    }

    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
        act( "I see no door $T here.", ch, NULL, arg, TO_CHAR );
        return -1;
    }

    if ( !IS_SET(pexit->exit_info, EX_ISDOOR) )
    {
        send_to_char( "You can't do that.\n\r", ch );
        return -1;
    }

    return door;
}



void do_open( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Open what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* open portal */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1], EX_ISDOOR))
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1], EX_CLOSED))
            {
                send_to_char("It's already open.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1], EX_LOCKED))
            {
                send_to_char("It's locked.\n\r",ch);
                return;
            }

            REMOVE_BIT(obj->value[1], EX_CLOSED);
            act("You open $p.",ch,obj,NULL,TO_CHAR);
            act("$n opens $p.",ch,obj,NULL,TO_ROOM);
            return;
        }

        /* 'open object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's already open.\n\r",      ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
            { send_to_char( "You can't do that.\n\r",      ch ); return; }
        if ( IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's locked.\n\r",            ch ); return; }

        REMOVE_BIT(obj->value[1], CONT_CLOSED);
        send_to_char( "Ok.\n\r", ch );
        act( "$n opens $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'open door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
            { send_to_char( "It's already open.\n\r",      ch ); return; }
        if (  IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's locked.\n\r",            ch ); return; }

        REMOVE_BIT(pexit->exit_info, EX_CLOSED);
        act( "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        send_to_char( "Ok.\n\r", ch );

        /* open the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            CHAR_DATA *rch;

            REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
            for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
                act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
        }
    }

    return;
}



void do_close( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Close what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {

            if (!IS_SET(obj->value[1],EX_ISDOOR) )
          /*  ||   IS_SET(obj->value[1],EX_NOCLOSE)) */
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's already closed.\n\r",ch);
                return;
            }

            SET_BIT(obj->value[1],EX_CLOSED);
            act("You close $p.",ch,obj,NULL,TO_CHAR);
            act("$n closes $p.",ch,obj,NULL,TO_ROOM);
            return;
        }

        /* 'close object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's already closed.\n\r",    ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
            { send_to_char( "You can't do that.\n\r",      ch ); return; }

        SET_BIT(obj->value[1], CONT_CLOSED);
        send_to_char( "Ok.\n\r", ch );
        act( "$n closes $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'close door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit   = ch->in_room->exit[door];
        if ( IS_SET(pexit->exit_info, EX_CLOSED) )
            { send_to_char( "It's already closed.\n\r",    ch ); return; }

        SET_BIT(pexit->exit_info, EX_CLOSED);
        act( "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        send_to_char( "Ok.\n\r", ch );

        /* close the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            CHAR_DATA *rch;

            SET_BIT( pexit_rev->exit_info, EX_CLOSED );
            for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
                act( "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
        }
    }

    return;
}



bool char_has_key( CHAR_DATA *ch, int key )
{
    OBJ_DATA *obj;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( IS_IMMORTAL( ch ) )
            return TRUE;

        if ( !can_see_obj( ch, obj ) )
        {
            int chance = 0;
        
            chance = number_percent();

            if ( chance < 20 ) /* 20% chance they can open it */
                return TRUE;
            else
                return FALSE;
        }

        if ( obj->pIndexData->vnum == key )
            return TRUE;
    }

    return FALSE;
}



void do_lock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Lock what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1],EX_ISDOOR ) )
          /*  ||  IS_SET(obj->value[1],EX_NOCLOSE)) */
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }
            if (!IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's not closed.\n\r",ch);
                return;
            }

            if (obj->value[4] < 0 )
         /*   || IS_SET(obj->value[1],EX_NOLOCK))   */
            {
                send_to_char("It can't be locked.\n\r",ch);
                return;
            }

            if (!char_has_key(ch,obj->value[4]))
            {
                send_to_char("You lack the key.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1],EX_LOCKED))
            {
                send_to_char("It's already locked.\n\r",ch);
                return;
            }

            SET_BIT(obj->value[1],EX_LOCKED);
            act("You lock $p.",ch,obj,NULL,TO_CHAR);
            act("$n locks $p.",ch,obj,NULL,TO_ROOM);
            return;
        }


        /* 'lock object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( obj->value[2] < 0 )
            { send_to_char( "It can't be locked.\n\r",     ch ); return; }
        if ( !char_has_key( ch, obj->value[2] ) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's already locked.\n\r",    ch ); return; }

        SET_BIT(obj->value[1], CONT_LOCKED);
        send_to_char( "*Click*\n\r", ch );
        act( "$n locks $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'lock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit   = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( pexit->key < 0 )
            { send_to_char( "It can't be locked.\n\r",     ch ); return; }
        if ( !char_has_key( ch, pexit->key) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already locked.\n\r",    ch ); return; }

        SET_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char( "*Click*\n\r", ch );
        act( "$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

        /* lock the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            SET_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}



void do_unlock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Unlock what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (IS_SET(obj->value[1],EX_ISDOOR))
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's not closed.\n\r",ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                send_to_char("It can't be unlocked.\n\r",ch);
                return;
            }

            if (!char_has_key(ch,obj->value[4]))
            {
                send_to_char("You lack the key.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1],EX_LOCKED))
            {
                send_to_char("It's already unlocked.\n\r",ch);
                return;
            }

            REMOVE_BIT(obj->value[1],EX_LOCKED);
            act("You unlock $p.",ch,obj,NULL,TO_CHAR);
            act("$n unlocks $p.",ch,obj,NULL,TO_ROOM);
            return;
        }

        
        /* 'unlock object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( obj->value[2] < 0 )
            { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
        if ( !char_has_key( ch, obj->value[2] ) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        send_to_char( "*Click*\n\r", ch );
        act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'unlock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( pexit->key < 0 )
            { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
        if ( !char_has_key( ch, pexit->key) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char( "*Click*\n\r", ch );
        act( "$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

        /* unlock the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}



void do_pick( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Pick what?\n\r", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );

    if ( IS_AFFECTED( ch, AFF_BLIND ) )
    {
        int chance = number_percent();

        if ( chance > 33 )
        {
            send_to_char( "You can't see what you're working on.\n\r", ch );
            return;
        }
    }

    /* look for guards */
    for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
    {
        if ( IS_NPC(gch) && IS_AWAKE(gch) )
        {
            act( "$N is standing too close to the lock.",
                ch, NULL, gch, TO_CHAR );
            return;
        }
    }

    if ( !IS_NPC(ch) && number_percent( ) > ch->pcdata->learned[gsn_pick_lock] )
    {
        send_to_char( "You failed.\n\r", ch);
        check_improve(ch,gsn_pick_lock,FALSE,2);
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1],EX_ISDOOR))
            {   
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's not closed.\n\r",ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                send_to_char("It can't be unlocked.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1],EX_PICKPROOF))
            {
                send_to_char("You failed.\n\r",ch);
                return;
            }

            REMOVE_BIT(obj->value[1],EX_LOCKED);
            act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
            act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
            check_improve(ch,gsn_pick_lock,TRUE,2);
            return;
        }


        /* 'pick object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( obj->value[2] < 0 )
            { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
        if ( IS_SET(obj->value[1], CONT_PICKPROOF) )
            { send_to_char( "You failed.\n\r",             ch ); return; }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        send_to_char( "*Click*\n\r", ch );
        check_improve(ch,gsn_pick_lock,TRUE,2);
        act( "$n picks $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'pick door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( pexit->key < 0 && !IS_IMMORTAL(ch))
            { send_to_char( "It can't be picked.\n\r",     ch ); return; }
        if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
        if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
            { send_to_char( "You failed.\n\r",             ch ); return; }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char( "*Click*\n\r", ch );
        act( "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        check_improve(ch,gsn_pick_lock,TRUE,2);

        /* pick the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}




void do_stand( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( 200 );

    switch ( ch->position )
    {
    case POS_SLEEPING:
        if ( IS_AFFECTED(ch, AFF_SLEEP) )
        { 
            send_to_char( "You can't wake up!\n\r", ch ); 
            buffer_free( buf );
            return; 
        }

        send_to_char( "You wake and stand up.\n\r", ch );
        act( "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
        if ( !IS_NPC( ch ) && ch->pcdata->message_ctr )
        {
            bprintf( buf, "You have %d messages waiting.\n\r",
                ch->pcdata->message_ctr );
            send_to_char( buf->data, ch );
        }
        ch->position = POS_STANDING;
        break;

    case POS_RESTING: case POS_SITTING:
        send_to_char( "You stand up.\n\r", ch );
        act( "$n stands up.", ch, NULL, NULL, TO_ROOM );
        ch->position = POS_STANDING;
        break;

    case POS_STANDING:
        send_to_char( "You are already standing.\n\r", ch );
        break;

    case POS_FIGHTING:
        send_to_char( "You are already fighting!\n\r", ch );
        break;
    }

    buffer_free( buf );
    return;
}



void do_rest( CHAR_DATA *ch, char *argument )
{
    switch ( ch->position )
    {
    case POS_SLEEPING:
        if ( IS_AFFECTED(ch,AFF_SLEEP)) 
        { 
            send_to_char("You can't wake up!\n\r",ch); 
            return; 
        }
        send_to_char( "You wake up and start resting.\n\r", ch );
        act ("$n wakes up and starts resting.",ch,NULL,NULL,TO_ROOM);
        ch->position = POS_RESTING;
        break;

    case POS_RESTING:
        send_to_char( "You are already resting.\n\r", ch );
        break;

    case POS_STANDING:
        send_to_char( "You rest.\n\r", ch );
        act( "$n sits down and rests.", ch, NULL, NULL, TO_ROOM );
        ch->position = POS_RESTING;
        break;

    case POS_SITTING:
        send_to_char("You rest.\n\r",ch);
        act("$n rests.",ch,NULL,NULL,TO_ROOM);
        ch->position = POS_RESTING;
        break;

    case POS_FIGHTING:
        send_to_char( "You are already fighting!\n\r", ch );
        break;
    }


    return;
}


void do_sit (CHAR_DATA *ch, char *argument )
{
    switch (ch->position)
    {
        case POS_SLEEPING:
            if ( IS_AFFECTED(ch,AFF_SLEEP)) 
            { 
                send_to_char("You can't wake up!\n\r",ch); 
                return; 
            }
            send_to_char("You wake up.\n\r",ch);
            act("$n wakes and sits up.",ch,NULL,NULL,TO_ROOM);
            ch->position = POS_SITTING;
            break;
        case POS_RESTING:
            send_to_char("You stop resting.\n\r",ch);
            ch->position = POS_SITTING;
            break;
        case POS_SITTING:
            send_to_char("You are already sitting down.\n\r",ch);
            break;
        case POS_FIGHTING:
            send_to_char("Maybe you should finish this fight first?\n\r",ch);
            break;
        case POS_STANDING:
            send_to_char("You sit down.\n\r",ch);
            act("$n sits down on the ground.",ch,NULL,NULL,TO_ROOM);
            ch->position = POS_SITTING;
            break;
    }
    return;
}


void do_sleep( CHAR_DATA *ch, char *argument )
{
    switch ( ch->position )
    {
    case POS_SLEEPING:
        send_to_char( "You are already sleeping.\n\r", ch );
        break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING: 
        send_to_char( "You go to sleep.\n\r", ch );
        act( "$n goes to sleep.", ch, NULL, NULL, TO_ROOM );
        ch->position = POS_SLEEPING;
        break;

    case POS_FIGHTING:
        send_to_char( "You are already fighting!\n\r", ch );
        break;
    }

    return;
}



void do_wake( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        do_stand( ch, argument ); 
        return; 
    }

    if ( !IS_AWAKE(ch) )
    { 
        send_to_char( "You are asleep yourself!\n\r", ch ); 
        return; 
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    { 
        send_to_char( "They aren't here.\n\r", ch ); 
        return; 
    }

    if ( IS_AWAKE(victim) )
    { 
        act( "$N is already awake.", ch, NULL, victim, TO_CHAR ); 
        return; 
    }

    if ( IS_AFFECTED(victim, AFF_SLEEP) )
    { 
        act( "You can't wake $M!",   ch, NULL, victim, TO_CHAR );  
        return; 
    }

    victim->position = POS_STANDING;
    act( "You wake $M.", ch, NULL, victim, TO_CHAR );
    act( "$n wakes you.", ch, NULL, victim, TO_VICT );
    return;
}



void do_sneak( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;

    send_to_char( "You attempt to move silently.\n\r", ch );
    affect_strip( ch, gsn_sneak );

    if ( IS_NPC(ch) || number_percent( ) < ch->pcdata->learned[gsn_sneak] ) {
        check_improve(ch,gsn_sneak,TRUE,3);
        af.where     = TO_AFFECTS;
        af.type      = gsn_sneak;
//        af.level     = ch->level; 
		af.level	 = 0;
//        af.duration  = ch->level;
        af.duration  = number_range( 2, 10 );
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_SNEAK;
        affect_to_char( ch, &af );
    } else {
        check_improve( ch, gsn_sneak, FALSE, 3 );
	}

    return;
}



void do_hide( CHAR_DATA *ch, char *argument )
{
    send_to_char( "You attempt to hide.\n\r", ch );

    if ( IS_AFFECTED(ch, AFF_HIDE) )
        REMOVE_BIT(ch->affected_by, AFF_HIDE);

    if ( IS_NPC(ch) || number_percent( ) < ch->pcdata->learned[gsn_hide] )
    {
        SET_BIT(ch->affected_by, AFF_HIDE);
        check_improve(ch,gsn_hide,TRUE,3);
    }
    else
        check_improve(ch,gsn_hide,FALSE,3);

    return;
}



/*
 * Contributed by Alander.
 */
void do_visible( CHAR_DATA *ch, char *argument )
{
    affect_strip ( ch, gsn_invis                        );
    affect_strip ( ch, gsn_mass_invis                   );
    affect_strip ( ch, gsn_sneak                        );
    REMOVE_BIT   ( ch->affected_by, AFF_HIDE            );
    REMOVE_BIT   ( ch->affected_by, AFF_INVISIBLE       );
    REMOVE_BIT   ( ch->affected_by, AFF_SNEAK           );
    send_to_char( "Ok.\n\r", ch );
    return;
}



void do_recall( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;

    if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET))
    {
        send_to_char("Only players can recall.\n\r",ch);
        buffer_free( buf );
        return;
    }
                
    act( "$n prays for transportation!", ch, 0, 0, TO_ROOM );

    if (!IS_NPC(ch))
    {
        if ( ch->pcdata->recall_room == NULL)
        {
             ch->pcdata->recall_room = get_room_index( ROOM_VNUM_TEMPLE);
        }
        location = ch->pcdata->recall_room;
    }
    else
    {
        location = ch->master->pcdata->recall_room;     
    }

    if ( ch->in_room == location )
    {
        send_to_char( "Look around, look familiar?\n\r", ch);
        buffer_free( buf );
        return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_AFFECTED(ch, AFF_CURSE) )
    {
        send_to_char( "The gods have forsaken you.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = ch->fighting ) != NULL )
    {
        int skill;

        if (IS_NPC(ch))
            skill = 40 + number_range( 1, 50 );
        else
            skill = ch->pcdata->learned[gsn_recall];

        if ( number_percent() < 80 * skill / 100 )
        {
            check_improve(ch,gsn_recall,FALSE,6);
            WAIT_STATE( ch, 4 );
            /* uuhh.. why not a send_to_char() here? -- Rahl */
          /*  bprintf( buf, "You failed!\n\r");
           *  send_to_char( buf->data, ch ); 
           */
            send_to_char( "You failed!\n\r", ch );
            buffer_free( buf );
            return;
        }

		if ( !IS_NPC( ch ) ) {
        	check_improve(ch,gsn_recall,TRUE,4);
        	bprintf( buf, "You recall from combat!\n\r" );
        	send_to_char( buf->data, ch );
		}
        stop_fighting( ch, TRUE );
        
    }

    ch->move /= 2;
    send_to_char("You pray to the gods for transportation!\n\r", ch);
    act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    
    if (ch->pet != NULL)
        do_recall(ch->pet,"");

    buffer_free( buf );
    return;
}



void do_train( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *mob;
    sh_int stat = - 1;
    char *pOutput = NULL;
    int cost;

    if ( IS_NPC(ch) )
    {
        buffer_free( buf );
        return;
    }

    /*
     * Check for trainer.
     */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN) )
            break;
    }

    if ( mob == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( argument[0] == '\0' )
    {
        bprintf( buf, "You have %d training sessions.\n\r", ch->train );
        send_to_char( buf->data, ch );
        /* WTF??? --Rahl */
        /* argument = "foo"; */
    }

    cost = 1;

    if ( !str_cmp( argument, "str" ) )
    {
        if ( class_table[ch->ch_class].attr_prime == STAT_STR )
            cost    = 1;
        stat        = STAT_STR;
        pOutput     = "strength";
    }

    else if ( !str_cmp( argument, "int" ) )
    {
        if ( class_table[ch->ch_class].attr_prime == STAT_INT )
            cost    = 1;
        stat        = STAT_INT;
        pOutput     = "intelligence";
    }

    else if ( !str_cmp( argument, "wis" ) )
    {
        if ( class_table[ch->ch_class].attr_prime == STAT_WIS )
            cost    = 1;
        stat        = STAT_WIS;
        pOutput     = "wisdom";
    }

    else if ( !str_cmp( argument, "dex" ) )
    {
        if ( class_table[ch->ch_class].attr_prime == STAT_DEX )
            cost    = 1;
        stat        = STAT_DEX;
        pOutput     = "dexterity";
    }

    else if ( !str_cmp( argument, "con" ) )
    {
        if ( class_table[ch->ch_class].attr_prime == STAT_CON )
            cost    = 1;
        stat        = STAT_CON;
        pOutput     = "constitution";
    }

    else if ( !str_cmp(argument, "hp" ) )
        cost = 1;

    else if ( !str_cmp(argument, "mana" ) )
        cost = 1;

    else if ( !str_cmp(argument, "move" ) )
        cost = 1;

    else
    {
        bprintf( buf, "You can train:" );
        if ( ch->perm_stat[STAT_STR] < get_max_train(ch,STAT_STR)) 
            buffer_strcat( buf, " str" );
        if ( ch->perm_stat[STAT_INT] < get_max_train(ch,STAT_INT))  
            buffer_strcat( buf, " int" );
        if ( ch->perm_stat[STAT_WIS] < get_max_train(ch,STAT_WIS)) 
            buffer_strcat( buf, " wis" );
        if ( ch->perm_stat[STAT_DEX] < get_max_train(ch,STAT_DEX))  
            buffer_strcat( buf, " dex" );
        if ( ch->perm_stat[STAT_CON] < get_max_train(ch,STAT_CON))  
            buffer_strcat( buf, " con" );
        if ( !IS_IMMORTAL( ch ) )
            buffer_strcat( buf, " hp mana move");

        if ( buf->data[strlen(buf->data)-1] != ':' )
        {
            buffer_strcat( buf, ".\n\r" );
            send_to_char( buf->data, ch );
        }
        else
        {
            act( "You have nothing left to train.", ch, NULL, NULL,
                TO_CHAR );
        }

        buffer_free( buf );
        return;
    }

    if (!str_cmp("hp",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            buffer_free( buf );
            return;
        }
 
        ch->train -= cost;
        ch->pcdata->perm_hit += 5;
        ch->max_hit += 5;
        ch->hit +=5;
        act( "Your durability increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's durability increases!",ch,NULL,NULL,TO_ROOM);
        buffer_free( buf );
        return;
    }
 
    if (!str_cmp("mana",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            buffer_free( buf );
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_mana += 10;
        ch->max_mana += 10;
        ch->mana += 10;
        act( "Your power increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's power increases!",ch,NULL,NULL,TO_ROOM);
        buffer_free( buf );
        return;
    }

    if (!str_cmp("move",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            buffer_free( buf );
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_move += 10;
        ch->max_move += 10;
        ch->move += 10;
        act( "Your stamina increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's stamina increases!",ch,NULL,NULL,TO_ROOM);
        buffer_free( buf );
        return;
    }

    if ( ch->perm_stat[stat]  >= get_max_train(ch,stat) )
    {
        act( "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR );
        buffer_free( buf );
        return;
    }

    if ( cost > ch->train )
    {
        send_to_char( "You don't have enough training sessions.\n\r", ch );
        buffer_free( buf );
        return;
    }

    ch->train           -= cost;
  
    ch->perm_stat[stat]         += 1;
    act( "Your $T increases!", ch, NULL, pOutput, TO_CHAR );
    act( "$n's $T increases!", ch, NULL, pOutput, TO_ROOM );
    buffer_free( buf );
    return;
}


bool check_web( CHAR_DATA *ch ) {
    AFFECT_DATA *af;
    int chance = 0;
    int orig_dur = 0;
        
    if ( IS_IMMORTAL( ch ) ) {
        return TRUE;
	}

     
    af = affect_find( ch->affected, skill_lookup("web") );

    orig_dur = ( af->level / 3 );
    chance = ( get_curr_stat( ch, STAT_STR ) );

    if ( af->duration < ( orig_dur / 9 ) ) {
        chance += 20;
	}

    if ( af->duration > ( orig_dur / 3 ) ) {
        chance -= 20;
	}

    if ( number_percent() < chance ) {
	    return TRUE;
	}

	return FALSE;
}

void do_beacon( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    if ( IS_NPC( ch ) )
    {
        send_to_char( "Try again when you aren't switched.\n\r", ch );
        return;
    }

    if ( arg[0] == '\0' )
    {
        ch->pcdata->recall_room = ch->in_room;
        send_to_char( "Beacon set.\n\r", ch );
        return;
    }
   
    if ( !strcmp( arg, "reset" ) )
    {
        ch->pcdata->recall_room = get_room_index( ROOM_VNUM_TEMPLE );
        send_to_char( "Beacon reset.\n\r", ch );
        return;
    }
   
    send_to_char( "Syntax: Beacon\n\r        Beacon reset.\n\r", ch );
    return;
}


/*
 * clan recall based on code by John Buntin
 * modified and implemented here by Rahl 
 */
void do_clan_recall( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    int clan_hall;

    if ( IS_NPC( ch ) )
    {
        send_to_char( "Only players can recall.\n\r", ch );
        return;
    }

    if ( ch->pcdata->clan == 0 )
    {
        send_to_char( "You aren't in a clan!\n\r", ch );
        return;
    }

    act( "$n prays for safe passage.", ch, NULL, NULL, TO_ROOM );

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
    || IS_AFFECTED( ch, AFF_CURSE ) )
    {
        send_to_char( "The gods have forsaken you.\n\r", ch );
        return;
    }

    if ( ch->fighting != NULL )
    {
        send_to_char( "You may not recall out of a fight!\n\r", ch );
        return;
    }
 
    switch ( ch->pcdata->clan )
    {
        default: clan_hall = 3001;      break;
        case 1:  clan_hall = 10900;     break;
        case 2:  clan_hall = 3001;      break;
        case 3:  clan_hall = 3001;      break;
        case 4:  clan_hall = 3001;      break;
        case 5:  clan_hall = 3001;      break;
    }

    location = get_room_index( clan_hall );

    ch->move /= 2;
    act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    if ( ch->pet != NULL )
    {
        char_from_room( ch->pet );
        char_to_room( ch->pet, location );
    }

    return;
}
