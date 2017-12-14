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
 *                              Authorization                                *
 *****************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "h/mud.h"

typedef struct auth_data AUTH_DATA;
struct auth_data
{
   AUTH_DATA *next, *prev;
   char *name;
   int info;
};

AUTH_DATA *first_auth, *last_auth;
#define AUTH_FILE SYSTEM_DIR "auth.dat"

void free_auth( AUTH_DATA *auth )
{
   if( !auth )
      return;
   STRFREE( auth->name );
   DISPOSE( auth );
}

void free_all_auths( void )
{
   AUTH_DATA *auth, *auth_next;

   for( auth = first_auth; auth; auth = auth_next )
   {
      auth_next = auth->next;

      UNLINK( auth, first_auth, last_auth, next, prev );
      free_auth( auth );
   }
}

void save_auths( void )
{
   AUTH_DATA *auth;
   FILE *fp;

   if( !first_auth )
   {
      remove_file( AUTH_FILE );
      return;
   }

   if( !( fp = fopen( AUTH_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, AUTH_FILE );
      perror( AUTH_FILE );
      return;
   }

   for( auth = first_auth; auth; auth = auth->next )
   {
      fprintf( fp, "%s", "#AUTH\n" );
      fprintf( fp, "Name   %s~\n", auth->name );
      fprintf( fp, "Info   %d\n", auth->info );
      fprintf( fp, "%s", "End\n\n" );
   }

   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

AUTH_DATA *new_auth( void )
{
   AUTH_DATA *auth = NULL;

   CREATE( auth, AUTH_DATA, 1 );
   if( !auth )
   {
      bug( "%s: auth is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   auth->name = NULL;
   auth->info = 0;
   return auth;
}

void fread_auth( FILE *fp )
{
   AUTH_DATA *auth;
   const char *word;
   bool fMatch;

   auth = new_auth( );

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
               if( auth && auth->name && valid_pfile( auth->name ) )
                  LINK( auth, first_auth, last_auth, next, prev );
               else
                  free_auth( auth );
               return;
	    }
	    break;

         case 'I':
            KEY( "Info", auth->info, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", auth->name, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_auth( auth );
}

void load_auths( void )
{
   FILE *fp;

   first_auth = last_auth = NULL;
   if( !( fp = fopen( AUTH_FILE, "r" ) ) )
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
      if( !str_cmp( word, "AUTH" ) )
      {
         fread_auth( fp );
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

/* This will add the character to the auth list */
void add_to_auth( char *name )
{
   AUTH_DATA *auth;

   if( !name || name[0] == '\0' )
      return;

   /* No point in having it in the list more then once */
   for( auth = first_auth; auth; auth = auth->next )
   {
      if( !str_cmp( auth->name, name ) )
      {
         to_channel_printf( "auth", PERM_IMM, "%s: reapplying", auth->name );
         auth->info = 0;
         save_auths( );
         return;
      }
   }

   auth = new_auth( );
   if( !auth )
   {
      bug( "%s: auth is NULL after new_auth.", __FUNCTION__ );
      return;
   }
   auth->name = STRALLOC( name );
   auth->info = 0;
   LINK( auth, first_auth, last_auth, next, prev );
   to_channel_printf( "auth", PERM_IMM, "%s: applying", auth->name );
}

/* See if they are waiting to be authorized or not */
AUTH_DATA *get_auth( char *name )
{
   AUTH_DATA *auth;

   if( !name || name[0] == '\0' )
      return NULL;
   for( auth = first_auth; auth; auth = auth->next )
   {
      if( !str_cmp( auth->name, name ) )
         return auth;
   }
   return NULL;
}

CMDF( do_authorize )
{
   CHAR_DATA *tmp = NULL;
   AUTH_DATA *auth;
   char arg[MIL];

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      if( !first_auth )
      {
         send_to_char( "There is no one waiting to be authorized.\r\n", ch );
         return;
      }
      for( auth = first_auth; auth; auth = auth->next )
         ch_printf( ch, "%s %s.\r\n", auth->name, ( auth->info == 1 ) ? "has been denied access" : "is applying to be authorized" );
      send_to_char( "Usage: authorize <name> [no/add]\r\n", ch );
      return;
   }

   /* Ok lets allow a way to deny a name that was already authorized or authorize was off */
   if( argument != NULL && argument[0] != '\0' && !str_cmp( argument, "add" ) )
   {
      if( ( auth = get_auth( arg ) ) )
      {
         ch_printf( ch, "The name %s is already on the authorize list.\r\n", arg );
         return;
      }
      add_to_auth( arg ); /* Add it to the list */
      if( !( auth = get_auth( arg ) ) ) /* Get it and set it to denied */
         bug( "%s: %s wasn't added to the list correctly?", __FUNCTION__, arg );
      else
      {
         for( tmp = first_char; tmp; tmp = tmp->next )
         {
            if( is_npc( tmp ) )
               continue;

            if( !str_cmp( auth->name, tmp->name ) )
               break;
         }
         auth->info = 1;
         to_channel_printf( "auth", PERM_IMM, "%s: denied", auth->name );
         ch_printf( ch, "%s has been denied.\r\n", auth->name );
         if( tmp )
            ch_printf( tmp, "%s has denied your name. Please use the name command to change it.\r\n", ch->name );
         save_auths( );
      }
      return;
   }

   if( !( auth = get_auth( arg ) ) )
   {
      ch_printf( ch, "No one by the name of %s is waiting to be authorized.\r\n", arg );
      return;
   }

   for( tmp = first_char; tmp; tmp = tmp->next )
   {
      if( is_npc( tmp ) )
         continue;

      if( !str_cmp( auth->name, tmp->name ) )
         break;
   }

   if( argument != NULL && argument[0] != '\0' && !str_cmp( argument, "no" ) )
   {
      to_channel_printf( "auth", PERM_IMM, "%s: denied", auth->name );
      ch_printf( ch, "%s has been denied.\r\n", auth->name );
      if( tmp )
         ch_printf( tmp, "%s has denied your name. Please use the name command to change it.\r\n", ch->name );
      auth->info = 1;
   }
   else
   {
      to_channel_printf( "auth", PERM_IMM, "%s: authorized", auth->name );
      ch_printf( ch, "%s has been authorized.\r\n", auth->name );
      if( tmp )
         ch_printf( tmp, "%s has authorized your name. Hope you enjoy the game.\r\n", ch->name );
      UNLINK( auth, first_auth, last_auth, next, prev );
      free_auth( auth );
   }
   save_auths( );
}

CMDF( do_name )
{
   AUTH_DATA *auth;
   CHAR_DATA *tmp;

   if( is_npc( ch ) )
      return;

   if( !( auth = get_auth( ch->name ) ) || auth->info != 1 )
   {
      send_to_char( "You don't have to change your name.\r\n", ch );
      return;
   }

   if( !check_parse_name( argument, true ) )
   {
      send_to_char( "Illegal name, try another.\r\n", ch );
      return;
   }

   if( !str_cmp( ch->name, argument ) )
   {
      send_to_char( "That's already your name!\r\n", ch );
      return;
   }

   for( tmp = first_char; tmp; tmp = tmp->next )
   {
      if( is_npc( tmp ) )
         continue;

      if( !str_cmp( argument, tmp->name ) )
         break;
   }

   if( tmp || valid_pfile( argument ) )
   {
      send_to_char( "That name is already taken.  Please choose another.\r\n", ch );
      return;
   }

   rename_character( ch, argument );

   send_to_char( "Your name has been changed.\r\n", ch );
   UNLINK( auth, first_auth, last_auth, next, prev );
   free_auth( auth );
   add_to_auth( ch->name );
   save_auths( );
}

void auth_update( void )
{
   AUTH_DATA *auth;
   int count = 0;

   for( auth = first_auth; auth; auth = auth->next )
   {
      if( auth->info != 0 )
         continue;
      if( ++count == 1 )
         to_channel( "Pending authorizations:", "auth", 1 );
      to_channel_printf( "auth", 1, " %s", auth->name );
   }
}

/* Ok lets see if they need to change their name and if so let them know */
bool not_authorized( CHAR_DATA *ch )
{
   AUTH_DATA *auth;

   if( !ch || is_npc( ch ) )
      return false;

   if( !( auth = get_auth( ch->name ) ) || auth->info != 1 )
      return false;

   send_to_char( "Your name has been denied and you need to change it using the name command.\r\n", ch );
   return true;
}
