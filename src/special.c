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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "special.h"

/*
 * Special Functions Table.     OLC
 */
const   struct  spec_type       spec_table      [ ] =
{
    /*
     * Special function commands.
     */
    { "spec_breath_any",        spec_breath_any         },
    { "spec_breath_acid",       spec_breath_acid        },
    { "spec_breath_fire",       spec_breath_fire        },
    { "spec_breath_frost",      spec_breath_frost       },
    { "spec_breath_gas",        spec_breath_gas         },
    { "spec_breath_lightning",  spec_breath_lightning   },
    { "spec_cast_adept",        spec_cast_adept         },
    { "spec_cast_cleric",       spec_cast_cleric        },
    { "spec_cast_judge",        spec_cast_judge         },
    { "spec_cast_mage",         spec_cast_mage          },
    { "spec_cast_undead",       spec_cast_undead        },
    { "spec_executioner",       spec_executioner        },
    { "spec_fido",              spec_fido               },
    { "spec_guard",             spec_guard              },
    { "spec_clan_guard",        spec_clan_guard         },
    { "spec_janitor",           spec_janitor            },
    { "spec_mayor",             spec_mayor              },
    { "spec_poison",            spec_poison             },
    { "spec_thief",             spec_thief              },
    { "spec_puff",              spec_puff               },      /* ROM OLC */
    /* added by Rahl */
    { "spec_questmaster",       spec_questmaster        },
    { "spec_assassin",          spec_assassin           },
    { "spec_gamemaster",        spec_gamemaster         },
    { "spec_flower_shop",       spec_flower_shop        },
    { "spec_engraver",          spec_engraver           },
 
    /*
     * End of list.
     */
    { "",                       0       }
};



/*****************************************************************************
 Name:          spec_string
 Purpose:       Given a function, return the appropriate name.
 Called by:     <???>
 ****************************************************************************/
char *spec_string( SPEC_FUN *fun )      /* OLC */
{
    int cmd;
    
    for ( cmd = 0; spec_table[cmd].spec_fun != NULL; cmd++ )
        if ( fun == spec_table[cmd].spec_fun )
            return spec_table[cmd].spec_name;

    return 0;
}



/*****************************************************************************
 Name:          spec_lookup
 Purpose:       Given a name, return the appropriate spec fun.
 Called by:     do_mset(act_wiz.c) load_specials,reset_area(db.c)
 ****************************************************************************/
SPEC_FUN *spec_lookup( const char *name )       /* OLC */
{
    int cmd;
    
    for ( cmd = 0; spec_table[cmd].spec_name[0] != '\0'; cmd++ )
        if ( !str_cmp( name, spec_table[cmd].spec_name ) )
            return spec_table[cmd].spec_fun;

    return 0;
}

/*
 * Core procedure for dragons.
 */
bool dragon( CHAR_DATA *ch, char *spell_name )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int sn;

    if ( ch->position != POS_FIGHTING )
        return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( victim == NULL )
        return FALSE;

    if ( ( sn = skill_lookup( spell_name ) ) < 0 )
        return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR );
    return TRUE;
}



/*
 * Special procedures for mobiles.
 */
bool spec_breath_any( CHAR_DATA *ch )
{
    if ( ch->position != POS_FIGHTING )
        return FALSE;

    switch ( number_bits( 3 ) )
    {
    case 0: return spec_breath_fire             ( ch );
    case 1:
    case 2: return spec_breath_lightning        ( ch );
    case 3: return spec_breath_gas              ( ch );
    case 4: return spec_breath_acid             ( ch );
    case 5:
    case 6:
    case 7: return spec_breath_frost            ( ch );
    }

    return FALSE;
}



bool spec_breath_acid( CHAR_DATA *ch )
{
    return dragon( ch, "acid breath" );
}



bool spec_breath_fire( CHAR_DATA *ch )
{
    return dragon( ch, "fire breath" );
}



bool spec_breath_frost( CHAR_DATA *ch )
{
    return dragon( ch, "frost breath" );
}



bool spec_breath_gas( CHAR_DATA *ch )
{
    int sn;

    if ( ch->position != POS_FIGHTING )
        return FALSE;

    if ( ( sn = skill_lookup( "gas breath" ) ) < 0 )
        return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, NULL, TARGET_CHAR );
    return TRUE;
}



bool spec_breath_lightning( CHAR_DATA *ch )
{
    return dragon( ch, "lightning breath" );
}



bool spec_cast_adept( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;

    if ( !IS_AWAKE(ch) )
        return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim != ch && can_see( ch, victim ) && number_bits( 1 ) == 0 
             && !IS_NPC(victim) && victim->level < 11)
            break;
    }

    if ( victim == NULL )
        return FALSE;

    switch ( number_bits( 4 ) )
    {
    case 0:
        act( "$n utters the word 'abrazak'.", ch, NULL, NULL, TO_ROOM );
        spell_armor( skill_lookup( "armor" ), ch->level, ch, victim,
                TARGET_CHAR );
        return TRUE;

    case 1:
        act( "$n utters the word 'fido'.", ch, NULL, NULL, TO_ROOM );
        spell_bless( skill_lookup( "bless" ), ch->level, ch, victim,
                TARGET_CHAR );
        return TRUE;

    case 2:
        act( "$n utters the word 'judicandus noselacri'.", ch, NULL, NULL, TO_ROOM );
        spell_cure_blindness( skill_lookup( "cure blindness" ),
            ch->level, ch, victim, TARGET_CHAR );
        return TRUE;

    case 3:
        act( "$n utters the word 'pzar'.", ch, NULL, NULL, TO_ROOM );
        spell_heal( skill_lookup( "heal" ),
            ch->level, ch, victim, TARGET_CHAR );
        return TRUE;

    case 4:
        act( "$n utters the words 'judicandus sausabru'.", ch, NULL, NULL, TO_ROOM );
        spell_cure_poison( skill_lookup( "cure poison" ),
            ch->level, ch, victim, TARGET_CHAR );
        return TRUE;

    case 5:
        act( "$n utters the words 'candusima'.", ch, NULL, NULL, TO_ROOM );
        spell_refresh( skill_lookup( "refresh" ), ch->level, ch, victim,
                TARGET_CHAR );
        return TRUE;

    }

    return FALSE;
}



bool spec_cast_cleric( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    if ( ch->position != POS_FIGHTING )
        return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( victim == NULL )
        return FALSE;

    for ( ;; )
    {
        int min_level;

        switch ( number_bits( 4 ) )
        {
        case  0: min_level =  0; spell = "blindness";      break;
        case  1: min_level =  3; spell = "cause serious";  break;
        case  2: min_level =  7; spell = "earthquake";     break;
        case  3: min_level =  9; spell = "cause critical"; break;
        case  4: min_level = 10; spell = "dispel evil";    break;
        case  5: min_level = 12; spell = "curse";          break;
        case  6: min_level = 12; spell = "change sex";     break;
        case  7: min_level = 13; spell = "flamestrike";    break;
        case  8:
        case  9:
        case 10: min_level = 15; spell = "harm";           break;
        case 11: min_level = 15; spell = "plague";         break;
        default: min_level = 16; spell = "dispel magic";   break;
        }

        if ( ch->level >= min_level )
            break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR );
    return TRUE;
}

bool spec_cast_judge( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;
 
    if ( ch->position != POS_FIGHTING )
        return FALSE;
 
    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
            break;
    }
 
    if ( victim == NULL )
        return FALSE;
 
    spell = "high explosive";
    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR );
    return TRUE;
}



bool spec_cast_mage( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    if ( ch->position != POS_FIGHTING )
        return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( victim == NULL )
        return FALSE;

    for ( ;; )
    {
        int min_level;

        switch ( number_bits( 4 ) )
        {
        case  0: min_level =  0; spell = "blindness";      break;
        case  1: min_level =  3; spell = "chill touch";    break;
        case  2: min_level =  7; spell = "weaken";         break;
        case  3: min_level =  8; spell = "teleport";       break;
        case  4: min_level = 11; spell = "colour spray";   break;
        case  5: min_level = 12; spell = "change sex";     break;
        case  6: min_level = 13; spell = "energy drain";   break;
        case  7:
        case  8:
        case  9: min_level = 15; spell = "fireball";       break;
        case 10: min_level = 20; spell = "plague";         break;
        default: min_level = 20; spell = "acid blast";     break;
        }

        if ( ch->level >= min_level )
            break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR );
    return TRUE;
}



bool spec_cast_undead( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    if ( ch->position != POS_FIGHTING )
        return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( victim == NULL )
        return FALSE;

    for ( ;; )
    {
        int min_level;

        switch ( number_bits( 4 ) )
        {
        case  0: min_level =  0; spell = "curse";          break;
        case  1: min_level =  3; spell = "weaken";         break;
        case  2: min_level =  6; spell = "chill touch";    break;
        case  3: min_level =  9; spell = "blindness";      break;
        case  4: min_level = 12; spell = "poison";         break;
        case  5: min_level = 15; spell = "energy drain";   break;
        case  6: min_level = 18; spell = "harm";           break;
        case  7: min_level = 21; spell = "teleport";       break;
        case  8: min_level = 20; spell = "plague";         break;
        default: min_level = 18; spell = "harm";           break;
        }

        if ( ch->level >= min_level )
            break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR );
    return TRUE;
}


bool spec_executioner( CHAR_DATA *ch )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *crime;

    if ( !IS_AWAKE(ch) || ch->fighting != NULL )
    {
        buffer_free( buf );
        return FALSE;
    }

    crime = "";
    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;

/*      if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) )
            { crime = "KILLER"; break; }*/

        if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) )
            { crime = "THIEF"; break; }
    }

    if ( victim == NULL )
    {
        buffer_free( buf );
        return FALSE;
    }

    bprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
        victim->name, crime );
    do_yell( ch, buf->data );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    char_to_room( create_mobile( get_mob_index(MOB_VNUM_CITYGUARD) ),
        ch->in_room );
    char_to_room( create_mobile( get_mob_index(MOB_VNUM_CITYGUARD) ),
        ch->in_room );
    buffer_free( buf );
    return TRUE;
}

/* A procedure for Puff the Fractal Dragon--> it gives her an attitude.
Note that though this procedure may make Puff look busy, she in
fact does nothing quite more often than she did in Merc 1.0;
due to null victim traps, my own do-nothing options, and various ways
to return without doing much, Puff is... well, not as BAD of a gadfly
as she may look, I assure you.  But I think she's fun this way ;)

(btw--- should you ever want to test out your socials, just tweak
the percentage table ('silliness') to make her do lots of socials,
and then go to a quiet room and load up about thirty Puffs... ;) 
                
        written by Seth of Rivers of Mud         */
                        
bool spec_puff( CHAR_DATA *ch )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    int rnd_social, sn, silliness;
    bool pc_found = TRUE;
    CHAR_DATA *v_next;
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    CHAR_DATA *nch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;
    extern int social_count;
        
    if ( !IS_AWAKE(ch) )
    {
        buffer_free( buf );
        return FALSE;
    }

    victim = NULL;
  
/* Here's Furey's aggress routine, with some surgery done to it.  
  All it does is pick a potential victim for a social.  
  (Thank you, Furey-- I screwed this up many times until I
  learned of your way of doing it)                      */
                
    for ( wch = char_list; wch != NULL; wch = wch_next )
    {
        wch_next = wch->next;
        if ( IS_NPC(wch)
        ||   wch->in_room == NULL )
            continue;

        for ( nch = wch->in_room->people; nch != NULL; nch = ch_next )
        {
            int count;

            ch_next     = nch->next_in_room;

            if ( !IS_NPC(nch) 
            ||   number_bits(1) == 0)
                continue;

            /*
             * Ok we have a 'wch' player character and a 'nch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             */
            count       = 0;
            victim      = NULL;
            for ( vch = wch->in_room->people; vch != NULL; vch = vch_next )
            {
                vch_next = vch->next_in_room;

                if ( !IS_NPC(vch) )
                {
                    if ( number_range( 0, count ) == 0 )
                        victim = vch;
                    count++;
                }
            }

            if (victim == NULL)
            {
                buffer_free( buf );
                return FALSE;
            }
        }

    }

    rnd_social = (number_range (0, ( social_count - 1)) );
                        
    /* Choose some manner of silliness to perpetrate.  */
        
    silliness = number_range (1, 100);
                
    if ( silliness <= 20)
    {
        buffer_free( buf );
        return TRUE;
    }
    else if ( silliness <= 30)
    {
        bprintf( buf, "Tongue-tied and twisted, just an earthbound"
            " misfit, ..."); 
        do_say ( ch, buf->data);
    }
    else if ( silliness <= 40)
    {
        bprintf( buf, "The colors, the colors!");
        do_say ( ch, buf->data);
    }
    else if ( silliness <= 55)
    {
        bprintf( buf, "Did you know that I'm written in C?");
        do_say ( ch, buf->data );
    }
    else if ( silliness <= 75)
    {
        act( social_table[rnd_social].others_no_arg, 
            ch, NULL, NULL, TO_ROOM    );
        act( social_table[rnd_social].char_no_arg,   
            ch, NULL, NULL, TO_CHAR    );
    }
    else if ( silliness <= 85)
    {           
        if ( (!pc_found)
        ||       (victim != ch->in_room->people) ) 
        {
            buffer_free( buf );
            return FALSE;
        }
        act( social_table[rnd_social].others_found, 
            ch, NULL, victim, TO_NOTVICT );
        act( social_table[rnd_social].char_found,  
            ch, NULL, victim, TO_CHAR    );
        act( social_table[rnd_social].vict_found, 
            ch, NULL, victim, TO_VICT    );
    }
                
    else if ( silliness <= 97)  
    {   
        act( "For a moment, $n flickers and phases.", 
            ch, NULL, NULL, TO_ROOM );
        act( "For a moment, you flicker and phase.", 
            ch, NULL, NULL, TO_CHAR );
    }
        
/* The Fractal Dragon sometimes teleports herself around, to check out
        new and stranger things.  HOWEVER, to stave off some possible Puff
        repop problems, and to make it possible to play her as a mob without
        teleporting helplessly, Puff does NOT teleport if she's in Limbo,
        OR if she's not fighting or standing.  If you're playing Puff and 
        you want to talk with someone, just rest or sit!
*/
        
    else{
        if (ch->position < POS_FIGHTING)
        {
            act( "For a moment, $n seems lucid...", 
                ch, NULL, NULL, TO_ROOM );
            act( "   ...but then $e returns to $s contemplations once"
                " again.", ch, NULL, NULL, TO_ROOM );
            act( "For a moment, the world's mathematical beauty is lost"
                " to you!", ch, NULL, NULL, TO_CHAR );
            act( "   ...but joy! yet another novel phenomenon seizes your"
                " attention.", ch, NULL, NULL, TO_CHAR);
            buffer_free( buf );
            return TRUE;
        }

        if ( ( sn = skill_lookup( "teleport" ) ) < 0 )
        {
            buffer_free( buf );
            return FALSE;
        }

        (*skill_table[sn].spell_fun) ( sn, ch->level, ch, ch, TARGET_CHAR );
     }


/* Puff has only one spell, and it's the most annoying one, of course.
        (excepting energy drain, natch)  But to a bemused mathematician,
        what could possibly be a better resolution to conflict? ;) 
        Oh-- and notice that Puff casts her one spell VERY well.     */
                        
    if ( ch->position != POS_FIGHTING )
    {
        buffer_free( buf );
        return FALSE;
    }

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
                break;
    }

    if ( victim == NULL )
    {
        buffer_free( buf );
        return FALSE;
    }

    if ( ( sn = skill_lookup( "teleport" ) ) < 0 )
    {
        buffer_free( buf );
        return FALSE;
    }

    (*skill_table[sn].spell_fun) ( sn, 50, ch, victim, TARGET_CHAR );

    buffer_free( buf );
    return TRUE;

}

bool spec_fido( CHAR_DATA *ch )
{
    OBJ_DATA *corpse;
    OBJ_DATA *c_next;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    if ( !IS_AWAKE(ch) )
        return FALSE;

    for ( corpse = ch->in_room->contents; corpse != NULL; corpse = c_next )
    {
        c_next = corpse->next_content;
        if ( corpse->item_type != ITEM_CORPSE_NPC )
            continue;

        act( "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );
        for ( obj = corpse->contains; obj; obj = obj_next )
        {
            obj_next = obj->next_content;
            obj_from_obj( obj );
            obj_to_room( obj, ch->in_room );
        }
        extract_obj( corpse );
        return TRUE;
    }

    return FALSE;
}



bool spec_guard( CHAR_DATA *ch )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *ech;
    char *crime;
    int max_evil;

    if ( !IS_AWAKE(ch) || ch->fighting != NULL )
    {
        buffer_free( buf );
        return FALSE;
    }

    max_evil = 300;
    ech      = NULL;
    crime    = "";

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;

/*      if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) )
            { crime = "KILLER"; break; }*/

        if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) )
            { crime = "THIEF"; break; }

        if ( victim->fighting != NULL
        &&   victim->fighting != ch
        &&   victim->alignment < max_evil )
        {
            max_evil = victim->alignment;
            ech      = victim;
        }
    }

    if ( victim != NULL )
    {
        bprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!",
            victim->name, crime );
        do_yell( ch, buf->data );
        multi_hit( ch, victim, TYPE_UNDEFINED );
        buffer_free( buf );
        return TRUE;
    }

    if ( ech != NULL )
    {
        act( "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!",
            ch, NULL, NULL, TO_ROOM );
        multi_hit( ch, ech, TYPE_UNDEFINED );
        buffer_free( buf );
        return TRUE;
    }

    buffer_free( buf );
    return FALSE;
}

bool spec_clan_guard( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *ech;
    BUFFER *buf = buffer_new( 200 );

    if ( !IS_AWAKE(ch) || ch->fighting != NULL )
    {
        buffer_free( buf );
        return FALSE;
    }

    ech      = NULL;

    for ( victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room )
    {
        if ( victim->fighting != NULL
        &&   victim->fighting != ch 
        &&   (IS_NPC (victim) ? victim->pIndexData->clan != ch->pIndexData->clan
        : victim->pcdata->clan != ch->pIndexData->clan) )
        {
            ech      = victim;
        }
    }

    if ( ech != NULL )
    {
        act( "$n screams 'PROTECT THE CLAN!!  BANZAI!!",
            ch, NULL, NULL, TO_ROOM );
        bprintf(buf, "Quick, %s is fighting %s!!", ech->name, 
            ech->fighting->name);
        do_yell(ch, buf->data);
        multi_hit( ch, ech, TYPE_UNDEFINED );
        buffer_free( buf );
        return TRUE;
    }
   
    buffer_free( buf );
    return FALSE;
}



bool spec_janitor( CHAR_DATA *ch )
{
    OBJ_DATA *trash;
    OBJ_DATA *trash_next;

    if ( !IS_AWAKE(ch) )
        return FALSE;

    for ( trash = ch->in_room->contents; trash != NULL; trash = trash_next )
    {
        trash_next = trash->next_content;
        if ( !IS_SET( trash->wear_flags, ITEM_TAKE ) || !can_loot(ch,trash))
            continue;
        if ( trash->item_type == ITEM_DRINK_CON
        ||   trash->item_type == ITEM_TRASH
        ||   trash->cost < 10 )
        {
            act( "$n picks up some trash.", ch, NULL, NULL, TO_ROOM );
            obj_from_room( trash );
            obj_to_char( trash, ch );
            return TRUE;
        }
    }

    return FALSE;
}



bool spec_mayor( CHAR_DATA *ch )
{
    static const char open_path[] =
        "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

    static const char close_path[] =
        "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path;
    static int pos;
    static bool move;

    if ( !move )
    {
        if ( time_info.hour ==  6 )
        {
            path = open_path;
            move = TRUE;
            pos  = 0;
        }

        if ( time_info.hour == 20 )
        {
            path = close_path;
            move = TRUE;
            pos  = 0;
        }
    }

    if ( ch->fighting != NULL )
        return spec_cast_cleric( ch );
    if ( !move || ch->position < POS_SLEEPING )
        return FALSE;

    switch ( path[pos] )
    {
    case '0':
    case '1':
    case '2':
    case '3':
        move_char( ch, path[pos] - '0', FALSE );
        break;

    case 'W':
        ch->position = POS_STANDING;
        act( "$n awakens and groans loudly.", ch, NULL, NULL, TO_ROOM );
        break;

    case 'S':
        ch->position = POS_SLEEPING;
        act( "$n lies down and falls asleep.", ch, NULL, NULL, TO_ROOM );
        break;

    case 'a':
        act( "$n says 'Hello Honey!'", ch, NULL, NULL, TO_ROOM );
        break;

    case 'b':
        act( "$n says 'What a view!  I must do something about that dump!'",
            ch, NULL, NULL, TO_ROOM );
        break;

    case 'c':
        act( "$n says 'Vandals!  Youngsters have no respect for anything!'",
            ch, NULL, NULL, TO_ROOM );
        break;

    case 'd':
        act( "$n says 'Good day, citizens!'", ch, NULL, NULL, TO_ROOM );
        break;

    case 'e':
        act( "$n says 'I hereby declare the city of Midgaard open!'",
            ch, NULL, NULL, TO_ROOM );
        break;

    case 'E':
        act( "$n says 'I hereby declare the city of Midgaard closed!'",
            ch, NULL, NULL, TO_ROOM );
        break;

    case 'O':
/*      do_unlock( ch, "gate" ); */
        do_open( ch, "gate" );
        break;

    case 'C':
        do_close( ch, "gate" );
/*      do_lock( ch, "gate" ); */
        break;

    case '.' :
        move = FALSE;
        break;
    }

    pos++;
    return FALSE;
}



bool spec_poison( CHAR_DATA *ch )
{
    CHAR_DATA *victim;

    if ( ch->position != POS_FIGHTING
    || ( victim = ch->fighting ) == NULL
    ||   number_percent( ) > 2 * ch->level )
        return FALSE;

    act( "You bite $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n bites $N!",  ch, NULL, victim, TO_NOTVICT );
    act( "$n bites you!", ch, NULL, victim, TO_VICT    );
    spell_poison( gsn_poison, ch->level, ch, victim, TARGET_CHAR );
    return TRUE;
}



bool spec_thief( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    long gold;

    if ( ch->position != POS_STANDING )
        return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;

        if ( IS_NPC(victim)
        ||   IS_IMMORTAL( victim )
        ||   number_bits( 5 ) != 0 
        ||   !can_see(ch,victim))
            continue;

        if ( IS_AWAKE(victim) && number_range( 0, ch->level ) == 0 )
        {
            act( "You discover $n's hands in your wallet!",
                ch, NULL, victim, TO_VICT );
            act( "$N discovers $n's hands in $S wallet!",
                ch, NULL, victim, TO_NOTVICT );
            return TRUE;
        }
        else
        {
            gold = victim->gold * UMIN(number_range( 1, 20 ),ch->level) / 100;
            gold = UMIN(gold, ch->level * ch->level * 20 );
            ch->gold     += gold;
            victim->gold -= gold;
            return TRUE;
        }
    }

    return FALSE;
}

/* added by Rahl */
bool spec_questmaster( CHAR_DATA *ch )
{
        return TRUE;
}

/* 
 * added by Rahl. Got the code off the net somewhere, but I don't see
 * credits, so I can't give credit where credit is due. Some slight 
 * modifications, of course.
 */
bool spec_assassin( CHAR_DATA *ch )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int rnd_say;

    if ( ch->fighting != NULL )
    {
        buffer_free( buf );
        return FALSE;
    }

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->ch_class != 2 ) /* thieves */
            break;
    }

    if ( victim == NULL || victim == ch || IS_IMMORTAL( victim ) )
    {
        buffer_free( buf );
        return FALSE;
    }

    if ( victim->level > ch->level + 7 || IS_NPC( victim ) )
    {
        buffer_free( buf );
        return FALSE;
    }

    rnd_say = number_range( 1, 10 );

    if ( rnd_say <= 5 )
        bprintf( buf, "Death is the true end..." );
    else if ( rnd_say <= 6 )
        bprintf( buf, "Time to die!" );
    else if ( rnd_say <= 7 )
        bprintf( buf, "Sayonara!" );
    else if ( rnd_say <= 8 )
        bprintf( buf, "Welcome to your fate!" );
    else if ( rnd_say <= 9 )
        bprintf( buf, "It is a good day to die..." );
    else if ( rnd_say <= 10 )
        bprintf( buf, "Ever dance with the devil?" );

    do_say( ch, buf->data );
    multi_hit( ch, victim, gsn_backstab );
    buffer_free( buf );
    return TRUE;
}

bool spec_gamemaster( CHAR_DATA *ch )
{
    return spec_cast_mage( ch );
}

bool spec_flower_shop( CHAR_DATA *ch )
{
    return spec_cast_mage( ch );
}

bool spec_engraver( CHAR_DATA *ch )
{
    return spec_cast_mage( ch );
}

