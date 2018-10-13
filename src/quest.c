///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2018 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/****************************************************************************
*  Automated Quest code written by Vassago of MOONGATE, moongate.ams.com    *
*  4000. Copyright (c) 1996 Ryan Addams, All Rights Reserved. Use of this   *
*  code is allowed provided you add a credit line to the effect of:         *
*  "Quest Code (c) 1996 Ryan Addams" to your logon screen with the rest     *
*  of the standard diku/rom credits. If you use this or a modified version  *
*  of this code, let me know via email: moongate@moongate.ams.com. Further  *
*  updates will be posted to the rom mailing list. If you'd like to get     *
*  the latest version of quest.c, please send a request to the above add-   *
*  ress. Quest Code v2.03. Please do not remove this notice from this file. *
****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "interp.h"

AFFECT_DATA *new_affect  (void);

/* Object vnums for Quest Rewards */

#define QUEST_ITEM1 90
#define QUEST_ITEM2 91
#define QUEST_ITEM3 92
#define QUEST_ITEM4 93
#define QUEST_ITEM5 94

/* Object vnums for object quest 'tokens'. In Moongate, the tokens are
   things like 'the Shield of Moongate', 'the Sceptre of Moongate'. These
   items are worthless and have the rot-death flag, as they are placed
   into the world when a player receives an object quest. */

#define QUEST_OBJQUEST1 95
#define QUEST_OBJQUEST2 96
#define QUEST_OBJQUEST3 97
#define QUEST_OBJQUEST4 98
#define QUEST_OBJQUEST5 99

/* Local functions */

void generate_quest     ( CHAR_DATA *ch, CHAR_DATA *questman );
void quest_update       ( void );
bool quest_level_diff   ( int clevel, int mlevel);
bool chance             ( int num );
ROOM_INDEX_DATA         *find_location( CHAR_DATA *ch, char *arg );

/* CHANCE function. I use this everywhere in my code, very handy :> */

bool chance(int num)
{
    if (number_range(1,100) <= num) return TRUE;
    else return FALSE;
}

/* The main quest function */

void do_quest(CHAR_DATA *ch, char *argument) {
    CHAR_DATA *questman;
    OBJ_DATA *obj=NULL, *obj_next;
    OBJ_INDEX_DATA *questinfoobj;
    MOB_INDEX_DATA *questinfo;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if( IS_NPC( ch ) ) {
        buffer_free( buf );
        return;
    }

    if( !( arg3[0] == '\0') ) {
        buffer_free( buf );
        return;
    }

    if( IS_SET( ch->act, PLR_QUESTOR ) && ch->countdown == 0 ) {
        REMOVE_BIT(ch->act,PLR_QUESTOR);
	}

    if (arg1[0] == '\0') {
        send_to_char("QUEST commands: POINTS INFO TIME REQUEST COMPLETE "
            "GIVEUP.\n\r",ch);
        send_to_char("For more information, type 'HELP QUEST'.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg1, "giveup" ) ) {
        if ( IS_SET( ch->act, PLR_QUESTOR ) ) {
            REMOVE_BIT( ch->act, PLR_QUESTOR );
            ch->nextquest = 10;
            ch->countdown = 0;
            ch->questmob = 0;
            ch->questgiver = NULL;
            send_to_char("You have given up on your quest!\n\r",ch);
            send_to_char("You must wait 10 minutes before you may quest"
                " again.\n\r",ch);
            buffer_free( buf );
            return;
        } else {
            send_to_char("You're not on a quest.\n\r",ch);
            buffer_free( buf );
            return;
        }
    } // giveup
 
    if ( !str_prefix( arg1, "info" ) ) {
        if ( IS_SET( ch->act, PLR_QUESTOR ) ) {
            if (ch->questmob == -1 && ch->questgiver->short_descr != NULL) {
                bprintf(buf, "Your quest is ALMOST complete!\n\rGet back"
                    " to %s before your time runs out!\n\r",
                    ch->questgiver->short_descr);
                send_to_char(buf->data, ch);
            } else if ( ch->questobj > 0 ) {
                questinfoobj = get_obj_index( ch->questobj );
                if ( questinfoobj != NULL ) {
                    /* was name */
                    bprintf(buf, "You are on a quest to recover the fabled"
                        " %s!\n\r", questinfoobj->short_descr);
                    send_to_char(buf->data, ch);
                } else {
                    send_to_char("You aren't currently on a quest.\n\r",
                        ch);
				}
                buffer_free( buf );
                return;
            } else if ( ch->questmob > 0 ) {
                questinfo = get_mob_index(ch->questmob);
                if ( questinfo != NULL ) {
                    bprintf(buf, "You are on a quest to slay the dreaded"
                        " %s!\n\r", questinfo->short_descr);
                    send_to_char(buf->data, ch);
                } else {
					send_to_char("You aren't currently on a quest.\n\r",
                        ch);
				}
                buffer_free( buf );
                return;
            }
        } else {
            send_to_char( "You aren't currently on a quest.\n\r", ch );
		}
        buffer_free( buf );
        return;
    } // info

    if ( !str_prefix( arg1, "points" ) ) {
        bprintf( buf, "You have %d quest points.\n\r", ch->questpoints );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    if ( !str_prefix( arg1, "time" ) ) {
        if ( !IS_SET( ch->act, PLR_QUESTOR ) ) {
            send_to_char( "You aren't currently on a quest.\n\r", ch );
            if ( ch->nextquest > 1 ) {
                bprintf( buf, "There are %d minutes remaining until you can" 
                    " go on another quest.\n\r", ch->nextquest );
                send_to_char(buf->data, ch);
            } else if ( ch->nextquest == 1 ) {
                bprintf( buf, "There is less than a minute remaining until"
                    " you can go on another quest.\n\r" );
                send_to_char( buf->data, ch );
            }
        } else if ( ch->countdown > 0 ) {
            bprintf( buf, "Time left for current quest: %d\n\r", 
                ch->countdown );
            send_to_char( buf->data, ch );
        }
        buffer_free( buf );
        return;
    } // time

/* Checks for a character in the room with spec_questmaster set. This special
   procedure must be defined in special.c. You could instead use an
   ACT_QUESTMASTER flag instead of a special procedure. */

    for ( questman = ch->in_room->people; questman != NULL; questman = questman->next_in_room) {
        if ( !IS_NPC( questman ) ) {
			continue;
		}

        if ( questman->spec_fun == spec_lookup( "spec_questmaster" ) ) {
			break;
		}
    }

    if ( questman == NULL || questman->spec_fun != spec_lookup( "spec_questmaster" ) ) {
        send_to_char("You can't do that here.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( questman->fighting != NULL) {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        buffer_free( buf );
        return;
    }

    ch->questgiver = questman;

    if ( !str_prefix(arg1, "request" ) ) {
        act( "$n asks $N for a quest.", ch, NULL, questman, TO_ROOM);
        act ("You ask $N for a quest.",ch, NULL, questman, TO_CHAR);

        if ( IS_SET( ch->act, PLR_QUESTOR ) ) {
            bprintf(buf, "But you're already on a quest!");
            do_say(questman, buf->data);
            buffer_free( buf );
            return;
        }

        if ( ch->nextquest > 0 ) {
            bprintf(buf, "You're very brave, %s, but let someone else have" 
                " a chance.", ch->name);
            do_say(questman, buf->data);
            bprintf(buf, "Come back later.");
            do_say(questman, buf->data);
            buffer_free( buf );
            return;
        }

        if ( IS_IMMORTAL( ch ) ) {
            bprintf( buf, "I humble myself before you, %s, but I'm afraid"
                " I have nothing worthy of your attention.", ch->name );
            do_say( questman, buf->data );
            buffer_free( buf );
            return;
        }

        bprintf(buf, "Thank you, brave %s!",ch->name);
        do_say( questman, buf->data );
        ch->questmob = 0;
        ch->questobj = 0;

        generate_quest(ch, questman);

        if ( ( ch->questmob > 0 ) || ( ch->questobj > 0 ) ) {
            ch->countdown = number_range( 10, 30 );
            SET_BIT( ch->act, PLR_QUESTOR );
            bprintf( buf, "You have %d minutes to complete this quest.",
                ch->countdown );
            do_say( questman, buf->data );
            bprintf( buf, "May the gods go with you!" );
            do_say( questman, buf->data );
        }

        buffer_free( buf );
        return;
		// end of "request"
    } else if ( !str_prefix( arg1, "complete" ) ) {
        act( "$n informs $N $e has completed $s quest.", ch, NULL, questman, 
            TO_ROOM);
        act ("You inform $N you have completed $s quest.",ch, NULL, questman, 
            TO_CHAR);
        if ( ch->questgiver != questman ) {
            bprintf(buf, "I did not send you on a quest. Perhaps you're"
                " thinking of someone else.");
            do_say(questman,buf->data);
            buffer_free( buf );
            return;
        }

        if ( IS_SET( ch->act, PLR_QUESTOR ) ) {
            if ( ( ch->questmob == -1 ) && ( ch->countdown > 0 ) ) {
                int reward, pointreward, pracreward = 0;

                reward = number_range( 15, 250 );
                pointreward = number_range( 10,40 );
                
                bprintf(buf, "Congratulations on completing your quest!");
                do_say( questman,buf->data );
                bprintf( buf, "As a reward, I am giving you %d quest points"
                    " and %d gold.",pointreward,reward);
                do_say( questman, buf->data );

                if ( chance( 15 ) ) {
                    pracreward = number_range(1,6);
                    bprintf(buf, "You gain %d practices!\n\r",pracreward);
                    send_to_char(buf->data, ch);
                    ch->practice += pracreward;
                }

                REMOVE_BIT( ch->act, PLR_QUESTOR );
                ch->questgiver = NULL;
                ch->countdown = 0;
                ch->questmob = 0;
                ch->questobj = 0;
                ch->nextquest = 15;
                ch->gold += reward;
                ch->questpoints += pointreward;
        
                buffer_free( buf );
                return;
            } else if ( ( ch->questobj > 0 ) && ( ch->countdown > 0 ) ) {
                bool obj_found = FALSE;

                for ( obj = ch->carrying; obj != NULL; obj = obj_next ) {
                    obj_next = obj->next_content;

                    if ( ( obj != NULL ) && ( obj->pIndexData->vnum == ch->questobj ) ) {
                        obj_found = TRUE;
                        break;
                    }
                }

				if ( obj_found == TRUE ) {
                    int reward, pointreward, pracreward = 0;

                    reward = number_range(150,2500);
                    pointreward = number_range(10,40);
                    
                    act("You hand $p to $N.",ch, obj, questman, TO_CHAR);
                    act("$n hands $p to $N.",ch, obj, questman, TO_ROOM);

                    bprintf( buf, "Congratulations on completing your"
                        " quest!" );
                    do_say(questman,buf->data);
                    bprintf(buf,"As a reward, I am giving you %d quest"
                        " points and %d gold.", pointreward, reward);
                    do_say(questman,buf->data);

                    if ( chance( 15 ) ) {
                        pracreward = number_range(1,6);
                        bprintf(buf, "You gain %d practices!\n\r", 
                            pracreward);
                        send_to_char(buf->data, ch);
                        ch->practice += pracreward;
                    }

                    REMOVE_BIT(ch->act, PLR_QUESTOR);
                    ch->questgiver = NULL;
                    ch->countdown = 0;
                    ch->questmob = 0;
                    ch->questobj = 0;
                    ch->nextquest = 15;
                    ch->gold += reward;
                    ch->questpoints += pointreward;
                    extract_obj(obj);
        
                    buffer_free( buf );
                    return;
                } else {
                    bprintf(buf, "You haven't completed the quest yet, but"
                        " there is still time!");
                    do_say(questman, buf->data);
                    buffer_free( buf );
                    return;
                }
                buffer_free( buf );
                return;
            } else if ( ( ( ch->questmob > 0 ) || ( ch->questobj > 0 ) ) && ( ch->countdown > 0 ) ) {
                bprintf(buf, "You haven't completed the quest yet, but"
                    " there is still time!");
                do_say(questman, buf->data);
                buffer_free( buf );
                return;
            }
        }
        if ( ch->nextquest > 0 ) {
            bprintf(buf,"But you didn't complete your quest in time!");
        } else { 
            bprintf(buf, "You have to REQUEST a quest first, %s.",
                ch->name);
		}
        do_say(questman, buf->data);
        buffer_free( buf );
        return;
    }

    send_to_char("QUEST commands: POINTS INFO TIME REQUEST COMPLETE"
        " GIVEUP.\n\r",ch);
    send_to_char("For more information, type 'HELP QUEST'.\n\r",ch);
    buffer_free( buf );
    return;
}

void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman) {
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *room;
    OBJ_DATA *questitem;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    /*  Randomly selects a mob from the world mob list. If you don't
        want a mob to be selected, make sure it is immune to summon.
        Or, you could add a new mob flag called ACT_NOQUEST. The mob
        is selected for both mob and obj quests, even tho in the obj
        quest the mob is not used. This is done to assure the level
        of difficulty for the area isn't too great for the player. */

    for (victim = char_list; victim != NULL; victim = victim->next) {
        if ( !IS_NPC( victim ) ) {
			continue;
		}

        if (quest_level_diff(ch->level, victim->level) == TRUE
            && !IS_SET(victim->imm_flags, IMM_SUMMON)
            && !IS_SET(victim->act, ACT_NO_KILL)
            && !IS_SET(victim->act, ACT_TRAIN)
            && !IS_SET(victim->act, ACT_PRACTICE)
            && victim->pIndexData != NULL
            && victim->pIndexData->pShop == NULL
            && !IS_SET(victim->act, ACT_PET)
            && !IS_SET(victim->affected_by, AFF_CHARM)
            && !IS_SET(victim->act, ACT_IS_HEALER)
            && chance(30)) break;
    }

    if ( victim == NULL  ) {
        do_say(questman, "I'm sorry, but I don't have any quests for you"
            " at this time.");
        do_say(questman, "Try again later.");
        ch->nextquest = 5;
        buffer_free( buf );
        return;
    }

    if ( ( room = find_location( ch, victim->name ) ) == NULL ) {
        do_say( questman, "I'm sorry, but I don't have any quests for you"
            " at this time.");
        do_say( questman, "Try again later.");
        ch->nextquest = 5;
        buffer_free( buf );
        return;
    }

    /*  40% chance it will send the player on a 'recover item' quest. */

    if ( chance( 40 ) ) {
        int objvnum = 0;

        switch( number_range( 0,4 ) ) {
            case 0:
            	objvnum = QUEST_OBJQUEST1;
            	break;
            case 1:
            	objvnum = QUEST_OBJQUEST2;
            	break;
            case 2:
            	objvnum = QUEST_OBJQUEST3;
            	break;
            case 3:
            	objvnum = QUEST_OBJQUEST4;
            	break;
            case 4:
            	objvnum = QUEST_OBJQUEST5;
            	break;
        }

        questitem = create_object( get_obj_index(objvnum), ch->level );
        obj_to_room(questitem, room);
        ch->questobj = questitem->pIndexData->vnum;

        bprintf(buf, "Vile pilferers have stolen %s `Gfrom the royal"
            " treasury!", questitem->short_descr);
        do_say(questman, buf->data);
        do_say(questman, "My court wizardess, with her magic mirror, has"
            " pinpointed it's location.");

        /* I changed my area names so that they have just the name of the area
           and none of the level stuff. You may want to comment these next two
           lines. - Vassago */

        bprintf(buf, "%s whispers to you, 'Look in the general area of %s"
            " for %s...'\n\r", questman->short_descr,
            &room->area->name[16],room->name);
        send_to_char(buf->data,ch);

        buffer_free( buf );
        return;
    } else {
    	switch( number_range( 0, 1 ) ) {
        	case 0:
        		bprintf(buf, "An enemy of mine, %s`G, is making vile threats"
            	" against the crown.",victim->short_descr);
        		do_say(questman, buf->data);
        		do_say( questman, "This threat must be eliminated!");
        		break;
        	case 1:
        		bprintf(buf, "Shadows' most heinous criminal, %s`G, has escaped"
            	" from the dungeon!",victim->short_descr);
        		do_say(questman, buf->data);
        		bprintf(buf, "Since the escape, %s `Ghas murdered %d "
            	" civillians!",victim->short_descr,number_range(2,20));
        		do_say(questman, buf->data);
        		do_say(questman,"The penalty for this crime is death, and you"
            	" are to deliver the sentence!");
        		break;
    	}

    	if ( room->name != NULL ) {
	        bprintf(buf, "%s whispers to you, 'Seek %s`w out somewhere in the"
            " vicinity of %s`w...'\n\r",
            questman->short_descr,victim->short_descr,room->name);
    	    send_to_char(buf->data,ch);

        	/* I changed my area names so that they have just the name of the area
           	and none of the level stuff. You may want to comment these next two
           	lines. - Vassago */

        	bprintf(buf, "%s whispers to you, 'That location is in the "
            "general area of %s.'\n\r",
            questman->short_descr,&room->area->name[16]);
 	        send_to_char(buf->data,ch);
 
   		}

    	ch->questmob = victim->pIndexData->vnum;
	}
    buffer_free( buf );
    return;
}

/* Level differences to search for. Moongate has 350
   levels, so you will want to tweak these greater or
   less than statements for yourself. - Vassago */

bool quest_level_diff(int clevel, int mlevel) {
    int upper = 0;
    int lower = 0;
    
    /* was clevel+10 - changed by Rahl */
    upper = clevel + 7;
    if ( upper > 100 ) {
    	upper = 90;
	}

    lower = clevel - 2;  

    if ( ( mlevel >= lower ) && ( mlevel < upper ) ) {
    	return TRUE;
    } else {
        return FALSE;
	}
}
                
/* Called from update_handler() by pulse_area */

void quest_update( void ) {
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;

    for ( d = descriptor_list; d != NULL; d = d->next ) {
        if ( ( d->character != NULL ) && ( d->connected == CON_PLAYING ) ) {

        	ch = d->character;

        	if (ch->nextquest > 0) {
            	ch->nextquest--;

	            if (ch->nextquest == 0) {
        	        send_to_char("You may now quest again.\n\r",ch);
            	    return;
            	}
        	} else if (IS_SET(ch->act,PLR_QUESTOR)) {
            	if (--ch->countdown <= 0) {
               		BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

                	ch->nextquest = 15;
                	bprintf(buf, "You have run out of time for your"
                    " quest!\n\rYou may quest again in %d minutes.\n\r",
                    ch->nextquest);
                	send_to_char(buf->data, ch);
                	REMOVE_BIT(ch->act, PLR_QUESTOR);
                	ch->questgiver = NULL;
                	ch->countdown = 0;
                	ch->questmob = 0;
                	buffer_free( buf );
            	}

	            if (ch->countdown > 0 && ch->countdown < 6) {
	                send_to_char("Better hurry, you're almost out of time for"
                    " your quest!\n\r",ch);
    	            return;
        	    }
        	}
        }
    }

    return;
}

void add_random_apply( CHAR_DATA *ch, OBJ_DATA *obj ) {
    AFFECT_DATA af;
    int applies, x;

    applies = number_fuzzy( ( ch->level / 6 ) - 2 );
    for ( x = 0; x < applies; x++ ) {
        switch( number_range( 0, 17 ) ) {
            case 0:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_HITROLL;
                af.modifier = 2;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 1:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_DAMROLL;
                af.modifier = 2;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 2:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_MANA;
                af.modifier = 10;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 3:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_HIT;
                af.modifier = 10;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 4:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_MOVE;
                af.modifier = 10;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 5:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_AC;
                af.modifier = -2;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 6:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_SAVING_SPELL;
                af.modifier = -1;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 7:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_INT;
                af.modifier = 1;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 8:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_CON;
                af.modifier = 1;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 9:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_DEX;
                af.modifier = 1;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 10:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_STR;
                af.modifier = 1;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
            case 11:
                af.where = TO_OBJECT;
                af.type = 0;
                af.level = ch->level;
                af.duration = -1;
                af.location = APPLY_WIS;
                af.modifier = 1;
                af.bitvector = ITEM_MAGIC;
                affect_to_obj( obj, &af );
                break;
        }
    }
    return;
}
