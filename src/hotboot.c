/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2003 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine, and Adjani.    *
 * All Rights Reserved.                                                     *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                             Hotboot module                               *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#if !defined(WIN32)
   #include <dlfcn.h>
#else
   #include <windows.h>
   #define dlopen( libname, flags ) LoadLibrary( (libname) )
   #define dlclose( libname ) FreeLibrary( (HINSTANCE) (libname) )
#endif
#include <unistd.h>
#include "h/mud.h"
#include "h/mccp.h"

/* Warm reboot stuff, gotta make sure to thank Erwin for this :) */
CMDF( do_hotboot )
{
   FILE *fp;
   CHAR_DATA *victim = NULL;
   DESCRIPTOR_DATA *d, *de_next;
   char buf[100], buf2[100], buf3[100];
   extern int control;
   int count = 0;
   bool found = false;

   for( d = first_descriptor; d; d = d->next )
   {
      if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
      && ( victim = d->character ) && !is_npc( victim ) && victim->in_room
      && victim->fighting && victim->level >= 1 && victim->level <= MAX_LEVEL )
      {
         found = true;
         count++;
      }
   }

   if( found )
   {
      ch_printf( ch, "Can't hotboot at this time. There are %d combats in progress.\r\n", count );
      return;
   }

   found = false;
   for( d = first_descriptor; d; d = d->next )
   {
      if( d->connected == CON_EDITING && d->character )
      {
         found = true;
         break;
      }
   }

   if( found )
   {
      send_to_char( "Can't hotboot at this time. Someone is using the line editor.\r\n", ch );
      return;
   }

   log_printf( "Hotboot initiated by %s.", ch->name );
   open_mud_log();
   log_string("Hotboot: Spawning new log file");
   log_printf( "Hotboot initiated by %s.", ch->name );

   if( !( fp = fopen( HOTBOOT_FILE, "w" ) ) )
   {
      send_to_char( "Hotboot file not writeable, aborted.\r\n", ch );
      bug( "Could not write to hotboot file: %s. Hotboot aborted.", HOTBOOT_FILE );
      perror( "do_copyover:fopen" );
      return;
   }

   log_string( "Saving player files and connection states...." );
   if( ch && ch->desc )
      write_to_descriptor( ch->desc, "\033[0m", 0 );
   snprintf( buf, sizeof( buf ), "\r\nThe flow of time is halted momentarily as the world is reshaped!\r\n" );

   /* For each playing descriptor, save its state */
   for( d = first_descriptor; d; d = de_next )
   {
      CHAR_DATA *och = d->character;

      de_next = d->next;   /* We delete from the list , so need to save this */
      if( !d->character || d->connected < CON_PLAYING )  /* drop those logging on */
      {
         write_to_descriptor( d, "\r\nSorry, we are rebooting. Come back in a few minutes.\r\n", 0 );
         close_socket( d, false );  /* throw'em out */
      }
      else
      {
         int room = sysdata.room_limbo;

         if( och->pcdata && xIS_SET( och->pcdata->flags, PCFLAG_IDLE ) && och->was_in_room )
            room = och->was_in_room->vnum;
         else if( och->in_room )
            room = och->in_room->vnum;
         else
            bug( "%s: NULL room for %s, leaving room as %d.", __FUNCTION__, och->name, room );
         fprintf( fp, "%d %d %d %d %s %s\n", d->descriptor, room, d->port, d->idle, och->name, d->host );
         /* One of two places this gets changed */
         och->pcdata->hotboot = true;
         save_char_obj( och );
         write_to_descriptor( d, buf, 0 );
         compressEnd( d );
         d->can_compress = false;
      }
   }

   fprintf( fp, "0 0 0 %d maxp maxp\n", sysdata.maxplayers );
   fprintf( fp, "%s", "-1\n" );
   fclose( fp );
   fp = NULL;

#ifdef IMC
   imc_hotboot( );
#endif

   /* added this in case there's a need to debug the contents of the various files */
   if( argument && !str_cmp( argument, "debug" ) )
   {
      log_string( "Hotboot debug - Aborting before execl" );
      return;
   }

   log_string( "Executing hotboot...." );

   /* exec - descriptors are inherited */
   snprintf( buf, sizeof( buf ), "%d", port );
   snprintf( buf2, sizeof( buf2 ), "%d", control );
#ifdef IMC
   if( this_imcmud )
      snprintf( buf3, sizeof( buf3 ), "%d", this_imcmud->desc );
   else
      strncpy( buf3, "-1", sizeof( buf3 ) );
#else
   strncpy( buf3, "-1", sizeof( buf3 ) );
#endif

   set_alarm( 0 );
   dlclose( sysdata.dlHandle );
   execl( EXE_FILE, "LOP", buf, "hotboot", buf2, buf3, ( char * )NULL );

   /* Failed - sucessful exec won't return */
   perror( "do_hotboot: execl" );

   if( !( sysdata.dlHandle = dlopen( NULL, RTLD_LAZY ) ) )
   {
      bug( "%s", "FATAL ERROR: Unable to reopen system executable handle!" );
      exit( 1 );
   }

   bug( "%s", "Hotboot execution failed!!" );
   send_to_char( "Hotboot FAILED!\r\n", ch );
}

/* Recover from a hotboot - load players */
void hotboot_recover( void )
{
   DESCRIPTOR_DATA *d = NULL;
   FILE *fp;
   char name[100], host[MSL];
   int desc, room, dport, idle, maxp = 0, fscanned;
   bool fOld;

   if( !( fp = fopen( HOTBOOT_FILE, "r" ) ) ) /* there are some descriptors open which will hang forever then ? */
   {
      perror( "hotboot_recover: fopen" );
      bug( "%s", "Hotboot file not found. Exitting." );
      exit( 1 );
   }

   remove_file( HOTBOOT_FILE ); /* In case something crashes - doesn't prevent reading */
   for( ;; )
   {
      d = NULL;

      fscanned = fscanf( fp, "%d %d %d %d %s %s\n", &desc, &room, &dport, &idle, name, host );

      if( desc == -1 || feof( fp ) )
         break;

      if( !str_cmp( name, "maxp" ) || !str_cmp( host, "maxp" ) )
      {
         maxp = idle;
         continue;
      }

      /* Write something, and check if it goes error-free */
      if( !write_to_descriptor_old( desc, "The ether swirls in chaos.\r\n", 0 ) )
      {
         bug( "%s closeing descriptor.", __FUNCTION__ );
         close( desc ); /* nope */
         continue;
      }

      CREATE( d, DESCRIPTOR_DATA, 1 );

      d->next = NULL;
      d->descriptor = desc;
      d->connected = CON_GET_NAME;
      d->outsize = 2000;
      d->tempidle = 0;
      d->idle = 0;
      d->lines = 0;
      d->scrlen = 24;
      d->newstate = 0;
      d->prevcolor = 0x08;

      CREATE( d->outbuf, char, d->outsize );

      d->host = STRALLOC( host );
      d->port = dport;
      d->idle = idle;
      CREATE( d->mccp, MCCP, 1 );
      d->can_compress = false;
      LINK( d, first_descriptor, last_descriptor, next, prev );
      d->connected = CON_COPYOVER_RECOVER; /* negative so close_socket will cut them off */

      /* Now, find the pfile */
      fOld = load_char_obj( d, name, false, true );

      if( !fOld ) /* Player file not found?! */
      {
         write_to_descriptor( d, "Somehow, your character was lost during hotboot. Contact the immortals ASAP.\r\n", 0 );
         close_socket( d, false );
      }
      else  /* ok! */
      {
         write_to_descriptor( d, "Time resumes its normal flow.\r\n", 0 );

         /* Insert in the char_list */
         LINK( d->character, first_char, last_char, next, prev );

         if( d->character && d->character->pcdata && xIS_SET( d->character->pcdata->flags, PCFLAG_IDLE ) )
         {
            d->character->was_in_room = get_room_index( room );
            d->character->in_room = get_room_index( sysdata.room_limbo );
         }
         else
         {
            d->character->in_room = get_room_index( room );
            d->character->was_in_room = get_room_index( room );
         }
         if( !d->character->in_room )
            d->character->in_room = get_room_index( sysdata.room_temple );

         char_to_room( d->character, d->character->in_room );
         act( AT_MAGIC, "A puff of ethereal smoke dissipates around you!", d->character, NULL, NULL, TO_CHAR );
         act( AT_MAGIC, "$n appears in a puff of ethereal smoke!", d->character, NULL, NULL, TO_ROOM );
         d->connected = CON_PLAYING;
         if( ++num_descriptors > sysdata.maxplayers )
            sysdata.maxplayers = num_descriptors;
         handle_hostlog( "Recovered from Hotboot", d->character );
         write_to_descriptor( d, (const char *)will_compress2_str, 0 );
      }
   }
   fclose( fp );
   fp = NULL;
   if( maxp > sysdata.maxplayers )
      sysdata.maxplayers = maxp;
   log_string( "Hotboot recovery complete." );
}
