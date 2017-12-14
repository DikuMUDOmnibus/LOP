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

#include <stdio.h>
#include "h/mud.h"
#include "h/bti.h"

BTI_DATA *first_bti, *last_bti;
int top_bti;

void free_bti( BTI_DATA *bti )
{
   STRFREE( bti->text );
   STRFREE( bti->poster );
   DISPOSE( bti );
}

void remove_bti( BTI_DATA *bti )
{
   UNLINK( bti, first_bti, last_bti, next, prev );
   top_bti--;
}

void free_all_bti( void )
{
   BTI_DATA *bti, *bti_next;

   for( bti = first_bti; bti; bti = bti_next )
   {
      bti_next = bti->next;
      remove_bti( bti );
      free_bti( bti );
   }
}

void add_bti( BTI_DATA *bti )
{
   LINK( bti, first_bti, last_bti, next, prev );
   top_bti++;
}

void save_bti( void )
{
   BTI_DATA *bti;
   FILE *fp;

   if( !first_bti )
   {
      remove_file( BTI_FILE );
      return;
   }
   if( !( fp = fopen( BTI_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, BTI_FILE );
      return;
   }
   for( bti = first_bti; bti; bti = bti->next )
   {
      fprintf( fp, "%s", "#BTI\n" );
      if( bti->poster )
         fprintf( fp, "Poster  %s~\n", bti->poster );
      fprintf( fp, "Time    %ld\n", bti->added );
      fprintf( fp, "Type    %d\n", bti->type );
      fprintf( fp, "Room    %d\n", bti->room );
      if( bti->text )
         fprintf( fp, "Text    %s~\n", strip_cr( bti->text ) );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

void fread_bti( FILE *fp )
{
   BTI_DATA *bti;
   const char *word;
   bool fMatch;

   CREATE( bti, BTI_DATA, 1 );

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
               add_bti( bti );
               return;
	    }
	    break;

         case 'P':
            KEY( "Poster", bti->poster, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Room", bti->room, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Text", bti->text, fread_string( fp ) );
            KEY( "Time", bti->added, fread_time( fp ) );
            KEY( "Type", bti->type, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_bti( bti );
}

void load_bti( void )
{
   FILE *fp;

   top_bti = 0;
   first_bti = last_bti = NULL;

   if( !( fp = fopen( BTI_FILE, "r" ) ) )
      return;
   for( ;; )
   {
      char letter;
      const char *word;

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

      word = feof( fp ) ? "END" : fread_word( fp );
      if( !str_cmp( word, "BTI" ) )
      {
         fread_bti( fp );
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

/* Find a specif bti entry using a number */
BTI_DATA *find_bti( CHAR_DATA *ch, int num )
{
   BTI_DATA *bti;
   int cnt = 0;

   if( !ch || num <= 0 )
      return NULL;
   for( bti = first_bti; bti; bti = bti->next )
   {
      if( ++cnt == num )
         return bti;
   }
   return NULL;
}

void show_bti( CHAR_DATA *ch, BTI_DATA *bti, int count )
{
   ch_printf( ch, "&C[&W%4d&C] &W%7s &CPosted by &W%s &Cfrom room [&W%d&C] %s&D\r\n",
      count, bti->type == 0 ? "BUG" : bti->type == 1 ? "IDEA" : bti->type == 2 ? "TYPO" : "UNKNOWN",
      bti->poster ? bti->poster : "(Unknown)", bti->room, distime( bti->added ) );
   ch_printf( ch, "&W%s&D\r\n", bti->text ? bti->text : "(Not set yet)" );
}

/* Give a quick count on login of the BTIs */
void check_bti( CHAR_DATA *ch )
{
   BTI_DATA *bti;
   int bugs = 0, typos = 0, ideas = 0, unknowns = 0;

   if( !ch || !is_immortal( ch ) )
      return;

   for( bti = first_bti; bti; bti = bti->next )
   {
      if( bti->type == 0 )
         bugs++;
      else if( bti->type == 1 )
         ideas++;
      else if( bti->type == 2 )
         typos++;
      else
         unknowns++;
   }
   if( bugs )
      ch_printf( ch, "There %s currently %d bug%s.\r\n", bugs == 1 ? "is" : "are", bugs, bugs == 1 ? "" : "s" );
   if( ideas )
      ch_printf( ch, "There %s currently %d idea%s.\r\n", ideas == 1 ? "is" : "are", ideas, ideas == 1 ? "" : "s" );
   if( typos )
      ch_printf( ch, "There %s currently %d typo%s.\r\n", typos == 1 ? "is" : "are", typos, typos == 1 ? "" : "s");
   if( unknowns )
      ch_printf( ch, "There %s currently %d unknown%s.\r\n", unknowns == 1 ? "is" : "are", unknowns, unknowns == 1 ? "" : "s");
}

CMDF( do_bti )
{
   BTI_DATA *bti;
   char arg[MIL];
   int count = 0;

   if( !ch || is_npc( ch ) || !ch->desc )
      return;

   set_pager_color( AT_HELP, ch );
   argument = one_argument( argument, arg );

   /* Show all the bti data if no arg or not a number */
   if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
   {
      for( bti = first_bti; bti; bti = bti->next )
      {
         count++;
         show_bti( ch, bti, count );
      }
      if( count == 0 )
         send_to_char( "No bti to show.\r\n", ch );
      return;
   }
   count = atoi( arg );
   if( !( bti = find_bti( ch, count ) ) )
   {
      send_to_char( "No such bti to modify.\r\n", ch );
      return;
   }
   argument = one_argument( argument, arg );
   if( !is_immortal( ch ) )
   {
      show_bti( ch, bti, count );
      return;
   }
   /* Remove a bti entry */
   if( !str_cmp( arg, "remove" ) )
   {
      remove_bti( bti );
      save_bti( );
      send_to_char( "That bti was removed and the bti was saved.\r\n", ch );
      return;
   }
   /* Handle displaying it again if the right argument wasnt given */
   show_bti( ch, bti, count );
}

void newbti( CHAR_DATA *ch, int type, char *argument )
{
   BTI_DATA *bti;
   const char *msg = "unknown";

   if( !ch )
      return;
   if( type == 0 )
      msg = "bug";
   else if( type == 1 )
      msg = "idea";
   else if( type == 2 )
      msg = "typo";
   set_char_color( AT_PLAIN, ch );
   if( !argument || argument[0] == '\0' )
   {
      ch_printf( ch, "\r\nUsage: '%s <message>'\r\n", msg );
      return;
   }
   CREATE( bti, BTI_DATA, 1 );
   bti->poster = STRALLOC( ch->name );
   bti->added = current_time;
   bti->text = STRALLOC( argument );
   bti->type = type;
   bti->room = ( ch->in_room ? ch->in_room->vnum : 0 );
   add_bti( bti );
   save_bti( );
   ch_printf( ch, "Thanks, your %s has been recorded.\r\n", msg );
}

CMDF( do_bug )
{
   newbti( ch, 0, argument );
}

CMDF( do_idea )
{
   newbti( ch, 1, argument );
}

CMDF( do_typo )
{
   newbti( ch, 2, argument );
}
