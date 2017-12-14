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
 *****************************************************************************/

/* Define maximum number of climate settings - FB */
#define MAX_CLIMATE 5

const char *con_state( short state );

int get_flag( char *flag, const char *flagarray[], int max );
int get_langflag( char *flag );
int get_langnum( char *flag );
extern const char *part_messages[];
extern const char *cmd_flags[];
extern const char *lang_names[];
extern const char *pos_names[];
extern const char *sex_names[];
extern const char *sect_flags[];
extern const char *style_names[];
extern const char *spell_damage[];
extern const char *spell_save_effect[];
extern const char *spell_saves[];
extern const char *spell_action[];
extern const char *spell_power[];
extern const char *spell_class[];
extern const char *spell_flag[];
extern const char *target_type[];
extern const char *channelflags[];
extern const char *stattypes[];
extern const char *save_flag[];
extern const char *perms_flag[];
extern const char *groups_flag[];
extern const char *he_she[];
extern const char *him_her[];
extern const char *his_her[];
extern const char *prog_names[];
extern const char *ris_flags[];
extern const char *r_flags[];
extern const char *where_name[];
extern const char *a_flags[];
extern const char *lodge_name[];
extern const char *item_w_flags[];
extern const char *trap_flags[];
extern const char *w_flags[];
extern const char *o_types[];
extern const char *o_flags[];
extern const char *part_flags[];
extern const char *attack_flags[];
extern const char *defense_flags[];
extern const char *act_flags[];
extern const char *plr_flags[];
extern const char *wear_locs[];
extern const char *a_types[];
extern const char *ex_flags[];
extern const char *a_types[];
extern const char *pc_flags[];
extern const char *area_flags[];
extern const char *wind_msg[6];
extern const char *precip_msg[3];
extern const char *windtemp_msg[6][6];
extern const char *preciptemp_msg[6][6];
extern const char *wind_settings[MAX_CLIMATE];
extern const char *precip_settings[MAX_CLIMATE];
extern const char *temp_settings[MAX_CLIMATE];
extern int part_vnums[];
extern int const lang_array[];

#define KEY( literal, field, value )   \
if ( !str_cmp( word, (literal) ) )     \
{                                      \
   (field) = (value);                  \
   fMatch = true;                      \
   break;                              \
}

/* Key using strings */
#define SKEY( literal, field, fpin, flagarray, max )                 \
if( !str_cmp( word, (literal) ) )                                    \
{                                                                    \
   infoflags = fread_flagstring( (fpin) );                           \
   while( infoflags && infoflags[0] != '\0' )                        \
   {                                                                 \
      infoflags = one_argument( infoflags, flag );                   \
      value = get_flag( flag, (flagarray), (max) );                  \
      if( value < 0 || value >= (max) )                              \
         bug( "%s: Unknown %s: %s", __FUNCTION__, (literal), flag ); \
      else                                                           \
         (field) = value;                                            \
   }                                                                 \
   fMatch = true;                                                    \
   break;                                                            \
}

/* KEY with EXT and While */
#define WEXTKEY( literal, field, fpin, flagarray, max )              \
if( !str_cmp( word, (literal) ) )                                    \
{                                                                    \
   infoflags = fread_flagstring( (fpin) );                           \
   while( infoflags && infoflags[0] != '\0' )                        \
   {                                                                 \
      infoflags = one_argument( infoflags, flag );                   \
      value = get_flag( flag, (flagarray), (max) );                  \
      if( value < 0 || value >= (max) )                              \
         bug( "%s: Unknown %s: %s", __FUNCTION__, (literal), flag ); \
      else                                                           \
         xSET_BIT( (field), value );                                 \
   }                                                                 \
   fMatch = true;                                                    \
   break;                                                            \
}

/* Uses short since can only be one of these at a time */
typedef enum
{
   PERM_ALL,   PERM_IMM,   PERM_BUILDER,   PERM_LEADER,   PERM_HEAD,
   PERM_IMP,   PERM_MAX
} perms;

/* Uses short since can only be one of these at a time */
/* Although these are the same as the perms it was just for simplicty and these are to change the groups for commands */
typedef enum
{
   GROUP_ALL,   GROUP_IMM,   GROUP_BUILDER,   GROUP_LEADER,   GROUP_HEAD,
   GROUP_IMP,   GROUP_MAX
} groups;

/* Uses EXT_BV - Autosave flags */
typedef enum
{
   SV_DEATH,     SV_KILL,   SV_PASSCHG,   SV_DROP,      SV_PUT,
   SV_GIVE,      SV_AUTO,   SV_ZAPDROP,   SV_AUCTION,   SV_GET,
   SV_RECEIVE,   SV_IDLE,   SV_FILL,      SV_EMPTY,     SV_MAX
} save_flags;

/* Uses EXT_BV */
typedef enum
{
   LANG_COMMON,   LANG_ELVISH,     LANG_DWARVEN,    LANG_PIXIE,   LANG_OGRE,
   LANG_ORCISH,   LANG_TROLLISH,   LANG_HALFLING,   LANG_GITH,    LANG_GNOME,
   LANG_UNKNOWN
} languages;
#define VALID_LANGS ( LANG_COMMON | LANG_ELVISH | LANG_DWARVEN | LANG_PIXIE | LANG_OGRE | LANG_ORCISH | \
                      LANG_TROLLISH | LANG_HALFLING | LANG_GITH | LANG_GNOME )

typedef enum
{
   STAT_STR,   STAT_INT,   STAT_WIS,   STAT_DEX,   STAT_CON,
   STAT_CHA,   STAT_LCK,   STAT_MAX
} stat_types;

typedef enum
{
   SS_NONE,     SS_POISON_DEATH,   SS_ROD_WANDS,   SS_PARA_PETRI,
   SS_BREATH,   SS_SPELL_STAFF,    SS_MAX
} save_types;

/* Uses EXT_BV */
typedef enum
{
   CHANNEL_GLOBAL,    CHANNEL_YELL,    CHANNEL_LOG,        CHANNEL_CLAN,
   CHANNEL_COUNCIL,   CHANNEL_TELLS,   CHANNEL_RACETALK,   CHANNEL_WHISPER,
   CHANNEL_NATION,    CHANNEL_CLASS,   CHANNEL_FCHAT,      CHANNEL_MAX
} channel_types;

/* Uses EXT_BV */
typedef enum
{
   SF_WATER,          SF_AREA,         SF_DISTANT,     SF_NOSELF,
   SF_ACCUMULATIVE,   SF_RECASTABLE,   SF_CANSCRIBE,   SF_CANBREW,
   SF_GROUPSPELL,     SF_OBJECT,       SF_CHARACTER,   SF_SECRETSKILL,
   SF_PKSENSITIVE,    SF_STOPONFAIL,   SF_NOFIGHT,     SF_NODISPEL,
   SF_CANMIX,         SF_CANCONCOCT,   SF_CANCARVE,    SF_CANIMBUE,
   SF_NOMOUNT,        SF_CANDOALL,     SF_MAX
} spell_flags;
#define SPELL_FLAG( skill, flag ) ( xIS_SET( (skill)->flags, (flag) ) )

/* Each of the Spell parts here uses short since can only be one of each at a time */
typedef enum
{
   SD_NONE,   SD_FIRE,   SD_WIND,   SD_EARTH,
   SD_WATER,  SD_ICE,    SD_COLD,   SD_ELECTRICITY,
   SD_ENERGY, SD_ACID,   SD_POISON, SD_DRAIN,
   SD_HOLY,   SD_SHADOW, SD_MAX
} spell_dam_types;
#define SPELL_DAMAGE( skill )   ( (skill)->damage )
#define SET_SDAM( skill, val )  ( (skill)->damage = (val) )
#define IS_MAGIC( dt )          ( is_valid_sn( (dt) ) && skill_table[(dt)] && skill_table[(dt)]->magical )
#define IS_FIRE( dt )           ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_FIRE )
#define IS_WIND( dt )           ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_WIND )
#define IS_EARTH( dt )          ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_EARTH )
#define IS_WATER( dt )          ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_WATER )
#define IS_ICE( dt )            ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_ICE )
#define IS_COLD( dt )           ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_COLD )
#define IS_ELECTRICITY( dt )    ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_ELECTRICITY )
#define IS_ENERGY( dt )         ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_ENERGY )
#define IS_ACID( dt )           ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_ACID )
#define IS_POISON( dt )         ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_POISON )
#define IS_DRAIN( dt )          ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_DRAIN )
#define IS_HOLY( dt )           ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_HOLY )
#define IS_SHADOW( dt )         ( is_valid_sn( (dt) ) && SPELL_DAMAGE( skill_table[(dt)]) == SD_SHADOW )

typedef enum
{
   SA_NONE,      SA_CREATE,     SA_DESTROY,   SA_RESIST,
   SA_SUSCEPT,   SA_DIVINATE,   SA_OBSCURE,   SA_CHANGE,
   SA_MAX
} spell_act_types;
#define SPELL_ACTION( skill )   ( (skill)->action )
#define SET_SACT( skill, val )  ( (skill)->action = (val) )

typedef enum
{
   SP_NONE,   SP_MINOR,   SP_GREATER,   SP_MAJOR,
   SP_MAX
} spell_power_types;
#define SPELL_POWER( skill )    ( (skill)->power )
#define SET_SPOW( skill, val )  ( (skill)->power = (val) )

typedef enum
{
   SC_NONE,     SC_LUNAR,   SC_SOLAR,   SC_TRAVEL,
   SC_SUMMON,   SC_LIFE,    SC_DEATH,   SC_ILLUSION,
   SC_MAX
} spell_class_types;
#define SPELL_CLASS( skill )    ( (skill)->Class )
#define SET_SCLA( skill, val )  ( (skill)->Class = (val) )

typedef enum
{
   SE_NONE,    SE_NEGATE,  SE_EIGHTHDAM, SE_QUARTERDAM,
   SE_HALFDAM, SE_3QTRDAM, SE_REFLECT,   SE_ABSORB,
   SE_MAX
} spell_save_effects;
#define SPELL_SAVE( skill )     ( (skill)->save )
#define SET_SSAV( skill, val ) ( (skill)->save = (val) )

typedef enum
{
   TAR_IGNORE,    TAR_CHAR_OFFENSIVE,   TAR_CHAR_DEFENSIVE,   TAR_CHAR_SELF,
   TAR_OBJ_INV,   TAR_MAX
} target_types;

typedef enum
{
   TRAP_TYPE_POISON_GAS = 1,   TRAP_TYPE_POISON_DART,     TRAP_TYPE_POISON_NEEDLE,    TRAP_TYPE_POISON_DAGGER,
   TRAP_TYPE_POISON_ARROW,     TRAP_TYPE_BLINDNESS_GAS,   TRAP_TYPE_SLEEPING_GAS,     TRAP_TYPE_FLAME,
   TRAP_TYPE_EXPLOSION,        TRAP_TYPE_ACID_SPRAY,      TRAP_TYPE_ELECTRIC_SHOCK,   TRAP_TYPE_BLADE,
   TRAP_TYPE_MAX
} trap_types;

typedef enum
{
   TRAP_NONE,         TRAP_ROOM,      TRAP_OBJ,      TRAP_ENTER_ROOM,
   TRAP_LEAVE_ROOM,   TRAP_OPEN,      TRAP_CLOSE,    TRAP_GET,
   TRAP_PUT,          TRAP_PICK,      TRAP_UNLOCK,   TRAP_N,
   TRAP_S,            TRAP_E,         TRAP_W,        TRAP_U,
   TRAP_D,            TRAP_EXAMINE,   TRAP_NE,       TRAP_NW,
   TRAP_SE,           TRAP_SW,        TRAP_MAX
} trapflags;

/* Can only be one style at a time so uses short */
typedef enum
{
   STYLE_BERSERK,   STYLE_AGGRESSIVE,   STYLE_FIGHTING,   STYLE_DEFENSIVE,
   STYLE_EVASIVE,   STYLE_MAX
} styles;

/* Uses EXT_BV */
typedef enum
{
   CMD_FLAG_POLYMORPHED,   CMD_FLAG_NOSHOW,   CMD_FLAG_NPC,   CMD_FLAG_PC,
   CMD_FLAG_ALLOW_TILDE,   CMD_FULL_NAME,     CMD_MAX
} cmdflags;

/* Uses EXT_BV */
typedef enum
{
   AFLAG_NOPKILL,   AFLAG_FREEKILL,   AFLAG_NOTELEPORT,   AFLAG_PROTOTYPE,
   AFLAG_NOQUEST,   AFLAG_NOSHOW,     AFLAG_NOLOAD,       AFLAG_MAX
} aflags;

/* One apply at a time so uses short */
/* Since a Vampire uses mana now instead of blood just using APPLY_MANA */
typedef enum
{
   APPLY_NONE,         APPLY_EXT_AFFECT,    APPLY_RESISTANT,   APPLY_WEAPONSPELL,
   APPLY_WEARSPELL,    APPLY_REMOVESPELL,   APPLY_STRIPSN,     APPLY_HIT,
   APPLY_MANA,         APPLY_MOVE,          APPLY_HITROLL,     APPLY_DAMROLL,
   APPLY_ARMOR,        APPLY_STAT,          APPLY_WAITSTATE,   APPLY_MAX
} apply_types;

/* Used to reverse the apply to the caster */
#define REVERSE_APPLY 1000

/* Uses EXT_BV */
typedef enum
{
   ITEM_NO_TAKE,          ITEM_WEAR_HEAD,       ITEM_WEAR_EARS,     ITEM_WEAR_EAR,
   ITEM_WEAR_EYES,        ITEM_WEAR_EYE,        ITEM_WEAR_FACE,     ITEM_WEAR_NECK,
   ITEM_WEAR_SHOULDERS,   ITEM_WEAR_SHOULDER,   ITEM_WEAR_ABOUT,    ITEM_WEAR_BODY,
   ITEM_WEAR_BACK,        ITEM_WEAR_ARMS,       ITEM_WEAR_ARM,      ITEM_WEAR_WRISTS,
   ITEM_WEAR_WRIST,       ITEM_WEAR_HANDS,      ITEM_WEAR_HAND,     ITEM_WEAR_FINGERS,
   ITEM_WEAR_FINGER,      ITEM_WEAR_HOLD_B,     ITEM_WEAR_HOLD,     ITEM_WEAR_WAIST,
   ITEM_WEAR_LEGS,        ITEM_WEAR_LEG,        ITEM_WEAR_ANKLES,   ITEM_WEAR_ANKLE,
   ITEM_WEAR_FEET,        ITEM_WEAR_FOOT,       ITEM_WEAR_MAX
} item_wear_flags;

typedef enum
{
   WEAR_NONE = -1,    WEAR_HEAD = 0,   WEAR_EARS,        WEAR_EAR_L,
   WEAR_EAR_R,        WEAR_EYES,       WEAR_EYE_L,       WEAR_EYE_R,
   WEAR_FACE,         WEAR_NECK,       WEAR_SHOULDERS,   WEAR_SHOULDER_L,
   WEAR_SHOULDER_R,   WEAR_ABOUT,      WEAR_BODY,        WEAR_BACK,
   WEAR_ARMS,         WEAR_ARM_L,      WEAR_ARM_R,       WEAR_WRISTS,
   WEAR_WRIST_L,      WEAR_WRIST_R,    WEAR_HANDS,       WEAR_HAND_L,
   WEAR_HAND_R,       WEAR_FINGERS,    WEAR_FINGER_L,    WEAR_FINGER_R,
   WEAR_HOLD_B,       WEAR_HOLD_L,     WEAR_HOLD_R,      WEAR_WAIST,
   WEAR_LEGS,         WEAR_LEG_L,      WEAR_LEG_R,       WEAR_ANKLES,
   WEAR_ANKLE_L,      WEAR_ANKLE_R,    WEAR_FEET,        WEAR_FOOT_L,
   WEAR_FOOT_R,       WEAR_MAX
} wear_locations;

/* Uses EXT_BV */
typedef enum
{
   ITEM_GLOW,           ITEM_LOYAL,             ITEM_INVIS,        ITEM_MAGIC,
   ITEM_NODROP,         ITEM_BLESS,             ITEM_ANTI_GOOD,    ITEM_ANTI_EVIL,
   ITEM_ANTI_NEUTRAL,   ITEM_NOREMOVE,          ITEM_INVENTORY,    ITEM_ORGANIC,
   ITEM_METAL,          ITEM_DONATION,          ITEM_CLANOBJECT,   ITEM_CLANCORPSE,
   ITEM_HIDDEN,         ITEM_POISONED,          ITEM_COVERING,     ITEM_DEATHROT,
   ITEM_BURIED,         ITEM_PROTOTYPE,         ITEM_NOLOCATE,     ITEM_GROUNDROT,
   ITEM_PKDISARMED,     ITEM_NOSCRAP,           ITEM_ENCHANTED,    ITEM_QUEST,
   ITEM_LODGED,         ITEM_PIERCED,           ITEM_RANDOM,       ITEM_NOCONTAINER,
   ITEM_NOGROUP,        ITEM_CONTINUOUS_FIRE,   ITEM_PTRAP,        ITEM_WILDERNESS,
   ITEM_MAX
} item_extra_flags;

/* Since for now only one type at a time uses short */
typedef enum
{
   ITEM_NONE,             ITEM_LIGHT,         ITEM_SCROLL,      ITEM_WAND,
   ITEM_STAFF,            ITEM_WEAPON,        ITEM_TREASURE,    ITEM_ARMOR,
   ITEM_POTION,           ITEM_TRASH,         ITEM_CONTAINER,   ITEM_DRINK_CON,
   ITEM_KEY,              ITEM_FOOD,          ITEM_MONEY,       ITEM_BOAT,
   ITEM_CORPSE_NPC,       ITEM_CORPSE_PC,     ITEM_FOUNTAIN,    ITEM_PILL,
   ITEM_BLOOD,            ITEM_BLOODSTAIN,    ITEM_SCRAPS,      ITEM_PIPE,
   ITEM_HERB_CON,         ITEM_HERB,          ITEM_FIRE,        ITEM_SWITCH, 
   ITEM_LEVER,            ITEM_PULLCHAIN,     ITEM_BUTTON,      ITEM_TRAP,
   ITEM_MAP,              ITEM_PORTAL,        ITEM_PAPER,       ITEM_LOCKPICK,
   ITEM_MISSILE_WEAPON,   ITEM_PROJECTILE,    ITEM_QUIVER,      ITEM_SHOVEL,
   ITEM_SALVE,            ITEM_COOK,          ITEM_KEYRING,     ITEM_GEM,
   ITEM_MORTAR,           ITEM_POWDER,        ITEM_AXE,         ITEM_WOOD,
   ITEM_FISH,             ITEM_FISHINGPOLE,   ITEM_PIECE,       ITEM_FURNITURE,
   ITEM_CLEANER,          ITEM_TYPE_MAX
} item_types;

/* Uses an int */
typedef enum
{
   SECT_INSIDE,       SECT_CITY,          SECT_FIELD,        SECT_FOREST,
   SECT_HILLS,        SECT_MOUNTAIN,      SECT_WATER_SWIM,   SECT_WATER_NOSWIM,
   SECT_UNDERWATER,   SECT_AIR,           SECT_DESERT,       SECT_DUNNO,
   SECT_OCEANFLOOR,   SECT_UNDERGROUND,   SECT_LAVA,         SECT_SWAMP,
   SECT_MAX
} sector_types;

/* Uses EXT_BV */
typedef enum
{
   EX_ISDOOR,    EX_CLOSED,      EX_LOCKED,       EX_SECRET,
   EX_SWIM,      EX_PICKPROOF,   EX_FLY,          EX_CLIMB,
   EX_DIG,       EX_EATKEY,      EX_NOPASSDOOR,   EX_HIDDEN,
   EX_PASSAGE,   EX_PORTAL,      EX_xCLIMB,       EX_xENTER,
   EX_xLEAVE,    EX_xAUTO,       EX_NOFLEE,       EX_xSEARCHABLE,
   EX_BASHED,    EX_BASHPROOF,   EX_NOMOB,        EX_WINDOW,
   EX_xLOOK,     EX_ISBOLT,      EX_BOLTED,       EX_MAX
} exitflags;


/* Uses EXT_BV */
typedef enum
{
   RIS_FIRE,       RIS_WIND,       RIS_EARTH,         RIS_WATER,
   RIS_ICE,        RIS_COLD,       RIS_ELECTRICITY,   RIS_ENERGY,
   RIS_BLUNT,      RIS_PIERCE,     RIS_SLASH,         RIS_ACID,
   RIS_POISON,     RIS_DRAIN,      RIS_SLEEP,         RIS_CHARM,
   RIS_NONMAGIC,   RIS_MAGIC,      RIS_PARALYSIS,     RIS_HOLY,
   RIS_SHADOW,     RIS_ABDAMAGE,   RIS_MAX
} ris_types;

/* Uses EXT_BV */
typedef enum
{
   AFF_NONE,           AFF_BLIND,           AFF_INVISIBLE,       AFF_DETECT_EVIL,
   AFF_DETECT_INVIS,   AFF_DETECT_MAGIC,    AFF_DETECT_HIDDEN,   AFF_SANCTUARY,
   AFF_FAERIE_FIRE,    AFF_INFRARED,        AFF_CURSE,           AFF_POISON,
   AFF_PROTECT,        AFF_PARALYSIS,       AFF_SNEAK,           AFF_HIDE,
   AFF_SLEEP,          AFF_CHARM,           AFF_FLYING,          AFF_PASS_DOOR,
   AFF_FLOATING,       AFF_TRUESIGHT,       AFF_DETECTTRAPS,     AFF_SCRYING,
   AFF_FIRESHIELD,     AFF_SHOCKSHIELD,     AFF_ICESHIELD,       AFF_BERSERK,
   AFF_AQUA_BREATH,    AFF_ACIDMIST,        AFF_VENOMSHIELD,     AFF_DETECT_SNEAK,
   AFF_SILENCE,        AFF_NOMOVE,          AFF_MAX
} affected_by_types;

/* Uses EXT_BV */
typedef enum
{
   ATCK_BITE,         ATCK_CLAWS,           ATCK_TAIL,         ATCK_PUNCH,
   ATCK_KICK,         ATCK_TRIP,            ATCK_BASH,         ATCK_STUN,
   ATCK_GOUGE,        ATCK_BACKSTAB,        ATCK_FEED,         ATCK_DRAIN,
   ATCK_FIREBREATH,   ATCK_FROSTBREATH,     ATCK_ACIDBREATH,   ATCK_LIGHTNBREATH,
   ATCK_GASBREATH,    ATCK_POISON,          ATCK_BLINDNESS,    ATCK_CAUSESERIOUS,
   ATCK_EARTHQUAKE,   ATCK_CAUSECRITICAL,   ATCK_CURSE,        ATCK_FIREBALL,
   ATCK_MAX
} attack_types;

/* Uses EXT_BV */
typedef enum
{
   DFND_PARRY,         DFND_DODGE,          DFND_HEAL,          DFND_CURELIGHT,
   DFND_CURESERIOUS,   DFND_CURECRITICAL,   DFND_DISPELMAGIC,   DFND_DISPELEVIL,
   DFND_SANCTUARY,     DFND_FIRESHIELD,     DFND_SHOCKSHIELD,   DFND_SHIELD,
   DFND_BLESS,         DFND_STONESKIN,      DFND_TELEPORT,      DFND_DISARM,
   DFND_ICESHIELD,     DFND_GRIP,           DFND_TRUESIGHT,     DFND_ACIDMIST,
   DFND_VENOMSHIELD,   DFND_MAX
} defense_types;

/* Uses EXT_BV */
typedef enum
{
   PLR_IS_NPC,     PLR_SHOVEDRAG,    PLR_AUTOEXIT,       PLR_AUTOLOOT,
   PLR_AUTOSAC,    PLR_BLANK,        PLR_BRIEF,          PLR_COMBINE,
   PLR_PROMPT,     PLR_TELNET_GA,    PLR_HOLYLIGHT,      PLR_WIZINVIS,
   PLR_SILENCE,    PLR_NO_EMOTE,     PLR_NO_TELL,        PLR_DENY,
   PLR_FREEZE,     PLR_ANSI,         PLR_NICE,           PLR_FLEE,
   PLR_AUTOGOLD,   PLR_AFK,          PLR_K_LIVE,         PLR_QUESTOR,
   PLR_TELLOFF,    PLR_WHISPEROFF,   PLR_COMPASS,        PLR_SOLO,
   PLR_SUICIDE,    PLR_NOASSIST,     PLR_GROUPAFFECTS,   PLR_NOHINTS,
   PLR_SMARTSAC,   PLR_NOINDUCT,     PLR_AUTOSPLIT,      PLR_NOFINFO,
   PLR_SPARING,    PLR_WILDERNESS,   PLR_MAX
} player_flags;

/* Uses EXT_BV */
typedef enum
{
  ACT_IS_NPC,       ACT_SENTINEL,     ACT_SCAVENGER,   ACT_AGGRESSIVE,
  ACT_STAY_AREA,    ACT_WIMPY,        ACT_PET,         ACT_PRACTICE,
  ACT_IMMORTAL,     ACT_META_AGGR,    ACT_RUNNING,     ACT_MOUNTABLE,
  ACT_MOUNTED,      ACT_SCHOLAR,      ACT_SECRETIVE,   ACT_HARDHAT,
  ACT_MOBINVIS,     ACT_NOASSIST,     ACT_PACIFIST,    ACT_NOATTACK,
  ACT_ANNOYING,     ACT_STATSHIELD,   ACT_PROTOTYPE,   ACT_BANKER,
  ACT_QUESTGIVER,   ACT_UNDERTAKER,   ACT_NOKILL,      ACT_NODEATH,
  ACT_AUTOPURGE,    ACT_WILDERNESS,   ACT_NOSLICE,     ACT_NOBLOOD,
  ACT_MAX
} actflags;

/* Uses EXT_BV */
typedef enum
{
   PCFLAG_DEADLY,    PCFLAG_NORECALL,    PCFLAG_NOINTRO,
   PCFLAG_GAG,       PCFLAG_RETIRED,     PCFLAG_GUEST,      PCFLAG_NOSUMMON,
   PCFLAG_PAGERON,   PCFLAG_NOTITLE,     PCFLAG_GROUPWHO,   PCFLAG_DND,
   PCFLAG_IDLE,      PCFLAG_NOPDELETE,   PCFLAG_PRIVATE,    PCFLAG_ADULT,
   PCFLAG_MAX
} pcflags;

/* Uses EXT_BV */
typedef enum
{
   PART_HEAD,        PART_ARMS,        PART_LEGS,         PART_HEART,
   PART_BRAINS,      PART_GUTS,        PART_HANDS,        PART_FEET,
   PART_FINGERS,     PART_EAR,         PART_EYE,          PART_LONG_TONGUE,
   PART_EYESTALKS,   PART_TENTACLES,   PART_FINS,         PART_WINGS,
   PART_TAIL,        PART_SCALES,      PART_CLAWS,        PART_FANGS,
   PART_HORNS,       PART_TUSKS,       PART_TAILATTACK,   PART_SHARPSCALES,
   PART_BEAK,        PART_HAUNCH,      PART_HOOVES,       PART_PAWS,
   PART_FORELEGS,    PART_FEATHERS,    PART_MAX
} parts;

/* Uses EXT_BV */
typedef enum
{
   ROOM_DARK,           ROOM_DEATH,        ROOM_NO_MOB,         ROOM_INDOORS,
   ROOM_NO_MAGIC,       ROOM_TUNNEL,       ROOM_PRIVATE,        ROOM_SAFE,
   ROOM_SOLITARY,       ROOM_PET_SHOP,     ROOM_NO_RECALL,      ROOM_DONATION,
   ROOM_NODROPALL,      ROOM_SILENCE,      ROOM_LOGSPEECH,      ROOM_NODROP,
   ROOM_STORAGEROOM,    ROOM_NO_SUMMON,    ROOM_NO_ASTRAL,      ROOM_TELEPORT,
   ROOM_TELESHOWDESC,   ROOM_NOFLOOR,      ROOM_NOSUPPLICATE,   ROOM_ARENA,
   ROOM_NOMISSILE,      ROOM_DND,          ROOM_LOCKER,         ROOM_BFS_MARK,
   ROOM_EXPLORER,       ROOM_WILDERNESS,   ROOM_LIGHT,          ROOM_MAX
} roomflags;

/* Since can only be in one position at a time uses a short */
typedef enum
{
   POS_DEAD,       POS_MORTAL,     POS_INCAP,       POS_STUNNED,
   POS_SLEEPING,   POS_BERSERK,    POS_RESTING,     POS_AGGRESSIVE,
   POS_SITTING,    POS_FIGHTING,   POS_DEFENSIVE,   POS_EVASIVE,
   POS_STANDING,   POS_MOUNTED,    POS_SHOVE,       POS_DRAG,
   POS_MAX
} positions;

/* Since can only be one sex at a time uses a short */
typedef enum
{
   SEX_NEUTRAL,   SEX_MALE,   SEX_FEMALE,   SEX_MAX
} sex_types;

typedef enum
{
   ACT_PROG,        SPEECH_PROG,   RAND_PROG,     FIGHT_PROG,       DEATH_PROG,
   HITPRCNT_PROG,   ENTRY_PROG,    GREET_PROG,    ALL_GREET_PROG,   GIVE_PROG,
   BRIBE_PROG,      HOUR_PROG,     TIME_PROG,     WEAR_PROG,        REMOVE_PROG,
   SAC_PROG,        LOOK_PROG,     EXA_PROG,      ZAP_PROG,         GET_PROG,
   DROP_PROG,       DAMAGE_PROG,   REPAIR_PROG,   PULL_PROG,        PUSH_PROG,
   SLEEP_PROG,      REST_PROG,     LEAVE_PROG,    SCRIPT_PROG,      USE_PROG,
   SCRAP_PROG,      OPEN_PROG,     CLOSE_PROG,    PUT_PROG,         MAX_PROG
} prog_types;
