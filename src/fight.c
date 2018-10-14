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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "interp.h"

#define MAX_DAMAGE_MESSAGE 32

/*
 * Local functions.
 */
void    check_assist    ( CHAR_DATA *ch, CHAR_DATA *victim );
bool    check_dodge     ( CHAR_DATA *ch, CHAR_DATA *victim );
void    check_killer    ( CHAR_DATA *ch, CHAR_DATA *victim );
bool    check_parry     ( CHAR_DATA *ch, CHAR_DATA *victim );
/* check_block added by Rahl - fix by Rindar (Ron Cole ) */
bool    check_block     ( CHAR_DATA *ch, CHAR_DATA *victim );
/* blink by Rahl */
bool    check_blink     ( CHAR_DATA *ch, CHAR_DATA *victim );
void    dam_message     ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
                            int dt, bool immune );
void    death_cry       ( CHAR_DATA *ch );
void    group_gain      ( CHAR_DATA *ch, CHAR_DATA *victim );
int     xp_compute      ( CHAR_DATA *gch, CHAR_DATA *victim, 
                            int total_levels, int members );
bool    is_safe         ( CHAR_DATA *ch, CHAR_DATA *victim );
void    make_corpse     ( CHAR_DATA *ch );
void    one_hit         ( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
void    second_one_hit  ( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
void    mob_hit         ( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
void    chaos_kill      ( CHAR_DATA *victim );
void    raw_kill        ( CHAR_DATA *victim );
void    set_fighting    ( CHAR_DATA *ch, CHAR_DATA *victim );
void    disarm          ( CHAR_DATA *ch, CHAR_DATA *victim );
void    chaos_log       ( CHAR_DATA *ch, char *argument );
/* added by Rahl */
void    pk_kill 		( CHAR_DATA *victim );


/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update( void ) {
    CHAR_DATA *ch;
    CHAR_DATA *victim;

    for ( ch = char_list; ch != NULL; ch = ch->next ) {
        CHAR_DATA *ch_next;
        ch_next = ch->next;

        if ( ( victim = ch->fighting ) == NULL || ch->in_room == NULL ) {
            continue;
		}

        if ( IS_AWAKE(ch) && ch->in_room == victim->in_room ) {
            multi_hit( ch, victim, TYPE_UNDEFINED );
        } else {
            if (!IS_NPC(victim)) victim->pcdata->nemesis=victim->fighting;      
            stop_fighting( ch, FALSE );
        }

        if ( ( victim = ch->fighting ) == NULL ) {
            continue;
        }

        /*
         * Fun for the whole family!
         */
        check_assist(ch,victim);
    }

    return;
}

/* for auto assisting */
void check_assist(CHAR_DATA *ch,CHAR_DATA *victim)
{
    CHAR_DATA *rch, *rch_next;

    for (rch = ch->in_room->people; rch != NULL; rch = rch_next)
    {
        rch_next = rch->next_in_room;
        
        if (IS_AWAKE(rch) && rch->fighting == NULL)
        {

            /* quick check for ASSIST_PLAYER */
            if (!IS_NPC(ch) && IS_NPC(rch) 
            && IS_SET(rch->off_flags,ASSIST_PLAYERS)
            &&  rch->level + 6 > victim->level
          /*  && !IS_SET(ch->act, PLR_KILLER) */ )
            {
                do_emote(rch,"screams and attacks!");
                multi_hit(rch,victim,TYPE_UNDEFINED);
                continue;
            }

            /* PCs next */
            if ((!IS_NPC(ch) && IS_NPC(victim)) 
                || (IS_AFFECTED(ch,AFF_CHARM)) )
            {
                if ( ( (!IS_NPC(rch) && IS_SET(rch->act,PLR_AUTOASSIST))
                ||     IS_AFFECTED(rch,AFF_CHARM)) 
                &&   is_same_group(ch,rch) )
                    multi_hit (rch,victim,TYPE_UNDEFINED);
                
                continue;
            }
        
            /* now check the NPC cases */
            
            if (IS_NPC(ch) && !IS_AFFECTED(ch,AFF_CHARM))
        
            {
                if ( (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALL))

                ||   (IS_NPC(rch) && rch->race == ch->race 
                   && IS_SET(rch->off_flags,ASSIST_RACE))

                ||   (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALIGN)
                   &&   ((IS_GOOD(rch)    && IS_GOOD(ch))
                     ||  (IS_EVIL(rch)    && IS_EVIL(ch))
                     ||  (IS_NEUTRAL(rch) && IS_NEUTRAL(ch)))) 

                ||   (rch->pIndexData == ch->pIndexData 
                   && IS_SET(rch->off_flags,ASSIST_VNUM)))

                {
                    CHAR_DATA *vch;
                    CHAR_DATA *target;
                    int number;

                    if (number_bits(1) == 0)
                        continue;
                
                    target = NULL;
                    number = 0;
                    for (vch = ch->in_room->people; vch; vch = vch->next)
                    {
                        if (can_see(rch,vch)
                        &&  is_same_group(vch,victim)
                        &&  number_range(0,number) == 0)
                        {
                            target = vch;
                            number++;
                        }
                    }

                    if (target != NULL)
                    {
                        do_emote(rch,"screams and attacks!");
                        multi_hit(rch,target,TYPE_UNDEFINED);
                    }
                }       
            }
        }
    }
}


/*
 * Do one group of attacks.
 */
void multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *second_wield;
    int     chance;

    /* decrement the wait */
    if (ch->desc == NULL)
        ch->wait = UMAX(0,ch->wait - PULSE_VIOLENCE);


    /* no attacks for stunnies -- just a check */
    if (ch->position < POS_RESTING)
        return;

    if (IS_NPC(ch))
    {
        mob_hit(ch,victim,dt);
        return;
    }

    one_hit( ch, victim, dt );
    if ( (second_wield = get_eq_char( ch, WEAR_SECOND_WIELD ) ) != NULL )
        second_one_hit( ch, victim, dt);

    if (ch->fighting != victim)
        return;

    if (IS_AFFECTED(ch,AFF_HASTE))
        one_hit(ch,victim,dt);

    if ( ch->fighting != victim || dt == gsn_backstab )
        return;

    /*
     * New stuff by Rahl for circle
     */
    if ( ch->fighting != victim || dt == gsn_circle )
        return;

    chance = get_skill(ch,gsn_second_attack);

    /* 
     * new stuff by Rahl for haste
     */
    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        chance /= 2;

    if ( number_percent( ) < chance )
    {
        one_hit( ch, victim, dt );
        check_improve(ch,gsn_second_attack,TRUE,5);
        if ( ch->fighting != victim )
            return;
    }

    chance = get_skill(ch,gsn_third_attack)/2;

    /*
     * new stuff by Rahl for slow
     */
    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        chance = 0;

    if ( number_percent( ) < chance )
    {
        one_hit( ch, victim, dt );
        check_improve(ch,gsn_third_attack,TRUE,6);
        if ( ch->fighting != victim )
            return;
    }

    return;
}

/* procedure for all mobile attacks */
void mob_hit (CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    int chance,number;
    CHAR_DATA *vch, *vch_next;

    one_hit(ch,victim,dt);

    if (ch->fighting != victim) {
        return;
    }

    /* Area attack -- BALLS nasty! */
 
    if (IS_SET(ch->off_flags,OFF_AREA_ATTACK)) {
        for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
            vch_next = vch->next;
            if ((vch != victim && vch->fighting == ch)) {
                one_hit(ch,vch,dt);
            }
        }
    }

    if (IS_AFFECTED(ch,AFF_HASTE) || ( IS_SET(ch->off_flags,OFF_FAST)
        && !IS_AFFECTED( ch, AFF_SLOW ) ) ) {
        one_hit(ch,victim,dt);
    }

    if (ch->fighting != victim || dt == gsn_backstab) {
        return;
    }

    /* new stuff by Rahl for circle */
    if ( ch->fighting != victim || dt == gsn_circle ) {
        return;
    }

    chance = get_skill(ch,gsn_second_attack);

    /* new stuff by Rahl for slow */
    if ( IS_AFFECTED( ch, AFF_SLOW ) ) {
        chance /= 2;
    }

    if (number_percent() < chance) {
        one_hit(ch,victim,dt);
        if (ch->fighting != victim) {
            return;
        }
    }

    chance = get_skill(ch,gsn_third_attack)/2;

    /* new stuff by Rahl for slow */
    if ( IS_AFFECTED( ch, AFF_SLOW ) ) {
        chance = 0;
    }

    if (number_percent() < chance) {
        one_hit(ch,victim,dt);
        if (ch->fighting != victim) {
            return;
        }
    } 

    /* oh boy!  Fun stuff! */

    if (ch->wait > 0) {
        return;
    }

    number = number_range(0,2);

    /*if (number == 1 && IS_SET(ch->act,ACT_MAGE))
        { mob_cast_mage(ch,victim); return; };*/

    /*if (number == 2 && IS_SET(ch->act,ACT_CLERIC))
        { mob_cast_cleric(ch,victim); return; };*/

    /* now for the skills */

    number = number_range(0,9);

    switch(number) {
        case (0) :
            if (IS_SET(ch->off_flags,OFF_BASH)) {
                do_bash(ch,"");
            }
            break;

        case (1) :
            if (IS_SET(ch->off_flags,OFF_BERSERK) && !IS_AFFECTED(ch,AFF_BERSERK)) {
                do_berserk(ch,"");
            }
            break;

        case (2) :
            if (IS_SET(ch->off_flags,OFF_DISARM) 
            || (get_weapon_sn(ch) != gsn_hand_to_hand 
            && (IS_SET(ch->act,ACT_WARRIOR)
            ||  IS_SET(ch->act,ACT_THIEF)))) {
                do_disarm(ch,"");
            }
            break;

        case (3) :
            if (IS_SET(ch->off_flags,OFF_KICK)) {
                do_kick(ch,"");
            }
            break;

        case (4) :
            if (IS_SET(ch->off_flags,OFF_KICK_DIRT)) {
                do_dirt(ch,"");
            }
            break;

        /*case (5) :
            if (IS_SET(ch->off_flags,OFF_TAIL))
                do_tail(ch,"");
            break; */

        case (6) :
            if (IS_SET(ch->off_flags,OFF_TRIP)) {
                do_trip(ch,"");
            }
            break;

        /*
        case (7) :
            if (IS_SET(ch->off_flags,OFF_CRUSH))
                do_crush(ch,"");
            break;
        */

        /*
         * both below cases by Rahl, but because of 1) postion and 2) pk
         * rules, neither works, but they're here if I feel like playing
         * with them later. I should remove the pk check on circle
         * cuz you need to be fighting to do it anyways, so that 
         * check should have already been made.
         */
        /*
        case (8) :
            if ( IS_SET( ch->off_flags, OFF_WHIRL ) )
                do_whirlwind( ch, "" );
            break;
        case (9) :
            if ( IS_SET( ch->off_flags, OFF_CIRCLE ) )
                do_circle( ch, "" );
            break; 
        */
        default:
            break;
    }
}
        

/*
 * Hit one guy once.
 */
void one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *wield;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int dam;
    int diceroll;
    int sn,skill;
    int dam_type;
/* added by Rahl */
    bool result;

    sn = -1;


    /* just in case */
    if (victim == ch || ch == NULL || victim == NULL)
        return;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
        return;

    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_WIELD );

    if ( dt == TYPE_UNDEFINED )
    {
        dt = TYPE_HIT;
        if ( wield != NULL && wield->item_type == ITEM_WEAPON )
            dt += wield->value[3];
        else 
            dt += ch->dam_type;
    }

    if (dt < TYPE_HIT)
        if (wield != NULL)
            dam_type = attack_table[wield->value[3]].damage;
        else
            dam_type = attack_table[ch->dam_type].damage;
    else
        dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
        dam_type = DAM_BASH;

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn);

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
        thac0_00 = 20;
        thac0_32 = -4;   /* as good as a thief */ 
        if (IS_SET(ch->act,ACT_WARRIOR))
            thac0_32 = -10;
        else if (IS_SET(ch->act,ACT_THIEF))
            thac0_32 = -4;
        else if (IS_SET(ch->act,ACT_CLERIC))
            thac0_32 = 2;
        else if (IS_SET(ch->act,ACT_MAGE))
            thac0_32 = 6;
    }
    else
    {
        thac0_00 = class_table[ch->ch_class].thac0_00;
        thac0_32 = class_table[ch->ch_class].thac0_32;
    }

    thac0  = interpolate( ch->level, thac0_00, thac0_32 );

    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;

    if (dt == gsn_backstab)
        thac0 -= 10 * (100 - get_skill(ch,gsn_backstab));

    /* new stuff by Rahl for circle */
    if ( dt == gsn_circle )
        thac0 -= 10 * ( 100 - get_skill( ch, gsn_circle ) );

    switch(dam_type)
    {
        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
    }; 
        
    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;
     
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;

    if ( victim->position < POS_FIGHTING)
        victim_ac += 4;
 
    if (victim->position < POS_RESTING)
        victim_ac += 6;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
        ;

    if ( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    {
        /* Miss. */
        damage( ch, victim, 0, dt, dam_type, TRUE );
        tail_chain( );
        return;
    }

    /*
     * Hit.
     * Calc damage.
     */
    if ( IS_NPC(ch) &&  wield == NULL )
        dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
        
    else
    {
        if (sn != -1)
            check_improve(ch,sn,TRUE,5);
        if ( wield != NULL )
        {
            dam = dice(wield->value[1],wield->value[2]) * skill/100;

            if (get_eq_char(ch,WEAR_SHIELD) == NULL)  /* no shield = more */
                dam = dam * 21/20;
        }
        else
            dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
    }

    /*
     * Bonuses.
     */
    if ( get_skill(ch,gsn_enhanced_damage) > 0 )
    {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_enhanced_damage))
        {
            check_improve(ch,gsn_enhanced_damage,TRUE,6);
            dam += dam * diceroll/100;
        }
    }

    if ( !IS_AWAKE(victim) )
        dam *= 2;
     else if (victim->position < POS_FIGHTING)
        dam = dam * 3 / 2;

    if ( dt == gsn_backstab && wield != NULL) {
        if ( wield->value[0] != 2 ) {
            dam *= 2 + ch->level / 10; 
        } else { 
            dam *= 2 + ch->level / 8;
		}
	}

    /* new stuff by Rahl for circle */
    /* This is where the damage is.. half of backstab right now */
    if ( dt == gsn_circle && wield != NULL ) {
        if ( wield->value[0] != 2 ) {
            dam *= 2 + ch->level / 20;
        } else {
            dam *= 2 + ch->level / 16;
		}
	}

    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    if ( dam <= 0 )
        dam = 1;


/* changed by Rahl */ 
/*    damage( ch, victim, dam, dt, dam_type ); */

/* added by Rahl */
    result = damage( ch, victim, dam, dt, dam_type, TRUE );
    
    /* but do we have a funky weapon? */
    if (result && wield != NULL)
    { 
        int dam;

        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_POISON))
        {
            int level;
            AFFECT_DATA *poison, af;

            if ((poison = affect_find(wield->affected,gsn_poison)) == NULL)
                level = wield->level;
            else
                level = poison->level;
        
            if (!saves_spell(level / 2,victim, DAM_POISON)) 
            {
                send_to_char("You feel poison coursing through your veins.",
                    victim);
                act("$n is poisoned by the venom on $p.",
                    victim,wield,NULL,TO_ROOM);

                af.where     = TO_AFFECTS;
                af.type      = gsn_poison;
                af.level     = level * 3/4;
                af.duration  = level / 2;
                af.location  = APPLY_STR;
                af.modifier  = -1;
                af.bitvector = AFF_POISON;
                affect_join( victim, &af );
            }

            /* weaken the poison if it's temporary */
            if (poison != NULL)
            {
                poison->level = UMAX(0,poison->level - 2);
                poison->duration = UMAX(0,poison->duration - 1);
        
                if (poison->level == 0 || poison->duration == 0)
                    act("The poison on $p has worn off.",ch,wield,NULL,TO_CHAR);
            }
        }


        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_VAMPIRIC))
        {
            dam = number_range(1, wield->level / 5 + 1);
            act("$p draws life from $n.",victim,wield,NULL,TO_ROOM);
            act("You feel $p drawing your life away.",
                victim,wield,NULL,TO_CHAR);
            damage(ch,victim,dam,0,DAM_NEGATIVE,FALSE);
            ch->alignment = UMAX(-1000,ch->alignment - 1);
            ch->hit += dam/2;
        }

        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FLAMING))
        {
            dam = number_range(1,wield->level / 4 + 1);
            act("$n is burned by $p.",victim,wield,NULL,TO_ROOM);
            act("$p sears your flesh.",victim,wield,NULL,TO_CHAR);
            fire_effect( (void *) victim,wield->level/2,dam,TARGET_CHAR);
            damage(ch,victim,dam,0,DAM_FIRE, FALSE);
        }

        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FROST))
        {
            dam = number_range(1,wield->level / 6 + 2);
            act("$p freezes $n.",victim,wield,NULL,TO_ROOM);
            act("The cold touch of $p surrounds you with ice.",
                victim,wield,NULL,TO_CHAR);
            cold_effect(victim,wield->level/2,dam,TARGET_CHAR);
            damage(ch,victim,dam,0,DAM_COLD,FALSE);
        }

        /* sharp/vorpal case added (made up) by Rahl */
        if (ch->fighting == victim && (IS_WEAPON_STAT(wield, WEAPON_SHARP)
                || IS_WEAPON_STAT(wield, WEAPON_VORPAL)))
        {
            dam = number_range(1, wield->level / 5 + 3 );
            damage( ch, victim, dam, 0, DAM_SLASH, FALSE );
        }

        if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_SHOCKING))
        {
            dam = number_range(1,wield->level/5 + 2);
            act("$n is struck by lightning from $p.",victim,wield,NULL,TO_ROOM);
            act("You are shocked by $p.",victim,wield,NULL,TO_CHAR);
            shock_effect(victim,wield->level/2,dam,TARGET_CHAR);
            damage(ch,victim,dam,0,DAM_LIGHTNING,FALSE);
        }
    }
        

    tail_chain( );
    return;
}

void second_one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *wield;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int dam;
    int diceroll;
    int sn,skill;
    int dam_type;

    sn = -1;


    /* just in case */
    if (victim == ch || ch == NULL || victim == NULL)
        return;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
        return;

    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_SECOND_WIELD );

    if ( dt == TYPE_UNDEFINED )
    {
        dt = TYPE_HIT;
        if ( wield != NULL && wield->item_type == ITEM_WEAPON )
            dt += wield->value[3];
        else 
            dt += ch->dam_type;
    }

    if (dt < TYPE_HIT)
        if (wield != NULL)
            dam_type = attack_table[wield->value[3]].damage;
        else
            dam_type = attack_table[ch->dam_type].damage;
    else
        dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
        dam_type = DAM_BASH;

    /* get the weapon skill */
    sn = get_second_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn);

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
        thac0_00 = 20;
        thac0_32 = -4;   /* as good as a thief */ 
        if (IS_SET(ch->act,ACT_WARRIOR))
            thac0_32 = -10;
        else if (IS_SET(ch->act,ACT_THIEF))
            thac0_32 = -4;
        else if (IS_SET(ch->act,ACT_CLERIC))
            thac0_32 = 2;
        else if (IS_SET(ch->act,ACT_MAGE))
            thac0_32 = 6;
    }
    else
    {
        thac0_00 = class_table[ch->ch_class].thac0_00;
        thac0_32 = class_table[ch->ch_class].thac0_32;
    }

    thac0  = interpolate( ch->level, thac0_00, thac0_32 );

    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;

    if (dt == gsn_backstab)
        thac0 -= 10 * (100 - get_skill(ch,gsn_backstab));

    /* New stuff for circle */
    if ( dt == gsn_circle )
        thac0 -= 10 * ( 100 - get_skill( ch, gsn_circle ) );

    switch(dam_type)
    {
        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
    }; 
        
    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;
     
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;

    if ( victim->position < POS_FIGHTING)
        victim_ac += 4;
 
    if (victim->position < POS_RESTING)
        victim_ac += 6;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
        ;

    if ( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    {
        /* Miss. */
        damage( ch, victim, 0, dt, dam_type, TRUE );
        tail_chain( );
        return;
    }

    /*
     * Hit.
     * Calc damage.
     */
    if ( IS_NPC(ch) && wield == NULL)
        dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
        
    else
    {
        if (sn != -1)
            check_improve(ch,sn,TRUE,5);
        if ( wield != NULL )
        {
            dam = dice(wield->value[1],wield->value[2]) * skill/100;

            if (get_eq_char(ch,WEAR_SHIELD) == NULL)  /* no shield = more */
                dam = dam * 21/20;
        }
        else
            dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
    }

    /*
     * Bonuses.
     */
    if ( get_skill(ch,gsn_enhanced_damage) > 0 )
    {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_enhanced_damage))
        {
            check_improve(ch,gsn_enhanced_damage,TRUE,6);
            dam += dam * diceroll/100;
        }
    }

    if ( !IS_AWAKE(victim) )
        dam *= 2;
     else if (victim->position < POS_FIGHTING)
        dam = dam * 3 / 2;

    if ( dt == gsn_backstab && wield != NULL) {
        if ( wield->value[0] != 2 ) {
            dam *= 2 + ch->level / 10; 
        } else {
            dam *= 2 + ch->level / 8;
		}
	}

    /* new stuff by Rahl for circle */
    if ( dt == gsn_circle && wield != NULL ) {
        if ( wield->value[0] != 2 ) {
            dam *= 2 + ch->level / 20;
        } else {
            dam *= 2 + ch->level / 16;
		}
	}

    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    if ( dam <= 0 )
        dam = 1;

    damage( ch, victim, dam, dt, dam_type, TRUE );
    tail_chain( );
    return;
}

bool damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type, bool show ) 
{
    char buf[MAX_INPUT_LENGTH];
    OBJ_DATA *corpse;
    bool immune;
    int chaos_points = 0;

    if ( victim->position == POS_DEAD )
        return FALSE;

    /*
     * Stop up any residual loopholes.
     */
    if ( dam > 2000 )
    {
        bug( "Damage: %d: more than 2000 points!", dam );
        sprintf( buf, "[****] Player %s doing excessive damage.", ch->name );
        log_string( buf );
        dam = 2000;
        if (!IS_IMMORTAL(ch))
        {
            OBJ_DATA *obj;
            obj = get_eq_char( ch, WEAR_WIELD );
            send_to_char("You really shouldn't cheat.\n\r",ch);
            if ( obj != NULL )
                extract_obj(obj);
            obj = get_eq_char( ch, WEAR_SECOND_WIELD );
            if ( obj != NULL )
                extract_obj( obj );
        }

    }

    
    /* damage reduction */
/*
    if ( dam > 30)
        dam = (dam - 30)/2 + 30;
    if ( dam > 75)
        dam = (dam - 75)/2 + 75;
*/
    if ( !IS_IMMORTAL( ch ) )
    {
        if ( dam > 100 )
            dam = ( dam - 100 ) / 2 + 100;

        if ( dam > 250 )
            dam = ( dam - 250 ) / 2 + 250;
    }


   
    if ( victim != ch )
    {
        /*
         * Certain attacks are forbidden.
         * Most other attacks are returned.
         */
        if ( is_safe( ch, victim ) )
            return FALSE;
        check_killer( ch, victim );

        if ( victim->position > POS_STUNNED )
        {
            if ( victim->fighting == NULL )
                set_fighting( victim, ch );
            if (victim->timer <= 4)
                victim->position = POS_FIGHTING;
        }

        if ( victim->position > POS_STUNNED )
        {
            if ( ch->fighting == NULL )
                set_fighting( ch, victim );

            /*
             * If victim is charmed, ch might attack victim's master.
             */
            if ( IS_NPC(ch)
            &&   IS_NPC(victim)
            &&   IS_AFFECTED(victim, AFF_CHARM)
            &&   victim->master != NULL
            &&   victim->master->in_room == ch->in_room
            &&   number_bits( 3 ) == 0 )
            {
                if (!IS_NPC(victim)) victim->pcdata->nemesis=victim->fighting;  
                stop_fighting( ch, FALSE );
                multi_hit( ch, victim->master, TYPE_UNDEFINED );
                return FALSE;
            }
        }

        /*
         * More charm stuff.
         */
        if ( victim->master == ch )
            stop_follower( victim );
    }

    /*
     * Inviso attacks ... not.
     */
    if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
        affect_strip( ch, gsn_invis );
        affect_strip( ch, gsn_mass_invis );
        REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
        act( "`K$n `Kfades into existence.`w", ch, NULL, NULL, TO_ROOM );
    }

    /*
     * Damage modifiers.
     */
    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
        dam /= 2;

    if ( ( IS_AFFECTED( victim, AFF_PROTECT_EVIL ) && IS_EVIL( ch ) )
    || ( IS_AFFECTED( victim, AFF_PROTECT_GOOD ) && IS_GOOD ( ch ) ) )
        dam -= dam / 4;

    immune = FALSE;


    /*
     * Check for parry, and dodge.
     * and shield block - Rahl 
     */
    if ( dt >= TYPE_HIT && ch != victim)
    {
        if ( check_parry( ch, victim ) )
            return FALSE;
        if ( check_dodge( ch, victim ) )
            return FALSE;
        if ( check_block( ch, victim ) )
            return FALSE;
        if ( check_blink( ch, victim ) )
            return FALSE;
    }

    switch(check_immune(victim,dam_type))
    {
        case(IS_IMMUNE):
            immune = TRUE;
            dam = 0;
            break;
        case(IS_RESISTANT):     
            dam -= dam/3;
            break;
        case(IS_VULNERABLE):
            dam += dam/2;
            break;
    }

    if (show)
    dam_message( ch, victim, dam, dt, immune );

    if (dam == 0)
        return FALSE;

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= dam;
    if ( !IS_NPC(victim)
    &&   IS_IMMORTAL( victim) 
    &&   victim->hit < 1 )
        victim->hit = 1;
    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
        act( "`R$n `Ris mortally wounded, and will die soon, if not aided.`w",
            victim, NULL, NULL, TO_ROOM );
        send_to_char( 
            "`RYou are mortally wounded, and will die soon, if not aided.\n\r`w",
            victim );
        break;

    case POS_INCAP:
        act( "`R$n `Ris incapacitated and will slowly die, if not aided.`w",
            victim, NULL, NULL, TO_ROOM );
        send_to_char(
            "`RYou are incapacitated and will slowly die, if not aided.\n\r`w",
            victim );
        break;

    case POS_STUNNED:
        act( "`R$n `Ris stunned, but will probably recover.`w",
            victim, NULL, NULL, TO_ROOM );
        send_to_char("`RYou are stunned, but will probably recover.\n\r`w",
            victim );
        break;

    case POS_DEAD:
        act( "`R$n `Ris DEAD!!`w", victim, 0, 0, TO_ROOM );
        send_to_char( "`RYou have been KILLED!!\n\r\n\r`w", victim );
        break;

    default:
        if ( dam > victim->max_hit / 4 )
            send_to_char( "`RThat really did HURT`R!\n\r`w", victim );
        if ( victim->hit < victim->max_hit / 4 )
            send_to_char( "`RYou sure are BLEEDING`R!\n\r`w", victim );
        break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim) ) 
    {
        if (!IS_NPC(victim)) victim->pcdata->nemesis=victim->fighting;  
        stop_fighting( victim, FALSE );
    }

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
    {
        group_gain( ch, victim );

        if ( !IS_NPC(victim) )
        {
            sprintf( log_buf, "%s killed by %s at %d",
                victim->name,
                (IS_NPC(ch) ? ch->short_descr : ch->name),
                victim->in_room->vnum );
            log_string( log_buf );

            /*
             * Dying penalty:
             * Lose a stat?
             */
            if ( !chaos /* && stat > 0 */  &&
			( !IS_SET( ch->act, PLR_KILLER ) 
			|| !IS_SET( victim->act, PLR_KILLER ) ) ) {
            	// stat --
			}
        }
        if ( chaos )
        {
          /* chaos_points = 0; */
          /* chaos_points update by Rahl */       
          if ( !IS_NPC( victim ) && (ch->level < victim->level ))
              chaos_points = chaos_points + (2*(victim->level - ch->level));
          if ( !IS_NPC( victim ) )      
              chaos_points += victim->level;
		  if ( !IS_NPC( ch ) )
          	ch->pcdata->chaos_score = chaos_points;
        }

            if (!IS_NPC(victim)) {
            sprintf(buf, "%s has been slain by %s!", victim->name,
            IS_NPC(ch) ? ch->short_descr : ch->name);
            /* added by Rahl */
            wiznet( buf, NULL, NULL, WIZ_DEATHS, 0, 0 );

            //do_sendinfo(ch, buf);
        }
        /* added by Rahl */
        else
        {
            sprintf( buf, "%s killed by %s at %s. (room %d)",
                victim->short_descr, (IS_NPC(ch) ? ch->short_descr : ch->name),
                ch->in_room->name, ch->in_room->vnum );
            wiznet( buf, NULL, NULL, WIZ_MOBDEATHS, 0, 0 );
        } 

        /* insert bounty stuff here */
        if ( !IS_NPC( ch ) 
        && !IS_NPC( victim )
        && !chaos
        && ( victim != ch )
        && ( IS_SET( ch->act, PLR_BOUNTY_HUNTER ) &&
                victim->pcdata->bounty > 0 ) )
        {
            sprintf( buf, "You receive a %ld gold bounty for killing %s.\n\r",
                 victim->pcdata->bounty, victim->name );
            send_to_char( buf, ch );
            ch->gold += victim->pcdata->bounty;
            victim->pcdata->bounty = 0;
            victim->pcdata->pkilled++;
            ch->pcdata->pkills++;
            pk_kill( victim );
        }
        else if (chaos)
            chaos_kill( victim );
        else if ( IS_SET( victim->act, PLR_KILLER ) && IS_SET( ch->act,
        PLR_KILLER ) )
        {
            pk_kill( victim );
            ch->pcdata->pkills++;
            victim->pcdata->pkilled++;
        }
        else
        {
            if ( !IS_NPC( victim ) )
                victim->pcdata->killed++;
            raw_kill( victim );
        }           

		/* bonus stuff */
		if ( !IS_NPC( ch ) && IS_NPC( victim ) ) {
            if ( victim->fBonusMob ) {
                int bonusPoint_gain = victim->level / 10;
				if ( bonusPoint_gain < 1 )
					bonusPoint_gain = 1;
                send_to_char( "`GCongratulations! You have found a "
                    "bonus mob!`w\n\r", ch );
                sprintf( buf, "`GYou gain %d bonus point%s.`w\n\r",
                    bonusPoint_gain, ( bonusPoint_gain > 1 ) ? "s" : "" );
                send_to_char( buf, ch );
                ch->bonusPoints += bonusPoint_gain;
				if ( DEBUG ) {
					sprintf( buf, "%s killed by %s - bonus mob.\n\r",
						victim->short_descr, ch->name );
					log_string( buf );
				} // DEBUG
			} // if
		} // if
 
        /* RT new auto commands */

        if ( !IS_NPC(ch) && IS_NPC(victim) )
        {
            corpse = get_obj_list( ch, "corpse", ch->in_room->contents ); 

            if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
                 corpse && corpse->contains) /* exists and not empty */
                do_get( ch, "all corpse" );

            if (IS_SET(ch->act,PLR_AUTOGOLD) &&
                corpse && corpse->contains  && /* exists and not empty */
                !IS_SET(ch->act,PLR_AUTOLOOT))
              do_get(ch, "coins corpse");
            
            if ( IS_SET(ch->act, PLR_AUTOSAC) ) {
              if ( IS_SET(ch->act,PLR_AUTOLOOT) && corpse && corpse->contains) {
                return TRUE;  /* leave if corpse has treasure */
              } else {
                do_sacrifice( ch, "corpse" );
			  }
			}
        }

        return TRUE;
    }

    if ( victim == ch )
        return TRUE;

    /*
     * Take care of link dead people.
     */
    if ( !IS_NPC(victim) && victim->desc == NULL )
    {
        if ( number_range( 0, victim->wait ) == 0 )
        {
            do_recall( victim, "" );
            return TRUE;
        }
    }

    /*
     * Wimp out?
     */
    if ( IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2)
    {
        if ( ( IS_SET(victim->act, ACT_WIMPY) && number_bits( 2 ) == 0
        &&   victim->hit < victim->max_hit / 5) 
        ||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
        &&     victim->master->in_room != victim->in_room ) )
            do_flee( victim, "" );
    }

    if ( !IS_NPC(victim)
    &&   victim->hit > 0
    &&   victim->hit <= victim->wimpy
    &&   victim->wait < PULSE_VIOLENCE / 2 )
        do_flee( victim, "" );

    tail_chain( );
    return TRUE;
}

bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim )
{

    /* no killing NPCs with ACT_NO_KILL */

    if (IS_NPC(victim) && IS_SET(victim->act,ACT_NO_KILL))
    {
        send_to_char("I don't think the gods would approve.\n\r",ch);
        return TRUE;
    }

    /* no fighting in safe rooms */
    if (IS_SET(ch->in_room->room_flags,ROOM_SAFE))
    {
        send_to_char("Not in this room.\n\r",ch);
        return TRUE;
    }

    if (victim->fighting == ch)
        return FALSE;

    if (IS_NPC(ch))
    {
        /* charmed mobs and pets cannot attack players */
        if (!IS_NPC(victim) && (IS_AFFECTED(ch,AFF_CHARM)
                            ||  IS_SET(ch->act,ACT_PET)))
            return TRUE;

        return FALSE;
     }

     else /* Not NPC */
     {  
        if (IS_IMMORTAL(ch))
            return FALSE;

        /* no pets */
        if (IS_NPC(victim) && IS_SET(victim->act,ACT_PET))
        {
            act("But $N looks so cute and cuddly...",ch,NULL,victim,TO_CHAR);
            return TRUE;
        }

        /* no charmed mobs unless char is the the owner */
        if (IS_AFFECTED(victim,AFF_CHARM) && ch != victim->master)
        {
            send_to_char("You don't own that monster.\n\r",ch);
            return TRUE;
        }

        return FALSE;
    }
}

bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area )
{
    /* can't zap self (crash bug) */
    if (ch == victim)
        return TRUE;

    /* immortals not hurt in area attacks */
    if (IS_IMMORTAL(victim) &&  area)
        return TRUE;

    /* no killing NO_KILL mobiles */
    if (IS_NPC(victim) && IS_SET(victim->act,ACT_NO_KILL))
        return TRUE;

    /* no fighting in safe rooms */
    if (IS_SET(ch->in_room->room_flags,ROOM_SAFE))
        return TRUE;

    if (victim->fighting == ch)
        return FALSE;
 
    if (IS_NPC(ch))
    {
        /* charmed mobs and pets cannot attack players */
        if (!IS_NPC(victim) && (IS_AFFECTED(ch,AFF_CHARM)
                            ||  IS_SET(ch->act,ACT_PET)))
            return TRUE;
        
        /* area affects don't hit other mobiles */
        if (IS_NPC(victim) && area)
            return TRUE;
 
        return FALSE;
    }
 
    else /* Not NPC */
    {
        if (IS_IMMORTAL(ch) && !area)
            return FALSE;
 
        /* no pets */
        if (IS_NPC(victim) && IS_SET(victim->act,ACT_PET))
            return TRUE;
 
        /* no charmed mobs unless char is the the owner */
        if (IS_AFFECTED(victim,AFF_CHARM) && ch != victim->master)
            return TRUE;
 
        /* no player killing */
        if ( !IS_NPC(victim) )
        {
            if ( ( !chaos &&
            ( !IS_SET(victim->act, PLR_KILLER) 
            || !IS_SET(ch->act, PLR_KILLER) ) )
            && ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) 
            || victim->pcdata->bounty <= 0 ) ) )
          {
            send_to_char( "You can only kill other player killers.\n\r", ch );
            return TRUE;
          }
        }

        /* cannot use spells if not in same group */
        if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
            return TRUE;
 
        return FALSE;
    }
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
    /* added by Rahl */
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    while ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
        victim = victim->master;

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC(victim)
    ||   IS_SET(victim->act, PLR_KILLER)
    ||   IS_SET(victim->act, PLR_THIEF) )
    {
        buffer_free( buf );
        return;
    }

    /*
     * Charm-o-rama.
     */
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
    {
        if ( ch->master == NULL )
        {
            bprintf( buf, "Check_killer: %s bad AFF_CHARM",
                IS_NPC(ch) ? ch->short_descr : ch->name );
            bug( buf->data, 0 );
            affect_strip( ch, gsn_charm_person );
            REMOVE_BIT( ch->affected_by, AFF_CHARM );
            buffer_free( buf );
            return;
        }
/*
        send_to_char( "`R*** You are now a KILLER!! ***`w\n\r", ch->master );
        SET_BIT(ch->master->act, PLR_KILLER);
*/
        stop_follower( ch );
        buffer_free( buf );
        return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch)
    ||   ch == victim
    ||   IS_IMMORTAL( ch ) 
    ||   IS_SET(ch->act, PLR_KILLER)
    ||   chaos 
    ||  ( IS_SET( ch->act, PLR_BOUNTY_HUNTER ) && victim->pcdata->bounty ) )
    {
        buffer_free( buf );
        return;
    }

    send_to_char( "`R*** You are now a KILLER!! ***`w\n\r", ch );
    SET_BIT(ch->act, PLR_KILLER);
    /* added by Rahl */
    bprintf( buf, "%s is attempting to murder %s.", 
        ch->name, victim->name );
    wiznet( buf->data, NULL, NULL, WIZ_FLAGS, 0, 0 );

    save_char_obj( ch );
    buffer_free( buf );
    return;
}



/*
 * Check for parry.
 */
bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;

    if ( IS_NPC(victim) )
    {
        chance  = UMIN( 30, victim->level );
    }
    else
    {
        if ( get_eq_char( victim, WEAR_WIELD ) == NULL )
            return FALSE;
        chance  = victim->pcdata->learned[gsn_parry] / 2;
    }

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "`BYou parry $n`B's attack.`w",  ch, NULL, victim, TO_VICT    );
    act( "`B$N `Bparries your attack.`w", ch, NULL, victim, TO_CHAR    );
    check_improve(victim,gsn_parry,TRUE,6);
    return TRUE;
}



/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;

    if ( IS_NPC(victim) )
        chance  = UMIN( 30, victim->level );
    else
        chance  = victim->pcdata->learned[gsn_dodge] / 2;

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "`BYou dodge $n`B's attack.`w", ch, NULL, victim, TO_VICT    );
    act( "`B$N `Bdodges your attack.`w", ch, NULL, victim, TO_CHAR    );
    check_improve(victim,gsn_dodge,TRUE,6);
    return TRUE;
}



/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{
    if ( victim->hit > 0 )
    {
        if ( victim->position <= POS_STUNNED )
            victim->position = POS_STANDING;
        return;
    }

    if ( IS_NPC(victim) && victim->hit < 1 )
    {
        victim->position = POS_DEAD;
        return;
    }

    if ( victim->hit <= -11 )
    {
        victim->position = POS_DEAD;
        return;
    }

         if ( victim->hit <= -6 ) victim->position = POS_MORTAL;
    else if ( victim->hit <= -3 ) victim->position = POS_INCAP;
    else                          victim->position = POS_STUNNED;

    return;
}



/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->fighting != NULL )
    {
        bug( "Set_fighting: already fighting", 0 );
        return;
    }

    if ( IS_AFFECTED(ch, AFF_SLEEP) )
        affect_strip( ch, gsn_sleep );

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    return;
}



/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
    CHAR_DATA *fch;

    for ( fch = char_list; fch != NULL; fch = fch->next )
    {
        if ( fch == ch || ( fBoth && fch->fighting == ch ) )
        {
            fch->fighting       = NULL;
            fch->position       = IS_NPC(fch) ? ch->default_pos : POS_STANDING;
            update_pos( fch );
        }
    }

    return;
}



/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;

    if ( IS_NPC(ch) )
    {
        name            = ch->short_descr;
        corpse          = create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
        corpse->timer   = number_range( 3, 6 );
        if ( ch->gold > 0 )
        {
            obj_to_obj( create_money( ch->gold ), corpse );
            ch->gold = 0;
        }
        corpse->cost = 0;
    }
    else
    {
        name            = ch->name;
        corpse          = create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
        corpse->timer   = number_range( 25, 40 );
/*
        REMOVE_BIT(ch->act,PLR_CANLOOT);
*/

/*      if (!IS_SET(ch->act,PLR_KILLER) && !IS_SET(ch->act,PLR_THIEF)) */
        if ( !IS_SET( ch->act, PLR_THIEF ) )
        {
            free_string( corpse->owner );
            corpse->owner = str_dup(ch->name);
        }
        else
            corpse->owner = NULL;
        corpse->cost = 0;
    }

    corpse->level = ch->level;

    bprintf( buf, corpse->short_descr, name );
    free_string( corpse->short_descr );
    corpse->short_descr = str_dup( buf->data );

    bprintf( buf, corpse->description, name );
    free_string( corpse->description );
    corpse->description = str_dup( buf->data );

    if ( IS_NPC(ch) || (ch->pcdata->nemesis == NULL) 
        || (!IS_NPC(ch) && IS_NPC(ch->pcdata->nemesis)) ) 
    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;
        obj_from_char( obj );
        if (obj->item_type == ITEM_POTION)
            obj->timer = number_range(500,1000);
        if (obj->item_type == ITEM_SCROLL)
            obj->timer = number_range(1000,2500);
        if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH))
            obj->timer = number_range(5,10);
        REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);
        REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);

        if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
            extract_obj( obj );
        else
            obj_to_obj( obj, corpse );
    }

    obj_to_room( corpse, ch->in_room );
    buffer_free( buf );
    return;
}



/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    int door;
    int vnum;

    vnum = 0;
    msg = "`YYou hear $n`Y's death cry.`w";

    switch ( number_bits(4))
    {
    case  0: msg  = "$n `whits the ground ... DEAD."; break;
    case  1: 
        if (ch->material == 0)
        {
            msg  = "`R$n `Rsplatters blood on your armor.`w";           
            break;
        }
    case  2:                                                    
        if (IS_SET(ch->parts,PART_GUTS))
        {
            msg = "`R$n `Rspills $s guts all over the floor.`w";
            vnum = OBJ_VNUM_GUTS;
        }
        break;
    case  3: 
        if (IS_SET(ch->parts,PART_HEAD))
        {
            msg  = "`R$n`R's severed head plops on the ground.`w";
            vnum = OBJ_VNUM_SEVERED_HEAD;                               
        }
        break;
    case  4: 
        if (IS_SET(ch->parts,PART_HEART))
        {
            msg  = "`R$n`R's heart is torn from $s chest.`w";
            vnum = OBJ_VNUM_TORN_HEART;                         
        }
        break;
    case  5: 
        if (IS_SET(ch->parts,PART_ARMS))
        {
            msg  = "`R$n`R's arm is sliced from $s dead body.`w";
            vnum = OBJ_VNUM_SLICED_ARM;                         
        }
        break;
    case  6: 
        if (IS_SET(ch->parts,PART_LEGS))
        {
            msg  = "`R$n`R's leg is sliced from $s dead body.`w";
            vnum = OBJ_VNUM_SLICED_LEG;                         
        }
        break;
    case 7:
        if (IS_SET(ch->parts,PART_BRAINS))
        {
            msg = "`R$n`R's head is shattered, and $s brains splash all over you.`w";
            vnum = OBJ_VNUM_BRAINS;
        }
    }

    act( msg, ch, NULL, NULL, TO_ROOM );

    if ( vnum != 0 )
    {
        BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
        OBJ_DATA *obj;
        char *name;

        name            = IS_NPC(ch) ? ch->short_descr : ch->name;
        obj             = create_object( get_obj_index( vnum ), 0 );
        obj->timer      = number_range( 4, 7 );

        bprintf( buf, obj->short_descr, name );
        free_string( obj->short_descr );
        obj->short_descr = str_dup( buf->data );

        bprintf( buf, obj->description, name );
        free_string( obj->description );
        obj->description = str_dup( buf->data );

        if (obj->item_type == ITEM_FOOD)
        {
            if (IS_SET(ch->form,FORM_POISON))
                obj->value[3] = 1;
            else if (!IS_SET(ch->form,FORM_EDIBLE))
                obj->item_type = ITEM_TRASH;
        }

        obj_to_room( obj, ch->in_room );
        buffer_free( buf );
    }

    if ( IS_NPC(ch) )
        msg = "`RYou hear something's death cry.`w";
    else
        msg = "`RYou hear someone's death cry.`w";

    was_in_room = ch->in_room;
    for ( door = 0; door <= 5; door++ )
    {
        EXIT_DATA *pexit;

        if ( ( pexit = was_in_room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL
        &&   pexit->u1.to_room != was_in_room )
        {
            ch->in_room = pexit->u1.to_room;
            act( msg, ch, NULL, NULL, TO_ROOM );
        }
    }
    ch->in_room = was_in_room;

    return;
}

void chaos_kill( CHAR_DATA *victim)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    DESCRIPTOR_DATA *d;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    stop_fighting( victim, TRUE );
    for ( obj = victim->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;
        obj_from_char( obj );
        obj_to_room( obj, victim->in_room );
    }
    act( "`B$n`B's corpse is sucked into the ground!!`w", victim, 0, 0, TO_ROOM );
    if ( !IS_NPC(victim) )
    {   
        bprintf(buf, "was slain with %d chaos points.",
            victim->pcdata->chaos_score);
        chaos_log( victim, buf->data );
    }
    d = victim->desc;
    extract_char( victim, TRUE );
    if ( d != NULL )
        close_socket( d );

    buffer_free( buf );
    return;
}


void raw_kill( CHAR_DATA *victim )
{
    int i;

    make_corpse( victim );
    stop_fighting( victim, TRUE );

    if ( IS_NPC(victim) )
    {
        victim->pIndexData->killed++;
        extract_char( victim, TRUE );
        return;
    }

    extract_char( victim, FALSE );
    while ( victim->affected )
        affect_remove( victim, victim->affected );
    victim->affected_by = 0;
    for (i = 0; i < 4; i++)
        victim->armor[i]= 100;
    victim->position    = POS_RESTING;
    victim->hit         = UMAX( 1, victim->hit  );
    victim->mana        = UMAX( 1, victim->mana );
    victim->move        = UMAX( 1, victim->move );
    /* RT added to prevent infinite deaths */
    REMOVE_BIT(victim->act, PLR_THIEF);
    REMOVE_BIT(victim->act, PLR_BOUGHT_PET);
/*
    save_char_obj( victim ); 
*/
    /* Add back race affects */
    victim->affected_by = victim->affected_by|race_table[victim->race].aff;
    victim->affected2_by = race_table[victim->race].aff2;

    return;
}


void pk_kill( CHAR_DATA *victim )
{
    make_corpse( victim );
    stop_fighting( victim, TRUE );

    if ( IS_NPC(victim) )
    {
        victim->pIndexData->killed++;
        extract_char( victim, TRUE );
        return;
    }

    extract_char( victim, FALSE );
    while ( victim->affected )
        affect_remove( victim, victim->affected );
    victim->affected_by = 0;
    victim->position    = POS_RESTING;
    victim->hit         = UMAX( 1, victim->hit  );
    victim->mana        = UMAX( 1, victim->mana );
    victim->move        = UMAX( 1, victim->move );
    /* RT added to prevent infinite deaths */
    REMOVE_BIT(victim->act, PLR_THIEF);
    REMOVE_BIT(victim->act, PLR_BOUGHT_PET);
  /*  save_char_obj( victim ); */ 
    /* Add back race affects */
    victim->affected_by = victim->affected_by|race_table[victim->race].aff;
    victim->affected2_by = race_table[victim->race].aff2;
    return;
}



void group_gain( CHAR_DATA *ch, CHAR_DATA *victim ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *gch;
    CHAR_DATA *lch;
    int xp;
    int members;
    int group_levels;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if ( !IS_NPC(victim) || victim == ch ) {
        buffer_free( buf );
        return;
    }
    
    members = 0;
    group_levels = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group( gch, ch ) )
        {
            members++;
            group_levels += gch->level;
        }
    }

    if ( members == 0 )
    {
        bug( "Group_gain: members.", members );
        members = 1;
        group_levels = ch->level ;
    }

    lch = (ch->leader != NULL) ? ch->leader : ch;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        OBJ_DATA *obj;
        OBJ_DATA *obj_next;

        if ( !is_same_group( gch, ch ) || IS_NPC(gch))
            continue;


        if (IS_SET(ch->act, PLR_QUESTOR) && IS_NPC(victim))
        {
            if (ch->questmob == victim->pIndexData->vnum)
            {
                send_to_char( "`MYou have almost completed your QUEST!`w\n\r", ch );
                send_to_char( "`MReturn to the questmaster before your time runs out!`w\n\r", ch );
                ch->questmob = -1;
            }
        }

        xp = xp_compute( gch, victim, group_levels, members );  
        bprintf( buf, "`WYou receive %d experience points.\n\r`w", xp );
        send_to_char( buf->data, gch );
        gain_exp( gch, xp );

        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if ( obj->wear_loc == WEAR_NONE )
                continue;

            if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(ch)    )
            ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
            ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ) )
            {
                act( "`WYou are zapped by $p.`w", ch, obj, NULL, TO_CHAR );
                act( "`W$n `Wis zapped by $p.`w",   ch, obj, NULL, TO_ROOM );
                obj_from_char( obj );
                obj_to_room( obj, ch->in_room );
            }
        }
    }

    buffer_free( buf );
    return;
}



/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute( CHAR_DATA *gch, CHAR_DATA *victim, int total_levels, int members )
{
    int xp = 0;
	int base_exp = 0;
    int align;
    int change;

    /* compute the base exp */
    int gch_hp = gch->max_hit;
    int victim_hp = victim->max_hit;

    if ( victim_hp >= gch_hp ) {
		base_exp = 50;
	} else if ( victim_hp >= ( gch_hp - ( gch_hp * 0.1 ) ) ) {
		base_exp = 40;
    } else if ( victim_hp >= ( gch_hp - ( gch_hp * 0.25 ) )  ) {
		base_exp = 25;
	} else if ( victim_hp >= ( gch_hp - ( gch_hp * 0.5 ) ) ) {
		base_exp = 10;
	} else if ( victim_hp >= ( gch_hp - ( gch_hp * 0.75 ) ) ) {
		base_exp = 5;
	} else {
		base_exp = 1;
	}
    
    /* do alignment computations */
    /*
     * the / 10's by Rahl.. too many people complain about align changing
     * too fast. Were /2's
     */
    align = ( victim->alignment - gch->alignment );

    if (IS_SET(victim->act,ACT_NOALIGN))
    {
        /* no change */
    }

    else if (align > 500) /* monster is more good than slayer */
    {
        change = (align - 500) * (gch->level/total_levels+(1/members))/10; 
        change = UMAX(1,change);
        gch->alignment = UMAX(-1000,gch->alignment - change);
    }

    else if (align < -500) /* monster is more evil than slayer */
    {
        change =  ( -1 * align - 500) * (gch->level/total_levels+(1/members))/10;
        change = UMAX(1,change);
        gch->alignment = UMIN(1000,gch->alignment + change);
    }

    else /* improve this someday */
    {
        change =  gch->alignment * (gch->level/total_levels+(1/members))/10;  
        gch->alignment -= change;
    }
    
    /* calculate exp multiplier */
    if (IS_SET(victim->act,ACT_NOALIGN))
    xp = base_exp;

    else if (gch->alignment > 500)  /* for goodie two shoes */
    {
        if (victim->alignment < -750)
            xp = base_exp * 4/3;
   
        else if (victim->alignment < -500)
            xp = base_exp * 5/4;

        else if (victim->alignment > 250)
            xp = base_exp * 3/4; 

        else if (victim->alignment > 500)
            xp = base_exp / 2;

        else if (victim->alignment > 750)
            xp = base_exp / 4;

        else
            xp = base_exp;
    }

    else if (gch->alignment < -500) /* for baddies */
    {
        if (victim->alignment > 750)
            xp = base_exp * 5/4;
        
        else if (victim->alignment > 500)
            xp = base_exp * 11/10; 

        else if (victim->alignment < -750)
            xp = base_exp * 1/2;

        else if (victim->alignment < -500)
            xp = base_exp * 3/4;

        else if (victim->alignment < -250)
            xp = base_exp * 9/10;

        else
            xp = base_exp;
    }

    else if (gch->alignment > 200)  /* a little good */
    {

        if (victim->alignment < -500)
            xp = base_exp * 6/5;

        else if (victim->alignment > 750)
            xp = base_exp * 1/2;

        else if (victim->alignment > 0)
            xp = base_exp * 3/4; 
        
        else
            xp = base_exp;
    }

    else if (gch->alignment < -200) /* a little bad */
    {
        if (victim->alignment > 500)
            xp = base_exp * 6/5;
 
        else if (victim->alignment < -750)
            xp = base_exp * 1/2;

        else if (victim->alignment < 0)
            xp = base_exp * 3/4;

        else
            xp = base_exp;
    }

    else /* neutral */
    {

        if (victim->alignment > 500 || victim->alignment < -500)
            xp = base_exp * 4/3;

        else if (victim->alignment < 200 || victim->alignment > -200)
            xp = base_exp * 1/2;

        else
            xp = base_exp;
    }

   
    /* randomize the rewards */
    xp = number_range (xp * 3/4, xp * 5/4);

    /* adjust for grouping */
   /* if (members > 1 ) xp = xp * (((gch->level/total_levels)+(1/members))/2);
*/
      if (members > 1 ) {
		xp = (gch->level * (xp / total_levels));
		/* 15% increase by Rahl */
      	xp *= 1.15;
	  }
    return xp;
}


void dam_message( CHAR_DATA *ch, CHAR_DATA *victim,int dam,int dt,bool immune )
{
    BUFFER *buf1 = buffer_new( 256 );
    BUFFER *buf2 = buffer_new( 256 );
    BUFFER *buf3 = buffer_new( 256 );
    const char *vs;
    const char *vp;
    const char *attack;
    char punct;

         if ( dam ==   0 ) { vs = "miss";       vp = "misses";          }
    else if ( dam <=   4 ) { vs = "scratch";    vp = "scratches";       }
    else if ( dam <=   8 ) { vs = "graze";      vp = "grazes";          }
    else if ( dam <=  12 ) { vs = "hit";        vp = "hits";            }
    else if ( dam <=  16 ) { vs = "injure";     vp = "injures";         }
    else if ( dam <=  20 ) { vs = "wound";      vp = "wounds";          }
    else if ( dam <=  24 ) { vs = "maul";       vp = "mauls";           }
    else if ( dam <=  28 ) { vs = "decimate";   vp = "decimates";       }
    else if ( dam <=  32 ) { vs = "devastate";  vp = "devastates";      }
    else if ( dam <=  36 ) { vs = "maim";       vp = "maims";           }
    else if ( dam <=  40 ) { vs = "MUTILATE";   vp = "MUTILATES";       }
    else if ( dam <=  44 ) { vs = "PULVERIZE";  vp = "PULVERIZES";      }
    else if ( dam <=  48 ) { vs = "DISMEMBER";  vp = "DISMEMBERS";      }
    else if ( dam <=  52 ) { vs = "MASSACRE";   vp = "MASSACRES";       }
    else if ( dam <=  56 ) { vs = "MANGLE";     vp = "MANGLES";         }
    else if ( dam <=  60 ) { vs = "*** DEMOLISH ***";
                             vp = "*** DEMOLISHES ***";                 }
    else if ( dam <=  75 ) { vs = "*** DEVASTATE ***";
                             vp = "*** DEVASTATES ***";                 }
    else if ( dam <= 100)  { vs = "=== OBLITERATE ===";
                             vp = "=== OBLITERATES ===";                }
    else if ( dam <= 125)  { vs = ">>> ANNIHILATE <<<";
                             vp = ">>> ANNIHILATES <<<";                }
    else if ( dam <= 150)  { vs = "<<< ERADICATE >>>";
                             vp = "<<< ERADICATES >>>";                 }
    else if ( dam <= 165)  { vs = "<><><> BUTCHER <><><>";
                             vp = "<><><> BUTCHERS <><><>";             }
    else if ( dam <= 185)  { vs = "<><><> DISEMBOWEL <><><>";
                             vp = "<><><> DISEMBOWELS <><><>";          }
    /* more stuff added by Rahl in a fit of boredom */
    else if ( dam <= 200)  { vs = "\\/\\/\\/ EVISCERATE /\\/\\/\\";
                             vp = "\\/\\/\\/ EVISCERATES /\\/\\/\\";    }
    else if ( dam <= 225)   { vs = "<*<*< DISINTEGRATE >*>*>";
                              vp = "<*<*< DISINTEGRATES >*>*>";         }
    else if ( dam <= 250)  { vs = "<--- LACERATE --->";
                             vp = "<--- LACERATES --->";                }
    else if ( dam <= 275)  { vs = "*=*=* RAVAGE *=*=*";
                             vp = "*=*=* RAVAGES *=*=*";                }

   /* changed from less than 200 by Rahl so I could add UNGODLY things :)*/
    else if ( dam <= 300)  { vs = "do UNSPEAKABLE things to";
                             vp = "does UNSPEAKABLE things to";         }
    else                   { vs = "do UNGODLY things to";
                             vp = "does UNGODLY things to";             }

    punct   = (dam <= 24) ? '.' : '!';

    if ( dt == TYPE_HIT )
    {
        if (dam > 0)
            if (ch  == victim)
            {
                bprintf( buf1, "`w$n `R%s`Y $melf%c",vp,punct);
                bprintf( buf2, "`wYou `R%s`Y yourself%c",vs,punct);
            }
            else
            {
                bprintf( buf1, "`w$n `R%s`w $N%c",  vp, punct );
                if ( IS_SET( ch->act, PLR_AUTODAMAGE ) )
                    bprintf( buf2, "`wYou `R%s`w $N%c `M(Damage %d)`w",
                        vs, punct, dam );
                else
                    bprintf( buf2, "`wYou `R%s`w $N%c", vs, punct );
                bprintf( buf3, "`w$n `R%s`w you%c", vp, punct );
            }
        else if (ch == victim)
        {
            bprintf( buf1, "`B$n %s $melf%c`w",vp,punct);
            bprintf( buf2, "`BYou %s yourself%c`w",vs,punct);
        }
        else
        {
            bprintf( buf1, "`B$n %s $N%c`w",  vp, punct );
            if ( IS_SET( ch->act, PLR_AUTODAMAGE ) )
                bprintf( buf2, "`BYou %s $N%c `M(Damage: %d)`w",vs, punct,
                    dam);
            else
                bprintf( buf2, "`BYou %s $N%c`w", vs, punct );
            bprintf( buf3, "`B$n %s you%c`w", vp, punct );
        }
    }
    else
    {
        if ( dt >= 0 && dt < MAX_SKILL )
            attack      = skill_table[dt].noun_damage;
        else if ( dt >= TYPE_HIT
        && dt <= TYPE_HIT + MAX_DAMAGE_MESSAGE) 
            attack      = attack_table[dt - TYPE_HIT].name;
        else
        {
            bug( "Dam_message: bad dt %d.", dt );
            dt  = TYPE_HIT;
            attack  = attack_table[0].name;
        }

        if (immune)
        {
            if (ch == victim)
            {
                bprintf(buf1,"`B$n `Bis unaffected by $s own %s.`w",
                    attack);
                bprintf(buf2,"`BLuckily, you are immune to that.`w");
            } 
            else
            {
                bprintf(buf1,"`B$N is unaffected by $n`B's %s!`w",
                    attack);
                bprintf(buf2,"`B$N is unaffected by your %s!`w",attack);
                bprintf(buf3,"`B$n`B's %s is powerless against you.`w",
                    attack);
            }
        }
        else
        {
            if (dam > 0)
                if (ch == victim)
                {
                    bprintf( buf1, "`w$n's %s `R%s`w $m%c",
                        attack,vp,punct);
                    if ( IS_SET( ch->act, PLR_AUTODAMAGE ) )
                        bprintf( buf2, 
                            "`wYour %s `R%s `wyou%c `M(Damage %d)`w",
                            attack,vp,punct,dam);
                    else                    
                        bprintf( buf2, "`wYour %s `R%s`w you%c",
                            attack,vp,punct);
                }
                else
                {
                    bprintf( buf1, "`w$n's %s `R%s`w $N%c",  
                        attack, vp, punct );
                    if ( IS_SET( ch->act, PLR_AUTODAMAGE ) )
                        bprintf( buf2, 
                            "`wYour %s `R%s`w $N%c `M(Damage %d)`w", 
                            attack, vp, punct,dam );
                    else
                        bprintf( buf2, "`wYour %s `R%s`w $N%c",  
                            attack, vp, punct );
                    bprintf( buf3, "`w$n's %s `R%s`w you%c", 
                        attack, vp, punct );
                }
            else if (ch == victim)
            {
                bprintf( buf1, "`B$n's %s %s $m%c`w",attack,vp,punct);
                if ( IS_SET( ch->act, PLR_AUTODAMAGE ) )
                    bprintf( buf2, "`BYour %s %s you%c `M(Damage %d)`w",
                        attack,vp,punct,dam);
                else
                    bprintf( buf2, "`BYour %s %s you%c`w", 
                        attack,vp,punct);
            }
            else
            {
                bprintf( buf1, "`B$n's %s %s $N%c`w",  
                    attack, vp, punct );
                if ( IS_SET( ch->act, PLR_AUTODAMAGE ) )
                    bprintf( buf2, "`BYour %s %s $N%c `M(Damage %d)`w", 
                        attack, vp, punct, dam );
                else
                    bprintf( buf2, "`BYour %s %s $N%c`w",  
                        attack, vp, punct );
                bprintf( buf3, "`B$n's %s %s you%c`w", 
                    attack, vp, punct );
            }
        }
    }

    if (ch == victim)
    {
        act(buf1->data,ch,NULL,NULL,TO_ROOM);
        act(buf2->data,ch,NULL,NULL,TO_CHAR);
    }
    else
    {
        act( buf1->data, ch, NULL, victim, TO_NOTVICT );
        act( buf2->data, ch, NULL, victim, TO_CHAR );
        act( buf3->data, ch, NULL, victim, TO_VICT );
    }

    buffer_free( buf1 );
    buffer_free( buf2 );
    buffer_free( buf3 );
    return;
}



/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
        return;

    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE))
    {
        act("`W$S weapon won't budge!`w",ch,NULL,victim,TO_CHAR);
        act("`W$n`W tries to disarm you, but your weapon won't budge!`w",
            ch,NULL,victim,TO_VICT);
        act("`W$n`W tries to disarm $N`W, but fails.`w",ch,NULL,victim,TO_NOTVICT);
        return;
    }

    act( "`W$n`W disarms you and sends your weapon flying!`w", 
         ch, NULL, victim, TO_VICT    );
    act( "`WYou disarm $N`W!`w",  ch, NULL, victim, TO_CHAR    );
    act( "`W$n disarms $N`W!`w",  ch, NULL, victim, TO_NOTVICT );

    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
        obj_to_char( obj, victim );
    else
    {
        obj_to_room( obj, victim->in_room );
        if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
            get_obj(victim,obj,NULL);
    }

    return;
}

void do_berserk( CHAR_DATA *ch, char *argument)
{
    int chance, hp_percent;

    if ((chance = get_skill(ch,gsn_berserk)) == 0
    ||  (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BERSERK))
    ||  (!IS_NPC(ch)
    &&   ch->level < skill_table[gsn_berserk].skill_level[ch->ch_class]))
    {
        send_to_char("You turn red in the face, but nothing happens.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk)
    ||  is_affected(ch,skill_lookup("frenzy")))
    {
        send_to_char("You get a little madder.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CALM))
    {
        send_to_char("You're feeling to mellow to berserk.\n\r",ch);
        return;
    }

    if (ch->mana < 50)
    {
        send_to_char("You can't get up enough energy.\n\r",ch);
        return;
    }

    /* modifiers */

    /* fighting */
    if (ch->position == POS_FIGHTING)
        chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit/ch->max_hit;
    chance += 25 - hp_percent/2;

    if (number_percent() < chance)
    {
        AFFECT_DATA af;

        WAIT_STATE(ch,PULSE_VIOLENCE);
        ch->mana -= 50;
        ch->move /= 2;

        /* heal a little damage */
        ch->hit += ch->level * 2;
        ch->hit = UMIN(ch->hit,ch->max_hit);

        send_to_char("`RYour pulse races as you are consumned by rage!\n\r`w",ch);
        act("`W$n `Wgets a wild look in $s eyes.`w",ch,NULL,NULL,TO_ROOM);
        check_improve(ch,gsn_berserk,TRUE,2);

        af.where        = TO_AFFECTS;
        af.type         = gsn_berserk;
        af.level        = ch->level;
        af.duration     = number_fuzzy(ch->level / 8);
        af.modifier     = UMAX(1,ch->level/5);
        af.bitvector    = AFF_BERSERK;

        af.location     = APPLY_HITROLL;
        affect_to_char(ch,&af);

        af.location     = APPLY_DAMROLL;
        affect_to_char(ch,&af);

        af.modifier     = UMAX(10,10 * (ch->level/5));
        af.location     = APPLY_AC;
        affect_to_char(ch,&af);
    }

    else
    {
        WAIT_STATE(ch,3 * PULSE_VIOLENCE);
        ch->mana -= 25;
        ch->move /= 2;

        send_to_char("Your pulse speeds up, but nothing happens.\n\r",ch);
        check_improve(ch,gsn_berserk,FALSE,2);
    }
}

void do_bash( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);
 
    if ( (chance = get_skill(ch,gsn_bash)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BASH))
    ||   (!IS_NPC(ch)
    &&    ch->level < skill_table[gsn_bash].skill_level[ch->ch_class]))
    {   
        send_to_char("Bashing? What's that?\n\r",ch);
        return;
    }
 
    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("But you aren't fighting anyone!\n\r",ch);
            return;
        }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }

    if ( !IS_NPC(victim) )
    {
     
        if  (( !chaos && 
        ( !IS_SET(victim->act, PLR_KILLER) || !IS_SET(ch->act,
        PLR_KILLER) ) ) 
        && 
        ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) ||
        victim->pcdata->bounty <= 0 ) ) )
        {
            send_to_char( "You can only kill other player killers.\n\r", ch );
            return;
        }
    }

    if (victim->position < POS_FIGHTING)
    {
        act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR);
        return;
    } 

    if (victim == ch)
    {
        send_to_char("You try to bash your brains out, but fail.\n\r",ch);
        return;
    }

    if (is_safe(ch,victim))
        return;

    if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
        act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
        return;
    }

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 25;
    chance -= victim->carry_weight / 20;

    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 25;
    else
        chance += (ch->size - victim->size) * 10; 


    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= get_curr_stat(victim,STAT_DEX) * 4/3;

    chance -= GET_AC(victim,AC_BASH) /15;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST))
        chance += 10;
    if ( IS_AFFECTED( ch, AFF_HASTE ) )
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST))
        chance -= 20;
    if ( IS_AFFECTED( victim, AFF_HASTE ) )
        chance -= 20;
    /* added by Rahl */
    if (IS_AFFECTED(ch, AFF_SLOW))
        chance -=10;
    if (IS_AFFECTED(victim, AFF_SLOW))
        chance += 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* now the attack */
    if (number_percent() < chance)
    {
    
        act("`W$n `Wsends you sprawling with a powerful bash!`w",
                ch,NULL,victim,TO_VICT);
        act("`WYou slam into $N`W, and send $M flying!`w",ch,NULL,victim,TO_CHAR);
        act("`W$n `Wsends $N `Wsprawling with a powerful bash.`w",
                ch,NULL,victim,TO_NOTVICT);
        check_improve(ch,gsn_bash,TRUE,1);

        WAIT_STATE(victim, 1.5 * PULSE_VIOLENCE);
        WAIT_STATE(ch,skill_table[gsn_bash].beats);
        victim->position = POS_RESTING;
/*
        damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash,
            DAM_BASH, TRUE);
*/
/* new damage attempt by Rahl */
        damage(ch,victim,
            number_range((ch->level/4),(ch->level/2)+ch->size+(chance/10)),
            gsn_bash, DAM_BASH, TRUE);        
    }
    else
    {
        damage(ch,victim,0,gsn_bash,DAM_BASH, TRUE);
        act("`BYou fall flat on your face!`w",
            ch,NULL,victim,TO_CHAR);
        act("`B$n `Bfalls flat on $s face.`w",
            ch,NULL,victim,TO_NOTVICT);
        act("`BYou evade $n`B's bash, causing $m to fall flat on $s face.`w",
            ch,NULL,victim,TO_VICT);
        check_improve(ch,gsn_bash,FALSE,1);
        ch->position = POS_RESTING;
        WAIT_STATE(ch,skill_table[gsn_bash].beats * 3/2); 
    }
}

void do_dirt( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_dirt)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK_DIRT))
    ||   (!IS_NPC(ch)
    &&    ch->level < skill_table[gsn_dirt].skill_level[ch->ch_class]))
    {
        send_to_char("You get your feet dirty.\n\r",ch);
        return;
    }

    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("But you aren't in combat!\n\r",ch);
            return;
        }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(victim,AFF_BLIND))
    {
        act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        send_to_char("Very funny.\n\r",ch);
        return;
    }

    if (is_safe(ch,victim))
        return;
    if ( !IS_NPC(victim) )
    {
        if (( !chaos &&
        ( !IS_SET(victim->act, PLR_KILLER) || !IS_SET(ch->act,
        PLR_KILLER) ) )
        &&
        ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) ||
        victim->pcdata->bounty <= 0 ) ) )
        {
            send_to_char( "You can only kill other player killers.\n\r", ch );
            return;
        }
    }


    if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
        act("But $N is such a good friend!",ch,NULL,victim,TO_CHAR);
        return;
    }

    /* modifiers */

    /* dexterity */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_DEX);

    /* speed  */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 25;
    /* added by Rahl */
    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        chance -= 10;
    if ( IS_AFFECTED( victim, AFF_SLOW ) )
        chance += 10;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0)
        chance += 1;

    /* terrain */

    switch(ch->in_room->sector_type)
    {
        case(SECT_INSIDE):              chance -= 20;   break;
        case(SECT_CITY):                chance -= 10;   break;
        case(SECT_FIELD):               chance +=  5;   break;
        case(SECT_FOREST):                              break;
        case(SECT_HILLS):                               break;
        case(SECT_MOUNTAIN):            chance -= 10;   break;
        case(SECT_WATER_SWIM):          chance  =  0;   break;
        case(SECT_WATER_NOSWIM):        chance  =  0;   break;
        case(SECT_AIR):                 chance  =  0;   break;
        case(SECT_DESERT):              chance += 10;   break;
        case(SECT_UNDERGROUND):         chance +=  5;   break;
    }

    if (chance == 0)
    {
        send_to_char("There isn't any dirt to kick.\n\r",ch);
        return;
    }

    /* now the attack */
    if (number_percent() < chance)
    {
        AFFECT_DATA af;
        act("`W$n `Wis blinded by the dirt in $s eyes!`w",victim,NULL,NULL,TO_ROOM);
        damage(ch,victim,number_range(2,5),gsn_dirt,DAM_NONE, TRUE);
        send_to_char("`WYou can't see a thing!\n\r`w",victim);
        check_improve(ch,gsn_dirt,TRUE,2);
        WAIT_STATE(ch,skill_table[gsn_dirt].beats);

        af.where        = TO_AFFECTS;
        af.type         = gsn_dirt;
        af.level        = ch->level;
        af.duration     = 0;
        af.location     = APPLY_HITROLL;
        af.modifier     = -4;
        af.bitvector    = AFF_BLIND;

        affect_to_char(victim,&af);
    }
    else
    {
        damage(ch,victim,0,gsn_dirt,DAM_NONE, TRUE);
        check_improve(ch,gsn_dirt,FALSE,2);
        WAIT_STATE(ch,skill_table[gsn_dirt].beats);
    }
}

void do_trip( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_trip)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_TRIP))
    ||   (!IS_NPC(ch) 
          && ch->level < skill_table[gsn_trip].skill_level[ch->ch_class]))
    {
        send_to_char("Tripping?  What's that?\n\r",ch);
        return;
    }


    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("But you aren't fighting anyone!\n\r",ch);
            return;
        }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }

    if (is_safe(ch,victim))
        return;

    if ( !IS_NPC(victim) )
    {
        if (( !chaos &&
        ( !IS_SET(victim->act, PLR_KILLER) || !IS_SET(ch->act,
        PLR_KILLER) ) )
        &&
        ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) ||
        victim->pcdata->bounty <= 0 ) ) )
        {
            send_to_char( "You can only kill other player killers.\n\r", ch );
            return;
        }
    }

    if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(victim,AFF_FLYING))
    {
        act("$S feet aren't on the ground.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (victim->position < POS_FIGHTING)
    {
        act("$N is already down.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        send_to_char("`BYou fall flat on your face!\n\r`w",ch);
        WAIT_STATE(ch,2 * skill_table[gsn_trip].beats);
        act("$n trips over $s own feet!",ch,NULL,NULL,TO_ROOM);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
        act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
        return;
    }

    /* modifiers */

    /* size */
    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */

    /* dex */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;


    /* now the attack */
    if (number_percent() < chance)
    {
        act("`W$n `Wtrips you and you go down!`w",ch,NULL,victim,TO_VICT);
        act("`WYou trip $N `Wand $N `Wgoes down!`w",ch,NULL,victim,TO_CHAR);
        act("`W$n`W trips $N`W, sending $M to the ground.`w",ch,NULL,victim,TO_NOTVICT);
        check_improve(ch,gsn_trip,TRUE,1);

        WAIT_STATE(victim,2 * PULSE_VIOLENCE);
        WAIT_STATE(ch,skill_table[gsn_trip].beats);
        victim->position = POS_RESTING;
/*      damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_trip,
            DAM_BASH);
*/
/* new damage attempt by Rahl */
        damage(ch,victim,number_range(ch->level/2,ch->level+3*victim->size),
gsn_trip, DAM_BASH, TRUE);
    }
    else
    {
        damage(ch,victim,0,gsn_trip,DAM_BASH, TRUE);
        WAIT_STATE(ch,skill_table[gsn_trip].beats*2/3);
        check_improve(ch,gsn_trip,FALSE,1);
    } 
}



void do_kill( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Kill whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( !IS_NPC(victim) )
    {
        if (
        ( !chaos && 
        ( !IS_SET(victim->act, PLR_KILLER) || !IS_SET(ch->act,
        PLR_KILLER) ) ) 
        && 
        ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) ||
        victim->pcdata->bounty <= 0 ) ) )
        {
            send_to_char( "You can only kill other player killers.\n\r", ch );
            return;
        }
    }

    if ( victim == ch )
    {
        send_to_char( "You hit yourself.  Ouch!\n\r", ch );
        multi_hit( ch, ch, TYPE_UNDEFINED );
        return;
    }

    if ( is_safe( ch, victim ) )
        return;

    if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
        act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( ch->position == POS_FIGHTING )
    {
        send_to_char( "You do the best you can!\n\r", ch );
        return;
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    check_killer( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}



void do_murde( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to MURDER, spell it out.\n\r", ch );
    return;
}



void do_murder( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Murder whom?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)))
    {
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "Suicide is a mortal sin.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( is_safe( ch, victim ) )
    {
        buffer_free( buf );
        return;
    }

    if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
        act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
        buffer_free( buf );
        return;
    }

    if ( ch->position == POS_FIGHTING )
    {
        send_to_char( "You do the best you can!\n\r", ch );
        buffer_free( buf );
        return;
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    if (IS_NPC(ch))
        bprintf(buf, "`YHelp! I am being attacked by %s!`w",
            ch->short_descr);
    else
        bprintf( buf, "`YHelp!  I am being attacked by %s!`w", 
            ch->name );
    do_yell( victim, buf->data );
    check_killer( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    buffer_free( buf );
    return;
}



void do_backstab( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Backstab whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "How can you sneak up on yourself?\n\r", ch );
        return;
    }

    if ( is_safe( ch, victim ) )
      return;
    if ( !IS_NPC(victim) )
    {
        if (( !chaos &&
        ( !IS_SET(victim->act, PLR_KILLER) || !IS_SET(ch->act,
        PLR_KILLER) ) )
        &&
        ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) ||
        victim->pcdata->bounty <= 0 ) ) )
        {
            send_to_char( "You can only kill other player killers.\n\r", ch );
            return;
        }
    }

    if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
        send_to_char( "You need to wield a weapon to backstab.\n\r", ch );
        return;
    }

    if ( victim->fighting != NULL )
    {
        send_to_char( "You can't backstab a fighting person.\n\r", ch );
        return;
    }

    if ( victim->hit < victim->max_hit && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        act( "$N is hurt and suspicious ... you can't sneak up.",
            ch, NULL, victim, TO_CHAR );
        return;
    }
    
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_backstab].beats );
    if ( !IS_AWAKE(victim)
    ||   IS_NPC(ch)
    ||   number_percent( ) < ch->pcdata->learned[gsn_backstab] )
    {
        check_improve(ch,gsn_backstab,TRUE,1);
        multi_hit( ch, victim, gsn_backstab );
    }
    else
    {
        check_improve(ch,gsn_backstab,FALSE,1);
        damage( ch, victim, 0, gsn_backstab,DAM_NONE, TRUE );
    }

    return;
}

/* circle code added by Rahl */
void do_circle( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA  *victim;
    OBJ_DATA *obj;

    one_argument( argument, arg );
 
    if ( arg[0] == '\0' )
    {
        if ( ch->fighting != NULL )
            victim = ch->fighting;
        else
        {
            send_to_char( "Circle whom?\n\r", ch );
            return;
        }
    }

    else if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( is_safe( ch, victim ) )
        return;

    /*
     * stuff below removed by Rahl. See comments above in mob_hit
     */
/*
    if ( !IS_NPC( victim ) )
    {
        if (( !chaos &&
        ( !IS_SET(victim->act, PLR_KILLER) || !IS_SET(ch->act,
        PLR_KILLER) ) )
        &&
        ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) ||
        victim->pcdata->bounty <= 0 ) ) )
        {
            send_to_char( "You can only kill other player killers.\n\r",
                ch );
            return;
        }
    }
*/
 
   if ( IS_NPC(victim) && victim->fighting != NULL && !is_same_group( ch,
        victim->fighting) )
    {
        send_to_char( "Kill stealing is not permitted.\n\r", ch );
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
    {
        send_to_char( "You need to wield a weapon to circle.\n\r", ch );
        return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
        send_to_char( "You must be fighting in order to circle.\n\r", ch );
        return;
    }

    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_circle].beats );
    act( "`WYou circle around behind $N`W.`w", ch, NULL, victim, TO_CHAR );
    act( "`W$n `Wcircles around behind $N`W.`w", ch, NULL, victim, TO_ROOM );
    act( "`W$n `Wcircles around behind you!`w", ch, NULL, victim, TO_VICT ); 
    if ( !IS_AWAKE( victim )
        || IS_NPC( ch ) 
        || number_percent( )  < ch->pcdata->learned[gsn_circle] )
    {
        check_improve( ch, gsn_circle, TRUE, 1 );
        multi_hit( ch, victim, gsn_circle );
    }
    else
    {
        check_improve( ch, gsn_circle, FALSE, 1 );
        damage( ch, victim, 0, gsn_circle, DAM_NONE, TRUE );
    }

    return;
}

void do_flee( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim;
    int attempt;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( ( victim = ch->fighting ) == NULL )
    {
        if ( ch->position == POS_FIGHTING )
            ch->position = POS_STANDING;
        send_to_char( "You aren't fighting anyone.\n\r", ch );
        buffer_free( buf );
        return;
    }

    was_in = ch->in_room;
    for ( attempt = 0; attempt < 6; attempt++ )
    {
        EXIT_DATA *pexit;
        int door;

        door = number_door( );
        if ( ( pexit = was_in->exit[door] ) == 0
        ||   pexit->u1.to_room == NULL
        ||   IS_SET(pexit->exit_info, EX_CLOSED)
        || ( IS_NPC(ch)
        &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
            continue;

        move_char( ch, door, FALSE );
        if ( ( now_in = ch->in_room ) == was_in )
            continue;

        ch->in_room = was_in;
        act( "$n has fled!", ch, NULL, NULL, TO_ROOM );
        ch->in_room = now_in;

        stop_fighting( ch, TRUE );
        buffer_free( buf );
        return;
    }

    send_to_char( "`RPANIC! You couldn't escape!\n\r`w", ch );
    buffer_free( buf );
    return;
}



void do_rescue( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *fch;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Rescue whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "What about fleeing instead?\n\r", ch );
        return;
    }

    if ( !is_same_group(ch,victim))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( !IS_NPC(ch) && IS_NPC(victim) )
    {
        send_to_char( "Doesn't need your help!\n\r", ch );
        return;
    }

    if ( ch->fighting == victim )
    {
        send_to_char( "Too late.\n\r", ch );
        return;
    }

    if ( ( fch = victim->fighting ) == NULL )
    {
        send_to_char( "That person is not fighting right now.\n\r", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_rescue].beats );
    if ( !IS_NPC(ch) && number_percent( ) > ch->pcdata->learned[gsn_rescue] )
    {
        send_to_char( "`BYou fail the rescue.\n\r`w", ch );
        check_improve(ch,gsn_rescue,FALSE,1);
        return;
    }

    act( "`WYou rescue $N`W!`w",  ch, NULL, victim, TO_CHAR    );
    act( "`W$n `Wrescues you!`w", ch, NULL, victim, TO_VICT    );
    act( "`W$n `Wrescues $N`W!`w",  ch, NULL, victim, TO_NOTVICT );
    check_improve(ch,gsn_rescue,TRUE,1);

    stop_fighting( fch, FALSE );
    stop_fighting( victim, FALSE );

    check_killer( ch, fch );
    set_fighting( ch, fch );
    set_fighting( fch, ch );
    return;
}



void do_kick( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;

    if ( !IS_NPC(ch)
    &&   ch->level < skill_table[gsn_kick].skill_level[ch->ch_class] )
    {
        send_to_char(
            "You better leave the martial arts to fighters.\n\r", ch );
        return;
    }

    if (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK))
        return;

    if ( ( victim = ch->fighting ) == NULL )
    {
        send_to_char( "You aren't fighting anyone.\n\r", ch );
        return;
    }

    if ( !IS_NPC(victim) )
    {
        if (( !chaos &&
        ( !IS_SET(victim->act, PLR_KILLER) || !IS_SET(ch->act,
        PLR_KILLER) ) )
        &&
        ( !chaos && ( !IS_SET( ch->act, PLR_BOUNTY_HUNTER ) ||
        victim->pcdata->bounty <= 0 ) ) )
        {
            send_to_char( "You can only kill other player killers.\n\r", ch );
            return;
        }
    }

    WAIT_STATE( ch, skill_table[gsn_kick].beats );
    if ( IS_NPC(ch) || number_percent( ) < ch->pcdata->learned[gsn_kick] )
    {
/*      damage( ch, victim, number_range( 1, ch->level ), gsn_kick,DAM_BASH );
*/
/* new damage attempt by Rahl */
        damage( ch, victim, number_range( ch->level/2, 2*ch->level ), 
            gsn_kick, DAM_BASH, TRUE );
        check_improve(ch,gsn_kick,TRUE,1);
    }
    else
    {
        damage( ch, victim, 0, gsn_kick,DAM_BASH, TRUE );
        check_improve(ch,gsn_kick,FALSE,1);
    }

    return;
}




void do_disarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance,hth,ch_weapon,vict_weapon,ch_vict_weapon;

    hth = 0;

    if ((chance = get_skill(ch,gsn_disarm)) == 0)
    {
        send_to_char( "You don't know how to disarm opponents.\n\r", ch );
        return;
    }

    if ( get_eq_char( ch, WEAR_WIELD ) == NULL 
    &&   ((hth = get_skill(ch,gsn_hand_to_hand)) == 0
    ||    (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_DISARM))))
    {
        send_to_char( "You must wield a weapon to disarm.\n\r", ch );
        return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
        send_to_char( "You aren't fighting anyone.\n\r", ch );
        return;
    }

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
    {
        send_to_char( "Your opponent is not wielding a weapon.\n\r", ch );
        return;
    }

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch,get_weapon_sn(ch));
    vict_weapon = get_weapon_skill(victim,get_weapon_sn(victim));
    ch_vict_weapon = get_weapon_skill(ch,get_weapon_sn(victim));

    /* modifiers */

    /* skill */
    if ( get_eq_char(ch,WEAR_WIELD) == NULL)
        chance = chance * hth/150;
    else
        chance = chance * ch_weapon/100;

    chance += (ch_vict_weapon/2 - vict_weapon) / 2; 

    /* dex vs. strength */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_STR);

    /* level */
    chance += (ch->level - victim->level) * 2;
 
    /* and now the attack */
    if (number_percent() < chance)
    {
        WAIT_STATE( ch, skill_table[gsn_disarm].beats );
        disarm( ch, victim );
        check_improve(ch,gsn_disarm,TRUE,1);
    }
    else
    {
        WAIT_STATE(ch,skill_table[gsn_disarm].beats);
        act("`BYou fail to disarm $N`B.`w",ch,NULL,victim,TO_CHAR);
        act("`B$n `Btries to disarm you, but fails.`w",ch,NULL,victim,TO_VICT);
        act("`B$n `Btries to disarm $N`B, but fails.`w",ch,NULL,victim,TO_NOTVICT);
        check_improve(ch,gsn_disarm,FALSE,1);
    }
    return;
}



void do_sla( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}


void do_slay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    if ( arg[0] == '\0' ) {
        /* send_to_char( "Slay whom?\n\r", ch ); */
        send_to_char( "Syntax: [Char] [Type]\n\r", ch );
        send_to_char( "Types: Skin, Immolate, Demon, Shatter, Slit, Deheart, Pounce.\n\r", ch);
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\n\r", ch );
        return;
    }

    if ( !IS_NPC(victim) && char_getImmRank( victim ) >= char_getImmRank(ch) ) {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "skin" ) ) {
        act( "`RYou rip the flesh from $N `Rand send his soul to the fiery "
             "depths of hell.`w", ch, NULL, victim, TO_CHAR );
        act( "`RYour flesh has been torn from your bones and your bodyless "
             "soul now watches your bones incenerate in the fires of hell.`w", 
             ch, NULL, victim, TO_VICT );
        act( "`R$n `Rrips the flesh off of $N`R, releasing his soul into " 
             "the fiery depths of hell.`w", ch, NULL, victim, TO_NOTVICT );
    } else if ( !str_cmp( arg2, "deheart" ) ) {
        act( "`RYou rip through $N`R's chest and pull out $S beating heart" 
             " in your hand.`w", ch, NULL, victim, TO_CHAR );
        act( "`RYou feel a sharp pain as $n`R rips into your chest and pulls "
             "out your beating heart in $S hand.`w", ch, NULL, victim,
                TO_VICT );
        act( "`RSpecks of blood hit your face as $n `Rrips through $N`R's "
             "chest pulling out $S beating heart.`w", ch, NULL, victim,
             TO_NOTVICT );
    } else if ( !str_cmp( arg2, "immolate" ) ) {
        act( "`RYour fireball turns $N`R into a blazing inferno.`w",  ch, 
            NULL, victim, TO_CHAR    );
      act( "`R$n`R releases a searing fireball in your direction.`w", ch,
            NULL, victim, TO_VICT    );
      act( "`R$n`R points at $N`R, who bursts into a flaming inferno.`w",
           ch, NULL, victim, TO_NOTVICT );
    } else if ( !str_cmp( arg2, "shatter" ) ) {
      act( "`CYou freeze $N `Cwith a glance and shatter the frozen corpse "
           "into tiny shards.`w",  ch, NULL, victim, TO_CHAR );
      act( "`C$n `Cfreezes you with a glance and shatters your frozen body "
           "into tiny shards.`w", ch, NULL, victim, TO_VICT );
      act( "`C$n `Cfreezes $N with a glance and shatters the frozen body "
           "into tiny shards.`w",  ch, NULL, victim, TO_NOTVICT );
    } else if ( !str_cmp( arg2, "demon" ) ) {
      act( "`YYou gesture, and a slavering demon appears.  With a horrible "
           "grin, the foul creature turns on $N`Y, who screams in panic "
           "before being eaten alive.`w",  ch, NULL, victim, TO_CHAR );
      act( "`Y$n`R gestures, and a slavering demon appears.  The foul creature "
           "turns on you with a horrible grin. You scream in panic " 
           "before being eaten alive.`w",  ch, NULL, victim, TO_VICT );
      act( "`Y$n `Ygestures, and a slavering demon appears.  With a horrible"
           " grin, the foul creature turns on $N`Y, who screams in panic"
           " before being eaten alive.`w",  ch, NULL, victim, TO_NOTVICT );
    } else if ( !str_cmp( arg2, "pounce" ) ) {
      act( "`mLeaping upon $N `mwith bared fangs, you tear open $S throat "
           "and toss the corpse to the ground...`w",  ch, NULL, victim,
           TO_CHAR );
      act( "`mIn a heartbeat, $n `mrips $s fangs through your throat! Your "
           "blood sprays and pours to the ground as your life ends...`w",
           ch, NULL, victim, TO_VICT );
      act( "`mLeaping suddenly, $n`m sinks $s fangs into $N`m's throat. As "
           "blood sprays and gushes to the ground, $n `mtosses $N`m's dying "
           "body away.`w",  ch, NULL, victim, TO_NOTVICT );
    } else if ( !str_cmp( arg2, "slit" )) {
      act( "`RYou calmly slit $N`R's throat.`w", ch, NULL, victim, TO_CHAR );
      act( "`R$n `Rreaches out with a clawed finger and calmly slits your"
           " throat.`w", ch, NULL, victim, TO_VICT );
      act( "`RA claw extends from $n`R's hand as $e calmly slits $N`R's"
           " throat.`w", ch, NULL, victim, TO_NOTVICT );
    } else {
        act( "`RYou slay $M in cold blood!`w",  ch, NULL, victim, TO_CHAR );
        act( "`R$n slays you in cold blood!`w", ch, NULL, victim, TO_VICT );
        act( "`R$n slays $N`R in cold blood!`w",  ch, NULL, victim, TO_NOTVICT );
    }

    if ( !IS_NPC(victim) ) {
        victim->pcdata->nemesis=NULL;
    }

    if (chaos) {
        chaos_kill( victim );
    }

    if (!chaos) {
        raw_kill( victim );
    }

    return;
}


/* Push added by Rahl */
void do_push( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    char *arg;
    EXIT_DATA *pExit;
    int door;
    char dir[MAX_INPUT_LENGTH];
    CHAR_DATA *target;
    int chance;
    char victim[MAX_INPUT_LENGTH];

    arg = one_argument( argument, victim );
    arg = one_argument( arg, dir );

    if ( victim[0] == '\0' )
    {
        send_to_char( "Push whom?\n\r", ch );
        return;
    }

    if ( dir[0] == '\0' )
    {
        send_to_char( "Push where?\n\r", ch );
        return;
    }

    else if (!str_cmp(dir, "n") || !str_cmp(dir, "north")) door = 0;
    else if (!str_cmp(dir, "e") || !str_cmp(dir, "east")) door = 1;
    else if (!str_cmp(dir, "s") || !str_cmp(dir, "south")) door = 2;
    else if (!str_cmp(dir, "w") || !str_cmp(dir, "west")) door = 3;
    else if (!str_cmp(dir, "u") || !str_cmp(dir, "up")) door = 4;
    else if (!str_cmp(dir, "d") || !str_cmp(dir, "down")) door = 5;

    else
    {
        send_to_char( "Push where?\n\r", ch );
        return;
    }

    /* not in safe rooms */
    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
    {
        send_to_char( "Don't even think about it in here!\n\r", ch );
        return;
    }

    /* not while fighting */
    if (ch->position == POS_FIGHTING)
    {
        send_to_char( "Not while fighting!\n\r", ch );
        return;
    }

    target = get_char_room( ch, victim );

    if (target == NULL)
    {
        send_to_char( "Try pushing someone in the same room.\n\r", ch );
        return;
    }

    if (target == ch)
    {
        send_to_char( "You don't need to push yourself around. Just walk.\n\r", ch );
        return;
    }

    if (target->fighting != NULL)
    {
        send_to_char( "That's not nice!\n\r", ch );
        return;
    }

   if ( IS_AFFECTED( target, AFF_WEB ) )
   {
        send_to_char( "They seem to be stuck in someone's webs.\n\r", ch );
        return;
    }

    in_room = ch->in_room;

    /* check if the exit exists */
    if ((pExit = in_room->exit[door]) == NULL
       || (to_room = pExit->u1.to_room) == NULL
       || !can_see_room(ch,pExit->u1.to_room)
       || IS_SET(pExit->exit_info, EX_CLOSED))
    {
        send_to_char( "You cannot push your victim in that direction!\n\r", ch );
        act( "$n slams $N against the wall!", ch, NULL, target, TO_ROOM );
        return;
    }

    if (in_room == to_room)
    {
        return;
    }

    if ( IS_SET( to_room->room_flags, ROOM_SAFE ) )
    {
        send_to_char( "Don't even think about pushing them in there.\n\r", ch );
        return;
    }

    chance = get_skill( ch, gsn_push );

    /* can't use if haven't practiced or if don't have */
    if ( chance == 0 ) 
    {
        send_to_char( "Push? What's that?\n\r", ch );
        return;
    }

    if (target->size < ch->size)
        chance += (ch->size - target->size)*10;
    else
        chance += (ch->size - target->size)*5;

    chance += get_curr_stat(ch,STAT_STR);
    chance += ch->level - target->level;

    if ( IS_SET( target->act, ACT_SENTINEL ) )
        chance -= 10;

    if (number_percent () < chance )
    {
        act( "You push $N out of the room.", ch, NULL, target, TO_CHAR );
        act( "$n pushes you out of the room.", ch, NULL, target, TO_VICT);
        act( "$n pushes $N out of the room.", ch, NULL, target, TO_NOTVICT);
        check_improve( ch, gsn_push, TRUE, 1 );
        WAIT_STATE( ch, skill_table[gsn_push].beats );
        char_from_room(target);
        char_to_room(target,to_room);
        do_look( target, "auto" );
    }
    else
    {
        check_improve( ch, gsn_push, FALSE, 1 );
        if (IS_NPC( target ))
           damage( ch, target, 0, gsn_push, DAM_NONE, TRUE );
        else
           send_to_char( "You are too weak.\n\r", ch );
        WAIT_STATE( ch, skill_table[gsn_push].beats*3/2 );
    }

    return;
}

void chaos_log( CHAR_DATA *ch, char *argument )
{
    append_file( ch, CHAOS_FILE , argument );
    return;
}



/* grip added by Rahl */
void do_grip( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    AFFECT_DATA af;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Tighten your grip on what? Yourself!?\n\r", ch );
        return;
    }

    if ( !str_prefix( arg, "primary" ) )
    {
        obj = get_eq_char( ch, WEAR_WIELD );
    }
    else if ( !str_prefix( arg, "secondary" ) )
    {
        obj = get_eq_char( ch, WEAR_SECOND_WIELD );
    }
    else
    {
        obj = NULL;
    }

    if ( obj != NULL )
    {
        if ( IS_OBJ_STAT( obj, ITEM_NOREMOVE ) )
        {
            send_to_char( "You already have a good grip on your weapon.\n\r", ch );
            return;
        }

        if ( number_percent( ) < ch->pcdata->learned[gsn_grip] )
        {

                WAIT_STATE( ch, 20 );
        
                send_to_char( "You tighten your grip on your weapon.\n\r", ch );
                check_improve( ch, gsn_grip, TRUE, 1 );

                af.where        = TO_OBJECT;    
                af.type         = gsn_grip;
                af.level        = ch->level;
                af.duration     = ch->level;
                af.location     = 0;
                af.modifier     = 0;
                af.bitvector    = ITEM_NOREMOVE;
                affect_to_obj( obj, &af );       
        }
        else
        {
            send_to_char( "You can't quite get a good grip on it.\n\r", ch );
            check_improve( ch, gsn_grip, FALSE, 1 );
        } 
    }
    else
    {
        send_to_char( "Tighten your grip on what? Yourself!?\n\r", ch );
    }
    return;
}

void do_whirlwind( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *pChar;
   CHAR_DATA *pChar_next;
   OBJ_DATA *wield;
   bool found = FALSE;
   
   if (    !IS_NPC( ch ) 
        && ch->level < skill_table[gsn_whirlwind].skill_level[ch->ch_class] )
   {
      send_to_char( "You don't know how to do that.\n\r", ch );
      return;
   }

   if ( get_skill( ch, gsn_whirlwind ) == 0 )
   {
        send_to_char( "You spin around... and around... and only manage to make yourself dizzy.\n\rPerhaps you'd better practice more.\n\r", ch );
        return;
   } 

   if ( ( wield = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "You need to wield a weapon first...\n\r", ch );
      return;
   }
   
   act( "$n holds $p firmly, and starts spinning round...", ch, wield, NULL, 
        TO_ROOM );
   act( "You hold $p firmly, and start spinning round...",  ch, wield,
        NULL, TO_CHAR );
   
   pChar_next = NULL;   
   for ( pChar = ch->in_room->people; pChar; pChar = pChar_next )
   {
      pChar_next = pChar->next_in_room;
      if ( IS_NPC( pChar ) )
      {
         if ( pChar->fighting != NULL && !is_same_group(ch,pChar->fighting))
         {
             send_to_char("Kill stealing is not permitted.\n\r",ch);
             continue;
         }       
         found = TRUE;
         act( "$n turns towards YOU!", ch, NULL, pChar, TO_VICT    );
         one_hit( ch, pChar, gsn_whirlwind );
      }
   }
   
   if ( !found )
   {
      act( "$n looks dizzy, and a tiny bit embarassed.", ch, NULL, NULL,
        TO_ROOM );
      act( "You feel dizzy, and a tiny bit embarassed.", ch, NULL, NULL,
        TO_CHAR );
   }
   
   WAIT_STATE( ch, skill_table[gsn_whirlwind].beats );
   
   if ( !found && number_percent() < 25 )
   {
      act( "$n loses $s balance and falls into a heap.",  ch, NULL, NULL,
        TO_ROOM );
      act( "You lose your balance and fall into a heap.", ch, NULL, NULL,
        TO_CHAR );
      ch->position = POS_STUNNED;
   }
   
   return;
}      

bool check_block( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;
    OBJ_DATA *obj;

    if ( !IS_AWAKE(victim) )
        return FALSE;

    chance = get_skill(victim,gsn_shield_block) / 5 + 3;

    if ( ( obj = get_eq_char( victim, WEAR_SHIELD ) ) == NULL )
        return FALSE;

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "`BYou block $n`B's attack with your shield.`w",  ch, NULL,
         victim, TO_VICT );
    act( "`B$N `Bblocks your attack with $S shield.`w", ch, NULL, victim,
        TO_CHAR );

    /* 25% chance of damaging your shield */
/*
    if ( number_percent() <= 25 )
        obj->condition -= 1;
*/

    check_improve(victim,gsn_shield_block,TRUE,6);
    return TRUE;
}

bool check_blink( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;

    if ( !IS_AWAKE( victim ) )
        return FALSE;

    if ( !IS_AFFECTED2( victim, AFF_BLINK ) )
        return FALSE;

    if ( !IS_NPC( victim ) )
        chance = victim->pcdata->learned[skill_lookup( "blink" )] / 2;
    else
        chance = UMIN( 30, victim->level );

    chance += ( victim->level - ch->level );

    if ( number_percent() >= chance )
        return FALSE;

    act( "`BYou blink out of existence, avoiding $n`B's attack.`w",
        ch, NULL, victim, TO_VICT );
    act( "`B$N blinks out of existence, avoiding your attack.`w",
        ch, NULL, victim, TO_CHAR );

    /* shouldn't need a check_improve() in here, as it's a spell */
    return TRUE;
}
