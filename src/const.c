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
 *			     Mud constants module			     *
 *****************************************************************************/

#include <stdio.h>
#include "h/mud.h"

const char *he_she[]  = { "it",    "he",    "she" };
const char *him_her[] = { "it",    "him",   "her" };
const char *his_her[] = { "its",   "his",   "her" };

const char *con_state( short state )
{
   if( state == CON_GET_NAME )              return "Get Name";
   if( state == CON_GET_OLD_PASSWORD )      return "Get Password";
   if( state == CON_CONFIRM_NEW_NAME )      return "Confirm Name";
   if( state == CON_GET_NEW_PASSWORD )      return "New Password";
   if( state == CON_CONFIRM_NEW_PASSWORD )  return "Confirm Password";
   if( state == CON_GET_NEW_SEX )           return "New Sex";
   if( state == CON_READ_MOTD )             return "Read MOTD";
   if( state == CON_GET_NEW_RACE )          return "New Race";
   if( state == CON_GET_NEW_CLASS )         return "New Class";
   if( state == CON_GET_WANT_ANSI )         return "Want ANSI";
   if( state == CON_PRESS_ENTER )           return "Press Enter";
   if( state == CON_COPYOVER_RECOVER )      return "Copyover Recover";
   if( state == CON_GET_PKILL )             return "Get PKill";
   if( state == CON_PLAYING )               return "Playing";
   if( state == CON_EDITING )               return "Editing";
   return "Unknown";
};

const char *perms_flag[] =
{
   "all",   "imm",   "builder",   "leader",   "head",
   "imp",   "max"
};

/* Although these are the same as the perms_flag it was just for simplicty and these are to change the groups for commands */
const char *groups_flag[] =
{
   "all",   "imm",   "builder",   "leader",   "head",
   "imp",   "max"
};

const char *save_flag[] =
{
   "death",     "kill",   "passwd",   "drop",      "put",
   "give",      "auto",   "zap",      "auction",   "get",
   "receive",   "idle",   "fill",     "empty",     "max"
};

const char *stattypes[] =
{
   "strength",   "intelligence",   "wisdom",   "dexterity",   "constitution",
   "charisma",   "luck",           "max"
};

int const lang_array[] =
{
   LANG_COMMON,   LANG_ELVISH,     LANG_DWARVEN,    LANG_PIXIE,   LANG_OGRE,
   LANG_ORCISH,   LANG_TROLLISH,   LANG_HALFLING,   LANG_GITH,    LANG_GNOME,
   LANG_UNKNOWN
};

const char *lang_names[] =
{
   "common",   "elvish",     "dwarven",    "pixie",   "ogre",
   "orcish",   "trollish",   "halfling",   "gith",    "gnome",
   "max"
};

const char *channelflags[] =
{
   "global",    "yell",        "log",        "clan",
   "council",   "tells",       "racetalk",   "whisper",
   "nation",    "classtalk",   "fchat",      "max"
};

const char *spell_flag[] =
{
   "water",          "area",         "distant",     "noself",
   "accumulative",   "recastable",   "canscribe",   "canbrew",
   "group",          "object",       "character",   "secretskill",
   "pksensitive",    "stoponfail",   "nofight",     "nodispel",
   "canmix",         "canconcoct",   "cancarve",    "canimbue",
   "nomount",        "candoall",     "max"
};

const char *spell_saves[] =
{
   "none",     "poison_death",   "wands",   "para_petri",
   "breath",   "spell_staff",    "max"
};

const char *spell_save_effect[] =
{
   "none",      "negate",    "eightdam",   "quarterdam",
   "halfdam",   "3qtrdam",   "reflect",    "absorb",
   "max"
};

const char *spell_damage[] =
{
   "none",     "fire",     "wind",     "earth",
   "water",    "ice",      "cold",     "electricity",
   "energy",   "acid",     "poison",   "drain",
   "holy",     "shadow",   "max"
};

const char *spell_action[] =
{
   "none",      "create",     "destroy",   "resist",
   "suscept",   "divinate",   "obscure",   "change",
   "max"
};

const char *spell_power[] =
{
   "none",   "minor",   "greater",   "major",
   "max"
};

const char *spell_class[] =
{
   "none",     "lunar",   "solar",   "travel",
   "summon",   "life",    "death",   "illusion",
   "max"
};

const char *target_type[] =
{
   "ignore",   "offensive",   "defensive",   "self",
   "objinv",   "max"
};

const char *trap_flags[] =
{
   "none",    "room",      "obj",      "enter",
   "leave",   "open",      "close",    "get",
   "put",     "pick",      "unlock",   "north",
   "south",   "east",      "west",     "up",
   "down",    "examine",   "ne",       "nw",
   "se",      "sw",        "max"
};

const char *cmd_flags[] =
{
   "polymorphed",   "noshow",     "npc",   "pc",
   "allow_tilde",   "fullname",   "max"
};

const char *style_names[] =
{
   "berserk",   "aggressive",   "standard",   "defensive",
   "evasive",   "max"
};

const char *a_types[] =
{
   "none",        "affected",      "resistant",   "weaponspell",
   "wearspell",   "removespell",   "stripsn",     "hit",
   "mana",        "move",          "hitroll",     "damroll",
   "armor",       "stat",          "waitstate",   "max"
};

const char *ris_flags[] =
{
   "fire",       "wind",       "earth",         "water",
   "ice",        "cold",       "electricity",   "energy",
   "blunt",      "pierce",     "slash",         "acid",
   "poison",     "drain",      "sleep",         "charm",
   "nonmagic",   "magic",      "paralysis",     "holy",
   "shadow",     "abdamage",   "max"
};

const char *a_flags[] =
{
   "none",           "blind",          "invisible",       "detect_evil",
   "detect_invis",   "detect_magic",   "detect_hidden",   "sanctuary",
   "faerie_fire",    "infrared",       "curse",           "poison",
   "protect",        "paralysis",      "sneak",           "hide",
   "sleep",          "charm",          "flying",          "pass_door",
   "floating",       "truesight",      "detect_traps",    "scrying",
   "fireshield",     "shockshield",    "iceshield",       "berserk",
   "aqua_breath",    "acidmist",       "venomshield",     "detect_sneak",
   "silence",        "nomove",         "max"
};

const char *attack_flags[] =
{
   "bite",         "claws",           "tail",         "punch",
   "kick",         "trip",            "bash",         "stun",
   "gouge",        "backstab",        "feed",         "drain",
   "firebreath",   "frostbreath",     "acidbreath",   "lightnbreath",
   "gasbreath",    "poison",          "blindness",    "causeserious",
   "earthquake",   "causecritical",   "curse",        "fireball",
   "max"
};

const char *defense_flags[] =
{
   "parry",         "dodge",          "heal",          "curelight",
   "cureserious",   "curecritical",   "dispelmagic",   "dispelevil",
   "sanctuary",     "fireshield",     "shockshield",   "shield",
   "bless",         "stoneskin",      "teleport",      "disarm",
   "iceshield",     "grip",           "truesight",     "acidmist",
   "venomshield",   "max"
};

const char *plr_flags[] =
{
   "npc",        "shovedrag",    "autoexits",      "autoloot",
   "autosac",    "blank",        "brief",          "combine",
   "prompt",     "telnet_ga",    "holylight",      "wizinvis",
   "silence",    "noemote",      "notell",         "deny",
   "freeze",     "ansi",         "nice",           "flee",
   "autogold",   "afk",          "keepalive",      "questor",
   "telloff",    "whisperoff",   "compass",        "solo",
   "suicide",    "noassist",     "groupaffects",   "nohints",
   "smartsac",   "noinduct",     "autosplit",      "noinfo",
   "sparing",    "wilderness",   "max"
};

const char *act_flags[] =
{
   "npc",          "sentinel",     "scavenger",   "aggressive",
   "stayarea",     "wimpy",        "pet",         "practice",
   "immortal",     "meta_aggr",    "running",     "mountable",
   "mounted",      "scholar",      "secretive",   "hardhat",
   "mobinvis",     "noassist",     "pacifist",    "noattack",
   "annoying",     "statshield",   "prototype",   "banker",
   "questgiver",   "undertaker",   "nokill",      "nodeath",
   "autopurge",    "wilderness",   "noslice",     "noblood",
   "max"
};

const char *o_types[] =
{
   "none",            "light",         "scroll",      "wand",
   "staff",           "weapon",        "treasure",    "armor",
   "potion",          "trash",         "container",   "drinkcon",
   "key",             "food",          "money",       "boat",
   "corpse",          "corpse_pc",     "fountain",    "pill",
   "blood",           "bloodstain",    "scraps",      "pipe",
   "herbcon",         "herb",          "fire",        "switch",
   "lever",           "pullchain",     "button",      "trap",
   "map",             "portal",        "paper",       "lockpick",
   "missileweapon",   "projectile",    "quiver",      "shovel",
   "salve",           "cook",          "keyring",     "gem",
   "mortar",          "powder",        "axe",         "wood",
   "fish",            "fishingpole",   "piece",       "furniture",
   "cleaner",         "max"
};

const char *pc_flags[] =
{
   "deadly",   "norecall",    "nointro",
   "gag",      "retired",     "guest",      "nosummon",
   "pager",    "notitled",    "groupwho",   "dnd",
   "idle",     "nopdelete",   "private",    "adult",
   "max"
};

const char *r_flags[] =
{
   "dark",           "death",        "nomob",          "indoors",
   "nomagic",        "tunnel",       "private",        "safe",
   "solitary",       "petshop",      "norecall",       "donation",
   "nodropall",      "silence",      "logspeech",      "nodrop",
   "storageroom",    "nosummon",     "noastral",       "teleport",
   "teleshowdesc",   "nofloor",      "nosupplicate",   "arena",
   "nomissile",      "dnd",          "locker",         "bfs_mark",
   "explorer",       "wilderness",   "light",          "max"
};

const char *ex_flags[] =
{
   "isdoor",      "closed",      "locked",       "secret",
   "swim",        "pickproof",   "fly",          "climb",
   "dig",         "eatkey",      "nopassdoor",   "hidden",
   "passage",     "portal",      "can_climb",    "can_enter",
   "can_leave",   "auto",        "noflee",       "searchable",
   "bashed",      "bashproof",   "nomob",        "window",
   "can_look",    "isbolt",      "bolted",       "max"
};

const char *w_flags[] =
{
   "notake",      "head",       "ears",     "ear",
   "eyes",        "eye",        "face",     "neck",
   "shoulders",   "shoulder",   "about",    "body",
   "back",        "arms",       "arm",      "wrists",
   "wrist",       "hands",      "hand",     "fingers",
   "finger",      "2hands",     "hold",     "waist",
   "legs",        "leg",        "ankles",   "ankle",
   "feet",        "foot",       "max"
};

const char *item_w_flags[] =
{
   /* This actually starts at 0 which is WEAR_HEAD */
   /* -1 */      "head",      "ears",        "ear",
   "ear",        "eyes",      "eye",         "eye",
   "face",       "neck",      "shoulders",   "shoulder",
   "shoulder",   "about",     "body",        "back",
   "arms",       "arm",       "arm",         "wrists",
   "wrist",      "wrist",     "hands",       "hand",
   "hand",       "fingers",   "finger",      "finger",
   "2hand",      "hold",      "hold",        "waist",
   "legs",       "leg",       "leg",         "ankles",
   "ankle",      "ankle",     "feet",        "foot",
   "foot",       "max"
};

const char *wear_locs[] =
{
   "head",      "ears",        "ear1",        "ear2",
   "eyes",      "eye1",        "eye2",        "face",
   "neck",      "shoulders",   "shoulder1",   "shoulder2",
   "about",     "body",        "back",        "arms",
   "arm1",      "arm2",        "wrists",      "wrist1",
   "wrist2",    "hands",       "hand1",       "hand2",
   "fingers",   "finger1",     "finger2",     "2hand",
   "hold1",     "hold2",       "waist",       "legs",
   "leg1",      "leg2",        "ankles",      "ankle1",
   "ankle2",    "feet",        "foot1",       "foot2",
   "max"
};

const char *where_name[] =
{
   "<worn on head>              ",
   "<worn on ears>              ",   "<worn on left ear>          ",   "<worn on right ear>         ",
   "<worn on eyes>              ",   "<worn on left eye>          ",   "<worn on right eye>         ",
   "<worn over face>            ",
   "<worn on neck>              ",
   "<worn on shoulders>         ",   "<worn on left shoulder>     ",   "<worn on right shoulder>    ",
   "<worn about body>           ",
   "<worn on body>              ",
   "<worn on back>              ",
   "<worn on arms>              ",   "<worn on left arm>          ",   "<worn on right arm>         ",
   "<worn around wrists>        ",   "<worn around left wrist>    ",   "<worn around right wrist>   ",
   "<worn on hands>             ",   "<worn on left hand>         ",   "<worn on right hand>        ",
   "<worn on fingers>           ",   "<worn on left ring finger>  ",   "<worn on right ring finger> ",
   "<held in both hands>        ",   "<held in left hand>         ",   "<held in right hand>        ",
   "<worn around waist>         ",
   "<worn on legs>              ",   "<worn on left leg>          ",   "<worn on right leg>         ",
   "<worn around ankles>        ",   "<worn around left ankle>    ",   "<worn around right ankle>   ",
   "<worn on feet>              ",   "<worn on left foot>         ",   "<worn on right foot>        "
};

const char *lodge_name[] =
{ /* Not all of the positions are used, be sure to show the ones that are used */
   "<lodged in head>            ",
   "                            ",   "<lodged in left ear>        ",   "<lodged in right ear>       ",
   "                            ",   "<lodged in left eye>        ",   "<lodged in right eye>       ",
   "<lodged in face>            ",
   "<lodged in neck>            ",
   "                            ",   "<lodged in left shoulder>   ",   "<lodged in right shoulder>  ",
   "                            ",
   "<lodged in body>            ",
   "<lodged in back>            ",
   "                            ",   "<lodged in left arm>        ",   "<lodged in right arm>       ",
   "                            ",   "<lodged in left wrist>      ",   "<lodged in right wrist>     ",
   "                            ",   "<lodged in left hand>       ",   "<lodged in right hand>      ",
   "                            ",   "                            ",   "                            ",
   "                            ",   "                            ",   "                            ",
   "<lodged in waist>           ",
   "                            ",   "<lodged in left leg>        ",   "<lodged in right leg>       ",
   "                            ",   "<lodged in left ankle>      ",   "<lodged in right ankle>     ",
   "                            ",   "<lodged in left foot>       ",   "<lodged in right foot>      "
};

const char *o_flags[] =
{
   "glow",          "loyal",            "invis",        "magic",
   "nodrop",        "bless",            "antigood",     "antievil",
   "antineutral",   "noremove",         "inventory",    "organic",
   "metal",         "donation",         "clanobject",   "clancorpse",
   "hidden",        "poisoned",         "covering",     "deathrot",
   "buried",        "prototype",        "nolocate",     "groundrot",
   "pkdisarmed",    "noscrap",          "enchanted",    "quest",
   "lodged",        "pierced",          "random",       "nocontainer",
   "nogroup",       "continuousfire",   "ptrap",        "wilderness",
   "max"
};

const char *area_flags[] =
{
   "nopkill",   "freekill",   "noteleport",   "prototype",
   "noquest",   "noshow",     "noload",       "max"
};

const char *sect_flags[] =
{
   "inside",       "city",          "field",        "forest",
   "hills",        "mountain",      "water_swim",   "water_noswim",
   "underwater",   "air",           "desert",       "dunno",
   "oceanfloor",   "underground",   "lava",         "swamp",
   "max"
};

const char *pos_names[] =
{
   "dead",       "mortal",     "incap",       "stunned",
   "sleeping",   "berserk",    "resting",     "aggressive",
   "sitting",    "fighting",   "defensive",   "evasive",
   "standing",   "mounted",    "shove",       "drag",
   "max"
};

const char *sex_names[] =
{
   "neutral",   "male",   "female",   "max"
};

const char *prog_names[] =
{
   "act",        "speech",   "rand",     "fight",      "death",
   "hitprcnt",   "entry",    "greet",    "allgreet",   "give",
   "bribe",      "hour",     "time",     "wear",       "remove",
   "sac",        "look",     "exa",      "zap",        "get",
   "drop",       "damage",   "repair",   "pull",       "push",
   "sleep",      "rest",     "leave",    "script",     "use",
   "scrap",      "open",     "close",    "put",        "max"
};

/* Part things */
const char *part_flags[] =
{
   "head",        "arms",        "legs",         "heart",
   "brains",      "guts",        "hands",        "feet",
   "fingers",     "ear",         "eye",          "long_tongue",
   "eyestalks",   "tentacles",   "fins",         "wings",
   "tail",        "scales",      "claws",        "fangs",
   "horns",       "tusks",       "tailattack",   "sharpscales",
   "beak",        "haunches",    "hooves",       "paws",
   "forelegs",    "feathers",    "max"
};

/* Vnums for the various bodyparts */
int part_vnums[] =
{
   12, /* Head */        14, /* arms */        15, /* legs */         13, /* heart */
   44, /* brains */      16, /* guts */        45, /* hands */        46, /* feet */
   47, /* fingers */     48, /* ear */         49, /* eye */          50, /* long_tongue */
   51, /* eyestalks */   52, /* tentacles */   53, /* fins */         54, /* wings */
   55, /* tail */        56, /* scales */      59, /* claws */        87, /* fangs */
   58, /* horns */       57, /* tusks */       55, /* tailattack */   85, /* sharpscales */
   84, /* beak */        86, /* haunches */    83, /* hooves */       82, /* paws */
   81, /* forelegs */    80, /* feathers */     0   /* max */
};

/* Messages for flinging off the various bodyparts */
const char *part_messages[] =
{
   "$n's severed head plops from its neck.",
   "$n's arm is sliced from $s dead body.",
   "$n's leg is sliced from $s dead body.",
   "$n's heart is torn from $s chest.",
   "$n's brains spill grotesquely from $s head.",
   "$n's guts spill grotesquely from $s torso.",
   "$n's hand is sliced from $s dead body.",
   "$n's foot is sliced from $s dead body.",
   "A finger is sliced from $n's dead body.",
   "$n's ear is sliced from $s dead body.",
   "$n's eye is gouged from its socket.",
   "$n's tongue is torn from $s mouth.",
   "An eyestalk is sliced from $n's dead body.",
   "A tentacle is severed from $n's dead body.",
   "A fin is sliced from $n's dead body.",
   "A wing is severed from $n's dead body.",
   "$n's tail is sliced from $s dead body.",
   "A scale falls from the body of $n.",
   "A claw is torn from $n's dead body.",
   "$n's fangs are torn from $s mouth.",
   "A horn is wrenched from the body of $n.",
   "$n's tusk is torn from $s dead body.",
   "$n's tail is sliced from $s dead body.",
   "A ridged scale falls from the body of $n.",
   "$n's beak is sliced from $s dead body.",
   "$n's haunches are sliced from $s dead body.",
   "A hoof is sliced from $n's dead body.",
   "A paw is sliced from $n's dead body.",
   "$n's foreleg is sliced from $s dead body.",
   "Some feathers fall from $n's dead body.",
   "Max Part Message"
};

int get_flag( char *flag, const char *flagarray[], int max )
{
   int x;

   if( !flag || flag[0] == '\0' || !flagarray )
      return -1;
   for( x = 0; x < max; x++ )
   {
      if( !str_cmp( flag, flagarray[x] ) )
         return x;
      /* This check can possibly slow the mud down a bit more...comment it out if it does */
      /* Normaly in things that use get_flag I end it with a max so why not make use of it to */
      if( !str_cmp( "max", flagarray[x] ) )
      {
         bug( "%s: max found at %d when max is %d, breaking out to avoid possible crash.", __FUNCTION__, x, max );
         break;
      }
   }
   return -1;
}

int get_langflag( char *flag )
{
   unsigned int x;

   for( x = 0; lang_array[x] != LANG_UNKNOWN; x++ )
      if( !str_cmp( flag, lang_names[x] ) )
         return lang_array[x];
   return LANG_UNKNOWN;
}

int get_langnum( char *flag )
{
   unsigned int x;

   for( x = 0; lang_array[x] != LANG_UNKNOWN; x++ )
      if( !str_cmp( flag, lang_names[x] ) )
         return x;
   return -1;
}

/*
 * Liquid properties.
 * Used in #OBJECT section of area file.
 */
const struct liq_type liq_table[LIQ_MAX] =
{
   {(char *)"unknown", (char *)"unknown", {0, 0, 0}}, /* 0 */
   {(char *)"water", (char *)"clear", {0, 1, 10}},  /*  1 */
   {(char *)"beer", (char *)"amber", {3, 2, 5}},
   {(char *)"wine", (char *)"rose", {5, 2, 5}},
   {(char *)"ale", (char *)"brown", {2, 2, 5}},
   {(char *)"dark ale", (char *)"dark", {1, 2, 5}}, /* 5 */
   {(char *)"whisky", (char *)"golden", {6, 1, 4}},
   {(char *)"lemonade", (char *)"pink", {0, 1, 8}},
   {(char *)"firebreather", (char *)"boiling", {10, 0, 0}},
   {(char *)"local specialty", (char *)"everclear", {3, 3, 3}},
   {(char *)"slime mold juice", (char *)"green", {0, 4, -8}}, /* 10 */
   {(char *)"milk", (char *)"white", {0, 3, 6}},
   {(char *)"tea", (char *)"tan", {0, 1, 6}},
   {(char *)"coffee", (char *)"black", {0, 1, 6}},
   {(char *)"blood", (char *)"red", {0, 2, -1}},
   {(char *)"salt water", (char *)"clear", {0, 1, -2}}, /* 15 */
   {(char *)"cola", (char *)"cherry", {0, 1, 5}},
   {(char *)"mead", (char *)"honey color", {4, 2, 5}},
   {(char *)"grog", (char *)"thick brown", {3, 2, 5}}  /* 18 */
};

const char *s_blade_messages[24] =
{
   "miss",        "barely scratch",   "scratch",     "nick",         "cut",
   "hit",         "tear",             "rip",         "gash",         "lacerate",
   "hack",        "maul",             "rend",        "decimate",     "mangle",
   "devastate",   "cleave",           "butcher",     "disembowel",   "disfigure",
   "gut",         "eviscerate",       "slaughter",   "annihilate"
};

const char *p_blade_messages[24] =
{
   "misses",       "barely scratches",   "scratches",    "nicks",         "cuts",
   "hits",         "tears",              "rips",         "gashes",        "lacerates",
   "hacks",        "mauls",              "rends",        "decimates",     "mangles",
   "devastates",   "cleaves",            "butchers",     "disembowels",   "disfigures",
   "guts",         "eviscerates",        "slaughters",   "annihilates"
};

const char *s_blunt_messages[24] =
{
   "miss",        "barely scuff",   "scuff",        "pelt",       "bruise",
   "strike",      "thrash",         "batter",       "flog",       "pummel",
   "smash",       "maul",           "bludgeon",     "decimate",   "shatter",
   "devastate",   "maim",           "cripple",      "mutilate",   "disfigure",
   "massacre",    "pulverize",      "obliterate",   "annihilate"
};

const char *p_blunt_messages[24] =
{
   "misses",       "barely scuffs",   "scuffs",        "pelts",       "bruises",
   "strikes",      "thrashes",        "batters",       "flogs",       "pummels",
   "smashes",      "mauls",           "bludgeons",     "decimates",   "shatters",
   "devastates",   "maims",           "cripples",      "mutilates",   "disfigures",
   "massacres",    "pulverizes",      "obliterates",   "annihilates"
};

const char *s_generic_messages[24] =
{
   "miss",      "brush",        "scratch",      "graze",        "nick",
   "jolt",      "wound",        "injure",       "hit",          "jar",
   "thrash",    "maul",         "decimate",     "traumatize",   "devastate",
   "maim",      "demolish",     "mutilate",     "massacre",     "pulverize",
   "destroy",   "obliterate",   "annihilate",   "smite"
};

const char *p_generic_messages[24] =
{
   "misses",     "brushes",       "scratches",     "grazes",        "nicks",
   "jolts",      "wounds",        "injures",       "hits",          "jars",
   "thrashes",   "mauls",         "decimates",     "traumatizes",   "devastates",
   "maims",      "demolishes",    "mutilates",     "massacres",     "pulverizes",
   "destroys",   "obliterates",   "annihilates",   "smites"
};

const char *attack_table[DAM_MAX] =
{
   "hit",     "slice",   "stab",     "slash",
   "whip",    "claw",    "blast",    "pound",
   "crush",   "bite",    "pierce",   "suction",
   "bolt",    "arrow",   "dart",     "stone",
   "pea"
};

const char **const s_message_table[DAM_MAX] =
{
   s_generic_messages,  /* hit */
   s_blade_messages,    /* slice */
   s_blade_messages,    /* stab */
   s_blade_messages,    /* slash */
   s_blunt_messages,    /* whip */
   s_blade_messages,    /* claw */
   s_generic_messages,  /* blast */
   s_blunt_messages,    /* pound */
   s_blunt_messages,    /* crush */
   s_blade_messages,    /* bite */
   s_blade_messages,    /* pierce */
   s_blunt_messages,    /* suction */
   s_generic_messages,  /* bolt */
   s_generic_messages,  /* arrow */
   s_generic_messages,  /* dart */
   s_generic_messages,  /* stone */
   s_generic_messages   /* pea */
};

const char **const p_message_table[DAM_MAX] =
{
   p_generic_messages,  /* hit */
   p_blade_messages,    /* slice */
   p_blade_messages,    /* stab */
   p_blade_messages,    /* slash */
   p_blunt_messages,    /* whip */
   p_blade_messages,    /* claw */
   p_generic_messages,  /* blast */
   p_blunt_messages,    /* pound */
   p_blunt_messages,    /* crush */
   p_blade_messages,    /* bite */
   p_blade_messages,    /* pierce */
   p_blunt_messages,    /* suction */
   p_generic_messages,  /* bolt */
   p_generic_messages,  /* arrow */
   p_generic_messages,  /* dart */
   p_generic_messages,  /* stone */
   p_generic_messages   /* pea */
};

/* Weather constants - FB */
const char *temp_settings[MAX_CLIMATE] =
{
   "cold",   "cool",   "normal",   "warm",   "hot"
};

const char *precip_settings[MAX_CLIMATE] =
{
   "arid",   "dry",   "normal",   "damp",   "wet"
};

const char *wind_settings[MAX_CLIMATE] =
{
   "still",   "calm",   "normal",   "breezy",   "windy"
};

const char *preciptemp_msg[6][6] =
{
   /* precip = 0 */
   {
      "Frigid temperatures settle over the land",
      "It is bitterly cold",
      "The weather is crisp and dry",
      "A comfortable warmth sets in",
      "A dry heat warms the land",
      "Seething heat bakes the land"
   },
   /* precip = 1 */
   {
      "A few flurries drift from the high clouds",
      "Frozen drops of rain fall from the sky",
      "An occasional raindrop falls to the ground",
      "Mild drops of rain seep from the clouds",
      "It is very warm, and the sky is overcast",
      "High humidity intensifies the seering heat"
   },
   /* precip = 2 */
   {
      "A brief snow squall dusts the earth",
      "A light flurry dusts the ground",
      "Light snow drifts down from the heavens",
      "A light drizzle mars an otherwise perfect day",
      "A few drops of rain fall to the warm ground",
      "A light rain falls through the sweltering sky"
   },
   /* precip = 3 */
   {
      "Snowfall covers the frigid earth",
      "Light snow falls to the ground",
      "A brief shower moistens the crisp air",
      "A pleasant rain falls from the heavens",
      "The warm air is heavy with rain",
      "A refreshing shower eases the oppresive heat"
   },
   /* precip = 4 */
   {
      "Sleet falls in sheets through the frosty air",
      "Snow falls quickly, piling upon the cold earth",
      "Rain pelts the ground on this crisp day",
      "Rain drums the ground rythmically",
      "A warm rain drums the ground loudly",
      "Tropical rain showers pelt the seering ground"
   },
   /* precip = 5 */
   {
      "A downpour of frozen rain covers the land in ice",
      "A blizzard blankets everything in pristine white",
      "Torrents of rain fall from a cool sky",
      "A drenching downpour obscures the temperate day",
      "Warm rain pours from the sky",
      "A torrent of rain soaks the heated earth"
   }
};

const char *windtemp_msg[6][6] =
{
   /* wind = 0 */
   {
      "The frigid air is completely still",
      "A cold temperature hangs over the area",
      "The crisp air is eerily calm",
      "The warm air is still",
      "No wind makes the day uncomfortably warm",
      "The stagnant heat is sweltering"
   },
   /* wind = 1 */
   {
      "A light breeze makes the frigid air seem colder",
      "A stirring of the air intensifies the cold",
      "A touch of wind makes the day cool",
      "It is a temperate day, with a slight breeze",
      "It is very warm, the air stirs slightly",
      "A faint breeze stirs the feverish air"
   },
   /* wind = 2 */
   {
      "A breeze gives the frigid air bite",
      "A breeze swirls the cold air",
      "A lively breeze cools the area",
      "It is a temperate day, with a pleasant breeze",
      "Very warm breezes buffet the area",
      "A breeze ciculates the sweltering air"
   },
   /* wind = 3 */
   {
      "Stiff gusts add cold to the frigid air",
      "The cold air is agitated by gusts of wind",
      "Wind blows in from the north, cooling the area",
      "Gusty winds mix the temperate air",
      "Brief gusts of wind punctuate the warm day",
      "Wind attempts to cut the sweltering heat"
   },
   /* wind = 4 */
   {
      "The frigid air whirls in gusts of wind",
      "A strong, cold wind blows in from the north",
      "Strong wind makes the cool air nip",
      "It is a pleasant day, with gusty winds",
      "Warm, gusty winds move through the area",
      "Blustering winds punctuate the seering heat"
   },
   /* wind = 5 */
   {
      "A frigid gale sets bones shivering",
      "Howling gusts of wind cut the cold air",
      "An angry wind whips the air into a frenzy",
      "Fierce winds tear through the tepid air",
      "Gale-like winds whip up the warm air",
      "Monsoon winds tear the feverish air"
   }
};

const char *precip_msg[3] =
{
   "there is not a cloud in the sky",
   "pristine white clouds are in the sky",
   "thick, gray clouds mask the sun"
};

const char *wind_msg[6] =
{
   "there is not a breath of wind in the air",
   "a slight breeze stirs the air",
   "a breeze wafts through the area",
   "brief gusts of wind punctuate the air",
   "angry gusts of wind blow",
   "howling winds whip the air into a frenzy"
};

/* See if character is holding a type of eq */
OBJ_DATA *get_eq_hold( CHAR_DATA *ch, int type )
{
   OBJ_DATA *obj, *maxobj = NULL;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( ( obj->wear_loc == WEAR_HOLD_B || obj->wear_loc == WEAR_HOLD_L
      || obj->wear_loc == WEAR_HOLD_R ) && obj->item_type == type )
      {
         if( !obj->pIndexData->layers )
            return obj;
         else if( !maxobj || obj->pIndexData->layers > maxobj->pIndexData->layers )
            maxobj = obj;
      }
   }
   return maxobj;
}

/* This one is used incase you want to get another hold item then the other one */
OBJ_DATA *get_next_eq_hold( CHAR_DATA *ch, OBJ_DATA *oobj, int type )
{
   OBJ_DATA *obj, *maxobj = NULL;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( oobj && oobj == obj )
         continue;
      if( ( obj->wear_loc == WEAR_HOLD_B || obj->wear_loc == WEAR_HOLD_L
      || obj->wear_loc == WEAR_HOLD_R ) && obj->item_type == type )
      {
         if( !obj->pIndexData->layers )
            return obj;
         else if( !maxobj || obj->pIndexData->layers > maxobj->pIndexData->layers )
            maxobj = obj;
      }
   }
   return maxobj;
}

int get_ac( CHAR_DATA *ch )
{
   if( !ch )
      return 0;
   return ( ch->armor + ( is_awake( ch ) ? ( get_curr_dex( ch ) / 5 ) : 0 ) );
}

int get_hitroll( CHAR_DATA *ch )
{
   int bonus;

   if( !ch )
      return 0;
   bonus = UMAX( 0, ( get_curr_dex( ch ) - ( get_curr_str( ch ) / 2 ) ) );
   return ( ch->hitroll + bonus + ( 2 - ( abs( ch->mental_state ) / 10 ) ) );
}

int get_damroll( CHAR_DATA *ch )
{
   int bonus;

   if( !ch )
      return 0;
   bonus = UMAX( 0, ( get_curr_str( ch ) - ( get_curr_dex( ch ) / 2 ) ) );
   return ( ch->damroll + bonus + ( 2 - ( abs( ch->mental_state ) / 10 ) ) );
}

void code_check( void )
{
    int x;

    if( ( x = ( sizeof( perms_flag ) / sizeof( perms_flag[0] ) ) - 1 ) != PERM_MAX )
       bug( "perms_flag = %d when it should be %d\r\n", x, PERM_MAX );
    if( ( x = ( sizeof( save_flag ) / sizeof( save_flag[0] ) ) - 1 ) != SV_MAX )
       bug( "save_flag = %d when it should be %d\r\n", x, SV_MAX );
    if( ( x = ( sizeof( stattypes ) / sizeof( stattypes[0] ) ) - 1 ) != STAT_MAX )
       bug( "stattypes = %d when it should be %d\r\n", x, STAT_MAX );
    if( ( x = ( sizeof( channelflags ) / sizeof( channelflags[0] ) ) - 1 ) != CHANNEL_MAX )
       bug( "channelflags = %d when it should be %d\r\n", x, CHANNEL_MAX );
    if( ( x = ( sizeof( spell_flag ) / sizeof( spell_flag[0] ) ) - 1 ) != SF_MAX )
       bug( "spell_flag = %d when it should be %d\r\n", x, SF_MAX );
    if( ( x = ( sizeof( spell_saves ) / sizeof( spell_saves[0] ) ) - 1 ) != SS_MAX )
       bug( "spell_saves = %d when it should be %d\r\n", x, SS_MAX );
    if( ( x = ( sizeof( spell_damage ) / sizeof( spell_damage[0] ) ) - 1 ) != SD_MAX )
       bug( "spell_damage = %d when it should be %d\r\n", x, SD_MAX );
    if( ( x = ( sizeof( spell_action ) / sizeof( spell_action[0] ) ) - 1 ) != SA_MAX )
       bug( "spell_action = %d when it should be %d\r\n", x, SA_MAX );
    if( ( x = ( sizeof( spell_power ) / sizeof( spell_power[0] ) ) - 1 ) != SP_MAX )
       bug( "spell_power = %d when it should be %d\r\n", x, SP_MAX );
    if( ( x = ( sizeof( spell_class ) / sizeof( spell_class[0] ) ) - 1 ) != SC_MAX )
       bug( "spell_class = %d when it should be %d\r\n", x, SC_MAX );
    if( ( x = ( sizeof( spell_save_effect ) / sizeof( spell_save_effect[0] ) ) - 1 ) != SE_MAX )
       bug( "spell_save_effect = %d when it should be %d\r\n", x, SE_MAX );
    if( ( x = ( sizeof( target_type ) / sizeof( target_type[0] ) ) - 1 ) != TAR_MAX )
       bug( "target_type = %d when it should be %d\r\n", x, TAR_MAX );
    if( ( x = ( sizeof( trap_flags ) / sizeof( trap_flags[0] ) ) - 1 ) != TRAP_MAX )
       bug( "trap_flags = %d when it should be %d\r\n", x, TRAP_MAX );
    if( ( x = ( sizeof( cmd_flags ) / sizeof( cmd_flags[0] ) ) - 1 ) != CMD_MAX )
       bug( "cmd_flags = %d when it should be %d\r\n", x, CMD_MAX );
    if( ( x = ( sizeof( style_names ) / sizeof( style_names[0] ) ) - 1 ) != STYLE_MAX )
       bug( "style_names = %d when it should be %d\r\n", x, STYLE_MAX );
    if( ( x = ( sizeof( a_types ) / sizeof( a_types[0] ) ) - 1 ) != APPLY_MAX )
       bug( "a_types = %d when it should be %d\r\n", x, APPLY_MAX );
    if( ( x = ( sizeof( ris_flags ) / sizeof( ris_flags[0] ) ) - 1 ) != RIS_MAX )
       bug( "ris_flags = %d when it should be %d\r\n", x, RIS_MAX );
    if( ( x = ( sizeof( a_flags ) / sizeof( a_flags[0] ) ) - 1 ) != AFF_MAX )
       bug( "a_flags = %d when it should be %d\r\n", x, AFF_MAX );
    if( ( x = ( sizeof( attack_flags ) / sizeof( attack_flags[0] ) ) - 1 ) != ATCK_MAX )
       bug( "attack_flags = %d when it should be %d\r\n", x, ATCK_MAX );
    if( ( x = ( sizeof( defense_flags ) / sizeof( defense_flags[0] ) ) - 1 ) != DFND_MAX )
       bug( "defense_flags = %d when it should be %d\r\n", x, DFND_MAX );
    if( ( x = ( sizeof( plr_flags ) / sizeof( plr_flags[0] ) ) - 1 ) != PLR_MAX )
       bug( "plr_flags = %d when it should be %d\r\n", x, PLR_MAX );
    if( ( x = ( sizeof( act_flags ) / sizeof( act_flags[0] ) ) - 1 ) != ACT_MAX )
       bug( "act_flags = %d when it should be %d\r\n", x, ACT_MAX );
    if( ( x = ( sizeof( o_types ) / sizeof( o_types[0] ) ) - 1 ) != ITEM_TYPE_MAX )
       bug( "o_types = %d when it should be %d\r\n", x, ITEM_TYPE_MAX );
    if( ( x = ( sizeof( pc_flags ) / sizeof( pc_flags[0] ) ) - 1 ) != PCFLAG_MAX )
       bug( "pc_flags = %d when it should be %d\r\n", x, PCFLAG_MAX );
    if( ( x = ( sizeof( r_flags ) / sizeof( r_flags[0] ) ) - 1 ) != ROOM_MAX )
       bug( "r_flags = %d when it should be %d\r\n", x, ROOM_MAX );
    if( ( x = ( sizeof( ex_flags ) / sizeof( ex_flags[0] ) ) - 1 ) != EX_MAX )
       bug( "ex_flags = %d when it should be %d\r\n", x, EX_MAX );
    /* Yea I know these two are switched lol */
    if( ( x = ( sizeof( w_flags ) / sizeof( w_flags[0] ) ) - 1 ) != ITEM_WEAR_MAX )
       bug( "w_flags = %d when it should be %d\r\n", x, ITEM_WEAR_MAX );
    if( ( x = ( sizeof( item_w_flags ) / sizeof( item_w_flags[0] ) ) - 1 ) != WEAR_MAX )
       bug( "item_w_flags = %d when it should be %d\r\n", x, WEAR_MAX );
    if( ( x = ( sizeof( wear_locs ) / sizeof( wear_locs[0] ) ) - 1 ) != WEAR_MAX )
       bug( "wear_locs = %d when it should be %d\r\n", x, WEAR_MAX );
    /* Could also use WEAR_MAX for these two, since they don't have a max don't subtract 1 */
    if( ( x = ( sizeof( where_name ) / sizeof( where_name[0] ) ) ) != MAX_WHERE_NAME )
       bug( "where_name = %d when it should be %d\r\n", x, MAX_WHERE_NAME );
    if( ( x = ( sizeof( lodge_name ) / sizeof( lodge_name[0] ) ) ) != MAX_WHERE_NAME )
       bug( "lodge_name = %d when it should be %d\r\n", x, MAX_WHERE_NAME );
    if( ( x = ( sizeof( o_flags ) / sizeof( o_flags[0] ) ) - 1 ) != ITEM_MAX )
       bug( "o_flags = %d when it should be %d\r\n", x, ITEM_MAX );
    if( ( x = ( sizeof( area_flags ) / sizeof( area_flags[0] ) ) - 1 ) != AFLAG_MAX )
       bug( "area_flags = %d when it should be %d\r\n", x, AFLAG_MAX );
    if( ( x = ( sizeof( sect_flags ) / sizeof( sect_flags[0] ) ) - 1 ) != SECT_MAX )
       bug( "sect_flags = %d when it should be %d\r\n", x, SECT_MAX );
    if( ( x = ( sizeof( pos_names ) / sizeof( pos_names[0] ) ) - 1 ) != POS_MAX )
       bug( "pos_names = %d when it should be %d\r\n", x, POS_MAX );
    if( ( x = ( sizeof( sex_names ) / sizeof( sex_names[0] ) ) - 1 ) != SEX_MAX )
       bug( "sex_names = %d when it should be %d\r\n", x, SEX_MAX );
    if( ( x = ( sizeof( part_flags ) / sizeof( part_flags[0] ) ) - 1 ) != PART_MAX )
       bug( "part_flags = %d when it should be %d\r\n", x, PART_MAX );
    if( ( x = ( sizeof( part_vnums ) / sizeof( part_vnums[0] ) ) - 1 ) != PART_MAX )
       bug( "part_vnums = %d when it should be %d\r\n", x, PART_MAX );
    if( ( x = ( sizeof( part_messages ) / sizeof( part_messages[0] ) ) - 1 ) != PART_MAX )
       bug( "part_messages = %d when it should be %d\r\n", x, PART_MAX );
}
