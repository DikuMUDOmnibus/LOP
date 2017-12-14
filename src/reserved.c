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
 *                       Reserved_Data info                                  *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"

typedef struct reserve_data RESERVE_DATA;

/* Yeesh.. remind us of the old MERC ban structure? :) */
struct reserve_data
{
   RESERVE_DATA *next, *prev;
   char *name;
};

RESERVE_DATA *first_reserved, *last_reserved;

bool is_reserved_name( char *name )
{
   RESERVE_DATA *res;

   for( res = first_reserved; res; res = res->next )
      if( ( *res->name == '*' && !str_infix( res->name + 1, name ) ) || !str_cmp( res->name, name ) )
         return true;
   return false;
}

void save_reserved( void )
{
   RESERVE_DATA *res;
   FILE *fp;

   if( !first_reserved )
   {
      remove_file( RESERVED_LIST );
      return;
   }
   if( !( fp = fopen( RESERVED_LIST, "w" ) ) )
   {
      bug( "%s: can't open %s for writing", __FUNCTION__, RESERVED_LIST );
      perror( RESERVED_LIST );
      return;
   }
   for( res = first_reserved; res; res = res->next )
      fprintf( fp, "%s~\n", res->name );
   fprintf( fp, "$~\n" );
   fclose( fp );
   fp = NULL;
}

void add_reserved( RESERVE_DATA *pRes )
{
   RESERVE_DATA *res = NULL;

   if( !pRes )
   {
      bug( "%s: NULL pRes", __FUNCTION__ );
      return;
   }

   pRes->next = pRes->prev = NULL;
   for( res = first_reserved; res; res = res->next )
   {
      if( strcasecmp( res->name, pRes->name ) > 0 )
      {
         INSERT( pRes, res, first_reserved, next, prev );
         return;
      }
   }
   LINK( pRes, first_reserved, last_reserved, next, prev );
}

void free_reserve( RESERVE_DATA *res )
{
   STRFREE( res->name );
   DISPOSE( res );
}

void free_all_reserved( void )
{
   RESERVE_DATA *res, *res_next;

   for( res = first_reserved; res; res = res_next )
   {
      res_next = res->next;
      UNLINK( res, first_reserved, last_reserved, next, prev );
      free_reserve( res );
   }
}

CMDF( do_reserve )
{
   char arg[MIL];
   RESERVE_DATA *res;

   set_char_color( AT_PLAIN, ch );
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      int wid = 0;

      send_to_char( "-- Reserved Names --\r\n", ch );
      for( res = first_reserved; res; res = res->next )
      {
         ch_printf( ch, "%-19s ", res->name );
         if( ++wid == 4 )
         {
            wid = 0;
            send_to_char( "\r\n", ch );
         }
      }
      if( wid != 0 )
         send_to_char( "\r\n", ch );
      return;
   }
   for( res = first_reserved; res; res = res->next )
   {
      if( res->name && !str_cmp( arg, res->name ) )
      {
         ch_printf( ch, "%s is no longer reserved.\r\n", res->name );
         UNLINK( res, first_reserved, last_reserved, next, prev );
         free_reserve( res );
         save_reserved( );
         return;
      }
   }
   CREATE( res, RESERVE_DATA, 1 );
   if( !res )
   {
      bug( "%s: couldn't CREATE reserve.", __FUNCTION__ );
      return;
   }
   res->name = STRALLOC( arg );
   ch_printf( ch, "%s is now reserved.\r\n", res->name );
   add_reserved( res );
   save_reserved( );
}

void load_reserved( void )
{
   RESERVE_DATA *res;
   FILE *fp;
   char *name;

   if( !( fp = fopen( RESERVED_LIST, "r" ) ) )
      return;

   for( ;; )
   {
      if( feof( fp ) )
         break;
      name = fread_flagstring( fp );
      if( !name || name[0] == '\0' || name[0] == '$' || name[0] == EOF )
         break;
      CREATE( res, RESERVE_DATA, 1 );
      res->name = STRALLOC( name );
      add_reserved( res );
   }
   fclose( fp );
   fp = NULL;
}
