///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/**************************************************************************
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
#include <ctype.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "interp.h"

/* color added by Rahl */
char *  const   where_name      [] =
{
    "`c<used as light>    `w ",
    "`c<worn on finger>   `w ",
    "`c<worn on finger>   `w ",
    "`c<worn around neck> `w ",
    "`c<worn around neck> `w ",
    "`c<worn on body>     `w ",
    "`c<worn on head>     `w ",
    "`c<worn on legs>     `w ",
    "`c<worn on feet>     `w ",
    "`c<worn on hands>    `w ",
    "`c<worn on arms>     `w ",
    "`c<worn as shield>   `w ",
    "`c<worn about body>  `w ",
    "`c<worn about waist> `w ",
    "`c<worn around wrist>`w ",
    "`c<worn around wrist>`w ",
    "`c<wielded>          `w ",
    "`c<held>             `w ",
    "`c<secondary wield>  `w "
};


/*
 * Local functions.
 */
char *  format_obj_to_char      ( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort );
void    show_list_to_char       ( OBJ_DATA *list, CHAR_DATA *ch,
                                    bool fShort, bool fShowNothing );
void    show_char_to_char_0     ( CHAR_DATA *victim, CHAR_DATA *ch );
void    show_char_to_char_1     ( CHAR_DATA *victim, CHAR_DATA *ch );
void    show_char_to_char       ( CHAR_DATA *list, CHAR_DATA *ch );
bool    check_blind             ( CHAR_DATA *ch );



char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
    /* color added by Rahl */
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';
    if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "`W(Invis)`g " );
    if ( IS_AFFECTED(ch, AFF_DETECT_EVIL)
         && IS_OBJ_STAT(obj, ITEM_EVIL)   )   strcat( buf, "`R(Red Aura)`g " );
    /* blue aura added by Rahl */
    if ( IS_AFFECTED( ch, AFF_DETECT_GOOD )
        && IS_OBJ_STAT( obj, ITEM_BLESS ) )   strcat( buf, "`B(Blue Aura)`g " );
    if ( IS_AFFECTED(ch, AFF_DETECT_MAGIC)
         && IS_OBJ_STAT(obj, ITEM_MAGIC)  )   strcat( buf, "`C(Magical)`g " );
    if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   strcat( buf, "`Y(Glowing)`g " );
    if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "`M(Humming)`g " );

    strcat( buf, "`g" );

    if ( fShort )
    {
        if ( obj->short_descr != NULL )
            strcat( buf, obj->short_descr );
    }
    else
    {
        if ( obj->description != NULL )
            strcat( buf, obj->description );
    }

    strcat( buf, "`w" );

    return buf;
}



/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *output;
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;

    if ( ch->desc == NULL )
    {
        buffer_free( buf );
        return;
    }

    /*
     * Alloc space for output lines.
     */
    output = buffer_new( MAX_INPUT_LENGTH );     

    count = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
        count++;
    prgpstrShow = malloc( count * sizeof(char *) );
    prgnShow    = malloc( count * sizeof(int) );
    nShow       = 0;

    /*
     * Format the list of objects.
     */
    for ( obj = list; obj != NULL; obj = obj->next_content )
    { 
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
        {
            pstrShow = format_obj_to_char( obj, ch, fShort );
            fCombine = FALSE;

            if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
            {
                /*
                 * Look for duplicates, case sensitive.
                 * Matches tend to be near end so run loop backwords.
                 */
                for ( iShow = nShow - 1; iShow >= 0; iShow-- )
                {
                    if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
                    {
                        prgnShow[iShow]++;
                        fCombine = TRUE;
                        break;
                    }
                }
            }

            /*
             * Couldn't combine, or didn't want to.
             */
            if ( !fCombine )
            {
                prgpstrShow [nShow] = str_dup( pstrShow );
                prgnShow    [nShow] = 1;
                nShow++;
            }
        }
    }

    /*
     * Output the formatted list.
     */
    for ( iShow = 0; iShow < nShow; iShow++ )
    {
        if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
        {
            if ( prgnShow[iShow] != 1 )
            {
                bprintf( buf, "`g(%2d) ", prgnShow[iShow] );
                buffer_strcat( output, buf->data );
            }
            else
            {
                buffer_strcat( output, "`g     " );
            }
        }
        buffer_strcat( output, prgpstrShow[iShow] );
        buffer_strcat( output, "\n\r`w" );
        free_string( prgpstrShow[iShow] );
    }

    if ( fShowNothing && nShow == 0 )
    {
        if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
            send_to_char( "`g     ", ch );
        send_to_char( "Nothing.\n\r`w", ch );
    }

    page_to_char( output->data, ch );
    /*
     * Clean up.
     */
    buffer_free( buf );
    buffer_free( output );
    free( prgpstrShow );
    free( prgnShow );

    return;
}



void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
   BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
   
   /* Target flag added by Rahl for autoquesting */
   if (IS_NPC(victim) && ch->questmob > 0 && 
       victim->pIndexData->vnum == ch->questmob)
           buffer_strcat( buf, "`M<-TARGET-> `C" );
   if ( IS_AFFECTED(victim, AFF_INVISIBLE)   ) 
        buffer_strcat( buf, "`W(Invis) `C"      );
   if ( IS_SET(victim->act, PLR_WIZINVIS) && !IS_NPC(victim)   ) 
        buffer_strcat( buf, "`B(Wizi) `C"            );
   /* incog added by Rahl */
   if ( IS_SET( victim->act, PLR_INCOGNITO ) && !IS_NPC( victim ) ) 
        buffer_strcat( buf, "`C(Incog) `C"      );
   if ( IS_AFFECTED(victim, AFF_HIDE)        ) 
        buffer_strcat( buf, "`K(Hide) `C"       );
   if ( IS_AFFECTED(victim, AFF_CHARM)       ) 
        buffer_strcat( buf, "`Y(Charmed) `C"    );
   if ( IS_AFFECTED(victim, AFF_PASS_DOOR)   ) 
        buffer_strcat( buf, "`w(Translucent) `C");
   if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) ) 
        buffer_strcat( buf, "`R(Pink Aura) `C"  );
   if ( IS_EVIL(victim) &&   IS_AFFECTED(ch, AFF_DETECT_EVIL)     ) 
        buffer_strcat( buf, "`R(Red Aura) `C"   );
    /* detect good added by Rahl */
   if ( IS_GOOD( victim ) && IS_AFFECTED( ch, AFF_DETECT_GOOD )) 
        buffer_strcat( buf, "`Y(Golden Aura) `C" );
   if ( IS_AFFECTED(victim, AFF_SANCTUARY)   ) 
        buffer_strcat( buf, "`W(White Aura) `C" );
   if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_AFK ) )
        buffer_strcat( buf, "`W(AFK) `C"     );
   if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER ) )
        buffer_strcat( buf, "`R(PK) `C"     );
   if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF  ) )
        buffer_strcat( buf, "`K(THIEF) `C"      );
   /* bounty hunter and quiet added by Rahl */
   if ( !IS_NPC(victim) && IS_SET(victim->comm, COMM_QUIET ) )
        buffer_strcat( buf, "`Y(QUIET) `C"      );
   if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_BOUNTY_HUNTER ) )
        buffer_strcat( buf, "`M(BH) `C"         );   

   if ( victim->position == victim->start_pos && victim->long_descr[0] != '\0' )
     {
        buffer_strcat( buf, "`c" );
        buffer_strcat( buf, victim->long_descr );
    }
   if ( IS_AFFECTED(victim, AFF_WEB) && IS_NPC(victim) 
       && victim->position == victim->start_pos) 
     {
        buffer_strcat( buf, "`g" );
        buffer_strcat( buf, PERS( victim, ch ) );
        buffer_strcat( buf, " is covered in sticky webs.\n\r`w");
     }
   if ( victim->position == victim->start_pos && victim->long_descr[0] != '\0' )
     {
        send_to_char( buf->data, ch );
        send_to_char( "`w", ch);
        buffer_free( buf );
        return;
     }
   
   
    buffer_strcat( buf, PERS( victim, ch ) );
   if ( !IS_NPC(victim) && !IS_SET(ch->comm, COMM_BRIEF) )
     buffer_strcat( buf, victim->pcdata->title );
   
   switch ( victim->position )
     {
      case POS_DEAD:     
        buffer_strcat( buf, " `Cis DEAD!!" ); 
        break;
      case POS_MORTAL:   
        buffer_strcat( buf, " `Cis mortally wounded." );   
        break;
      case POS_INCAP:    
        buffer_strcat( buf, " `Cis incapacitated." );      
        break;
      case POS_STUNNED:  
        buffer_strcat( buf, " `Cis lying here stunned." ); 
        break;
      case POS_SLEEPING: 
        buffer_strcat( buf, " `Cis sleeping here." );
        break;
      case POS_RESTING:  
        buffer_strcat( buf, " `Cis resting here." ); 
        break;
      case POS_SITTING:  
        buffer_strcat( buf, " `Cis sitting here." );
        break;
      case POS_STANDING: 
        buffer_strcat( buf, " `Cis here." ); 
        break;
      case POS_FIGHTING:
        buffer_strcat( buf, " `Cis here, fighting " );
        if ( victim->fighting == NULL )
          buffer_strcat( buf, "`Cthin air??" );
        else if ( victim->fighting == ch )
          buffer_strcat( buf, "`CYOU!" );
        else if ( victim->in_room == victim->fighting->in_room )
          {
             buffer_strcat( buf, PERS( victim->fighting, ch ) );
             buffer_strcat( buf, ".`w" );
          }
        else
          buffer_strcat( buf, "`Csomone who left??`w" );
        break;
     }
   if ( IS_AFFECTED(victim, AFF_WEB)   ) 
     {
        buffer_strcat( buf, "\n\r`g" );
        buffer_strcat( buf, PERS( victim, ch ) );
        buffer_strcat( buf, " is covered in sticky webs.`w");
     }
   buffer_strcat( buf, "\n\r`w" );
   buf->data[0] = UPPER(buf->data[0]);
   send_to_char( "`C", ch );
   send_to_char( buf->data, ch );
   buffer_free( buf );
   return;
}



void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    OBJ_DATA *obj;
    int iWear;
    int percent;
    bool found;

    if ( can_see( victim, ch ) )
    {
        if (ch == victim)
            act( "$n looks at $mself.",ch,NULL,NULL,TO_ROOM);
        else
        {
            act( "$n looks at you.", ch, NULL, victim, TO_VICT    );
            act( "$n looks at $N.",  ch, NULL, victim, TO_NOTVICT );
        }
    }

   if ( victim->description[0] != '\0' )
     {
        /* was send_to_char */
        page_to_char( victim->description, ch );
        send_to_char( "`w", ch );
     }
    else
    {
        act( "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
    }

    if ( victim->max_hit > 0 )
        percent = ( 100 * victim->hit ) / victim->max_hit;
    else
        percent = -1;
/*
    strcpy( buf, PERS(victim, ch) );
*/
    bprintf( buf, PERS( victim, ch ) );
    if (percent >= 100) 
        buffer_strcat( buf, " is in excellent condition.\n\r");
    else if (percent >= 90) 
        buffer_strcat( buf, " has a few scratches.\n\r");
    else if (percent >= 75) 
        buffer_strcat( buf," has some small wounds and bruises.\n\r");
    else if (percent >=  50) 
        buffer_strcat( buf, " has quite a few wounds.\n\r");
    else if (percent >= 30)
        buffer_strcat( buf, " has some big nasty wounds and scratches.\n\r");
    else if (percent >= 15)
        buffer_strcat ( buf, " looks pretty hurt.\n\r");
    else if (percent >= 0 )
        buffer_strcat (buf, " is in awful condition.\n\r");
    else
        buffer_strcat(buf, " is `Rbleeding to death.`w\n\r");

    buf->data[0] = UPPER(buf->data[0]);
    send_to_char( buf->data, ch );

    found = FALSE;
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
        if ( ( obj = get_eq_char( victim, iWear ) ) != NULL
        &&   can_see_obj( ch, obj ) )
        {
            if ( !found )
            {
                send_to_char( "\n\r", ch );
                act( "$N is using:", ch, NULL, victim, TO_CHAR );
                found = TRUE;
            }
            send_to_char( where_name[iWear], ch );
            send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
            send_to_char( "\n\r", ch );
        }
    }

    if ( victim != ch
    &&   !IS_NPC(ch)
    &&   number_percent( ) < ch->pcdata->learned[gsn_peek] )
    {
        send_to_char( "\n\rYou peek at the inventory:\n\r", ch );
        check_improve(ch,gsn_peek,TRUE,4);
        show_list_to_char( victim->carrying, ch, TRUE, TRUE );
    }

    buffer_free( buf );
    return;
}



void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
    CHAR_DATA *rch;

    for ( rch = list; rch != NULL; rch = rch->next_in_room )
    {
        if ( rch == ch )
            continue;

        if ( !IS_NPC(rch)
        &&   IS_SET(rch->act, PLR_WIZINVIS)
        &&   char_getImmRank( ch ) < rch->invis_level )
            continue;

        if ( can_see( ch, rch ) )
        {
            show_char_to_char_0( rch, ch );
        }
        else if ( room_is_dark( ch->in_room )
        &&        IS_AFFECTED(rch, AFF_INFRARED ) )
        {
            send_to_char( "`RYou see glowing red eyes watching YOU!\n\r`w", ch );
        }
    }

    return;
} 



bool check_blind( CHAR_DATA *ch )
{

    if (!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT))
        return TRUE;

    if ( IS_AFFECTED(ch, AFF_BLIND) )
    { 
        send_to_char( "You can't see a thing!\n\r", ch ); 
        return FALSE; 
    }

    return TRUE;
}

/* changes your scroll */
void do_scroll(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( 100 );
    int lines;

    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        if (ch->lines == 0)
            send_to_char("You do not page long messages.\n\r",ch);
        else
        {
            bprintf(buf,"You currently display %d lines per page.\n\r",
                    ch->lines + 2);
            send_to_char(buf->data,ch);
        }
        buffer_free( buf );
        return;
    }

    if (!is_number(arg))
    {
        send_to_char("You must provide a number.\n\r",ch);
        buffer_free( buf );
        return;
    }

    lines = atoi(arg);

    if (lines == 0)
    {
        send_to_char("Paging disabled.\n\r",ch);
        ch->lines = 0;
        buffer_free( buf );
        return;
    }

    if (lines < 10 || lines > 100)
    {
        send_to_char("You must provide a reasonable number.\n\r",ch);
        buffer_free( buf );
        return;
    }

    bprintf(buf,"Scroll set to %d lines.\n\r",lines);
    send_to_char(buf->data,ch);
    buffer_free( buf );
    ch->lines = lines - 2;
    return;
}

/* RT does socials */
void do_socials(CHAR_DATA *ch, char *argument)
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    int iSocial;
    int col;
/* added by Rahl so we can use page_to_char */
    BUFFER *buffer;
     
    buffer = buffer_new( MAX_INPUT_LENGTH );

    col = 0;
   
    for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
    {
        bprintf(buf,"%-12s",social_table[iSocial].name);
        /* changed by Rahl so we can use page_to_char */
        buffer_strcat( buffer, buf->data );
        if (++col % 6 == 0)
            buffer_strcat( buffer, "\n\r" );
    }

    if ( col % 6 != 0)
        buffer_strcat( buffer, "\n\r" );

    page_to_char( buffer->data, ch );
    buffer_free( buf );
    buffer_free( buffer );

    return;
}


 
/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"motd");
}

void do_imotd(CHAR_DATA *ch, char *argument)
{  
    do_help(ch,"imotd");
}

void do_rules(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"rules");
}

void do_wizlist(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void do_autolist(CHAR_DATA *ch, char *argument)
{
    /* lists most player flags */
    if (IS_NPC(ch))
      return;

    send_to_char("   `Waction     status`w\n\r",ch);
    send_to_char("`K---------------------`w\n\r",ch);
 
    send_to_char("autoassist     ",ch);
    if (IS_SET(ch->act,PLR_AUTOASSIST))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch); 

    send_to_char("autoexit       ",ch);
    if (IS_SET(ch->act,PLR_AUTOEXIT))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch);

    send_to_char("autogold       ",ch);
    if (IS_SET(ch->act,PLR_AUTOGOLD))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch);

    send_to_char("autoloot       ",ch);
    if (IS_SET(ch->act,PLR_AUTOLOOT))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch);

    send_to_char("autosac        ",ch);
    if (IS_SET(ch->act,PLR_AUTOSAC))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch);

    send_to_char("autosplit      ",ch);
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch);

    /* autodamage added by Rahl */
    send_to_char("autodamage     ", ch );
    if (IS_SET(ch->act, PLR_AUTODAMAGE))
        send_to_char("`GON`w\n\r", ch );
    else
        send_to_char("`ROFF`w\n\r", ch );

    send_to_char("prompt         ",ch);
    if (IS_SET(ch->comm,COMM_PROMPT))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch);

    send_to_char("combine items  ",ch);
    if (IS_SET(ch->comm,COMM_COMBINE))
        send_to_char("`GON`w\n\r",ch);
    else
        send_to_char("`ROFF`w\n\r",ch);

    if (!IS_SET(ch->act,PLR_CANLOOT))
        send_to_char("Your corpse is safe from thieves.\n\r",ch);
    else 
        send_to_char("Your corpse may be looted.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOSUMMON))
        send_to_char("You cannot be summoned.\n\r",ch);
    else
        send_to_char("You can be summoned.\n\r",ch);
   
    if (IS_SET(ch->act,PLR_NOFOLLOW))
        send_to_char("You do not welcome followers.\n\r",ch);
    else
        send_to_char("You accept followers.\n\r",ch);

    /* nobounty added by Rahl */
/*
    if ( IS_SET( ch->act, PLR_NOBOUNTY ) )
        send_to_char( "Bounties may not be placed on your head.\n\r", ch );
    else
        send_to_char( "Bounties may be placed on your head.\n\r", ch );
*/

        
    if (IS_SET(ch->act,PLR_COLOR))
        send_to_char("You have ansi `Yc`Ro`Bl`Co`Gr`w turned `Yon`w.\n\r",ch);
    else
        send_to_char("You have ansi color turned off.\n\r",ch);
}

void do_autoassist(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
    
    if (IS_SET(ch->act,PLR_AUTOASSIST))
    {
      send_to_char("Autoassist removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOASSIST);
    }
    else
    {
      send_to_char("You will now assist when needed.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOASSIST);
    }
}

void do_autoexit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOEXIT))
    {
      send_to_char("Exits will no longer be displayed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOEXIT);
    }
    else
    {
      send_to_char("Exits will now be displayed.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOEXIT);
    }
}

void do_autogold(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOGOLD))
    {
      send_to_char("Autogold removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOGOLD);
    }
    else
    {
      send_to_char("Automatic gold looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOGOLD);
    }
}

void do_autoloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOLOOT))
    {
      send_to_char("Autolooting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOLOOT);
    }
    else
    {
      send_to_char("Automatic corpse looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOLOOT);
    }
}

void do_autosac(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOSAC))
    {
      send_to_char("Autosacrificing removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOSAC);
    }
    else
    {
      send_to_char("Automatic corpse sacrificing set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOSAC);
    }
}

void do_autosplit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
    {
      send_to_char("Autosplitting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOSPLIT);
    }
    else
    {
      send_to_char("Automatic gold splitting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOSPLIT);
    }
}

void do_brief(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_BRIEF))
    {
      send_to_char("Full descriptions activated.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_BRIEF);
    }
    else
    {
      send_to_char("Short descriptions activated.\n\r",ch);
      SET_BIT(ch->comm,COMM_BRIEF);
    }
}

void do_compact(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMPACT))
    {
      send_to_char("Compact mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMPACT);
    }
    else
    {
      send_to_char("Compact mode set.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMPACT);
    }
}

void do_prompt(CHAR_DATA *ch, char *argument)
{
   if (!IS_NPC(ch)) 
   {
        free_string( ch->pcdata->prompt );
        if( !strcmp( argument, "default" ) )
            ch->pcdata->prompt = str_dup( "%i`K/`W%H`w HP %n`K/`W%M`w MP %w`K/`W%V`w MV `K> ");

        else if( !strcmp( argument, "combat" ) )
             ch->pcdata->prompt = str_dup( "`gTank: %l  `rEnemy: %e%r`w%i`K/`W%H `wHP %n`K/`W%M `wMP %w`K/`W%V `wMV `K>" );

        else
        {
           smash_tilde( argument );
           ch->pcdata->prompt = strdup( argument );
        }
     send_to_char("Prompt set.\n\r",ch);
     return;
   }
   else send_to_char("Mobiles may not change thier prompts.\n\r",ch);
}

void do_combine(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMBINE))
    {
      send_to_char("Long inventory selected.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMBINE);
    }
    else
    {
      send_to_char("Combined inventory selected.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMBINE);
    }
}

void do_noloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_CANLOOT))
    {
      send_to_char("Your corpse is now safe from thieves.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_CANLOOT);
    }
    else
    {
      send_to_char("Your corpse may now be looted.\n\r",ch);
      SET_BIT(ch->act,PLR_CANLOOT);
    }
}

void do_nofollow(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_NOFOLLOW))
    {
      send_to_char("You now accept followers.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    }
    else
    {
      send_to_char("You no longer accept followers.\n\r",ch);
      SET_BIT(ch->act,PLR_NOFOLLOW);
      die_follower( ch );
    }
}

void do_nosummon(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
      if (IS_SET(ch->imm_flags,IMM_SUMMON))
      {
        send_to_char("You are no longer immune to summon.\n\r",ch);
        REMOVE_BIT(ch->imm_flags,IMM_SUMMON);
      }
      else
      {
        send_to_char("You are now immune to summoning.\n\r",ch);
        SET_BIT(ch->imm_flags,IMM_SUMMON);
      }
    }
    else
    {
      if (IS_SET(ch->act,PLR_NOSUMMON))
      {
        send_to_char("You are no longer immune to summon.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_NOSUMMON);
      }
      else
      {
        send_to_char("You are now immune to summoning.\n\r",ch);
        SET_BIT(ch->act,PLR_NOSUMMON);
      }
    }
}

void do_nocolor(CHAR_DATA *ch, char *argument)
{
  if (IS_SET(ch->act,PLR_COLOR))
  {
    send_to_char("You no longer see in color.\n\r",ch);
    REMOVE_BIT(ch->act,PLR_COLOR);
  }
  else
  {
    send_to_char("You can see in `Yc`Ro`Bl`Co`Gr`w.\n\r",ch);
    SET_BIT(ch->act,PLR_COLOR);
  }
}

void do_afk(CHAR_DATA *ch, char *argument)
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( IS_NPC( ch ) )
    {
        send_to_char( "You can't do that!\n\r", ch );
        buffer_free( buf );
        return;
    }
  
  if (IS_SET(ch->act,PLR_AFK))
  {
    send_to_char("You are no longer set AFK.\n\r",ch);
    REMOVE_BIT(ch->act,PLR_AFK);
    act( "`R$n returns to $s keyboard.`w",ch,NULL,NULL,TO_ROOM);
    bprintf(buf, "%s is no-longer AFK.", ch->name);
     if (!IS_SET(ch->act,PLR_WIZINVIS) && !IS_SET( ch->act, PLR_INCOGNITO ))
        do_sendinfo(ch, buf->data);
    wiznet( "$N is no longer AFK.", ch, NULL, WIZ_AFK, 0, char_getImmRank(ch) );
    if ( ch->pcdata->message_ctr )
    {
        bprintf( buf, "You have %d messages waiting.\n\r",
            ch->pcdata->message_ctr );
        send_to_char( buf->data, ch ); 
     }
	if ( ch->pcdata->away_message != NULL ) {
		free_string( ch->pcdata->away_message );
		ch->pcdata->away_message = NULL;
	}
  }
  else
  {
    send_to_char("You are now set AFK.\n\r",ch);
    SET_BIT(ch->act,PLR_AFK);
    act( "`W$n is away from $s keyboard for awhile.`w",ch,NULL,NULL,TO_ROOM);
    bprintf(buf, "%s has gone AFK.", ch->name);
    if (!IS_SET(ch->act,PLR_WIZINVIS) && !IS_SET( ch->act, PLR_INCOGNITO ))
        do_sendinfo(ch, buf->data);
    wiznet( "$N has gone AFK.", ch, NULL, WIZ_AFK, 0, char_getImmRank(ch) );
	if ( argument[0] != '\0' ) {
		if ( ch->pcdata->away_message != NULL ) {
			free_string( ch->pcdata->away_message );
		}
		ch->pcdata->away_message = str_dup( argument );
		strcat( ch->pcdata->away_message, "\n\r" );
		bprintf( buf, "Your away message has been set to: %s\n\r",
			argument );
		send_to_char( buf->data, ch );
	}
   }
   buffer_free( buf );
   return;
}

void do_pk(CHAR_DATA *ch, char *argument)
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
   
    if (IS_NPC(ch))
    {
        buffer_free( buf );
        return;
    }  

   if (ch->pcdata->confirm_pk)
   {
        if (argument[0] != '\0')
        {
            send_to_char("PK readiness status removed.\n\r",ch);
            ch->pcdata->confirm_pk = FALSE;
            buffer_free( buf );
            return;
        }
        else
        {
            if (IS_SET(ch->act,PLR_KILLER)) 
            {
                buffer_free( buf );
                return;
            }
            SET_BIT(ch->act,PLR_KILLER);
            act("`R$n glows briefly with a red aura, you get the feeling you should keep your distance.`w",
            ch,NULL,NULL,TO_ROOM);
            send_to_char("`RYou are now a Player Killer, good luck, you'll need it.\n\r`w", ch);
            bprintf(buf, "%s has become a player killer!", ch->name);
            do_sendinfo(ch, buf->data);
            buffer_free( buf );
            return;
        }
    }

    if (argument[0] != '\0')
    {
        send_to_char("Just type pk. No argument.\n\r",ch);
        buffer_free( buf );
        return;
    }

    send_to_char("Type pk again to confirm this command.\n\r",ch);
    send_to_char("WARNING: this command is virtually irreversible.\n\r",ch);
    send_to_char("If you don't know what pk is for read help pk, DON'T type this command again.\n\r",ch);
    send_to_char("Typing pk with an argument will undo pk readiness status.\n\r",
        ch);
    ch->pcdata->confirm_pk = TRUE;
    buffer_free( buf );
    return;
}


void eval_dir(char *dir, int mov_dir, int num,CHAR_DATA *ch, int *see, ROOM_INDEX_DATA *first_room)
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *mob_in_room;

   for ( mob_in_room=first_room->people; mob_in_room != NULL ;mob_in_room=mob_in_room->next_in_room)
   {
        if (can_see(ch, mob_in_room)) {
            if (*see == 0) {
                bprintf(buf, "  `W%s `wfrom you, you see:\n\r",dir);
                send_to_char(buf->data, ch);
                *see += 1;
            }
            if(!IS_NPC(mob_in_room)) {
                bprintf(buf, "     %s%s - %d %s\n\r",
                    mob_in_room->name,
                    mob_in_room->pcdata->title,num,dir);
                send_to_char(buf->data, ch);
            }
            else {
                bprintf(buf, "     %s - %d %s\n\r",
                    mob_in_room->short_descr,num,dir);
                send_to_char( buf->data, ch);
            }
        }
    }

    buffer_free( buf );
    return;
}

void show_dir_mobs(char *dir,int move_dir,CHAR_DATA *ch,int depth) {
    ROOM_INDEX_DATA *cur_room=ch->in_room;
    EXIT_DATA *pexit;
    int see=0;
    int i;
        
    for (i=1;( i<=depth && (pexit = cur_room->exit[move_dir])
             &&   pexit->u1.to_room
             &&   pexit->u1.to_room != cur_room )
             && !IS_SET(pexit->exit_info, EX_CLOSED);i++) {
             cur_room = pexit->u1.to_room;
             eval_dir(dir,move_dir,i,ch,&see,cur_room);
        }     
}

char *dir_text[]={"North","East","South","West","Up","Down"};

void do_scan(CHAR_DATA *ch, char *argument)
{
    int door;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *mob_in_room;

    for ( mob_in_room=ch->in_room->people ; mob_in_room != NULL ;
         mob_in_room=mob_in_room->next_in_room) {
            if (can_see(ch, mob_in_room)) {

                if(!IS_NPC(mob_in_room)) {
                    bprintf(buf, "     %s%s - right here.\n\r",
                        mob_in_room->name,mob_in_room->pcdata->title);
                     send_to_char(buf->data, ch); 
                }
                else {
                    bprintf(buf, "     %s - right here.\n\r",
                        mob_in_room->short_descr);
                     send_to_char( buf->data, ch); 
                }

         }

    }

    for ( door = 0; door <= 5; door++ )
        show_dir_mobs(dir_text[door],door,ch,3);
    act( "$n scans $s surroundings.", ch, NULL, NULL, TO_ROOM );
    buffer_free( buf );
    return;
}

/* should this be for detect magic only? */
void do_affects( CHAR_DATA *ch, char *argument)
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    AFFECT_DATA *paf;
    
    switch ( ch->position )
    {
    case POS_DEAD:     
        send_to_char( "You are DEAD!!\n\r",             ch );
        break;
    case POS_MORTAL:
        send_to_char( "You are mortally wounded.\n\r",  ch );
        break;
    case POS_INCAP:
        send_to_char( "You are incapacitated.\n\r",     ch );
        break;
    case POS_STUNNED:
        send_to_char( "You are stunned.\n\r",           ch );
        break;
    case POS_SLEEPING:
        send_to_char( "You are sleeping.\n\r",          ch );
        break;
    case POS_RESTING:
        send_to_char( "You are resting.\n\r",           ch );
        break;
    case POS_STANDING:
        send_to_char( "You are standing.\n\r",          ch );
        break;
    case POS_FIGHTING:
        send_to_char( "You are fighting.\n\r",          ch );
        break;
    /* added by Rahl */
    case POS_SITTING:
        send_to_char( "You are sitting.\n\r",           ch );
        break;
    }
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]   > 10 )
        send_to_char( "You are drunk.\n\r",   ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] ==  0 )
        send_to_char( "You are thirsty.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   ==  0 )
        send_to_char( "You are hungry.\n\r",  ch );
    if ( ch->affected != NULL )
    {
        send_to_char( "You are affected by:\n\r", ch );
        for ( paf = ch->affected; paf != NULL; paf = paf->next )
        {
            bprintf( buf, "Spell: '%s'", skill_table[paf->type].name );
            send_to_char( buf->data, ch );

/*
            bprintf( buf,
                " modifies %s by %d for %d hours",
                affect_loc_name( paf->location ),
                paf->modifier,
                paf->duration );
            send_to_char( buf->data, ch );
*/

			if ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) ) {
            	bprintf( buf,
                	" modifies %s by %d for %d hours",
                	affect_loc_name( paf->location ),
                	paf->modifier );
				send_to_char( buf->data, ch );
			}

            send_to_char( ".\n\r", ch );
        }
    }
    buffer_free( buf );
    return;
}


void do_look( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char *pdesc;
    int door;
    int number,count;

    if ( ch->desc == NULL )
    {
        buffer_free( buf );
        return;
    }

    if ( ch->position < POS_SLEEPING )
    {
        send_to_char( "You can't see anything but stars!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->position == POS_SLEEPING )
    {
        send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !check_blind( ch ) )
    {
        buffer_free( buf );
        return;
    }

    if ( !IS_NPC(ch)
    &&   !IS_SET(ch->act, PLR_HOLYLIGHT)
    /* added by Rahl */
    &&   !IS_AFFECTED( ch, AFF_DARK_VISION )
    &&   room_is_dark( ch->in_room ) )
    {
        send_to_char( "It is pitch black ... \n\r", ch );
        show_char_to_char( ch->in_room->people, ch );
        buffer_free( buf );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    number = number_argument(arg1,arg3);
    count = 0;

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {
        /* 'look' or 'look auto' */
        send_to_char( "`B", ch);
        send_to_char( ch->in_room->name, ch );
        send_to_char( "`w\n\r", ch );

        if ( arg1[0] == '\0'
        || ( !IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF) ) )
        {
            send_to_char( "  ",ch);
            send_to_char( ch->in_room->description, ch );
        }

        if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) )
        {
            send_to_char("\n\r`W",ch);
            do_exits( ch, "auto" );
            send_to_char("`w",ch);
        }

        show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE );
        show_char_to_char( ch->in_room->people,   ch );
        buffer_free( buf );
        return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
    {
        /* 'look in' */
        if ( arg2[0] == '\0' )
        {
            send_to_char( "Look in what?\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
        {
            send_to_char( "You do not see that here.\n\r", ch );
            buffer_free( buf );
            return;
        }

        switch ( obj->item_type )
        {
        default:
            send_to_char( "That is not a container.\n\r", ch );
            break;

        case ITEM_DRINK_CON:
            if ( obj->value[1] <= 0 )
            {
                send_to_char( "It is empty.\n\r", ch );
                break;
            }

            bprintf( buf, "It's %s full of a %s liquid.\n\r",
                obj->value[1] <     obj->value[0] / 4
                    ? "less than" :
                obj->value[1] < 3 * obj->value[0] / 4
                    ? "about"     : "more than",
                liq_table[obj->value[2]].liq_color
                );

            send_to_char( buf->data, ch );
            break;

        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            if ( IS_SET(obj->value[1], CONT_CLOSED) )
            {
                send_to_char( "It is closed.\n\r", ch );
                break;
            }

            act( "$p contains:", ch, obj, NULL, TO_CHAR );
            show_list_to_char( obj->contains, ch, TRUE, TRUE );
            break;
        }
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
        show_char_to_char_1( victim, ch );
        buffer_free( buf );
        return;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL ) {
                if (++count == number) {
                    send_to_char( pdesc, ch );
                    buffer_free( buf );
                    return;
                } else { 
					continue;
				}
			}

            pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
            if ( pdesc != NULL ) {
                if (++count == number) {       
                    send_to_char( pdesc, ch );
                    buffer_free( buf );
                    return;
                } else {
					continue;
				}
			}

            if ( is_name( arg3, obj->name ) )
                if (++count == number)
                {
                    send_to_char( obj->description, ch );
                    send_to_char( "\n\r",ch);
                    buffer_free( buf );
                    return;
                }
        }
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
                if (++count == number)
                {
                    send_to_char( pdesc, ch );
                    buffer_free( buf );
                    return;
                }

            pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
            if ( pdesc != NULL )
                if (++count == number)
                {
                    send_to_char( pdesc, ch );
                    buffer_free( buf );
                    return;
                }
        }

        if ( is_name( arg3, obj->name ) )
            if (++count == number)
            {
                send_to_char( obj->description, ch );
                send_to_char("\n\r",ch);
                buffer_free( buf );
                return;
            }
    }
    
    if (count > 0 && count != number)
    {
        if (count == 1)
            bprintf(buf,"You only see one %s here.\n\r",arg3);
        else
            bprintf(buf,"You only see %d %s's here.\n\r",count,arg3);
        
        send_to_char(buf->data,ch);
        buffer_free( buf );
        return;
    }

    pdesc = get_extra_descr( arg1, ch->in_room->extra_descr );
    if ( pdesc != NULL )
    {
        send_to_char( pdesc, ch );
        buffer_free( buf );
        return;
    }

         if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) ) door = 0;
    else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east"  ) ) door = 1;
    else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) ) door = 2;
    else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west"  ) ) door = 3;
    else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up"    ) ) door = 4;
    else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down"  ) ) door = 5;
    else
    {
        send_to_char( "You do not see that here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    /* 'look direction' */
    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
        send_to_char( "Nothing special there.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( pexit->description != NULL && pexit->description[0] != '\0' )
        send_to_char( pexit->description, ch );
    else
        send_to_char( "Nothing special there.\n\r", ch );

    if ( pexit->keyword    != NULL
    &&   pexit->keyword[0] != '\0'
    &&   pexit->keyword[0] != ' ' )
    {
        if ( IS_SET(pexit->exit_info, EX_CLOSED) && !IS_SET(pexit->exit_info, EX_HIDDEN) )
        {
            act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
        }
        else if ( ( IS_SET(pexit->exit_info, EX_ISDOOR)
                && !IS_SET(pexit->exit_info, EX_HIDDEN) )
                || ( IS_SET(pexit->exit_info, EX_ISDOOR)
                && IS_SET(pexit->exit_info, EX_HIDDEN)
                && !IS_SET(pexit->exit_info, EX_CLOSED) ) )
        {
            act( "The $d is open.",   ch, NULL, pexit->keyword, TO_CHAR );
        }
    }
    buffer_free( buf );
    return;
}

/* RT added back for the hell of it */
void do_read (CHAR_DATA *ch, char *argument )
{
    do_look(ch,argument);
}

void do_examine( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Examine what?\n\r", ch );
        buffer_free( buf );
        return;
    }

    do_look( ch, arg );

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        switch ( obj->item_type )
        {
        default:
            break;

        case ITEM_JUKEBOX:
            do_play(ch,"list");
            break;

        case ITEM_DRINK_CON:
        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            send_to_char( "When you look inside, you see:\n\r", ch );
            bprintf( buf, "in %s", arg );
            do_look( ch, buf->data );
        }
    }

    buffer_free( buf );
    return;
}



/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits( CHAR_DATA *ch, char *argument )
{
    extern char * const dir_name[];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door;
    BUFFER *buf2 = buffer_new( MAX_INPUT_LENGTH );

    fAuto  = !str_cmp( argument, "auto" );

    if ( !check_blind( ch ) )
    {
        buffer_free( buf );
        return;
    }

/*
    strcpy( buf, fAuto ? "[Exits:" : "Obvious exits:\n\r" );
*/
    bprintf( buf, fAuto ? "[Exits:" : "Obvious exits:\n\r" );

    found = FALSE;
    for ( door = 0; door <= 5; door++ )
    {
        if ( ( pexit = ch->in_room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL
        &&   can_see_room(ch,pexit->u1.to_room)
        &&   !IS_SET(pexit->exit_info, EX_CLOSED)
        &&   !IS_SET(pexit->exit_info, EX_HIDDEN) )
        {
            found = TRUE;
            if ( fAuto )
            {
                buffer_strcat( buf, " " );
                buffer_strcat( buf, dir_name[door] );
            }
            else
            {
                bprintf( buf2 /*+ strlen(buf->data) */, 
                    "%-5s - %s\n\r",
                    capitalize( dir_name[door] ),
                    room_is_dark( pexit->u1.to_room )
                        ?  "Too dark to tell"
                        : pexit->u1.to_room->name
                    );
                buffer_strcat( buf, buf2->data );
            }
        }
    }

    if ( !found )
        buffer_strcat( buf, fAuto ? " none" : "None.\n\r" );

    if ( fAuto )
        buffer_strcat( buf, "]\n\r" );

    send_to_char( buf->data, ch );
    buffer_free( buf );
    buffer_free( buf2 );
    return;
}

void do_worth( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if (IS_NPC(ch))
    {
        bprintf(buf,"You have %ld gold.\n\r",ch->gold);
        send_to_char(buf->data,ch);
        buffer_free( buf );
        return;
    }

    bprintf(buf, "You have %ld gold, and %ld experience.\n\r",
        ch->gold, ch->exp);
    send_to_char(buf->data,ch);

    buffer_free( buf );
    return;
}

char *statdiff(int normal, int modified)
{
    static char tempstr[10];
    
    strcpy(tempstr, "\0");
    if (normal < modified) sprintf(tempstr, "+%d", modified-normal);
    else if (normal > modified) sprintf(tempstr, "-%d", normal-modified);
    else if (normal == modified) sprintf(tempstr, "  ");
    return (tempstr);
}

void do_score( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( 100 );
    BUFFER *tempbuf = buffer_new( 100 );
    AFFECT_DATA *paf;
    int i,x;
    
    buffer_clear( tempbuf );
    buffer_clear( buf );

    if (!IS_NPC(ch)) {
    send_to_char( "\r", ch );
    bprintf( buf, "      `y/~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/~~\\\n\r");
    send_to_char( buf->data, ch);
    bprintf( buf, "     |   `W%s", ch->name);
    send_to_char( buf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 35-strlen(ch->name) ; x++) 
        buffer_strcat( buf, " ");
    send_to_char( buf->data, ch);
    bprintf( buf, "%3d years old (%4d hours) `y|____|\n\r", 
        get_age(ch), (ch->played + (int) (current_time - ch->logon))/3600);
    send_to_char( buf->data, ch);
    bprintf( buf, "     |+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+|\n\r");
    send_to_char( buf->data, ch);
    bprintf( buf, "     | `YSTR:     `G%2d `W%s `y| `YRace: `G%s",
        ch->perm_stat[STAT_STR], statdiff(ch->perm_stat[STAT_STR],
        get_curr_stat(ch,STAT_STR)), race_table[ch->race].name);
    send_to_char( buf->data, ch); 
    buffer_clear( buf );
/*    for ( x = 0; x < 41-strlen(race_table[ch->race].name) ; x++) */
    for ( x = 0; x < 10-strlen(race_table[ch->race].name) ; x++ )
        buffer_strcat( buf, " " );
    send_to_char(buf->data, ch);
    bprintf( buf, " `YPractices: `G%3d                ", ch->practice );
    send_to_char( buf->data, ch );
    bprintf( buf, "`y|\n\r     | `YINT:     `G%2d `W%s `y| `YClass: `G%s", 
        ch->perm_stat[STAT_INT],
        statdiff(ch->perm_stat[STAT_INT], get_curr_stat(ch,STAT_INT)),
        class_table[ch->ch_class].name);
    send_to_char(buf->data, ch);
    buffer_clear( buf );
/*    for ( x = 0; x < 40-strlen(class_table[ch->ch_class].name) ; x++)*/
    for ( x = 0; x < 9-strlen(class_table[ch->ch_class].name) ; x++ )
        buffer_strcat( buf, " " );
    send_to_char(buf->data, ch);
    bprintf( buf, " `YTrains:    `G%3d                ", ch->train );
    send_to_char( buf->data, ch );
    bprintf( buf, "`y|\n\r     | `YWIS:     `G%2d `W%s `y|                                                `y|\n\r",
        ch->perm_stat[STAT_WIS], statdiff(ch->perm_stat[STAT_WIS],
        get_curr_stat(ch,STAT_WIS)));
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YDEX:     `G%2d `W%s `y| `YAlignment: `G%5d `W[",
        ch->perm_stat[STAT_DEX],
        statdiff(ch->perm_stat[STAT_DEX], get_curr_stat(ch,STAT_DEX)),
        ch->alignment);
    send_to_char(buf->data, ch);
    if ( ch->alignment >  900 )      
        bprintf( buf, "Angelic]                     `y|\n\r" );
    else if ( ch->alignment >  700 ) 
        bprintf( buf, "Saintly]                     `y|\n\r" );
    else if ( ch->alignment >  350 ) 
        bprintf( buf, "Good]                        `y|\n\r" );
    else if ( ch->alignment >  100 ) 
        bprintf( buf, "Kind]                        `y|\n\r" );
    else if ( ch->alignment > -100 ) 
        bprintf( buf, "Neutral]                     `y|\n\r" );
    else if ( ch->alignment > -350 ) 
        bprintf( buf, "Mean]                        `y|\n\r" );
    else if ( ch->alignment > -700 ) 
        bprintf( buf, "Evil]                        `y|\n\r" );
    else if ( ch->alignment > -900 ) 
        bprintf( buf, "Demonic]                     `y|\n\r" );
    else                             
        bprintf( buf, "Satanic]                     `y|\n\r" );
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YCON:     `G%2d `W%s `y| `YSex: `G%s          `YCreation Points : `G%3d     `y|\n\r",
        ch->perm_stat[STAT_CON], 
        statdiff(ch->perm_stat[STAT_CON], get_curr_stat(ch,STAT_CON)),
        ch->sex == 0 ? "Sexless" : ch->sex == 1 ? "Male  " : "female",
        ch->pcdata->points);
    send_to_char(buf->data, ch);
    bprintf( buf, "     |+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+|\n\r");
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YItems Carried   ");
    send_to_char(buf->data, ch);
    bprintf( tempbuf, "`G%d`y/`G%d",ch->carry_number, can_carry_n(ch));
    send_to_char(tempbuf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 19-strlen(tempbuf->data)+6 ; x++)
        buffer_strcat(buf, " ");
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    bprintf( buf, "`YArmor vs magic  : `G%5d      `y|\n\r", 
        GET_AC(ch,AC_EXOTIC));
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YWeight Carried  ");
    send_to_char(buf->data, ch);
    bprintf( tempbuf, "`G%d`y/`G%d",ch->carry_weight, can_carry_w(ch));
    send_to_char(tempbuf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 19-strlen(tempbuf->data)+6 ; x++)
        buffer_strcat(buf, " ");
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    bprintf( buf, "`YArmor vs bash   : `G%5d      `y|\n\r",
        GET_AC(ch,AC_BASH));
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YGold            ");
    send_to_char(buf->data, ch);
    bprintf( tempbuf, "`G%ld", ch->gold);
    send_to_char(tempbuf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 19-strlen(tempbuf->data)+2 ; x++)
        buffer_strcat(buf, " ");
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    bprintf( buf,"`YArmor vs pierce : `G%5d      `y|\n\r",
        GET_AC(ch,AC_PIERCE));
    send_to_char(buf->data, ch);
    bprintf( buf, "     |                                    `YArmor vs slash  : `G%5d      `y|\n\r",
                 GET_AC(ch,AC_SLASH));
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YCurrent XP       ");
    send_to_char(buf->data, ch);
    bprintf( tempbuf, "`G%ld", ch->exp);
    send_to_char(tempbuf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 47-strlen(tempbuf->data)+2 ; x++)
        buffer_strcat(buf, " ");
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    bprintf( buf, "`y|\n\r     |                  ");
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 47 ; x++) {
        buffer_strcat(buf, " ");
	}
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    bprintf( buf, "`y|\n\r     |                                     `YHitP: `G%5d `y/ `G%5d         `y|\n\r",
        ch->hit, ch->max_hit);
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YBonus to Hit: `W+");
    send_to_char(buf->data, ch);
    bprintf( tempbuf, "%d",GET_HITROLL(ch));
    send_to_char(tempbuf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 21-strlen(tempbuf->data) ; x++)
        buffer_strcat(buf, " ");
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    bprintf( buf, "`YMana: `G%5d `y/ `G%5d         `y|\n\r", ch->mana, ch->max_mana);
    send_to_char(buf->data, ch);
    bprintf( buf, "     | `YBonus to Dam: `W+");
    send_to_char(buf->data, ch);
    bprintf( tempbuf, "%d",GET_DAMROLL(ch));
    send_to_char(tempbuf->data, ch);
    buffer_clear( buf );
    for ( x = 0; x < 21-strlen(tempbuf->data) ; x++)
        buffer_strcat(buf, " ");
    send_to_char(buf->data,ch);
    bprintf( buf, "`YMove: `G%5d `y/ `G%5d         `y|\n\r", ch->move, ch->max_move);
    send_to_char(buf->data, ch);
    buffer_clear( buf );
    bprintf( buf, "`y     | `YQuest points: `G%4d", ch->questpoints );
    send_to_char( buf->data, ch );
    bprintf( buf, "   `YQuest timer: `G%3d    `YBonus Points: `G%4d     `y|\n\r", 
        ch->nextquest ? ch->nextquest : ch->countdown, ch->bonusPoints );
    send_to_char( buf->data, ch );
    bprintf( buf, "`y  /~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/   |\n\r");
    send_to_char(buf->data, ch);
    bprintf( buf, "  \\________________________________________________________________\\__/`w\n\r");
    send_to_char(buf->data, ch);
    }
    else {
    bprintf( buf,
        "You are %s%s, %d years old (%d hours).\n\r",
        ch->name,
        IS_NPC(ch) ? "" : ch->pcdata->title,
        get_age(ch),
        ( ch->played + (int) (current_time - ch->logon) ) / 3600);
    send_to_char( buf->data, ch );

    bprintf(buf, "Race: %s  Sex: %s  Class:  %s\n\r",
        race_table[ch->race].name,
        ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
        IS_NPC(ch) ? "mobile" : class_table[ch->ch_class].name);
    send_to_char(buf->data,ch);
        

    bprintf( buf,
        "You have %d/%d hit, %d/%d mana, %d/%d movement.\n\r",
        ch->hit,  ch->max_hit,
        ch->mana, ch->max_mana,
        ch->move, ch->max_move);
    send_to_char( buf->data, ch );

    bprintf( buf,
        "You have %d practices and %d training sessions.\n\r",
        ch->practice, ch->train);
    send_to_char( buf->data, ch );

    bprintf( buf,
        "You are carrying %d/%d items with weight %d/%d pounds.\n\r",
        ch->carry_number, can_carry_n(ch),
        ch->carry_weight, can_carry_w(ch) );
    send_to_char( buf->data, ch );

    bprintf( buf,
        "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
        ch->perm_stat[STAT_STR],
        get_curr_stat(ch,STAT_STR),
        ch->perm_stat[STAT_INT],
        get_curr_stat(ch,STAT_INT),
        ch->perm_stat[STAT_WIS],
        get_curr_stat(ch,STAT_WIS),
        ch->perm_stat[STAT_DEX],
        get_curr_stat(ch,STAT_DEX),
        ch->perm_stat[STAT_CON],
        get_curr_stat(ch,STAT_CON) );
    send_to_char( buf->data, ch );

    bprintf( buf,
        "You have scored %ld exp, and have %ld gold coins.\n\r",
        ch->exp,  ch->gold );
    send_to_char( buf->data, ch );

    bprintf( buf, "Wimpy set to %d hit points.\n\r", ch->wimpy );
    send_to_char( buf->data, ch );

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]   > 10 )
        send_to_char( "You are drunk.\n\r",   ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] ==  0 )
        send_to_char( "You are thirsty.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   ==  0 )
        send_to_char( "You are hungry.\n\r",  ch );

    switch ( ch->position )
    {
    case POS_DEAD:     
        send_to_char( "You are DEAD!!\n\r",             ch );
        break;
    case POS_MORTAL:
        send_to_char( "You are mortally wounded.\n\r",  ch );
        break;
    case POS_INCAP:
        send_to_char( "You are incapacitated.\n\r",     ch );
        break;
    case POS_STUNNED:
        send_to_char( "You are stunned.\n\r",           ch );
        break;
    case POS_SLEEPING:
        send_to_char( "You are sleeping.\n\r",          ch );
        break;
    case POS_RESTING:
        send_to_char( "You are resting.\n\r",           ch );
        break;
    case POS_STANDING:
        send_to_char( "You are standing.\n\r",          ch );
        break;
    case POS_FIGHTING:
        send_to_char( "You are fighting.\n\r",          ch );
        break;
    }


    /* print AC values */
    bprintf( buf,"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
             GET_AC(ch,AC_PIERCE),
             GET_AC(ch,AC_BASH),
             GET_AC(ch,AC_SLASH),
             GET_AC(ch,AC_EXOTIC));
        send_to_char(buf->data,ch);

    for (i = 0; i < 4; i++)
    {
        char * temp;

        switch(i)
        {
            case(AC_PIERCE):    temp = "piercing";      break;
            case(AC_BASH):      temp = "bashing";       break;
            case(AC_SLASH):     temp = "slashing";      break;
            case(AC_EXOTIC):    temp = "magic";         break;
            default:            temp = "error";         break;
        }
        
        send_to_char("You are ", ch);

        if      (GET_AC(ch,i) >=  101 ) 
            bprintf(buf,"hopelessly vulnerable to %s.\n\r",temp);
        else if (GET_AC(ch,i) >= 80) 
            bprintf(buf,"defenseless against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= 60)
            bprintf(buf,"barely protected from %s.\n\r",temp);
        else if (GET_AC(ch,i) >= 40)
            bprintf(buf,"slighty armored against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= 20)
            bprintf(buf,"somewhat armored against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= 0)
            bprintf(buf,"armored against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= -20)
            bprintf(buf,"well-armored against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= -40)
            bprintf(buf,"very well-armored against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= -60)
            bprintf(buf,"heavily armored against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= -80)
            bprintf(buf,"superbly armored against %s.\n\r",temp);
        else if (GET_AC(ch,i) >= -100)
            bprintf(buf,"almost invulnerable to %s.\n\r",temp);
        else
            bprintf(buf,"divinely armored against %s.\n\r",temp);

        /* why no send_to_char here? --Rahl */

        send_to_char( buf->data, ch );
    }


    /* RT wizinvis and holy light */
    if ( IS_IMMORTAL(ch))
    {
      send_to_char("Holy Light: ",ch);
      if (IS_SET(ch->act,PLR_HOLYLIGHT))
        send_to_char("on",ch);
      else
        send_to_char("off",ch);
 
      if (IS_SET(ch->act,PLR_WIZINVIS))
      {
        bprintf( buf, "  Invisible: rank %d",ch->invis_level);
        send_to_char(buf->data,ch);
      }

      /* incog added by Rahl */
      if ( IS_SET( ch->act, PLR_INCOGNITO ) )
      {
        bprintf( buf, "  Incognito: rank %d", ch->incog_level );
        send_to_char( buf->data, ch );
      }

      send_to_char("\n\r",ch);
    }

    bprintf( buf, "Hitroll: %d  Damroll: %d.\n\r",
        GET_HITROLL(ch), GET_DAMROLL(ch) );
    send_to_char( buf->data, ch );
    
    bprintf( buf, "Alignment: %d.  ", ch->alignment );
    send_to_char( buf->data, ch );

    send_to_char( "You are ", ch );
         if ( ch->alignment >  900 ) send_to_char( "angelic.\n\r", ch );
    else if ( ch->alignment >  700 ) send_to_char( "saintly.\n\r", ch );
    else if ( ch->alignment >  350 ) send_to_char( "good.\n\r",    ch );
    else if ( ch->alignment >  100 ) send_to_char( "kind.\n\r",    ch );
    else if ( ch->alignment > -100 ) send_to_char( "neutral.\n\r", ch );
    else if ( ch->alignment > -350 ) send_to_char( "mean.\n\r",    ch );
    else if ( ch->alignment > -700 ) send_to_char( "evil.\n\r",    ch );
    else if ( ch->alignment > -900 ) send_to_char( "demonic.\n\r", ch );
    else                             send_to_char( "satanic.\n\r", ch );
    
    if ( ch->affected != NULL )
    {
        send_to_char( "You are affected by:\n\r", ch );
        for ( paf = ch->affected; paf != NULL; paf = paf->next )
        {
            bprintf( buf, "Spell: '%s'", skill_table[paf->type].name );
            send_to_char( buf->data, ch );

            bprintf( buf,
                " modifies %s by %d for %d hours",
                affect_loc_name( paf->location ),
                paf->modifier,
                paf->duration );
            send_to_char( buf->data, ch );

            send_to_char( ".\n\r", ch );
        }
    }
    }

    buffer_free( buf );
    buffer_free( tempbuf );

    return;
}



char *  const   day_name        [] =
{
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
    "the Great Gods", "the Sun"
};

char *  const   month_name      [] =
{
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
    "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
    "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
    "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

void do_time( CHAR_DATA *ch, char *argument )
{
    extern char str_boot_time[];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char *suf;
    int day;

    day     = time_info.day + 1;

         if ( day > 4 && day <  20 ) suf = "th";
    else if ( day % 10 ==  1       ) suf = "st";
    else if ( day % 10 ==  2       ) suf = "nd";
    else if ( day % 10 ==  3       ) suf = "rd";
    else                             suf = "th";

    bprintf( buf, 
        "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\rBroken Shadows started up at %s\rThe system time is %s\r",
        (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
        time_info.hour >= 12 ? "pm" : "am",
        day_name[day % 7],
        day, suf,
        month_name[time_info.month],
        str_boot_time,
        (char *) ctime( &current_time )
        );

    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}



void do_weather( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    static char * const sky_look[4] =
    {
        "cloudless",
        "cloudy",
        "rainy",
        "lit by flashes of lightning"
    };

    if ( !IS_OUTSIDE(ch) )
    {
        send_to_char( "You can't see the weather indoors.\n\r", ch );
        buffer_free( buf );
        return;
    }

    bprintf( buf, "The sky is %s and %s.\n\r",
        sky_look[weather_info.sky],
        weather_info.change >= 0
        ? "a warm southerly breeze blows"
        : "a cold northern gust blows"
        );
    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}



void do_help( CHAR_DATA *ch, char *argument )
{
    HELP_DATA *pHelp;
    char argall[MAX_INPUT_LENGTH],argone[MAX_INPUT_LENGTH];

    if ( argument[0] == '\0' )
        argument = "summary";

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0' )
    {
        argument = one_argument(argument,argone);
        if (argall[0] != '\0')
            strcat(argall," ");
        strcat(argall,argone);
    }

    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
    {
        if ( pHelp->level > char_getImmRank( ch ) ) {
            continue;
		}

        if ( is_name( argall, pHelp->keyword ) )
        {
            if ( pHelp->level >= 0 && str_cmp( argall, "imotd" ) )
            {
                send_to_char( pHelp->keyword, ch );
                send_to_char( "\n\r", ch );
            }

            /*
             * Strip leading '.' to allow initial blanks.
             */
            if ( pHelp->text[0] == '.' )
                page_to_char( pHelp->text+1, ch );
            else
                page_to_char( pHelp->text  , ch );
            return;
        }
    }

    send_to_char( "No help on that word.\n\r", ch );
    return;
}


/* old whois command */
void do_whoname (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    BUFFER *buf;
    DESCRIPTOR_DATA *d;
    bool found = FALSE;

    one_argument(argument,arg);
  
    if (arg[0] == '\0')
    {
        send_to_char("You must provide a name.\n\r",ch);
        return;
    }

    output = buffer_new( MAX_INPUT_LENGTH );
    buf = buffer_new( MAX_INPUT_LENGTH );


    for (d = descriptor_list; d != NULL; d = d->next)
    {
        CHAR_DATA *wch;
        char const *ch_class;

        if ( ( d->connected != CON_PLAYING ) || !can_see(ch,d->character))
            continue;
        
        wch = ( d->original != NULL ) ? d->original : d->character;

        if (!can_see(ch,wch))
            continue;

        if (!str_prefix(arg,wch->name))
        {
            found = TRUE;
            
            /* work out the printing */
            ch_class = class_table[wch->ch_class].who_name;
            switch( char_getImmRank( wch ))
            {
                /* Was 3-letter strings */
                case MAX_RANK : 
                    ch_class = " Implementor ";    break;
                case MAX_RANK - 1 : 
                    ch_class = "   Creator   ";    break;
                case MAX_RANK - 2 : 
                    ch_class = "  Supremacy  ";    break;
                case MAX_RANK - 3 : 
                    ch_class = "    Deity    ";    break;
                case MAX_RANK - 4 : 
                    ch_class = "     God     ";    break;
                case MAX_RANK - 5 : 
                    ch_class = "   Immortal  ";    break;
                case MAX_RANK - 6 : 
                    ch_class = "   Demigod   ";    break;
                case MAX_RANK - 7 : 
                    ch_class = "    Angel    ";    break;
                case MAX_RANK - 8 : 
                    ch_class = "   Avatar    ";    break;
                case MAX_RANK - 9 : 
                    ch_class = "   Avatar    ";    break;
            }
    
            /* a little formatting */
        if (!IS_IMMORTAL( wch ) || IS_NPC( wch ) )
        {
        bprintf( buf, "`K[`Y%s `G%s`K] %s%s%s%s%s %s%s%s `w%s%s`w\n\r",
            wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name 
                                    : "     ",
            /* class, */
            IS_NPC( wch ) ? class_table[wch->ch_class].who_name : ch_class,
            IS_NPC(wch) ? 
                (wch->pIndexData->clan == 0) ? "" : "`W[`w" :
                (wch->pcdata->clan ==0) ? "" : "`W[`w",
            IS_NPC(wch) ? vis_clan(wch->pIndexData->clan)
                : vis_clan(wch->pcdata->clan),
            IS_NPC(wch) ? 
                (wch->pIndexData->clan == 0) ? "" : "`W]`w" :
                (wch->pcdata->clan ==0) ? "" : "`W]`w",
            IS_SET(wch->act, PLR_AFK) ? "`W(AFK) " : "",
            IS_SET(wch->act, PLR_KILLER) ? "`R(PK) " : "",
            IS_SET(wch->act, PLR_THIEF)  ? "`K(THIEF) "  : "",
            /* QUIET case added by Rahl */
            IS_SET(wch->comm, COMM_QUIET) ? "`Y(QUIET) " : "",
            /* bounty hunter added by Rahl */
            IS_SET(wch->act, PLR_BOUNTY_HUNTER) ? "`M(BH) " : "",
            /* changed from wch->name - Rahl */
            !IS_NPC( wch ) ? wch->name : wch->short_descr,
            IS_NPC(wch) ? "" : wch->pcdata->title );
        }
        else
        {
        bprintf( buf, "`K[`C%s`K] %s%s%s%s%s%s%s%s%s%s`w%s%s`w\n\r",
            ch_class,
            IS_NPC(wch) ? 
                (wch->pIndexData->clan == 0) ? "" : "`W[`w" :
                (wch->pcdata->clan ==0) ? "" : "`W[`w",
            IS_NPC(wch) ? vis_clan(wch->pIndexData->clan)
                : vis_clan(wch->pcdata->clan),
            IS_NPC(wch) ? 
                (wch->pIndexData->clan == 0) ? "" : "`W]`w " :
                (wch->pcdata->clan ==0) ? "" : "`W]`w ",
            /* incog and wizi added by Rahl */
            IS_SET( wch->act, PLR_WIZINVIS ) ? "`B(Wizi) " : "",
            IS_SET( wch->act, PLR_INCOGNITO ) ? "`C(Incog) " : "",
            IS_SET(wch->act, PLR_AFK) ? "`W(AFK) " : "",
            IS_SET(wch->act, PLR_KILLER) ? "`R(PK) " : "",
            IS_SET(wch->act, PLR_THIEF)  ? "`K(THIEF) "  : "",
            /* QUIET case added by Rahl */
            IS_SET(wch->comm, COMM_QUIET) ? "`Y(QUIET) " : "",
            /* bounty hunter added by Rahl */
            IS_SET(wch->act, PLR_BOUNTY_HUNTER) ? "`M(BH) " : "",
            /* changed from wch->name - Rahl */
            !IS_NPC( wch ) ? wch->name : wch->short_descr,
            IS_NPC(wch) ? "" : wch->pcdata->title );
        }
        buffer_strcat(output,buf->data);
        }
    }

    if (!found)
    {
        send_to_char("No one of that name is playing.\n\r",ch);
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    page_to_char(output->data,ch);
    buffer_free( output );
    buffer_free( buf );
    return;
}

void insert_sort(CHAR_DATA *who_list[300], CHAR_DATA *ch, int length)
{
        while ( ( length ) && char_getImmRank( who_list[length-1] ) < char_getImmRank( ch ) ) {
                who_list[length]=who_list[length-1];
                length--;
        }
        who_list[length]=ch;
}

void chaos_sort(CHAR_DATA *who_list[300], CHAR_DATA *ch, int length)
{
        while ( ( length ) && who_list[length-1]->pcdata->chaos_score < ch->pcdata->chaos_score) {
                who_list[length]=who_list[length-1];
                length--;
        }
        who_list[length]=ch;
}


/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */
void do_who( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf;
    BUFFER *buf2;
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *who_list[300];
    int iClass;
    int iRace;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    int length;
    int maxlength;
    int count;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool fClassRestrict;
    bool fRaceRestrict;
    bool fImmortalOnly;
    bool doneimmort=FALSE;
    bool donemort=FALSE;


    /*
     * Set default arguments.
     */
    iLevelLower    = 0;
    iLevelUpper    = MAX_RANK;
    fClassRestrict = FALSE;
    fRaceRestrict = FALSE;
    fImmortalOnly  = FALSE;
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        rgfClass[iClass] = FALSE;
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
        rgfRace[iRace] = FALSE;

    /*
     * Parse arguments.
     */
    nNumber = 0;
    for ( ;; )
    {
        char arg[MAX_STRING_LENGTH];

        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;

        if ( is_number( arg ) )
        {
            switch ( ++nNumber )
            {
            case 1: iLevelLower = atoi( arg ); break;
            case 2: iLevelUpper = atoi( arg ); break;
            default:
                send_to_char( "Only two rank numbers allowed.\n\r", ch );
                return;
            }
        }
        else
        {

            /*
             * Look for classes to turn on.
             */
            if ( argument == "imm" )
            {
                fImmortalOnly = TRUE;
            }
            else
            {
                iClass = class_lookup(arg);
                if (iClass == -1)
                {
                    iRace = race_lookup(arg);

                    if (iRace == 0 || iRace >= MAX_PC_RACE)
                    {
                        do_whoname(ch, arg);
                        return;
                    }
                    else
                    {
                        fRaceRestrict = TRUE;
                        rgfRace[iRace] = TRUE;
                    }
                }
                else
                {
                    fClassRestrict = TRUE;
                    rgfClass[iClass] = TRUE;
                }
            }
        }
    }
    
    length=0;
    for ( d = descriptor_list ; d ; d = d->next ) {
        if (( d->connected == CON_PLAYING ) || (d->connected == CON_NOTE_TO) 
        || (d->connected == CON_NOTE_SUBJECT) || (d->connected == CON_NOTE_EXPIRE)
        || (d->connected == CON_NOTE_TEXT) || (d->connected == CON_NOTE_FINISH) )
        {
            insert_sort(who_list, d->character, length);
            length++;
        }
    }

    maxlength=length;

    /*
     * Now show matching chars.
     */
    output = buffer_new( MAX_INPUT_LENGTH );
    buf = buffer_new( MAX_INPUT_LENGTH );
    buf2 = buffer_new( MAX_INPUT_LENGTH );

    nMatch = 0;
    for ( length=0 ; length < maxlength ; length++ )
    {
        char const *ch_class;

        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if ( ( char_getImmRank(who_list[length] ) >= MAX_RANK-9 ) && doneimmort==FALSE ) {
                bprintf( buf, "`K[`RVisible Immortals`K]\n\r\n\r");
                doneimmort=TRUE;
                buffer_strcat( output, buf->data );
        } else if ( ( char_getImmRank( who_list[length] ) < MAX_RANK-9 ) && donemort == FALSE) {
                if ( doneimmort == TRUE ) {
                        bprintf( buf, "\n\r");
                        buffer_strcat( output, buf->data);
                }
                bprintf( buf, "`K[`RVisible Mortals`K]\n\r\n\r");
                donemort = TRUE;
                buffer_strcat( output, buf->data );
        }
        if ( who_list[length]->desc->connected != CON_PLAYING || !can_see( ch, who_list[length] ) )
            continue;

        if ( char_getImmRank( who_list[length] ) < iLevelLower
        ||   char_getImmRank( who_list[length] ) > iLevelUpper
        || ( fImmortalOnly  && char_getImmRank( who_list[length] ) < MAX_RANK - 9 )
        || ( fClassRestrict && !rgfClass[who_list[length]->ch_class] ) 
        || ( fRaceRestrict && !rgfRace[who_list[length]->race]))
            continue;

        nMatch++;

        /*
         * Figure out what to print for class.
         */

        ch_class = class_table[who_list[length]->ch_class].who_name;
        switch ( char_getImmRank( who_list[length] ) )
        {
        default: break;
            {
                /* Was 3-letter strings */
                case MAX_RANK : 
                    ch_class = " Implementor ";    break;
                case MAX_RANK - 1 : 
                    ch_class = "   Creator   ";    break;
                case MAX_RANK - 2 : 
                    ch_class = "  Supremacy  ";    break;
                case MAX_RANK - 3 : 
                    ch_class = "    Deity    ";    break;
                case MAX_RANK - 4 : 
                    ch_class = "     God     ";    break;
                case MAX_RANK - 5 : 
                    ch_class = "   Immortal  ";    break;
                case MAX_RANK - 6 : 
                    ch_class = "   Demigod   ";    break;
                case MAX_RANK - 7 : 
                    ch_class = "    Angel    ";    break;
                case MAX_RANK - 8 : 
                    ch_class = "   Avatar    ";    break;
                case MAX_RANK - 9 : 
                    ch_class = "   Avatar    ";    break;
            }
        }
       
        /*
         * Format it up.
         */
       if (!IS_IMMORTAL( who_list[length] ) || IS_NPC( who_list[length] ) )
       {
       bprintf( buf, "`K[`Y%s `G%s`K] %s%s%s%s%s%s%s%s%s%s`w%s%s\n\r",
               who_list[length]->race < MAX_PC_RACE ? pc_race_table[who_list[length]->race].who_name 
               : "     ",
                /* class, */
               IS_NPC( who_list[length] ) ?
                   class_table[who_list[length]->ch_class].who_name : ch_class,
               IS_NPC(who_list[length]) ? 
               (who_list[length]->pIndexData->clan == 0) ? "" : "`W[`w" :
               (who_list[length]->pcdata->clan ==0) ? "" : "`W[`w",
               IS_NPC(who_list[length]) ? vis_clan(who_list[length]->pIndexData->clan)
               : vis_clan(who_list[length]->pcdata->clan),
               IS_NPC(who_list[length]) ? 
               (who_list[length]->pIndexData->clan == 0) ? "" : "`W]`w " :
               (who_list[length]->pcdata->clan ==0) ? "" : "`W]`w ",
               IS_SET(who_list[length]->act, PLR_WIZINVIS) ? "`B(Wizi)`w " : "",
               IS_SET(who_list[length]->act, PLR_INCOGNITO) ? "`C(Incog)`w " : "",
               IS_SET(who_list[length]->act, PLR_AFK) ? "`W(AFK) " : "",
               IS_SET(who_list[length]->act, PLR_KILLER) ? "`R(PK) " : "",
               IS_SET(who_list[length]->act, PLR_THIEF)  ? "`K(THIEF) "  : "",
               /* QUIET case added by Rahl */
               IS_SET(who_list[length]->comm, COMM_QUIET) ? "`Y(QUIET) " : "",
               /* bounty hunter added by Rahl */
               IS_SET(who_list[length]->act, PLR_BOUNTY_HUNTER) ? "`M(BH) " : "",
               /* changed from who_list[length]->name -- Rahl */
               !IS_NPC( who_list[length] ) ? who_list[length]->name :
                    who_list[length]->short_descr,
               IS_NPC(who_list[length]) ? "" : who_list[length]->pcdata->title);
       }
       else
       {
       bprintf( buf, "`K[`C%s`K] %s%s%s%s%s%s%s%s%s%s`w%s%s\n\r",
               ch_class,
               IS_NPC(who_list[length]) ? 
               (who_list[length]->pIndexData->clan == 0) ? "" : "`W[`w" :
               (who_list[length]->pcdata->clan ==0) ? "" : "`W[`w",
               IS_NPC(who_list[length]) ? vis_clan(who_list[length]->pIndexData->clan)
               : vis_clan(who_list[length]->pcdata->clan),
               IS_NPC(who_list[length]) ? 
               (who_list[length]->pIndexData->clan == 0) ? "" : "`W]`w " :
               (who_list[length]->pcdata->clan ==0) ? "" : "`W]`w ",
               IS_SET(who_list[length]->act, PLR_WIZINVIS) ? "`B(Wizi)`w " : "",
               IS_SET(who_list[length]->act, PLR_INCOGNITO) ? "`C(Incog)`w " : "",
               IS_SET(who_list[length]->act, PLR_AFK) ? "`W(AFK) " : "",
               IS_SET(who_list[length]->act, PLR_KILLER) ? "`R(PK) " : "",
               IS_SET(who_list[length]->act, PLR_THIEF)  ? "`K(THIEF) "  : "",
               /* QUIET case added by Rahl */
               IS_SET(who_list[length]->comm, COMM_QUIET) ? "`Y(QUIET) " : "",
               /* bounty hunter added by Rahl */
               IS_SET(who_list[length]->act, PLR_BOUNTY_HUNTER) ? "`M(BH) " : "",
               /* changed from who_list[length]->name -- Rahl */
               !IS_NPC( who_list[length] ) ? who_list[length]->name :
                    who_list[length]->short_descr,
               IS_NPC(who_list[length]) ? "" : who_list[length]->pcdata->title);
       }
       buffer_strcat(output, buf->data);
    }
   
   bprintf( buf2, "\n\r`wVisible Players Shown: `W%d\n\r`w", nMatch );
   buffer_strcat(output,buf2->data);
   count=0;
   for ( d = descriptor_list ; d ; d = d->next ) {
       if ( d->connected == CON_PLAYING 
            && ( (!IS_SET(d->character->act, PLR_WIZINVIS)) 
            && (!IS_SET(d->character->act, PLR_INCOGNITO))) )
         {
            count++;
         }
       else if (((d->connected == CON_PLAYING)  || (d->connected == CON_NOTE_TO) 
        || (d->connected == CON_NOTE_SUBJECT) || (d->connected == CON_NOTE_EXPIRE)
        || (d->connected == CON_NOTE_TEXT) || (d->connected == CON_NOTE_FINISH)) 
                && (!(d->character->invis_level > char_getImmRank( ch  )) ) )
         
         { count++; }
       
    }
   
/*   bprintf( buf2, "`wTotal Players Online: `W%d\n\r`w", count );
   buffer_strcat(output,buf2->data); */
    page_to_char( output->data, ch );
    buffer_free( output );
    buffer_free( buf );
    buffer_free( buf2 );
    return;
}

void do_inventory( CHAR_DATA *ch, char *argument )
{
    send_to_char( "You are carrying:\n\r", ch );
    show_list_to_char( ch->carrying, ch, TRUE, TRUE );
    return;
}



void do_equipment( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    int iWear;
    bool found;

    send_to_char( "You are using:\n\r", ch );
    found = FALSE;
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
        if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
            continue;

        send_to_char( where_name[iWear], ch );
        if ( can_see_obj( ch, obj ) )
        {
            send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
            send_to_char( "\n\r`w", ch );
        }
        else
        {
            send_to_char( "Something.\n\r", ch );
        }
        found = TRUE;
    }

    if ( !found )
        send_to_char( "Nothing.\n\r", ch );

    return;
}



void do_compare( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Compare what to what?\n\r", ch );
        return;
    }

    if ( ( obj1 = get_obj_carry( ch, arg1 ) ) == NULL )
    {
        send_to_char( "You do not have that item.\n\r", ch );
        return;
    }

    if (arg2[0] == '\0')
    {
        for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
        {
            if (obj2->wear_loc != WEAR_NONE
            &&  can_see_obj(ch,obj2)
            &&  obj1->item_type == obj2->item_type
            &&  (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
                break;
        }

        if (obj2 == NULL)
        {
            send_to_char("You aren't wearing anything comparable.\n\r",ch);
            return;
        }
    } 

    else if ( (obj2 = get_obj_carry(ch,arg2) ) == NULL )
    {
        send_to_char("You do not have that item.\n\r",ch);
        return;
    }

    msg         = NULL;
    value1      = 0;
    value2      = 0;

    if ( obj1 == obj2 )
    {
        msg = "You compare $p to itself.  It looks about the same.";
    }
    else if ( obj1->item_type != obj2->item_type )
    {
        msg = "You can't compare $p and $P.";
    }
    else
    {
        switch ( obj1->item_type )
        {
        default:
            msg = "You can't compare $p and $P.";
            break;

        case ITEM_ARMOR:
            value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
            value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
            break;

        case ITEM_WEAPON:
                value1 = (1 + obj1->value[2]) * obj1->value[1];

                value2 = (1 + obj2->value[2]) * obj2->value[1];
            break;
        }
    }

    if ( msg == NULL )
    {
             if ( value1 == value2 ) msg = "$p and $P look about the same.";
        else if ( value1  > value2 ) msg = "$p looks better than $P.";
        else                         msg = "$p looks worse than $P.";
    }

    act( msg, ch, obj1, obj2, TO_CHAR );
    return;
}



void do_credits( CHAR_DATA *ch, char *argument )
{
    do_help( ch, "diku" );
    return;
}



void do_where( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool found;

    one_argument( argument, arg );

	// only imms can do a where during CHAOS
    if ( (chaos) && ( char_getImmRank( ch ) < HERO ) ) {
         send_to_char( "Where? Your killer is right behind you!\n\r", ch);
         buffer_free( buf );
         return;
    }

	// immortal default 'where'
    if ( arg[0] == '\0' && ( char_getImmRank( ch ) >= HERO ) ) {
        send_to_char( "Current players:\n\r", ch );
        found = FALSE;
        for ( d = descriptor_list; d; d = d->next ) {
            if ( d->connected == CON_PLAYING
            && ( victim = d->character ) != NULL
            &&   !IS_NPC(victim)
            &&   victim->in_room != NULL
            &&   can_see( ch, victim ) ) {
                found = TRUE;
                bprintf( buf, "%-28s [%5d] %s`w\n\r",
                    victim->name, victim->in_room->vnum, victim->in_room->name);
                send_to_char( buf->data, ch );
            } // for
        } // if
        if ( !found ) {
            send_to_char( "None\n\r", ch );
		}
    } else if (char_getImmRank( ch ) >= HERO) { // with arg
        found = FALSE;
        for ( victim = char_list; victim != NULL; victim = victim->next ) {
            if ( victim->in_room != NULL
            &&   can_see( ch, victim )
            &&   is_name( arg, victim->name ) ) {
                found = TRUE;
                bprintf( buf, "%-28s %s`w\n\r",
                    PERS(victim, ch), victim->in_room->name );
                send_to_char( buf->data, ch );
                break;
            } // if
        } // for
        if ( !found ) {
            act( "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
		} // if
    } else if ( arg[0] == '\0' && ( char_getImmRank( ch ) < HERO) ) {
		// Mortal default case
        found = FALSE;
        send_to_char("Players near you:\n\r",ch);
        for ( d = descriptor_list; d; d = d->next ) {
            if ( d->connected == CON_PLAYING
            && ( victim = d->character ) != NULL
            &&   !IS_NPC(victim)
            &&   victim->in_room != NULL
            &&   victim->in_room->area == ch->in_room->area
            &&   can_see( ch, victim ) ) {
                found = TRUE;
                bprintf( buf, "%-28s %s`w\n\r",
                    victim->name, victim->in_room->name );
                send_to_char( buf->data, ch );
            } // if
        } // for
        if ( !found ) {
            send_to_char( "None\n\r", ch );
		} // if
    } else { // Mortal with arg
        found = FALSE;
        for ( victim = char_list; victim != NULL; victim = victim->next ) {
            if ( victim->in_room != NULL
            &&   victim->in_room->area == ch->in_room->area
            &&   !IS_AFFECTED(victim, AFF_HIDE)
            &&   !IS_AFFECTED(victim, AFF_SNEAK)
            &&   can_see( ch, victim )
            &&   is_name( arg, victim->name ) ) {
                found = TRUE;
                bprintf( buf, "%-28s %s`w\n\r",
                    PERS(victim, ch), victim->in_room->name );
                send_to_char( buf->data, ch );
                break;
            } // if
        } // for

        if ( !found ) {
            act( "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
		} // if
    } // if

    buffer_free( buf );
    return;
}




void do_consider( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Consider killing whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They're not here.\n\r", ch );
        return;
    }

    if (is_safe(ch,victim))
    {
        send_to_char("Don't even think about it.\n\r",ch);
        return;
    }

    diff = (victim->max_hit/ch->max_hit)*100 ;
         
         if ( diff <= 50 )  msg = "Comparing HP: $N isn't even in the ballpark.";
    else if ( diff <= 65 )  msg = "Comparing HP: $N almost makes you want to laugh.";
    else if ( diff <= 85 )  msg = "Comparing HP: $N Isn't quite up to your level.";
    else if ( diff <= 115 ) msg = "Comparing HP: You're about equal.";
    else if ( diff <= 125 ) msg = "Comparing HP: $N's just a bit tougher than you.";
    else if ( diff <= 140 ) msg = "Comparing HP: Maybe you should consider attacking something else.";
    else                    msg = "Comparing HP: $N puts you to shame.";
    act( msg, ch, NULL, victim, TO_CHAR );

    return;
}



void set_title( CHAR_DATA *ch, char *title )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( IS_NPC(ch) )
    {
        bug( "Set_title: NPC.", 0 );
        return;
    }

    if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' )
    {
/*
        buf->data[0] = ' ';
        strcpy( buf+1, title );
*/
        bprintf( buf, " " );
        buffer_strcat( buf, title );
    }
    else
    {
/*
        strcpy( buf, title );
*/
        bprintf( buf, "%s", title );
    }
    
    buffer_strcat( buf, "`w");

    free_string( ch->pcdata->title );
    ch->pcdata->title = str_dup( buf->data );
    return;
}



void do_title( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_STRING_LENGTH );

    if ( IS_NPC(ch) )
    {
        buffer_free( buf );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Change your title to what?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( strlen(argument) > 45 )
        argument[45] = '\0';

    smash_tilde( argument );
    set_title( ch, argument );

/* title echo by Rahl */
    bprintf( buf, "Your title is now: %s\n\r", argument );
    send_to_char( buf->data, ch );    
/*
    send_to_char( "Ok.\n\r", ch );
*/

    buffer_free( buf );
    return;
}

void do_description( CHAR_DATA *ch, char *argument )
{
   if (ch->desc != NULL)  /* only if ch has a descriptor 'cause string_append will barf */
     {
        string_append( ch, &ch->description );
        return;
     }
   return;
}

void do_report( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    bprintf( buf,
        "`BYou say 'I have %d/%d hp %d/%d mana %d/%d mv.'\n\r`w",
        ch->hit,  ch->max_hit,
        ch->mana, ch->max_mana,
        ch->move, ch->max_move );

    send_to_char( buf->data, ch );

     bprintf( buf, "`B$n says 'I have %d/%d hp %d/%d mana %d/%d mv.'`w",
        ch->hit,  ch->max_hit,
        ch->mana, ch->max_mana,
        ch->move, ch->max_move );
        
    act( buf->data, ch, NULL, NULL, TO_ROOM );

    buffer_free( buf );
    return;
}



void do_practice( CHAR_DATA *ch, char *argument ) {
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );
    int sn = 0;

    if ( IS_NPC(ch) ) {
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

    if ( argument[0] == '\0' ) {
        int col = 0;

        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            if ( skill_table[sn].name == NULL ) {
                break;
			}

			/* skill is not known */
            if ( ch->pcdata->learned[sn] < 1 ) {
                continue;
			}

            bprintf( buf, "%-18s %3d%%  ", skill_table[sn].name, 
				ch->pcdata->learned[sn] );
            buffer_strcat( buffer, buf->data );

            if ( ++col % 3 == 0 ) {   
                buffer_strcat( buffer, "\n\r" );
            }
        }

        if ( col % 3 != 0 ) {
            buffer_strcat( buffer, "\n\r" );
        }

        bprintf( buf, "You have %d practice sessions left.\n\r",
            ch->practice );
        buffer_strcat( buffer, buf->data );
        
        page_to_char( buffer->data, ch );
    } else {
        CHAR_DATA *mob;
        int adept = 0;

        if ( !IS_AWAKE( ch ) ) {
            send_to_char( "In your dreams, or what?\n\r", ch );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room ) {
            if ( IS_NPC( mob ) && IS_SET( mob->act, ACT_PRACTICE ) ) {
                break;
			}
        }

        if ( mob == NULL ) {
            send_to_char( "You can't do that here.\n\r", ch );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        if ( ch->practice <= 0 ) {
            send_to_char( "You have no practice sessions left.\n\r", ch );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        if ( ( sn = skill_lookup( argument ) ) < 0
        || ch->pcdata->learned[sn] < 1 /* skill is not known */
        || skill_table[sn].rating[ch->ch_class] == 0 ) {
            send_to_char( "You can't practice that.\n\r", ch );
            buffer_free( buf );
            buffer_free( buffer );
            return;
        }

        adept = class_table[ch->ch_class].skill_adept;

        if ( ch->pcdata->learned[sn] >= adept ) {
            bprintf( buf, "You are already learned at %s.\n\r",
                skill_table[sn].name );
            send_to_char( buf->data, ch );
        } else {
            ch->practice--;
            ch->pcdata->learned[sn] += 
                int_app[get_curr_stat( ch, STAT_INT )].learn / 
                skill_table[sn].rating[ch->ch_class];
            if ( ch->pcdata->learned[sn] < adept ) {
                act( "You practice $T.",
                    ch, NULL, skill_table[sn].name, TO_CHAR );
                act( "$n practices $T.",
                    ch, NULL, skill_table[sn].name, TO_ROOM );
            } else {
                ch->pcdata->learned[sn] = adept;
                act( "You are now learned at $T.",
                    ch, NULL, skill_table[sn].name, TO_CHAR );
                act( "$n is now learned at $T.",
                    ch, NULL, skill_table[sn].name, TO_ROOM );
            }
        }
    }
    buffer_free( buffer );
    buffer_free( buf );
    return;
}



/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
        wimpy = ch->max_hit / 5;
    else
        wimpy = atoi( arg );

    if ( wimpy < 0 )
    {
        send_to_char( "Your courage exceeds your wisdom.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( wimpy > ch->max_hit/2 )
    {
        send_to_char( "Such cowardice ill becomes you.\n\r", ch );
        buffer_free( buf );
        return;
    }

    ch->wimpy   = wimpy;
    bprintf( buf, "Wimpy set to %d hit points.\n\r", wimpy );
    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}



void do_password( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if ( IS_NPC(ch) )
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace(*argument) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace(*argument) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: password <old> <new>.\n\r", ch );
        return;
    }

    if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
    {
        WAIT_STATE( ch, 40 );
        send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
        return;
    }

    if ( strlen(arg2) < 5 )
    {
        send_to_char(
            "New password must be at least five characters long.\n\r", ch );
        return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt( arg2, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
        if ( *p == '~' )
        {
            send_to_char(
                "New password not acceptable, try again.\n\r", ch );
            return;
        }
    }

    free_string( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    save_char_obj( ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}

/* RT configure command SMASHED */
void do_search( CHAR_DATA *ch, char *argument )
{
    extern char * const dir_name[];
    EXIT_DATA *pexit;
    int door;
    bool found;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( !check_blind( ch ) )
    {
        buffer_free( buf );
        return; 
    }

    send_to_char( "You start searching for secret doors.\n\r", ch );
    found=FALSE;

    for ( door = 0; door <= 5; door++ )
    {
        if ( ( pexit = ch->in_room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL
        &&   IS_SET(pexit->exit_info, EX_CLOSED)
        &&   IS_SET(pexit->exit_info, EX_HIDDEN) )
        {
            found=TRUE;
        bprintf( buf, "You found a secret exit %s.\n\r", dir_name[door]);       
        send_to_char( buf->data, ch );
        }         
    }
        if (!found)
        {
            send_to_char( "You found no secret exits.\n\r", ch );
            buffer_free( buf );
            return;
        }
    buffer_free( buf );
    return;
}

void do_cwho( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *output = buffer_new( MAX_INPUT_LENGTH );
    DESCRIPTOR_DATA *d;
    CHAR_DATA *who_list[300];
    int length;
    int maxlength;

    if(!chaos)
    {
        send_to_char( "There is no `rC`RH`YA`RO`rS`w active.\n\r",ch);
        buffer_free( buf );
        buffer_free( output );
        return;
    }

    length=0;
    for ( d = descriptor_list ; d ; d = d->next ) 
        {
           if ( d->connected == CON_PLAYING ) 
           {
            chaos_sort(who_list, d->character, length);
            length++;
           }
        }

    output->data[0] = '\0';
    maxlength=length;
    for ( length=0 ; length < maxlength ; length++ )
    {
        bprintf( buf, "`K[`W%4d`K] `w%s\n\r",
            who_list[length]->pcdata->chaos_score,who_list[length]->name);
        buffer_strcat(output, buf->data);
    }
        /* was send_to_char */
    page_to_char( output->data, ch );
    buffer_free( buf );
    buffer_free( output );
    return;
}

void do_show(CHAR_DATA *ch, char *argument)
{
   CLAN_DATA * pClan;
   char arg[MAX_INPUT_LENGTH];
/*   char arg2[MAX_INPUT_LENGTH]; */
   BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
/*   sh_int tmpcount; */
   DESCRIPTOR_DATA *d;
   BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );

   buffer->data[0] = '\0';

   argument = one_argument( argument, arg );
/*   strcpy (arg2, argument); */

    send_to_char( "\r", ch );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Available lists:\n\r", ch );
        send_to_char( "clans bounties\n\r", ch );
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }
    if (!strcmp(arg,"clan") || !strcmp(arg,"clans"))
        {
/*         if (arg2[0]=='\0')
             {
*/              send_to_char( "`wClans that exist on Broken Shadows:\n\r", ch);
                send_to_char("`K===================================`w\n\r",ch);
                for ( pClan=clan_first; pClan != NULL ; pClan=pClan->next)
                  {
                     bprintf(buf,"`K[`w%3d`K] `WShort name: `w%10s    `WLong name: `w%s`w\n\r",pClan->number,
                             pClan->name, pClan->visible);
                     send_to_char(buf->data,ch);
                  }
                buffer_free( buffer );
                buffer_free( buf );
                return;
             }
/*
           pClan=find_clan(arg2);
           if (pClan==NULL)
             {
                send_to_char("That clan does not exist!\n\r", ch);
                buffer_free( buf );
                buffer_free( buffer );
                return;
             }
           bprintf(buf,"Members of %s`w:\n\r", pClan->visible);
           send_to_char(buf->data, ch);
           for ( tmpcount=0 ; tmpcount < pClan->num_members ; tmpcount++ )
             {
                bprintf(buf,"Member #%d : %s%s\n\r",
                         tmpcount,pClan->members[tmpcount],
                        !str_cmp(pClan->members[tmpcount],pClan->leader) ? " (Leader)" : "");
                send_to_char(buf->data, ch);
             }
           if (pClan->auto_accept == 0)
             {
                send_to_char("This clan does NOT auto-accept new members that meet the requirements.\n\r", ch);
             }
           else
             {
*               send_to_char("This clan will auto-accept new members that meet the requirements.\n\r", ch); 
*            }
*          buffer_free( buffer );
*          buffer_free( buf );
*          return;
*       }
*/

    /* bounty added by Rahl */
    if ( !strcmp( arg, "bounty" ) || !strcmp( arg, "bounties" ) )
    {
        CHAR_DATA *wch;

        send_to_char( "Players online with bounties:\n\r", ch );
        send_to_char( "=============================\n\r", ch );

        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->connected != CON_PLAYING )
                continue;

            wch = d->character;
            if ( wch->desc->original != NULL )
                continue;
            if ( wch->pcdata->bounty > 0 )
            {
                bprintf( buf, "%s - %ld\n\r", wch->name,
                    wch->pcdata->bounty );
                buffer_strcat( buffer, buf->data );
            }
        }
        page_to_char( buffer->data, ch );
        buffer_free( buf );
        buffer_free( buffer );
        return;
    }

   send_to_char("No list available for that.\n\r",ch);
   buffer_free( buf );
   buffer_free( buffer );
   return;
}


// removing level/rank now. will have to look into putting symbolic
// names in at some point
void do_finger(CHAR_DATA *ch, char *argument)
{
   char         arg[MAX_INPUT_LENGTH];
   CHAR_DATA    *victim;
   FILE         *fp;
   char         pfile[MAX_STRING_LENGTH], *title;
   char         *word, *ltime, *class, *email, *comment, *spou, *clan;
   long         logon;
   BUFFER       *buf = buffer_new( MAX_INPUT_LENGTH ); 
   char         *race;
   sh_int       leader;
   int          pkills, pkilled, killed;

   argument = one_argument( argument, arg );
   pfile[0]='\0';
   word=NULL;
   ltime=NULL;
   class=NULL;
   race=NULL;
   title=NULL;
   clan=NULL;
   email = strdup( "(none)" );   
   pkills = 0;
   pkilled = 0;
   killed = 0;
   leader = 0;
   comment = strdup( "(none)" );
   spou = NULL;

   if ( arg[0] == '\0' || arg[0] == '.' || arg[0] == '/')
     {
        send_to_char( "You want information about whom?\n\r", ch );
        buffer_free( buf );
        return;
     }
   
   else  if ( ( victim = get_char_world( ch, arg ) ) != NULL )
     {
        if (IS_NPC(victim) )
          { 
             send_to_char("You want information about whom?\n\r",ch);
             buffer_free( buf );
             return;
          }
        bprintf(buf, "%s%s is a %s %s.\n\r", victim->name,
                victim->pcdata->title, 
                pc_race_table[victim->race].name, class_table[victim->ch_class].name);
        send_to_char(buf->data, ch);
        if ( victim->pcdata->clan == 0 )
          bprintf(buf, "%s is not a member of any clan.\n\r", victim->name );
        else
          bprintf(buf, "%s is %s of %s`w.\n\r", victim->name,
                  is_clan_leader(victim, clan_lookup(victim->pcdata->clan) ) ? "a leader" : "a member",
                  vis_clan(victim->pcdata->clan) );
        send_to_char(buf->data, ch);
        bprintf(buf, "%s last logged on %s\r", victim->name,
            ctime(&victim->logon) );
        send_to_char(buf->data, ch);
        bprintf( buf, "Email address: %s`w\n\r", victim->pcdata->email );
        send_to_char( buf->data, ch );
        bprintf( buf, "Comment: %s`w\n\r", victim->pcdata->comment );
        send_to_char( buf->data, ch );
        bprintf( buf, "Killed: %d   Pkills: %d   Pkilled: %d\n\r",
            victim->pcdata->killed,
            victim->pcdata->pkills, victim->pcdata->pkilled );
        send_to_char( buf->data, ch );
        if ( victim->pcdata->spouse != NULL )
            bprintf( buf, "%s is married to %s.\n\r", victim->name,
                victim->pcdata->spouse );
        else
            bprintf( buf, "%s is not married.\n\r", victim->name );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
     }
   else
     { 
        sprintf( pfile, "%s%s", PLAYER_DIR, capitalize( arg ) );
        if ( ( fp = fopen( pfile, "r" ) ) != NULL )
          {
             for ( ; ; )
               {
                  word = fread_word(fp);
                  if (!str_cmp(word,"#END"))
                    {
                       break;
                    }
                  if ( !str_cmp( word, "Spou" ) )
                    {
                        spou = fread_string( fp );
                    }
                  if ( !str_cmp( word, "Comnt" ) )
                  {
                        comment = fread_string( fp );
                  }
                  if ( !str_cmp( word, "Email" ) )
                  {
                       email = fread_string(fp);
                  } 
                  if ( !str_cmp( word, "Pkills" ) )
                  {
                        pkills = fread_number( fp );
                  }
                  if ( !str_cmp( word, "Pkilled" ) )
                  {
                        pkilled = fread_number( fp );
                  }
                  if ( !str_cmp( word, "Killed" ) )
                  {
                        killed = fread_number( fp );
                  }
                  if (!str_cmp(word,"Log") )
                    {
                       logon=fread_number(fp);
                       ltime=ctime(&logon);
                    }
                  if (!str_cmp(word, "Race") )
                    {
                       race = fread_string(fp);
                    }
                  if (!str_cmp(word, "Cla") )
                    {
                       class = (class_table[fread_number(fp)].name);
                    }
                  if (!str_cmp(word, "Titl") )
                    {
                       title = fread_string(fp);
                    }
                  if (!str_cmp(word, "Clan") )
                    {
                       clan = fread_string(fp);
                    }
                  if ( !str_cmp( word, "Clan_leader" ) )
                  {
                        leader = fread_number( fp );
                  }
                  fread_to_eol(fp);
                //  if ( word )
                  //   free( word ); 
               }
             fclose(fp);
             bprintf(buf, "%s %s is a %s %s.\n\r",
                capitalize(arg), title, race, class);
             send_to_char(buf->data, ch);
             if ( clan != NULL )
               bprintf(buf, "%s is %s of %s`w.\n\r", capitalize(arg), 
                       leader ? "a leader" : "a member",
                       vis_clan( get_clan( clan ) ) );
             else
               bprintf(buf, "%s is not a member of any clan.\n\r",
                    capitalize(arg) );
             send_to_char(buf->data, ch);
             if (ltime == NULL)
               bprintf(buf, "Last login unknown.\n\r");
             else
               bprintf(buf, "%s last logged on %s\r",
                  capitalize(arg),ltime);
             send_to_char(buf->data,ch);
             bprintf( buf, "Email address: %s`w\n\r", email );
             send_to_char( buf->data, ch );
             bprintf( buf, "Comment: %s`w\n\r", comment );
             send_to_char( buf->data, ch );
             bprintf( buf, "Killed: %d   Pkills: %d   Pkilled: %d\n\r",
                killed, pkills, pkilled );
             send_to_char( buf->data, ch );
             if ( spou != NULL )
                 bprintf( buf, "%s is married to %s.\n\r",
                     capitalize(arg), spou );
             else
                 bprintf( buf, "%s is not married.\n\r", capitalize(arg) );
             send_to_char( buf->data, ch );
             buffer_free( buf );
             return;
          }
        send_to_char("That character does not exist on this mud.\n\r",ch);
        buffer_free( buf );
        return;
     }
}

/* 
 * Lore written by Rahl. Can use on object anywhere in the world,
 * but you also get less info than identify 
 */
void do_lore( CHAR_DATA *ch, char *argument ) {
    OBJ_DATA *obj;
    char arg[MAX_STRING_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    BUFFER *buf2;

    buf2 = buffer_new( 50 );
    buf2->data[0] = '\0';

    one_argument( argument, arg );

    obj = get_obj_world( ch, arg );

    /* 
     * <blush> oops. Dunno how I forgot this the first time around
     * -Rahl
     */

    if ( obj == NULL ) {
        bprintf( buf, "You've never heard of a %s.\n\r", arg );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    } 

    if ( get_skill( ch, gsn_lore ) == 0 ) {
        send_to_char( "You don't know anything about it.\n\r", ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "What do you want information on?\n\r", ch );
        buffer_free( buf );
        buffer_free( buf2 );
        return;
    }

    if ( number_percent( ) < get_skill( ch, gsn_lore ) ) {
        bprintf( buf, "'%s' is type %s\n\r", obj->name,
            item_type_name( obj ) );
        buffer_strcat( buf2, buf->data ); 

        bprintf( buf, "Extra flags %s.\n\r", 
            extra_bit_name( obj->extra_flags) );
        buffer_strcat( buf2, buf->data ); 

        switch( obj->item_type ) {
            case ITEM_SCROLL:
            case ITEM_POTION:
            case ITEM_PILL:
                buffer_strcat( buf2, "Spells of: '" );

                if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL ) {
                    buffer_strcat( buf2, skill_table[obj->value[1]].name );
                    buffer_strcat( buf2, "'" );
                }

                if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL ) {
                    buffer_strcat( buf2, " '" );
                    buffer_strcat( buf2, skill_table[obj->value[2]].name );
                    buffer_strcat( buf2, "'" );
                }

                if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL ) {
                    buffer_strcat( buf2, " '" );
                    buffer_strcat( buf2, skill_table[obj->value[3]].name );
                    buffer_strcat( buf2, "'" );
                }

                buffer_strcat( buf2, ".\n\r" );
                break;

            case ITEM_WAND:
            case ITEM_STAFF:

                if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL ) {
                    buffer_strcat( buf2, " '" );
                    buffer_strcat( buf2, skill_table[obj->value[3]].name );
                    buffer_strcat( buf2, "'" );
                }

                buffer_strcat( buf2, ".\n\r" );
                break;

            case ITEM_WEAPON:
                buffer_strcat( buf2, "Weapon type is " );
                switch ( obj->value[0] ) {
                    case( WEAPON_EXOTIC ) : 
                        buffer_strcat( buf2, "exotic.\n\r" );
                        break;
                    case( WEAPON_SWORD )  : 
                        buffer_strcat( buf2, "sword.\n\r" );
                        break;
                    case( WEAPON_DAGGER ) : 
                        buffer_strcat( buf2, "dagger.\n\r" );
                        break;
                    case( WEAPON_SPEAR )  : 
                        buffer_strcat( buf2, "spear/staff.\n\r" );
                        break;
                    case( WEAPON_MACE )   : 
                        buffer_strcat( buf2, "mace/club.\n\r" );
                        break;
                    case( WEAPON_AXE )    : 
                        buffer_strcat( buf2, "axe.\n\r" );
                        break;
                    case( WEAPON_FLAIL )  : 
                        buffer_strcat(buf2, "flail.\n\r" );
                        break;
                    case( WEAPON_WHIP )   : 
                        buffer_strcat( buf2, "whip.\n\r" );
                        break;
                    case( WEAPON_POLEARM ): 
                        buffer_strcat( buf2, "polearm.\n\r" );
                        break;
                    default             : 
                        buffer_strcat( buf2, "unknown.\n\r" );
                        break;
                } /* switch */
    
                bprintf(buf, "Average damage %d.\n\r",
                   number_fuzzier( ( 1 + obj->value[2]) *
                   obj->value[1] / 2 ) );
                buffer_strcat( buf2, buf->data );
                break;

            case ITEM_ARMOR:
                bprintf( buf,
                    "Armor class is %d pierce, %d bash, %d slash, and"
                    " %d vs. magic.\n\r", number_fuzziest( obj->value[0] ), 
                    number_fuzziest( obj->value[1] ), 
                    number_fuzziest( obj->value[2] ), 
                    number_fuzziest( obj->value[3] ) );
                buffer_strcat( buf2, buf->data );
                break;
        } /* switch */

    	page_to_char( buf2->data, ch ); 

    	check_improve( ch, gsn_lore, TRUE, 1 );
    } else {
        send_to_char( "You can't remember a thing about it.\n\r", ch );
        check_improve( ch, gsn_lore, FALSE, 1 );
    }

    buffer_free( buf );
    buffer_free( buf2 );
    return;
}


/*
 * Bank stuff added by Rahl based on code by Judson Knott
 */
void do_balance( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf;

    if ( IS_NPC( ch ) )
        return;

    buf = buffer_new( MAX_INPUT_LENGTH );

    bprintf( buf, "You have %ld gold in the bank.\n\r", ch->bank );
    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}

void do_deposit( CHAR_DATA *ch, char *argument ) {
    CHAR_DATA *banker;
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    long amount;

    if ( IS_NPC( ch ) ) {
        buffer_free( buf );
        return;
    }

    if ( !IS_SET( ch->in_room->room_flags, ROOM_BANK ) ) {
        send_to_char( "You have to be in a bank to make a deposit!\n\r",
                ch );
        buffer_free( buf );
        return;
    }

    banker = NULL;
    for ( banker = ch->in_room->people; banker; banker = banker->next_in_room )
    {
        if ( IS_NPC( banker ) && IS_SET( banker->pIndexData->act, ACT_BANKER ) )
            break;
    }

    if ( !banker ) {
        send_to_char( "The banker is not available at this moment.\n\r", ch );
        buffer_free( buf );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "How much gold do you want to deposit?\n\r", ch );
        buffer_free( buf );
        return;
    }

    amount = atol( arg );

    if ( amount >= ( ch->gold + 1 ) ) {
        bprintf( buf, "You don't have %ld gold!", amount );
        do_say( ch, buf->data );
        buffer_free( buf );
        return;
    }

    if ( amount <= 0 ) {
        send_to_char( "You must deposit a positive amount!\n\r", ch );
        buffer_free( buf );
        return;
    }

    ch->bank += amount;
    ch->gold -= amount;
    bprintf( buf, "You have deposited %ld gold.", amount );
    do_say( ch, buf->data );
    bprintf( buf, "Your bank account now contains %ld gold.", ch->bank );
    do_say( ch, buf->data );
    buffer_free( buf );
    return;
}

void do_withdraw( CHAR_DATA *ch, char *argument ) {
    CHAR_DATA *banker;
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    long amount;

   if (IS_NPC( ch ) ) {
        buffer_free( buf );
        return;
   }

    if ( !IS_SET( ch->in_room->room_flags, ROOM_BANK ) ) {
        send_to_char( "You have to be in a bank to make a withdrawl!\n\r",
                ch );
        buffer_free( buf );
        return;
    }

    banker = NULL;
    for ( banker = ch->in_room->people; banker; banker = banker->next_in_room )
    {
        if ( IS_NPC( banker ) && IS_SET( banker->pIndexData->act, ACT_BANKER ) )
            break;
    }

    if ( !banker ) {
        send_to_char( "The banker is not available at this moment.\n\r", ch );
        buffer_free( buf );
        return;
    }
    
    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "How much gold do you want to withdraw?\n\r", ch );
        buffer_free( buf );
        return;
    }

    amount = atol( arg );

    if ( amount <= 0 ) {
        send_to_char( "You must withdraw a positive amount!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( amount >= ( ch->bank +1 ) ) {
        bprintf( buf, "You don't have %ld gold in the bank.", amount );
		do_say( ch,  buf->data );
        buffer_free( buf );
        return;
    }

    ch->gold += amount;
    ch->bank -= amount;
    bprintf( buf, "You have withdrawn %ld gold.", amount );
    do_say( ch, buf->data );
    bprintf( buf, "Your bank account now contains %ld gold.", ch->bank );
    do_say( ch, buf->data );
    buffer_free( buf );
    return;
} 

/*
 * Clan accept command written by Rahl.
 */
void do_accept( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CLAN_DATA *Clan;

    if ( IS_NPC( ch ) )
    {
        send_to_char( "Try again when you aren't switched.\n\r", ch );
        return;
    }

    if ( ch->pcdata->clan == 0 )
    {
        send_to_char( "You aren't even in a clan!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !is_clan_leader( ch, clan_lookup( ch->pcdata->clan ) ) )
    {
        send_to_char( "You aren't a clan leader!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax: accept [character]\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        bprintf( buf, "%s can't join a clan.\n\r", victim->short_descr );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    if ( victim->pcdata->clan != 0 )
    {
        bprintf( buf, "%s is already in a clan!\n\r", victim->name );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    bprintf( buf, "%d", ch->pcdata->clan );
    Clan = find_clan( buf->data );

    bprintf( buf, "%s %s", victim->name, Clan->name );
    do_test( ch, buf->data );

    if ( clan_accept( victim,  Clan->name ) != 0 )
    {
        buffer_free( buf );
        return;
    }
    else
    {
/*
        Clan->members[Clan->num_members] = strdup( victim->name );
*/
        Clan->num_members += 1;
        victim->pcdata->clan = ch->pcdata->clan;
        bprintf( buf, "You are now a member of %s.\n\r",
                vis_clan( ch->pcdata->clan ) );
        send_to_char( buf->data, victim );
        bprintf( buf, "%s is now a member of %s.\n\r", victim->name,
                vis_clan( ch->pcdata->clan ) );
        log_string( buf->data );
        send_to_char( buf->data, ch );
        do_save( victim, "" );
        do_asave( victim, "clans" );
    }
    buffer_free( buf );
    return;
}


/*
 * Clan disown command written by Rahl.
 */
void do_disown( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CLAN_DATA *Clan;
 /*   int count, count2; */

    if ( IS_NPC( ch ) )
    {
        send_to_char( "Try again when you aren't switched.\n\r", ch );
        return;
    }

    if ( ch->pcdata->clan == 0 )
    {
        send_to_char( "You aren't even in a clan!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( !is_clan_leader( ch, clan_lookup( ch->pcdata->clan ) ) )
    {
        send_to_char( "You aren't a clan leader!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax: disown [character]\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        bprintf( buf, "%s isn't in a clan.\n\r", victim->short_descr );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
        bprintf( buf, "%s is not in %s!\n\r", victim->name, 
                vis_clan( ch->pcdata->clan ) );
        send_to_char( buf->data, ch );
        buffer_free( buf );
        return;
    }

    bprintf( buf, "%d", ch->pcdata->clan );
    Clan = find_clan( buf->data );
/*
    for ( count = 0; count < Clan->max_members; count++ )
    {
        if ( Clan->members[count] == victim->name )
        {
            Clan->members[count] = Clan->members[count+1];
            for ( count2 = count + 1; count2 < Clan->max_members - 1; count2++ )
            {
                Clan->members[count2] = Clan->members[count2+1];
            }
        }
        else
            continue;
    }
*/
    Clan->num_members -= 1;
    if ( is_clan_leader( victim, clan_lookup( victim->pcdata->clan ) ) )
        victim->pcdata->clan_leader = 0;
    victim->pcdata->clan = 0;
    bprintf( buf, "You are no longer a member of %s.\n\r",
        vis_clan( ch->pcdata->clan ) );
    send_to_char( buf->data, victim );
    bprintf( buf, "%s is no longer a member of %s.\n\r", victim->name,
        vis_clan( ch->pcdata->clan ) );
    log_string( buf->data );
    send_to_char( buf->data, ch );
    do_save( victim, "" );
    do_asave( victim, "clans" );
    buffer_free( buf );
    return;
}


/* 
 * Bounty written by FRiTZ, implemented here by Rahl with a few changes
 * Some of the ideas by Exar 
 */
void do_bounty( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Place a bounty on whose head?\n\r", ch );
        send_to_char( "Syntax: Bounty [character] [amount]\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here right now.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        send_to_char( "Not on NPCs.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET( ch->act, PLR_BOUNTY_HUNTER ) )
    {
        send_to_char( "Bounty hunters can't place bounties on people.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_IMMORTAL( ch ) )
    {
        send_to_char( "That's not very nice.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_IMMORTAL( victim ) )
    {
        send_to_char( "And how would they be killed?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( IS_SET( victim->act, PLR_NOBOUNTY ) )
    {
        send_to_char( "Not on that character.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( is_number( arg2 ) )
    {
        long amount;
        
        amount = atol( arg2 );

        if ( ch->gold < amount )
        {
            send_to_char( "You don't have that much gold!\n\r", ch );
            buffer_free( buf );
            return;
        }

        if ( amount < 2000 )
        {
            send_to_char( "You must place a bounty of at least 2000 gold.\n\r", ch );
            buffer_free( buf );
            return;
        }

        ch->gold -= amount;
        victim->pcdata->bounty += amount;

        bprintf( buf, "You have placed a bounty of %ld gold on %s.\n\r",
            amount, victim->name );
        send_to_char( buf->data, ch );
        bprintf( buf, "%s now has a bounty of %ld gold.\n\r",
            victim->name, victim->pcdata->bounty );
        send_to_char( buf->data, ch );

        buffer_free( buf );
        return;
    }
    else
    {
        send_to_char( "The bounty must be a number.\n\r", ch );
        buffer_free( buf );
        return;
    }
    buffer_free( buf );
    return;
}

/* 
 * email from EmberMUD 0.29. Implemented by Rahl with a few small
 * changes
 */
void do_email( CHAR_DATA *ch, char *argument )
{
   char last_char;
   int i, length, dots;
   bool at;
   BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
   char *errorstring = "Invalid email address. Address must be in name@host.domain format.\n\r";

   if ( IS_NPC(ch) )
   {
     buffer_free( buf );
     return;
   }

   if ( argument[0] == '\0' )
     {
        send_to_char( "Email address deleted.\n\r", ch );
        free_string(ch->pcdata->email);
        ch->pcdata->email = str_dup ( "(none)" );
        buffer_free( buf );
        return;
     }

   if ( strlen(argument) > 45 )
     argument[45] = '\0';

   smash_tilde( argument );

   length = strlen( argument ) - 1;

   if (argument[0] == '@' || argument[0] == '.' || argument[length] == '@'
        || argument[length] == '.') {
        send_to_char( errorstring, ch );
        buffer_free( buf );
        return;
   }

   at = FALSE;
   dots = 0;
   last_char = '\0';

   for (i = 1; i < length ; i++) {
        switch (argument[i]) {
            case '@':
                if (!at) at = TRUE;
                else {
                    send_to_char( errorstring, ch );
                    buffer_free( buf );
                    return;
                }
                break;
            case '.':
               if (last_char == '@' || last_char == '.' || !at) {
                   send_to_char( errorstring, ch );
                   buffer_free( buf );
                   return;
               }
               dots++;
               break;
           case ' ':
               send_to_char( errorstring, ch );
               buffer_free( buf );
               return;
               break;
        }
        last_char = argument[i];
   }

   if ( dots < 1 ) {
        send_to_char( errorstring, ch );
        buffer_free( buf );
        return;
   }

   free_string(ch->pcdata->email);
   bprintf( buf, "%s", argument );
   buffer_strcat( buf, "`w" );
   ch->pcdata->email = str_dup( buf->data );
   bprintf(buf,"Ok, your email address is now: %s\n\r",ch->pcdata->email);
   send_to_char( buf->data, ch );

   free( errorstring );
   buffer_free( buf );

   return;
}

/* 
 * Saw this on a few MUDs. Thought it would be cool.
 * written by Rahl
 */
void do_comment( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( IS_NPC( ch ) )
    {
        send_to_char( "What's there for you to comment on? You're just "
            "suppsed to stand around and get killed.\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Comment cleared.\n\r", ch );
        free_string( ch->pcdata->comment );
        ch->pcdata->comment = strdup( "(none)" );
    }
    else
    {
        if ( strlen( argument ) > 60 )
            argument[60] = '\0';
        free_string( ch->pcdata->comment );
        smash_tilde( argument );
        bprintf( buf, argument );
        buffer_strcat( buf, "`w" );
        ch->pcdata->comment = strdup( buf->data );
        bprintf( buf, "Your comment is now: %s\n\r", ch->pcdata->comment );
        send_to_char( buf->data, ch );
    }
    buffer_free( buf );
    return;
}

/* petition by Rahl */
void do_petition( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CLAN_DATA *pClan;
    BUFFER *buf = buffer_new( 100 );

    one_argument( argument, arg );

    if ( IS_NPC( ch ) )
    {
        send_to_char( "You can't join a clan!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->pcdata->clan != 0 )
    {
        send_to_char( "But you're already in a clan!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( arg[0] == '\0' )
    {
        send_to_char( "Which clan do you wish to petition?\n\r", ch );
        buffer_free( buf );
        return;
    }

    pClan = find_clan( arg );

    if ( pClan == NULL )
    {
        send_to_char( "That clan does not exist.\n\r", ch );
        send_to_char( "Try using the clan short name shown in 'show clans'\n\r", ch );
        buffer_free( buf );
        return;
    }

    make_note( "Personal", ch->name, pClan->name, pClan->visible, 14, 
        "I wish to join your clan." );

    bprintf( buf, "You have sent a note to %s`w, asking to join the "
        "clan.\n\r", pClan->visible );
    send_to_char( buf->data, ch );

    buffer_free( buf );

    return;
}

/* new_changes by Rahl */
void do_new_changes( CHAR_DATA *ch, char *argument )
{
    FILE *fp;
    char filename[200];
    BUFFER *buffer = buffer_new( MAX_INPUT_LENGTH );

    buffer->data[0] = '\0';

    sprintf( filename, "%s", CHANGES_FILE );
 
    fp = fopen( filename, "r" );

    if ( !fp )
    {
        buffer_free( buffer );
        return;
    }

    while( 1 )
    {
        buffer_strcat( buffer, fread_string( fp ) );
        if ( feof( fp ) )
            break;
    }

    fclose( fp );

    page_to_char( buffer->data, ch );
    buffer_free( buffer );
    return;
}

/* autodamage by Rahl */        
void do_autodamage( CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTODAMAGE))
    {
        send_to_char( "You will no longer see damage amounts.\n\r", ch );
        REMOVE_BIT( ch->act, PLR_AUTODAMAGE );
    }
    else
    {
        send_to_char( "You will now see damage amounts.\n\r", ch );
        SET_BIT( ch->act, PLR_AUTODAMAGE );
    }
    return;
}

/* nobounty by Rahl */
void do_nobounty( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_NOBOUNTY ) )
    {
        send_to_char( "Bounties can now be placed on you.\n\r", ch );
        REMOVE_BIT( ch->act, PLR_NOBOUNTY );
    }
    else
    {
        send_to_char( "Bounties may not be placed on you.\n\r", ch );
        SET_BIT( ch->act, PLR_NOBOUNTY );
    }
    return;
}


void do_areport( CHAR_DATA *ch, char *argument )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    AFFECT_DATA *paf;    

    bprintf( buf, "You say 'I am affected by the following:'\n\r" );
    send_to_char( buf->data, ch );
    if ( ch->affected == NULL )
        send_to_char( "Nothing.\n\r", ch );
    else
    {
        for ( paf = ch->affected; paf != NULL; paf = paf->next )
        {
             bprintf( buf, "Spell: '%s'\n\r", skill_table[paf->type].name );
             send_to_char( buf->data, ch );
        }
    }

    bprintf( buf, "$n says 'I am affected by the following:'" );
    act( buf->data, ch, NULL, NULL, TO_ROOM );
    if ( ch->affected == NULL )
        act( "Nothing.\n\r", ch, NULL, NULL, TO_ROOM );
    else
    {
        for ( paf = ch->affected; paf != NULL; paf = paf->next )
        {
             bprintf( buf, "Spell: '%s'", skill_table[paf->type].name );
             act( buf->data, ch, NULL, NULL, TO_ROOM );
        }
    }
    buffer_free( buf );
    return;
}
