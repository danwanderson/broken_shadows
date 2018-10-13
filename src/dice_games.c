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

/***************************************************************************
 * A few really simple dice games. If you want to change the odds, simply  *
 * change the dice() numbers.                                              *
 *                                                                         *
 * To install: add dice_games.o to your makefile, add do_games to interp.c *
 * and interp.h, and add spec_gamemaster to special.c                      *
 *                                                                         *
 * If you choose to use this code, please retain my name in this file.     *
 * Suggestions for improvement are welcome.   -- Rahl (Daniel Anderson)    *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"

void win( CHAR_DATA *ch, long amnt );
void lose( CHAR_DATA *ch, long amnt );

void win( CHAR_DATA *ch, long amnt )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    ch->gold += amnt;
    bprintf( buf, "You win %ld gold!\n\r", amnt );
    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}

void lose( CHAR_DATA *ch, long amnt )
{
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );

    if ( ch->gold >= amnt )
        ch->gold -= amnt;
    else
        ch->gold = 0;
    bprintf( buf, "You lost %ld gold!\n\r", amnt );
    send_to_char( buf->data, ch );
    buffer_free( buf );
    return;
}

void game_even_odd( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    BUFFER* buf = buffer_new( MAX_INPUT_LENGTH );
    long amount;
    int roll;
    CHAR_DATA *gamemaster;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: game even-odd <even|odd> <amount>\n\r", ch );
        buffer_free( buf );
        return;
    }

    for ( gamemaster = ch->in_room->people; gamemaster != NULL; gamemaster = gamemaster->next_in_room )
    {
        if ( !IS_NPC( gamemaster ) )
            continue;
        if ( gamemaster->spec_fun == spec_lookup( "spec_gamemaster" ) )
            break;
    }

    if ( gamemaster == NULL || gamemaster->spec_fun != spec_lookup( "spec_gamemaster" ) )
    {
        send_to_char("You can't do that here.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( gamemaster->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( !is_number( arg2 ) )
    {
        send_to_char( "You must bet a number.\n\r", ch );
        buffer_free( buf );
        return;
    }

    roll = dice( 1, 5 );

    amount = atol( arg2 );

    if ( amount < 1 )
    {
        send_to_char( "Bet SOMETHING, will you?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( amount > 500 )
    {
        send_to_char( "You'll break the house!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->gold < amount )
    {
        send_to_char( "You don't have that much gold!\n\r", ch );
        buffer_free( buf );
        return;
    }

    bprintf( buf, "%s rolls the dice.\n\rThe roll is %d.\n\r", 
        gamemaster->short_descr, roll );
    send_to_char( buf->data, ch );
    
    if ( !str_cmp( arg, "odd" ) )
    {
        WAIT_STATE( ch, 10 );
        if ( roll %2 != 0 )     /* you win! */
            win( ch, amount );
        else
            lose( ch, amount );
        buffer_free( buf );
        return;
    }
    else if ( !str_cmp( arg, "even" ) )
    {
        WAIT_STATE( ch, 10 );
        if ( roll %2 == 0 )
            win( ch, amount );
        else
            lose( ch, amount );
        buffer_free( buf );
        return;
    }
    else
    {
        send_to_char( "Syntax: game even-odd <even|odd> <amount>\n\r", ch );
    }
    buffer_free( buf );
    return;
}


void game_high_low( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    long amount;
    int roll;
    CHAR_DATA *gamemaster;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: game high-low <high|low> <amount>\n\r", ch );
        buffer_free( buf );
        return;
    }

    for ( gamemaster = ch->in_room->people; gamemaster != NULL; gamemaster = gamemaster->next_in_room )
    {
        if ( !IS_NPC( gamemaster ) )
            continue;
        if ( gamemaster->spec_fun == spec_lookup( "spec_gamemaster" ) )
            break;
    }

    if ( gamemaster == NULL || gamemaster->spec_fun != spec_lookup( "spec_gamemaster" ) )
    {
        send_to_char("You can't do that here.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( gamemaster->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( !is_number( arg2 ) )
    {
        send_to_char( "You must bet a number.\n\r", ch );
        buffer_free( buf );
        return;
    }

    roll = dice( 1, 9 );

    amount = atol( arg2 );

    if ( amount < 1 )
    {
        send_to_char( "Bet SOMETHING, will you?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( amount > 500 )
    {
        send_to_char( "You'll break the house!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->gold < amount )
    {
        send_to_char( "You don't have that much gold!\n\r", ch );
        buffer_free( buf );
        return;
    }

    bprintf( buf, "%s rolls the dice.\n\rThe roll is %d.\n\r", 
        gamemaster->short_descr, roll );
    send_to_char( buf->data, ch );
    
    if ( !str_cmp( arg, "high" ) )
    {
        WAIT_STATE( ch, 10 );
        if ( roll > 6 )     /* you win! */
            win( ch, amount );
        else
            lose( ch, amount );
        buffer_free( buf );
        return;
    }
    else if ( !str_cmp( arg, "low" ) )
    {
        WAIT_STATE( ch, 10 );
        if ( roll < 6 )
            win( ch, amount );
        else
            lose( ch, amount );
        buffer_free( buf );
        return;
    }
    else
    {
        send_to_char( "Syntax: game high-low <high|low> <amount>\n\r", ch );
    }
    buffer_free( buf );
    return;
}


void game_higher_lower( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buf = buffer_new( MAX_INPUT_LENGTH );
    long amount;
    int your_roll, his_roll;
    CHAR_DATA *gamemaster;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Syntax: game higher-lower <amount>\n\r", ch );
        buffer_free( buf );
        return;
    }

    for ( gamemaster = ch->in_room->people; gamemaster != NULL; gamemaster = gamemaster->next_in_room )
    {
        if ( !IS_NPC( gamemaster ) )
            continue;
        if ( gamemaster->spec_fun == spec_lookup( "spec_gamemaster" ) )
            break;
    }

    if ( gamemaster == NULL || gamemaster->spec_fun != spec_lookup( "spec_gamemaster" ) )
    {
        send_to_char("You can't do that here.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( gamemaster->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        buffer_free( buf );
        return;
    }

    if ( !is_number( arg ) )
    {
        send_to_char( "You must bet a number.\n\r", ch );
        buffer_free( buf );
        return;
    }

    your_roll = dice( 2, 4 );
    his_roll = dice( 2, 6 );

    amount = atol( arg );

    if ( amount < 1 )
    {
        send_to_char( "Bet SOMETHING, will you?\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( amount > 500 )
    {
        send_to_char( "You'll break the house!\n\r", ch );
        buffer_free( buf );
        return;
    }

    if ( ch->gold < amount )
    {
        send_to_char( "You don't have that much gold!\n\r", ch );
        buffer_free( buf );
        return;
    }

    bprintf( buf, "%s rolls the dice and gets a %d.\n\rYour roll is %d.\n\r",
        gamemaster->short_descr, his_roll, your_roll );
    send_to_char( buf->data, ch );

    WAIT_STATE( ch, 10 );
    
    if ( your_roll > his_roll )     /* you win! */
        win( ch, amount * 1.25 );
    else
        lose( ch, amount );
    buffer_free( buf );
    return;
}

void do_game( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    
    argument = one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;

    if ( !str_cmp( arg, "higher-lower" ) )
        game_higher_lower( ch, argument );
    else if ( !str_cmp( arg, "even-odd" ) )
        game_even_odd( ch, argument );
    else if ( !str_cmp( arg, "high-low" ) )
        game_high_low( ch, argument );
    else
    {
        send_to_char( "Current games are: higher-lower, even-odd, and high-low.\n\r", ch );
        return;
    }
}
