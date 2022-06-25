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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include <stdlib.h>
#include "merc.h"
#include "music.h"
#include "interp.h"


/*
 * Local functions.
 */
int     hit_gain        ( CHAR_DATA *ch );
int     mana_gain       ( CHAR_DATA *ch );
int     move_gain       ( CHAR_DATA *ch );
void    mobile_update   ( void );
void    weather_update  ( void );
void    char_update     ( void );
void    regen_update    ( void );
void    obj_update      ( void );
void    aggr_update     ( void );
void    room_update     ( void );

/* added by Rahl for autoquesting */
void    quest_update    ( void ); /* quest.c */
/* added by Rahl */
void    auction_update  ( void );
void    spell_update    ( void );

/* used for saving */

int     save_number = 0;



/*
 * Advancement stuff.
 */
void advance_level( CHAR_DATA *ch )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

    ch->pcdata->last_level = 
        ( ch->played + (int) (current_time - ch->logon) ) / 3600;
    add_hp      = con_app[get_curr_stat(ch,STAT_CON)].hitp + number_range(
                    class_table[ch->ch_class].hp_min,
                    class_table[ch->ch_class].hp_max );
    add_mana    = number_range(2,(2*get_curr_stat(ch,STAT_INT)
                                  + get_curr_stat(ch,STAT_WIS))/5);
    if (!class_table[ch->ch_class].fMana)
        add_mana /= 2;
    add_move    = number_range( 1, (get_curr_stat(ch,STAT_CON)
                                  + get_curr_stat(ch,STAT_DEX))/6 );
    add_prac    = wis_app[get_curr_stat(ch,STAT_WIS)].practice;

    add_hp = add_hp * 9/10;
    add_mana = add_mana * 9/10;
    add_move = add_move * 9/10;

    add_hp      = UMAX(  1, add_hp   );
    add_mana    = UMAX(  1, add_mana );
    add_move    = UMAX(  6, add_move );

    ch->exp             -= exp_per_level(ch,ch->pcdata->points);
    ch->max_hit         += add_hp;
    ch->max_mana        += add_mana;
    ch->max_move        += add_move;
    ch->practice        += add_prac;
    ch->train           += 1;

    ch->pcdata->perm_hit        += add_hp;
    ch->pcdata->perm_mana       += add_mana;
    ch->pcdata->perm_move       += add_move;

    if ( !IS_NPC(ch) )
        REMOVE_BIT( ch->act, PLR_BOUGHT_PET );

    bprintf( buf,
        "Your gain is: %d/%d hp, %d/%d m, %d/%d mv %d/%d prac.\n\r",
        add_hp,         ch->max_hit,
        add_mana,       ch->max_mana,
        add_move,       ch->max_move,
        add_prac,       ch->practice
        );
    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}   



void gain_exp( CHAR_DATA *ch, int gain )
{
    if ( IS_NPC(ch) || ch->level >= LEVEL_HERO )
        return;

/*    ch->exp = UMAX( exp_per_level(ch,ch->pcdata->points), ch->exp + gain );*/
    ch->exp+=gain;
    while ( ch->level < LEVEL_HERO && ch->exp >= 
        exp_per_level(ch,ch->pcdata->points))
    {
        BUFFER *buf = buffer_new( 200 );
        
        bprintf(buf, "%s has made it to level %d!", ch->name, ch->level + 1);
        /* added by Rahl */
        wiznet( buf->data, ch, NULL, WIZ_LEVELS, 0, 0 );

        do_sendinfo(ch, buf->data);
        send_to_char( "You raise a level!!  ", ch );
        advance_level( ch );
        ch->level += 1;
        save_char_obj(ch);
        buffer_free( buf );
    }

    return;
}



/*
 * Regeneration stuff.
 */
int hit_gain( CHAR_DATA *ch )
{
    int gain;
    int number;

    if ( IS_NPC(ch) )
    {
        gain =  5 + ch->level;

        switch(ch->position)
        {
            default :           gain /= 2;                      break;
            case POS_SLEEPING:  gain = 3 * gain/2;              break;
            case POS_RESTING:                                   break;
            case POS_FIGHTING:  gain /= 3;                      break;
        }

        
    }
    else
    {
        gain = UMAX(3,get_curr_stat(ch,STAT_CON) - 3 + ch->level/2); 
        gain += class_table[ch->ch_class].hp_max - 10;
        number = number_percent();
        if (number < ch->pcdata->learned[gsn_fast_healing])
        {
            gain += number * gain / 100;
            if (ch->hit < ch->max_hit)
                check_improve(ch,gsn_fast_healing,TRUE,8);
        }

        switch ( ch->position )
        {
            default:            gain /= 4;                      break;
            case POS_SLEEPING:                                  break;
            case POS_RESTING:   gain /= 2;                      break;
            case POS_FIGHTING:  gain /= 6;                      break;
        }

        if ( ch->pcdata->condition[COND_FULL]   == 0 )
            gain /= 2;

        if ( ch->pcdata->condition[COND_THIRST] == 0 )
            gain /= 2;

    }

    if ( IS_AFFECTED(ch, AFF_POISON) )
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED( ch, AFF_SLOW ) )
        gain /=2 ;

    if (chaos)
        gain /=2 ;
  
    return UMIN(gain, ch->max_hit - ch->hit);
}



int mana_gain( CHAR_DATA *ch )
{
    int gain;
    int number;

    if ( IS_NPC(ch) )
    {
        gain = 5 + ch->level;
        switch (ch->position)
        {
            default:            gain /= 2;              break;
            case POS_SLEEPING:  gain = 3 * gain/2;      break;
            case POS_RESTING:                           break;
            case POS_FIGHTING:  gain /= 3;              break;
        }
    }
    else
    {
        gain = (get_curr_stat(ch,STAT_WIS) 
              + get_curr_stat(ch,STAT_INT) + ch->level) / 2;
        number = number_percent();
        if (number < ch->pcdata->learned[gsn_meditation])
        {
            gain += number * gain / 100;
            if (ch->mana < ch->max_mana)
                check_improve(ch,gsn_meditation,TRUE,8);
        }
        if (!class_table[ch->ch_class].fMana)
            gain /= 2;

        switch ( ch->position )
        {
            default:            gain /= 4;                      break;
            case POS_SLEEPING:                                  break;
            case POS_RESTING:   gain /= 2;                      break;
            case POS_FIGHTING:  gain /= 6;                      break;
        }

        if ( ch->pcdata->condition[COND_FULL]   == 0 )
            gain /= 2;

        if ( ch->pcdata->condition[COND_THIRST] == 0 )
            gain /= 2;

    }

    if ( IS_AFFECTED( ch, AFF_POISON ) )
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED( ch, AFF_SLOW ) )
        gain /=2 ;

    if (chaos)
        gain /=2 ;

    return UMIN(gain, ch->max_mana - ch->mana);
}



int move_gain( CHAR_DATA *ch )
{
    int gain;

    if ( IS_NPC(ch) )
    {
        gain = ch->level;
    }
    else
    {
        gain = UMAX( 15, ch->level );

        switch ( ch->position )
        {
        case POS_SLEEPING: gain += get_curr_stat(ch,STAT_DEX);          break;
        case POS_RESTING:  gain += get_curr_stat(ch,STAT_DEX) / 2;      break;
        }

        if ( ch->pcdata->condition[COND_FULL]   == 0 )
            gain /= 2;

        if ( ch->pcdata->condition[COND_THIRST] == 0 )
            gain /= 2;
    }

    if ( IS_AFFECTED(ch, AFF_POISON) )
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED( ch, AFF_SLOW ) )
        gain /=2 ;

    return UMIN(gain, ch->max_move - ch->move);
}



void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
    int condition;

    if ( value == 0 || IS_NPC(ch) || ch->level >= LEVEL_HERO)
        return;

    condition                           = ch->pcdata->condition[iCond];
    if (condition == -1)
        return;
    ch->pcdata->condition[iCond]        = URANGE( 0, condition + value, 48 );

    if ( ch->pcdata->condition[iCond] == 0 )
    {
        switch ( iCond )
        {
        case COND_FULL:
            send_to_char( "You are hungry.\n\r",  ch );
            break;

        case COND_THIRST:
            send_to_char( "You are thirsty.\n\r", ch );
            break;

        case COND_DRUNK:
            if ( condition != 0 )
                send_to_char( "You are sober.\n\r", ch );
            break;
        }
    }

    return;
}



/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    EXIT_DATA *pexit;
    int door;

    /* Examine all mobs. */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;

        if ( !IS_NPC(ch) || ch->in_room == NULL || IS_AFFECTED(ch,AFF_CHARM))
            continue;

        if (ch->in_room->area->empty && !IS_SET(ch->act,ACT_UPDATE_ALWAYS))
            continue;

        /* Examine call for special procedure */
        if ( ch->spec_fun != 0 )
        {
            if ( (*ch->spec_fun) ( ch ) )
                continue;
        }

        /* That's all for sleeping / busy monster, and empty zones */
        if ( ch->position != POS_STANDING )
            continue;

        /* Scavenge */
        if ( IS_SET(ch->act, ACT_SCAVENGER)
        &&   ch->in_room->contents != NULL
        &&   number_bits( 6 ) == 0 )
        {
            OBJ_DATA *obj;
            OBJ_DATA *obj_best;
            int max;

            max         = 1;
            obj_best    = 0;
            for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
            {
                if ( CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj)
                     && obj->cost > max  && obj->cost > 0)
                {
                    obj_best    = obj;
                    max         = obj->cost;
                }
            }

            if ( obj_best )
            {
                obj_from_room( obj_best );
                obj_to_char( obj_best, ch );
                act( "$n gets $p.", ch, obj_best, NULL, TO_ROOM );
            }
        }

        /* Wander */
        if ( !IS_SET(ch->act, ACT_SENTINEL) 
        && number_bits(4) == 0
        && ( door = number_bits( 5 ) ) <= 5
        && ( pexit = ch->in_room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL
        &&   !IS_SET(pexit->exit_info, EX_CLOSED)
        &&   !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
        && ( !IS_SET(ch->act, ACT_STAY_AREA)
        ||   pexit->u1.to_room->area == ch->in_room->area ) )
        {
            move_char( ch, door, FALSE );
            if ( ch->position < POS_STANDING )
                continue;
        }

/*       Flee
        if ( ch->hit < ch->max_hit / 2
        && ( door = number_bits( 3 ) ) <= 5
        && ( pexit = ch->in_room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL
        &&   !IS_SET(pexit->exit_info, EX_CLOSED)
        &&   !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) )
        {
            CHAR_DATA *rch;
            bool found;

            found = FALSE;
            for ( rch  = pexit->u1.to_room->people;
                  rch != NULL;
                  rch  = rch->next_in_room )
            {
                if ( !IS_NPC(rch) )
                {
                    found = TRUE;
                    break;
                }
            }
            if ( !found )
                move_char( ch, door, FALSE );
        }
*/

    }

    return;
}



/*
 * Update the weather.
 */
void weather_update( void )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int diff;

    buf[0] = '\0';

    switch ( ++time_info.hour )
    {
    case  5:
        weather_info.sunlight = SUN_LIGHT;
        strcat( buf, "The day has begun.\n\r" );
        break;

    case  6:
        weather_info.sunlight = SUN_RISE;
        strcat( buf, "The sun rises in the east.\n\r" );
        break;

    case 19:
        weather_info.sunlight = SUN_SET;
        strcat( buf, "The sun slowly disappears in the west.\n\r" );
        break;

    case 20:
        weather_info.sunlight = SUN_DARK;
        strcat( buf, "The night has begun.\n\r" );
        break;

    case 24:
        time_info.hour = 0;
        time_info.day++;
        break;
    }

    if ( time_info.day   >= 35 )
    {
        time_info.day = 0;
        time_info.month++;
    }

    if ( time_info.month >= 17 )
    {
        time_info.month = 0;
        time_info.year++;
    }

    /*
     * Weather change.
     */
    if ( time_info.month >= 9 && time_info.month <= 16 )
        diff = weather_info.mmhg >  985 ? -2 : 2;
    else
        diff = weather_info.mmhg > 1015 ? -2 : 2;

    weather_info.change   += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    weather_info.change    = UMAX(weather_info.change, -12);
    weather_info.change    = UMIN(weather_info.change,  12);

    weather_info.mmhg += weather_info.change;
    weather_info.mmhg  = UMAX(weather_info.mmhg,  960);
    weather_info.mmhg  = UMIN(weather_info.mmhg, 1040);

    switch ( weather_info.sky )
    {
    default: 
        bug( "Weather_update: bad sky %d.", weather_info.sky );
        weather_info.sky = SKY_CLOUDLESS;
        break;

    case SKY_CLOUDLESS:
        if ( weather_info.mmhg <  990
        || ( weather_info.mmhg < 1010 && number_bits( 2 ) == 0 ) )
        {
            strcat( buf, "The sky is getting cloudy.\n\r" );
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_CLOUDY:
        if ( weather_info.mmhg <  970
        || ( weather_info.mmhg <  990 && number_bits( 2 ) == 0 ) )
        {
            strcat( buf, "It starts to rain.\n\r" );
            weather_info.sky = SKY_RAINING;
        }

        if ( weather_info.mmhg > 1030 && number_bits( 2 ) == 0 )
        {
            strcat( buf, "The clouds disappear.\n\r" );
            weather_info.sky = SKY_CLOUDLESS;
        }
        break;

    case SKY_RAINING:
        if ( weather_info.mmhg <  970 && number_bits( 2 ) == 0 )
        {
            strcat( buf, "Lightning flashes in the sky.\n\r" );
            weather_info.sky = SKY_LIGHTNING;
        }

        if ( weather_info.mmhg > 1030
        || ( weather_info.mmhg > 1010 && number_bits( 2 ) == 0 ) )
        {
            strcat( buf, "The rain stopped.\n\r" );
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_LIGHTNING:
        if ( weather_info.mmhg > 1010
        || ( weather_info.mmhg >  990 && number_bits( 2 ) == 0 ) )
        {
            strcat( buf, "The lightning has stopped.\n\r" );
            weather_info.sky = SKY_RAINING;
            break;
        }
        break;
    }

    if ( buf[0] != '\0' )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->connected == CON_PLAYING
            &&   IS_OUTSIDE(d->character)
            &&   IS_AWAKE(d->character) )
                send_to_char( buf, d->character );
        }
    }

    return;
}



/*
 * Update all chars, including mobs.
*/
void char_update( void )
{   
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *ch_quit;

    ch_quit     = NULL;

    /* update save counter */
    save_number++;

    if (save_number > 30)
        save_number = 0;

    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        AFFECT_DATA *paf;
        AFFECT_DATA *paf_next;

        ch_next = ch->next;

        if ( (ch->timer > 30) && ch->level < LEVEL_IMMORTAL )
            ch_quit = ch;

    
        // imms who have been linkdead for 30 ticks get
        // booted -- Rahl 
        if ( ( ch->level >= LEVEL_IMMORTAL ) 
		&& !IS_NPC( ch )
        && ( ch->desc == NULL )
        && ( ch->timer > 30 ) ) {
            ch_quit = ch;
        }       

		if ( ( ch->timer > ( IS_IMMORTAL( ch ) ? 20 : 10 ) ) 
		&& !IS_SET( ch->act, PLR_AFK ) && ( ch->desc != NULL ) ) {
			do_afk( ch, "" );
		}

        if ( ch->position >= POS_STUNNED )
        {
            /* check to see if we need to go home */
            if (IS_NPC(ch) && ch->zone != NULL && ch->zone != ch->in_room->area
            && ch->desc == NULL &&  ch->fighting == NULL
            && !IS_AFFECTED(ch,AFF_CHARM) && number_percent() < 5)
            {
                act("$n wanders on home.",ch,NULL,NULL,TO_ROOM);
                extract_char(ch,TRUE);
                continue;
            }   

            /* 
             * Pets who have become 'lost' (no charm) are no longer
             * pets. If they're sentinel, it's removed so they can 
             * wander around, get killed, etc -- Rahl
             */
            if ( IS_NPC( ch ) && IS_SET( ch->act, ACT_PET ) 
            && !IS_AFFECTED( ch, AFF_CHARM ) )
            {
                /* pull off the sentinel bit (if there is one) */
                if ( IS_SET( ch->act, ACT_SENTINEL ) )
                    REMOVE_BIT( ch->act, ACT_SENTINEL );

                /* pull off the pet bit */
                REMOVE_BIT( ch->act, ACT_PET );

                /* clear the old desc (which had the owner's name) */
                free_string( ch->description );

                /* make the new desc the same as the ones in the stock */
                ch->description = strdup( ch->pIndexData->description );

                /* set the zone to the current area */
                /* (so they won't "wander home") */
				if ( ( ch->in_room != NULL ) 
				&& ( ch->in_room->area != NULL ) ) {
	                ch->zone = ch->in_room->area;
				}
            }

            if ( ch->hit < ch->max_hit )
                ch->hit  += hit_gain(ch);
            else
                ch->hit = ch->max_hit;

            if ( ch->mana < ch->max_mana )
                ch->mana += mana_gain(ch);
            else
                ch->mana = ch->max_mana;

            if ( ch->move < ch->max_move )
                ch->move += move_gain(ch);
            else
                ch->move = ch->max_move;
        }

        if ( ch->position == POS_STUNNED )
            update_pos( ch );

        if ( !IS_NPC(ch) )
        {
            OBJ_DATA *obj;

            if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
            &&   obj->item_type == ITEM_LIGHT
            &&   obj->value[2] > 0 )
            {
                if ( --obj->value[2] == 0 && ch->in_room != NULL )
                {
                    --ch->in_room->light;
                    act( "$p goes out.", ch, obj, NULL, TO_ROOM );
                    act( "$p flickers and goes out.", ch, obj, NULL, TO_CHAR );
                    extract_obj( obj );
                }
                else if ( obj->value[2] <= 5 && ch->in_room != NULL)
                    act("$p flickers.",ch,obj,NULL,TO_CHAR);
            }

            ++ch->timer;

            if ( ch->timer >= 12 && ch->level < LEVEL_IMMORTAL )
            {
                if ( ch->was_in_room == NULL && ch->in_room != NULL )
                {
                    ch->was_in_room = ch->in_room;
                    if ( ch->fighting != NULL )
                        stop_fighting( ch, TRUE );
                    act( "$n disappears into the void.",
                        ch, NULL, NULL, TO_ROOM );
                    send_to_char( "You disappear into the void.\n\r", ch );
                    if (ch->level > 1)
                        save_char_obj( ch );
                    char_from_room( ch );
                    char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
                }
            }
            if (ch->level < LEVEL_IMMORTAL )
           {
            gain_condition( ch, COND_DRUNK,  -1 * time_info.hour % 2 );
            gain_condition( ch, COND_FULL,   -1 * time_info.hour % 2 );
            gain_condition( ch, COND_THIRST, -1 * time_info.hour % 2 );
           }
        }

        for ( paf = ch->affected; paf != NULL; paf = paf_next )
        {
            paf_next    = paf->next;
            if ( paf->duration > 0 )
            {
                paf->duration--;
                if (number_range(0,4) == 0 && paf->level > 0)
                  paf->level--;  /* spell strength fades with time */
            }
            else if ( paf->duration < 0 )
                ;
            else
            {
                if ( paf_next == NULL
                ||   paf_next->type != paf->type
                ||   paf_next->duration > 0 )
                {
                    if ( paf->type > 0 && skill_table[paf->type].msg_off )
                    {
                        send_to_char( skill_table[paf->type].msg_off, ch );
                        send_to_char( "\n\r", ch );
                    }
                }
          
                affect_remove( ch, paf );
            }
        }

        /*
         * Careful with the damages here,
         *   MUST NOT refer to ch after damage taken,
         *   as it may be lethal damage (on NPC).
         */

        if (is_affected(ch, gsn_plague) && ch != NULL)
        {
            AFFECT_DATA *af, plague;
            CHAR_DATA *vch;
            int save, dam;

            if (ch->in_room == NULL)
                continue;  /* changed from return */
            
            act("$n writhes in agony as plague sores erupt from $s skin.",
                ch,NULL,NULL,TO_ROOM);
            send_to_char("You writhe in agony from the plague.\n\r",ch);
            for ( af = ch->affected; af != NULL; af = af->next )
            {
                if (af->type == gsn_plague)
                    break;
            }
        
            if (af == NULL)
            {
                REMOVE_BIT(ch->affected_by,AFF_PLAGUE);
                continue; /* was return */
            }
        
            if (af->level == 1)
                continue; /* was return */
        
            plague.where                = TO_AFFECTS;
            plague.type                 = gsn_plague;
            plague.level                = af->level - 1; 
            plague.duration     = number_range(1,2 * plague.level);
            plague.location             = APPLY_STR;
            plague.modifier     = -5;
            plague.bitvector    = AFF_PLAGUE;
        
            for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                switch(check_immune(vch,DAM_DISEASE))
                {
                    case(IS_NORMAL)     : save = af->level - 4; break;
                    case(IS_IMMUNE)     : save = 0;             break;
                    case(IS_RESISTANT)  : save = af->level - 8; break;
                    case(IS_VULNERABLE) : save = af->level;     break;
                    default                     : save = af->level - 4; break;
                }
            
                if (save != 0 && !saves_spell(save,vch, DAM_DISEASE) 
                && !IS_IMMORTAL(vch)
                &&  !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0)
                {
                    send_to_char("You feel hot and feverish.\n\r",vch);
                    act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM);
                    affect_join(vch,&plague);
                }
            }

            dam = UMIN(ch->level,5);
            ch->mana -= dam;
            ch->move -= dam;
            damage( ch, ch, dam, gsn_plague,DAM_DISEASE, TRUE );
        }
        else if ( IS_AFFECTED(ch, AFF_POISON) && ch != NULL 
                && !IS_AFFECTED( ch, AFF_SLOW ))
        {
            act( "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You shiver and suffer.\n\r", ch );
            damage( ch, ch, 2, gsn_poison, DAM_POISON, TRUE );
        }
        else if ( ch->position == POS_INCAP && number_range(0,1) == 0)
        {
            damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, TRUE );
        }
        else if ( ch->position == POS_MORTAL )
        {
            damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, TRUE );
        }
    }

    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;

        if (ch->desc != NULL && save_number == 30
                && (!chaos) )
            save_char_obj(ch);

        if ( ch == ch_quit )
            do_quit( ch, "" );
    }

    return;
}


void regen_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;

    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;

        if ( ch->position >= POS_STUNNED && IS_AFFECTED(ch, AFF_REGENERATION ) ) 
       {
            if ( ch->hit  < ch->max_hit )
                ch->hit  += hit_gain(ch);
            else
                ch->hit = ch->max_hit;
        }
    }
    return;
}

void room_update( void )
{
   /* char               buf[MAX_STRING_LENGTH]; */
   DESCRIPTOR_DATA   *d;
   CHAR_DATA         *ch;
   
   for (d = descriptor_list; d != NULL; d = d->next )
     {
        if ( d->connected == CON_PLAYING)
          {
             ch = d->character;
          }
     }
   return;
}

                  

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{   
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf, *paf_next;

    for ( obj = object_list; obj != NULL; obj = obj_next )
    {
        CHAR_DATA *rch;
        char *message;

        obj_next = obj->next;

        /* go through affects and decrement */
        for ( paf = obj->affected; paf != NULL; paf = paf_next )
        {
            paf_next    = paf->next;
            if ( paf->duration > 0 )
            {
                paf->duration--;
                if (number_range(0,4) == 0 && paf->level > 0)
                  paf->level--;  /* spell strength fades with time */
            }
            else if ( paf->duration < 0 )
                ;
            else
            {
                if ( paf_next == NULL
                ||   paf_next->type != paf->type
                ||   paf_next->duration > 0 )
                {
                /*
                 * msg_off changed to msg_obj by Rahl.. a few other small
                 * changes in this if statement, too 
                 */
                    if ( paf->type > 0 && skill_table[paf->type].msg_obj )
                    {
                        /* added by Rahl */
                        if ( obj->carried_by != NULL )
                        {
                            rch = obj->carried_by;
                            act( skill_table[paf->type].msg_obj, 
                                rch, obj, NULL, TO_CHAR );
                        }
                        if ( obj->in_room != NULL 
                        && obj->in_room->people != NULL )
                        {
                            rch = obj->in_room->people;
                            act( skill_table[paf->type].msg_obj,
                                rch, obj, NULL,TO_ALL );
                        }
                        /* end junk by Rahl */
                    }
                }

                affect_remove_obj( obj, paf );
            }
        }


        if ( obj->timer <= 0 || --obj->timer > 0 )
        /* 
         * objects in air fall to the ground below. Added by Rahl,
         * based on code I got from Talon at Lunar Eclipse, which
         * he found on the net somewhere. Unfortuantely, the author
         * didn't put his name on the code I have, so I can't give him
         * the credit he deserves
         */
        {
            if ( obj->in_room 
            && obj->in_room->sector_type == SECT_AIR
            && ( obj->wear_flags & ITEM_TAKE ) 
            && obj->in_room->exit[5]
            && obj->in_room->exit[5]->u1.to_room )
            {
                ROOM_INDEX_DATA *new_room = obj->in_room->exit[5]->u1.to_room;
                
                if ( ( rch = obj->in_room->people ) != NULL )
                {
                    act( "$p falls away.", rch, obj, NULL, TO_ALL );
                }

                obj_from_room( obj );
                obj_to_room( obj, new_room ); 

                if ( ( rch = obj->in_room->people ) != NULL )
                {
                    act( "$p floats by.", rch, obj, NULL, TO_ALL );
                }
            }
            continue;
        }

        switch ( obj->item_type )
        {
        default:              message = "$p crumbles into dust.";  break;
        case ITEM_FOUNTAIN:   message = "$p dries up.";         break;
        case ITEM_CORPSE_NPC: message = "$p decays into dust."; break;
        case ITEM_CORPSE_PC:  message = "$p decays into dust."; break;
        case ITEM_FOOD:       message = "$p decomposes.";       break;
        case ITEM_POTION:     message = "$p has evaporated from disuse.";       
                                                                break;
        case ITEM_WEAPON:     message = "$p flickers and disappears."; break;
        case ITEM_KEY:        message = "$p flickers and disappears."; break;
        case ITEM_PORTAL:     message = "$p fades out of existence."; break;
        }

        if ( obj->carried_by != NULL )
        {
            if (IS_NPC(obj->carried_by) 
            &&  obj->carried_by->pIndexData->pShop != NULL)
                obj->carried_by->gold += obj->cost/5;
            else
                act( message, obj->carried_by, obj, NULL, TO_CHAR );
        }
        else if ( obj->in_room != NULL
        &&      ( rch = obj->in_room->people ) != NULL )
        {
            if (! (obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT
                   && !CAN_WEAR(obj->in_obj,ITEM_TAKE)))
            {
                act( message, rch, obj, NULL, TO_ROOM );
                act( message, rch, obj, NULL, TO_CHAR );
            }
        }

        if (obj->item_type == ITEM_CORPSE_PC && obj->contains)
        {   /* save the contents */
            OBJ_DATA *t_obj, *next_obj;

            for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
            {
                next_obj = t_obj->next_content;
                obj_from_obj(t_obj);

                if (obj->in_obj) /* in another object */
                    obj_to_obj(t_obj,obj->in_obj);

                if (obj->carried_by)  /* carried */
                    obj_to_char(t_obj,obj->carried_by);

                if (obj->in_room == NULL)  /* destroy it */
                    extract_obj(t_obj);

                else /* to a room */
                    obj_to_room(t_obj,obj->in_room);
            }
        }

        extract_obj( obj );
    }

    return;
}



/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update( void )
{
    CHAR_DATA *wch;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;

    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
		if ( ( wch->in_room == NULL ) 
		|| ( wch->in_room->area == NULL ) ) {
			continue;
		}


        if ( IS_NPC(wch)
        ||   wch->level >= LEVEL_IMMORTAL
        ||   wch->in_room == NULL 
        ||   wch->in_room->area->empty)
            continue;

        for ( ch = wch->in_room->people; ch != NULL; ch = ch_next )
        {
            int count;

            ch_next     = ch->next_in_room;

            if ( !IS_NPC(ch)
            ||   !IS_SET(ch->act, ACT_AGGRESSIVE)
            ||   IS_SET(ch->in_room->room_flags,ROOM_SAFE)
            ||   IS_AFFECTED(ch,AFF_CALM)
            ||   ch->fighting != NULL
            ||   IS_AFFECTED(ch, AFF_CHARM)
            ||   !IS_AWAKE(ch)
            ||   ( IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch) )
            ||   !can_see( ch, wch ) 
            ||   number_bits(1) == 0)
                continue;

            /*
             * Ok we have a 'wch' player character and a 'ch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             */
            count       = 0;
            victim      = NULL;
            for ( vch = wch->in_room->people; vch != NULL; vch = vch_next )
            {
                vch_next = vch->next_in_room;

                if ( !IS_NPC(vch)
                &&   vch->level < LEVEL_IMMORTAL
                &&   ch->level >= vch->level - 5 
                &&   ( !IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch) )
                &&   can_see( ch, vch ) )
                {
                    if ( number_range( 0, count ) == 0 )
                        victim = vch;
                    count++;
                }
            }

            if ( victim == NULL )
                continue;

            multi_hit( ch, victim, TYPE_UNDEFINED );
        }
    }

    return;
}



/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler( void )
{
   static  int     pulse_area;
   static  int     pulse_mobile;
   static  int     pulse_violence;
   static  int     pulse_point;
   static  int     pulse_music;   
   static  int     pulse_bonus;

   if ( --pulse_area     <= 0 )
     {
        pulse_area      = number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 );
        area_update     ( );
        /* this line added by Rahl for autoquesting */  
        quest_update    ( );
     }
   
    if ( --pulse_music    <= 0 )
    {
        pulse_music     = PULSE_MUSIC;
        song_update();
    }

   if ( --pulse_mobile   <= 0 )
     {
        pulse_mobile    = PULSE_MOBILE;
        mobile_update   ( );
     }
   
   if ( --pulse_violence <= 0 )
     {
        pulse_violence  = PULSE_VIOLENCE;
        violence_update ( );
     }

   if ( --pulse_bonus <= 0 )
	{
		pulse_bonus = PULSE_BONUS;
		bonus_update();
	}
   
   if ( --pulse_point    <= 0 )
     {
        BUFFER *buf = buffer_new( 100 );
        
        bprintf(buf,"TICK!"); /* \r removed by Rahl */
        /* added by Rahl */
        wiznet( buf->data, NULL, NULL, WIZ_TICKS, 0, 0 );

        /* removed by Rahl.. what a huge waste of space */
/*      log_string(buf->data); */
        pulse_point     = PULSE_TICK;
        
        /* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */

        buffer_free( buf );       

        weather_update  ( );
        char_update     ( );
        obj_update      ( );
        room_update     ( );
     }
   else if ( pulse_point == PULSE_TICK/2)
     {
        regen_update    ( );
        room_update     ( );
        spell_update();
     }

    /* added by Rahl */
    auction_update( );

    aggr_update( );
    tail_chain( );
    return;
}

/* added by Rahl */
/* the auction update - another very important part */
void auction_update( void )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( auction->item != NULL )
    {
        if ( --auction->pulse <= 0 ) /* decrease pulse */
        {
            auction->pulse = PULSE_AUCTION;
            switch ( ++auction->going ) /* increase the going state */
            {
                case 1: /* going once */
                case 2: /* going twice */
                    if ( auction->bet > 0 )
                    {
                        bprintf( buf, "%s`m: going %s for %ld.`w",
                            auction->item->short_descr, ( ( auction->going
                            == 1 ) ? "once" : "twice" ), auction->bet );
                    }
                    else
                    {
                        bprintf( buf, "%s`m: going %s (no bid received yet).`w",
                            auction->item->short_descr, ( ( auction->going
                            == 1 ) ? "once" : "twice" ) );
                    }
                    talk_auction( buf->data );
                    break;

                case 3: /* sold */
                    if ( auction->bet > 0 )
                    {
                        bprintf( buf, "%s `msold to %s for %ld.`w", 
                            auction->item->short_descr, 
                            IS_NPC( auction->buyer ) ?
                            auction->buyer->short_descr : auction->buyer->name,
                            auction->bet );
                        talk_auction( buf->data );
                        obj_to_char( auction->item, auction->buyer );
                        act( "The auctioneer appears before you in a puff of smoke and hands you $p.",
                            auction->buyer, auction->item, NULL, TO_CHAR );
                        act( "The auctioneer appears before $n and hands $m $p.",
                            auction->buyer, auction->item, NULL, TO_ROOM );
                        /* give him the money */
                        auction->seller->gold += auction->bet; 
                        auction->item = NULL; /* reset item */
                    }
                    else /* not sold */
                    {
                        bprintf( buf, "`mNo bids received for %s `m- "
                            "object has been removed.`w",
                            auction->item->short_descr );
                        talk_auction( buf->data );
                        /* added by Rahl */
                        extract_obj( auction->item );
                        /* removed here */
/*                      act( "The auctioneer appears before you to return $p to you.",
                            auction->seller, auction->item, NULL, TO_CHAR );
                        act( "The auctioneer appears before $n to return $p to $m.",
                            auction->seller, auction->item, NULL, TO_ROOM );
                        obj_to_char( auction->item, auction->seller );
*/
                        auction->item = NULL; /* clear auction */
                    }
            } /* switch */
        } /* if */
    } /* if */
    buffer_free( buf );
}


/****************************************************************************
 * spell_update() by Rahl.
 * I'm trying to make something similar to the T.E.S.S. on ROM
 * Probably a better way than using 'switch', but it's easy
 * It should be noted that mobs with both a spec fucntion and 
 * set ACT_CLERIC or ACT_MAGE would probably be overkill. Perhaps
 * I should make a check for a spec fucntion and if they have one, to
 * just find another mob? Prolly too much of a pain, though cuz not all
 * the specs are for combat purposes. Be easier just to note the builders
 * to be careful
 **************************************************************************/
void spell_update( void )
{
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    int chance = 0;
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *d_next;
    int found = 0;

    /*
     * go through the char list, one at a time. This includes
     * both PCs and NPCs
     */
    for ( wch = char_list; wch != NULL; wch = wch_next )
    {
        /* go to the next node (char) in the linked list */
        wch_next = wch->next;

        /* we only want NPCs to cast stuff.. */
        if ( !IS_NPC( wch ) )
            continue;

		/* Why spell up if they can't be killed? */
		if ( IS_SET( wch->act, ACT_NO_KILL ) )
			continue;

        /* make sure they're in a room/area */
        if ( wch->in_room == NULL || wch->in_room->area == NULL )
            continue;

        /*
         * Go through the list of PCs, one at a time
         */
        for ( d = descriptor_list; d != NULL; d = d_next )
        {
            /* go to the next PC */
            d_next = d->next;

            /* only check playing chars */
            /* should I check for those writing notes? */
            if ( d->connected != CON_PLAYING )
                continue;

            /* just to make sure.. better safe than sorry.. */
            if ( d->character->in_room == NULL 
            || d->character->in_room->area == NULL )
                continue;

            /* don't give away that an imm's in the area.. invis or not */
            if ( IS_IMMORTAL( d->character ) )
                continue;

            /* anyone in the same area? */
            if ( d->character->in_room->area == wch->in_room->area )
            {
                /* yup */
                found = TRUE;
                break;
            }
            else
                found = FALSE;
        }

        /* no one in the area, so find another char/mob to check */
        if ( !found )
            continue;

        /* 33 posibilities at the moment.. might need more.. */
        chance = number_percent( ) / 3;

        /* clerics */
        if ( IS_SET( wch->act, ACT_CLERIC ) )
        {
            /*
             * if they aren't fighting, they can only spend half their
             * mana spelling up. The rest will be used in combat, healing
             * etc.
             */
            if ( ( wch->fighting == NULL )
            && ( wch->mana > ( wch->max_mana / 2 ) ) )
            {
                /*
                 * This is kind of random, but that's kind of what we
                 * want. Basically, we go through the 33 possibilities
                 * that we have available. Some spells have more cases,
                 * and therefore a better chance of being cast.
                 * There's also the default, were they cast nothing.
                 * Not sure what spells I want here or how often, but
                 * this isn't bad for a first try...
                 */
                switch ( chance )
                {
                    case 1:
                    case 2:
                        if ( !is_affected( wch, skill_lookup( "shield" ) ) )
                            do_cast( wch, "shield" );
                        break;
                    case 3:
                        if ( !IS_AFFECTED( wch, AFF_SANCTUARY ) )
                            do_cast( wch, "sanctuary" );
                        break;
                    case 4:
                    case 5:
                    case 6:
                        if ( !is_affected( wch, skill_lookup( "armor" ) ) )
                            do_cast( wch, "armor" );
                        break;
                    case 7:
                    case 8:
                    case 9:
                        if ( !is_affected( wch, skill_lookup( "bless" ) ) )
                            do_cast( wch, "bless" );
                        break;
                    case 10:
                    case 11:
                        if ( !IS_AFFECTED( wch, AFF_PROTECT_GOOD ) )
                            do_cast( wch, "'protection good'" );
                        break;
                    case 12:
                    case 13:
                        if ( !IS_AFFECTED( wch, AFF_FLYING ) )
                            do_cast( wch, "fly" );
                        break;
                    case 14:
                    case 15:
                        if ( !IS_AFFECTED( wch, AFF_PROTECT_EVIL ) )
                            do_cast( wch, "'protection evil'" );
                        break;
                    case 16:
                    case 17:
                        if ( !is_affected( wch, skill_lookup( "frenzy" ) ) )
                            do_cast( wch, "frenzy" );
                        break;
                    case 19:
                        if ( !IS_AFFECTED( wch, AFF_REGENERATION ) )
                            do_cast( wch, "regeneration" );
                        break;
                    case 20:
                    case 21:
                    case 24:
                        if ( IS_AFFECTED( wch, AFF_CURSE ) )
                            do_cast( wch, "'remove curse'" );
                        else if ( IS_AFFECTED( wch, AFF_BLIND ) )
                            do_cast( wch, "'cure blindness'" );
                        else if ( IS_AFFECTED( wch, AFF_PLAGUE ) )
                            do_cast( wch, "'cure disease'" );
                        else if ( IS_AFFECTED( wch, AFF_POISON ) )
                            do_cast( wch, "'cure poison'" );
                        else
                        {
                            if ( wch->level < 10 )
                                do_cast( wch, "'cure light'" );
                            else if ( wch->level < 20 )
                                do_cast( wch, "'cure serious'" );
                            else if ( wch->level < 30 )
                                do_cast( wch, "'cure critical'" );
                            else if ( wch->level < 55 )
                                do_cast( wch, "heal" );
                            else if ( wch->level < 65 )
                                do_cast( wch, "restoration" );
                            else 
                                do_cast( wch, "greater heal" );
                        }
                        break;
                    case 22:
                    case 23:
                        if ( !is_affected( wch, 
                          skill_lookup( "stone skin" ) ) )
                            do_cast( wch, "'stone skin'" );
                        break;
                    default:
                        break;
                } /* switch */
            } /* if */
            else
            {
                /* 
                 * This is the combat section. Not sure how much will
                 * be offensive spells and how much will be healing..
                 */
                if ( wch->fighting == NULL )
                    continue;

                switch( chance )
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 30:
                    case 31:
                    case 32:
                        if ( wch->level < 10 )
                            do_cast( wch, "'cure light'" );
                        else if ( wch->level < 20 )
                            do_cast( wch, "'cure serious'" );
                        else if ( wch->level < 30 )
                            do_cast( wch, "'cure critical'" );
                        else if ( wch->level < 55 )
                            do_cast( wch, "heal" );
                        else if ( wch->level < 65 )
                            do_cast( wch, "restoration" );
                        else 
                            do_cast( wch, "greater heal" );
                        break;
                    case 6:
                    case 7:     
                        do_cast( wch, "blindness" );
                        break;
                   case 8:
                   case 9:
                        do_cast( wch, "slow" );
                        break;
                   case 10:
                   case 11:
                        do_cast( wch, "weaken" );
                        break;
                   case 12:
                   case 13:
                        do_cast( wch, "'energy drain'" );
                        break;
                   case 14:
                        do_cast( wch, "'change sex'" );
                        break;
                   case 15:
                   case 16:
                        do_cast( wch, "curse" );
                        break;
                   case 17:
                   case 18:
                        do_cast( wch, "plague" );
                        break;
                   case 19:
                        do_cast( wch, "posion" );
                        break;
                   case 20:
                        do_cast( wch, "'heat metal'" );
                        break;
                   case 21:
                        if ( !IS_EVIL( wch ) )
                            do_cast( wch, "'dispel evil'" );
                        else
                            do_cast( wch, "'dispel good'" );
                        break;
                   case 22:
                        do_cast( wch, "earthquake" );
                        break;
                   case 23:
                        do_cast( wch, "flamestrike" );
                        break;
                   case 24:
                        do_cast( wch, "thunderbolt" );
                        break;
                   case 25:
                        if ( !IS_EVIL( wch ) )
                            do_cast( wch, "demonfire" );
                        break;
                   case 26:
                   case 27:
                        if ( wch->fighting->level < 10 )
                            do_cast( wch, "'cause light'" );
                        else if ( wch->fighting->level < 15 )
                            do_cast( wch, "'cause serious'" );
                        else if ( wch->fighting->level < 20 )
                            do_cast( wch, "'cause critical'" );
                        else if ( wch->fighting->level < 55 )
                            do_cast( wch, "harm" );
                        else
                            do_cast( wch, "'greater harm'" );
                        break;
                   case 28:
                   case 29:
                        do_cast( wch, "'dispel magic'" );
                        break;
                   default: 
                        break; 
                } /* switch */
            } /* else */
        } /* if */

        if ( IS_SET( wch->act, ACT_MAGE ) )
        {
            if ( wch->fighting == NULL && 
            ( wch->mana > ( wch->max_mana / 2 ) ) )
            {
                switch( chance )
                {
                    /*
                     * This is the mage non-combat stuff 
                     */
                    case 1:
                    case 2:
                    case 18:
                        if ( !is_affected( wch, skill_lookup( "shield" ) ) )
                            do_cast( wch, "shield" );
                        break;
                    case 3:
                        if ( !IS_AFFECTED( wch, AFF_SANCTUARY ) )
                            do_cast( wch, "sanctuary" );
                        break;
                    case 4:
                    case 5:
                    case 6:
                        if ( !is_affected( wch, skill_lookup( "armor" ) ) )
                            do_cast( wch, "armor" );
                        break;
                    case 7:
                    case 8:
                    case 9:
                        if ( !is_affected( wch, 
                          skill_lookup( "giant strength" ) ) )
                            do_cast( wch, "'giant strength" );
                        break;
                    case 10:
                    case 11:
                        if ( !IS_AFFECTED( wch, AFF_PROTECT_GOOD ) )
                            do_cast( wch, "'protection good'" );
                        break;
                    case 12:
                    case 13:
                        if ( !IS_AFFECTED( wch, AFF_FLYING ) )
                            do_cast( wch, "fly" );
                        break;
                    case 14:
                    case 15:
                        if ( !IS_AFFECTED( wch, AFF_PROTECT_EVIL ) )
                            do_cast( wch, "'protection evil'" );
                        break;
                    case 16:
                    case 17:
                        if ( !IS_AFFECTED( wch, AFF_HASTE ) )
                            do_cast( wch, "haste" );
                        break;
                    case 22:
                    case 23:
                        if ( !is_affected( wch, 
                          skill_lookup( "stone skin" ) ) )
                            do_cast( wch, "'stone skin'" );
                        break;
                    default:
                        break;
                } /* switch */
            } /* if */
            else
            {
                /* 
                 * This is the mage combat stuff
                 */
                if ( wch->fighting == NULL )
                    continue;

                switch( chance )
                {
                    case 1:
                    case 2:
                    case 3:
                        do_cast( wch, "fear" );
                        break;
                    case 4:
                    case 5:
                        do_cast( wch, "'burning hands'" );
                        break;
                    case 6:
                    case 7:     
                        do_cast( wch, "blindness" );
                        break;
                   case 8:
                   case 9:
                        do_cast( wch, "slow" );
                        break;
                   case 10:
                   case 11:
                        do_cast( wch, "weaken" );
                        break;
                   case 12:
                   case 13:
                        do_cast( wch, "'energy drain'" );
                        break;
                   case 14:
                        do_cast( wch, "'change sex'" );
                        break;
                   case 15:
                   case 16:
                        do_cast( wch, "curse" );
                        break;
                   case 17:
                   case 18:
                        do_cast( wch, "plague" );
                        break;
                   case 19:
                        do_cast( wch, "posion" );
                        break;
                   case 20:
                        do_cast( wch, "'heat metal'" );
                        break;
                   case 21:
                        do_cast( wch, "fireball" );
                        break;
                   case 22:
                        do_cast( wch, "'lightning bolt'" );
                        break;
                   case 23:
                        do_cast( wch, "'acid blast'" );
                        break;
                   case 24:
                        do_cast( wch, "'chill touch'" );
                        break;
                   case 25:
                        do_cast( wch, "'colour spray'" );
                        break;
                   default:
                        break;
                } /* switch */
            } /* else */
        } /* if */
    } /* for */

    /* ta da! */
    return;
}
