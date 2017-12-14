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
 * 			Database management module			     *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#if !defined(WIN32)
   #include <dlfcn.h>
#else
   #include <windows.h>
   #define dlopen( libname, flags ) LoadLibrary( (libname) )
#endif
#include "h/mud.h"
#include "h/mssp.h"

#define WEATHER_FILE SYSTEM_DIR "weather.dat"

void check_all_resets( void );
void free_track_resets( OBJ_INDEX_DATA *oindex, MOB_INDEX_DATA *mindex );
bool check_area_vnum_conflicts( int vnum );

/* Globals. */
SHOP_DATA *first_shop, *last_shop;
REPAIR_DATA *first_repair, *last_repair;
CHAR_DATA *first_char, *last_char;
GROUP_DATA *first_group, *last_group;
OBJ_DATA *first_object, *last_object;
OBJ_DATA *first_corpse, *last_corpse;
AREA_DATA *first_area, *last_area;
AREA_DATA *first_area_name, *last_area_name;   /*Used for alphanum. sort */
AREA_DATA *first_build, *last_build;
AREA_DATA *first_asort, *last_asort;
AREA_DATA *first_bsort, *last_bsort;

EXTRACT_CHAR_DATA *extracted_char_queue;
time_t last_restore_all_time = 0;

int weath_unit;   /* global weather param */
int rand_factor;
int climate_factor;
int neigh_factor;
int max_vector;

int cur_qchars;
int nummobsloaded;
int numobjsloaded;
int physicalobjects;

time_t mud_start_time;

bool MOBtrigger;
bool DONT_UPPER;

int gsn_taste;
int gsn_smell;
int gsn_barehanded;
int gsn_pugilism;
int gsn_long_blades;
int gsn_short_blades;
int gsn_flexible_arms;
int gsn_talonous_arms;
int gsn_bludgeons;
int gsn_missile_weapons;
int gsn_detrap;
int gsn_trapset;
int gsn_backstab;
int gsn_circle;
int gsn_dodge;
int gsn_duck;
int gsn_block;
int gsn_shieldblock;
int gsn_counter;
int gsn_hide;
int gsn_peek;
int gsn_pick_lock;
int gsn_sneak;
int gsn_steal;
int gsn_gouge;
int gsn_poison_weapon;
int gsn_disarm;
int gsn_enhanced_damage;
int gsn_kick;
int gsn_parry;
int gsn_rescue;
int gsn_punch;
int gsn_bash;
int gsn_stun;
int gsn_bashdoor;
int gsn_grip;
int gsn_berserk;
int gsn_tumble;
int gsn_feed;
int gsn_bloodlet;
int gsn_chop;
int gsn_makefire;
int gsn_aid;
int gsn_track;
int gsn_search;
int gsn_dig;
int gsn_mount;
int gsn_bite;
int gsn_claw;
int gsn_tail;
int gsn_scribe;
int gsn_brew;
int gsn_imbue;
int gsn_concoct;
int gsn_carve;
int gsn_mix;
int gsn_climb;
int gsn_cook;
int gsn_scan;
int gsn_slice;
int gsn_aqua_breath;
int gsn_blindness;
int gsn_charm_person;
int gsn_curse;
int gsn_invis;
int gsn_poison;
int gsn_sleep;
int gsn_lightning_bolt;
int gsn_common;

/* for searching */
int gsn_first_spell;
int gsn_first_skill;
int gsn_first_weapon;
int gsn_first_tongue;
int gsn_top_sn;

/* For styles?  Trying to rebuild from some kind of accident here - Blod */
int gsn_style_evasive;
int gsn_style_defensive;
int gsn_style_standard;
int gsn_style_aggressive;
int gsn_style_berserk;

/* Locals. */
MOB_INDEX_DATA *mob_index_hash[MKH];
OBJ_INDEX_DATA *obj_index_hash[MKH];
ROOM_INDEX_DATA *room_index_hash[MKH];

SYSTEM_DATA sysdata;

extern int top_command;
extern int top_social;
extern int top_news;
extern int top_trivia;
extern int top_answers;
extern int top_bti;
int top_affect;
int top_explorer;
int top_area;
int top_ed;
int top_exit;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int num_corpses;
int top_shop;
int top_repair;

/* Semi-locals. */
bool unfoldload;
bool unfoldbadload;
bool fBootDb;
char strArea[MIL];
FILE *fpArea;

bool assigning_gsns;

/* Assign gsn's for skills which need them. */
void assign_gsns( void )
{
   assigning_gsns = true;
   log_string( "Assigning gsn's" );
   gsn_taste = skill_lookup( "taste" );
   gsn_smell = skill_lookup( "smell" );
   gsn_style_evasive = skill_lookup( "evasive style" );
   gsn_style_defensive = skill_lookup( "defensive style" );
   gsn_style_standard = skill_lookup( "standard style" );
   gsn_style_aggressive = skill_lookup( "aggressive style" );
   gsn_style_berserk = skill_lookup( "berserk style" );
   gsn_barehanded = skill_lookup( "barehanded" );
   gsn_pugilism = skill_lookup( "pugilism" );
   gsn_long_blades = skill_lookup( "long blades" );
   gsn_short_blades = skill_lookup( "short blades" );
   gsn_flexible_arms = skill_lookup( "flexible arms" );
   gsn_talonous_arms = skill_lookup( "talonous arms" );
   gsn_bludgeons = skill_lookup( "bludgeons" );
   gsn_missile_weapons = skill_lookup( "missile weapons" );
   gsn_detrap = skill_lookup( "detrap" );
   gsn_detrap = skill_lookup( "detrap" );
   gsn_trapset = skill_lookup( "trapset" );
   gsn_backstab = skill_lookup( "backstab" );
   gsn_circle = skill_lookup( "circle" );
   gsn_tumble = skill_lookup( "tumble" );
   gsn_dodge = skill_lookup( "dodge" );
   gsn_duck = skill_lookup( "duck" );
   gsn_block = skill_lookup( "block" );
   gsn_shieldblock = skill_lookup( "shieldblock" );
   gsn_counter = skill_lookup( "counter" );
   gsn_hide = skill_lookup( "hide" );
   gsn_peek = skill_lookup( "peek" );
   gsn_pick_lock = skill_lookup( "pick lock" );
   gsn_sneak = skill_lookup( "sneak" );
   gsn_steal = skill_lookup( "steal" );
   gsn_gouge = skill_lookup( "gouge" );
   gsn_poison_weapon = skill_lookup( "poison weapon" );
   gsn_disarm = skill_lookup( "disarm" );
   gsn_enhanced_damage = skill_lookup( "enhanced damage" );
   gsn_kick = skill_lookup( "kick" );
   gsn_parry = skill_lookup( "parry" );
   gsn_rescue = skill_lookup( "rescue" );
   gsn_punch = skill_lookup( "punch" );
   gsn_bash = skill_lookup( "bash" );
   gsn_stun = skill_lookup( "stun" );
   gsn_bashdoor = skill_lookup( "doorbash" );
   gsn_grip = skill_lookup( "grip" );
   gsn_berserk = skill_lookup( "berserk" );
   gsn_feed = skill_lookup( "feed" );
   gsn_bloodlet = skill_lookup( "bloodlet" );
   gsn_chop = skill_lookup( "chop" );
   gsn_makefire = skill_lookup( "makefire" );
   gsn_aid = skill_lookup( "aid" );
   gsn_track = skill_lookup( "track" );
   gsn_search = skill_lookup( "search" );
   gsn_dig = skill_lookup( "dig" );
   gsn_mount = skill_lookup( "mount" );
   gsn_bite = skill_lookup( "bite" );
   gsn_claw = skill_lookup( "claw" );
   gsn_tail = skill_lookup( "tail" );
   gsn_scribe = skill_lookup( "scribe" );
   gsn_brew = skill_lookup( "brew" );
   gsn_imbue = skill_lookup( "imbue" );
   gsn_carve = skill_lookup( "carve" );
   gsn_concoct = skill_lookup( "concoct" );
   gsn_mix = skill_lookup( "mix" );
   gsn_climb = skill_lookup( "climb" );
   gsn_cook = skill_lookup( "cook" );
   gsn_scan = skill_lookup( "scan" );
   gsn_slice = skill_lookup( "slice" );
   gsn_lightning_bolt = skill_lookup( "lightning bolt" );
   gsn_aqua_breath = skill_lookup( "aqua breath" );
   gsn_blindness = skill_lookup( "blindness" );
   gsn_charm_person = skill_lookup( "charm person" );
   gsn_curse = skill_lookup( "curse" );
   gsn_invis = skill_lookup( "invis" );
   gsn_poison = skill_lookup( "poison" );
   gsn_sleep = skill_lookup( "sleep" );
   gsn_common = skill_lookup( "common" );
   assigning_gsns = false;
}

void shutdown_mud( const char *reason )
{
   FILE *fp;

   if( ( fp = fopen( SHUTDOWN_FILE, "a" ) ) )
   {
      fprintf( fp, "%s\n", reason );
      fclose( fp );
      fp = NULL;
   }
}

/* Big mama top level function. */
void boot_db( bool fCopyOver )
{
   fpArea = NULL;
   remove_file( BOOTLOG_FILE );

   if( !fCopyOver )
      fprintf( stderr, ".~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~[  Boot  Log  ]~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\n" );
   else
      fprintf( stderr, ".~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~[ Hotboot Log ]~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\n" );

   log_string( "Setting maxdoublegold" );
   setmaxdoublegold( );

   log_string( "Creating transfer data" );
   create_transfer( );

   log_string( "Loading transfer data" );
   load_transfer( );

   log_string( "Loading fish name data" );
   load_fnames( );

   log_string( "Loading channels..." );
   load_channels( );

   log_string( "Initializing libdl support..." );

   /* Open up a handle to the executable's symbol table for later use when working with commands */
   if( !( sysdata.dlHandle = dlopen( NULL, RTLD_LAZY ) ) )
   {
      log_string( "dl: Error opening local system executable as handle, please check compile flags." );
      shutdown_mud( "libdl failure" );
      exit( 1 );   
   }

   log_string( "Loading Calendar..." );
   load_calendarinfo( );

   log_string( "Loading helps..." );
   load_helps( );

   log_string( "Loading news..." );
   load_news( );

   log_string( "Loading trivia..." );
   load_trivia( );

   log_string( "Loading HighScores..." );
   load_highscores( );

   log_string( "Loading Hints..." );
   load_hints( );

   log_string( "Loading mud wide resets..." );
   load_mwresets( );

   log_string( "Loading bugs, typos, and ideas..." );
   load_bti( );

   log_string( "Loading banks..." );
   load_banks( );

   log_string( "Loading authorizes..." );
   load_auths( );

   log_string( "Loading lockershare information..." );
   load_lockershares( );

   log_string( "Loading commands..." );
   load_commands( );

   mud_start_time = current_time;

   log_string( "Loading spec_funs..." );
   load_specfuns( );

   log_string( "Loading sysdata configuration..." );
   load_systemdata( );

   log_string( "Loading socials" );
   load_socials( );

   log_string( "Loading skill table" );
   load_skill_table( );
   sort_skill_table( );
   remap_slot_numbers( ); /* must be after the sort */
   assign_gsns( );
   update_skill_helps( ); /* Update skill helps */
   update_command_helps( ); /* Update command helps */
   check_reqs( ); /* Update req_skills */

   log_string( "Loading classes" );
   load_classes( );

   log_string( "Loading class restrictions" );
   load_class_restrictions( );

   log_string( "Loading races" );
   load_races( );

   log_string( "Loading herb table" );
   load_herb_table( );

   log_string( "Loading personal table" );
   load_pers_table( );

   log_string( "Loading tongues" );
   load_tongues( );

   log_string( "Making wizlist" );
   make_wizlist( );

   log_string( "Loading MSSP Data..." );
   load_mssp_data( );

   fBootDb = true;
   unfoldload = false;
   unfoldbadload = false;

   top_shop = 0;
   top_repair = 0;
   nummobsloaded = 0;
   numobjsloaded = 0;
   physicalobjects = 0;
   sysdata.maxplayers = 0;
   num_corpses = 0;
   first_object = last_object = NULL;
   first_corpse = last_corpse = NULL;
   first_char = last_char = NULL;
   first_hating = last_hating = NULL;
   first_hunting = last_hunting = NULL;
   first_fearing = last_fearing = NULL;
   first_group = last_group = NULL;
   first_area = last_area = NULL;
   first_area_name = last_area_name = NULL; /* Used for alphanum. sort */
   first_build = last_build = NULL;
   first_shop = last_shop = NULL;
   first_repair = last_repair = NULL;
   first_asort = last_asort = NULL;
   extracted_char_queue = NULL;
   cur_qchars = 0;
   cur_char = NULL;
   cur_char_died = false;
   quitting_char = NULL;
   loading_char = NULL;
   saving_char = NULL;
   first_ban_class = last_ban_class = NULL;
   first_ban_race = last_ban_race = NULL;
   first_ban = last_ban = NULL;

   weath_unit = 10;
   rand_factor = 2;
   climate_factor = 1;
   neigh_factor = 3;
   max_vector = weath_unit * 3;

   /* Init random number generator. */
   log_string( "Initializing random number generator" );
   init_mm( );

   /* Load time_info */
   log_string( "Loading time_info" );
   load_timeinfo( );

   /* Read in all the area files. */
   {
      FILE *fp;

      log_string( "Reading in area files..." );
      if( !( fp = fopen( AREA_LIST, "r" ) ) )
      {
         perror( AREA_LIST );
         shutdown_mud( "Unable to open area list" );
         exit( 1 );
      }

      for( ;; )
      {
         mudstrlcpy( strArea, fread_word( fp ), sizeof( strArea ) );
         if( strArea[0] == '$' )
            break;

         load_area_file( last_area, strArea );
      }
      fclose( fp );
      fp = NULL;
   }

   log_string( "Loading buildlist" );
   load_buildlist( );

   /* initialize supermob. must be done before reset_area! */
   init_supermob( );

   /*
    * Fix up exits.
    * Declare db booting over.
    * Reset all areas once.
    * Load up the notes file.
    */
   log_string( "Fixing exits" );
   fix_exits( );
   fBootDb = false;

   log_string( "Checking all resets" );
   check_all_resets( );

   log_string( "Resetting areas" );
   area_update( );

   log_string( "Loading boards" );
   load_boards( );

   log_string( "Loading host logs" );
   load_hostlog( );

   load_clans( );
   load_councils( );
   load_deity( );

   log_string( "Loading bans" );
   load_banlist( );

   log_string( "Loading rewards" );
   load_rewards( );

   log_string( "Loading reserved names" );
   load_reserved( );

   log_string( "Loading corpses" );
   load_corpses( );

   /* Morphs MUST be loaded after class and race tables are set up --Shaddai */
   log_string( "Loading Morphs" );
   load_morphs( );

   log_string( "Loading storages" );
   load_storages( );

   log_string( "Loading mpdamages" );
   load_mpdamages( );

   MOBtrigger = true;

   /* Initialize area weather data */
   load_weatherdata( );
   init_area_weather( );

   /* Go ahead and check to make sure some things are right and give bugs if not */
   code_check( );
}

/* Load an 'area' header line. */
void load_area( FILE *fp )
{
   AREA_DATA *pArea;

   CREATE( pArea, AREA_DATA, 1 );
   pArea->first_room = pArea->last_room = NULL;
   pArea->name = fread_string( fp );
   pArea->author = STRALLOC( "unknown" );
   pArea->filename = STRALLOC( strArea );
   pArea->age = 15;
   pArea->vnum = 0;
   pArea->nplayer = 0;
   pArea->low_vnum = 0;
   pArea->hi_vnum = 0;
   pArea->low_soft_range = 0;
   pArea->hi_soft_range = MAX_LEVEL;
   pArea->low_hard_range = 0;
   pArea->hi_hard_range = MAX_LEVEL;
   pArea->top_obj = 0;
   pArea->top_mob = 0;
   pArea->top_room = 0;

   /* initialize weather data - FB */
   CREATE( pArea->weather, WEATHER_DATA, 1 );
   pArea->weather->temp = 0;
   pArea->weather->precip = 0;
   pArea->weather->wind = 0;
   pArea->weather->temp_vector = 0;
   pArea->weather->precip_vector = 0;
   pArea->weather->wind_vector = 0;
   pArea->weather->climate_temp = 2;
   pArea->weather->climate_precip = 2;
   pArea->weather->climate_wind = 2;
   pArea->weather->first_neighbor = NULL;
   pArea->weather->last_neighbor = NULL;
   pArea->weather->echo = NULL;
   pArea->weather->echo_color = AT_GRAY;
   LINK( pArea, first_area, last_area, next, prev );
   top_area++;
}

/* Handle freeing the version number if it is there, not needed with new format though */
void load_version( AREA_DATA *tarea, FILE *fp )
{
   fread_number( fp );
}

/* Load an author section. Scryn 2/1/96 */
void load_author( AREA_DATA *tarea, FILE *fp )
{
   STRFREE( tarea->author );
   tarea->author = fread_string( fp );
}

/* Reset Message Load, Rennard */
void load_resetmsg( AREA_DATA *tarea, FILE *fp )
{
   STRFREE( tarea->resetmsg );
   tarea->resetmsg = fread_string( fp );
}

void load_flags( AREA_DATA *tarea, FILE *fp )
{
   char *infoflags, flag[MSL];
   int value;

   infoflags = fread_flagstring( fp );
   while( infoflags && infoflags[0] != '\0' )
   {
      infoflags = one_argument( infoflags, flag );
      value = get_flag( flag, area_flags, AFLAG_MAX );
      if( value < 0 || value >= AFLAG_MAX )
         bug( "%s: invalid area flag (%s)", __FUNCTION__, flag );
      else
         xSET_BIT( tarea->flags, value );
   }
}

/* Add a character to the list of all characters - Thoric */
void add_char( CHAR_DATA *ch )
{
   LINK( ch, first_char, last_char, next, prev );
   if( ch->pIndexData )
      LINK( ch, ch->pIndexData->first_copy, ch->pIndexData->last_copy, next_index, prev_index );
}

/* load a mob section. */
void load_mobiles( AREA_DATA *tarea, FILE *fp )
{
   MOB_INDEX_DATA *pMobIndex = NULL;
   const char *word;
   char *infoflags = NULL, flag[MIL];
   int vnum = 0, iHash, value;
   bool oldmob = false, tmpBootDb, fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;
      if( word[0] == EOF )
      {
         bug( "%s: feof didn't detect EOF but fread_word returned EOF...ending now!", __FUNCTION__ );
         word = "End";
      }

      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case '>':
            if( !strcmp( word, ">" ) )
            {
               ungetc( '>', fp );
               mprog_read_programs( fp, pMobIndex );
               fMatch = true;
               break;
            }
            break;

         case '#':
            if( !strcmp( word, "#0" ) )
               return;
            break;

         case 'A':
            if( !strcmp( word, "Absorb" ) )
            {
               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  value = get_flag( flag, ris_flags, RIS_MAX );
                  if( value < 0 || value >= RIS_MAX )
                     bug( "%s: Unknown %s: %s", __FUNCTION__, word, flag );
                  else /* Set them some absorbtion */
                     pMobIndex->resistant[value] = 110;
               }
               fMatch = true;
               break;
            }
            KEY( "Alignment", pMobIndex->alignment, fread_number( fp ) );
            KEY( "Armor", pMobIndex->ac, fread_number( fp ) );
            WEXTKEY( "Attacks", pMobIndex->attacks, fp, attack_flags, ATCK_MAX );
            WEXTKEY( "Affected", pMobIndex->affected_by, fp, a_flags, AFF_MAX );
            break;

         case 'B':
            if( !str_cmp( word, "BGold" ) )
            {
               fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'D':
            KEY( "DamRoll", pMobIndex->damroll, fread_number( fp ) );
            KEY( "Description", pMobIndex->description, fread_string( fp ) );
            WEXTKEY( "Defenses", pMobIndex->defenses, fp, defense_flags, DFND_MAX );
            SKEY( "DefPosition", pMobIndex->defposition, fp, pos_names, POS_MAX );
            break;

         case 'E':
            if( !strcmp( word, "End" ) )
            {
               if( pMobIndex->ac < 0 )
                  pMobIndex->ac = -pMobIndex->ac;

               /* Make sure its set to NPC if it isn't already */
               if( !xIS_SET( pMobIndex->act, ACT_IS_NPC ) )
                  xSET_BIT( pMobIndex->act, ACT_IS_NPC );
               if( !oldmob )
               {
                  iHash = vnum % MKH;
                  pMobIndex->next = mob_index_hash[iHash];
                  mob_index_hash[iHash] = pMobIndex;
                  top_mob_index++;
                  LINK( pMobIndex, tarea->first_mob, tarea->last_mob, next_amob, prev_amob );
                  pMobIndex->avnum = tarea->vnum;
                  pMobIndex->area = tarea;
               }
               fMatch = true;
               break;
            }
            break;

         case 'F':
            WEXTKEY( "Flags", pMobIndex->act, fp, act_flags, ACT_MAX );
            break;

         case 'G':
            KEY( "Gold", pMobIndex->gold, fread_number( fp ) );
            break;

         case 'H':
            KEY( "HitRoll", pMobIndex->hitroll, fread_number( fp ) );
            KEY( "Height", pMobIndex->height, fread_number( fp ) );
            if( !strcmp( word, "Hit" ) )
            {
               pMobIndex->minhit = fread_number( fp );
               pMobIndex->maxhit = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'I':
            if( !strcmp( word, "Immune" ) )
            {
               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  value = get_flag( flag, ris_flags, RIS_MAX );
                  if( value < 0 || value >= RIS_MAX )
                     bug( "%s: Unknown %s: %s", __FUNCTION__, word, flag );
                  else /* Set them Immune */
                     pMobIndex->resistant[value] = 100;
               }
               fMatch = true;
               break;
            }
            break;

         case 'L':
            KEY( "Level", pMobIndex->level, fread_number( fp ) );
            KEY( "Long", pMobIndex->long_descr, fread_string( fp ) );
            break;

         case 'M':
            if( !str_cmp( word, "MGold" ) )
            {
               fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'N':
            if( !strcmp( word, "NResistant" ) )
            {
               int tmpvalue = fread_number( fp );
               infoflags = fread_flagstring( fp );
               value = get_flag( infoflags, ris_flags, RIS_MAX );
               if( value < 0 || value >= RIS_MAX )
                  bug( "%s: Unknown %s: %s", __FUNCTION__, word, infoflags );
               else
                  pMobIndex->resistant[value] = tmpvalue;
               fMatch = true;
               break;
            }
            KEY( "NumAttacks", pMobIndex->numattacks, fread_number( fp ) );
            KEY( "Name", pMobIndex->name, fread_string( fp ) );
            if( !strcmp( word, "NRepair" ) )
            {
               REPAIR_DATA *rShop;
               int tmpvalue;
               char *srepair,sarg[MSL];

               CREATE( rShop, REPAIR_DATA, 1 );
               for( tmpvalue = 0; tmpvalue < ITEM_TYPE_MAX; tmpvalue++ )
                  rShop->fix_type[tmpvalue] = false;
               rShop->keeper = pMobIndex->vnum;
               rShop->profit_fix = fread_number( fp );
               rShop->open_hour = fread_number( fp );
               rShop->close_hour = fread_number( fp );
               srepair = fread_flagstring( fp );
               while( srepair && srepair[0] != '\0' )
               {
                  srepair = one_argument( srepair, sarg );
                  tmpvalue = get_flag( sarg, o_types, ITEM_TYPE_MAX );
                  if( tmpvalue > 0 && tmpvalue < ITEM_TYPE_MAX )
                     rShop->fix_type[tmpvalue] = true;
               }
               pMobIndex->rShop = rShop;
               LINK( rShop, first_repair, last_repair, next, prev );
               top_repair++;
               fMatch = true;
               break;
            }
            if( !strcmp( word, "NShop" ) )
            {
               SHOP_DATA *pShop;
               int tmpvalue;
               char *stype, sarg[MSL];

               CREATE( pShop, SHOP_DATA, 1 );
               for( tmpvalue = 0; tmpvalue < ITEM_TYPE_MAX; tmpvalue++ )
                  pShop->buy_type[tmpvalue] = false;
               pShop->keeper = pMobIndex->vnum;
               pShop->profit_buy = fread_number( fp );
               pShop->profit_sell = fread_number( fp );
               pShop->open_hour = fread_number( fp );
               pShop->close_hour = fread_number( fp );
               stype = fread_flagstring( fp );
               while( stype && stype[0] != '\0' )
               {
                  stype = one_argument( stype, sarg );
                  tmpvalue = get_flag( sarg, o_types, ITEM_TYPE_MAX );
                  if( tmpvalue > 0 && tmpvalue < ITEM_TYPE_MAX )
                     pShop->buy_type[tmpvalue] = true;
               }
               pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
               pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
               pMobIndex->pShop = pShop;
               LINK( pShop, first_shop, last_shop, next, prev );
               top_shop++;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "NStat" ) )
            {
               int ustat, stat;

               stat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
                  pMobIndex->perm_stats[ustat] = stat;
               fMatch = true;
               break;
            }
            break;

         case 'P':
            WEXTKEY( "Parts", pMobIndex->xflags, fp, part_flags, PART_MAX );
            break;

         case 'R':
            if( !strcmp( word, "Repair" ) )
            {
               REPAIR_DATA *rShop;
               int tmpvalue;

               CREATE( rShop, REPAIR_DATA, 1 );
               for( tmpvalue = 0; tmpvalue < ITEM_TYPE_MAX; tmpvalue++ )
                  rShop->fix_type[tmpvalue] = false;
               for( tmpvalue = 0; tmpvalue < 3; tmpvalue++ )
               {
                  rShop->fix_type[fread_number( fp )] = true;
               }
               rShop->keeper = pMobIndex->vnum;
               rShop->profit_fix = fread_number( fp );
               fread_number( fp );
               rShop->open_hour = fread_number( fp );
               rShop->close_hour = fread_number( fp );
               pMobIndex->rShop = rShop;
               LINK( rShop, first_repair, last_repair, next, prev );
               top_repair++;
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Resistant" ) )
            {
               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  value = get_flag( flag, ris_flags, RIS_MAX );
                  if( value < 0 || value >= RIS_MAX )
                     bug( "%s: Unknown %s: %s", __FUNCTION__, word, flag );
                  else /* Set them some resistance */
                     pMobIndex->resistant[value] = 10;
               }
               fMatch = true;
               break;
            }
            break;

         case 'S':
            if( !strcmp( word, "Susceptible" ) )
            {
               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  value = get_flag( flag, ris_flags, RIS_MAX );
                  if( value < 0 || value >= RIS_MAX )
                     bug( "%s: Unknown %s: %s", __FUNCTION__, word, flag );
                  else /* Set them susceptible */
                     pMobIndex->resistant[value] = -10;
               }
               fMatch = true;
               break;
            }
            SKEY( "Sex", pMobIndex->sex, fp, sex_names, SEX_MAX );
            KEY( "Short", pMobIndex->short_descr, fread_string( fp ) );
            if( !strcmp( word, "Special" ) )
            {
               infoflags = fread_word( fp );
               if( !( pMobIndex->spec_fun = spec_lookup( infoflags ) ) )
               {
                  bug( "%s: 'M': vnum %d.", __FUNCTION__, pMobIndex->vnum );
                  pMobIndex->spec_funname = NULL;
               }
               else
                  pMobIndex->spec_funname = STRALLOC( infoflags );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Shop" ) )
            {
               int tmpvalue;
               SHOP_DATA *pShop;

               CREATE( pShop, SHOP_DATA, 1 );
               for( tmpvalue = 0; tmpvalue < ITEM_TYPE_MAX; tmpvalue++ )
                  pShop->buy_type[tmpvalue] = false;
               for( tmpvalue = 0; tmpvalue < 5; tmpvalue++ )
                  pShop->buy_type[fread_number( fp )] = true;
               pShop->keeper = pMobIndex->vnum;
               pShop->profit_buy = fread_number( fp );
               pShop->profit_sell = fread_number( fp );
               pShop->open_hour = fread_number( fp );
               pShop->close_hour = fread_number( fp );
               pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
               pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
               pMobIndex->pShop = pShop;
               LINK( pShop, first_shop, last_shop, next, prev );
               top_shop++;
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Saves" ) )
            {
               pMobIndex->saving_poison_death = fread_number( fp );
               pMobIndex->saving_wand = fread_number( fp );
               pMobIndex->saving_para_petri = fread_number( fp );
               pMobIndex->saving_breath = fread_number( fp );
               pMobIndex->saving_spell_staff = fread_number( fp );
               fMatch = true;
               break;
            }
            WEXTKEY( "Speaks", pMobIndex->speaks, fp, lang_names, LANG_UNKNOWN );
            WEXTKEY( "Speaking", pMobIndex->speaking, fp, lang_names, LANG_UNKNOWN );
            if( !strcmp( word, "Stats" ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               pMobIndex->perm_stats[STAT_STR] = fread_number( fp );
               pMobIndex->perm_stats[STAT_INT] = fread_number( fp );
               pMobIndex->perm_stats[STAT_WIS] = fread_number( fp );
               pMobIndex->perm_stats[STAT_DEX] = fread_number( fp );
               pMobIndex->perm_stats[STAT_CON] = fread_number( fp );
               pMobIndex->perm_stats[STAT_CHA] = fread_number( fp );
               pMobIndex->perm_stats[STAT_LCK] = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'V':
            if( !strcmp( word, "Vnum" ) )
            {
               vnum = fread_number( fp );
               tmpBootDb = fBootDb;
               fBootDb = false;
               if( get_mob_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     bug( "Load_mobiles: vnum %d duplicated.", vnum );
                     if( unfoldload )
                     {
                        unfoldbadload = true;
                        return;
                     }
                     shutdown_mud( "duplicate vnum" );
                     exit( 1 );
                  }
                  else
                  {
                     pMobIndex = get_mob_index( vnum );
                     log_printf_plus( LOG_BUILD, sysdata.perm_log, "Cleaning mobile: %d", vnum );
                     clean_mob( pMobIndex );
                     oldmob = true;
                  }
               }
               else
               {
                  int stat;

                  oldmob = false;
                  CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
                  pMobIndex->pShop = NULL;
                  pMobIndex->rShop = NULL;
                  for( stat = 0; stat < STAT_MAX; stat++ ) /* Should really default these better then 0 lol */
                     pMobIndex->perm_stats[stat] = 13;
               }
               pMobIndex->vnum = vnum;
               ++tarea->top_mob;
               fBootDb = tmpBootDb;
               if( fBootDb )
               {
                  if( !tarea->low_vnum )
                     tarea->low_vnum = vnum;
                  if( vnum > tarea->hi_vnum )
                     tarea->hi_vnum = vnum;
               }
               fMatch = true;
               break;
            }
            break;

         case 'W':
            KEY( "Weight", pMobIndex->weight, fread_number( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   bug( "%s: #0 not found?", __FUNCTION__ );
}

/* Load an obj section. */
void load_objects( AREA_DATA *tarea, FILE *fp )
{
   OBJ_INDEX_DATA *pObjIndex = NULL;
   const char *word;
   char *infoflags = NULL, flag[MIL];
   int vnum = 0, iHash, value, stat;
   bool tmpBootDb, oldobj = false, fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;
      if( word[0] == EOF )
      {
         bug( "%s: feof didn't detect EOF but fread_word returned EOF...ending now!", __FUNCTION__ );
         word = "End";
      }

      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case '#':
            if( !strcmp( word, "#0" ) )
               return;
            break;

         case '>':
            if( !strcmp( word, ">" ) )
            {
               ungetc( '>', fp );
               oprog_read_programs( fp, pObjIndex );
               fMatch = true;
               break;
            }
            break;

         case 'A':
            KEY( "Action", pObjIndex->action_desc, fread_string( fp ) );
            break;

         case 'C':
            KEY( "Cost", pObjIndex->cost, fread_number( fp ) );
            if( !str_cmp( word, "Classes" ) )
            {
               int iclass;

               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  for( iclass = 0; iclass < MAX_PC_CLASS; iclass++ )
                  {
                     if( !class_table[iclass] || !class_table[iclass]->name )
                        continue;
                     if( !str_cmp( class_table[iclass]->name, flag ) )
                     {
                        xSET_BIT( pObjIndex->class_restrict, iclass );
                        break;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'D':
            KEY( "Description", pObjIndex->description, fread_string( fp ) );
            KEY( "Desc", pObjIndex->desc, fread_string( fp ) );
            break;

         case 'E':
            if( !strcmp( word, "End" ) )
            {
               if( !oldobj )
               {
                  iHash = vnum % MKH;
                  pObjIndex->next = obj_index_hash[iHash];
                  obj_index_hash[iHash] = pObjIndex;
                  top_obj_index++;
                  LINK( pObjIndex, tarea->first_obj, tarea->last_obj, next_aobj, prev_aobj );
                  pObjIndex->avnum = tarea->vnum;
                  pObjIndex->area = tarea;
               }
               fMatch = true;
               break;
            }
            if( !strcmp( word, "E" ) )
            {
               EXTRA_DESCR_DATA *ed;

               CREATE( ed, EXTRA_DESCR_DATA, 1 );
               ed->keyword = fread_string( fp );
               ed->description = fread_string( fp );
               LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc, next, prev );
               top_ed++;
               fMatch = true;
               break;
            }
            break;

         case 'F':
            WEXTKEY( "Flags", pObjIndex->extra_flags, fp, o_flags, ITEM_MAX );
            break;

         case 'L':
            KEY( "Level", pObjIndex->level, fread_number( fp ) );
            KEY( "Layers", pObjIndex->layers, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", pObjIndex->name, fread_string( fp ) );
            if( !strcmp( word, "NAffect" ) )
            {
               AFFECT_DATA *paf;

               if( ( paf = fread_chaffect( fp, 2, __FILE__, __LINE__ ) ) )
               {
                  LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
                  top_affect++;
               }
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "NStat" ) )
            {
               int ustat;

               stat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
                  pObjIndex->stat_reqs[ustat] = stat;
               fMatch = true;
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "Races" ) )
            {
               int irace;

               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  for( irace = 0; irace < MAX_PC_RACE; irace++ )
                  {
                     if( !race_table[irace] || !race_table[irace]->name )
                        continue;
                     if( !str_cmp( race_table[irace]->name, flag ) )
                     {
                        xSET_BIT( pObjIndex->race_restrict, irace );
                        break;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'S':
            KEY( "Short", pObjIndex->short_descr, fread_string( fp ) );
            if( !strcmp( word, "Stats" ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               pObjIndex->stat_reqs[STAT_STR] = fread_number( fp );
               pObjIndex->stat_reqs[STAT_INT] = fread_number( fp );
               pObjIndex->stat_reqs[STAT_WIS] = fread_number( fp );
               pObjIndex->stat_reqs[STAT_DEX] = fread_number( fp );
               pObjIndex->stat_reqs[STAT_CON] = fread_number( fp );
               pObjIndex->stat_reqs[STAT_CHA] = fread_number( fp );
               pObjIndex->stat_reqs[STAT_LCK] = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'T':
            SKEY( "Type", pObjIndex->item_type, fp, o_types, ITEM_TYPE_MAX );
            break;

         case 'V':
            if( !strcmp( word, "Val0" ) )
            {
               pObjIndex->value[0] = fread_number( fp );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Val1" ) )
            {
               pObjIndex->value[1] = fread_number( fp );
               if( pObjIndex->item_type == ITEM_PILL || pObjIndex->item_type == ITEM_POTION
               || pObjIndex->item_type == ITEM_SCROLL )
                  pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Val2" ) )
            {
               pObjIndex->value[2] = fread_number( fp );
               if( pObjIndex->item_type == ITEM_PILL || pObjIndex->item_type == ITEM_POTION
               || pObjIndex->item_type == ITEM_SCROLL )
                  pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Val3" ) )
            {
               pObjIndex->value[3] = fread_number( fp );
               if( pObjIndex->item_type == ITEM_PILL || pObjIndex->item_type == ITEM_POTION
               || pObjIndex->item_type == ITEM_SCROLL || pObjIndex->item_type == ITEM_STAFF
               || pObjIndex->item_type == ITEM_WAND || pObjIndex->item_type == ITEM_SALVE )
                  pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Val4" ) )
            {
               pObjIndex->value[4] = fread_number( fp );
               if( pObjIndex->item_type == ITEM_SALVE )
                  pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Val5" ) )
            {
               pObjIndex->value[5] = fread_number( fp );
               if( pObjIndex->item_type == ITEM_SALVE )
                  pObjIndex->value[5] = skill_lookup( fread_word( fp ) );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Vnum" ) )
            {
               vnum = fread_number( fp );
               tmpBootDb = fBootDb;
               fBootDb = false;
               if( get_obj_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     bug( "%s: vnum %d duplicated.", __FUNCTION__, vnum );
                     if( unfoldload )
                     {
                        unfoldbadload = true;
                        return;
                     }
                     shutdown_mud( "duplicate vnum" );
                     exit( 1 );
                  }
                  else
                  {
                     pObjIndex = get_obj_index( vnum );
                     log_printf_plus( LOG_BUILD, sysdata.perm_log, "Cleaning object: %d", vnum );
                     clean_obj( pObjIndex );
                     oldobj = true;
                  }
               }
               else
               {
                  oldobj = false;
                  pObjIndex = new_object( vnum );
               }
               fBootDb = tmpBootDb;
               pObjIndex->vnum = vnum;
               ++tarea->top_obj;
               if( fBootDb )
               {
                  if( !tarea->low_vnum )
                     tarea->low_vnum = vnum;
                  if( vnum > tarea->hi_vnum )
                     tarea->hi_vnum = vnum;
               }
               fMatch = true;
               break;
            }
            break;

         case 'W':
            KEY( "Weight", pObjIndex->weight, fread_number( fp ) );
            WEXTKEY( "Wear", pObjIndex->wear_flags, fp, w_flags, ITEM_WEAR_MAX );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   bug( "%s: #0 not found?", __FUNCTION__ );
}

void check_all_resets( void )
{
   ROOM_INDEX_DATA *rid, *rid_next;
   RESET_DATA *pReset, *pReset_next;
   int icnt, count;

   for( icnt = 0; icnt < MKH; icnt++ )
   {
      for( rid = room_index_hash[icnt]; rid; rid = rid_next )
      {
         rid_next = rid->next;

         count = 0;
         for( pReset = rid->first_reset; pReset; pReset = pReset_next )
         {
            pReset_next = pReset->next;

            ++count;
            switch( UPPER( pReset->command ) )
            {
               default:
                  break;

               case 'M':
                  if( !get_mob_index( pReset->arg1 ) )
                     boot_log( "%s: Area[%s] Room[%d] Reset(%d) 'M': Mobile(%d) doesn't exist.", __FUNCTION__, rid->area->filename, rid->vnum, count, pReset->arg1 );
                  break;

               case 'O':
                  if( !get_obj_index( pReset->arg1 ) )
                     boot_log( "%s: Area[%s] Room[%d] Reset(%d) 'O': Object(%d) doesn't exist.", __FUNCTION__, rid->area->filename, rid->vnum, count, pReset->arg1 );
                  break;

               case 'P':
                  if( !get_obj_index( pReset->arg1 ) )
                     boot_log( "%s: Area[%s] Room[%d] Reset(%d) 'P': Object(%d) doesn't exist.", __FUNCTION__, rid->area->filename, rid->vnum, count, pReset->arg1 );

                  if( !get_obj_index( pReset->arg3 ) )
                     boot_log( "%s: Area[%s] Room[%d] Reset(%d) 'P': Destination Object(%d) doesn't exist.", __FUNCTION__, rid->area->filename, rid->vnum, count, pReset->arg3 );
                  break;

               case 'G':
               case 'E':
                  if( !get_obj_index( pReset->arg1 ) )
                     boot_log( "%s: Area[%s] Room[%d] Reset(%d) '%c': Object(%d) doesn't exist.", __FUNCTION__, rid->area->filename, rid->vnum, count, UPPER( pReset->command ), pReset->arg1 );
                  break;

               case 'R':
                  if( pReset->arg2 < 0 || pReset->arg2 >= DIR_MAX )
                  {
                     bug( "%s: 'R': bad exit %d.", __FUNCTION__, pReset->arg2 );
                     boot_log( "%s: Area[%s] Room[%d] Reset(%d) 'R': Bad Exit(%d).", __FUNCTION__, rid->area->filename, rid->vnum, count, pReset->arg2 );
                     break;
                  }
                  break;
            }
         }
      }
   }
}

void load_room_reset( ROOM_INDEX_DATA *room, FILE *fp, short rver )
{
   char letter;
   int extra, arg1, arg2, arg3, count = 0;
   short rchance = 100, xcord = 0, ycord = 0;
   bool not01 = false, wilderness = false;

   letter = fread_letter( fp );
   extra = fread_number( fp );
   if( letter == 'M' || letter == 'O' )
      extra = 0;
   arg1 = fread_number( fp );
   arg2 = fread_number( fp );
   arg3 = ( letter == 'G' || letter == 'R' ) ? 0 : fread_number( fp );
   if( rver >= 1 )
   {
      rchance = fread_number( fp );
      rchance = URANGE( 1, rchance, 100 );
   }

   if( rver >= 2 )
   {
      xcord = fread_number( fp );
      ycord = fread_number( fp );
      wilderness = true;
   }

   fread_to_eol( fp );
   ++count;

   /*
    * Validate parameters.
    * We're calling the index functions for the side effect.
    */
   switch( letter )
   {
      default: /* Doesn't add bad commands so just returns */
         bug( "%s: bad command '%c'.", __FUNCTION__, letter );
         if( fBootDb )
            boot_log( "%s: %s (%d) bad command '%c'.", __FUNCTION__, room->area->filename, count, letter );
         return;

      case 'P':
         if( extra > 1 )
            not01 = true;
         break;

      case 'M':
      case 'O':
      case 'G':
      case 'E':
      case 'R':
      case 'T':
      case 'H':
         break;

      case 'D': /* Just return here so it doesn't get added since we no longer use the door resets */
         return;
   }
   add_reset( room, letter, extra, arg1, arg2, arg3, rchance, xcord, ycord, wilderness );

   if( !not01 )
      renumber_put_resets( room );
}

/* Load a room section. */
void load_rooms( AREA_DATA *tarea, FILE *fp )
{
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   const char *word;
   char *infoflags = NULL, flag[MIL];
   int vnum = 0, door, iHash, value;
   bool tmpBootDb, oldroom = false, fMatch;

   tarea->first_room = tarea->last_room = NULL;
   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;
      if( word[0] == EOF )
      {
         bug( "%s: feof didn't detect EOF but fread_word returned EOF...ending now!", __FUNCTION__ );
         word = "End";
      }

      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case '#':
            if( !strcmp( word, "#0" ) )
               return;
            break;

         case '>':
            if( !strcmp( word, ">" ) )
            {
               ungetc( '>', fp );
               rprog_read_programs( fp, pRoomIndex );
               fMatch = true;
               break;
            }
            break;

         case 'D':
            KEY( "Description", pRoomIndex->description, fread_string( fp ) );
            if( !strcmp( word, "Door" ) )
            {
               EXIT_DATA *pexit;
               const char *check;
               bool fcheck, finished = false;

               door = fread_number( fp );
               if( door < 0 || door >= DIR_MAX )
               {
                  bug( "%s: vnum %d has bad door number %d.", __FUNCTION__, vnum, door );
                  if( fBootDb )
                     exit( 1 );
               }
               else
               {
                  pexit = make_exit( pRoomIndex, NULL, door );
                  pexit->key = -1;
                  for( ;; )
                  {
                     check = feof( fp ) ? "End" : fread_word( fp );
                     fcheck = false;
                     if( check[0] == EOF )
                     {
                        bug( "%s: feof didn't detect EOF but fread_word returned EOF...ending now!", __FUNCTION__ );
                        check = "End";
                     }
                     switch( UPPER( check[0] ) )
                     {
                        case '*':
                           fcheck = true;
                           fread_to_eol( fp );
                           break;

                        case '#':
                           if( !strcmp( check, "#0" ) )
                              return;
                           break;

                        case 'D':
                           if( !strcmp( check, "Description" ) )
                           {
                              pexit->description = fread_string( fp );
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'E':
                           if( !strcmp( check, "End" ) )
                           {
                              finished = true;
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'F':
                           if( !strcmp( check, "Flags" ) )
                           {
                              infoflags = fread_flagstring( fp );
                              while( infoflags && infoflags[0] != '\0' )
                              {
                                 infoflags = one_argument( infoflags, flag );
                                 value = get_flag( flag, ex_flags, EX_MAX );
                                 if( value < 0 || value >= EX_MAX )
                                    bug( "%s: Unknown exit_info: %s", __FUNCTION__, flag );
                                 else
                                 {
                                    xSET_BIT( pexit->base_info, value );
                                    xSET_BIT( pexit->exit_info, value );
                                 }
                              }
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'K':
                           if( !strcmp( check, "Key" ) )
                           {
                              pexit->key = fread_number( fp );
                              fcheck = true;
                              break;
                           }
                           if( !strcmp( check, "Keyword" ) )
                           {
                              pexit->keyword = fread_string( fp );
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'P':
                           if( !strcmp( check, "Pulltype" ) )
                           {
                              infoflags = fread_flagstring( fp );
                              while( infoflags && infoflags[0] != '\0' )
                              {
                                  infoflags = one_argument( infoflags, flag );
                                  value = get_pulltype( flag );
                                  if( value == -1 )
                                     bug( "%s: Unknown pulltype: %s", __FUNCTION__, flag );
                                  else
                                     pexit->pulltype = value;
                              }
                              fcheck = true;
                              break;
                           }
                           if( !strcmp( check, "Pull" ) )
                           {
                              pexit->pull = fread_number( fp );
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'T':
                           if( !strcmp( check, "To" ) )
                           {
                              pexit->vnum = fread_number( fp );
                              fcheck = true;
                              break;
                           }
                           break;
                     }
                     if( finished )
                        break;
                     if( !fcheck )
                     {
                        bug( "%s: %s: Unknown word %s", __FUNCTION__, word, check );
                        fread_to_eol( fp );
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'E':
            if( !strcmp( word, "Exit" ) )
            {
               EXIT_DATA *pexit;
               const char *check;
               bool fcheck, finished = false;

               infoflags = fread_word( fp );
               door = get_dir( infoflags );
               if( door < 0 || door >= DIR_MAX )
               {
                  bug( "%s: vnum %d has bad door number %d(%s).", __FUNCTION__, vnum, door, infoflags );
                  if( fBootDb )
                     exit( 1 );
               }
               else
               {
                  pexit = make_exit( pRoomIndex, NULL, door );
                  pexit->key = -1;
                  for( ;; )
                  {
                     check = feof( fp ) ? "End" : fread_word( fp );
                     fcheck = false;
                     if( check[0] == EOF )
                     {
                        bug( "%s: feof didn't detect EOF but fread_word returned EOF...ending now!", __FUNCTION__ );
                        check = "End";
                     }
                     switch( UPPER( check[0] ) )
                     {
                        case '*':
                           fcheck = true;
                           fread_to_eol( fp );
                           break;

                        case '#':
                           if( !strcmp( check, "#0" ) )
                              return;
                           break;

                        case 'D':
                           if( !strcmp( check, "Description" ) )
                           {
                              pexit->description = fread_string( fp );
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'E':
                           if( !strcmp( check, "End" ) )
                           {
                              finished = true;
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'F':
                           if( !strcmp( check, "Flags" ) )
                           {
                              infoflags = fread_flagstring( fp );
                              while( infoflags && infoflags[0] != '\0' )
                              {
                                 infoflags = one_argument( infoflags, flag );
                                 value = get_flag( flag, ex_flags, EX_MAX );
                                 if( value < 0 || value >= EX_MAX )
                                    bug( "%s: Unknown exit_info: %s", __FUNCTION__, flag );
                                 else
                                 {
                                    xSET_BIT( pexit->base_info, value );
                                    xSET_BIT( pexit->exit_info, value );
                                 }
                              }
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'K':
                           if( !strcmp( check, "Key" ) )
                           {
                              pexit->key = fread_number( fp );
                              fcheck = true;
                              break;
                           }
                           if( !strcmp( check, "Keyword" ) )
                           {
                              pexit->keyword = fread_string( fp );
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'P':
                           if( !strcmp( check, "Pulltype" ) )
                           {
                              infoflags = fread_flagstring( fp );
                              while( infoflags && infoflags[0] != '\0' )
                              {
                                  infoflags = one_argument( infoflags, flag );
                                  value = get_pulltype( flag );
                                  if( value == -1 )
                                     bug( "%s: Unknown pulltype: %s", __FUNCTION__, flag );
                                  else
                                     pexit->pulltype = value;
                              }
                              fcheck = true;
                              break;
                           }
                           if( !strcmp( check, "Pull" ) )
                           {
                              pexit->pull = fread_number( fp );
                              fcheck = true;
                              break;
                           }
                           break;

                        case 'T':
                           if( !strcmp( check, "To" ) )
                           {
                              pexit->vnum = fread_number( fp );
                              fcheck = true;
                              break;
                           }
                           break;
                     }
                     if( finished )
                        break;
                     if( !fcheck )
                     {
                        bug( "%s: %s: Unknown word %s", __FUNCTION__, word, check );
                        fread_to_eol( fp );
                     }
                  }
               }
               fMatch = true;
               break;
            }
            if( !strcmp( word, "End" ) )
            {
               if( !oldroom )
               {
                  iHash = vnum % MKH;
                  pRoomIndex->next = room_index_hash[iHash];
                  room_index_hash[iHash] = pRoomIndex;
                  LINK( pRoomIndex, tarea->first_room, tarea->last_room, next_aroom, prev_aroom );
                  pRoomIndex->avnum = tarea->vnum;
                  top_room++;
                  if( xIS_SET( pRoomIndex->room_flags, ROOM_EXPLORER ) )
                     top_explorer++;
               }
               fMatch = true;
               break;
            }
            if( !strcmp( word, "E" ) )
            {
               EXTRA_DESCR_DATA *ed;

               CREATE( ed, EXTRA_DESCR_DATA, 1 );
               ed->keyword = fread_string( fp );
               ed->description = fread_string( fp );
               LINK( ed, pRoomIndex->first_extradesc, pRoomIndex->last_extradesc, next, prev );
               top_ed++;
               fMatch = true;
               break;
            }
            break;

         case 'F':
            if( !strcmp( word, "Flags" ) )
            {
               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  value = get_flag( flag, r_flags, ROOM_MAX );
                  if( value < 0 || value >= ROOM_MAX )
                     bug( "%s: Unknown flag: %s", __FUNCTION__, flag );
                  else
                     xSET_BIT( pRoomIndex->room_flags, value );
               }
               fMatch = true;
               break;
            }
            break;

         case 'M':
            if( !strcmp( word, "MR" ) )
            {
               load_room_reset( pRoomIndex, fp, 2 );
               fMatch = true;
               break;
            }
            break;

         case 'N':
            KEY( "Name", pRoomIndex->name, fread_string( fp ) );
            if( !strcmp( word, "NR" ) )
            {
               load_room_reset( pRoomIndex, fp, 1 );
               fMatch = true;
               break;
            }
            break;

         case 'R':
            if( !strcmp( word, "R" ) )
            {
               load_room_reset( pRoomIndex, fp, 0 );
               fMatch = true;
               break;
            }
            break;

         case 'S':
            SKEY( "Sector", pRoomIndex->sector_type, fp, sect_flags, SECT_MAX );
            break;

         case 'T':
            KEY( "Teledelay", pRoomIndex->tele_delay, fread_number( fp ) );
            KEY( "Televnum", pRoomIndex->tele_vnum, fread_number( fp ) );
            KEY( "Tunnel", pRoomIndex->tunnel, fread_number( fp ) );
            break;

         case 'V':
            if( !strcmp( word, "Vnum" ) )
            {
               vnum = fread_number( fp );
               tmpBootDb = fBootDb;
               fBootDb = false;
               if( get_room_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     bug( "%s: vnum %d duplicated.", __FUNCTION__, vnum );
                     if( unfoldload )
                     {
                        unfoldbadload = true;
                        return;
                     }
                     shutdown_mud( "duplicate vnum" );
                     exit( 1 );
                  }
                  else
                  {
                     pRoomIndex = get_room_index( vnum );
                     log_printf_plus( LOG_BUILD, sysdata.perm_log, "Cleaning room: %d", vnum );
                     clean_room( pRoomIndex );
                     oldroom = true;
                  }
               }
               else
               {
                  oldroom = false;
                  pRoomIndex = new_room( vnum );
                  ++tarea->top_room;
                  pRoomIndex->area = tarea;
                  fBootDb = tmpBootDb;
                  if( fBootDb )
                  {
                     if( !tarea->low_vnum )
                        tarea->low_vnum = vnum;
                     if( vnum > tarea->hi_vnum )
                        tarea->hi_vnum = vnum;
                  }
               }
               pRoomIndex->light = 0;
               pRoomIndex->first_exit = pRoomIndex->last_exit = NULL;
               pRoomIndex->sector_type = SECT_INSIDE;
               fMatch = true;
               break;
            }
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   bug( "%s: #0 not found?", __FUNCTION__ );
}

/* Load soft / hard area ranges. */
void load_ranges( AREA_DATA *tarea, FILE *fp )
{
   char *ln;
   int x1, x2, x3, x4;

   ln = fread_line( fp );
   sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );
   tarea->low_soft_range = x1;
   tarea->hi_soft_range = x2;
   tarea->low_hard_range = x3;
   tarea->hi_hard_range = x4;
}

/* Load climate information for the area - Fireblade */
void load_climate( AREA_DATA *tarea, FILE *fp )
{
   tarea->weather->climate_temp = fread_number( fp );
   tarea->weather->climate_precip = fread_number( fp );
   tarea->weather->climate_wind = fread_number( fp );
}

/* Load data for a neghboring weather system - Fireblade */
void load_neighbor( AREA_DATA *tarea, FILE *fp )
{
   NEIGHBOR_DATA *tnew;

   CREATE( tnew, NEIGHBOR_DATA, 1 );
   tnew->next = NULL;
   tnew->prev = NULL;
   tnew->address = NULL;
   tnew->name = fread_string( fp );
   LINK( tnew, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
   ROOM_INDEX_DATA *pRoomIndex;
   EXIT_DATA *pexit, *pexit_next, *r_exit;
   int iHash;

   for( iHash = 0; iHash < MKH; iHash++ )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         bool fexit = false;

         for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit_next )
         {
            pexit_next = pexit->next;
            pexit->rvnum = pRoomIndex->vnum;
            if( pexit->vnum <= 0 || !( pexit->to_room = get_room_index( pexit->vnum ) ) )
            {
               if( fBootDb )
                  boot_log( "%s: room %d, exit %s leads to bad vnum (%d)", __FUNCTION__,
                            pRoomIndex->vnum, dir_name[pexit->vdir], pexit->vnum );

               bug( "%s: Deleting %s exit in room %d", __FUNCTION__, dir_name[pexit->vdir], pRoomIndex->vnum );
               extract_exit( pRoomIndex, pexit );
            }
            else
               fexit = true;
         }
      }
   }

   /* Set all the rexit pointers - Thoric */
   for( iHash = 0; iHash < MKH; iHash++ )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next )
         {
            if( pexit->to_room && !pexit->rexit )
            {
               r_exit = get_exit_to( pexit->to_room, rev_dir[pexit->vdir], pRoomIndex->vnum );
               if( r_exit )
               {
                  pexit->rexit = r_exit;
                  r_exit->rexit = pexit;
               }
            }
         }
      }
   }
}

/*
 * (prelude...) This is going to be fun... NOT!
 * (conclusion) QSort is f*cked!
 */
int exit_comp( EXIT_DATA **xit1, EXIT_DATA **xit2 )
{
   int d1, d2;

   d1 = ( *xit1 )->vdir;
   d2 = ( *xit2 )->vdir;

   if( d1 < d2 )
      return -1;
   if( d1 > d2 )
      return 1;
   return 0;
}

void sort_exits( ROOM_INDEX_DATA *room )
{
   EXIT_DATA *pexit;
   EXIT_DATA *exits[MAX_REXITS];
   int x, nexits;

   nexits = 0;
   for( pexit = room->first_exit; pexit; pexit = pexit->next )
   {
      exits[nexits++] = pexit;
      if( nexits > MAX_REXITS )
      {
         bug( "%s: more than %d exits in room... fatal", __FUNCTION__, nexits );
         return;
      }
   }
   qsort( &exits[0], nexits, sizeof( EXIT_DATA *), ( int ( * )( const void *, const void * ) )exit_comp );
   for( x = 0; x < nexits; x++ )
   {
      if( x > 0 )
         exits[x]->prev = exits[x - 1];
      else
      {
         exits[x]->prev = NULL;
         room->first_exit = exits[x];
      }
      if( x >= ( nexits - 1 ) )
      {
         exits[x]->next = NULL;
         room->last_exit = exits[x];
      }
      else
         exits[x]->next = exits[x + 1];
   }
}

void randomize_exits( ROOM_INDEX_DATA *room, short maxdir )
{
   EXIT_DATA *pexit;
   int nexits, d0, d1, count, door;
   int vdirs[MAX_REXITS];

   nexits = 0;
   for( pexit = room->first_exit; pexit; pexit = pexit->next )
      vdirs[nexits++] = pexit->vdir;

   for( d0 = 0; d0 < nexits; d0++ )
   {
      if( vdirs[d0] > maxdir )
         continue;
      count = 0;
      while( vdirs[( d1 = number_range( d0, nexits - 1 ) )] > maxdir || ++count > 5 );
      if( vdirs[d1] > maxdir )
         continue;
      door = vdirs[d0];
      vdirs[d0] = vdirs[d1];
      vdirs[d1] = door;
   }
   count = 0;
   for( pexit = room->first_exit; pexit; pexit = pexit->next )
      pexit->vdir = vdirs[count++];

   sort_exits( room );
}

/* Repopulate areas periodically. */
void area_update( void )
{
   AREA_DATA *pArea;

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      CHAR_DATA *pch;
      int reset_age = pArea->reset_frequency ? pArea->reset_frequency : 15;

      if( ( reset_age == -1 && pArea->age == -1 ) || ++pArea->age < ( reset_age - 1 ) )
         continue;

      /* Check for PC's. */
      if( pArea->nplayer > 0 && pArea->age == ( reset_age - 1 ) )
      {
         char buf[MSL];

         /* Rennard */
         if( pArea->resetmsg )
            snprintf( buf, sizeof( buf ), "%s\r\n", pArea->resetmsg );
         else
            mudstrlcpy( buf, "You hear some squeaking sounds...\r\n", sizeof( buf ) );
         for( pch = first_char; pch; pch = pch->next )
         {
            if( !is_npc( pch ) && is_awake( pch ) && pch->in_room && pch->in_room->area == pArea )
            {
               set_char_color( AT_RESET, pch );
               send_to_char( buf, pch );
            }
         }
      }

      /* Check age and reset. */
      if( pArea->nplayer == 0 || pArea->age >= reset_age )
      {
         reset_area( pArea );
         if( reset_age == -1 )
            pArea->age = -1;
         else
            pArea->age = number_range( 0, reset_age / 5 );
      }
   }
}

/* Create an instance of a mobile. */
CHAR_DATA *create_mobile( MOB_INDEX_DATA *pMobIndex )
{
   CHAR_DATA *mob;
   int stat;

   if( !pMobIndex )
   {
      bug( "%s: NULL pMobIndex.", __FUNCTION__ );
      exit( 1 );
   }

   CREATE( mob, CHAR_DATA, 1 );
   if( !mob )
   {
      bug( "%s: mob is still NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }

   clear_char( mob );
   mob->pIndexData = pMobIndex;
   mob->name = QUICKLINK( pMobIndex->name );
   mob->short_descr = QUICKLINK( pMobIndex->short_descr );
   mob->long_descr = QUICKLINK( pMobIndex->long_descr );
   mob->description = QUICKLINK( pMobIndex->description );
   mob->spec_fun = pMobIndex->spec_fun;
   mob->spec_funname = QUICKLINK( pMobIndex->spec_funname );
   mob->mpscriptpos = 0;
   mob->level = number_range( pMobIndex->level - 1, pMobIndex->level + 1 );
   mob->level = URANGE( 1, mob->level, MAX_LEVEL );
   mob->act = pMobIndex->act;
   mob->reset = NULL;
   if( xIS_SET( mob->act, ACT_MOBINVIS ) )
      mob->mobinvis = mob->level;
   mob->affected_by = pMobIndex->affected_by;
   mob->alignment = pMobIndex->alignment;
   mob->sex = pMobIndex->sex;

   if( pMobIndex->ac )
      mob->armor = pMobIndex->ac;
   else
      mob->armor = interpolate( mob->level, 100, -100 );

   mob->max_hit = UMAX( 1, number_range( pMobIndex->minhit, pMobIndex->maxhit ) );
   mob->hit = mob->max_hit;

   mob->gold = pMobIndex->gold;
   mob->position = pMobIndex->defposition;
   mob->defposition = pMobIndex->defposition;

   for( stat = 0; stat < STAT_MAX; stat++ )
      mob->perm_stats[stat] = pMobIndex->perm_stats[stat];

   mob->hitroll = pMobIndex->hitroll;
   mob->damroll = pMobIndex->damroll;
   mob->xflags = pMobIndex->xflags;
   mob->saving_poison_death = pMobIndex->saving_poison_death;
   mob->saving_wand = pMobIndex->saving_wand;
   mob->saving_para_petri = pMobIndex->saving_para_petri;
   mob->saving_breath = pMobIndex->saving_breath;
   mob->saving_spell_staff = pMobIndex->saving_spell_staff;
   mob->height = pMobIndex->height;
   mob->weight = pMobIndex->weight;

   for( stat = 0; stat < RIS_MAX; stat++ )
      mob->resistant[stat] = pMobIndex->resistant[stat];

   mob->attacks = pMobIndex->attacks;
   mob->defenses = pMobIndex->defenses;
   mob->numattacks = pMobIndex->numattacks;
   mob->speaks = pMobIndex->speaks;
   mob->speaking = pMobIndex->speaking;

   add_char( mob );
   pMobIndex->count++;
   nummobsloaded++;
   return mob;
}

/* Create an instance of an object. */
OBJ_DATA *create_object( OBJ_INDEX_DATA *pObjIndex, int level )
{
   OBJ_DATA *obj;

   if( !pObjIndex )
   {
      bug( "%s: NULL pObjIndex. EXITING THE MUD...", __FUNCTION__ );
      exit( 1 );
   }

   CREATE( obj, OBJ_DATA, 1 );
   if( !obj )
   {
      bug( "%s: obj is still NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }

   obj->pIndexData = pObjIndex;
   obj->reset = NULL;
   obj->in_room = NULL;
   obj->level = level;
   obj->wear_loc = -1;
   obj->t_wear_loc = -1;
   obj->count = 1;
   obj->bsplatter = 0; /* Clean */
   obj->bstain = 0; /* Clean */
   obj->name = QUICKLINK( pObjIndex->name );
   obj->short_descr = QUICKLINK( pObjIndex->short_descr );
   obj->description = QUICKLINK( pObjIndex->description );
   obj->desc = QUICKLINK( pObjIndex->desc );
   obj->action_desc = QUICKLINK( pObjIndex->action_desc );
   obj->owner = NULL;
   obj->item_type = pObjIndex->item_type;
   obj->extra_flags = pObjIndex->extra_flags;
   obj->wear_flags = pObjIndex->wear_flags;
   obj->value[0] = pObjIndex->value[0];
   obj->value[1] = pObjIndex->value[1];
   obj->value[2] = pObjIndex->value[2];
   obj->value[3] = pObjIndex->value[3];
   obj->value[4] = pObjIndex->value[4];
   obj->value[5] = pObjIndex->value[5];
   obj->weight = pObjIndex->weight;
   obj->cost = pObjIndex->cost;

   /* Mess with object properties. */
   switch( obj->item_type )
   {
      default:
         break;

      case ITEM_COOK:
      case ITEM_FOOD:
      case ITEM_FISH:
         obj->timer = obj->value[1];
         break;
   }

   LINK( obj, first_object, last_object, next, prev );
   if( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC )
   {
      LINK( obj, first_corpse, last_corpse, next_corpse, prev_corpse );
      ++num_corpses;
   }
   LINK( obj, pObjIndex->first_copy, pObjIndex->last_copy, next_index, prev_index );
   ++pObjIndex->count;
   ++numobjsloaded;
   ++physicalobjects;

   randomize_obj( obj );

   return obj;
}

/* Clear a new character. */
void clear_char( CHAR_DATA *ch )
{
   int stat;

   ch->editor = NULL;
   ch->name = NULL;
   ch->short_descr = NULL;
   ch->long_descr = NULL;
   ch->description = NULL;
   ch->next = ch->prev = NULL;
   ch->reply = NULL;
   ch->retell = NULL;
   ch->first_hunting = ch->last_hunting = NULL;
   ch->first_hating = ch->last_hating = NULL;
   ch->first_fearing = ch->last_fearing = NULL;
   ch->first_carrying = ch->last_carrying = NULL;
   ch->next_in_room = ch->prev_in_room = NULL;
   ch->next_in_group = ch->prev_in_group = NULL;
   ch->group = NULL;
   ch->fighting = NULL;
   ch->first_affect = ch->last_affect = NULL;
   ch->first_host = ch->last_host = NULL;
   ch->prev_cmd = ch->last_cmd = NULL;
   ch->dest_buf = NULL;
   ch->alloc_ptr = NULL;
   ch->spare_ptr = NULL;
   ch->mount = NULL;
   ch->mounter = NULL;
   ch->morph = NULL;
   ch->was_in_room = NULL;
   xCLEAR_BITS( ch->act );
   xCLEAR_BITS( ch->affected_by );
   xCLEAR_BITS( ch->no_affected_by );
   xCLEAR_BITS( ch->xflags );
   xCLEAR_BITS( ch->speaking );
   xSET_BIT( ch->speaking, LANG_COMMON );
   xCLEAR_BITS( ch->speaks );
   xSET_BIT( ch->speaks, LANG_COMMON );
   ch->logon = current_time;
   ch->bsplatter = 0;
   ch->armor = 0;
   ch->position = POS_STANDING;
   ch->mobinvis = 0;
   ch->practice = 0;
   ch->hit = 20;
   ch->max_hit = 20;
   ch->mana = 100;
   ch->max_mana = 100;
   ch->move = 100;
   ch->max_move = 100;
   ch->height = 72;
   ch->weight = 180;
   ch->race = 0;
   ch->substate = 0;
   ch->tempnum = 0;
   ch->mental_state = -10;
   ch->saving_poison_death = 0;
   ch->saving_wand = 0;
   ch->saving_para_petri = 0;
   ch->saving_breath = 0;
   ch->saving_spell_staff = 0;
   ch->style = STYLE_FIGHTING;
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      ch->perm_stats[stat] = 13;
      ch->mod_stats[stat] = 0;
   }
   for( stat = 0; stat < RIS_MAX; stat++ )
      ch->resistant[stat] = 0;
}

/* Free a character. */
void free_char( CHAR_DATA *ch )
{
   CHAR_DATA *pet, *pet_next;
   OBJ_DATA *obj;
   AFFECT_DATA *paf;
   TIMER *timer;
   MPROG_ACT_LIST *mpact, *mpact_next;
   HOST_DATA *host, *host_next;
   EXP_DATA *fexp, *nexp;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return;
   }

   if( ch->desc )
      bug( "%s: char still has descriptor.", __FUNCTION__ );

   if( ch->morph )
      DISPOSE( ch->morph );

   if( ch->group )
      remove_char_from_group( ch );

   while( ( obj = ch->last_carrying ) )
      extract_obj( obj );

   while( ( paf = ch->last_affect ) )
      affect_remove( ch, paf );

   while( ( timer = ch->first_timer ) )
      extract_timer( ch, timer );

   if( ch->editor )
      stop_editing( ch );

   for( host = ch->first_host; host; host = host_next )
   {
      host_next = host->next;
      STRFREE( host->host );
      UNLINK( host, ch->first_host, ch->last_host, next, prev );
      DISPOSE( host );
   }

   stop_hhf( ch );

   STRFREE( ch->name );
   STRFREE( ch->short_descr );
   STRFREE( ch->long_descr );
   STRFREE( ch->description );
   STRFREE( ch->spec_funname );

   stop_hunting( ch, NULL, true );
   stop_hating( ch, NULL, true );
   stop_fearing( ch, NULL, true );
   free_fight( ch );

   if( ch->pnote )
      free_note( ch->pnote );

   if( ch->pcdata )
   {
      KILLED_DATA *killed, *killednext;
      MCLASS_DATA *mclass, *mnext;
      IGNORE_DATA *temp, *next;
      PER_HISTORY *phistory;

      for( killed = ch->pcdata->first_killed; killed; killed = killednext )
      {
          killednext = killed->next;
          UNLINK( killed, ch->pcdata->first_killed, ch->pcdata->last_killed, next, prev );
          DISPOSE( killed );
      }

      for( pet = ch->pcdata->first_pet; pet; pet = pet_next )
      {
         pet_next = pet->next_pet;
         UNLINK( pet, ch->pcdata->first_pet, ch->pcdata->last_pet, next_pet, prev_pet );
         extract_char( pet, true );
      }

      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mnext )
      {
         mnext = mclass->next;
         UNLINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
         DISPOSE( mclass );
      }

      for( fexp = ch->pcdata->first_explored; fexp; fexp = nexp )
      {
         nexp = fexp->next;
         UNLINK( fexp, ch->pcdata->first_explored, ch->pcdata->last_explored, next, prev );
         DISPOSE( fexp );
      }

      free_all_friends( ch );

      if( ch->pcdata->gnote )
         free_note( ch->pcdata->gnote );

      /* free up memory allocated to stored ignored names */
      for( temp = ch->pcdata->first_ignored; temp; temp = next )
      {
         next = temp->next;
         UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
         STRFREE( temp->name );
         DISPOSE( temp );
      }

      STRFREE( ch->pcdata->channels );
      STRFREE( ch->pcdata->spouse );
      STRFREE( ch->pcdata->filename );
      STRFREE( ch->pcdata->pwd );
      STRFREE( ch->pcdata->bamfin );
      STRFREE( ch->pcdata->bamfout );
      STRFREE( ch->pcdata->rank );
      STRFREE( ch->pcdata->title );
      STRFREE( ch->pcdata->bio );
      STRFREE( ch->pcdata->bestowments );
      STRFREE( ch->pcdata->homepage );
      STRFREE( ch->pcdata->email );
      STRFREE( ch->pcdata->msn );
      STRFREE( ch->pcdata->yahoo );
      STRFREE( ch->pcdata->gtalk );
      STRFREE( ch->pcdata->prompt );
      STRFREE( ch->pcdata->fprompt );
      STRFREE( ch->pcdata->subprompt );
      while( ch->pcdata->last_tell )
      {
         phistory = ch->pcdata->last_tell;
         UNLINK( phistory, ch->pcdata->first_tell, ch->pcdata->last_tell, next, prev );
         free_phistory( phistory );
      }
      while( ch->pcdata->last_say )
      {
         phistory = ch->pcdata->last_say;
         UNLINK( phistory, ch->pcdata->first_say, ch->pcdata->last_say, next, prev );
         free_phistory( phistory );
      }
      while( ch->pcdata->last_yell )
      {
         phistory = ch->pcdata->last_yell;
         UNLINK( phistory, ch->pcdata->first_yell, ch->pcdata->last_yell, next, prev );
         free_phistory( phistory );
      }
      while( ch->pcdata->last_whisper )
      {
         phistory = ch->pcdata->last_whisper;
         UNLINK( phistory, ch->pcdata->first_whisper, ch->pcdata->last_whisper, next, prev );
         free_phistory( phistory );
      }
#ifdef IMC
      imc_freechardata( ch );
#endif
      UNLINK( ch->pcdata, first_pc, last_pc, next, prev );
      ch->pcdata->character = NULL;
      DISPOSE( ch->pcdata );
   }

   for( mpact = ch->mpact; mpact; mpact = mpact_next )
   {
      mpact_next = mpact->next;
      STRFREE( mpact->buf );
      DISPOSE( mpact );
   }

   DISPOSE( ch );
}

/* Get an extra description from a list. */
char *get_extra_descr( const char *name, EXTRA_DESCR_DATA *ed )
{
   for( ; ed; ed = ed->next )
      if( is_name( name, ed->keyword ) )
         return ed->description;
   return NULL;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index( int vnum )
{
   MOB_INDEX_DATA *pMobIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pMobIndex = mob_index_hash[vnum % MKH]; pMobIndex; pMobIndex = pMobIndex->next )
      if( pMobIndex->vnum == vnum )
         return pMobIndex;

   if( fBootDb )
      bug( "%s: bad vnum %d.", __FUNCTION__, vnum );

   return NULL;
}

/*
 * Translates obj virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index( int vnum )
{
   OBJ_INDEX_DATA *pObjIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pObjIndex = obj_index_hash[vnum % MKH]; pObjIndex; pObjIndex = pObjIndex->next )
      if( pObjIndex->vnum == vnum )
         return pObjIndex;

   if( fBootDb )
      bug( "%s: bad vnum %d.", __FUNCTION__, vnum );

   return NULL;
}

/*
 * Translates room virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index( int vnum )
{
   ROOM_INDEX_DATA *pRoomIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pRoomIndex = room_index_hash[vnum % MKH]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      if( pRoomIndex->vnum == vnum )
         return pRoomIndex;

   if( fBootDb )
      bug( "%s: bad vnum %d.", __FUNCTION__, vnum );

   return NULL;
}

/*
 * Added lots of EOF checks, as most of the file crashes are based on them.
 * If an area file encounters EOF, the fread_* functions will shutdown the
 * MUD, as all area files should be read in in full or bad things will
 * happen during the game.  Any files loaded in without fBootDb which
 * encounter EOF will return what they have read so far.   These files
 * should include player files, and in-progress areas that aren't loaded
 * upon bootup.
 * -- Altrag
 */

/* Read a letter from a file. */
char fread_letter( FILE *fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return '\0';
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   return c;
}

/* Read a number from a file. */
int fread_number( FILE *fp )
{
   char c;
   int number;
   bool sign;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = false;
   if( c == '+' )
   {
      c = getc( fp );
   }
   else if( c == '-' )
   {
      sign = true;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_number( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

unsigned int fread_un_number( FILE *fp )
{
   unsigned int number;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   if( !isdigit( c ) )
   {
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/* Read a time from a file. */
time_t fread_time( FILE *fp )
{
   time_t number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = false;
   if( c == '+' )
   {
      c = getc( fp );
   }
   else if( c == '-' )
   {
      sign = true;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = ( 0 - number );

   if( c == '|' )
      number += fread_time( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/* Read a double from a file. */
double fread_double( FILE *fp )
{
   double number, dcount, ndecim;
   bool sign, fdecimal;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0.0;
   dcount = 0.1;

   sign = false;
   fdecimal = false;

   if( c == '+' )
   {
      c = getc( fp );
   }
   else if( c == '-' )
   {
      sign = true;
      c = getc( fp );
   }

   if( !isdigit( c ) && c != '.' )
   {
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) || c == '.' )
   {
      ndecim = 0.0;

      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      if( c == '.' )
      {
         fdecimal = true;
         c = getc( fp );
         continue;
      }
      if( !fdecimal )
         number = number * 10 + c - '0';
      else
      {
         ndecim = ( ( c - '0' ) * dcount );
         number += ndecim;
         dcount = ( dcount * .1 ); /* Update dcount so its ready for next one */
      }
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_double( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/* custom str_dup using create -Thoric */
char *str_dup( char const *str )
{
   static char *ret;
   int len;

   if( !str )
      return NULL;

   len = strlen( str ) + 1;

   CREATE( ret, char, len );
   mudstrlcpy( ret, str, MSL );
   return ret;
}

/* Read a string from file and return it */
char *fread_flagstring( FILE *fp )
{
   static char flagstring[MSL];
   char *plast;
   char c;
   int ln;

   plast = flagstring;
   flagstring[0] = '\0';
   ln = 0;

   /* Skip blanks. Read first char. */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return (char *)"";
      }
      c = getc( fp );
   }
   while( isspace( c ) );
   if( ( *plast++ = c ) == '~' )
      return (char *)"";

   for( ;; )
   {
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s: string too long", __FUNCTION__ );
         *plast = '\0';
         return flagstring;
      }
      switch( *plast = getc( fp ) )
      {
         default:
            plast++;
            ln++;
            break;

         case EOF:
            bug( "%s: EOF", __FUNCTION__ );
            if( fBootDb )
               exit( 1 );
            *plast = '\0';
            return flagstring;
            break;

         case '\n':
            plast++;
            ln++;
            *plast++ = '\r';
            ln++;
            break;

         case '\r':
            break;

         case '~':
            *plast = '\0';
            return flagstring;
      }
   }
}

/* Read a string from file fp */
char *fread_string( FILE *fp )
{
   char buf[MSL];

   mudstrlcpy( buf, fread_flagstring( fp ), MSL );
   if( buf == NULL || buf[0] == '\0' )
      return NULL;
   return STRALLOC( buf );
}

/* Read to end of line (for comments). */
void fread_to_eol( FILE *fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         return;
      }
      c = getc( fp );
   }
   while( c != '\r' && c != '\n' );

   do
   {
      c = getc( fp );
   }
   while( c == '\r' || c == '\n' );

   ungetc( c, fp );
}

/* Read to end of line into static buffer - Thoric */
char *fread_line( FILE *fp )
{
   static char line[MSL];
   char *pline;
   char c;
   int ln;

   pline = line;
   line[0] = '\0';
   ln = 0;

   /* Skip blanks. Read first char. */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         mudstrlcpy( line, "", sizeof( line ) );
         return line;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   ungetc( c, fp );
   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         *pline = '\0';
         return line;
      }
      c = getc( fp );
      *pline++ = c;
      ln++;
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s: line too long", __FUNCTION__ );
         break;
      }
   }
   while( c != '\r' && c != '\n' );

   do
   {
      c = getc( fp );
   }
   while( c == '\r' || c == '\n' );

   ungetc( c, fp );
   *pline = '\0';
   return line;
}

/* Read one word (into static buffer). */
char *fread_word( FILE *fp )
{
   static char word[MIL];
   char *pword;
   char cEnd;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         word[0] = '\0';
         return word;
      }
      cEnd = getc( fp );
   }
   while( isspace( cEnd ) );

   if( cEnd == '\'' || cEnd == '"' )
   {
      pword = word;
   }
   else
   {
      word[0] = cEnd;
      pword = word + 1;
      cEnd = ' ';
   }

   for( ; pword < word + MIL; pword++ )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
            exit( 1 );
         *pword = '\0';
         return word;
      }
      *pword = getc( fp );
      if( cEnd == ' ' ? isspace( *pword ) : *pword == cEnd )
      {
         if( cEnd == ' ' )
            ungetc( *pword, fp );
         *pword = '\0';
         return word;
      }
   }
   bug( "%s: word too long", __FUNCTION__ );
   return NULL;
}

CMDF( do_memory )
{
   char arg[MIL];
#ifdef HASHSTR
   int hash;
#endif

   set_char_color( AT_PLAIN, ch );
   argument = one_argument( argument, arg );
   send_to_char( "\r\n&wSystem Memory [arguments - hash, check, showhigh]\r\n", ch );
   ch_printf( ch, "&wObjAffects: &W%5d  &wAreas:   &W%5d\r\n", top_affect, top_area );
   ch_printf( ch, "&wExtDes:     &W%5d  &wExits:   &W%5d\r\n", top_ed, top_exit );
   ch_printf( ch, "&wHelps:      &W%5d  &wResets:  &W%5d\r\n", top_help, top_reset );
   ch_printf( ch, "&wCommands:   &W%5d  &wSocials: &W%5d\r\n", top_command, top_social );
   ch_printf( ch, "&wIdxMobs:    &W%5d  &wMobiles: &W%5d\r\n", top_mob_index, nummobsloaded );
   ch_printf( ch, "&wIdxObjs:    &W%5d  &wObjs:    &W%5d(%d)\r\n", top_obj_index, numobjsloaded, physicalobjects );
   ch_printf( ch, "&wCorpses:    &W%5d  &wExplorer:&W%5d\r\n", num_corpses, top_explorer );
   ch_printf( ch, "&wQuestions:  &W%5d  &wAnswers: &W%5d\r\n", top_trivia, top_answers );
   ch_printf( ch, "&wRooms:      &W%5d  &wNews:    &W%5d\r\n", top_room, top_news );
   ch_printf( ch, "&wBTI:        &W%5d  &wCurCq's  &W%5d\r\n", top_bti, cur_qchars );
   ch_printf( ch, "&wShops:      &W%5d  &wRepShps: &W%5d\r\n", top_shop, top_repair );
   ch_printf( ch, "&wPlayers:    &W%5d  &wTopSn:   &W%5d(%d)\r\n", num_descriptors, top_sn, MAX_SKILL );
   ch_printf( ch, "&wUsing MCCP: &W%5d\r\n", mccpusers );
   ch_printf( ch, "&wMaxplrs:    &W%5d  &wRaces:   &W%5d(%d)\r\n", sysdata.maxplayers, MAX_PC_RACE, MAX_RACE );
   ch_printf( ch, "&wMaxEver:    &W%5d  &wClasses: &W%5d(%d)\r\n", sysdata.alltimemax, MAX_PC_CLASS, MAX_CLASS );
   ch_printf( ch, "&wMaxEver was recorded &W%s\r\n", distime( sysdata.time_of_max ) );

   if( arg == NULL || arg[0] == '\0' )
      return;

#ifndef HASHSTR
   send_to_char( "Hash strings not enabled.\r\n", ch );
#else
   if( !str_cmp( arg, "check" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: memory check <string to check for>\r\n", ch );
         return;
      }
      send_to_char( check_hash( argument ), ch );
      return;
   }
   if( !str_cmp( arg, "showhigh" ) )
   {
      show_high_hash( atoi( argument ) );
      send_to_char( "You will have to look in the current log file to see the hash info you wanted.\r\n", ch );
      return;
   }
   if( argument[0] != '\0' )
      hash = atoi( argument );
   else
      hash = -1;
   if( !str_cmp( arg, "hash" ) )
   {
      ch_printf( ch, "Hash statistics:\r\n%s", hash_stats( ) );
      if( hash != -1 )
         hash_dump( hash );
   }
#endif
}

/*
 * Generate a random number.
 * Ooops was (number_mm() % to) + from which doesn't work -Shaddai
 */
int number_range( int from, int to )
{
   if( ( to - from ) < 1 )
      return from;
   return ( ( number_mm( ) % ( to - from + 1 ) ) + from );
}

/*
 * Generate a percentile roll.
 * number_mm() % 100 only does 0-99, changed to do 1-100 -Shaddai
 */
int number_percent( void )
{
   return ( number_mm( ) % 100 ) + 1;
}

/* Generate a random door. */
int number_door( void )
{
   int door;

   while( ( door = number_mm( ) & ( 16 - 1 ) ) > 9 )
      ;

   return door;
}

int number_bits( int width )
{
   return number_mm( ) & ( ( 1 << width ) - 1 );
}

/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
static int rgiState[2 + 55];

void init_mm( void )
{
   int *piState;
   int iState;

   piState = &rgiState[2];

   piState[-2] = 55 - 55;
   piState[-1] = 55 - 24;

   piState[0] = ( ( int )current_time ) & ( ( 1 << 30 ) - 1 );
   piState[1] = 1;
   for( iState = 2; iState < 55; iState++ )
      piState[iState] = ( piState[iState - 1] + piState[iState - 2] ) & ( ( 1 << 30 ) - 1 );
}

int number_mm( void )
{
   int *piState;
   int iState1;
   int iState2;
   int iRand;

   piState = &rgiState[2];
   iState1 = piState[-2];
   iState2 = piState[-1];
   iRand = ( piState[iState1] + piState[iState2] ) & ( ( 1 << 30 ) - 1 );
   piState[iState1] = iRand;
   if( ++iState1 == 55 )
      iState1 = 0;
   if( ++iState2 == 55 )
      iState2 = 0;
   piState[-2] = iState1;
   piState[-1] = iState2;
   return iRand >> 6;
}

/* Roll some dice. - Thoric */
int dice( int number, int size )
{
   int idice;
   int sum;

   switch( size )
   {
      case 0:
         return 0;
      case 1:
         return number;
   }

   for( idice = 0, sum = 0; idice < number; idice++ )
      sum += number_range( 1, size );

   return sum;
}

/* Simple linear interpolation. */
int interpolate( int level, int value_00, int value_32 )
{
   return value_00 + level * ( value_32 - value_00 ) / 32;
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
   for( ; *str != '\0'; str++ )
   {
      if( *str == '~' )
         *str = '-';
   }
}

/*
 * Encodes the tildes in a string.				-Thoric
 * Used for player-entered strings that go into disk files.
 */
void hide_tilde( char *str )
{
   for( ; *str != '\0'; str++ )
   {
      if( *str == '~' )
         *str = HIDDEN_TILDE;
   }
}

char *show_tilde( const char *str )
{
   static char buf[MSL];
   char *bufptr;

   bufptr = buf;
   for( ; *str != '\0'; str++, bufptr++ )
   {
      if( *str == HIDDEN_TILDE )
         *bufptr = '~';
      else
         *bufptr = *str;
   }
   *bufptr = '\0';

   return buf;
}

/*
 * Compare strings, case insensitive.
 * Return true if different
 *   (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
   /* If both empty they match I think */
   if( !astr && !bstr )
      return false;

   /* If just one or the other is NULL they dont match */
   if( !astr || !bstr )
      return true;

   /* Check the strings to see if they don't match */
   for( ; *astr || *bstr; astr++, bstr++ )
   {
      if( LOWER( *astr ) != LOWER( *bstr ) )
         return true;
   }

   /* If you get here then they matched */
   return false;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return true if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr )
{
   /* If both empty they match I think */
   if( !astr && !bstr )
      return false;

   /* If just one or the other is NULL they dont match */
   if( !astr || !bstr )
      return true;

   for( ; *astr; astr++, bstr++ )
   {
      if( LOWER( *astr ) != LOWER( *bstr ) )
         return true;
   }

   /* If you get here then they matched */
   return false;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns true is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
   int sstr1, sstr2, ichar;
   char c0;

   if( ( c0 = LOWER( astr[0] ) ) == '\0' )
      return false;

   sstr1 = strlen( astr );
   sstr2 = strlen( bstr );

   for( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
      if( c0 == LOWER( bstr[ichar] ) && !str_prefix( astr, bstr + ichar ) )
         return false;

   return true;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return true if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
   int sstr1, sstr2;

   sstr1 = strlen( astr );
   sstr2 = strlen( bstr );
   if( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
      return false;
   else
      return true;
}

/*
 * Returns an initial-capped string.
 * Rewritten by FearItself@AvP
 */
char *capitalize( const char *str )
{ 
   static char buf[MSL];
   char *dest = buf;
   enum { Normal, Color } state = Normal;
   bool bFirst = true;
   char c;

   while( (c = *str++) )
   {
      if( state == Normal )
      {
         if( c == '&' || c == '^' || c == '}' )
         {
            state = Color;
         }
         else if( isalpha(c) )
         {
            c = bFirst ? toupper(c) : tolower(c);
            bFirst = false;
         }
      }
      else
      {
         state = Normal;
      }
      *dest++ = c;
   }
   *dest = c;

   return buf;
}

/* Returns a lowercase string. */
char *strlower( const char *str )
{
   static char strlow[MSL];
   int i;

   for( i = 0; str[i] != '\0'; i++ )
      strlow[i] = LOWER( str[i] );
   strlow[i] = '\0';
   return strlow;
}

/* Returns an uppercase string. */
char *strupper( const char *str )
{
   static char strup[MSL];
   int i;

   strup[0] = '\0';
   for( i = 0; str[i] != '\0'; i++ )
      strup[i] = UPPER( str[i] );
   strup[i] = '\0';
   return strup;
}

/* Returns true or false if a letter is a vowel - Thoric */
bool isavowel( char letter )
{
   char c;

   c = LOWER( letter );
   if( c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' )
      return true;
   else
      return false;
}

/* Shove either "a " or "an " onto the beginning of a string - Thoric */
char *aoran( const char *str )
{
   static char temp[MSL];

   if( !str )
   {
      bug( "%s: NULL str", __FUNCTION__ );
      return (char *)"";
   }

   if( isavowel( str[0] ) || ( strlen( str ) > 1 && LOWER( str[0] ) == 'y' && !isavowel( str[1] ) ) )
      mudstrlcpy( temp, "an ", sizeof( temp ) );
   else
      mudstrlcpy( temp, "a ", sizeof( temp ) );
   mudstrlcat( temp, str, sizeof( temp ) );
   return temp;
}

/* Added for Kayle's MProg Variable Expansion, 8/25/2010 */
/* Simple function to add a single letter to the end of something. */
void add_letter( char *string, char letter )
{
  char buf[MSL];

  sprintf( buf, "%c", letter );
  mudstrlcat( string, buf, sizeof( buf ) );
}

/* Added for Kayle's MProg Variable Expansion, 8/25/2010 */
const char *format( const char *fmt, ... ) 
 { 
    static char newstring[MIL]; 
    char buf[MSL * 2];        /* better safe than sorry */ 
    va_list args; 
  
    newstring[0] = '\0'; 
  
    if( fmt[0] == '\0' ) 
       return " "; 
  
    va_start( args, fmt );  
    vsnprintf( buf, MSL * 2, fmt, args ); 
    va_end( args ); 
  
    if( buf[0] == '\0' ) 
       return " "; 
    mudstrlcpy( newstring, buf, MIL ); 
    return newstring; 
 }

/* Append a string to a file. */
void append_file( CHAR_DATA *ch, const char *file, const char *str )
{
   FILE *fp;
   struct tm *t = localtime( &current_time );

   if( ( ch && is_npc( ch ) ) || str[0] == '\0' )
      return;

   if( !( fp = fopen( file, "a" ) ) )
   {
      perror( file );
      send_to_char( "Could not open the file!\r\n", ch );
   }
   else
   {
      fprintf( fp, "[%5d] [%2.2d:%2.2d:%2.2d %2.2d/%2.2d/%4.4d] %s: %s\n", ( ch && ch->in_room ) ? ch->in_room->vnum : 0,
         t->tm_hour, t->tm_min, t->tm_sec, ( t->tm_mon + 1 ), t->tm_mday, ( t->tm_year + 1900 ),
         ( ch && ch->name ) ? ch->name : "(Unknown)", str );
      fclose( fp );
      fp = NULL;
   }
}

/* Append a string to a file. */
void append_to_file( const char *file, const char *str )
{
   FILE *fp;

   if( !( fp = fopen( file, "a" ) ) )
      perror( file );
   else
   {
      fprintf( fp, "%s\n", str );
      fclose( fp );
      fp = NULL;
   }
}

/* Reports a bug. */
void bug( const char *str, ... )
{
   FILE *fp;
   char buf[MSL];
   int letter;
   struct stat fst;

   mudstrlcpy( buf, "[*****] BUG: ", sizeof( buf ) );
   {
      va_list param;

      va_start( param, str );
      vsnprintf( buf + strlen( buf ), ( sizeof( buf ) - strlen( buf ) ), str, param );
      va_end( param );
   }

   log_string_plus( buf, LOG_BUG, PERM_IMM );

   if( fpArea )
   {
      int iLine;
      int iChar;

      if( fpArea == stdin )
      {
         iLine = 0;
      }
      else
      {
         iChar = ftell( fpArea );
         fseek( fpArea, 0, 0 );
         for( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
         {
            while( ( letter = getc( fpArea ) ) && letter != EOF && letter != '\n' )
               ;
         }
         fseek( fpArea, iChar, 0 );
      }

      log_printf_plus( LOG_BUG, PERM_IMM, "[*****] FILE: %s LINE: %d", strArea, iLine );

      if( stat( SHUTDOWN_FILE, &fst ) != -1 )   /* file exists */
      {
         if( ( fp = fopen( SHUTDOWN_FILE, "a" ) ) )
         {
            fprintf( fp, "[*****] <FILE: %s LINE: %d> %s\n", strArea, iLine, buf );
            fclose( fp );
            fp = NULL;
         }
      }
   }
}

/* Add a string to the boot-up log - Thoric */
void boot_log( const char *str, ... )
{
   FILE *fp;
   va_list param;
   char buf[MSL];

   mudstrlcpy( buf, "[*****] BOOT: ", sizeof( buf ) );
   va_start( param, str );
   vsnprintf( buf + strlen( buf ), ( sizeof( buf ) - strlen( buf ) ), str, param );
   va_end( param );
   log_string( buf );

   if( ( fp = fopen( BOOTLOG_FILE, "a" ) ) )
   {
      fprintf( fp, "%s\n", buf );
      fclose( fp );
      fp = NULL;
   }
}

/* Dump a text file to a player, a line at a time - Thoric */
void show_file( CHAR_DATA *ch, const char *filename )
{
   FILE *fp;
   char buf[MSL];
   int c;
   int num = 0;

   if( ( fp = fopen( filename, "r" ) ) )
   {
      while( !feof( fp ) )
      {
         while( ( buf[num] = fgetc( fp ) ) != EOF
         && buf[num] != '\r' && buf[num] != '\n' && num < ( MSL - 2 ) )
            num++;
         c = fgetc( fp );
         if( ( c != '\r' && c != '\n' ) || c == buf[num] )
            ungetc( c, fp );
         buf[num++] = '\r';
         buf[num++] = '\n';
         buf[num] = '\0';
         send_to_pager( buf, ch );
         num = 0;
      }
      /* Thanks to stu <sprice@ihug.co.nz> from the mailing list in pointing This out. */
      fclose( fp );
      fp = NULL;
   }
}

/* Added as a way to delete a line from a file, Sometimes just want to remove something here and there but not all of it */
bool remove_line_from_file( const char *filename, int dline )
{
   FILE *fp, *fpw;
   char tmpfile[MIL];
   int oline = 0;
   bool ldeleted = false;

   snprintf( tmpfile, sizeof( tmpfile ), "%s.tmp", filename );

   /* File we are going to read from */
   if( !( fp = fopen( filename, "r" ) ) )
      return false;
   if( !( fpw = fopen( tmpfile, "w" ) ) )
   {
      fclose( fp );
      fp = NULL;
      return false;
   }

   while( !feof( fp ) )
   {
      if( ++oline != dline )
         fprintf( fpw, "%s", fread_line( fp ) );
      if( oline == dline )
      {
         fread_line( fp );
         ldeleted = true;
      }
   }

   fclose( fp );
   fp = NULL;
   fclose( fpw );
   fpw = NULL;

   rename( tmpfile, filename );

   return ldeleted;
}

/* Show the boot log file - Thoric */
CMDF( do_dmesg )
{
   set_pager_color( AT_LOG, ch );
   send_to_pager( "Boot Log Messages:\r\n", ch );
   show_file( ch, BOOTLOG_FILE );
}

/* wizlist builder! - Thoric */
void towizfile( const char *line )
{
   FILE *fp;
   char outline[MSL];
   int filler, xx;

   outline[0] = '\0';
   if( line && line[0] != '\0' )
   {
      filler = ( 78 - color_strlen( line ) );
      if( filler < 1 )
         filler = 1;
      filler /= 2;
      for( xx = 0; xx < filler; xx++ )
         mudstrlcat( outline, " ", sizeof( outline ) );
      mudstrlcat( outline, line, sizeof( outline ) );
   }
   mudstrlcat( outline, "\r\n", sizeof( outline ) );
   if( ( fp = fopen( WIZLIST_FILE, "a" ) ) )
   {
      fputs( outline, fp );
      fclose( fp );
      fp = NULL;
   }
}

typedef struct wizent WIZENT;
/* Structure used to build wizlist */
struct wizent
{
   WIZENT *next, *last;
   char *name;
   int level;
   short sex;
};
WIZENT *first_wiz, *last_wiz;

void add_to_wizlist( char *name, int level, short sex )
{
   WIZENT *wiz, *tmp;

   CREATE( wiz, WIZENT, 1 );
   wiz->name = STRALLOC( name );
   wiz->level = level;
   wiz->sex = sex;

   if( !first_wiz )
   {
      wiz->last = NULL;
      wiz->next = NULL;
      first_wiz = wiz;
      last_wiz = wiz;
      return;
   }

   /* insert sort, of sorts */
   for( tmp = first_wiz; tmp; tmp = tmp->next )
   {
      if( level > tmp->level )
      {
         if( !tmp->last )
            first_wiz = wiz;
         else
            tmp->last->next = wiz;
         wiz->last = tmp->last;
         wiz->next = tmp;
         tmp->last = wiz;
         return;
      }
   }

   wiz->last = last_wiz;
   wiz->next = NULL;
   last_wiz->next = wiz;
   last_wiz = wiz;
}

/* Wizlist builder - Thoric */
void make_wizlist( void )
{
   DIR *dp;
   struct dirent *dentry;
   FILE *gfp;
   EXT_BV iflags;
   WIZENT *wiz, *wiznext;
   const char *word;
   char buf[MSL], *infoflags, flag[MSL], filename[MIL];
   int ilevel, heads = 0, imps = 0, leaders = 0, builders = 0, imms = 0, guests = 0, retirees = 0, servants = 0, value;
   short sex;

   first_wiz = NULL;
   last_wiz = NULL;

   dp = opendir( GOD_DIR );

   ilevel = 0;
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         snprintf( filename, sizeof( filename ), "%s%s", GOD_DIR, dentry->d_name );
         if( ( gfp = fopen( filename, "r" ) ) )
         {
            word = feof( gfp ) ? "End" : fread_word( gfp );
            infoflags = fread_flagstring( gfp );
            if( is_number( infoflags ) )
               ilevel = atoi( infoflags );
            else
               ilevel = get_flag( infoflags, perms_flag, PERM_MAX );
            sex = SEX_NEUTRAL;
            word = feof( gfp ) ? "End" : fread_word( gfp );
            if( !str_cmp( word, "Sex" ) )
            {
               infoflags = fread_flagstring( gfp );
               value = get_flag( infoflags, sex_names, SEX_MAX );
               if( value >= SEX_NEUTRAL && value < SEX_MAX )
                  sex = value;
            }
            word = feof( gfp ) ? "End" : fread_word( gfp );
            xCLEAR_BITS( iflags );
            if( !str_cmp( word, "Pcflags" ) )
            {
               infoflags = fread_flagstring( gfp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  value = get_flag( flag, pc_flags, PCFLAG_MAX );
                  if( value >= 0 && value < PCFLAG_MAX )
                     xSET_BIT( iflags, value );
               }
            }
            fclose( gfp );
            gfp = NULL;
            if( xIS_SET( iflags, PCFLAG_RETIRED ) )
            {
               ilevel = -1;
               retirees++;
            }
            if( xIS_SET( iflags, PCFLAG_GUEST ) )
            {
               ilevel = -2;
               guests++;
            }
            if( ilevel == PERM_IMP )
               imps++;
            if( ilevel == PERM_HEAD )
               heads++;
            if( ilevel == PERM_LEADER )
               leaders++;
            if( ilevel == PERM_BUILDER )
               builders++;
            if( ilevel == PERM_IMM )
               imms++;
            if( ilevel == PERM_ALL )
               servants++;
            add_to_wizlist( dentry->d_name, ilevel, sex );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   remove_file( WIZLIST_FILE );
   snprintf( buf, sizeof( buf ), " &[lblue]Masters of the &[green]%s&[lblue]!", sysdata.mud_name );
   towizfile( buf );
   buf[0] = '\0';
   ilevel = PERM_MAX;
   for( wiz = first_wiz; wiz; wiz = wiz->next )
   {
      if( wiz->level < ilevel )
      {
         if( buf[0] )
         {
            towizfile( buf );
            buf[0] = '\0';
         }
         towizfile( "&[lblue]" );
         ilevel = wiz->level;
         switch( ilevel )
         {
            case PERM_IMP:
               if( imps != 1 )
                  towizfile( " Implementors" );
               else
                  towizfile( " Implementor" );
               break;

            case PERM_HEAD:
               if( heads != 1 )
                  towizfile( " Head Immortals" );
               else
                  towizfile( " Head Immortal" );
               break;

            case PERM_LEADER:
               if( leaders != 1 )
                  towizfile( " Leaders" );
               else
                  towizfile( " Leader" );
               break;

            case PERM_BUILDER:
               if( builders != 1 )
                  towizfile( " Builders" );
               else
                  towizfile( " Builder" );
               break;

            case PERM_IMM:
               if( imms != 1 )
                  towizfile( " Immortals" );
               else
                  towizfile( " Immortal" );
               break;

            case -1:
               if( retirees != 1 )
                  towizfile( " Retirees" );
               else
                  towizfile( " Retired" );
               break;

            case -2:
               if( guests != 1 )
                  towizfile( " Guests" );
               else
                  towizfile( " Guest" );
               break;

            case PERM_ALL:
            default:
               if( servants != 1 )
                  towizfile( " Servants" );
               else
                  towizfile( " Servant" );
               break;
         }
      }
      if( color_strlen( buf ) + strlen( wiz->name ) > 76 )
      {
         towizfile( buf );
         buf[0] = '\0';
      }
      mudstrlcat( buf, " ", sizeof( buf ) );
      if( wiz->sex == SEX_NEUTRAL )
         mudstrlcat( buf, "&[people]", sizeof( buf ) );
      if( wiz->sex == SEX_FEMALE )
         mudstrlcat( buf, "&[female]", sizeof( buf ) );
      if( wiz->sex == SEX_MALE )
         mudstrlcat( buf, "&[male]", sizeof( buf ) );
      mudstrlcat( buf, wiz->name, sizeof( buf ) );
      if( color_strlen( buf ) > 70 )
      {
         towizfile( buf );
         buf[0] = '\0';
      }
   }

   if( buf[0] )
      towizfile( buf );

   for( wiz = first_wiz; wiz; wiz = wiznext )
   {
      wiznext = wiz->next;
      STRFREE( wiz->name );
      DISPOSE( wiz );
   }
   first_wiz = last_wiz = NULL;
}

CMDF( do_makewizlist )
{
   make_wizlist( );
   send_to_char( "Done.\r\n", ch );
}

/* mud prog functions */

/* This routine reads in scripts of MUDprograms from a file */
int mprog_name_to_type( char *name )
{
   if( !str_cmp( name, "in_file_prog" ) )
      return IN_FILE_PROG;
   if( !str_cmp( name, "act_prog" ) )
      return ACT_PROG;
   if( !str_cmp( name, "speech_prog" ) )
      return SPEECH_PROG;
   if( !str_cmp( name, "rand_prog" ) )
      return RAND_PROG;
   if( !str_cmp( name, "fight_prog" ) )
      return FIGHT_PROG;
   if( !str_cmp( name, "hitprcnt_prog" ) )
      return HITPRCNT_PROG;
   if( !str_cmp( name, "death_prog" ) )
      return DEATH_PROG;
   if( !str_cmp( name, "entry_prog" ) )
      return ENTRY_PROG;
   if( !str_cmp( name, "greet_prog" ) )
      return GREET_PROG;
   if( !str_cmp( name, "all_greet_prog" ) )
      return ALL_GREET_PROG;
   if( !str_cmp( name, "give_prog" ) )
      return GIVE_PROG;
   if( !str_cmp( name, "bribe_prog" ) )
      return BRIBE_PROG;
   if( !str_cmp( name, "time_prog" ) )
      return TIME_PROG;
   if( !str_cmp( name, "hour_prog" ) )
      return HOUR_PROG;
   if( !str_cmp( name, "wear_prog" ) )
      return WEAR_PROG;
   if( !str_cmp( name, "remove_prog" ) )
      return REMOVE_PROG;
   if( !str_cmp( name, "sac_prog" ) )
      return SAC_PROG;
   if( !str_cmp( name, "look_prog" ) )
      return LOOK_PROG;
   if( !str_cmp( name, "exa_prog" ) )
      return EXA_PROG;
   if( !str_cmp( name, "zap_prog" ) )
      return ZAP_PROG;
   if( !str_cmp( name, "open_prog" ) )
      return OPEN_PROG;
   if( !str_cmp( name, "close_prog" ) )
      return CLOSE_PROG;
   if( !str_cmp( name, "get_prog" ) )
      return GET_PROG;
   if( !str_cmp( name, "drop_prog" ) )
      return DROP_PROG;
   if( !str_cmp( name, "damage_prog" ) )
      return DAMAGE_PROG;
   if( !str_cmp( name, "scrap_prog" ) )
      return SCRAP_PROG;
   if( !str_cmp( name, "put_prog" ) )
      return PUT_PROG;
   if( !str_cmp( name, "repair_prog" ) )
      return REPAIR_PROG;
   if( !str_cmp( name, "greet_prog" ) )
      return GREET_PROG;
   if( !str_cmp( name, "pull_prog" ) )
      return PULL_PROG;
   if( !str_cmp( name, "push_prog" ) )
      return PUSH_PROG;
   if( !str_cmp( name, "sleep_prog" ) )
      return SLEEP_PROG;
   if( !str_cmp( name, "rest_prog" ) )
      return REST_PROG;
   if( !str_cmp( name, "leave_prog" ) )
      return LEAVE_PROG;
   if( !str_cmp( name, "script_prog" ) )
      return SCRIPT_PROG;
   if( !str_cmp( name, "use_prog" ) )
      return USE_PROG;
   return ( ERROR_PROG );
}

void mobprog_file_read( MOB_INDEX_DATA *mob, char *f )
{
   MPROG_DATA *mprg = NULL;
   FILE *progfile;
   char MUDProgfile[MIL];
   char letter;

   snprintf( MUDProgfile, sizeof( MUDProgfile ), "%s%s", PROG_DIR, f );

   if( !( progfile = fopen( MUDProgfile, "r" ) ) )
   {
      bug( "%s: couldn't open mudprog file", __FUNCTION__ );
      return;
   }

   for( ;; )
   {
      letter = fread_letter( progfile );

      if( letter == '|' )
         break;

      if( letter != '>' )
      {
         bug( "%s: MUDPROG char", __FUNCTION__ );
         break;
      }

      CREATE( mprg, MPROG_DATA, 1 );
      mprg->type = mprog_name_to_type( fread_word( progfile ) );
      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: mudprog file type error", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         case IN_FILE_PROG:
            bug( "%s: Nested file programs aren't allowed.", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         default:
            mprg->arglist = fread_string( progfile );
            mprg->comlist = fread_string( progfile );
            mprg->fileprog = true;
            xSET_BIT( mob->progtypes, mprg->type );
            mprg->next = mob->mudprogs;
            mob->mudprogs = mprg;
            break;
      }
   }
   fclose( progfile );
   progfile = NULL;
}

/* This procedure is responsible for reading any in_file MUDprograms. */
void mprog_read_programs( FILE *fp, MOB_INDEX_DATA *mob )
{
   MPROG_DATA *mprg;
   char *word;
   char letter;

   for( ;; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, mob->vnum );
         exit( 1 );
      }

      CREATE( mprg, MPROG_DATA, 1 );
      mprg->next = mob->mudprogs;
      mob->mudprogs = mprg;

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, mob->vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = false;
            mobprog_file_read( mob, mprg->arglist );
            break;

         default:
            xSET_BIT( mob->progtypes, mprg->type );
            mprg->fileprog = false;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
}

/*************************************************************/
/* obj prog functions */
/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

/* This routine reads in scripts of OBJprograms from a file */
void objprog_file_read( OBJ_INDEX_DATA *obj, char *f )
{
   MPROG_DATA *mprg = NULL;
   FILE *progfile;
   char MUDProgfile[MIL];
   char letter;

   snprintf( MUDProgfile, sizeof( MUDProgfile ), "%s%s", PROG_DIR, f );
   if( !( progfile = fopen( MUDProgfile, "r" ) ) )
   {
      bug( "%s: couldn't open mudprog file", __FUNCTION__ );
      return;
   }

   for( ;; )
   {
      letter = fread_letter( progfile );

      if( letter == '|' )
         break;

      if( letter != '>' )
      {
         bug( "%s: MUDPROG char", __FUNCTION__ );
         break;
      }

      CREATE( mprg, MPROG_DATA, 1 );
      mprg->type = mprog_name_to_type( fread_word( progfile ) );
      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: mudprog file type error", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         case IN_FILE_PROG:
            bug( "%s: Nested file programs aren't allowed.", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         default:
            mprg->arglist = fread_string( progfile );
            mprg->comlist = fread_string( progfile );
            mprg->fileprog = true;
            xSET_BIT( obj->progtypes, mprg->type );
            mprg->next = obj->mudprogs;
            obj->mudprogs = mprg;
            break;
      }
   }
   fclose( progfile );
   progfile = NULL;
}

/* This procedure is responsible for reading any in_file OBJprograms. */
void oprog_read_programs( FILE *fp, OBJ_INDEX_DATA *obj )
{
   MPROG_DATA *mprg;
   char *word;
   char letter;

   for( ;; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, obj->vnum );
         exit( 1 );
      }
      CREATE( mprg, MPROG_DATA, 1 );
      mprg->next = obj->mudprogs;
      obj->mudprogs = mprg;

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, obj->vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = false;
            objprog_file_read( obj, mprg->arglist );
            break;

         default:
            xSET_BIT( obj->progtypes, mprg->type );
            mprg->fileprog = false;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
}

/*************************************************************/
/* room prog functions */
/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

/* This routine reads in scripts of OBJprograms from a file */
void roomprog_file_read( ROOM_INDEX_DATA *room, char *f )
{
   MPROG_DATA *mprg = NULL;
   FILE *progfile;
   char MUDProgfile[MIL];
   char letter;

   snprintf( MUDProgfile, sizeof( MUDProgfile ), "%s%s", PROG_DIR, f );

   if( !( progfile = fopen( MUDProgfile, "r" ) ) )
   {
      bug( "%s: couldn't open mudprog file", __FUNCTION__ );
      return;
   }

   for( ;; )
   {
      letter = fread_letter( progfile );

      if( letter == '|' )
         break;

      if( letter != '>' )
      {
         bug( "%s: MUDPROG char", __FUNCTION__ );
         break;
      }

      CREATE( mprg, MPROG_DATA, 1 );
      mprg->type = mprog_name_to_type( fread_word( progfile ) );
      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: mudprog file type error", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         case IN_FILE_PROG:
            bug( "%s: Nested file programs aren't allowed.", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         default:
            mprg->arglist = fread_string( progfile );
            mprg->comlist = fread_string( progfile );
            mprg->fileprog = true;
            xSET_BIT( room->progtypes, mprg->type );
            mprg->next = room->mudprogs;
            room->mudprogs = mprg;
            break;
      }
   }
   fclose( progfile );
   progfile = NULL;
}

/* This procedure is responsible for reading any in_file ROOMprograms. */
void rprog_read_programs( FILE *fp, ROOM_INDEX_DATA *room )
{
   MPROG_DATA *mprg;
   char *word;
   char letter;

   for( ;; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, room->vnum );
         exit( 1 );
      }
      CREATE( mprg, MPROG_DATA, 1 );
      mprg->next = room->mudprogs;
      room->mudprogs = mprg;

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, room->vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = false;
            roomprog_file_read( room, mprg->arglist );
            break;

         default:
            xSET_BIT( room->progtypes, mprg->type );
            mprg->fileprog = false;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
}

/*************************************************************/
/*
 * Function to delete a room index.
 * Called from do_rdelete in build.c - Narn, May/96
 * Don't ask me why they return bool.. :).. oh well.. -- Alty
 * Don't ask me either, so I changed it to void. - Samson
 */
void delete_room( ROOM_INDEX_DATA *room )
{
   ROOM_INDEX_DATA *prev, *limbo = get_room_index( sysdata.room_limbo );
   OBJ_DATA *o;
   CHAR_DATA *ch;
   EXTRA_DESCR_DATA *ed;
   EXIT_DATA *ex;
   MPROG_ACT_LIST *mpact;
   MPROG_DATA *mp;
   int hash;

   UNLINK( room, room->area->first_room, room->area->last_room, next_aroom, prev_aroom );

   while( ( ch = room->first_person ) )
   {
      if( !is_npc( ch ) )
      {
         char_from_room( ch );
         char_to_room( ch, limbo );
      }
      else
         extract_char( ch, true );
   }

   for( ch = first_char; ch; ch = ch->next )
   {
      if( ch->was_in_room == room )
         ch->was_in_room = ch->in_room;
      if( ch->substate == SUB_ROOM_DESC && ch->dest_buf == room )
      {
         stop_editing( ch );
         ch->substate = SUB_NONE;
         ch->dest_buf = NULL;
         send_to_char( "The room is no more.\r\n", ch );
      }
      else if( ch->substate == SUB_ROOM_EXTRA && ch->dest_buf )
      {
         for( ed = room->first_extradesc; ed; ed = ed->next )
         {
            if( ed == ch->dest_buf )
            {
               stop_editing( ch );
               ch->substate = SUB_NONE;
               ch->dest_buf = NULL;
               send_to_char( "The room is no more.\r\n", ch );
               break;
            }
         }
      }
   }

   while( ( o = room->first_content ) )
      extract_obj( o );

   wipe_resets( room );

   while( ( ed = room->first_extradesc ) )
   {
      room->first_extradesc = ed->next;
      STRFREE( ed->keyword );
      STRFREE( ed->description );
      DISPOSE( ed );
      --top_ed;
   }
   while( ( ex = room->first_exit ) )
      extract_exit( room, ex );

   while( ( mpact = room->mpact ) )
   {
      room->mpact = mpact->next;
      STRFREE( mpact->buf );
      DISPOSE( mpact );
   }
   while( ( mp = room->mudprogs ) )
   {
      room->mudprogs = mp->next;
      STRFREE( mp->arglist );
      STRFREE( mp->comlist );
      DISPOSE( mp );
   }
   STRFREE( room->name );
   STRFREE( room->description );

   hash = room->vnum % MKH;
   if( room == room_index_hash[hash] )
      room_index_hash[hash] = room->next;
   else
   {
      for( prev = room_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == room )
            break;
      if( prev )
         prev->next = room->next;
      else
         bug( "%s: room %d not in hash bucket %d.", __FUNCTION__, room->vnum, hash );
   }
   DISPOSE( room );
   --top_room;
}

/* See comment on delete_room. */
void delete_obj( OBJ_INDEX_DATA *obj )
{
   OBJ_INDEX_DATA *prev;
   OBJ_DATA *o, *o_next;
   CHAR_DATA *ch;
   EXTRA_DESCR_DATA *ed;
   AFFECT_DATA *af;
   MPROG_DATA *mp;
   int hash;

   UNLINK( obj, obj->area->first_obj, obj->area->last_obj, next_aobj, prev_aobj );

   /* Remove references to object index */
   for( o = obj->first_copy; o; o = o_next )
   {
      o_next = o->next_index;
      if( o->pIndexData == obj )
         extract_obj( o );
   }

   for( ch = first_char; ch; ch = ch->next )
   {
      if( ch->substate == SUB_OBJ_EXTRA && ch->dest_buf )
      {
         for( ed = obj->first_extradesc; ed; ed = ed->next )
         {
            if( ed == ch->dest_buf )
            {
               stop_editing( ch );
               ch->substate = SUB_NONE;
               send_to_char( "You suddenly forget which object you were editing!\r\n", ch );
               break;
            }
         }
      }
      else if( ch->substate == SUB_MPROG_EDIT && ch->dest_buf )
      {
         for( mp = obj->mudprogs; mp; mp = mp->next )
         {
            if( mp == ch->dest_buf )
            {
               stop_editing( ch );
               ch->dest_buf = NULL;
               ch->substate = SUB_NONE;
               send_to_char( "You suddenly forget which object you were working on.\r\n", ch );
               break;
            }
         }
      }
   }

   while( ( ed = obj->first_extradesc ) )
   {
      obj->first_extradesc = ed->next;
      STRFREE( ed->keyword );
      STRFREE( ed->description );
      DISPOSE( ed );
      --top_ed;
   }
   while( ( af = obj->first_affect ) )
   {
      obj->first_affect = af->next;
      DISPOSE( af );
      --top_affect;
   }
   while( ( mp = obj->mudprogs ) )
   {
      obj->mudprogs = mp->next;
      STRFREE( mp->arglist );
      STRFREE( mp->comlist );
      DISPOSE( mp );
   }
   STRFREE( obj->name );
   STRFREE( obj->short_descr );
   STRFREE( obj->description );
   STRFREE( obj->action_desc );
   STRFREE( obj->desc );

   free_track_resets( obj, NULL );

   hash = obj->vnum % MKH;
   if( obj == obj_index_hash[hash] )
      obj_index_hash[hash] = obj->next;
   else
   {
      for( prev = obj_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == obj )
            break;
      if( prev )
         prev->next = obj->next;
      else
         bug( "%s: object %d not in hash bucket %d.", __FUNCTION__, obj->vnum, hash );
   }

   DISPOSE( obj );
   --top_obj_index;
}

/* See comment on delete_room. */
void delete_mob( MOB_INDEX_DATA *mob )
{
   MOB_INDEX_DATA *prev;
   CHAR_DATA *ch, *ch_next;
   MPROG_DATA *mp;
   int hash;

   UNLINK( mob, mob->area->first_mob, mob->area->last_mob, next_amob, prev_amob );

   for( ch = mob->first_copy; ch; ch = ch_next )
   {
      ch_next = ch->next_index;

      if( ch->pIndexData == mob )
         extract_char( ch, true );
      else if( ch->substate == SUB_MPROG_EDIT && ch->dest_buf )
      {
         for( mp = mob->mudprogs; mp; mp = mp->next )
         {
            if( mp == ch->dest_buf )
            {
               send_to_char( "Your victim has departed.\r\n", ch );
               stop_editing( ch );
               ch->dest_buf = NULL;
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
   }

   while( ( mp = mob->mudprogs ) )
   {
      mob->mudprogs = mp->next;
      STRFREE( mp->arglist );
      STRFREE( mp->comlist );
      DISPOSE( mp );
   }

   if( mob->pShop )
   {
      UNLINK( mob->pShop, first_shop, last_shop, next, prev );
      DISPOSE( mob->pShop );
      --top_shop;
   }

   if( mob->rShop )
   {
      UNLINK( mob->rShop, first_repair, last_repair, next, prev );
      DISPOSE( mob->rShop );
      --top_repair;
   }

   STRFREE( mob->name );
   STRFREE( mob->short_descr );
   STRFREE( mob->long_descr );
   STRFREE( mob->description );
   STRFREE( mob->spec_funname );

   free_track_resets( NULL, mob );

   hash = mob->vnum % MKH;
   if( mob == mob_index_hash[hash] )
      mob_index_hash[hash] = mob->next;
   else
   {
      for( prev = mob_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == mob )
            break;
      if( prev )
         prev->next = mob->next;
      else
         bug( "%s: mobile %d not in hash bucket %d.", __FUNCTION__, mob->vnum, hash );
   }

   DISPOSE( mob );
   --top_mob_index;
}

ROOM_INDEX_DATA *new_room( int vnum )
{
   ROOM_INDEX_DATA *nroom;

   CREATE( nroom, ROOM_INDEX_DATA, 1 );
   if( !nroom )
   {
      bug( "%s: nroom is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   nroom->first_person = nroom->last_person = NULL;
   nroom->first_content = nroom->last_content = NULL;
   nroom->first_reset = nroom->last_reset = NULL;
   nroom->first_extradesc = nroom->last_extradesc = NULL;
   nroom->first_exit = nroom->last_exit = NULL;
   nroom->name = NULL;
   nroom->description = NULL;
   xCLEAR_BITS( nroom->room_flags );
   nroom->area = NULL;
   nroom->vnum = vnum;
   nroom->sector_type = SECT_INSIDE;
   nroom->light = 0;
   nroom->tele_delay = 0;
   nroom->tele_vnum = 0;
   nroom->tunnel = 0;
   nroom->charcount = 0;
   nroom->objcount = 0;
   return nroom;
}

/* Creat a new room (for online building) - Thoric */
ROOM_INDEX_DATA *make_room( int vnum, AREA_DATA *area )
{
   ROOM_INDEX_DATA *pRoomIndex;
   int iHash;

   if( !( pRoomIndex = new_room( vnum ) ) )
   {
      bug( "%s: pRoomIndex is NULL after new_room.", __FUNCTION__ );
      return NULL;
   }
   pRoomIndex->area = area;
   pRoomIndex->name = STRALLOC( "Floating in a void" );
   LINK( pRoomIndex, area->first_room, area->last_room, next_aroom, prev_aroom );

   iHash = vnum % MKH;
   pRoomIndex->next = room_index_hash[iHash];
   room_index_hash[iHash] = pRoomIndex;
   ++top_room;

   return pRoomIndex;
}

OBJ_INDEX_DATA *new_object( int vnum )
{
   OBJ_INDEX_DATA *nobj;
   int x;

   CREATE( nobj, OBJ_INDEX_DATA, 1 );
   if( !nobj )
   {
      bug( "%s: nobj is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   nobj->vnum = vnum;
   nobj->name = NULL;
   nobj->short_descr = NULL;
   nobj->description = NULL;
   nobj->action_desc = NULL;
   nobj->first_affect = nobj->last_affect = NULL;
   nobj->first_extradesc = nobj->last_extradesc = NULL;
   nobj->first_copy = nobj->last_copy = NULL;
   xCLEAR_BITS( nobj->extra_flags );
   xCLEAR_BITS( nobj->wear_flags );
   xCLEAR_BITS( nobj->class_restrict );
   xCLEAR_BITS( nobj->race_restrict );
   nobj->item_type = ITEM_TRASH;
   nobj->value[0] = -1;
   nobj->value[1] = -1;
   nobj->value[2] = -1;
   nobj->value[3] = -1;
   nobj->value[4] = -1;
   nobj->value[5] = -1;
   nobj->weight = 1;
   nobj->cost = 0;
   nobj->count = 0;
   nobj->layers = 0;
   for( x = 0; x < STAT_MAX; x++ )
      nobj->stat_reqs[x] = 0;
   return nobj;
}

/*
 * Create a new INDEX object (for online building) - Thoric
 * Option to clone an existing index object.
 */
OBJ_INDEX_DATA *make_object( int vnum, int cvnum, char *name )
{
   OBJ_INDEX_DATA *pObjIndex = NULL, *cObjIndex = NULL;
   char buf[MSL];
   int iHash;

   if( cvnum > 0 )
      cObjIndex = get_obj_index( cvnum );
   if( !( pObjIndex = new_object( vnum ) ) )
   {
      bug( "%s: failed to CREATE pObjIndex.", __FUNCTION__ );
      return NULL;
   }
   pObjIndex->name = STRALLOC( name );
   if( !cObjIndex )
   {
      snprintf( buf, sizeof( buf ), "A newly created %s", name );
      pObjIndex->short_descr = STRALLOC( buf );
      snprintf( buf, sizeof( buf ), "Some god dropped a newly created %s here.", name );
      pObjIndex->description = STRALLOC( buf );
      pObjIndex->short_descr[0] = LOWER( pObjIndex->short_descr[0] );
      pObjIndex->description[0] = UPPER( pObjIndex->description[0] );
      xSET_BIT( pObjIndex->extra_flags, ITEM_PROTOTYPE );
   }
   else
   {
      EXTRA_DESCR_DATA *ed, *ced;
      AFFECT_DATA *paf, *cpaf;

      pObjIndex->short_descr = QUICKLINK( cObjIndex->short_descr );
      pObjIndex->description = QUICKLINK( cObjIndex->description );
      pObjIndex->action_desc = QUICKLINK( cObjIndex->action_desc );
      pObjIndex->item_type = cObjIndex->item_type;
      pObjIndex->extra_flags = cObjIndex->extra_flags;
      xSET_BIT( pObjIndex->extra_flags, ITEM_PROTOTYPE );
      pObjIndex->wear_flags = cObjIndex->wear_flags;
      pObjIndex->value[0] = cObjIndex->value[0];
      pObjIndex->value[1] = cObjIndex->value[1];
      pObjIndex->value[2] = cObjIndex->value[2];
      pObjIndex->value[3] = cObjIndex->value[3];
      pObjIndex->value[4] = cObjIndex->value[4];
      pObjIndex->value[5] = cObjIndex->value[5];
      pObjIndex->weight = cObjIndex->weight;
      pObjIndex->cost = cObjIndex->cost;
      for( ced = cObjIndex->first_extradesc; ced; ced = ced->next )
      {
         CREATE( ed, EXTRA_DESCR_DATA, 1 );
         ed->keyword = QUICKLINK( ced->keyword );
         ed->description = QUICKLINK( ced->description );
         LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc, next, prev );
         top_ed++;
      }
      for( cpaf = cObjIndex->first_affect; cpaf; cpaf = cpaf->next )
      {
         CREATE( paf, AFFECT_DATA, 1 );
         paf->type = cpaf->type;
         paf->duration = cpaf->duration;
         paf->location = cpaf->location;
         paf->modifier = cpaf->modifier;
         paf->bitvector = cpaf->bitvector;
         LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
         top_affect++;
      }
   }
   iHash = vnum % MKH;
   pObjIndex->next = obj_index_hash[iHash];
   obj_index_hash[iHash] = pObjIndex;
   top_obj_index++;

   return pObjIndex;
}

/*
 * Create a new INDEX mobile (for online building) - Thoric
 * Option to clone an existing index mobile.
 */
MOB_INDEX_DATA *make_mobile( int vnum, int cvnum, char *name )
{
   MOB_INDEX_DATA *pMobIndex = NULL, *cMobIndex = NULL;
   char buf[MSL];
   int iHash, stat;

   if( cvnum > 0 )
      cMobIndex = get_mob_index( cvnum );
   CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
   if( !pMobIndex )
   {
      bug( "%s: failed to CREATE pMobIndex.", __FUNCTION__ );
      return NULL;
   }
   pMobIndex->vnum = vnum;
   pMobIndex->count = 0;
   pMobIndex->killed = 0;
   pMobIndex->name = STRALLOC( name );
   pMobIndex->first_copy = pMobIndex->last_copy = NULL;
   if( !cMobIndex )
   {
      snprintf( buf, sizeof( buf ), "A newly created %s", name );
      pMobIndex->short_descr = STRALLOC( buf );
      snprintf( buf, sizeof( buf ), "Some god abandoned a newly created %s here.\r\n", name );
      pMobIndex->long_descr = STRALLOC( buf );
      pMobIndex->description = NULL;
      xCLEAR_BITS( pMobIndex->act );
      xSET_BIT( pMobIndex->act, ACT_IS_NPC );
      xSET_BIT( pMobIndex->act, ACT_PROTOTYPE );
      xCLEAR_BITS( pMobIndex->affected_by );
      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;
      pMobIndex->spec_fun = NULL;
      pMobIndex->mudprogs = NULL;
      xCLEAR_BITS( pMobIndex->progtypes );
      pMobIndex->alignment = 0;
      pMobIndex->level = 1;
      pMobIndex->ac = 0;
      pMobIndex->minhit = 0;
      pMobIndex->maxhit = 0;
      pMobIndex->gold = 0;
      pMobIndex->defposition = POS_STANDING;
      pMobIndex->sex = 0;
      for( stat = 0; stat < STAT_MAX; stat++ )
         pMobIndex->perm_stats[stat] = 13;
      for( stat = 0; stat < RIS_MAX; stat++ )
         pMobIndex->resistant[stat] = 0;
      pMobIndex->numattacks = 0;
      xCLEAR_BITS( pMobIndex->xflags );
      xCLEAR_BITS( pMobIndex->attacks );
      xCLEAR_BITS( pMobIndex->defenses );
   }
   else
   {
      pMobIndex->short_descr = QUICKLINK( cMobIndex->short_descr );
      pMobIndex->long_descr = QUICKLINK( cMobIndex->long_descr );
      pMobIndex->description = QUICKLINK( cMobIndex->description );
      pMobIndex->act = cMobIndex->act;
      xSET_BIT( pMobIndex->act, ACT_PROTOTYPE );
      pMobIndex->affected_by = cMobIndex->affected_by;
      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;
      pMobIndex->spec_fun = cMobIndex->spec_fun;
      pMobIndex->mudprogs = NULL;
      xCLEAR_BITS( pMobIndex->progtypes );
      pMobIndex->alignment = cMobIndex->alignment;
      pMobIndex->level = cMobIndex->level;
      pMobIndex->ac = cMobIndex->ac;
      pMobIndex->minhit = cMobIndex->minhit;
      pMobIndex->maxhit = cMobIndex->maxhit;
      pMobIndex->gold = cMobIndex->gold;
      pMobIndex->defposition = cMobIndex->defposition;
      pMobIndex->sex = cMobIndex->sex;
      for( stat = 0; stat < STAT_MAX; stat++ )
         pMobIndex->perm_stats[stat] = cMobIndex->perm_stats[stat];
      pMobIndex->xflags = cMobIndex->xflags;
      for( stat = 0; stat < RIS_MAX; stat++ )
         pMobIndex->resistant[stat] = cMobIndex->resistant[stat];
      pMobIndex->numattacks = cMobIndex->numattacks;
      pMobIndex->attacks = cMobIndex->attacks;
      pMobIndex->defenses = cMobIndex->defenses;
   }
   iHash = vnum % MKH;
   pMobIndex->next = mob_index_hash[iHash];
   mob_index_hash[iHash] = pMobIndex;
   top_mob_index++;

   return pMobIndex;
}

/*
 * Creates a simple exit with no fields filled but rvnum and optionally to_room and vnum. - Thoric
 * Exits are inserted into the linked list based on vdir.
 */
EXIT_DATA *make_exit( ROOM_INDEX_DATA *pRoomIndex, ROOM_INDEX_DATA *to_room, short door )
{
   EXIT_DATA *pexit, *texit;
   bool broke;

   CREATE( pexit, EXIT_DATA, 1 );
   pexit->vdir = door;
   pexit->rvnum = pRoomIndex->vnum;
   pexit->to_room = to_room;
   xCLEAR_BITS( pexit->exit_info );
   xCLEAR_BITS( pexit->base_info );
   if( to_room )
   {
      pexit->vnum = to_room->vnum;
      texit = get_exit_to( to_room, rev_dir[door], pRoomIndex->vnum );
      if( texit ) /* assign reverse exit pointers */
      {
         texit->rexit = pexit;
         pexit->rexit = texit;
      }
   }
   broke = false;
   for( texit = pRoomIndex->first_exit; texit; texit = texit->next )
   {
      if( door < texit->vdir )
      {
         broke = true;
         break;
      }
   }
   if( !pRoomIndex->first_exit )
      pRoomIndex->first_exit = pexit;
   else
   {
      /* keep exits in incremental order - insert exit into list */
      if( broke && texit )
      {
         if( !texit->prev )
            pRoomIndex->first_exit = pexit;
         else
            texit->prev->next = pexit;
         pexit->prev = texit->prev;
         pexit->next = texit;
         texit->prev = pexit;
         top_exit++;
         return pexit;
      }
      pRoomIndex->last_exit->next = pexit;
   }
   pexit->next = NULL;
   pexit->prev = pRoomIndex->last_exit;
   pRoomIndex->last_exit = pexit;
   top_exit++;
   return pexit;
}

/* If it didn't find a vnum go ahead and set it to the next free vnum to use 1+ */
void assign_area_vnum( AREA_DATA *tarea )
{
   int avnum = 0;

   if( !tarea || tarea->vnum > 0 )
      return;

   while( tarea->vnum <= 0 )
   {
      if( !check_area_vnum_conflicts( ++avnum ) )
         tarea->vnum = avnum;
   }
   log_printf( "%s: %s vnum set to %d.", __FUNCTION__, tarea->filename, tarea->vnum );
}

void load_area_file( AREA_DATA *tarea, char *filename )
{
   char newfilename[MIL];

   if( fBootDb )
      tarea = last_area;

   if( !fBootDb && !tarea )
   {
      bug( "%s: null area!", __FUNCTION__ );
      return;
   }

   if( tarea && xIS_SET( tarea->flags, AFLAG_PROTOTYPE ) )
      snprintf( newfilename, sizeof( newfilename ), "%s%s", BUILD_DIR, filename );
   else
      snprintf( newfilename, sizeof( newfilename ), "%s%s", AREA_DIR, filename );

   if( !( fpArea = fopen( newfilename, "r" ) ) )
   {
      perror( newfilename );
      bug( "%s: error loading file (%s) for reading", __FUNCTION__, newfilename );
      return;
   }

   for( ;; )
   {
      char *word;

      if( fread_letter( fpArea ) != '#' )
      {
         bug( "%s: #not found", __FUNCTION__ );
         exit( 1 );
      }

      word = fread_word( fpArea );

      if( word[0] == '$' )
         break;
      else if( !str_cmp( word, "AREA" ) )
      {
         if( fBootDb )
         {
            load_area( fpArea );
            tarea = last_area;
         }
         else
         {
            STRFREE( tarea->name );
            tarea->name = fread_string( fpArea );
         }
      }
      else if( !str_cmp( word, "AVNUM" ) )
      {
         int avnum = fread_number( fpArea );

         if( check_area_vnum_conflicts( avnum ) )
         {
            bug( "%s: area vnum %d already being used by another area.", __FUNCTION__, avnum );
            if( fBootDb )
               exit( 1 );
         }
         tarea->vnum = avnum;
      }
      else if( !str_cmp( word, "AUTHOR" ) )
         load_author( tarea, fpArea );
      else if( !str_cmp( word, "FLAGS" ) )
         load_flags( tarea, fpArea );
      else if( !str_cmp( word, "RANGES" ) )
         load_ranges( tarea, fpArea );
      else if( !str_cmp( word, "ECONOMY" ) )
      {
         fread_number( fpArea );
         fread_number( fpArea );
      }
      else if( !str_cmp( word, "RESETMSG" ) )
         load_resetmsg( tarea, fpArea );
      else if( !str_cmp( word, "RESETFREQ" ) )
      {
         tarea->reset_frequency = fread_number( fpArea );
         tarea->age = tarea->reset_frequency;
      }
      else if( !str_cmp( word, "MOBILES" ) )
      {
         assign_area_vnum( tarea );
         load_mobiles( tarea, fpArea );
      }
      else if( !str_cmp( word, "OBJECTS" ) )
      {
         assign_area_vnum( tarea );
         load_objects( tarea, fpArea );
      }
      else if( !str_cmp( word, "ROOMS" ) )
      {
         assign_area_vnum( tarea );
         load_rooms( tarea, fpArea );
      }
      else if( !str_cmp( word, "CLIMATE" ) )
         load_climate( tarea, fpArea );
      else if( !str_cmp( word, "NEIGHBOR" ) )
         load_neighbor( tarea, fpArea );
      else if( !str_cmp( word, "VERSION" ) )
         load_version( tarea, fpArea );
      else if( !str_cmp( word, "SPELLLIMIT" ) )
         fread_number( fpArea );
      else
      {
         bug( "%s: bad section name: %s", __FUNCTION__, word );
         if( fBootDb )
            exit( 1 );
         else
         {
            fclose( fpArea );
            fpArea = NULL;
            return;
         }
      }
      if( unfoldbadload )
      {
         fclose( fpArea );
         fpArea = NULL;
         bug( "%s: There was a problem while loading the area and it has been stopped here to minimize problems.", __FUNCTION__ );
         bug( "%s: You need to restart the mud soon to get rid of everything that was loaded during the loading of the area.", __FUNCTION__ );
         return;
      }
   }
   fclose( fpArea );
   fpArea = NULL;
   if( tarea )
   {
      if( fBootDb )
      {
         sort_area_by_name( tarea ); /* 4/27/97 */
         sort_area( tarea, false );
      }
      log_printf( "%s: %-14s: Vnums: %5d - %-5d", __FUNCTION__, tarea->filename, tarea->low_vnum, tarea->hi_vnum );
      SET_BIT( tarea->status, AREA_LOADED );

      /* If it didn't find a vnum go ahead and set it to the next free vnum to use 1+ */
      if( tarea->vnum <= 0 )
      {
         int avnum = 0;

         while( tarea->vnum <= 0 )
         {
            if( !check_area_vnum_conflicts( ++avnum ) )
               tarea->vnum = avnum;
         }
         log_printf( "%s: %s vnum set to %d.", __FUNCTION__, tarea->filename, tarea->vnum );
      }
   }
   else
      log_printf( "%s: (%s)", __FUNCTION__, filename );
}

/*
 * Build list of in_progress areas.  Do not load areas.
 * define AREA_READ if you want it to build area names rather than reading
 * them out of the area files. -- Altrag
 */
void load_buildlist( void )
{
   DIR *dp;
   struct dirent *dentry;
   AREA_DATA *pArea;
   FILE *fp;
   char temp, buf[MIL], line[81], word[81], *fgetsed;
   int low, hi, mlow, mhi, olow, ohi, rlow, rhi;
   bool badfile = false;

   dp = opendir( GOD_DIR );
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         snprintf( buf, sizeof( buf ), "%s%s", GOD_DIR, dentry->d_name );
         if( !( fp = fopen( buf, "r" ) ) )
         {
            bug( "%s: invalid file %s", __FUNCTION__, buf );
            perror( buf );
            dentry = readdir( dp );
            continue;
         }
         log_string( buf );
         badfile = false;
         rlow = rhi = olow = ohi = mlow = mhi = 0;
         while( !feof( fp ) && !ferror( fp ) )
         {
            low = 0;
            hi = 0;
            word[0] = 0;
            line[0] = 0;
            if( ( temp = fgetc( fp ) ) != EOF )
               ungetc( temp, fp );
            else
               break;

            fgetsed = fgets( line, 80, fp );
            sscanf( line, "%s %d %d", word, &low, &hi );
            if( !strcmp( word, "VnumRange" ) )
               rlow = low, rhi = hi;
         }
         fclose( fp );
         if( rlow && rhi && !badfile )
         {
            snprintf( buf, sizeof( buf ), "%s%s.are", BUILD_DIR, dentry->d_name );
            if( !( fp = fopen( buf, "r" ) ) )
            {
               bug( "%s: can't open building area file [%s] for read", __FUNCTION__, buf );
               perror( buf );
               dentry = readdir( dp );
               continue;
            }

            mudstrlcpy( strArea, buf, sizeof( strArea ) );

#if !defined(READ_AREA) /* Dont always want to read stuff.. dunno.. shrug */
            mudstrlcpy( word, fread_word( fp ), sizeof( word ) );
            if( word[0] != '#' || strcmp( &word[1], "AREA" ) )
            {
               snprintf( buf, sizeof( buf ), "%s: %s.are: no #AREA found.", __FUNCTION__, dentry->d_name );
               fclose( fp );
               dentry = readdir( dp );
               continue;
            }
#endif
            CREATE( pArea, AREA_DATA, 1 );
            snprintf( buf, sizeof( buf ), "%s.are", dentry->d_name );
            pArea->author = STRALLOC( dentry->d_name );
            pArea->filename = STRALLOC( buf );
#if !defined(READ_AREA)
            pArea->name = fread_string( fp );
#else
            snprintf( buf, sizeof( buf ), "{PROTO} %s's area in progress", dentry->d_name );
            pArea->name = STRALLOC( buf );
#endif
            fclose( fp );
            pArea->low_vnum = rlow;
            pArea->hi_vnum = rhi;
            pArea->low_soft_range = -1;
            pArea->hi_soft_range = -1;
            pArea->low_hard_range = -1;
            pArea->hi_hard_range = -1;

            CREATE( pArea->weather, WEATHER_DATA, 1 );   /* FB */
            pArea->weather->temp = 0;
            pArea->weather->precip = 0;
            pArea->weather->wind = 0;
            pArea->weather->temp_vector = 0;
            pArea->weather->precip_vector = 0;
            pArea->weather->wind_vector = 0;
            pArea->weather->climate_temp = 2;
            pArea->weather->climate_precip = 2;
            pArea->weather->climate_wind = 2;
            pArea->weather->first_neighbor = NULL;
            pArea->weather->last_neighbor = NULL;
            pArea->weather->echo = NULL;
            pArea->weather->echo_color = AT_GRAY;
            pArea->first_room = pArea->last_room = NULL;
            xSET_BIT( pArea->flags, AFLAG_PROTOTYPE );
            LINK( pArea, first_build, last_build, next, prev );
            sort_area( pArea, true );
            top_area++;
            fBootDb = false;
            load_area_file( pArea, pArea->filename );
            fBootDb = true;
            mudstrlcpy( strArea, "$", sizeof( strArea ) );
            reset_area( pArea );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
}

/* Sort areas by name alphanumercially - 4/27/97, Fireblade */
void sort_area_by_name( AREA_DATA *pArea )
{
   AREA_DATA *temp_area;

   if( !pArea )
   {
      bug( "%s: NULL pArea", __FUNCTION__ );
      return;
   }
   for( temp_area = first_area_name; temp_area; temp_area = temp_area->next_sort_name )
   {
      if( strcmp( pArea->name, temp_area->name ) < 0 )
      {
         INSERT( pArea, temp_area, first_area_name, next_sort_name, prev_sort_name );
         break;
      }
   }
   if( !temp_area )
      LINK( pArea, first_area_name, last_area_name, next_sort_name, prev_sort_name );
}

/* Sort by room vnums - Altrag & Thoric */
void sort_area( AREA_DATA *pArea, bool proto )
{
   AREA_DATA *area = NULL;
   AREA_DATA *first_sort, *last_sort;
   bool found;

   if( !pArea )
   {
      bug( "%s: NULL pArea", __FUNCTION__ );
      return;
   }

   if( proto )
   {
      first_sort = first_bsort;
      last_sort = last_bsort;
   }
   else
   {
      first_sort = first_asort;
      last_sort = last_asort;
   }

   found = false;
   pArea->prev_sort = pArea->next_sort = NULL;

   if( !first_sort )
   {
      first_sort = last_sort = pArea;
      found = true;
   }
   else
   {
      for( area = first_sort; area; area = area->next_sort )
      {
         if( pArea->low_vnum < area->low_vnum )
         {
            if( !area->prev_sort )
               first_sort = pArea;
            else
               area->prev_sort->next_sort = pArea;
            pArea->prev_sort = area->prev_sort;
            pArea->next_sort = area;
            area->prev_sort = pArea;
            found = true;
            break;
         }
      }
   }

   if( !found )
   {
      pArea->prev_sort = last_sort;
      pArea->next_sort = NULL;
      last_sort->next_sort = pArea;
      last_sort = pArea;
   }

   if( proto )
   {
      first_bsort = first_sort;
      last_bsort = last_sort;
   }
   else
   {
      first_asort = first_sort;
      last_asort = last_sort;
   }
}

/*
 * Display vnums currently assigned to areas -Altrag & Thoric
 * Sorted, and flagged if loaded.
 */
void show_vnums( CHAR_DATA *ch, int low, int high, bool proto, bool shownl, const char *loadst, const char *notloadst )
{
   AREA_DATA *pArea, *first_sort;
   int count, loaded;

   count = 0;
   loaded = 0;
   set_pager_color( AT_PLAIN, ch );
   if( proto )
      first_sort = first_bsort;
   else
      first_sort = first_asort;
   for( pArea = first_sort; pArea; pArea = pArea->next_sort )
   {
      if( IS_SET( pArea->status, AREA_DELETED ) )
         continue;
      if( pArea->low_vnum < low )
         continue;
      if( pArea->hi_vnum > high )
         break;
      if( IS_SET( pArea->status, AREA_LOADED ) )
         loaded++;
      else if( !shownl )
         continue;
      pager_printf( ch, "%-15s| Vnums: %5d - %-5d%s\r\n",
         ( pArea->filename ? pArea->filename : "(invalid)" ),
           pArea->low_vnum, pArea->hi_vnum,
           IS_SET( pArea->status, AREA_LOADED ) ? loadst : notloadst );
      count++;
   }
   pager_printf( ch, "Areas listed: %d  Loaded: %d\r\n", count, loaded );
}

/* Shows prototype vnums ranges, and if loaded */
CMDF( do_vnums )
{
   char arg1[MIL], arg2[MIL];
   int low, high;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   low = 1;
   high = MAX_VNUM;
   if( arg1 != NULL && arg1[0] != '\0' )
   {
      low = atoi( arg1 );
      if( arg2 != NULL && arg2[0] != '\0' )
         high = atoi( arg2 );
   }
   show_vnums( ch, low, high, true, true, " *", "" );
}

/* Shows installed areas, sorted.  Mark unloaded areas with an X */
CMDF( do_zones )
{
   char arg1[MIL], arg2[MIL];
   int low, high;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   low = 1;
   high = MAX_VNUM;
   if( arg1 != NULL && arg1[0] != '\0' )
   {
      low = atoi( arg1 );
      if( arg2 != NULL && arg2[0] != '\0' )
         high = atoi( arg2 );
   }
   show_vnums( ch, low, high, false, true, "", " X" );
}

/* Check to make sure range of vnums is free - Scryn 2/27/96 */
CMDF( do_check_vnums )
{
   AREA_DATA *pArea;
   char arg1[MSL];
   int low_range, high_range;
   bool area_conflict;

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Please specify the low end of the range to be searched.\r\n", ch );
      return;
   }
   low_range = atoi( arg1 );

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Please specify the high end of the range to be searched.\r\n", ch );
      return;
   }
   high_range = atoi( arg1 );

   if( low_range < 1 || low_range > MAX_VNUM )
   {
      send_to_char( "Invalid argument for bottom of range.\r\n", ch );
      return;
   }

   if( high_range < 1 || high_range > MAX_VNUM )
   {
      send_to_char( "Invalid argument for top of range.\r\n", ch );
      return;
   }

   if( high_range < low_range )
   {
      send_to_char( "Bottom of range must be below top of range.\r\n", ch );
      return;
   }
   set_char_color( AT_PLAIN, ch );
   for( pArea = first_asort; pArea; pArea = pArea->next_sort )
   {
      area_conflict = false;
      if( IS_SET( pArea->status, AREA_DELETED ) )
         continue;
      if( low_range < pArea->low_vnum && pArea->low_vnum < high_range )
         area_conflict = true;
      if( low_range < pArea->hi_vnum && pArea->hi_vnum < high_range )
         area_conflict = true;
      if( ( low_range >= pArea->low_vnum ) && ( low_range <= pArea->hi_vnum ) )
         area_conflict = true;
      if( ( high_range <= pArea->hi_vnum ) && ( high_range >= pArea->low_vnum ) )
         area_conflict = true;
      if( area_conflict )
      {
         ch_printf( ch, "Conflict: %-15s| ", ( pArea->filename ? pArea->filename : "(invalid)" ) );
         ch_printf( ch, "Vnums: %5d - %-5d\r\n", pArea->low_vnum, pArea->hi_vnum );
      }
   }
   for( pArea = first_bsort; pArea; pArea = pArea->next_sort )
   {
      area_conflict = false;
      if( IS_SET( pArea->status, AREA_DELETED ) )
         continue;
      if( low_range < pArea->low_vnum && pArea->low_vnum < high_range )
         area_conflict = true;
      if( low_range < pArea->hi_vnum && pArea->hi_vnum < high_range )
         area_conflict = true;
      if( ( low_range >= pArea->low_vnum ) && ( low_range <= pArea->hi_vnum ) )
         area_conflict = true;
      if( ( high_range <= pArea->hi_vnum ) && ( high_range >= pArea->low_vnum ) )
         area_conflict = true;
      if( area_conflict )
      {
         ch_printf( ch, "Conflict: %-15s| ", ( pArea->filename ? pArea->filename : "(invalid)" ) );
         ch_printf( ch, "Vnums: %5d - %-5d\r\n", pArea->low_vnum, pArea->hi_vnum );
      }
   }
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
   return;
}

/*
 * Initialize the weather for all the areas
 * Last Modified: July 21, 1997
 * Fireblade
 */
void init_area_weather( void )
{
   AREA_DATA *pArea;
   NEIGHBOR_DATA *neigh, *next_neigh;

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      int cf;

      /* init temp and temp vector */
      cf = pArea->weather->climate_temp - 2;
      pArea->weather->temp = number_range( -weath_unit, weath_unit ) + cf * number_range( 0, weath_unit );
      pArea->weather->temp_vector = cf + number_range( -rand_factor, rand_factor );

      /* init precip and precip vector */
      cf = pArea->weather->climate_precip - 2;
      pArea->weather->precip = number_range( -weath_unit, weath_unit ) + cf * number_range( 0, weath_unit );
      pArea->weather->precip_vector = cf + number_range( -rand_factor, rand_factor );

      /* init wind and wind vector */
      cf = pArea->weather->climate_wind - 2;
      pArea->weather->wind = number_range( -weath_unit, weath_unit ) + cf * number_range( 0, weath_unit );
      pArea->weather->wind_vector = cf + number_range( -rand_factor, rand_factor );

      /* check connections between neighbors */
      for( neigh = pArea->weather->first_neighbor; neigh; neigh = next_neigh )
      {
         AREA_DATA *tarea;
         NEIGHBOR_DATA *tneigh;

         /* get the address if needed */
         if( !neigh->address )
            neigh->address = get_area( neigh->name );

         /* area does not exist */
         if( !neigh->address )
         {
            next_neigh = neigh->next;
            UNLINK( neigh, pArea->weather->first_neighbor, pArea->weather->last_neighbor, next, prev );
            STRFREE( neigh->name );
            DISPOSE( neigh );
            fold_area( pArea, pArea->filename, false );
            continue;
         }

         /* make sure neighbors both point to each other */
         tarea = neigh->address;
         for( tneigh = tarea->weather->first_neighbor; tneigh; tneigh = tneigh->next )
         {
            if( !strcmp( pArea->name, tneigh->name ) )
               break;
         }

         if( !tneigh )
         {
            CREATE( tneigh, NEIGHBOR_DATA, 1 );
            tneigh->name = STRALLOC( pArea->name );
            LINK( tneigh, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
            fold_area( tarea, tarea->filename, false );
         }

         tneigh->address = pArea;
         next_neigh = neigh->next;
      }
   }
}

/*
 * Load weather data from appropriate file in system dir
 * Last Modified: July 24, 1997 - Fireblade
 */
void load_weatherdata( void )
{
   FILE *fp;

   if( ( fp = fopen( WEATHER_FILE, "r" ) ) )
   {
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );

         if( letter != '#' )
         {
            bug( "%s: # not found", __FUNCTION__ );
            return;
         }

         word = fread_word( fp );

         if( !str_cmp( word, "RANDOM" ) )
            rand_factor = fread_number( fp );
         else if( !str_cmp( word, "CLIMATE" ) )
            climate_factor = fread_number( fp );
         else if( !str_cmp( word, "NEIGHBOR" ) )
            neigh_factor = fread_number( fp );
         else if( !str_cmp( word, "UNIT" ) )
            weath_unit = fread_number( fp );
         else if( !str_cmp( word, "MAXVECTOR" ) )
            max_vector = fread_number( fp );
         else if( !str_cmp( word, "END" ) )
         {
            fclose( fp );
            fp = NULL;
            break;
         }
         else
         {
            bug( "%s: unknown field", __FUNCTION__ );
            fclose( fp );
            fp = NULL;
            break;
         }
      }
   }
}

/* Write data for global weather parameters - Fireblade */
void save_weatherdata( void )
{
   FILE *fp;

   if( !( fp = fopen( WEATHER_FILE, "w" ) ) )
   {
      bug( "%s: could not open %s for writing", __FUNCTION__, WEATHER_FILE );
      return;
   }
   fprintf( fp, "#RANDOM    %d\n", rand_factor );
   fprintf( fp, "#CLIMATE   %d\n", climate_factor );
   fprintf( fp, "#NEIGHBOR  %d\n", neigh_factor );
   fprintf( fp, "#UNIT      %d\n", weath_unit );
   fprintf( fp, "#MAXVECTOR %d\n", max_vector );
   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
}

size_t newmudstrlcpy( char *dst, const char *src, size_t siz, const char *filename, int line )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;

   if( !src )
   {
      bug( "%s: %s[%d] passing NULL src string!", __FUNCTION__, filename, line );
      return 0;
   }

   if( !dst )
   {
      bug( "%s: %s[%d] passing NULL dst string!", __FUNCTION__, filename, line );
      return 0;
   }

   /* Copy as many bytes as will fit */
   if( n != 0 && --n != 0 )
   {
      do
      {
         if( ( *d++ = *s++ ) == 0 )
            break;
      }
      while( --n != 0 );
   }

   /* Not enough room in dst, add NULL and traverse rest of src */
   if( n == 0 )
   {
      if( siz != 0 )
         *d = '\0';  /* NUL-terminate dst */
      while( *s++ )
         ;
   }
   return ( s - src - 1 ); /* count does not include NUL */
}

size_t mudstrlcat( char *dst, const char *src, size_t siz )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;
   size_t dlen;

   if( !src )
   {
      bug( "%s: NULL src string passed!", __FUNCTION__ );
      return 0;
   }

   /* Find the end of dst and adjust bytes left but don't go past end */
   while( n-- != 0 && *d != '\0' )
      d++;
   dlen = d - dst;
   n = siz - dlen;

   if( n == 0 )
      return ( dlen + strlen( s ) );
   while( *s && *s != '\0' )
   {
      if( n != 1 )
      {
         *d++ = *s;
         n--;
      }
      s++;
   }
   *d = '\0';
   return ( dlen + ( s - src ) );   /* count does not include NUL */
}
