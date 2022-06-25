///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <arpa/telnet.h>
#include <form.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"

// Global constants and variables (for this file, anyways)
const   char    echo_off_str    [] = { IAC, WILL, TELOPT_ECHO, '\0' };
const   char    echo_on_str     [] = { IAC, WONT, TELOPT_ECHO, '\0' };

bool rolled = FALSE;
int stat1[5],stat2[5],stat3[5],stat4[5],stat5[5];


void handle_con_get_name( DESCRIPTOR_DATA *d, char *argument ) {
	bool fOld;
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH];

    if ( argument[0] == '\0' ) {
        close_socket( d );
        return;
    }

    argument[0] = UPPER(argument[0]);

    if ( !check_parse_name( argument ) ) {
        write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
        return;
    }

    fOld = load_char_obj( d, argument );
    ch   = d->character;

    
    // This mess written by Rahl in a fit of rage, inspiration, and
    // boredom (gotta love art classes after someone tries to destroy
    // your MUD).
    // Ok.. now the important stuff - This code will "discourage"
    // people from trying to make imms and hide (imms above MAX_LEVEL
    // +2, who can't be seen on the "who" list.
    if ( ch->level > MAX_LEVEL + 2 ) {
        log_buf[0] = '\0';
        sprintf( log_buf, "%s@%s tried to log in at level %d.",
            argument, d->host, ch->level );
        log_string( log_buf );
        wiznet( log_buf, ch, NULL, WIZ_SECURE, 0, 0 );
        write_to_buffer( d, "Naughty, naughty. We don't like "
			"cheaters.\n\n\n\n", 0 );
        close_socket( d );
        return;
    }

    if ( IS_SET(ch->act, PLR_DENY) ) {
        sprintf( log_buf, "Denying access to %s@%s.", argument, d->host );
        log_string( log_buf );
        wiznet( log_buf, NULL, NULL, WIZ_SITES, 0, 0 );

        write_to_buffer( d, "You are denied access.\n\r", 0 );
        close_socket( d );
        return;
    }

    if (check_ban(d->host,BAN_PERMIT) && !IS_SET(ch->act,PLR_PERMIT)) {
        write_to_buffer(d,"Your site has been banned from Broken "
			"Shadows.\n\r", 0 );
        close_socket(d);
        return;
    }

    if ( check_reconnect( d, argument, FALSE ) ) {
        fOld = TRUE;
    } else {
        if ( wizlock && !IS_HERO(ch)) {
            write_to_buffer( d, "The game is wizlocked.\n\r", 0 );
            close_socket( d );
            return;
        }
        else if (chaos && !IS_HERO(ch)) {
            write_to_buffer( d, "The game is in CHAOS!\n\r", 0 );
            close_socket( d );
            return;
       }
    }

    if ( fOld ) {
        // Old player 
        write_to_buffer( d, "Password: ", 0 );
        write_to_buffer( d, echo_off_str, 0 );
        d->connected = CON_GET_OLD_PASSWORD;
        return;
    } else {
        // New player 
        if (newlock) {
            write_to_buffer( d, "The game is newlocked.\n\r", 0 );
            close_socket( d );
            return;
        }

        if (check_ban(d->host,BAN_NEWBIES)) {
            write_to_buffer(d,
                "New players are not allowed from your site.\n\r",0);
            close_socket(d);
            return;
        }

        sprintf( buf, "Did I get that right, %s (Y/N)? ", argument );
        write_to_buffer( d, buf, 0 );
        d->connected = CON_CONFIRM_NEW_NAME;
        return;
    }
}

void handle_con_get_old_password( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

    write_to_buffer( d, "\n\r", 2 );

    if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) ) {
        write_to_buffer( d, "Wrong password.\n\r", 0 );
        close_socket( d );
        return;
    }

    if ( ch->pcdata->pwd[0] == 0) {
        write_to_buffer( d, "Warning! Null password!\n\r",0 );
        write_to_buffer( d, "Please report old password with bug.\n\r",0);
        write_to_buffer( d,
            "Type 'password null <new password>' to fix.\n\r",0);
    }

    write_to_buffer( d, echo_on_str, 0 );

    if ( check_reconnect( d, ch->name, TRUE ) )
        return;

    if ( check_playing( d, ch->name ) )
        return;

    sprintf( log_buf, "%s@%s has connected.", ch->name, d->host );
    log_string( log_buf );
    wiznet( log_buf, NULL, NULL, WIZ_SITES, 0, get_trust( ch ) );

	if ( ch->desc->ansi ) {
		SET_BIT( ch->act, PLR_COLOR );
	} else {
		REMOVE_BIT( ch->act, PLR_COLOR );
	}

    if ( IS_IMMORTAL(ch) ) {
        do_help( ch, "imotd" );
        d->connected = CON_READ_IMOTD;
    } else {
        do_help( ch, "motd" );
        d->connected = CON_READ_MOTD;
    }
}

void handle_con_break_connect( DESCRIPTOR_DATA *d, char *argument ) {
	DESCRIPTOR_DATA *d_old, *d_next;
	CHAR_DATA *ch = d->character;

    switch( *argument ) {
    	case 'y' : case 'Y':
        	for ( d_old = descriptor_list; d_old != NULL; d_old = d_next ) {
            	d_next = d_old->next;
            	if (d_old == d || d_old->character == NULL)
                	continue;

	            if (str_cmp(ch->name, d_old->original ?
	                d_old->original->name : d_old->character->name))
	                continue;

 	           close_socket(d_old);
        	} // for

        	if (check_reconnect(d,ch->name,TRUE))
            	return;

        	write_to_buffer(d,"Reconnect attempt failed.\n\rName: ",0);

        	if ( d->character != NULL ) {
            	free_char( d->character );
            	d->character = NULL;
        	}

        	d->connected = CON_GET_NAME;
			break;

        case 'n' : case 'N':
            write_to_buffer(d,"Name: ",0);
            if ( d->character != NULL ) {
                free_char( d->character );
                d->character = NULL;
            }
            d->connected = CON_GET_NAME;
            break;

        default:
            write_to_buffer(d,"Please type Y or N? ",0);
            break;
    } // switch 
}

void handle_con_confirm_new_name( DESCRIPTOR_DATA *d, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *ch = d->character;

    switch ( *argument ) {
        case 'y': case 'Y':
            sprintf( buf, "New character.\n\rGive me a password for %s: %s",
                ch->name, echo_off_str );
            write_to_buffer( d, buf, 0 );
            d->connected = CON_GET_NEW_PASSWORD;
            break;

        case 'n': case 'N':
            write_to_buffer( d, "Ok, what IS it, then? ", 0 );
            free_char( d->character );
            d->character = NULL;
            d->connected = CON_GET_NAME;
            break;

        default:
            write_to_buffer( d, "Please type Yes or No? ", 0 );
            break;
    } // switch
}

void handle_con_get_new_password( DESCRIPTOR_DATA *d, char *argument ) {
	char *pwdnew;
	char *p;
	CHAR_DATA *ch = d->character;

    write_to_buffer( d, "\n\r", 2 );

    if ( strlen(argument) < 5 ) {
        write_to_buffer( d,
            "Password must be at least five characters long.\n\rPassword: ",
            0 );
        return;
    }

    pwdnew = crypt( argument, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ ) {
        if ( *p == '~' ) {
            write_to_buffer( d,
                "New password not acceptable, try again.\n\rPassword: ",
                0 );
            return;
        } // if
    } // for

    free_string( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    write_to_buffer( d, "Please retype password: ", 0 );
    d->connected = CON_CONFIRM_NEW_PASSWORD;
}

void handle_con_confirm_new_password( DESCRIPTOR_DATA *d, char *argument ) {
	int race;
	CHAR_DATA *ch = d->character;

    write_to_buffer( d, "\n\r", 2 );

    if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) ) {
        write_to_buffer( d, "Passwords don't match.\n\rRetype password: ",
            0 );
        d->connected = CON_GET_NEW_PASSWORD;
        return;
    }

    write_to_buffer( d, echo_on_str, 0 );
    write_to_buffer(d,"The following races are available:\n\r  ",0);

    for ( race = 1; race_table[race].name != NULL; race++ ) {
        if (!race_table[race].pc_race)
            break;

        if ( !race_table[race].remort_race ||
          ( race_table[race].remort_race && IS_SET( ch->act,
            PLR_REMORT ) ) ) {
            write_to_buffer(d,race_table[race].name,0);
            write_to_buffer(d," ",1);
        } // if
    } // for

    write_to_buffer(d,"\n\r",0);
    write_to_buffer(d,"What is your race (help for more information)? ",0);
    d->connected = CON_GET_NEW_RACE;
}

void handle_con_get_new_race( DESCRIPTOR_DATA *d, char *argument ) {
	int race;
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *ch = d->character;
	int i;

    one_argument(argument,arg);

    if (!strcmp(arg,"help")) {

        argument = one_argument(argument,arg);

        if (argument[0] == '\0')
            do_help(ch,"race help");
        else
            do_help(ch,argument);

        write_to_buffer(d,
            "What is your race (help for more information)? ",0);
        return;
    } // if 

    race = race_lookup(argument);

    if (race == 0 || !race_table[race].pc_race || ( !IS_SET( ch->act,
        PLR_REMORT ) && race_table[race].remort_race ) ) {
        write_to_buffer(d,"That is not a valid race.\n\r",0);
        write_to_buffer(d,"The following races are available:\n\r  ",0);

        for ( race = 1; race_table[race].name != NULL; race++ ) {
            if (!race_table[race].pc_race)
                break;

            if ( !race_table[race].remort_race ||
               ( race_table[race].remort_race && IS_SET( ch->act,
               PLR_REMORT ) ) ) {
                write_to_buffer(d,race_table[race].name,0);
                write_to_buffer(d," ",1);
            } // if
        } // for

        write_to_buffer(d,"\n\r",0);
        write_to_buffer(d,
            "What is your race? (help for more information) ",0);
        return;
    } // if

    ch->race = race;
    // initialize stats 
    for (i = 0; i < MAX_STATS; i++) {
        ch->perm_stat[i] = pc_race_table[race].stats[i];
	}

    ch->affected_by = ch->affected_by|race_table[race].aff;
    ch->affected2_by = ch->affected2_by|race_table[race].aff2;
    ch->imm_flags   = ch->imm_flags|race_table[race].imm;
    ch->res_flags   = ch->res_flags|race_table[race].res;
    ch->vuln_flags  = ch->vuln_flags|race_table[race].vuln;
    ch->form        = race_table[race].form;
    ch->parts       = race_table[race].parts;

    // add skills 
    for (i = 0; i < 5; i++) {
        if (pc_race_table[race].skills[i] == NULL)
            break;
        group_add(ch,pc_race_table[race].skills[i],FALSE);
    }

    // add cost 
    ch->pcdata->points = pc_race_table[race].points;
    ch->size = pc_race_table[race].size;
    write_to_buffer( d, "What is your sex (M/F)? ", 0 );
    d->connected = CON_GET_NEW_SEX;
}

void handle_con_get_new_sex( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

    switch ( argument[0] ) {
	    case 'm': 
		case 'M': 
			ch->sex = SEX_MALE;
        	ch->pcdata->true_sex = SEX_MALE;
        	break;
    	case 'f': 
		case 'F': 
			ch->sex = SEX_FEMALE;
        	ch->pcdata->true_sex = SEX_FEMALE;
        	break;
    	default:
        	write_to_buffer( d, "That's not a sex.\n\rWhat IS your sex? ", 0 );
        	return;
    } // switch

    write_to_buffer( d, "Press enter to start rolling your stats. ", 0);
    d->connected = CON_GET_STATS;
}

void handle_con_get_stats( DESCRIPTOR_DATA *d, char *argument ) {
	char buf[MAX_STRING_LENGTH];
	int iClass;
	int x;
	CHAR_DATA *ch = d->character;

    if (rolled==TRUE) {
        switch(argument[0]) {
	        case '0' :
    	    case '1' :
        	case '2' :
        	case '3' :
        	case '4' :
                ch->perm_stat[0]=stat1[atoi(argument)];
                ch->perm_stat[1]=stat2[atoi(argument)];
                ch->perm_stat[2]=stat3[atoi(argument)];
                ch->perm_stat[3]=stat4[atoi(argument)];
                ch->perm_stat[4]=stat5[atoi(argument)];

                strcpy( buf, "Select a class [" );
                for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
                {
                    if ( !class_table[iClass].remort_class ||
                       ( class_table[iClass].remort_class &&
                        IS_SET( ch->act, PLR_REMORT ) ) ) {

                        if ( iClass > 0 ) {
                            strcat( buf, " " );
						}
                        strcat( buf, class_table[iClass].name );
                    } // if
                } // for

                strcat( buf, "]: " );
                write_to_buffer( d, buf, 0 );
                d->connected = CON_GET_NEW_CLASS;
                break;
            default:
                write_to_buffer( d, "                       0    1    2    3    4\n\r", 0);
                write_to_buffer( d, "     Strength     :", 0);

                for (x = 0 ; x < 5 ; x++) {
                    stat1[x]=number_range(8+pc_race_table[ch->race].stats[0],
                        pc_race_table[ch->race].max_stats[0]);
                    sprintf(buf, "   %2d", stat1[x]);
                    write_to_buffer( d, buf, 0);
                } // for

                write_to_buffer( d, "\n\r     Intelligence :", 0);

                for (x = 0 ; x < 5 ; x++) {
                    stat2[x]=number_range(8+pc_race_table[ch->race].stats[1],
                        pc_race_table[ch->race].max_stats[1]);
                    sprintf(buf, "   %2d", stat2[x]);
                    write_to_buffer( d, buf, 0);
                } // for

                write_to_buffer( d, "\n\r     Wisdom       :", 0);

                for (x = 0 ; x < 5 ; x++) {
                    stat3[x] =
                        number_range(8+pc_race_table[ch->race].stats[2],
                        pc_race_table[ch->race].max_stats[2]);
                    sprintf(buf, "   %2d", stat3[x]);
                    write_to_buffer( d, buf, 0);
                } // for 

                write_to_buffer( d, "\n\r     Dexterity    :", 0);

                for (x = 0 ; x < 5 ; x++) {
                    stat4[x] =
                        number_range(8+pc_race_table[ch->race].stats[3],
                        pc_race_table[ch->race].max_stats[3]);
                    sprintf(buf, "   %2d", stat4[x]);
                    write_to_buffer( d, buf, 0);
                } // for

                write_to_buffer( d, "\n\r     Constitution :", 0);

                for (x = 0 ; x < 5 ; x++) {
                    stat5[x] =
                        number_range(8+pc_race_table[ch->race].stats[4],
                        pc_race_table[ch->race].max_stats[4]);
                    sprintf(buf, "   %2d", stat5[x]);
                    write_to_buffer( d, buf, 0);
                } // for

                write_to_buffer( d, "\n\r\n\r  Press enter to roll again, else enter number of column: ", 0);
                rolled=TRUE;
                break;
            } // switch
        } else {
            write_to_buffer( d, "                       0    1    2    3"
                "    4\n\r", 0);
            write_to_buffer( d, "     Strength     :",0);

            for (x = 0 ; x < 5 ; x++) {
                stat1[x]=number_range(8+pc_race_table[ch->race].stats[0],
                    pc_race_table[ch->race].max_stats[0]);
                sprintf(buf, "   %2d", stat1[x]);
                write_to_buffer( d, buf, 0);
            }

            write_to_buffer( d, "\n\r     Intelligence :", 0);

            for (x = 0 ; x < 5 ; x++) {
                stat2[x]=number_range(8+pc_race_table[ch->race].stats[1],
                    pc_race_table[ch->race].max_stats[1]);
                sprintf(buf, "   %2d", stat2[x]);
                write_to_buffer( d, buf, 0);
            }

            write_to_buffer( d, "\n\r     Wisdom       :", 0);

            for (x = 0 ; x < 5 ; x++) {
                stat3[x]=number_range(8+pc_race_table[ch->race].stats[2],
                    pc_race_table[ch->race].max_stats[2]);
                sprintf(buf, "   %2d", stat3[x]);
                write_to_buffer( d, buf, 0);
            }

            write_to_buffer( d, "\n\r     Dexterity    :", 0);

            for (x = 0 ; x < 5 ; x++) {
                stat4[x]=number_range(8+pc_race_table[ch->race].stats[3],
                    pc_race_table[ch->race].max_stats[3]);
                sprintf(buf, "   %2d", stat4[x]);
                write_to_buffer( d, buf, 0);
            }

            write_to_buffer( d, "\n\r     Constitution :", 0);

            for (x = 0 ; x < 5 ; x++) {
                stat5[x]=number_range(8+pc_race_table[ch->race].stats[4],
                    pc_race_table[ch->race].max_stats[4]);
                sprintf(buf, "   %2d", stat5[x]);
                write_to_buffer( d, buf, 0);
            }

            write_to_buffer( d, "\n\r\n\r     Press enter to roll again,"
                " else enter number of column: ", 0);
            rolled=TRUE;
    } // if
}

void handle_con_get_new_class( DESCRIPTOR_DATA *d, char *argument ) {
	int iClass;
	CHAR_DATA *ch = d->character;

	iClass = class_lookup(argument);

    if ( iClass == -1 || ( class_table[iClass].remort_class &&
        !IS_SET( ch->act, PLR_REMORT ) ) ) {
        write_to_buffer( d,
            "That's not a class.\n\rWhat IS your class? ", 0 );
        return;
    }

    ch->ch_class = iClass;

    sprintf( log_buf, "%s@%s new player.", ch->name, d->host );
    log_string( log_buf );
    wiznet( "Newbie alert! $N sighted!", ch, NULL, WIZ_NEWBIE, 0,0);
    wiznet( log_buf, NULL, NULL, WIZ_SITES, 0, get_trust( ch ) );

    write_to_buffer( d, "\n\r", 2 );
    // Paladins can't be evil.. - Rahl 
    if (ch->ch_class == 4) {
        write_to_buffer( d, "Your class is good by nature.\n\r",0);
    } else {
        write_to_buffer( d, "You may be good, neutral, or"
            " evil.\n\r",0);
        write_to_buffer( d, "Which alignment (G/N/E)? ",0);
    }

    d->connected = CON_GET_ALIGNMENT;
}

void handle_con_get_alignment( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

    // Paladins can't be evil - Rahl 
    if (ch->ch_class == 4) {
		ch->alignment = 750;
    } else {
		 switch( argument[0])
    	{
        	case 'g' : 
			case 'G' : 
				ch->alignment = 750;  
				break;
        	case 'n' : 
			case 'N' : 
				ch->alignment = 0;
			    break;
        	case 'e' : 
			case 'E' : 
				ch->alignment = -750; 
				break;
        	default:
            	write_to_buffer(d,"That's not a valid alignment.\n\r",0);
            	write_to_buffer(d,"Which alignment (G/N/E)? ",0);
            	return;
    	} // switch 
	} // if

    write_to_buffer(d,"\n\r",0);

    group_add(ch,"rom basics",FALSE);
    group_add(ch,class_table[ch->ch_class].base_group,FALSE);
    ch->pcdata->learned[gsn_recall] = 50;
    write_to_buffer(d,"Do you wish to customize this character?\n\r",0);
    write_to_buffer(d,"Customization takes time, but allows a wider "
		" range of skills and abilities.\n\r", 0 );
    write_to_buffer(d,"Customize (Y/N)? ",0);
    d->connected = CON_DEFAULT_CHOICE;
}

void handle_con_default_choice( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch  = d->character;
	char buf[MAX_STRING_LENGTH];
	int i;

    write_to_buffer(d,"\n\r",2);
    switch ( argument[0] ) {
        case 'y': 
		case 'Y':
            ch->gen_data = alloc_perm(sizeof(*ch->gen_data) );
            ch->gen_data->points_chosen = ch->pcdata->points;
            do_help(ch,"group header");
            list_group_costs(ch);
            write_to_buffer(d,"You already have the following skills:\n\r",0);
            do_skills(ch,"");
            do_help(ch,"menu choice");
            d->connected = CON_GEN_GROUPS;
            break;
        case 'n': 
		case 'N':
            group_add(ch,class_table[ch->ch_class].default_group,TRUE);
            write_to_buffer( d, "\n\r", 2 );
            write_to_buffer( d,
                "Please pick a weapon from the following choices:\n\r",0);
            buf[0] = '\0';

            for ( i = 0; weapon_table[i].name != NULL; i++ ) {
                if (ch->pcdata->learned[*weapon_table[i].gsn] > 0 ) {
                    strcat( buf, weapon_table[i].name );
                    strcat( buf, " ");
                } // if
			} // for

           	strcat( buf, "\n\rYour choice? ");
            write_to_buffer( d, buf, 0 );
            d->connected = CON_PICK_WEAPON;
            break;
        default:
            write_to_buffer( d, "Please answer (Y/N)? ", 0 );
            return;
    } // switch
}

void handle_con_pick_weapon( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;
	int weapon;
	char buf[MAX_STRING_LENGTH];
	int i;

    write_to_buffer( d, "\n\r", 2 );
    weapon = weapon_lookup( argument );
    if ( weapon == -1 || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0 ) {
        write_to_buffer( d,
            "That's not a valid selection. Choices are:\n\r",0);
        buf[0] = '\0';

        for ( i = 0; weapon_table[i].name != NULL; i++ ) {
            if ( ch->pcdata->learned[*weapon_table[i].gsn] > 0 ) {
                strcat( buf, weapon_table[i].name );
                strcat( buf, " " );
            } // if
		} // for
        strcat( buf, "\n\rYour choice? " );
        write_to_buffer( d, buf, 0 );
        return;
    } // if

    ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
    write_to_buffer( d, "\n\r", 2 );
 
	if ( ch->desc->ansi ) {
		SET_BIT( ch->act, PLR_COLOR );
	} else {
		REMOVE_BIT( ch->act, PLR_COLOR );
	}
	
    write_to_buffer( d, "\n\r", 2 );
    do_help( ch, "motd" );
	d->connected = CON_READ_MOTD;
}

void handle_con_gen_groups( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;
	char buf[MAX_STRING_LENGTH];
	int i;

    send_to_char("\n\r",ch);
    if (!str_cmp(argument,"done")) {
        sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
        send_to_char(buf,ch);

        if (ch->pcdata->points < 40) {
            ch->train = (40 - ch->pcdata->points + 1) / 2;
		}

        write_to_buffer( d, "\n\r", 2 );
        write_to_buffer( d,
            "Please pick a weapon from the following choices:\n\r", 0);
        buf[0] = '\0';

        for ( i=0; weapon_table[i].name != NULL; i++ ) {
            if ( ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
                strcat( buf, weapon_table[i].name );
                strcat( buf, " " );
            } // if
		} // for

        strcat( buf, "\n\rYour choice? " );
        write_to_buffer(d, buf, 0 );
        d->connected = CON_PICK_WEAPON;
        return;
    } // if

    if (!parse_gen_groups(ch,argument)) {
    	send_to_char(
    		"Choices are: list,learned,premise,add,drop,info,help, "
			"and done.\n\r" ,ch);
	} // if

    do_help(ch,"menu choice");
}

void handle_con_begin_remort( DESCRIPTOR_DATA *d, char *argument ) {
	int race;

    write_to_buffer( d, "Now beginning the remorting process.\n\r\n\r", 0 );
    write_to_buffer( d, "The following races are available:\n\r  ", 0 );

    for ( race = 1; race_table[race].name != NULL; race++ ) {
        if ( !race_table[race].pc_race ) {
            break;
		} // if
        write_to_buffer( d, race_table[race].name, 0 );
        write_to_buffer( d, "  ", 2 );
    } // for

    write_to_buffer( d, "\n\r", 0 );
    write_to_buffer( d, "What is your race (help for more information)? ", 0 );
	d->connected = CON_GET_NEW_RACE;
}

void handle_con_read_imotd( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;

    write_to_buffer(d,"\n\r",2);
    do_help( ch, "motd" );
    d->connected = CON_READ_MOTD;
}

void handle_con_read_motd( DESCRIPTOR_DATA *d, char *argument ) {
	CHAR_DATA *ch = d->character;
	//char buf[MAX_STRING_LENGTH];

    write_to_buffer( d, "\n\rWelcome to Broken Shadows.\n\r", 0 );
    ch->next        = char_list;
    char_list       = ch;
    d->connected    = CON_PLAYING;
    reset_char(ch);

    if ( ch->level == 0 ) {
        ch->level   = 1;
        ch->exp     = 0;
        ch->hit     = ch->max_hit;
        ch->mana    = ch->max_mana;
        ch->move    = ch->max_move;
        ch->train    = 3; // changed from 1 by Rahl 
        ch->practice = 5;
        set_title( ch, "the newborn" );

        // turn color on - Rahl 
        //do_nocolor( ch, "" );

        do_outfit(ch,"");
        obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP),0),ch);

        char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
        send_to_char("\n\r",ch);
        save_char_obj( ch );
        do_help(ch,"NEWBIE INFO");
        send_to_char("\n\r",ch);
    } else if ( ch->in_room != NULL ) {
        char_to_room( ch, ch->in_room );
    } else if ( IS_IMMORTAL(ch) ) {
        char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
    } else {
        char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
    } // if

    act( "$n has entered the game.", ch, NULL, NULL, TO_ROOM );
    wiznet( "$N has left real life behind.", ch, NULL, WIZ_LOGINS,
            WIZ_SITES, get_trust( ch ) );

    do_look( ch, "auto" );

/*
    if (ch->gold > 10000 && !IS_IMMORTAL(ch)) {
        sprintf(buf,"You are taxed %ld gold to pay for Zedd's crack "
			"habit.\n\r", (ch->gold - 10000) / 2);
        send_to_char(buf,ch);
        ch->gold -= (ch->gold - 10000) / 2;
    }

    if ( ch->bank > 50000 && !IS_IMMORTAL( ch ) ) {
        sprintf( buf, "Your bank account has been taxed %ld gold as a "
            "safe-keeping fee.\n\r", ( ch->bank - 50000 ) / 4 );
        send_to_char( buf, ch );
        ch->bank -= ( ch->bank - 50000 ) / 4;
    }
*/

    if (ch->pet != NULL) {
        char_to_room(ch->pet,ch->in_room);
        act("$n has entered the game.",ch->pet,NULL,NULL,TO_ROOM);
    }

    if (IS_SET(ch->act,PLR_AFK)) {
        do_afk(ch,NULL);
	}

    ch->pcdata->chaos_score = 0;
    do_board( ch, "" ); // show board status 
}

void handle_con_ansi( DESCRIPTOR_DATA *d, char *argument ) {
	extern char *help_greeting;

	if ( argument[0] == '\0' || UPPER( argument[0] ) == 'Y' ) {
		d->ansi = TRUE;
		send_to_desc( "\n\r`RANSI color enabled!`w\n\r", d );
		d->connected = CON_GET_NAME;

		if ( help_greeting[0] == '.' ) {
			send_to_desc( help_greeting + 1, d );
		} else {
			send_to_desc( help_greeting, d );
		}

		return;
	}

	if ( UPPER( argument[0] ) == 'N' ) {
		d->ansi = FALSE;
		send_to_desc( "\n\rANSI color disabled!\n\r", d );
		d->connected = CON_GET_NAME;

		if ( help_greeting[0] == '.' ) {
			send_to_desc( help_greeting + 1, d );
		} else {
			send_to_desc( help_greeting, d );
		}
		
		return;
	} else {
		send_to_desc( "Do you want ANSI color? [Y/n] ", d );
		return;
	}
}
