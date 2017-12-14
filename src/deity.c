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
 *                           Deity handling module                           *
 *****************************************************************************/

/* Put together by Rennard for Realms of Despair.  Brap on...*/
#include <stdio.h>
#include "h/mud.h"

DEITY_DATA *first_deity, *last_deity;

void free_deity_worshippers( DEITY_DATA *deity )
{
   MEMBER_DATA *member, *member_next;

   if( !deity )
      return;
   for( member = deity->first_worshipper; member; member = member_next )
   {
      member_next = member->next;
      UNLINK( member, deity->first_worshipper, deity->last_worshipper, next, prev );
      free_member( member );
   }
}

bool is_deity_worshipper( DEITY_DATA *deity, const char *name )
{
   MEMBER_DATA *member;

   if( !deity || !name || name[0] == '\0' )
      return false;
   for( member = deity->first_worshipper; member; member = member->next )
   {
      if( !str_cmp( member->name, name ) )
         return true;
   }
   return false;
}

void add_deity_worshipper( DEITY_DATA *deity, const char *name )
{
   MEMBER_DATA *member;

   if( !deity || !name || name[0] == '\0' )
      return;
   if( is_deity_worshipper( deity, name ) )
      return;
   CREATE( member, MEMBER_DATA, 1 );
   member->name = STRALLOC( name );
   LINK( member, deity->first_worshipper, deity->last_worshipper, next, prev );
   if( ( deity->worshippers + 1 ) > 0 )
      deity->worshippers++;
}

bool rename_deity_worshipper( DEITY_DATA *deity, const char *oname, const char *nname )
{
   MEMBER_DATA *member;

   if( !deity || !oname || oname[0] == '\0' || !nname || nname[0] == '\0' )
      return false;

   for( member = deity->first_worshipper; member; member = member->next )
   {
      if( !str_cmp( member->name, oname ) )
      {
         STRSET( member->name, nname );
         save_deity( deity );
         return true;
      }
   }

   return false;
}

bool remove_deity_worshipper( DEITY_DATA *deity, const char *name )
{
   MEMBER_DATA *member;

   if( !deity || !name || name[0] == '\0' )
      return false;

   for( member = deity->first_worshipper; member; member = member->next )
   {
      if( !str_cmp( member->name, name ) )
      {
         UNLINK( member, deity->first_worshipper, deity->last_worshipper, next, prev );
         free_member( member );
         if( --deity->worshippers < 0 )
            deity->worshippers = 0;
         return true;
      }
   }
   return false;
}

void free_deity( DEITY_DATA *deity )
{
   UNLINK( deity, first_deity, last_deity, next, prev );
   free_deity_worshippers( deity );
   STRFREE( deity->name );
   STRFREE( deity->description );
   STRFREE( deity->filename );
   DISPOSE( deity );
}

void write_deity_list( void )
{
   DEITY_DATA *deity;
   FILE *fp;

   if( !( fp = fopen( DEITY_LIST, "w" ) ) )
   {
      bug( "%s: FATAL: can't open %s for writing!", __FUNCTION__, DEITY_LIST );
      perror( DEITY_LIST );
      return;
   }
   for( deity = first_deity; deity; deity = deity->next )
      if( deity && deity->filename )
         fprintf( fp, "%s\n", deity->filename );
   fprintf( fp, "$\n" );
   fclose( fp );
   fp = NULL;
}

void delete_deity( DEITY_DATA *deity )
{
   PC_DATA *pc;

   if( !deity )
      return;

   /* Remove the deity from pcdata */
   for( pc = first_pc; pc; pc = pc->next )
   {
      if( pc->deity && pc->deity == deity )
      {
         pc->favor = 0;
         pc->deity = NULL;
         if( pc->character )
         {
            send_to_char( "Your deity no longer exist.\r\n", pc->character );
            update_aris( pc->character );
            save_char_obj( pc->character );
         }
      }
   }

   if( deity->filename )
   {
      char filename[MSL];

      snprintf( filename, sizeof( filename ), "%s%s", DEITY_DIR, deity->filename );
      remove( filename );
   }

   free_deity( deity );
   write_deity_list( );
}

void free_deities( void )
{
   DEITY_DATA *deity, *deity_next;

   for( deity = first_deity; deity; deity = deity_next )
   {
      deity_next = deity->next;
      free_deity( deity );
   }
}

/* Get pointer to deity structure from deity name */
DEITY_DATA *get_deity( char *name )
{
   DEITY_DATA *deity;

   for( deity = first_deity; deity; deity = deity->next )
      if( !str_cmp( name, deity->name ) )
         return deity;
   return NULL;
}

/* Save a deity's data to its data file */
void save_deity( DEITY_DATA *deity )
{
   FILE *fp;
   MEMBER_DATA *member;
   char filename[MIL];
   int stat;

   if( !deity )
   {
      bug( "%s: NULL deity!", __FUNCTION__ );
      return;
   }

   if( !deity->name )
   {
      bug( "%s: NULL deity name!", __FUNCTION__ );
      return;
   }

   if( !deity->filename )
   {
      bug( "%s: %s has no filename", __FUNCTION__, deity->name );
      return;
   }

   snprintf( filename, sizeof( filename ), "%s%s", DEITY_DIR, deity->filename );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: couldn't open %s for writing.", __FUNCTION__, filename );
      perror( filename );
      return;
   }

   fprintf( fp, "Filename     %s~\n", deity->filename );
   fprintf( fp, "Name         %s~\n", deity->name );
   if( deity->description )
      fprintf( fp, "Description  %s~\n", deity->description );
   if( !xIS_EMPTY( deity->affected ) )
      fprintf( fp, "Affected     %s~\n", ext_flag_string( &deity->affected, a_flags ) );
   for( stat = 0; stat < RIS_MAX; stat++ )
      if( deity->resistant[stat] != 0 )
         fprintf( fp, "NResistant   %d %s~\n", deity->resistant[stat], ris_flags[stat] );
   if( deity->alignment )
      fprintf( fp, "Alignment    %d\n", deity->alignment );
   if( deity->flee )
      fprintf( fp, "Flee         %d\n", deity->flee );
   if( deity->kill )
      fprintf( fp, "Kill         %d\n", deity->kill );
   if( deity->kill_magic )
      fprintf( fp, "Kill_magic   %d\n", deity->kill_magic );
   if( deity->sac )
      fprintf( fp, "Sac          %d\n", deity->sac );
   if( deity->bury_corpse )
      fprintf( fp, "Bury_corpse  %d\n", deity->bury_corpse );
   if( deity->aid_spell )
      fprintf( fp, "Aid_spell    %d\n", deity->aid_spell );
   if( deity->aid )
      fprintf( fp, "Aid          %d\n", deity->aid );
   if( deity->steal )
      fprintf( fp, "Steal        %d\n", deity->steal );
   if( deity->backstab )
      fprintf( fp, "Backstab     %d\n", deity->backstab );
   if( deity->die )
      fprintf( fp, "Die          %d\n", deity->die );
   if( deity->spell_aid )
      fprintf( fp, "Spell_aid    %d\n", deity->spell_aid );
   if( deity->dig_corpse )
      fprintf( fp, "Dig_corpse   %d\n", deity->dig_corpse );
   if( deity->scorpse )
      fprintf( fp, "Scorpse      %d\n", deity->scorpse );
   if( deity->savatar )
      fprintf( fp, "Savatar      %d\n", deity->savatar );
   if( deity->sdeityobj )
      fprintf( fp, "Sdeityobj    %d\n", deity->sdeityobj );
   if( deity->srecall )
      fprintf( fp, "Srecall      %d\n", deity->srecall );
   if( !xIS_EMPTY( deity->Class ) )
      fprintf( fp, "Class        %s~\n", ext_class_string( &deity->Class ) );
   if( !xIS_EMPTY( deity->race ) )
      fprintf( fp, "Race         %s~\n", ext_race_string( &deity->race ) );
   if( deity->sex != -1 )
      fprintf( fp, "Sex          %d\n", deity->sex );
   if( deity->resistnum )
      fprintf( fp, "Resistnum    %d\n", deity->resistnum );
   if( deity->affectednum )
      fprintf( fp, "Affectednum  %d\n", deity->affectednum );
   if( deity->objvnum )
      fprintf( fp, "Objvnum      %d\n", deity->objvnum );
   if( deity->mobvnum )
      fprintf( fp, "Mobvnum      %d\n", deity->mobvnum );

   for( member = deity->first_worshipper; member; member = member->next )
      fprintf( fp, "Worshipper   %s~\n", member->name );

   fprintf( fp, "%s", "End\n" );
   fclose( fp );
   fp = NULL;
}

/* Create a new deity */
DEITY_DATA *new_deity( void )
{
   DEITY_DATA *deity = NULL;
   int stat;

   CREATE( deity, DEITY_DATA, 1 );
   if( !deity )
   {
      bug( "%s: deity NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   deity->name = NULL;
   deity->filename = NULL;
   deity->description = NULL;
   deity->alignment = 0;
   deity->flee = 0;
   deity->kill = 0;
   deity->kill_magic = 0;
   deity->sac = 0;
   deity->bury_corpse = 0;
   deity->aid = 0;
   deity->aid_spell = 0;
   deity->steal = 0;
   deity->backstab = 0;
   deity->die = 0;
   deity->spell_aid = 0;
   deity->dig_corpse = 0;
   deity->scorpse = 0;
   deity->savatar = 0;
   deity->sdeityobj = 0;
   deity->srecall = 0;
   deity->sex = -1;
   deity->affectednum = 0;
   deity->objvnum = 0;
   deity->mobvnum = 0;
   deity->first_worshipper = deity->last_worshipper = NULL;
   for( stat = 0; stat < RIS_MAX; stat++ )
      deity->resistant[stat] = 0;
   deity->resistnum = 0;
   deity->worshippers = 0;
   xCLEAR_BITS( deity->Class );
   xCLEAR_BITS( deity->race );
   xCLEAR_BITS( deity->affected );
   return deity;
}

/* Read in actual deity data */
void fread_deity( DEITY_DATA *deity, FILE *fp )
{
   const char *word;
   char *infoflags, flag[MIL];
   int value;
   bool fMatch;

   deity = new_deity( );

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
            WEXTKEY( "Affected", deity->affected, fp, a_flags, AFF_MAX );
            KEY( "Affectednum", deity->affectednum, fread_number( fp ) );
            KEY( "Aid", deity->aid, fread_number( fp ) );
            KEY( "Aid_spell", deity->aid_spell, fread_number( fp ) );
            KEY( "Alignment", deity->alignment, fread_number( fp ) );
            break;

         case 'B':
            KEY( "Backstab", deity->backstab, fread_number( fp ) );
            KEY( "Bury_corpse", deity->bury_corpse, fread_number( fp ) );
            break;

         case 'C':
            if( !str_cmp( word, "Class" ) )
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
                        xSET_BIT( deity->Class, iclass );
                        break;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'D':
            KEY( "Description", deity->description, fread_string( fp ) );
            KEY( "Die", deity->die, fread_number( fp ) );
            KEY( "Dig_corpse", deity->dig_corpse, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               LINK( deity, first_deity, last_deity, next, prev );
               return;
            }
            break;

         case 'F':
            KEY( "Filename", deity->filename, fread_string( fp ) );
            KEY( "Flee", deity->flee, fread_number( fp ) );
            break;

         case 'K':
            KEY( "Kill", deity->kill, fread_number( fp ) );
            KEY( "Kill_magic", deity->kill_magic, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Mobvnum", deity->mobvnum, fread_number( fp ) );
            break;

         case 'N':
            if( !str_cmp( word, "NResistant" ) )
            {
               int tmpvalue = fread_number( fp );
               infoflags = fread_flagstring( fp );
               value = get_flag( infoflags, ris_flags, RIS_MAX );
               if( value < 0 || value >= RIS_MAX )
                  bug( "%s: Unknown %s: %s", __FUNCTION__, word, infoflags );
               else
                  deity->resistant[value] = tmpvalue;
               fMatch = true;
               break;
            }
            KEY( "Name", deity->name, fread_string( fp ) );
            break;

         case 'O':
            KEY( "Objvnum", deity->objvnum, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Resist" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            KEY( "Resistnum", deity->resistnum, fread_number( fp ) );
            if( !str_cmp( word, "Race" ) )
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
                        xSET_BIT( deity->race, irace );
                        break;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'S':
            KEY( "Sac", deity->sac, fread_number( fp ) );
            KEY( "Savatar", deity->savatar, fread_number( fp ) );
            KEY( "Scorpse", deity->scorpse, fread_number( fp ) );
            KEY( "Sdeityobj", deity->sdeityobj, fread_number( fp ) );
            KEY( "Srecall", deity->srecall, fread_number( fp ) );
            KEY( "Sex", deity->sex, fread_number( fp ) );
            KEY( "Spell_aid", deity->spell_aid, fread_number( fp ) );
            KEY( "Steal", deity->steal, fread_number( fp ) );
            if( !str_cmp( word, "Suscept" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Susceptnum" ) )
            {
               fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "Worshipper" ) )
            {
               char *name = fread_flagstring( fp );

               if( valid_pfile( name ) )
                  add_deity_worshipper( deity, name );
               else
                  bug( "%s: not adding worshipper %s because no pfile found.", __FUNCTION__, name );
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
   free_deity( deity );
}

/* Load a deity file */
void load_deity_file( const char *deityfile )
{
   DEITY_DATA *deity = NULL;
   FILE *fp;
   char filename[MIL];

   snprintf( filename, sizeof( filename ), "%s%s", DEITY_DIR, deityfile );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      bug( "%s: couldn't open %s for reading.", __FUNCTION__, filename );
      perror( filename );
      return;
   }

   fread_deity( deity, fp );

   fclose( fp );
   fp = NULL;
}

/* Load in all the deity files */
void load_deity( void )
{
   FILE *fp;
   const char *filename;

   first_deity = last_deity = NULL;

   log_string( "Loading deities..." );
   if( !( fp = fopen( DEITY_LIST, "r" ) ) )
   {
      bug( "%s: couldn't open %s for reading.", __FUNCTION__, DEITY_LIST );
      perror( DEITY_LIST );
      return;
   }

   for( ;; )
   {
      filename = feof( fp ) ? "$" : fread_word( fp );
      if( filename[0] == '$' )
         break;
      log_string( filename );
      load_deity_file( filename );
   }
   fclose( fp );
   fp = NULL;
   log_string( " Done loading deities " );
}

void show_deity( CHAR_DATA *ch, DEITY_DATA *deity )
{
   MEMBER_DATA *member;
   int cnt = 0;

   if( !ch || is_npc( ch ) )
      return;

   if( !deity )
   {
      send_to_char( "No such deity.\r\n", ch );
      return;
   }

   if( deity->name )
      ch_printf( ch, "Deity      : %s\r\n", deity->name );
   if( deity->filename )
      ch_printf( ch, "Filename   : %s\r\n", deity->filename );
   if( deity->description )
      ch_printf( ch, "Description:\r\n%s\r\n", deity->description );
   if( !xIS_EMPTY( deity->race ) )
      ch_printf( ch, "Races      : %s\r\n", ext_race_string( &deity->race ) );
   if( !xIS_EMPTY( deity->Class ) )
      ch_printf( ch, "Classes    : %s\r\n", ext_class_string( &deity->Class ) );
   if( deity->sex != -1 )
      ch_printf( ch, "Sex        : %s\r\n", sex_names[deity->sex] );

   ch_printf( ch, "Alignment  : %d\r\n", deity->alignment );
   ch_printf( ch, "Mobvnum    : %d\r\n", deity->mobvnum );
   ch_printf( ch, "Objvnum    : %d\r\n", deity->objvnum );

   for( cnt = 0; cnt < RIS_MAX; cnt++ )
   {
      if( deity->resistant[cnt] == 0 )
         continue;
      ch_printf( ch, "Resistant  : %s %d\r\n", ris_flags[cnt], deity->resistant[cnt] );
   
   }
   ch_printf( ch, "Resistnum  : %d\r\n", deity->resistnum );

   if( !xIS_EMPTY( deity->affected ) )
   {
      ch_printf( ch, "Affected   : %s\r\n", ext_flag_string( &deity->affected, a_flags ) );
      ch_printf( ch, "Affectednum: %d\r\n", deity->affectednum );
   }
   ch_printf( ch, "Flee       : %d\r\n", deity->flee );
   ch_printf( ch, "Kill       : %-5d  Kill_magic: %d\r\n", deity->kill, deity->kill_magic );
   ch_printf( ch, "Aid        : %-5d  Aid_spell : %-5d  Spell_aid  : %d\r\n", deity->aid, deity->aid_spell, deity->spell_aid );
   ch_printf( ch, "Sac        : %d\r\n", deity->sac );
   ch_printf( ch, "Bury_corpse: %-5d  Dig_corpse: %d\r\n", deity->bury_corpse, deity->dig_corpse );
   ch_printf( ch, "Steal      : %d\r\n", deity->steal );
   ch_printf( ch, "Backstab   : %d\r\n", deity->backstab );
   ch_printf( ch, "Die        : %d\r\n", deity->die );
   ch_printf( ch, "Scorpse    : %d\r\n", deity->scorpse );
   ch_printf( ch, "Savatar    : %d\r\n", deity->savatar );
   ch_printf( ch, "Sdeityobj  : %d\r\n", deity->sdeityobj );
   ch_printf( ch, "Srecall    : %d\r\n", deity->srecall );

   ch_printf( ch, "Worshippers: %d\r\n", deity->worshippers );
   for( member = deity->first_worshipper; member; member = member->next )
   {
      ch_printf( ch, "  %15.15s", member->name );
      if( ++cnt == 4 )
      {
         cnt = 0;
         send_to_char( "\r\n", ch );
      }
   }
   if( cnt != 0 )
      send_to_char( "\r\n", ch );
}

CMDF( do_setdeity )
{
   DEITY_DATA *deity;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int value = 0;

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         send_to_char( "You can't do this while in another command.\r\n", ch );
         return;

      case SUB_DEITYDESC:
         deity = ( DEITY_DATA * ) ch->dest_buf;
         STRFREE( deity->description );
         deity->description = copy_buffer( ch );
         stop_editing( ch );
         save_deity( deity );
         ch->substate = ch->tempnum;
         return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: setdeity <deity> [create/delete]\r\n", ch );
      send_to_char( "Usage: setdeity <deity> <field> <toggle>\r\n", ch );
      send_to_char( "Field being one of:\r\n", ch );
      send_to_char( "      sex    class     filename    resistnum\r\n", ch );
      send_to_char( "     name    race2     affected  affectednum\r\n", ch );
      send_to_char( "     race  mobvnum    alignment  description\r\n", ch );
      send_to_char( "     type  objvnum    resistant\r\n", ch );
      send_to_char( "Field Favor adjustments:\r\n", ch );
      send_to_char( "      aid     flee   backstab   dig_corpse\r\n", ch );
      send_to_char( "      die     kill  aid_spell   kill_magic\r\n", ch );
      send_to_char( "      sac    steal  spell_aid  bury_corpse\r\n", ch );
      send_to_char( "Field Favor requirements for supplicate:\r\n", ch );
      send_to_char( "  savatar  scorpse    srecall    sdeityobj\r\n", ch );
      return;
   }

   deity = get_deity( arg1 );

   if( !str_cmp( arg2, "create" ) )
   {
      if( deity )
      {
         send_to_char( "There is already a deity with that name.\r\n", ch );
         return;
      }
      if( !( deity = new_deity( ) ) )
         return;
      LINK( deity, first_deity, last_deity, next, prev );
      deity->name = STRALLOC( arg1 );
      deity->filename = STRALLOC( strlower( arg1 ) );
      write_deity_list( );
      save_deity( deity );
      ch_printf( ch, "%s deity has been created\r\n", deity->name );
      return;
   }

   if( !deity )
   {
      send_to_char( "No such deity to set.\r\n", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' )
   {
      show_deity( ch, deity );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      delete_deity( deity );
      send_to_char( "The deity has been deleted.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      DEITY_DATA *udeity;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set a deity's name to nothing.\r\n", ch );
         return;
      }
      if( ( udeity = get_deity( argument ) ) )
      {
         send_to_char( "There is already another deity with that name.\r\n", ch );
         return;
      }
      STRSET( deity->name, argument );
      save_deity( deity );
      send_to_char( "Deity name changed.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set a deity's filename to nothing.\r\n", ch );
         return;
      }
      if( !can_use_path( ch, DEITY_DIR, argument ) )
         return;
      if( deity->filename )
      {
         snprintf( filename, sizeof( filename ), "%s%s", DEITY_DIR, deity->filename );
         if( !remove( filename ) )
            send_to_char( "Old deity file deleted.\r\n", ch );
      }
      STRSET( deity->filename, argument );
      save_deity( deity );
      write_deity_list( );
      send_to_char( "Deity filename changed.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "description" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_DEITYDESC;
      ch->dest_buf = deity;
      start_editing( ch, deity->description );
      return;
   }

   if( !str_cmp( arg2, "alignment" ) )
   {
      deity->alignment = URANGE( -1000, atoi( argument ), 1000 );
      ch_printf( ch, "Alignment set to %d.\r\n", deity->alignment );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "objvnum" ) )
   {
      value = URANGE( 0, atoi( argument ), MAX_VNUM );
      if( value != 0 && !get_obj_index( value ) )
      {
         send_to_char( "No object has that vnum. Use 0 for no objvnum.\r\n", ch );
         return;
      }

      deity->objvnum = value;
      save_deity( deity );
      send_to_char( "Deity objvnum changed.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "mobvnum" ) )
   {
      value = URANGE( 0, atoi( argument ), MAX_VNUM );
      if( value != 0 && !get_mob_index( value ) )
      {
         send_to_char( "No mobile has that vnum. Use 0 for no mobvnum.\r\n", ch );
         return;
      }

      deity->mobvnum = value;
      save_deity( deity );
      send_to_char( "Deity mobvnum changed.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "race" ) )
   {
      bool modified = false;
      int stat;

      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         stat = get_pc_race( arg1 );
         if( stat < 0 || stat >= MAX_PC_RACE )
            ch_printf( ch, "Invalid race [%s].\r\n", arg1 );
         else
         {
            xTOGGLE_BIT( deity->race, stat );
            modified = true;
         }
      }
      if( modified )
      {
         send_to_char( "Deity Race(s) have been set.\r\n", ch );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "class" ) )
   {
      bool modified = false;
      int stat;

      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         stat = get_pc_class( arg1 );
         if( stat < 0 || stat >= MAX_PC_CLASS )
            ch_printf( ch, "Invalid class [%s].\r\n", arg1 );
         else
         {
            xTOGGLE_BIT( deity->Class, stat );
            modified = true;
         }
      }
      if( modified )
      {
         send_to_char( "Deity Class(es) have been set.\r\n", ch );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      bool fMatch = false;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = true;
            xCLEAR_BITS( deity->affected );
         }
         else
         {
            value = get_flag( arg3, a_flags, AFF_MAX );
            if( value < 0 || value >= AFF_MAX )
               ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
            else
            {
               xTOGGLE_BIT( deity->affected, value );
               fMatch = true;
            }
         }
      }

      if( fMatch )
      {
         send_to_char( "Deity affected has been set.\r\n", ch );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "resistant" ) )
   {
      int stat;
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         for( stat = 0; stat < RIS_MAX; stat++ )
            deity->resistant[stat] = 0;
         fMatch = true;
      }
      else
      {
         value = get_flag( arg3, ris_flags, RIS_MAX );
         if( value < 0 || value >= RIS_MAX )
            ch_printf( ch, "Unknown ris: %s\r\n", arg3 );
         else
         {
            deity->resistant[value] = atoi( argument );
            fMatch = true;
         }
      }
      if( fMatch )
      {
         send_to_char( "Deity resistant has been set.\r\n", ch );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "sex" ) )
   {
      deity->sex = URANGE( -1, value, ( SEX_MAX - 1 ) );
      save_deity( deity );
      ch_printf( ch, "Sex set to %d.\r\n", deity->sex );
      return;
   }

   value = atoi( argument );

   if( !str_cmp( arg2, "flee" ) )
      deity->flee = value;
   else if( !str_cmp( arg2, "kill" ) )
      deity->kill = value;
   else if( !str_cmp( arg2, "kill_magic" ) )
      deity->kill_magic = value;
   else if( !str_cmp( arg2, "sac" ) )
      deity->sac = value;
   else if( !str_cmp( arg2, "bury_corpse" ) )
      deity->bury_corpse = value;
   else if( !str_cmp( arg2, "aid_spell" ) )
      deity->aid_spell = value;
   else if( !str_cmp( arg2, "aid" ) )
      deity->aid = value;
   else if( !str_cmp( arg2, "steal" ) )
      deity->steal = value;
   else if( !str_cmp( arg2, "backstab" ) )
      deity->backstab = value;
   else if( !str_cmp( arg2, "die" ) )
      deity->die = value;
   else if( !str_cmp( arg2, "spell_aid" ) )
      deity->spell_aid = value;
   else if( !str_cmp( arg2, "dig_corpse" ) )
      deity->dig_corpse = value;
   else if( !str_cmp( arg2, "scorpse" ) )
      deity->scorpse = value;
   else if( !str_cmp( arg2, "savatar" ) )
      deity->savatar = value;
   else if( !str_cmp( arg2, "sdeityobj" ) )
      deity->sdeityobj = value;
   else if( !str_cmp( arg2, "srecall" ) )
      deity->srecall = value;
   else if( !str_cmp( arg2, "affectednum" ) )
      deity->affectednum = value;
   else if( !str_cmp( arg2, "resistnum" ) )
      deity->resistnum = value;
   else
   {
      do_setdeity( ch, (char *)"" );
      return;
   }
   save_deity( deity );
   ch_printf( ch, "Deity %s set.\r\n", arg2 );
}

CMDF( do_devote )
{
   DEITY_DATA *deity;
   MCLASS_DATA *mclass;

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( ch->level < 5 )
   {
      send_to_char( "You aren't yet prepared for such devotion.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Devote yourself to which deity?\r\n", ch );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      if( !ch->pcdata->deity )
      {
         send_to_char( "You aren't worshipping any deity.\r\n", ch );
         return;
      }
      remove_deity_worshipper( ch->pcdata->deity, ch->name );
      save_deity( ch->pcdata->deity );
      ch->pcdata->deity = NULL;
      ch->pcdata->favor = 0;
      send_to_char( "You cease to worship any deity.\r\n", ch );
      update_aris( ch );
      save_char_obj( ch );
      return;
   }

   if( ch->pcdata->deity )
   {
      send_to_char( "You're already devoted to a deity.\r\n", ch );
      return;
   }

   if( !( deity = get_deity( argument ) ) )
   {
      send_to_char( "No such deity holds weight on this world.\r\n", ch );
      return;
   }

   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      if( mclass->wclass >= 0 && xIS_SET( deity->Class, mclass->wclass ) )
      {
         send_to_char( "That deity won't accept your worship due to your class.\r\n", ch );
         return;
      }
   }

   if( ( deity->sex != -1 ) && ( deity->sex != ch->sex ) )
   {
      send_to_char( "That deity won't accept worshippers of your sex.\r\n", ch );
      return;
   }

   if( xIS_SET( deity->race, ch->race ) )
   {
      send_to_char( "That deity won't accept worshippers of your race.\r\n", ch );
      return;
   }

   add_deity_worshipper( deity, ch->name );
   ch->pcdata->deity = deity;
   act( AT_MAGIC, "Body and soul, you devote yourself to $t!", ch, ch->pcdata->deity->name, NULL, TO_CHAR );
   act( AT_MAGIC, "Body and soul, $n devotes $mself to $t!", ch, ch->pcdata->deity->name, NULL, TO_ROOM );
   save_deity( ch->pcdata->deity );
   ch->pcdata->favor = 0;
   update_aris( ch );
   save_char_obj( ch );
}

CMDF( do_deities )
{
   DEITY_DATA *deity;

   if( !argument || argument[0] == '\0' )
   {
      if( !first_deity )
      {
         send_to_pager( "&gThere are no deities on this world.\r\n", ch );
         return;
      }
      pager_printf( ch, "&G%20s &w| &g%15s\r\n", "Deity", "Worshippers" );
      send_to_pager( "&W-------------------- &w| &W---------------\r\n", ch );
      for( deity = first_deity; deity; deity = deity->next )
         pager_printf( ch, "&G%20s &w| &g%15s\r\n", deity->name, num_punct( deity->worshippers ) );
      send_to_pager( "&gFor detailed information on a deity, try 'deities <deity>' or 'help deities'\r\n", ch );
      return;
   }

   if( !( deity = get_deity( argument ) ) )
   {
      send_to_pager( "&gNo such deity exist.\r\n", ch );
      return;
   }
   pager_printf( ch, "&gDeity: &G%s\r\n", deity->name );
   pager_printf( ch, "&gDescription:\r\n&G%s\r\n", deity->description ? deity->description : "(Not Set)" );
}

CMDF( do_supplicate )
{
   CHAR_DATA *rch = NULL;
   DEITY_DATA *deity;
   int oldfavor;

   if( is_npc( ch ) || !ch->pcdata->deity )
   {
      send_to_char( "You have no deity to supplicate to.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Supplicate for what?\r\n", ch );
      return;
   }

   oldfavor = ch->pcdata->favor;
   deity = ch->pcdata->deity;

   if( !str_cmp( argument, "corpse" ) && !is_pkill( ch ) )
   {
      OBJ_DATA *obj;
      char buf2[MSL];
      bool found, althere, charge;

      if( ch->pcdata->favor < deity->scorpse )
      {
         send_to_char( "You aren't favored enough for a corpse retrieval.\r\n", ch );
         return;
      }

      if( xIS_SET( ch->in_room->room_flags, ROOM_STORAGEROOM ) )
      {
         send_to_char( "You can't supplicate in a storage room.\r\n", ch );
         return;
      }

      found = false;
      althere = false;
      charge = false;
      snprintf( buf2, sizeof( buf2 ), "the corpse of %s", ch->name );
      for( obj = first_corpse; obj; obj = obj->next_corpse )
      {
         if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
         {
            if( obj->in_room == ch->in_room )
            {
               althere = true;
               continue;
            }

            found = true;
            if( xIS_SET( obj->in_room->room_flags, ROOM_NOSUPPLICATE ) )
            {
               act( AT_MAGIC, "The image of your corpse appears, but suddenly wavers away...", ch, NULL, NULL, TO_CHAR );
               continue;
            }

            charge = true;
            if( ( rch = obj->in_room->first_person ) && !is_obj_stat( obj, ITEM_BURIED ) )
            {
               act( AT_MAGIC, "$T suddenly vanishes, surrounded by a divine presence...", rch, NULL, buf2, TO_ROOM );
               act( AT_MAGIC, "$T suddenly vanishes, surrounded by a divine presence...", rch, NULL, buf2, TO_CHAR );
            }

            act( AT_MAGIC, "Your corpse appears suddenly, surrounded by a divine presence...", ch, NULL, NULL, TO_CHAR );
            act( AT_MAGIC, "$n's corpse appears suddenly, surrounded by a divine force...", ch, NULL, NULL, TO_ROOM );
            obj_from_room( obj );
            obj = obj_to_room( obj, ch->in_room );
         }
      }

      if( !found )
      {
         if( !althere )
            send_to_char( "No corpse of yours litters the world...\r\n", ch );
         else
            send_to_char( "Your corpse(s) are already in this room...\r\n", ch );
         return;
      }

      if( charge )
         ch->pcdata->favor -= deity->scorpse;
      update_aris( ch );
      return;
   }

   if( !str_cmp( argument, "avatar" ) )
   {
      MOB_INDEX_DATA *pMobIndex;
      CHAR_DATA *victim;

      if( deity->mobvnum <= 0 )
      {
         send_to_char( "Your deity doesn't have an avatar for you to supplicate.\r\n", ch );
         return;
      }
      if( ch->pcdata->favor < deity->savatar )
      {
         send_to_char( "You aren't favored enough for that.\r\n", ch );
         return;
      }
      if( !( pMobIndex = get_mob_index( deity->mobvnum ) ) )
      {
         bug( "%s: mob vnum %d doesn't exist.", __FUNCTION__, deity->mobvnum );
         return;
      }
      if( !( victim = create_mobile( pMobIndex ) ) )
      {
         bug( "%s: couldn't create_mobile for vnum %d.", __FUNCTION__, deity->mobvnum );
         return;
      }
      char_to_room( victim, ch->in_room );
      act( AT_MAGIC, "$n summons $N!", ch, NULL, victim, TO_ROOM );
      act( AT_MAGIC, "You summon $N!", ch, NULL, victim, TO_CHAR );
      add_follower( victim, ch );
      xSET_BIT( victim->affected_by, AFF_CHARM );
      victim->alignment = deity->alignment;
      victim->max_hit = ( ch->hit * 6 + ch->pcdata->favor );
      victim->hit = victim->max_hit;
      ch->pcdata->favor -= deity->savatar;
      update_aris( ch );
      return;
   }

   if( !str_cmp( argument, "object" ) )
   {
      OBJ_DATA *obj;
      OBJ_INDEX_DATA *pObjIndex;

      if( deity->objvnum <= 0 )
      {
         send_to_char( "Your deity doesn't have an object for you to supplicate.\r\n", ch );
         return;
      }
      if( ch->pcdata->favor < deity->sdeityobj )
      {
         send_to_char( "You aren't favored enough for that.\r\n", ch );
         return;
      }

      if( !( pObjIndex = get_obj_index( deity->objvnum ) ) )
      {
         bug( "%s: object vnum %d doesnt exist?", __FUNCTION__, deity->objvnum );
         return;
      }

      if( !( obj = create_object( pObjIndex, ch->level ) ) )
      {
         bug( "%s: couldn't create_object vnum %d.", __FUNCTION__, deity->objvnum );
         return;
      }

      if( !can_wear( obj, ITEM_NO_TAKE ) )
         obj = obj_to_char( obj, ch );
      else
         obj = obj_to_room( obj, ch->in_room );

      act( AT_MAGIC, "$n weaves $p from divine matter!", ch, obj, NULL, TO_ROOM );
      act( AT_MAGIC, "You weave $p from divine matter!", ch, obj, NULL, TO_CHAR );
      ch->pcdata->favor -= ch->pcdata->deity->sdeityobj;
      update_aris( ch );
      return;
   }

   if( !str_cmp( argument, "recall" ) )
   {
      ROOM_INDEX_DATA *location;

      if( ch->pcdata->favor < deity->srecall )
      {
         send_to_char( "Your favor is inadequate for such a supplication.\r\n", ch );
         return;
      }

      if( xIS_SET( ch->in_room->room_flags, ROOM_NOSUPPLICATE ) )
      {
         send_to_char( "You have been forsaken!\r\n", ch );
         return;
      }

      if( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !is_immortal( ch ) )
      {
         send_to_char( "You can't supplicate recall under adrenaline!\r\n", ch );
         return;
      }

      location = NULL;

      if( !is_npc( ch ) && ch->pcdata->clan )
         location = get_room_index( ch->pcdata->clan->recall );

      if( !is_npc( ch ) && !location && ch->level >= 5 && xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
         location = get_room_index( sysdata.room_deadly );

      /* 1998-01-02, h */
      if( !location )
         location = get_room_index( race_table[ch->race]->race_recall );
      if( !location )
         location = get_room_index( sysdata.room_temple );
      if( !location )
      {
         send_to_char( "You're completely lost.\r\n", ch );
         return;
      }

      act( AT_MAGIC, "$n disappears in a column of divine power.", ch, NULL, NULL, TO_ROOM );
      char_from_room( ch );
      char_to_room( ch, location );
      if( ch->mount )
      {
         char_from_room( ch->mount );
         char_to_room( ch->mount, location );
      }
      act( AT_MAGIC, "$n appears in the room from a column of divine mist.", ch, NULL, NULL, TO_ROOM );
      do_look( ch, (char *)"auto" );
      ch->pcdata->favor -= deity->srecall;
      update_aris( ch );
      return;
   }

   send_to_char( "You can't supplicate for that.\r\n", ch );
}

/*
 * Internal function to adjust favor.
 * Fields are:
 *  0 = flee         1 = kill          2 = kill_magic
 *  3 = sac          4 = bury_corpse   5 = aid_spell
 *  6 = aid          7 = steal         8 = backstab
 *  9 = die         10 = spell_aid    11 = dig_corpse
 */
void adjust_favor( CHAR_DATA *ch, int field, int mod )
{
   DEITY_DATA *deity;
   int aligndiff = 0;

   if( is_npc( ch ) || !ch->pcdata->deity )
      return;

   deity = ch->pcdata->deity;

   aligndiff = ( ch->alignment - deity->alignment );
   if( deity->alignment != 0 && ( aligndiff > 650 || aligndiff < -650 ) )
   {
      ch->pcdata->favor = URANGE( -2500, ( ch->pcdata->favor - 2 ), 2500 );
      update_aris( ch );
      return;
   }

   if( mod < 1 )
      mod = 1;

   switch( field )
   {
      default:
         return;

      case 0:
         ch->pcdata->favor += ( deity->flee / mod );
         break;

      case 1:
         ch->pcdata->favor += ( deity->kill / mod );
         break;

      case 2:
         ch->pcdata->favor += ( deity->kill_magic / mod );
         break;

      case 3:
         ch->pcdata->favor += ( deity->sac / mod );
         break;

      case 4:
         ch->pcdata->favor += ( deity->bury_corpse / mod );
         break;

      case 5:
         ch->pcdata->favor += ( deity->aid_spell / mod );
         break;

      case 6:
         ch->pcdata->favor += ( deity->aid / mod );
         break;

      case 7:
         ch->pcdata->favor += ( deity->steal / mod );
         break;

      case 8:
         ch->pcdata->favor += ( deity->backstab / mod );
         break;

      case 9:
         ch->pcdata->favor += ( deity->die / mod );
         break;

      case 10:
         ch->pcdata->favor += ( deity->spell_aid / mod );
         break;

      case 11:
         ch->pcdata->favor += ( deity->dig_corpse / mod );
         break;
   }
   ch->pcdata->favor = URANGE( -2500, ch->pcdata->favor, 2500 );
   update_aris( ch );
}

/* 1997, Blodkai */
CMDF( do_remains )
{
   OBJ_DATA *obj;
   char buf[MSL];
   int favor = 0;
   bool found = false;

   if( !ch || is_npc( ch ) )
      return;

   set_char_color( AT_MAGIC, ch );
   if( !ch->pcdata->deity )
   {
      send_to_pager( "You have no deity from which to seek such assistance...\r\n", ch );
      return;
   }

   favor = number_range( ch->level, ch->level * 2 );
   if( ch->pcdata->favor < favor )
   {
      send_to_pager( "Your favor is insufficient for such assistance...\r\n", ch );
      return;
   }

   pager_printf( ch, "%s appears in a vision, revealing that your remains...\r\n", ch->pcdata->deity->name );
   snprintf( buf, sizeof( buf ), "the corpse of %s", ch->name );
   for( obj = first_corpse; obj; obj = obj->next_corpse )
   {
      if( obj->pIndexData->vnum != OBJ_VNUM_CORPSE_PC )
         continue;
      if( str_cmp( buf, obj->short_descr ) )
         continue;

      found = true;

      if( obj->in_room )
         pager_printf( ch, "  - at %s will endure for %d ticks\r\n", obj->in_room->name ? obj->in_room->name : "Not Set", obj->timer );

      if( obj->carried_by )
         pager_printf( ch, "  - carried by %s will endure for %d ticks\r\n", obj->carried_by->name ? obj->carried_by->name : "Not Set", obj->timer );

      if( obj->in_obj )
      {
         OBJ_DATA *tobj = NULL;

         pager_printf( ch, "  - in %s\r\n", obj->in_obj->short_descr ? obj->in_obj->short_descr : "Not Set" );

         tobj = obj->in_obj;
         while( tobj->in_obj )
         {
            pager_printf( ch, "  - in %s\r\n", tobj->in_obj->short_descr ? tobj->in_obj->short_descr : "Not Set" );
            tobj = tobj->in_obj;
         }
         if( tobj )
         {
            if( tobj->in_obj )
               pager_printf( ch, "  - in %s will endure for %d ticks\r\n", tobj->in_obj->short_descr ? tobj->in_obj->short_descr : "Not Set", obj->timer );
            if( tobj->in_room )
               pager_printf( ch, "  - at %s will endure for %d ticks\r\n", tobj->in_room->name ? tobj->in_room->name : "Not Set", obj->timer );
            if( tobj->carried_by )
               pager_printf( ch, "  - carried by %s will endure for %d ticks\r\n", tobj->carried_by->name, obj->timer );
         }
      }
   }

   if( !found )
      send_to_pager( "  - no longer exist.\r\n", ch );
   else
      ch->pcdata->favor -= favor;
}
