///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/* Healer code written for Merc 2.0 muds by Alander 
   direct questions or comments to rtaylor@cie-2.uoregon.edu
   any use of this code must include this header */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"
#include "magic.h"

void do_heal(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH];
    int cost,sn;
    SPELL_FUN *spell;
    char *words;        

    /* check for healer */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && IS_SET(mob->act, ACT_IS_HEALER) )
            break;
    }
 
    if ( mob == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        return;
    }

    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        /* display price list */
        /* restore mana used to be 1000 gold - changed by Rahl */
        /* infuse and flux added by Rahl */

        act("$N says 'I offer the following spells:'",ch,NULL,mob,TO_CHAR);
        send_to_char("  light: cure light wounds        25 gold\n\r",ch);
        send_to_char("  serious: cure serious wounds    75 gold\n\r",ch);
        send_to_char("  critic: cure critical wounds   150 gold\n\r",ch);
        send_to_char("  heal: healing spell            500 gold\n\r",ch);
        send_to_char("  infuse: mega heal             1000 gold\n\r",ch);
        send_to_char("  blind: cure blindness           50 gold\n\r",ch);
        send_to_char("  disease: cure disease          100 gold\n\r",ch);
        send_to_char("  poison:  cure poison           150 gold\n\r",ch); 
        send_to_char("  uncurse: remove curse          200 gold\n\r",ch);
        send_to_char("  refresh: restore movement       50 gold\n\r",ch);
        send_to_char("  mana:  restore mana            500 gold\n\r",ch);
        send_to_char("  flux:  mega mana              1250 gold\n\r",ch);
        send_to_char(" Type heal <type> to be healed.\n\r",ch);
        return;
    }

    switch (arg[0])
    {
        case 'l' :
            spell = spell_cure_light;
            sn    = skill_lookup("cure light");
            words = "judicandus dies";
            cost  = 25;
            break;

        case 's' :
            spell = spell_cure_serious;
            sn    = skill_lookup("cure serious");
            words = "judicandus gzfuajg";
            cost  = 75;
            break;

        case 'c' :
            spell = spell_cure_critical;
            sn    = skill_lookup("cure critical");
            words = "judicandus qfuhuqar";
            cost  = 150;
            break;

        case 'h' :
            spell = spell_heal;
            sn = skill_lookup("heal");
            words = "pzar";
            cost  = 500;
            break;

        /* infuse/mega heal by Rahl */
        case 'i' :
            spell = spell_mega_heal;
            sn = skill_lookup( "mega heal" );
            words = "infuse";
            cost = 1000;
            break;

        case 'b' :
            spell = spell_cure_blindness;
            sn    = skill_lookup("cure blindness");
            words = "judicandus noselacri";             
            cost  = 50;
            break;

        case 'd' :
            spell = spell_cure_disease;
            sn    = skill_lookup("cure disease");
            words = "judicandus eugzagz";
            cost = 100;
            break;

        case 'p' :
            spell = spell_cure_poison;
            sn    = skill_lookup("cure poison");
            words = "judicandus sausabru";
            cost  = 150;
            break;
        
        case 'u' :
            spell = spell_remove_curse; 
            sn    = skill_lookup("remove curse");
            words = "candussido judifgz";
            cost  = 200;
            break;

        case 'r' :
            spell =  spell_refresh;
            sn    = skill_lookup("refresh");
            words = "candusima"; 
            cost  = 50;
            break;

        case 'm' :
            spell = NULL;
            sn = -1;
            words = "energizer";
          /* used to be 1000 gold - changed by Rahl */
            cost = 500;
            break;

        /* flux/mega mana by Rahl */
        case 'f' :
            spell = spell_mega_mana;
            sn = skill_lookup("mega mana");
            words = "flux";
            cost = 1250;
            break;

        default :
            act("$N says 'Type 'heal' for a list of spells.'",
                ch,NULL,mob,TO_CHAR);
            return;
    }

    if (cost > ch->gold)
    {
        act("$N says 'You do not have enough gold for my services.'",
            ch,NULL,mob,TO_CHAR);
        return;
    }

    WAIT_STATE(ch,PULSE_VIOLENCE);

    ch->gold -= cost;
    mob->gold += cost;
    act("$n utters the words '$T'.",mob,NULL,words,TO_ROOM);
  
    if (spell == NULL)  /* restore mana trap...kinda hackish */
    {
        /* 
         * I don't think the healer heals enough mana so I'm gonna 
         * change it. Rahl 
         */
/*
 *      ch->mana += dice(2,8) + mob->level / 4;
 */
        /*
         * New mana gain by Rahl.
         */
        ch->mana += 100;

        ch->mana = UMIN(ch->mana,ch->max_mana);
        send_to_char("A warm glow passes through you.\n\r",ch);
        return;
     }

     if (sn == -1)
        return;
    
     spell(sn,mob->level,mob,ch, TARGET_CHAR);
}
