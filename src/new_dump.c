///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2022 by Daniel Anderson
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
 * Ever wonder just what the stats on things were in the game? Statting    *
 * one object or mob at a time can be tedious and frequently you have to   *
 * stop and write things down, like hitpoints, armor classes, etc, if you  *
 * are trying to build an area and you want to preserve some continuity.   *
 * Granted, there should probably be tables and such availabe for builders'*
 * use (as there are on Broken Shadows), but you have to base those tables *
 * off something.                                                          *
 *                                                                         *
 * Well... this function is a cross between stat and dump. It loads each   *
 * mob and object briefly and writes its stats (or at least the vital ones)*
 * to a file. I removed a lot of the things from the stat part, mostly     *
 * empty lines where PC data was stored and also a lot of the things that  *
 * returned 0, such as carry weight.                                       *
 *                                                                         *
 * The files are place in the parent directory of the area directory by    *
 * default and are rather large (about 800k each for Shadows). With a      *
 * little work (I wrote a little C++ program to do this), they can be      *
 * converted into a character-delimeted file, so you can import it into    *
 * Access, Excel, or many other popular programs. I could have modified    *
 * This file to write it out in that format, but I was too lazy.           *
 *                                                                         *
 * Oh yeah. There's also a section for rooms. This is straight from rstat  *
 * It hasn't been tweaked at all. The first time I used it, it hit an      *
 * endless loop somewhere in there and I was too lazy to debug it. If you  *
 * want to uncomment it and debug it for me, feel free :)                  *
 *                                                                         *
 * One other thing work noting: Since it does load all the objects and     *
 * mobs in quick succession, CPU and memory usage climbs for about 10-15   *
 * seconds. This might cause a bit of lag for the players. I dunno. I      *
 * haven't used it when players were on.                                   *
 *                                                                         *
 * If you choose to use this code, please retain my name in this file and  *
 * send me an email (dwa1844@rit.edu) saying you are using it. Suggestions *
 * for improvement are welcome                                             *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "merc.h"

/* 
 * new_dump written by Rahl (Daniel Anderson) of Broken Shadows
 */
void do_new_dump( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA *pMobIndex;
    OBJ_INDEX_DATA *pObjIndex;
/*    ROOM_INDEX_DATA *pRoomIndex; */
    FILE *fp;
    int vnum,nMatch = 0;
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
/*    CHAR_DATA *rch; */
/*    int door; */
    AFFECT_DATA *paf;
    CHAR_DATA *mob;

    /* open file */
    fclose(fpReserve);

    /* start printing out mobile data */
    fp = fopen("../mob.txt","w");

    fprintf(fp,"\nMobile Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
    if ((pMobIndex = get_mob_index(vnum)) != NULL)
    {
        nMatch++;
        mob = create_mobile( pMobIndex );
        sprintf( buf, "Name: %s.\n",
            mob->name );
        fprintf( fp, buf );

        sprintf( buf, "Vnum: %d  Race: %s  Sex: %s  Room: %d  Count %d\n", 
            IS_NPC(mob) ? mob->pIndexData->vnum : 0,
            race_table[mob->race].name,
            mob->sex == SEX_MALE    ? "male"   :
            mob->sex == SEX_FEMALE  ? "female" : "neutral",
            mob->in_room == NULL    ?        0 : mob->in_room->vnum, 
            mob->pIndexData->count );
        fprintf( fp, buf );
        
        sprintf( buf, 
            "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n",
                mob->perm_stat[STAT_STR],
                get_curr_stat(mob,STAT_STR),
                mob->perm_stat[STAT_INT],
                get_curr_stat(mob,STAT_INT),
                mob->perm_stat[STAT_WIS],
                get_curr_stat(mob,STAT_WIS),
                mob->perm_stat[STAT_DEX],
                get_curr_stat(mob,STAT_DEX),
                mob->perm_stat[STAT_CON],
                get_curr_stat(mob,STAT_CON) );
        fprintf( fp, buf );

        sprintf( buf, "Hp: %d  Mana: %d  Move: %d  Hit: %d  Dam: %d\n",
                mob->max_hit,
            mob->max_mana,
            mob->max_move,
            GET_HITROLL(mob), GET_DAMROLL(mob) );
        fprintf( fp, buf );

        sprintf( buf,
            "Lv: %d  Align: %d  Gold: %ld  Damage: %dd%d  Message: %s\n",
                mob->level,                   
                mob->alignment,
                mob->gold,
                mob->damage[DICE_NUMBER],mob->damage[DICE_TYPE],
                attack_table[mob->dam_type].name);
            fprintf( fp, buf );

        sprintf(buf,"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n",
            GET_AC(mob,AC_PIERCE), GET_AC(mob,AC_BASH),
            GET_AC(mob,AC_SLASH),  GET_AC(mob,AC_EXOTIC));
        fprintf( fp, buf );

        sprintf(buf, "Act: %s\n",act_bit_name(mob->act));
        fprintf( fp, buf );
    
        if (IS_NPC(mob) && mob->off_flags)
        {
            sprintf(buf, "Offense: %s\n",off_bit_name(mob->off_flags));
            fprintf( fp, buf );
        }

        if (mob->imm_flags)
        {
            sprintf(buf, "Immune: %s\n",imm_bit_name(mob->imm_flags));
            fprintf( fp, buf );
        }
 
        if (mob->res_flags)
        {
            sprintf(buf, "Resist: %s\n", imm_bit_name(mob->res_flags));
            fprintf( fp, buf );
        }

        if (mob->vuln_flags)
        {
            sprintf(buf, "Vulnerable: %s\n", imm_bit_name(mob->vuln_flags));
            fprintf( fp, buf );
        }

        sprintf(buf, "Form: %s\nParts: %s\n", 
            form_bit_name(mob->form), part_bit_name(mob->parts));
        fprintf( fp, buf );

        if (mob->affected_by)
        {
            sprintf(buf, "Affected by %s\n", 
                affect_bit_name(mob->affected_by));
            fprintf( fp, buf );
        }
	
		if ( mob->affected2_by ) {
			sprintf( buf, "Affected2_by %s\n",
				affect2_bit_name( mob->affected2_by ) );
			fprintf( fp, buf );
		}

        sprintf( buf, "Short description: %s\nLong  description: %s",
            mob->short_descr,
            mob->long_descr[0] != '\0' ? mob->long_descr : "(none)\n" );
        fprintf( fp, buf );

        if ( IS_NPC(mob) && mob->spec_fun != 0 )
        {
            sprintf( buf, "Mobile has special procedure. - %s\n", spec_string( mob->spec_fun ) );
            fprintf( fp, buf );
        }

        for ( paf = mob->affected; paf != NULL; paf = paf->next )
        {
            sprintf( buf,
                "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n",
                skill_table[(int) paf->type].name,
                affect_loc_name( paf->location ),
                paf->modifier,
                paf->duration,
                affect_bit_name( paf->bitvector ),
                paf->level
                );
            fprintf( fp, buf );
        }
        fprintf( fp, "\n" );
        extract_char( mob, FALSE );
    }
    fclose(fp);

    /* start printing out object data */
    fp = fopen("../obj.txt","w");

    fprintf(fp,"\nObject Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
    if ((pObjIndex = get_obj_index(vnum)) != NULL)
    {
        nMatch++;

        obj = create_object( pObjIndex, 0 );

        sprintf( buf, "Name(s): %s\n",
            obj->name );
        fprintf( fp, buf );

        sprintf( buf, "Vnum: %d  Type: %s  Number: %d/%d  Weight: %d/%d\n",
            obj->pIndexData->vnum, 
            item_type_name(obj), 1, get_obj_number( obj ),
                        obj->weight, get_obj_weight( obj ) );
        fprintf( fp, buf );

        sprintf( buf, "Short description: %s\nLong description: %s\n",
            obj->short_descr, obj->description );
        fprintf( fp, buf );

        sprintf( buf, "Wear bits: %s\tExtra bits: %s\n",
            wear_bit_name(obj->wear_flags), extra_bit_name( obj->extra_flags ) );
        fprintf( fp, buf );

        sprintf( buf, "Level: %d  Cost: %d  Timer: %d\n",
            obj->level, obj->cost, obj->timer );
        fprintf( fp, buf );

        sprintf( buf,
            "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n",
            obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
            obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
            obj->carried_by == NULL    ? "(none)" : obj->carried_by->name,
            obj->wear_loc );
        fprintf( fp, buf );
    
        sprintf( buf, "Values: %d %d %d %d %d\n",
            obj->value[0], obj->value[1], obj->value[2], obj->value[3],
            obj->value[4] );
        fprintf( fp, buf );
    
        /* now give out vital statistics as per identify */
    
        switch ( obj->item_type )
        {
            case ITEM_SCROLL: 
            case ITEM_POTION:
            case ITEM_PILL:
                sprintf( buf, "Level %d spells of:", obj->value[0] );
                fprintf( fp, buf );

                if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[1]].name );
                    fprintf( fp, "'" );
                }

                if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[2]].name );
                    fprintf( fp, "'" );
                }

                if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[3]].name );
                    fprintf( fp, "'" );
                }

                fprintf( fp, ".\n" );
                break;

            case ITEM_WAND: 
            case ITEM_STAFF: 
                sprintf( buf, "Has %d(%d) charges of level %d",
                    obj->value[1], obj->value[2], obj->value[0] );
                fprintf( fp, buf );
      
                if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[3]].name );
                    fprintf( fp, "'" );
                }

                fprintf( fp, ".\n" );
                break;
      
            case ITEM_WEAPON:
                fprintf( fp, "Weapon type is " );
                switch (obj->value[0])
                {
                    case(WEAPON_EXOTIC)     : fprintf(fp, "exotic\n");  break;
                    case(WEAPON_SWORD)      : fprintf(fp, "sword\n");   break;  
                    case(WEAPON_DAGGER)     : fprintf(fp, "dagger\n");  break;
                    case(WEAPON_SPEAR)  : fprintf(fp, "spear/staff\n"); break;
                    case(WEAPON_MACE)   : fprintf(fp, "mace/club\n");   break;
                    case(WEAPON_AXE)    : fprintf(fp, "axe\n");     break;
                    case(WEAPON_FLAIL)  : fprintf(fp, "flail\n");   break;
                    case(WEAPON_WHIP)   : fprintf(fp, "whip\n");        break;
                    case(WEAPON_POLEARM)    : fprintf(fp, "polearm\n"); break;
                    default         : fprintf(fp, "unknown\n"); break;
                }
                    sprintf(buf,"Damage is %dd%d (average %d)\n",
                        obj->value[1],obj->value[2],
                        (1 + obj->value[2]) * obj->value[1] / 2);
                fprintf( fp, buf );
        
                if (obj->value[4])  /* weapon flags */
                {
                    sprintf(buf,"Weapons flags: %s\n",weapon_bit_name(obj->value[4]));
                    fprintf(fp, buf);
                }
                break;

            case ITEM_ARMOR:
                sprintf( buf, 
                    "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n", 
                    obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
                fprintf( fp, buf );
                break;
        }  /* switch */

        for ( paf = obj->affected; paf != NULL; paf = paf->next )
        {
            sprintf( buf, "Affects %s by %d, level %d",
                affect_loc_name( paf->location ), paf->modifier,paf->level );
            fprintf( fp, buf );
            /* added by Rahl */
            if ( paf->duration > -1 )
                sprintf( buf, ", %d hours.\n", paf->duration );
            else
                sprintf( buf, ".\n" );
            fprintf( fp, buf );
            if ( paf->bitvector )
            {
                switch ( paf->where )
                {
                    case TO_AFFECTS:
                        sprintf( buf, "Adds %s affect.\n", 
                            affect_bit_name( paf->bitvector ) );
                        break;
                    case TO_WEAPON:
                        sprintf( buf, "Adds %s weapon flags.\n",
                            weapon_bit_name( paf->bitvector ) );
                            break;
                    case TO_OBJECT:
                        sprintf( buf, "Adds %s object flag.\n",
                            extra_bit_name( paf->bitvector ) );
                        break;
                    case TO_IMMUNE:
                        sprintf( buf, "Adds immunity to %s.\n",
                            imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_RESIST:
                        sprintf( buf, "Adds resistance to %s.\n",
                            imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_VULN:
                        sprintf( buf, "Adds vulnerability to %s.\n",
                            imm_bit_name( paf->bitvector ) );
                        break;
                    default:
                        sprintf( buf, "Unknown bit %d %d\n",
                            paf->where, paf->bitvector );
                        break;
                }
                fprintf( fp, buf );
            }  /* if */
        }  /* for */

        if (!obj->enchanted)
            for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
            {
                sprintf( buf, "Affects %s by %d, level %d.\n",
                    affect_loc_name( paf->location ), paf->modifier,paf->level );
                fprintf( fp, buf );
                if ( paf->bitvector )
                {
                    switch ( paf->where )
                    {
                        case TO_AFFECTS:
                            sprintf( buf, "Adds %s affect.\n", 
                                affect_bit_name( paf->bitvector ) );
                            break;
                        case TO_WEAPON:
                            sprintf( buf, "Adds %s weapon flags.\n",
                                weapon_bit_name( paf->bitvector ) );
                            break;
                        case TO_OBJECT:
                            sprintf( buf, "Adds %s object flag.\n",
                                extra_bit_name( paf->bitvector ) );
                            break;
                        case TO_IMMUNE:
                            sprintf( buf, "Adds immunity to %s.\n",
                                imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_RESIST:
                            sprintf( buf, "Adds resistance to %s.\n",
                                imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_VULN:
                            sprintf( buf, "Adds vulnerability to %s.\n",
                                imm_bit_name( paf->bitvector ) );
                            break;
                        default:
                            sprintf( buf, "Unknown bit %d %d\n",
                                paf->where, paf->bitvector );
                            break;
                     }      /* switch */
                    fprintf( fp, buf );
                }       /* if */
            }   /* for */
    fprintf( fp, "\n" );
    extract_obj( obj );

    }       /* if */
    /* close file */
    fclose(fp);
    

    /* start printing out room data */
 /*   fp = fopen("../room.txt","w");

    fprintf(fp,"\nRoom Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_vnum_room; vnum++)
    if ((pRoomIndex = get_room_index(vnum)) != NULL)
    {
        nMatch++;
        sprintf( buf, "Name: '%s.'\nArea: '%s'.\n",
            pRoomIndex->name,
            pRoomIndex->area->name );
        fprintf( fp, buf );

        sprintf( buf,
            "Vnum: %d.  Sector: %d.  Light: %d.\n",
            pRoomIndex->vnum,
            pRoomIndex->sector_type,
            pRoomIndex->light );
        fprintf( fp, buf );

        sprintf( buf,
            "Room flags: %d.\nDescription:\n%s",
            pRoomIndex->room_flags,
            pRoomIndex->description );
        fprintf( fp, buf );

        if ( pRoomIndex->extra_descr != NULL )
        {
            EXTRA_DESCR_DATA *ed;

            fprintf( fp, "Extra description keywords: '" );
            for ( ed = pRoomIndex->extra_descr; ed; ed = ed->next )
            {
                fprintf( fp, ed->keyword );
                if ( ed->next != NULL )
                    fprintf( fp, " " );
            }
            fprintf( fp, "'.\n" );
        }

        fprintf( fp, "Characters:" );
        for ( rch = pRoomIndex->people; rch; rch = rch->next_in_room )
        {
            fprintf( fp, " " );
            one_argument( rch->name, buf );
            fprintf( fp, buf );
        }
    
        fprintf( fp, ".\nObjects:   " );
        for ( obj = pRoomIndex->contents; obj; obj = obj->next_content )
        {
            fprintf( fp, " " );
            one_argument( obj->name, buf );
            fprintf( fp, buf );
        }
        fprintf( fp, ".\n" );

        for ( door = 0; door <= 5; door++ )
        {
            EXIT_DATA *pexit;

            if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
            {
                sprintf( buf,
                    "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\nKeyword: '%s'.  Description: %s",
                    door,
                    (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
                    pexit->key,
                    pexit->exit_info,
                    pexit->keyword,
                    pexit->description[0] != '\0' ? pexit->description : "(none).\n" );
                fprintf( fp, buf );
            }
        }

    }
*/  
    /* close file */
/*    fclose(fp); */

    fpReserve = fopen( NULL_FILE, "r" );

    send_to_char( "Done writing files...\n\r", ch );
}
