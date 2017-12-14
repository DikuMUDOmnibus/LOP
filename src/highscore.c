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
 *                              High Scores                                  *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "h/mud.h"

#define HIGHSCORE_FILE SYSTEM_DIR "highscores.dat"
typedef struct highscore_data HIGHSCORE_DATA;
struct highscore_data
{
   HIGHSCORE_DATA *next, *prev;
   char *name;
   int value;
   int pos;
};

typedef struct hightable_data HIGHTABLE_DATA;
struct hightable_data
{
   HIGHTABLE_DATA *next, *prev;
   HIGHSCORE_DATA *first_highscore, *last_highscore;
   char *name;
   int max;
   bool reversed; /* Should it be a reversed table - Lowest is best */
};

HIGHTABLE_DATA *first_hightable, *last_hightable;

short show_highscore_count( HIGHTABLE_DATA *table )
{
   HIGHSCORE_DATA *score;
   short count = 0;

   if( table )
      for( score = table->first_highscore; score; score = score->next )
         count++;
   return count;
}

void save_highscore( void )
{
   HIGHTABLE_DATA *table;
   HIGHSCORE_DATA *score;
   FILE *fp;

   if( !first_hightable )
   {
      remove_file( HIGHSCORE_FILE );
      return;
   }
   if( !( fp = fopen( HIGHSCORE_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, HIGHSCORE_FILE );
      perror( HIGHSCORE_FILE );
      return;
   }

   for( table = first_hightable; table; table = table->next )
   {
      if( !table->name )
         continue;
      fprintf( fp, "Name      %s~\n", table->name );
      fprintf( fp, "Max       %d\n", table->max );
      if( table->reversed )
         fprintf( fp, "%s\n", "Reversed" );
      for( score = table->first_highscore; score; score = score->next )
         fprintf( fp, "HighScore %s~ %d\r\n", score->name, score->value );
   }
   fprintf( fp, "%s", "End\n" );
   fclose( fp );
   fp = NULL;
}

void free_highscore( HIGHSCORE_DATA *score )
{
   if( !score )
      return;
   STRFREE( score->name );
   DISPOSE( score );
}

void free_hightable( HIGHTABLE_DATA *table )
{
   HIGHSCORE_DATA *score, *score_next;

   if( !table )
      return;
   STRFREE( table->name );
   for( score = table->first_highscore; score; score = score_next )
   {
      score_next = score->next;
      UNLINK( score, table->first_highscore, table->last_highscore, next, prev );
      free_highscore( score );
   }
   DISPOSE( table );
}

void free_hightables( void )
{
   HIGHTABLE_DATA *table, *table_next;

   for( table = first_hightable; table; table = table_next )
   {
      table_next = table->next;
      UNLINK( table, first_hightable, last_hightable, next, prev );
      free_hightable( table );
   }
}

void trim_highscore( HIGHTABLE_DATA *table )
{
   HIGHSCORE_DATA *score, *score_next;
   int count = 0;

   for( score = table->first_highscore; score; score = score_next )
   {
      score_next = score->next;
      score->pos = ++count;
      if( count >= table->max )
      {
         UNLINK( score, table->first_highscore, table->last_highscore, next, prev );
         free_highscore( score );
      }
   }
}

void link_highscore( HIGHTABLE_DATA *table, HIGHSCORE_DATA *nscore, bool silent )
{
   HIGHSCORE_DATA *score, *score_next;
   char buf[MSL];
   int count = 0;

   for( score = table->first_highscore; score; score = score_next )
   {
      score_next = score->next;
      ++count;

      if( ( table->reversed && nscore->value < score->value )
      || ( !table->reversed && nscore->value > score->value ) )
      {
         if( !silent && nscore->pos > count )
         {
            snprintf( buf, sizeof( buf ), "%s has obtained position %d on highscore table %s.",
               nscore->name, count, table->name );
            to_channel( buf, "highscore", PERM_ALL );
         }
         else if( !silent && nscore->pos < count )
         {
            snprintf( buf, sizeof( buf ), "%s has dropped to position %d on highscore table %s.",
               nscore->name, count, table->name );
            to_channel( buf, "highscore", PERM_ALL );
         }
         INSERT( nscore, score, table->first_highscore, next, prev );
         trim_highscore( table );
         return;
      }
   }
   if( ++count < table->max )
   {
      if( !silent && nscore->pos > count )
      {
         snprintf( buf, sizeof( buf ), "%s has obtained position %d on highscore table %s.",
            nscore->name, count, table->name );
         to_channel( buf, "highscore", PERM_ALL );
      }
      else if( !silent && nscore->pos < count )
      {
         snprintf( buf, sizeof( buf ), "%s has dropped to position %d on highscore table %s.",
            nscore->name, count, table->name );
         to_channel( buf, "highscore", PERM_ALL );
      }
      LINK( nscore, table->first_highscore, table->last_highscore, next, prev );
      trim_highscore( table );
      return;
   }
   free_highscore( nscore );
   trim_highscore( table );
}

HIGHTABLE_DATA *add_hightable( char *name )
{
   HIGHTABLE_DATA *table;

   if( !name || name[0] == '\0' )
      return NULL;

   CREATE( table, HIGHTABLE_DATA, 1 );
   STRSET( table->name, name );
   table->first_highscore = NULL;
   table->last_highscore = NULL;
   table->max = 20;
   table->reversed = false;
   LINK( table, first_hightable, last_hightable, next, prev );
   return table;
}

void add_highscore( HIGHTABLE_DATA *table, char *name, int value, int silent )
{
   HIGHSCORE_DATA *score;

   if( !table || !name )
      return;
   CREATE( score, HIGHSCORE_DATA, 1 );
   STRSET( score->name, name );
   score->value = value;
   score->pos = ( table->max + 1 );
   link_highscore( table, score, silent );
}

void load_highscores( void )
{
   FILE *fp;
   HIGHTABLE_DATA *table = NULL;
   char *name, *sname;
   int value;
   bool fMatch;

   first_hightable = last_hightable = NULL;
   if( !( fp = fopen( HIGHSCORE_FILE, "r" ) ) )
      return;
   for( ;; )
   {
      char *word;

      fMatch = false;
      if( feof( fp ) )
         break;
      word = fread_word( fp );
      if( word[0] == EOF )
         break;

      switch( UPPER( word[0] ) )
      {
         default:
            break;

         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               fclose( fp );
               fp = NULL;
               return;
            }
            break;

         case 'H':
            if( !str_cmp( word, "HighScore" ) )
            {
               sname = fread_flagstring( fp );
               value = fread_number( fp );
               if( table && valid_pfile( sname ) )
                  add_highscore( table, sname, value, true );
               fMatch = true;
               break;
            }
            break;

         case 'M':
            if( !str_cmp( word, "Max" ) )
            {
               value = fread_number( fp );
               if( table )
                  table->max = value;
               fMatch = true;
               break;
            }
            break;

         case 'N':
            if( !str_cmp( word, "Name" ) )
            {
               name = fread_flagstring( fp );
               if( ( table = add_hightable( name ) ) )
               {
                  table->max = 20;
                  table->reversed = false;
               }
               fMatch = true;
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "Reversed" ) )
            {
               if( table )
                  table->reversed = true;
            }
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   fclose( fp );
   fp = NULL;
}

void remove_from_highscores( const char *name )
{
   HIGHTABLE_DATA *table;
   HIGHSCORE_DATA *score, *score_next = NULL;

   for( table = first_hightable; table; table = table->next )
   {
      for( score = table->first_highscore; score; score = score_next )
      {
         score_next = score->next;

         if( str_cmp( score->name, name ) )
            continue;
         UNLINK( score, table->first_highscore, table->last_highscore, next, prev );
         free_highscore( score );
      }
   }
}

HIGHTABLE_DATA *find_hightable( const char *name )
{
   HIGHTABLE_DATA *table;

   for( table = first_hightable; table; table = table->next )
      if( !str_cmp( table->name, name ) )
         return table;
   return NULL;
}

HIGHSCORE_DATA *find_highscore( HIGHTABLE_DATA *table, const char *name )
{
   HIGHSCORE_DATA *score;

   for( score = table->first_highscore; score; score = score->next )
      if( !str_cmp( score->name, name ) )
         return score;
   return NULL;
}

void check_highscores( HIGHTABLE_DATA *table, HIGHSCORE_DATA *score )
{
   if( !table || !score )
      return;

   if( ( score->prev && score->value > score->prev->value )
   || ( score->next && score->value < score->next->value ) )
   {
      UNLINK( score, table->first_highscore, table->last_highscore, next, prev );
      link_highscore( table, score, false );
   }
}

void update_highscore( CHAR_DATA *ch )
{
   HIGHTABLE_DATA *table;
   HIGHSCORE_DATA *score;

   if( !ch || is_npc( ch ) || is_immortal( ch ) )
      return;
   if( ( table = find_hightable( "Gold" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->gold;
         check_highscores( table, score );
      }
      else if( ch->gold > 0 )
         add_highscore( table, ch->name, ch->gold, false );
   }
   if( ( table = find_hightable( "SudokuWins" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->pcdata->swins;
         check_highscores( table, score );
      }
      else if( ch->pcdata->swins > 0 )
         add_highscore( table, ch->name, ch->pcdata->swins, false );
   }
   if( ( table = find_hightable( "RubiksFinished" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->pcdata->rwins;
         check_highscores( table, score );
      }
      else if( ch->pcdata->rwins > 0 )
         add_highscore( table, ch->name, ch->pcdata->rwins, false );
   }
   if( ( table = find_hightable( "MKills" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->pcdata->mkills;
         check_highscores( table, score );
      }
      else if( ch->pcdata->mkills > 0 )
         add_highscore( table, ch->name, ch->pcdata->mkills, false );
   }
   if( ( table = find_hightable( "MDeaths" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->pcdata->mdeaths;
         check_highscores( table, score );
      }
      else if( ch->pcdata->mdeaths > 0 )
         add_highscore( table, ch->name, ch->pcdata->mdeaths, false );
   }
   if( ( table = find_hightable( "PKills" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->pcdata->pkills;
         check_highscores( table, score );
      }
      else if( ch->pcdata->pkills > 0 )
         add_highscore( table, ch->name, ch->pcdata->pkills, false );
   }
   if( ( table = find_hightable( "PDeaths" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->pcdata->pdeaths;
         check_highscores( table, score );
      }
      else if( ch->pcdata->pdeaths > 0 )
         add_highscore( table, ch->name, ch->pcdata->pdeaths, false );
   }
   if( ( table = find_hightable( "QCompleted" ) ) )
   {
      if( ( score = find_highscore( table, ch->name ) ) )
      {
         score->value = ch->pcdata->questcompleted;
         check_highscores( table, score );
      }
      else if( ch->pcdata->questcompleted > 0 )
         add_highscore( table, ch->name, ch->pcdata->questcompleted, false );
   }
   save_highscore( );
}

CMDF( do_highscore )
{
   HIGHSCORE_DATA *score;
   HIGHTABLE_DATA *table;
   int cnt = 0, count = 0;

   set_char_color( AT_HIGHSCORE, ch );
   if( !argument || argument[0] == '\0' )
   {
      for( table = first_hightable; table; table = table->next )
      {
         count++;
         ch_printf( ch, "&[hscore2]%20s&[hscore][&[hscore2]%2d&[hscore]]", table->name, show_highscore_count( table ) );
         if( ++cnt >= 3 )
         {
            cnt = 0;
            send_to_char( "\r\n", ch );
         }
      }
      if( cnt != 0 )
         send_to_char( "\r\n", ch );
      if( !count )
         send_to_char( "There are currently no highscore tables.\r\n", ch );
      return;
   }
   if( !( table = find_hightable( argument ) ) )
   {
      send_to_char( "No such highscore table to look at.\r\n", ch );
      send_to_char( "Checking highscore table to see if it matches someone in the lists.\r\n", ch );
      count = 0;
      for( table = first_hightable; table; table = table->next )
      {
         cnt = 0;
         for( score = table->first_highscore; score; score = score->next )
         {
            cnt++;
            if( str_cmp( score->name, argument ) )
               continue;
            count++;
            ch_printf( ch, "&[hscore2]%s &[hscore]is in &[hscore2]%3d &[hscore]on highscore table &[hscore2]%s&[hscore].\r\n", score->name, cnt, table->name );
         }
      }
      if( !count )
          send_to_char( "Sorry it doesn't look like anyone matching that name is on the list.\r\n", ch );
      return;
   }

   ch_printf( ch, "&[hscore]HighScore Table: &[hscore2]%s&[hscore].\r\n", table->name );

   for( score = table->first_highscore; score; score = score->next )
      ch_printf( ch, "&[hscore2]%3d&[hscore]> &[hscore2]%20s %10d\r\n", ++cnt, score->name, score->value );

   if( !cnt )
      send_to_char( "No one is currently on this highscore table.\r\n", ch );
}

CMDF( do_clearhightable )
{
   HIGHTABLE_DATA *table;
   HIGHSCORE_DATA *score, *score_next;

   set_char_color( AT_HIGHSCORE, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: clearhightable <name>\r\n", ch );
      return;
   }
   if( !( table = find_hightable( argument ) ) )
   {
      send_to_char( "No such highscore table to clear.\r\n", ch );
      return;
   }

   for( score = table->first_highscore; score; score = score_next )
   {
      score_next = score->next;
      UNLINK( score, table->first_highscore, table->last_highscore, next, prev );
      free_highscore( score );
   }

   save_highscore( );

   send_to_char( "HighScore table cleared.\r\n", ch );
}

CMDF( do_makehightable )
{
   HIGHTABLE_DATA *table;

   set_char_color( AT_HIGHSCORE, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makehightable <name>\r\n", ch );
      return;
   }

   if( ( table = find_hightable( argument ) ) )
   {
      send_to_char( "There is already a highscore table with that name.\r\n", ch );
      return;
   }

   if( !( table = add_hightable( argument ) ) )
   {
      send_to_char( "HighScore table wasn't created.\r\n", ch );
      return;
   }

   save_highscore( );

   send_to_char( "HighScore table created and highscore tables saved.\r\n", ch );
}

CMDF( do_sethightable )
{
   HIGHTABLE_DATA *table, *new_table;
   char arg[MIL];

   set_char_color( AT_HIGHSCORE, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: sethightable <table> max/name <setting>\r\n", ch );
      send_to_char( "Usage: sethightable <table> reversed\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( table = find_hightable( arg ) ) )
   {
      send_to_char( "No such highscore table to set.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "max" ) )
   {
      short value;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You need to specify a value between 1 and 100.\r\n", ch );
         return;
      }
      value = atoi( argument );
      if( value < 1 || value > 100 )
      {
         send_to_char( "You need to specify a value between 1 and 100.\r\n", ch );
         return;
      }
      table->max = URANGE( 1, atoi( argument ), 100 );
      save_highscore( );
      ch_printf( ch, "The max for that table has been set to %d.\r\n", table->max );
      return;
   }

   if( !str_cmp( arg, "reversed" ) )
   {
     table->reversed = !table->reversed;
     save_highscore( );
     if( table->reversed )
        send_to_char( "That table will use reversed linking now.\r\n", ch );
     else
        send_to_char( "That table will use normal linking now.\r\n", ch );
     return;
   }

   if( !str_cmp( arg, "name" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set a hightable name to nothing.\r\n", ch );
         return;
      }
      if( ( new_table = find_hightable( argument ) ) )
      {
         send_to_char( "There is already a hightable useing that name.\r\n", ch );
         return;
      }
      STRSET( table->name, argument );
      save_highscore( );
      send_to_char( "The hightable name has been changed.\r\n", ch );
      return;
   }
}

CMDF( do_deletehightable )
{
   HIGHTABLE_DATA *table;

   set_char_color( AT_HIGHSCORE, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: deletehightable <table>\r\n", ch );
      return;
   }

   if( !( table = find_hightable( argument ) ) )
   {
      send_to_char( "No such highscore table to delete.\r\n", ch );
      return;
   }

   UNLINK( table, first_hightable, last_hightable, next, prev );
   free_hightable( table );

   save_highscore( );

   send_to_char( "HighScore table deleted.\r\n", ch );
}
