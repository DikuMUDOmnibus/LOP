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
#include <string.h>
#include "h/mud.h"

int top_command;

void add_command( CMDTYPE *command, bool cinsert )
{
   CMDTYPE *tmp, *prev;
   int x;

   if( !command )
   {
      bug( "%s: NULL command", __FUNCTION__ );
      return;
   }

   if( !command->name )
   {
      bug( "%s: NULL command->name", __FUNCTION__ );
      return;
   }

   if( !command->do_fun )
   {
      bug( "%s: NULL command->do_fun", __FUNCTION__ );
      return;
   }

   for( x = 0; command->name[x] != '\0'; x++ )
      command->name[x] = LOWER( command->name[x] );

   top_command++;
   HASH_LINK( command, command_hash, tmp, prev, cinsert );
}

CMDTYPE *find_command( char *command, bool exact )
{
   CMDTYPE *cmd;
   int hash;

   if( !command || command[0] == '\0' )
      return NULL;

   hash = LOWER( command[0] ) % 126;

   for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
   {
      if( ( !exact && !str_prefix( command, cmd->name ) )
      || ( exact && !str_cmp( command, cmd->name ) ) )
         return cmd;
   }

   return NULL;
}

void free_command( CMDTYPE *command )
{
   STRFREE( command->name );
   STRFREE( command->fun_name );
   STRFREE( command->htext );
   DISPOSE( command );
   top_command--;
}

void fread_command( FILE *fp )
{
   CMDTYPE *command;
   const char *word;
   char *infoflags, flag[MSL];
   int value;
   bool fMatch;

   CREATE( command, CMDTYPE, 1 );
   command->lag_count = 0; /* can't have caused lag yet... FB */
   xCLEAR_BITS( command->flags );
   command->htext = NULL;
   command->perm = 0;
   command->group = 0;

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

         case 'C':
            KEY( "Code", command->fun_name, STRALLOC( fread_word( fp ) ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !command->name )
               {
                  bug( "%s: Name not found", __FUNCTION__ );
                  free_command( command );
                  return;
               }

               if( !command->fun_name )
               {
                  bug( "%s: No function name supplied for %s", __FUNCTION__, command->name );
                  free_command( command );
                  return;
               }

               if( command->htext && command->htext[0] == '.' && command->htext[1] == ' ' )
               {
                  char *tmptext = un_fix_help( command->htext );
                  STRSET( command->htext, tmptext );
               }

               /*
                * Mods by Trax
                * Fread in code into char* and try linkage here then
                * deal in the "usual" way I suppose..
                */
	       command->do_fun = skill_function( command->fun_name );
               if( command->do_fun == skill_notfound )
               {
                  bug( "%s: Function %s not found for %s", __FUNCTION__, command->fun_name, command->name );
                  free_command( command );
                  return;
               }

               add_command( command, false );
               add_command_help( command, false );
               return;
            }
            break;

         case 'F':
            WEXTKEY( "Flags", command->flags, fp, cmd_flags, CMD_MAX );
            break;

         case 'G':
            SKEY( "Group", command->group, fp, groups_flag, GROUP_MAX );
            break;

         case 'H':
            KEY( "HText", command->htext, fread_string( fp ) );
            break;

         case 'L':
            KEY( "Log", command->log, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", command->name, fread_string( fp ) );
            break;

         case 'P':
            SKEY( "Position", command->position, fp, pos_names, POS_MAX );
            SKEY( "Perm", command->perm, fp, perms_flag, PERM_MAX );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void load_commands( void )
{
   FILE *fp;

   top_command = 0;

   if( !( fp = fopen( COMMAND_FILE, "r" ) ) )
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
         bug( "%s: # not found, (%c) found instead.", __FUNCTION__, letter );
         break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "COMMAND" ) )
      {
         fread_command( fp );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section (%s).", __FUNCTION__, word );
         continue;
      }
   }
   fclose( fp );
   fp = NULL;
}

void save_commands( bool autosave )
{
   FILE *fp;
   CMDTYPE *command;
   int x;
   bool found = false;

   if( autosave && !sysdata.autosavecommands )
      return;

   if( !( fp = fopen( COMMAND_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing", __FUNCTION__, COMMAND_FILE );
      perror( COMMAND_FILE );
      return;
   }

   for( x = 0; x < 126; x++ )
   {
      for( command = command_hash[x]; command; command = command->next )
      {
         if( !command->name || command->name[0] == '\0' )
         {
            bug( "%s: blank command in hash bucket %d", __FUNCTION__, x );
            continue;
         }
         found = true;
         fprintf( fp, "#COMMAND\n" );
         fprintf( fp, "Name        %s~\n", command->name );
         fprintf( fp, "Code        %s\n", command->fun_name ? command->fun_name : "" );
         fprintf( fp, "Position    %s~\n", pos_names[command->position] );
         fprintf( fp, "Perm        %s~\n", perms_flag[command->perm] );
         fprintf( fp, "Group       %s~\n", groups_flag[command->group] );
         fprintf( fp, "Log         %d\n", command->log );
         if( command->htext )
            fprintf( fp, "HText       %s~\n", help_fix( command->htext ) );
         if( !xIS_EMPTY( command->flags ) )
            fprintf( fp, "Flags       %s~\n", ext_flag_string( &command->flags, cmd_flags ) );
         fprintf( fp, "End\n\n" );
      }
   }
   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
   if( !found )
      remove_file( COMMAND_FILE );
}

void free_commands( void )
{
   CMDTYPE *command, *cmd_next;
   int hash;

   for( hash = 0; hash < 126; hash++ )
   {
      for( command = command_hash[hash]; command; command = cmd_next )
      {
         cmd_next = command->next;
         command->next = NULL;
         command->do_fun = NULL;
         free_command( command );
      }
   }
}

void unlink_command( CMDTYPE *command )
{
   CMDTYPE *tmp;

   if( !command )
   {
      bug( "%s: NULL command", __FUNCTION__ );
      return;
   }

   if( !command->name || command->name[0] == '\0' )
   {
      bug( "%s: command with invalid name", __FUNCTION__ );
      return;
   }

   HASH_UNLINK( command, command_hash, tmp );
}

CMDF( do_cedit )
{
   CMDTYPE *command;
   char arg1[MIL], arg2[MIL];

   if( !ch->desc )
      return;
   set_char_color( AT_IMMORT, ch );

   switch( ch->substate )
   {
      default:
         break;

      case SUB_CHELP_EDIT:
         if( !( command = ( CMDTYPE * ) ch->dest_buf ) )
         {
            bug( "%s: sub_chelp_edit: NULL ch->dest_buf", __FUNCTION__ );
            stop_editing( ch );
            return;
         }
         STRFREE( command->htext );
         command->htext = copy_buffer( ch );
         stop_editing( ch );
         add_command_help( command, true );
         save_commands( true );
         return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: cedit save cmdtable\r\n", ch );
      if( get_trust( ch ) >= PERM_HEAD )
      {
         send_to_char( "Usage: cedit save cmdtable\r\n", ch );
         send_to_char( "Usage: cedit <command> help\r\n", ch );
         send_to_char( "Usage: cedit <command> create [code]\r\n", ch );
         send_to_char( "Usage: cedit <command> [field] [values]\r\n", ch );
         send_to_char( "   Fields: delete show raise lower list perm group position log code flags\r\n", ch );
      }
      return;
   }

   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( arg1, "save" ) && !str_cmp( arg2, "cmdtable" ) )
   {
      save_commands( false );
      send_to_char( "Saved.\r\n", ch );
      return;
   }

   command = find_command( arg1, false );
   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( arg2, "create" ) )
   {
      /* Reget the command but this time check for the exact command */
      if( ( command = find_command( arg1, true ) ) )
      {
         send_to_char( "That command already exists!\r\n", ch );
         return;
      }
      CREATE( command, CMDTYPE, 1 );
      command->lag_count = 0; /* FB */
      command->group = 0;
      command->name = STRALLOC( arg1 );
      command->perm = get_trust( ch );
      if( *argument )
         one_argument( argument, arg2 );
      else
         snprintf( arg2, sizeof( arg2 ), "do_%s", arg1 );
      command->do_fun = skill_function( arg2 );
      command->fun_name = STRALLOC( arg2 );
      add_command( command, true );
      save_commands( true );
      send_to_char( "Command added.\r\n", ch );
      if( command->do_fun == skill_notfound )
         ch_printf( ch, "Code %s not found.  Set to no code.\r\n", arg2 );
      return;
   }

   if( !command )
   {
      send_to_char( "Command not found.\r\n", ch );
      return;
   }
   else if( command->perm > get_trust( ch ) )
   {
      send_to_char( "You can't touch this command.\r\n", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
   {
      ch_printf( ch, "Command:  %s\r\n", command->name );
      ch_printf( ch, "Perm:     %s\r\n", perms_flag[command->perm] );
      ch_printf( ch, "Group:    %s\r\n", groups_flag[command->group] );
      ch_printf( ch, "Position: %s\r\n", pos_names[command->position] );
      ch_printf( ch, "Log:      %d\r\n", command->log );
      ch_printf( ch, "Code:     %s\r\n", command->fun_name );
      ch_printf( ch, "Flags:    %s\r\n", ext_flag_string( &command->flags, cmd_flags ) );
      if( command->userec.num_uses )
         send_timer( &command->userec, ch );
      if( command->htext )
      {
         send_to_char( ".~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", ch );
         send_to_char( command->htext, ch );
         send_to_char( ".~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", ch );
      }
      return;
   }

   if( get_trust( ch ) < PERM_HEAD )
   {
      do_cedit( ch, (char *)"" );
      return;
   }

   if( !str_cmp( arg2, "help" ) )
   {
      ch->substate = SUB_CHELP_EDIT;
      ch->dest_buf = command;
      start_editing( ch, command->htext );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      CMDTYPE *tmp, *tmp_next;
      int hash = command->name[0] % 126;

      if( ( tmp = command_hash[hash] ) == command )
      {
         send_to_char( "That command is already at the top.\r\n", ch );
         return;
      }
      if( tmp->next == command )
      {
         command_hash[hash] = command;
         tmp_next = tmp->next;
         tmp->next = command->next;
         command->next = tmp;
         save_commands( true );
         ch_printf( ch, "Moved %s above %s.\r\n", command->name, command->next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         tmp_next = tmp->next;
         if( tmp_next->next == command )
         {
            tmp->next = command;
            tmp_next->next = command->next;
            command->next = tmp_next;
            save_commands( true );
            ch_printf( ch, "Moved %s above %s.\r\n", command->name, command->next->name );
            return;
         }
      }
      send_to_char( "ERROR -- Not Found!\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "lower" ) )
   {
      CMDTYPE *tmp, *tmp_next;
      int hash = command->name[0] % 126;

      if( !command->next )
      {
         send_to_char( "That command is already at the bottom.\r\n", ch );
         return;
      }
      tmp = command_hash[hash];
      if( tmp == command )
      {
         tmp_next = tmp->next;
         command_hash[hash] = command->next;
         command->next = tmp_next->next;
         tmp_next->next = command;
         save_commands( true );
         ch_printf( ch, "Moved %s below %s.\r\n", command->name, tmp_next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         if( tmp->next == command )
         {
            tmp_next = command->next;
            tmp->next = tmp_next;
            command->next = tmp_next->next;
            tmp_next->next = command;
            save_commands( true );
            ch_printf( ch, "Moved %s below %s.\r\n", command->name, tmp_next->name );
            return;
         }
      }
      send_to_char( "ERROR -- Not Found!\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "list" ) )
   {
      CMDTYPE *tmp;
      int hash = command->name[0] % 126;
      int col = 0;

      pager_printf( ch, "Priority placement for [%s]:\r\n", command->name );
      for( tmp = command_hash[hash]; tmp; tmp = tmp->next )
      {
         pager_printf( ch, "  %s%-12s", tmp == command ? "&[green]" : "&[plain]", tmp->name );
         if( ++col == 6 )
         {
            send_to_pager( "\r\n", ch );
            col = 0;
         }
      }
      if( col != 0 )
         send_to_pager( "\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "delete" ) )
   {
      unlink_command( command );
      free_command( command );
      save_commands( true );
      send_to_char( "Deleted.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "code" ) )
   {
      DO_FUN *fun = skill_function( argument );

      if( fun == skill_notfound )
      {
         send_to_char( "Code not found.\r\n", ch );
         return;
      }
      command->do_fun = fun;
      STRSET( command->fun_name, argument );
      save_commands( true );
      send_to_char( "Code Set.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "perm" ) )
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
      command->perm = level;
      save_commands( true );
      send_to_char( "Perm Set.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "group" ) )
   {
      int level;

      if( is_number( argument ) )
         level = atoi( argument );
      else
         level = get_flag( argument, groups_flag, GROUP_MAX );
      if( level < 0 || level >= GROUP_MAX )
      {
         send_to_char( "Group out of range.\r\n", ch );
         return;
      }
      command->group = level;
      save_commands( true );
      send_to_char( "Group Set.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "log" ) )
   {
      int clog = atoi( argument );

      if( clog < 0 || clog > LOG_COMM )
      {
         send_to_char( "Log out of range.\r\n", ch );
         return;
      }
      command->log = clog;
      save_commands( true );
      send_to_char( "Log Set.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "position" ) )
   {
      int position;

      position = get_flag( argument, pos_names, POS_MAX );
      if( position < 0 || position > POS_DRAG )
      {
         send_to_char( "Position out of range.\r\n", ch );
         return;
      }
      command->position = position;
      save_commands( true );
      send_to_char( "Position Set.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "flags" ) )
   {
      char flag[MSL];
      int value;
      bool fchange = false;

      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, flag );
         value = get_flag( flag, cmd_flags, CMD_MAX );
         if( value < 0 || value >= CMD_MAX )
            ch_printf( ch, "Unknown flag %s.\r\n", flag );
         else
         {
            fchange = true;
            xTOGGLE_BIT( command->flags, value );
            ch_printf( ch, "%s flag %s.\r\n", cmd_flags[value], xIS_SET( command->flags, value ) ? "set" : "unset" );
         }
      }
      if( fchange )
         save_commands( true );
      return;
   }
   if( !str_cmp( arg2, "name" ) )
   {
      CMDTYPE *checkcmd;
      bool relocate;

      one_argument( argument, arg1 );
      if( arg1 == NULL || arg1[0] == '\0' )
      {
         send_to_char( "Can't clear name field!\r\n", ch );
         return;
      }
      if( ( checkcmd = find_command( arg1, true ) ) )
      {
         ch_printf( ch, "There is already a command named %s.\r\n", arg1 );
         return;
      }
      if( arg1[0] != command->name[0] )
      {
         unlink_command( command );
         relocate = true;
      }
      else
         relocate = false;
      STRSET( command->name, arg1 );
      if( relocate )
         add_command( command, true );
      save_commands( true );
      send_to_char( "Name Set.\r\n", ch );
      return;
   }

   do_cedit( ch, (char *)"" );
}

CMDF( do_commands )
{
   CMDTYPE *command;
   char arg[MIL], arg2[MIL];
   int col = 0, cnt = 0, hash, level, minlevel, maxlevel;
   bool found, dislevel, changecolor = true, showhidden = false;

   set_pager_color( AT_PLAIN, ch );
   argument = one_argument( argument, arg );
   if( arg != NULL && arg[0] != '\0' && !str_cmp( arg, "showhidden" ) && get_trust( ch ) >= PERM_IMM )
   {
      argument = one_argument( argument, arg );
      showhidden = true;
   }
   argument = one_argument( argument, arg2 );
   if( arg != NULL && arg[0] != '\0' && is_number( arg ) )
      minlevel = URANGE( 0, atoi( arg ), ( PERM_MAX - 1 ) );
   else
      minlevel = 0;
   if( arg2 != NULL && arg2[0] != '\0' && is_number( arg2 ) )
      maxlevel = URANGE( minlevel, atoi( arg2 ), ( PERM_MAX - 1 ) );
   else if( arg != NULL && arg[0] != '\0' && is_number( arg ) )
      maxlevel = minlevel;
   else
      maxlevel = ( PERM_MAX - 1 );

   found = false;
   for( level = minlevel; level <= maxlevel; level++ )
   {
      dislevel = true;
      col = 0;
      for( hash = 0; hash < 126; hash++ )
      {
         for( command = command_hash[hash]; command; command = command->next )
         {
            if( command->perm != level )
               continue;
            if( command->perm > get_trust( ch ) 
            && ( is_npc( ch ) || !ch->pcdata->council || !is_name( command->name, ch->pcdata->council->powers ) || command->perm > ( get_trust( ch ) + MAX_CPD ) )
            && ( is_npc( ch ) || !ch->pcdata->bestowments || ch->pcdata->bestowments[0] == '\0'
               || ( !is_name( command->name, ch->pcdata->bestowments ) && !is_name( groups_flag[command->group], ch->pcdata->bestowments ) )
               || command->perm > ( get_trust( ch ) + sysdata.bestow_dif ) ) )
               continue;
            if( arg != NULL && arg[0] != '\0' && !is_number( arg ) && command->name && command->name[0] != '\0'
            && str_prefix( arg, command->name ) )
               continue;
            if( xIS_SET( command->flags, CMD_FLAG_NOSHOW ) && !showhidden )
               continue;
            if( !is_npc( ch ) && xIS_SET( command->flags, CMD_FLAG_NPC ) && !showhidden )
               continue;
            if( is_npc( ch ) && xIS_SET( command->flags, CMD_FLAG_PC ) && !showhidden )
               continue;
            found = true;
            cnt++;
            if( dislevel )
            {
               dislevel = false;
               pager_printf( ch, "\r\n&W[Permission &C%s&W]\r\n", perms_flag[level] );
               changecolor = true;
            }
            if( xIS_SET( command->flags, CMD_FLAG_NOSHOW )
            || xIS_SET( command->flags, CMD_FLAG_NPC )
            || xIS_SET( command->flags, CMD_FLAG_PC ) )
            {
               send_to_pager( "&R", ch );
               changecolor = true;
            }
            else if( changecolor )
            {
               send_to_pager( "&C", ch );
               changecolor = false;
            }
            pager_printf( ch, "%-16s", command->name );
            if( ++col == 5 )
            {
               send_to_pager( "\r\n", ch );
               col = 0;
            }
         }
      }
      if( col != 0 )
         send_to_pager( "\r\n", ch );
   }
   if( col != 0 )
      send_to_pager( "\r\n", ch );

   if( arg != NULL && arg[0] != '\0' )
   {
      if( is_number( arg ) )
      {
         if( !found )
            pager_printf( ch, "&WNo command found from level &C%d &Wto &C%d&W.&D\r\n", minlevel, maxlevel );
         else
            pager_printf( ch, "&C%d &Wcommands found from level &C%d &Wto &C%d&W.&D\r\n", cnt, minlevel, maxlevel );
      }
      else
      {
         if( !found )
            pager_printf( ch, "&WNo command found under &C%s&W.&D\r\n", arg );
         else
            pager_printf( ch, "&C%d &Wcommands found under &C%s&W.&D\r\n", cnt, arg );
      }
   }
   else
   {
      if( !found )
         send_to_pager( "&WNo commands found at all.&D\r\n", ch );
      else
         pager_printf( ch, "&C%d &Wcommands found in all.&D\r\n", cnt );
   }
}

CMDF( do_restrict )
{
   CMDTYPE *cmd;
   char arg[MIL], arg2[MIL], buf[MSL];
   short level, hash;
   bool found = false, canrestrict = false;

   if( !ch )
      return;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Restrict which command?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg2 );
   if( arg == NULL || arg2[0] == '\0' )
      level = get_trust( ch );
   else
      level = get_flag( arg2, perms_flag, PERM_MAX );

   if( level < 0 || level > get_trust( ch ) )
   {
      send_to_char( "You can't restrict it to that permission level.\r\n", ch );
      return;
   }
   level = URANGE( 0, level, get_trust( ch ) );

   hash = arg[0] % 126;
   for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
   {
      if( !str_prefix( arg, cmd->name ) )
      {
         found = true;
         if( cmd->perm <= get_trust( ch ) )
         {
            canrestrict = true;
            break;
         }
      }
   }

   if( !found )
      ch_printf( ch, "No command found for (%s).\r\n", arg );
   else if( !canrestrict )
      send_to_char( "You may not restrict that command.\r\n", ch );
   else
   {
      if( !str_prefix( arg2, "show" ) )
      {
         snprintf( buf, sizeof( buf ), "%s show", cmd->name );
         do_cedit( ch, buf );
         return;
      }
      cmd->perm = level;
      ch_printf( ch, "You restrict %s to %s\r\n", cmd->name, perms_flag[cmd->perm] );
      log_printf( "%s restricting %s to %s", ch->name, cmd->name, perms_flag[cmd->perm] );
   }
}
