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
 *                     Low-level communication module                        *
 *****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "h/mud.h"
#include "h/mccp.h"
#include "h/sha256.h"
#include "h/mssp.h"

/* Socket and TCP/IP stuff. */
#ifdef WIN32
   #include <io.h>
   #include <winsock2.h>
   #undef EINTR
   #undef EMFILE
   #define EINTR WSAEINTR
   #define EMFILE WSAEMFILE
   #define EWOULDBLOCK WSAEWOULDBLOCK
   #define MAXHOSTNAMELEN 32

   #define  TELOPT_ECHO '\x01'
   #define  NOP         '\xF1'
   #define  GA          '\xF9'
   #define  SB          '\xFA'
   #define  WILL        '\xFB'
   #define  WONT        '\xFC'
   #define  DO          '\xFD'
   #define  DONT        '\xFE'
   #define  IAC         '\xFF'

   void bailout( void );
   void shutdown_checkpoint( void );
#else
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <netinet/in_systm.h>
   #include <netinet/ip.h>
   #include <arpa/inet.h>
   #include <arpa/telnet.h>
   #include <netdb.h>
#endif

#ifdef IMC
   void imc_delete_info( void );
   void free_imcdata( bool complete );
#endif

const unsigned char echo_off_str[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const unsigned char echo_on_str[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const unsigned char go_ahead_str[] = { IAC, GA, '\0' };
const unsigned char nop_str[] = { IAC, NOP, '\0' };

const char *keep_alive_msg[] =
{
   "This mud still has along way to go, before its completed.\r\n",
   "Feel free to make suggestions about ways to make the game better.\r\n",
   "Keeping link alive.\r\n"
};

void check_bti( CHAR_DATA *ch );

/* Global variables. */
DESCRIPTOR_DATA *first_descriptor, *last_descriptor;
PC_DATA *first_pc, *last_pc;
DESCRIPTOR_DATA *d_next;   /* Next descriptor in loop */
int num_descriptors;
bool mud_down; /* Shutdown       */
bool service_shut_down; /* Shutdown by operator closing down service */
time_t boot_time;
HOUR_MIN_SEC set_boot_time_struct;
HOUR_MIN_SEC *set_boot_time;
char str_boot_time[MIL];
char lastplayercmd[MIL * 2];
time_t current_time; /* Time of this pulse      */
int control;   /* Controlling descriptor  */
int newdesc;   /* New descriptor    */
fd_set in_set; /* Set of desc's for reading  */
fd_set out_set;   /* Set of desc's for writing  */
fd_set exc_set;   /* Set of desc's with errors  */
int maxdesc;
char *alarm_section = (char *)"(unknown)";

extern char *help_greeting;

int port;

/*
 * Clean all memory on exit to help find leaks
 * Yeah I know, one big ugly function -Druid
 * Added to AFKMud by Samson on 5-8-03.
 */
void cleanup_memory( void )
{
   CHAR_DATA *character;
   OBJ_DATA *object;
   DESCRIPTOR_DATA *desc, *desc_next;
   int hash, loopa;

#ifdef IMC
   fprintf( stderr, "%s", "IMC2 Data.\n" );
   free_imcdata( true );
   imc_delete_info( );
#endif

   fprintf( stderr, "%s", "Ban Data.\n" );
   free_bans( );

   fprintf( stderr, "%s", "Morph Data.\n" );
   free_morphs( );

   fprintf( stderr, "%s", "Commands.\n" );
   free_commands( );

   fprintf( stderr, "%s", "Deities.\n" );
   free_deities( );

   fprintf( stderr, "%s", "Clans.\n" );
   free_clans( );

   fprintf( stderr, "%s", "Councils.\n" );
   free_councils( );

   fprintf( stderr, "%s", "Socials.\n" );
   free_socials( );

   fprintf( stderr, "%s", "Helps.\n" );
   free_helps( );

   fprintf( stderr, "%s", "News.\n" );
   free_all_news( );

   stop_trivia( ); /* Stop trivia if it is running */

   fprintf( stderr, "%s", "Trivias.\n" );
   free_all_trivias( );

   fprintf( stderr, "%s", "HighScores.\n" );
   free_hightables( );

   fprintf( stderr, "%s", "Calendar.\n" );
   free_all_calendarinfo( );

   fprintf( stderr, "%s", "Mud Wide Resets.\n" );
   free_all_mwresets( );

   fprintf( stderr, "%s", "Reserved.\n" );
   free_all_reserved( );

   fprintf( stderr, "%s", "Bugs, Typos, and Ideas.\n" );
   free_all_bti( );

   fprintf( stderr, "%s", "Banks.\n" );
   free_all_banks( );

   fprintf( stderr, "%s", "MpDamages.\n" );
   free_all_mpdamages( );

   fprintf( stderr, "%s", "MSSP information.\n" );
   free_mssp_info( );

   fprintf( stderr, "%s", "Authorize data.\n" );
   free_all_auths( );

   fprintf( stderr, "%s", "Host Logs.\n" );
   free_all_hostlogs( );

   fprintf( stderr, "%s", "Locker Share Information.\n" );
   free_all_lockershare( );

   fprintf( stderr, "%s", "Rewards.\n" );
   free_rewards( );

   fprintf( stderr, "%s", "Languages.\n" );
   free_tongues( );

   fprintf( stderr, "%s", "Hints.\n" );
   free_hints( );

   fprintf( stderr, "%s", "Boards.\n" );
   free_boards( );

   fprintf( stderr, "%s", "Whacking supermob.\n" );
   if( supermob )
   {
      char_from_room( supermob );
      UNLINK( supermob, first_char, last_char, next, prev );
      UNLINK( supermob, supermob->pIndexData->first_copy, supermob->pIndexData->last_copy, next_index, prev_index );
      free_char( supermob );
   }

   fprintf( stderr, "%s", "Fish Names.\n" );
   free_all_fnames( );

   fprintf( stderr, "%s", "Fish.\n" );
   free_all_fish( );

   fprintf( stderr, "%s", "Objects.\n" );
   while( ( object = last_object ) )
      extract_obj( object );

   fprintf( stderr, "%s", "Characters.\n" );
   clean_char_queue( );
   while( ( character = last_char ) )
      extract_char( character, true );
   clean_char_queue( );

   fprintf( stderr, "%s", "Descriptors.\n" );
   for( desc = first_descriptor; desc; desc = desc_next )
   {
      desc_next = desc->next;
      UNLINK( desc, first_descriptor, last_descriptor, next, prev );
      free_desc( desc );
   }

   fprintf( stderr, "%s", "Groups.\n" );
   free_all_groups( );

   fprintf( stderr, "%s", "Races.\n" );
   for( hash = 0; hash < MAX_RACE; hash++ )
   {
      if( !race_table[hash] )
         continue;
      for( loopa = 0; loopa < MAX_WHERE_NAME; loopa++ )
      {
         STRFREE( race_table[hash]->where_name[loopa] );
         STRFREE( race_table[hash]->lodge_name[loopa] );
      }
      STRFREE( race_table[hash]->name );
      DISPOSE( race_table[hash] );
   }

   fprintf( stderr, "%s", "Classes.\n" );
   for( hash = 0; hash < MAX_CLASS; hash++ )
   {
      if( !class_table[hash] )
         continue;
      STRFREE( class_table[hash]->name );
      DISPOSE( class_table[hash] );
   }

   fprintf( stderr, "%s", "Teleport Data.\n" );
   free_teleports( );

   fprintf( stderr, "%s", "Area Data Tables.\n" );
   close_all_areas( );

   fprintf( stderr, "%s", "System data.\n" );
   STRFREE( sysdata.mud_name );

   fprintf( stderr, "%s", "Skills and Herbs.\n" );
   free_skills( );

   fprintf( stderr, "%s", "Mudprog act lists.\n" );
   free_prog_actlists( );

   fprintf( stderr, "%s", "Specfun lists.\n" );
   free_specfuns( );

   fprintf( stderr, "%s", "Globals.\n" );
   STRFREE( ranged_target_name );

   fprintf( stderr, "%s", "Transfer Data.\n" );
   free_transfer( );

   fprintf( stderr, "%s", "Channels.\n" );
   free_all_channels( );

   fprintf( stderr, "%s", "Checking string hash for leftovers.\n" );
   {
      bool found = false;

      for( hash = 0; hash < 1024; hash++ )
         if( hash_dump( hash ) )
            found = true;
      if( !found )
         fprintf( stderr, "%s", "Nothing was found left in the string hash.\n" );
   }

   fprintf( stderr, "%s", "Cleanup complete, exiting.\n" );
}  /* cleanup memory */

void init_socket( void )
{
   struct sockaddr_in sa;
   int x = 1;

   if( ( control = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
   {
      perror( "Init_socket: socket" );
      exit( 1 );
   }

   if( setsockopt( control, SOL_SOCKET, SO_REUSEADDR, ( void * )&x, sizeof( x ) ) < 0 )
   {
      perror( "Init_socket: SO_REUSEADDR" );
      close( control );
      exit( 1 );
   }

   memset( &sa, '\0', sizeof( sa ) );
   sa.sin_family = AF_INET;
   sa.sin_port = htons( port );

   if( bind( control, ( struct sockaddr * )&sa, sizeof( sa ) ) == -1 )
   {
      perror( "Init_socket: bind" );
      close( control );
      exit( 1 );
   }

   if( listen( control, SOMAXCONN ) < 0 )
   {
      perror( "Init_socket: listen" );
      close( control );
      exit( 1 );
   }
}

#ifdef WIN32
   int mainthread( int argc, char **argv )
#else
   int main( int argc, char **argv )
#endif
{
   struct timeval now_time;
#ifdef IMC
   int imcsocket = -1;
#endif
   bool fCopyOver = false;

   DONT_UPPER = false;
   num_descriptors = 0;
   mccpusers = 0;
   first_descriptor = NULL;
   last_descriptor = NULL;

   /* Init time. */
   gettimeofday( &now_time, NULL );
   current_time = ( time_t ) now_time.tv_sec;
   current_time += ( time_t ) TIME_MODIFY; /* Increase it to EDT */
   boot_time = time( 0 );  /*  <-- I think this is what you wanted */
   mudstrlcpy( str_boot_time, distime( current_time ), sizeof( str_boot_time ) );

   /* Init boot time. */
   set_boot_time = &set_boot_time_struct;
   set_boot_time->manual = 0;

   /* Get the port number. */
   port = 2700;
   if( argc > 1 )
   {
      if( !is_number( argv[1] ) )
      {
         fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
         exit( 1 );
      }
      else if( ( port = atoi( argv[1] ) ) <= 1024 )
      {
         fprintf( stderr, "Port number must be above 1024.\n" );
         exit( 1 );
      }
      if( argv[2] && argv[2][0] )
      {
         fCopyOver = true;
         control = atoi( argv[3] );
#ifdef IMC
         imcsocket = atoi( argv[4] );
#endif
      }
      else
         fCopyOver = false;
   }

   /* Run the game. */
#ifdef WIN32
   {
      /* Initialise Windows sockets library */
      unsigned short wVersionRequested = MAKEWORD( 1, 1 );
      WSADATA wsadata;
      int err;

      /* Need to include library: wsock32.lib for Windows Sockets */
      err = WSAStartup( wVersionRequested, &wsadata );
      if( err )
      {
         fprintf( stderr, "Error %i on WSAStartup\n", err );
         exit( 1 );
      }

      /* standard termination signals */
      signal( SIGINT, ( void * )bailout );
      signal( SIGTERM, ( void * )bailout );
   }
#endif /* WIN32 */

   log_string( "Booting Database" );
   boot_db( fCopyOver );
   log_string( "Initializing socket" );
   if( !fCopyOver )  /* We have already the port if copyover'ed */
      init_socket( );

#ifdef IMC
   /* Initialize and connect to IMC2 */
   imc_startup( false, imcsocket, fCopyOver );
#endif

   log_printf( "%s ready on port %d.", sysdata.mud_name, port );

   if( fCopyOver )
   {
      log_string( "Initiating hotboot recovery." );
      hotboot_recover( );
   }

   game_loop( );

   close( control );

#ifdef IMC
   imc_shutdown( false );
#endif

#ifdef WIN32
   /* Shut down Windows sockets */
   WSACleanup( ); /* clean up */
   kill_timer( ); /* stop timer thread */
#endif

   /* That's all, folks. */
   log_string( "Normal termination of game." );

   log_string( "Cleaning up Memory." );
   cleanup_memory( );

   exit( 0 );
}

/* LAG alarm! - Thoric */
void caught_alarm( int signum )
{
   bug( "%s: ALARM CLOCK!  In section %s", __FUNCTION__, alarm_section );
   echo_to_all( AT_IMMORT, "Alas, the hideous malevalent entity known only as 'Lag' rises once more!\r\n", ECHOTAR_ALL );
   if( newdesc )
   {
      FD_CLR( newdesc, &in_set );
      FD_CLR( newdesc, &out_set );
      FD_CLR( newdesc, &exc_set );
      log_string( "clearing newdesc" );
   }
}

bool check_bad_desc( int desc )
{
   if( FD_ISSET( desc, &exc_set ) )
   {
      FD_CLR( desc, &in_set );
      FD_CLR( desc, &out_set );
      log_string( "Bad FD caught and disposed." );
      return true;
   }
   return false;
}

void accept_new( void )
{
   static struct timeval null_time;
   DESCRIPTOR_DATA *d;

   FD_ZERO( &in_set );
   FD_ZERO( &out_set );
   FD_ZERO( &exc_set );

   FD_SET( control, &in_set );

   maxdesc = control;

   for( d = first_descriptor; d; d = d->next )
   {
      maxdesc = UMAX( maxdesc, d->descriptor );
      FD_SET( d->descriptor, &in_set );
      FD_SET( d->descriptor, &out_set );
      FD_SET( d->descriptor, &exc_set );
   }

   if( select( maxdesc + 1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
   {
      perror( "accept_new: select: poll" );
      exit( 1 );
   }

   if( FD_ISSET( control, &exc_set ) )
   {
      bug( "Exception raise on controlling descriptor %d", control );
      FD_CLR( control, &in_set );
      FD_CLR( control, &out_set );
   }
   else if( FD_ISSET( control, &in_set ) )
      new_descriptor( );
}

void game_loop( void )
{
   DESCRIPTOR_DATA *d;
   struct timeval last_time;
   char cmdline[MIL];

#ifndef WIN32
   signal( SIGPIPE, SIG_IGN );
   signal( SIGALRM, caught_alarm );
#endif

   fprintf( stderr, ".~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~[  Game Loop  ]~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\n" );

   gettimeofday( &last_time, NULL );
   current_time = ( time_t )last_time.tv_sec + TIME_MODIFY;

   /* Main loop */
   while( !mud_down )
   {
      accept_new( );

      /*
       * Kick out descriptors with raised exceptions
       * or have been idle, then check for input.
       */
      for( d = first_descriptor; d; d = d_next )
      {
         d_next = d->next;

         if( ++d->tempidle >= PULSE_PER_SECOND )
         {
            d->idle++;  /* make it so a descriptor can idle out */
            d->tempidle = 0;
         }
         if( d->character && ++d->character->temp_played >= PULSE_PER_SECOND )
         {
            if( d->idle <= 0 ) /* If they are idle they aren't actually playing */
               d->character->played++; /* Increase played time (goes by seconds) */
            d->character->temp_played = 0;
         }
         if( FD_ISSET( d->descriptor, &exc_set ) )
         {
            FD_CLR( d->descriptor, &in_set );
            FD_CLR( d->descriptor, &out_set );
            if( d->character && ( d->connected == CON_PLAYING || d->connected == CON_EDITING ) )
               save_char_obj( d->character );
            d->outtop = 0;
            close_socket( d, true );
            continue;
         }
         else if( ( !d->character && d->idle > 60 )  /* 1 minute */
         || ( d->connected != CON_PLAYING && d->idle > 180 )  /* 3 minutes */
         || ( d->idle > 3600 && get_trust( d->character ) < PERM_IMM ) ) /* 1 hour */
         {
            write_to_descriptor( d, "Idle timeout... disconnecting.\r\n", 0 );
            d->outtop = 0;
            close_socket( d, true );
            continue;
         }
         else
         {
            d->fcommand = false;

            if( FD_ISSET( d->descriptor, &in_set ) )
            {
               d->idle = 0;
               d->tempidle = 0;
               if( d->character )
                  d->character->timer = 0;
               if( !read_from_descriptor( d ) )
               {
                  FD_CLR( d->descriptor, &out_set );
                  if( d->character && ( d->connected == CON_PLAYING || d->connected == CON_EDITING ) )
                     save_char_obj( d->character );
                  d->outtop = 0;
                  close_socket( d, false );
                  continue;
               }
            }

            if( d->character && d->character->wait > 0 )
            {
               --d->character->wait;
               continue;
            }

            read_from_buffer( d );
            if( d->incomm[0] && d->incomm[0] != '\0' )
            {
               d->fcommand = true;
               stop_idling( d->character );

               mudstrlcpy( cmdline, d->incomm, sizeof( cmdline ) );
               d->incomm[0] = '\0';

               if( d->character )
                  set_cur_char( d->character );

               if( d->pagepoint )
                  set_pager_input( d, cmdline );
               else
               {
                  switch( d->connected )
                  {
                     default:
                        nanny( d, cmdline );
                        break;

                     case CON_PLAYING:
                        interpret( d->character, cmdline );
                        break;

                     case CON_EDITING:
                        edit_buffer( d->character, cmdline );
                        break;
                  }
               }
            }
            /* later should add in a config for getting the message and time in mins to display it */
            else if( d->character && d->character->pcdata && xIS_SET( d->character->act, PLR_K_LIVE )
            && d->outtime < ( current_time - ( 60 * URANGE( 1, d->character->pcdata->kltime, 10 ) ) ) )
               write_to_descriptor( d, keep_alive_msg[number_range( 0, 2 )], 0 );
         }
         if( d == last_descriptor )
            break;
      }

#ifdef IMC
      imc_loop( );
#endif

      /* Autonomous game motion. */
      update_handler( );

      /* Output. */
      for( d = first_descriptor; d; d = d_next )
      {
         d_next = d->next;

         if( ( d->fcommand || d->outtop > 0 ) && FD_ISSET( d->descriptor, &out_set ) )
         {
            if( d->pagepoint )
            {
               if( !pager_output( d ) )
               {
                  if( d->character && ( d->connected == CON_PLAYING || d->connected == CON_EDITING ) )
                     save_char_obj( d->character );
                  d->outtop = 0;
                  close_socket( d, false );
               }
            }
            else if( !flush_buffer( d, true ) )
            {
               if( d->character && ( d->connected == CON_PLAYING || d->connected == CON_EDITING ) )
                  save_char_obj( d->character );
               d->outtop = 0;
               close_socket( d, false );
            }
         }
         if( d == last_descriptor )
            break;
      }

      /*
       * Synchronize to a clock.
       * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
       * Careful here of signed versus unsigned arithmetic.
       */
      {
         struct timeval now_time;
         long secDelta;
         long usecDelta;

         gettimeofday( &now_time, NULL );
         usecDelta = ( ( int )last_time.tv_usec ) - ( ( int )now_time.tv_usec ) + 1000000 / PULSE_PER_SECOND;
         secDelta = ( ( int )last_time.tv_sec ) - ( ( int )now_time.tv_sec );
         while( usecDelta < 0 )
         {
            usecDelta += 1000000;
            secDelta -= 1;
         }

         while( usecDelta >= 1000000 )
         {
            usecDelta -= 1000000;
            secDelta += 1;
         }

         if( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
         {
            struct timeval stall_time;

            stall_time.tv_usec = usecDelta;
            stall_time.tv_sec = secDelta;
#ifdef WIN32
            Sleep( ( stall_time.tv_sec * 1000L ) + ( stall_time.tv_usec / 1000L ) );
#else
            if( select( 0, NULL, NULL, NULL, &stall_time ) < 0 && errno != EINTR )
            {
               perror( "game_loop: select: stall" );
               exit( 1 );
            }
#endif
         }
      }

      gettimeofday( &last_time, NULL );
      current_time = ( time_t ) last_time.tv_sec;
      current_time += ( time_t ) TIME_MODIFY;
   }

   fflush( stderr ); /* make sure strerr is flushed */
}

void new_descriptor( void )
{
   DESCRIPTOR_DATA *dnew;
   char buf[MSL];
   char log_buf[MSL];
   struct sockaddr_in sock;
   struct hostent *from;
#ifndef WIN32
   socklen_t size;
#else
   unsigned int size;
   unsigned long arg = 1;
#endif
   int desc;

   size = sizeof( sock );
   if( check_bad_desc( control ) )
   {
      set_alarm( 0 );
      return;
   }

   set_alarm( 20 );
   alarm_section = (char *)"new_descriptor::accept";
   if( ( desc = accept( control, ( struct sockaddr * )&sock, &size ) ) < 0 )
   {
      perror( "New_descriptor: accept" );
      bug( "%s: accept", __FUNCTION__ );
      set_alarm( 0 );
      return;
   }

   if( check_bad_desc( control ) )
   {
      set_alarm( 0 );
      return;
   }

#if !defined(FNDELAY)
   #define FNDELAY O_NDELAY
#endif

   set_alarm( 20 );
   alarm_section = (char *)"new_descriptor: after accept";

#ifdef WIN32
   if( ioctlsocket( desc, FIONBIO, &arg ) == -1 )
#else
   if( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
#endif
   {
      perror( "New_descriptor: fcntl: FNDELAY" );
      set_alarm( 0 );
      return;
   }
   if( check_bad_desc( control ) )
      return;

   CREATE( dnew, DESCRIPTOR_DATA, 1 );
   dnew->next = NULL;
   dnew->descriptor = desc;
   dnew->connected = CON_GET_NAME;
   dnew->outsize = 2000;
   dnew->idle = 0;
   dnew->tempidle = 0;
   dnew->lines = 0;
   dnew->scrlen = 24;
   dnew->port = ntohs( sock.sin_port );
   dnew->newstate = 0;
   dnew->prevcolor = 0x07;
   dnew->can_compress = false;
   dnew->speed = 32;
   CREATE( dnew->mccp, MCCP, 1 );
   CREATE( dnew->outbuf, char, dnew->outsize );

   mudstrlcpy( buf, inet_ntoa( sock.sin_addr ), sizeof( buf ) );
   if( sysdata.NAME_RESOLVING )
   {
      from = gethostbyaddr( ( char * )&sock.sin_addr, sizeof( sock.sin_addr ), AF_INET );
      dnew->host = STRALLOC( ( from ? from->h_name : buf ) );
   }
   else
      dnew->host = STRALLOC( buf );

   log_printf_plus( LOG_COMM, PERM_HEAD, "Host: %s, Port: %d.", dnew->host, dnew->port );

   if( multi_check( dnew, false ) )
   {
      free_desc( dnew );
      set_alarm( 0 );
      return;
   }

   if( check_total_bans( dnew ) )
   {
      write_to_descriptor( dnew, "Your site has been banned from this Mud.\r\n", 0 );
      free_desc( dnew );
      set_alarm( 0 );
      return;
   }

   LINK( dnew, first_descriptor, last_descriptor, next, prev );

   /* MCCP Compression */
   write_to_buffer( dnew, (const char *)will_compress2_str, 0 );

   /* Send the greeting. */
   if( !help_greeting || help_greeting[0] == '\0' )
      send_to_desc( "There is currently no greeting file to display.\r\n", dnew );
   else if( help_greeting[0] == '.' )
      send_to_desc( help_greeting + 1, dnew );
   else
      send_to_desc( help_greeting, dnew );

   if( ++num_descriptors > sysdata.maxplayers )
      sysdata.maxplayers = num_descriptors;
   if( sysdata.maxplayers > sysdata.alltimemax )
   {
      sysdata.time_of_max = current_time;
      sysdata.alltimemax = sysdata.maxplayers;
      snprintf( log_buf, sizeof( log_buf ), "Broke all-time maximum player record: %d", sysdata.alltimemax );
      log_string_plus( log_buf, LOG_COMM, sysdata.perm_log );
      to_channel( log_buf, "monitor", PERM_IMM );
      save_sysdata( false );
   }
   set_alarm( 0 );
}

void free_desc( DESCRIPTOR_DATA *d )
{
   compressEnd( d );
   close( d->descriptor );
   STRFREE( d->host );
   DISPOSE( d->outbuf );
   DISPOSE( d->pagebuf );
   DISPOSE( d->mccp );
   DISPOSE( d );
}

void close_socket( DESCRIPTOR_DATA *dclose, bool force )
{
   CHAR_DATA *ch;
   DESCRIPTOR_DATA *d;
   bool DoNotUnlink = false;

   /* flush outbuf */
   if( !force && dclose->outtop > 0 )
      flush_buffer( dclose, false );

   /* say bye to whoever's snooping this descriptor */
   if( dclose->snoop_by )
      write_to_buffer( dclose->snoop_by, "Your victim has left the game.\r\n", 0 );

   /* stop snooping everyone else */
   for( d = first_descriptor; d; d = d->next )
      if( d->snoop_by == dclose )
         d->snoop_by = NULL;

   if( ( ch = dclose->character ) )
   {
      log_printf_plus( LOG_COMM, get_trust( ch ), "Closing link to %s.", ch->pcdata->filename );
      handle_hostlog( "Closing Link", ch );
      if( dclose->connected == CON_PLAYING || dclose->connected == CON_EDITING )
      {
         act( AT_ACTION, "$n has lost $s link.", ch, NULL, NULL, TO_CANSEE );
         ch->desc = NULL;
      }
      else
      {
         ch->desc = NULL;
         free_char( ch );
      }
   }

   if( !DoNotUnlink )
   {
      if( d_next == dclose )
         d_next = d_next->next;
      UNLINK( dclose, first_descriptor, last_descriptor, next, prev );
   }

   compressEnd( dclose );

   if( dclose->descriptor == maxdesc )
      --maxdesc;

   free_desc( dclose );
   --num_descriptors;
}

bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
   unsigned int iStart;
   int iErr;

   /* Hold horses if pending command already. */
   if( d->incomm[0] != '\0' )
      return true;

   /* Check for overflow. */
   iStart = strlen( d->inbuf );
   if( iStart >= sizeof( d->inbuf ) - 10 )
   {
      log_printf( "%s input overflow!", d->host );
      write_to_descriptor( d, "\r\n*** PUT A LID ON IT!!! ***\r\nYou can't enter the same command more than 20 consecutive times!\r\n", 0 );
      return false;
   }

   for( ;; )
   {
      int nRead;

      nRead = recv( d->descriptor, d->inbuf + iStart, sizeof( d->inbuf ) - 10 - iStart, 0 );
#ifdef WIN32
      iErr = WSAGetLastError( );
#else
      iErr = errno;
#endif
      if( nRead > 0 )
      {
         iStart += nRead;

         /* Update the incomm here before adding more to the line etc */
         update_transfer( 1, nRead );

         if( d->inbuf[iStart - 1] == '\r' || d->inbuf[iStart - 1] == '\n' )
            break;

         /* Reached limit so have to let it carry on */
         if( iStart >= ( sizeof( d->inbuf ) - 10 ) )
            break;
      }
      else if( nRead == 0 )
      {
         if( d->character && d->character->name )
         {
            log_printf_plus( LOG_COMM, get_trust( d->character ), "EOF encountered on read for %s.", d->character->name );
            handle_hostlog( "EOF encountered", d->character );
         }
         else if( d->host )
            log_printf_plus( LOG_COMM, PERM_HEAD, "EOF encountered on read from %s.", d->host );
         else
            log_string_plus( "EOF encountered on read.", LOG_COMM, PERM_HEAD );
         return false;
      }
      else if( iErr == EWOULDBLOCK )
         break;
      else
      {
         perror( "Read_from_descriptor" );
         return false;
      }
   }

   d->inbuf[iStart] = '\0';
   return true;
}

/* Transfer one line from input buffer to input line. */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
   int i, j, k, iac = 0;

   /* Hold horses if pending command already. */
   if( d->incomm[0] != '\0' )
      return;

   /* Look for at least one new line. */
   for( i = 0; d->inbuf[i] != '\r' && d->inbuf[i] != '\n' && i < MIS; i++ )
   {
      if( d->inbuf[i] == '\0' )
         return;
   }

   /* Canonical input processing. */
   for( i = 0, k = 0; d->inbuf[i] != '\r' && d->inbuf[i] != '\n'; i++ )
   {
      if( k >= MIL )
      {
         write_to_descriptor( d, "Line too long.\r\n", 0 );
         d->inbuf[i] = '\n';
         d->inbuf[i + 1] = '\0';
         break;
      }

      if( d->inbuf[i] == ( signed char )IAC )
         iac = 1;
      else if( iac == 1
      && ( d->inbuf[i] == ( signed char )DO || d->inbuf[i] == ( signed char )DONT || d->inbuf[i] == ( signed char )WILL ) )
         iac = 2;
      else if( iac == 2 )
      {
         iac = 0;
         if( d->inbuf[i] == ( signed char )TELOPT_COMPRESS2 )
         {
            if( d->inbuf[i - 1] == ( signed char )DO )
               compressStart( d );
            else if( d->inbuf[i - 1] == ( signed char )DONT )
               compressEnd( d );
         }
      }
      else if( d->inbuf[i] == '\b' && k > 0 )
         --k;
      else if( isascii( d->inbuf[i] ) && isprint( d->inbuf[i] ) )
         d->incomm[k++] = d->inbuf[i];
   }

   /* Finish off the line. */
   if( k == 0 )
      d->incomm[k++] = ' ';
   d->incomm[k] = '\0';

   /* Deal with bozos with #repeat 1000 ... */
   if( k > 1 || d->incomm[0] == '!' )
   {
      if( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
         d->repeat = 0;
      else
      {
         if( ++d->repeat >= 20 )
         {
            write_to_descriptor( d, "\r\n*** PUT A LID ON IT!!! ***\r\nYou can't enter the same command more than 20 consecutive times!\r\n", 0 );
            mudstrlcpy( d->incomm, "quit", sizeof( d->incomm ) );
         }
      }
   }

   /* Do '!' substitution. */
   if( d->incomm[0] == '!' )
      mudstrlcpy( d->incomm, d->inlast, sizeof( d->incomm ) );
   else
      mudstrlcpy( d->inlast, d->incomm, sizeof( d->inlast ) );

   /* Shift the input buffer. */
   while( d->inbuf[i] == '\r' || d->inbuf[i] == '\n' )
      i++;
   for( j = 0; ( d->inbuf[j] = d->inbuf[i + j] ) != '\0'; j++ )
      ;
}

/* Low level output function. */
bool flush_buffer( DESCRIPTOR_DATA *d, bool fPrompt )
{
   char buf[MSL];
   int temp, size;

   if( !d )
   {
      bug( "%s: NULL descriptor.", __FUNCTION__ );
      return false;
   }

   temp = URANGE( 1, d->speed, 32 );
   size = URANGE( 128, ( 128 * temp ), ( MSL - 1 ) );

   /* If buffer has more than <size> inside, spit out .256K at a time */
   if( !mud_down && d->outtop > size )
   {
      memcpy( buf, d->outbuf, size );
      d->outtop -= size;
      memmove( d->outbuf, d->outbuf + size, d->outtop );
      if( d->snoop_by )
      {
         char snoopbuf[MSL];

         if( d->character && d->character->name )
         {
            snprintf( snoopbuf, sizeof( snoopbuf ), "%s", d->character->name );
            write_to_buffer( d->snoop_by, snoopbuf, 0 );
         }
         write_to_buffer( d->snoop_by, "% ", 2 );
         write_to_buffer( d->snoop_by, buf, 0 );
      }
      if( !write_to_descriptor( d, buf, size ) )
      {
         d->outtop = 0;
         return false;
      }
      return true;
   }

   /* Bust a prompt. */
   if( fPrompt && !mud_down && d->connected == CON_PLAYING )
   {
      CHAR_DATA *ch = NULL;

      ch = d->character;
      if( ch && !is_npc( ch ) && xIS_SET( ch->act, PLR_ANSI ) )
      {
         write_to_buffer( d, ANSI_RESET, 0 );
         d->prevcolor = 0x08;
      }

      if( xIS_SET( ch->act, PLR_BLANK ) )
         write_to_buffer( d, "\r\n", 2 );

      if( xIS_SET( ch->act, PLR_PROMPT ) )
         display_prompt( d );

      if( ch && !is_npc( ch ) && xIS_SET( ch->act, PLR_ANSI ) )
      {
         write_to_buffer( d, ANSI_RESET, 0 );
         d->prevcolor = 0x08;
      }

      if( xIS_SET( ch->act, PLR_TELNET_GA ) )
         write_to_buffer( d, (const char *)go_ahead_str, 0 );

      if( xIS_SET( ch->act, PLR_BLANK ) && d->outtop > 0 )
         write_to_buffer( d, "\r\n", 2 );
   }

   /* Short-circuit if nothing to write. */
   if( d->outtop == 0 )
      return true;

   /* Snoop-o-rama. */
   if( d->snoop_by )
   {
      /* without check, 'force mortal quit' while snooped caused crash, -h */
      if( d->character && d->character->name )
      {
         snprintf( buf, sizeof( buf ), "%s", d->character->name );
         write_to_buffer( d->snoop_by, buf, 0 );
      }
      write_to_buffer( d->snoop_by, "% ", 2 );
      write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
   }

   /* OS-dependent output. */
   if( !write_to_descriptor( d, d->outbuf, d->outtop ) )
   {
      d->outtop = 0;
      return false;
   }
   else
   {
      d->outtop = 0;
      return true;
   }
}

/* Append onto an output buffer. */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, unsigned int length )
{
   if( !d )
   {
      bug( "%s: NULL descriptor", __FUNCTION__ );
      return;
   }

   /* Normally a bug... but can happen if loadup is used. */
   if( !d->outbuf )
      return;

   /* Find length in case caller didn't. */
   if( length <= 0 )
      length = strlen( txt );

   /* Initial \r\n if needed. */
   if( d->outtop == 0 && !d->fcommand )
   {
      d->outbuf[0] = '\r';
      d->outbuf[1] = '\n';
      d->outtop = 2;
   }

   /* Expand the buffer as needed. */
   while( d->outtop + length >= d->outsize )
   {
      if( d->outsize > 32000 )
      {
         /* empty buffer */
         d->outtop = 0;

         /* Bugfix by Samson - moved bug() call up */
         bug( "Buffer overflow. Closing (%s)@%s.", d->character ? d->character->name : "???",
            d->host ? d->host : "???" );
         close_socket( d, true );
         return;
      }
      d->outsize *= 2;
      RECREATE( d->outbuf, char, d->outsize );
   }

   /* Copy. */
   strncpy( d->outbuf + d->outtop, txt, length );
   d->outtop += length;
   d->outbuf[d->outtop] = '\0';
}

/*
 * Added block checking to prevent random booting of the descriptor. Thanks go
 * out to Rustry for his suggestions. -Orion
 */
bool write_to_descriptor_old( int desc, const char *txt, int length )
{
   int iStart = 0, nWrite = 0, nBlock = 0, iErr = 0;

   if( length <= 0 )
      length = strlen( txt );

   for( iStart = 0; iStart < length; iStart += nWrite )
   {
      nBlock = UMIN( length - iStart, 4096 );
      nWrite = send( desc, txt + iStart, nBlock, 0 );

      if( nWrite > 0 )
         update_transfer( 2, nWrite );

      if( nWrite == -1 )
      {
         iErr = errno;
         if( iErr == EWOULDBLOCK )
         {
            nWrite = 0;
            continue;
         }
         else
         {
            perror( "Write_to_descriptor" );
            return false;
         }
      }
   }
   return true;
}

/*
 * This is the MCCP version. Use write_to_descriptor_old to send non-compressed
 * text.
 * Updated to run with the block checks by Orion... if it doesn't work, blame
 * him.;P -Orion
 */
bool write_to_descriptor( DESCRIPTOR_DATA *d, const char *txt, int length )
{
   int iStart = 0, nWrite = 0, nBlock, iErr, len;

   if( length <= 0 )
      length = strlen( txt );

   if( d && d->mccp->out_compress )
   {
      size_t mccpsaved = length;

      /* Won't send more then it has to so make sure we check if its under length */
      if( mccpsaved > strlen( txt ) )
         mccpsaved = strlen( txt );

      d->mccp->out_compress->next_in = ( unsigned char * )txt;
      d->mccp->out_compress->avail_in = length;

      while( d->mccp->out_compress->avail_in )
      {
         d->mccp->out_compress->avail_out = COMPRESS_BUF_SIZE - ( d->mccp->out_compress->next_out - d->mccp->out_compress_buf );

         if( d->mccp->out_compress->avail_out )
         {
            int status = deflate( d->mccp->out_compress, Z_SYNC_FLUSH );

            if( status != Z_OK )
               return false;
         }

         len = d->mccp->out_compress->next_out - d->mccp->out_compress_buf;
         if( len > 0 )
         {
            for( iStart = 0; iStart < len; iStart += nWrite )
            {
               nBlock = UMIN( len - iStart, MSL );
               nWrite = send( d->descriptor, d->mccp->out_compress_buf + iStart, nBlock, 0 );

               if( nWrite > 0 )
               {
                  update_transfer( 3, nWrite );
                  mccpsaved -= nWrite;
               }

               if( nWrite == -1 )
               {
                  iErr = errno;
                  perror( "Write_to_descriptor" );
                  if( iErr == EWOULDBLOCK )
                  {
                     nWrite = 0;
                     continue;
                  }
                  else
                     return false;
               }

               if( !nWrite )
                  break;
            }

            if( !iStart )
               break;

            if( iStart < len )
               memmove( d->mccp->out_compress_buf, d->mccp->out_compress_buf + iStart, len - iStart );

            d->mccp->out_compress->next_out = d->mccp->out_compress_buf + len - iStart;
         }
      }
      d->outtime = current_time;
      if( mccpsaved > 0 )
         update_transfer( 4, mccpsaved );
      return true;
   }

   if( !write_to_descriptor_old( d->descriptor, txt, length ) )
      return false;
   if( d )
      d->outtime = current_time;
   return true;
}

void show_title( DESCRIPTOR_DATA *d )
{
   CHAR_DATA *ch;

   ch = d->character;
   if( ch && !xIS_SET( ch->pcdata->flags, PCFLAG_NOINTRO ) )
   {
      if( xIS_SET( ch->act, PLR_ANSI ) )
         send_ansi_title( ch );
      else
         send_ascii_title( ch );
   }
   else
      write_to_buffer( d, "Press enter...\r\n", 0 );
   d->connected = CON_PRESS_ENTER;
}

bool can_use_class( DESCRIPTOR_DATA *d, int iclass )
{
   MCLASS_DATA *mclass;

   if( !d || !d->character )
      return false;
   if( iclass < 0 || iclass >= MAX_PC_CLASS )
      return false;
   if( !class_table[iclass] || !class_table[iclass]->name || !str_cmp( class_table[iclass]->name, "unused" ) )
      return false;
   if( d->character && d->character->pcdata )
   {
      for( mclass = d->character->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         /* If they already have this class they can't use it */
         if( iclass == mclass->wclass )
            return false;
         /* If a class they already have doesn't allow iclass they can't use it */
         if( xIS_SET( class_table[mclass->wclass]->class_restriction, iclass ) )
            return false;
         /* If the class they want doesn't allow ones they already have they can't use it */
         if( xIS_SET( class_table[iclass]->class_restriction, mclass->wclass ) )
            return false;
      }
   }
   if( check_class_bans( iclass ) )
      return false;
   return true;
}

char *send_class_list( DESCRIPTOR_DATA *d )
{
   char buf[MSL], *sbuf;
   int iClass;
   bool found = false;

   buf[0] = '\0';
   for( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )
   {
      if( !can_use_class( d, iClass ) )
         continue;
      if( !found && d->character && d->character->pcdata && d->character->pcdata->first_mclass )
         mudstrlcat( buf, "None ", sizeof( buf ) );
      if( found )
         mudstrlcat( buf, " ", sizeof( buf ) );
      found = true;
      mudstrlcat( buf, class_table[iClass]->name, sizeof( buf ) );
   }
   sbuf = buf;
   return sbuf;
}

bool can_use_race( DESCRIPTOR_DATA *d, int iRace )
{
   MCLASS_DATA *mclass;

   if( !d || !d->character )
      return false;
   if( iRace < 0 || iRace >= MAX_PC_RACE )
      return false;
   if( !race_table[iRace] || !race_table[iRace]->name || !str_cmp( race_table[iRace]->name, "unused" ) )
      return false;
   if( d->character && d->character->pcdata )
   {
      for( mclass = d->character->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( xIS_SET( race_table[iRace]->class_restriction, mclass->wclass ) )
            return false;
      }
   }
   if( check_race_bans( iRace ) )
      return false;
   return true;
}

char *send_race_list( DESCRIPTOR_DATA *d )
{
   char buf[MSL], *sbuf;
   int iRace;
   bool found = false;

   buf[0] = '\0';
   if( !d )
      return (char *)"";
   for( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
   {
      if( !can_use_race( d, iRace ) )
         continue;
      if( found )
         mudstrlcat( buf, " ", sizeof( buf ) );
      found = true;
      mudstrlcat( buf, race_table[iRace]->name, sizeof( buf ) );
   }
   sbuf = buf;
   return sbuf;
}

/* Deal with sockets that haven't logged in yet. */
void nanny( DESCRIPTOR_DATA *d, char *argument )
{
   CHAR_DATA *ch;
   MCLASS_DATA *mclass;
   char buf[MSL], arg[MSL], log_buf[MSL], *pwdnew;
   int iRace, iClass;
   bool fOld, chk, redo, skipclass = sysdata.skipclasses, mcdone = false;

   while( isspace( *argument ) )
      argument++;

   ch = d->character;

   switch( d->connected )
   {
      default:
         bug( "%s: bad d->connected %d.", __FUNCTION__, d->connected );
         close_socket( d, true );
         return;

      case CON_GET_NAME:
         if( !argument || argument[0] == '\0' )
         {
            close_socket( d, false );
            return;
         }

         argument[0] = UPPER( argument[0] );

         if( !str_cmp( argument, "MSSP-REQUEST" ) )
         {
            send_mssp_data( d );
            //Uncomment below if you want to know when an MSSP request occurs
            //log_printf( "IP: %s requested MSSP data!", d->host );
            close_socket( d, false );
            return;
         }

         if( !str_prefix( "GET / HTTP/", argument ) )
         {
            close_socket( d, false );
            return;
         }

         /* Old players can keep their characters. -- Alty */
         if( !check_parse_name( argument, ( d->newstate != 0 ) ) )
         {
            write_to_buffer( d, argument, 0 );
            write_to_buffer( d, " is an Illegal name, try another.\r\nName: ", 0 );
            return;
         }

         if( !str_cmp( argument, "New" ) )
         {
            if( d->newstate == 0 )
            {
               /* New player. Don't allow new players if DENY_NEW_PLAYERS is true */
               if( sysdata.DENY_NEW_PLAYERS )
               {
                  write_to_buffer( d, "The mud is currently preparing for a reboot.\r\n", 0 );
                  write_to_buffer( d, "New players aren't accepted during this time.\r\n", 0 );
                  write_to_buffer( d, "Please try again in a few minutes.\r\n", 0 );
                  close_socket( d, false );
                  return;
               }
               write_to_buffer( d, "\r\nChoosing a name is one of the most important parts of this game...\r\n"
                                "Make sure to pick a name appropriate to the character you're going\r\n"
                                "to role play, and be sure that it suits a medieval theme.\r\n"
                                "If the name you select is not acceptable, you will be asked to choose\r\n"
                                "another one.\r\n\r\nPlease choose a name for your character: ", 0 );
               d->newstate++;
               d->connected = CON_GET_NAME;
               return;
            }
            else
            {
               write_to_buffer( d, "Illegal name, try another.\r\nName: ", 0 );
               return;
            }
         }

         fOld = load_char_obj( d, argument, true, false );
         if( !d->character )
         {
            log_printf( "Bad player file %s@%s.", argument, d->host );
            write_to_buffer( d, "Your playerfile is corrupt...Please notify an Immortal.\r\n", 0 );
            close_socket( d, false );
            return;
         }

         ch = d->character;
         if( check_bans( ch, BAN_SITE ) )
         {
            write_to_buffer( d, "Your site has been banned from this Mud.\r\n", 0 );
            close_socket( d, false );
            return;
         }

         if( fOld )
         {
            if( check_bans( ch, BAN_CLASS ) )
            {
               write_to_buffer( d, "Your class has been banned from this Mud.\r\n", 0 );
               close_socket( d, false );
               return;
            }
            if( check_bans( ch, BAN_RACE ) )
            {
               write_to_buffer( d, "Your race has been banned from this Mud.\r\n", 0 );
               close_socket( d, false );
               return;
            }
         }

         if( xIS_SET( ch->act, PLR_DENY ) )
         {
            log_printf_plus( LOG_COMM, PERM_HEAD, "Denying access to %s@%s.", argument, d->host );
            if( d->newstate != 0 )
            {
               write_to_buffer( d, "That name is already taken.  Please choose another: ", 0 );
               d->connected = CON_GET_NAME;
               d->character->desc = NULL;
               free_char( d->character ); /* Big Memory Leak before --Shaddai */
               d->character = NULL;
               return;
            }
            write_to_buffer( d, "You're denied access.\r\n", 0 );
            close_socket( d, false );
            return;
         }

         /* Make sure the character can be loaded from the host. */
         if( !check_host( ch ) )
         {
            snprintf( buf, sizeof( buf ), "%s's char being hacked from %s.", argument, d->host );
            log_printf_plus( LOG_COMM, PERM_HEAD, "%s", buf );
            handle_hostlog( "Hacked", ch );
            append_to_file( HACKED_FILE, buf ); /* Keep a good log of all hacking attempts */
            write_to_buffer( d, "This hacking attempt has been logged.\r\n", 0 );
            close_socket( d, false );
            return;
         }

         if( ( chk = check_reconnect( d, argument, false ) ) )
            fOld = true;
         else
         {
            if( sysdata.wizlock && !is_immortal( ch ) )
            {
               write_to_buffer( d, "The game is wizlocked.  Only immortals can connect now.\r\n", 0 );
               write_to_buffer( d, "Please try back later.\r\n", 0 );
               close_socket( d, false );
               return;
            }
            if( multi_check( d, true ) )
            {
               close_socket( d, false );
               return;
            }
         }

         if( fOld )
         {
            if( d->newstate != 0 )
            {
               write_to_buffer( d, "That name is already taken.  Please choose another: ", 0 );
               d->connected = CON_GET_NAME;
               d->character->desc = NULL;
               free_char( d->character ); /* Big Memory Leak before --Shaddai */
               d->character = NULL;
               return;
            }
            /* Old player */
            write_to_buffer( d, "Password: ", 0 );
            write_to_buffer( d, (const char *)echo_off_str, 0 );
            d->connected = CON_GET_OLD_PASSWORD;
            return;
         }
         else
         {
            if( d->newstate == 0 )
            {
               /* No such player */
               write_to_buffer( d, "\r\nNo such player exists.\r\nPlease check your spelling, or type new to start a new player.\r\n\r\nName: ", 0 );
               d->connected = CON_GET_NAME;
               d->character->desc = NULL;
               free_char( d->character ); /* Big Memory Leak before --Shaddai */
               d->character = NULL;
               return;
            }

            snprintf( buf, sizeof( buf ), "Did I get that right, %s (Y/N)? ", argument );
            write_to_buffer( d, buf, 0 );
            d->connected = CON_CONFIRM_NEW_NAME;
            return;
         }
         break;

      case CON_GET_OLD_PASSWORD:
         write_to_buffer( d, "\r\n", 2 );

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
         {
            write_to_buffer( d, "Wrong password, disconnecting.\r\n", 0 );
            /* clear descriptor pointer to get rid of bug message in log */
            d->character->desc = NULL;
            close_socket( d, false );
            return;
         }

         write_to_buffer( d, (const char *)echo_on_str, 0 );

         if( check_playing( d, ch->pcdata->filename, true ) )
            return;

         if( ( chk = check_reconnect( d, ch->pcdata->filename, true ) ) )
            return;

         if( multi_check( d, true ) )
         {
            d->character->desc = NULL;
            close_socket( d, false );
            return;
         }

         mudstrlcpy( buf, ch->pcdata->filename, sizeof( buf ) );
         d->character->desc = NULL;
         free_char( d->character );
         d->character = NULL;
         fOld = load_char_obj( d, buf, false, false );
         ch = d->character;
         if( is_fighting( ch ) )
            ch->position = POS_STANDING;
         fix_hhf( ch );
         log_printf_plus( LOG_COMM, get_trust( ch ), "%s (%s) has connected.", ch->name, d->host );
         handle_hostlog( "Connected", ch );
         show_title( d );
         break;

      case CON_CONFIRM_NEW_NAME:
         switch( *argument )
         {
            case 'y':
            case 'Y':
               snprintf( buf, sizeof( buf ),
                         "\r\nMake sure to use a password that won't be easily guessed by someone else."
                         "\r\nPick a good password for %s: %s", ch->name, echo_off_str );
               write_to_buffer( d, buf, 0 );
               d->connected = CON_GET_NEW_PASSWORD;
               break;

            case 'n':
            case 'N':
               write_to_buffer( d, "Ok, what IS it, then? ", 0 );
               d->character->desc = NULL;
               free_char( d->character );
               d->character = NULL;
               d->connected = CON_GET_NAME;
               break;

            default:
               write_to_buffer( d, "Please type Yes or No. ", 0 );
               break;
         }
         break;

      case CON_GET_NEW_PASSWORD:
         write_to_buffer( d, "\r\n", 2 );

         if( strlen( argument ) < 5 )
         {
            write_to_buffer( d, "Password must be at least five characters long.\r\nPassword: ", 0 );
            return;
         }

         if( argument[0] == '!' )
         {
            send_to_char( "New password can't begin with the '!' character.\r\n", ch );
            return;
         }
         pwdnew = sha256_crypt( argument );   /* SHA-256 Encryption */
         STRSET( ch->pcdata->pwd, pwdnew );
         write_to_buffer( d, "\r\nPlease retype the password to confirm: ", 0 );
         d->connected = CON_CONFIRM_NEW_PASSWORD;
         break;

      case CON_CONFIRM_NEW_PASSWORD:
         write_to_buffer( d, "\r\n", 2 );

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
         {
            write_to_buffer( d, "Passwords don't match.\r\nRetype password: ", 0 );
            d->connected = CON_GET_NEW_PASSWORD;
            return;
         }

         write_to_buffer( d, (const char *)echo_on_str, 0 );
         write_to_buffer( d, "\r\nWhat is your sex (M/F/N)? ", 0 );
         d->connected = CON_GET_NEW_SEX;
         break;

      case CON_GET_NEW_SEX:
         switch( argument[0] )
         {
            case 'm':
            case 'M':
               ch->sex = SEX_MALE;
               break;
            case 'f':
            case 'F':
               ch->sex = SEX_FEMALE;
               break;
            case 'n':
            case 'N':
               ch->sex = SEX_NEUTRAL;
               break;
            default:
               write_to_buffer( d, "That's not a sex.\r\nWhat IS your sex? ", 0 );
               return;
         }

         if( skipclass )
         {
            write_to_buffer( d, "\r\nYou may choose from the following races, or type help [race] to learn more:\r\n", 0 );
            snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_race_list( d ) );
            write_to_buffer( d, buf, 0 );
            add_mclass( d->character, -1, 1, 100, 0.0, true );
            d->connected = CON_GET_NEW_RACE;
         }
         else
         {
            write_to_buffer( d, "\r\nYou may choose from the following classes, or type help [class] to learn more:\r\n", 0 );
            snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_class_list( d ) );
            write_to_buffer( d, buf, 0 );
            d->connected = CON_GET_NEW_CLASS;
         }
         break;

      case CON_GET_NEW_CLASS:
         argument = one_argument( argument, arg );
         if( !str_cmp( arg, "help" ) )
         {
            for( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )
            {
               if( !can_use_class( d, iClass ) )
                  continue;
               if( toupper( argument[0] ) == toupper( class_table[iClass]->name[0] )
               && !str_prefix( argument, class_table[iClass]->name ) && valid_help( class_table[iClass]->name ) )
               {
                  do_help( ch, class_table[iClass]->name );
                  write_to_buffer( d, "\r\nYou may choose from the following classes, or type help [class] to learn more:\r\n", 0 );
                  snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_class_list( d ) );
                  write_to_buffer( d, buf, 0 );
                  return;
               }
            }
            write_to_buffer( d, "No help on that topic. Please choose a class or type help [class] to learn more:\r\n", 0 );
            snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_class_list( d ) );
            write_to_buffer( d, buf, 0 );
            return;
         }

         mclass = NULL;
         if( !str_cmp( arg, "none" ) && ch->pcdata->first_mclass )
            mcdone = true;

         if( !mcdone )
         {
            for( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )
            {
               if( !can_use_class( d, iClass ) )
                  continue;
               if( toupper( arg[0] ) == toupper( class_table[iClass]->name[0] )
               && !str_prefix( arg, class_table[iClass]->name )
               && !char_is_class( ch, iClass ) )
               {
                  mclass = add_mclass( ch, iClass, 1, 100, 0.0, true );
                  break;
               }
            }

            if( iClass >= MAX_PC_CLASS || !class_table[iClass] || !class_table[iClass]->name )
            {
               write_to_buffer( d, "\r\nYou may choose from the following classes, or type help [class] to learn more:\r\n", 0 );
               snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_class_list( d ) );
               write_to_buffer( d, buf, 0 );
               if( mclass )
               {
                  UNLINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
                  DISPOSE( mclass );
               }
               return;
            }
         }

         if( check_bans( ch, BAN_CLASS ) )
         {
            write_to_buffer( d, "\r\nThat class is not currently available.\r\nYou may choose from the following classes, or type help [class] to learn more:\r\n", 0 );
            snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_class_list( d ) );
            write_to_buffer( d, buf, 0 );
            if( mclass )
            {
               UNLINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
               DISPOSE( mclass );
            }
            return;
         }

         {
            int mcount = 0;

            for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
               mcount++;
            if( !mcdone && mcount < sysdata.mclass )
            {
               write_to_buffer( d, "\r\nYou may now choose another class.\r\nYou may choose from the following classes, or type help [class] to learn more:\r\n", 0 );
               snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_class_list( d ) );
               write_to_buffer( d, buf, 0 );
               return;
            }
            if( mcount > 1 )
            {
               int cpercent = ( 100 / mcount );
               int rest = umax( 0, ( 100 - ( cpercent * mcount ) ) );

               for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
               {
                  mclass->cpercent = ( cpercent + rest );
                  rest = 0; /* Already given out the rest to the first class, set it to 0 */
               }
            }
         }

         write_to_buffer( d, "\r\nYou may choose from the following races, or type help [race] to learn more:\r\n", 0 );
         snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_race_list( d ) );
         write_to_buffer( d, buf, 0 );

         redo = true;
         for( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
         {
            if( !can_use_race( d, iRace ) )
               continue;
            redo = false;
         }
         if( redo )
         {
            MCLASS_DATA *mclass_next;

            send_to_char( "It looks like there are no races for that class please choose another class.\r\n", ch );
            for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass_next )
            {
               mclass_next = mclass->next;
               UNLINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
               DISPOSE( mclass );
            }
            return;
         }

         for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
         {
            if( mclass->wclass >= 0 && mclass->wclass < MAX_PC_CLASS )
            {
               class_table[mclass->wclass]->used++;
               write_class_file( mclass->wclass );
            }
         }
         d->connected = CON_GET_NEW_RACE;
         break;

      case CON_GET_NEW_RACE:
         argument = one_argument( argument, arg );
         if( !str_cmp( arg, "help" ) )
         {
            for( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
            {
               if( !can_use_race( d, iRace ) )
                  continue;
               if( toupper( argument[0] ) == toupper( race_table[iRace]->name[0] )
               && !str_prefix( argument, race_table[iRace]->name ) && valid_help( race_table[iRace]->name ) )
               {
                  do_help( ch, race_table[iRace]->name );
                  write_to_buffer( d, "\r\nYou may choose from the following races, or type help [race] to learn more:\r\n", 0 );
                  snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_race_list( d ) );
                  write_to_buffer( d, buf, 0 );
                  return;
               }
            }
            write_to_buffer( d, "\r\nNo help on that topic.\r\nYou may choose from the following races, or type help [race] to learn more:\r\n", 0 );
            snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_race_list( d ) );
            write_to_buffer( d, buf, 0 );
            return;
         }

         for( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
         {
            if( !can_use_race( d, iRace ) )
               continue;
            if( toupper( arg[0] ) == toupper( race_table[iRace]->name[0] )
            && !str_prefix( arg, race_table[iRace]->name ) )
            {
               ch->race = iRace;
               break;
            }
         }

         if( !can_use_race( d, iRace ) )
         {
            write_to_buffer( d, "\r\nYou may choose from the following races, or type help [race] to learn more:\r\n", 0 );
            snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_race_list( d ) );
            write_to_buffer( d, buf, 0 );
            return;
         }

         if( check_bans( ch, BAN_RACE ) )
         {
            write_to_buffer( d, "That race is not currently available.\r\nWhat is your race? ", 0 );
            write_to_buffer( d, "\r\nYou may choose from the following races, or type help [race] to learn more:\r\n", 0 );
            snprintf( buf, sizeof( buf ), "[%s]\r\n: ", send_race_list( d ) );
            write_to_buffer( d, buf, 0 );
            return;
         }

         race_table[ch->race]->used++;
         write_race_file( ch->race );
         write_to_buffer( d, "\r\nWould you like to be a player killer, [Y/N]? ", 0 );
         d->connected = CON_GET_PKILL;
         break;

      case CON_GET_PKILL:
         switch( argument[0] )
         {
            case 'y':
            case 'Y':
               xSET_BIT( ch->pcdata->flags, PCFLAG_DEADLY );
               break;

            case 'n':
            case 'N':
               break;

            default:
               write_to_buffer( d, "Would you like to be a player killer, [Y/N]? ", 0 );
               return;
         }
         write_to_buffer( d, "\r\nWould you like ANSI or no graphic/color support, (A/N)? ", 0 );
         d->connected = CON_GET_WANT_ANSI;
         break;

      case CON_GET_WANT_ANSI:
         switch( argument[0] )
         {
            case 'a':
            case 'A':
               xSET_BIT( ch->act, PLR_ANSI );
               break;

            case 'n':
            case 'N':
               break;

            default:
               write_to_buffer( d, "Invalid selection.\r\nANSI or NONE? ", 0 );
               return;
         }
         if( skipclass )
            snprintf( log_buf, sizeof( log_buf ), "%s@%s new %s.", ch->name, d->host, dis_race_name( ch->race ) );
         else
            snprintf( log_buf, sizeof( log_buf ), "%s@%s new %s %s.", ch->name, d->host, dis_race_name( ch->race ), dis_main_class_name( ch ) );
         log_string_plus( log_buf, LOG_COMM, sysdata.perm_log );
         handle_hostlog( "New", ch );
         to_channel( log_buf, "monitor", PERM_IMM );
         write_to_buffer( d, "Press [ENTER] ", 0 );
         show_title( d );
         ch->level = 1;
         ch->pcdata->speed = 32;
         ch->pcdata->birth_month = time_info.month;
         ch->pcdata->birth_day = time_info.day;
         ch->pcdata->birth_year = ( time_info.year - 17 );
         STRSET( ch->pcdata->channels, "classtalk racetalk counciltalk music quest clantalk nationtalk chat wartalk traffic yell auction highscore trivia fchat" );
         ch->position = POS_STANDING;
         d->connected = CON_PRESS_ENTER;
         set_pager_color( AT_PLAIN, ch );
         return;
         break;

      case CON_PRESS_ENTER:
         if( ch->level == 1 )
            reset_colors( ch );

         set_char_color( AT_PLAIN, ch );
         set_pager_color( AT_PLAIN, ch );
         if( xIS_SET( ch->act, PLR_ANSI ) )
            send_to_pager( "\033[2J", ch );
         else
            send_to_pager( "\014", ch );
         if( is_immortal( ch ) )
            do_help( ch, (char *)"imotd" );
         else if( ch->level == MAX_LEVEL )
            do_help( ch, (char *)"amotd" );
         else if( ch->level < MAX_LEVEL && ch->level > 1 )
            do_help( ch, (char *)"motd" );
         else if( ch->level == 1 )
            do_help( ch, (char *)"nmotd" );
         send_to_pager( "\r\nPress [ENTER] ", ch );
         d->connected = CON_READ_MOTD;
         break;

      case CON_READ_MOTD:
         {
            char motdbuf[MSL];

            snprintf( motdbuf, sizeof( motdbuf ), "\r\nWelcome to %s...\r\n", sysdata.mud_name );
            write_to_buffer( d, motdbuf, 0 );
         }
         add_char( ch );
         d->connected = CON_PLAYING;

         if( ch->level == 1 )
         {
            int iLang, uLang;

            ch->pcdata->clan = NULL;

            if( ( iLang = skill_lookup( "common" ) ) < 0 )
               bug( "%s", "Nanny: can't find common language." );
            else
               ch->pcdata->learned[iLang] = 100;

            /* Give them their racial languages */
            if( race_table[ch->race] )
            {
               for( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
               {
                  if( xIS_SET( race_table[ch->race]->language, lang_array[iLang] ) )
                   {
                     if( ( uLang = skill_lookup( lang_names[iLang] ) ) < 0 )
                        bug( "%s: can't find racial language [%s].", __FUNCTION__, lang_names[iLang] );
                     else
                        ch->pcdata->learned[uLang] = 100;
                  }
               }
            }

            set_base_stats( ch );
            set_title( ch, (char *)"the newbie" );
            xSET_BIT( ch->act, PLR_AUTOGOLD );
            xSET_BIT( ch->act, PLR_AUTOEXIT );
            xSET_BIT( ch->act, PLR_SUICIDE );
            xSET_BIT( ch->pcdata->flags, PCFLAG_GAG );
            xSET_BIT( ch->act, PLR_COMPASS );
            xSET_BIT( ch->act, PLR_GROUPAFFECTS );
            xSET_BIT( ch->act, PLR_AUTOLOOT );
            xSET_BIT( ch->act, PLR_AUTOSPLIT );
            xSET_BIT( ch->act, PLR_AUTOSAC );
            xSET_BIT( ch->act, PLR_SMARTSAC );
            if( !sysdata.WAIT_FOR_AUTH )
               char_to_room( ch, get_room_index( sysdata.room_school ) );
            else
            {
               char_to_room( ch, get_room_index( sysdata.room_authstart ) );
               add_to_auth( ch->name );
               save_auths( );
            }
         }
         else if( ch->in_room )
            char_to_room( ch, ch->in_room );
         else
            char_to_room( ch, get_room_index( sysdata.room_temple ) );

         if( !ch->was_in_room )
            ch->was_in_room = ch->in_room;

         if( get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
            remove_timer( ch, TIMER_SHOVEDRAG );

         if( get_timer( ch, TIMER_PKILLED ) > 0 )
            remove_timer( ch, TIMER_PKILLED );

         act( AT_ACTION, "$n has entered the game.", ch, NULL, NULL, TO_CANSEE );

         if( ch->pcdata->first_pet )
         {
            CHAR_DATA *pet;

            for( pet = ch->pcdata->first_pet; pet; pet = pet->next_pet )
            {
               act( AT_ACTION, "$n returns to $s master from the Void.", pet, NULL, ch, TO_NOTVICT );
               act( AT_ACTION, "$N returns with you to the realms.", ch, NULL, pet, TO_CHAR );
            }
         }
         do_look( ch, (char *)"auto" );

         if( !ch->pcdata->channels )
            send_to_char( "\r\n&WYou aren't listening to any channels currently. Type \"listen all\" to listen to all channels.\r\n", ch );

         check_auction( ch );

         show_unread_notes( ch );

         not_authorized( ch );

         check_bti( ch );

         {
            char message[MSL];

            snprintf( message, sizeof( message ), "%s has logged in.", ch->name );
            send_friend_info( ch, message );
         }

         break;
   }
}

/* Parse a name for acceptability. */
bool check_parse_name( char *name, bool newchar )
{
   /* Length restrictions. */
   if( strlen( name ) < 3 )
      return false;

   if( strlen( name ) > 12 )
      return false;

   if( newchar && is_reserved_name( name ) )
      return false;

   /* Illegal characters */
   if( strstr( name, ".." ) || strstr( name, "/" ) || strstr( name, "\\" ) )
      return false;

   return true;
}

bool is_valid_path( CHAR_DATA *ch, const char *direct, const char *filename )
{
   /* Length restrictions */
   if( !filename || filename[0] == '\0' )
   {
      if( ch )
         send_to_char( "Empty filename is not valid.\r\n", ch );
      return false;
   }
   if( strlen( filename ) < 3 )
   {
      if( ch )
         ch_printf( ch, "Filename (%s) isn't long enough.\r\n", filename );
      return false;
   }

   /* Illegal characters */
   if( strstr( filename, ".." ) || strstr( filename, "/" ) || strstr( filename, "\\" ) )
   {
      if( ch )
         send_to_char( "A filename may not contain a '..', '/', or '\\' in it.\r\n", ch );
      return false;
   }

   return true;
}

bool can_use_path( CHAR_DATA *ch, const char *direct, const char *filename )
{
   struct stat fst;
   char newfilename[1024];

   /* Length restrictions */
   if( !filename || filename[0] == '\0' )
   {
      if( ch )
         send_to_char( "Empty filename is not valid.\r\n", ch );
      return false;
   }
   if( strlen( filename ) < 3 )
   {
      if( ch )
         ch_printf( ch, "Filename (%s) isn't long enough.\r\n", filename );
      return false;
   }

   /* Illegal characters */
   if( strstr( filename, ".." ) || strstr( filename, "/" ) || strstr( filename, "\\" ) )
   {
      if( ch )
         send_to_char( "A filename may not contain a '..', '/', or '\\' in it.\r\n", ch );
      return false;
   }

   /* If that filename is already being used lets not allow it now to be on the safe side */
   snprintf( newfilename, sizeof( newfilename ), "%s%s", direct, filename );
   if( stat( newfilename, &fst ) != -1 )
   {
      if( ch )
         ch_printf( ch, "The filename (%s) is already used.\r\n", filename );
      return false;
   }

   /* If we got here assume its valid */
   return true;
}

bool multi_check( DESCRIPTOR_DATA *host, bool full )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *ch;
   int tconns = 0, dconns = 0, pconns = 0;

   if( !host || !host->host || host->host[0] == '\0' )
      return false;

   for( d = first_descriptor; d; d = d->next )
   {
      if( !d->host )
         continue;
      if( str_cmp( d->host, host->host ) )
         continue;
      tconns++;
      if( !( ch = d->character ) || d == host )
         continue;
      if( xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
         dconns++;
      else
         pconns++;
   }
   if( tconns >= sysdata.mlimit_total )
   {
      write_to_descriptor( host, "You have to many connections to the mud already.\r\n", 0 );
      return true;
   }
   if( full )
   {
      if( dconns >= sysdata.mlimit_deadly )
      {
         write_to_descriptor( host, "You have to many deadlys connected to the mud already.\r\n", 0 );
         return true;
      }
      if( pconns >= sysdata.mlimit_peaceful )
      {
         write_to_descriptor( host, "You have to many peacefuls connected to the mud already.\r\n", 0 );
         return true;
      }
   }
   return false;
}

/* Look for link-dead player to reconnect. */
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn )
{
   CHAR_DATA *ch;

   for( ch = first_char; ch; ch = ch->next )
   {
      if( !is_npc( ch ) && ( !fConn || !ch->desc ) && ch->pcdata->filename && !str_cmp( name, ch->pcdata->filename ) )
      {
         if( fConn && ch->desc )
         {
            write_to_buffer( d, "Already playing.\r\nName: ", 0 );
            d->connected = CON_GET_NAME;
            if( d->character )
            {
               /* clear descriptor pointer to get rid of bug message in log */
               d->character->desc = NULL;
               free_char( d->character );
               d->character = NULL;
            }
            return false;
         }
         if( fConn == false )
            STRSET( d->character->pcdata->pwd, ch->pcdata->pwd );
         else
         {
            d->character->desc = NULL;
            free_char( d->character );
            if( !( d->character = ch ) )
            {
               write_to_buffer( d, "Wow there is some major issue.\r\nName: ", 0 );
               d->connected = CON_GET_NAME;
               return false;
            }
            log_printf_plus( LOG_COMM, get_trust( ch ), "%s (%s) reconnected.", ch->name, d->host );
            handle_hostlog( "Reconnected", ch );
            ch->desc = d;
            ch->timer = 0;
            send_to_char( "Reconnecting.\r\n", ch );
            act( AT_ACTION, "$n has reconnected.", ch, NULL, NULL, TO_CANSEE );
            fix_hhf( ch );
            if( ch->editor )
            {
               send_to_char( "Continue entering your text now (/? = help /s = save /c = clear /l = list)\r\n", ch );
               send_to_char( "-----------------------------------------------------------------------\r\n> ", ch );
               d->connected = CON_EDITING;
            }
            else
            {
               d->connected = CON_PLAYING;
               do_look( ch, (char *)"auto" );
            }
         }
         return true;
      }
   }

   return false;
}

/* Check if already playing. */
bool check_playing( DESCRIPTOR_DATA *d, char *name, bool kick )
{
   CHAR_DATA *ch;
   DESCRIPTOR_DATA *dold;
   int cstate;

   for( dold = first_descriptor; dold; dold = dold->next )
   {
      if( dold != d
      && ( dold->character ) && !str_cmp( name, dold->character->pcdata->filename ) )
      {
         cstate = dold->connected;
         ch = dold->character;
         if( !ch->name || ( cstate != CON_PLAYING && cstate != CON_EDITING ) )
         {
            write_to_buffer( d, "Already connected - try again.\r\n", 0 );
            log_printf_plus( LOG_COMM, PERM_HEAD, "%s already connected.", ch->pcdata->filename );
            return true;
         }
         if( !kick )
            return true;
         write_to_buffer( d, "Already playing... Kicking off old connection.\r\n", 0 );
         write_to_buffer( dold, "Kicking off old connection... bye!\r\n", 0 );
         close_socket( dold, false );
         /* clear descriptor pointer to get rid of bug message in log */
         d->character->desc = NULL;
         free_char( d->character );
         d->character = ch;
         ch->desc = d;
         ch->timer = 0;
         log_printf_plus( LOG_COMM, get_trust( ch ), "%s@%s reconnected, kicking off old link.",
             ch->pcdata->filename, d->host );
         handle_hostlog( "Reconnected", ch );
         send_to_char( "Reconnecting.\r\n", ch );
         fix_hhf( ch );
         act( AT_ACTION, "$n has reconnected, kicking off old link.", ch, NULL, NULL, TO_CANSEE );
         d->connected = cstate;
         if( ch->editor )
         {
            send_to_char( "Continue entering your text now (/? = help /s = save /c = clear /l = list)\r\n", ch );
            send_to_char( "-----------------------------------------------------------------------\r\n> ", ch );
         }
         else
         {
            d->connected = CON_PLAYING;
            do_look( ch, (char *)"auto" );
         }
         return true;
      }
   }

   return false;
}

void stop_idling( CHAR_DATA *ch )
{
   ROOM_INDEX_DATA *was_in_room;

   if( !ch || !ch->desc || ch->desc->connected != CON_PLAYING || !is_idle( ch ) )
      return;

   ch->timer = 0;
   was_in_room = ch->was_in_room;
   char_from_room( ch );
   char_to_room( ch, was_in_room );
   ch->was_in_room = ch->in_room;
   xREMOVE_BIT( ch->pcdata->flags, PCFLAG_IDLE );
   act( AT_ACTION, "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
}

/*
 * Function to strip off the "a" or "an" or "the" or "some" from an object's
 * short description for the purpose of using it in a sentence sent to
 * the owner of the object.  (Ie: an object with the short description
 * "a long dark blade" would return "long dark blade" for use in a sentence
 * like "Your long dark blade".  The object name isn't always appropriate
 * since it contains keywords that may not look proper.		-Thoric
 */
char *myobj( OBJ_DATA *obj )
{
   if( !str_prefix( "a ", obj->short_descr ) )
      return obj->short_descr + 2;
   if( !str_prefix( "an ", obj->short_descr ) )
      return obj->short_descr + 3;
   if( !str_prefix( "the ", obj->short_descr ) )
      return obj->short_descr + 4;
   if( !str_prefix( "some ", obj->short_descr ) )
      return obj->short_descr + 5;
   return obj->short_descr;
}

char *obj_short( OBJ_DATA *obj )
{
   static char buf[MSL];

   if( obj->count > 1 )
   {
      snprintf( buf, sizeof( buf ), "%s (%d)", obj->short_descr, obj->count );
      return buf;
   }
   return obj->short_descr;
}

#define NAME( ch ) ( is_npc( (ch) ) ? (ch)->short_descr : (ch)->name )

char *MORPHNAME( CHAR_DATA *ch )
{
   if( ch->morph && ch->morph->morph && ch->morph->morph->short_desc )
      return ch->morph->morph->short_desc;
   else
      return NAME(ch);
}

char *act_string( const char *format, CHAR_DATA *to, CHAR_DATA *ch, void *arg1, void *arg2, int flags )
{
   CHAR_DATA *vch = ( CHAR_DATA * ) arg2;
   OBJ_DATA *obj1 = ( OBJ_DATA * ) arg1;
   OBJ_DATA *obj2 = ( OBJ_DATA * ) arg2;
   const char *str = format;
   const char *i = "";
   static char buf[MSL];
   char temp[MSL];
   char *point = buf;

   if( str[0] == '$' )
      DONT_UPPER = false;

   while( *str != '\0' )
   {
      if( *str != '$' )
      {
         *point++ = *str++;
         continue;
      }
      ++str;
      if( !arg2 && *str >= 'A' && *str <= 'Z' && *str != 'Q' )
      {
         bug( "Act: missing arg2 for code %c:", *str );
         bug( "%s", format );
         i = " <@@@> ";
      }
      else
      {
         switch( *str )
         {
            default:
               bug( "Act: bad code %c.", *str );
               i = " <@@@> ";
               break;

            case 'e':
               if( ch->sex > 2 || ch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", ch->name, ch->sex );
                  i = "it";
               }
               else
                  i = !can_see( to, ch ) ? "it" : he_she[URANGE( 0, ch->sex, 2 )];
               break;

            case 'E':
               if( vch->sex > 2 || vch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", vch->name, vch->sex );
                  i = "it";
               }
               else
                  i = !can_see( to, vch ) ? "it" : he_she[URANGE( 0, vch->sex, 2 )];
               break;

            case 'm':
               if( ch->sex > 2 || ch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", ch->name, ch->sex );
                  i = "it";
               }
               else
                  i = !can_see( to, ch ) ? "it" : him_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'M':
               if( vch->sex > 2 || vch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", vch->name, vch->sex );
                  i = "it";
               }
               else
                  i = !can_see( to, vch ) ? "it" : him_her[URANGE( 0, vch->sex, 2 )];
               break;

            case 'n':
               if( !can_see( to, ch ) )
                  i = "Someone";
               else if( !ch->morph )
                  i = ( to ? PERS( ch, to ) : NAME( ch ) );
               else if( !IS_SET( flags, STRING_IMM ) )
                  i = ( to ? MORPHPERS( ch, to ) : MORPHNAME( ch ) );
               else
               {
                  snprintf( temp, sizeof( temp ), "(MORPH) %s", ( to ? PERS( ch, to ) : NAME( ch ) ) );
                  i = temp;
               }
               break;

            case 'N':
               if( !can_see( to, vch ) )
                  i = "Someone";
               else if( !vch->morph )
                  i = ( to ? PERS( vch, to ) : NAME( vch ) );
               else if( !IS_SET( flags, STRING_IMM ) )
                  i = ( to ? MORPHPERS( vch, to ) : MORPHNAME( vch ) );
               else
               {
                  snprintf( temp, sizeof( temp ), "(MORPH) %s", ( to ? PERS( vch, to ) : NAME( vch ) ) );
                  i = temp;
               }
               break;

            case 'q':
               i = ( to == ch ) ? "" : "s";
               break;

            case 'Q':
               i = ( to == ch ) ? "your" : his_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 's':
               if( ch->sex > 2 || ch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", ch->name, ch->sex );
                  i = "its";
               }
               else
                  i = !can_see( to, ch ) ? "its" : his_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'S':
               if( vch->sex > 2 || vch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", vch->name, vch->sex );
                  i = "its";
               }
               else
                  i = !can_see( to, vch ) ? "its" : his_her[URANGE( 0, vch->sex, 2 )];
               break;

            case 't':
               i = ( char * )arg1;
               break;

            case 'T':
               i = ( char * )arg2;
               break;

            case 'p':
               if( !obj1 )
               {
                  bug( "act_string: $p used with NULL obj1!" );
                  i = "something";
               }
               if( to && obj1 )
                  i = ( can_see_obj( to, obj1 ) ? obj_short( obj1 ) : "something" );
               break;

            case 'P':
               if( !obj2 )
               {
                  bug( "act_string: $P used with NULL obj2!" );
                  i = "something";
               }
               if( to && obj2 )
                  i = ( can_see_obj( to, obj2 ) ? obj_short( obj2 ) : "something" );
               break;

            case 'd':
               if( !arg2 || ( ( char * )arg2 )[0] == '\0' )
                  i = "door";
               else
               {
                  char fname[MIL];

                  one_argument( ( char * )arg2, fname );
                  i = fname;
               }
               break;
         }
      }
      ++str;
      while( i && *i && ( *point = *i ) != '\0' )
         ++point, ++i;
   }
   mudstrlcpy( point, "\r\n", sizeof( point ) );
   if( !DONT_UPPER )
   {
      /* Up the part it should when its a global type thing */
      if( !str_prefix( "[From afar] ", buf ) )
         buf[12] = UPPER( buf[12] );
      else
         buf[0] = UPPER( buf[0] );
   }
   return buf;
}

#undef NAME

void act( short AType, const char *format, CHAR_DATA *ch, void *arg1, void *arg2, int type )
{
   CHAR_DATA *to;
   CHAR_DATA *vch = ( CHAR_DATA * ) arg2;
   char *txt;

   /* Discard null and zero-length messages. */
   if( !format || format[0] == '\0' )
      return;

   if( !ch )
   {
      bug( "Act: null ch. (%s)", format );
      return;
   }

   if( !ch->in_room )
      to = NULL;
   else if( type == TO_CHAR )
      to = ch;
   else if( type == TO_OTHERS )
      to = first_char;
   else
      to = ch->in_room->first_person;

   /* ACT_SECRETIVE handling */
   if( is_npc( ch ) && xIS_SET( ch->act, ACT_SECRETIVE ) && type != TO_CHAR )
      return;

   if( type == TO_VICT )
   {
      if( !vch )
      {
         bug( "%s", "Act: null vch with TO_VICT." );
         bug( "%s (%s)", ch->name, format );
         return;
      }
      if( !vch->in_room )
      {
         bug( "%s", "Act: vch in NULL room!" );
         bug( "%s -> %s (%s)", ch->name, vch->name, format );
         return;
      }
      to = vch;
   }

   if( MOBtrigger && type != TO_CHAR && type != TO_VICT && type != TO_OTHERS && to )
   {
      OBJ_DATA *to_obj;

      txt = act_string( format, NULL, ch, arg1, arg2, STRING_IMM );
      if( HAS_PROG( to->in_room, ACT_PROG ) )
         rprog_act_trigger( txt, to->in_room, ch, ( OBJ_DATA * ) arg1, ( void * )arg2 );
      for( to_obj = to->in_room->first_content; to_obj; to_obj = to_obj->next_content )
         if( HAS_PROG( to_obj->pIndexData, ACT_PROG ) )
            oprog_act_trigger( txt, to_obj, ch, ( OBJ_DATA * ) arg1, ( void * )arg2 );
   }

   /*
    * Anyone feel like telling me the point of looping through the whole
    * room when we're only sending to one char anyways..? -- Alty 
    */
   for( ; to; to = ( type == TO_CHAR || type == TO_VICT ) ? NULL : ( type == TO_OTHERS ) ? to->next : to->next_in_room )
   {
      if( ( !to->desc && ( is_npc( to ) && !HAS_PROG( to->pIndexData, ACT_PROG ) ) ) || !is_awake( to ) )
         continue;
      if( type == TO_CHAR && to != ch )
         continue;
      if( type == TO_VICT && ( to != vch || to == ch ) )
         continue;
      if( type == TO_ROOM )
      {
         if( to == ch || !can_see_character( to, ch ) )
            continue;
         /* My bad, this I ment to have so immortals always see who is sneaking when leaving, this should be correct now */
         if( sneaking_char && !is_immortal( to ) && !IS_AFFECTED( to, AFF_DETECT_SNEAK ) )
            continue;
      }
      if( type == TO_OTHERS && ( to == ch || to == vch || is_npc( to ) ) )
         continue;
      if( type == TO_NOTVICT )
      {
         if( to == ch || to == vch || !can_see_character( to, ch ) )
            continue;
      }
      if( type == TO_CANSEE && ( to == ch
      || ( !is_immortal( to ) && !is_npc( ch ) && ( xIS_SET( ch->act, PLR_WIZINVIS )
      && ( get_trust( to ) < ( ch->pcdata ? ch->pcdata->wizinvis : 0 ) ) ) ) ) )
         continue;

      /* No clue why ignore wasn't handled here instead */
      if( to && ch && is_ignoring( to, ch ) )
      {
         if( !is_immortal( ch ) || get_trust( to ) > get_trust( ch ) )
            continue;
         else
            ch_printf( to, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
      }

      if( to && vch && is_ignoring( to, vch ) )
      {
         if( !is_immortal( vch ) || get_trust( to ) > get_trust( vch ) )
            continue;
         else
            ch_printf( to, "You attempt to ignore %s, but are unable to do so.\r\n", vch->name );
      }

      if( is_immortal( to ) )
         txt = act_string( format, to, ch, arg1, arg2, STRING_IMM );
      else
         txt = act_string( format, to, ch, arg1, arg2, STRING_NONE );

      if( to->desc )
      {
         set_char_color( AType, to );
         send_to_char( txt, to );
      }
      if( MOBtrigger )
      {
         /* Note: use original string, not string with ANSI. -- Alty */
         mprog_act_trigger( txt, to, ch, ( OBJ_DATA * ) arg1, ( void * )arg2 );
      }
   }
}

char *default_fprompt( CHAR_DATA *ch )
{
   static char buf[100];

   mudstrlcpy( buf, "&w<&Y%hhp ", sizeof( buf ) );
   if( is_vampire( ch ) )
      mudstrlcat( buf, "&R%bbp", sizeof( buf ) );
   else
      mudstrlcat( buf, "&C%mm", sizeof( buf ) );
   mudstrlcat( buf, " &G%vmv&w>", sizeof( buf ) );
   mudstrlcat( buf, "&w[&Y%g&wGold]", sizeof( buf ) );
   if( is_npc( ch ) || is_immortal( ch ) )
   {
      mudstrlcat( buf, "%i", sizeof( buf ) );
      mudstrlcat( buf, "&w[&RRVnum &W%r&w]", sizeof( buf ) );
   }
   if( ch->questcountdown > 0 )
   {
      if( xIS_SET( ch->act, PLR_QUESTOR ) )
         mudstrlcat( buf, "&w[&WQuest Time &Y%q&w]", sizeof( buf ) );
      else
         mudstrlcat( buf, "&w[&WTill Quest &Y%q&w]", sizeof( buf ) );
   }
   mudstrlcat( buf, "&w<&C%n&w[&R%c&w]>", sizeof( buf ) );
   return buf;
}

char *default_prompt( CHAR_DATA *ch )
{
   static char buf[100];

   mudstrlcpy( buf, "&w<&Y%hhp ", sizeof( buf ) );
   if( is_vampire( ch ) )
      mudstrlcat( buf, "&R%bbp", sizeof( buf ) );
   else
      mudstrlcat( buf, "&C%mm", sizeof( buf ) );
   mudstrlcat( buf, " &G%vmv&w>", sizeof( buf ) );
   mudstrlcat( buf, "&w[&Y%g&wGold]", sizeof( buf ) );
   if( is_npc( ch ) || is_immortal( ch ) )
   {
      mudstrlcat( buf, "%i", sizeof( buf ) );
      mudstrlcat( buf, "&w[&RRVnum &W%r&w]", sizeof( buf ) );
   }
   if( ch->questcountdown > 0 )
   {
      if( xIS_SET( ch->act, PLR_QUESTOR ) )
         mudstrlcat( buf, "&w[&WQuest Time &Y%q&w]", sizeof( buf ) );
      else
         mudstrlcat( buf, "&w[&WTill Quest &Y%q&w]", sizeof( buf ) );
   }
   return buf;
}

double get_percent( int fnum, int snum )
{
   double percent = 0.0;

   if( fnum == snum )
      return 100.0;
   if( fnum <= 0 )
      return ( double )fnum;

   percent += ( 100 * fnum );
   percent /= ( snum );
   return percent;
}

void display_prompt( DESCRIPTOR_DATA *d )
{
   CHAR_DATA *ch = d->character;
   CHAR_DATA *och = d->character;
   CHAR_DATA *victim;
   const char *prompt;
   char buf[MSL];
   char *pbuf = buf;
   double percent;
   unsigned int pstat;
   bool ansi = ( !is_npc( och ) && xIS_SET( och->act, PLR_ANSI ) );

   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return;
   }

   if( !is_npc( ch ) && ch->substate != SUB_NONE && ch->pcdata->subprompt && ch->pcdata->subprompt[0] != '\0' )
      prompt = ch->pcdata->subprompt;
   else if( is_npc( ch ) || ( !ch->fighting && ( !ch->pcdata->prompt || !*ch->pcdata->prompt ) ) )
      prompt = default_prompt( ch );
   else if( ch->fighting )
   {
      if( !ch->pcdata->fprompt || !*ch->pcdata->fprompt )
         prompt = default_fprompt( ch );
      else
         prompt = ch->pcdata->fprompt;
   }
   else
      prompt = ch->pcdata->prompt;
   if( ansi )
   {
      mudstrlcpy( pbuf, ANSI_RESET, sizeof( buf ) );
      d->prevcolor = 0x08;
      pbuf += 4;
   }

   /* Clear out old color stuff */
   for( ; *prompt; prompt++ )
   {
      /*
       * '%' = prompt commands
       * Note: foreground changes will revert background to 0 (black)
       */
      if( *prompt != '%' )
      {
         *( pbuf++ ) = *prompt;
         continue;
      }
      ++prompt;
      if( !*prompt )
         break;
      if( *prompt == *( prompt - 1 ) )
      {
         *( pbuf++ ) = *prompt;
         continue;
      }
      switch( *( prompt - 1 ) )
      {
         default:
            bug( "%s: bad command char '%c'.", __FUNCTION__, *( prompt - 1 ) );
            break;
         case '%':
            *pbuf = '\0';
            pstat = 0x80000000;
            switch( *prompt )
            {
               case '%':
                  *pbuf++ = '%';
                  *pbuf = '\0';
                  break;

               case 'a':
                  pstat = ch->alignment;
                  break;

               case 'A':
                  snprintf( pbuf, sizeof( buf ), "%s%s%s", IS_AFFECTED( ch, AFF_INVISIBLE ) ? "I" : "",
                     IS_AFFECTED( ch, AFF_HIDE ) ? "H" : "", IS_AFFECTED( ch, AFF_SNEAK ) ? "S" : "" );
                  break;

               case 'C':
                  if( ch->fighting && ( victim = ch->fighting->who )
                  && victim->fighting && ( victim = victim->fighting->who ) )
                  {
                     if( victim->hit > 0 && victim->max_hit > 0 )
                     {
                        percent = get_percent( victim->hit, victim->max_hit );
                        if( percent < 1 )
                           snprintf( pbuf, sizeof( buf ), "%.3f%%", percent );
                        else
                           snprintf( pbuf, sizeof( buf ), "%.f%%", percent );
                     }
                  }
                  break;

               case 'c':
                  if( ch->fighting && ( victim = ch->fighting->who ) )
                  {
                     if( victim->hit > 0 && victim->max_hit > 0 )
                     {
                        percent = get_percent( victim->hit, victim->max_hit );
                        if( percent < 1 )
                           snprintf( pbuf, sizeof( buf ), "%.3f%%", percent );
                        else
                           snprintf( pbuf, sizeof( buf ), "%.f%%", percent );
                     }
                  }
                  break;

               case 'F':
                  if( is_immortal( och ) )
                     snprintf( pbuf, sizeof( buf ), "%s", ext_flag_string( &ch->in_room->room_flags, r_flags ) );
                  break;

               case 'g':
                  snprintf( pbuf, sizeof( buf ), "%s", show_char_gold( ch ) );
                  break;

               case 'h':
                  pstat = ch->hit;
                  break;

               case 'H':
                  pstat = ch->max_hit;
                  break;

               case 'i':
                  if( ( !is_npc( ch ) && xIS_SET( ch->act, PLR_WIZINVIS ) )
                  || ( is_npc( ch ) && xIS_SET( ch->act, ACT_MOBINVIS ) ) )
                     snprintf( pbuf, sizeof( buf ), "(Invis %d) ", ( is_npc( ch ) ? ch->mobinvis : ch->pcdata->wizinvis ) );
                  else if( IS_AFFECTED( ch, AFF_INVISIBLE ) )
                     mudstrlcpy( pbuf, "(Invis) ", sizeof( buf ) );
                  break;

               case 'I':
                  pstat = ( is_npc( ch ) ? ( xIS_SET( ch->act, ACT_MOBINVIS ) ? ch->mobinvis : 0 )
                     : ( xIS_SET( ch->act, PLR_WIZINVIS ) ? ch->pcdata->wizinvis : 0 ) );
                  break;

               case 'j':
               case 'J':
                  mudstrlcpy( pbuf, "\r\n", sizeof( buf ) );
                  break;

               case 'b':
               case 'm':
                  pstat = ch->mana;
                  break;

               case 'B':
               case 'M':
                  pstat = ch->max_mana;
                  break;

               case 'N':
                  if( ch->fighting && ( victim = ch->fighting->who )
                  && victim->fighting && ( victim = victim->fighting->who ) )
                  {
                     if( ch == victim )
                        mudstrlcpy( pbuf, "You", sizeof( buf ) );
                     else if( is_npc( victim ) && victim->short_descr )
                        mudstrlcpy( pbuf, victim->short_descr, sizeof( buf ) );
                     else if( victim->name )
                        mudstrlcpy( pbuf, victim->name, sizeof( buf ) );
                     if( pbuf )
                        pbuf[0] = UPPER( pbuf[0] );
                  }
                  break;

               case 'n':
                  if( ch->fighting && ( victim = ch->fighting->who ) )
                  {
                     if( ch == victim )
                        mudstrlcpy( pbuf, "You", sizeof( buf ) );
                     else if( is_npc( victim ) && victim->short_descr )
                        mudstrlcpy( pbuf, victim->short_descr, sizeof( buf ) );
                     else if( victim->name )
                        mudstrlcpy( pbuf, victim->name, sizeof( buf ) );
                     if( pbuf )
                        pbuf[0] = UPPER( pbuf[0] );
                  }
                  break;

               case 'q':
                  pstat = ch->questcountdown;
                  break;

               case 'r':
                  if( is_immortal( och ) )
                     pstat = ch->in_room->vnum;
                  break;

               case 'S':
                  mudstrlcpy( pbuf, style_names[ch->style], sizeof( buf ) );
                  break;

               case 't':
                  pstat = time_info.hour;
                  break;

               case 'T':
                  if( time_info.hour < 5 )
                     mudstrlcpy( pbuf, "night", sizeof( buf ) );
                  else if( time_info.hour < 6 )
                     mudstrlcpy( pbuf, "dawn", sizeof( buf ) );
                  else if( time_info.hour < 19 )
                     mudstrlcpy( pbuf, "day", sizeof( buf ) );
                  else if( time_info.hour < 21 )
                     mudstrlcpy( pbuf, "dusk", sizeof( buf ) );
                  else
                     mudstrlcpy( pbuf, "night", sizeof( buf ) );
                  break;

               case 'u':
                  pstat = num_descriptors;
                  break;

               case 'U':
                  pstat = sysdata.maxplayers;
                  break;

               case 'v':
                  pstat = ch->move;
                  break;

               case 'V':
                  pstat = ch->max_move;
                  break;
            }
            if( pstat != 0x80000000 )
               snprintf( pbuf, ( sizeof( buf ) - strlen( buf ) ), "%d", pstat );
            pbuf += strlen( pbuf );
            break;
      }
   }
   *pbuf = '\0';
   send_to_char( buf, ch );
}

void set_pager_input( DESCRIPTOR_DATA *d, char *argument )
{
   while( isspace( *argument ) )
      argument++;
   d->pagecmd = *argument;
}

bool pager_output( DESCRIPTOR_DATA *d )
{
   register char *last;
   CHAR_DATA *ch;
   register int lines;
   int pclines;
   bool ret;

   if( !d || !d->pagepoint || d->pagecmd == -1 )
      return true;
   ch = d->character;
   pclines = UMAX( ch->pcdata->pagerlen, 5 ) - 1;
   switch( LOWER( d->pagecmd ) )
   {
      default:
         lines = 0;
         break;

      case 'b':
         lines = -1 - ( pclines * 2 );
         break;

      case 'r':
         lines = -1 - pclines;
         break;

      case 'n':
         lines = 0;
         pclines = 0x7FFFFFFF;   /* As many lines as possible */
         break;

      case 'q':
         d->pagetop = 0;
         d->pagepoint = NULL;
         flush_buffer( d, true );
         DISPOSE( d->pagebuf );
         d->pagesize = MSL;
         return true;
   }
   while( lines < 0 && d->pagepoint >= d->pagebuf )
   {
      if( *( --d->pagepoint ) == '\n' )
         ++lines;
   }
   if( *d->pagepoint == '\r' && *( ++d->pagepoint ) == '\n' )
      ++d->pagepoint;
   if( d->pagepoint < d->pagebuf )
      d->pagepoint = d->pagebuf;
   for( lines = 0, last = d->pagepoint; lines < pclines; ++last )
   {
      if( !*last )
         break;
      else if( *last == '\n' )
         ++lines;
   }
   if( *last == '\r' )
      ++last;
   if( last != d->pagepoint )
   {
      if( !write_to_descriptor( d, d->pagepoint, ( last - d->pagepoint ) ) )
         return false;
      d->pagepoint = last;
   }
   while( isspace( *last ) )
      ++last;
   if( !*last )
   {
      d->pagetop = 0;
      d->pagepoint = NULL;
      flush_buffer( d, true );
      DISPOSE( d->pagebuf );
      d->pagesize = MSL;
      return true;
   }
   d->pagecmd = -1;
   if( xIS_SET( ch->act, PLR_ANSI ) )
      if( write_to_descriptor( d, ANSI_LBLUE, 0 ) == false )
         return false;
   if( ( ret = write_to_descriptor( d, "(C)ontinue, (N)on-stop, (R)efresh, (B)ack, (Q)uit: [C] ", 0 ) ) == false )
      return false;
   if( xIS_SET( ch->act, PLR_ANSI ) )
   {
      char buf[32];

      snprintf( buf, sizeof( buf ), "%s", color_str( d->pagecolor, ch ) );
      ret = write_to_descriptor( d, buf, 0 );
   }
   return ret;
}

#ifdef WIN32

void shutdown_mud( const char *reason );

void bailout( void )
{
   echo_to_all( AT_IMMORT, "MUD shutting down by system operator NOW!!", ECHOTAR_ALL );
   shutdown_mud( "MUD shutdown by system operator" );
   log_string( "MUD shutdown by system operator" );
   Sleep( 5000 ); /* give "echo_to_all" time to display */
   mud_down = true;  /* This will cause game_loop to exit */
   service_shut_down = true;  /* This will cause characters to be saved */
   fflush( stderr );
}

#endif

bool exists_file( char *name )
{
   struct stat fst;

   if( !name || name[0] == '\0' )
      return false;
   if( stat( name, &fst ) != -1 )
      return true;
   else
      return false;
}

/* This way we only remove the oldest log instead of just removing all them */
void remove_oldest_log( void )
{
   DIR *dp;
   struct dirent *de;
   struct stat fst;
   char buf[MSL];
   static char oldestname[MSL];
   time_t oldesttime = current_time;

   if ( !( dp = opendir( "log/" ) ) )
   {
      bug( "%s: can't open log/", __FUNCTION__ );
      perror( "log/" );
      return;
   }

   oldestname[0] = '\0';

   /* Ok have the directory open so lets check the files and the time and remove the oldest one */
   while( ( de = readdir ( dp ) ) )
   {
      if ( de->d_name[0] == '.' )
         continue;

      snprintf ( buf, sizeof ( buf ), "log/%s", de->d_name );
      if( stat( buf, &fst ) == -1 )
         continue;

      if( oldesttime > fst.st_mtime ) /* The oldest has the lowest time */
      {
         oldesttime = fst.st_mtime;
         snprintf( oldestname, sizeof( oldestname ), "log/%s", de->d_name );
      }
   }
   closedir ( dp );
   dp = NULL;

   if( oldestname != NULL && oldestname[0] != '\0' && !remove( oldestname ) )
      log_printf( "%s: %s has been deleted to keep the log files down.", __FUNCTION__, oldestname );
}

void open_mud_log( void )
{
   FILE *error_log;
   char buf[MIL];
   int logindex, tcount = 0;

   for( logindex = 1000; ; logindex++ )
   {
      snprintf( buf, sizeof( buf ), "log/%d.log", logindex );
      if( exists_file( buf ) )
         continue;
      if( logindex > 1025 )
      {
         remove_oldest_log( );
         if( ++tcount > 5 ) /* Don't allow a constant loop if for some reason it can't remove a log */
         {
            bug( "%s: failed to open a new mud log.", __FUNCTION__ );
            return;
         }
         logindex = 999;
         continue;
      }
      break;
   }
   if( !( error_log = fopen( buf, "a" ) ) )
   {
      fprintf( stderr, "Unable to append to %s.", buf );
      exit( 1 );
   }

   dup2( fileno( error_log ), STDERR_FILENO );
   fclose( error_log );
   error_log = NULL;
}
