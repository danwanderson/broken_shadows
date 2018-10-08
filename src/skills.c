///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/* Code specifically for the new skill system */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"
#include "magic.h"
#include <math.h>
#include "interp.h"


/* used to get new skills */
void do_gain( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *trainer;
    int gn = 0, sn = 0;

    if ( IS_NPC( ch ) ) {
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    /* find a trainer */
    for ( trainer = ch->in_room->people; 
          trainer != NULL; 
          trainer = trainer->next_in_room ) {
		if ( IS_NPC( trainer ) && IS_SET( trainer->act, ACT_GAIN ) ) {
            break;
		}
	}

    if ( trainer == NULL || !can_see( ch, trainer ) ) {
        send_to_char("You can't do that here.\n\r",ch);
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        do_say( trainer, "Pardon me?" );
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    if ( !str_prefix( arg, "list" ) ) {
        int col = 0;
        
        bprintf( buf, "`W%-18s %-5s %-18s %-5s %-18s %-5s`w\n\r",
        	"group", "cost", "group", "cost", "group", "cost" );
        buffer_strcat( buffer, buf->data );

        for ( gn = 0; gn < MAX_GROUP; gn++ ) {
            if ( group_table[gn].name == NULL ) {
                break;
			}

            if ( !ch->pcdata->group_known[gn]
            &&  group_table[gn].rating[ch->ch_class] > 0 ) {
                bprintf( buf, "%-18s %-5d ", group_table[gn].name, 
					group_table[gn].rating[ch->ch_class] );
                buffer_strcat( buffer, buf->data );

                if ( ++col % 3 == 0 ) {
                    buffer_strcat( buffer, "\n\r" );
				}
            }
        } /* for */

        if ( col % 3 != 0 ) {
            buffer_strcat( buffer, "\n\r" );
		}
        
        buffer_strcat( buffer, "\n\r" );

        col = 0;

        bprintf( buf, "`W%-18s %-5s %-18s %-5s %-18s %-5s\n\r`w",
        	"skill", "cost", "skill", "cost", "skill", "cost" );
        buffer_strcat(buffer, buf->data);
 
        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            if ( skill_table[sn].name == NULL ) {
                break;
			}
 
            if ( !ch->pcdata->learned[sn]
            &&  skill_table[sn].rating[ch->ch_class] > 0
            &&  skill_table[sn].spell_fun == spell_null ) {
                bprintf( buf, "%-18s %-5d ", skill_table[sn].name,
					skill_table[sn].rating[ch->ch_class] );
            	buffer_strcat( buffer, buf->data );

                if ( ++col % 3 == 0 ) {
                    buffer_strcat( buffer, "\n\r" );
				}
            }
        }

        if ( col % 3 != 0 ) {
            buffer_strcat( buffer, "\n\r" );
		}

        buffer_free( buf );
        page_to_char( buffer->data, ch );
        buffer_free( buffer );
        return;
    }

    if ( !str_prefix( arg, "convert" ) ) {
        if ( ch->practice < 10 ) {
            act( "$N tells you 'You are not yet ready.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        act( "$N helps you apply your practice to training.",
	        ch, NULL, trainer, TO_CHAR );
        ch->practice -= 10;
        ch->train +=1 ;
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    if ( !str_prefix( arg, "points" ) ) {
        if ( ch->train < 2 ) {
            act( "$N tells you 'You are not yet ready.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        if ( ch->pcdata->points <= 40 ) {
            act( "$N tells you 'There would be no point in that.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        act( "$N trains you, and you feel more at ease with your skills.",
            ch, NULL, trainer, TO_CHAR );

        ch->train -= 2;
        ch->pcdata->points -= 1;
        ch->exp = 0;
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    /* else add a group/skill */

    gn = group_lookup( argument );

    if ( gn > 0 ) {
        if ( ch->pcdata->group_known[gn] ) {
            act( "$N tells you 'You already know that group!'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        if ( group_table[gn].rating[ch->ch_class] <= 0 ) {
            act( "$N tells you 'That group is beyond your powers.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        if ( ch->train < group_table[gn].rating[ch->ch_class] ) {
            act( "$N tells you 'You are not yet ready for that group.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        /* add the group */
        gn_add( ch, gn );
        act( "$N trains you in the art of $t.", ch,
			group_table[gn].name, trainer, TO_CHAR );
        ch->train -= group_table[gn].rating[ch->ch_class];
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    sn = skill_lookup( argument );

    if ( sn > -1 ) {
        if ( skill_table[sn].spell_fun != spell_null ) {
            act( "$N tells you 'You must learn the full group.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }
            

        if ( ch->pcdata->learned[sn] ) {
            act( "$N tells you 'You already know that skill!'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }
 
        if ( skill_table[sn].rating[ch->ch_class] <= 0 ) {
            act( "$N tells you 'That skill is beyond your powers.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }
 
        if ( ch->train < skill_table[sn].rating[ch->ch_class] ) {
            act( "$N tells you 'You are not yet ready for that skill.'",
                ch, NULL, trainer, TO_CHAR );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }
 
        /* add the skill */
        ch->pcdata->learned[sn] = 1;
        act( "$N trains you in the art of $t.", ch, 
			skill_table[sn].name, trainer, TO_CHAR );
        ch->train -= skill_table[sn].rating[ch->ch_class];
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    act( "$N tells you 'I do not understand...'", ch, NULL, trainer,
		TO_CHAR );
    buffer_free( buf );
    buffer_free( buffer );
    return;
}
    



/* RT spells and skills show the players spells (or skills) */

void do_spells( CHAR_DATA *ch, char *argument ) {
    int sn = 0;
	int mana = 0;
    bool found = FALSE;
    BUFFER *buf;
	int column = 0;
	BUFFER *buffer;

    if ( IS_NPC( ch ) ) {
    	return;
	}

    buf = buffer_new( MAX_INPUT_LENGTH );
    buffer = buffer_new( MAX_INPUT_LENGTH );
 
    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
    	if ( skill_table[sn].name == NULL ) {
        	break;
		}

        if ( skill_table[sn].spell_fun != spell_null &&
        ch->pcdata->learned[sn] > 0 ) {
        	found = TRUE;
            mana = mana_cost( skill_table[sn].min_mana, 
				ch->pcdata->learned[sn] );
            bprintf( buf, "%-18s  %3d mana  ", skill_table[sn].name, 
				mana );
        
       		if ( ++column % 2 == 0 ) {
				buffer_strcat( buf, "\n\r" );
        	}

			buffer_strcat( buffer, buf->data );
      	}
    }

    if ( !found ) {
    	send_to_char("You know no spells.\n\r",ch);
    	buffer_free( buf );
    	buffer_free( buffer );
    	return;
    }
    
    buffer_strcat( buffer, "\n\r" );
    page_to_char( buffer->data, ch );

    buffer_free( buf );
    buffer_free( buffer );
	return;
}

void do_skills( CHAR_DATA *ch, char *argument ) {
    int sn = 0;
    bool found = FALSE;
    BUFFER *buf;
    BUFFER *buffer; 
	int column = 0;

    if ( IS_NPC( ch ) ) {
    	return;
	}
 
    buffer = buffer_new( MAX_INPUT_LENGTH ); 
    buf = buffer_new( MAX_INPUT_LENGTH );

    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
    	if ( skill_table[sn].name == NULL ) {
        	break;
		}

 
      	if ( skill_table[sn].spell_fun == spell_null &&
        ch->pcdata->learned[sn] > 0 ) {
        	found = TRUE;
        	bprintf( buf, "%-18s %3d%%      ",
				skill_table[sn].name, ch->pcdata->learned[sn] );
 
          	if ( ++column % 2 == 0 ) {
            	buffer_strcat( buf, "\n\r" );
			}
            
			buffer_strcat( buffer, buf->data );
		}
	}
 
    if ( !found ) {
		send_to_char("You know no skills.\n\r",ch);
		buffer_free( buf );
		buffer_free( buffer );
		return;
    }
 
    buffer_strcat( buffer, "\n\r" );
    page_to_char( buffer->data, ch );

    buffer_free( buffer );
    buffer_free( buf );
    return;
}


/* shows skills, groups and costs (only if not bought) */
/* Used in handle_con.c when a character is creating */
void list_group_costs( CHAR_DATA *ch ) {
    BUFFER *buf = buffer_new( 100 );
    BUFFER *buffer = buffer_new( MAX_STRING_LENGTH );
    int gn = 0;
	int sn = 0;
	int col = 0;

    if ( IS_NPC( ch ) ) {
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    bprintf( buf, "`W%-18s %-5s %-18s %-5s %-18s %-5s`w\n\r",
        "group", "cp", "group", "cp", "group", "cp" );
    buffer_strcat( buffer, buf->data );

    for ( gn = 0; gn < MAX_GROUP; gn++ ) {
        if ( group_table[gn].name == NULL ) {
            break;
		}

        if ( !ch->gen_data->group_chosen[gn] 
        &&  !ch->pcdata->group_known[gn]
        &&  group_table[gn].rating[ch->ch_class] > 0 ) {
            bprintf( buf, "%-18s %-5d ", group_table[gn].name,
            	group_table[gn].rating[ch->ch_class] );
            buffer_strcat( buffer, buf->data );

            if ( ++col % 3 == 0 ) {
                buffer_strcat( buffer, "\n\r" );
			}
        }
    }

    if ( col % 3 != 0 ) {
        buffer_strcat( buffer, "\n\r" );
	}

    buffer_strcat( buffer, "\n\r" );

    col = 0;
 
    bprintf( buf, "`W%-18s %-5s %-18s %-5s %-18s %-5s`w\n\r",
        "skill", "cp", "skill", "cp", "skill", "cp" );
    buffer_strcat( buffer, buf->data );
 
    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
        if ( skill_table[sn].name == NULL ) {
            break;
		}
 
        if ( !ch->gen_data->skill_chosen[sn] 
        &&  ch->pcdata->learned[sn] == 0
        &&  skill_table[sn].spell_fun == spell_null
        &&  skill_table[sn].rating[ch->ch_class] > 0 ) {
        	bprintf( buf, "%-18s %-5d ", skill_table[sn].name,
            	skill_table[sn].rating[ch->ch_class] );
            buffer_strcat( buffer, buf->data );

            if ( ++col % 3 == 0 ) {
                buffer_strcat( buffer, "\n\r" );
			}
        }
    }

    if ( col % 3 != 0 ) {
        buffer_strcat( buffer, "\n\r" );
	}

    buffer_strcat( buffer, "\n\r" );

    bprintf( buf, "Creation points: %d\n\r", ch->pcdata->points );
    buffer_strcat( buffer, buf->data );
    page_to_char( buffer->data, ch );
    buffer_free( buffer );
    buffer_free( buf );
    return;
}

/* Color in this function, but it doesn't show up */
void list_group_chosen( CHAR_DATA *ch ) {
    BUFFER *buf = buffer_new( 100 );
	BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );
    int gn = 0;
	int sn = 0;
	int col = 0;
 
    if ( IS_NPC( ch ) ) {
        buffer_free( buf );
		buffer_free( buffer );
        return;
    }
 
    bprintf( buf, "`W%-18s %-5s %-18s %-5s %-18s %-5s`w\n\r",
        "group", "cp", "group", "cp", "group", "cp" );
	buffer_strcat( buffer, buf->data );
 
    for ( gn = 0; gn < MAX_GROUP; gn++ ) {
        if ( group_table[gn].name == NULL ) {
            break;
		}
 
        if ( ch->gen_data->group_chosen[gn] 
        &&  group_table[gn].rating[ch->ch_class] > 0 ) {
            bprintf( buf, "%-18s %-5d ", group_table[gn].name,
            	group_table[gn].rating[ch->ch_class] );
			buffer_strcat( buffer, buf->data );
	
            if ( ++col % 3 == 0 ) {
                buffer_strcat( buffer, "\n\r" );
			}
        }
    }

    if ( col % 3 != 0 ) {
        buffer_strcat( buffer, "\n\r" );
	}

    buffer_strcat( buffer, "\n\r" );
 
    col = 0;
 
    bprintf( buf, "`W%-18s %-5s %-18s %-5s %-18s %-5s`w\n\r",
        "skill", "cp", "skill", "cp", "skill", "cp" );
    buffer_strcat( buffer, buf->data );
 
    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
        if ( skill_table[sn].name == NULL ) {
            break;
		}
 
        if ( ch->gen_data->skill_chosen[sn] 
        &&  skill_table[sn].rating[ch->ch_class] > 0 ) {
        	bprintf( buf, "%-18s %-5d ", skill_table[sn].name,
	            skill_table[sn].rating[ch->ch_class] );
            buffer_strcat( buffer, buf->data );

            if ( ++col % 3 == 0 ) {
                buffer_strcat( buffer, "\n\r" );
			}
        }
    }

    if ( col % 3 != 0 ) {
        buffer_strcat( buffer, "\n\r" );
	}

    buffer_strcat( buffer, "\n\r" );
 
    bprintf( buf, "Creation points: %d\n\r",
		ch->gen_data->points_chosen );
	buffer_strcat( buffer, buf->data );
    page_to_char( buffer->data, ch );
    buffer_free( buf );
	buffer_free( buffer );
    return;
}


/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups( CHAR_DATA *ch, char *argument ) {
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( 100 );
    int gn = 0;
	int sn = 0;
	int i = 0;
 
    if (argument[0] == '\0') {
        buffer_free( buf );
        return FALSE;
    }

    argument = one_argument( argument, arg );

    if ( !str_prefix( arg, "help" ) ) {
        if ( argument[0] == '\0' ) {
            do_help( ch, "group help" );
            buffer_free( buf );
            return TRUE;
        }

        do_help( ch, argument );
        buffer_free( buf );
        return TRUE;
    }

    if ( !str_prefix( arg, "add" ) ) {
        if ( argument[0] == '\0' ) {
            send_to_char( "You must provide a skill name.\n\r", ch );
            buffer_free( buf );
            return TRUE;
        }

        gn = group_lookup( argument );
        if ( gn != -1 ) {
            if ( ch->gen_data->group_chosen[gn]
            ||  ch->pcdata->group_known[gn] ) {
                send_to_char( "You already know that group!\n\r", ch );
                buffer_free( buf );
                return TRUE;
            }

            if ( group_table[gn].rating[ch->ch_class] < 1 ) {
                send_to_char( "That group is not available.\n\r", ch );
                buffer_free( buf );
                return TRUE;
            }

			/* 
			 * Added by Rahl -- written by Dennis Riechel 
			 * Checks to see if you know groups that are included in
			 * other groups - like knowing detection and trying to 
			 * add mage default
			 */
        	for ( i=0; group_table[gn].spells[i] != NULL ; i++ ) {
           		if ( group_lookup( group_table[gn].spells[i] ) == -1 ) {
               		continue; 
				}
           		if ( ch->pcdata->group_known[group_lookup( group_table[gn].spells[i] )] ) {
               		send_to_char( "That group contains groups you already know.\n\r", ch);
					send_to_char("Please \"drop\" them if you wish to gain this one.\n\r", ch );
               		buffer_free( buf );
               		return TRUE;
           		}
    		}

    		for ( i=0; group_table[gn].spells[i] != NULL ; i++ ) {
        		if ( skill_lookup( group_table[gn].spells[i] ) == -1 ) {
            		continue; 
				}
        		if ( ch->gen_data->skill_chosen[skill_lookup( group_table[gn].spells[i] )] ) {
            		send_to_char( "That group contains skills/spells you already know.\n\r", ch );
            		send_to_char( "Please \"drop\" them if you wish to gain this one.\n\r", ch );
            		buffer_free( buf );
            		return TRUE;
        		}
    		}

            bprintf( buf, "%s group added\n\r", group_table[gn].name );
            send_to_char( buf->data, ch );
            ch->gen_data->group_chosen[gn] = TRUE;
            ch->gen_data->points_chosen += group_table[gn].rating[ch->ch_class];
            gn_add( ch, gn );
            ch->pcdata->points += group_table[gn].rating[ch->ch_class];
            buffer_free( buf );
            return TRUE;
        }

        sn = skill_lookup(argument);
        if ( sn != -1 ) {
            if ( ch->gen_data->skill_chosen[sn]
            ||  ch->pcdata->learned[sn] > 0 ) {
                send_to_char( "You already know that skill!\n\r", ch );
                buffer_free( buf );
                return TRUE;
            }

            if ( skill_table[sn].rating[ch->ch_class] < 1
            ||  skill_table[sn].spell_fun != spell_null ) {
                send_to_char( "That skill is not available.\n\r", ch );
                buffer_free( buf );
                return TRUE;
            }

            bprintf( buf, "%s skill added\n\r", skill_table[sn].name );
            send_to_char( buf->data, ch );
            ch->gen_data->skill_chosen[sn] = TRUE;
            ch->gen_data->points_chosen += skill_table[sn].rating[ch->ch_class];
            ch->pcdata->learned[sn] = 1;
            ch->pcdata->points += skill_table[sn].rating[ch->ch_class];
            buffer_free( buf );
            return TRUE;
        }

        send_to_char( "No skills or groups by that name...\n\r", ch );
        buffer_free( buf );
        return TRUE;
    }

    if ( !strcmp( arg, "drop" ) ) {
        if ( argument[0] == '\0' ) {
            send_to_char( "You must provide a skill to drop.\n\r", ch );
            buffer_free( buf );
            return TRUE;
        }

        gn = group_lookup( argument );
        if ( ( gn != -1 ) && ch->gen_data->group_chosen[gn] ) {
            send_to_char( "Group dropped.\n\r", ch );
            ch->gen_data->group_chosen[gn] = FALSE;
            ch->gen_data->points_chosen -= group_table[gn].rating[ch->ch_class];
            gn_remove( ch, gn );
            for ( i = 0; i < MAX_GROUP; i++ ) {
                if ( ch->gen_data->group_chosen[gn] ) {
                    gn_add( ch, gn );
				}
            }
            ch->pcdata->points -= group_table[gn].rating[ch->ch_class];
            buffer_free( buf );
            return TRUE;
        }

        sn = skill_lookup( argument );
        if ( ( sn != -1 ) && ch->gen_data->skill_chosen[sn] ) {
            send_to_char( "Skill dropped.\n\r", ch );
            ch->gen_data->skill_chosen[sn] = FALSE;
            ch->gen_data->points_chosen -= skill_table[sn].rating[ch->ch_class];
            ch->pcdata->learned[sn] = 0;
            ch->pcdata->points -= skill_table[sn].rating[ch->ch_class];
            buffer_free( buf );
            return TRUE;
        }

        send_to_char( "You haven't bought any such skill or group.\n\r",
			ch );
        buffer_free( buf );
        return TRUE;
    }

    if ( !str_prefix( arg, "premise" ) ) {
        do_help( ch, "premise" );
        buffer_free( buf );
        return TRUE;
    }

    if ( !str_prefix( arg, "list" ) ) {
        list_group_costs( ch );
        buffer_free( buf );
        return TRUE;
    }

    if ( !str_prefix( arg, "learned" ) ) {
        list_group_chosen( ch );
        buffer_free( buf );
        return TRUE;
    }

    if ( !str_prefix( arg, "info" ) ) {
        do_groups( ch, argument );
        buffer_free( buf );
        return TRUE;
    }

    buffer_free( buf );
    return FALSE;
}
            

/* shows all groups, or the sub-members of a group */
void do_groups( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( 100 );
    int gn = 0;
	int sn = 0;
	int col = 0;

    if ( IS_NPC( ch ) ) {
        buffer_free( buf );
        return;
    }

	/* show all groups */
	if ( ( argument[0] == '\0' ) || !str_cmp( argument, "all" ) ) { 
        for ( gn = 0; gn < MAX_GROUP; gn++ ) {
            if ( group_table[gn].name == NULL ) {
                break;
			}

            if ( ch->pcdata->group_known[gn] ) {
                bprintf( buf, "%-20s ", group_table[gn].name );
                send_to_char( buf->data, ch );

                if ( ++col % 3 == 0 ) {
                    send_to_char( "\n\r", ch );
				}
            }
        }

        if ( col % 3 != 0 ) {
            send_to_char( "\n\r", ch );
		}

        bprintf( buf, "Creation points: %d\n\r", ch->pcdata->points );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    /* show the sub-members of a group */
    gn = group_lookup( argument );

	if ( gn == -1 ) {
    	send_to_char( "No group of that name exist.\n\r", ch );
        send_to_char( "Type 'groups all' or 'info all' for a full listing.\n\r", ch );
        buffer_free( buf );
        return;
    }

	for ( sn = 0; sn < MAX_IN_GROUP; sn++ ) {
		if ( group_table[gn].spells[sn] == NULL ) {
            break;
		}

        bprintf( buf, "%-20s ", group_table[gn].spells[sn] );
        send_to_char( buf->data, ch );

        if ( ++col % 3 == 0 ) {
            send_to_char( "\n\r", ch );
		}
	}

    if ( col % 3 != 0 ) {
        send_to_char( "\n\r", ch );
	}

    buffer_free( buf );
    return;
}

/* checks for skill improvement */
void check_improve( CHAR_DATA *ch, int sn, bool success, int multiplier ) {
    int chance = 0;
    BUFFER *buf = buffer_new( 100 );

    if ( IS_NPC( ch ) ) {
        buffer_free( buf );
        return;
    }

	/* skill is not known or can't be improved */ 
    if ( ( skill_table[sn].rating[ch->ch_class] == 0 )
    ||  ( ch->pcdata->learned[sn] == 0 )
    ||  ( ch->pcdata->learned[sn] == 100 ) ) {
        buffer_free( buf );
        return;  
    }

    /* check to see if the character has a chance to learn */
    chance = 10 * int_app[get_curr_stat( ch, STAT_INT )].learn;
    chance /= ( multiplier * skill_table[sn].rating[ch->ch_class] );

    if ( number_range( 1, 1000 ) > chance ) {
        buffer_free( buf );
        return;
    }

    /* now that the character has a CHANCE to learn, see if they really have */ 

    if ( success ) {
        chance = URANGE( 5, 100 - ch->pcdata->learned[sn], 95 );
        if ( number_percent() < chance ) {
            bprintf( buf, "You have become better at %s!\n\r",
            	skill_table[sn].name);
            send_to_char( buf->data, ch );
            ch->pcdata->learned[sn]++;
            gain_exp( ch, 2 * skill_table[sn].rating[ch->ch_class] ); 
        }
    } else {
        chance = URANGE( 5, ch->pcdata->learned[sn] / 2, 30 );
        if ( number_percent() < chance ) {
            bprintf( buf, "You learn from your mistakes, and your %s skill improves.\n\r", skill_table[sn].name );
            send_to_char( buf->data,ch );
            ch->pcdata->learned[sn] += number_range(1,3);
            ch->pcdata->learned[sn] = UMIN( ch->pcdata->learned[sn], 100 );
            gain_exp( ch, 2 * skill_table[sn].rating[ch->ch_class] );
        }
    }

    buffer_free( buf );
    return;
}

/* returns a group index number given the name */
int group_lookup( const char *name ) {
    int gn = 0;
 
    for ( gn = 0; gn < MAX_GROUP; gn++ ) {
        if ( group_table[gn].name == NULL ) {
            break;
		}

        if ( LOWER( name[0] ) == LOWER( group_table[gn].name[0] )
        &&   !str_prefix( name, group_table[gn].name ) ) {
            return gn;
		}
    }
 
    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add( CHAR_DATA *ch, int gn ) {
    int i = 0;
    
    ch->pcdata->group_known[gn] = TRUE;

    for ( i = 0; i < MAX_IN_GROUP; i++ ) {
        if ( group_table[gn].spells[i] == NULL ) {
            break;
		}

        group_add( ch, group_table[gn].spells[i], FALSE );
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove( CHAR_DATA *ch, int gn ) {
    int i = 0;

    ch->pcdata->group_known[gn] = FALSE;

    for ( i = 0; i < MAX_IN_GROUP; i ++ ) {
        if ( group_table[gn].spells[i] == NULL ) {
            break;
		}
        group_remove( ch, group_table[gn].spells[i] );
    }
}
        
/* use for processing a skill or group for addition  */
void group_add( CHAR_DATA *ch, const char *name, bool deduct ) {
    int sn = 0;
	int gn = 0;

	/* NPCs do not have skills */
    if ( IS_NPC( ch ) ) {
        return;
	}

    sn = skill_lookup( name );

    if ( sn != -1 ) {
		/* not known */
        if ( ch->pcdata->learned[sn] == 0 ) {
            ch->pcdata->learned[sn] = 1;
            if ( deduct ) {
                ch->pcdata->points += skill_table[sn].rating[ch->ch_class]; 
			}
        }
        return;
    }
        
    /* now check groups */

    gn = group_lookup( name );

    if ( gn != -1 ) {
        if ( ch->pcdata->group_known[gn] == FALSE )  {
            ch->pcdata->group_known[gn] = TRUE;
            if ( deduct ) {
                ch->pcdata->points += group_table[gn].rating[ch->ch_class];
			}
        }

        gn_add( ch, gn ); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */
void group_remove( CHAR_DATA *ch, const char *name ) {
    int sn = 0;
	int gn = 0;
    
    sn = skill_lookup( name );

    if ( sn != -1 ) {
        ch->pcdata->learned[sn] = 0;
        return;
    }
 
    /* now check groups */
 
    gn = group_lookup( name );
 
    if ( ( gn != -1 ) && ch->pcdata->group_known[gn] == TRUE ) {
        ch->pcdata->group_known[gn] = FALSE;
        gn_remove( ch, gn );
		/* be sure to call gn_add on all remaining groups */
    }
}
