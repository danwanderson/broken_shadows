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
#include <stdlib.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"

extern AUCTION_DATA *auction;

/* 
 * random quote stuff.. added by Rahl.
 * originally by elfren@aros.net 
 */

/* quote structure */
struct quote_type {
    char *text;
    char *by;
};

/*
 * The quotes. Remember to change MAX_QUOTES in merc.h.
 * MAX_QUOTES _MUST_ equal the number of quotes you have
 */
const struct quote_type quote_table [MAX_QUOTES+1] = {
    { "", "" },
    { "I'll be back.", "Arnold" },
    { "Alas all good things must come to an end.", "ROM staff" },
    { "Y'all come back now, ya hear?", "" },
    { "See ya later, alligator.", "" },
    { "Friends come and go, but enemies accumulate.", "Jone's Motto" },
    { "The more things change, the more they stay insane.", "" },
    { "And now for something completely different...", "Monty Python" },
    { "I wanted to be a lumberjack!", "Monty Python" },
    { "Life is too important to take seriously.", "Corky Siegel" },
    { "An age is called Dark not because the light fails to shine"
        " but because people refuse to see it.", "James Michener,"
        " \"Space\"" },
	{ "Chaos reigns within.\n\rReflect, Repent, and Reboot.\n\rOrder"
		" shall return.", "" },
	{ "The web site you seek\n\rCannot be located but\n\rEndless "
		"others exist.", "" }
};

const char* colors[] = {
    "`w",
    "`R",
    "`Y",
	"`G",
	"`M",
	"`C",
	"`B"
};
    

/*
 * The quote routine
 */
void do_quote( CHAR_DATA *ch ) {   
    BUFFER *quote;
    int number = 0;
    int number2 = 0;

    quote = buffer_new( MAX_INPUT_LENGTH );
    number = number_range( 1, MAX_QUOTES );
    number2 = number_range( 1, 6 );
    bprintf( quote, "%s%s\n\r - %s\n\r\n\r`w",
        colors[number2],
        quote_table[number].text,
        quote_table[number].by );
    send_to_desc( quote->data, ch->desc );

    buffer_free( quote );

    return;
}


/* RT code to delete yourself */

void do_delet( CHAR_DATA *ch, char *argument) {
    send_to_char("You must type the full command to delete yourself.\n\r",ch);
}

void do_delete( CHAR_DATA *ch, char *argument) {
    char strsave[MAX_INPUT_LENGTH];
    CLAN_DATA *Clan;
    char buf[20];

    if ( IS_NPC( ch ) ) {
        return;
	}

    buf[0] = '\0';
    strsave[0] = '\0';

    if ( ch->pcdata->confirm_delete ) {
        if ( argument[0] != '\0' ) {
            send_to_char("Delete status removed.\n\r",ch);
            ch->pcdata->confirm_delete = FALSE;
            return;
        } else {
            sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->name ) );

            wiznet("$N turns $Mself into line noise.", ch, NULL,0,0,0);
        
            if ( ch->pcdata->clan != 0 ) {
                sprintf( buf, "%d", ch->pcdata->clan );
                Clan = find_clan ( buf );
                Clan->num_members -= 1;
            }

            do_quit(ch,"");
            unlink(strsave);
            return;
        }
    }

    if ( argument[0] != '\0' ) {
        send_to_char( "Just type delete. No argument.\n\r", ch );
        return;
    }

    send_to_char("Type delete again to confirm this command.\n\r",ch);
    send_to_char("WARNING: this command is irreversible.\n\r",ch);
    send_to_char("Typing delete with an argument will undo delete status.\n\r", ch);
    ch->pcdata->confirm_delete = TRUE;
    wiznet("$N is contemplating deletion.",ch,NULL,0,0,char_getImmRank(ch));

    return;
}
        

/* RT code to display channel status */
void do_channels( CHAR_DATA *ch, char *argument) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    /* lists all channels and their status */
    send_to_char("`W   channel   status`w\n\r",ch);
    send_to_char("`K---------------------`w\n\r",ch);
 
    send_to_char("gossip         ",ch);
    if (!IS_SET(ch->comm,COMM_NOGOSSIP))
      send_to_char("`GON`w\n\r",ch);
    else
      send_to_char("`ROFF`w\n\r",ch);

    send_to_char("auction        ",ch);
    if (!IS_SET(ch->comm,COMM_NOAUCTION))
      send_to_char("`GON`w\n\r",ch);
    else
      send_to_char("`ROFF`w\n\r",ch);

    send_to_char("music          ",ch);
    if (!IS_SET(ch->comm,COMM_NOMUSIC))
      send_to_char("`GON`w\n\r",ch);
    else
      send_to_char("`ROFF`w\n\r",ch);

    send_to_char("quote          ",ch);
    if (!IS_SET(ch->comm,COMM_NOQUOTE))
      send_to_char("`GON`w\n\r",ch);
    else
      send_to_char("`ROFF`w\n\r",ch);

    if ( IS_IMMORTAL( ch ) ) {
      send_to_char("imm channel    ",ch);
      if(!IS_SET(ch->comm,COMM_NOWIZ))
        send_to_char("`GON`w\n\r",ch);
      else
        send_to_char("`ROFF`w\n\r",ch);
    }

    send_to_char("info           ",ch);
    if (!IS_SET(ch->comm,COMM_NOINFO))
      send_to_char("`GON`w\n\r",ch);
    else
      send_to_char("`ROFF`w\n\r",ch);

    if ( !IS_NPC( ch ) && ch->pcdata->clan ) {
        send_to_char("clan           ",ch);
        if (!IS_SET(ch->comm,COMM_NOCLAN))
            send_to_char("`GON\n\r`w",ch);
        else
            send_to_char("`ROFF\n\r`w",ch);   
    }

    send_to_char("quiet mode     ",ch);
    if (IS_SET(ch->comm,COMM_QUIET))
      send_to_char("`GON`w\n\r",ch);
    else
      send_to_char("`ROFF`w\n\r",ch);

    if ( IS_SET( ch->comm, COMM_SNOOP_PROOF ) )
        send_to_char( "You are immune to snooping.\n\r", ch );

    if (ch->lines != PAGELEN) {
        if (ch->lines) {
            bprintf(buf,"You display %d lines of scroll.\n\r",
                ch->lines+2);
            send_to_char(buf->data,ch);
        } else
            send_to_char("Scroll buffering is off.\n\r",ch);
    }

    if (IS_SET(ch->comm,COMM_NOTELL))
      send_to_char("You cannot use tell.\n\r",ch);
 
    if (IS_SET(ch->comm,COMM_NOCHANNELS))
     send_to_char("You cannot use channels.\n\r",ch);

    if (IS_SET(ch->comm,COMM_NOEMOTE))
      send_to_char("You cannot emote.\n\r",ch);

    buffer_free( buf );
    return;
}

/* RT quiet blocks out all communication */
void do_quiet ( CHAR_DATA *ch, char * argument) {
    if ( IS_SET( ch->comm, COMM_QUIET ) ) {
        send_to_char("Quiet mode removed.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_QUIET);
    } else {
        send_to_char("From now on, you will only hear says and emotes.\n\r",ch);
        SET_BIT(ch->comm,COMM_QUIET);
    }
}


/* RT chat replaced with ROM gossip */
void do_gossip( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_STRING_LENGTH );
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0' ) {
        if (IS_SET(ch->comm,COMM_NOGOSSIP)) {
            send_to_char("`BGossip channel is now ON.\n\r`w",ch);
            REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
        } else {
            send_to_char("`BGossip channel is now OFF.\n\r`w",ch);
            SET_BIT(ch->comm,COMM_NOGOSSIP);
        }
    } else  { /* gossip message sent, turn gossip on if it isn't already */
        if (IS_SET(ch->comm,COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r",ch);
            buffer_free( buf );
            return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          buffer_free( buf );
          return;
        }

        REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
 
        bprintf( buf, "`BYou gossip '%s`B'`w\n\r", argument );
        send_to_char( buf->data, ch );

        for ( d = descriptor_list; d != NULL; d = d->next ) {
            CHAR_DATA *victim;
 
            victim = d->original ? d->original : d->character;
 
            if ( d->connected == CON_PLAYING &&
                 d->character != ch &&
                 !IS_SET(victim->comm,COMM_NOGOSSIP) &&
                 !IS_SET(victim->comm,COMM_QUIET) ) {
                 act_new( "`B$n gossips '$t`B'`w", 
                    ch,argument, d->character, TO_VICT,POS_SLEEPING );
            }
        }
    }
    buffer_free( buf );
    return;
}


/* RT music channel */
void do_music( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0' ) {
	    if (IS_SET(ch->comm,COMM_NOMUSIC)) {
	        send_to_char("`CMusic channel is now ON.\n\r`w",ch);
    	    REMOVE_BIT(ch->comm,COMM_NOMUSIC);
    	} else {
      		send_to_char("`CMusic channel is now OFF.\n\r`w",ch);
        	SET_BIT(ch->comm,COMM_NOMUSIC);
    	}
    } else { /* music sent, turn music on if it isn't already */
        if (IS_SET(ch->comm,COMM_QUIET)) {
        	send_to_char("You must turn off quiet mode first.\n\r",ch);
        	buffer_free( buf );
        	return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          buffer_free( buf );
          return;
        }
 
        REMOVE_BIT(ch->comm,COMM_NOMUSIC);
 
        bprintf( buf, "`CYou MUSIC: '%s`C'\n\r`w", argument );
        send_to_char( buf->data, ch );

        for ( d = descriptor_list; d != NULL; d = d->next ) {
            CHAR_DATA *victim;
 
            victim = d->original ? d->original : d->character;
 
            if ( d->connected == CON_PLAYING &&
                 d->character != ch &&
                 !IS_SET(victim->comm,COMM_NOMUSIC) &&
                 !IS_SET(victim->comm,COMM_QUIET) ) {
                 act_new("`C$n MUSIC: '$t`C'`w",
                    ch,argument,d->character,TO_VICT,POS_SLEEPING);
            }
        }
    }
    buffer_free( buf );
    return;
}

void do_immtalk( CHAR_DATA *ch, char *argument ) {
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' ) {
        if (IS_SET(ch->comm,COMM_NOWIZ)) {
            send_to_char("`WImmortal channel is now ON\n\r`w",ch);
            REMOVE_BIT(ch->comm,COMM_NOWIZ);
        } else {
            send_to_char("`WImmortal channel is now OFF\n\r`w",ch);
            SET_BIT(ch->comm,COMM_NOWIZ);
        } 
        return;
    }

    REMOVE_BIT(ch->comm,COMM_NOWIZ);

    act_new("`W$n: $t`w",ch,argument,NULL,TO_CHAR,POS_DEAD);

    for ( d = descriptor_list; d != NULL; d = d->next ) {
         if ( d->connected == CON_PLAYING && 
             IS_IMMORTAL(d->character) && 
             !IS_SET(d->character->comm,COMM_NOWIZ) ) {
            act_new("`W$n: $t`w",ch,argument,d->character,TO_VICT,POS_DEAD);
        }
    }

    return;
}



void do_say( CHAR_DATA *ch, char *argument ) {
    if ( argument[0] == '\0' ) {
        send_to_char( "Say what?\n\r", ch );
        return;
    }

    act( "`G$n says '$T`G'`w", ch, NULL, argument, TO_ROOM );
    act( "`GYou say '$T`G'`w", ch, NULL, argument, TO_CHAR );
    return;
}



void do_sendinfo( CHAR_DATA *ch, char *argument ) {
    DESCRIPTOR_DATA *d;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    
    if (argument[0] == '\0' ) {
      if (IS_SET(ch->comm,COMM_NOINFO)) {
        send_to_char("`BInfo channel is now ON.\n\r`w",ch);
        REMOVE_BIT(ch->comm,COMM_NOINFO);
      } else {
        send_to_char("`BInfo channel is now OFF.\n\r`w",ch);
        SET_BIT(ch->comm,COMM_NOINFO);
      }
      buffer_free( buf );
      return;
    }
    bprintf(buf, "`WINFO: `R");
    buffer_strcat(buf, argument);
    buffer_strcat(buf, "\n\r`w");
    for ( d = descriptor_list; d != NULL; d = d->next ) {
        CHAR_DATA *victim;

        victim = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING &&
            !IS_SET(victim->comm, COMM_NOINFO)) {
            send_to_char(buf->data,d->character);
        }
    }

    buffer_free( buf );
    return;
}



void do_tell( CHAR_DATA *ch, char *argument ) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( IS_SET(ch->comm, COMM_NOTELL) ) {
        send_to_char( "Your message didn't get through.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET(ch->comm, COMM_QUIET) ) {
        send_to_char( "You must turn off quiet mode first.\n\r", ch);
        buffer_free( buf );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "Tell whom what?\n\r", ch );
        buffer_free( buf );
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ( ( victim = get_char_world( ch, arg ) ) == NULL
    || ( IS_NPC(victim) && victim->in_room != ch->in_room ) ) {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim->desc == NULL && !IS_NPC(victim)) {
        act("$N seems to have misplaced $S link...try again later.",
           ch,NULL,victim,TO_CHAR);
        bprintf(buf,"`R%s tells you '%s`R'`w\n\r",PERS(ch,victim),argument);
        buf->data[0] = UPPER(buf->data[0]);
    
        victim->reply   = ch;
        buffer_strcat( victim->pcdata->buffer, buf->data );
        victim->pcdata->message_ctr++;
        buffer_free( buf );
        return;
    }

    if ( !IS_IMMORTAL(ch) && !IS_AWAKE(victim) ) {
        act( "$E can't hear you, but your tell is being recorded.", ch, 0,
            victim, TO_CHAR );
    
        if ( IS_NPC( victim ) ) {
            buffer_free( buf );
            return;
        }

        bprintf(buf,"`R%s tells you '%s`R'`w\n\r",
            PERS(ch,victim),argument);
        buf->data[0] = UPPER(buf->data[0]);

        victim->reply   = ch;
        buffer_strcat( victim->pcdata->buffer, buf->data );
        victim->pcdata->message_ctr++;
        buffer_free( buf );
        return;
    }
  
    if ( IS_SET(victim->comm,COMM_QUIET) && !IS_IMMORTAL(ch)) {
        act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
        buffer_free( buf );
        return;
    }

    if ( IS_SET( victim->act, PLR_AFK ) ) {

        if ( IS_NPC( victim ) ) {
            buffer_free( buf );
            return;
        }

	    send_to_char( "They're AFK, but will get your message when they return.\n\r", ch );
		if ( victim->pcdata->away_message ) {
			send_to_char( "Message: ", ch );
			send_to_char( victim->pcdata->away_message, ch );
		}
        bprintf(buf,"`R%s tells you '%s`R'`w\n\r",
            PERS(ch,victim),argument);
        buf->data[0] = UPPER(buf->data[0]);
  
        victim->reply   = ch;
        buffer_strcat( victim->pcdata->buffer, buf->data );
        victim->pcdata->message_ctr++;
        buffer_free( buf );
        return;
    }

    act( "`RYou tell $N '$t`R'`w", ch, argument, victim, TO_CHAR );
    act_new("`R$n tells you '$t`R'`w",ch,argument,victim,TO_VICT,POS_DEAD);
    victim->reply   = ch;

    buffer_free( buf );
    return;
}



void do_reply( CHAR_DATA *ch, char *argument ) {
    CHAR_DATA *victim;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );


    if ( IS_SET(ch->comm, COMM_NOTELL) ) { 
        send_to_char( "Your message didn't get through.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = ch->reply ) == NULL ) {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim->desc == NULL && !IS_NPC(victim)) {
        act("$N seems to have misplaced $S link...try again later.",
            ch,NULL,victim,TO_CHAR);
        bprintf(buf,"`R%s tells you '%s`R'`w\n\r",
            PERS(ch,victim),argument);
        buf->data[0] = UPPER(buf->data[0]);

        victim->reply   = ch;
        buffer_strcat( victim->pcdata->buffer, buf->data );
        victim->pcdata->message_ctr++;
        buffer_free( buf );
        return;
    }

    if ( !IS_IMMORTAL(ch) && !IS_AWAKE(victim) ) {
        act( "$E can't hear you, but your tell is being recorded.", ch, 0,
            victim, TO_CHAR );

        if ( IS_NPC( victim ) ) {
            buffer_free( buf );
            return;
        }

        bprintf(buf,"`R%s tells you '%s`R'`w\n\r",
            PERS(ch,victim),argument);
        buf->data[0] = UPPER(buf->data[0]);

        victim->reply   = ch;
        buffer_strcat( victim->pcdata->buffer, buf->data );
        victim->pcdata->message_ctr++;
        buffer_free( buf );
        return;
    }

    if ( IS_SET(victim->comm,COMM_QUIET) && !IS_IMMORTAL(ch)) {
        act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
        buffer_free( buf );
        return;
    }

    if ( IS_SET( victim->act, PLR_AFK ) ) {
        if ( IS_NPC( victim ) ) {
            act_new( "$E is AFK, and not receiving tells.",
                ch,NULL,victim,TO_CHAR,POS_DEAD);
            buffer_free( buf );
            return;
        }

	    send_to_char( "They're AFK, but will get your message when they return.\n\r", ch );
		if ( victim->pcdata->away_message ) {
			send_to_char( "Message: ", ch );
			send_to_char( victim->pcdata->away_message, ch );

		}
        bprintf(buf,"`R%s tells you '%s`R'`w\n\r",
            PERS(ch,victim),argument);
        buf->data[0] = UPPER(buf->data[0]);

        victim->reply   = ch;
        buffer_strcat( victim->pcdata->buffer, buf->data );
        victim->pcdata->message_ctr++;
        buffer_free( buf );
        return;
    }

    act("`RYou tell $N '$t`R'`w",ch,argument,victim,TO_CHAR);
    act_new("`R$n tells you '$t`R'`w",ch,argument,victim,TO_VICT,POS_DEAD);
    victim->reply   = ch;

    buffer_free( buf );

    return;
}



void do_yell( CHAR_DATA *ch, char *argument ) {
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' ) {
        send_to_char( "Yell what?\n\r", ch );
        return;
    }


    act("`YYou yell '$t`Y'`w",ch,argument,NULL,TO_CHAR);
    for ( d = descriptor_list; d != NULL; d = d->next ) {
        if ( d->connected == CON_PLAYING
        &&   d->character != ch
        &&   d->character->in_room != NULL
        &&   d->character->in_room->area == ch->in_room->area 
        &&   !IS_SET(d->character->comm,COMM_QUIET) ) {
            act("`Y$n yells '$t`Y'`w",ch,argument,d->character,TO_VICT);
        }
    }

    return;
}



void do_emote( CHAR_DATA *ch, char *argument ) {
    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
        send_to_char( "You can't emote.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }

    act( "$n $T`w", ch, NULL, argument, TO_ROOM );
    act( "$n $T`w", ch, NULL, argument, TO_CHAR );
    return;
}

void do_info( CHAR_DATA *ch, char *argument ) {
      if (IS_SET(ch->comm,COMM_NOINFO)) {
        send_to_char("`BInfo channel is now ON.\n\r`w",ch);
        REMOVE_BIT(ch->comm,COMM_NOINFO);
      } else {
        send_to_char("`BInfo channel is now OFF.\n\r`w",ch);
        SET_BIT(ch->comm,COMM_NOINFO);
      }
      return;
}


void do_qui( CHAR_DATA *ch, char *argument ) {
    send_to_char( "If you want to QUIT, you have to spell it out.\n\r", ch );
    return;
}



void do_quit( CHAR_DATA *ch, char *argument ) {
    DESCRIPTOR_DATA *d;

    send_to_char( "\n\r", ch );

    if ( IS_NPC(ch) ) {
        return;
	}

    if ( ch->position == POS_FIGHTING ) {
        send_to_char( "No way! You are fighting.\n\r", ch );
        return;
    }

    if ( ch->position  < POS_STUNNED  ) {
        send_to_char( "You're not DEAD yet.\n\r", ch );
        return;
    }

    if ( auction->item != NULL && ( ( ch == auction->buyer ) 
    || ( ch == auction->seller ) ) ) {
        send_to_char( "Wait till you have bought/sold the item on auction.\n\r", ch );
        return;
    }

    /* call the random quote function */
    do_quote( ch );

    show_time( ch );

    act( "$n has left the game.", ch, NULL, NULL, TO_ROOM );
    sprintf( log_buf, "%s has quit.", ch->name );
    log_string( log_buf );
    
    log_buf[0] = '\0';

    wiznet("$N rejoins the real world.",ch,NULL,WIZ_LOGINS,0,char_getImmRank(ch));

    /*
     * After extract_char the ch is no longer valid!
     */
    save_char_obj( ch );

    /* free note that might be there somehow */
    if ( ch->pcdata->in_progress )
        free_note( ch->pcdata->in_progress );

    d = ch->desc;
    extract_char( ch, TRUE );

    if ( d != NULL ) {
        close_socket( d );
	}

    return;
}



void do_save( CHAR_DATA *ch, char *argument ) {
    if ( IS_NPC(ch) ) {
        return;
	}

    if ( chaos ) {
    	send_to_char("Saving is not allowed during `rC`RH`YA`RO`rS.\n\r`w",ch);
           return;
    }

    save_char_obj( ch );
    send_to_char("Saving.\n\r", ch );
    return;
}



/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
void do_follow( CHAR_DATA *ch, char *argument ) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Follow whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL ) {
        act( "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
        return;
    }

    if ( victim == ch ) {
        if ( ch->master == NULL ) {
            send_to_char( "You already follow yourself.\n\r", ch );
            return;
        }
        stop_follower(ch);
        return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOFOLLOW) ) {
        act("$N doesn't seem to want any followers.\n\r",
             ch,NULL,victim, TO_CHAR);
        return;
    }

    REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    
    if ( ch->master != NULL )
        stop_follower( ch );

    add_follower( ch, victim );
        return;
}


void add_follower( CHAR_DATA *ch, CHAR_DATA *master ) {
    if ( ch->master != NULL ) {
        bug( "Add_follower: non-null master.", 0 );
        return;
    }

    ch->master        = master;
    ch->leader        = NULL;

    if ( can_see( master, ch ) )
        act( "$n now follows you.", ch, NULL, master, TO_VICT );

    act( "You now follow $N.",  ch, NULL, master, TO_CHAR );

    return;
}



void stop_follower( CHAR_DATA *ch ) {
    if ( ch->master == NULL ) {
        bug( "Stop_follower: null master.", 0 );
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) ) {
        REMOVE_BIT( ch->affected_by, AFF_CHARM );
        affect_strip( ch, gsn_charm_person );
    }

    if ( can_see( ch->master, ch ) && ch->in_room != NULL) {
        act( "$n stops following you.", ch, NULL, ch->master, TO_VICT );
        act( "You stop following $N.",  ch, NULL, ch->master, TO_CHAR    );
    }
    if (ch->master->pet == ch)
    ch->master->pet = NULL;

    ch->master = NULL;
    ch->leader = NULL;
    return;
}

/* nukes charmed monsters and pets */
void nuke_pets( CHAR_DATA *ch ) {    
    CHAR_DATA *pet;

    if ((pet = ch->pet) != NULL) {
        stop_follower(pet);
        if (pet->in_room != NULL)
            act("$N slowly fades away.",ch,NULL,pet,TO_NOTVICT);
        extract_char(pet,TRUE);
    }
    ch->pet = NULL;

    return;
}



void die_follower( CHAR_DATA *ch ) {
    CHAR_DATA *fch;

    if ( ch->master != NULL ) {
        if (ch->master->pet == ch)
            ch->master->pet = NULL;
        stop_follower( ch );
    }

    ch->leader = NULL;

    for ( fch = char_list; fch != NULL; fch = fch->next ) {
    if ( fch->master == ch )
        stop_follower( fch );
    if ( fch->leader == ch )
        fch->leader = fch;
    }

    return;
}



void do_order( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    argument = one_argument( argument, arg );
    one_argument(argument,arg2);

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "Order whom to do what?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ((!str_prefix(arg2,"delete")) || (!str_prefix(arg2, "password"))
    || ( !str_cmp( arg2, "pk" ) ) || ( !str_cmp( arg2, "quit" ) ) ) {
        send_to_char("That will NOT be done.\n\r",ch);
        sprintf( log_buf, "%s: order %s %s", ch->name, arg, arg2 );
        wiznet(log_buf,ch,NULL,WIZ_SECURE,0,char_getImmRank(ch));
        log_string( log_buf );
        buffer_free( buf );
        return;
    }


    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg, "all" ) ) {
        fAll   = TRUE;
        victim = NULL;
    } else {
        fAll   = FALSE;
        if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
            send_to_char( "They aren't here.\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( victim == ch ) {
            send_to_char( "Aye aye, right away!\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( !IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch ) {
            send_to_char( "Do it yourself!\n\r", ch );
            buffer_free( buf );
            return;
        }
    }

    found = FALSE;
    for ( och = ch->in_room->people; och != NULL; och = och_next ) {
        och_next = och->next_in_room;

        if ( IS_AFFECTED(och, AFF_CHARM)
        &&   och->master == ch
        && ( fAll || och == victim ) ) {
            found = TRUE;
            bprintf( buf, "$n orders you to '%s'.", argument );
            act( buf->data, ch, NULL, och, TO_VICT );
            interpret( och, argument );
        }
    }

    if ( found )
        send_to_char( "Ok.\n\r", ch );
    else
        send_to_char( "You have no followers here.\n\r", ch );

    buffer_free( buf );
    return;
}



void do_group( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        CHAR_DATA *gch;
        CHAR_DATA *leader;

        leader = (ch->leader != NULL) ? ch->leader : ch;
        bprintf( buf, "`K[`W%s's group`K]`w\n\r", PERS(leader, ch) );
        send_to_char( buf->data, ch );

        for ( gch = char_list; gch != NULL; gch = gch->next ) {
            if ( is_same_group( gch, ch ) ) {
                bprintf( buf,
        "`K[`G%s`K] `w%-13s `W%4d`K/`W%4d hp %4d`K/`W%4d mana %4d`K/`W%4d mv\n\r",
                IS_NPC(gch) ? "Mob" : class_table[gch->ch_class].who_name,
                capitalize( PERS(gch, ch) ),
                gch->hit,   gch->max_hit,
                gch->mana,  gch->max_mana,
                gch->move,  gch->max_move);

                send_to_char( buf->data, ch );
            }
        }

        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch ) ) {
        send_to_char( "But you are following someone else!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( victim->master != ch && ch != victim ) {
        act( "$N isn't following you.", ch, NULL, victim, TO_CHAR );
        buffer_free( buf );
        return;
    }
    
    if (IS_AFFECTED(victim,AFF_CHARM)) {
        send_to_char("You can't remove charmed mobs from your group.\n\r",ch);
        buffer_free( buf );
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_CHARM)) {
        act("You like your master too much to leave $m!",ch,NULL,victim,
            TO_VICT);
        buffer_free( buf );
        return;
    }

    if ( is_same_group( victim, ch ) && ch != victim ) {
        victim->leader = NULL;
        act( "$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT );
        act( "$n removes you from $s group.",  ch, NULL, victim, TO_VICT );
        act( "You remove $N from your group.", ch, NULL, victim, TO_CHAR );
        buffer_free( buf );
        return;
    }

    if ( ( IS_IMMORTAL( ch ) && !IS_IMMORTAL( victim ) ) 
    ||   ( IS_IMMORTAL( victim ) && !IS_IMMORTAL( ch ) ) ) {
        act( "$N cannot join $n's group.", ch, NULL, victim,TO_NOTVICT );
        act( "You cannot join $n's group.", ch, NULL, victim, TO_VICT );
        act( "$N cannot join your group.", ch, NULL, victim, TO_CHAR );
        buffer_free( buf );
        return;
    }


    victim->leader = ch;
    act( "$N joins $n's group.", ch, NULL, victim, TO_NOTVICT );
    act( "You join $n's group.", ch, NULL, victim, TO_VICT    );
    act( "$N joins your group.", ch, NULL, victim, TO_CHAR    );

    buffer_free( buf );
    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount;
    int share;
    int extra;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Split how much?\n\r", ch );
        buffer_free( buf );
        return;
    }
    
    amount = atoi( arg );

    if ( amount < 0 ) {
        send_to_char( "Your group wouldn't like that.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( amount == 0 ) {
        send_to_char( "You hand out zero coins, but no one notices.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->gold < amount ) { 
        send_to_char( "You don't have that much gold.\n\r", ch );
        buffer_free( buf );
        return;
    }
  
    members = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room ) {
        if ( is_same_group( gch, ch ) && !IS_AFFECTED(gch,AFF_CHARM))
            members++;
    }

    if ( members < 2 ) {
        send_to_char( "Just keep it all.\n\r", ch );
        buffer_free( buf );
        return;
    }
        
    share = amount / members;
    extra = amount % members;

    if ( share == 0 ) {
        send_to_char( "Don't even bother, cheapskate.\n\r", ch );
        buffer_free( buf );
        return;
    }

    ch->gold -= amount;
    ch->gold += share + extra;

    bprintf( buf,
       "You split %d gold coins.  Your share is %d gold coins.\n\r",
       amount, share + extra );
    send_to_char( buf->data, ch );

    bprintf( buf, "$n splits %d gold coins.  Your share is %d gold coins.",
        amount, share );

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room ) {
        if ( gch != ch && is_same_group( gch, ch ) && !IS_AFFECTED(gch,AFF_CHARM)) {
            act( buf->data, ch, NULL, gch, TO_VICT );
            gch->gold += share;
        }
    }

    buffer_free( buf );
    return;
}



void do_gtell( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *gch;

    if ( argument[0] == '\0' ) {
        send_to_char( "Tell your group what?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET( ch->comm, COMM_NOTELL ) ) {
        send_to_char( "Your message didn't get through!\n\r", ch );
        buffer_free( buf );
        return;
    }

    /*
     * Note use of send_to_char, so gtell works on sleepers.
     */
    bprintf( buf, "`C%s tells the group '%s`C'.\n\r`w", ch->name, argument );
    for ( gch = char_list; gch != NULL; gch = gch->next ) {
        if ( is_same_group( gch, ch ) )
            send_to_char( buf->data, gch );
    }

    buffer_free( buf );
    return;
}



/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch ) {
    if ( ach->leader != NULL ) ach = ach->leader;
    if ( bch->leader != NULL ) bch = bch->leader;
    return ach == bch;
}

/*
 * Clan channel by Rahl
 */
void do_ctell( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    DESCRIPTOR_DATA *d;

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' ) {
        if (IS_SET(ch->comm, COMM_NOCLAN)) {
            send_to_char( "Clan channel is now ON.\n\r", ch );
            REMOVE_BIT(ch->comm, COMM_NOCLAN);
        } else {
            send_to_char( "Clan channel is now OFF.\n\r", ch);
            SET_BIT(ch->comm, COMM_NOCLAN);
        }
    } else {
        if ( IS_SET(ch->comm, COMM_QUIET )) {
            send_to_char("You must turn off quiet mode first!\n\r", ch);
            buffer_free( buf );
            return;
        }

        if ( IS_SET(ch->comm, COMM_NOCHANNELS )) {
            send_to_char("The gods have revoked your channel privileges!\n\r", ch);
            buffer_free( buf );
            return;
        }

        if ( ch->pcdata->clan == 0 ) {
            send_to_char( "You aren't in a clan!\n\r", ch );
            buffer_free( buf );
            return;
        }

        REMOVE_BIT(ch->comm, COMM_NOCLAN);
        bprintf( buf, "`W[%s`W]`M %s: %s`w",
            vis_clan( ch->pcdata->clan ), ch->name, argument );
        send_to_char( buf->data, ch );
        send_to_char( "\n\r", ch );
        for ( d = descriptor_list; d != NULL; d = d->next ) {
            CHAR_DATA *victim;
            victim = d->original ? d->original : d->character;
    
            if ( d->connected == CON_PLAYING &&
                 d->character != ch &&
                 is_same_clan (ch, victim ) &&
                 !IS_SET(victim->comm, COMM_NOCLAN) &&
                !IS_SET(victim->comm, COMM_QUIET) ) {
                act_new( "$t", ch, buf->data, d->character,
                    TO_VICT,POS_SLEEPING);
            }
        }
    }
    buffer_free( buf );
    return;
}

bool is_same_clan (CHAR_DATA *ch, CHAR_DATA *victim) {
    if ( ch->pcdata->clan == victim->pcdata->clan )
        return TRUE;
    else 
        return FALSE;
}



void do_message( CHAR_DATA *ch, char *argument ) {
    if ( IS_NPC( ch ) ) {
        send_to_char( "You don't record messages.\n\r", ch );
        return;
    }

    if ( ch->pcdata->buffer->data[0] == '\0' ) {
        send_to_char( "You have no messages waiting.\n\r", ch );
    } else {
        page_to_char( ch->pcdata->buffer->data, ch );
        buffer_clear( ch->pcdata->buffer );
        ch->pcdata->message_ctr = 0;
    }
}

/* added by Rahl */
/* new auction by Erwin Andreasen */
/* put an item on auction or see the stats on the current item or bet */
void do_auction( CHAR_DATA *ch, char *argument ) {
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_STRING_LENGTH );

    argument = one_argument( argument, arg1 );

    /* NPC can be extracted (killed) at any time and thus can't auction */
    if ( IS_NPC( ch ) ) {
        send_to_char( "Sorry. You can't use the new auction channel.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( arg1[0] == '\0' ) {
        if ( auction->item != NULL ) {
            /* show item data here */
            if ( auction->bet > 0 )
                bprintf( buf, "`MCurrent bid on this item is %ld gold.\n\r`w",
                   auction->bet );
            else
                bprintf( buf, "`MNo bids on this item have been received.\n\r`w" );
            send_to_char( buf->data, ch );
            /* do_lore? - Rahl */
            spell_identify( 0, 0, ch, auction->item, TARGET_OBJ );
            buffer_free( buf );
            return;
        } else {
            send_to_char( "Auction WHAT?\n\r", ch );
            buffer_free( buf );
            return;
        }
    }

    if ( IS_IMMORTAL( ch ) && !str_cmp( arg1, "stop" ) ) {
        if ( auction->item == NULL ) {
            send_to_char( "There is no auction going on that you can stop.\n\r", ch );
            buffer_free( buf );
            return;
        } else { /* stop the auction */
            bprintf( buf, "`mSale of %s `mhas been stopped by an immortal. Item confiscated.", 
                auction->item->short_descr );
            talk_auction( buf->data );
            obj_to_char( auction->item, ch );
            auction->item = NULL;
            if ( auction->buyer != NULL ) /* return the money to the buyer */ {
                auction->buyer->gold += auction->bet;
                send_to_char( "Your money has been returned.\n\r", auction->buyer );
            }
           buffer_free( buf );
           return;
       }
    }

    if ( !str_cmp( arg1, "bid" ) ) {
        if ( auction->item != NULL ) {
            long newbet;
        
            /* make - perhaps - a bet now */
            if ( argument[0] == '\0' ) {
                send_to_char( "Bid how much?\n\r", ch );
                buffer_free( buf );
                return;
            }

            newbet = atol( argument );

            if ( newbet < ( auction->bet + 100 ) ) {
                send_to_char( "You must bid at least 100 gold over the current bid.\n\r", ch );
                buffer_free( buf );
                return;
            }

            if ( newbet > ch->gold ) {
                send_to_char( "You do not have that much money.\n\r", ch );
                buffer_free( buf );
                return;
            }

            if ( ch == auction->seller ) {
                send_to_char( "That's a dirty tactic.\n\r", ch );
                buffer_free( buf );
                return;
            }

            /* the acutal bet is ok */
        
            /* return the gold to the last buyer, if one exists */
            if ( auction->buyer != NULL )
                auction->buyer->gold += auction->bet;
    
            ch->gold -= newbet; /* subtract the gold - important :) */
            auction->buyer = ch;
            auction->bet = newbet;
            auction->going = 0;
            auction->pulse = PULSE_AUCTION; /* start the auction over again */ 
      
            bprintf( buf, "`mA bid of %ld gold has been received on %s`m.`w", 
                newbet, auction->item->short_descr );
            talk_auction( buf->data );
            buffer_free( buf );
            return;
        } else {
            send_to_char( "There isn't anything being auctioned right now.\n\r", ch );
            buffer_free( buf );
            return;
        }
    }

    /* on and off added by Rahl */
    if ( !str_cmp( arg1, "on" ) ) {
        if ( IS_SET( ch->comm, COMM_NOAUCTION ) ) {
            REMOVE_BIT( ch->comm, COMM_NOAUCTION );
            send_to_char( "Auction channel is now on.\n\r", ch );
            buffer_free( buf );
            return;
        } else {
            send_to_char( "Auction channel is already on.\n\r", ch );
            buffer_free( buf );
            return;
        }
    }

    if ( !str_cmp( arg1, "off" ) ) {
        if ( !IS_SET( ch->comm, COMM_NOAUCTION ) ) {
            SET_BIT( ch->comm, COMM_NOAUCTION );
            send_to_char( "Auction channel is now off.\n\r", ch );
            buffer_free( buf );
            return;
        } else {
            send_to_char( "Auction channel is already off.\n\r", ch );
            buffer_free( buf );
            return;
        }
    }

    /* finally... */

    /* does char have the item? */
    obj = get_obj_carry( ch, arg1 );

    if ( obj == NULL ) {
        send_to_char( "You aren't carrying that.\n\r", ch );
        buffer_free( buf );
        return;
    }

    /* added by Rahl */
    if ( IS_OBJ_STAT( obj, ITEM_NODROP ) ) {
        send_to_char( "You can't auction items you can't drop.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( auction->item == NULL ) {
        switch( obj->item_type ) {
            default:
                act( "You cannot auction items of type $T.", ch, NULL,
                    item_type_name( obj ), TO_CHAR );
                buffer_free( buf );
                return;

            case ITEM_WEAPON:
            case ITEM_ARMOR:
            case ITEM_STAFF:
            case ITEM_WAND:
            case ITEM_SCROLL:
                obj_from_char( obj );
                auction->item = obj;
                auction->bet = 0;
                auction->buyer = NULL;
                auction->seller = ch;
                auction->pulse = PULSE_AUCTION;
                auction->going = 0;
                bprintf( buf, "`mA new item has been received: %s`m.`w",
                    obj->short_descr );
                talk_auction( buf->data );
                buffer_free( buf );
                return;
          } /* switch */
    } else {
        act( "Try again later - $p is being auctioned right now!", ch,
            auction->item, NULL, TO_CHAR );
        buffer_free( buf );
        return;
    }
    buffer_free( buf );
    return;
}

/* added by Rahl */
/*
 * this function sends raw argument over the AUCTION: channel
 * I am not too sure if this method is right...
 */
void talk_auction( char *argument ) {
    DESCRIPTOR_DATA *d;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    // Commented out - not sure it does anything
    //CHAR_DATA *original;

    bprintf( buf, "`mAUCTION: %s`w\n\r", argument );

    for ( d = descriptor_list; d != NULL; d = d->next ) {
        //original = d->original ? d->original : d->character; /* if switched */
        if ( ( d->connected == CON_PLAYING ) && 
           !IS_SET( d->character->comm, COMM_NOAUCTION ) &&
           !IS_SET( d->character->comm, COMM_QUIET ) ) {
            send_to_char( buf->data, d->character );
        }
    }
    buffer_free( buf );
    return;
}

/* 
 * code by James Seldon (jseldon@cariboo.bc.ca)
 * added here by Rahl
 */
void show_time( CHAR_DATA *ch ) {
    long h, m, s = 0;
    BUFFER *buf;

    buf = buffer_new( 100 );

    s = current_time - ch->logon;

    h = s / 3600;
    s -= h * 3600;
    m = s / 60;
    s -= m * 60;
   
    bprintf( buf, "You were on for %ld hours, %ld minutes, and %ld"
                  " seconds.\n\r\n\r", h, m, s );
    send_to_char( buf->data, ch );

    buffer_free( buf );
    return;
}

void do_gmote( CHAR_DATA *ch, char *argument ) {
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
        send_to_char( "You can't emote.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Gmote what?\n\r", ch );
        return;
    }

    act( "`cGmote: `w$n $T`w", ch, NULL, argument, TO_CHAR );

    for ( d = descriptor_list; d != NULL; d = d->next ) {
        victim = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_QUIET) ) {
            act_new( "`cGmote: `w$n $t`w", 
                ch, argument, d->character, TO_VICT, POS_RESTING );
        }
    }

    return;
}

void do_pmote( CHAR_DATA *ch, char *argument ) {
    CHAR_DATA *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }

    act( "$n $t", ch, argument, NULL, TO_CHAR );

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr(argument,vch->name)) == NULL) {
            act("$N $t",vch,argument,ch,TO_CHAR);
            continue;
        }

        strcpy(temp, argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = vch->name;

        for (; *letter != '\0'; letter++) {
            if (*letter == '\'' && matches == strlen(vch->name)) {
                strcat(temp,"r");
                continue;
            }

            if (*letter == 's' && matches == strlen(vch->name)) {
                matches = 0;
                continue;
            }

            if (matches == strlen(vch->name)) {
                matches = 0;
            }

            if (*letter == *name) {
                matches++;
                name++;
                if (matches == strlen(vch->name)) {
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
            last[0] = '\0';
            name = vch->name;
        }

        act("$N $t",vch,temp,ch,TO_CHAR);
    }

    return;
}

/* Quote channel added by Raven */
void do_quote_channel( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_STRING_LENGTH );
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0' ) {
        if (IS_SET(ch->comm,COMM_NOQUOTE)) {
            send_to_char("`YQuote channel is now ON.\n\r`w",ch);
            REMOVE_BIT(ch->comm,COMM_NOQUOTE);
        } else {
            send_to_char("`YQuote channel is now OFF.\n\r`w",ch);
            SET_BIT(ch->comm,COMM_NOQUOTE);
        }
    } else  { 
        if (IS_SET(ch->comm,COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r",ch);
            buffer_free( buf );
            return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          buffer_free( buf );
          return;
        }

        REMOVE_BIT(ch->comm,COMM_NOQUOTE);
 
        bprintf( buf, "`YYou quote '%s`Y'`w\n\r", argument );
        send_to_char( buf->data, ch );

        for ( d = descriptor_list; d != NULL; d = d->next ) {
            CHAR_DATA *victim;
 
            victim = d->original ? d->original : d->character;
 
            if ( d->connected == CON_PLAYING &&
                 d->character != ch &&
                 !IS_SET(victim->comm,COMM_NOQUOTE) &&
                 !IS_SET(victim->comm,COMM_QUIET) ) {
                 act_new( "`Y$n quotes '$t`Y'`w", 
                    ch,argument, d->character, TO_VICT,POS_SLEEPING );
            }
        }
    }
    buffer_free( buf );
    return;
}


