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

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "h/mud.h"
#include "h/news.h"

NEWS_DATA *first_news, *last_news;
int top_news;

void free_news( NEWS_DATA *news )
{
   STRFREE( news->text );
   STRFREE( news->poster );
   STRFREE( news->modified_by );
   DISPOSE( news );
}

void remove_news( NEWS_DATA *news )
{
   UNLINK( news, first_news, last_news, next, prev );
   top_news--;
}

void free_all_news( void )
{
   NEWS_DATA *news, *news_next;

   for( news = first_news; news; news = news_next )
   {
      news_next = news->next;
      remove_news( news );
      free_news( news );
   }
}

void add_news( NEWS_DATA *news )
{
   LINK( news, first_news, last_news, next, prev );
   top_news++;
}

/* Check to see if argument matches date of news in some way */
bool check_date( NEWS_DATA *news, char *argument )
{
   char buf[MIL];
   struct tm *time;
   int check = 0, day = 0, month = 0, year = 0;

   if( !news )
      return false;

   time = localtime( &news->added );

   day = time->tm_mday;
   month = ( time->tm_mon + 1 );
   year = ( time->tm_year + 1900 );

   /* Full date */
   snprintf( buf, sizeof( buf ), "%d/%d/%d", month, day, year );
   if( !str_cmp( buf, argument ) )
      return true;

   /* Month and day */
   snprintf( buf, sizeof( buf ), "%d/%d", month, day );
   if( !str_cmp( buf, argument ) )
      return true;

   /* Month and year */
   snprintf( buf, sizeof( buf ), "%d/%d", month, year );
   if( !str_cmp( buf, argument ) )
      return true;

   /* Day and Year */
   snprintf( buf, sizeof( buf ), "%d/%d", day, year );
   if( !str_cmp( buf, argument ) )
      return true;

   /* Month, day, or year */
   check = atoi( argument );
   if( check == month || check == day || check == year )
      return true;

   return false;
}

void save_news( void )
{
   NEWS_DATA *news;
   FILE *fp;

   if( !first_news )
   {
      remove_file( NEWS_FILE );
      return;
   }

   if( !( fp = fopen( NEWS_FILE, "w" ) ) )
      return;
   for( news = first_news; news; news = news->next )
   {
      fprintf( fp, "%s", "#NEWS\n" );
      if( news->poster )
         fprintf( fp, "Poster     %s~\n", news->poster );
      fprintf( fp, "Perm       %s~\n", perms_flag[news->level] );
      fprintf( fp, "Time       %ld\n", news->added );
      if( news->modified )
         fprintf( fp, "Modified   %ld\n", news->modified );
      if( news->modified_by )
         fprintf( fp, "ModifiedBy %s~\n", news->modified_by );
      if( news->text )
         fprintf( fp, "Text       %s~\n", strip_cr( news->text ) );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

NEWS_DATA *new_news( void )
{
   NEWS_DATA *news = NULL;

   CREATE( news, NEWS_DATA, 1 );
   if( !news )
   {
      bug( "%s: news is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }

   news->modified_by = NULL;
   news->poster = NULL;
   news->text = NULL;
   news->modified = 0;
   news->added = 0;
   news->level = PERM_ALL;
   return news;
}

void fread_news( FILE *fp )
{
   NEWS_DATA *news;
   const char *word;
   char *infoflags, flag[MSL];
   int value;
   bool fMatch;

   news = new_news( );

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

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               add_news( news );
               return;
	    }
	    break;

         case 'L':
            KEY( "Level", news->level, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Modified", news->modified, fread_time( fp ) );
            KEY( "ModifiedBy", news->modified_by, fread_string( fp ) );
            break;

         case 'P':
            KEY( "Poster", news->poster, fread_string( fp ) );
            SKEY( "Perm", news->level, fp, perms_flag, PERM_MAX );
            break;

         case 'T':
            KEY( "Text", news->text, fread_string( fp ) );
            KEY( "Time", news->added, fread_time( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_news( news );
}

void load_news( void )
{
   FILE *fp;

   top_news = 0;
   first_news = last_news = NULL;

   if( !( fp = fopen( NEWS_FILE, "r" ) ) )
      return;
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
         bug( "%s: # not found.", __FUNCTION__ );
         break;
      }
      word = fread_word( fp );
      if( !str_cmp( word, "NEWS" ) )
      {
         fread_news( fp );
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

/* Find a specif news entry using a number */
NEWS_DATA *find_news( CHAR_DATA *ch, int num )
{
   NEWS_DATA *news;
   int cnt = 0;

   if( !ch )
      return NULL;
   for( news = first_news; news; news = news->next )
   {
      if( get_trust( ch ) < news->level )
         continue;
      if( ++cnt == num )
         return news;
   }
   return NULL;
}

void show_news( CHAR_DATA *ch, NEWS_DATA *news, int count )
{
   ch_printf( ch, "&C[&W%4d&C] &CPosted by &W%s on %s\r\n", count, news->poster, distime( news->added ) );
   if( news->modified )
      ch_printf( ch, "&CLast modified by &W%s on %s\r\n", news->modified_by ? news->modified_by : "(Unknown)", distime( news->modified ) );
   ch_printf( ch, "&W%s&D\r\n", news->text ? news->text : "(Nothing set for the text)" );
}

CMDF( do_news )
{
   NEWS_DATA *news;
   char arg[MIL];
   int count = 0, displayed = 0;
   bool udate = false;

   if( is_npc( ch ) )
   {
      send_to_char( "NPCs have no reason to do news.\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor.\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_HELP_EDIT:
         if( !( news = ( NEWS_DATA * ) ch->dest_buf ) )
         {
            bug( "%s: sub_help_edit: NULL ch->dest_buf", __FUNCTION__ );
            stop_editing( ch );
            return;
         }
         STRFREE( news->text );
         news->text = copy_buffer( ch );
         stop_editing( ch );
         save_news( );
         return;
   }

   set_pager_color( AT_HELP, ch );
   argument = one_argument( argument, arg );

   /* should we use date? */
   if( !str_cmp( arg, "date" ) )
   {
      udate = true;
      argument = one_argument( argument, arg );
   }

   /* Add a new news entry */
   if( arg != NULL && arg[0] != '\0' && !str_cmp( arg, "add" ) )
   {
      if( !( news = new_news( ) ) )
         return;
      if( argument && argument[0] != '\0' )
      {
         int level;

         if( is_number( argument ) )
            level = URANGE( 0, atoi( argument ), ( PERM_MAX - 1 ) );
         else
            level = get_flag( argument, perms_flag, PERM_MAX );
         if( level < 0 || level > get_trust( ch ) || level >= PERM_MAX )
         {
            send_to_char( "Permission out of range.\r\n", ch );
            return;
         }
         news->level = level;
      }
      ch->substate = SUB_HELP_EDIT;
      ch->dest_buf = news;
      news->poster = STRALLOC( ch->name );
      news->added = current_time;
      add_news( news );
      start_editing( ch, news->text );
      return;
   }

   /* Show all the news data if no arg or not a number or using a date */
   if( arg == NULL || arg[0] == '\0' || !is_number( arg ) || udate )
   {
      for( news = first_news; news; news = news->next )
      {
         if( get_trust( ch ) < news->level )
            continue;
         count++;
         if( arg != NULL && arg[0] != '\0' && str_cmp( arg, news->poster ) && !check_date( news, arg ) )
            continue;
         else if( ( arg == NULL || arg[0] == '\0' )
         && ch->pcdata && news != last_news && news->added <= ch->pcdata->news_read )
            continue;
         displayed++;
         show_news( ch, news, count );
         if( ch->pcdata && news->added > ch->pcdata->news_read )
            ch->pcdata->news_read = news->added;

         /* Limit the max it shows to 20? */
         if( displayed == 20 )
            break;
      }
      if( displayed == 0 )
      {
         if( arg == NULL || arg[0] == '\0' )
            send_to_char( "No news to show.\r\n", ch );
         else
            ch_printf( ch, "No news found matching %s.\r\n", arg );
      }
      return;
   }

   count = atoi( arg );
   if( !( news = find_news( ch, count ) ) )
   {
      send_to_char( "No such news to modify.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !is_immortal( ch ) )
   {
      show_news( ch, news, count );
      return;
   }

   /* Remove a news entry */
   if( !str_cmp( arg, "remove" ) )
   {
      remove_news( news );
      free_news( news );
      save_news( );
      send_to_char( "That news was removed and the news was saved.\r\n", ch );
      return;
   }

   /* Edit a news entry, only changes the text */
   if( !str_cmp( arg, "edit" ) )
   {
      ch->substate = SUB_HELP_EDIT;
      ch->dest_buf = news;
      STRSET( news->modified_by, ch->name );
      news->modified = current_time;
      start_editing( ch, news->text );
      return;
   }

   if( !str_cmp( arg, "perm" ) )
   {
      int level;

      if( is_number( argument ) )
         level = atoi( argument );
      else
         level = get_flag( argument, perms_flag, PERM_MAX );
      if( level < 0 || level > get_trust( ch ) || level >= PERM_MAX )
      {
         send_to_char( "Permission out of range.\r\n", ch );
         return;
      }
      news->level = level;
      save_news( );
      ch_printf( ch, "Permission on that news is now [%d]%s and the news has been saved.\r\n", news->level, perms_flag[news->level] );
      return;
   }

   /* Handle displaying it again if the right argument wasnt given */
   show_news( ch, news, count );
}
