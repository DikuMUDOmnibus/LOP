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
 * 			Table load/save Module				     *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#if !defined(WIN32)
   #include <dlfcn.h>
#else
   #include <windows.h>
   #define dlsym( handle, name ) ( (void*)GetProcAddress( (HINSTANCE) (handle), (name) ) )
   #define dlerror() GetLastError()
#endif
#include "h/mud.h"

/* global variables */
int top_sn;
int top_herb;
int top_pers;

SKILLTYPE *skill_table[MAX_SKILL];
SKILLTYPE *herb_table[MAX_HERB];
SKILLTYPE *pers_table[MAX_PERS];

LANG_DATA *first_lang, *last_lang;

const char *const skill_tname[] =
{
   "unknown",  "Spell",     "Skill",  "Weapon",  "Tongue",
   "Herb",     "Personal",  "Deleted"
};

SPELL_FUN *spell_function( char *name )
{
   void *funHandle;
#if !defined(WIN32)
   const char *error;
#else
   DWORD error;
#endif

   funHandle = dlsym( sysdata.dlHandle, name );
   if( ( error = dlerror() ) )
   {
      bug( "Error locating %s in symbol table. %s", name, error );
      return spell_notfound;
   }
   return (SPELL_FUN*)funHandle;
}

DO_FUN *skill_function( char *name )
{
   void *funHandle;
#if !defined(WIN32)
   const char *error;
#else
   DWORD error;
#endif

   funHandle = dlsym( sysdata.dlHandle, name );
   if( ( error = dlerror() ) )
   {
      bug( "Error locating %s in symbol table. %s", name, error );
      return skill_notfound;
   }
   return (DO_FUN*)funHandle;
}

/* Function used by qsort to sort skills */
int skill_comp( SKILLTYPE ** sk1, SKILLTYPE ** sk2 )
{
   SKILLTYPE *skill1 = ( *sk1 );
   SKILLTYPE *skill2 = ( *sk2 );

   if( !skill1 && skill2 )
      return 1;
   if( skill1 && !skill2 )
      return -1;
   if( !skill1 && !skill2 )
      return 0;
   if( skill1->type < skill2->type )
      return -1;
   if( skill1->type > skill2->type )
      return 1;
   return strcasecmp( skill1->name, skill2->name );
}

/* Sort the skill table with qsort */
void sort_skill_table( void )
{
   if( top_sn <= 0 )
      return;
   log_string( "Sorting skill table..." );
   qsort( &skill_table[0], top_sn, sizeof( SKILLTYPE * ), ( int ( * )( const void *, const void * ) )skill_comp );
}

/* Remap slot numbers to sn values */
void remap_slot_numbers( void )
{
   SKILLTYPE *skill;
   SMAUG_AFF *aff;
   char tmp[32];
   int sn;

   log_string( "Remapping slots to sns" );
   gsn_first_spell = -1;
   gsn_first_skill = -1;
   gsn_first_weapon = -1;
   gsn_first_tongue = -1;
   gsn_top_sn = top_sn;

   for( sn = 0; sn <= top_sn; sn++ )
   {
      if( ( skill = skill_table[sn] ) )
      {
         if( gsn_first_spell == -1 && skill->type == SKILL_SPELL )
            gsn_first_spell = sn;
         else if( gsn_first_skill == -1 && skill->type == SKILL_SKILL )
            gsn_first_skill = sn;
         else if( gsn_first_weapon == -1 && skill->type == SKILL_WEAPON )
            gsn_first_weapon = sn;
         else if( gsn_first_tongue == -1 && skill->type == SKILL_TONGUE )
            gsn_first_tongue = sn;

         for( aff = skill->first_affect; aff; aff = aff->next )
         {
            if( ( aff->location % REVERSE_APPLY ) == APPLY_WEAPONSPELL || ( aff->location % REVERSE_APPLY ) == APPLY_WEARSPELL
            || ( aff->location % REVERSE_APPLY ) == APPLY_REMOVESPELL  || ( aff->location % REVERSE_APPLY ) == APPLY_STRIPSN )
            {
               snprintf( tmp, sizeof( tmp ), "%d", slot_lookup( atoi( aff->modifier ) ) );
               STRFREE( aff->modifier );
               aff->modifier = STRALLOC( tmp );
            }
         }
      }
   }
}

/* Write skill data to a file */
void fwrite_skill( FILE *fp, SKILLTYPE *skill )
{
   SMAUG_AFF *aff;
   int modifier, stat;

   fprintf( fp, "Name         %s~\n", skill->name );
   fprintf( fp, "Type         %s\n", skill_tname[skill->type] );
   if( skill->tmpspell )
      fprintf( fp, "%s\n", "Spell" );
   if( !skill->magical )
      fprintf( fp, "%s\n", "NonMagical" );
   if( skill->damage )
      fprintf( fp, "SDamage      %s~\n", spell_damage[skill->damage] );
   if( skill->action )
      fprintf( fp, "SAction      %s~\n", spell_action[skill->action] );
   if( skill->Class )
      fprintf( fp, "SClass       %s~\n", spell_class[skill->Class] );
   if( skill->power )
      fprintf( fp, "SPower       %s~\n", spell_power[skill->power] );
   if( skill->save )
      fprintf( fp, "SSave        %s~\n", spell_save_effect[skill->save] );
   if( !xIS_EMPTY( skill->flags ) )
      fprintf( fp, "Flags        %s~\n", ext_flag_string( &skill->flags, spell_flag ) );
   if( skill->target )
      fprintf( fp, "Target       %s~\n", target_type[skill->target] );
   if( skill->minimum_position )
      fprintf( fp, "Minpos       %s~\n", pos_names[skill->minimum_position] );
   if( !xIS_EMPTY( skill->spell_sector ) )
      fprintf( fp, "Ssector      %s~\n", ext_flag_string( &skill->spell_sector, sect_flags ) );
   if( skill->saves )
      fprintf( fp, "Saves        %d\n", skill->saves );
   if( skill->slot != -1 )
      fprintf( fp, "Slot         %d\n", skill->slot );
   if( skill->req_skill != -1 && is_valid_sn( skill->req_skill ) && skill_table[ skill->req_skill ] )
      fprintf( fp, "ReqSkill     '%s'\n", skill_table[ skill->req_skill ]->name );
   if( skill->min_mana )
      fprintf( fp, "Mana         %d\n", skill->min_mana );
   if( skill->beats )
      fprintf( fp, "Rounds       %d\n", skill->beats );
   if( skill->range )
      fprintf( fp, "Range        %d\n", skill->range );

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( skill->stats[stat] <= 0 )
         continue;
      fprintf( fp, "Stat         %d %s~\n", skill->stats[stat], stattypes[stat] );
   }

   if( skill->skill_fun && skill->skill_fun_name )
      fprintf( fp, "Code         %s\n", skill->skill_fun_name );
   else if( skill->spell_fun && skill->spell_fun_name )
      fprintf( fp, "Code         %s\n", skill->spell_fun_name );

   if( skill->noun_damage )
      fprintf( fp, "Dammsg       %s~\n", skill->noun_damage );
   if( skill->msg_off )
      fprintf( fp, "Wearoff      %s~\n", skill->msg_off );
   if( skill->hit_char )
      fprintf( fp, "Hitchar      %s~\n", skill->hit_char );
   if( skill->hit_vict )
      fprintf( fp, "Hitvict      %s~\n", skill->hit_vict );
   if( skill->hit_room )
      fprintf( fp, "Hitroom      %s~\n", skill->hit_room );
   if( skill->hit_dest )
      fprintf( fp, "Hitdest      %s~\n", skill->hit_dest );

   if( skill->miss_char )
      fprintf( fp, "Misschar     %s~\n", skill->miss_char );
   if( skill->miss_vict )
      fprintf( fp, "Missvict     %s~\n", skill->miss_vict );
   if( skill->miss_room )
      fprintf( fp, "Missroom     %s~\n", skill->miss_room );

   if( skill->die_char )
      fprintf( fp, "Diechar      %s~\n", skill->die_char );
   if( skill->die_vict )
      fprintf( fp, "Dievict      %s~\n", skill->die_vict );
   if( skill->die_room )
      fprintf( fp, "Dieroom      %s~\n", skill->die_room );

   if( skill->imm_char )
      fprintf( fp, "Immchar      %s~\n", skill->imm_char );
   if( skill->imm_vict )
      fprintf( fp, "Immvict      %s~\n", skill->imm_vict );
   if( skill->imm_room )
      fprintf( fp, "Immroom      %s~\n", skill->imm_room );

   if( skill->abs_char )
      fprintf( fp, "Abschar      %s~\n", skill->abs_char );
   if( skill->abs_vict )
      fprintf( fp, "Absvict      %s~\n", skill->abs_vict );
   if( skill->abs_room )
      fprintf( fp, "Absroom      %s~\n", skill->abs_room );

   if( skill->dice )
      fprintf( fp, "Dice         %s~\n", skill->dice );
   if( skill->value )
      fprintf( fp, "Value        %d\n", skill->value );
   if( skill->difficulty )
      fprintf( fp, "Difficulty   %d\n", skill->difficulty );
   if( skill->participants )
      fprintf( fp, "Participants %d\n", skill->participants );
   if( skill->components )
      fprintf( fp, "Components   %s~\n", skill->components );
   if( skill->teachers )
      fprintf( fp, "Teachers     %s~\n", skill->teachers );
   if( skill->htext )
      fprintf( fp, "HText        %s~\n", help_fix( skill->htext ) );

   for( aff = skill->first_affect; aff; aff = aff->next )
   {
      fprintf( fp, "Affect       '%s' '%s' %d ",
         aff->duration ? aff->duration : "0", aff->location ? a_types[aff->location % REVERSE_APPLY] : "0",
         aff->location >= REVERSE_APPLY ? 1 : 0 );
      modifier = atoi( aff->modifier );
      if( ( ( aff->location % REVERSE_APPLY ) >= APPLY_WEAPONSPELL
      && ( aff->location % REVERSE_APPLY ) <= APPLY_STRIPSN )
      && is_valid_sn( modifier ) )
         fprintf( fp, "'%d' ", skill_table[modifier]->slot );
      else if( ( aff->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT
      && modifier >= 0 && modifier < AFF_MAX )
         fprintf( fp, "'%s' ", a_flags[modifier] );
      else
         fprintf( fp, "'%s' ", aff->modifier );

      if( ( aff->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT 
      && aff->bitvector >= 0 && aff->bitvector < AFF_MAX )
         fprintf( fp, "'%s'\n", a_flags[aff->bitvector] );
      else if( ( aff->location % REVERSE_APPLY ) == APPLY_STAT
      && aff->bitvector >= 0 && aff->bitvector < STAT_MAX )
         fprintf( fp, "'%s'\n", stattypes[aff->bitvector] );
      else if( ( aff->location % REVERSE_APPLY ) == APPLY_RESISTANT
      && aff->bitvector >= 0 && aff->bitvector < RIS_MAX )
         fprintf( fp, "'%s'\n", ris_flags[aff->bitvector] );
      else
         fprintf( fp, "'%d'\n", aff->bitvector );
   }

   fprintf( fp, "End\n\n" );
}

/* Save the skill table to disk */
void save_skill_table( bool autosave )
{
   FILE *fp;
   int x;

   if( autosave && !sysdata.autosaveskills )
      return;

   if( !( fp = fopen( SKILL_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writting", __FUNCTION__, SKILL_FILE );
      perror( SKILL_FILE );
      return;
   }

   for( x = 0; x < top_sn; x++ )
   {
      if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' || skill_table[x]->type == SKILL_DELETED )
         continue;
      fprintf( fp, "#SKILL\n" );
      fwrite_skill( fp, skill_table[x] );
   }
   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
}

/* Save the herb table to disk */
void save_herb_table( bool autosave )
{
   FILE *fp;
   int x;

   if( autosave && !sysdata.autosaveskills )
      return;

   if( !( fp = fopen( HERB_FILE, "w" ) ) )
   {
      bug( "Can't open %s for writting", HERB_FILE );
      perror( HERB_FILE );
      return;
   }

   for( x = 0; x < top_herb; x++ )
   {
      if( !herb_table[x]->name || herb_table[x]->name[0] == '\0' || herb_table[x]->type == SKILL_DELETED )
         continue;
      fprintf( fp, "#HERB\n" );
      fwrite_skill( fp, herb_table[x] );
   }
   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
}

void save_pers_table( bool autosave )
{
   FILE *fp;
   int x;

   if( autosave && !sysdata.autosaveskills )
      return;

   if( !( fp = fopen( PERSONAL_FILE, "w" ) ) )
   {
      bug( "Can't open %s for writting", PERSONAL_FILE );
      perror( PERSONAL_FILE );
      return;
   }

   for( x = 0; x < top_pers; x++ )
   {
      if( !pers_table[x]->name || pers_table[x]->name[0] == '\0' || pers_table[x]->type == SKILL_DELETED )
         continue;
      fprintf( fp, "#PERSONAL\n" );
      fwrite_skill( fp, pers_table[x] );
   }
   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
}

int get_skill( char *skilltype )
{
   if( !str_cmp( skilltype, "Spell" ) )
      return SKILL_SPELL;
   if( !str_cmp( skilltype, "Skill" ) )
      return SKILL_SKILL;
   if( !str_cmp( skilltype, "Weapon" ) )
      return SKILL_WEAPON;
   if( !str_cmp( skilltype, "Tongue" ) )
      return SKILL_TONGUE;
   if( !str_cmp( skilltype, "Herb" ) )
      return SKILL_HERB;
   if( !str_cmp( skilltype, "Personal" ) )
      return SKILL_PERSONAL;
   if( !str_cmp( skilltype, "Deleted" ) )
      return SKILL_DELETED;
   return SKILL_UNKNOWN;
}

SKILLTYPE *fread_skill( FILE *fp )
{
   SKILLTYPE *skill;
   const char *word;
   char *infoflags, flag[MSL];
   int value;
   bool fMatch;

   if( !( skill = new_skill( ) ) )
   {
      bug( "%s: failed to create a new skill.", __FUNCTION__ );
      return NULL;
   }

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;

      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "Abschar", skill->abs_char, fread_string( fp ) );
            KEY( "Absroom", skill->abs_room, fread_string( fp ) );
            KEY( "Absvict", skill->abs_vict, fread_string( fp ) );
            if( !str_cmp( word, "Affect" ) )
            {
               SMAUG_AFF *aff;
               char modifier[MIL];
               int modchange = 0;
               bool dadd = false;

               CREATE( aff, SMAUG_AFF, 1 );
               aff->duration = STRALLOC( fread_word( fp ) );

               infoflags = fread_word( fp );
               if( str_cmp( infoflags, "0" ) )
               {
                  /* Change blood to mana */
                  if( !str_cmp( infoflags, "Blood" ) )
                     infoflags = (char *)"Mana";
                  else if( !str_cmp( infoflags, "Supplicate" ) || !str_cmp( infoflags, "Immune" ) || !str_cmp( infoflags, "Absorb" ) )
                  {
                     if( !str_cmp( infoflags, "Supplicate" ) )
                        modchange = -1;
                     else if( !str_cmp( infoflags, "Immune" ) )
                        modchange = 100;
                     else if( !str_cmp( infoflags, "Absorb" ) )
                        modchange = 110;
                     infoflags = (char *)"Resistant";
                  }
                  value = get_flag( infoflags, a_types, APPLY_MAX );
                  if( value < 0 || value >= APPLY_MAX )
                  {
                     bug( "%s(%s): Unknown apply %s",
                        __FUNCTION__, skill->name ? skill->name : "Unknown", infoflags );
                     aff->location = 0;
                     dadd = true;
                  }
                  else
                     aff->location = value;
               }
               else
                  aff->location = 0;

               value = fread_number( fp );
               if( value == 1 )
                  aff->location += REVERSE_APPLY;

               aff->bitvector = -1;

               infoflags = fread_word( fp );
               if( ( aff->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
               {
                  value = get_flag( infoflags, a_flags, AFF_MAX );
                  if( value < 0 || value >= AFF_MAX )
                  {
                     bug( "%s(%s): Unknown affect %s", __FUNCTION__, skill->name ? skill->name : "Unknown",
                        infoflags );
                     aff->modifier = STRALLOC( "-1" );
                     dadd = true;
                  }
                  else
                  {
                     snprintf( modifier, sizeof( modifier ), "%d", value );
                     aff->modifier = STRALLOC( modifier );
                  }
               }
               else if( ( aff->location % REVERSE_APPLY ) == APPLY_RESISTANT && !is_number( infoflags ) )
               { /* Lets convert the old ones over */
                  value = get_flag( infoflags, ris_flags, RIS_MAX );
                  if( value < 0 || value >= RIS_MAX )
                  {
                     bug( "%s(%s): Unknown %s %s", __FUNCTION__, skill->name ? skill->name : "Unknown",
                        a_types[aff->location % REVERSE_APPLY], infoflags );
                     aff->modifier = STRALLOC( "-1" );
                     dadd = true;
                  }
                  else
                  {
                     if( modchange == 0 )
                        modchange = 1;
                     snprintf( modifier, sizeof( modifier ), "%d", modchange );
                     aff->modifier = STRALLOC( modifier );
                     aff->bitvector = value;
                  }
               }
               else
                  aff->modifier = STRALLOC( infoflags );

               infoflags = fread_word( fp );
               if( str_cmp( infoflags, "-1" ) )
               {
                  if( ( aff->location % REVERSE_APPLY ) == APPLY_STAT )
                  {
                     value = get_flag( infoflags, stattypes, STAT_MAX );
                     if( value < 0 || value >= STAT_MAX )
                     {
                        bug( "%s(%s): Unknown stat %s",
                           __FUNCTION__, skill->name ? skill->name : "Unknown", infoflags );
                        dadd = true;
                     }
                     else
                        aff->bitvector = value;
                  }
                  else if( ( aff->location % REVERSE_APPLY ) == APPLY_RESISTANT )
                  {
                     value = get_flag( infoflags, ris_flags, RIS_MAX );
                     if( value < 0 || value >= RIS_MAX )
                     {
                        bug( "%s(%s): Unknown ris %s",
                           __FUNCTION__, skill->name ? skill->name : "Unknown", infoflags );
                        dadd = true;
                     }
                     else
                        aff->bitvector = value;
                  }
                  else
                  {
                     value = get_flag( infoflags, a_flags, AFF_MAX );
                     if( value < 0 || value >= AFF_MAX )
                     {
                        bug( "%s(%s): Unknown affect %s",
                           __FUNCTION__, skill->name ? skill->name : "Unknown", infoflags );
                        dadd = true;
                     }
                     else
                        aff->bitvector = value;
                  }
               }

               if( !dadd )
                  LINK( aff, skill->first_affect, skill->last_affect, next, prev );
               else
               {
                  bug( "%s(%s): Something is bad in affect and it won't be added.",
                     __FUNCTION__, skill->name ? skill->name : "Unknown" );
                  STRFREE( aff->duration );
                  STRFREE( aff->modifier );
                  DISPOSE( aff );
               }
               fMatch = true;
               break;
            }
            break;

         case 'C':
            if ( !str_cmp( word, "Code" ) )
            {
               SPELL_FUN *spellfun;
               DO_FUN *dofun;
               char *w = fread_word( fp );

               fMatch = true;
               if( !str_prefix( "do_", w ) && ( dofun = skill_function(w) ) != skill_notfound )
               {
                  skill->skill_fun = dofun;
                  skill->skill_fun_name = STRALLOC(w);
               }
               else if( str_prefix( "do_", w ) && ( spellfun = spell_function(w) ) != spell_notfound )
               {
                  skill->spell_fun = spellfun;
                  skill->spell_fun_name = STRALLOC(w);
               }
               else
               {
                  bug( "%s(%s): unknown code %s", __FUNCTION__, skill->name ? skill->name : "Unknown", w );
                  skill->spell_fun = spell_null;
               }
               break;
            }
            KEY( "Components", skill->components, fread_string( fp ) );
            break;

         case 'D':
            KEY( "Dammsg", skill->noun_damage, fread_string( fp ) );
            KEY( "Dice", skill->dice, fread_string( fp ) );
            KEY( "Diechar", skill->die_char, fread_string( fp ) );
            KEY( "Dieroom", skill->die_room, fread_string( fp ) );
            KEY( "Dievict", skill->die_vict, fread_string( fp ) );
            KEY( "Difficulty", skill->difficulty, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !skill->skill_fun && !skill->spell_fun )
                  bug( "%s(%s): No skill or spell code set.", __FUNCTION__, skill->name ? skill->name : "Unknown" );

               if( skill->htext && skill->htext[0] == '.' && skill->htext[1] == ' ' )
               {
                  char *tmptext = un_fix_help( skill->htext );
                  STRSET( skill->htext, tmptext );
               }

               if( skill->saves != 0 && SPELL_SAVE( skill ) == SE_NONE )
               {
                  bug( "%s(%s):  Has saving throw (%d) with no saving effect.",
                     __FUNCTION__, skill->name ? skill->name : "Unknown", skill->saves );
                  SET_SSAV( skill, SE_NEGATE );
               }

               add_skill_help( skill, false );
               return skill;
            }
            break;

         case 'F':
            WEXTKEY( "Flags", skill->flags, fp, spell_flag, SF_MAX );
            break;

         case 'H':
            KEY( "HText", skill->htext, fread_string( fp ) );
            KEY( "Hitchar", skill->hit_char, fread_string( fp ) );
            KEY( "Hitdest", skill->hit_dest, fread_string( fp ) );
            KEY( "Hitroom", skill->hit_room, fread_string( fp ) );
            KEY( "Hitvict", skill->hit_vict, fread_string( fp ) );
            break;

         case 'I':
            KEY( "Immchar", skill->imm_char, fread_string( fp ) );
            KEY( "Immroom", skill->imm_room, fread_string( fp ) );
            KEY( "Immvict", skill->imm_vict, fread_string( fp ) );
            break;

         case 'M':
            KEY( "Mana", skill->min_mana, fread_number( fp ) );
            SKEY( "Minpos", skill->minimum_position, fp, pos_names, POS_MAX );
            KEY( "Misschar", skill->miss_char, fread_string( fp ) );
            KEY( "Missroom", skill->miss_room, fread_string( fp ) );
            KEY( "Missvict", skill->miss_vict, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Name", skill->name, fread_string( fp ) );
            if( !str_cmp( word, "NonMagical" ) )
            {
               skill->magical = false;
               fMatch = true;
               break;
            }
            break;

         case 'P':
            KEY( "Participants", skill->participants, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Range", skill->range, fread_number( fp ) );
            KEY( "Rounds", skill->beats, fread_number( fp ) );
            if( !str_cmp( word, "ReqSkill" ) )
            {
               infoflags = fread_word( fp );
               skill->reqskillname = STRALLOC( infoflags );
               fMatch = true;
               break;
            }
            break;

         case 'S':
            if( !str_cmp( word, "Spell" ) )
            {
               skill->tmpspell = true;
               fMatch = true;
               break;
            }
            SKEY( "SDamage", skill->damage, fp, spell_damage, SD_MAX );
            SKEY( "SAction", skill->action, fp, spell_action, SA_MAX );
            SKEY( "SClass", skill->Class, fp, spell_class, SC_MAX );
            SKEY( "SPower", skill->power, fp, spell_power, SP_MAX );
            SKEY( "SSave", skill->save, fp, spell_save_effect, SE_MAX );
            KEY( "Saves", skill->saves, fread_number( fp ) );
            KEY( "Slot", skill->slot, fread_number( fp ) );
            WEXTKEY( "Ssector", skill->spell_sector, fp, sect_flags, SECT_MAX );
            if( !str_cmp( word, "Stat" ) )
            {
               int ustat, stat;

               stat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
                  skill->stats[ustat] = stat;
               fMatch = true;
               break;
            }
            break;

         case 'T':
            SKEY( "Target", skill->target, fp, target_type, TAR_MAX );
            if( !str_cmp( word, "Teachers" ) )
            {
               char modifier[MSL];

               modifier[0] = '\0';
               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  if( skill->type == SKILL_PERSONAL )
                  {
                     if( !valid_pfile( flag ) )
                        bug( "%s: [%s] doesn't have a matching pfile for teacher, for skill [%s].", __FUNCTION__, flag, skill->name ? skill->name : "Unknown" );
                     else
                        mudstrlcat( modifier, flag, sizeof( modifier ) );
                  }
                  else
                  {
                     if( get_mob_index( atoi( flag ) ) || skill->type == SKILL_PERSONAL )
                        mudstrlcat( modifier, flag, sizeof( modifier ) );
                     else
                        bug( "%s: [%s] isn't a valid teacher for skill [%s].", __FUNCTION__, flag,
                           skill->name ? skill->name : "Unknown" );
                  }
               }
               skill->teachers = STRALLOC( modifier );
               fMatch = true;
               break;
            }
            KEY( "Type", skill->type, get_skill( fread_word( fp ) ) );
            break;

         case 'V':
            KEY( "Value", skill->value, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Wearoff", skill->msg_off, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void load_skill_table( void )
{
   FILE *fp;

   if( !( fp = fopen( SKILL_FILE, "r" ) ) )
   {
      perror( SKILL_FILE );
      bug( "%s: Can't open %s", __FUNCTION__, SKILL_FILE );
      exit( 0 );
   }

   top_sn = 0;
   for( ;; )
   {
      char letter;
      char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      if( letter != '#' )
      {
         bug( "%s: (%c) found instead of a #.", __FUNCTION__, letter );
         break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "SKILL" ) )
      {
         if( top_sn >= MAX_SKILL )
         {
            bug( "%s: more skills than MAX_SKILL %d", __FUNCTION__, MAX_SKILL );
            fclose( fp );
            fp = NULL;
            return;
         }
         skill_table[top_sn++] = fread_skill( fp );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section (%s).", __FUNCTION__, word );
         fread_to_eol( fp );
         continue;
      }
   }
   fclose( fp );
   fp = NULL;
}

void load_herb_table( void )
{
   FILE *fp;

   if( !( fp = fopen( HERB_FILE, "r" ) ) )
   {
      bug( "%s: Can't open %s", __FUNCTION__, HERB_FILE );
      exit( 0 );
   }
   top_herb = 0;
   for( ;; )
   {
      char letter;
      char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      if( letter != '#' )
      {
         bug( "%s: (%c) found instead of a #.", __FUNCTION__, letter );
         break;
      }
      word = fread_word( fp );
      if( !str_cmp( word, "HERB" ) )
      {
         if( top_herb >= MAX_HERB )
         {
            bug( "%s: more herbs than MAX_HERB %d", __FUNCTION__, MAX_HERB );
            fclose( fp );
            fp = NULL;
            return;
         }
         herb_table[top_herb++] = fread_skill( fp );
         if( herb_table[top_herb - 1]->slot == -1 )
            herb_table[top_herb - 1]->slot = top_herb - 1;
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section (%s).", __FUNCTION__, word );
         fread_to_eol( fp );
         continue;
      }
   }
   fclose( fp );
   fp = NULL;
}

void load_pers_table( void )
{
   FILE *fp;

   top_pers = 0;
   if( !( fp = fopen( PERSONAL_FILE, "r" ) ) )
   {
      bug( "%s: Can't open %s", __FUNCTION__, PERSONAL_FILE );
      return; /* This file isn't to big of a deal if it is missing */
   }
   for( ;; )
   {
      char letter;
      char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      if( letter != '#' )
      {
         bug( "%s: (%c) found instead of a #.", __FUNCTION__, letter );
         break;
      }
      word = fread_word( fp );
      if( !str_cmp( word, "PERSONAL" ) )
      {
         if( top_pers >= MAX_PERS )
         {
            bug( "%s: more personals than MAX_PERS %d", __FUNCTION__, MAX_PERS );
            fclose( fp );
            fp = NULL;
            return;
         }
         pers_table[top_pers++] = fread_skill( fp );
         if( pers_table[top_pers - 1]->slot == -1 )
            pers_table[top_pers - 1]->slot = top_pers - 1;
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section (%s).", __FUNCTION__, word );
         fread_to_eol( fp );
         continue;
      }
   }
   fclose( fp );
   fp = NULL;
}

void free_tongues( void )
{
   LANG_DATA *lang;
   LCNV_DATA *lcnv;

   while( ( lang = last_lang ) )
   {
      while( ( lcnv = lang->last_precnv ) )
      {
         UNLINK( lcnv, lang->first_precnv, lang->last_precnv, next, prev );
         STRFREE( lcnv->old );
         STRFREE( lcnv->lnew );
         DISPOSE( lcnv );
      }

      while( ( lcnv = lang->last_cnv ) )
      {
         UNLINK( lcnv, lang->first_cnv, lang->last_cnv, next, prev );
         STRFREE( lcnv->old );
         STRFREE( lcnv->lnew );
         DISPOSE( lcnv );
      }
      STRFREE( lang->name );
      STRFREE( lang->alphabet );
      UNLINK( lang, first_lang, last_lang, next, prev );
      DISPOSE( lang );
   }
}

/* Tongues / Languages loading/saving functions - Altrag */
void fread_cnv( FILE *fp, LCNV_DATA **first_cnv, LCNV_DATA **last_cnv )
{
   LCNV_DATA *cnv;
   char letter;

   for( ;; )
   {
      letter = fread_letter( fp );
      if( letter == '~' || letter == EOF )
         break;
      ungetc( letter, fp );
      CREATE( cnv, LCNV_DATA, 1 );

      cnv->old = STRALLOC( fread_word( fp ) );
      cnv->olen = strlen( cnv->old );
      cnv->lnew = STRALLOC( fread_word( fp ) );
      cnv->nlen = strlen( cnv->lnew );
      fread_to_eol( fp );
      LINK( cnv, *first_cnv, *last_cnv, next, prev );
   }
}

void load_tongues( void )
{
   FILE *fp;
   LANG_DATA *lng;
   char *word, letter;

   if( !( fp = fopen( TONGUE_FILE, "r" ) ) )
   {
      perror( TONGUE_FILE );
      return;
   }
   for( ;; )
   {
      letter = fread_letter( fp );
      if( letter == EOF )
         return;
      else if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      else if( letter != '#' )
      {
         bug( "%s: Letter '%c' not #.", __FUNCTION__, letter );
         exit( 0 );
      }
      word = fread_word( fp );
      if( !str_cmp( word, "end" ) )
         break;
      fread_to_eol( fp );
      CREATE( lng, LANG_DATA, 1 );
      lng->name = STRALLOC( word );
      fread_cnv( fp, &lng->first_precnv, &lng->last_precnv );
      lng->alphabet = fread_string( fp );
      fread_cnv( fp, &lng->first_cnv, &lng->last_cnv );
      fread_to_eol( fp );
      LINK( lng, first_lang, last_lang, next, prev );
   }
   fclose( fp );
   fp = NULL;
}

void fwrite_langs( void )
{
   FILE *fp;
   LANG_DATA *lng;
   LCNV_DATA *cnv;

   if( !( fp = fopen( TONGUE_FILE, "w" ) ) )
   {
      perror( TONGUE_FILE );
      return;
   }
   for( lng = first_lang; lng; lng = lng->next )
   {
      fprintf( fp, "#%s\n", lng->name );
      for( cnv = lng->first_precnv; cnv; cnv = cnv->next )
         fprintf( fp, "'%s' '%s'\n", cnv->old, cnv->lnew );
      fprintf( fp, "~\n%s~\n", lng->alphabet );
      for( cnv = lng->first_cnv; cnv; cnv = cnv->next )
         fprintf( fp, "'%s' '%s'\n", cnv->old, cnv->lnew );
      fprintf( fp, "\n" );
   }
   fprintf( fp, "#end\n\n" );
   fclose( fp );
   fp = NULL;
}
