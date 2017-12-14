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

#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "h/mud.h"
#include "h/trivia.h"

UTRIVIA_DATA *utrivia;

TRIVIA_DATA *first_trivia, *last_trivia;
int top_trivia;
int top_answers;
int currentquestion;

void free_one_answer( ANSWER_DATA *answer )
{
   STRFREE( answer->answer );
   DISPOSE( answer );
}

void free_answers( TRIVIA_DATA *trivia )
{
   ANSWER_DATA *answer, *next_answer;

   for( answer = trivia->first_answer; answer; answer = next_answer )
   {
      next_answer = answer->next;
      UNLINK( answer, trivia->first_answer, trivia->last_answer, next, prev );
      top_answers--;
      free_one_answer( answer );
   }
}

void free_trivia( TRIVIA_DATA *trivia )
{
   STRFREE( trivia->question );
   free_answers( trivia );
   DISPOSE( trivia );
}

void remove_trivia( TRIVIA_DATA *trivia )
{
   UNLINK( trivia, first_trivia, last_trivia, next, prev );
   top_trivia--;
}

void free_all_trivias( void )
{
   TRIVIA_DATA *trivia, *trivia_next;

   for( trivia = first_trivia; trivia; trivia = trivia_next )
   {
      trivia_next = trivia->next;
      remove_trivia( trivia );
      free_trivia( trivia );
   }
}

void add_trivia( TRIVIA_DATA *trivia )
{
   LINK( trivia, first_trivia, last_trivia, next, prev );
   top_trivia++;
}

/* Look to see if this is an answer to the question */
bool check_answer( TRIVIA_DATA *trivia, char *argument )
{
   ANSWER_DATA *answer;

   if( !trivia )
      return false;

   for( answer = trivia->first_answer; answer; answer = answer->next )
   {
      if( !str_cmp( answer->answer, argument ) )
         return true;
   }

   return false;
}

char *get_hint( char *str )
{
   static char newstr[MSL];
   int i, j;

   if( !str || str[0] == '\0' )
      return (char *)"";

   for( i = j = 0; str[i] != '\0'; i++ )
   {
      if( str[i] != '\r' && str[i] != '\n' )
      {
         if( str[i] == ' ' )
            newstr[j++] = str[i];
         else
            newstr[j++] = '*';
      }
   }
   newstr[j] = '\0';
   return newstr;
}

void give_hint( CHAR_DATA *ch, TRIVIA_DATA *trivia )
{
   ANSWER_DATA *answer;

   if( !trivia || !ch )
      return;

   if( !trivia->first_answer )
   {
      send_to_char( "Sorry but there doesn't seem to be any answer for that question.", ch );
      return;
   }

   for( answer = trivia->first_answer; answer; answer = answer->next )
      ch_printf( ch, "%s%s", answer->prev ? " OR " : "Hint: ", get_hint( answer->answer ) );

   send_to_char( "\r\n", ch );
}

void save_trivia( void )
{
   TRIVIA_DATA *trivia;
   ANSWER_DATA *answer;
   FILE *fp;

   if( !first_trivia )
   {
      remove_file( TRIVIA_FILE );
      return;
   }

   if( !( fp = fopen( TRIVIA_FILE, "w" ) ) )
      return;
   for( trivia = first_trivia; trivia; trivia = trivia->next )
   {
      fprintf( fp, "%s", "#TRIVIA\n" );
      if( trivia->question )
         fprintf( fp, "Question  %s~\n", strip_cr( trivia->question ) );
      fprintf( fp, "Reward    %d\n", trivia->reward );
      for( answer = trivia->first_answer; answer; answer = answer->next )
         fprintf( fp, "Answer    %s~\n", strip_cr( answer->answer ) );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

TRIVIA_DATA *new_trivia( void )
{
   TRIVIA_DATA *trivia = NULL;

   CREATE( trivia, TRIVIA_DATA, 1 );
   if( !trivia )
   {
      bug( "%s: trivia is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }

   trivia->question = NULL;
   trivia->reward = 0;
   trivia->first_answer = trivia->last_answer = NULL;
   return trivia;
}

void fread_trivia( FILE *fp )
{
   TRIVIA_DATA *trivia;
   ANSWER_DATA *answer;
   const char *word;
   bool fMatch;

   trivia = new_trivia( );

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
               add_trivia( trivia );
               return;
	    }
	    break;

         case 'A':
            if( !str_cmp( word, "Answer" ) )
            {
               CREATE( answer, ANSWER_DATA, 1 );
               if( !answer )
                  fread_flagstring( fp );
               else
               {
                  answer->answer = fread_string( fp );
                  LINK( answer, trivia->first_answer, trivia->last_answer, next, prev );
                  top_answers++;
               }
               fMatch = true;
               break;
            }
            break;

         case 'Q':
            KEY( "Question", trivia->question, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Reward", trivia->reward, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_trivia( trivia );
}

void load_trivia( void )
{
   FILE *fp;

   top_trivia = 0;
   top_answers = 0;
   first_trivia = last_trivia = NULL;
   utrivia = NULL;

   if( !( fp = fopen( TRIVIA_FILE, "r" ) ) )
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
      if( !str_cmp( word, "TRIVIA" ) )
      {
         fread_trivia( fp );
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

/* Find a specific answer for a trivia */
ANSWER_DATA *find_answer( CHAR_DATA *ch, TRIVIA_DATA *trivia, int num )
{
   ANSWER_DATA *answer;
   int count = 0;

   if( !ch || !trivia )
      return NULL;
   for( answer = trivia->first_answer; answer; answer = answer->next )
   {
      if( ++count == num )
         return answer;
   }
   return NULL;
}

/* Find a specific trivia entry using a number */
TRIVIA_DATA *find_trivia( int num )
{
   TRIVIA_DATA *trivia;
   int cnt = 0;

   for( trivia = first_trivia; trivia; trivia = trivia->next )
   {
      if( ++cnt == num )
         return trivia;
   }
   return NULL;
}

void show_trivia( CHAR_DATA *ch, TRIVIA_DATA *trivia, int count )
{
   ANSWER_DATA *answer;
   int cnt = 0;

   ch_printf( ch, "&C[&W%4d&C] &CQuestion: &W%s\r\n", count, trivia->question );
   ch_printf( ch, "&CReward: &W%d&D\r\n", trivia->reward );
   for( answer = trivia->first_answer; answer; answer = answer->next )
      ch_printf( ch, "&C[&W%4d&C] &CAnswer: &W%s&D\r\n", ++cnt, answer->answer ? answer->answer : "(Nothing set for the answer)" );
}

void send_to_trivia( const char *argument )
{
   to_channel( argument, "Trivia", PERM_ALL );
}

void trivia_printf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   send_to_trivia( buf );
}

void stop_trivia( void )
{
   int cnt;

   if( !utrivia || !utrivia->running )
      return;
   trivia_printf( "&wThe &W%s &gT&Gr&Wiv&Gi&ga &wContest is now Closed!&D", sysdata.mud_name );
   utrivia->running = false;
   utrivia->trivia = NULL;
   utrivia->timer = 0;
   for( cnt = 0; cnt < MAX_QUESTIONS; cnt++ )
      utrivia->qasked[cnt] = -1;
   DISPOSE( utrivia );
}

void trivia_question( void )
{
   TRIVIA_DATA *trivia;
   int cnt = 0, check, tried = 0;
   bool qused = false;

   if( !utrivia || !utrivia->running)
      return;

   /* Limited to MAX_QUESTIONS */
   if( currentquestion >= MAX_QUESTIONS )
      return;

   if( !first_trivia )
   {
      send_to_trivia( "&wThere are currently no questions to ask.&D" );
      stop_trivia( );
      return;
   }

   for( trivia = first_trivia; trivia; trivia = trivia->next )
   {
      qused = false;

      /* Don't let it run in a loop */
      if( ++tried >= 1000 )
      {
         stop_trivia( );
         return;
      }

      /* Get a random question */
      cnt = number_range( 1, top_trivia );

      /* Check to see if the question has been used recently */
      for( check = 0; check < MAX_QUESTIONS; check++ )
      {
         if( cnt == utrivia->qasked[check] )
         {
            qused = true;
            break;
         }
      }

      if( qused )
         continue;

      if( !( utrivia->trivia = find_trivia( cnt ) ) )
         continue;

      trivia_printf( "&wThis question is worth &W%d &wtrivia points.&D", utrivia->trivia->reward );
      trivia_printf( "&W%s&D", utrivia->trivia->question );
      utrivia->timer = number_range( 30, 60 );
      trivia_printf( "&wYou have &W%d &wseconds to answer.&D", utrivia->timer );
      utrivia->qasked[currentquestion++] = cnt;
      return;
   }
}

void trivia_update( void )
{
   if( !utrivia || !utrivia->running )
      return;

   if( currentquestion >= MAX_QUESTIONS || currentquestion > top_trivia )
   {
      stop_trivia( );
      return;
   }

   if( !utrivia->trivia )
      trivia_question( );

   if( utrivia->timer <= 0 && !utrivia->trivia )
   {
      stop_trivia( );
      return;
   }

   if( utrivia->timer <= 0 && utrivia->trivia )
   {
      send_to_trivia( "&wTime ran out for this Trivia Question with no winner!&D" );
      utrivia->trivia = NULL;
      utrivia->timer = 0;
      trivia_question( );
   }
   if( utrivia->timer >= 1 )
      utrivia->timer--;
}

CMDF( do_guess )
{
   if( !ch || is_npc( ch ) )
      return;
   if( !utrivia || !utrivia->running )
   {
      send_to_char( "Trivia isn't currently running\r\n", ch );
      return;
   }

   if( utrivia->trivia && ( argument == NULL || argument[0] == '\0' ) )
   {
      show_trivia( ch, utrivia->trivia, currentquestion );
      return;
   }

   if( !utrivia->trivia )
   {
      send_to_char( "There currently isn't any question to guess the answer to.\r\n", ch );
      return;
   }

   /* Not a correct answer */
   if( !( check_answer( utrivia->trivia, argument ) ) )
   {
      if( !str_cmp( argument, "hint" ) )
      {
         give_hint( ch, utrivia->trivia );
         return;
      }
      trivia_printf( "&W%s &Rincorrectly &wanswered the question!&D", ch->name );
   }
   else /* Correct answer */
   {
      trivia_printf( "&W%s &Ccorrectly &wanswered the question!&D", ch->name );
      trivia_printf( "&WThe answer was: &C%s&D.", argument ); /* Argument was found to be the answer so show what it was */
      ch->pcdata->quest_curr += utrivia->trivia->reward; /* Don't forget to reward them lol */
      save_char_obj( ch ); /* As well as save the character */
      utrivia->trivia = NULL;
      utrivia->timer = 0;
      trivia_question();
   }
}

CMDF( do_trivia )
{
   TRIVIA_DATA *trivia;
   ANSWER_DATA *answer;
   char arg[MIL];
   int count = 0, displayed = 0;

   if( is_npc( ch ) )
   {
      send_to_char( "NPCs have no reason to do trivia.\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor.\r\n", ch );
      return;
   }

   set_pager_color( AT_HELP, ch );
   argument = one_argument( argument, arg );

   /* Add a new trivia */
   if( arg != NULL && arg[0] != '\0' && !str_cmp( arg, "add" ) )
   {
      if( argument == NULL || argument[0] == '\0' )
      {
         send_to_char( "You can't create a trivia with no question.\r\n", ch );
         return;
      }
      if( !( trivia = new_trivia( ) ) )
         return;
      STRSET( trivia->question, argument );
      add_trivia( trivia );
      save_trivia( );
      send_to_char( "Your new trivia has been added.\r\n", ch );
      return;
   }

   if( arg != NULL && arg[0] != '\0' && !str_cmp( arg, "start" ) )
   {
      if( utrivia && utrivia->running )
      {
         send_to_char( "Trivia is already running.\r\n", ch );
         return;
      }

      CREATE( utrivia, UTRIVIA_DATA, 1 );
      if( !utrivia )
      {
         bug( "%s: utrivia still NULL after CREATE", __FUNCTION__ );
         return;
      }
      utrivia->running = true;
      utrivia->trivia = NULL;
      utrivia->timer = 0;
      trivia_printf( "&wThe &W%s &gT&Gr&Wiv&Gi&ga &wContest is now Open!&D", sysdata.mud_name );
      for( count = 0; count < MAX_QUESTIONS; count++)
         utrivia->qasked[count] = -1;
      currentquestion = 0;
      trivia_question();
      return;
   }

   if( arg != NULL && arg[0] != '\0' && !str_cmp( arg, "stop" ) )
   {
      stop_trivia( );
      return;
   }

   /* Show all Questions and what number to use */
   if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
   {
      for( trivia = first_trivia; trivia; trivia = trivia->next )
      {
         ch_printf( ch, "&C[&W%4d&C] &CQuestion: &W%s\r\n", ++count, trivia->question );

         /* Limit the max it shows to 100? */
         if( ++displayed == 100 )
            break;
      }

      if( displayed == 0 )
         send_to_char( "No trivia to show.\r\n", ch );
      return;
   }

   count = atoi( arg );
   if( !( trivia = find_trivia( count ) ) )
   {
      send_to_char( "No such trivia to modify.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !is_immortal( ch ) )
   {
      show_trivia( ch, trivia, count );
      return;
   }

   /* Remove a trivia entry */
   if( !str_cmp( arg, "remove" ) )
   {
      remove_trivia( trivia );
      free_trivia( trivia );
      save_trivia( );
      send_to_char( "That trivia was removed and the trivia table was saved.\r\n", ch );
      return;
   }

   /* Add/Edit answers to the trivia */
   if( !str_cmp( arg, "answer" ) )
   {
      argument = one_argument( argument, arg );
      if( !str_cmp( arg, "add" ) )
      {
         if( argument == NULL || argument[0] == '\0' )
         {
            send_to_char( "You can't add an empty answer.\r\n", ch );
            return;
         }
         CREATE( answer, ANSWER_DATA, 1 );
         STRSET( answer->answer, argument );
         LINK( answer, trivia->first_answer, trivia->last_answer, next, prev );
         top_answers++;
         save_trivia( );
         send_to_char( "That answer has been added.\r\n", ch );
         return;
      }

      count = atoi( arg );
      if( !( answer = find_answer( ch, trivia, count ) ) )
      {
         send_to_char( "No such answer to modify.\r\n", ch );
         return;
      }

      argument = one_argument( argument, arg );

      if( !str_cmp( arg, "edit" ) )
      {
         if( argument == NULL || argument[0] == '\0' )
         {
            send_to_char( "You can't edit an answer to be empty.\r\n", ch );
            return;
         }

         STRSET( answer->answer, argument );
         save_trivia( );
         send_to_char( "That answer has been modified.\r\n", ch );
         return;
      }
      else if( !str_cmp( arg, "remove" ) )
      {
         UNLINK( answer, trivia->first_answer, trivia->last_answer, next, prev );
         top_answers--;
         free_one_answer( answer );
         save_trivia( );
         send_to_char( "That answer has been removed.\r\n", ch );
         return;
      }
      send_to_char( "Usage: trivia answer add <answer>\r\n", ch );
      send_to_char( "Usage: trivia answer <#> remove\r\n", ch );
      send_to_char( "Usage: trivia answer <#> edit <answer>\r\n", ch );
      return;
   }

   /* Edit a trivia question */
   if( !str_cmp( arg, "edit" ) )
   {
      if( argument == NULL || argument[0] == '\0' )
      {
         send_to_char( "You can't change a trivia to have no question.\r\n", ch );
         return;
      }
      STRSET( trivia->question, argument );
      save_trivia( );
      send_to_char( "The trivia question has been changed.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "reward" ) )
   {
      trivia->reward = UMAX( 0, atoi( argument ) );
      save_trivia( );
      ch_printf( ch, "Reward on that trivia is now %d.\r\n", trivia->reward );
      return;
   }

   /* Handle displaying it again if the right argument wasnt given */
   show_trivia( ch, trivia, count );
}
