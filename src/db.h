///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2018 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/* files used in db.c */

/* vals from db.c */
extern bool fBootDb;
extern MOB_INDEX_DATA   * mob_index_hash          [MAX_KEY_HASH];
extern OBJ_INDEX_DATA   * obj_index_hash          [MAX_KEY_HASH];
extern int              top_mob_index;
extern int              top_obj_index;
extern int              top_affect;
extern int              top_ed; 


/* from db2.c */
extern int      social_count;

/* func from db.c */
extern void assign_area_vnum( int vnum );                    /* OLC */

