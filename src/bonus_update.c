///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////



#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "interp.h"

// in merc.h
//#define PULSE_BONUS   ( 1800 * PULSE_PER_SECOND ) // 30 mins 
// add bool fBonusMob to struct char_data
// add int  bonusPoints to char_data

// add the code below to update_handler in update.c
// as well as static  int pulse_bonus;
/*
   if ( --pulse_bonus <= 0 )
    {
        pulse_bonus = PULSE_BONUS;
        bonus_update();
    }
*/

/*
  add the below to bool damage() (and damage_old/new, if you have it
  right before the auto stuff
*/
       /* bonus stuff */
/*
        if ( !IS_NPC( ch ) && IS_NPC( victim ) ) {
            if ( victim->fBonusMob ) {
				int bonusPoint_gain = victim->level / 10;
				if ( bonusPoint_gain < 1 )
   					bonusPoint_gain = 1;
                send_to_char( "`GCongratulations! You have found a "
                    "bonus mob!`w\n\r", ch );
                sprintf( buf, "`GYou gain %d bonus points.`w\n\r",
                    bonusPoint_gain );
                send_to_char( buf, ch );
                ch->tpoints += bonusPoint_gain;
            }
        }
*/


#define MAX_MOB_LEVEL  120
#define BONUS_RETRY_COUNT 200 

CHAR_DATA *get_random_mob( void ) {
	CHAR_DATA *victim;
	int chance;

    for (victim = char_list; victim != NULL; victim = victim->next) {
		chance = number_percent();

        if ( !IS_NPC( victim ) ) 
			continue;

        if ( !IS_SET(victim->imm_flags, IMM_SUMMON)
            && !IS_SET(victim->act, ACT_NO_KILL)
            && !IS_SET(victim->act, ACT_TRAIN)
            && !IS_SET(victim->act, ACT_PRACTICE)
            && victim->pIndexData != NULL
            && victim->pIndexData->pShop == NULL
            && !IS_SET(victim->act, ACT_PET)
            && !IS_SET(victim->affected_by, AFF_CHARM)
            && !IS_SET(victim->act, ACT_IS_HEALER )
			&& ( chance < 4 ) ) 
				break;
    }
	
	if ( victim != NULL )
		return victim;
	else 
		return NULL;
}

void bonus_update( void ) {
	int counter;
	int i;
    CHAR_DATA *bonusMob;
	char buf[MAX_STRING_LENGTH];

    for ( bonusMob = char_list; bonusMob != NULL; bonusMob = bonusMob->next ) {
		if ( bonusMob->fBonusMob ) {
			bonusMob->fBonusMob = FALSE; 
		}
	}

	for ( counter  = 0; counter <= MAX_MOB_LEVEL; counter++ ) {
		for ( i = 0; i <= BONUS_RETRY_COUNT; i++ ) {
			bonusMob = get_random_mob();

			if ( bonusMob == NULL ) {
				continue;
			} else if ( bonusMob->level != counter ) {
				continue;
			} else {
				if ( DEBUG ) {
					sprintf( buf, "B_UPDATE: %s (v:%d r:%d) set as bonus mob.",
						bonusMob->short_descr, bonusMob->pIndexData->vnum,
						bonusMob->in_room->vnum );
					log_string( buf );
				}
				bonusMob->fBonusMob = TRUE;
				break;
			} // if
		} // for
	} // for
} 
