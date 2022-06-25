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

/*
 * object cases, af.where, and most of the new damage stuff by Rahl
 * (Daniel Anderson) of Broken Shadows. Most of these changes (except the
 * damage) is from ROM 2.4b4 code
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"

/*
 * Local functions.
 */
void    say_spell       ( CHAR_DATA *ch, int sn );

/*
 * Imported functions
 */
bool    remove_obj      ( CHAR_DATA *ch, int iWear, bool fReplace );

/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
    int sn;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
        &&   !str_prefix( name, skill_table[sn].name ) )
            return sn;
    }

    return -1;
}



/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
    extern bool fBootDb;
    int sn;

    if ( slot <= 0 )
        return -1;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( slot == skill_table[sn].slot )
            return sn;
    }

    if ( fBootDb )
    {
        bug( "Slot_lookup: bad slot %d.", slot );
        abort( );
    }

    return -1;
}



/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA *ch, int sn )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf2 = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *rch;
    char *pName;
    int iSyl;
    int length;

    struct syl_type
    {
        char *  old;
        char *  new;
    };

    static const struct syl_type syl_table[] =
    {
        { " ",          " "             },
        { "ar",         "abra"          },
        { "au",         "kada"          },
        { "bless",      "fido"          },
        { "blind",      "nose"          },
        { "bur",        "mosa"          },
        { "cu",         "judi"          },
        { "de",         "oculo"         },
        { "en",         "unso"          },
        { "light",      "dies"          },
        { "lo",         "hi"            },
        { "mor",        "zak"           },
        { "move",       "sido"          },
        { "ness",       "lacri"         },
        { "ning",       "illa"          },
        { "per",        "duda"          },
        { "ra",         "gru"           },
        { "fresh",      "ima"           },
        { "re",         "candus"        },
        { "son",        "sabru"         },
        { "tect",       "infra"         },
        { "tri",        "cula"          },
        { "ven",        "nofo"          },
        { "a", "a" }, { "b", "b" }, { "c", "q" }, { "d", "e" },
        { "e", "z" }, { "f", "y" }, { "g", "o" }, { "h", "p" },
        { "i", "u" }, { "j", "y" }, { "k", "t" }, { "l", "r" },
        { "m", "w" }, { "n", "i" }, { "o", "a" }, { "p", "s" },
        { "q", "d" }, { "r", "f" }, { "s", "g" }, { "t", "h" },
        { "u", "j" }, { "v", "z" }, { "w", "x" }, { "x", "n" },
        { "y", "l" }, { "z", "k" },
        { "", "" }
    };

    buf->data[0]        = '\0';
    for ( pName = skill_table[sn].name; *pName != '\0'; pName += length )
    {
        for ( iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++ )
        {
            if ( !str_prefix( syl_table[iSyl].old, pName ) )
            {
                buffer_strcat( buf, syl_table[iSyl].new );
                break;
            }
        }

        if ( length == 0 )
            length = 1;
    }

    bprintf( buf2, "$n utters the words, '%s'.", buf->data );
    bprintf( buf,  "$n utters the words, '%s'.", skill_table[sn].name );

    for ( rch = ch->in_room->people; rch; rch = rch->next_in_room )
    {
        if ( rch != ch )
            act( ch->ch_class==rch->ch_class ? buf->data : buf2->data, ch, NULL,
                rch, TO_VICT );
    }

    buffer_free( buf );
    buffer_free( buf2 );
    return;
}



/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_spell( int level, CHAR_DATA *victim, int dam_type )
{
    int save;

    save = 50 + ( victim->level - level - victim->saving_throw ) * 5;
    if (IS_AFFECTED(victim,AFF_BERSERK))
        save += victim->level/2;

    /* added by Rahl */
    switch ( check_immune( victim, dam_type ) )
    {
        case IS_IMMUNE:         return TRUE;
        case IS_RESISTANT:      save += 2;      break;
        case IS_VULNERABLE:     save -= 2;      break;
    }
   
    if (!IS_NPC( victim ) && class_table[victim->ch_class].fMana )
        save = 9 * save / 10;

    /* end stuff by Rahl */

    save = URANGE( 5, save, 95 );
    return number_percent( ) < save;
}

/* RT save for dispels */

bool saves_dispel( int dis_level, int spell_level, int duration)
{
    int save;
    
    if (duration == -1)
      spell_level += 5;  
      /* very hard to dispel permanent effects */

    save = 50 + (spell_level - dis_level) * 5;
    save = URANGE( 5, save, 95 );
    return number_percent( ) < save;
}

/* co-routine for dispel magic and cancellation */

bool check_dispel( int dis_level, CHAR_DATA *victim, int sn)
{
    AFFECT_DATA *af;

    if (is_affected(victim, sn))
    {
        for ( af = victim->affected; af != NULL; af = af->next )
        {
            if ( af->type == sn )
            {
                if (!saves_dispel(dis_level,af->level,af->duration))
                {
                    affect_strip(victim,sn);
                    if ( skill_table[sn].msg_off )
                    {
                        send_to_char( skill_table[sn].msg_off, victim );
                        send_to_char( "\n\r", victim );
                    }
                    return TRUE;
                }
                else
                    af->level--;
            }
        }
    }
    return FALSE;
}

/* for finding mana costs -- temporary version */
int mana_cost (CHAR_DATA *ch, int min_mana, int level)
{
    if (ch->level + 2 == level)
        return 1000;
    return UMAX(min_mana,(100/(2 + ch->level - level)));
}



/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name;

void do_cast( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    void *vo;
    int mana;
    int sn;
    int target;         /* added by Rahl */

    /*
     * Charmed NPC's can't cast spells, but others can.
     */
    if ( IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
        return; 

    target_name = one_argument( argument, arg1 );
    one_argument( target_name, arg2 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Cast which what where?\n\r", ch );
        return;
    }

    if ( ( sn = skill_lookup( arg1 ) ) < 0
    || ( !IS_NPC(ch) && ch->level < skill_table[sn].skill_level[ch->ch_class] ) )
    {
        send_to_char( "You don't know any spells of that name.\n\r", ch );
        return;
    }
  
    if ( ch->position < skill_table[sn].minimum_position )
    {
        send_to_char( "You can't concentrate enough.\n\r", ch );
        return;
    }

    if (ch->level + 2 == skill_table[sn].skill_level[ch->ch_class])
        mana = 50;
    else
        mana = UMAX(
            skill_table[sn].min_mana,
            100 / ( 2 + ch->level - skill_table[sn].skill_level[ch->ch_class] ) );

    /*
     * Locate targets.
     */
    victim      = NULL;
    obj         = NULL;
    vo          = NULL;
    target      = TARGET_NONE;          /* added by Rahl */      

    switch ( skill_table[sn].target )
    {
    default:
        bug( "Do_cast: bad target for sn %d.", sn );
        return;

    case TAR_IGNORE:
        break;

     /* new case by Rahl */
    case TAR_CHAR_OTHER:
        if ( arg2[0] == '\0' )
        {
            send_to_char( "Cast the spell on whom?\n\r", ch );
            return;
        }
        else
        {
            if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
            {
                send_to_char( "They aren't here.\n\r", ch );
                return;
            }
        }
      
        if ( ch == victim )
        {
            send_to_char( "You cannot cast this spell on yourself.\n\r",ch );
            return;
        }

        vo = (void *) victim;
        target = TARGET_CHAR;           /* added by Rahl */
        break;

    case TAR_CHAR_OFFENSIVE:
        if ( arg2[0] == '\0' )
        {
            if ( ( victim = ch->fighting ) == NULL )
            {
                send_to_char( "Cast the spell on whom?\n\r", ch );
                return;
            }
        }
        else
        {
            if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
            {
                send_to_char( "They aren't here.\n\r", ch );
                return;
            }
        }

        if ( ch == victim )
        {
            send_to_char( "You can't do that to yourself.\n\r", ch );
            return;
        }

        if ( !IS_NPC(ch) )
        {

            if (is_safe_spell(ch,victim,FALSE) && victim != ch)
            {
                send_to_char("Not on that target.\n\r",ch);
                return; 
            }
        }

        if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
        {
            send_to_char( "You can't do that on your own follower.\n\r",
                ch );
            return;
        }

        vo = (void *) victim;
        target = TARGET_CHAR;           /* added by Rahl */
        break;

    case TAR_CHAR_DEFENSIVE:
        if ( arg2[0] == '\0' )
        {
            victim = ch;
        }
        else
        {
            if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
            {
                send_to_char( "They aren't here.\n\r", ch );
                return;
            }
        }

        vo = (void *) victim;
        target = TARGET_CHAR;           /* added by Rahl */
        break;

    case TAR_CHAR_SELF:
        if ( arg2[0] != '\0' && !is_name( arg2, ch->name ) )
        {
            send_to_char( "You cannot cast this spell on another.\n\r", ch );
            return;
        }

        vo = (void *) ch;
        target = TARGET_CHAR;           /* added by Rahl */
        break;

    case TAR_OBJ_INV:
        if ( arg2[0] == '\0' )
        {
            send_to_char( "What should the spell be cast upon?\n\r", ch );
            return;
        }

        if ( ( obj = get_obj_carry( ch, arg2 ) ) == NULL )
        {
            send_to_char( "You are not carrying that.\n\r", ch );
            return;
        }

        vo = (void *) obj;
        target = TARGET_OBJ;            /* added by Rahl */
        break;

    /* added by Rahl */
    case TAR_OBJ_CHAR_OFF:
        if ( arg2[0] == '\0' )
        {
            if ( ( victim = ch->fighting ) == NULL )
            {
                send_to_char( "Cast the spell on whom or what?\n\r", ch );
                return;
            }
            target = TARGET_CHAR;
        }
        else if ( ( victim = get_char_room( ch, target_name ) ) != NULL )
        {
            target = TARGET_CHAR;
        }
        
        if ( target == TARGET_CHAR ) /* check the sanity of the attack */
        {
            if ( is_safe_spell( ch, victim, FALSE ) && victim != ch )
            {
                send_to_char( "Not on that target.\n\r", ch );
                return;
            }
            
            if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
            {
                send_to_char( "You can't do that to your own follower.\n\r", ch );
                return;
            }

            vo = (void *) victim;
        }
        else if ( ( obj = get_obj_here( ch, target_name ) ) != NULL )
        {
            vo = (void *) obj;
            target = TARGET_OBJ;
        }
        else
        {
            send_to_char( "You don't see that here.\n\r", ch );
            return;
        }
        break;

    case TAR_OBJ_CHAR_DEF:
        if ( arg2[0] == '\0' )
        {
            vo = (void *) ch;
            target = TARGET_CHAR;
        }
        else if ( ( victim = get_char_room( ch, target_name ) ) != NULL )
        {
            vo = (void *) victim;       
            target = TARGET_CHAR;
        }
        else if ( ( obj = get_obj_carry( ch, target_name ) ) != NULL )
        {
            vo = (void *) obj;
            target = TARGET_OBJ;
        }
        else
        {
           send_to_char( "You don't see that here.\n\r", ch );
           return;
        }
        break;
        /* end stuff by Rahl */
    }

    /*      
     * I'm going to change this so that anyone can't cast more than they
     * have. Not sure what this will do elsewhere. -- Rahl
     */
    if ( /* !IS_NPC(ch) && */ ch->mana < mana )
    {
        send_to_char( "You don't have enough mana.\n\r", ch );
        return;
    }
      
    if ( str_cmp( skill_table[sn].name, "ventriloquate" ) )
        say_spell( ch, sn );
      
    WAIT_STATE( ch, skill_table[sn].beats );
      
    if ( !IS_NPC(ch) && number_percent( ) > ch->pcdata->learned[sn] )
    {
        send_to_char( "You lost your concentration.\n\r", ch );
        check_improve(ch,sn,FALSE,1);
        ch->mana -= mana / 2;
    }
    else
    {
        ch->mana -= mana;
        (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vo, target );
        check_improve(ch,sn,TRUE,1);
    }

    if ( ( skill_table[sn].target == TAR_CHAR_OFFENSIVE
        /* next 2 lines added by Rahl */
        || ( skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR ) )
    &&   victim != ch
    &&   victim->master != ch)
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        for ( vch = ch->in_room->people; vch; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( victim == vch && victim->fighting == NULL )
            {
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return;
}



/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    void *vo;
    int target = TARGET_NONE;

    if ( sn <= 0 )
        return;

    if ( sn >= MAX_SKILL || skill_table[sn].spell_fun == 0 )
    {
        bug( "Obj_cast_spell: bad sn %d.", sn );
        return;
    }

    switch ( skill_table[sn].target )
    {
    default:
        bug( "Obj_cast_spell: bad target for sn %d.", sn );
        return;

    case TAR_IGNORE:
        vo = NULL;
        break;

    case TAR_CHAR_OFFENSIVE:
        if ( victim == NULL )
            victim = ch->fighting;
        if ( victim == NULL )
        {
            send_to_char( "You can't do that.\n\r", ch );
            return;
        }
        if (is_safe_spell(ch,victim,FALSE) && ch != victim)
        {
            send_to_char("Something isn't right...\n\r",ch);
            return;
        }
        vo = (void *) victim;
        target = TARGET_CHAR;   /* added by Rahl */
        break;

    case TAR_CHAR_DEFENSIVE:
        if ( victim == NULL )
            victim = ch;
        vo = (void *) victim;
        target = TARGET_CHAR;   /* added by Rahl */
        break;

    case TAR_CHAR_SELF:
        vo = (void *) ch;
        target = TARGET_CHAR;   /* added by Rahl */
        break;

    case TAR_OBJ_INV:
        if ( obj == NULL )
        {
            send_to_char( "You can't do that.\n\r", ch );
            return;
        }
        vo = (void *) obj;
        target = TARGET_OBJ;    /* added by Rahl */
        break;

    /* begin stuff by Rahl */
    case TAR_OBJ_CHAR_OFF:
        if ( victim == NULL && obj == NULL )
            if ( ch->fighting != NULL )
                victim = ch->fighting;
            else
            {
                send_to_char( "You can't do that.\n\r", ch );
                return;
            }

            if ( victim != NULL )
            {
                if ( is_safe_spell( ch, victim, FALSE ) && ch != victim )
                {
                    send_to_char( "Something isn't right...\n\r", ch );
                    return;
                }
        
                vo = (void *) victim;
                target = TARGET_CHAR;
            }
            else
            {
                vo = (void *) obj;
                target = TARGET_OBJ;
            }
        break;

    case TAR_OBJ_CHAR_DEF:
        if ( victim == NULL && obj == NULL )
        {
            vo = (void *) ch;
            target = TARGET_CHAR;
        }
        else if ( victim != NULL )
        {
            vo = (void *) victim;
            target = TARGET_CHAR;
        }
        else
        {
            vo = (void *) obj;  
            target = TARGET_OBJ;
        }
        break;
        
        /* end stuff by Rahl */

    }

    target_name = "";
    (*skill_table[sn].spell_fun) ( sn, level, ch, vo, target );

    

    if ( ( skill_table[sn].target == TAR_CHAR_OFFENSIVE
        /* next 2 lines added by Rahl */
        || ( skill_table[sn].target == TAR_OBJ_CHAR_OFF && target ==
                TARGET_CHAR ))
    &&   victim != ch
    &&   victim->master != ch )
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        for ( vch = ch->in_room->people; vch; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( victim == vch && victim->fighting == NULL )
            {
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return;
}



/*
 * Spell functions.
 */
void spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 10 );
    if ( saves_spell( level, victim, DAM_ACID ) )
        dam /= 2;
    act("$n grins as acid erupts from $s hand.",ch,NULL,NULL,TO_ROOM);
    damage( ch, victim, dam, sn,DAM_ACID, TRUE );
    return;
}



void spell_armor( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already protected.\n\r",ch);
        else
          act("$N is already armored.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.modifier  = -20;
    af.location  = APPLY_AC;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel someone protecting you.\n\r", victim );
    if ( ch != victim )
        act("$N is protected by your magic.",ch,NULL,victim,TO_CHAR);
    return;
}



void spell_bless( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    /* begin stuff added by Rahl */
    OBJ_DATA *obj;

    /* deal with the object case first */       
    if ( target == TARGET_OBJ )
    {
        obj = (OBJ_DATA *) vo;
        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        {
            act( "$p is already blessed.", ch, obj, NULL, TO_CHAR );
            return;
        }
        
        if ( IS_OBJ_STAT( obj, ITEM_EVIL ) )
        {
            AFFECT_DATA *paf;
        
            paf = affect_find( obj->affected, gsn_curse );
            if ( !saves_dispel( level, paf != NULL ? paf->level : obj->level, 0 ) ) 
            {
                if ( paf != NULL )
                    affect_remove_obj( obj, paf );
                act( "$p glows a pale blue.", ch, obj, NULL, TO_ALL );
                REMOVE_BIT( obj->extra_flags, ITEM_EVIL );
                return;
            }
            else
            {
                act( "The evil of $p is too powerful for you to overcome.",
                        ch, obj, NULL, TO_CHAR );
                return;
            }
        }
        
        af.where         = TO_OBJECT;
        af.type          = sn;
        af.level         = level;
        af.duration      = 6 + level;
        af.location      = APPLY_SAVING_SPELL;
        af.modifier      = -1;
        af.bitvector     = ITEM_BLESS;
        affect_to_obj( obj, &af );
        
        act( "$p glows with a holy aura.", ch, obj, NULL, TO_ALL );

        if ( obj->wear_loc != WEAR_NONE )
            ch->saving_throw -= 1;
        return;
    }

    /* character target */
    victim = (CHAR_DATA *) vo;
/* end stuff by Rahl */

    if ( victim->position == POS_FIGHTING || is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already blessed.\n\r",ch);
        else
          act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;          /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = 6+level;
    af.location  = APPLY_HITROLL;
    af.modifier  = level / 8;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = 0 - level / 8;
    affect_to_char( victim, &af );
    send_to_char( "You feel righteous.\n\r", victim );
    if ( ch != victim )
        act("You grant $N the favor of your god.",ch,NULL,victim,TO_CHAR);
    return;
}



void spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_BLIND) 
        || saves_spell( level, victim, DAM_OTHER ) ) // DAM_OTHER added
        return;                                      // by Rahl         

    af.where     = TO_AFFECTS;  /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4;
    af.duration  = 1+level;
    af.bitvector = AFF_BLIND;
    affect_to_char( victim, &af );
    send_to_char( "You are blinded!\n\r", victim );
    act("$n appears to be blinded.",victim,NULL,NULL,TO_ROOM);
    return;
}



void spell_burning_hands( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
/*    static const sh_int dam_each[] = 
    {
         0,
         0,  0,  0,  0, 14,     17, 20, 23, 26, 29,
        29, 29, 30, 30, 31,     31, 32, 32, 33, 33,
        34, 34, 35, 35, 36,     36, 37, 37, 38, 38,
        39, 39, 40, 40, 41,     41, 42, 42, 43, 43,
        44, 44, 45, 45, 46,     46, 47, 47, 48, 48
    };
*/
    int dam;

/*    level     = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
*/
/* New damage by Rahl */
        dam=dice(level,4);

    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;                       
    act("Jets of flame erupt from $n's hands.",ch,NULL,NULL,TO_ROOM);
    damage( ch, victim, dam, sn, DAM_FIRE, TRUE );
    return;
}



void spell_call_lightning( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    if ( !IS_OUTSIDE(ch) )
    {
        send_to_char( "You must be out of doors.\n\r", ch );
        return;
    }

    if ( weather_info.sky < SKY_RAINING )
    {
        send_to_char( "You need bad weather.\n\r", ch );
        return;
    }

    dam = dice(level/2, 8);

    send_to_char( "The gods' lightning strikes your foes!\n\r", ch );
    act( "$n calls the gods' lightning to strike $s foes!",
        ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ( vch->in_room == ch->in_room )
        {
            if ( vch != ch && ( IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch) ) )
                damage( ch, vch, saves_spell( level,vch, DAM_LIGHTNING ) ?
                        dam / 2 : dam, sn, DAM_LIGHTNING, TRUE );
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area
        &&   IS_OUTSIDE(vch)
        &&   IS_AWAKE(vch) )
            send_to_char( "Lightning flashes in the sky.\n\r", vch );
    }

    return;
}

/* RT calm spell stops all fighting in the room */

void spell_calm( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *vch;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;    
    int chance;
    AFFECT_DATA af;

    /* get sum of all mobile levels in the room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->position == POS_FIGHTING)
        {
            count++;
            if (IS_NPC(vch))
              mlevel += vch->level;
            else
              mlevel += vch->level/2;
            high_level = UMAX(high_level,vch->level);
        }
    }

    /* compute chance of stopping combat */
    chance = 4 * level - high_level + 2 * count;

    if (IS_IMMORTAL(ch)) /* always works */
      mlevel = 0;

    if (number_range(0, chance) >= mlevel)  /* hard to stop large fights */
    {
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (IS_NPC(vch) && (IS_SET(vch->imm_flags,IMM_MAGIC) ||
                                IS_SET(vch->act,ACT_UNDEAD)))
              return;

            if (IS_AFFECTED(vch,AFF_CALM) || IS_AFFECTED(vch,AFF_BERSERK)
            ||  is_affected(vch,skill_lookup("frenzy")))
              return;
            
            send_to_char("A wave of calm passes over you.\n\r",vch);

            if (vch->fighting || vch->position == POS_FIGHTING)
              stop_fighting(vch,FALSE);

            af.where = TO_AFFECTS;
            af.type = sn;
            af.level = level;
            af.duration = level/4;
            af.location = APPLY_HITROLL;
            if (!IS_NPC(vch))
              af.modifier = -5;
            else
              af.modifier = -2;
            af.bitvector = AFF_CALM;
            affect_to_char(vch,&af);

            af.location = APPLY_DAMROLL;
            affect_to_char(vch,&af);
        }
    }
}

void spell_cancellation( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
 
    level += 2;

    if ((!IS_NPC(ch) && IS_NPC(victim) && 
         !(IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) ) ||
        (IS_NPC(ch) && !IS_NPC(victim)) )
    {
        send_to_char("You failed, try dispel magic.\n\r",ch);
        return;
    }

    /* unlike dispel magic, the victim gets NO save */
 
    /* begin running through the spells */
 
    if (check_dispel(level,victim,skill_lookup("armor")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("bless")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("blindness")))
    {
        found = TRUE;
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
    }

    if ( check_dispel( level, victim, skill_lookup( "blink" ) ) )
    {
        found = TRUE;
        act( "$n blinks back into existence.", victim, NULL, NULL, TO_ROOM );
    }

    if (check_dispel(level,victim,skill_lookup("calm")))
    {
        found = TRUE;
        act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
    }
 
    if (check_dispel(level,victim,skill_lookup("change sex")))
    {
        found = TRUE;
        act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
        send_to_char("You feel more like yourself again.\n\r",victim);
    }
 
    if (check_dispel(level,victim,skill_lookup("charm person")))
    {
        found = TRUE;
        act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
    }
 
    if (check_dispel(level,victim,skill_lookup("chill touch")))
    {
        found = TRUE;
        act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
    }
 
    if (check_dispel(level,victim,skill_lookup("curse")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect evil")))
        found = TRUE;

    /* detect good added by Rahl */
    if ( check_dispel( level, victim, skill_lookup( "detect good" ) ) )
        found = TRUE; 

    if (check_dispel(level,victim,skill_lookup("detect hidden")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect invis")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect hidden")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect magic")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("faerie fire")))
    {
        act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("fly")))
    {
        act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("frenzy")))
    {
        act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("giant strength")))
    {
        act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("haste")))
    {
        act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("infravision")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("invis")))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("mass invis")))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("pass door")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("protection evil")))
        found = TRUE;

    /* protection good added by Rahl */
    if ( check_dispel( level, victim, skill_lookup( "protection good" ) ) )
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("sanctuary")))
    {
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("shield")))
    {
        act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("sleep")))
        found = TRUE;
 
    /* slow added by Rahl */
    if ( check_dispel( level, victim, skill_lookup( "slow" ) ) )
    {
        act( "$n is no longer moving so slowly.", victim, NULL, NULL, TO_ROOM );
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("stone skin")))
    {
        act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("weaken")))
    {
        act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
    if (check_dispel(level,victim,skill_lookup("web")))
    {
        act("The webs around $n disolve.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (found)
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);
}

void spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    damage( ch, (CHAR_DATA *) vo, dice(1, 8) + level / 3, sn, DAM_HARM, TRUE );
    return;
}



void spell_cause_critical( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    damage( ch, (CHAR_DATA *) vo, dice(3, 8) + level - 6, sn, DAM_HARM, TRUE );
    return;
}



void spell_cause_serious( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    damage( ch, (CHAR_DATA *) vo, dice(2, 8) + level / 2, sn, DAM_HARM, TRUE );
    return;
}

void spell_chain_lightning(int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *tmp_vict,*last_vict,*next_vict;
    bool found;
    int dam;

    /* first strike */

    act("A lightning bolt leaps from $n's hand and arcs to $N.",
        ch,NULL,victim,TO_ROOM);
    act("A lightning bolt leaps from your hand and arcs to $N.",
        ch,NULL,victim,TO_CHAR);
    act("A lightning bolt leaps from $n's hand and hits you!",
        ch,NULL,victim,TO_VICT);  

    dam = dice(level,6);
    if (saves_spell(level,victim, DAM_LIGHTNING))
        dam /= 3;
    damage(ch,victim,dam,sn,DAM_LIGHTNING, TRUE);
    last_vict = victim;
    level -= 4;   /* decrement damage */

    /* new targets */
    while (level > 0)
    {
        found = FALSE;
        for (tmp_vict = ch->in_room->people; 
             tmp_vict != NULL; 
             tmp_vict = next_vict) 
        {
          next_vict = tmp_vict->next_in_room;
          if (!is_safe_spell(ch,tmp_vict,TRUE) && tmp_vict != last_vict)
          {
            found = TRUE;
            last_vict = tmp_vict;
            act("The bolt arcs to $n!",tmp_vict,NULL,NULL,TO_ROOM);
            act("The bolt hits you!",tmp_vict,NULL,NULL,TO_CHAR);
            dam = dice(level,6);
            if (saves_spell(level,tmp_vict, DAM_LIGHTNING))
                dam /= 3;
            damage(ch,tmp_vict,dam,sn,DAM_LIGHTNING, TRUE );
            level -= 4;  /* decrement damage */
          }
        }   /* end target searching loop */
        
        if (!found) /* no target found, hit the caster */
        {
          if (ch == NULL)
            return;

          if (last_vict == ch) /* no double hits */
          {
            act("The bolt seems to have fizzled out.",ch,NULL,NULL,TO_ROOM);
            act("The bolt grounds out through your body.",
                ch,NULL,NULL,TO_CHAR);
            return;
          }
        
          last_vict = ch;
          act("The bolt arcs to $n...whoops!",ch,NULL,NULL,TO_ROOM);
          send_to_char("You are struck by your own lightning!\n\r",ch);
          dam = dice(level,6);
          if (saves_spell(level,ch, DAM_LIGHTNING))
            dam /= 3;
          damage(ch,ch,dam,sn,DAM_LIGHTNING, TRUE );
          level -= 4;  /* decrement damage */
          if (ch == NULL) 
            return;
        }
    /* now go back and find more targets */
    }
}
          

void spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ))
    {
        if (victim == ch)
          send_to_char("You've already been changed.\n\r",ch);
        else
          act("$N has already had $s(?) sex changed.",ch,NULL,victim,TO_CHAR);
        return;
    }
    if (saves_spell(level , victim, DAM_OTHER))
        return; 
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
    af.location  = APPLY_SEX;
    do
    {
        af.modifier  = number_range( 0, 2 ) - victim->sex;
    }
    while ( af.modifier == 0 );
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel different.\n\r", victim );
    act("$n doesn't look like $mself anymore...",victim,NULL,NULL,TO_ROOM);
    return;
}



void spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( victim == ch )
    {
        send_to_char( "You like yourself even better!\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(victim, AFF_CHARM)
    ||   IS_AFFECTED(ch, AFF_CHARM)
    ||   level < victim->level
    ||   IS_SET(victim->imm_flags,IMM_CHARM)
    ||   saves_spell( level, victim, DAM_MENTAL ) ) //instead of DAM_CHARM
        return;


    if (IS_SET(victim->in_room->room_flags,ROOM_LAW))
    {
        send_to_char(
            "The mayor does not allow charming in the city limits.\n\r",ch);
        return;
    }

    if ( victim->master )
        stop_follower( victim );
    add_follower( victim, ch );
    victim->leader = ch;
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 4 );
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( victim, &af );
    act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT );
    if ( ch != victim )
        act("$N looks at you with adoring eyes.",ch,NULL,victim,TO_CHAR);
    return;
}



void spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
/*    static const sh_int dam_each[] = 
    {
         0,
         0,  0,  6,  7,  8,      9, 12, 13, 13, 13,
        14, 14, 14, 15, 15,     15, 16, 16, 16, 17,
        17, 17, 18, 18, 18,     19, 19, 19, 20, 20,
        20, 21, 21, 21, 22,     22, 22, 23, 23, 23,
        24, 24, 24, 25, 25,     25, 26, 26, 26, 27
    };
*/
    AFFECT_DATA af;
    int dam;
/*
    level       = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
*/

/* New damage by Rahl */
    dam=dice(level,3);

    if ( !saves_spell( level, victim, DAM_COLD ) )
    {
        act("$n turns blue and shivers.",victim,NULL,NULL,TO_ROOM);
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 6;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_join( victim, &af );
    }
    else
    {
        dam /= 2;
    }

    damage( ch, victim, dam, sn, DAM_COLD, TRUE );
    return;
}



void spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
/*    static const sh_int dam_each[] = 
    {
         0,
         0,  0,  0,  0,  0,      0,  0,  0,  0,  0,
        30, 35, 40, 45, 50,     55, 55, 55, 56, 57,
        58, 58, 59, 60, 61,     61, 62, 63, 64, 64,
        65, 66, 67, 67, 68,     69, 70, 70, 71, 72,
        73, 73, 74, 75, 76,     76, 77, 78, 79, 79
    };
*/
    int dam;
/*
    level       = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2,  dam_each[level] * 2 );
*/
/* New damage by Rahl */
        dam=dice(level,7);

    if ( saves_spell( level, victim, DAM_LIGHT ) )
        dam /= 2;
    else 
        spell_blindness(skill_lookup("blindness"),level/2,ch,(void *)
                victim, TARGET_CHAR);

    act("A rainbow shoots from $n's hand.",ch,NULL,NULL,TO_ROOM);
    damage( ch, victim, dam, sn, DAM_LIGHT, TRUE );
    return;
}



void spell_continual_light( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    OBJ_DATA *light;
 
/* stuff added by Rahl */
    if ( target_name[0] != '\0' ) /* do a glow on some object */
    {
        light = get_obj_carry( ch, target_name );

        if ( light == NULL )
        {
            send_to_char( "You don't see that here.\n\r", ch );
            return;
        }
        
        if ( IS_OBJ_STAT( light, ITEM_GLOW ) )
        {
            act( "$p is already glowing.", ch, light, NULL, TO_CHAR );
            return;
        }
        
        SET_BIT( light->extra_flags, ITEM_GLOW );
        act( "$p glows with a white light.", ch, light, NULL, TO_ALL );
        return;
    }
/* end stuff by Rahl */

    light = create_object( get_obj_index( OBJ_VNUM_LIGHT_BALL ), 0 );
    obj_to_room( light, ch->in_room );
    act( "$n twiddles $s thumbs and $p appears.",   ch, light, NULL, TO_ROOM );
    act( "You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR );
    return;
}



void spell_control_weather( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    if ( !str_cmp( target_name, "better" ) )
        weather_info.change += dice( level / 3, 4 );
    else if ( !str_cmp( target_name, "worse" ) )
        weather_info.change -= dice( level / 3, 4 );
    else
        send_to_char ("Do you want it to get better or worse?\n\r", ch );

    send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *mushroom;

    mushroom = create_object( get_obj_index( OBJ_VNUM_MUSHROOM ), 0 );
    mushroom->value[0] = 5 + level;
    obj_to_room( mushroom, ch->in_room );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR );
    return;
}


/* create rose added by Rahl */
void spell_create_rose( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *rose;

    rose = create_object( get_obj_index( OBJ_VNUM_ROSE ), 0 );
    act( "$n has created a beautiful red rose.", ch, rose, NULL, TO_ROOM );
    send_to_char( "You create a beautiful red rose.\n\r", ch );
    obj_to_char( rose, ch );
    return;
}


void spell_create_spring( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *spring;

    spring = create_object( get_obj_index( OBJ_VNUM_SPRING ), 0 );
    spring->timer = level;
    obj_to_room( spring, ch->in_room );
    act( "$p flows from the ground.", ch, spring, NULL, TO_ROOM );
    act( "$p flows from the ground.", ch, spring, NULL, TO_CHAR );
    return;
}


void spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int water;

    if ( obj->item_type != ITEM_DRINK_CON )
    {
        send_to_char( "It is unable to hold water.\n\r", ch );
        return;
    }

    if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
    {
        send_to_char( "It contains some other liquid.\n\r", ch );
        return;
    }

    water = UMIN(
                level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
                obj->value[0] - obj->value[1]
                );
  
    if ( water > 0 )
    {
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;
        if ( !is_name( "water", obj->name ) )
        {
            BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

            bprintf( buf, "%s water", obj->name );
            free_string( obj->name );
            obj->name = str_dup( buf->data );
            buffer_free( buf );
        }
        act( "$p is filled.", ch, obj, NULL, TO_CHAR );
    }

    return;
}



void spell_cure_blindness( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( !is_affected( victim, gsn_blindness ) )
    {
        if (victim == ch)
          send_to_char("You aren't blind.\n\r",ch);
        else
          act("$N doesn't appear to be blinded.",ch,NULL,victim,TO_CHAR);
        return;
    }
 
    if (check_dispel(level,victim,gsn_blindness))
    {
        send_to_char( "Your vision returns!\n\r", victim );
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}



void spell_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    heal = dice(3, 8) + level - 6;
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}

/* RT added to cure plague */
void spell_cure_disease( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( !is_affected( victim, gsn_plague ) )
    {
        if (victim == ch)
          send_to_char("You aren't ill.\n\r",ch);
        else
          act("$N doesn't appear to be diseased.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    if (check_dispel(level,victim,gsn_plague))
    {
        send_to_char("Your sores vanish.\n\r",victim);
        act("$n looks relieved as $s sores vanish.",victim,NULL,NULL,TO_ROOM);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}



void spell_cure_light( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    heal = dice(1, 8) + level / 3;
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
 
    if ( !is_affected( victim, gsn_poison ) )
    {
        if (victim == ch)
          send_to_char("You aren't poisoned.\n\r",ch);
        else
          act("$N doesn't appear to be poisoned.",ch,NULL,victim,TO_CHAR);
        return;
    }
 
    if (check_dispel(level,victim,gsn_poison))
    {
        send_to_char("A warm feeling runs through your body.\n\r",victim);
        act("$n looks much better.",victim,NULL,NULL,TO_ROOM);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}

void spell_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    heal = dice(2, 8) + level /2 ;
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_curse( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    OBJ_DATA *obj;      /* added by Rahl */

    /* added by Rahl */
    /* deal with the object case first */
    if ( target == TARGET_OBJ )
    {
        obj = (OBJ_DATA *) vo;
        if ( IS_OBJ_STAT( obj, ITEM_EVIL ) )
        {
            act( "$p is already filled with evil.", ch, obj, NULL, TO_CHAR );
            return;
        }
        
        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        {
            AFFECT_DATA *paf;

            paf = affect_find( obj->affected, skill_lookup( "bless" ) );
            if ( !saves_dispel( level, paf != NULL ? paf->level :
                obj->level, 0 ) )
            {
                if ( paf != NULL )
                    affect_remove_obj( obj, paf );
                act( "$p glows with a red aura.", ch, obj, NULL, TO_ALL );
                REMOVE_BIT( obj->extra_flags, ITEM_BLESS );
                return;
            }
            else
            {
                act( "The holy aura of $p is too powerful for you to overcome.", 
                    ch, obj, NULL, TO_CHAR );
                return;
            }
        }
        
        af.where        = TO_OBJECT;
        af.type         = sn;
        af.level        = level;
        af.duration     = 2 * level;
        af.location     = APPLY_SAVING_SPELL;
        af.modifier     = +1;
        af.bitvector    = ITEM_EVIL;
        affect_to_obj( obj, &af );

        act( "$p glows with a malevolent aura.", ch, obj, NULL, TO_ALL );

        if ( obj->wear_loc != WEAR_NONE )
            ch->saving_throw += 1;
        return;
    }

    /* character curses */ 
/* end most of the stuff by Rahl */

    if ( IS_AFFECTED(victim, AFF_CURSE) || saves_spell( level, victim,
                DAM_NEGATIVE ) )
        return;
        
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2*level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = level / 8;
    affect_to_char( victim, &af );

    send_to_char( "You feel unclean.\n\r", victim );
    if ( ch != victim )
        act("$N looks very uncomfortable.",ch,NULL,victim,TO_CHAR);
    return;
}

/* RT replacement demonfire spell */

void spell_demonfire(int sn, int level, CHAR_DATA *ch, void *vo, int
        target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC(ch) && !IS_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The demons turn upon you!\n\r",ch);
    }

    ch->alignment = UMAX(-1000, ch->alignment - 50);

    if (victim != ch)
    {
        act("$n calls forth the demons of Hell upon $N!",
            ch,NULL,victim,TO_ROOM);
        act("$n has assailed you with the demons of Hell!",
            ch,NULL,victim,TO_VICT);
        send_to_char("You conjure forth the demons of hell!\n\r",ch);
    }
    dam = dice( level, 10 );
    if ( saves_spell( level, victim, DAM_NEGATIVE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_NEGATIVE, TRUE );
//    spell_curse( gsn_curse, 3*level/4, ch, (void *) victim, TARGET_CHAR );
}


void spell_detect_evil( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_EVIL) )
    {
        if (victim == ch)
          send_to_char("You can already sense evil.\n\r",ch);
        else
          act("$N can already detect evil.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS; /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}

/* added by Rahl */
void spell_detect_good( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED( victim, AFF_DETECT_GOOD ) )
    {
        if ( victim == ch )
            send_to_char( "You can already sense good.\n\r", ch );
        else
            act( "$N can already detect good.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_GOOD;
    affect_to_char( victim, &af );

    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}


void spell_detect_hidden( int sn, int level, CHAR_DATA *ch, void *vo, int
target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (chaos)
    {
        send_to_char( "Your killer is right behind you!\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(victim, AFF_DETECT_HIDDEN) )
    {
        if (victim == ch)
          send_to_char("You are already as alert as you can be. \n\r",ch);
        else
          act("$N can already sense hidden lifeforms.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char( victim, &af );
    send_to_char( "Your awareness improves.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_invis( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (chaos)
    {
        send_to_char( "You killer is right behind you!\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(victim, AFF_DETECT_INVIS) )
    {
        if (victim == ch)
          send_to_char("You can already see invisible.\n\r",ch);
        else
          act("$N can already see invisible things.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS; /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_magic( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_MAGIC) )
    {
        if (victim == ch)
          send_to_char("You can already sense magical auras.\n\r",ch);
        else
          act("$N can already detect magic.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;

    if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD )
    {
        if ( obj->value[3] != 0 )
            send_to_char( "You smell poisonous fumes.\n\r", ch );
        else
            send_to_char( "It looks delicious.\n\r", ch );
    }
    else
    {
        send_to_char( "It doesn't look poisoned.\n\r", ch );
    }

    return;
}



void spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
  
    if ( !IS_NPC(ch) && IS_EVIL(ch) )
        victim = ch;
  
    if ( IS_GOOD(victim) )
    {
        act( "The gods protect $N.", ch, NULL, victim, TO_ROOM );
        return;
    }

    if ( IS_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if (victim->hit > (ch->level * 4))
      dam = dice( level, 4 );
    else
      dam = UMAX(victim->hit, dice(level,4));
    if ( saves_spell( level, victim, DAM_HOLY ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_HOLY, TRUE );
    return;
}

/* added by Rahl */
void spell_dispel_good( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC( ch ) && IS_GOOD( ch ) )
        victim = ch;

    if ( IS_EVIL( victim ) )
    {
        act( "$N is protected by $S evil.", ch, NULL, victim, TO_ROOM );
        return;
    }

    if ( IS_NEUTRAL( victim ) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( victim->hit > ( ch->level * 4 ) )
        dam = dice ( level, 4 );
    else
        dam = UMAX( victim->hit, dice( level, 4 ) );
    if ( saves_spell( level, victim, DAM_NEGATIVE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_NEGATIVE, TRUE );
    return;
}



/* modified for enhanced use */
void spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;

    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char( "You feel a brief tingling sensation.\n\r",victim);
        send_to_char( "You failed.\n\r", ch);
        return;
    }

    /* begin running through the spells */ 

    if (check_dispel(level,victim,skill_lookup("armor")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("bless")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("blindness")))
    {
        found = TRUE;
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
    }
 
    if ( check_dispel( level, victim, skill_lookup( "blink" ) ) )
    {
        found = TRUE;
        act( "$n blinks back into existence.", victim, NULL, NULL, TO_ROOM );
    }

    if (check_dispel(level,victim,skill_lookup("calm")))
    {
        found = TRUE;
        act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
    }
 
    if (check_dispel(level,victim,skill_lookup("change sex")))
    {
        found = TRUE;
        act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
    }
 
    if (check_dispel(level,victim,skill_lookup("charm person")))
    {
        found = TRUE;
        act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
    }
 
    if (check_dispel(level,victim,skill_lookup("chill touch")))
    {
        found = TRUE;
        act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
    }
 
    if (check_dispel(level,victim,skill_lookup("curse")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect evil")))
        found = TRUE;

    /* detect good added by Rahl */
    if ( check_dispel( level, victim, skill_lookup( "detect good" ) ) )
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect hidden")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect invis")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect hidden")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect magic")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("faerie fire")))
    {
        act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("fly")))
    {
        act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("frenzy")))
    {
        act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("giant strength")))
    {
        act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("haste")))
    {
        act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("infravision")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("invis")))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("mass invis")))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("pass door")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("protection evil")))
        found = TRUE;

    /* protection good added by Rahl */
    if ( check_dispel( level, victim, skill_lookup( "protection good" ) ) )
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("sanctuary")))
    {
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (IS_AFFECTED(victim,AFF_SANCTUARY) 
        && !saves_dispel(level, victim->level,-1)
        && !is_affected(victim,skill_lookup("sanctuary")))
    {
        REMOVE_BIT(victim->affected_by,AFF_SANCTUARY);
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("shield")))
    {
        act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("sleep")))
        found = TRUE;
 
    /* slow added by Rahl */
    if ( check_dispel( level, victim, skill_lookup( "slow" ) ) )
    {
        act( "$n is no longer moving so slowly.", victim, NULL, NULL, TO_ROOM );
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("stone skin")))
    {
        act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("weaken")))
    {
        act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
    if (check_dispel(level,victim,skill_lookup("web")))
    {
        act("The webs around $n disolve.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (found)
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);
}

void spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    send_to_char( "The earth trembles beneath your feet!\n\r", ch );
    act( "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ( vch->in_room == ch->in_room )
        {
            if ( vch != ch && !is_safe_spell(ch,vch,TRUE))
                if (IS_AFFECTED(vch,AFF_FLYING))
                    damage(ch,vch,0,sn,DAM_BASH, TRUE );
                else
                    damage( ch, vch, level + dice(2, 8), sn, DAM_BASH, TRUE );
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area )
            send_to_char( "The earth trembles and shivers.\n\r", vch );
    }

    return;
}

void spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf; 
    int result, fail;
    int ac_bonus, added;
    bool ac_found = FALSE;

    if (obj->item_type != ITEM_ARMOR)
    {
        send_to_char("That isn't an armor.\n\r",ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The item must be carried to be enchanted.\n\r",ch);
        return;
    }

    /* this means they have no bonus */
    ac_bonus = 0;
    fail = 25;  /* base 25% chance of failure */

    /* find the bonuses */

    if (!obj->enchanted)
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_AC )
            {
                ac_bonus = paf->modifier;
                ac_found = TRUE;
                fail += 5 * (ac_bonus * ac_bonus);
            }

            else  /* things get a little harder */
                fail += 20;
        }
 
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location == APPLY_AC )
        {
            ac_bonus = paf->modifier;
            ac_found = TRUE;
            fail += 5 * (ac_bonus * ac_bonus);
        }

        else /* things get a little harder */
            fail += 20;
    }

    /* apply other modifiers */
    fail -= level;

    if (IS_OBJ_STAT(obj,ITEM_BLESS))
        fail -= 15;
    if (IS_OBJ_STAT(obj,ITEM_GLOW))
        fail -= 5;

    fail = URANGE(5,fail,95);

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5))  /* item destroyed */
    {
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_CHAR);
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
        return;
    }

    if (result < (fail / 2)) /* item disenchanted */
    {
        AFFECT_DATA *paf_next;

        act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);
        obj->enchanted = TRUE;

        /* remove all affects */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next; 
            paf->next = affect_free;
            affect_free = paf;
        }
        obj->affected = NULL;

        /* clear all flags */
        obj->extra_flags = 0;
        return;
    }

    if ( result <= fail )  /* failed, no bad result */
    {
        send_to_char("Nothing seemed to happen.\n\r",ch);
        return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *af_new;
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

            af_new->where       = paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }

    if (result <= (100 - level/5))  /* success! */
    {
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR);
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        added = -1;
    }
    
    else  /* exceptional enchant */
    {
        act("$p glows a brillant gold!",ch,obj,NULL,TO_CHAR);
        act("$p glows a brillant gold!",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags,ITEM_MAGIC);
        SET_BIT(obj->extra_flags,ITEM_GLOW);
        added = -2;
    }
                
    /* now add the enchantments */ 

    if (obj->level < LEVEL_HERO)
        obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (ac_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if ( paf->location == APPLY_AC)
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX(paf->level,level);
            }
        }
    }
    else /* add a new affect */
    {
        if (affect_free == NULL)
            paf = alloc_perm(sizeof(*paf));
        else
        {
            paf = affect_free;
            affect_free = affect_free->next;
        }

        paf->where      = TO_OBJECT;
        paf->type       = sn;
        paf->level      = level;
        paf->duration   = -1;
        paf->location   = APPLY_AC;
        paf->modifier   =  added;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }

}




void spell_enchant_weapon( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf; 
    int result, fail;
    int hit_bonus, dam_bonus, added;
    bool hit_found = FALSE, dam_found = FALSE;

    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("That isn't a weapon.\n\r",ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The item must be carried to be enchanted.\n\r",ch);
        return;
    }

    /* this means they have no bonus */
    hit_bonus = 0;
    dam_bonus = 0;
    fail = 25;  /* base 25% chance of failure */

    /* find the bonuses */

    if (!obj->enchanted)
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_HITROLL )
            {
                hit_bonus = paf->modifier;
                hit_found = TRUE;
                fail += 2 * (hit_bonus * hit_bonus);
            }

            else if (paf->location == APPLY_DAMROLL )
            {
                dam_bonus = paf->modifier;
                dam_found = TRUE;
                fail += 2 * (dam_bonus * dam_bonus);
            }

            else  /* things get a little harder */
                fail += 25;
        }
 
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location == APPLY_HITROLL )
        {
            hit_bonus = paf->modifier;
            hit_found = TRUE;
            fail += 2 * (hit_bonus * hit_bonus);
        }

        else if (paf->location == APPLY_DAMROLL )
        {
            dam_bonus = paf->modifier;
            dam_found = TRUE;
            fail += 2 * (dam_bonus * dam_bonus);
        }

        else /* things get a little harder */
            fail += 25;
    }

    /* apply other modifiers */
    fail -= 3 * level/2;

    if (IS_OBJ_STAT(obj,ITEM_BLESS))
        fail -= 15;
    if (IS_OBJ_STAT(obj,ITEM_GLOW))
        fail -= 5;

    fail = URANGE(5,fail,95);

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5))  /* item destroyed */
    {
        act("$p shivers violently and explodes!",ch,obj,NULL,TO_CHAR);
        act("$p shivers violently and explodeds!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
        return;
    }

    if (result < (fail / 2)) /* item disenchanted */
    {
        AFFECT_DATA *paf_next;

        act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);
        obj->enchanted = TRUE;

        /* remove all affects */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next; 
            paf->next = affect_free;
            affect_free = paf;
        }
        obj->affected = NULL;

        /* clear all flags */
        obj->extra_flags = 0;
        return;
    }

    if ( result <= fail )  /* failed, no bad result */
    {
        send_to_char("Nothing seemed to happen.\n\r",ch);
        return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *af_new;
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

            af_new->where       = paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }

    if (result <= (100 - level/5))  /* success! */
    {
        act("$p glows blue.",ch,obj,NULL,TO_CHAR);
        act("$p glows blue.",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        added = 1;
    }
    
    else  /* exceptional enchant */
    {
        act("$p glows a brillant blue!",ch,obj,NULL,TO_CHAR);
        act("$p glows a brillant blue!",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags,ITEM_MAGIC);
        SET_BIT(obj->extra_flags,ITEM_GLOW);
        added = 2;
    }
                
    /* now add the enchantments */ 

    if (obj->level < LEVEL_HERO - 1)
        obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (dam_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if ( paf->location == APPLY_DAMROLL)
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX(paf->level,level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags,ITEM_HUM);
            }
        }
    }
    else /* add a new affect */
    {
        if (affect_free == NULL)
            paf = alloc_perm(sizeof(*paf));
        else
        {
            paf = affect_free;
            affect_free = affect_free->next;
        }

        paf->where      = TO_OBJECT;
        paf->type       = sn;
        paf->level      = level;
        paf->duration   = -1;
        paf->location   = APPLY_DAMROLL;
        paf->modifier   =  added;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }

    if (hit_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if ( paf->location == APPLY_HITROLL)
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX(paf->level,level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags,ITEM_HUM);
            }
        }
    }
    else /* add a new affect */
    {
        if (affect_free == NULL)
            paf = alloc_perm(sizeof(*paf));
        else
        {
            paf = affect_free;
            affect_free = affect_free->next;
        }
 
        paf->type       = sn;
        paf->level      = level;
        paf->duration   = -1;
        paf->location   = APPLY_HITROLL;
        paf->modifier   =  added;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }

}



/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( saves_spell( level, victim, DAM_NEGATIVE ) )
    {
        send_to_char("You feel a momentary chill.\n\r",victim);         
        return;
    }

    ch->alignment = UMAX(-1000, ch->alignment - 50);
    if ( victim->level <= 2 )
    {
        dam              = ch->hit + 1;
    }
    else
    {
        gain_exp( victim, 0 -  5 * number_range( level/2, 3 * level / 2 ) );
        victim->mana    /= 2;
        victim->move    /= 2;
        dam              = dice(1, level);
        ch->hit         += dam;
    }

    send_to_char("You feel your life slipping away!\n\r",victim);
    send_to_char("Wow....what a rush!\n\r",ch);
    damage( ch, victim, dam, sn, DAM_NEGATIVE, TRUE );

    return;
}



void spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
/*    static const sh_int dam_each[] = 
    {
          0,
          0,   0,   0,   0,   0,          0,   0,   0,   0,   0,
          0,   0,   0,   0,  30,         35,  40,  45,  50,  55,
         60,  65,  70,  75,  80,         82,  84,  86,  88,  90,
         92,  94,  96,  98, 100,        102, 104, 106, 108, 110,
        112, 114, 116, 118, 120,        122, 124, 126, 128, 130
    };
*/
    int dam;
/*
    level       = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
*/
/* New damage by Rahl */
        dam=dice(level,8);

    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;
    act("$n smirks and throws a fireball at $N.", ch, NULL, victim, TO_NOTVICT );
    act("You throw a fireball at $N.",ch,NULL,victim,TO_CHAR);
    act("$n smirks, and throws a fireball at you.",ch,NULL,victim,TO_VICT);
    damage( ch, victim, dam, sn, DAM_FIRE, TRUE );
    return;
}

/* added by Rahl */
void spell_fireproof( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
    {
        act( "$p is already protected from burning.", ch, obj, NULL,
                TO_CHAR );
        return;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 4 );
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = ITEM_BURN_PROOF;

    affect_to_obj( obj, &af );

    act( "You protect $p from fire.", ch, obj, NULL, TO_CHAR );
    act( "$p is surrounted by a protective aura.", ch, obj, NULL, TO_ROOM );
}


void spell_flamestrike( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

/*    dam = dice(6, 8); */
/* New damage by Rahl */
        dam=dice(level,8);

    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;
    act("$n calls down a column of fire from the sky.",ch,NULL,NULL,TO_ROOM);
    damage( ch, victim, dam, sn, DAM_FIRE, TRUE );
    return;
}



void spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
        return;
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = 2 * level;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char( victim, &af );
    send_to_char( "You are surrounded by a pink outline.\n\r", victim );
    act( "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM );
    return;
}



void spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *ich;

    act( "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
    send_to_char( "You conjure a cloud of purple smoke.\n\r", ch );

    for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
    {
        if ( !IS_NPC(ich) && IS_SET(ich->act, PLR_WIZINVIS) )
            continue;

        if ( ich == ch || saves_spell( level, ich, DAM_OTHER ) )
            continue;

        affect_strip ( ich, gsn_invis                   );
        affect_strip ( ich, gsn_mass_invis              );
        affect_strip ( ich, gsn_sneak                   );
        REMOVE_BIT   ( ich->affected_by, AFF_HIDE       );
        REMOVE_BIT   ( ich->affected_by, AFF_INVISIBLE  );
        REMOVE_BIT   ( ich->affected_by, AFF_SNEAK      );
        act( "$n is revealed!", ich, NULL, NULL, TO_ROOM );
        send_to_char( "You are revealed!\n\r", ich );
    }

    return;
}



void spell_fly( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FLYING) )
    {
        if (victim == ch)
          send_to_char("You are already airborne.\n\r",ch);
        else
          act("$N doesn't need your help to fly.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level + 3;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char( victim, &af );
    send_to_char( "Your feet rise off the ground.\n\r", victim );
    act( "$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM );
    return;
}

/* RT clerical berserking spell */

void spell_frenzy(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim,sn) || IS_AFFECTED(victim,AFF_BERSERK))
    {
        if (victim == ch)
          send_to_char("You are already in a frenzy.\n\r",ch);
        else
          act("$N is already in a frenzy.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (is_affected(victim,skill_lookup("calm")))
    {
        if (victim == ch)
          send_to_char("Why don't you just relax for a while?\n\r",ch);
        else
          act("$N doesn't look like $e wants to fight anymore.",
              ch,NULL,victim,TO_CHAR);
        return;
    }

    if ((IS_GOOD(ch) && !IS_GOOD(victim)) ||
        (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
        (IS_EVIL(ch) && !IS_EVIL(victim))
       )
    {
        act("Your god doesn't seem to like $N.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 3;
    af.modifier  = level / 6;
    af.bitvector = 0;

    af.location  = APPLY_HITROLL;
    affect_to_char(victim,&af);

    af.location  = APPLY_DAMROLL;
    affect_to_char(victim,&af);

    af.modifier  = 10 * (level / 6);
    af.location  = APPLY_AC;
    affect_to_char(victim,&af);

    send_to_char("You are filled with holy wrath!\n\r",victim);
    act("$n gets a wild look in $s eyes!",victim,NULL,NULL,TO_ROOM);
}

/* RT ROM-style gate */
    
void spell_gate( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;
    bool gate_pet;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   !can_see_room(ch,victim->in_room) 
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   victim->level >= level + 3
    ||   (chaos)
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */ 
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
/*    ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON)) */
    ||   (IS_NPC(victim) && saves_spell( level, victim, DAM_OTHER ) ) )
    {
        send_to_char( "A disturbance in the planar fabric prevents your transportation.\n\r", ch );
        return;
    }

    if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
        gate_pet = TRUE;
    else
        gate_pet = FALSE;
    
    act("$n steps through a gate and vanishes.",ch,NULL,NULL,TO_ROOM);
    send_to_char("You step through a gate and vanish.\n\r",ch);
    char_from_room(ch);
    char_to_room(ch,victim->in_room);

    act("$n has arrived through a gate.",ch,NULL,NULL,TO_ROOM);
    do_look(ch,"auto");

    if (gate_pet)
    {
        act("$n steps through a gate and vanishes.",ch->pet,NULL,NULL,TO_ROOM);
        send_to_char("You step through a gate and vanish.\n\r",ch->pet);
        char_from_room(ch->pet);
        char_to_room(ch->pet,victim->in_room);
        act("$n has arrived through a gate.",ch->pet,NULL,NULL,TO_ROOM);
        do_look(ch->pet,"auto");
    }
}



void spell_giant_strength( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already as strong as you can get!\n\r",ch);
        else
          act("$N can't get any stronger.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "Your muscles surge with heightened power!\n\r", victim );
    act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM);
    return;
}



void spell_harm( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = UMAX(  20, victim->hit - dice(1,4) );
    if ( saves_spell( level, victim, DAM_HARM ) )
        dam = UMIN( 50, dam / 2 );
    dam = UMIN( 100, dam );
    damage( ch, victim, dam, sn, DAM_HARM, TRUE );
    return;
}

/* greater harm by Rahl */
void spell_greater_harm( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = UMAX(  40, victim->hit - dice(2,8) );
    if ( saves_spell( level, victim, DAM_HARM ) )
        dam = UMIN( 100, dam / 2 );
    dam = UMIN( 250, dam );
    damage( ch, victim, dam, sn, DAM_HARM, TRUE );
    return;
}

/* RT haste spell */

void spell_haste( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
 
    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_HASTE)
    ||   IS_SET(victim->off_flags,OFF_FAST))
    {
        if (victim == ch)
          send_to_char("You can't move any faster!\n\r",ch);
        else
          act("$N is already moving as fast as $e can.",
              ch,NULL,victim,TO_CHAR);
        return;
    }

   /* added by Rahl */
   if ( IS_AFFECTED( victim, AFF_SLOW ) )
    {
        if ( !check_dispel( level, victim, skill_lookup( "slow" ) ) )
        {
            if ( victim != ch )
                send_to_char( "Spell failed.\n\r", ch );
            send_to_char( "You feel momentarily faster.\n\r", victim );
            return;
        }

        act( "$n is moving less slowly.", victim, NULL, NULL, TO_ROOM );
        return;
    }
        /* end stuff by Rahl */

    af.where     = TO_AFFECTS;  /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    if (victim == ch)
      af.duration  = level/2;
    else
      af.duration  = level/4;
    af.location  = APPLY_DEX;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself moving more quickly.\n\r", victim );
    act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}

/* greater heal by Rahl */
void spell_greater_heal( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->hit = UMIN( victim->hit + 250, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel MUCH better.\n\r", victim );
    if ( ch != victim )
        send_to_char( "A greater healing power flows through you.\n\r", ch );
    return;
}

/* mega heal by Rahl */
void spell_mega_heal( int sn, int level, CHAR_DATA *ch, void *vo, int
        target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->hit = UMIN( victim->hit + 500, victim->max_hit );
    update_pos( victim );
    send_to_char( "Your health improves greatly.\n\r", victim );
    if ( ch != victim )
        send_to_char( "A mega healing power flows through you.\n\r", ch );
    return;
}

void spell_heal( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->hit = UMIN( victim->hit + 100, victim->max_hit );
    update_pos( victim );
    send_to_char( "A warm feeling fills your body.\n\r", victim );
    if ( ch != victim )
        send_to_char( "A healing power flows through you.\n\r", ch );
    return;
}

/* RT really nasty high-level attack spell */
void spell_holy_word(int sn, int level, CHAR_DATA *ch, void *vo, int
        target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;
    int bless_num, curse_num, frenzy_num;
   
    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse"); 
    frenzy_num = skill_lookup("frenzy");

    act("$n utters a word of divine power!",ch,NULL,NULL,TO_ROOM);
    send_to_char("You utter a word of divine power.\n\r",ch);
 
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

        if ((IS_GOOD(ch) && IS_GOOD(vch)) ||
            (IS_EVIL(ch) && IS_EVIL(vch)) ||
            (IS_NEUTRAL(ch) && IS_NEUTRAL(vch)) )
        {
          send_to_char("You feel more powerful.\n\r",vch);
          spell_frenzy(frenzy_num,level,ch,(void *) vch, TARGET_CHAR); 
          spell_bless(bless_num,level,ch,(void *) vch, TARGET_CHAR);
        }

        else if ((IS_GOOD(ch) && IS_EVIL(vch)) ||
                 (IS_EVIL(ch) && IS_GOOD(vch)) )
        {
          if (!is_safe_spell(ch,vch,TRUE))
          {
            spell_curse(curse_num,level,ch,(void *) vch, TARGET_CHAR);
            act("$n screams in agony as a holy word is burned into $s body",
            vch,NULL,NULL,TO_NOTVICT);
            send_to_char("You are struck down!\n\r",vch);
            dam = dice(level,6);
            damage(ch,vch,dam,sn,DAM_HOLY, TRUE );
          }
        }

        else if (IS_NEUTRAL(ch))
        {
          if (!is_safe_spell(ch,vch,TRUE))
          {
            spell_curse(curse_num,level/2,ch,(void *) vch, TARGET_CHAR);
            act("$n screams in agony as a holy word is burned into $s body",
            vch,NULL,NULL,TO_NOTVICT);
            send_to_char("You are struck down!\n\r",vch);
            dam = dice(level,4);
            damage(ch,vch,dam,sn,DAM_HOLY, TRUE );
          }
        }
    }  
    
    send_to_char("You feel drained.\n\r",ch);
    gain_exp( ch, -1 * number_range(1,10) * 10);
    ch->move = 0;
    ch->hit /= 2;
}
 
void spell_identify( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    AFFECT_DATA *paf;

    bprintf( buf,
        "Object '%s' is type %s, extra flags %s.\n\rWeight is %d, value is %d, level is %d.\n\r",

        obj->name,
        item_type_name( obj ),
        extra_bit_name( obj->extra_flags ),
        obj->weight,
        obj->cost,
        obj->level
        );
    send_to_char( buf->data, ch );

    switch ( obj->item_type )
    {
    case ITEM_SCROLL: 
    case ITEM_POTION:
    case ITEM_PILL:
        bprintf( buf, "Level %d spells of:", obj->value[0] );
        send_to_char( buf->data, ch );

        if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[1]].name, ch );
            send_to_char( "'", ch );
        }

        if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[2]].name, ch );
            send_to_char( "'", ch );
        }

        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[3]].name, ch );
            send_to_char( "'", ch );
        }

        send_to_char( ".\n\r", ch );
        break;

    case ITEM_WAND: 
    case ITEM_STAFF: 
        bprintf( buf, "Has %d(%d) charges of level %d",
            obj->value[1], obj->value[2], obj->value[0] );
        send_to_char( buf->data, ch );
      
        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[3]].name, ch );
            send_to_char( "'", ch );
        }

        send_to_char( ".\n\r", ch );
        break;
      
    case ITEM_WEAPON:
        send_to_char("Weapon type is ",ch);
        switch (obj->value[0])
        {
            case(WEAPON_EXOTIC) : send_to_char("exotic.\n\r",ch);       break;
            case(WEAPON_SWORD)  : send_to_char("sword.\n\r",ch);        break;  
            case(WEAPON_DAGGER) : send_to_char("dagger.\n\r",ch);       break;
            case(WEAPON_SPEAR)  : send_to_char("spear/staff.\n\r",ch);  break;
            case(WEAPON_MACE)   : send_to_char("mace/club.\n\r",ch);    break;
            case(WEAPON_AXE)    : send_to_char("axe.\n\r",ch);          break;
            case(WEAPON_FLAIL)  : send_to_char("flail.\n\r",ch);        break;
            case(WEAPON_WHIP)   : send_to_char("whip.\n\r",ch);         break;
            case(WEAPON_POLEARM): send_to_char("polearm.\n\r",ch);      break;
            default             : send_to_char("unknown.\n\r",ch);      break;
        }
            bprintf(buf,"Damage is %dd%d (average %d).\n\r",
                obj->value[1],obj->value[2],
                (1 + obj->value[2]) * obj->value[1] / 2);
        send_to_char( buf->data, ch );
        break;

    case ITEM_ARMOR:
        bprintf( buf, 
        "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r", 
            obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
        send_to_char( buf->data, ch );
        break;
    }

    if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location != APPLY_NONE && paf->modifier != 0 )
        {
            bprintf( buf, "Affects %s by %d.\n\r",
                affect_loc_name( paf->location ), paf->modifier );
            send_to_char( buf->data, ch );
/* added by Rahl */
            if ( paf->bitvector )
            {
                switch ( paf->where )
                {
                    case TO_AFFECTS:
                        bprintf( buf, "Adds %s affect.\n",
                            affect_bit_name( paf->bitvector ) );
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
                        bprintf( buf, "Adds vulnerability to %s.\n",    
                            imm_bit_name( paf->bitvector ) );
                        break;
                    default:
                        bprintf( buf, "Unknown bit %d: %d\n\r",
                            paf->where, paf->bitvector );
                        break;
                }
                send_to_char ( buf->data, ch );
            }
        }
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location != APPLY_NONE && paf->modifier != 0 )
        {
            bprintf( buf, "Affects %s by %d",
                affect_loc_name( paf->location ), paf->modifier );
            send_to_char( buf->data, ch );
            if ( paf->duration > -1 )
                bprintf( buf, ", %d hours.\n\r", paf->duration );
            else
                bprintf( buf, ".\n\r" );
            send_to_char( buf->data, ch );
            if ( paf->bitvector )
            {
                switch ( paf->where )
                {
                    case TO_AFFECTS:
                        bprintf( buf, "Adds %s affect.\n",
                            affect_bit_name( paf->bitvector ) );
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
                        bprintf( buf, "Adds vulnerability to %s.\n",    
                            imm_bit_name( paf->bitvector ) );
                        break;
                    default:
                        bprintf( buf, "Unknown bit %d: %d\n\r",
                            paf->where, paf->bitvector );
                        break;
                }
                send_to_char( buf->data, ch );
            }
/* end stuff by Rahl */
        }
    }
    buffer_free( buf );
    return;
}



void spell_infravision( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_INFRARED) )
    {
        if (victim == ch)
          send_to_char("You can already see in the dark.\n\r",ch);
        else
          act("$N already has infravision.\n\r",ch,NULL,victim,TO_CHAR);
        return;
    }
    act( "$n's eyes glow red.\n\r", ch, NULL, NULL, TO_ROOM );
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes glow red.\n\r", victim );
    return;
}



void spell_invis( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    OBJ_DATA *obj;      /* added by Rahl */

    /* added by Rahl */
    /* object invisibility */
    if ( target == TARGET_OBJ )
    {
        obj = (OBJ_DATA *) vo;
        
        if ( IS_OBJ_STAT( obj, ITEM_INVIS ) )
        {
            act( "$p is already invisible.", ch, obj, NULL, TO_CHAR );
            return;
        }
        
        af.where         = TO_OBJECT;
        af.type          = sn;
        af.level         = level;
        af.duration      = level + 12;
        af.location      = APPLY_NONE;
        af.modifier      = 0;
        af.bitvector     = ITEM_INVIS;
        
        affect_to_obj( obj, &af );

        act( "$p fades out of sight.", ch, obj, NULL, TO_ALL );
        return;
    }

    /* character invisibility */
    /* end stuff by Rahl */

    if ( IS_AFFECTED(victim, AFF_INVISIBLE) )
        return;

    act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );
    af.where     = TO_AFFECTS;  /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char( victim, &af );
    send_to_char( "You fade out of existence.\n\r", victim );
    return;
}



void spell_know_alignment( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    char *msg;
    int ap;

    ap = victim->alignment;

         if ( ap >  700 ) msg = "$N has a pure and good aura.";
    else if ( ap >  350 ) msg = "$N is of excellent moral character.";
    else if ( ap >  100 ) msg = "$N is often kind and thoughtful.";
    else if ( ap > -100 ) msg = "$N doesn't have a firm moral commitment.";
    else if ( ap > -350 ) msg = "$N lies to $S friends.";
    else if ( ap > -700 ) msg = "$N is a black-hearted murderer.";
    else msg = "$N is the embodiment of pure evil!.";

    act( msg, ch, NULL, victim, TO_CHAR );
    return;
}



void spell_lightning_bolt( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
/*    static const sh_int dam_each[] = 
    {
         0,
         0,  0,  0,  0,  0,      0,  0,  0, 25, 28,
        31, 34, 37, 40, 40,     41, 42, 42, 43, 44,
        44, 45, 46, 46, 47,     48, 48, 49, 50, 50,
        51, 52, 52, 53, 54,     54, 55, 56, 56, 57,
        58, 58, 59, 60, 60,     61, 62, 62, 63, 64
    };
*/
    int dam;
/*
    level       = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
*/
/* New damage by Rahl */
        dam=dice(level,6);

    if ( saves_spell( level, victim, DAM_LIGHTNING ) )
        dam /= 2;
    act("$n directs a bolt of lightning at $N.",ch,NULL,victim,TO_ROOM);
    damage( ch, victim, dam, sn, DAM_LIGHTNING, TRUE );
    return;
}



void spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    buffer = buffer_new( 500 );
    buffer->data[0] = '\0';
    found = FALSE;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;
 
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( target_name, obj->name ) 
        ||   (!IS_IMMORTAL(ch) && number_percent() > 2 * level)
        ||   ch->level < obj->level
        ||   IS_OBJ_STAT( obj, ITEM_NOLOCATE ) )    /* added by Rahl */
            continue;

        found = TRUE;
        number++;

        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;

        if ( in_obj->carried_by != NULL && can_see)
        {
            sprintf( buf, "%s carried by %s\n\r",
                obj->short_descr, PERS(in_obj->carried_by, ch) );
        }
        else
        {
            if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
                sprintf( buf, "%s in %s [Room %d]\n\r",
                    obj->short_descr, 
                    in_obj->in_room->name, in_obj->in_room->vnum);
            else
                sprintf( buf, "%s in %s\n\r",
                    obj->short_descr, in_obj->in_room == NULL
                        ? "somewhere" : in_obj->in_room->name );
        }

        buf[0] = UPPER(buf[0]);
        buffer_strcat(buffer,buf);

        if (number >= max_found)
            break;
    }

    if ( !found )
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
    else if (ch->lines)
        page_to_char(buffer->data,ch);
    else
        send_to_char(buffer->data,ch);

    buffer_free( buffer );

    return;
}



void spell_magic_missile( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

/*  static const sh_int dam_each[] = 
    {
         0,
         3,  3,  4,  4,  5,      6,  6,  6,  6,  6,
         7,  7,  7,  7,  7,      8,  8,  8,  8,  8,
         9,  9,  9,  9,  9,     10, 10, 10, 10, 10,
        11, 11, 11, 11, 11,     12, 12, 12, 12, 12,
        13, 13, 13, 13, 13,     14, 14, 14, 14, 14
    };

    level       = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
*/
/* 
 * removed by Rahl

        if (level >25)
           dam=dice(3,6); */
    /*else*/
/*         dam = (dice(level/5,3) + (level/10)); */

/* New damage by Rahl */
        dam = dice(level,2);

    if ( saves_spell( level, victim, DAM_ENERGY ) )
        dam /= 2;
    act("A green glowing arrow fires from $n's outstretched hand.",ch,NULL,NULL,TO_ROOM);
    damage( ch, victim, dam, sn, DAM_ENERGY, TRUE );
    return;
}

void spell_mass_healing(int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *gch;
    int heal_num, refresh_num;
    
    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh"); 

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ((IS_NPC(ch) && IS_NPC(gch)) ||
            (!IS_NPC(ch) && !IS_NPC(gch)))
        {
            spell_heal(heal_num,level,ch,(void *) gch, TARGET_CHAR);
            spell_refresh(refresh_num,level,ch,(void *) gch, TARGET_CHAR);  
        }
    }
}
            

void spell_mass_invis( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    AFFECT_DATA af;
    CHAR_DATA *gch;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !is_same_group( gch, ch ) || IS_AFFECTED(gch, AFF_INVISIBLE) )
            continue;
        act( "$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM );
        send_to_char( "You slowly fade out of existence.\n\r", gch );
        af.where     = TO_AFFECTS;  /* added by Rahl */
        af.type      = sn;
        af.level     = level/2;
        af.duration  = 24;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char( gch, &af );
    }
    send_to_char( "Ok.\n\r", ch );

    return;
}



void spell_null( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    send_to_char( "That's not a spell!\n\r", ch );
    return;
}



void spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_PASS_DOOR) )
    {
        if (victim == ch)
          send_to_char("You are already out of phase.\n\r",ch);
        else
          act("$N is already shifted out of phase.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 4 );
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char( victim, &af );
    act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You turn translucent.\n\r", victim );
    return;
}

/* RT plague spell, very nasty */

void spell_plague( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (saves_spell(level,victim, DAM_DISEASE ) || 
        (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
    {
        if (ch == victim)
          send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
        else
          act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type       = sn;
    af.level      = level * 3/4;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = -5; 
    af.bitvector = AFF_PLAGUE;
    affect_join(victim,&af);
   
    send_to_char
      ("You scream in agony as plague sores erupt from your skin.\n\r",victim);
    act("$n screams in agony as plague sores erupt from $s skin.",
        victim,NULL,NULL,TO_ROOM);
}

void spell_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    OBJ_DATA *obj;      /* added by Rahl */

    /* added by Rahl */
    if ( target == TARGET_OBJ )
    {
        obj = (OBJ_DATA *) vo;
        
        if ( obj->item_type == ITEM_FOOD || obj->item_type ==  ITEM_DRINK_CON )
        {
            if ( IS_OBJ_STAT( obj, ITEM_BLESS )
                || IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
            {
                act( "Your spell fails to corrupt $p.", ch, obj, NULL,
                        TO_CHAR );
                return;
            }
            obj->value[3] = 1;
            act( "$p is infused with poisonous vapros.", ch, obj, NULL,
                TO_ALL );
            return;
        }

        if ( obj->item_type == ITEM_WEAPON )
        {
            if ( IS_WEAPON_STAT( obj, WEAPON_FLAMING )
            ||   IS_WEAPON_STAT( obj, WEAPON_FROST )
            ||   IS_WEAPON_STAT( obj, WEAPON_VAMPIRIC )
            ||   IS_WEAPON_STAT( obj, WEAPON_SHARP )
            ||   IS_WEAPON_STAT( obj, WEAPON_VORPAL )
            ||   IS_WEAPON_STAT( obj, WEAPON_SHOCKING )
            ||   IS_OBJ_STAT( obj, ITEM_BLESS ) 
            ||   IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
            {
                act( "You can't seem to envenom $p.", ch, obj, NULL,
                        TO_CHAR );
                return;
            }
        
            if ( IS_WEAPON_STAT( obj, WEAPON_POISON ) )
            {
                act( "$p is already envenomed.", ch, obj, NULL, TO_CHAR );
                return;
            }
        
            af.where     = TO_WEAPON;
            af.type      = sn;
            af.level     = level / 2;
            af.duration  = level / 8;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj( obj, &af );
        
            act( "$p is coated with a deadly venom.", ch, obj, NULL,
                TO_ALL );
            return;
        }
        
        act( "You can't seem to poison $p.", ch, obj, NULL, TO_CHAR );
        return;
    }
/* end stuff by Rahl */

    /* character case */
    if ( saves_spell( level, victim, DAM_POISON ) )
    {
        act("$n turns slightly green, but it passes.",victim,NULL,NULL,TO_ROOM);
        send_to_char("You feel momentarily ill, but it passes.\n\r",victim);
        return;
    }
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = -2;
    af.bitvector = AFF_POISON;
    affect_join( victim, &af );
    send_to_char( "You feel very sick.\n\r", victim );
    act("$n looks very ill.",victim,NULL,NULL,TO_ROOM);
    return;
}



void spell_protection_evil( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_PROTECT_EVIL)
    ||   IS_AFFECTED(victim, AFF_PROTECT_GOOD) )
    {
        if (victim == ch)
          send_to_char("You are already protected.\n\r",ch);
        else
          act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "You feel holy and pure.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from evil.",ch,NULL,victim,TO_CHAR);
    return;
}


/* added by Rahl */
void spell_protection_good( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_PROTECT_EVIL)
    ||   IS_AFFECTED(victim, AFF_PROTECT_GOOD) )
    {
        if (victim == ch)
          send_to_char("You are already protected.\n\r",ch);
        else
          act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "You feel aligned with darkness.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from good.",ch,NULL,victim,TO_CHAR);
    return;
}


void spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->move = UMIN( victim->move + level, victim->max_move );
    if (victim->max_move == victim->move)
        send_to_char("You feel fully refreshed!\n\r",victim);
    else
        send_to_char( "You feel less tired.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}


/* modified by Rahl */
void spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
    OBJ_DATA *obj;
/*     int iWear; */

    /* object cases first */
    if ( target == TARGET_OBJ )
    {
        obj = (OBJ_DATA *) vo;
        
        if ( IS_OBJ_STAT( obj, ITEM_NODROP ) )
        {
            if ( !IS_OBJ_STAT( obj, ITEM_NOUNCURSE )
            && !saves_dispel( level + 2, obj->level, 0 ) )
            {
                AFFECT_DATA *paf;
                paf = affect_find( obj->affected, gsn_grip );
                if ( paf != NULL )
                    affect_remove_obj( obj, paf );
                REMOVE_BIT( obj->extra_flags, ITEM_NODROP );
                act( "$p glows blue.", ch, obj, NULL, TO_ALL );
                return;
            }
            act( "The curse on $p is beyond your power.", ch, obj, NULL,
                TO_CHAR );
        }
        if ( IS_OBJ_STAT( obj, ITEM_NOREMOVE ) )
        {
            if ( !IS_OBJ_STAT( obj, ITEM_NOUNCURSE )
            && !saves_dispel( level + 2, obj->level, 0 ) )
            {
                AFFECT_DATA *paf;
                paf = affect_find( obj->affected, gsn_grip );
                if ( paf != NULL )
                    affect_remove_obj( obj, paf );
                REMOVE_BIT( obj->extra_flags, ITEM_NOREMOVE );
                act( "$p glows blue.", ch, obj, NULL, TO_ALL );
                return;
            }
            act( "The curse on $p is beyond your power.", ch, obj, NULL,
                TO_CHAR );
        }

        act( "There doesn't seem to be a curse on $p.", ch, obj, NULL,
                TO_CHAR );
        return;
    }

    /* characters */
    if (check_dispel(level,victim,gsn_curse))
    {
        send_to_char("You feel better.\n\r",victim);
        act("$n looks more relaxed.",victim,NULL,NULL,TO_ROOM);
    }

/*    for ( iWear = 0; (iWear < MAX_WEAR && !found); iWear ++)
    {
        if ((obj = get_eq_char(victim,iWear)) == NULL)
            continue;

        if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
        {   // attempt to remove curse 
            if (!saves_dispel(level,obj->level,0))
            {
                found = TRUE; 
                REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
                act("$p glows blue.",victim,obj,NULL,TO_CHAR);
                act("$p glows blue.",victim,obj,NULL,TO_ROOM);
            }
         }
    }
*/
   for (obj = victim->carrying; (obj != NULL && !found); obj = obj->next_content)
   {
        if ((IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
        && !IS_OBJ_STAT( obj, ITEM_NOUNCURSE ) )
        {   /* attempt to remove curse */
            if (!saves_dispel(level,obj->level,0))
            {
                AFFECT_DATA *paf;
                paf = affect_find( obj->affected, gsn_grip );
                if ( paf != NULL )
                    affect_remove_obj( obj, paf );

                found = TRUE;
                REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
                act("Your $p glows blue.",victim,obj,NULL,TO_CHAR);
                act("$n's $p glows blue.",victim,obj,NULL,TO_ROOM);
            }
         }
    }
}

void spell_sanctuary( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
    {
        if (victim == ch)
          send_to_char("You are already in sanctuary.\n\r",ch);
        else
          act("$N is already in sanctuary.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     =  TO_AFFECTS;         /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 6 );
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a white aura.\n\r", victim );
    return;
}



void spell_shield( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already shielded from harm.\n\r",ch);
        else
          act("$N is already protected by a shield.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;  /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = 8 + level;
    af.location  = APPLY_AC;
    af.modifier  = -20;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a force shield.\n\r", victim );
    return;
}



void spell_shocking_grasp( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
/*    static const int dam_each[] = 
    {
         0,
         0,  0,  0,  0,  0,      0, 20, 25, 29, 33,
        36, 39, 39, 39, 40,     40, 41, 41, 42, 42,
        43, 43, 44, 44, 45,     45, 46, 46, 47, 47,
        48, 48, 49, 49, 50,     50, 51, 51, 52, 52,
        53, 53, 54, 54, 55,     55, 56, 56, 57, 57
    };
*/
    int dam;
/*
    level       = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
*/
/* New Damage by Rahl */
        dam=dice(level,5);

    if ( saves_spell( level, victim, DAM_LIGHTNING ) )
        dam /= 2;

    act("$n grabs hold of you and shocks you.",ch,NULL,victim,TO_VICT);
    act("$n grabs hold of $N and shocks $M.",ch,NULL,victim,TO_ROOM);
    damage( ch, victim, dam, sn, DAM_LIGHTNING, TRUE );
    return;
}



void spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
  
    if ( IS_AFFECTED(victim, AFF_SLEEP)
    ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD))
    ||   level < victim->level
    ||   saves_spell( level, victim, DAM_OTHER ) )
        return;

    af.where     = TO_AFFECTS;  /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = 4 + level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SLEEP;
    affect_join( victim, &af );

    if ( IS_AWAKE(victim) )
    {
        send_to_char( "You feel very sleepy ..... zzzzzz.\n\r", victim );
        act( "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
        victim->position = POS_SLEEPING;
    }

    return;
}



void spell_stone_skin( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( ch, sn ) )
    {
        if (victim == ch)
          send_to_char("Your skin is already as hard as a rock.\n\r",ch); 
        else
          act("$N is already as hard as can be.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;  /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = -40;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "Your skin turns to stone.\n\r", victim );
    return;
}



void spell_summon( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   (chaos)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_AGGRESSIVE))
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)
    ||   victim->fighting != NULL
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON))  
    ||   (IS_NPC(victim) && saves_spell( level, victim, DAM_OTHER ) ) )
    {
        send_to_char( "A disturbance interrupts your summons.\n\r", ch );
        return;
    }

    act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
    act( "$n has summoned you!", ch, NULL, victim,   TO_VICT );
    do_look( victim, "auto" );
    return;
}



void spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *pRoomIndex;

    if ( victim->in_room == NULL
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    || ( !IS_NPC(ch) && victim->fighting != NULL )
    || ( chaos )
    || ( victim != ch
    && ( saves_spell( level, victim, DAM_OTHER ) ) ) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

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

    if (victim != ch)
        send_to_char("You have been teleported!\n\r",victim);

    act( "$n vanishes!", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    act( "$n slowly fades into existence.", victim, NULL, NULL, TO_ROOM );
    do_look( victim, "auto" );
    return;
}



void spell_ventriloquate( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    BUFFER *buf1 = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf2 = buffer_new( MAX_INPUT_LENGTH );
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;

    target_name = one_argument( target_name, speaker );

    bprintf( buf1, "%s says '%s'.\n\r",              speaker, target_name );
    bprintf( buf2, "Someone makes %s say '%s'.\n\r", speaker, target_name );
    buf1->data[0] = UPPER(buf1->data[0]);

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( !is_name( speaker, vch->name ) )
            send_to_char( saves_spell( level, vch, DAM_OTHER ) ?
                buf2->data : buf1->data, vch );
    }

    buffer_free( buf1 );
    buffer_free( buf2 );
    return;
}



void spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) || saves_spell( level, victim, DAM_OTHER ) )
        return;
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 2;
    af.location  = APPLY_STR;
    af.modifier  = -1 * (level / 5);
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );
    send_to_char( "You feel weaker.\n\r", victim );
    act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
    return;
}



/* RT recall spell is back */

void spell_word_of_recall( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *location;
    
    if (IS_NPC(victim))
      return;
   
    if ((location = get_room_index( ROOM_VNUM_TEMPLE)) == NULL)
    {
        send_to_char("You are completely lost.\n\r",victim);
        return;
    } 

    if (IS_SET(victim->in_room->room_flags,ROOM_NO_RECALL) ||
        IS_AFFECTED(victim,AFF_CURSE))
    {
        send_to_char("Spell failed.\n\r",victim);
        return;
    }

    if (victim->fighting != NULL)
        stop_fighting(victim,TRUE);
    
    ch->move /= 2;
    act("$n disappears.",victim,NULL,NULL,TO_ROOM);
    char_from_room(victim);
    char_to_room(victim,location);
    act("$n appears in the room.",victim,NULL,NULL,TO_ROOM);
    do_look(victim,"auto");
}

/* Flame Sword by Rahl */

void spell_flame_sword( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *flame_sword;

    flame_sword = create_object( get_obj_index( OBJ_VNUM_FLAME_SWORD ), 0);
    flame_sword->timer = level*2;
    flame_sword->level = level;
    flame_sword->value[1] = level*0.25;
    obj_to_char( flame_sword, ch );
    act( "$n has created $p!", ch, flame_sword, NULL, TO_ROOM );
    act( "You have created $p!", ch, flame_sword, NULL, TO_CHAR );
    return;
}

/* Flash Fire by Rahl */

void spell_flash_fire( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level, 12);
    if ( saves_spell( level,victim, DAM_FIRE ) )
        dam /=2;
    act( "A fire breaks out and consumes $N.", ch, NULL, victim, TO_NOTVICT);
    act( "A fire breaks out and consumes you.", ch, NULL, victim, TO_VICT );
    damage( ch, victim, dam, sn, DAM_FIRE, TRUE );
    return;
}

/* Inferno by Rahl */

void spell_inferno( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam=dice(level, 13);
    if ( saves_spell(level,victim, DAM_FIRE) )
        dam /=2;
    act( "An inferno engulfs $N.", ch, NULL, victim, TO_NOTVICT );
    act( "An inferno engulfs you!.", ch, NULL, victim, TO_VICT );
    damage( ch, victim, dam, sn, DAM_FIRE, TRUE );
    return;
}

/* Thunderbolt by Rahl */

void spell_thunderbolt( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    dam = dice(8,(10*(level/6)));
    if ( saves_spell(level,victim, DAM_LIGHTNING) )
        dam /=2;
    act( "A thunderbolt leaps from your hand and strikes $N.", ch, NULL, 
        victim, TO_CHAR );
    act( "A thunderbolt leaps from $n's hand and strikes $N.", ch, NULL, 
        victim, TO_NOTVICT );
    act( "A thunderbolt leaps from $n's hand and strikes you.", ch, NULL, 
        victim, TO_VICT );
    damage( ch, victim, dam, sn, DAM_LIGHTNING, TRUE );
    return;
}

/* Instant Death by Rahl */

void spell_instant_death( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 15 );
    damage( ch, victim, dam, sn, DAM_OTHER, TRUE );
    return;
}


/*
 * NPC spells.
 */
void spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj_lose;
    OBJ_DATA *obj_next;
    OBJ_DATA *t_obj,*n_obj;
    int dam;
    int hpch;
    int i;

    if ( number_percent( ) < 2 * level && !saves_spell( level, victim,
        DAM_ACID ) )
    {
        /* umm.. why was this ch? -- Rahl */
        for ( obj_lose = victim->carrying; obj_lose != NULL; obj_lose = obj_next )
        {
            int iWear;

            obj_next = obj_lose->next_content;

            if  ( ( number_bits( 2 ) != 0 )
            || IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF)
            ||  IS_OBJ_STAT(obj_lose,ITEM_NOPURGE) )
                continue;

            switch ( obj_lose->item_type )
            {
            case ITEM_ARMOR:
                if ( obj_lose->value[0] > 0 )
                {
                    act( "$p is pitted and etched!",
                        victim, obj_lose, NULL, TO_CHAR );
                    if ( ( iWear = obj_lose->wear_loc ) != WEAR_NONE )
                        for (i = 0; i < 4; i ++)
                            victim->armor[i] -= apply_ac( obj_lose, iWear, i );
                    for (i = 0; i < 4; i ++)
                        obj_lose->value[i] -= 1;
                    obj_lose->cost      = 0;
                    if ( iWear != WEAR_NONE )
                        for (i = 0; i < 4; i++)
                            victim->armor[i] += apply_ac( obj_lose, iWear, i );
                }
                break;

            case ITEM_CONTAINER:
                act( "$p fumes and dissolves, destroying some of the contents.",
                    victim, obj_lose, NULL, TO_CHAR );
                /* save some of  the contents */

                for (t_obj = obj_lose->contains; t_obj != NULL; t_obj = n_obj)
                {
                    n_obj = t_obj->next_content;
                    obj_from_obj(t_obj);

                    if (number_bits(2) == 0 || victim->in_room == NULL)
                        extract_obj(t_obj);
                    else 
                        obj_to_room(t_obj,victim->in_room);
                }

                extract_obj( obj_lose );
                break;
                
            }
        }
    }

    hpch = UMAX( 10, ch->hit );
    dam  = number_range( hpch/16+1, hpch/8 );
    if ( saves_spell( level, victim, DAM_ACID ) )
        dam /= 2;
    dam *= 1.5;
    damage( ch, victim, dam, sn, DAM_ACID, TRUE );
    return;
}



void spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj_lose;
    OBJ_DATA *obj_next;
    OBJ_DATA *t_obj, *n_obj;
    int dam;
    int hpch;

    if ( number_percent( ) < 2 * level && !saves_spell( level, victim,
        DAM_FIRE ) )
    {
        for ( obj_lose = victim->carrying; obj_lose != NULL;
        obj_lose = obj_next )
        {
            char *msg;

            obj_next = obj_lose->next_content;
            if ( (  number_bits( 2 ) != 0 )
            || IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF)
            ||  IS_OBJ_STAT(obj_lose,ITEM_NOPURGE) )
                continue;

            switch ( obj_lose->item_type )
            {
            default:             continue;
            case ITEM_CONTAINER: msg = "$p ignites and burns!";   break;
            case ITEM_POTION:    msg = "$p bubbles and boils!";   break;
            case ITEM_SCROLL:    msg = "$p crackles and burns!";  break;
            case ITEM_STAFF:     msg = "$p smokes and chars!";    break;
            case ITEM_WAND:      msg = "$p sparks and sputters!"; break;
            case ITEM_FOOD:      msg = "$p blackens and crisps!"; break;
            case ITEM_PILL:      msg = "$p melts and drips!";     break;
            }

            act( msg, victim, obj_lose, NULL, TO_CHAR );
            if (obj_lose->item_type == ITEM_CONTAINER)
            {
                /* save some of  the contents */

                for (t_obj = obj_lose->contains; t_obj != NULL; t_obj = n_obj)
                {
                    n_obj = t_obj->next_content;
                    obj_from_obj(t_obj);

                    if (number_bits(2) == 0 || ch->in_room == NULL)
                        extract_obj(t_obj);
                    else
                        obj_to_room(t_obj,ch->in_room);
                }
            }

            extract_obj( obj_lose );
        }
    }

    hpch = UMAX( 10, ch->hit );
    dam  = number_range( hpch/16+1, hpch/8 );
    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;
    dam *= 1.5;
    damage( ch, victim, dam, sn, DAM_FIRE, TRUE );
    return;
}



void spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj_lose;
    OBJ_DATA *obj_next;
    int dam;
    int hpch;

    if ( number_percent( ) < 2 * level && !saves_spell( level, victim,
        DAM_COLD ) )
    {
        for ( obj_lose = victim->carrying; obj_lose != NULL;
        obj_lose = obj_next )
        {
            char *msg;

            obj_next = obj_lose->next_content;
            if ( number_bits( 2 ) != 0 )
                continue;

            switch ( obj_lose->item_type )
            {
            default:            continue;
            case ITEM_DRINK_CON:
            case ITEM_POTION:   msg = "$p freezes and shatters!"; break;
            }

            act( msg, victim, obj_lose, NULL, TO_CHAR );
            extract_obj( obj_lose );
        }
    }

    hpch = UMAX( 10, ch->hit );
    dam  = number_range( hpch/16+1, hpch/8 );
    if ( saves_spell( level, victim, DAM_COLD ) )
        dam /= 2;
    dam *= 1.5;
    damage( ch, victim, dam, sn, DAM_COLD, TRUE );
    return;
}



void spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;
    int hpch;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;
        if ( !is_safe_spell(ch,vch,TRUE))
        {
            hpch = UMAX( 10, ch->hit );
            dam  = number_range( hpch/16+1, hpch/8 );
            if ( saves_spell( level, vch, DAM_POISON ) )
                dam /= 2;
            dam *= 1.5;
            poison_effect( vch, level, dam, TARGET_CHAR ); /* added by Rahl */
            damage( ch, vch, dam, sn, DAM_POISON, TRUE );
        }
    }
    return;
}



void spell_lightning_breath( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    int hpch;

    hpch = UMAX( 10, ch->hit );
    dam = number_range( hpch/16+1, hpch/8 );
    if ( saves_spell( level, victim, DAM_LIGHTNING ) )
        dam /= 2;
    dam *= 1.5;
    shock_effect( victim, level, dam, TARGET_CHAR ); /* added by Rahl */ 
    damage( ch, victim, dam, sn, DAM_LIGHTNING, TRUE );
    return;
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
 
    dam = number_range( 25, 100 );
    if ( saves_spell( level, victim, DAM_PIERCE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_PIERCE, TRUE );
    return;
}

void spell_high_explosive( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
 
    dam = number_range( 30, 120 );
    if ( saves_spell( level, victim, DAM_PIERCE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_PIERCE, TRUE );
    return;
}

/*  Spells added by Thexder Firehawk */  

void spell_firewind( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 12 );
    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;
    act("$n dissappears in a gust of burning winds.",victim,NULL,NULL,TO_ROOM);
    damage( ch, victim, dam, sn,DAM_FIRE, TRUE );
    return;
}

void spell_meteor_swarm( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;
        if ( !is_safe_spell(ch,vch,TRUE))
        {
            dam  = dice(10, 10);
            if ( saves_spell( level, vch, DAM_FIRE ) )
                dam /= 2;
act( "$n is hit full force with a fireball.",vch,NULL,NULL,TO_ROOM);
            damage( ch, vch, dam, sn, DAM_FIRE, TRUE );
        }
    }
    return;
}


void spell_multi_missile(int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *tmp_vict,*last_vict,*next_vict;
    bool found;
    int dam;
    int num_missile;

    /* first strike */
    num_missile = level/5;

    act("A magic missile erupts from $n's hand and hits $N in the chest.",
        ch,NULL,victim,TO_NOTVICT);
    act("A magic missile flies from your hand and hits $N in the chest.",
        ch,NULL,victim,TO_CHAR);
    act("A magic missile flies from $n's hand and hits you in the chest!",
        ch,NULL,victim,TO_VICT);  

    dam = dice(level/2,3);
    if (saves_spell(level,victim, DAM_ENERGY ))
        dam /= 3;
    damage(ch,victim,dam,sn,DAM_ENERGY, TRUE );
    last_vict = victim;
    num_missile -= 1;   /* decrement number of missiles */

    /* new targets */
    while (num_missile > 0)
    {
        found = FALSE;
        for (tmp_vict = ch->in_room->people; 
             tmp_vict != NULL; 
             tmp_vict = next_vict) 
        {
          next_vict = tmp_vict->next_in_room;
          if (!is_safe_spell(ch,tmp_vict,TRUE) && tmp_vict != last_vict && num_missile > 0)
          {
            found = TRUE;
            last_vict = tmp_vict;
act("A magic missile erupts from $n's hand and hits $N in the chest.",
        ch,NULL,tmp_vict,TO_NOTVICT);
    act("A magic missile flies from your hand and hits $N in the chest.",
        ch,NULL,tmp_vict,TO_CHAR);
    act("A magic missile flies from $n's hand and hits you in the chest!",
        ch,NULL,tmp_vict,TO_VICT);  
            dam = dice(level/2,3);
            if (saves_spell(level,tmp_vict, DAM_ENERGY ))
                dam /= 3;
            damage(ch,tmp_vict,dam,sn,DAM_ENERGY, TRUE );
            num_missile -= 1;  /* decrement number of missiles */
          }
        }   /* end target searching loop */
        
        if (!found) /* no target found, hit the caster */
        {
          if (ch == NULL)
            return;

          if (last_vict == ch) /* no double hits */
          {
            return;
          }
        
          last_vict = ch;
          num_missile -= 1;  /* decrement damage */
          if (ch == NULL) 
            return;
        }
    /* now go back and find more targets */
    }
}
void spell_disintegrate( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 11 );
    if ( saves_spell( level, victim, DAM_ENERGY ) )
        dam /= 2;
    act("$n's blast disintegrates a piece of $N's body!",ch,NULL,victim,TO_NOTVICT);
    act("Your blast disintegrates a piece of $N's body!",ch,NULL,victim,TO_CHAR);
    act("$n's blast disintegrates a piece of your body!",ch,NULL,victim,TO_VICT);
    damage( ch, victim, dam, sn,DAM_ENERGY, TRUE );
    return;
}

void spell_ice_ray( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    dam = dice( level, 11 );

    act("A blue ray from $n's hand strikes $N.",ch,NULL,victim,TO_NOTVICT);
    act("A blue ray shoots from your finger and strikes $N.",ch,NULL,victim,TO_CHAR);
    act("$n points at you sending a chilling ray into your chest.",ch,NULL,victim,TO_VICT);    

    if ( !saves_spell( level, victim, DAM_COLD ) )
    {
        act("$n turns blue and shivers.",victim,NULL,NULL,TO_ROOM);
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 12;
        af.location  = APPLY_STR;
        af.modifier  = -2;
        af.bitvector = 0;
        affect_join( victim, &af );
    }
    else
    {
        dam /= 2;
    }

    damage( ch, victim, dam, sn,DAM_COLD, TRUE );
    return;
}

void spell_hellfire(int sn, int level, CHAR_DATA *ch, void *vo, int
        target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *tmp_vict,*last_vict,*next_vict;
    bool found;
    int dam;
    int num_blast;

    /* first strike */
    num_blast = level/7;

    act("A jet of flame erupts from $n's hand and engulfs $N.",
        ch,NULL,victim,TO_NOTVICT);
    act("A jet of flames fly from your hand and engulf $N.",
        ch,NULL,victim,TO_CHAR);
    act("A jet of flames flies from $n's hand and engulfs you!",
        ch,NULL,victim,TO_VICT);  

    dam = dice(level/2,5);
    if (saves_spell(level,victim, DAM_FIRE))
        dam /= 2;
    damage(ch,victim,dam,sn,DAM_FIRE, TRUE);
    last_vict = victim;
    num_blast -= 1;   /* decrement number of blasts */

    /* new targets */
    while (num_blast > 0)
    {
        found = FALSE;
        for (tmp_vict = ch->in_room->people; 
             tmp_vict != NULL; 
             tmp_vict = next_vict) 
        {
          next_vict = tmp_vict->next_in_room;
          if (!is_safe_spell(ch,tmp_vict,TRUE) && tmp_vict != last_vict && num_blast > 0)
          {
            found = TRUE;
            last_vict = tmp_vict;

    act("A jet of flame erupts from $n's hand and engulfs $N.",
        ch,NULL,tmp_vict,TO_NOTVICT);
    act("A jet of flames fly from your hand and engulf $N.",
        ch,NULL,tmp_vict,TO_CHAR);
    act("A jet of flames flies from $n's hand and engulfs you!",
        ch,NULL,tmp_vict,TO_VICT);  

            dam = dice(level/2,5);
            if (saves_spell(level,tmp_vict, DAM_FIRE))
                dam /= 2;
            damage(ch,tmp_vict,dam,sn,DAM_FIRE, TRUE);
            num_blast -= 1;  /* decrement number of blasts */
          }
        }   /* end target searching loop */
        
        if (!found) /* no target found, hit the caster */
        {
          if (ch == NULL)
            return;

          if (last_vict == ch) /* no double hits */
          {
            return;
          }
        
          last_vict = ch;
          num_blast -= 1;  /* decrement number of blasts */
          if (ch == NULL) 
            return;
        }
    /* now go back and find more targets */
    }
}

void spell_ice_storm(int sn, int level, CHAR_DATA *ch, void *vo, int
        target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *tmp_vict,*last_vict,*next_vict;
    bool found;
    AFFECT_DATA af;
    int dam;
    int num_blast;

    /* first strike */
    num_blast = level/10;
    act("A blue ray from $n's hand strikes $N.",ch,NULL,victim,TO_NOTVICT);
    act("A blue ray shoots from your finger and strikes $N.",ch,NULL,victim,TO_CHAR);
    act("$n points at you sending a chilling ray into your chest.",ch,NULL,victim,TO_VICT);    

    dam = dice(level/2,10);
            if (!saves_spell(level,victim, DAM_COLD))
    {
        act("$n turns blue and shivers.",victim,NULL,NULL,TO_ROOM);
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 8;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_join( victim, &af );
    }
    else
    {
        dam /= 2;
    }

    damage(ch,victim,dam,sn,DAM_COLD, TRUE);
    last_vict = victim;
    num_blast -= 1;   /* decrement number of blasts */

    /* new targets */
    while (num_blast > 0)
    {
        found = FALSE;
        for (tmp_vict = ch->in_room->people; 
             tmp_vict != NULL; 
             tmp_vict = next_vict) 
        {
          next_vict = tmp_vict->next_in_room;
          if (!is_safe_spell(ch,tmp_vict,TRUE) && tmp_vict != last_vict && num_blast > 0)
          {
            found = TRUE;
            last_vict = tmp_vict;
    act("A blue ray from $n's hand strikes $N.",ch,NULL,tmp_vict,TO_NOTVICT);
    act("A blue ray shoots from your finger and strikes $N."
        ,ch,NULL,tmp_vict,TO_CHAR);
    act("$n points at you sending a chilling ray into your chest."
        ,ch,NULL,tmp_vict,TO_VICT);    

            dam = dice(level/2,10);
            if (!saves_spell(level,tmp_vict, DAM_COLD))
    {
        act("$n turns blue and shivers.",tmp_vict,NULL,NULL,TO_ROOM);
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 8;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_join( tmp_vict, &af );
    }
    else
    {
        dam /= 2;
    }

            damage(ch,tmp_vict,dam,sn,DAM_COLD, TRUE);
            num_blast -= 1;  /* decrement number of blasts */
          }
        }   /* end target searching loop */
        
        if (!found) /* no target found, hit the caster */
        {
          if (ch == NULL)
            return;

          if (last_vict == ch) /* no double hits */
          {
            return;
          }
        
          last_vict = ch;
          num_blast -= 1;  /* decrement number of blasts */
          if (ch == NULL) 
            return;
        }
    /* now go back and find more targets */
    }
}


/* Vision spell similiar to 'at person look' */

void spell_vision( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *original;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   !can_see_room(ch,victim->in_room) 
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */ 
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_MENTAL))
    ||   (IS_NPC(victim) && saves_spell( level, victim, DAM_MENTAL ) )
    ||   chaos )
    {
        send_to_char( "There is a disturbance in the ether.\n\r", ch );
        return;
    }

    
    act("$n falls into a trance.",ch,NULL,NULL,TO_ROOM);
    send_to_char("Through a cloudy fog, you have a vision.\n\r",ch);
    original = ch->in_room;
    char_from_room(ch);
    char_to_room(ch,victim->in_room);

    send_to_char("You feel a shiver down your spine.\n\r",victim);
    do_look(ch,"auto");
    char_from_room(ch);
    char_to_room(ch, original);
}

void spell_restoration( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->hit = UMIN( victim->hit + 175, victim->max_hit );
    update_pos( victim );
    act("$n glows with a blinding light.",ch,NULL,NULL,TO_ROOM);
    send_to_char( "A warm feeling fills your body.\n\r", victim );
    if ( ch != victim )
        send_to_char( "You surge with healing power.\n\r", ch );
    return;
}

void spell_regeneration( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_REGENERATION) )
    {
        if (victim == ch)
          send_to_char("Your body is already in a regenerative state.\n\r",ch);
        else
          act("$N's body is already in a regerative state.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;  /* added by Rahl */
    af.type      = sn;
    af.level     = level;
    af.duration  = level/12;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_REGENERATION;
    affect_to_char( victim, &af );
    send_to_char( "Your body tingles.\n\r", victim );
    if ( ch != victim )
    act("You imbue $N's body with regenerative powers."
        ,ch,NULL,victim,TO_CHAR);
    return;
}

void spell_test_area( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;
    int num_blast;

    num_blast = 5;
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;
        if ( !is_safe_spell(ch,vch,TRUE) && num_blast > 0)
        {
            dam  = dice(10,10);

    act("A jet of flame erupts from $n's hand and engulfs $N.",
        ch,NULL,vch,TO_NOTVICT);
    act("A jet of flames fly from your hand and engulf $N.",
        ch,NULL,vch,TO_CHAR);
    act("A jet of flames flies from $n's hand and engulfs you!",
        ch,NULL,vch,TO_VICT);  

            if ( saves_spell( level, vch, DAM_ENERGY ) )
                dam /= 2;
            damage( ch, vch, dam, sn, DAM_ENERGY, TRUE );
        num_blast -=1;
        }
    }
    return;
}

void spell_web( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    /* added by Rahl */
    if (IS_SET(victim->in_room->room_flags, ROOM_SAFE ))
    {
        send_to_char( "You can't cast that here.\n\r", ch );
        return;
    }

    if ( !IS_NPC( victim ) && (!IS_SET( ch->act, PLR_KILLER ) || !IS_SET(
        victim->act, PLR_KILLER )) )
    {
        send_to_char( "You can't do that.\n\r", ch );
        return;
    }

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already webbed.\n\r",ch);
        else
          act("$N is already webbed.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/3;
    af.location  = APPLY_AC;
    af.modifier  = 80;
    af.bitvector = AFF_WEB;
    affect_to_char( victim, &af );
    send_to_char( "Your are covered in sticky webs!\n\r", victim );
    act("$n is enmeshed in sticky webs!",victim,NULL,NULL,TO_ROOM);
    return;
}

/* mega mana by Rahl */
void spell_mega_mana( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->mana = UMIN( victim->mana + 500, victim->max_mana );
    update_pos( victim );
    send_to_char( "Your magical power increases.\n\r", victim );
    if ( ch != victim )
         send_to_char( "A magical power flows through you.\n\r", ch );
    return;
}


/*
brew.c by Jason Huang (huangjac@netcom.com) 
*/

void spell_imprint( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int       sp_slot, i, mana, lev;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

      if (skill_table[sn].spell_fun == spell_null )
      {
        send_to_char("That is not a spell.\n\r",ch);
        buffer_free( buf );
        return;
      }

    /* counting the number of spells contained within */

    for (sp_slot = i = 1; i < 4; i++) 
        if (obj->value[i] != -1)
            sp_slot++;

    if (sp_slot > 3)
    {
        act ("$p cannot contain any more spells.", ch, obj, NULL, TO_CHAR);
        buffer_free( buf );
        return;
    }

   /* scribe/brew costs 4 times the normal mana required to cast the spell */

   lev = skill_table[sn].skill_level[ch->ch_class];
   mana = 4 * (UMAX(skill_table[sn].min_mana, 100/(2 + ch->level - lev)));
 /*   mana = 4 * mana_cost(ch, sn); */
            
    if ( !IS_NPC(ch) && ch->mana < mana )
    {
        send_to_char( "You don't have enough mana.\n\r", ch );
        buffer_free( buf );
        return;
    }
      

    if ( number_percent( ) > ch->pcdata->learned[sn] )
    {
        send_to_char( "You lost your concentration.\n\r", ch );
        ch->mana -= mana / 2;
        buffer_free( buf );
        return;
    }

    /* executing the imprinting process */
    ch->mana -= mana;
    obj->value[sp_slot] = sn;

    /* Making it successively harder to pack more spells into potions or 
       scrolls - JH */ 

    switch( sp_slot )
    {
   
    default:
        bug( "sp_slot has more than %d spells.", sp_slot );
        return;

    case 1:
        if ( number_percent() > 80 )
        { 
          bprintf(buf, "The magic enchantment has failed --- the %s vanishes.\n\r", 
              item_type_name(obj) );
          send_to_char( buf->data, ch );
          extract_obj( obj );
          buffer_free( buf );
          return;
        }     
        break;
    case 2:
        if ( number_percent() > 25 )
        { 
          bprintf(buf, "The magic enchantment has failed --- the %s vanishes.\n\r",
              item_type_name(obj) );
          send_to_char( buf->data, ch );
          extract_obj( obj );
          buffer_free( buf );
          return;
        }     
        break;

    case 3:
        if ( number_percent() > 10 )
        { 
          bprintf(buf, "The magic enchantment has failed --- the %s vanishes.\n\r", 
              item_type_name(obj) );
          send_to_char( buf->data, ch );
          extract_obj( obj );
          buffer_free( buf );
          return;
        }     
        break;
    } 
  

    /* labeling the item */

    free_string (obj->short_descr);
    bprintf ( buf, "a %s of ", item_type_name(obj) ); 
    for (i = 1; i <= sp_slot ; i++)
      if (obj->value[i] != -1)
      {
        buffer_strcat (buf, skill_table[obj->value[i]].name);
        (i != sp_slot ) ? buffer_strcat (buf, ", ") : 
            buffer_strcat (buf, "") ; 
      }
    obj->short_descr = str_dup(buf->data);
        
    free_string( obj->description );
    bprintf( buf, "%s lies here.", obj->short_descr );
    buf->data[0] = UPPER( buf->data[0] );
    obj->description = str_dup( buf->data );

    bprintf( buf, "%s %s", obj->name, item_type_name(obj) );
    free_string( obj->name );
    obj->name = str_dup( buf->data );        

    bprintf(buf, "You have imbued a new spell to the %s.\n\r", 
        item_type_name(obj) );
    send_to_char( buf->data, ch );

    buffer_free( buf );
    return;
}


/* slow added by Rahl */
void spell_slow( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) || IS_AFFECTED( victim, AFF_SLOW ) )
    {
        if ( victim == ch )
            send_to_char( "You can't move any slower!\n\r", ch );
        else
            act( "$N can't get any slower!", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( saves_spell( level, victim, DAM_OTHER )
    || IS_SET( victim->imm_flags, IMM_MAGIC ) )
    {
        if ( victim != ch )
            send_to_char( "Nothing seemed to happen.\n\r", ch );
        send_to_char( "You feel momentarily lethargic.\n\r", victim );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_HASTE ) )
    {
        if ( !check_dispel( level, victim, skill_lookup( "haste" ) ) )
        {
            if ( victim != ch )
                send_to_char( "Spell failed.\n\r", ch );
            send_to_char( "You feel momentarily slower.\n\r", victim );
            return;
        }

        act( "$n is moving less quickly.", victim, NULL, NULL, TO_ROOM );
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 2;
    af.location  = APPLY_DEX;
    af.modifier  = -1 - ( level >= 18 ) - ( level >= 25 ) - ( level >= 32 );
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );

    send_to_char( "You feel yourself slowing d o w n...\n\r", victim );
    act( "$n starts to move in slow motion.", victim, NULL, NULL, TO_ROOM );
    return;
}

/* heat metal added by Rahl */
void spell_heat_metal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj_lose, *obj_next;
    int dam = 0;
    bool fail = TRUE;
 
   if (!saves_spell(level + 2,victim,DAM_FIRE) 
   &&  !IS_SET(victim->imm_flags,IMM_FIRE))
   {
        for ( obj_lose = victim->carrying;
              obj_lose != NULL; 
              obj_lose = obj_next)
        {
            obj_next = obj_lose->next_content;
            if ( number_range(1,2 * level) > obj_lose->level 
            &&   !saves_spell(level,victim,DAM_FIRE)
/*          &&   !IS_OBJ_STAT(obj_lose,ITEM_NONMETAL)  */
            &&   !IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF))
            {
                switch ( obj_lose->item_type )
                {
                case ITEM_ARMOR:
                if (obj_lose->wear_loc != -1) /* remove the item */
                {
                    if (can_drop_obj(victim,obj_lose)
                    &&  (obj_lose->weight / 10) < 
                        number_range(1,2 * get_curr_stat(victim,STAT_DEX))
                    &&  remove_obj( victim, obj_lose->wear_loc, TRUE ))
                    {
                        act("$n yelps and throws $p to the ground!",
                            victim,obj_lose,NULL,TO_ROOM);
                        act("You remove and drop $p before it burns you.",
                            victim,obj_lose,NULL,TO_CHAR);
                        dam += (number_range(1,obj_lose->level) / 3);
                        obj_from_char(obj_lose);
                        obj_to_room(obj_lose, victim->in_room);
                        fail = FALSE;
                    }
                    else /* stuck on the body! ouch! */
                    {
                        act("Your skin is seared by $p!",
                            victim,obj_lose,NULL,TO_CHAR);
                        dam += (number_range(1,obj_lose->level));
                        fail = FALSE;
                    }

                }
                else /* drop it if we can */
                {
                    if (can_drop_obj(victim,obj_lose))
                    {
                        act("$n yelps and throws $p to the ground!",
                            victim,obj_lose,NULL,TO_ROOM);
                        act("You and drop $p before it burns you.",
                            victim,obj_lose,NULL,TO_CHAR);
                        dam += (number_range(1,obj_lose->level) / 6);
                        obj_from_char(obj_lose);
                        obj_to_room(obj_lose, victim->in_room);
                        fail = FALSE;
                    }
                    else /* cannot drop */
                    {
                        act("Your skin is seared by $p!",
                            victim,obj_lose,NULL,TO_CHAR);
                        dam += (number_range(1,obj_lose->level) / 2);
                        fail = FALSE;
                    }
                }
                break;
                case ITEM_WEAPON:
                if (obj_lose->wear_loc != -1) /* try to drop it */
                {
                    if (IS_WEAPON_STAT(obj_lose,WEAPON_FLAMING))
                                continue;

                    if (can_drop_obj(victim,obj_lose) 
                    &&  remove_obj(victim,obj_lose->wear_loc,TRUE))
                    {
                        act("$n is burned by $p, and throws it to the ground.",
                            victim,obj_lose,NULL,TO_ROOM);
                        send_to_char(
                            "You throw your red-hot weapon to the ground!\n\r",
                            victim);
                        dam += 1;
                        obj_from_char(obj_lose);
                        obj_to_room(obj_lose,victim->in_room);
                        fail = FALSE;
                    }
                    else /* YOWCH! */
                    {
                        send_to_char("Your weapon sears your flesh!\n\r",
                            victim);
                        dam += number_range(1,obj_lose->level);
                        fail = FALSE;
                    }
                }
                else /* drop it if we can */
                {
                    if (can_drop_obj(victim,obj_lose))
                    {
                        act("$n throws a burning hot $p to the ground!",
                            victim,obj_lose,NULL,TO_ROOM);
                        act("You and drop $p before it burns you.",
                            victim,obj_lose,NULL,TO_CHAR);
                        dam += (number_range(1,obj_lose->level) / 6);
                        obj_from_char(obj_lose);
                        obj_to_room(obj_lose, victim->in_room);
                        fail = FALSE;
                    }
                    else /* cannot drop */
                     {
                        act("Your skin is seared by $p!",
                            victim,obj_lose,NULL,TO_CHAR);
                        dam += (number_range(1,obj_lose->level) / 2);
                        fail = FALSE;
                    }
                }
                break;
                }
            }
        }
    } 
    if (fail)
    {
        send_to_char("Your spell had no effect.\n\r", ch);
        send_to_char("You feel momentarily warmer.\n\r",victim);
    }
    else /* damage! */
    {
        if (saves_spell(level,victim,DAM_FIRE))
            dam = 2 * dam / 3;
        damage( ch, victim, dam, sn, DAM_FIRE, TRUE );
    }
}

/* ray of truth added by Rahl */
void spell_ray_of_truth (int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, align;
 
    if (IS_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The energy explodes inside you!\n\r",ch);
    }
 
    if (victim != ch)
    {
        act("$n raises $s hand, and a blinding ray of light shoots forth!",
            ch,NULL,NULL,TO_ROOM);
        send_to_char(
           "You raise your hand and a blinding ray of light shoots forth!\n\r",
           ch);
    }

    if (IS_GOOD(victim))
    {
        act("$n seems unharmed by the light.",victim,NULL,victim,TO_ROOM);
        send_to_char("The light seems powerless to affect you.\n\r",victim);
        return;
    }

    dam = dice( level, 10 );
    if ( saves_spell( level, victim,DAM_HOLY) )
        dam /= 2;

    align = victim->alignment;
    align -= 350;

    if (align < -1000)
        align = -1000 + (align + 1000) / 3;

    dam = (dam * align * align) / 1000000;

    damage( ch, victim, dam, sn, DAM_HOLY, TRUE );
    spell_blindness(gsn_blindness, 
        3 * level / 4, ch, (void *) victim,TARGET_CHAR);
}


/* recharge added by Rahl */
void spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int chance, percent;

    if ((obj->item_type != ITEM_WAND) && (obj->item_type != ITEM_STAFF))
    {
        send_to_char("That item does not carry charges.\n\r",ch);
        return;
    }

    if (obj->value[3] >= 3 * level / 2)
    {
        send_to_char("Your skills are not great enough for that.\n\r",ch);
        return;
    }

    if (obj->value[1] == 0)
    {
        send_to_char("That item has already been recharged once.\n\r",ch);
        return;
    }

    chance = 40 + 2 * level;

    chance -= obj->value[3]; /* harder to do high-level spells */
    chance -= (obj->value[1] - obj->value[2]) *
              (obj->value[1] - obj->value[2]);


    chance = UMAX(level/2,chance);

    percent = number_percent();

    if (percent < chance / 2)
    {
        act("$p glows softly.",ch,obj,NULL,TO_CHAR);
        act("$p glows softly.",ch,obj,NULL,TO_ROOM);
        obj->value[2] = UMAX(obj->value[1],obj->value[2]);
        obj->value[1] = 0;
        return;
    }

    else if (percent <= chance)
    {
        int chargeback,chargemax;

        act("$p glows softly.",ch,obj,NULL,TO_CHAR);
        act("$p glows softly.",ch,obj,NULL,TO_CHAR);

        chargemax = obj->value[1] - obj->value[2];
        
        if (chargemax > 0)
            chargeback = UMAX(1,chargemax * percent / 100);
        else
            chargeback = 0;

        obj->value[2] += chargeback;
        obj->value[1] = 0;
        return;
    }   

    else if (percent <= UMIN(95, 3 * chance / 2))
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        if (obj->value[1] > 1)
            obj->value[1]--;
        return;
    }

    else /* whoops! */
    {
        act("$p glows brightly and explodes!",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly and explodes!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
    }
}


void spell_portal( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;

        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   (chaos)
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   !can_see_room(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) ) 
  /*  ||        (is_clan(victim) && !is_same_clan(ch,victim))) */
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   

    stone = get_eq_char(ch,WEAR_HOLD);
    if (!IS_IMMORTAL(ch) 
    &&  (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\n\r",ch);
        return;
    }

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR);
        act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR);
        extract_obj(stone);
    }

    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 2 + level / 25; 
    portal->value[3] = victim->in_room->vnum;

    obj_to_room(portal,ch->in_room);

    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR);
}

void spell_nexus( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;
    ROOM_INDEX_DATA *to_room, *from_room;

    from_room = ch->in_room;
 
        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   (chaos)
    ||   victim == ch
    ||   (to_room = victim->in_room) == NULL
    ||   !can_see_room(ch,to_room) || !can_see_room(ch,from_room)
    ||   IS_SET(to_room->room_flags, ROOM_SAFE)
    ||   IS_SET(from_room->room_flags,ROOM_SAFE)
    ||   IS_SET(to_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(to_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(to_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(from_room->room_flags,ROOM_NO_RECALL)
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) ) 
 /*   ||         (is_clan(victim) && !is_same_clan(ch,victim))) */
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   
 
    stone = get_eq_char(ch,WEAR_HOLD);
    if (!IS_IMMORTAL(ch)
    &&  (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\n\r",ch);
        return;
    }
 
    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR);
        act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR);
        extract_obj(stone);
    }

    /* portal one */ 
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 1 + level / 10;
    portal->value[3] = to_room->vnum;
 
    obj_to_room(portal,from_room);
 
    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR);

    /* no second portal if rooms are the same */
    if (to_room == from_room)
        return;

    /* portal two */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 1 + level/10;
    portal->value[3] = from_room->vnum;

    obj_to_room(portal,to_room);

    if (to_room->people != NULL)
    {
        act("$p rises up from the ground.",to_room->people,portal,NULL,TO_ROOM);
        act("$p rises up from the ground.",to_room->people,portal,NULL,TO_CHAR);
    }
}

/* 
 * Resurrect origiannly by Dribble. Added to Shadows by Rahl
 * (with slight modifications, of course)
 */
void spell_resurrect( int sn, int level, CHAR_DATA *ch, void *vo, int
        target )
{
    OBJ_DATA *obj;
    CHAR_DATA *mob;
    int i;

    obj = get_obj_here( ch, target_name );

    if ( obj == NULL )
    {
        send_to_char( "Resurrect what?\n\r", ch );
        return;
    }

    /* nothing but NPC corpses */

    if ( obj->item_type != ITEM_CORPSE_NPC )
    {
        if ( obj->item_type == ITEM_CORPSE_PC )
            send_to_char( "You can't resurrect players.\n\r", ch );
        else
            send_to_char( "You can't resurrect inanimate objects.\n\r", ch );
        return;
    }

    if ( obj->level > ( ch->level + 2 ) )
    {
        send_to_char( "You couldn't call forth such a great spirit.\n\r",
                ch );
        return;
    }

    if ( ch->pet != NULL )
    {
        send_to_char( "You already have a pet.\n\r", ch );
        return;
    }

    /* 
     * chew on the zombie a little bit, recalculate level-dependent stats
     */
    mob = create_mobile( get_mob_index( MOB_VNUM_ZOMBIE ) );

    mob->level          = obj->level;
    mob->max_hit        = mob->level * 8 + number_range(
                                mob->level * mob->level / 4,
                                mob->level * mob->level );
    mob->max_hit *= .9;
    mob->hit            = mob->max_hit;
    mob->max_mana       = 100 + dice( mob->level, 10 );
    mob->mana           = mob->max_mana;
    for ( i = 0; i < 3; i++ )
    {
        mob->armor[i]   = interpolate( mob->level, 100, -100 );
    }
    mob->armor[3]       = interpolate( mob->level, 100, 0 );

    for ( i = 0; i < MAX_STATS; i++ )
    {
        mob->perm_stat[i] = 11 + mob->level / 4;
    }

    if ( mob->level > 10 )
    {
        mob->damage[DICE_NUMBER] = mob->level / 10;
        mob->damage[DICE_TYPE]   = mob->level / 5;
        mob->damroll             = mob->level / 2 - 15;
    }

    /* you rang? */
    char_to_room( mob, ch->in_room );
    act( "$p springs to life as a hideous zombie!", ch, obj, NULL, TO_CHAR ); 
    act( "$p springs to life as a hideous zombie!", ch, obj, NULL, TO_ROOM );

    extract_obj ( obj );

    /* Yessss, masssster... */
    SET_BIT( mob->affected_by, AFF_CHARM );
    SET_BIT( mob->act, ACT_PET );
    mob->comm = COMM_NOTELL|COMM_NOCHANNELS; 
    add_follower( mob, ch );
    mob->leader = ch;
    ch->pet = mob;
    /* for a little flavor */
    do_say( mob, "How may I serve you, master?" );

    return;
}


/* Original Code by Jason Huang (god@sure.net)                  */
/* Permission to use this code is granted provided this header  */
/* is retained an unaltered.                                    */

/*
 * Modified for use with Shadows by Rahl 
 */
void spell_fear( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;

    victim = get_char_room( ch, target_name );

    if ( victim == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( ch->fighting != NULL )
        victim = ch->fighting;

    if  ( victim == ch
        || !victim->in_room
        || IS_SET( victim->in_room->room_flags, ROOM_SAFE )
        || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
        || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
        || IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
        || victim->level >= level
        || victim->in_room->area != ch->in_room->area
        || ( IS_NPC( victim ) && saves_spell( level, victim, DAM_OTHER ) ) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

    do_flee( victim, "" );
    return;
}

void spell_blink( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;
    AFFECT_DATA af;

    victim = get_char_room( ch, target_name );

    if ( ch != victim )
    {
        send_to_char( "You may not cast this spell on another.\n\r", ch );
        return;
    }

    if ( IS_AFFECTED2( ch, AFF_BLINK ) )
    {   
        send_to_char( "You are already out of phase.\n\r", ch );
        return;
    }

    af.where = TO_AFFECTS2;
    af.type = sn;
    af.level = level;
    af.duration = level / 3;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_BLINK;

    affect_to_char( ch, &af );

    send_to_char( "You flicker for a moment as you body loses"
        " consistency.\n\r", ch );
    act( "$n blinks out of existence for a moment.", ch, NULL, NULL, 
        TO_ROOM );
    return;
}
