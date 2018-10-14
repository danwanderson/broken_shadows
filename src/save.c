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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"
 
extern  int     _filbuf         (FILE *);



/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST        100
static  OBJ_DATA *      rgObjNest       [MAX_NEST];



/*
 * Local functions.
 */
void    fwrite_char     ( CHAR_DATA *ch,  FILE *fp );
void    fwrite_obj      ( CHAR_DATA *ch,  OBJ_DATA  *obj,
                            FILE *fp, int iNest );
void    fwrite_pet      ( CHAR_DATA *pet, FILE *fp);
void    fread_char      ( CHAR_DATA *ch,  FILE *fp );
void    fread_pet       ( CHAR_DATA *ch,  FILE *fp );
void    fread_obj       ( CHAR_DATA *ch,  FILE *fp );



/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    FILE *fp;

    if ( IS_NPC(ch) )
        return;

    if ( ch->desc != NULL && ch->desc->original != NULL )
        ch = ch->desc->original;

#if defined(unix)
    /* create god log */
    if (IS_IMMORTAL(ch) )
    {
        fclose(fpReserve);
        sprintf(strsave, "%s%s",GOD_DIR, capitalize(ch->name));
        if ((fp = fopen(strsave,"w")) == NULL)
        {
            bug("Save_char_obj: fopen",0);
            perror(strsave);
        }

        fprintf(fp,"Lev %2d %s%s\n",
            ch->level, ch->name, ch->pcdata->title);
        fclose( fp );
        fpReserve = fopen( NULL_FILE, "r" );
    }
#endif

    fclose( fpReserve );
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->name ) );
    if ( ( fp = fopen( PLAYER_TEMP, "w" ) ) == NULL )
    {
        bug( "Save_char_obj: fopen", 0 );
        perror( strsave );
    }
    else
    {
        fwrite_char( ch, fp );
        if ( ch->carrying != NULL )
            fwrite_obj( ch, ch->carrying, fp, 0 );
        /* save the pets */
        if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
            fwrite_pet(ch->pet,fp);
        fprintf( fp, "#END\n" );
    }
    fclose( fp );
    /* move the file */
    sprintf(buf,"mv %s %s",PLAYER_TEMP,strsave);
    system(buf);
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}



/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA *ch, FILE *fp )
{
    AFFECT_DATA *paf;
    int sn, gn, pos, i;

    fprintf( fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER" );

    fprintf( fp, "Name %s~\n",  ch->name                );
    fprintf( fp, "Vers %d\n",   3                       );
    if (ch->short_descr[0] != '\0')
        fprintf( fp, "ShD  %s~\n",      ch->short_descr );
    if( ch->long_descr[0] != '\0')
        fprintf( fp, "LnD  %s~\n",      ch->long_descr  );
    if (ch->description[0] != '\0')
        fprintf( fp, "Desc %s~\n",      ch->description );
    fprintf( fp, "Race %s~\n", pc_race_table[ch->race].name );
    fprintf( fp, "Sex  %d\n",   ch->sex                 );
    fprintf( fp, "Cla  %d\n",   ch->ch_class               );
    fprintf( fp, "Levl %d\n",   ch->level               );
    fprintf( fp, "Sec  %d\n",    ch->pcdata->security   );      /* OLC */
    fprintf( fp, "Log  %d\n",    (int)(ch->logon)       );   /* Added for finger command */
    fprintf( fp, "Plyd %d\n",
        ch->played + (int) (current_time - ch->logon)   );
    fprintf( fp, "Note %d\n",           (int)ch->last_note      );
    fprintf( fp, "Scro %d\n",   ch->lines               );
    fprintf( fp, "Room %d\n",
        (  ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
        && ch->was_in_room != NULL )
            ? ch->was_in_room->vnum
            : ch->in_room == NULL ? 3001 : ch->in_room->vnum );

    fprintf( fp, "HMV  %d %d %d %d %d %d\n",
        ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
	if ( IS_IMMORTAL( ch ) ) {
	    fprintf( fp, "Rank %d\n",	char_getImmRank( ch ) );
	}

    if (ch->gold > 0)
      fprintf( fp, "Gold %ld\n",        ch->gold                );
    else
      fprintf( fp, "Gold %d\n", 0                       ); 
    /* bank stuff added by Rahl */
    if( ch->bank > 0 )
        fprintf( fp, "Bank %ld\n",      ch->bank        );
    else
        fprintf( fp, "Bank %d\n", 0                     );
    /* bounty added by Rahl */
    if ( ch->pcdata->bounty > 0 )
        fprintf( fp, "Bounty %ld\n",    ch->pcdata->bounty      );
    else
        fprintf( fp, "Bounty %d\n", 0                   );
    /* kill counters added by Rahl */
    if ( ch->pcdata->pkills > 0 )
        fprintf( fp, "Pkills %d\n",     ch->pcdata->pkills );
    else
        fprintf( fp, "Pkills %d\n", 0   );
    if ( ch->pcdata->pkilled > 0 )
        fprintf( fp, "Pkilled %d\n",    ch->pcdata->pkilled );
    else
        fprintf( fp, "Pkilled %d\n", 0  );
    if ( ch->pcdata->killed > 0 )
        fprintf( fp, "Killed %d\n",     ch->pcdata->killed );
    else
        fprintf( fp, "Killed %d\n", 0 );

    if ( ch->pcdata->spouse != NULL )
        fprintf( fp, "Spou %s~\n", ch->pcdata->spouse   );
    fprintf( fp, "Exp  %ld\n",  ch->exp                 );
    if (ch->act != 0)
        fprintf( fp, "ActF %s\n",  print_flags (ch->act ) );
    if (ch->affected_by != 0)
        fprintf( fp, "AfByF %s\n", print_flags( ch->affected_by ) );
    if ( ch->affected2_by != 0 )
        fprintf( fp, "AfBy2 %s\n", print_flags(ch->affected2_by) );
    fprintf( fp, "CommF %s\n",  print_flags( ch->comm ) );
    /* added by Rahl */
    if (ch->wiznet)
        fprintf( fp, "WiznF %s\n",  print_flags( ch->wiznet ) );

    if (ch->invis_level != 0)
        fprintf( fp, "Invi %d\n",       ch->invis_level );
    /* added by Rahl */
    if ( ch->incog_level != 0 )
        fprintf( fp, "Inco %d\n",       ch->incog_level );
    fprintf( fp, "Pos  %d\n",   
        ch->position == POS_FIGHTING ? POS_STANDING : ch->position );
    if (ch->practice != 0)
        fprintf( fp, "Prac %d\n",       ch->practice    );
    if (ch->train != 0)
        fprintf( fp, "Trai %d\n",       ch->train       );
    if (ch->saving_throw != 0)
        fprintf( fp, "Save  %d\n",      ch->saving_throw);
    fprintf( fp, "Alig  %d\n",  ch->alignment           );
    if (ch->hitroll != 0)
        fprintf( fp, "Hit   %d\n",      ch->hitroll     );
    if (ch->damroll != 0)
        fprintf( fp, "Dam   %d\n",      ch->damroll     );
    fprintf( fp, "ACs %d %d %d %d\n",   
        ch->armor[0],ch->armor[1],ch->armor[2],ch->armor[3]);
    if (ch->wimpy !=0 )
        fprintf( fp, "Wimp  %d\n",      ch->wimpy       );
    fprintf( fp, "Attr %d %d %d %d %d\n",
        ch->perm_stat[STAT_STR],
        ch->perm_stat[STAT_INT],
        ch->perm_stat[STAT_WIS],
        ch->perm_stat[STAT_DEX],
        ch->perm_stat[STAT_CON] );

    fprintf (fp, "AMod %d %d %d %d %d\n",
        ch->mod_stat[STAT_STR],
        ch->mod_stat[STAT_INT],
        ch->mod_stat[STAT_WIS],
        ch->mod_stat[STAT_DEX],
        ch->mod_stat[STAT_CON] );

    if ( IS_NPC(ch) )
    {
        fprintf( fp, "Vnum %d\n",       ch->pIndexData->vnum    );
    }
    else
    {
        fprintf( fp, "Pass %s~\n",      ch->pcdata->pwd         );
        if (ch->pcdata->bamfin[0] != '\0')
            fprintf( fp, "Bin  %s~\n",  ch->pcdata->bamfin);
        if (ch->pcdata->bamfout[0] != '\0')
                fprintf( fp, "Bout %s~\n",      ch->pcdata->bamfout);
        fprintf( fp, "Titl %s~\n",      ch->pcdata->title       );
        fprintf( fp, "Pnts %d\n",       ch->pcdata->points      );
        fprintf( fp, "TSex %d\n",       ch->pcdata->true_sex    );
        fprintf( fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit, 
                                                   ch->pcdata->perm_mana,
                                                   ch->pcdata->perm_move);
        fprintf( fp, "Cond %d %d %d\n",
            ch->pcdata->condition[0],
            ch->pcdata->condition[1],
            ch->pcdata->condition[2] );
        smash_tilde( ch->pcdata->prompt );
        fprintf( fp, "Prom %s~\n", ch->pcdata->prompt);
        if ( ch->pcdata->clan )
            fprintf( fp, "Clan %s~\n", clan_lookup( ch->pcdata->clan ) );
        if ( ch->pcdata->clan_leader )
            fprintf( fp, "Clan_leader %d\n", ch->pcdata->clan_leader );

        if (ch->questpoints != 0)
            fprintf( fp, "QuestPnts %d\n", ch->questpoints );
        else
            fprintf( fp, "QuestPnts 0\n" );
        if (ch->nextquest != 0)
            fprintf( fp, "QuestNext %d\n", ch->nextquest );
        else
            fprintf( fp, "QuestNext 0\n" );
        if (ch->countdown != 0)
            fprintf( fp, "QuestNext %d\n", 30 );
		if ( ch->bonusPoints ) {
			fprintf( fp, "BPoints %d\n", ch->bonusPoints );
		}

    if (ch->pcdata->recall_room == NULL)
         ch->pcdata->recall_room = get_room_index( ROOM_VNUM_TEMPLE );
    fprintf( fp, "Recl %d\n",   ch->pcdata->recall_room->vnum   );

    /* email and comment added by Rahl */
    if ( ch->pcdata->email == NULL )
        fprintf( fp, "Email (none)~\n" );
    else
        fprintf( fp, "Email %s~\n",  ch->pcdata->email );
    if ( ch->pcdata->comment == NULL ) {
        fprintf( fp, "Comnt (none)~\n" );
    } else {
        fprintf( fp, "Comnt %s~\n", ch->pcdata->comment );
    }

        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (ch->pcdata->alias[pos] == NULL
            ||  ch->pcdata->alias_sub[pos] == NULL)
                break;

            smash_tilde(ch->pcdata->alias[pos]);
            fprintf(fp,"Alias %s %s~\n",ch->pcdata->alias[pos],
                    ch->pcdata->alias_sub[pos]);
        }

        /* save note board status */
        /* save number of boards in case that number changes */
        fprintf( fp, "Boards    %d ", MAX_BOARD );
        for ( i = 0; i < MAX_BOARD; i++ )
            fprintf( fp, "%s %ld ", boards[i].short_name,
                ch->pcdata->last_note[i]);
        fprintf( fp, "\n" );

        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0 )
            {
                fprintf( fp, "Sk %d '%s'\n",
                    ch->pcdata->learned[sn], skill_table[sn].name );
            }
        }

        for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name != NULL && ch->pcdata->group_known[gn])
            {
                fprintf( fp, "Gr '%s'\n",group_table[gn].name);
            }
        }
    }

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type < 0 || paf->type>= MAX_SKILL)
            continue;
        
        fprintf( fp, "AffD '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type].name,
            paf->where,
            paf->level,
            paf->duration,
            paf->modifier,
            paf->location,
            paf->bitvector
            );
    }

    fprintf( fp, "End\n\n" );
    return;
}

/* write a pet */
void fwrite_pet( CHAR_DATA *pet, FILE *fp)
{
    AFFECT_DATA *paf;
    
    fprintf(fp,"#PET\n");
    
    fprintf(fp,"Vnum %d\n",pet->pIndexData->vnum);
    
    fprintf(fp,"Name %s~\n", pet->name);
    if (pet->short_descr != pet->pIndexData->short_descr)
        fprintf(fp,"ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
        fprintf(fp,"LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
        fprintf(fp,"Desc %s~\n", pet->description);
    if (pet->race != pet->pIndexData->race)
        fprintf(fp,"Race %s~\n", race_table[pet->race].name);
    fprintf(fp,"Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
        fprintf(fp,"Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
        pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
    if (pet->gold > 0)
        fprintf(fp,"Gold %ld\n",pet->gold);
    if (pet->exp > 0)
        fprintf(fp, "Exp  %ld\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
        fprintf( fp, "ActF %s\n", print_flags( pet->act ) );
    if (pet->affected_by != pet->pIndexData->affected_by)
        fprintf( fp, "AfByF %s\n", print_flags( pet->affected_by ) );
    /* added by Rahl */
    if ( pet->affected2_by != pet->pIndexData->affected2_by )
        fprintf( fp, "AfBy2 %s\n",      print_flags( pet->affected2_by ));
    if (pet->comm != 0)
        fprintf( fp, "CommF %s\n", print_flags( pet->comm ) );
    fprintf(fp,"Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
        fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
        fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
        fprintf(fp, "Hit  %d\n", pet->hitroll);
    if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
        fprintf(fp, "Dam  %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n",
        pet->armor[0],pet->armor[1],pet->armor[2],pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d\n",
        pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
        pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
        pet->perm_stat[STAT_CON]);
    fprintf(fp, "AMod %d %d %d %d %d\n",
        pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
        pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
        pet->mod_stat[STAT_CON]);
    
    for ( paf = pet->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;
            
        fprintf(fp, "AffD '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type].name, paf->where,
            paf->level, paf->duration, paf->modifier,paf->location,
            paf->bitvector);
    }
    
    fprintf(fp,"End\n");
    return;
}
    
/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if ( obj->next_content != NULL )
        fwrite_obj( ch, obj->next_content, fp, iNest );

    /*
     * Castrate storage characters.
     */
    if ( (ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER)
    ||   obj->item_type == ITEM_KEY
    ||   (obj->item_type == ITEM_MAP && !obj->value[0]))
        return; 

    fprintf( fp, "#O\n" );
    fprintf( fp, "Vnum %d\n",   obj->pIndexData->vnum        );
    if (obj->enchanted)
        fprintf( fp,"Enchanted\n");
    fprintf( fp, "Nest %d\n",   iNest                );

    /* these data are only used if they do not match the defaults */

    if ( obj->name != obj->pIndexData->name)
        fprintf( fp, "Name %s~\n",      obj->name                    );
    if ( obj->short_descr != obj->pIndexData->short_descr)
        fprintf( fp, "ShD  %s~\n",      obj->short_descr             );
    if ( obj->description != obj->pIndexData->description)
        fprintf( fp, "Desc %s~\n",      obj->description             );
    if ( obj->extra_flags != obj->pIndexData->extra_flags)
        fprintf( fp, "ExtraF %s\n", print_flags( obj->extra_flags ) );
    if ( obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf( fp, "WearF %s\n", print_flags( obj->wear_flags ) );
    if ( obj->item_type != obj->pIndexData->item_type)
        fprintf( fp, "Ityp %d\n",       obj->item_type               );
    if ( obj->weight != obj->pIndexData->weight)
        fprintf( fp, "Wt   %d\n",       obj->weight                  );

    /* variable data */

    fprintf( fp, "Wear %d\n",   obj->wear_loc                );
    if (obj->level != 0)
        fprintf( fp, "Lev  %d\n",       obj->level                   );
    if (obj->timer != 0)
        fprintf( fp, "Time %d\n",       obj->timer           );
    fprintf( fp, "Cost %d\n",   obj->cost                    );
    if (obj->value[0] != obj->pIndexData->value[0]
    ||  obj->value[1] != obj->pIndexData->value[1]
    ||  obj->value[2] != obj->pIndexData->value[2]
    ||  obj->value[3] != obj->pIndexData->value[3]
    ||  obj->value[4] != obj->pIndexData->value[4]) 
        fprintf( fp, "Val  %d %d %d %d %d\n",
            obj->value[0], obj->value[1], obj->value[2], obj->value[3],
            obj->value[4]            );

    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
        if ( obj->value[1] > 0 )
        {
            fprintf( fp, "Spell 1 '%s'\n", 
                skill_table[obj->value[1]].name );
        }

        if ( obj->value[2] > 0 )
        {
            fprintf( fp, "Spell 2 '%s'\n", 
                skill_table[obj->value[2]].name );
        }

        if ( obj->value[3] > 0 )
        {
            fprintf( fp, "Spell 3 '%s'\n", 
                skill_table[obj->value[3]].name );
        }

        break;

    case ITEM_PILL:
    case ITEM_STAFF:
    case ITEM_WAND:
        if ( obj->value[3] > 0 )
        {
            fprintf( fp, "Spell 3 '%s'\n", 
                skill_table[obj->value[3]].name );
        }

        break;
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;
        fprintf( fp, "AffD '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type].name,
            paf->where,
            paf->level,
            paf->duration,
            paf->modifier,
            paf->location,
            paf->bitvector
            );
    }

    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
        fprintf( fp, "ExDe %s~ %s~\n",
            ed->keyword, ed->description );
    }

    fprintf( fp, "End\n\n" );

    if ( obj->contains != NULL )
        fwrite_obj( ch, obj->contains, fp, iNest + 1 );

    return;
}



/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name )
{
    static PC_DATA pcdata_zero;
    char strsave[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH*2];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;

    if ( char_free == NULL )
    {
        ch                              = alloc_perm( sizeof(*ch) );
    }
    else
    {
        ch                              = char_free;
        char_free                       = char_free->next;
    }
    clear_char( ch );

    if ( pcdata_free == NULL )
    {
        ch->pcdata                      = alloc_perm( sizeof(*ch->pcdata) );
    }
    else
    {
        ch->pcdata                      = pcdata_free;
        pcdata_free                     = pcdata_free->next;
    }
    *ch->pcdata                         = pcdata_zero;

    d->character                        = ch;
    ch->desc                            = d;
    ch->name                            = str_dup( name );
    ch->version                         = 0;
    ch->race                            = race_lookup("human");
    ch->affected_by                     = 0;
/*
    ch->affected2_by                    = 0;
 */
    ch->affected2_by    = ch->affected2_by|race_table[ch->race].aff2;
    ch->act                             = PLR_NOSUMMON
                                        | PLR_AUTOSAC
                                        | PLR_AUTOLOOT
                                        | PLR_AUTOGOLD
                                        | PLR_AUTOEXIT;
    ch->comm                            = COMM_COMBINE 
                                        | COMM_PROMPT;
    /* every char starts in the default board from login. This board
     * should be read_level == 0
     */
    ch->pcdata->board                   = &boards[DEFAULT_BOARD];

    ch->invis_level                     = 0;
/* added by Rahl */
    ch->incog_level                     = 0;
    ch->practice                        = 0;
    ch->train                           = 0;
    ch->hitroll                         = 0;
    ch->damroll                         = 0;
    ch->wimpy                           = 0;
    ch->saving_throw                    = 0;
    ch->pcdata->points                  = 0;
    ch->pcdata->confirm_delete          = FALSE;
    ch->pcdata->pwd                     = str_dup( "" );
    ch->pcdata->bamfin                  = str_dup( "" );
    ch->pcdata->bamfout                 = str_dup( "" );
    ch->pcdata->title                   = str_dup( "" );
    for (stat =0; stat < MAX_STATS; stat++)
        ch->perm_stat[stat]             = 13;
    ch->pcdata->perm_hit                = 0;
    ch->pcdata->perm_mana               = 0;
    ch->pcdata->perm_move               = 0;
    ch->pcdata->true_sex                = 0;
    ch->pcdata->condition[COND_THIRST]  = 48; 
    ch->pcdata->condition[COND_FULL]    = 48;
    ch->pcdata->security                = 0;    /* OLC */
    ch->pcdata->prompt                  = str_dup( "%i`K/`W%H`w HP %n`K/`W%M`w MP %w`K/`W%V`w MV `K> ");
    ch->pcdata->clan                    = 0;
    ch->questpoints                     = 0;
    ch->nextquest                       = 0;
    /* bank and bounty added by Rahl */
    ch->bank                            = 0;
    ch->pcdata->bounty                  = 0;
    ch->pcdata->recall_room             = get_room_index( ROOM_VNUM_TEMPLE );
    /* added by Rahl */
    ch->pcdata->email                   = strdup( "(none)" );
    ch->pcdata->comment                 = strdup( "(none)" );
	ch->pcdata->away_message			= NULL;
    ch->pcdata->buffer                  = buffer_new(1000);
    ch->pcdata->message_ctr             = 0;
    ch->pcdata->kills                   = 0;
    ch->pcdata->killed                  = 0;
    ch->pcdata->pkills                  = 0;
    ch->pcdata->pkilled                 = 0;
    ch->pcdata->clan_leader             = 0;
	ch->bonusPoints						= 0;
	char_setImmRank( ch, 0 );
    found = FALSE;
    fclose( fpReserve );
    
    #if defined(unix)
    /* decompress if .gz file exists */
    sprintf( strsave, "%s%s%s", PLAYER_DIR, capitalize(name),".gz");
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
        fclose(fp);
        sprintf(buf,"gzip -dfq %s",strsave);
        system(buf);
    }
    #endif

    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( name ) );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
        int iNest;

        for ( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

        found = TRUE;
        for ( ; ; )
        {
            char letter;
            char *word;

            letter = fread_letter( fp );
            if ( letter == '*' )
            {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' )
            {
                bug( "Load_char_obj: # not found.", 0 );
                break;
            }

            word = fread_word( fp );
            if      ( !str_cmp( word, "PLAYER" ) ) fread_char ( ch, fp );
            else if ( !str_cmp( word, "OBJECT" ) ) fread_obj  ( ch, fp );
            else if ( !str_cmp( word, "O"      ) ) fread_obj  ( ch, fp );
            else if ( !str_cmp( word, "PET"    ) ) fread_pet  ( ch, fp );
            else if ( !str_cmp( word, "END"    ) ) break;
            else
            {
                bug( "Load_char_obj: bad section.", 0 );
                break;
            }
        }
        fclose( fp );
    }

    fpReserve = fopen( NULL_FILE, "r" );


    /* initialize race */
    if (found)
    {
        int i;

        if (ch->race == 0)
            ch->race = race_lookup("human");

        ch->size = pc_race_table[ch->race].size;
        ch->dam_type = 17; /*punch */

        for (i = 0; i < 5; i++)
        {
            if (pc_race_table[ch->race].skills[i] == NULL)
                break;
            group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
        }
        ch->affected_by = ch->affected_by|race_table[ch->race].aff;
        ch->imm_flags   = ch->imm_flags | race_table[ch->race].imm;
        ch->res_flags   = ch->res_flags | race_table[ch->race].res;
        ch->vuln_flags  = ch->vuln_flags | race_table[ch->race].vuln;
        ch->form        = race_table[ch->race].form;
        ch->parts       = race_table[ch->race].parts;
    }


    return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                                    \
                                if ( !str_cmp( word, literal ) )        \
                                {                                       \
                                    field  = value;                     \
                                    fMatch = TRUE;                      \
                                    break;                              \
                                }

void fread_char( CHAR_DATA *ch, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word = NULL;
    int count = 0;
    bool fMatch = FALSE;

    for ( ; ; )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER(word[0]) )
        {
        case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

        case 'A':
            KEY( "Act",         ch->act,                fread_number( fp ) );
            KEY( "ActF",        ch->act,                fread_flag( fp ) );
            KEY( "AffectedBy",  ch->affected_by,        fread_number( fp ) );
            KEY( "AfBy",        ch->affected_by,        fread_number( fp ) );
            KEY( "AfByF",       ch->affected_by,        fread_flag( fp ) );
            KEY( "Alignment",   ch->alignment,          fread_number( fp ) );
            KEY( "Alig",        ch->alignment,          fread_number( fp ) );
            /* added by Rahl */
            KEY( "AfBy2",       ch->affected2_by,       fread_flag( fp ) );

            if (!str_cmp( word, "Alias"))
            {
                if (count >= MAX_ALIAS)
                {
                    fread_to_eol(fp);
                    fMatch = TRUE;
                    break;
                }

                ch->pcdata->alias[count]        = str_dup(fread_word(fp));
                ch->pcdata->alias_sub[count]    = fread_string(fp);
                count++;
                fMatch = TRUE;
                break;
            }

            if (!str_cmp( word, "AC") || !str_cmp(word,"Armor"))
            {
                fread_to_eol(fp);
                fMatch = TRUE;
                break;
            }

            if (!str_cmp(word,"ACs"))
            {
                int i;

                for (i = 0; i < 4; i++)
                    ch->armor[i] = fread_number(fp);
                fMatch = TRUE;
                break;
            }

            if ( !str_cmp( word, "Affect" ) || !str_cmp( word, "Aff" ) 
            ||   !str_cmp( word, "AffD"))
            {
                AFFECT_DATA *paf;

                if ( affect_free == NULL )
                {
                    paf         = alloc_perm( sizeof(*paf) );
                }
                else
                {
                    paf         = affect_free;
                    affect_free = affect_free->next;
                }

                if (!str_cmp(word,"AffD"))
                {
                    int sn;
                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_char: unknown skill.",0);
                    else
                        paf->type = sn;
                }
                else  /* old form */
                    paf->type   = fread_number( fp );
                paf->where      = fread_number( fp );
                if (ch->version == 0)
                  paf->level = ch->level;
                else
                  paf->level    = fread_number( fp );
                paf->duration   = fread_number( fp );
                paf->modifier   = fread_number( fp );
                paf->location   = fread_number( fp );
                paf->bitvector  = fread_number( fp );
                paf->next       = ch->affected;
                ch->affected    = paf;
                fMatch = TRUE;
                break;
            }

            if ( !str_cmp( word, "AttrMod"  ) || !str_cmp(word,"AMod"))
            {
                int stat;
                for (stat = 0; stat < MAX_STATS; stat ++)
                   ch->mod_stat[stat] = fread_number(fp);
                fMatch = TRUE;
                break;
            }

            if ( !str_cmp( word, "AttrPerm" ) || !str_cmp(word,"Attr"))
            {
                int stat;

                for (stat = 0; stat < MAX_STATS; stat++)
                    ch->perm_stat[stat] = fread_number(fp);
                fMatch = TRUE;
                break;
            }
            break;

        case 'B':
            KEY( "Bamfin",      ch->pcdata->bamfin,     fread_string( fp ) );
            KEY( "Bamfout",     ch->pcdata->bamfout,    fread_string( fp ) );
            KEY( "Bin",         ch->pcdata->bamfin,     fread_string( fp ) );
            KEY( "Bout",        ch->pcdata->bamfout,    fread_string( fp ) );
        /* bank stuff added by Rahl */
            KEY( "Bank",        ch->bank,               fread_number( fp ) );
        /* bounty added by Rahl */
            KEY( "Bounty",      ch->pcdata->bounty,     fread_number( fp ) );

			KEY( "BPoints",		ch->bonusPoints,		fread_number( fp ) );
            /* Read in board status */
            if (!str_cmp(word, "Boards" ))
            {
                int i,num = fread_number (fp); /* number of boards saved */
                char *boardname;

                for (; num ; num-- ) /* for each of the board saved */
                {
                    boardname = fread_word (fp);
                    i = board_lookup (boardname); /* find board number */
 
                    if (i == BOARD_NOTFOUND) /* Does board still exist? */
                    {
                        sprintf (buf, "fread_char: %s had unknown board name: %s. Skipped.", ch->name, boardname);
                        log_string (buf);
                        fread_number (fp); /* read last_note and skip info */
                    }                               
                    else /* Save it */
                        ch->pcdata->last_note[i] = fread_number(fp);
               }        /* for */

               fMatch = TRUE;
            } /* Boards */

            break;

        case 'C':
            KEY( "Class",       ch->ch_class,              fread_number( fp ) );
            KEY( "Cla",         ch->ch_class,              fread_number( fp ) );
            KEY( "Clan",        ch->pcdata->clan,       get_clan( fread_string( fp ) ) );
            KEY( "Clan_leader", ch->pcdata->clan_leader, fread_number( fp ) );

            if ( !str_cmp( word, "Condition" ) || !str_cmp(word,"Cond"))
            {
                ch->pcdata->condition[0] = fread_number( fp );
                ch->pcdata->condition[1] = fread_number( fp );
                ch->pcdata->condition[2] = fread_number( fp );
                fMatch = TRUE;
                break;
            }
            KEY("Prom",         ch->pcdata->prompt,     fread_string( fp ) );
            KEY("Comm",         ch->comm,               fread_number( fp ) );
            KEY("CommF",        ch->comm,               fread_flag( fp ) ); 
            KEY("Comnt",        ch->pcdata->comment,    fread_string( fp ) );
            break;

        case 'D':
            KEY( "Damroll",     ch->damroll,            fread_number( fp ) );
            KEY( "Dam",         ch->damroll,            fread_number( fp ) );
            KEY( "Description", ch->description,        fread_string( fp ) );
            KEY( "Desc",        ch->description,        fread_string( fp ) );
            break;

        case 'E':
            if ( !str_cmp( word, "End" ) )
                return;
            KEY( "Exp",         ch->exp,                fread_number( fp ) );
            KEY( "Email",       ch->pcdata->email,      fread_string( fp ) );
            break;

        case 'G':
            KEY( "Gold",        ch->gold,               fread_number( fp ) );
            if ( !str_cmp( word, "Group" )  || !str_cmp(word,"Gr"))
            {
                int gn;
                char *temp;
 
                temp = fread_word( fp ) ;
                gn = group_lookup(temp);
                /* gn    = group_lookup( fread_word( fp ) ); */
                if ( gn < 0 )
                {
                    fprintf(stderr,"%s",temp);
                    bug( "Fread_char: unknown group. ", 0 );
                }
                else
                    gn_add(ch,gn);
                fMatch = TRUE;
            }
            break;

        case 'H':
            KEY( "Hitroll",     ch->hitroll,            fread_number( fp ) );
            KEY( "Hit",         ch->hitroll,            fread_number( fp ) );

            if ( !str_cmp( word, "HpManaMove" ) || !str_cmp(word,"HMV"))
            {
                ch->hit         = fread_number( fp );
                ch->max_hit     = fread_number( fp );
                ch->mana        = fread_number( fp );
                ch->max_mana    = fread_number( fp );
                ch->move        = fread_number( fp );
                ch->max_move    = fread_number( fp );
                fMatch = TRUE;
                break;
            }

            if ( !str_cmp( word, "HpManaMovePerm" ) || !str_cmp(word,"HMVP"))
            {
                ch->pcdata->perm_hit    = fread_number( fp );
                ch->pcdata->perm_mana   = fread_number( fp );
                ch->pcdata->perm_move   = fread_number( fp );
                fMatch = TRUE;
                break;
            }
      
            break;

        case 'I':
            KEY( "InvisLevel",  ch->invis_level,        fread_number( fp ) );
            KEY( "Invi",        ch->invis_level,        fread_number( fp ) );
        /* added by Rahl */
            KEY( "Inco",        ch->incog_level,        fread_number( fp ) );
            break;

        /* added by Rahl */
        case 'K':
            KEY( "Kills",       ch->pcdata->kills,      fread_number( fp ) );
            KEY( "Killed",      ch->pcdata->killed,     fread_number( fp ) );
            break;

        case 'L':
            KEY( "Level",       ch->level,              fread_number( fp ) );
            KEY( "Lev",         ch->level,              fread_number( fp ) );
            KEY( "Levl",        ch->level,              fread_number( fp ) );
            KEY( "LongDescr",   ch->long_descr,         fread_string( fp ) );
            KEY( "LnD",         ch->long_descr,         fread_string( fp ) );
          if ( !str_cmp( word, "Log" ))
            {
                fread_number(fp);
                fMatch = TRUE;
                break;
            }
            break;

        case 'N':
            KEY( "Name",        ch->name,               fread_string( fp ) );
            KEY( "Note",        ch->last_note,          fread_number( fp ) );
            break;

        case 'P':
            KEY( "Password",    ch->pcdata->pwd,        fread_string( fp ) );
            KEY( "Pass",        ch->pcdata->pwd,        fread_string( fp ) );
            KEY( "Played",      ch->played,             fread_number( fp ) );
            KEY( "Plyd",        ch->played,             fread_number( fp ) );
            KEY( "Points",      ch->pcdata->points,     fread_number( fp ) );
            KEY( "Pnts",        ch->pcdata->points,     fread_number( fp ) );
            KEY( "Position",    ch->position,           fread_number( fp ) );
            KEY( "Pos",         ch->position,           fread_number( fp ) );
            KEY( "Practice",    ch->practice,           fread_number( fp ) );
            KEY( "Prac",        ch->practice,           fread_number( fp ) );
            KEY( "Prom",        ch->pcdata->prompt,     fread_string( fp ) );
            KEY( "Pkills",      ch->pcdata->pkills,     fread_number( fp ) );
            KEY( "Pkilled",     ch->pcdata->pkilled,    fread_number( fp ) );
            break;

/* case Q added for autoquesting - by Rahl */
        case 'Q':
            KEY( "QuestPnts",   ch->questpoints,        fread_number( fp ) );
            KEY( "QuestNext",   ch->nextquest,          fread_number( fp ) );

        case 'R':
            KEY( "Race",        ch->race,       
                                race_lookup(fread_string( fp )) );

			if ( !str_cmp( word, "Rank" ) ) {
		 		char_setImmRank( ch, fread_number( fp ) );
				fMatch = TRUE;
				break;
			}

            if ( !str_cmp( word, "Recl" ) )
            {
                ch->pcdata->recall_room = get_room_index( fread_number( fp ) );
                if ( ch->pcdata->recall_room == NULL )
                    ch->pcdata->recall_room = get_room_index( ROOM_VNUM_TEMPLE );
                fMatch = TRUE;
                break;
            }
            if ( !str_cmp( word, "Room" ) )
            {
                ch->in_room = get_room_index( fread_number( fp ) );
                if ( ch->in_room == NULL )
                    ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
                fMatch = TRUE;
                break;
            }

            break;

        case 'S':
            KEY( "SavingThrow", ch->saving_throw,       fread_number( fp ) );
            KEY( "Save",        ch->saving_throw,       fread_number( fp ) );
            KEY( "Scro",        ch->lines,              fread_number( fp ) );
            KEY( "Sex",         ch->sex,                fread_number( fp ) );
            KEY( "ShortDescr",  ch->short_descr,        fread_string( fp ) );
            KEY( "ShD",         ch->short_descr,        fread_string( fp ) );
            KEY( "Sec",         ch->pcdata->security,   fread_number( fp ) );   /* OLC */
            KEY( "Spou",        ch->pcdata->spouse,     fread_string( fp ) );

            if ( !str_cmp( word, "Skill" ) || !str_cmp(word,"Sk"))
            {
                int sn;
                int value;
                char *temp;

                value = fread_number( fp );
                temp = fread_word( fp ) ;
                sn = skill_lookup(temp);
                /* sn    = skill_lookup( fread_word( fp ) ); */
                if ( sn < 0 )
                {
                    fprintf(stderr,"%s",temp);
                    bug( "Fread_char: unknown skill. ", 0 );
                }
                else
                    ch->pcdata->learned[sn] = value;
                fMatch = TRUE;
            }

            break;

        case 'T':
            KEY( "TrueSex",     ch->pcdata->true_sex,   fread_number( fp ) );
            KEY( "TSex",        ch->pcdata->true_sex,   fread_number( fp ) );
            KEY( "Trai",        ch->train,              fread_number( fp ) );
			KEY( "TPoints",		ch->bonusPoints,		fread_number( fp ) );

            if ( !str_cmp( word, "Title" )  || !str_cmp( word, "Titl"))
            {
                ch->pcdata->title = fread_string( fp );
                if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ',' 
                &&  ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?')
                {
                    sprintf( buf, " %s", ch->pcdata->title );
                    free_string( ch->pcdata->title );
                    ch->pcdata->title = str_dup( buf );
                }
                fMatch = TRUE;
                break;
            }

            break;

        case 'V':
            KEY( "Version",     ch->version,            fread_number ( fp ) );
            KEY( "Vers",        ch->version,            fread_number ( fp ) );
            if ( !str_cmp( word, "Vnum" ) )
            {
                ch->pIndexData = get_mob_index( fread_number( fp ) );
                fMatch = TRUE;
                break;
            }
            break;

        case 'W':
            KEY( "Wimpy",       ch->wimpy,              fread_number( fp ) );
            KEY( "Wimp",        ch->wimpy,              fread_number( fp ) );
        /* added by Rahl */
            KEY( "Wizn",        ch->wiznet,             fread_number( fp ) );
            KEY( "WiznF",       ch->wiznet,             fread_flag( fp ) );
            break;

		}

        if ( !fMatch )
        {
            bug( "Fread_char: no match.", 0 );
            fread_to_eol( fp );
        }
    }
}

/* load a pet from the forgotten reaches */
void fread_pet( CHAR_DATA *ch, FILE *fp )
{
    char *word;
    CHAR_DATA *pet;
    bool fMatch;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word,"Vnum"))
    {
        int vnum;
        
        vnum = fread_number(fp);
        if (get_mob_index(vnum) == NULL)
        {
            bug("Fread_pet: bad vnum %d.",vnum);
            pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
        }
        else
            pet = create_mobile(get_mob_index(vnum));
    }
    else
    {
        bug("Fread_pet: no vnum in file.",0);
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }
    
    for ( ; ; )
    {
        word    = feof(fp) ? "END" : fread_word(fp);
        fMatch = FALSE;
        
        switch (UPPER(word[0]))
        {
        case '*':
            fMatch = TRUE;
            fread_to_eol(fp);
            break;
                
        case 'A':
            KEY( "Act",         pet->act,               fread_number(fp));
            KEY( "ActF",        pet->act,               fread_flag( fp ) );
            KEY( "AfBy",        pet->affected_by,       fread_number(fp));
            KEY( "AfByF",       pet->affected_by,       fread_flag( fp ) );
            KEY( "Alig",        pet->alignment,         fread_number(fp));
            /* added by Rahl */
            KEY( "AfBy2",       pet->affected2_by,      fread_flag( fp ) );

            if (!str_cmp(word,"ACs"))
            {
                int i;
                
                for (i = 0; i < 4; i++)
                    pet->armor[i] = fread_number(fp);
                fMatch = TRUE;
                break;
            }
            
            if (!str_cmp(word,"AffD"))
            {
                AFFECT_DATA *paf;
                int sn;
                
                if (affect_free == NULL)
                    paf = alloc_perm(sizeof(*paf));
                else
                {
                    paf = affect_free;
                    affect_free = affect_free->next;
                }
                
                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.",0);
                else
                   paf->type = sn;
                
                paf->where      = fread_number(fp);   
                paf->level      = fread_number(fp);
                paf->duration   = fread_number(fp);
                paf->modifier   = fread_number(fp);
                paf->location   = fread_number(fp);
                paf->bitvector  = fread_number(fp);
                paf->next       = pet->affected;
                pet->affected   = paf;
                fMatch          = TRUE;
                break;
            }
             
            if (!str_cmp(word,"AMod"))
            {
                int stat;
                
                for (stat = 0; stat < MAX_STATS; stat++)
                    pet->mod_stat[stat] = fread_number(fp);
                fMatch = TRUE;
                break;
            }
             
            if (!str_cmp(word,"Attr"))
            {
                 int stat;
                 
                 for (stat = 0; stat < MAX_STATS; stat++)
                     pet->perm_stat[stat] = fread_number(fp);
                 fMatch = TRUE;
                 break;
            }
            break;
             
         case 'C':
             KEY( "Comm",       pet->comm,              fread_number(fp));
             KEY( "CommF",      pet->comm,              fread_flag( fp ) );
             break;
             
         case 'D':
             KEY( "Dam",        pet->damroll,           fread_number(fp));
             KEY( "Desc",       pet->description,       fread_string(fp));
             break;
             
         case 'E':
             if (!str_cmp(word,"End"))
             {
                pet->leader = ch;
                pet->master = ch;
                ch->pet = pet;
                return;
             }
             KEY( "Exp",        pet->exp,               fread_number(fp));
             break;
             
         case 'G':
             KEY( "Gold",       pet->gold,              fread_number(fp));
             break;
             
         case 'H':
             KEY( "Hit",        pet->hitroll,           fread_number(fp));
             
             if (!str_cmp(word,"HMV"))
             {
                pet->hit        = fread_number(fp);
                pet->max_hit    = fread_number(fp);
                pet->mana       = fread_number(fp);
                pet->max_mana   = fread_number(fp);
                pet->move       = fread_number(fp);
                pet->max_move   = fread_number(fp);
                fMatch = TRUE;
                break;
             }
             break;
             
        case 'L':
             KEY( "Levl",       pet->level,             fread_number(fp));
             KEY( "LnD",        pet->long_descr,        fread_string(fp));
             break;
             
        case 'N':
             KEY( "Name",       pet->name,              fread_string(fp));
             break;
             
        case 'P':
             KEY( "Pos",        pet->position,          fread_number(fp));
             break;
             
        case 'R':
            KEY( "Race",        pet->race, race_lookup(fread_string(fp)));
            break;
            
        case 'S' :
            KEY( "Save",        pet->saving_throw,      fread_number(fp));
            KEY( "Sex",         pet->sex,               fread_number(fp));
            KEY( "ShD",         pet->short_descr,       fread_string(fp));
            break;
            
        if ( !fMatch )
        {
            bug("Fread_pet: no match.",0);
            fread_to_eol(fp);
        }
        
        }
    }
    
}



void fread_obj( CHAR_DATA *ch, FILE *fp )
{
    static OBJ_DATA obj_zero;
    OBJ_DATA *obj;
    char *word;
    int iNest;
    bool fMatch = FALSE;
    bool fNest;
    bool fVnum;
    bool first;
    bool make_new;    /* update object */
    
    fVnum = FALSE;
    obj = NULL;
    first = TRUE;  /* used to counter fp offset */
    make_new = FALSE;

    word   = feof( fp ) ? "End" : fread_word( fp );
    if (!str_cmp(word,"Vnum" ))
    {
        int vnum;
        first = FALSE;  /* fp will be in right place */
 
        vnum = fread_number( fp );
        if (  get_obj_index( vnum )  == NULL )
        {
            bug( "Fread_obj: bad vnum %d.", vnum );
        }
        else
        {
            obj = create_object(get_obj_index(vnum),-1);
        }
            
    }

    if (obj == NULL)  /* either not found or old style */
    {
        if ( obj_free == NULL )
        {
            obj         = alloc_perm( sizeof(*obj) );
        }
        else
        {
            obj         = obj_free;
            obj_free    = obj_free->next;
        }

        *obj            = obj_zero;
        obj->name               = str_dup( "" );
        obj->short_descr        = str_dup( "" );
        obj->description        = str_dup( "" );
    }

    fNest               = FALSE;
    fVnum               = TRUE;
    iNest               = 0;

    for ( ; ; )
    {
        if (first) {
            first = FALSE;
        } else {
            word   = feof( fp ) ? "End" : fread_word( fp );
		}
        fMatch = FALSE;

        switch ( UPPER(word[0]) )
        {
        case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

        case 'A':
            if ( !str_cmp( word, "Affect" ) || !str_cmp(word,"Aff")
            ||   !str_cmp( word, "AffD"))
            {
                AFFECT_DATA *paf;

                if ( affect_free == NULL )
                {
                    paf         = alloc_perm( sizeof(*paf) );
                }
                else
                {
                    paf         = affect_free;
                    affect_free = affect_free->next;
                }

                if (!str_cmp(word, "AffD"))
                {
                    int sn;
                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_obj: unknown skill.",0);
                    else
                        paf->type = sn;
                }
                else /* old form */
                    paf->type   = fread_number( fp );
                paf->where      = fread_number( fp );
                if (ch->version == 0)
                  paf->level = 20;
                else
                  paf->level    = fread_number( fp );
                paf->duration   = fread_number( fp );
                paf->modifier   = fread_number( fp );
                paf->location   = fread_number( fp );
                paf->bitvector  = fread_number( fp );
                paf->next       = obj->affected;
                obj->affected   = paf;
                fMatch          = TRUE;
                break;
            }
            break;

        case 'C':
            KEY( "Cost",        obj->cost,              fread_number( fp ) );
            break;

        case 'D':
            KEY( "Description", obj->description,       fread_string( fp ) );
            KEY( "Desc",        obj->description,       fread_string( fp ) );
            break;

        case 'E':

            if ( !str_cmp( word, "Enchanted"))
            {
                obj->enchanted = TRUE;
                fMatch  = TRUE;
                break;
            }

            KEY( "ExtraFlags",  obj->extra_flags,       fread_number( fp ) );
            KEY( "ExtF",        obj->extra_flags,       fread_number( fp ) );
            KEY( "ExtraF",      obj->extra_flags,       fread_flag( fp ) );

            if ( !str_cmp( word, "ExtraDescr" ) || !str_cmp(word,"ExDe"))
            {
                EXTRA_DESCR_DATA *ed;

                if ( extra_descr_free == NULL )
                {
                    ed                  = alloc_perm( sizeof(*ed) );
                }
                else
                {
                    ed                  = extra_descr_free;
                    extra_descr_free    = extra_descr_free->next;
                }

                ed->keyword             = fread_string( fp );
                ed->description         = fread_string( fp );
                ed->next                = obj->extra_descr;
                obj->extra_descr        = ed;
                fMatch = TRUE;
            }

            if ( !str_cmp( word, "End" ) )
            {
                if ( !fNest || ( fVnum && obj->pIndexData == NULL ) )
                {
                    bug( "Fread_obj: incomplete object.", 0 );
                    free_string( obj->name        );
                    free_string( obj->description );
                    free_string( obj->short_descr );
                    obj->next = obj_free;
                    obj_free  = obj;
                    return;
                }
                else
                {
                    if ( !fVnum )
                    {
                        free_string( obj->name        );
                        free_string( obj->description );
                        free_string( obj->short_descr );
                        obj->next = obj_free;
                        obj_free  = obj;

                        obj = create_object( get_obj_index( OBJ_VNUM_DUMMY ), 0 );
                    }

                    if (make_new)
                    {
                        int wear;
                        
                        wear = obj->wear_loc;
                        extract_obj(obj);

                        obj = create_object(obj->pIndexData,0);
                        obj->wear_loc = wear;
                    }
                    if ( iNest == 0 || rgObjNest[iNest] == NULL )
                        obj_to_char( obj, ch );
                    else
                        obj_to_obj( obj, rgObjNest[iNest-1] );
                    return;
                }
            }
            break;

        case 'I':
            KEY( "ItemType",    obj->item_type,         fread_number( fp ) );
            KEY( "Ityp",        obj->item_type,         fread_number( fp ) );
            break;

        case 'L':
            KEY( "Level",       obj->level,             fread_number( fp ) );
            KEY( "Lev",         obj->level,             fread_number( fp ) );
            break;

        case 'N':
            KEY( "Name",        obj->name,              fread_string( fp ) );

            if ( !str_cmp( word, "Nest" ) )
            {
                iNest = fread_number( fp );
                if ( iNest < 0 || iNest >= MAX_NEST )
                {
                    bug( "Fread_obj: bad nest %d.", iNest );
                }
                else
                {
                    rgObjNest[iNest] = obj;
                    fNest = TRUE;
                }
                fMatch = TRUE;
            }
            break;

        case 'O':
            break;
                    

        case 'S':
            KEY( "ShortDescr",  obj->short_descr,       fread_string( fp ) );
            KEY( "ShD",         obj->short_descr,       fread_string( fp ) );

            if ( !str_cmp( word, "Spell" ) )
            {
                int iValue;
                int sn;

                iValue = fread_number( fp );
                sn     = skill_lookup( fread_word( fp ) );
                if ( iValue < 0 || iValue > 3 )
                {
                    bug( "Fread_obj: bad iValue %d.", iValue );
                }
                else if ( sn < 0 )
                {
                    bug( "Fread_obj: unknown skill.", 0 );
                }
                else
                {
                    obj->value[iValue] = sn;
                }
                fMatch = TRUE;
                break;
            }

            break;

        case 'T':
            KEY( "Timer",       obj->timer,             fread_number( fp ) );
            KEY( "Time",        obj->timer,             fread_number( fp ) );
            break;

        case 'V':
            if ( !str_cmp( word, "Values" ) || !str_cmp(word,"Vals"))
            {
                obj->value[0]   = fread_number( fp );
                obj->value[1]   = fread_number( fp );
                obj->value[2]   = fread_number( fp );
                obj->value[3]   = fread_number( fp );
                if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                   obj->value[0] = obj->pIndexData->value[0];
                fMatch          = TRUE;
                break;
            }

            if ( !str_cmp( word, "Val" ) )
            {
                obj->value[0]   = fread_number( fp );
                obj->value[1]   = fread_number( fp );
                obj->value[2]   = fread_number( fp );
                obj->value[3]   = fread_number( fp );
                obj->value[4]   = fread_number( fp );
                fMatch = TRUE;
                break;
            }

            if ( !str_cmp( word, "Vnum" ) )
            {
                int vnum;

                vnum = fread_number( fp );
                if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
                    bug( "Fread_obj: bad vnum %d.", vnum );
                else
                    fVnum = TRUE;
                fMatch = TRUE;
                break;
            }
            break;

        case 'W':
            KEY( "WearFlags",   obj->wear_flags,        fread_number( fp ) );
            KEY( "WeaF",        obj->wear_flags,        fread_number( fp ) );
            KEY( "WearF",       obj->wear_flags,        fread_flag( fp ) );
            KEY( "WearLoc",     obj->wear_loc,          fread_number( fp ) );
            KEY( "Wear",        obj->wear_loc,          fread_number( fp ) );
            KEY( "Weight",      obj->weight,            fread_number( fp ) );
            KEY( "Wt",          obj->weight,            fread_number( fp ) );
            break;

        }

        if ( !fMatch )
        {
            bug( "Fread_obj: no match.", 0 );
            fread_to_eol( fp );
        }
    }
}

/* added by Rahl */
char *print_flags(int flag)
{
    int count, pos = 0;
    static char buf[52];


    for (count = 0; count < 32;  count++)
    {
        if (IS_SET(flag,1<<count))
        {
            if (count < 26)
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + (count - 26);
            pos++;
        }
    }

    if (pos == 0)
    {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}
