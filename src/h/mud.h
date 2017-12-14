/*****************************************************************************
 * DikuMUD (C) 1990, 1991 by:                                                *
 *   Sebastian Hammer, Michael Seifert, Hans Henrik Staefeldt, Tom Madsen,   *
 *   and Katja Nyboe.                                                        *
 *---------------------------------------------------------------------------*
 * MERC 2.1 (C) 1992, 1993 by:                                               *
 *   Michael Chastain, Michael Quan, and Mitchell Tse.                       *
 *---------------------------------------------------------------------------*
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998 by: Derek Snider.                    *
 *   Team: Thoric, Altrag, Blodkai, Narn, Haus, Scryn, Rennard, Swordbearer, *
 *         gorog, Grishnakh, Nivek, Tricops, and Fireblade.                  *
 *---------------------------------------------------------------------------*
 * SMAUG 1.7 FUSS by: Samson and others of the SMAUG community.              *
 *                    Their contributions are greatly appreciated.           *
 *---------------------------------------------------------------------------*
 * LoP (C) 2006 - 2012 by: the LoP team.                                     *
 *---------------------------------------------------------------------------*
 * Win32 port by Nick Gammon                                                 *
 *---------------------------------------------------------------------------*
 *                           Main mud header file                            *
 *****************************************************************************/

#include <stdlib.h>
#include <limits.h>
#if defined(__CYGWIN__) || defined(__FreeBSD__)
   #include <sys/time.h>
#endif
#include <typeinfo>

#ifdef WIN32
   #include <winsock.h>
   #include <sys/types.h>
   #pragma warning( disable: 4018 4244 4550 4761 )
   #define index strchr
   #define rindex strrchr
   #define vsnprintf _vsnprintf  /* NJG */
   #define snprintf _snprintf /* NJG */
   #define lstat stat   /* NJG */
   #pragma comment( lib, "ws2_32.lib" )   /* NJG */
   #pragma comment( lib, "winmm.lib" ) /* NJG */
#endif

typedef int ch_ret;
typedef int obj_ret;

#define NM extern "C"
#define DECLARE_DO_FUN( fun )    NM DO_FUN    fun
#define DECLARE_SPEC_FUN( fun )  NM SPEC_FUN  fun
#define DECLARE_SPELL_FUN( fun ) NM SPELL_FUN fun

/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if !defined(BERR)
   #define BERR 255
#endif

#include "const.h"

/* Structure types. */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct ban_data BAN_DATA;
typedef struct char_data CHAR_DATA;
typedef struct hunt_hate_fear HHF_DATA;
typedef struct fighting_data FIGHT_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct char_morph CHAR_MORPH;
typedef struct morph_data MORPH_DATA;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;
typedef struct reset_data RESET_DATA;
typedef struct reset_track_data RESET_TRACK_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct race_type RACE_TYPE;
typedef struct class_type CLASS_TYPE;
typedef struct repairshop_data REPAIR_DATA;
typedef struct weather_data WEATHER_DATA;
typedef struct neighbor_data NEIGHBOR_DATA;  /* FB */
typedef struct council_data COUNCIL_DATA;
typedef struct mob_prog_data MPROG_DATA;
typedef struct mob_prog_act_list MPROG_ACT_LIST;
typedef struct editor_data EDITOR_DATA;
typedef struct timer_data TIMER;
typedef struct system_data SYSTEM_DATA;
typedef struct skill_type SKILLTYPE;
typedef struct social_type SOCIALTYPE;
typedef struct cmd_type CMDTYPE;
typedef struct killed_data KILLED_DATA;
typedef struct ignore_data IGNORE_DATA;
typedef struct host_data HOST_DATA;
typedef struct extended_bitvector EXT_BV;
typedef struct lcnv_data LCNV_DATA;
typedef struct lang_data LANG_DATA;
typedef struct group_data GROUP_DATA;

/* Function types. */
#define CMDF( fun ) NM void fun( CHAR_DATA *ch, char *argument ) 
typedef void DO_FUN( CHAR_DATA *ch, char *argument );
typedef ch_ret SPELL_FUN( int sn, int level, CHAR_DATA *ch, void *vo );
typedef bool SPEC_FUN( CHAR_DATA *ch );

#define DUR_CONV	23.333333333333333333333333
#define HIDDEN_TILDE	'*'

/* 32bit bitvector defines */
#define BV00 ( 1 <<  0 )
#define BV01 ( 1 <<  1 )
#define BV02 ( 1 <<  2 )
#define BV03 ( 1 <<  3 )
#define BV04 ( 1 <<  4 )
#define BV05 ( 1 <<  5 )
#define BV06 ( 1 <<  6 )
#define BV07 ( 1 <<  7 )
#define BV08 ( 1 <<  8 )
#define BV09 ( 1 <<  9 )
#define BV10 ( 1 << 10 )
#define BV11 ( 1 << 11 )
#define BV12 ( 1 << 12 )
#define BV13 ( 1 << 13 )
#define BV14 ( 1 << 14 )
#define BV15 ( 1 << 15 )
#define BV16 ( 1 << 16 )
#define BV17 ( 1 << 17 )
#define BV18 ( 1 << 18 )
#define BV19 ( 1 << 19 )
#define BV20 ( 1 << 20 )
#define BV21 ( 1 << 21 )
#define BV22 ( 1 << 22 )
#define BV23 ( 1 << 23 )
#define BV24 ( 1 << 24 )
#define BV25 ( 1 << 25 )
#define BV26 ( 1 << 26 )
#define BV27 ( 1 << 27 )
#define BV28 ( 1 << 28 )
#define BV29 ( 1 << 29 )
#define BV30 ( 1 << 30 )
#define BV31 ( 1 << 31 )
/* 32 USED! DO NOT ADD MORE! SB */

/* String and memory management parameters. */
#define MKH                2048
//#define MAX_KEY_HASH        MKH
#define MSL                4096
//#define MAX_STRING_LENGTH   MSL
#define MIL                1024
//#define MAX_INPUT_LENGTH    MIL
#define MIS                1024
//#define MAX_INBUF_SIZE      MIS
#define MLS                 252
//#define MAX_LINE_SIZE       MLS
#define MEL                  49
//#define MAX_EDITOR_LINES    MEL
#define MFN                1024
//#define MAX_FILE_NAME       MFN

#define HASHSTR   /* use string hashing */

#define MAX_NEST       20 /* maximum container nesting */

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_EXP_WORTH  500000
#define MIN_EXP_WORTH      20

#define MAX_FIGHT           8

#define MAX_VNUM               100000 /* Game can hold up to 2 billion but this is set low for protection */
#define MAX_REXITS		   20 /* Maximum exits allowed in 1 room */
#define MAX_SKILL		  500 /* Max amount of skills game can have is 1000 ( TYPE_UNKNOWN ) - ( TYPE_HIT ) */
#define SPELL_SILENT_MARKER   "silent" /* No OK. or Failed. */
#define MAX_CLASS           	   50
#define MAX_RACE                   50

#define MAX_CPD             4 /* Maximum council power level difference */
#define MAX_HERB           20 /* Max amount of herbs game can have is 1000 ( TYPE_HERB ) - ( TYPE_PERS ) */
#define MAX_PERS          500 /* Max amount of personal skills/spells game can have is (Unlimited currently) ( TYPE_PERS ) - ... */
#define MAX_PC_PERS         5 /* Maximum amount of personal skills/spells each character can have */
#define MAX_WHERE_NAME     40

#define MAX_LEVEL          100 /* Max Level of all players */
#define TIME_MODIFY          0 /* Done in seconds, Modifies current_time to what you want the time displayed in */
#define PERM_LOG           PERM_BUILDER

#define	SECONDS_PER_TICK   70
#define PULSE_PER_SECOND    4
#define PULSE_VIOLENCE    ( 3 * PULSE_PER_SECOND )
#define PULSE_MOBILE      ( 4 * PULSE_PER_SECOND )
#define PULSE_ROOM        ( 2 * PULSE_PER_SECOND )
#define PULSE_TICK        ( SECONDS_PER_TICK * PULSE_PER_SECOND )
#define PULSE_AREA        ( 60 * PULSE_PER_SECOND )

#define HAS_SPELL_INDEX     -1

typedef struct mpdamage_data MPDAMAGE_DATA;
struct mpdamage_data
{
   MPDAMAGE_DATA *next, *prev;
   char *keyword;
   char *roommsg; /* Hit messsage to room */
   char *charmsg; /* Hit message to char */
   char *victmsg; /* Hit message to victim */
   char *immroom; /* Immune message to room */
   char *immchar; /* Immune message to char */
   char *immvict; /* Immune message to victim */
   char *absroom; /* Absorb message to room */
   char *abschar; /* Absorb message to char */
   char *absvict; /* Absorb message to victim */
   bool resistant[RIS_MAX]; /* This will allow us to set what resistants if any should be checked */
};
#define MPDAMAGE_FILE SYSTEM_DIR "mpdamage.dat"

/* Command logging types. */
typedef enum
{
   LOG_NORMAL,   LOG_ALWAYS,   LOG_NEVER,   LOG_BUILD,   LOG_HIGH,
   LOG_COMM,     LOG_WARN,     LOG_BUG,     LOG_ALL
} log_types;

/* short cut crash bug fix provided by gfinello@mail.karmanet.it*/
typedef enum
{
   relMSET_ON, relOSET_ON
} relation_type;

typedef struct rel_data REL_DATA;

struct rel_data
{
   void *Actor;
   void *Subject;
   REL_DATA *next;
   REL_DATA *prev;
   relation_type Type;
};

/*
 * Return types for move_char, damage, greet_trigger, etc, etc
 * Added by Thoric to get rid of bugs
 */
typedef enum
{
   rNONE,           rCHAR_DIED,   rVICT_DIED,           rBOTH_DIED,
   rSPELL_FAILED,   rSTOP,        rVICT_IMMUNE = 128,   rERROR = 255
} ret_types;

/* Echo types for echo_to_all */
#define ECHOTAR_ALL	0
#define ECHOTAR_PC	1
#define ECHOTAR_IMM	2

/* Defines for extended bitvectors */
/* If you need more then 128 increase XBI by 1 (to add 32 more spots) */
#ifndef INTBITS
   #define INTBITS      32
#endif
#define XBM             31  /* extended bitmask   ( INTBITS - 1 )  */
#define RSV              5  /* right-shift value  ( sqrt(XBM+1) )  */
#define XBI              4  /* integers in an extended bitvector   */
#define MAX_BITS        XBI * INTBITS

/* Structure for extended bitvectors -- Thoric */
struct extended_bitvector
{
   int bits[XBI];
};

#include "color.h"
#include "hotboot.h"
#ifdef IMC
   #include "imc.h"
#endif

struct group_data
{
   GROUP_DATA *next, *prev;
   CHAR_DATA *leader; /* Who is the leader of this group */
   CHAR_DATA *first_char, *last_char; /* List of the characters in this group */
};

/* Structure for a morph -- Shaddai Morph structs. */
struct char_morph
{
   MORPH_DATA *morph;
   EXT_BV affected_by, no_affected_by;
   int resistant[RIS_MAX];
   int timer;             /* How much time is left */
   short ac;
   short blood;
   short stats[STAT_MAX];
   short damroll;
   short dodge;
   short hit;
   short hitroll;
   short mana;
   short move;
   short parry;
   short saving_breath;
   short saving_para_petri;
   short saving_poison_death;
   short saving_spell_staff;
   short saving_wand;
   short tumble;
};

struct morph_data
{
   MORPH_DATA *next, *prev;
   EXT_BV affected_by, no_affected_by;
   EXT_BV Class;              /* Classes not allowed to use this */
   EXT_BV race;               /* Races not allowed to use this */
   char *blood;               /* Blood added vamps only */
   char *damroll;
   char *deity;
   char *description;
   char *help;                /* What player sees for info on morph */
   char *hit;                 /* Hitpoints added */
   char *hitroll;
   char *key_words;           /* Keywords added to your name */
   char *long_desc;           /* New long_desc for player */
   char *mana;                /* Mana added not for vamps */
   char *morph_other;         /* What others see when you morph */
   char *morph_self;          /* What you see when you morph */
   char *move;                /* Move added */
   char *name;                /* Name used to polymorph into this */
   char *short_desc;          /* New short desc for player */
   char *no_skills;           /* Prevented Skills */
   char *skills;
   char *unmorph_other;       /* What others see when you unmorph */
   char *unmorph_self;        /* What you see when you unmorph */
   int resistant[RIS_MAX];
   int defpos;                /* Default position */
   int timer;                 /* Timer for how long it lasts */
   int obj[3];                /* Object needed to morph you */
   int used;                  /* How many times has this morph been used */
   int vnum;                  /* Unique identifier */
   int level;                 /* Minimum level to use this morph */
   short ac;
   short bloodused;           /* Amount of blood morph requires Vamps only */
   short stats[STAT_MAX];
   short dayfrom;             /* Starting Day you can morph into this */
   short dayto;               /* Ending Day you can morph into this */
   short dodge;               /* Percent of dodge added IE 1 = 1% */
   short favorused;           /* Amount of favor to morph */
   short gloryused;           /* Amount of glory used to morph */
   short hpused;              /* Amount of hps used to morph */
   short manaused;            /* Amount of mana used to morph */
   short moveused;            /* Amount of move used to morph */
   short parry;               /* Percent of parry added IE 1 = 1% */
   short pkill;               /* Pkill Only, Peacefull Only or Both */
   short saving_breath;       /* Below are saving adjusted */
   short saving_para_petri;
   short saving_poison_death;
   short saving_spell_staff;
   short saving_wand;
   short sex;                 /* The sex that can morph into this */
   short timefrom;            /* Hour starting you can morph */
   short timeto;              /* Hour ending that you can morph */
   short tumble;              /* Percent of tumble added IE 1 = 1% */
   bool no_cast;              /* Can you cast a spell to morph into it */
   bool objuse[3];            /* Objects needed to morph */
};

/* Tongues / Languages structures */
struct lcnv_data
{
   LCNV_DATA *next, *prev;
   char *old;
   char *lnew;
   int olen;
   int nlen;
};

struct lang_data
{
   LANG_DATA *next, *prev;
   LCNV_DATA *first_precnv, *last_precnv;
   LCNV_DATA *first_cnv, *last_cnv;
   char *name;
   char *alphabet;
};

/* Ban Types --- Shaddai */
#define BAN_WARN  -1
#define BAN_SITE   1
#define BAN_CLASS  2
#define BAN_RACE   3

/* Site ban structure. */
struct ban_data
{
   BAN_DATA *next, *prev;
   char *name;         /* Name of site/class/race banned */
   char *note;         /* Why it was banned */
   char *ban_by;       /* Who banned this site */
   char *ban_time;     /* Time it was banned */
   int flag;           /* Class or Race number */
   int unban_date;     /* When ban expires */
   int level;
   short duration;     /* How long it is banned for */
   bool warn;          /* Echo on warn channel */
   bool prefix;        /* Use of *site */
   bool suffix;        /* Use of site* */
};

/* Time and weather stuff. */
typedef enum
{
   SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET
} sun_positions;

typedef enum
{
   SKY_CLOUDLESS, SKY_CLOUDY, SKY_RAINING, SKY_LIGHTNING
} sky_conditions;

typedef struct hour_min_sec HOUR_MIN_SEC;
struct hour_min_sec
{
   int hour;
   int min;
   int sec;
   int manual;
};

typedef struct time_info_data TIME_INFO_DATA;
struct time_info_data
{
   int year;
   short hour;
   short day;
   short wday;
   short month;
   short sunlight;
};
extern TIME_INFO_DATA time_info; /* Timeinfo.c */

struct weather_data
{
   NEIGHBOR_DATA *first_neighbor, *last_neighbor;
   char *echo;
   int echo_color;
   short   temp,   temp_vector,   climate_temp;
   short precip, precip_vector, climate_precip;
   short   wind,   wind_vector,   climate_wind;
};

struct neighbor_data
{
   NEIGHBOR_DATA *next, *prev;
   AREA_DATA *address;
   char *name;
};

struct host_data
{
   HOST_DATA *next, *prev;
   char *host;
   bool prefix, suffix;
};

/* Connected state for a channel. */
typedef enum
{
   CON_GET_NAME = -99,     CON_GET_OLD_PASSWORD,       CON_CONFIRM_NEW_NAME,
   CON_GET_NEW_PASSWORD,   CON_CONFIRM_NEW_PASSWORD,   CON_GET_NEW_SEX,
   CON_READ_MOTD,          CON_GET_NEW_RACE,           CON_GET_NEW_CLASS,
   CON_GET_WANT_ANSI,      CON_PRESS_ENTER,            CON_COPYOVER_RECOVER,
   CON_GET_PKILL,          CON_PLAYING = 0,            CON_EDITING
} connection_types;

/* Character substates */
typedef enum
{
   SUB_NONE,                   SUB_PAUSE,        SUB_PERSONAL_DESC,   SUB_BAN_DESC,
   SUB_OBJ_SHORT,              SUB_OBJ_LONG,     SUB_OBJ_EXTRA,       SUB_MOB_LONG,
   SUB_MOB_DESC,               SUB_ROOM_DESC,    SUB_ROOM_EXTRA,      SUB_ROOM_EXIT_DESC,
   SUB_WRITING_NOTE,           SUB_MPROG_EDIT,   SUB_HELP_EDIT,       SUB_PERSONAL_BIO,
   SUB_REPEATCMD,              SUB_RESTRICTED,   SUB_DEITYDESC,       SUB_MORPH_DESC,
   SUB_MORPH_HELP,             SUB_PROJ_DESC,    SUB_MAP_EDIT,        SUB_EDITING_NOTE,
   SUB_SHELP_EDIT,             SUB_CHELP_EDIT,   SUB_OBJ_DESC,
   /* timer types ONLY below this point */
   SUB_TIMER_DO_ABORT = 128,   SUB_TIMER_CANT_ABORT
} char_substates;

/* Descriptor (channel) structure. */
struct descriptor_data
{
   DESCRIPTOR_DATA *next, *prev, *snoop_by;
   CHAR_DATA *character;
   struct mccp_data *mccp; /* Mud Client Compression Protocol */
   unsigned char prevcolor;
   char *host;
   char inbuf[MIS];
   char incomm[MIL];
   char inlast[MIL];
   char *outbuf;
   char *pagebuf;
   char *pagepoint;
   char pagecmd;
   char pagecolor;
   unsigned long outsize;
   unsigned long pagesize;
   time_t outtime;
   int repeat;
   int port;
   int pagetop;
   int descriptor;
   int outtop;
   int newstate;
   short connected;
   short idle;
   short tempidle;
   short lines;
   short scrlen;
   short speed;
   bool fcommand;
   bool can_compress;
};

/* TO types for act. */
typedef enum
{
   TO_ROOM, TO_NOTVICT, TO_VICT, TO_CHAR, TO_OTHERS, TO_CANSEE
} to_types;

#define INIT_WEAPON_CONDITION    12
#define MAX_ITEM_IMPACT		 30

/* Shop types. */
struct shop_data
{
   SHOP_DATA *next, *prev;
   int keeper;
   short profit_buy;
   short profit_sell;
   short open_hour;
   short close_hour;
   bool buy_type[ITEM_TYPE_MAX];
};

struct repairshop_data
{
   REPAIR_DATA *next, *prev;
   int keeper;
   short profit_fix;
   short open_hour;
   short close_hour;
   bool fix_type[ITEM_TYPE_MAX];
};

/* Mob program structures */
struct act_prog_data
{
   struct act_prog_data *next;
   void *vo;
};

struct mob_prog_act_list
{
   MPROG_ACT_LIST *next;
   CHAR_DATA *ch;
   OBJ_DATA *obj;
   void *vo;
   char *buf;
};

struct mob_prog_data
{
   MPROG_DATA *next;
   char *arglist;
   char *comlist;
   int resetdelay;
   short type;
   bool triggered;
   bool fileprog;
};

/* Per-class stuff. */
struct class_type
{
   EXT_BV class_restriction;
   char *name;        /* Name for 'who' */
   int used;          /* How many have picked this class on creation? */
   short clistplace;  /* Where its at in the class list currently */
};

/* race dedicated stuff */
struct race_type
{
   EXT_BV affected; /* Default affect bitvectors */
   EXT_BV class_restriction;
   EXT_BV language; /* Default racial language */
   EXT_BV where_restrict; /* Lets allow people to set what places they may not wear equipment */
   char *name; /* Race name */
   char *where_name[MAX_WHERE_NAME];
   char *lodge_name[MAX_WHERE_NAME];
   int resistant[RIS_MAX];
   int used; /* How many have picked this race on creation? */
   short base_stats[STAT_MAX]; /* Starting stats */
   short max_stats[STAT_MAX]; /* Maximum stats */
   short hit;
   short mana;
   short move;
   short ac_plus;
   short minalign, alignment, maxalign;
   short minheight, maxheight;
   short minweight, maxweight;
   short hunger_mod;
   short thirst_mod;
   short race_recall;
   short rlistplace; /* Where its at in the race list currently */
   short uses; /* 0 = Nothing, 1 = Mana, 2 = Blood */
};

typedef struct member_data MEMBER_DATA;
struct member_data
{
   MEMBER_DATA *next, *prev;
   char *name;
};

typedef struct victory_data VICTORY_DATA;
struct victory_data
{
   VICTORY_DATA *next, *prev;
   char *name;
   char *vname;
   time_t vtime;
   int level;
   int vlevel;
   int vkills;
};

typedef enum
{
   CLAN_PLAIN, CLAN_NATION
} clan_types;

typedef struct clan_data CLAN_DATA;
struct clan_data
{
   CLAN_DATA *next, *prev;
   MEMBER_DATA *first_member, *last_member;
   VICTORY_DATA *first_victory, *last_victory;
   char *filename;    /* Clan filename */
   char *name;        /* Clan name */
   char *motto;       /* Clan motto */
   char *description; /* A brief description of the clan */
   char *leader;      /* Head clan leader */
   char *number1;     /* First officer */
   char *number2;     /* Second officer */
   char *badge;       /* Clan badge on who/where/to_room */
   char *leadrank;    /* Leader's rank */
   char *onerank;     /* Number One's rank */
   char *tworank;     /* Number Two's rank */
   int recall;        /* Vnum of clan's recall room */
   int race;          /* For Nations */
   short clan_type;   /* See clan type defines */
   short members;     /* Number of clan members */
};

struct council_data
{
   COUNCIL_DATA *next, *prev;
   MEMBER_DATA *first_member, *last_member;
   char *filename;    /* Council filename */
   char *name;        /* Council name */
   char *description; /* A brief description of the council */
   char *head;        /* Council head */
   char *head2;       /* Council co-head */
   char *powers;      /* Council powers */
   short members;     /* Number of council members */
};

typedef struct deity_data DEITY_DATA;
struct deity_data
{
   DEITY_DATA *next, *prev;
   MEMBER_DATA *first_worshipper, *last_worshipper;
   EXT_BV affected;
   EXT_BV Class;            /* Classes allowed to use this */
   EXT_BV race;             /* Races allowed to use this */
   char *filename;
   char *name;
   char *description;
   int resistant[RIS_MAX];
   int sex;
   int resistnum;
   int affectednum;
   int objvnum;
   int mobvnum;
   short alignment;
   short worshippers;
   short sdeityobj;
   short savatar;
   short scorpse;
   short srecall;
   short flee;
   short kill;
   short kill_magic;
   short sac;
   short bury_corpse;
   short aid_spell;
   short aid;
   short spell_aid;
   short backstab;
   short steal;
   short die;
   short dig_corpse;
};

typedef struct vote_data VOTE_DATA;
struct vote_data
{
   VOTE_DATA *next, *prev;
   char *name;
   short vote;
};

typedef struct bid_data BID_DATA;
struct bid_data
{
   BID_DATA *next, *prev;
   char *name;
   int bid;
};

typedef struct read_data READ_DATA;
struct read_data
{
   READ_DATA *next, *prev;
   char *name;
};

/* Data structure for notes. */
struct note_data
{
   NOTE_DATA *next, *prev;
   VOTE_DATA *first_vote, *last_vote;
   BID_DATA *first_bid, *last_bid;  /* Used for auctions */
   READ_DATA *first_read, *last_read;
   OBJ_DATA *obj;   /* Used for auctions */
   char *sender;
   char *to_list;
   char *subject;
   char *text;
   time_t posttime;
   int voting;
   int yesvotes;
   int novotes;
   int abstentions;
   int autowin;     /* Used for auctions */
   int sfor;        /* Used for auctions */
   bool aclosed;    /* Used for auctions */
   bool acanceled;  /* Used for auctions */
};

/* An affect. So limited... so few fields... should we add more? */
struct affect_data
{
   AFFECT_DATA *next, *prev;
   EXT_BV bitvector;
   int duration;
   int modifier;
   int type;
   short location;
   bool enchantment; /* Was this an enchantment */
};

/* A SMAUG spell */
typedef struct smaug_affect SMAUG_AFF;
struct smaug_affect
{
   SMAUG_AFF *next, *prev;
   char *duration;
   char *modifier;
   int bitvector;
   short location;
};

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/* Well known mob virtual numbers. Defined in #MOBILES. */
/*  1-2  Used by other stuff */
#define MOB_VNUM_SUPERMOB            3
/*  4    unused */
#define MOB_VNUM_ANIMATED_CORPSE     5
/*  6-79 unused */
#define MOB_VNUM_VAMPIRE	    80
#define MOB_VNUM_CITYGUARD	    81
/* 82-99 unused */

/* Pipe flags */
#define PIPE_LIT        BV00
#define PIPE_GOINGOUT   BV01
#define PIPE_FULLOFASH  BV02

/* Flags for act_string -- Shaddai */
#define STRING_NONE        0
#define STRING_IMM      BV01

/* Well known object virtual numbers. */
/*  1    Unused */
#define OBJ_VNUM_MONEY_ONE            2
#define OBJ_VNUM_MONEY_SOME           3
/*  4-9  Unused */
#define OBJ_VNUM_CORPSE_NPC          10
#define OBJ_VNUM_CORPSE_PC           11
#define OBJ_VNUM_SEVERED_HEAD        12
#define OBJ_VNUM_TORN_HEART          13
#define OBJ_VNUM_SLICED_ARM          14
#define OBJ_VNUM_SLICED_LEG          15
#define OBJ_VNUM_SPILLED_GUTS        16
#define OBJ_VNUM_BLOOD               17
#define OBJ_VNUM_BLOODSTAIN          18
#define OBJ_VNUM_SCRAPS              19
#define OBJ_VNUM_MUSHROOM            20
#define OBJ_VNUM_LIGHT_BALL          21
#define OBJ_VNUM_SPRING              22
#define OBJ_VNUM_SKIN                23
#define OBJ_VNUM_SLICE               24
#define OBJ_VNUM_SHOPPING_BAG        25
#define OBJ_VNUM_BLOODLET            26
/* 27-29 Unused */
#define OBJ_VNUM_FIRE                30
#define OBJ_VNUM_TRAP                31
#define OBJ_VNUM_PORTAL              32
#define OBJ_VNUM_BLACK_POWDER        33
/* 34-35 Unused */
#define OBJ_VNUM_WOOD                36
/* 37-38 Unused */
#define OBJ_VNUM_WOODFIRE            39
/* 40-43 Unused */
/* 44-59 Used for bodyparts */
/* 60    Used for blood fountain */
/* 61-62 Unused */
/* 63    Used by 'extradimensional portal' */
/* 64-79 Unused */
/* 80-87 Used for bodyparts */
/* 88-90 Unused */
#define OBJ_VNUM_MAP                 91
/* 92-94 Unused */
#define OBJ_VNUM_FISH                95
/* 96    Unused */
#define OBJ_VNUM_LOCKER              97
/* 98-99 Unused */

/* Old Lever/dial/switch/button/pullchain flags, kept to convert over */
#define TRIG_UP           BV00
#define TRIG_UNLOCK       BV01
#define TRIG_LOCK         BV02
#define TRIG_D_NORTH      BV03
#define TRIG_D_SOUTH      BV04
#define TRIG_D_EAST       BV05
#define TRIG_D_WEST       BV06
#define TRIG_D_UP         BV07
#define TRIG_D_DOWN       BV08
#define TRIG_D_NORTHEAST  BV09
#define TRIG_D_NORTHWEST  BV10
#define TRIG_D_SOUTHEAST  BV11
#define TRIG_D_SOUTHWEST  BV12
#define TRIG_D_SOMEWHERE  BV13
#define TRIG_DOOR         BV14
#define TRIG_CONTAINER    BV15
#define TRIG_OPEN         BV16
#define TRIG_CLOSE        BV17
#define TRIG_PASSAGE      BV18
#define TRIG_OLOAD        BV19
#define TRIG_MLOAD        BV20
#define TRIG_TELEPORT     BV21
#define TRIG_TELEPORTALL  BV22
#define TRIG_TELEPORTPLUS BV23
#define TRIG_DEATH        BV24
#define TRIG_CAST         BV25
#define TRIG_RAND4        BV26
#define TRIG_RAND6        BV27
#define TRIG_RAND10       BV28
#define TRIG_RAND11       BV29
#define TRIG_SHOWROOMDESC BV30
#define TRIG_AUTORETURN   BV31

#define TELE_SHOWDESC     BV00
#define TELE_TRANSALL     BV01
#define TELE_TRANSALLPLUS BV02

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE BV00
#define CONT_PICKPROOF BV01
#define CONT_CLOSED    BV02
#define CONT_LOCKED    BV03
#define CONT_EATKEY    BV04

/* Directions. Used for exits in rooms. */
typedef enum
{
   DIR_NORTH,     DIR_EAST,      DIR_SOUTH,     DIR_WEST,
   DIR_UP,        DIR_DOWN,      DIR_NORTHEAST, DIR_NORTHWEST,
   DIR_SOUTHEAST, DIR_SOUTHWEST, DIR_SOMEWHERE, DIR_MAX
} dir_types;

/* Use DIR_MAX for things you want to allow all the others */
#define MAX_DIR    DIR_SOUTHWEST  /* max for normal walking */
#define DIR_PORTAL DIR_SOMEWHERE  /* portal direction */

#define PT_WATER  100
#define PT_AIR    200
#define PT_EARTH  300
#define PT_FIRE   400

/*
 * Push/pull types for exits					-Thoric
 * To differentiate between the current of a river, or a strong gust of wind
 */
typedef enum
{
   /* 0 - PT_WATER */
   PULL_UNDEFINED,            PULL_VORTEX,     PULL_VACUUM,    PULL_SLIP,
   PULL_ICE,                  PULL_MYSTERIOUS,
   /* PT_WATER - PT_AIR */
   PULL_CURRENT = PT_WATER,   PULL_WAVE,       PULL_WHIRLPOOL, PULL_GEYSER,
   /* PT_AIR - PT_EARTH */
   PULL_WIND = PT_AIR,        PULL_STORM,      PULL_COLDWIND,  PULL_BREEZE,
   /* PT_EARTH - PT_FIRE */
   PULL_LANDSLIDE = PT_EARTH, PULL_SINKHOLE,   PULL_QUICKSAND, PULL_EARTHQUAKE,
   /* PT_FIRE (and higher) */
   PULL_LAVA = PT_FIRE,       PULL_HOTAIR
} dir_pulltypes;

/* Conditions. */
typedef enum
{
   COND_DRUNK, COND_FULL, COND_THIRST, MAX_CONDS
} conditions;

typedef enum
{
   TIMER_NONE,      TIMER_RECENTFIGHT,   TIMER_SHOVEDRAG,    TIMER_DO_FUN,
   TIMER_APPLIED,   TIMER_PKILLED,       TIMER_ASUPPRESSED
} timer_types;

struct timer_data
{
   TIMER *next, *prev;
   DO_FUN *do_fun;
   int value;
   int count;
   short type;
};

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data
{
   MOB_INDEX_DATA *next;
   MOB_INDEX_DATA *next_amob, *prev_amob;
   AREA_DATA *area;
   CHAR_DATA *first_copy, *last_copy;
   RESET_TRACK_DATA *first_track, *last_track;
   SPEC_FUN *spec_fun;
   SHOP_DATA *pShop;
   REPAIR_DATA *rShop;
   MPROG_DATA *mudprogs;
   EXT_BV progtypes;
   EXT_BV act;
   EXT_BV affected_by;
   EXT_BV xflags;
   EXT_BV attacks;
   EXT_BV defenses;
   EXT_BV speaks, speaking;
   char *name;
   char *short_descr;
   char *long_descr;
   char *description;
   char *spec_funname;
   int resistant[RIS_MAX]; /* How does it affect resistances */
   int avnum; /* The area vnum this mobile belongs to */
   int vnum;
   int gold;
   int minhit;
   int maxhit;
   int level;
   short sex;
   short alignment;
   short ac;
   short numattacks;
   short defposition;
   short perm_stats[STAT_MAX];
   short saving_poison_death;
   short saving_wand;
   short saving_para_petri;
   short saving_breath;
   short saving_spell_staff;
   short height;
   short weight;
   short hitroll;
   short damroll;
   short count;
   short killed;
};

struct hunt_hate_fear
{
   HHF_DATA *next, *prev; /* For the mobiles' hunts, hates and fears */
   HHF_DATA *lnext, *lprev; /* For all mobiles' hunts, hates and fears */
   CHAR_DATA *mob; /* What mobile is hunting, hating and fearing */
   CHAR_DATA *who; /* Who they are hunting, hating and fearing */
   char *name; /* Name of who they are hunting, hating and fearing so can last past log on and off */
};

struct fighting_data
{
   CHAR_DATA *who;
   short align;
   short duration;
   short timeskilled;
};

struct editor_data
{
   short numlines;
   short on_line;
   short size;
   char line[MEL][( MLS + 2 )];
};

typedef struct extracted_char_data EXTRACT_CHAR_DATA;
struct extracted_char_data
{
   EXTRACT_CHAR_DATA *next;
   CHAR_DATA *ch;
   ROOM_INDEX_DATA *room;
   ch_ret retcode;
   bool extract;
};

/*
 * One character (PC or NPC).
 * (Shouldn't most of that build interface stuff use substate, dest_buf,
 * spare_ptr and tempnum?  Seems a little redundant)
 */
struct char_data
{
   CHAR_DATA *next, *prev;
   CHAR_DATA *next_in_group, *prev_in_group;
   CHAR_DATA *next_in_room, *prev_in_room;
   CHAR_DATA *next_in_area, *prev_in_area;
   CHAR_DATA *next_index, *prev_index;
   CHAR_DATA *master, *leader;
   CHAR_DATA *next_pet, *prev_pet;
   CHAR_DATA *reply, *retell;
   CHAR_DATA *mount;
   CHAR_DATA *mounter;
   CHAR_DATA *editing;
   GROUP_DATA *group; /* What group does is this character in */
   HOST_DATA *first_host, *last_host;
   OBJ_DATA *first_carrying, *last_carrying;
   ROOM_INDEX_DATA *in_room, *was_in_room;
   PC_DATA *pcdata;
   DO_FUN *last_cmd, *prev_cmd;
   TIMER *first_timer, *last_timer;
   NOTE_DATA *pnote;
   DESCRIPTOR_DATA *desc;
   AFFECT_DATA *first_affect, *last_affect;
   MOB_INDEX_DATA *pIndexData;
   FIGHT_DATA *fighting;
   MPROG_ACT_LIST *mpact;
   EDITOR_DATA *editor;
   CHAR_MORPH *morph;
   CHAR_DATA *summoning; /* Just points to who they are/were summoning */
   HHF_DATA *first_hunting, *last_hunting;
   HHF_DATA *first_hating, *last_hating;
   HHF_DATA *first_fearing, *last_fearing;
   SPEC_FUN *spec_fun;
   EXT_BV xflags;
   EXT_BV act;
   EXT_BV affected_by, no_affected_by;
   EXT_BV attacks;
   EXT_BV defenses;
   EXT_BV deaf;
   EXT_BV speaks, speaking;
   RESET_DATA *reset;
   void *dest_buf;
   void *spare_ptr;
   char *spec_funname;
   char *alloc_ptr;
   char *name;
   char *short_descr;
   char *long_descr;
   char *description;
   time_t logon;
   time_t save_time;
   double exp; /* Not saved (for now) but used so npcs and no class pcs can gain exp */
   int resistant[RIS_MAX]; /* How does it affect resistances */
   int bsplatter; /* Blood splatter */
   int mpactnum;
   int tempnum;
   int temp_played, played;
   int questcountdown;
   int questvnum;
   int questtype;
   int questgiver;
   int gold;
   int carry_weight;
   int carry_number;
   int retran;
   int regoto;
   int hit, max_hit;
   int mana, max_mana;
   int move, max_move;
   int level;
   unsigned short mpscriptpos;
   short num_fighting;
   short substate;
   short sex;
   short race;
   short trust;
   short timer;
   short wait;
   short practice;
   short numattacks;
   short saving_poison_death;
   short saving_wand;
   short saving_para_petri;
   short saving_breath;
   short saving_spell_staff;
   short alignment;
   short hitroll;
   short damroll;
   short position, defposition;
   short style;
   short height;
   short weight;
   short armor;
   short wimpy;
   short perm_stats[STAT_MAX];
   short mod_stats[STAT_MAX];
   short mental_state; /* simplified */
   short mobinvis; /* Mobinvis level SB */
   short colors[MAX_COLORS];
   short cords[2]; /* cords[0] = x, cords[1] = y */
   bool logged; /* are they logged? */
};

struct killed_data
{
   KILLED_DATA *next, *prev;
   int vnum;
   int count;
};

/* Structure for link list of ignored players */
struct ignore_data
{
   IGNORE_DATA *next, *prev;
   char *name;
};

typedef struct per_history PER_HISTORY;
struct per_history
{
   PER_HISTORY *next, *prev;
   char *text;
   time_t chtime;
};

typedef struct friend_data FRIEND_DATA;
struct friend_data
{
   FRIEND_DATA *next, *prev;
   char *name;
   short sex;
   bool approved; /* This isn't saved but is used to see if they have permission to have them on their list */
};

/* This is for explored data */
typedef struct exp_data EXP_DATA;
struct exp_data
{
   EXP_DATA *next, *prev;
   int vnum;
};

typedef struct mclass_data MCLASS_DATA;
struct mclass_data
{
   MCLASS_DATA *next, *prev;
   double exp;     /* How much exp they have for this class */
   int level;      /* Level they are in this class */
   int wclass;     /* Which class is this one? */
   short cpercent; /* How much percent of earned exp goes to this class? */
};

/* Data which only PC's have. */
struct pc_data
{
   PC_DATA *next, *prev;
   CHAR_DATA *character;
   CHAR_DATA *first_pet, *last_pet;
   MCLASS_DATA *first_mclass, *last_mclass;
   NOTE_DATA *gnote;
   CLAN_DATA *clan;
   CLAN_DATA *nation;
   COUNCIL_DATA *council;
   AREA_DATA *area;
   DEITY_DATA *deity;
   KILLED_DATA *first_killed, *last_killed;
   FRIEND_DATA *first_friend, *last_friend;   /* Keep a list of friends */
   IGNORE_DATA *first_ignored, *last_ignored; /* Keep track of who to ignore */
   EXP_DATA *first_explored, *last_explored;  /* Keep track of where they have explored */
   PER_HISTORY *first_tell, *last_tell;
   PER_HISTORY *first_whisper, *last_whisper;
   PER_HISTORY *first_yell, *last_yell;
   PER_HISTORY *first_say, *last_say;
   PER_HISTORY *first_fchat, *last_fchat;
#ifdef IMC
   IMC_CHARDATA *imcchardata;
#endif
   EXT_BV flags;                 /* Whether the player is deadly and whatever else we add.      */
   char *channels;
   char *spouse;
   char *homepage;
   char *email;
   char *msn;
   char *yahoo;
   char *gtalk;
   char *pwd;
   char *bamfin;
   char *bamfout;
   char *filename;               /* For the safe mset name -Shaddai */
   char *rank;
   char *title;
   char *bestowments;            /* Special bestowed commands     */
   char *bio;                    /* Personal Bio */
   char *prompt;                 /* User config prompts */
   char *fprompt;                /* Fight prompts */
   char *subprompt;              /* Substate prompt */
   char sudoku[9][9], dis_sudoku[9][9]; /* for the sudoku puzzles */
   time_t news_read;
   time_t restore_time;          /* The last time the char did a restore all */
   time_t sstarttime, sfastesttime, sslowesttime, slasttime;
   unsigned int swins, squits;   /* Sudoku wins and quits */
   unsigned int rwins, rquits;   /* Rubiks completed and quit */
   unsigned int pkills;          /* Number of pkills */
   unsigned int pdeaths;         /* Number of times pkilled (legally)  */
   unsigned int mkills;          /* Number of mobs killed */
   unsigned int mdeaths;         /* Number of deaths due to mobs */
   unsigned int questcompleted;  /* Number of quest completed */
   int nextregen;                /* When do they get another regen */
   int quest_curr;               /* Current number of quest points */
   int range_lo, range_hi;       /* Vnum range */
   int birth_year;
   int wizinvis;                 /* wizinvis level */
   int min_snoop;                /* minimum snoop level */
   int onboard;
   short rubik[54];              /* For the rubik's cube */
   short speed;
   short condition[MAX_CONDS];
   short learned[MAX_SKILL];
   short personal[MAX_PC_PERS];  /* This is to keep a quick list of the personal skills/spells each person has (Not saved, set on login) */
   short favor;                  /* deity favor */
   short charmies;               /* Number of Charmies */
   short pagerlen;               /* For pager length */
   short kltime;                 /* For the keep alive timer */
   short birth_month;
   short birth_day;
   bool hotboot;                 /* hotboot tracker */
};

/* Liquids. */
#define LIQ_WATER   1
#define LIQ_BLOOD  14
#define LIQ_MAX    19

struct liq_type
{
   char *liq_name;
   char *liq_color;
   short liq_affect[MAX_CONDS];
};

/* Damage types from the attack_table[] */
typedef enum
{
   DAM_HIT,     DAM_SLICE,   DAM_STAB,     DAM_SLASH,
   DAM_WHIP,    DAM_CLAW,    DAM_BLAST,    DAM_POUND,
   DAM_CRUSH,   DAM_BITE,    DAM_PIERCE,   DAM_SUCTION,
   DAM_BOLT,    DAM_ARROW,   DAM_DART,     DAM_STONE,
   DAM_PEA,     DAM_MAX
} damage_types;

/* Extra description data for a room or object. */
struct extra_descr_data
{
   EXTRA_DESCR_DATA *next, *prev;
   char *keyword;                 /* Keyword in look/examine */
   char *description;             /* What to see */
};

/* Prototype for an object. */
struct obj_index_data
{
   OBJ_INDEX_DATA *next;
   OBJ_INDEX_DATA *next_aobj, *prev_aobj;
   EXTRA_DESCR_DATA *first_extradesc, *last_extradesc;
   AFFECT_DATA *first_affect, *last_affect;
   RESET_TRACK_DATA *first_track, *last_track;
   OBJ_DATA *first_copy, *last_copy; /* A list of all the copys of this index in game */
   MPROG_DATA *mudprogs; /* objprogs */
   AREA_DATA *area;
   EXT_BV progtypes; /* objprogs */
   EXT_BV extra_flags;
   EXT_BV wear_flags;
   EXT_BV class_restrict; /* Classes that can't wear it */
   EXT_BV race_restrict; /* Races that can't wear it */
   char *name;
   char *short_descr; /* Short description for the object */
   char *description; /* Long description for the object */
   char *action_desc;
   char *desc; /* Description for the object */
   int avnum; /* The area vnum this object belongs to */
   int vnum;
   int cost;
   int value[6];
   int level;
   short stat_reqs[STAT_MAX];
   short item_type;
   short count;
   short weight;
   short layers;
};

/* One object. */
struct obj_data
{
   OBJ_DATA *next, *prev;
   OBJ_DATA *next_corpse, *prev_corpse;
   OBJ_DATA *next_content, *prev_content;
   OBJ_DATA *first_content, *last_content;
   OBJ_DATA *next_index, *prev_index;
   OBJ_DATA *in_obj;
   CHAR_DATA *carried_by;
   EXTRA_DESCR_DATA *first_extradesc, *last_extradesc;
   AFFECT_DATA *first_affect, *last_affect;
   OBJ_INDEX_DATA *pIndexData;
   ROOM_INDEX_DATA *in_room;
   EXT_BV extra_flags;
   EXT_BV wear_flags;
   EXT_BV class_restrict; /* Classes that can't wear it */
   EXT_BV race_restrict;  /* Races that can't wear it */
   MPROG_ACT_LIST *mpact;  /* mudprogs */
   RESET_DATA *reset;
   char *name;
   char *short_descr; /* Short description for the object */
   char *description; /* Long description for the object */
   char *desc; /* Description for the object */
   char *action_desc;
   char *owner;
   int mpactnum;  /* mudprogs */
   int cost;
   int value[6];
   int room_vnum; /* hotboot tracker */
   int level;
   int bsplatter; /* How much blood has been splattered on this eq */
   int bstain;    /* How much blood has left a stain on this eq */
   short timer;
   short count;   /* support for object grouping */
   short wear_loc;
   short t_wear_loc; /* temp wear loc used for saving objects wear loc */
   short weight;
   short stat_reqs[STAT_MAX];
   short item_type;
   short mpscriptpos;
   short cords[2]; /* 0 = x, 1 = y */
   bool auctioned; /* Item is being auctioned */
};

/* Exit data. */
struct exit_data
{
   EXIT_DATA *next, *prev;
   EXIT_DATA *rexit;         /* Reverse exit pointer */
   ROOM_INDEX_DATA *to_room; /* Pointer to destination room */
   EXT_BV exit_info;         /* door states & other flags */
   EXT_BV base_info;         /* Base door states & other flags, since lots of things changes exit_info flags */
   char *keyword;            /* Keywords for exit or door */
   char *description;        /* Description of exit */
   int vnum;                 /* Vnum of room exit leads to */
   int rvnum;                /* Vnum of room in opposite dir */
   int key;                  /* Key vnum */
   short vdir;               /* Physical "direction" */
   short pull;               /* pull of direction (current) */
   short pulltype;           /* type of pull (current, wind) */
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'H': hide an object
 *   'T': trap an object
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/* Room - reset definition. */
struct reset_data
{
   RESET_DATA *main_reset; /* Not saved just used in game to point to the main one for like P G and E */
   RESET_DATA *next, *prev;
   RESET_DATA *first_reset, *last_reset;
   RESET_DATA *next_reset, *prev_reset;
   CHAR_DATA *ch;
   OBJ_DATA *obj;
   char command;
   int extra;
   int arg1;
   int arg2;
   int arg3;
   short rchance; /* 1-100% chance of it reseting */
   short cords[2]; /* X = 0, Y = 1 */
   bool wilderness; /* Should this use cords */
};

/* Keep track of all the resets assigned to the object/mobile */
struct reset_track_data
{
   RESET_TRACK_DATA *next, *prev;
   RESET_DATA *reset; /* What reset is it pointing too */
};

/* Area flags */
typedef enum
{
   AREA_NONE,   AREA_DELETED,   AREA_LOADED
} area_flag;

/* Area definition. */
struct area_data
{
   AREA_DATA *next, *prev;
   AREA_DATA *next_sort, *prev_sort;
   AREA_DATA *next_sort_name, *prev_sort_name;
   ROOM_INDEX_DATA *first_room, *last_room;
   OBJ_INDEX_DATA *first_obj, *last_obj;
   MOB_INDEX_DATA *first_mob, *last_mob;
   CHAR_DATA *first_person, *last_person;
   WEATHER_DATA *weather;
   EXT_BV flags;
   char *name;
   char *filename;
   char *author;  /* Scryn */
   char *resetmsg;   /* Rennard */
   int vnum; /* New area vnum */
   int top_room;
   int top_obj;
   int top_mob;
   int low_vnum, hi_vnum;
   int low_soft_range, hi_soft_range;
   int low_hard_range, hi_hard_range;
   short status;
   short age;
   short nplayer;
   short reset_frequency;
   short max_players;
};

/* Used to keep track of system settings and statistics -Thoric */
struct system_data
{
   EXT_BV save_flags;            /* Toggles for saving conditions */
   void *dlHandle;
   char *mud_name;               /* Name of mud */
   time_t time_of_max;           /* Time of max ever */
   time_t next_pfile_cleanup;    /* Time of next pfile cleaning */
   int maxplayers;               /* Maximum players this boot */
   int alltimemax;               /* Maximum players ever */
   int room_limbo;
   int room_poly;
   int room_authstart;
   int room_deadly;
   int room_school;
   int room_temple;
   int room_altar;
   int groupleveldiff;           /* Max level difference allowed to group */
   short perm_muse;              /* Permission of muse channel */
   short perm_think;             /* Permission of think channel */
   short perm_build;             /* Permission of build channel */
   short perm_log;               /* Permission of log channel */
   short perm_modify_proto;      /* Permission to modify prototype stuff */
   short perm_override_private;  /* override private flag */
   short perm_mset_player;       /* Permission to mset a player */
   short perm_getobjnotake;      /* Get objects without take flag */
   short perm_forcepc;           /* The permission at which you can use force on players. */
   short bestow_dif;             /* Max # of levels between trust and command level for a bestow to work --Blodkai */
   short save_frequency;         /* How old to autosave someone */
   short ban_site, ban_class, ban_race; /* Levels to ban sites/classes/races */
   short mlimit_total;           /* Max Limit for total connections from 1 host */
   short mlimit_deadly;          /* Max Limit for deadly characters from 1 host */
   short mlimit_peaceful;        /* Max Limit for peaceful characters from 1 host */
   short maxpet;                 /* Max number of pets for each character */
   short maxkillhistory;         /* Max number of killes to be tracked */
   short expmulti;               /* Exp Multiplier for everyone */
   short mclass;                 /* How many classes can they be? */
   short version_major;
   short version_minor;
   short maxauction;             /* How many things can each person have on auction */
   bool NAME_RESOLVING;          /* Hostnames get resolved */
   bool DENY_NEW_PLAYERS;        /* New players can't connect */
   bool WAIT_FOR_AUTH;           /* New players must be auth'ed */
   bool save_pets;               /* Do pets save? */
   bool pk_loot;                 /* Pkill looting allowed? */
   bool wizlock;                 /* Game is wizlocked */
   bool skipclasses;             /* Skip picking classes on character creation */
   bool autosavecset;            /* Save cset when a change is made? */
   bool autosavecommands;        /* Save commands when a change is made? */
   bool autosavesocials;         /* Save socials when a change is made? */
   bool autosaveskills;          /* Save skills when a change is made? */
   bool autosavehelps;           /* Save helps when a change is made? */
   bool morph_opt;               /* Save morphs when a change is made? */
   bool groupall;                /* If false, pkills can only party with pkills */
};

/* Room type. */
struct room_index_data
{
   ROOM_INDEX_DATA *next;
   ROOM_INDEX_DATA *next_aroom, *prev_aroom;
   RESET_DATA *first_reset, *last_reset;
   RESET_DATA *last_mob_reset;
   RESET_DATA *last_obj_reset;
   CHAR_DATA *first_person, *last_person;
   OBJ_DATA *first_content, *last_content;
   EXTRA_DESCR_DATA *first_extradesc, *last_extradesc;
   AREA_DATA *area;
   EXIT_DATA *first_exit, *last_exit;
   MPROG_ACT_LIST *mpact;  /* mudprogs */
   EXT_BV room_flags;
   EXT_BV progtypes; /* mudprogs */
   MPROG_DATA *mudprogs;   /* mudprogs */
   char *name;
   char *description;
   int mpactnum;  /* mudprogs */
   int sector_type;
   int tele_vnum;
   int avnum; /* The area vnum this room belongs to */
   int vnum;
   int charcount; /* Amount of characters in room */
   int objcount; /* Amount of objects in room */
   short mpscriptpos;
   short light;   /* amount of light in the room */
   short tele_delay;
   short tunnel;  /* max people that will fit */
};

/*
 * Types of skill numbers.  Used to keep separate lists of sn's
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED    -1
/*    0 -  999 = spells/skills/weapons/tongues */
#define TYPE_HIT        1000
/* 1000 - 1999 = attack types */
#define TYPE_HERB       2000
/* 2000 - 2999 = herb types */
#define TYPE_PERS       3000
/* 3000 +      = personal types */

typedef enum
{
   SKILL_UNKNOWN,   SKILL_SPELL,      SKILL_SKILL,    SKILL_WEAPON,   SKILL_TONGUE,
   SKILL_HERB,      SKILL_PERSONAL,   SKILL_DELETED
} skill_types;

struct timerset
{
   int num_uses;
   struct timeval total_time;
   struct timeval min_time;
   struct timeval max_time;
};

/* Skills include spells as a particular case. */
struct skill_type
{
   EXT_BV flags;
   EXT_BV spell_sector;          /* Sectors Spell will work in */
   SMAUG_AFF *first_affect, *last_affect;
   SPELL_FUN *spell_fun;         /* Spell pointer (for spells) */
   char *spell_fun_name;         /* Spell function name - Trax */
   DO_FUN *skill_fun;            /* Skill pointer (for skills) */
   char *skill_fun_name;         /* Skill function name - Trax */
   char *name;
   char *noun_damage;            /* Damage message */
   char *msg_off;                /* Wear off message */
   char *hit_char;               /* Success message to caster */
   char *hit_vict;               /* Success message to victim */
   char *hit_room;               /* Success message to room */
   char *hit_dest;               /* Success message to dest room */
   char *miss_char;              /* Failure message to caster */
   char *miss_vict;              /* Failure message to victim */
   char *miss_room;              /* Failure message to room */
   char *die_char;               /* Victim death msg to caster */
   char *die_vict;               /* Victim death msg to victim */
   char *die_room;               /* Victim death msg to room */
   char *imm_char;               /* Victim immune msg to caster */
   char *imm_vict;               /* Victim immune msg to victim */
   char *imm_room;               /* Victim immune msg to room */
   char *abs_char;               /* Victim absorb msg to caster */
   char *abs_vict;               /* Victim absorb msg to victim */
   char *abs_room;               /* Victim absorb msg to room */
   char *dice;                   /* Dice roll */
   char *components;             /* Spell components, if any */
   char *teachers;               /* Skill requires a special teacher */
   char *htext;                  /* Skill help text */
   char *reqskillname;           /* Used on startup to save req_skill names to check after loading them all */
   int value;                    /* Misc value */
   int type;                     /* Spell/Skill/Weapon/Tongue */
   short participants;           /* # of required participants */
   short saves;                  /* What saving spell applies */
   short difficulty;             /* Difficulty of casting/learning */
   short range;                  /* Range of spell (rooms) */
   short damage;
   short Class;
   short action;
   short power;
   short save;
   int skill_level[MAX_CLASS];
   short skill_adept[MAX_CLASS];
   int race_level[MAX_RACE];
   short race_adept[MAX_RACE];
   short target;                 /* Legal targets */
   short minimum_position;       /* Position for caster / user */
   short slot;                   /* Slot for #OBJECT loading */
   short req_skill;              /* Slot for required skill to have to learn this one */
   short min_mana;               /* Minimum mana used */
   short beats;                  /* Rounds required to use skill */
   short stats[STAT_MAX];        /* Required stats to use this skill */
   struct timerset userec;       /* Usage record */
   bool tmpspell;                /* Used by personals to decide if it is a spell/skill */
   bool magical;                 /* Is this a magical spell/skill? */
};

/* Utility macros. */
int umin( int check, int ncheck );
int umax( int check, int ncheck );
int urange( int mincheck, int check, int maxcheck );
double dumin( double check, double ncheck );
double dumax( double check, double ncheck );
double durange( double mincheck, double check, double maxcheck );
char lower( char c );
char upper( char c );

#define UMIN( a, b )      ( umin( (a), (b) ) )
#define UMAX( a, b )      ( umax( (a), (b) ) )
#define URANGE(a, b, c )  ( urange( (a), (b), (c) ) )
#define LOWER( c )        ( lower( (c) ) )
#define UPPER( c )        ( upper( (c) ) )

/*
 * Old-style Bit manipulation macros
 * The bit passed is the actual value of the bit ( Use the BV## defines )
 */
#define IS_SET( flag, bit )	( (flag) & (bit) )
#define SET_BIT( var, bit )	( (var) |= (bit) )
#define REMOVE_BIT( var, bit )	( (var) &= ~(bit) )
#define TOGGLE_BIT( var, bit )	( (var) ^= (bit) )

/*
 * Macros for accessing virtually unlimited bitvectors.		-Thoric
 *
 * Note that these macros use the bit number rather than the bit value
 * itself -- which means that you can only access _one_ bit at a time
 *
 * This code uses an array of integers
 */

/*
 * The functions for these prototypes can be found in misc.c
 * They are up here because they are used by the macros below
 */
bool ext_is_empty( EXT_BV *bits );
void ext_clear_bits( EXT_BV *bits );
int ext_has_bits( EXT_BV *var, EXT_BV *bits );
bool ext_has_all_bits( EXT_BV *var, EXT_BV *bits );
bool ext_same_bits( EXT_BV *var, EXT_BV *bits );
void ext_set_bits( EXT_BV *var, EXT_BV *bits );
void ext_remove_bits( EXT_BV *var, EXT_BV *bits );
void ext_toggle_bits( EXT_BV *var, EXT_BV *bits );

/* Here are the extended bitvector macros: */
#define xIS_SET( var, bit )       ( (var).bits[(bit) >> RSV] & 1 << ( (bit) & XBM ) )
#define xSET_BIT( var, bit )      ( (var).bits[(bit) >> RSV] |= 1 << ( (bit) & XBM ) )
#define xSET_BITS( var, bit )     ( ext_set_bits( &(var), &(bit)))
#define xREMOVE_BIT( var, bit )   ( (var).bits[(bit) >> RSV] &= ~( 1 << ( (bit) & XBM ) ) )
#define xREMOVE_BITS( var, bit )  ( ext_remove_bits( &(var), &(bit) ) )
#define xTOGGLE_BIT( var, bit )   ( (var).bits[ (bit) >> RSV] ^= 1 << ( (bit) & XBM ) )
#define xTOGGLE_BITS( var, bit )  ( ext_toggle_bits( &(var), &(bit) ) )
#define xCLEAR_BITS( var )        ( ext_clear_bits( &(var) ) )
#define xIS_EMPTY( var )          ( ext_is_empty( &(var) ) )
#define xHAS_BITS( var, bit )     ( ext_has_bits( &(var), &(bit) ) )
#define xHAS_ALL_BITS( var, bit ) ( ext_has_all_bits( &(var), &(bit) ) )
#define xSAME_BITS( var, bit )    ( ext_same_bits( &(var), &(bit) ) )

/* Memory allocation macros. */
#define CREATE(result, type, number) \
do \
{ \
   if( !( (result) = (type *)calloc( (number), sizeof(type) ) ) ) \
   { \
      perror( "calloc failure" ); \
      fprintf( stderr, "Calloc failure @ %s:%d\n", __FILE__, __LINE__ ); \
      abort(); \
   } \
} while(0)

#define RECREATE(result, type, number) \
do \
{ \
   if( !( (result) = (type *)realloc( (result), sizeof(type) * (number) ) ) ) \
   { \
      perror( "realloc failure" ); \
      fprintf( stderr, "Realloc failure @ %s:%d\n", __FILE__, __LINE__ ); \
      abort(); \
   } \
} while(0)

/* Use this on normal things that aren't strings */
#define DISPOSE(point) \
do \
{ \
   if( (point) ) \
      free( (point) ); \
   (point) = NULL; \
} while( 0 )

/* This one is to use on strings that you wish to dispose of */
#define STRDISPOSE(point) \
do \
{ \
   if( (point) ) \
   { \
      if( typeid( (point) ) == typeid(char*) && in_hash_table( (const char *)(point) ) ) \
      { \
         log_printf( "&RSTRDISPOSE called on STRALLOC pointer: %s, line %d&D\n", __FILE__, __LINE__ ); \
         str_free( (char*)(point), __FILE__, __LINE__ ); \
      } \
      else \
         free( (point) ); \
   } \
   (point) = NULL; \
} while(0)

#ifdef HASHSTR

#define STRALLOC(point)  str_alloc( (point), __FILE__, __LINE__ )
#define QUICKLINK(point) quick_link( (point), __FILE__, __LINE__ )

#define STRFREE(point) \
do \
{ \
   if( (point) ) \
   { \
      if( !in_hash_table( (point) ) ) \
      { \
         log_printf( "&RSTRFREE called on str_dup pointer: %s, line %d&D\n", __FILE__, __LINE__ ); \
         free( (point) ); \
      } \
      else \
         str_free( (point), __FILE__, __LINE__ ); \
   } \
   (point) = NULL; \
} while(0)

#else

#define STRALLOC( point )  str_dup( (point) )
#define QUICKLINK( point ) str_dup( (point) )
#define STRFREE( point )   STRDISPOSE( (point) )

#endif

#define STRSET( point, new ) \
do \
{ \
   STRFREE( (point) ); \
   (point) = STRALLOC( (new) ); \
} while(0)

/* Used to link stuff into a hash list */
#define HASH_LINK(link, hashlist, tmp, prev, insert) \
do \
{ \
   int hash = ( LOWER( (link)->name[0] ) % 126 ); \
 \
   if( !( (prev) = (tmp) = (hashlist)[hash] ) ) \
   { \
      (link)->next = (hashlist)[hash]; \
      (hashlist)[hash] = (link); \
   } \
   else \
   { \
      for( ; (tmp); (tmp) = (tmp)->next ) \
      { \
         if( !(tmp)->next ) \
         { \
            (tmp)->next = (link); \
            (link)->next = NULL; \
            break; \
         } \
         else if( (insert) ) \
         { \
            if( (tmp) == (hashlist)[hash] \
            && strcmp( (link)->name, (tmp)->name ) < 0 ) \
            { \
               (link)->next = (hashlist)[hash]; \
               (hashlist)[hash] = (link); \
               break; \
            } \
            if( strcmp( (link)->name, (tmp)->next->name ) < 0 ) \
            { \
               (link)->next = (tmp)->next; \
               (tmp)->next = (link); \
               break; \
            } \
         } \
      } \
   } \
} while(0)

/* Used to unlink stuff into a hash list */
#define HASH_UNLINK(link, hashlist, tmp) \
do \
{ \
   int hash = ( LOWER( (link)->name[0] ) % 126 ); \
 \
   if( (link) == ( (tmp) = (hashlist)[hash] ) ) \
      (hashlist)[hash] = (tmp)->next; \
   else \
      for( ; (tmp); (tmp) = (tmp)->next ) \
         if( (link) == (tmp)->next ) \
         { \
            (tmp)->next = (tmp)->next->next; \
            break; \
         } \
} while(0)

/* double-linked list handling macros -Thoric */
/* Updated by Scion 8/6/1999 */
#define LINK(link, first, last, next, prev) \
do \
{ \
   if( !(first) ) \
   { \
      (first) = (link); \
      (last) = (link); \
   } \
   else \
      (last)->next = (link); \
   (link)->next = NULL; \
   if ((first) == (link)) \
      (link)->prev = NULL; \
   else \
      (link)->prev = (last);  \
   (last) = (link); \
} while(0)

#define INSERT(link, insert, first, next, prev) \
do \
{ \
   (link)->prev = (insert)->prev; \
   if ( !(insert)->prev ) \
      (first) = (link); \
   else \
      (insert)->prev->next = (link); \
   (insert)->prev = (link); \
   (link)->next = (insert); \
} while(0)

#define UNLINK(link, first, last, next, prev) \
do \
{ \
   if ( !(link)->prev ) \
   { \
      (first) = (link)->next; \
      if( (first) ) \
         (first)->prev = NULL; \
   } \
   else \
      (link)->prev->next = (link)->next; \
 \
   if( !(link)->next ) \
   { \
      (last) = (link)->prev; \
      if( (last) ) \
         (last)->next = NULL; \
   } \
   else \
      (link)->next->prev = (link)->prev; \
} while(0)

/* Character macros. */
#define IS_AFFECTED( ch, sn )    ( xIS_SET( (ch)->affected_by, (sn) ) )
#define HAS_BODYPART( ch, part ) ( xIS_EMPTY( (ch)->xflags ) || xIS_SET( (ch)->xflags, (part) ) )

/* MudProg macros. -Thoric */
#define HAS_PROG( what, prog )  ( xIS_SET( (what)->progtypes, (prog) ) )

/* Description macros. */
char *pers( CHAR_DATA *ch, CHAR_DATA *looker );
#define PERS( ch, looker )      ( pers( (ch), (looker) ) )

char *morphpers( CHAR_DATA *ch, CHAR_DATA *looker );
#define MORPHPERS( ch, looker ) ( morphpers( (ch), (looker) ) )

/* Structure for a command in the command lookup table. */
struct cmd_type
{
   CMDTYPE *next;
   DO_FUN *do_fun;
   EXT_BV flags;
   struct timerset userec;
   char *name;
   char *fun_name;
   char *htext;
   int lag_count; /* count lag flags for this cmd - FB */
   short position;
   short perm;
   short group; /* what group does this command go with if any? */
   short log;
};

/* Structure for a social in the socials table. */
struct social_type
{
   SOCIALTYPE *next;
   char *name;
   char *char_no_arg;
   char *others_no_arg;
   char *char_found;
   char *others_found;
   char *vict_found;
   char *char_auto;
   char *others_auto;
};

/*
 * Spell functions.
 * Defined in magic.c.
 */
DECLARE_SPELL_FUN( spell_null );
DECLARE_SPELL_FUN( spell_notfound );
DECLARE_SPELL_FUN( spell_dispel_evil );
DECLARE_SPELL_FUN( spell_dispel_magic );
DECLARE_SPELL_FUN( spell_earthquake );
DECLARE_SPELL_FUN( spell_energy_drain );
DECLARE_SPELL_FUN( spell_smaug );
DECLARE_SPELL_FUN( spell_teleport );

#include "dfiles.h"

/* object saving defines for fread/write_obj. -- Altrag */
#define OS_CARRY   0
#define OS_CORPSE  1
#define OS_AUCTION 2
#define OS_LOCKER  3

/* mud prog defines */
#define ERROR_PROG    -1
#define IN_FILE_PROG  -2

#define LEARNED( ch, sn )   ( is_npc( (ch) ) ? 80 : URANGE( 0, ch->pcdata->learned[sn], 100 ) )

/* Act_comm.c */
CMDF( do_emote );
CMDF( do_quit );
LANG_DATA *get_lang( const char *name );
char *drunk_speech( const char *argument, CHAR_DATA *ch );
char *translate( int percent, const char *in, const char *name );
void free_phistory( PER_HISTORY *phistory );
void add_phistory( int type, CHAR_DATA *ch, char *argument );
void fwrite_phistory( CHAR_DATA *ch, FILE *fp );
void fread_phistory( CHAR_DATA *ch, FILE *fp, int type );
void send_ansi_title( CHAR_DATA *ch );
void send_ascii_title( CHAR_DATA *ch );
void add_follower( CHAR_DATA *ch, CHAR_DATA *master );
void stop_follower( CHAR_DATA *ch );
void die_follower( CHAR_DATA *ch );
void remove_char_from_group( CHAR_DATA *ch );
void free_all_groups( void );
int knows_language( CHAR_DATA *ch, int language );
bool circle_follow( CHAR_DATA *ch, CHAR_DATA *victim );
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch );
/*------------*/

/* Act_info.c */
CMDF( do_look );
CMDF( do_exits );
CMDF( do_wimpy );
char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort );
char *double_punct( double foo );
char *num_punct( int foo );
const char *show_weather( AREA_DATA *area );
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing );
void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch );
void show_condition( CHAR_DATA *ch, CHAR_DATA *victim );
int get_door( char *arg );
bool is_ignoring( CHAR_DATA *ch, CHAR_DATA *ign_ch );
/*------------*/

/* Act_move.c */
extern bool sneaking_char;
CMDF( do_sit );
CMDF( do_sleep );
EXIT_DATA *get_exit( ROOM_INDEX_DATA *room, short dir );
EXIT_DATA *get_exit_to( ROOM_INDEX_DATA *room, short dir, int vnum );
EXIT_DATA *get_exit_num( ROOM_INDEX_DATA *room, short count );
EXIT_DATA *find_door( CHAR_DATA *ch, char *arg, bool quiet );
extern const short movement_loss[SECT_MAX];
extern const char *dir_name[];
extern const int trap_door[];
extern const short rev_dir[];
const char *rev_exit( short vdir );
void teleport( CHAR_DATA *ch, int room, int flags );
ch_ret move_char( CHAR_DATA *ch, EXIT_DATA *pexit, int fall, bool running );
ch_ret pullcheck( CHAR_DATA *ch, int pulse );
short encumbrance( CHAR_DATA *ch, short move );
bool will_fall( CHAR_DATA *ch, int fall );
/*------------*/

/* Act_obj.c */
void show_obj( CHAR_DATA *ch, OBJ_DATA *obj );
void damage_obj( OBJ_DATA *obj );
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, short wear_bit );
void obj_fall( OBJ_DATA *obj, bool through );
void randomize_obj( OBJ_DATA *obj );
short get_obj_resistance( OBJ_DATA *obj );
/*------------*/

/* Act_wiz.c */
CMDF( do_at );
CMDF( do_rstat );
CMDF( do_ostat );
CMDF( do_mstat );
CMDF( do_restoretime );
ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg );
void close_area( AREA_DATA *pArea );
void close_all_areas( void );
void transfer_char( CHAR_DATA *ch, CHAR_DATA *victim, ROOM_INDEX_DATA *location );
void echo_to_all( short AT_COLOR, const char *argument, short tar );
void echo_to_all_printf( short AT_COLOR, short tar, const char *fmt, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
bool check_area_conflict( AREA_DATA *area, int low_range, int hi_range );
bool rename_character( CHAR_DATA *ch, char *newname );
/*------------*/

/* Affects.c */
AFFECT_DATA *fread_chaffect( FILE *fp, int atype, const char *filename, int line );
void showaffect( CHAR_DATA *ch, AFFECT_DATA *paf, bool extdisplay );
void fwrite_objaffects( FILE *fp, AFFECT_DATA *paf );
void fwrite_chaffect( FILE *fp, AFFECT_DATA *paf );
void fwrite_oiaffect( FILE *fp, AFFECT_DATA *paf );
/*------------*/

/* Areas.c */
bool fold_area( AREA_DATA *tarea, char *filename, bool install );
/*------------*/

/* Auth.c */
void free_all_auths( void );
void save_auths( void );
void load_auths( void );
void add_to_auth( char *name );
void auth_update( void );
bool not_authorized( CHAR_DATA *ch );
/*------------*/

/* Ban.c */
extern BAN_DATA *first_ban, *last_ban;
extern BAN_DATA *first_ban_class, *last_ban_class;
extern BAN_DATA *first_ban_race, *last_ban_race;
void load_banlist( void );
void save_banlist( void );
void dispose_ban( BAN_DATA *pban, int type );
void free_bans( void );
int add_ban( CHAR_DATA *ch, char *arg1, char *arg2, int btime, int type );
bool check_race_bans( int iRace );
bool check_class_bans( int iClass );
bool check_total_bans( DESCRIPTOR_DATA *d );
bool check_expire( BAN_DATA *pban );
bool check_bans( CHAR_DATA *ch, int type );
/*------------*/

/* Bank.c */
char *show_char_gold( CHAR_DATA *ch );
void setmaxdoublegold( void );
void set_gold( CHAR_DATA *ch, int amount );
void decrease_gold( CHAR_DATA *ch, int amount );
void free_all_banks( void );
void load_banks( void );
void give_interest( void );
bool increase_gold( CHAR_DATA *ch, int amount );
bool has_gold( CHAR_DATA *ch, int amount );
bool can_hold_gold( CHAR_DATA *ch, int amount );
/*------------*/

/* Boards.c */
void note_attach( CHAR_DATA *ch );
void show_unread_notes( CHAR_DATA *ch );
void free_note( NOTE_DATA *pnote );
void do_note( CHAR_DATA *ch, char *arg_passed, bool IS_MAIL );
void load_boards( void );
void check_auction( CHAR_DATA *ch );
void free_boards( void );
/*------------*/

/* Bti.c */
void free_all_bti( void );
void load_bti( void );
/*------------*/

/* Build.c */
extern REL_DATA *first_relation, *last_relation;
CMDF( do_goto );
EXTRA_DESCR_DATA *SetRExtra( ROOM_INDEX_DATA *room, char *keywords );
EXTRA_DESCR_DATA *SetOExtra( OBJ_DATA *obj, char *keywords );
EXTRA_DESCR_DATA *SetOExtraProto( OBJ_INDEX_DATA *obj, char *keywords );
extern const char *ex_pmisc[];
extern const char *ex_pwater[];
extern const char *ex_pair[];
extern const char *ex_pearth[];
extern const char *ex_pfire[];
extern const char *trig_flags[];
extern const char *cont_flags[];
char *flag_string( int bitvector, const char *flagarray[] );
char *ext_class_string( EXT_BV *bitvector );
char *ext_race_string( EXT_BV *bitvector );
char *ext_flag_string( EXT_BV *bitvector, const char *flagarray[] );
char *strip_cr( char *str );
char *copy_buffer( CHAR_DATA *ch );
void stop_editing( CHAR_DATA *ch );
void start_editing( CHAR_DATA *ch, char *data );
void edit_buffer( CHAR_DATA *ch, char *argument );
void assign_area( CHAR_DATA *ch );
void RelCreate( relation_type tp, void *actor, void *subject );
void RelDestroy( relation_type tp, void *actor, void *subject );
void remove_from_everything( const char *name );
int get_dir( char *txt );
int get_trigflag( char *flag );
int get_pc_class( char *Class );
int get_pc_race( char *type );
int get_pulltype( char *type );
bool DelRExtra( ROOM_INDEX_DATA *room, char *keywords );
bool DelOExtra( OBJ_DATA *obj, char *keywords );
bool DelOExtraProto( OBJ_INDEX_DATA *obj, char *keywords );
bool can_rmodify( CHAR_DATA *ch, ROOM_INDEX_DATA *room );
bool can_medit( CHAR_DATA *ch, MOB_INDEX_DATA *mob );
bool is_npc( CHAR_DATA *ch );
bool is_immortal( CHAR_DATA *ch );
bool is_avatar( CHAR_DATA *ch );
/*------------*/

/* Channels.c */
void free_all_channels( void );
void load_channels( void );
void log_string_plus( const char *str, short log_type, int level );
#define log_string(txt) ( log_string_plus( (txt), LOG_NORMAL, PERM_LOG ) )
void log_printf_plus( short log_type, int level, const char *fmt, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
void log_printf( const char *fmt, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void to_channel( const char *argument, const char *channel, int level );
void to_channel_printf( const char *channel, int level, const char *fmt, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
bool check_channel( CHAR_DATA *ch, char *command, char *argument );
/*------------*/

/* Clans.c */
extern CLAN_DATA *first_clan, *last_clan;
extern COUNCIL_DATA *first_council, *last_council;
CLAN_DATA *get_clan( char *name );
COUNCIL_DATA *get_council( char *name );
void write_council_list( void );
void write_clan_list( void );
void save_council( COUNCIL_DATA *council );
void save_clan( CLAN_DATA *clan );
void load_councils( void );
void load_clans( void );
void update_clan_victory( CLAN_DATA *clan, CHAR_DATA *ch, CHAR_DATA *vict );
void free_member( MEMBER_DATA *member );
void add_clan_member( CLAN_DATA *clan, const char *name );
void free_clans( void );
void add_council_member( COUNCIL_DATA *council, const char *name );
void free_councils( void );
bool remove_council_member( COUNCIL_DATA *council, const char *name );
bool rename_council_member( COUNCIL_DATA *council, const char *oname, const char *nname );
bool is_clan_member( CLAN_DATA *clan, const char *name );
bool rename_clan_member( CLAN_DATA *clan, const char *oname, const char *nname );
bool remove_clan_member( CLAN_DATA *clan, const char *name );
/*------------*/

/* Classes.c */
extern int MAX_PC_CLASS;
extern CLASS_TYPE *class_table[MAX_CLASS];
extern int num_descriptors;
extern time_t boot_time;
MCLASS_DATA *add_mclass( CHAR_DATA *ch, int wclass, int level, int cpercent, double exp, bool limit );
void write_class_file( int cl );
void load_class_restrictions( void );
void load_classes( void );
void save_classes( void );
int get_char_cnum( CHAR_DATA *ch, char *argument );
bool char_is_class( CHAR_DATA *ch, int cnum );
/*------------*/

/* Comm.c */
/* So we can have different configs for different ports -- Shaddai */
extern int port;
extern bool mud_down;
extern time_t current_time;
extern PC_DATA *first_pc, *last_pc;
extern DESCRIPTOR_DATA *first_descriptor, *last_descriptor;
extern char lastplayercmd[MIL *2];
extern char str_boot_time[MIL];
char *myobj( OBJ_DATA *obj );
char *obj_short( OBJ_DATA *obj );
char *default_fprompt( CHAR_DATA *ch );
char *default_prompt( CHAR_DATA *ch );
void act( short AType, const char *format, CHAR_DATA *ch, void *arg1, void *arg2, int type );
void game_loop( void );
void new_descriptor( void );
void free_desc( DESCRIPTOR_DATA *d );
void close_socket( DESCRIPTOR_DATA *dclose, bool force );
void caught_alarm( int signum );
void read_from_buffer( DESCRIPTOR_DATA *d );
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, unsigned int length );
void nanny( DESCRIPTOR_DATA *d, char *argument );
void stop_idling( CHAR_DATA *ch );
void display_prompt( DESCRIPTOR_DATA *d );
void set_pager_input( DESCRIPTOR_DATA *d, char *argument );
void open_mud_log( void );
double get_percent( int fnum, int snum );
bool pager_output( DESCRIPTOR_DATA *d );
bool exists_file( char *name );
bool is_valid_path( CHAR_DATA *ch, const char *direct, const char *filename );
bool can_use_path( CHAR_DATA *ch, const char *direct, const char *filename );
bool multi_check( DESCRIPTOR_DATA *host, bool full );
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn );
bool check_playing( DESCRIPTOR_DATA *d, char *name, bool kick );
bool check_parse_name( char *name, bool newchar );
bool write_to_descriptor_old( int desc, const char *txt, int length );
bool write_to_descriptor( DESCRIPTOR_DATA *d, const char *txt, int length );
bool flush_buffer( DESCRIPTOR_DATA *d, bool fPrompt );
bool read_from_descriptor( DESCRIPTOR_DATA *d );
/*------------*/

/* Commands.c */
CMDTYPE *find_command( char *command, bool exact );
void load_commands( void );
void free_commands( void );
/*------------*/

/* Const.c */
/* Some that would be here are in Const.h */
extern const struct liq_type liq_table[LIQ_MAX];
extern const char *attack_table[DAM_MAX];
extern const char **const s_message_table[DAM_MAX];
extern const char **const p_message_table[DAM_MAX];
OBJ_DATA *get_eq_hold( CHAR_DATA *ch, int type );
OBJ_DATA *get_next_eq_hold( CHAR_DATA *ch, OBJ_DATA *oobj, int type );
void code_check( void );
int get_ac( CHAR_DATA *ch );
int get_hitroll( CHAR_DATA *ch );
int get_damroll( CHAR_DATA *ch );
/*------------*/

/* Db.c */
#include "db.h"
/*------------*/

/* Deity.c */
extern DEITY_DATA *first_deity, *last_deity;
DEITY_DATA *get_deity( char *name );
void save_deity( DEITY_DATA *deity );
void load_deity( void );
void free_deities( void );
void adjust_favor( CHAR_DATA *ch, int field, int mod );
bool rename_deity_worshipper( DEITY_DATA *deity, const char *oname, const char *nname );
bool remove_deity_worshipper( DEITY_DATA *deity, const char *name );
bool is_deity_worshipper( DEITY_DATA *deity, const char *name );
/*------------*/

/* Exp.c */
void update_level( CHAR_DATA *ch );
void advance_level( CHAR_DATA *ch );
void gain_exp( CHAR_DATA *ch, double gain );
double exp_level( CHAR_DATA *ch, int level );
double xp_compute( CHAR_DATA *gch, CHAR_DATA *victim );
/*------------*/

/* Fight.c */
extern HHF_DATA *first_hating, *last_hating;
extern HHF_DATA *first_hunting, *last_hunting;
extern HHF_DATA *first_fearing, *last_fearing;
CMDF( do_flee );
OBJ_DATA *get_eq_location( CHAR_DATA *ch, int iWear );
CHAR_DATA *who_fighting( CHAR_DATA *ch );
void stop_hunting( CHAR_DATA *ch, CHAR_DATA *victim, bool complete );
void start_hunting( CHAR_DATA *ch, CHAR_DATA *victim );
void stop_hating( CHAR_DATA *ch, CHAR_DATA *victim, bool complete );
void start_hating( CHAR_DATA *ch, CHAR_DATA *victim );
void stop_fearing( CHAR_DATA *ch, CHAR_DATA *victim, bool complete );
void start_fearing( CHAR_DATA *ch, CHAR_DATA *victim );
void stop_hhf( CHAR_DATA *ch );
void fix_hhf( CHAR_DATA *ch );
void violence_update( void );
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim );
void update_pos( CHAR_DATA *victim );
void free_fight( CHAR_DATA *ch );
void stop_fighting( CHAR_DATA *ch, bool fBoth );
void death_cry( CHAR_DATA *ch );
void raw_kill( CHAR_DATA *ch, CHAR_DATA *victim );
void group_gain( CHAR_DATA *ch, CHAR_DATA *victim );
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim );
void new_dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, unsigned int dt, OBJ_DATA *obj );
ch_ret one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
ch_ret multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
ch_ret projectile_hit( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, OBJ_DATA *projectile, short dist );
ch_ret damage( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon, int dam, int dt, bool candameq );
int align_compute( CHAR_DATA *gch, CHAR_DATA *victim );
int ris_damage( CHAR_DATA *ch, short dam, int ris );
bool in_arena( CHAR_DATA *ch );
bool is_safe( CHAR_DATA *ch, CHAR_DATA *victim, bool show_messg );
bool legal_loot( CHAR_DATA *ch, CHAR_DATA *victim );
bool is_fearing( CHAR_DATA *ch, CHAR_DATA *victim );
bool is_hating( CHAR_DATA *ch, CHAR_DATA *victim );
bool is_hunting( CHAR_DATA *ch, CHAR_DATA *victim );
/*------------*/

/* Fish.c */
void free_all_fnames( void );
void load_fnames( void );
void free_all_fish( void );
void update_fishing( void );
bool stop_obj_fishing( OBJ_DATA *obj );
bool stop_fishing( CHAR_DATA *ch );
/*------------*/

/* Friends.c */
FRIEND_DATA *find_friend( CHAR_DATA *ch, char *fname );
FRIEND_DATA *add_friend( CHAR_DATA *ch, char *fname );
void send_friend_info( CHAR_DATA *ch, char *message );
void free_all_friends( CHAR_DATA *ch );
/*------------*/

/* Handler.c */
extern CHAR_DATA *cur_char;
extern bool cur_char_died;
extern ch_ret global_retcode;
extern int falling;
CHAR_DATA *get_char_room( CHAR_DATA *ch, char *argument );
CHAR_DATA *get_char_world( CHAR_DATA *ch, char *argument );
CHAR_DATA *room_is_dnd( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex );
TIMER *get_timerptr( CHAR_DATA *ch, short type );
OBJ_DATA *get_obj_carry( CHAR_DATA *ch, char *argument );
OBJ_DATA *get_eq_char( CHAR_DATA *ch, int iWear );
OBJ_DATA *get_obj_wear( CHAR_DATA *ch, char *argument );
OBJ_DATA *new_get_obj_here( int type, CHAR_DATA *ch, char *argument );
OBJ_DATA *get_obj_here( CHAR_DATA *ch, char *argument );
OBJ_DATA *obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex );
OBJ_DATA *get_obj_list( CHAR_DATA *ch, char *argument, OBJ_DATA *list );
OBJ_DATA *get_obj_list_rev( CHAR_DATA *ch, char *argument, OBJ_DATA *list );
OBJ_DATA *obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch );
OBJ_DATA *get_objtype( CHAR_DATA *ch, short type );
OBJ_DATA *obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to );
OBJ_DATA *get_obj_world( CHAR_DATA *ch, char *argument );
OBJ_DATA *group_object( OBJ_DATA *obj1, OBJ_DATA *obj2 );
OBJ_DATA *get_trap( OBJ_DATA *obj );
OBJ_DATA *clone_object( OBJ_DATA *obj );
OBJ_DATA *get_obj_vnum( CHAR_DATA *ch, int vnum );
OBJ_DATA *find_obj( CHAR_DATA *ch, char *argument, bool carryonly );
AREA_DATA *get_area_obj( OBJ_INDEX_DATA *pObjIndex );
AREA_DATA *get_area( char *name );
char *distime( time_t updated );
char *affect_loc_name( int location );
char *shorttime( time_t updated );
const char *pull_type_name( int pulltype );
const char *dis_race_name( int race );
const char *dis_main_class_name( CHAR_DATA *ch );
const char *dis_class_name( int Class );
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf );
void extract_timer( CHAR_DATA *ch, TIMER *timer );
void clean_room( ROOM_INDEX_DATA *room );
void clean_obj( OBJ_INDEX_DATA *obj );
void free_teleports( void );
void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf );
void clean_char_queue( void );
void clean_mob( MOB_INDEX_DATA *mob );
void set_cur_char( CHAR_DATA *ch );
void add_timer( CHAR_DATA *ch, short type, int count, DO_FUN *fun, int value );
void extract_exit( ROOM_INDEX_DATA *room, EXIT_DATA *pexit );
void write_corpses( OBJ_DATA *obj, bool dontsave );
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd );
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf );
void add_kill( CHAR_DATA *ch, CHAR_DATA *mob );
void split_obj( OBJ_DATA *obj, int num );
void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear );
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj );
void separate_obj( OBJ_DATA *obj );
void extract_char( CHAR_DATA *ch, bool fPull );
void obj_from_obj( OBJ_DATA *obj );
void wait_state( CHAR_DATA *ch, int npulse );
void remove_file( const char *filename );
void save_char_obj( CHAR_DATA *ch );
void remove_timer( CHAR_DATA *ch, short type );
void update_aris( CHAR_DATA *ch );
void affect_strip( CHAR_DATA *ch, int sn );
void set_base_stats( CHAR_DATA *ch );
void char_from_room( CHAR_DATA *ch );
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex );
void obj_from_char( OBJ_DATA *obj );
void extract_obj( OBJ_DATA *obj );
void obj_from_room( OBJ_DATA *obj );
void check_chareq( CHAR_DATA *ch );
void queue_extracted_char( CHAR_DATA *ch, bool extract );
void worsen_mental_state( CHAR_DATA *ch, int mod );
void apply_ac( CHAR_DATA *ch, OBJ_DATA *obj, int iWear, bool remove );
ch_ret check_room_for_traps( CHAR_DATA *ch, int flag );
ch_ret check_for_trap( CHAR_DATA *ch, OBJ_DATA *obj, int flag );
ch_ret spring_trap( CHAR_DATA *ch, OBJ_DATA *obj );
int can_carry_n( CHAR_DATA *ch );
int get_obj_weight( OBJ_DATA *obj );
int can_carry_w( CHAR_DATA *ch );
int get_obj_number( OBJ_DATA *obj );
int get_real_obj_weight( OBJ_DATA *obj );
int times_killed( CHAR_DATA *ch, CHAR_DATA *mob );
short get_birth_year( CHAR_DATA *ch, short age );
short get_curr_stat( int stat, CHAR_DATA *ch );
short get_curr_str( CHAR_DATA *ch );
short get_curr_int( CHAR_DATA *ch );
short get_curr_wis( CHAR_DATA *ch );
short get_curr_dex( CHAR_DATA *ch );
short get_curr_con( CHAR_DATA *ch );
short get_curr_cha( CHAR_DATA *ch );
short get_curr_lck( CHAR_DATA *ch );
short get_timer( CHAR_DATA *ch, short type );
short get_trust( CHAR_DATA *ch );
short get_age( CHAR_DATA *ch );
short get_perm_stat( int stat, CHAR_DATA *ch );
short get_mod_stat( int stat, CHAR_DATA *ch );
bool in_hard_range( CHAR_DATA *ch, AREA_DATA *tarea );
bool is_devoted( CHAR_DATA *ch );
bool is_trapped( OBJ_DATA *obj );
bool is_affected( CHAR_DATA *ch, int sn );
bool ms_find_obj( CHAR_DATA *ch );
bool chance( CHAR_DATA *ch, short percent );
bool can_astral( CHAR_DATA *ch, CHAR_DATA *victim );
bool is_good( CHAR_DATA *ch );
bool is_evil( CHAR_DATA *ch );
bool is_neutral( CHAR_DATA *ch );
bool is_clanned( CHAR_DATA *ch );
bool is_nationed( CHAR_DATA *ch );
bool is_counciled( CHAR_DATA *ch );
bool in_magic_container( OBJ_DATA *obj );
bool empty_obj( OBJ_DATA *obj, OBJ_DATA *destobj, ROOM_INDEX_DATA *destroom );
bool is_obj_stat( OBJ_DATA *obj, int stat );
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex );
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex );
bool can_see_obj( CHAR_DATA *ch, OBJ_DATA *obj );
bool is_guest( CHAR_DATA *ch );
bool can_wear( OBJ_DATA *obj, int part );
bool nifty_is_name( char *str, char *namelist );
bool nifty_is_name_prefix( char *str, char *namelist );
bool is_name( const char *str, char *namelist );
bool is_retired( CHAR_DATA *ch );
bool char_died( CHAR_DATA *ch );
bool is_awake( CHAR_DATA *ch );
bool is_floating( CHAR_DATA *ch );
bool is_pkill( CHAR_DATA *ch );
bool check_subrestricted( CHAR_DATA *ch );
bool can_go( ROOM_INDEX_DATA *room, int door );
bool is_drunk( CHAR_DATA *ch, int drunk );
bool can_pkill( CHAR_DATA *ch );
bool can_drop_room( CHAR_DATA *ch );
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj );
bool is_vampire( CHAR_DATA *ch );
bool is_outside( CHAR_DATA *ch );
bool valid_pfile( const char *filename );
bool can_see( CHAR_DATA *ch, CHAR_DATA *victim );
bool can_take_proto( CHAR_DATA *ch );
bool is_valid_pers( int sn );
bool is_valid_herb( int sn );
bool is_valid_sn( int sn );
bool is_idle( CHAR_DATA *ch );
bool is_pacifist( CHAR_DATA *ch );
bool no_weather_sect( ROOM_INDEX_DATA *room );
/*------------*/

/* Hashstr.c */
char *quick_link( const char *str, const char *filename, int line );
char *str_alloc( const char *str, const char *filename, int line );
char *check_hash( const char *str );
char *hash_stats( void );
void show_high_hash( int top );
int str_free( const char *str, const char *filename, int line );
bool in_hash_table( const char *str );
bool hash_dump( int hash );
/*------------*/

/* Helps.c */
extern int top_help;
CMDF( do_help );
struct help_data *get_help( CHAR_DATA *ch, char *argument );
char *un_fix_help( char *text );
char *help_fix( char *text );
void add_skill_help( SKILLTYPE *skill, bool update );
void update_skill_helps( void );
void update_command_helps( void );
void load_helps( void );
void add_command_help( CMDTYPE *cmd, bool update );
void free_helps( void );
bool valid_help( char *argument );
/*------------*/

/* Highscore.c */
void load_highscores( void );
void free_hightables( void );
/*------------*/

/* Hint.c */
void load_hints( void );
void free_hints( void );
/*------------*/

/* Host.c */
bool check_host( CHAR_DATA *ch );
/*------------*/

/* Hostlist.c */
void load_hostlog( void );
void free_all_hostlogs( void );
void handle_hostlog( const char *message, CHAR_DATA *ch );
/*------------*/

/* Interp.c */
extern bool fLogAll;
extern CMDTYPE *command_hash[126];
extern SOCIALTYPE *social_index[126];
char *one_argument( char *argument, char *arg_first );
char *one_argument2( char *argument, char *arg_first );
char *check_cmd_flags( CHAR_DATA *ch, CMDTYPE *cmd );
void update_userec( struct timeval *time_used, struct timerset *userec );
void start_timer( struct timeval *sttime );
void send_timer( struct timerset *vtime, CHAR_DATA *ch );
void interpret( CHAR_DATA *ch, char *argument );
void interpret_printf( CHAR_DATA *ch, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
time_t end_timer( struct timeval *sttime );
int number_argument( char *argument, char *arg );
bool is_number( char *arg );
/*------------*/

/* Lockers.c */
void load_lockershares( void );
void check_lockers( void );
void rename_locker( CHAR_DATA *ch, char *newname );
void rename_lockershare( char *name, char *newname );
void free_all_lockershare( void );
bool is_locker_shared( char *name );
/*------------*/

/* News.c */
void free_all_news( void );
/*------------*/

/* Magic.c */
extern char *target_name;
extern char *ranged_target_name;
CMDF( do_cast );
ch_ret spell_affectchar( int sn, int level, CHAR_DATA *ch, void *vo );
ch_ret obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj );
int bsearch_skill_exact( const char *name, int first, int top );
int bsearch_skill_prefix( const char *name, int first, int top );
int herb_lookup( const char *name );
int pers_lookup( const char *name );
int dice_parse( CHAR_DATA *ch, CHAR_DATA *victim, int level, char *texp );
int slot_lookup( int slot );
int skill_lookup( const char *name );
bool saves_spell_staff( int level, CHAR_DATA *victim );
bool saves_poison_death( int level, CHAR_DATA *victim );
/*------------*/

/* Makeobjs.c */
OBJ_DATA *make_trap( int v0, int v1, int v2, int v3 );
OBJ_DATA *make_corpse( CHAR_DATA *ch, CHAR_DATA *killer );
OBJ_DATA *create_money( int amount );
void make_bloodstain( OBJ_DATA *obj, ROOM_INDEX_DATA *room );
void make_blood( CHAR_DATA *ch );
void set_money( OBJ_DATA *obj, int amount );
void make_scraps( OBJ_DATA *obj );
/*------------*/

/* Mccp.c */
extern int mccpusers;
/*------------*/

/* Misc.c */
CMDF( do_quaff );
EXT_BV meb( int bit );
EXT_BV fread_bitvector( FILE *fp );
void actiondesc( CHAR_DATA *ch, OBJ_DATA *obj );
/*------------*/

/* Mud_comm.c */
const char *mprog_type_to_name( int type );
void load_mpdamages( void );
void free_all_mpdamages( void );
int get_color( char *argument );
/*------------*/

/* Mud_prog.c */
extern struct act_prog_data *mob_act_list;
extern OBJ_DATA *supermob_obj;
extern CHAR_DATA *supermob;
void init_supermob( void );
void free_prog_actlists( void );
void progbug( const char *str, CHAR_DATA *mob );
void progbug_printf( CHAR_DATA *mob, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void mprog_entry_trigger( CHAR_DATA *mob );
void mprog_hitprcnt_trigger( CHAR_DATA *mob, CHAR_DATA *ch );
void mprog_greet_trigger( CHAR_DATA *ch );
void mprog_speech_trigger( char *txt, CHAR_DATA *actor );
void mprog_bribe_trigger( CHAR_DATA *mob, CHAR_DATA *ch, int amount );
void mprog_give_trigger( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj );
void mprog_act_trigger( char *buf, CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj, void *vo );
void mprog_death_trigger( CHAR_DATA *killer, CHAR_DATA *mob );
void mprog_fight_trigger( CHAR_DATA *mob, CHAR_DATA *ch );
void oprog_scrap_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_zap_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_fight_trigger( CHAR_DATA *ch );
void oprog_act_trigger( char *buf, OBJ_DATA *mobj, CHAR_DATA *ch, OBJ_DATA *obj, void *vo );
void oprog_speech_trigger( char *txt, CHAR_DATA *ch );
void oprog_examine_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_greet_trigger( CHAR_DATA *ch );
void oprog_open_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_pull_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_push_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_close_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_wear_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_damage_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_sac_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type, OBJ_DATA *iobj );
void oprog_drop_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_remove_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void oprog_put_trigger( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container );
void oprog_get_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void rprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type, ROOM_INDEX_DATA *room );
void rprog_rfight_trigger( CHAR_DATA *ch );
void rprog_act_trigger( char *buf, ROOM_INDEX_DATA *room, CHAR_DATA *ch, OBJ_DATA *obj, void *vo );
void rprog_leave_trigger( CHAR_DATA *ch );
void rprog_enter_trigger( CHAR_DATA *ch );
void rprog_speech_trigger( char *txt, CHAR_DATA *ch );
void rprog_sleep_trigger( CHAR_DATA *ch );
void rprog_rest_trigger( CHAR_DATA *ch );
void rprog_death_trigger( CHAR_DATA *ch );
void room_act_add( ROOM_INDEX_DATA *room );
void obj_act_add( OBJ_DATA *obj );
void set_supermob( OBJ_DATA *obj );
void rset_supermob( ROOM_INDEX_DATA *room );
void release_supermob( void );
int mprog_do_command( char *cmnd, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, CHAR_DATA *rndm, bool ignore, bool ignore_ors );
bool oprog_use_trigger( CHAR_DATA *ch, OBJ_DATA *obj, CHAR_DATA *vict, OBJ_DATA *targ );
/*------------*/

/* Mwresets.c */
void load_mwresets( void );
void free_all_mwresets( void );
void handle_mwreset( OBJ_DATA *corpse );
void handle_mwmobilereset( ROOM_INDEX_DATA *room );
/*------------*/

/* News.c */
void load_news( void );
/*------------*/

/* Player.c */
CMDF( do_pushup );
CMDF( do_situp );
void set_title( CHAR_DATA *ch, char *title );
void handle_stat( CHAR_DATA *ch, short stat, bool silence, short chance );
/*------------*/

/* Polymorph.c */
MORPH_DATA *find_morph( CHAR_DATA *ch, char *target, bool is_cast );
MORPH_DATA *get_morph( char *arg );
MORPH_DATA *get_morph_vnum( int vnum );
void show_morphs( CHAR_DATA *ch );
void do_unmorph_char( CHAR_DATA *ch );
void load_morphs( void );
void free_morphs( void );
int do_morph_char( CHAR_DATA *ch, MORPH_DATA *morph );
/*------------*/

/* Quest.c */
void load_rewards( void );
void free_rewards( void );
/*------------*/

/* Races.c */
extern int MAX_PC_RACE;
extern struct race_type *race_table[MAX_RACE];
void load_races( void );
void write_race_file( int ra );
/*------------*/

/* Reserved.c */
void load_reserved( void );
void free_all_reserved( void );
bool is_reserved_name( char *name );
/*------------*/

/* Reset.c */
RESET_DATA *add_reset( ROOM_INDEX_DATA *room, char letter, int extra, int arg1, int arg2, int arg3, short rchance, short xcord, short ycord, bool wilderness );
char *sprint_reset( RESET_DATA *pReset, short *num );
void delete_reset( RESET_DATA *pReset );
void wipe_resets( ROOM_INDEX_DATA *room );
void renumber_put_resets( ROOM_INDEX_DATA *room );
void reset_area( AREA_DATA *area );
/*------------*/

/* Save.c */
extern CHAR_DATA *quitting_char;
extern CHAR_DATA *loading_char;
extern CHAR_DATA *saving_char;
void load_corpses( void );
void set_alarm( long seconds );
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest, short os_type, bool hotboot );
void fread_obj( CHAR_DATA *ch, NOTE_DATA *pnote, FILE *fp, short os_type );
bool load_char_obj( DESCRIPTOR_DATA *d, char *name, bool preload, bool copyover );
/*------------*/

/* Skills.c */
CMDF( skill_notfound );
CMDF( do_dig );
CMDF( do_search );
CMDF( do_detrap );
CMDF( do_recall );
CMDF( do_bash );
CMDF( do_stun );
CMDF( do_gouge );
CMDF( do_feed );
SKILLTYPE *new_skill( void );
SKILLTYPE *get_skilltype( int sn );
void depierce_equipment( CHAR_DATA *ch, int iWear );
void check_reqs( void );
void free_skills( void );
void trip( CHAR_DATA *ch, CHAR_DATA *victim );
void disarm( CHAR_DATA *ch, CHAR_DATA *victim );
void learn_from_success( CHAR_DATA *ch, int sn );
void learn_from_failure( CHAR_DATA *ch, int sn );
int get_adept( CHAR_DATA *ch, int sn );
int first_learned( CHAR_DATA *ch, int sn );
int mana_cost( CHAR_DATA *ch, int sn );
int get_possible_adept( CHAR_DATA *ch, int sn );
bool check_skill( CHAR_DATA *ch, char *command, char *argument );
bool check_duck( CHAR_DATA *ch, CHAR_DATA *victim );
bool check_tumble( CHAR_DATA *ch, CHAR_DATA *victim );
bool check_shieldblock( CHAR_DATA *ch, CHAR_DATA *victim );
bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim );
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim );
bool check_counter( CHAR_DATA *ch, CHAR_DATA *victim, int dam );
bool check_block( CHAR_DATA *ch, CHAR_DATA *victim );
bool can_practice( CHAR_DATA *ch, int sn );
/*------------*/

/* Socials.c */
SOCIALTYPE *find_social( const char *command, bool exact );
void load_socials( void );
void free_socials( void );
bool check_social( CHAR_DATA *ch, const char *command, char *argument );
/*------------*/

/* Special.c */
SPEC_FUN *spec_lookup( char *name );
void load_specfuns( void );
void free_specfuns( void );
bool validate_spec_fun( char *name );
bool is_fighting( CHAR_DATA *ch );
/*------------*/

/* Storages.c */
void load_storages( void );
void save_storage( ROOM_INDEX_DATA *room );
/*------------*/

/* Sysdata.c */
void load_systemdata( void );
void save_sysdata( bool autosave );
/*------------*/

/* Tables.c */
extern LANG_DATA *first_lang, *last_lang;
extern int top_sn;
extern int top_herb;
extern int top_pers;
extern SKILLTYPE *herb_table[MAX_HERB];
extern SKILLTYPE *pers_table[MAX_PERS];
extern SKILLTYPE *skill_table[MAX_SKILL];
extern const char *const skill_tname[];
DO_FUN *skill_function( char *name );
void remap_slot_numbers( void );
void sort_skill_table( void );
void load_skill_table( void );
void load_herb_table( void );
void load_pers_table( void );
void load_tongues( void );
void free_tongues( void );
/*------------*/

/* Timeinfo.c */
char *show_birthday( CHAR_DATA *ch );
void load_calendarinfo( void );
void load_timeinfo( void );
void free_all_calendarinfo( void );
/*------------*/

/* Transfer.c */
void create_transfer( void );
void load_transfer( void );
void update_transfer( int type, int size );
void free_transfer( void );
/*------------*/

/* Trivia.c */
void load_trivia( void );
void stop_trivia( void );
void free_all_trivias( void );
/*------------*/

/* Update.c */
extern CHAR_DATA *gch_prev;
extern OBJ_DATA *gobj_prev;
bool weight_check( CHAR_DATA *ch, OBJ_DATA *obj );
void remove_portal( OBJ_DATA *portal );
void aggr_room_update( CHAR_DATA *wch, ROOM_INDEX_DATA *room );
void weather_update( void );
void update_handler( void );
void gain_condition( CHAR_DATA *ch, int iCond, int value );
void subtract_times( struct timeval *etime, struct timeval *sttime );
/*------------*/

/* Wilderness.c */
extern bool loc_in_wilderness;
extern int loc_cords[2];
void set_loc_cords( CHAR_DATA *ch );
void char_to_char_cords( CHAR_DATA *ch, CHAR_DATA *victim );
void put_in_wilderness( CHAR_DATA *ch, short x, short y );
void obj_to_char_cords( OBJ_DATA *obj, CHAR_DATA *ch );
void parse_wilderness_description( CHAR_DATA *ch, char *str );
void move_around_wilderness( CHAR_DATA *ch, short dir, char *str );
bool is_in_wilderness( CHAR_DATA *ch );
bool can_see_character( CHAR_DATA *ch, CHAR_DATA *rch );
/*------------*/
