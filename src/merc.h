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


/*
 * Accommodate old non-Ansi compilers.
 */
#if defined(TRADITIONAL)
#define const
#define args( list )                    ( )
#else
#define args( list )                    list
#endif

/* system calls */
int unlink();
int system();


/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if     !defined(FALSE)
#define FALSE    0
#endif

#if     !defined(TRUE)
#define TRUE     1
#endif

#if     defined(_AIX)
#if     !defined(const)
#define const
#endif
typedef int                             sh_int;
typedef int                             bool;
#define unix
#else
typedef short   int                     sh_int;
//typedef unsigned char                   bool;
#endif



/*
 * Structure types.
 */
typedef struct  affect_data             AFFECT_DATA;
typedef struct  area_data               AREA_DATA;
typedef struct  ban_data                BAN_DATA;
typedef struct  char_data               CHAR_DATA;
typedef struct  descriptor_data         DESCRIPTOR_DATA;
typedef struct  exit_data               EXIT_DATA;
typedef struct  extra_descr_data        EXTRA_DESCR_DATA;
typedef struct  help_data               HELP_DATA;
//typedef struct  kill_data               KILL_DATA;
typedef struct  mob_index_data          MOB_INDEX_DATA;
typedef struct  note_data               NOTE_DATA;
typedef struct  obj_data                OBJ_DATA;
typedef struct  obj_index_data          OBJ_INDEX_DATA;
typedef struct  pc_data                 PC_DATA;
typedef struct  gen_data                GEN_DATA;
typedef struct  reset_data              RESET_DATA;
typedef struct  room_index_data         ROOM_INDEX_DATA;
typedef struct  shop_data               SHOP_DATA;
typedef struct  time_info_data          TIME_INFO_DATA;
typedef struct  weather_data            WEATHER_DATA;
typedef struct  clan_data               CLAN_DATA;
/* added by Rahl */
typedef struct  auction_data            AUCTION_DATA; 
typedef struct  disabled_data           DISABLED_DATA;

/*
 * Function types.
 */
typedef void DO_FUN     args( ( CHAR_DATA *ch, char *argument ) );
typedef bool SPEC_FUN   args( ( CHAR_DATA *ch ) );
typedef void SPELL_FUN  args( ( int sn, int level, CHAR_DATA *ch, void
                                *vo, int target ) );



/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH             1024
#define MAX_STRING_LENGTH        4096
#define MAX_INPUT_LENGTH          256
#define PAGELEN                    22
/* moved here from db.c by Rahl */
#define MAX_MEM_LIST               11


/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS               999
#define MAX_SKILL                 999
#define MAX_GROUP                  50
#define MAX_IN_GROUP               30
#define MAX_ALIAS                   8
#define MAX_CLASS                   5
#define MAX_PC_RACE                13
#define MAX_LEVEL                 100
#define MAX_RANK                 10
#define MAX_CLAN                   10
#define MAX_CLAN_MEMBERS           30
#define MAX_ATTACK_TYPE            32
#define LEVEL_HERO                 (MAX_LEVEL - 9)
/* added by Rahl for ramdom quotes */
#define MAX_QUOTES                 12

#define PULSE_PER_SECOND            4
#define PULSE_VIOLENCE            ( 3 * PULSE_PER_SECOND)
#define PULSE_MOBILE              ( 4 * PULSE_PER_SECOND)
#define PULSE_TICK                (30 * PULSE_PER_SECOND)
#define PULSE_AREA                (60 * PULSE_PER_SECOND)

#define IMPLEMENTOR             MAX_RANK
#define CREATOR                 (MAX_RANK - 1)
#define SUPREME                 (MAX_RANK - 2)
#define DEITY                   (MAX_RANK - 3)
#define GOD                     (MAX_RANK - 4)
#define IMMORTAL                (MAX_RANK - 5)
#define DEMI                    (MAX_RANK - 6)
#define ANGEL                   (MAX_RANK - 7)
#define AVATAR                  (MAX_RANK - 8)
#define HERO                    (MAX_RANK - 9)

/* added by Rahl */
#define PULSE_AUCTION           ( 10 * PULSE_PER_SECOND ) 
                                /* 10 seconds */
#define PULSE_MUSIC             ( 6 * PULSE_PER_SECOND)
#define PULSE_BONUS			( 1800 * PULSE_PER_SECOND )

#include "board.h"
#include "buffer.h"

/*
 * Site ban structure.
 */

#define BAN_SUFFIX              A
#define BAN_PREFIX              B
#define BAN_NEWBIES             C
#define BAN_ALL                 D
#define BAN_PERMIT              E
#define BAN_PERMANENT           F

struct  ban_data
{
    BAN_DATA *  next;
    bool        valid;
    sh_int      ban_flags;
    sh_int      rank;
    char *      name;
};

/* auction structure - added by Rahl */
struct auction_data
{
    OBJ_DATA    * item;         /* pointer to the item */
    CHAR_DATA   * seller;       /* pointer to the seller */
    CHAR_DATA   * buyer;        /* pointer to the buyer */
    long        bet;    /* last bet or 0 if no bets */
    sh_int      going;  /* 1, 2, sold */
    sh_int      pulse;  /* how may pulses .25 sec until another call-out */
};

/* disabled command structure - added by Rahl */
struct disabled_data
{
    DISABLED_DATA *next;             /* pointer to the next node */
    struct cmd_type const *command;  /* pointer to the command struct */
    char *disabled_by;               /* name of disabler */
    sh_int level;                    /* level of disabler */
};

/* weapon structure - added by Rahl */
struct weapon_type
{
    char *name;
    sh_int vnum;
    sh_int type;
    sh_int *gsn;
};

/* wiznet structure - added by Rahl */
struct wiznet_type
{
    char *name;
    long flag;
    int  level;
};

/*
 * Time and weather stuff.
 */
#define SUN_DARK                    0
#define SUN_RISE                    1
#define SUN_LIGHT                   2
#define SUN_SET                     3

#define SKY_CLOUDLESS               0
#define SKY_CLOUDY                  1
#define SKY_RAINING                 2
#define SKY_LIGHTNING               3

struct  time_info_data
{
    int         hour;
    int         day;
    int         month;
    int         year;
};

struct  weather_data
{
    int         mmhg;
    int         change;
    int         sky;
    int         sunlight;
};



/*
 * Connected state for a channel.
 */
#define CON_PLAYING                      0
#define CON_GET_NAME                     1
#define CON_GET_OLD_PASSWORD             2
#define CON_CONFIRM_NEW_NAME             3
#define CON_GET_NEW_PASSWORD             4
#define CON_CONFIRM_NEW_PASSWORD         5
#define CON_GET_NEW_RACE                 6
#define CON_GET_NEW_SEX                  7
#define CON_GET_NEW_CLASS                8
#define CON_GET_ALIGNMENT                9
#define CON_DEFAULT_CHOICE              10 
#define CON_GEN_GROUPS                  11 
#define CON_PICK_WEAPON                 12
#define CON_READ_IMOTD                  13
#define CON_READ_MOTD                   14
#define CON_BREAK_CONNECT               15
#define CON_GET_STATS                   16
#define CON_NOTE_TO                     17
#define CON_NOTE_SUBJECT                18
#define CON_NOTE_EXPIRE                 19
#define CON_NOTE_TEXT                   20
#define CON_NOTE_FINISH                 21
#define CON_ANSI						24
#define CON_COPYOVER_RECOVER            25


/*
 * Descriptor (channel) structure.
 */
struct  descriptor_data
{
    DESCRIPTOR_DATA *   next;
    DESCRIPTOR_DATA *   snoop_by;
    CHAR_DATA *         character;
    CHAR_DATA *         original;
    char *              host;
    sh_int              descriptor;
    sh_int              connected;
    bool                fcommand;
    char                inbuf           [4 * MAX_INPUT_LENGTH];
    char                incomm          [MAX_INPUT_LENGTH];
    char                inlast          [MAX_INPUT_LENGTH];
    int                 repeat;
    char *              outbuf;
    int                 outsize;
    int                 outtop;
    char *              showstr_head;
    char *              showstr_point;
    void *              pEdit;          /* OLC */
    char **             pString;        /* OLC */
    int                 editor;         /* OLC */
	bool			    ansi;
};



/*
 * Attribute bonus structures.
 */
struct  str_app_type
{
    sh_int      tohit;
    sh_int      todam;
    sh_int      carry;
    sh_int      wield;
};

struct  int_app_type
{
    sh_int      learn;
};

struct  wis_app_type
{
    sh_int      practice;
};

struct  dex_app_type
{
    sh_int      defensive;
};

struct  con_app_type
{
    sh_int      hitp;
    sh_int      shock;
};



/*
 * TO types for act.
 */
#define TO_ROOM             0
#define TO_NOTVICT          1
#define TO_VICT             2
#define TO_CHAR             3
#define TO_ALL              4


/*
 * Help table types.
 */
struct  help_data
{
    HELP_DATA * next;
    sh_int      level;
    char *      keyword;
    char *      text;
};

/*
 * Shop types.
 */
#define MAX_TRADE        5

struct  shop_data
{
    SHOP_DATA * next;                   /* Next shop in list            */
    sh_int      keeper;                 /* Vnum of shop keeper mob      */
    sh_int      buy_type [MAX_TRADE];   /* Item types shop will buy     */
    sh_int      profit_buy;             /* Cost multiplier for buying   */
    sh_int      profit_sell;            /* Cost multiplier for selling  */
    sh_int      open_hour;              /* First opening hour           */
    sh_int      close_hour;             /* First closing hour           */
};



/*
 * Per-class stuff.
 */

#define MAX_GUILD       2
#define MAX_STATS       5
#define STAT_STR        0
#define STAT_INT        1
#define STAT_WIS        2
#define STAT_DEX        3
#define STAT_CON        4

struct  class_type
{
    char *      name;                   /* the full name of the class */
    char        who_name        [4];    /* Three-letter name for 'who'  */
    sh_int      attr_prime;             /* Prime attribute              */
    sh_int      weapon;                 /* First weapon                 */
    sh_int      guild[MAX_GUILD];       /* Vnum of guild rooms          */
    sh_int      skill_adept;            /* Maximum skill level          */
    sh_int      thac0_00;               /* Thac0 for level  0           */
    sh_int      thac0_32;               /* Thac0 for level 32           */
    sh_int      hp_min;                 /* Min hp gained on leveling    */
    sh_int      hp_max;                 /* Max hp gained on leveling    */
    bool        fMana;                  /* Class gains mana on level    */
    char *      base_group;             /* base skills gained           */
    char *      default_group;          /* default skills gained        */
    bool        remort_class;           /* is a remort class            */
};

struct attack_type
{
    char *      name;                   /* name and message */
    int         damage;                 /* damage class */
};

struct race_type
{
    char *      name;                   /* call name of the race */
    bool        pc_race;                /* can be chosen by pcs */
    long        act;                    /* act bits for the race */
    long        aff;                    /* aff bits for the race */
    /* added by Rahl */
    long        aff2;                   /* aff2 bits for the race */

    long        off;                    /* off bits for the race */
    long        imm;                    /* imm bits for the race */
    long        res;                    /* res bits for the race */
    long        vuln;                   /* vuln bits for the race */
    long        form;                   /* default form flag for the race */
    long        parts;                  /* default parts for the race */
    bool        remort_race;            /* is a remort race */
};


struct pc_race_type  /* additional data for pc races */
{
    char *      name;                   /* MUST be in race_type */
    char        who_name[6];
    sh_int      points;                 /* cost in points of the race */
    sh_int      class_mult[MAX_CLASS];  /* exp multiplier for class, * 100 */
    char *      skills[5];              /* bonus skills for the race */
    sh_int      stats[MAX_STATS];       /* starting stats */
    sh_int      max_stats[MAX_STATS];   /* maximum stats */
    sh_int      size;                   /* aff bits for the race */
};



/*
 * Data structure for notes.
 */
struct  note_data
{
    NOTE_DATA * next;
    char *      sender;
    char *      date;
    char *      to_list;
    char *      subject;
    char *      text;
    time_t      date_stamp;
    time_t      expire;
};



/*
 * An affect.
 */
struct  affect_data
{
    AFFECT_DATA *       next;
/* where added by Rahl */
    sh_int              where;
    sh_int              type;
    sh_int              level;
    sh_int              duration;
    sh_int              location;
    sh_int              modifier;
    int                 bitvector;
};

/* affect where locations - by Rahl */
#define TO_AFFECTS      0
#define TO_OBJECT       1
#define TO_IMMUNE       2
#define TO_RESIST       3
#define TO_VULN         4
#define TO_WEAPON       5
#define TO_AFFECTS2     6



/*
 * A kill structure (indexed by level).
 */
/*
struct  kill_data
{
    sh_int              number;
    sh_int              killed;
};
*/


/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO              3090
#define MOB_VNUM_CITYGUARD         3060
/* added by Rahl */
#define MOB_VNUM_ZOMBIE            2

/* RT ASCII conversions -- used so we can have letters in this file */

#define A                       1
#define B                       2
#define C                       4
#define D                       8
#define E                       16
#define F                       32
#define G                       64
#define H                       128

#define I                       256
#define J                       512
#define K                       1024
#define L                       2048
#define M                       4096
#define N                       8192
#define O                       16384
#define P                       32768

#define Q                       65536
#define R                       131072
#define S                       262144
#define T                       524288
#define U                       1048576
#define V                       2097152
#define W                       4194304
#define X                       8388608

#define Y                       16777216
#define Z                       33554432
#define aa                      67108864        /* doubled due to conflicts */
#define bb                      134217728
#define cc                      268435456    
#define dd                      536870912
#define ee                      1073741824

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC              (A)             /* Auto set for mobs    */
#define ACT_SENTINEL            (B)             /* Stays in one room    */
#define ACT_SCAVENGER           (C)             /* Picks up objects     */
#define ACT_AGGRESSIVE          (F)             /* Attacks PC's         */
#define ACT_STAY_AREA           (G)             /* Won't leave area     */
#define ACT_WIMPY               (H)
#define ACT_PET                 (I)             /* Auto set for pets    */
#define ACT_TRAIN               (J)             /* Can train PC's       */
#define ACT_PRACTICE            (K)             /* Can practice PC's    */
#define ACT_UNDEAD              (O)     
#define ACT_CLERIC              (Q)
#define ACT_MAGE                (R)
#define ACT_THIEF               (S)
#define ACT_WARRIOR             (T)
#define ACT_NOALIGN             (U)
#define ACT_NOPURGE             (V)
#define ACT_IS_HEALER           (aa)
#define ACT_GAIN                (bb)
#define ACT_UPDATE_ALWAYS       (cc)
#define ACT_NO_KILL             (dd)
/* added by Rahl */
#define ACT_BANKER              (ee)

/* damage classes */
#define DAM_NONE                0
#define DAM_BASH                1
#define DAM_PIERCE              2
#define DAM_SLASH               3
#define DAM_FIRE                4
#define DAM_COLD                5
#define DAM_LIGHTNING           6
#define DAM_ACID                7
#define DAM_POISON              8
#define DAM_NEGATIVE            9
#define DAM_HOLY                10
#define DAM_ENERGY              11
#define DAM_MENTAL              12
#define DAM_DISEASE             13
#define DAM_DROWNING            14
#define DAM_LIGHT               15
#define DAM_OTHER               16
#define DAM_HARM                17

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BACKSTAB            (B)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FADE                (G)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define OFF_CRUSH               (O)
#define ASSIST_ALL              (P)
#define ASSIST_ALIGN            (Q)
#define ASSIST_RACE             (R)
#define ASSIST_PLAYERS          (S)
#define ASSIST_GUARD            (T)
#define ASSIST_VNUM             (U)

/* new ones by Rahl */
#define OFF_CIRCLE              (V)
#define OFF_WHIRL               (X)

/* return values for check_imm */
#define IS_NORMAL               0
#define IS_IMMUNE               1
#define IS_RESISTANT            2
#define IS_VULNERABLE           3

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_MENTAL              (P)
#define IMM_DISEASE             (Q)
#define IMM_DROWNING            (R)
#define IMM_LIGHT               (S)
 
/* RES bits for mobs */
#define RES_CHARM               (B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_MENTAL              (P)
#define RES_DISEASE             (Q)
#define RES_DROWNING            (R)
#define RES_LIGHT               (S)
 
/* VULN bits for mobs */
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_MENTAL             (P)
#define VULN_DISEASE            (Q)
#define VULN_DROWNING           (R)
#define VULN_LIGHT              (S)
#define VULN_WOOD               (X)
#define VULN_SILVER             (Y)
#define VULN_IRON               (Z)
 
/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)  /* defined by material bit */
 
/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I)
#define FORM_CONSTRUCT          (J)
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)
 
#define FORM_BIPED              (M)
#define FORM_CENTAUR            (N)
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB               (S)
 
#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD         (cc)    
 
/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE                (K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS              (Y)


/*
 * Bits for "material"
 * Used in several places
 */
#define MAT_IRON                (A)
#define MAT_WOOD                (B)
#define MAT_DIAMOND             (C)
#define MAT_RUBY                (D)
#define MAT_EMERALD             (E)
#define MAT_GOLD                (F)
#define MAT_SILVER              (G)
#define MAT_TITANIUM            (H)
#define MAT_PLATINUM            (I)
#define MAT_LEATHER             (J)
#define MAT_CLOTH               (K)
#define MAT_PAPER               (L)
#define MAT_AMETHYST            (M)
#define MAT_GLASS               (N)
#define MAT_CRYSTAL             (O)

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND               (A)
#define AFF_INVISIBLE           (B)
#define AFF_DETECT_EVIL         (C)
#define AFF_DETECT_INVIS        (D)
#define AFF_DETECT_MAGIC        (E)
#define AFF_DETECT_HIDDEN       (F)
#define AFF_PROTECT_GOOD        (G)             /* added by Rahl */
#define AFF_SANCTUARY           (H)             /* was AFF_HOLD */

#define AFF_FAERIE_FIRE         (I)
#define AFF_INFRARED            (J)
#define AFF_CURSE               (K)
#define AFF_SLOW                (L)             /* added by Rahl */
#define AFF_POISON              (M)             /* was AFF_FLAMING */
#define AFF_PROTECT_EVIL        (N)
#define AFF_PARALYSIS           (O)             /* Unused       */
#define AFF_SNEAK               (P)

#define AFF_HIDE                (Q)
#define AFF_SLEEP               (R)
#define AFF_CHARM               (S)
#define AFF_FLYING              (T)
#define AFF_PASS_DOOR           (U)
#define AFF_HASTE               (V)
#define AFF_CALM                (W)
#define AFF_PLAGUE              (X)
#define AFF_WEAKEN              (Y)
#define AFF_DARK_VISION         (Z)
#define AFF_BERSERK             (aa)
#define AFF_SWIM                (bb)
#define AFF_REGENERATION        (cc)
#define AFF_WEB                 (dd)
/* added by Rahl */
#define AFF_DETECT_GOOD         (ee)


/*
 * Bits for 'affected2_by'
 * Used in #MOBILES
 * added by Rahl
 */
#define AFF_BLINK               (A)


/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL                   0
#define SEX_MALE                      1
#define SEX_FEMALE                    2

/* AC types */
#define AC_PIERCE                       0
#define AC_BASH                         1
#define AC_SLASH                        2
#define AC_EXOTIC                       3

/* dice */
#define DICE_NUMBER                     0
#define DICE_TYPE                       1
#define DICE_BONUS                      2

/* size */
#define SIZE_TINY                       0
#define SIZE_SMALL                      1
#define SIZE_MEDIUM                     2
#define SIZE_LARGE                      3
#define SIZE_HUGE                       4
#define SIZE_GIANT                      5



/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_MONEY_ONE            2
#define OBJ_VNUM_MONEY_SOME           3

#define OBJ_VNUM_CORPSE_NPC          10
#define OBJ_VNUM_CORPSE_PC           11
#define OBJ_VNUM_SEVERED_HEAD        12
#define OBJ_VNUM_TORN_HEART          13
#define OBJ_VNUM_SLICED_ARM          14
#define OBJ_VNUM_SLICED_LEG          15
#define OBJ_VNUM_GUTS                16
#define OBJ_VNUM_BRAINS              17

#define OBJ_VNUM_MUSHROOM            20
#define OBJ_VNUM_LIGHT_BALL          21
#define OBJ_VNUM_SPRING              22
/* added by Rahl */
#define OBJ_VNUM_FLAME_SWORD         23
#define OBJ_VNUM_STEAK               24 
#define OBJ_VNUM_ROSE                25
#define OBJ_VNUM_PORTAL              26
/* end stuff by Rahl */

#define OBJ_VNUM_SCHOOL_MACE       3700
#define OBJ_VNUM_SCHOOL_DAGGER     3701
#define OBJ_VNUM_SCHOOL_SWORD      3702
/* new stuff by Rahl */
#define OBJ_VNUM_SCHOOL_AXE        3717
#define OBJ_VNUM_SCHOOL_STAFF      3718
#define OBJ_VNUM_SCHOOL_FLAIL      3719
#define OBJ_VNUM_SCHOOL_WHIP       3720
#define OBJ_VNUM_SCHOOL_POLEARM    3721

#define OBJ_VNUM_SCHOOL_VEST       3703
#define OBJ_VNUM_SCHOOL_SHIELD     3704
#define OBJ_VNUM_SCHOOL_BANNER     3716
#define OBJ_VNUM_MAP               3162
#define OBJ_VNUM_BLANK_SCROLL      3398
#define OBJ_VNUM_EMPTY_VIAL        3399

/* more junk by Rahl */
#define OBJ_VNUM_BLACK_ROSE          27
#define OBJ_VNUM_WHITE_ROSE          28
#define OBJ_VNUM_RED_ROSE            29
#define OBJ_VNUM_PINK_ROSE           30
#define OBJ_VNUM_CORSAGE             31
#define OBJ_VNUM_DOZEN_ROSES         32



/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT                    1
#define ITEM_SCROLL                   2
#define ITEM_WAND                     3
#define ITEM_STAFF                    4
#define ITEM_WEAPON                   5
#define ITEM_TREASURE                 8
#define ITEM_ARMOR                    9
#define ITEM_POTION                  10
#define ITEM_CLOTHING                11
#define ITEM_FURNITURE               12
#define ITEM_TRASH                   13
#define ITEM_CONTAINER               15
#define ITEM_DRINK_CON               17
#define ITEM_KEY                     18
#define ITEM_FOOD                    19
#define ITEM_MONEY                   20
#define ITEM_BOAT                    22
#define ITEM_CORPSE_NPC              23
#define ITEM_CORPSE_PC               24
#define ITEM_FOUNTAIN                25
#define ITEM_PILL                    26
#define ITEM_PROTECT                 27
#define ITEM_MAP                     28
/* added by Rahl */
#define ITEM_WARP_STONE              29
#define ITEM_PORTAL                  30
#define ITEM_JUKEBOX                 31
#define ITEM_GEM                     32
#define ITEM_JEWELRY                 33

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW               (A)
#define ITEM_HUM                (B)
#define ITEM_DARK               (C)
#define ITEM_LOCK               (D)
#define ITEM_EVIL               (E)
#define ITEM_INVIS              (F)
#define ITEM_MAGIC              (G)
#define ITEM_NODROP             (H)
#define ITEM_BLESS              (I)
#define ITEM_ANTI_GOOD          (J)
#define ITEM_ANTI_EVIL          (K)
#define ITEM_ANTI_NEUTRAL       (L)
#define ITEM_NOREMOVE           (M)
#define ITEM_INVENTORY          (N)
#define ITEM_NOPURGE            (O)
#define ITEM_ROT_DEATH          (P)
#define ITEM_VIS_DEATH          (Q)
/* added by Rahl */
#define ITEM_MELT_DROP          (R)
#define ITEM_BURN_PROOF         (S)
#define ITEM_NOLOCATE           (T)
#define ITEM_NOUNCURSE          (U)
#define ITEM_RARE               (V)
#define ITEM_NOSACRIFICE		(X)

/* gate flags - added by Rahl */
#define GATE_NORMAL_EXIT        (A)
#define GATE_NOCURSE            (B)
#define GATE_GOWITH             (C)
#define GATE_BUGGY              (D)
#define GATE_RANDOM             (E)


/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE               (A)
#define ITEM_WEAR_FINGER        (B)
#define ITEM_WEAR_NECK          (C)
#define ITEM_WEAR_BODY          (D)
#define ITEM_WEAR_HEAD          (E)
#define ITEM_WEAR_LEGS          (F)
#define ITEM_WEAR_FEET          (G)
#define ITEM_WEAR_HANDS         (H)
#define ITEM_WEAR_ARMS          (I)
#define ITEM_WEAR_SHIELD        (J)
#define ITEM_WEAR_ABOUT         (K)
#define ITEM_WEAR_WAIST         (L)
#define ITEM_WEAR_WRIST         (M)
#define ITEM_WIELD              (N)
#define ITEM_HOLD               (O)
#define ITEM_TWO_HANDS          (P)

/* weapon class */
#define WEAPON_EXOTIC           0
#define WEAPON_SWORD            1
#define WEAPON_DAGGER           2
#define WEAPON_SPEAR            3
#define WEAPON_MACE             4
#define WEAPON_AXE              5
#define WEAPON_FLAIL            6
#define WEAPON_WHIP             7       
#define WEAPON_POLEARM          8

/* weapon types */
#define WEAPON_FLAMING          (A)
#define WEAPON_FROST            (B)
#define WEAPON_VAMPIRIC         (C)
#define WEAPON_SHARP            (D)
#define WEAPON_VORPAL           (E)
#define WEAPON_TWO_HANDS        (F)
/* added by Rahl */
#define WEAPON_SHOCKING         (G)
#define WEAPON_POISON           (H)



/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE                    0
#define APPLY_STR                     1
#define APPLY_DEX                     2
#define APPLY_INT                     3
#define APPLY_WIS                     4
#define APPLY_CON                     5
#define APPLY_SEX                     6
#define APPLY_CLASS                   7
#define APPLY_AGE                     9
#define APPLY_HEIGHT                 10
#define APPLY_WEIGHT                 11
#define APPLY_MANA                   12
#define APPLY_HIT                    13
#define APPLY_MOVE                   14
#define APPLY_GOLD                   15
#define APPLY_EXP                    16
#define APPLY_AC                     17
#define APPLY_HITROLL                18
#define APPLY_DAMROLL                19
#define APPLY_SAVING_PARA            20
#define APPLY_SAVING_ROD             21
#define APPLY_SAVING_PETRI           22
#define APPLY_SAVING_BREATH          23
#define APPLY_SAVING_SPELL           24
#define APPLY_ALIGN                  25

#define APPLY_SPELL_AFFECT           26

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE                1
#define CONT_PICKPROOF                2
#define CONT_CLOSED                   4
#define CONT_LOCKED                   8



/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO               2
#define ROOM_VNUM_TEMPLE           3001
#define ROOM_VNUM_ALTAR            3054
#define ROOM_VNUM_SCHOOL           3700
/* jail room added by Rahl */
#define ROOM_VNUM_JAIL                1

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK               (A)
#define ROOM_NO_MOB             (C)
#define ROOM_INDOORS            (D)

#define ROOM_PRIVATE            (J)
#define ROOM_SAFE               (K)
#define ROOM_SOLITARY           (L)
#define ROOM_PET_SHOP           (M)
#define ROOM_NO_RECALL          (N)
#define ROOM_IMP_ONLY           (O)
#define ROOM_GODS_ONLY          (P)
#define ROOM_HEROES_ONLY        (Q)
#define ROOM_NEWBIES_ONLY       (R)
#define ROOM_LAW                (S)
#define ROOM_NOTELEPORT         (U)
/* added by Rahl */
#define ROOM_BANK               (V)
#define ROOM_UNDERGROUND        (X)

/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH                     0
#define DIR_EAST                      1
#define DIR_SOUTH                     2
#define DIR_WEST                      3
#define DIR_UP                        4
#define DIR_DOWN                      5



/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR                     1
#define EX_CLOSED                     2
#define EX_LOCKED                     4
#define EX_PICKPROOF                  8
#define EX_PASSPROOF                 16
#define EX_HIDDEN                    32

/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE                   0
#define SECT_CITY                     1
#define SECT_FIELD                    2
#define SECT_FOREST                   3
#define SECT_HILLS                    4
#define SECT_MOUNTAIN                 5
#define SECT_WATER_SWIM               6
#define SECT_WATER_NOSWIM             7
#define SECT_UNUSED                   8
#define SECT_AIR                      9
#define SECT_DESERT                  10
/* added by Rahl */
#define SECT_UNDERGROUND             11
#define SECT_MAX                     12



/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE                    -1
#define WEAR_LIGHT                    0
#define WEAR_FINGER_L                 1
#define WEAR_FINGER_R                 2
#define WEAR_NECK_1                   3
#define WEAR_NECK_2                   4
#define WEAR_BODY                     5
#define WEAR_HEAD                     6
#define WEAR_LEGS                     7
#define WEAR_FEET                     8
#define WEAR_HANDS                    9
#define WEAR_ARMS                    10
#define WEAR_SHIELD                  11
#define WEAR_ABOUT                   12
#define WEAR_WAIST                   13
#define WEAR_WRIST_L                 14
#define WEAR_WRIST_R                 15
#define WEAR_WIELD                   16
#define WEAR_HOLD                    17
#define WEAR_SECOND_WIELD            18
#define MAX_WEAR                     19



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK                    0
#define COND_FULL                     1
#define COND_THIRST                   2



/*
 * Positions.
 */
#define POS_DEAD                      0
#define POS_MORTAL                    1
#define POS_INCAP                     2
#define POS_STUNNED                   3
#define POS_SLEEPING                  4
#define POS_RESTING                   5
#define POS_SITTING                   6
#define POS_FIGHTING                  7
#define POS_STANDING                  8



/*
 * ACT bits for players.
 */
#define PLR_IS_NPC              (A)             /* Don't EVER set.      */
#define PLR_BOUGHT_PET          (B)

/* RT auto flags */
#define PLR_AUTOASSIST          (C)
#define PLR_AUTOEXIT            (D)
#define PLR_AUTOLOOT            (E)
#define PLR_AUTOSAC             (F)
#define PLR_AUTOGOLD            (G)
#define PLR_AUTOSPLIT           (H)
/* 5 bits reserved, I-M */
/* autodamage added by Rahl */
#define PLR_AUTODAMAGE          (I)
#define PLR_CONSENT             (J)
#define PLR_REMORT              (K)

/* RT personal flags */
#define PLR_HOLYLIGHT           (N)
#define PLR_WIZINVIS            (O)
#define PLR_CANLOOT             (P)
#define PLR_NOSUMMON            (Q)
#define PLR_NOFOLLOW            (R)
#define PLR_COLOR               (S)
/* 4 bits reserved, S-V */
/* added by Rahl */
#define PLR_NOBOUNTY            (U)

/* penalty flags */
#define PLR_LOG                 (W)
#define PLR_DENY                (X)
#define PLR_FREEZE              (Y)
#define PLR_THIEF               (Z)
#define PLR_KILLER              (aa)
#define PLR_AFK                 (bb)
#define PLR_PERMIT              (T)
/* quest flag - added by Rahl */
#define PLR_QUESTOR             (cc)
/* Incog flag - added by Rahl */
#define PLR_INCOGNITO           (dd)
/* bounty hunter added by Rahl */
#define PLR_BOUNTY_HUNTER       (ee)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET              (A)
#define COMM_NOWIZ              (C)
#define COMM_NOAUCTION          (D)
#define COMM_NOGOSSIP           (E)
#define COMM_NOQUOTE			(F)
#define COMM_NOMUSIC            (G)
#define COMM_NOINFO             (H)
/* new stuff by Rahl */
#define COMM_NOCLAN             (K)

/* display flags */
#define COMM_COMPACT            (L)
#define COMM_BRIEF              (M)
#define COMM_PROMPT             (N)
#define COMM_COMBINE            (O)
#define COMM_TELNET_GA          (P)
/* 3 flags reserved, Q-S */

/* penalties */
#define COMM_NOEMOTE            (T)
#define COMM_NOTELL             (V)
#define COMM_NOCHANNELS         (W) 
#define COMM_SNOOP_PROOF        (Y)

/* WIZNET stuff added by Rahl */
#define WIZ_ON                  (A)
#define WIZ_TICKS               (B)
#define WIZ_LOGINS              (C)
#define WIZ_SITES               (D)
#define WIZ_LINKS               (E)
#define WIZ_DEATHS              (F)
#define WIZ_RESETS              (G)
#define WIZ_MOBDEATHS           (H)
#define WIZ_FLAGS               (I)
#define WIZ_PENALTIES           (J)
#define WIZ_SACCING             (K)
#define WIZ_LEVELS              (L)
#define WIZ_SECURE              (M)
#define WIZ_SWITCHES            (N)
#define WIZ_SNOOPS              (O)
#define WIZ_RESTORE             (P)
#define WIZ_LOAD                (Q)
#define WIZ_NEWBIE              (R)
#define WIZ_PREFIX              (S)
#define WIZ_SPAM                (T)
#define WIZ_AFK                 (U)


/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct  mob_index_data
{
    MOB_INDEX_DATA *    next;
    SPEC_FUN *          spec_fun;
    SHOP_DATA *         pShop;
    AREA_DATA *         area;           /* OLC */
    sh_int              vnum;
    sh_int              count;
    sh_int              killed;
    char *              player_name;
    char *              short_descr;
    char *              long_descr;
    char *              description;
    long                act;
    long                affected_by;
    sh_int              alignment;
    sh_int              level;
    sh_int              hitroll;
    sh_int              hit[3];
    sh_int              mana[3];
    sh_int              damage[3];
    sh_int              ac[4];
    sh_int              dam_type;
    long                off_flags;
    long                imm_flags;
    long                res_flags;
    long                vuln_flags;
    sh_int              start_pos;
    sh_int              default_pos;
    sh_int              sex;
    sh_int              race;
    long                gold;
    long                form;
    long                parts;
    sh_int              size;
    long                material;
    sh_int              clan;
    /* added by Rahl */
    long                affected2_by;

};


/*
 * One character (PC or NPC).
 */
struct  char_data
{
    CHAR_DATA *         next;
    CHAR_DATA *         next_in_room;
    CHAR_DATA *         master;
    CHAR_DATA *         leader;
    CHAR_DATA *         fighting;
    CHAR_DATA *         reply;
    CHAR_DATA *         pet;
    SPEC_FUN *          spec_fun;
    MOB_INDEX_DATA *    pIndexData;
    DESCRIPTOR_DATA *   desc;
    AFFECT_DATA *       affected;
    OBJ_DATA *          carrying;
    ROOM_INDEX_DATA *   in_room;
    ROOM_INDEX_DATA *   was_in_room;
    PC_DATA *           pcdata;
    GEN_DATA *          gen_data;
    char *              name;
    sh_int              version;
    char *              short_descr;
    char *              long_descr;
    char *              description;
    sh_int              sex;
    sh_int              ch_class;
    sh_int              race;
    sh_int              level;
    int                 played;
    int                 lines;  /* for the pager */
    time_t              logon;
    time_t              last_note;
    sh_int              timer;
    sh_int              wait;
    sh_int              daze;
    sh_int              hit;
    sh_int              max_hit;
    sh_int              mana;
    sh_int              max_mana;
    sh_int              move;
    sh_int              max_move;
    long                gold;
    long                exp;
    long                act;
    long                comm;   /* RT added to pad the vector */
    long                wiznet; /* wiznet stuff - added by Rahl */
    long                imm_flags;
    long                res_flags;
    long                vuln_flags;
    sh_int              invis_level;
    sh_int              incog_level; /* Incog level added by Rahl */
    int                 affected_by;
    sh_int              position;
    sh_int              practice;
    sh_int              train;
    sh_int              carry_weight;
    sh_int              carry_number;
    sh_int              saving_throw;
    sh_int              alignment;
    sh_int              hitroll;
    sh_int              damroll;
    sh_int              armor[4];
    sh_int              wimpy;
    /* stats */
    sh_int              perm_stat[MAX_STATS];
    sh_int              mod_stat[MAX_STATS];
    /* parts stuff */
    long                form;
    long                parts;
    sh_int              size;
    long                material;
    /* mobile stuff */
    long                off_flags;
    sh_int              damage[3];
    sh_int              dam_type;
    sh_int              start_pos;
    sh_int              default_pos;
    /* automated quest stuff - added by Rahl */
    CHAR_DATA *         questgiver;
    int                 questpoints;
    sh_int              nextquest;
    sh_int              countdown;
    sh_int              questobj;
    sh_int              questmob;
    /* bank stuff added by Rahl */
    long                bank;
    /* added by Rahl */
    long                affected2_by;
    AREA_DATA *         zone;
    char *              prefix;
	bool				fBonusMob;
	int 				bonusPoints;
	int					immRank;
};


/*
 * Data which only PC's have.
 */
struct  pc_data
{
    PC_DATA *           next;
    CHAR_DATA *         nemesis;
    char *              pwd;
    char *              bamfin;
    char *              bamfout;
    char *              title;
    sh_int              perm_hit;
    sh_int              perm_mana;
    sh_int              perm_move;
    sh_int              true_sex;
    int                 last_level;
    int                 security;
    sh_int              condition       [3];
    sh_int              learned         [MAX_SKILL];
    bool                group_known     [MAX_GROUP];
    sh_int              points;
    bool                confirm_delete;
    bool                confirm_pk;
    char *              prompt;
    sh_int              clan;
    ROOM_INDEX_DATA *   recall_room;
    int                 chaos_score;
    char *              alias[MAX_ALIAS];
    char *              alias_sub[MAX_ALIAS];
    /* bounty added by Rahl */
    long                bounty;
    BOARD_DATA *        board;
    time_t              last_note[MAX_BOARD];
    NOTE_DATA *         in_progress;
    char *              email;
    BUFFER *            buffer;
    int                 message_ctr;
    int                 pkills;
    int                 pkilled;
    int                 killed;
    int                 kills;
    char *              comment;
    char *              spouse;
    sh_int              clan_leader;
	char *				away_message;
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
    GEN_DATA    *next;
    bool        skill_chosen[MAX_SKILL];
    bool        group_chosen[MAX_GROUP];
    int         points_chosen;
};



/*
 * Liquids.
 */
#define LIQ_WATER        0
#define LIQ_MAX         21

struct  liq_type
{
    char *      liq_name;
    char *      liq_color;
    sh_int      liq_affect[3];
};



/*
 * Extra description data for a room or object.
 */
struct  extra_descr_data
{
    EXTRA_DESCR_DATA *next;     /* Next in list                     */
    char *keyword;              /* Keyword in look/examine          */
    char *description;          /* What to see                      */
};



/*
 * Prototype for an object.
 */
struct  obj_index_data
{
    OBJ_INDEX_DATA *    next;
    EXTRA_DESCR_DATA *  extra_descr;
    AFFECT_DATA *       affected;
    AREA_DATA *         area;           /* OLC */
    char *              name;
    char *              short_descr;
    char *              description;
    sh_int              vnum;
    sh_int              reset_num;
    long                material;
    sh_int              item_type;
    int                 extra_flags;
    sh_int              wear_flags;
    sh_int              level;
    sh_int              count;
    sh_int              weight;
    int                 cost;
    int                 value[5];
};



/*
 * One object.
 */
struct  obj_data
{
    OBJ_DATA *          next;
    OBJ_DATA *          next_content;
    OBJ_DATA *          contains;
    OBJ_DATA *          in_obj;
    CHAR_DATA *         carried_by;
    EXTRA_DESCR_DATA *  extra_descr;
    AFFECT_DATA *       affected;
    OBJ_INDEX_DATA *    pIndexData;
    ROOM_INDEX_DATA *   in_room;
    bool                enchanted;
    char *              owner;
    char *              name;
    char *              short_descr;
    char *              description;
    sh_int              item_type;
    int                 extra_flags;
    sh_int              wear_flags;
    sh_int              wear_loc;
    sh_int              weight;
    int                 cost;
    sh_int              level;
    long                material;
    sh_int              timer;
    int                 value   [5];
	bool immortalObject;
};



/*
 * Exit data.
 */
struct  exit_data
{
    union
    {
        ROOM_INDEX_DATA *       to_room;
        sh_int                  vnum;
    } u1;
    EXIT_DATA *         next;           /* OLC */
    int                 rs_flags;       /* OLC */
    int                 orig_door;      /* OLC */
    sh_int              exit_info;
    sh_int              key;
    char *              keyword;
    char *              description;
};



/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile 
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct  reset_data
{
    RESET_DATA *        next;
    char                command;
    sh_int              arg1;
    sh_int              arg2;
    sh_int              arg3;
};



/*
 * Area definition.
 */
struct  area_data
{
    AREA_DATA *         next;
    RESET_DATA *        reset_first;
    RESET_DATA *        reset_last;
    char *              name;
    sh_int              age;
    sh_int              nplayer;
    bool                empty;          /* ROM OLC */
    char *              filename;       /* OLC */
    char *              builders;       /* OLC */ /* Listing of */
    int                 security;       /* OLC */ /* Value 1-9  */
    int                 lvnum;          /* OLC */ /* Lower vnum */
    int                 uvnum;          /* OLC */ /* Upper vnum */
    int                 vnum;           /* OLC */ /* Area vnum  */
    int                 area_flags;     /* OLC */
};



/*
 * Room type.
 */
struct  room_index_data
{
   ROOM_INDEX_DATA     *next;
   RESET_DATA          *reset_first;    /* OLC */
   RESET_DATA          *reset_last;     /* OLC */
   CHAR_DATA           *people;
   OBJ_DATA            *contents;
   EXTRA_DESCR_DATA    *extra_descr;
   AREA_DATA           *area;
   EXIT_DATA           *exit    [6];
   char                *name;
   char                *description;
   sh_int               vnum;
   int                  room_flags;
   sh_int               light;
   sh_int               sector_type;
   /* added by Rahl */
   int                  status;
   int                  timer;
   char                *owner;
};



/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                     1000



/*
 *  Target types.
 */
#define TAR_IGNORE                  0
#define TAR_CHAR_OFFENSIVE          1
#define TAR_CHAR_DEFENSIVE          2
#define TAR_CHAR_SELF               3
#define TAR_OBJ_INV                 4
/* by Rahl */
#define TAR_CHAR_OTHER              5
#define TAR_OBJ_CHAR_OFF            6
#define TAR_OBJ_CHAR_DEF            7

#define TARGET_CHAR                 0
#define TARGET_OBJ                  1
#define TARGET_ROOM                 2
#define TARGET_NONE                 3

/*
 * Skills include spells as a particular case.
 */
struct  skill_type
{
    char *      name;                   /* Name of skill                */
    sh_int      skill_level[MAX_CLASS]; /* Level needed by class        */
    sh_int      rating[MAX_CLASS];      /* How hard it is to learn      */      
    SPELL_FUN * spell_fun;              /* Spell pointer (for spells)   */
    sh_int      target;                 /* Legal targets                */
    sh_int      minimum_position;       /* Position for caster / user   */
    sh_int *    pgsn;                   /* Pointer to associated gsn    */
    sh_int      slot;                   /* Slot for #OBJECT loading     */
    sh_int      min_mana;               /* Minimum mana used            */
    sh_int      beats;                  /* Waiting time after use       */
    char *      noun_damage;            /* Damage message               */
    char *      msg_off;                /* Wear off message             */
    /* added by Rahl */
    char *      msg_obj;                /* Wear off message for objects */
};

struct  group_type
{
    char *      name;
    sh_int      rating[MAX_CLASS];
    char *      spells[MAX_IN_GROUP];
};

struct clan_data
{
    CLAN_DATA *         next;
    sh_int              number;
    char *              name;
    char *              visible;
    char *              leader;
    char *              members[MAX_CLAN_MEMBERS];
    char *              god;
    sh_int              max_members;
    sh_int              min_level;
    sh_int              num_members;
    long                req_flags;
    long                cost_gold;
    long                cost_xp;
    bool                classes[MAX_CLASS];
    bool                races[MAX_PC_RACE];
    bool                auto_accept;
};



/*
 * These are skill_lookup return values for common skills and spells.
 */
extern  sh_int  gsn_backstab;
extern  sh_int  gsn_dodge;
extern  sh_int  gsn_hide;
extern  sh_int  gsn_peek;
extern  sh_int  gsn_pick_lock;
extern  sh_int  gsn_sneak;
extern  sh_int  gsn_steal;

extern  sh_int  gsn_disarm;
extern  sh_int  gsn_enhanced_damage;
extern  sh_int  gsn_kick;
extern  sh_int  gsn_parry;
extern  sh_int  gsn_rescue;
extern  sh_int  gsn_second_attack;
extern  sh_int  gsn_third_attack;

extern  sh_int  gsn_blindness;
extern  sh_int  gsn_charm_person;
extern  sh_int  gsn_curse;
extern  sh_int  gsn_invis;
extern  sh_int  gsn_mass_invis;
extern  sh_int  gsn_plague;
extern  sh_int  gsn_poison;
extern  sh_int  gsn_sleep;

/* new gsns */
extern sh_int  gsn_axe;
extern sh_int  gsn_dagger;
extern sh_int  gsn_flail;
extern sh_int  gsn_mace;
extern sh_int  gsn_polearm;
extern sh_int  gsn_shield_block;
extern sh_int  gsn_spear;
extern sh_int  gsn_sword;
extern sh_int  gsn_whip;
 
extern sh_int  gsn_bash;
extern sh_int  gsn_berserk;
extern sh_int  gsn_dirt;
extern sh_int  gsn_hand_to_hand;
extern sh_int  gsn_trip;
 
extern sh_int  gsn_fast_healing;
extern sh_int  gsn_haggle;
extern sh_int  gsn_lore;
extern sh_int  gsn_meditation;
 
extern sh_int  gsn_scrolls;
extern sh_int  gsn_staves;
extern sh_int  gsn_wands;
extern sh_int  gsn_recall;
extern sh_int  gsn_brew;
extern sh_int  gsn_scribe;

/* new gsns by Rahl */
extern sh_int  gsn_push;
extern sh_int  gsn_butcher;
extern sh_int  gsn_circle;
extern sh_int  gsn_grip;
extern sh_int  gsn_envenom;
extern sh_int  gsn_whirlwind;

/*
 * Utility macros.
 */
#define IS_VALID(data)          ((data) != NULL && (data)->valid)
#define VALIDATE(data)          ((data)->valid = TRUE)
#define INVALIDATE(data)        ((data)->valid = FALSE)
#define UMIN(a, b)              ((a) < (b) ? (a) : (b))
#define UMAX(a, b)              ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)         ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)                ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)                ((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define IS_SET(flag, bit)       ((flag) & (bit))
#define SET_BIT(var, bit)       ((var) |= (bit))
#define REMOVE_BIT(var, bit)    ((var) &= ~(bit))

/* added by Rahl for automated questing */
#define IS_QUESTOR(ch)          (IS_SET((ch)->act, PLR_QUESTOR))


/*
 * Character macros.
 */
#define IS_NPC(ch)              (IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)         ((ch)->immRank >= 1)
#define IS_HERO(ch)             (char_getImmRank(ch) >= HERO)
#define IS_TRUSTED(ch,level)    (char_getImmRank((ch)) >= (level))
#define IS_AFFECTED(ch, sn)     (IS_SET((ch)->affected_by, (sn)))

#define GET_AGE(ch)             ((int) (17 + ((ch)->played \
                                    + current_time - (ch)->logon )/72000))

#define IS_GOOD(ch)             (ch->alignment >= 350)
#define IS_EVIL(ch)             (ch->alignment <= -350)
#define IS_NEUTRAL(ch)          (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)            (ch->position > POS_SLEEPING)
#define GET_AC(ch,type)         ((ch)->armor[type]                          \
                        + ( IS_AWAKE(ch)                            \
                        ? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0 ))  
#define GET_HITROLL(ch) \
                ((ch)->hitroll+str_app[get_curr_stat(ch,STAT_STR)].tohit)
#define GET_DAMROLL(ch) \
                ((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)

#define IS_OUTSIDE(ch)          (!IS_SET(                                   \
                                    (ch)->in_room->room_flags,              \
                                    ROOM_INDOORS))

#define WAIT_STATE(ch, npulse)  ((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse)  ((ch)->daze = UMAX((ch)->daze, (npulse)))
/* added by Rahl */
#define IS_AFFECTED2( ch, sn )  ( IS_SET( ( ch )->affected2_by, ( sn ) ) )

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)     (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)  (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj,stat)(IS_SET((obj)->value[4],(stat)))



/*
 * Description macros.
 */
#define PERS(ch, looker)        ( can_see( looker, (ch) ) ?             \
                                ( IS_NPC(ch) ? (ch)->short_descr        \
                                : (ch)->name ) : "someone" )

/*
 * Structure for a social in the socials table.
 */
struct  social_type
{
    char      name[20];
    char *    char_no_arg;
    char *    others_no_arg;
    char *    char_found;
    char *    others_found;
    char *    vict_found;
    char *    char_not_found;
    char *    char_auto;
    char *    others_auto;
};


/*
 * Global constants.
 */
extern  const   struct  str_app_type    str_app         [26];
extern  const   struct  int_app_type    int_app         [26];
extern  const   struct  wis_app_type    wis_app         [26];
extern  const   struct  dex_app_type    dex_app         [26];
extern  const   struct  con_app_type    con_app         [26];

extern  const   struct  class_type      class_table     [MAX_CLASS];
extern  const   struct  attack_type     attack_table    [];
extern  const   struct  race_type       race_table      [];
extern  const   struct  pc_race_type    pc_race_table   [];
extern  const   struct  liq_type        liq_table       [LIQ_MAX];
extern  const   struct  skill_type      skill_table     [MAX_SKILL];
extern  const   struct  group_type      group_table     [MAX_GROUP];
extern          struct social_type      social_table[MAX_SOCIALS];

/* new stuff by Rahl */
extern  const   struct  weapon_type     weapon_table    [];
extern  const   struct  wiznet_type     wiznet_table    [];

/*
 * Global variables.
 */
extern          HELP_DATA         *     help_first;
extern          SHOP_DATA         *     shop_first;
extern          CLAN_DATA         *     clan_first;
extern          CLAN_DATA         *     clan_last;

/* extern               BAN_DATA          *     ban_list; */
extern          CHAR_DATA         *     char_list;
extern          DESCRIPTOR_DATA   *     descriptor_list;
extern          NOTE_DATA         *     note_list;
extern          OBJ_DATA          *     object_list;

extern          AFFECT_DATA       *     affect_free;
extern          BAN_DATA          *     ban_free;
extern          CHAR_DATA         *     char_free;
extern          DESCRIPTOR_DATA   *     descriptor_free;
extern          EXTRA_DESCR_DATA  *     extra_descr_free;
extern          NOTE_DATA         *     note_free;
extern          OBJ_DATA          *     obj_free;
extern          PC_DATA           *     pcdata_free;

extern          char                    bug_buf         [];
extern          time_t                  current_time;
extern          bool                    fLogAll;
extern          FILE *                  fpReserve;
//extern          KILL_DATA               kill_table      [];
extern          char                    log_buf         [];
extern          TIME_INFO_DATA          time_info;
extern          WEATHER_DATA            weather_info;

/* more junk added by Rahl */
extern AUCTION_DATA             * auction;
extern DISABLED_DATA            * disabled_first; /* interp.c */
#define DISABLED_FILE "disabled.txt"
#define CHANGES_FILE "changes.txt"
#define BAN_FILE "ban.txt"
#define MUSIC_FILE "music.txt"

/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if     defined(linux)
char *  crypt           ( const char *key, const char *salt );
#endif

#if     defined(sun)
char *  crypt           ( const char *key, const char *salt );
int     fclose          ( FILE *stream );
int     fprintf         ( FILE *stream, const char *format, ... );
#if     defined(SYSV)
siz_t   fread           ( void *ptr, size_t size, size_t n, 
                            FILE *stream );
#else
int     fread           ( void *ptr, int size, int n, FILE *stream );
#endif
int     fseek           ( FILE *stream, long offset, int ptrname );
void    perror          ( const char *s );
int     ungetc          ( int c, FILE *stream );
#endif


/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if     defined(NOCRYPT)
#define crypt(s1, s2)   (s1)
#endif


/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#define PLAYER_DIR      "../player/"    /* Player files                 */
#define PLAYER_TEMP     "../player/temp"
#define GOD_DIR         "../gods/"      /* list of gods                 */
#define NULL_FILE       "/dev/null"     /* To reserve one stream        */

#define AREA_LIST       "area.lst"      /* List of areas                */

#define BUG_FILE        "bugs.txt"      /* For 'bug' and bug( )         */
#define IDEA_FILE       "ideas.txt"     /* For 'idea'                   */
#define TYPO_FILE       "typos.txt"     /* For 'typo'                   */
#define NOTE_FILE       "notes.txt"     /* For 'notes'                  */
#define SHUTDOWN_FILE   "shutdown.txt"  /* For 'shutdown'               */
#define CHAOS_FILE      "chaos.txt"     /* For chaos points             */


/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD      CHAR_DATA
#define MID     MOB_INDEX_DATA
#define OD      OBJ_DATA
#define OID     OBJ_INDEX_DATA
#define RID     ROOM_INDEX_DATA
#define SF      SPEC_FUN
#define AD      AFFECT_DATA

/* act_comm.c */
bool    is_note_to      ( CHAR_DATA *ch, NOTE_DATA *pnote );
void    check_sex       ( CHAR_DATA *ch );
void    add_follower    ( CHAR_DATA *ch, CHAR_DATA *master );
void    stop_follower   ( CHAR_DATA *ch );
void    nuke_pets       ( CHAR_DATA *ch );
void    die_follower    ( CHAR_DATA *ch );
bool    is_same_group   ( CHAR_DATA *ach, CHAR_DATA *bch );
bool    is_same_clan    ( CHAR_DATA *ch, CHAR_DATA *victim );
void    talk_auction    ( char *argument );
void    show_time       ( CHAR_DATA *ch );

/* act_info.c */
void    set_title       ( CHAR_DATA *ch, char *title );

/* act_move.c */
void    move_char       ( CHAR_DATA *ch, int door, bool follow );

/* act_obj.c */
bool can_loot           (CHAR_DATA *ch, OBJ_DATA *obj );
void    get_obj         ( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container );

/* act_wiz.c */
/* added by Rahl */
void    wiznet          ( char *string, CHAR_DATA *ch, OBJ_DATA
                                *obj, long flag, long flag_skip, int
                                min_level );

/* alias.c */
void    substitute_alias ( DESCRIPTOR_DATA *d, char *input );

/* ban.c added by Rahl */
bool    check_ban       ( char *site, int type );

/* comm.c */
void    show_string     ( struct descriptor_data *d, char *input );
void    close_socket    ( DESCRIPTOR_DATA *dclose );
void    write_to_buffer ( DESCRIPTOR_DATA *d, const char *txt,
                            int length );
void    send_to_char    ( const char *txt, CHAR_DATA *ch );
void    send_to_desc    ( char *txt, DESCRIPTOR_DATA *d );
void    page_to_char    ( const char *txt, CHAR_DATA *ch );
void    act             ( const char *format, CHAR_DATA *ch,
                            const void *arg1, const void *arg2, int type );
void    act_new         ( const char *format, CHAR_DATA *ch, 
                            const void *arg1, const void *arg2, int type,
                            int min_pos );
bool    check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn );
bool    check_parse_name( char *name );
bool    check_playing( DESCRIPTOR_DATA *d, char *name );

/* db.c */
void    boot_db         ( void );
void    area_update     ( void );
CD *    create_mobile   ( MOB_INDEX_DATA *pMobIndex );
void    clone_mobile    ( CHAR_DATA *parent, CHAR_DATA *clone );
OD *    create_object   ( OBJ_INDEX_DATA *pObjIndex, int level );
void    clone_object    ( OBJ_DATA *parent, OBJ_DATA *clone );
void    clear_char      ( CHAR_DATA *ch );
void    free_char       ( CHAR_DATA *ch );
char *  get_extra_descr ( const char *name, EXTRA_DESCR_DATA *ed );
MID *   get_mob_index   ( int vnum );
OID *   get_obj_index   ( int vnum );
RID *   get_room_index  ( int vnum );
char    fread_letter    ( FILE *fp );
int     fread_number    ( FILE *fp );
long    fread_flag      ( FILE *fp );
char *  fread_string    ( FILE *fp );
char *  fread_string_eol( FILE *fp );
void    fread_to_eol    ( FILE *fp );
char *  fread_word      ( FILE *fp );
long    flag_convert    ( char letter );
void *  alloc_perm      ( int sMem );
char *  str_dup         ( const char *str );
void    free_string     ( char *pstr );
int     number_fuzzy    ( int number );
int     number_range    ( int from, int to );
int     number_percent  ( void );
int     number_door     ( void );
int     number_bits     ( int width );
int     number_mm       ( void );
int     dice            ( int number, int size );
int     interpolate     ( int level, int value_00, int value_32 );
void    smash_tilde     ( char *str );
bool    str_cmp         ( const char *astr, const char *bstr );
bool    str_prefix      ( const char *astr, const char *bstr );
bool    str_infix       ( const char *astr, const char *bstr );
bool    str_suffix      ( const char *astr, const char *bstr );
char *  capitalize      ( const char *str );
void    append_file     ( CHAR_DATA *ch, char *file, char *str );
void    bug             ( const char *str, int param );
void    log_string      ( const char *str );
void    tail_chain      ( void );
/* added by Rahl */
void    load_disabled   ( void );
void    save_disabled   ( void );
int     number_fuzzier  ( int number );
int     number_fuzziest ( int number );

/* effects.c - added by Rahl */
void    acid_effect     ( void *vo, int level, int dam, int target );
void    cold_effect     ( void *vo, int level, int dam, int target );
void    fire_effect     ( void *vo, int level, int dam, int target );
void    poison_effect   ( void *vo, int level, int dam, int target );
void    shock_effect    ( void *vo, int level, int dam, int target );

/* clans.c */

CLAN_DATA       *find_clan      (char *name);
void            save_clans      ( void );
char *          vis_clan        ( int clan );
sh_int          clan_accept     (CHAR_DATA *ch, char *clan );
char *          clan_lookup     ( int clan );
/*
bool            is_clan_leader  ( char *name, char *Clan );
*/
/* added by Rahl */
int             get_clan        ( char *name );
bool            is_clan_leader  ( CHAR_DATA *ch, char *Clan );

/* fight.c */
bool    is_safe         (CHAR_DATA *ch, CHAR_DATA *victim );
bool    is_safe_spell   (CHAR_DATA *ch, CHAR_DATA *victim, bool area );
void    violence_update ( void );
void    multi_hit       ( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
bool    damage          ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
                                int dt, int class, bool show );
void     update_pos     ( CHAR_DATA *victim ); 
void    stop_fighting   ( CHAR_DATA *ch, bool fBoth );

/* handler.c */
/* date and time functions added by Rahl - from EmberMUD 0.29 */
char *get_curdate();
char *get_curtime();
char *get_time();
char *get_date();
AD      *affect_find    (AFFECT_DATA *paf, int sn);
int     check_immune    (CHAR_DATA *ch, int dam_type );
long    material_lookup ( const char *name );
char *  material_name   ( long num );
int     race_lookup     ( const char *name );
int     class_lookup    ( const char *name );
int     get_skill       ( CHAR_DATA *ch, int sn );
int     get_weapon_sn   ( CHAR_DATA *ch );
int     get_second_weapon_sn    ( CHAR_DATA *ch );
int     get_weapon_skill ( CHAR_DATA *ch, int sn );
int     get_age         ( CHAR_DATA *ch );
void    reset_char      ( CHAR_DATA *ch );
int     get_curr_stat   ( CHAR_DATA *ch, int stat );
int     get_max_train   ( CHAR_DATA *ch, int stat );
int     can_carry_n     ( CHAR_DATA *ch );
int     can_carry_w     ( CHAR_DATA *ch );
bool    is_name         ( char *str, char *namelist );
bool    is_exact_name   ( char *str, char *namelist );
void    affect_to_char  ( CHAR_DATA *ch, AFFECT_DATA *paf );
void    affect_to_obj   ( OBJ_DATA *obj, AFFECT_DATA *paf );
void    affect_remove   ( CHAR_DATA *ch, AFFECT_DATA *paf );
void    affect_remove_obj (OBJ_DATA *obj, AFFECT_DATA *paf );
void    affect_strip    ( CHAR_DATA *ch, int sn );
bool    is_affected     ( CHAR_DATA *ch, int sn );
void    affect_join     ( CHAR_DATA *ch, AFFECT_DATA *paf );
void    char_from_room  ( CHAR_DATA *ch );
void    char_to_room    ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex );
void    obj_to_char     ( OBJ_DATA *obj, CHAR_DATA *ch );
void    obj_from_char   ( OBJ_DATA *obj );
int     apply_ac        ( OBJ_DATA *obj, int iWear, int type );
OD *    get_eq_char     ( CHAR_DATA *ch, int iWear );
void    equip_char      ( CHAR_DATA *ch, OBJ_DATA *obj, int iWear );
void    unequip_char    ( CHAR_DATA *ch, OBJ_DATA *obj );
int     count_obj_list  ( OBJ_INDEX_DATA *obj, OBJ_DATA *list );
void    obj_from_room   ( OBJ_DATA *obj );
void    obj_to_room     ( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex );
void    obj_to_obj      ( OBJ_DATA *obj, OBJ_DATA *obj_to );
void    obj_from_obj    ( OBJ_DATA *obj );
void    extract_obj     ( OBJ_DATA *obj );
void    extract_char    ( CHAR_DATA *ch, bool fPull );
CD *    get_char_room   ( CHAR_DATA *ch, char *argument );
CD *    get_char_world  ( CHAR_DATA *ch, char *argument );
OD *    get_obj_type    ( OBJ_INDEX_DATA *pObjIndexData );
OD *    get_obj_list    ( CHAR_DATA *ch, char *argument,
                            OBJ_DATA *list );
OD *    get_obj_carry   ( CHAR_DATA *ch, char *argument );
OD *    get_obj_wear    ( CHAR_DATA *ch, char *argument );
OD *    get_obj_here    ( CHAR_DATA *ch, char *argument );
OD *    get_obj_world   ( CHAR_DATA *ch, char *argument );
OD *    create_money    ( int amount );
int     get_obj_number  ( OBJ_DATA *obj );
int     get_obj_weight  ( OBJ_DATA *obj );
bool    room_is_dark    ( ROOM_INDEX_DATA *pRoomIndex );
bool    room_is_private ( ROOM_INDEX_DATA *pRoomIndex );
bool    can_see         ( CHAR_DATA *ch, CHAR_DATA *victim );
bool    can_see_obj     ( CHAR_DATA *ch, OBJ_DATA *obj );
bool    can_see_room    ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex );
bool    can_drop_obj    ( CHAR_DATA *ch, OBJ_DATA *obj );
char *  item_type_name  ( OBJ_DATA *obj );
char *  affect_loc_name ( int location );
char *  affect_bit_name ( int vector );
char *  extra_bit_name  ( int extra_flags );
char *  wear_bit_name   ( int wear_flags );
char *  act_bit_name    ( int act_flags );
char *  off_bit_name    ( int off_flags );
char *  imm_bit_name    ( int imm_flags );
char *  form_bit_name   ( int form_flags );
char *  part_bit_name   ( int part_flags );
char *  weapon_bit_name ( int weapon_flags );
char *  comm_bit_name   ( int comm_flags );
/* added by Rahl */
int weapon_lookup       ( const char *name );
long wiznet_lookup      ( const char *name );
void affect_enchant     ( OBJ_DATA *obj );
void affect_check       ( CHAR_DATA *ch, int where, int vector );
char *  affect2_bit_name ( int vector );
CD *    get_char_area   ( CHAR_DATA *ch, char *argument );
bool    is_room_owner   ( CHAR_DATA *ch, ROOM_INDEX_DATA *room );
void bugf( char *fmt, ...) __attribute__ ((format(printf,1,2)));
void log_stringf( char *fmt, ...) __attribute__ ((format(printf,1,2)));
void printf_to_char ( CHAR_DATA *ch, char *fmt, ...) __attribute__ ((format(printf,2,3)));

/* handle_con.c */
void handle_con_get_name( DESCRIPTOR_DATA *d, char *argument );
void handle_con_get_old_password( DESCRIPTOR_DATA *d, char *argument );
void handle_con_break_connect( DESCRIPTOR_DATA *d, char *argument );
void handle_con_confirm_new_name( DESCRIPTOR_DATA *d, char *argument );
void handle_con_get_new_password( DESCRIPTOR_DATA *d, char *argument );
void handle_con_confirm_new_password( DESCRIPTOR_DATA *d, char *argument );
void handle_con_get_new_race( DESCRIPTOR_DATA *d, char *argument );
void handle_con_get_new_sex( DESCRIPTOR_DATA *d, char *argument );
void handle_con_get_stats( DESCRIPTOR_DATA *d, char *argument );
void handle_con_get_new_class( DESCRIPTOR_DATA *d, char *argument );
void handle_con_get_alignment( DESCRIPTOR_DATA *d, char *argument );
void handle_con_default_choice( DESCRIPTOR_DATA *d, char *argument );
void handle_con_pick_weapon( DESCRIPTOR_DATA *d, char *argument );
void handle_con_gen_groups( DESCRIPTOR_DATA *d, char *argument );
void handle_con_read_imotd( DESCRIPTOR_DATA *d, char *argument );
void handle_con_read_motd( DESCRIPTOR_DATA *d, char *argument );
void handle_con_ansi( DESCRIPTOR_DATA *d, char *argument );


/* interp.c */
void    interpret       ( CHAR_DATA *ch, char *argument );
bool    is_number       ( char *arg );
int     number_argument ( char *argument, char *arg );
char *  one_argument    ( char *argument, char *arg_first );

/* magic.c */
int mana_cost( int min_mana, int percent );
int     skill_lookup    ( const char *name );
int     slot_lookup     ( int slot ); 
bool    saves_spell     ( int level, CHAR_DATA *victim, int dam_type );
void    obj_cast_spell  ( int sn, int level, CHAR_DATA *ch,
                                    CHAR_DATA *victim, OBJ_DATA *obj );

/* save.c */
void    save_char_obj   ( CHAR_DATA *ch );
bool    load_char_obj   ( DESCRIPTOR_DATA *d, char *name );
char *  print_flags     ( int flag );

/* skills.c */
bool    parse_gen_groups ( CHAR_DATA *ch,char *argument );
void    list_group_costs ( CHAR_DATA *ch );
void    list_group_known ( CHAR_DATA *ch );
void    check_improve   ( CHAR_DATA *ch, int sn, bool success, int multiplier );
int     group_lookup    (const char *name );
void    gn_add          ( CHAR_DATA *ch, int gn );
void    gn_remove       ( CHAR_DATA *ch, int gn );
void    group_add       ( CHAR_DATA *ch, const char *name, bool deduct );
void    group_remove    ( CHAR_DATA *ch, const char *name );

/* special.c */
SF *    spec_lookup     ( const char *name );

/* update.c */
void    gain_exp        ( CHAR_DATA *ch, int gain );
void    gain_condition  ( CHAR_DATA *ch, int iCond, int value );
void    update_handler  ( void );


/* quest.c */
void    add_random_apply ( CHAR_DATA *ch, OBJ_DATA *obj );

/* bonus_update.c */
void    bonus_update     ( void );
CHAR_DATA *get_random_mob ( void );

#undef  CD
#undef  MID
#undef  OD
#undef  OID
#undef  RID
#undef  SF


/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * This structure is used in special.c to lookup spec funcs and
 * also in olc_act.c to display listings of spec funcs.
 */
struct spec_type
{
    char *      spec_name;
    SPEC_FUN *  spec_fun;
};



/*
 * This structure is used in bit.c to lookup flags and stats.
 */
struct flag_type
{
    char * name;
    int  bit;
    bool settable;
};



/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY  1



/*
 * Area flags.
 */
#define         AREA_NONE       0
#define         AREA_CHANGED    1       /* Area has been modified. */
#define         AREA_ADDED      2       /* Area has been added to. */
#define         AREA_LOADING    4       /* Used for counting in db.c */



#define MAX_DIR 6
#define NO_FLAG -99     /* Must not be used in flags or stats. */


/*
 * Global Constants
 */
extern  char *  const   dir_name        [];
extern  const   sh_int  rev_dir         [];          /* sh_int - ROM OLC */
extern  const   struct  spec_type       spec_table      [];



/*
 * Global variables
 */
extern          AREA_DATA *             area_first;
extern          AREA_DATA *             area_last;
extern          SHOP_DATA *             shop_last;

extern          int                     top_affect;
extern          int                     top_area;
extern          int                     top_ed;
extern          int                     top_exit;
extern          int                     top_help;
extern          int                     top_mob_index;
extern          int                     top_obj_index;
extern          int                     top_reset;
extern          int                     top_room;
extern          int                     top_shop;
extern          int                     top_clan;

extern          int                     top_vnum_mob;
extern          int                     top_vnum_obj;
extern          int                     top_vnum_room;

extern          char                    str_empty       [1];

extern  MOB_INDEX_DATA *        mob_index_hash  [MAX_KEY_HASH];
extern  OBJ_INDEX_DATA *        obj_index_hash  [MAX_KEY_HASH];
extern  ROOM_INDEX_DATA *       room_index_hash [MAX_KEY_HASH];

extern			bool					DEBUG;
extern bool chaos;
extern bool wizlock;
extern bool newlock;

/* act_wiz.c */
/*
ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg );
*/
/* db.c */
void    reset_area      ( AREA_DATA * pArea );
void    reset_room      ( ROOM_INDEX_DATA *pRoom );

/* string.c */
void    string_edit     ( CHAR_DATA *ch, char **pString );
void    string_append   ( CHAR_DATA *ch, char **pString );
char *  string_replace  ( char * orig, char * old, char * newstr );
void    string_add      ( CHAR_DATA *ch, char *argument );
char *  format_string   ( char *oldstring /*, bool fSpace */ );
char *  first_arg       ( char *argument, char *arg_first, bool fCase );
char *  string_unpad    ( char * argument );
char *  string_proper   ( char * argument );
char *  del_last_line   ( char * string );

/* olc.c */
bool    run_olc_editor  ( DESCRIPTOR_DATA *d );
char    *olc_ed_name    ( CHAR_DATA *ch );
char    *olc_ed_vnum    ( CHAR_DATA *ch );

/* special.c */
char *  spec_string     ( SPEC_FUN *fun );      /* OLC */

/* bit.c */
extern const struct flag_type   area_flags[];
extern const struct flag_type   sex_flags[];
extern const struct flag_type   exit_flags[];
extern const struct flag_type   door_resets[];
extern const struct flag_type   room_flags[];
extern const struct flag_type   sector_flags[];
extern const struct flag_type   type_flags[];
extern const struct flag_type   extra_flags[];
extern const struct flag_type   wear_flags[];
extern const struct flag_type   act_flags[];
extern const struct flag_type   affect_flags[];
extern const struct flag_type   apply_flags[];
extern const struct flag_type   wear_loc_strings[];
extern const struct flag_type   wear_loc_flags[];
extern const struct flag_type   weapon_flags[];
extern const struct flag_type   container_flags[];
extern const struct flag_type   liquid_flags[];
/* added by Rahl */
extern const struct flag_type   affect2_flags[];


/* ROM OLC: */

extern const struct flag_type   material_type[];
extern const struct flag_type   form_flags[];
extern const struct flag_type   part_flags[];
extern const struct flag_type   ac_type[];
extern const struct flag_type   size_flags[];
extern const struct flag_type   off_flags[];
extern const struct flag_type   imm_flags[];
extern const struct flag_type   res_flags[];
extern const struct flag_type   vuln_flags[];
extern const struct flag_type   position_flags[];
extern const struct flag_type   weapon_class[];
extern const struct flag_type   weapon_type[];



/*****************************************************************************
 *                                 OLC END                                   *
 *****************************************************************************/

/* act_wiz.c */
ROOM_INDEX_DATA *       find_location   ( CHAR_DATA *ch, char *arg );

/* fight.c */
void                    death_cry       ( CHAR_DATA *ch );

int char_getImmRank( CHAR_DATA *ch );
void char_setImmRank( CHAR_DATA *ch, int newRank );
