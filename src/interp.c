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
 *			 Command interpretation module			     *
 *****************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "h/mud.h"

/* Log-all switch. */
bool fLogAll = false;

CMDTYPE *command_hash[126];    /* hash table for cmd_table */
SOCIALTYPE *social_index[126]; /* hash table for socials   */

void show_similar_commands( CHAR_DATA *ch, char *command )
{
   CMDTYPE *cmd;
   int hash;
   short matched = 0, checked = 0, totalmatched = 0, found = 0;

   if( !command || command[0] == '\0' )
      return;

   hash = LOWER( command[0] ) % 126;

   send_to_pager( "Similar Commands:\r\n", ch );
   for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
   {
      matched = 0;

      if( cmd->perm > get_trust( ch ) )
         continue;

      /* Lets check only up to 10 spots */
      for( checked = 0; checked <= 10; checked++ )
      {
         if( !cmd->name[checked] || !command[checked] )
            break;
         if( LOWER( cmd->name[checked] ) == LOWER( command[checked] ) )
            matched++;
      }

      if( ( matched > 1 && matched > ( checked / 2 ) ) || ( matched > 0 && checked < 2 ) )
      {
         pager_printf( ch, " %-20s ", cmd->name );
         if( ++found == 4 )
         {
            found = 0;
            send_to_pager( "\r\n", ch );
         }
         totalmatched++;
      }
   }
   if( found != 0 )
      send_to_pager( "\r\n", ch );
   if( totalmatched == 0 )
      send_to_pager( "No similar commands found.\r\n", ch );
}

/* Character not in position for command? */
bool check_pos( CHAR_DATA *ch, short position )
{
   if( is_npc( ch ) && ch->position > 3 ) /*Band-aid alert?  -- Blod */
      return true;

   if( ch->position < position )
   {
      switch( ch->position )
      {
         case POS_DEAD:
            send_to_char( "A little difficult to do when you're DEAD...\r\n", ch );
            break;

         case POS_MORTAL:
         case POS_INCAP:
            send_to_char( "You're hurt far too bad for that.\r\n", ch );
            break;

         case POS_STUNNED:
            send_to_char( "You're too stunned to do that.\r\n", ch );
            break;

         case POS_SLEEPING:
            send_to_char( "In your dreams, or what?\r\n", ch );
            break;

         case POS_RESTING:
            send_to_char( "Nah... You feel too relaxed...\r\n", ch );
            break;

         case POS_SITTING:
            send_to_char( "You can't do that sitting down.\r\n", ch );
            break;

         case POS_FIGHTING:
            if( position <= POS_EVASIVE )
               send_to_char( "This fighting style is too demanding for that!\r\n", ch );
            else
               send_to_char( "No way!  You're still fighting!\r\n", ch );
            break;

         case POS_DEFENSIVE:
            if( position <= POS_EVASIVE )
               send_to_char( "This fighting style is too demanding for that!\r\n", ch );
            else
               send_to_char( "No way!  You're still fighting!\r\n", ch );
            break;

         case POS_AGGRESSIVE:
            if( position <= POS_EVASIVE )
               send_to_char( "This fighting style is too demanding for that!\r\n", ch );
            else
               send_to_char( "No way!  You're still fighting!\r\n", ch );
            break;

         case POS_BERSERK:
            if( position <= POS_EVASIVE )
               send_to_char( "This fighting style is too demanding for that!\r\n", ch );
            else
               send_to_char( "No way!  You're still fighting!\r\n", ch );
            break;

         case POS_EVASIVE:
            send_to_char( "No way!  You're still fighting!\r\n", ch );
            break;
      }
      return false;
   }
   return true;
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret( CHAR_DATA *ch, char *argument )
{
   char command[MIL], logline[MIL], logname[MIL], log_buf[MSL], *buf;
   TIMER *timer = NULL;
   CMDTYPE *cmd = NULL;
   int trust, loglvl;
   bool found;
   struct timeval time_used;
   long tmptime;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: null in_room!", __FUNCTION__ );
      return;
   }

   found = false;
   if( ch->substate == SUB_REPEATCMD )
   {
      DO_FUN *fun;

      if( !( fun = ch->last_cmd ) )
      {
         ch->substate = SUB_NONE;
         bug( "%s: SUB_REPEATCMD with NULL last_cmd", __FUNCTION__ );
         return;
      }
      else
      {
         int x;

         /*
          * yes... we lose out on the hashing speediness here...
          * but the only REPEATCMDS are wizcommands (currently)
          */
         for( x = 0; x < 126; x++ )
         {
            for( cmd = command_hash[x]; cmd; cmd = cmd->next )
            {
               if( cmd->do_fun == fun )
               {
                  found = true;
                  break;
               }
            }
            if( found )
               break;
         }
         if( !found )
         {
            cmd = NULL;
            bug( "%s: SUB_REPEATCMD: last_cmd invalid", __FUNCTION__ );
            return;
         }
         snprintf( logline, sizeof( logline ), "(%s) %s", cmd->name, argument );
      }
   }

   if( !cmd )
   {
      /* Changed the order of these ifchecks to prevent crashing. */
      if( !argument || argument[0] == '\0' )
      {
         bug( "%s: null argument!", __FUNCTION__ );
         return;
      }

      /* Strip leading spaces. */
      while( isspace( *argument ) )
         argument++;

      if( !argument || argument[0] == '\0' )
         return;

      /* Implement freeze command. */
      if( !is_npc( ch ) && xIS_SET( ch->act, PLR_FREEZE ) )
      {
         send_to_char( "You're totally frozen!\r\n", ch );
         return;
      }

      /*
       * Grab the command word.
       * Special parsing so ' can be a command, also no spaces needed after punctuation.
       */
      mudstrlcpy( logline, argument, sizeof( logline ) );
      if( !isalpha( argument[0] ) && !isdigit( argument[0] ) )
      {
         command[0] = argument[0];
         command[1] = '\0';
         argument++;
         while( isspace( *argument ) )
            argument++;
      }
      else
         argument = one_argument( argument, command );

      /*
       * Look for command in command table.
       * Check for council powers and/or bestowments
       */
      trust = get_trust( ch );
      for( cmd = command_hash[LOWER( command[0] ) % 126]; cmd; cmd = cmd->next )
      {
         if( !str_prefix( command, cmd->name )
         && ( cmd->perm <= trust || ( !is_npc( ch ) && ch->pcdata->council && is_name( cmd->name, ch->pcdata->council->powers ) && cmd->perm <= ( trust + MAX_CPD ) )
         || ( !is_npc( ch ) && ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0'
            && ( is_name( cmd->name, ch->pcdata->bestowments ) || is_name( groups_flag[cmd->group], ch->pcdata->bestowments ) )
            && cmd->perm <= ( trust + sysdata.bestow_dif ) ) ) )
         {
            found = true;
            break;
         }
      }
      /* Turn off afk bit when any command performed. */
      if( !is_npc( ch ) && xIS_SET( ch->act, PLR_AFK ) && ( str_cmp( command, "AFK" ) ) )
      {
         xREMOVE_BIT( ch->act, PLR_AFK );
         act( AT_GRAY, "$n is no longer afk.", ch, NULL, NULL, TO_CANSEE );
#ifdef IMC
         if( IMCIS_SET( IMCFLAG( ch ), IMC_AFK ) )
         {
            send_to_char( "You're no longer AFK to IMC2.\r\n", ch );
            IMCREMOVE_BIT( IMCFLAG( ch ), IMC_AFK );
         }
#endif
      }
   }

   /* Log and snoop. */
   snprintf( lastplayercmd, sizeof( lastplayercmd ), "%s used %s", ch->name, logline );

   if( found && cmd->log == LOG_NEVER )
      mudstrlcpy( logline, "XXXXXXXX XXXXXXXX XXXXXXXX", sizeof( logline ) );

   loglvl = found ? cmd->log : LOG_NORMAL;

   if( ( ch->logged )
   || fLogAll || loglvl == LOG_BUILD || loglvl == LOG_HIGH || loglvl == LOG_ALWAYS )
   {
      /* Added by Narn to show who is switched into a mob that executes a logged command. */
      snprintf( log_buf, sizeof( log_buf ), "Log %s: %s", ch->name, logline );

      /*
       * Make it so a 'log all' will send most output to the log
       * file only, and not spam the log channel to death   -Thoric
       */
      if( fLogAll && loglvl == LOG_NORMAL && ( !ch->logged ) )
         loglvl = LOG_ALL;

      log_string_plus( log_buf, loglvl, URANGE( 0, ( get_trust( ch ) + 1 ), PERM_MAX ) );
   }

   if( ch->desc && ch->desc->snoop_by )
   {
      snprintf( logname, sizeof( logname ), "%s", ch->name );
      write_to_buffer( ch->desc->snoop_by, logname, 0 );
      write_to_buffer( ch->desc->snoop_by, "% ", 2 );
      write_to_buffer( ch->desc->snoop_by, logline, 0 );
      write_to_buffer( ch->desc->snoop_by, "\r\n", 2 );
   }

   /* check for a timer delayed command (search, dig, detrap, etc) */
   if( ( timer = get_timerptr( ch, TIMER_DO_FUN ) ) )
   {
      int tempsub;

      tempsub = ch->substate;
      ch->substate = SUB_TIMER_DO_ABORT;
      ( timer->do_fun ) ( ch, (char *)"" );
      if( char_died( ch ) )
         return;
      if( ch->substate != SUB_TIMER_CANT_ABORT )
      {
         ch->substate = tempsub;
         extract_timer( ch, timer );
      }
      else
      {
         ch->substate = tempsub;
         return;
      }
   }

   /* Look for command in skill and socials table. */
   if( !found )
   {
      if( !check_skill( ch, command, argument )
      && !check_social( ch, command, argument )
      && !check_channel( ch, command, argument )
#ifdef IMC
      && !imc_command_hook( ch, command, argument )
#endif
      )
      {
         EXIT_DATA *pexit;

         /* check for an auto-matic exit command */
         if( ( pexit = find_door( ch, command, true ) )
         && xIS_SET( pexit->exit_info, EX_xAUTO ) )
         {
            if( xIS_SET( pexit->exit_info, EX_CLOSED )
            && ( !IS_AFFECTED( ch, AFF_PASS_DOOR )
            || xIS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
            {
               if( !xIS_SET( pexit->exit_info, EX_SECRET ) )
                  act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
               else
                  send_to_char( "You can't do that here.\r\n", ch );
               return;
            }
            if( check_pos( ch, POS_STANDING ) )
               move_char( ch, pexit, 0, false );
            return;
         }
         show_similar_commands( ch, command );
      }
      return;
   }

   /* Character not in position for command? */
   if( !check_pos( ch, cmd->position ) )
      return;

   /*
    * So we can check commands for things like Polymorph
    * But still keep the online editing ability.  -- Shaddai
    * Send back the message to print out, so we have the option
    * this function might be usefull elsewhere.  Also using the
    * send_to_char so we can colorize the strings if need be. --Shaddai
    */
   if( ( buf = check_cmd_flags( ch, cmd ) ) && buf[0] != '\0' )
   {
      send_to_char( buf, ch );
      return;
   }

   if( xIS_SET( cmd->flags, CMD_FULL_NAME ) && str_cmp( command, cmd->name ) )
   {
      send_to_char( "You have to use the full name for that command to work.\r\n", ch );
      return;
   }

   /* Only allow tilde's if the command is set to */
   if( !xIS_SET( cmd->flags, CMD_FLAG_ALLOW_TILDE ) )
      smash_tilde( argument );

   /* Dispatch the command. */
   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = cmd->do_fun;
   start_timer( &time_used );
   ( *cmd->do_fun ) ( ch, argument );
   end_timer( &time_used );

   /* Update the record of how many times this command has been used (haus) */
   update_userec( &time_used, &cmd->userec );
   tmptime = UMIN( time_used.tv_sec, 19 ) * 1000000 + time_used.tv_usec;

   /* laggy command notice: command took longer than 1.5 seconds */
   if( tmptime > 1500000 )
   {
      log_printf_plus( LOG_NORMAL, get_trust( ch ), "[*****] LAG: %s: %s %s (R:%d S:%ld.%06ld)", ch->name,
         cmd->name, ( cmd->log == LOG_NEVER ? "XXX" : argument ),
         ch->in_room ? ch->in_room->vnum : 0, time_used.tv_sec, time_used.tv_usec );
      cmd->lag_count++; /* count the lag flags */
   }

   tail_chain( );
}

void interpret_printf( CHAR_DATA *ch, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   interpret( ch, buf );
}

/* Return true if an argument is completely numeric. */
bool is_number( char *arg )
{
   bool first = true;

   if( !arg || arg[0] == '\0' )
      return false;

   for( ; *arg != '\0'; arg++ )
   {
      if( first && *arg == '-' )
      {
         first = false;
         continue;
      }
      if( !isdigit( *arg ) )
         return false;
      first = false;
   }

   return true;
}

/* Given a string like 14.foo, return 14 and 'foo' */
int number_argument( char *argument, char *arg )
{
   char *pdot;
   int number;

   for( pdot = argument; *pdot != '\0'; pdot++ )
   {
      if( *pdot == '.' )
      {
         *pdot = '\0';
         number = atoi( argument );
         *pdot = '.';
         strcpy( arg, pdot + 1 );
         return number;
      }
   }

   strcpy( arg, argument );
   return 1;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
char *one_argument( char *argument, char *arg_first )
{
   char cEnd;
   short count;

   count = 0;

   if( !argument || argument[0] == '\0' )
   {
      arg_first[0] = '\0';
      return (char *)"\0";
   }

   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }
      if( *argument == '\n' )
         break;
      *arg_first = *argument;
      arg_first++;
      argument++;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      argument++;
   return argument;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.  Delimiters = { ' ', '-' }
 */
char *one_argument2( char *argument, char *arg_first )
{
   char cEnd;
   short count;

   count = 0;

   if( !argument || argument[0] == '\0' )
   {
      arg_first[0] = '\0';
      return argument;
   }

   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == cEnd || *argument == '-' )
      {
         argument++;
         break;
      }
      *arg_first = *argument;
      arg_first++;
      argument++;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      argument++;

   return argument;
}

CMDF( do_timecmd )
{
   struct timeval sttime;
   struct timeval etime;
   static bool timing;
   extern CHAR_DATA *timechar;
   char arg[MIL];

   send_to_char( "Timing\r\n", ch );
   if( timing )
      return;
   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "No command to time.\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "update" ) )
   {
      if( timechar )
         send_to_char( "Another person is already timing updates.\r\n", ch );
      else
      {
         timechar = ch;
         send_to_char( "Setting up to record next update loop.\r\n", ch );
      }
      return;
   }
   set_char_color( AT_PLAIN, ch );
   send_to_char( "Starting timer.\r\n", ch );
   timing = true;
   gettimeofday( &sttime, NULL );
   interpret( ch, argument );
   gettimeofday( &etime, NULL );
   timing = false;
   set_char_color( AT_PLAIN, ch );
   send_to_char( "Timing complete.\r\n", ch );
   subtract_times( &etime, &sttime );
   ch_printf( ch, "Timing took %ld.%06ld seconds.\r\n", etime.tv_sec, etime.tv_usec );
}

void start_timer( struct timeval *sttime )
{
   if( !sttime )
   {
      bug( "%s: NULL sttime.", __FUNCTION__ );
      return;
   }
   gettimeofday( sttime, NULL );
}

time_t end_timer( struct timeval *sttime )
{
   struct timeval etime;

   /* Mark etime before checking stime, so that we get a better reading.. */
   gettimeofday( &etime, NULL );
   if( !sttime || ( !sttime->tv_sec && !sttime->tv_usec ) )
   {
      bug( "%s: bad sttime.", __FUNCTION__ );
      return 0;
   }
   subtract_times( &etime, sttime );
   /* sttime becomes time used */
   *sttime = etime;
   return ( etime.tv_sec * 1000000 ) + etime.tv_usec;
}

void send_timer( struct timerset *vtime, CHAR_DATA *ch )
{
   struct timeval ntime;
   int carry;

   if( vtime->num_uses == 0 )
      return;
   ntime.tv_sec = vtime->total_time.tv_sec / vtime->num_uses;
   carry = ( vtime->total_time.tv_sec % vtime->num_uses ) * 1000000;
   ntime.tv_usec = ( vtime->total_time.tv_usec + carry ) / vtime->num_uses;
   ch_printf( ch, "Has been used %d times this boot.\r\n", vtime->num_uses );
   ch_printf( ch, "Time (in secs): min %ld.%06ld; avg: %ld.%06ld; max %ld.%06ld"
      "\r\n", vtime->min_time.tv_sec, vtime->min_time.tv_usec, ntime.tv_sec,
      ntime.tv_usec, vtime->max_time.tv_sec, vtime->max_time.tv_usec );
}

void update_userec( struct timeval *time_used, struct timerset *userec )
{
   userec->num_uses++;
   if( !timerisset( &userec->min_time ) || timercmp( time_used, &userec->min_time, < ) )
   {
      userec->min_time.tv_sec = time_used->tv_sec;
      userec->min_time.tv_usec = time_used->tv_usec;
   }
   if( !timerisset( &userec->max_time ) || timercmp( time_used, &userec->max_time, > ) )
   {
      userec->max_time.tv_sec = time_used->tv_sec;
      userec->max_time.tv_usec = time_used->tv_usec;
   }
   userec->total_time.tv_sec += time_used->tv_sec;
   userec->total_time.tv_usec += time_used->tv_usec;
   while( userec->total_time.tv_usec >= 1000000 )
   {
      userec->total_time.tv_sec++;
      userec->total_time.tv_usec -= 1000000;
   }
}

/*
 *  This function checks the command against the command flags to make
 *  sure they can use the command online.  This allows the commands to be
 *  edited online to allow or disallow certain situations.  May be an idea
 *  to rework this so we can edit the message sent back online, as well as
 *  maybe a crude parsing language so we can add in new checks online without
 *  haveing to hard-code them in.     -- Shaddai   August 25, 1997
 */

/* Needed a global here */
char cmd_flag_buf[MSL];

char *check_cmd_flags( CHAR_DATA *ch, CMDTYPE *cmd )
{

   if( ch->morph && xIS_SET( cmd->flags, CMD_FLAG_POLYMORPHED ) )
      snprintf( cmd_flag_buf, sizeof( cmd_flag_buf ), "You can't %s while you're polymorphed!\r\n", cmd->name );
   else if( xIS_SET( cmd->flags, CMD_FLAG_NPC ) && ( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) )
      snprintf( cmd_flag_buf, sizeof( cmd_flag_buf ), "You can't use %s unless your an uncharmed and unswitched npc!\r\n", cmd->name );
   else if( xIS_SET( cmd->flags, CMD_FLAG_PC ) && ( is_npc( ch ) || !ch->desc ) )
      snprintf( cmd_flag_buf, sizeof( cmd_flag_buf ), "You can't use %s unless your a pc with a desc!\r\n", cmd->name );
   else
      cmd_flag_buf[0] = '\0';

   return cmd_flag_buf;
}
