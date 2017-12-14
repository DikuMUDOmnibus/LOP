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
 *			     Host Log File       			     *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include "h/mud.h"

typedef struct iplog_data IPLOG_DATA;
struct iplog_data
{
   IPLOG_DATA *next, *prev;
   char *host;
   char *message;
   time_t mtime;
};

typedef struct hostlog_data HOSTLOG_DATA;
struct hostlog_data
{
   HOSTLOG_DATA *next, *prev;
   IPLOG_DATA *first_information, *last_information;
   char *name;
};

HOSTLOG_DATA *first_hostlog, *last_hostlog;

void free_iplog_data( IPLOG_DATA *iplog )
{
   if( !iplog )
      return;
   STRFREE( iplog->host );
   STRFREE( iplog->message );
   DISPOSE( iplog );
}

void free_iplogs( HOSTLOG_DATA *host )
{
   IPLOG_DATA *iplog, *iplog_next;

   if( !host )
      return;
   for( iplog = host->first_information; iplog; iplog = iplog_next )
   {
      iplog_next = iplog->next;
      UNLINK( iplog, host->first_information, host->last_information, next, prev );
      free_iplog_data( iplog );
   }
}

void free_one_hostlog( HOSTLOG_DATA *host )
{
   if( !host )
      return;
   free_iplogs( host );
   UNLINK( host, first_hostlog, last_hostlog, next, prev );
   STRFREE( host->name );
   DISPOSE( host );
}

void free_all_hostlogs( void )
{
   HOSTLOG_DATA *host, *host_next;

   for( host = first_hostlog; host; host = host_next )
   {
      host_next = host->next;
      free_one_hostlog( host );
   }
}

/* Used to see how many different ips have logged in today */
int get_ip_logins( void )
{
   HOSTLOG_DATA *host;
   IPLOG_DATA *iplog;
   int count = 0;

   for( host = first_hostlog; host; host = host->next )
   {
      for( iplog = host->first_information; iplog; iplog = iplog->next )
      {
         if( iplog->mtime < boot_time )
            continue;
         count++;
         break;
      }
   }

   return count;
}

HOSTLOG_DATA *find_hostlog( char *argument )
{
   HOSTLOG_DATA *host;

   for( host = first_hostlog; host; host = host->next )
   {
      if( host && host->name && argument && !str_cmp( host->name, argument ) )
         return host;
   }

   return NULL;
}

void save_hostlogs( void )
{
   HOSTLOG_DATA *host;
   IPLOG_DATA *iplog;
   FILE *fp;

   if( !first_hostlog )
   {
      remove_file( HOSTLOG_FILE );
      return;
   }
   if( !( fp = fopen( HOSTLOG_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, HOSTLOG_FILE );
      perror( HOSTLOG_FILE );
      return;
   }
   for( host = first_hostlog; host; host = host->next )
   {
      fprintf( fp, "%s", "#HOST\n" );
      fprintf( fp, "Name      %s~\n", host->name );
      for( iplog = host->first_information; iplog; iplog = iplog->next )
         fprintf( fp, "IPLOG     %s~ %s~ %ld\n", iplog->message, iplog->host, iplog->mtime );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

void handle_hostlog( const char *message, CHAR_DATA *ch )
{
   HOSTLOG_DATA *host;
   IPLOG_DATA *iplog;

   if( !( host = find_hostlog( ch->name ) ) )
   {
      CREATE( host, HOSTLOG_DATA, 1 );
      if( !host )
      {
         bug( "%s: host is NULL after create.", __FUNCTION__ );
         return;
      }
      host->name = STRALLOC( ch->name );
      LINK( host, first_hostlog, last_hostlog, next, prev );

      CREATE( iplog, IPLOG_DATA, 1 );
      if( !iplog )
      {
         bug( "%s: iplog is NULL after create.", __FUNCTION__ );
         return;
      }
      if( !ch || !ch->desc || !ch->desc->host )
         iplog->host = STRALLOC( "Unknown" );
      else
         iplog->host = STRALLOC( ch->desc->host );
      iplog->mtime = current_time;
      iplog->message = STRALLOC( message );
      LINK( iplog, host->first_information, host->last_information, next, prev );
      save_hostlogs( );
      return;
   }

   /* In order to keep the list smaller we will only keep the last 10 from each ip address ( Keep 9 and then adding 1 to it ) */
   {
      IPLOG_DATA *iplog_next;
      int count = 0;

      for( iplog = host->first_information; iplog; iplog = iplog_next )
      {
         iplog_next = iplog->next;
         if( !ch || !ch->desc || !ch->desc->host )
            continue;
         if( str_cmp( iplog->host, ch->desc->host ) )
            continue;
         if( ++count < 9 )
            continue;
         UNLINK( iplog, host->first_information, host->last_information, next, prev );
         free_iplog_data( iplog );
      }
   }

   CREATE( iplog, IPLOG_DATA, 1 );
   if( !iplog )
   {
      bug( "%s: iplog is NULL after create.", __FUNCTION__ );
      return;
   }

   if( !ch || !ch->desc || !ch->desc->host )
      iplog->host = STRALLOC( "Unknown" );
   else
      iplog->host = STRALLOC( ch->desc->host );
   iplog->mtime = current_time;
   iplog->message = STRALLOC( message );
   /* Always put the new ones at the top of the list when possible */
   if( host->first_information )
   {
      iplog->prev = NULL;
      iplog->next = host->first_information;
      host->first_information->prev = iplog;
      host->first_information = iplog;
   }
   else
      LINK( iplog, host->first_information, host->last_information, next, prev );
   save_hostlogs( );
}

CMDF( do_hsearch )
{
   HOSTLOG_DATA *host;
   IPLOG_DATA *iplog;
   int count = 0;
   bool found = false;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: hsearch <ip address>\r\n", ch );
      return;
   }

   for( host = first_hostlog; host; host = host->next )
   {
      for( iplog = host->first_information; iplog; iplog = iplog->next )
      {
         if( !str_cmp( iplog->host, argument ) )
         {
            if( !found )
               ch_printf( ch, "Characters accessed from host %s.\r\n", argument );
            found = true;
            ch_printf( ch, "%18s", host->name );
            if( ++count == 4 )
            {
               send_to_char( "\r\n", ch );
               count = 0;
            }
            else
               send_to_char( "  ", ch );
            break;
         }
      }
   }
   if( !found )
      ch_printf( ch, "No characters accessed from host %s.\r\n", argument );
   else if( count != 0 )
      send_to_char( "\r\n", ch );
}

CMDF( do_hlog )
{
   HOSTLOG_DATA *host;
   IPLOG_DATA *iplog;
   int count;

   if( !argument || argument[0] == '\0' )
   {
      for( host = first_hostlog; host; host = host->next )
      {
         ch_printf( ch, "%s:\r\n", host->name );
         count = 0;
         for( iplog = host->first_information; iplog; iplog = iplog->next )
         {
            ch_printf( ch, "   %s on %s from %s\r\n", iplog->message, distime( iplog->mtime ), iplog->host );
            if( count++ == 4 )
               break;
         }
      }
      return;
   }
   if( !( host = find_hostlog( argument ) ) )
   {
      send_to_char( "There is no such hostlog information to display.\r\n", ch );
      return;
   }

   ch_printf( ch, "%s: \r\n", host->name );
   for( iplog = host->first_information; iplog; iplog = iplog->next )
      ch_printf( ch, "   %s on %s from %s\r\n", iplog->message, distime( iplog->mtime ), iplog->host );
}

void fread_hostlog( FILE *fp )
{
   HOSTLOG_DATA *host;
   IPLOG_DATA *iplog;
   const char *word;
   bool fMatch;

   CREATE( host, HOSTLOG_DATA, 1 );
   host->name = NULL;
   host->first_information = host->last_information = NULL;

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
               LINK( host, first_hostlog, last_hostlog, next, prev );
               return;
	    }
	    break;

         case 'I':
            if( !str_cmp( word, "IPLOG" ) )
            {
               CREATE( iplog, IPLOG_DATA, 1 );
               if( !iplog )
               {
                  bug( "%s: iplog is NULL after create.", __FUNCTION__ );
                  fread_flagstring( fp );
                  fread_flagstring( fp );
                  fread_number( fp );
                  continue;
               }
               iplog->message = fread_string( fp );
               iplog->host = fread_string( fp );
               iplog->mtime = fread_time( fp );
               LINK( iplog, host->first_information, host->last_information, next, prev );
               fMatch = true;
               break;
            }
            break;

         case 'N':
            KEY( "Name", host->name, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_one_hostlog( host );
}

void load_hostlog( void )
{
   FILE *fp;

   first_hostlog = last_hostlog = NULL;
   if( !( fp = fopen( HOSTLOG_FILE, "r" ) ) )
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
      if( !str_cmp( word, "HOST" ) )
      {
         fread_hostlog( fp );
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
