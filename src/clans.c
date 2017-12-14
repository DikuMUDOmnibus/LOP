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
 *			     Special clan module			     *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include "h/mud.h"

CLAN_DATA *first_clan, *last_clan;
COUNCIL_DATA *first_council, *last_council;

void free_member( MEMBER_DATA *member )
{
   if( !member )
      return;
   STRFREE( member->name );
   DISPOSE( member );
}

void free_clan_members( CLAN_DATA *clan )
{
   MEMBER_DATA *member, *member_next;

   if( !clan )
      return;
   for( member = clan->first_member; member; member = member_next )
   {
      member_next = member->next;
      UNLINK( member, clan->first_member, clan->last_member, next, prev );
      free_member( member );
   }
}

void free_victory( VICTORY_DATA *victory )
{
   if( !victory )
      return;
   STRFREE( victory->name );
   STRFREE( victory->vname );
   DISPOSE( victory );
}

void free_clan_victories( CLAN_DATA *clan )
{
   VICTORY_DATA *victory, *next_victory;

   if( !clan )
      return;
   for( victory = clan->first_victory; victory; victory = next_victory )
   {
      next_victory = victory->next;
      UNLINK( victory, clan->first_victory, clan->last_victory, next, prev );
      free_victory( victory );
   }
}

void add_victory( CLAN_DATA *clan, const char *name, int level, char *vname, int vlevel, int vkills, time_t vtime )
{
   VICTORY_DATA *victory;

   if( !clan || !name || !vname )
      return;
   CREATE( victory, VICTORY_DATA, 1 );
   STRSET( victory->name, name );
   victory->level = level;
   STRSET( victory->vname, vname );
   victory->vlevel = vlevel;
   victory->vtime = vtime;
   victory->vkills = vkills;
   LINK( victory, clan->first_victory, clan->last_victory, next, prev );
}

/* Update existing victories and add new ones */
void update_clan_victory( CLAN_DATA *clan, CHAR_DATA *ch, CHAR_DATA *vict )
{
   VICTORY_DATA *victory, *next_victory;

   if( !clan || !ch || !vict )
      return;
   /* Update existing victories */
   for( victory = clan->first_victory; victory; victory = next_victory )
   {
      next_victory = victory->next;
      if( !str_cmp( victory->name, ch->name ) && !str_cmp( victory->vname, vict->name ) )
      {
         victory->vtime = current_time; /* Update time */
         victory->vkills++; /* Update times killed */
         victory->level = ch->level; /* Update levels */
         victory->vlevel = vict->level;
         save_clan( clan );
         return;
      }
   }
   /* Add a new victory */
   add_victory( clan, ch->name, ch->level, vict->name, vict->level, 1, current_time );
   save_clan( clan );
}

bool is_clan_member( CLAN_DATA *clan, const char *name )
{
   MEMBER_DATA *member;

   if( !clan || !name || name[0] == '\0' )
      return false;
   for( member = clan->first_member; member; member = member->next )
   {
      if( !str_cmp( member->name, name ) )
         return true;
   }
   return false;
}

bool rename_clan_member( CLAN_DATA *clan, const char *oname, const char *nname )
{
   MEMBER_DATA *member;

   if( !clan || !oname || oname[0] == '\0' || !nname || nname[0] == '\0' )
      return false;

   for( member = clan->first_member; member; member = member->next )
   {
      if( !str_cmp( member->name, oname ) )
      {
         if( clan->leader && !str_cmp( clan->leader, oname ) )
            STRSET( clan->leader, nname );
         if( clan->number1 && !str_cmp( clan->number1, oname ) )
            STRSET( clan->number1, nname );
         if( clan->number2 && !str_cmp( clan->number2, oname ) )
            STRSET( clan->number2, nname );
         STRSET( member->name, nname );
         save_clan( clan );
         return true;
      }
   }

   return false;
}

bool remove_clan_member( CLAN_DATA *clan, const char *name )
{
   MEMBER_DATA *member;

   if( !clan || !name || name[0] == '\0' )
      return false;
   for( member = clan->first_member; member; member = member->next )
   {
      if( !str_cmp( member->name, name ) )
      {
         if( clan->leader && !str_cmp( clan->leader, name ) )
            STRFREE( clan->leader );
         if( clan->number1 && !str_cmp( clan->number1, name ) )
            STRFREE( clan->number1 );
         if( clan->number2 && !str_cmp( clan->number2, name ) )
            STRFREE( clan->number2 );
         UNLINK( member, clan->first_member, clan->last_member, next, prev );
         free_member( member );
         if( --clan->members < 0 )
            clan->members = 0;
         return true;
      }
   }
   return false;
}

void add_clan_member( CLAN_DATA *clan, const char *name )
{
   MEMBER_DATA *member;

   if( !clan || !name || name[0] == '\0' )
      return;
   if( is_clan_member( clan, name ) )
      return;
   CREATE( member, MEMBER_DATA, 1 );
   member->name = STRALLOC( name );
   LINK( member, clan->first_member, clan->last_member, next, prev );
   if( ( clan->members + 1 ) > 0 )
      clan->members++;
}

void free_one_clan( CLAN_DATA *clan )
{
   if( !clan )
      return;
   free_clan_members( clan );
   free_clan_victories( clan );
   UNLINK( clan, first_clan, last_clan, next, prev );
   STRFREE( clan->filename );
   STRFREE( clan->name );
   STRFREE( clan->motto );
   STRFREE( clan->description );
   STRFREE( clan->leader );
   STRFREE( clan->number1 );
   STRFREE( clan->number2 );
   STRFREE( clan->leadrank );
   STRFREE( clan->onerank );
   STRFREE( clan->tworank );
   STRFREE( clan->badge );
   DISPOSE( clan );
}

void delete_clan( CLAN_DATA *clan )
{
   PC_DATA *pc;
   bool csave;

   if( !clan )
      return;

   /* Remove the clan from pcdata */
   for( pc = first_pc; pc; pc = pc->next )
   {
      csave = false;

      if( pc->clan && pc->clan == clan )
      {
         pc->clan = NULL;
         csave = true;
         send_to_char( "The clan you were in has been deleted.\r\n", pc->character );
      }
      if( pc->nation && pc->nation == clan )
      {
         pc->nation = NULL;
         csave = true;
         send_to_char( "The nation you were in has been deleted.\r\n", pc->character );
      }
      if( csave )
         save_char_obj( pc->character );
   }

   if( clan->filename )
   {
      char filename[MSL];

      snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename );
      remove( filename );
   }

   free_one_clan( clan );
   write_clan_list( );
}

void free_clans( void )
{
   CLAN_DATA *clan, *clan_next;

   for( clan = first_clan; clan; clan = clan_next )
   {
      clan_next = clan->next;
      free_one_clan( clan );
   }
}

/* Get pointer to clan structure from clan name. */
CLAN_DATA *get_clan( char *name )
{
   CLAN_DATA *clan;

   for( clan = first_clan; clan; clan = clan->next )
      if( clan->name && name && !str_cmp( name, clan->name ) )
         return clan;
   return NULL;
}

void write_clan_list( void )
{
   CLAN_DATA *tclan;
   FILE *fp;

   if( !( fp = fopen( CLAN_LIST, "w" ) ) )
   {
      bug( "%s: can't open %s for writing!", __FUNCTION__, CLAN_LIST );
      return;
   }
   for( tclan = first_clan; tclan; tclan = tclan->next )
      fprintf( fp, "%s\n", tclan->filename );
   fprintf( fp, "$\n" );
   fclose( fp );
   fp = NULL;
}

/* Save a clan's data to its data file */
void save_clan( CLAN_DATA *clan )
{
   FILE *fp;
   MEMBER_DATA *member;
   VICTORY_DATA *victory;
   char filename[MIL];

   if( !clan )
   {
      bug( "%s: NULL clan", __FUNCTION__ );
      return;
   }

   if( !clan->name )
   {
      bug( "%s: NULL clan->name", __FUNCTION__ );
      return;
   }

   if( !clan->filename )
   {
      bug( "%s: %s NULL filename", __FUNCTION__, clan->name );
      return;
   }

   snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      perror( filename );
      return;
   }

   fprintf( fp, "Name           %s~\n", clan->name );
   fprintf( fp, "Filename       %s~\n", clan->filename );
   if( clan->motto )
      fprintf( fp, "Motto          %s~\n", clan->motto );
   if( clan->description )
      fprintf( fp, "Description    %s~\n", clan->description );
   if( clan->leader )
   {
      fprintf( fp, "Leader         %s~\n", clan->leader );
      if( clan->leadrank )
         fprintf( fp, "Leadrank       %s~\n", clan->leadrank );
   }
   if( clan->number1 )
   {
      fprintf( fp, "NumberOne      %s~\n", clan->number1 );
      if( clan->onerank )
         fprintf( fp, "Onerank        %s~\n", clan->onerank );
   }
   if( clan->number2 )
   {
      fprintf( fp, "NumberTwo      %s~\n", clan->number2 );
      if( clan->tworank )
         fprintf( fp, "Tworank        %s~\n", clan->tworank );
   }
   if( clan->badge )
      fprintf( fp, "Badge          %s~\n", clan->badge );
   if( clan->clan_type )
      fprintf( fp, "Type           %d\n", clan->clan_type );
   if( clan->race )
      fprintf( fp, "Race           %d\n", clan->race );
   for( member = clan->first_member; member; member = member->next )
      fprintf( fp, "Member         %s~\n", member->name );
   for( victory = clan->first_victory; victory; victory = victory->next )
      fprintf( fp, "Victory        %ld %d %s~ %d %s~ %d\n", victory->vtime, victory->vkills, victory->name, victory->level, victory->vname, victory->vlevel );
   if( clan->recall )
      fprintf( fp, "Recall         %d\n", clan->recall );
   fprintf( fp, "End\n\n" );
   fclose( fp );
   fp = NULL;
}

CLAN_DATA *new_clan( void )
{
   CLAN_DATA *clan = NULL;

   CREATE( clan, CLAN_DATA, 1 );
   if( !clan )
   {
      bug( "%s: clan is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   clan->name = NULL;
   clan->filename = NULL;
   clan->motto = NULL;
   clan->description = NULL;
   clan->leader = clan->leadrank = NULL;
   clan->number1 = clan->onerank = NULL;
   clan->number2 = clan->tworank = NULL;
   clan->badge = NULL;
   clan->first_member = clan->last_member = NULL;
   clan->first_victory = clan->last_victory = NULL;
   clan->clan_type = 0;
   clan->members = 0;
   clan->recall = 0;
   return clan;
}

/* Read in actual clan data. */
CLAN_DATA *fread_clan( FILE *fp )
{
   CLAN_DATA *clan = NULL;
   const char *word;
   bool fMatch;

   clan = new_clan( );

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

         case 'B':
            KEY( "Badge", clan->badge, fread_string( fp ) );
            break;

         case 'D':
            KEY( "Description", clan->description, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return clan;
            break;

         case 'F':
            KEY( "Filename", clan->filename, fread_string( fp ) );
            break;

         case 'L':
            KEY( "Leader", clan->leader, fread_string( fp ) );
            KEY( "Leadrank", clan->leadrank, fread_string( fp ) );
            break;

         case 'M':
            if( !str_cmp( word, "Member" ) )
            {
               char *name = fread_flagstring( fp );

               if( valid_pfile( name ) )
                  add_clan_member( clan, name );
               else
                  bug( "%s: not adding member %s because no pfile found.", __FUNCTION__, name );
               fMatch = true;
               break;
            }
            KEY( "Motto", clan->motto, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Name", clan->name, fread_string( fp ) );
            KEY( "NumberOne", clan->number1, fread_string( fp ) );
            KEY( "NumberTwo", clan->number2, fread_string( fp ) );
            break;

         case 'O':
            KEY( "Onerank", clan->onerank, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Recall", clan->recall, fread_number( fp ) );
            KEY( "Race", clan->race, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Tworank", clan->tworank, fread_string( fp ) );
            KEY( "Type", clan->clan_type, fread_number( fp ) );
            break;

         case 'V':
            if( !str_cmp( word, "victory" ) )
            {
               char *name, *vname;
               time_t vtime;
               int vkills, level, vlevel;

               vtime = fread_time( fp );
               vkills = fread_number( fp );
               name = fread_string( fp ); /* Use fread_string instead of fread_flagstring here */
               level = fread_number( fp );
               vname = fread_string( fp ); /* Use fread_string instead of fread_flagstring here */
               vlevel = fread_number( fp );

               if( valid_pfile( name ) && valid_pfile( vname ) )
                  add_victory( clan, name, level, vname, vlevel, vkills, vtime );
               else
                  bug( "%s: not adding victory because either %s or %s no longer has a pfile.", __FUNCTION__, name, vname );

               /* Since we are doing 2 strings fread_flagstring would set them to the same thing, so set and free the strings */
               STRFREE( name );
               STRFREE( vname );

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

   /* Made it here? Then lets free the clan and return NULL */
   free_one_clan( clan );
   return NULL;
}

/* Load a clan file */
void load_clan_file( const char *clanfile )
{
   CLAN_DATA *clan;
   FILE *fp;
   char filename[MIL];

   snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clanfile );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      perror( filename );
      return;
   }

   clan = fread_clan( fp );
   fclose( fp );
   fp = NULL;

   if( !clan )
      return;

   LINK( clan, first_clan, last_clan, next, prev );
}

/* Load in all the clan files. */
void load_clans( void )
{
   FILE *fp;
   const char *filename;

   first_clan = last_clan = NULL;

   log_string( "Loading clans..." );
   if( !( fp = fopen( CLAN_LIST, "r" ) ) )
   {
      perror( CLAN_LIST );
      return;
   }

   for( ;; )
   {
      filename = feof( fp ) ? "$" : fread_word( fp );
      if( !filename || filename[0] == '\0' || filename[0] == '$' )
         break;
      log_string( filename );
      load_clan_file( filename );
   }
   fclose( fp );
   fp = NULL;
   log_string( " Done loading clans " );
}

CMDF( do_induct )
{
   CHAR_DATA *victim;
   CLAN_DATA *clan;
   char arg[MIL];

   if( is_npc( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( !is_name( "induct", ch->pcdata->bestowments )
   && str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if(  arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Induct whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   if( is_immortal( victim ) )
   {
      send_to_char( "You can't induct such a godly presence.\r\n", ch );
      return;
   }

   if( clan->clan_type == CLAN_NATION )
   {
      if( victim->race != clan->race )
      {
         send_to_char( "This player's race is not in accordance with your clan.\r\n", ch );
         return;
      }
   }
   else
   {
      if( victim->level < 10 )
      {
         send_to_char( "This player is not worthy of joining yet.\r\n", ch );
         return;
      }

      if( victim->level > ch->level )
      {
         send_to_char( "This player is too powerful for you to induct.\r\n", ch );
         return;
      }
   }

   if( victim->pcdata->clan )
   {
      ch_printf( ch, "This player already belongs to %s clan!\r\n", victim->pcdata->clan == clan ? "your" : "a" );
      return;
   }

   if( xIS_SET( victim->act, PLR_NOINDUCT ) )
   {
      send_to_char( "This player doesn't currently wish to be inducted into a clan.\r\n", ch );
      return;
   }

   if( clan->clan_type == CLAN_PLAIN )
      xSET_BIT( victim->pcdata->flags, PCFLAG_DEADLY );

   victim->pcdata->clan = clan;
   add_clan_member( clan, victim->name );
   act( AT_MAGIC, "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n inducts $N into $t", ch, clan->name, victim, TO_NOTVICT );
   act( AT_MAGIC, "$n inducts you into $t", ch, clan->name, victim, TO_VICT );
   echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been inducted into %s!", victim->name, clan->name );
   save_char_obj( victim );
   save_clan( clan );
}

CMDF( do_nation_induct )
{
   CHAR_DATA *victim;
   CLAN_DATA *nation;

   if( is_npc( ch ) || !ch->pcdata->nation )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   nation = ch->pcdata->nation;

   if( !is_name( "ninduct", ch->pcdata->bestowments )
   && str_cmp( ch->name, nation->leader ) && str_cmp( ch->name, nation->number1 ) && str_cmp( ch->name, nation->number2 ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Ninduct whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) || victim->pcdata->nation || is_immortal( victim ) || victim->race != nation->race )
   {
      send_to_char( "You can't ninduct them.\r\n", ch );
      return;
   }

   if( xIS_SET( victim->act, PLR_NOINDUCT ) )
   {
      send_to_char( "This player doesn't currently wish to be inducted into a nation.\r\n", ch );
      return;
   }

   victim->pcdata->nation = nation;
   add_clan_member( nation, victim->name );
   act( AT_MAGIC, "You induct $N into $t", ch, nation->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n inducts $N into $t", ch, nation->name, victim, TO_NOTVICT );
   act( AT_MAGIC, "$n inducts you into $t", ch, nation->name, victim, TO_VICT );
   echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been inducted into %s!", victim->name, nation->name );
   save_char_obj( victim );
   save_clan( nation );
}

/* Can the character outcast the victim? */
bool can_outcast( CLAN_DATA *clan, CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !clan || !ch || !victim )
      return false;
   if( !str_cmp( ch->name, clan->leader ) )
      return true;
   if( !str_cmp( victim->name, clan->leader ) )
      return false;
   if( !str_cmp( ch->name, clan->number1 ) )
      return true;
   if( !str_cmp( victim->name, clan->number1 ) )
      return false;
   if( !str_cmp( ch->name, clan->number2 ) )
      return true;
   if( !str_cmp( victim->name, clan->number2 ) )
      return false;
   return true;
}

CMDF( do_outcast )
{
   CHAR_DATA *victim;
   CLAN_DATA *clan;

   if( is_npc( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   clan = ch->pcdata->clan;

   if( !is_name( "outcast", ch->pcdata->bestowments )
   && str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Outcast whom?\r\n", ch );
      return;
   }

   if( !str_cmp( ch->name, clan->leader ) )
      victim = get_char_world( ch, argument );
   else
      victim = get_char_room( ch, argument );

   if( !victim || is_npc( victim ) )
   {
      if( !victim && !str_cmp( ch->name, clan->leader ) && remove_clan_member( clan, argument ) )
      {
         send_to_char( "That player has been removed from your clan.\r\n", ch );
         echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been outcast from %s!", capitalize( argument ), clan->name );
         save_clan( clan );
      }
      else
         send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Kick yourself out of your own clan?\r\n", ch );
      return;
   }

   if( victim->pcdata->clan != ch->pcdata->clan )
   {
      send_to_char( "This player does not belong to your clan!\r\n", ch );
      return;
   }

   if( !can_outcast( clan, ch, victim ) )
   {
      send_to_char( "You aren't able to outcast them.\r\n", ch );
      return;
   }

   remove_clan_member( clan, victim->name );
   victim->pcdata->clan = NULL;
   act( AT_MAGIC, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );
   echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been outcast from %s!", victim->name, clan->name );
   save_char_obj( victim );
   save_clan( clan );
}

CMDF( do_nation_outcast )
{
   CHAR_DATA *victim;
   CLAN_DATA *nation;

   if( is_npc( ch ) || !ch->pcdata->clan )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   nation = ch->pcdata->nation;

   if( !is_name( "noutcast", ch->pcdata->bestowments )
   && str_cmp( ch->name, nation->leader ) && str_cmp( ch->name, nation->number1 ) && str_cmp( ch->name, nation->number2 ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Noutcast whom?\r\n", ch );
      return;
   }

   if( !str_cmp( ch->name, nation->leader ) )
      victim = get_char_world( ch, argument );
   else
      victim = get_char_room( ch, argument );

   if( !victim || is_npc( victim ) )
   {
      if( !victim && !str_cmp( ch->name, nation->leader ) && remove_clan_member( nation, argument ) )
      {
         send_to_char( "That player has been removed from your nation.\r\n", ch );
         echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been outcast from %s!", capitalize( argument ), nation->name );
         save_clan( nation );
      }
      else
         send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( victim == ch || victim->pcdata->nation != nation )
   {
      ch_printf( ch, "You can't kick %s out of your nation!\r\n", victim == ch ? "yourself" : "them" );
      return;
   }

   if( !can_outcast( nation, ch, victim ) )
   {
      send_to_char( "You aren't able to outcast them.\r\n", ch );
      return;
   }

   remove_clan_member( nation, victim->name );
   victim->pcdata->nation = NULL;
   act( AT_MAGIC, "You outcast $N from $t", ch, nation->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n outcasts $N from $t", ch, nation->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n outcasts you from $t", ch, nation->name, victim, TO_VICT );
   echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been outcast from %s!", victim->name, nation->name );
   save_char_obj( victim );
   save_clan( nation );
}

void show_clan( CHAR_DATA *ch, CLAN_DATA *clan )
{
   MEMBER_DATA *member;
   int cnt = 0;

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !clan )
   {
      send_to_char( "No such clan or nation.\r\n", ch );
      return;
   }

   ch_printf( ch, "\r\n&w%s    : &W%s\r\n&wBadge: &W%s\r\n&wFilename : &W%s\r\n&wMotto    : &W%s\r\n",
      clan->clan_type == CLAN_NATION ? "Nation" : "Clan ",
      clan->name ? clan->name : "(Not Set)",
      clan->badge ? clan->badge : "(Not Set)",
      clan->filename ? clan->filename : "(Not Set)",
      clan->motto ? clan->motto : "(Not Set)" );
   ch_printf( ch, "&wDesc     : &W%s\r\n",
      clan->description ? clan->description : "(Not Set)" );
   ch_printf( ch, "&wLeader   : &W%-19.19s   &wRank: &W%s\r\n",
      clan->leader ? clan->leader : "(Not Set)",
      clan->leadrank ? clan->leadrank : "(Not Set)" );
   ch_printf( ch, "&wNumber1  : &W%-19.19s   &wRank: &W%s\r\n",
      clan->number1 ? clan->number1 : "(Not Set)",
      clan->onerank ? clan->onerank : "(Not Set)" );
   ch_printf( ch, "&wNumber2  : &W%-19.19s   &wRank: &W%s\r\n",
      clan->number2 ? clan->number2 : "(Not Set)",
      clan->tworank ? clan->tworank : "(Not Set)" );
   ch_printf( ch, "&wMembers  : &W%-6d   ", clan->members );
   ch_printf( ch, "&wRace   : &W%d &w(&W%s&w)", clan->race, dis_race_name( clan->race ) );
   send_to_char( "\r\n", ch );
   ch_printf( ch, "&wRecall : &W%-5d\r\n", clan->recall );
   send_to_char( "Members:\r\n", ch );
   if( !clan->first_member )
      send_to_char( "  This clan currently has no members.\r\n", ch );
   for( member = clan->first_member; member; member = member->next )
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

CMDF( do_setclan )
{
   CLAN_DATA *clan;
   char arg1[MIL], arg2[MIL];

   set_char_color( AT_PLAIN, ch );
   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if(  arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: setclan <clan> [create/delete]\r\n", ch );
      send_to_char( "Usage: setclan <clan> [<field> <setting>]\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "  *desc    *motto   number2  *filename\r\n", ch );
      send_to_char( "  *name    leader   onerank   leadrank\r\n", ch );
      send_to_char( "  *race    recall   storage\r\n", ch );
      send_to_char( "  *type   number1   tworank\r\n", ch );
      send_to_char( "\r\n* = Only Leaders and higher can set.\r\n", ch );
      return;
   }

   clan = get_clan( arg1 );
   if( !str_cmp( arg2, "create" ) )
   {
      if( clan )
      {
         send_to_char( "There is already a clan with that name.\r\n", ch );
         return;
      }
      if( !( clan = new_clan( ) ) )
         return;
      LINK( clan, first_clan, last_clan, next, prev );
      clan->name = STRALLOC( arg1 );
      ch_printf( ch, "%s clan created.\r\n", clan->name );
      return;
   }

   if( !clan )
   {
      send_to_char( "No such clan.\r\n", ch );
      return;
   }

   if(  arg2 == NULL || arg2[0] == '\0' )
   {
      show_clan( ch, clan );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      delete_clan( clan );
      send_to_char( "The clan has been deleted.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "leader" ) )
   {
      STRSET( clan->leader, argument );
      save_clan( clan );
      if( !clan->leader )
         send_to_char( "Leader has been cleared.\r\n", ch );
      else
         send_to_char( "Leader Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "number1" ) )
   {
      STRSET( clan->number1, argument );
      save_clan( clan );
      if( !clan->number1 )
         send_to_char( "Number1 has been cleared.\r\n", ch );
      else
         send_to_char( "Number1 Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "number2" ) )
   {
      STRSET( clan->number2, argument );
      save_clan( clan );
      if( !clan->number2 )
         send_to_char( "Number2 has been cleared.\r\n", ch );
      else
         send_to_char( "Number2 Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "leadrank" ) )
   {
      STRSET( clan->leadrank, argument );
      save_clan( clan );
      if( !clan->leadrank )
         send_to_char( "Leadrank has been cleared.\r\n", ch );
      else
         send_to_char( "Leadrank Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "onerank" ) )
   {
      STRSET( clan->onerank, argument );
      save_clan( clan );
      if( !clan->onerank )
         send_to_char( "Onerank has been cleared.\r\n", ch );
      else
         send_to_char( "Onerank Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "tworank" ) )
   {
      STRSET( clan->tworank, argument );
      save_clan( clan );
      if( !clan->tworank )
         send_to_char( "Tworank has been cleared.\r\n", ch );
      else
         send_to_char( "Tworank Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "badge" ) )
   {
      STRSET( clan->badge, argument );
      save_clan( clan );
      if( !clan->badge )
         send_to_char( "Badge has been cleared.\r\n", ch );
      else
         send_to_char( "Badge Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "recall" ) )
   {
      clan->recall = UMAX( 0, atoi( argument ) );
      save_clan( clan );
      ch_printf( ch, "Recall set to %d.\r\n", clan->recall );
      return;
   }

   if( get_trust( ch ) < PERM_LEADER )
   {
      do_setclan( ch, (char *)"" );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !str_cmp( argument, "nation" ) )
         clan->clan_type = CLAN_NATION;
      else if( !str_cmp( argument, "clan" ) )
         clan->clan_type = CLAN_PLAIN;
      else if( is_number( argument ) )
      {
         int value = atoi( argument );

         if( value < CLAN_PLAIN || value > CLAN_NATION )
         {
            send_to_char( "Usage: setclan <clan> type <nation/clan>\r\n", ch );
            return;
         }
         clan->clan_type = value;
      }
      else
      {
         send_to_char( "Usage: setclan <clan> type <nation/clan>\r\n", ch );
         return;
      }
      save_clan( clan );
      ch_printf( ch, "Type has been set to %s.\r\n", clan->clan_type == CLAN_NATION ? "Nation" : "Clan" );
      return;
   }

   if( !str_cmp( arg2, "race" ) )
   {
      int value;

      if( is_number( argument ) )
         value = atoi( argument );
      else
         value = get_pc_race( argument );
      if( value < -1 || value >= MAX_PC_RACE )
      {
         send_to_char( "Invalid race.\r\n", ch );
         return;
      }
      clan->race = value;
      if( clan->race == -1 )
         send_to_char( "Race has been set to nothing.\r\n", ch );
      else
         ch_printf( ch, "Race set to %d[%s].\r\n", clan->race, dis_race_name( clan->race ) );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      CLAN_DATA *uclan = NULL;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't name a clan nothing.\r\n", ch );
         return;
      }
      if( ( uclan = get_clan( argument ) ) )
      {
         send_to_char( "There is already another clan with that name.\r\n", ch );
         return;
      }
      STRSET( clan->name, argument );
      send_to_char( "Name Set.\r\n", ch );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set a clans's filename to nothing.\r\n", ch );
         return;
      }
      if( !can_use_path( ch, CLAN_DIR, argument ) )
         return;
      if( clan->filename )
      {
         snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename );
         if( !remove( filename ) )
            send_to_char( "Old clan file deleted.\r\n", ch );
      }
      STRSET( clan->filename, argument );
      send_to_char( "Filename Set.\r\n", ch );
      save_clan( clan );
      write_clan_list( );
      return;
   }

   if( !str_cmp( arg2, "motto" ) )
   {
      STRSET( clan->motto, argument );
      save_clan( clan );
      if( !clan->motto )
         send_to_char( "Motto has been cleared.\r\n", ch );
      else
         send_to_char( "Motto Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      STRSET( clan->description, argument );
      save_clan( clan );
      if( !clan->description )
         send_to_char( "Description has been cleared.\r\n", ch );
      else
         send_to_char( "Description Set.\r\n", ch );
      return;
   }
   do_setclan( ch, (char *)"" );
}

void free_council_members( COUNCIL_DATA *council )
{
   MEMBER_DATA *member, *member_next;

   if( !council )
      return;

   for( member = council->first_member; member; member = member_next )
   {
      member_next = member->next;
      UNLINK( member, council->first_member, council->last_member, next, prev );
      free_member( member );
   }
}

bool remove_council_member( COUNCIL_DATA *council, const char *name )
{
   MEMBER_DATA *member;

   if( !council || !name || name[0] == '\0' )
      return false;

   for( member = council->first_member; member; member = member->next )
   {
      if( !str_cmp( member->name, name ) )
      {
         if( council->head && !str_cmp( council->head, name ) )
            STRFREE( council->head );
         if( council->head2 && !str_cmp( council->head2, name ) )
            STRFREE( council->head2 );
         UNLINK( member, council->first_member, council->last_member, next, prev );
         free_member( member );
         if( --council->members < 0 )
            council->members = 0;
         return true;
      }
   }

   return false;
}

bool rename_council_member( COUNCIL_DATA *council, const char *oname, const char *nname )
{
   MEMBER_DATA *member;

   if( !council || !oname || oname[0] == '\0' || !nname || nname[0] == '\0' )
      return false;

   for( member = council->first_member; member; member = member->next )
   {
      if( !str_cmp( member->name, oname ) )
      {
         if( council->head && !str_cmp( council->head, oname ) )
            STRSET( council->head, nname );
         if( council->head2 && !str_cmp( council->head2, oname ) )
            STRSET( council->head2, nname );
         STRSET( member->name, nname );
         save_council( council );
         return true;
      }
   }

   return false;
}

void add_council_member( COUNCIL_DATA *council, const char *name )
{
   MEMBER_DATA *member;

   if( !council || !name || name[0] == '\0' )
      return;
   CREATE( member, MEMBER_DATA, 1 );
   if( !member )
   {
      bug( "%s: member is NULL after CREATE for council %s name %s.", __FUNCTION__, council->name, name );
      return;
   }
   member->name = STRALLOC( name );
   LINK( member, council->first_member, council->last_member, next, prev );
   if( ( council->members + 1 ) > 0 )
      council->members++;
}

void free_one_council( COUNCIL_DATA *council )
{
   if( !council )
      return;
   free_council_members( council );
   UNLINK( council, first_council, last_council, next, prev );
   STRFREE( council->description );
   STRFREE( council->filename );
   STRFREE( council->head );
   STRFREE( council->head2 );
   STRFREE( council->name );
   STRFREE( council->powers );
   DISPOSE( council );
}

void delete_council( COUNCIL_DATA *council )
{
   PC_DATA *pc;

   if( !council )
      return;

   /* Remove the clan from pcdata */
   for( pc = first_pc; pc; pc = pc->next )
   {
      if( pc->council && pc->council == council )
      {
         pc->council = NULL;
         send_to_char( "The council you were in has been deleted.\r\n", pc->character );
         save_char_obj( pc->character );
      }
   }

   if( council->filename )
   {
      char filename[MSL];

      snprintf( filename, sizeof( filename ), "%s%s", COUNCIL_DIR, council->filename );
      remove( filename );
   }

   free_one_council( council );
   write_council_list( );
}

void free_councils( void )
{
   COUNCIL_DATA *council, *council_next;

   for( council = first_council; council; council = council_next )
   {
      council_next = council->next;
      free_one_council( council );
   }
}

COUNCIL_DATA *get_council( char *name )
{
   COUNCIL_DATA *council;

   if( !name )
      return NULL;

   for( council = first_council; council; council = council->next )
      if( council->name && !str_cmp( name, council->name ) )
         return council;
   return NULL;
}

void write_council_list( void )
{
   COUNCIL_DATA *tcouncil;
   FILE *fp;

   if( !( fp = fopen( COUNCIL_LIST, "w" ) ) )
   {
      bug( "%s: can't open %s for writing!", __FUNCTION__, COUNCIL_LIST );
      return;
   }
   for( tcouncil = first_council; tcouncil; tcouncil = tcouncil->next )
      fprintf( fp, "%s\n", tcouncil->filename );
   fprintf( fp, "$\n" );
   fclose( fp );
   fp = NULL;
}

/* Save a council's data to its data file */
void save_council( COUNCIL_DATA *council )
{
   FILE *fp;
   MEMBER_DATA *member;
   char filename[MIL];

   if( !council )
   {
      bug( "%s: NULL council!", __FUNCTION__ );
      return;
   }

   if( !council->name )
   {
      bug( "%s: NULL council->name", __FUNCTION__ );
      return;
   }

   if( !council->filename || council->filename[0] == '\0' )
   {
      bug( "%s: %s has no filename", __FUNCTION__, council->name );
      return;
   }

   snprintf( filename, sizeof( filename ), "%s%s", COUNCIL_DIR, council->filename );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      perror( filename );
      return;
   }
   fprintf( fp, "Name         %s~\n", council->name );
   fprintf( fp, "Filename     %s~\n", council->filename );
   if( council->description )
      fprintf( fp, "Description  %s~\n", council->description );
   if( council->head )
      fprintf( fp, "Head         %s~\n", council->head );
   if( council->head2 )
      fprintf( fp, "Head2        %s~\n", council->head2 );
   if( council->powers )
      fprintf( fp, "Powers       %s~\n", council->powers );
   for( member = council->first_member; member; member = member->next )
      fprintf( fp, "Member         %s~\n", member->name );
   fprintf( fp, "End\n\n" );
   fclose( fp );
   fp = NULL;
}

COUNCIL_DATA *new_council( void )
{
   COUNCIL_DATA *council = NULL;

   CREATE( council, COUNCIL_DATA, 1 );
   if( !council )
   {
      bug( "%s: council is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   council->name = NULL;
   council->filename = NULL;
   council->head = NULL;
   council->head2 = NULL;
   council->powers = NULL;
   council->first_member = council->last_member = NULL;
   council->members = 0;
   return council;
}

/* Read in actual council data. */
COUNCIL_DATA *fread_council( FILE *fp )
{
   COUNCIL_DATA *council = NULL;
   const char *word;
   bool fMatch;

   council = new_council( );

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

         case 'D':
            KEY( "Description", council->description, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return council;
            break;

         case 'F':
            KEY( "Filename", council->filename, fread_string( fp ) );
            break;

         case 'H':
            KEY( "Head", council->head, fread_string( fp ) );
            KEY( "Head2", council->head2, fread_string( fp ) );
            break;

         case 'M':
            if( !str_cmp( word, "Member" ) )
            {
               char *name = fread_flagstring( fp );

               if( valid_pfile( name ) )
                  add_council_member( council, name );
               else
                  bug( "%s: not adding member %s because no pfile found.", __FUNCTION__, name );
               fMatch = true;
               break;
            }
            break;

         case 'N':
            KEY( "Name", council->name, fread_string( fp ) );
            break;

         case 'P':
            KEY( "Powers", council->powers, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_one_council( council );
   return NULL;
}

/* Load a council file */
void load_council_file( const char *councilfile )
{
   COUNCIL_DATA *council;
   FILE *fp;
   char filename[MIL];

   snprintf( filename, sizeof( filename ), "%s%s", COUNCIL_DIR, councilfile );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      perror( filename );
      return;
   }
   council = fread_council( fp );
   fclose( fp );
   if( !council )
      return;
   LINK( council, first_council, last_council, next, prev );
}

/* Load in all the council files. */
void load_councils( void )
{
   FILE *fpList;
   const char *filename;

   first_council = last_council = NULL;

   log_string( "Loading councils..." );
   if( !( fpList = fopen( COUNCIL_LIST, "r" ) ) )
   {
      bug( "%s: Can't read file: %s", __FUNCTION__, COUNCIL_LIST );
      perror( COUNCIL_LIST );
      return;
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      if( filename[0] == '$' )
         break;
      log_string( filename );
      load_council_file( filename );
   }
   fclose( fpList );
   fpList = NULL;
   log_string( " Done loading councils " );
}

CMDF( do_council_induct )
{
   CHAR_DATA *victim;
   COUNCIL_DATA *council;
   char arg[MIL];

   if( is_npc( ch ) || !ch->pcdata->council )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   council = ch->pcdata->council;

   if( str_cmp( ch->name, council->head )
   && str_cmp( ch->name, council->head2 )
   && str_cmp( council->name, "mortal council" ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if(  arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Induct whom into your council?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   if( victim->pcdata->council )
   {
      send_to_char( "This player already belongs to a council!\r\n", ch );
      return;
   }

   if( xIS_SET( victim->act, PLR_NOINDUCT ) )
   {
      send_to_char( "This player doesn't currently wish to be inducted into a council.\r\n", ch );
      return;
   }

   victim->pcdata->council = council;
   add_council_member( council, victim->name );
   act( AT_MAGIC, "You induct $N into $t", ch, council->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n inducts $N into $t", ch, council->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n inducts you into $t", ch, council->name, victim, TO_VICT );
   echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been inducted into %s!", victim->name, council->name );
   save_char_obj( victim );
   save_council( council );
}

CMDF( do_council_outcast )
{
   CHAR_DATA *victim;
   COUNCIL_DATA *council;
   char arg[MIL];

   if( is_npc( ch ) || !ch->pcdata->council )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   council = ch->pcdata->council;

   if( str_cmp( ch->name, council->head ) && str_cmp( ch->name, council->head2 )
   && str_cmp( council->name, "mortal council" ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if(  arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Outcast whom from your council?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Kick yourself out of your own council?\r\n", ch );
      return;
   }

   if( victim->pcdata->council != ch->pcdata->council )
   {
      send_to_char( "This player does not belong to your council!\r\n", ch );
      return;
   }

   if( !str_cmp( victim->name, ch->pcdata->council->head2 ) )
   {
      STRFREE( ch->pcdata->council->head2 );
      ch->pcdata->council->head2 = NULL;
   }

   remove_council_member( council, victim->name );
   victim->pcdata->council = NULL;
   act( AT_MAGIC, "You outcast $N from $t", ch, council->name, victim, TO_CHAR );
   act( AT_MAGIC, "$n outcasts $N from $t", ch, council->name, victim, TO_ROOM );
   act( AT_MAGIC, "$n outcasts you from $t", ch, council->name, victim, TO_VICT );
   echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has been outcast from %s!", victim->name, council->name );
   save_char_obj( victim );
   save_council( council );
}

void show_council( CHAR_DATA *ch, COUNCIL_DATA *council )
{
   MEMBER_DATA *member;
   int cnt = 0;

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !council )
   {
      send_to_char( "No such council.\r\n", ch );
      return;
   }

   ch_printf( ch, "\r\n&wCouncil :  &W%s\r\n&wFilename:  &W%s\r\n",
      council->name ? council->name : "(Not Set)", council->filename ? council->filename : "(Not Set)" );
   ch_printf( ch, "&wHead:      &W%s\r\n", council->head ? council->head : "(Not Set)" );
   ch_printf( ch, "&wHead2:     &W%s\r\n", council->head2 ? council->head2 : "(Not Set)" );
   ch_printf( ch, "&wPowers:    &W%s\r\n", council->powers ? council->powers : "(Not Set)" );
   ch_printf( ch, "&wDescription:\r\n&W%s\r\n", council->description ? council->description : "(Not Set)" );
   send_to_char( "Members:\r\n", ch );
   if( !council->first_member )
      send_to_char( "  This council has no members.\r\n", ch );
   for( member = council->first_member; member; member = member->next )
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

CMDF( do_setcouncil )
{
   COUNCIL_DATA *council;
   char arg1[MIL], arg2[MIL];

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if(  arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: setcouncil <council> [create/delete]\r\n", ch );
      send_to_char( "Usage: setcouncil <council> <field> <value>\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "  head  head2  members", ch );
      if( get_trust( ch ) >= PERM_LEADER )
         send_to_char( "  name  filename  desc", ch );
      if( get_trust( ch ) >= PERM_HEAD )
         send_to_char( "  powers", ch );
      send_to_char( "\r\n", ch );
      return;
   }

   council = get_council( arg1 );

   if( !str_cmp( arg2, "create" ) )
   {
      if( council )
      {
         send_to_char( "A council is already using that name.\r\n", ch );
         return;
      }
      if( !( council = new_council( ) ) )
         return;
      LINK( council, first_council, last_council, next, prev );
      council->name = STRALLOC( arg1 );
      ch_printf( ch, "%s council created.\r\n", council->name );
      return;
   }

   if( !council )
   {
      send_to_char( "No such council.\r\n", ch );
      return;
   }

   if(  arg2 == NULL || arg2[0] == '\0' )
   {
      show_council( ch, council );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      delete_council( council );
      send_to_char( "The council has been deleted.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "head" ) )
   {
      STRSET( council->head, argument );
      send_to_char( "Head Set.\r\n", ch );
      save_council( council );
      return;
   }

   if( !str_cmp( arg2, "head2" ) )
   {
      STRSET( council->head2, argument );
      send_to_char( "Head2 Set.\r\n", ch );
      save_council( council );
      return;
   }
   if( !str_cmp( arg2, "members" ) )
   {
      council->members = UMAX( 0, atoi( argument ) );
      send_to_char( "Done.\r\n", ch );
      save_council( council );
      return;
   }
   if( get_trust( ch ) < PERM_LEADER )
   {
      do_setcouncil( ch, (char *)"" );
      return;
   }
   if( !str_cmp( arg2, "name" ) )
   {
      COUNCIL_DATA *ucouncil;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Can't set a council name to nothing.\r\n", ch );
         return;
      }
      if( ( ucouncil = get_council( argument ) ) )
      {
         send_to_char( "A council is already using that name.\r\n", ch );
         return;
      }
      STRSET( council->name, argument );
      send_to_char( "Name Set.\r\n", ch );
      save_council( council );
      return;
   }
   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set a council's filename to nothing.\r\n", ch );
         return;
      }
      if( !can_use_path( ch, COUNCIL_DIR, argument ) )
         return;
      if( council->filename )
      {
         snprintf( filename, sizeof( filename ), "%s%s", COUNCIL_DIR, council->filename );
         if( !remove( filename ) )
            send_to_char( "Old council file deleted.\r\n", ch );
      }
      STRSET( council->filename, argument );
      send_to_char( "Filename Set.\r\n", ch );
      save_council( council );
      write_council_list( );
      return;
   }
   if( !str_cmp( arg2, "desc" ) )
   {
      STRSET( council->description, argument );
      send_to_char( "Desc Set.\r\n", ch );
      save_council( council );
      return;
   }
   if( get_trust( ch ) < PERM_HEAD )
   {
      do_setcouncil( ch, (char *)"" );
      return;
   }
   if( !str_cmp( arg2, "powers" ) )
   {
      STRSET( council->powers, argument );
      send_to_char( "Powers Set.\r\n", ch );
      save_council( council );
      return;
   }

   do_setcouncil( ch, (char *)"" );
}

void quit_clan( CHAR_DATA *ch, CLAN_DATA *clan )
{
   if( !ch || is_npc( ch ) || !clan )
      return;
   if( ch->pcdata->clan == clan )
      ch->pcdata->clan = NULL;
   else if( ch->pcdata->nation == clan )
      ch->pcdata->nation = NULL;
   else
   {
      send_to_char( "You don't belong to that clan or nation.\r\n", ch );
      return;
   }
   remove_clan_member( clan, ch->name );
   act( AT_MAGIC, "You quit $t", ch, clan->name, NULL, TO_CHAR );
   act( AT_MAGIC, "$n quit $t", ch, clan->name, NULL, TO_ROOM );
   echo_to_all_printf( AT_MAGIC, ECHOTAR_ALL, "%s has quit %s!", ch->name, clan->name );
   save_char_obj( ch );
   save_clan( clan );
}

CMDF( do_clan_quit )
{
   if( !ch || is_npc( ch ) )
      return;
   if( !ch->pcdata->clan || ch->pcdata->clan->clan_type != CLAN_PLAIN )
   {
      send_to_char( "You aren't in a clan.\r\n", ch );
      return;
   }   
   if( !str_cmp( argument, "now" ) )
      quit_clan( ch, ch->pcdata->clan );
   else
      send_to_char( "Usage: cquit now\r\n", ch );
}

CMDF( do_nation_quit )
{
   if( !ch || is_npc( ch ) )
      return;
   if( !ch->pcdata->nation || ch->pcdata->nation->clan_type != CLAN_NATION )
   {
      send_to_char( "You aren't in a nation.\r\n", ch );
      return;
   }   
   if( !str_cmp( argument, "now" ) )
      quit_clan( ch, ch->pcdata->nation );
   else
      send_to_char( "Usage: nquit now\r\n", ch );
}

void show_clans( CHAR_DATA *ch, CLAN_DATA *clan, int type )
{
   MEMBER_DATA *member;
   const char *disname = "Clan", *argname = "clans", *oargname = "clan";
   int count = 0, color1 = AT_BLOOD, color2 = AT_GRAY;

   if( !ch )
      return;
   if( type == CLAN_NATION )
   {
      color1 = AT_YELLOW;
      color2 = AT_GREEN;
      disname = "Nation";
      oargname = "nation";
      argname = "nations";
   }
   if( !clan )
   {
      set_char_color( color1, ch );
      ch_printf( ch, "\r\n%-13s %-13s %-7s\r\n", disname, "Leader", "Members" );
      send_to_char( "________________________________________________\r\n\r\n", ch );
      set_char_color( color2, ch );
      for( clan = first_clan; clan; clan = clan->next )
      {
         if( clan->clan_type != type )
            continue;
         ch_printf( ch, "%-13s %-13s %7d\r\n",
            clan->name ? clan->name : "(Not Set)",
            clan->leader ? clan->leader : "(Not Set)", clan->members );
         count++;
      }
      if( !count )
         ch_printf( ch, "There are no %s currently formed.\r\n", argname );
      set_char_color( color1, ch );
      send_to_char( "________________________________________________\r\n\r\n", ch );
      ch_printf( ch, "Use '%s <%s>' for detailed information.\r\n", argname, oargname );
      return;
   }
   set_char_color( color1, ch );
   ch_printf( ch, "\r\n%s, '%s'\r\n\r\n", clan->name ? clan->name : "(Not Set)", clan->motto ? clan->motto : "(Not Set)" );
   set_char_color( color2, ch );
   ch_printf( ch, "Leader:  %s\r\nNumber One :  %s\r\nNumber Two :  %s\r\n",
      clan->leader ? clan->leader : "(Not Set)",
      clan->number1 ? clan->number1 : "(Not Set)",
      clan->number2 ? clan->number2 : "(Not Set)" );

   ch_printf( ch, "Members    :  %d\r\n", clan->members );
   set_char_color( color1, ch );
   ch_printf( ch, "\r\nDescription:  %s\r\n", clan->description ? clan->description : "(Not Set)" );
   send_to_char( "Members:\r\n", ch );
   set_char_color( color2, ch );
   for( member = clan->first_member; member; member = member->next )
   {
      ch_printf( ch, "  %15.15s", member->name );
      if( ++count == 4 )
      {
         count = 0;
         send_to_char( "\r\n", ch );
      }
   }
   if( count != 0 )
      send_to_char( "\r\n", ch );
}

/* Added multiple level pkill and pdeath support. --Shaddai */
CMDF( do_clans )
{
   CLAN_DATA *clan;

   if( !argument || argument[0] == '\0' )
   {
      show_clans( ch, NULL, CLAN_PLAIN );
      return;
   }

   if( !( clan = get_clan( argument ) ) || clan->clan_type != CLAN_PLAIN )
   {
      set_char_color( AT_BLOOD, ch );
      ch_printf( ch, "No clan called %s.\r\n", argument );
      return;
   }
   show_clans( ch, clan, CLAN_PLAIN );
}

CMDF( do_nations )
{
   CLAN_DATA *nation;

   if( !argument || argument[0] == '\0' )
   {
      show_clans( ch, NULL, CLAN_NATION );
      return;
   }

   if( !( nation = get_clan( argument ) ) || nation->clan_type != CLAN_NATION )
   {
      set_char_color( AT_YELLOW, ch );
      ch_printf( ch, "No nation called %s.\r\n", argument );
      return;
   }
   show_clans( ch, nation, CLAN_NATION );
}

CMDF( do_councils )
{
   COUNCIL_DATA *council;
   int count = 0;

   set_char_color( AT_CYAN, ch );
   if( !first_council )
   {
      send_to_char( "There are no councils currently formed.\r\n", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      set_char_color( AT_CYAN, ch );
      send_to_char( "\r\nTitle                    Head\r\n", ch );
      send_to_char( "__________________________________________________\r\n\r\n", ch );
      set_char_color( AT_GRAY, ch );
      for( council = first_council; council; council = council->next )
      {
         ch_printf( ch, "&w%-24s", council->name ? council->name : "(Not Set)" );
         ch_printf( ch, " %s", council->head ? council->head : "(Not Set)" );
         if( council->head2 )
            ch_printf( ch, " and %s", council->head2 );
         send_to_char( "\r\n", ch );
         count++;
      }
      if( !count )
         send_to_char( "There are no councils currently formed.\r\n", ch );
      set_char_color( AT_CYAN, ch );
      send_to_char( "__________________________________________________\r\n\r\n", ch );
      send_to_char( "Use 'councils <council>' for detailed information.\r\n", ch );
      return;
   }
   if( !( council = get_council( argument ) ) )
   {
      ch_printf( ch, "&cNo council called %s exists...\r\n", argument );
      return;
   }
   ch_printf( ch, "&c\r\n%s\r\n", council->name ? council->name : "(Not Set)" );
   ch_printf( ch, "&cHead:    &w%s\r\n", council->head ? council->head : "(Not Set)" );
   if( council->head2 )
      ch_printf( ch, "&cCo-Head: &w%s\r\n", council->head2 );

   ch_printf( ch, "&cMembers:  &w%d\r\n", council->members );
   ch_printf( ch, "&cDescription:\r\n&w%s\r\n", council->description ? council->description : "(Not Set)" );
}

CMDF( do_victories )
{
   CLAN_DATA *clan;
   VICTORY_DATA *victory;
   char arg[MSL];
   bool clear = false;

   if( !ch || is_npc( ch ) )
      return;

   argument = one_argument( argument, arg );
   if( arg != NULL && arg[0] != '\0' )
   {
      if( ( clan = get_clan( arg ) ) )
         argument = one_argument( argument, arg );
   }
   else
      clan = ch->pcdata->clan;

   if( is_immortal( ch ) && arg != NULL && arg[0] != '\0' && !str_cmp( arg, "clear" ) )
      clear = true;

   if( !clan )
   {
      send_to_char( "No clan to display victories for.\r\n", ch );
      return;
   }

   if( clan->clan_type != CLAN_PLAIN )
   {
      send_to_char( "The specified clan isn't able to legaly pkill others.\r\n", ch );
      return;
   }

   if( clear )
   {
      free_clan_victories( clan );
      save_clan( clan );
      ch_printf( ch, "%s victories has been cleared.\r\n", clan->name );
      return;
   }

   if( !clan->first_victory )
   {
      send_to_char( "The specified clan doesn't have any victories.\r\n", ch );
      return;
   }

   for( victory = clan->first_victory; victory; victory = victory->next )
   {
      pager_printf( ch, "[%d]%s has killed [%d]%s %d time%s, last time was %s\r\n", victory->level, victory->name, victory->vlevel, victory->vname,
         victory->vkills, victory->vkills != 1 ? "s" : "", distime( victory->vtime ) );
   }
}

CMDF( do_shove )
{
   EXIT_DATA *pexit;
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *to_room;
   char arg[MIL], arg2[MIL];
   int exit_dir, schance = 0;
   bool nogo;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( is_npc( ch ) || !xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
   {
      send_to_char( "Only deadly characters can shove.\r\n", ch );
      return;
   }

   if( get_timer( ch, TIMER_PKILLED ) > 0 )
   {
      send_to_char( "You can't shove a player right now.\r\n", ch );
      return;
   }

   if(  arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Shove whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You shove yourself around, to no avail.\r\n", ch );
      return;
   }
   if( is_npc( victim ) || !xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
   {
      send_to_char( "You can only shove deadly characters.\r\n", ch );
      return;
   }

   if( ch->level - victim->level > 5 || victim->level - ch->level > 5 )
   {
      send_to_char( "There is too great an experience difference for you to even bother.\r\n", ch );
      return;
   }

   if( get_timer( victim, TIMER_PKILLED ) > 0 )
   {
      send_to_char( "You can't shove that player right now.\r\n", ch );
      return;
   }

   if( victim->position != POS_STANDING )
   {
      act( AT_PLAIN, "$N isn't standing up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if(  arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Shove them in which direction?\r\n", ch );
      return;
   }

   exit_dir = get_dir( arg2 );
   if( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 )
   {
      send_to_char( "That character can't be shoved right now.\r\n", ch );
      return;
   }
   victim->position = POS_SHOVE;
   nogo = false;
   if( !( pexit = get_exit( ch->in_room, exit_dir ) ) )
      nogo = true;
   else if( xIS_SET( pexit->exit_info, EX_CLOSED )
   && ( !IS_AFFECTED( victim, AFF_PASS_DOOR )
   || xIS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
      nogo = true;
   if( nogo )
   {
      send_to_char( "There's no exit in that direction.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }
   to_room = pexit->to_room;
   if( xIS_SET( to_room->room_flags, ROOM_DEATH ) )
   {
      send_to_char( "You can't shove someone into a death trap.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }

   if( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) )
   {
      send_to_char( "That character can't enter that area.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }

   /* Add 3 points to chance for every str point above 15, subtract for below 15 */
   schance += ( ( get_curr_str( ch ) - 15 ) * 3 );

   schance += ( ch->level - victim->level );

   if( schance < number_percent( ) )
   {
      send_to_char( "You failed.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }
   act( AT_ACTION, "You shove $M.", ch, NULL, victim, TO_CHAR );
   act( AT_ACTION, "$n shoves you.", ch, NULL, victim, TO_VICT );
   move_char( victim, get_exit( ch->in_room, exit_dir ), 0, false );
   if( !char_died( victim ) )
      victim->position = POS_STANDING;
   wait_state( ch, 12 );
   /* Remove protection from shove/drag if char shoves -- Blodkai */
   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) && get_timer( ch, TIMER_SHOVEDRAG ) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
}

CMDF( do_drag )
{
   CHAR_DATA *victim;
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *to_room;
   char arg[MIL], arg2[MIL];
   int exit_dir, schance = 0;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( is_npc( ch ) )
   {
      send_to_char( "Only characters can drag.\r\n", ch );
      return;
   }

   if( get_timer( ch, TIMER_PKILLED ) > 0 )
   {
      send_to_char( "You can't drag a player right now.\r\n", ch );
      return;
   }

   if(  arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Drag whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You take yourself by the scruff of your neck, but go nowhere.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "You can only drag characters.\r\n", ch );
      return;
   }

   if( !xIS_SET( victim->act, PLR_SHOVEDRAG ) && !xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
   {
      send_to_char( "That character doesn't seem to appreciate your attentions.\r\n", ch );
      return;
   }

   if( get_timer( victim, TIMER_PKILLED ) > 0 )
   {
      send_to_char( "You can't drag that player right now.\r\n", ch );
      return;
   }

   if( victim->fighting )
   {
      send_to_char( "You try, but can't get close enough.\r\n", ch );
      return;
   }

   if( !xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) && xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
   {
      send_to_char( "You can't drag a deadly character.\r\n", ch );
      return;
   }

   if( !xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) && victim->position > 3 )
   {
      send_to_char( "They don't seem to need your assistance.\r\n", ch );
      return;
   }

   if(  arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Drag them in which direction?\r\n", ch );
      return;
   }

   if( ch->level - victim->level > 5 || victim->level - ch->level > 5 )
   {
      if( xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) && xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
      {
         send_to_char( "There is too great an experience difference for you to even bother.\r\n", ch );
         return;
      }
   }

   if( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 )
   {
      send_to_char( "That character can't be dragged right now.\r\n", ch );
      return;
   }

   exit_dir = get_dir( arg2 );

   if( !( pexit = get_exit( ch->in_room, exit_dir ) ) || xIS_SET( pexit->exit_info, EX_HIDDEN ) )
   {
      send_to_char( "There's no exit in that direction.\r\n", ch );
      return;
   }

   if( xIS_SET( pexit->exit_info, EX_CLOSED )
   && ( !IS_AFFECTED( victim, AFF_PASS_DOOR ) || xIS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
   {
      send_to_char( "That exit is closed.\r\n", ch );
      return;
   }

   to_room = pexit->to_room;
   if( xIS_SET( to_room->room_flags, ROOM_DEATH ) )
   {
      send_to_char( "You can't drag someone into a death trap.\r\n", ch );
      return;
   }

   if( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) )
   {
      send_to_char( "That character can't enter that area.\r\n", ch );
      return;
   }

   /* Add 3 points to chance for every str point above 15, subtract for below 15 */
   schance += ( ( get_curr_str( ch ) - 15 ) * 3 );
   schance += ( ch->level - victim->level );

   if( schance < number_percent( ) )
   {
      send_to_char( "You failed.\r\n", ch );
      victim->position = POS_STANDING;
      return;
   }
   if( victim->position < POS_STANDING )
   {
      short temp;

      temp = victim->position;
      victim->position = POS_DRAG;
      act( AT_ACTION, "You drag $M into the next room.", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$n grabs your hair and drags you.", ch, NULL, victim, TO_VICT );
      move_char( victim, get_exit( ch->in_room, exit_dir ), 0, false );
      if( !char_died( victim ) )
         victim->position = temp;
      move_char( ch, get_exit( ch->in_room, exit_dir ), 0, false );
      wait_state( ch, 12 );
      return;
   }
   send_to_char( "You can't do that to someone who is standing.\r\n", ch );
}
