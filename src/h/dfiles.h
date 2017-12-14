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

/* Data files used by the server. */
#define PLAYER_DIR	"player/"                  /* Player files */
#define BACKUP_DIR      "backup/"                  /* Backup directory */

#define GOD_DIR		"gods/"                    /* God Info Dir */

#define BOARD_DIR       "boards/"                  /* Board data dir */
#define BOARD_FILE	BOARD_DIR "boards.dat"     /* For bulletin boards */

#define CLAN_DIR        "clans/"                   /* Clan data dir */
#define CLAN_LIST	CLAN_DIR "clan.lst"        /* List of clans */

#define COUNCIL_DIR     "councils/"                /* Council data dir */
#define COUNCIL_LIST	COUNCIL_DIR "council.lst"  /* List of councils */

#define DEITY_DIR       "deity/"                   /* Deity data dir */
#define DEITY_LIST	DEITY_DIR   "deity.lst"    /* List of deities */

#define BUILD_DIR       "building/"                /* Online building save dir     */

#define SYSTEM_DIR         "system/"                      /* Main system files */
#define TIMEINFO_FILE      SYSTEM_DIR "timeinfo.dat"      /* Time info */
#define CALENDARINFO_FILE  SYSTEM_DIR "calendarinfo.dat"  /* Calendar info */
#define HELP_FILE          SYSTEM_DIR "help.dat"          /* Help files */
#define SHUTDOWN_FILE	   SYSTEM_DIR "shutdown.dat"      /* For 'shutdown' */
#define ANSITITLE_FILE	   SYSTEM_DIR "mudtitle.ans"
#define ASCTITLE_FILE	   SYSTEM_DIR "mudtitle.asc"
#define BOOTLOG_FILE	   SYSTEM_DIR "boot.dat"          /* Boot up error file  */
#define LOG_FILE           SYSTEM_DIR "log.dat"           /* For talking in logged rooms */
#define MOBLOG_FILE        SYSTEM_DIR "moblog.dat"        /* For mplog messages */
#define WIZLIST_FILE       SYSTEM_DIR "WIZLIST"           /* Wizlist */
#define SKILL_FILE         SYSTEM_DIR "skills.dat"        /* Skill table */
#define HERB_FILE          SYSTEM_DIR "herbs.dat"         /* Herb table */
#define PERSONAL_FILE      SYSTEM_DIR "personal.dat"      /* Personal table */
#define TONGUE_FILE        SYSTEM_DIR "tongues.dat"       /* Tongue tables */
#define SOCIAL_FILE        SYSTEM_DIR "socials.dat"       /* Socials */
#define COMMAND_FILE       SYSTEM_DIR "commands.dat"      /* Commands */
#define BAN_LIST           SYSTEM_DIR "ban.lst"           /* List of bans */
#define RESERVED_LIST	   SYSTEM_DIR "reserved.lst"      /* List of reserved names */
#define MORPH_FILE         SYSTEM_DIR "morph.dat"         /* For morph data */
#define HOSTLOG_FILE       SYSTEM_DIR "hostlogs.dat"      /* For all hostlogs information */
#define HACKED_FILE        SYSTEM_DIR "hacked.dat"        /* For all hacked attempts */

#define PROG_DIR        "mudprogs/"                /* MUDProg files */
#define CORPSE_DIR      "corpses/"                 /* Corpses */

#define CLASS_DIR       "classes/"                 /* Classes */
#define CLASS_LIST	CLASS_DIR   "class.lst"    /* List of classes */

#define RACE_DIR        "races/"                   /* Races */
#define RACE_LIST	RACE_DIR    "race.lst"     /* List of races */

#define AREA_DIR        "area/"
#define AREA_LIST	AREA_DIR "area.lst"        /* List of areas */

#define LOCKER_DIR      "lockers/"                 /* Lockers */

#define STORAGE_DIR     "storages/"                /* Storages */
