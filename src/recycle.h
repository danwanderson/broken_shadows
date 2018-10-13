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
*       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@pacinfo.com)                              *
*           Gabrielle Taylor (gtaylor@pacinfo.com)                         *
*           Brian Moore (rom@rom.efn.org)                                  *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

/* externs */
extern char str_empty[1];
extern int mobile_count;

/* ban data recycling */
#define BD BAN_DATA
BD      *new_ban (void);
void    free_ban (BAN_DATA *ban);
#undef BD

/* char gen data recycling */
#define GD GEN_DATA
GD      *new_gen_data (void);
void    free_gen_data (GEN_DATA * gen);
#undef GD

/* extra descr recycling */
#define ED EXTRA_DESCR_DATA
ED      *new_extra_descr (void);
void    free_extra_descr (EXTRA_DESCR_DATA *ed);
#undef ED

/* affect recycling */
#define AD AFFECT_DATA
AD      *new_affect (void);
void    free_affect (AFFECT_DATA *af);
#undef AD

/* object recycling */
#define OD OBJ_DATA
OD      *new_obj (void);
void    free_obj (OBJ_DATA *obj);
#undef OD

/* character recyling */
#define CD CHAR_DATA
#define PD PC_DATA
CD      *new_char (void);
void    free_char (CHAR_DATA *ch);
PD      *new_pcdata (void);
void    free_pcdata (PC_DATA *pcdata);
#undef PD
#undef CD


