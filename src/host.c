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
 *                             Host module                                   *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"

CMDF( do_host )
{
   char type[MIL], arg1[MIL];
   char *name, *arg2 = NULL;
   HOST_DATA *host;

   if( !ch || !ch->desc || is_npc( ch ) )
      return;

   argument = one_argument( argument, type );
   argument = one_argument( argument, arg1 );

   if( type == NULL || type[0] == '\0' )
   {
      ch_printf( ch, "You're currently connected from %s.\r\n", ch->desc->host );
      if( !ch->first_host )
         send_to_char( "You don't currently have any host protection for this character.\r\n", ch );
      else
      {
         for( host = ch->first_host; host; host = host->next )
            ch_printf( ch, "%c%s%c\r\n", ( host->prefix ? '*' : ' ' ), host->host, ( host->suffix ? '*' : ' ' ) );
      }
      return;
   }

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: host [add/delete <host>]\r\n", ch );
      return;
   }

   if( !str_cmp( type, "delete" ) )
   {
      arg2 = arg1;
      if( arg2[0] == '*' )
         arg2++;
      if( arg2[strlen( arg2 ) - 1] == '*' )
         arg2[strlen( arg2 ) - 1] = '\0';

      for( host = ch->first_host; host; host = host->next )
      {
         if( !str_cmp( arg2, host->host ) )
         {
            STRFREE( host->host );
            UNLINK( host, ch->first_host, ch->last_host, next, prev );
            DISPOSE( host );
            save_char_obj( ch );
            send_to_char( "Deleted.\r\n", ch );
            if( !check_host( ch ) )
               send_to_char( "WARNING: If you try to login to this character from your current host it won't let you.\r\n", ch );
            return;
         }
      }
      send_to_char( "You don't have that host listed.\r\n", ch );
      return;
   }
   else if( !str_cmp( type, "add" ) )
   {
      bool prefix = false, suffix = false;
      int i;

      name = arg1;
      if( name[0] == '*' )
      {
         prefix = true;
         name++;
      }
      if( name[strlen( name ) - 1] == '*' )
      {
         suffix = true;
         name[strlen( name ) - 1] = '\0';
      }

      for( i = 0; i < ( int )strlen( name ); i++ )
         name[i] = LOWER( name[i] );
      for( host = ch->first_host; host; host = host->next )
      {
         if( !str_cmp( host->host, name ) )
         {
            send_to_char( "Entry already exists.\r\n", ch );
            return;
         }
      }
      host = NULL;
      CREATE( host, HOST_DATA, 1 );
      host->host = STRALLOC( name );
      host->prefix = prefix;
      host->suffix = suffix;
      LINK( host, ch->first_host, ch->last_host, next, prev );
      save_char_obj( ch );
   }
   else
   {
      send_to_char( "Usage: immhost [add/delete <host>]\r\n", ch );
      return;
   }
   send_to_char( "Added.\r\n", ch );
   if( !check_host( ch ) )
      send_to_char( "WARNING: If you try to login to this character from your current host it won't let you.\r\n", ch );
}

bool check_host( CHAR_DATA *ch )
{
   HOST_DATA *host;

   if( !ch || !ch->desc || !ch->first_host )
      return true;
   for( host = ch->first_host; host; host = host->next )
   {
      if( host->prefix && host->suffix && strstr( ch->desc->host, host->host ) )
         return true;
      else if( host->prefix && !str_suffix( host->host, ch->desc->host ) )
         return true;
      else if( host->suffix && !str_prefix( host->host, ch->desc->host ) )
         return true;
      else if( !str_cmp( host->host, ch->desc->host ) )
        return true;
   }
   return false;
}
