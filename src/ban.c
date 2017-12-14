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
 *                            Ban module by Shaddai                          *
 *****************************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "h/mud.h"

BAN_DATA *first_ban, *last_ban;
BAN_DATA *first_ban_class, *last_ban_class;
BAN_DATA *first_ban_race, *last_ban_race;

void free_ban( BAN_DATA *pban )
{
   STRFREE( pban->name );
   STRFREE( pban->ban_time );
   STRFREE( pban->note );
   STRFREE( pban->ban_by );
   DISPOSE( pban );
}

/* Print the bans out to the screen.  Shaddai */
void show_bans( CHAR_DATA *ch, int type )
{
   BAN_DATA *pban;
   int bnum = 0;

   set_pager_color( AT_IMMORT, ch );
   switch( type )
   {
      case BAN_SITE:
         send_to_pager( "Banned sites:\r\n", ch );
         send_to_pager( "[ #] Warn (Lv) Time                     By              For   Site\r\n", ch );
         send_to_pager( "---- ---- ---- ------------------------ --------------- ----  ---------------\r\n", ch );
         set_pager_color( AT_PLAIN, ch );
         for( pban = first_ban; pban; pban = pban->next )
         {
            pager_printf( ch, "[%2d] %-4s (%2d) %-24s %-15s %4d  %c%s%c\r\n",
                ++bnum, ( pban->warn ) ? "YES" : "no", pban->level,
                pban->ban_time, pban->ban_by, pban->duration,
                ( pban->prefix ) ? '*' : ' ', pban->name, ( pban->suffix ) ? '*' : ' ' );
         }
         return;

      case BAN_RACE:
         send_to_pager( "Banned races:\r\n", ch );
         send_to_pager( "[ #] Warn (Lv) Time                     By              For   Race\r\n", ch );
         pban = first_ban_race;
         break;

      case BAN_CLASS:
         send_to_pager( "Banned classes:\r\n", ch );
         send_to_pager( "[ #] Warn (Lv) Time                     By              For   Class\r\n", ch );
         pban = first_ban_class;
         break;

      default:
         bug( "%s: Bad type %d", __FUNCTION__, type );
         return;
   }
   send_to_pager( "---- ---- ---- ------------------------ --------------- ----  ---------------\r\n", ch );
   set_pager_color( AT_PLAIN, ch );
   for( ; pban; pban = pban->next )
      pager_printf( ch, "[%2d] %-4s (%2d) %-24s %-15s %4d  %s\r\n", ++bnum,
         ( pban->warn ) ? "YES" : "no", pban->level, pban->ban_time, pban->ban_by, pban->duration, pban->name );
}

/* The workhose, checks for bans on sites/classes and races. Shaddai */
bool check_bans( CHAR_DATA *ch, int type )
{
   BAN_DATA *pban;
   char new_host[MSL];
   int i;
   bool fMatch = false;

   switch( type )
   {
      case BAN_RACE:
         pban = first_ban_race;
         break;

      case BAN_CLASS:
         pban = first_ban_class;
         break;

      case BAN_SITE:
         pban = first_ban;
         for( i = 0; i < ( int )( strlen( ch->desc->host ) ); i++ )
            new_host[i] = LOWER( ch->desc->host[i] );
         new_host[i] = '\0';
         break;

      default:
         bug( "%s Ban type %d.", __FUNCTION__, type );
         return false;
   }
   for( ; pban; pban = pban->next )
   {
      if( type == BAN_CLASS )
      {
         MCLASS_DATA *mclass;
         bool banned = false;

         for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
         {
            if( pban->flag == mclass->wclass )
            {
               banned = true;
               break;
            }
         }
         if( !banned )
            return false;
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_CLASS );
            save_banlist( );
            return false;
         }
         if( ch->level > pban->level )
         {
            if( pban->warn )
               log_printf_plus( LOG_WARN, sysdata.perm_log, "%s class logging in from %s.", pban->name, ch->desc->host );
            return false;
         }
         else
            return true;
      }
      if( type == BAN_RACE && pban->flag == ch->race )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_RACE );
            save_banlist( );
            return false;
         }
         if( ch->level > pban->level )
         {
            if( pban->warn )
               log_printf_plus( LOG_WARN, sysdata.perm_log, "%s race logging in from %s.", pban->name, ch->desc->host );
            return false;
         }
         else
            return true;
      }
      if( type == BAN_SITE )
      {
         if( pban->prefix && pban->suffix && strstr( new_host, pban->name ) )
            fMatch = true;
         else if( pban->prefix && !str_suffix( pban->name, new_host ) )
            fMatch = true;
         else if( pban->suffix && !str_prefix( pban->name, new_host ) )
            fMatch = true;
         else if( !str_cmp( pban->name, new_host ) )
            fMatch = true;
         if( fMatch )
         {
            if( check_expire( pban ) )
            {
               dispose_ban( pban, BAN_SITE );
               save_banlist( );
               return false;
            }
            if( ch->level > pban->level )
            {
               if( pban->warn )
                  log_printf_plus( LOG_WARN, sysdata.perm_log, "%s logging in from site %s.", ch->name, ch->desc->host );
               return false;
            }
            else
               return true;
         }
      }
   }
   return false;
}

/* Load up one class or one race ban structure. */
void fread_ban( FILE *fp, int type )
{
   BAN_DATA *pban;
   unsigned int i = 0;
   bool fMatch = false;

   CREATE( pban, BAN_DATA, 1 );
   pban->name = fread_string( fp );
   pban->level = fread_number( fp );
   pban->duration = fread_number( fp );
   pban->unban_date = fread_number( fp );
   if( type == BAN_SITE )
   {  /* Sites have 2 extra numbers written out */
      pban->prefix = fread_number( fp );
      pban->suffix = fread_number( fp );
   }
   pban->warn = fread_number( fp );
   pban->ban_by = fread_string( fp );
   pban->ban_time = fread_string( fp );
   pban->note = fread_string( fp );

   /* Need to lookup the class or race number if it is of that type */
   if( type == BAN_CLASS )
   {
      for( i = 0; i < MAX_CLASS; i++ )
      {
         if( !str_cmp( class_table[i]->name, pban->name ) )
         {
            fMatch = true;
            break;
         }
      }
   }
   else if( type == BAN_RACE )
   {
      for( i = 0; i < MAX_RACE; i++ )
      {
         if( !str_cmp( race_table[i]->name, pban->name ) )
         {
            fMatch = true;
            break;
         }
      }
   }
   else if( type == BAN_SITE )
   {
      for( i = 0; i < strlen( pban->name ); i++ )
      {
         if( pban->name[i] == '@' )
         {
            char *temp;
            char *temp2;

            temp = STRALLOC( pban->name );
            temp[i] = '\0';
            temp2 = &pban->name[i + 1];
            STRSET( pban->name, temp2 );
            STRFREE( temp );
            break;
         }
      }
   }

   if( type == BAN_RACE || type == BAN_CLASS )
   {
      if( fMatch )
         pban->flag = i;
      else  /* The file is corupted throw out this ban structure */
      {
         bug( "%s: Bad class or race structure %d.", __FUNCTION__, i );
         free_ban( pban );
         return;
      }
   }
   if( type == BAN_CLASS )
      LINK( pban, first_ban_class, last_ban_class, next, prev );
   else if( type == BAN_RACE )
      LINK( pban, first_ban_race, last_ban_race, next, prev );
   else if( type == BAN_SITE )
      LINK( pban, first_ban, last_ban, next, prev );
   else  /* Bad type throw out the ban structure */
   {
      bug( "%s: Bad type %d", __FUNCTION__, type );
      free_ban( pban );
   }
}

/* Load all those nasty bans up :) Shaddai */
void load_banlist( void )
{
   FILE *fp;
   const char *word;
   bool fMatch = false;

   if( !( fp = fopen( BAN_LIST, "r" ) ) )
      return;
   for( ;; )
   {
      word = feof( fp ) ? "END" : fread_word( fp );
      fMatch = false;
      switch( UPPER( word[0] ) )
      {
         case 'C':
            if( !str_cmp( word, "CLASS" ) )
            {
               fread_ban( fp, BAN_CLASS );
               fMatch = true;
            }
            break;

         case 'E':
            if( !str_cmp( word, "END" ) ) /*File should always contain END */
            {
               fclose( fp );
               fp = NULL;
               log_string( "Done." );
               return;
            }
            break;

         case 'R':
            if( !str_cmp( word, "RACE" ) )
            {
               fread_ban( fp, BAN_RACE );
               fMatch = true;
            }
            break;

         case 'S':
            if( !str_cmp( word, "SITE" ) )
            {
               fread_ban( fp, BAN_SITE );
               fMatch = true;
            }
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   fclose( fp );
   fp = NULL;
   bug( "%s: END wasn't found, but finished anyways.", __FUNCTION__ );
}

/* Saves all bans, for sites, classes and races. Shaddai */
void save_banlist( void )
{
   BAN_DATA *pban;
   FILE *fp;

   if( !first_ban && !first_ban_race && !first_ban_class )
   {
      remove_file( BAN_LIST );
      return;
   }

   if( !( fp = fopen( BAN_LIST, "w" ) ) )
   {
      bug( "%s: Can't open %s", __FUNCTION__, BAN_LIST );
      perror( BAN_LIST );
      return;
   }

   /* Print out all the site bans */
   for( pban = first_ban; pban; pban = pban->next )
   {
      fprintf( fp, "SITE\n" );
      fprintf( fp, "%s~\n", pban->name );
      fprintf( fp, "%d %d %d %d %d %d\n", pban->level, pban->duration,
         pban->unban_date, pban->prefix, pban->suffix, pban->warn );
      fprintf( fp, "%s~\n%s~\n%s~\n", pban->ban_by, pban->ban_time, pban->note );
   }

   /* Print out all the race bans */
   for( pban = first_ban_race; pban; pban = pban->next )
   {
      fprintf( fp, "RACE\n" );
      fprintf( fp, "%s~\n", pban->name );
      fprintf( fp, "%d %d %d %d\n", pban->level, pban->duration, pban->unban_date, pban->warn );
      fprintf( fp, "%s~\n%s~\n%s~\n", pban->ban_by, pban->ban_time, pban->note );
   }

   /* Print out all the class bans */
   for( pban = first_ban_class; pban; pban = pban->next )
   {
      fprintf( fp, "CLASS\n" );
      fprintf( fp, "%s~\n", pban->name );
      fprintf( fp, "%d %d %d %d\n", pban->level, pban->duration, pban->unban_date, pban->warn );
      fprintf( fp, "%s~\n%s~\n%s~\n", pban->ban_by, pban->ban_time, pban->note );
   }
   fprintf( fp, "END\n" ); /* File must have an END even if empty */
   fclose( fp );
   fp = NULL;
}

/* The main command for ban, lots of arguments so be carefull what you change here. Shaddai */
CMDF( do_ban )
{
   BAN_DATA *pban;
   char arg1[MIL], arg2[MIL], arg3[MIL], arg4[MIL], *temp;
   int value = 0, btime;

   if( is_npc( ch ) )   /* Don't want mobs banning sites ;) */
   {
      send_to_char( "Monsters are too dumb to do that!\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   set_char_color( AT_IMMORT, ch );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   argument = one_argument( argument, arg4 );

   /* Do we have a time duration for the ban? */
   if( arg4 != NULL && arg4[0] != '\0' && is_number( arg4 ) )
      btime = atoi( arg4 );
   else
      btime = -1;

   /*
    * -1 is default, but no reason the time should be greater than 1000
    * or less than 1, after all if it is greater than 1000 you are talking
    * around 3 years.
    */
   if( btime != -1 && ( btime < 1 || btime > 1000 ) )
   {
      send_to_char( "Time value is -1 (forever) or from 1 to 1000.\r\n", ch );
      return;
   }

   /* Need to be carefull with sub-states or everything will get messed up. */
   switch( ch->substate )
   {
      default:
         bug( "%s: illegal substate", __FUNCTION__ );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;

      case SUB_NONE:
         ch->tempnum = SUB_NONE;
         break;

         /* Returning to end the editing of the note */
      case SUB_BAN_DESC:
         add_ban( ch, (char *)"", (char *)"", 0, 0 );
         return;
   }
   if( arg1 == NULL || arg1[0] == '\0' )
      goto Usage_message;

   /* If no args are sent after the class/site/race, show the current banned items. Shaddai */
   if( !str_cmp( arg1, "site" ) )
   {
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         show_bans( ch, BAN_SITE );
         return;
      }

      /* Are they high enough to ban sites? */
      if( get_trust( ch ) < sysdata.ban_site )
      {
         ch_printf( ch, "You must be %d level to add bans.\r\n", sysdata.ban_site );
         return;
      }
      if( arg3 == NULL || arg3[0] == '\0' )
         goto Usage_message;
      if( !add_ban( ch, arg2, arg3, btime, BAN_SITE ) )
         return;
   }
   else if( !str_cmp( arg1, "race" ) )
   {
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         show_bans( ch, BAN_RACE );
         return;
      }

      /* Are they high enough level to ban races? */
      if( get_trust( ch ) < sysdata.ban_race )
      {
         ch_printf( ch, "You must be %d level to add bans.\r\n", sysdata.ban_race );
         return;
      }
      if( arg3 == NULL || arg3[0] == '\0' )
         goto Usage_message;
      if( !add_ban( ch, arg2, arg3, btime, BAN_RACE ) )
         return;
   }
   else if( !str_cmp( arg1, "class" ) )
   {
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         show_bans( ch, BAN_CLASS );
         return;
      }

      /* Are they high enough to ban classes? */
      if( get_trust( ch ) < sysdata.ban_class )
      {
         ch_printf( ch, "You must be %d level to add bans.\r\n", sysdata.ban_class );
         return;
      }
      if( arg3 == NULL || arg3[0] == '\0' )
         goto Usage_message;
      if( !add_ban( ch, arg2, arg3, btime, BAN_CLASS ) )
         return;
   }
   else if( !str_cmp( arg1, "show" ) )
   {
      /* This will show the note attached to a ban */
      if( arg2 == NULL || arg3 == NULL || arg2[0] == '\0' || arg3[0] == '\0' )
         goto Usage_message;
      temp = arg3;
      if( arg3[0] == '#' ) /* Use #1 to show the first ban */
      {
         temp = arg3;
         temp++;
         if( !is_number( temp ) )
         {
            send_to_char( "Which ban # to show?\r\n", ch );
            return;
         }
         if( ( value = atoi( temp ) ) < 1 )
         {
            send_to_char( "You must specify a number greater than 0.\r\n", ch );
            return;
         }
      }
      else if( is_number( arg3 ) )
      {
         if( ( value = atoi( arg3 ) ) < 1 )
         {
            send_to_char( "The number has to be above 0.\r\n", ch );
            return;
         }
      }

      if( !str_cmp( arg2, "site" ) )
      {
         pban = first_ban;
         if( temp[0] == '*' )
            temp++;
         if( temp[strlen( temp ) - 1] == '*' )
            temp[strlen( temp ) - 1] = '\0';
      }
      else if( !str_cmp( arg2, "class" ) )
         pban = first_ban_class;
      else if( !str_cmp( arg2, "race" ) )
         pban = first_ban_race;
      else
         goto Usage_message;

      for( ; pban; pban = pban->next )
      {
         if( value == 1 || !str_cmp( pban->name, temp ) )
            break;
         else if( value > 1 )
            value--;
      }

      if( !pban )
      {
         send_to_char( "No such ban.\r\n", ch );
         return;
      }
      ch_printf( ch, "Banned by: %s\r\n", pban->ban_by );
      send_to_char( pban->note, ch );
      return;
   }
   else
      goto Usage_message;
   return;

   /*
    * Catch all Usage message, make sure that return stays above this or you
    * will get the Usage message everytime you issue the command even if it
    * is a valid one.  Shaddai
    */
   Usage_message:
      send_to_char( "Usage: ban site/race/class [<address/race/class> <type> <duration>]\r\n", ch );
      send_to_char( "Usage: ban show site/race/class <address/race/class/#>\r\n", ch );
      send_to_char( "Duration is the length of the ban in days or -1 (Forever).\r\n", ch );
      send_to_char( "Type can be: newbie, mortal, all, warn or level.\r\n", ch );
      return;
}

/* Allow a already banned site/class or race.  Shaddai */
CMDF( do_allow )
{
   BAN_DATA *pban;
   char arg1[MIL], arg2[MIL], *temp = NULL;
   int value = 0;
   bool fMatch = false;

   if( is_npc( ch ) ) /* No mobs allowing sites */
   {
      send_to_char( "Monsters are too dumb to do that!\r\n", ch );
      return;
   }

   if( !ch->desc ) /* No desc is a bad thing */
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   set_char_color( AT_IMMORT, ch );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
      goto Usage_message;

   if( arg2[0] == '#' ) /* Use #1 to allow the first ban in the list specified */
   {
      temp = arg2;
      temp++;
      if( !is_number( temp ) )
      {
         send_to_char( "Which ban # to allow?\r\n", ch );
         return;
      }
      value = atoi( temp );
   }
   else if( is_number( arg2 ) )
   {
      if( ( value = atoi( arg2 ) ) < 1 )
      {
         send_to_char( "The number has to be above 0.\r\n", ch );
         return;
      }
   }

   if( !str_cmp( arg1, "site" ) )
   {
      if( !value )
      {
         if( strlen( arg2 ) < 2 )
         {
            send_to_char( "You have to have at least 2 chars for a ban\r\n", ch );
            send_to_char( "If you are trying to allow by number use #\r\n", ch );
            return;
         }

         temp = arg2;
         if( arg2[0] == '*' )
            temp++;
         if( temp[strlen( temp ) - 1] == '*' )
            temp[strlen( temp ) - 1] = '\0';
      }

      for( pban = first_ban; pban; pban = pban->next )
      {
         /* Need to make sure we dispose properly of the ban_data Or memory problems will be created. Shaddai */
         if( value == 1 || !str_cmp( pban->name, temp ) )
         {
            fMatch = true;
            dispose_ban( pban, BAN_SITE );
            break;
         }
         if( value > 1 )
            value--;
      }
   }
   else if( !str_cmp( arg1, "race" ) )
   {
      arg2[0] = toupper( arg2[0] );
      for( pban = first_ban_race; pban; pban = pban->next )
      {
         /* Need to make sure we dispose properly of the ban_data Or memory problems will be created. Shaddai */
         if( value == 1 || !str_cmp( pban->name, arg2 ) )
         {
            fMatch = true;
            dispose_ban( pban, BAN_RACE );
            break;
         }
         if( value > 1 )
            value--;
      }
   }
   else if( !str_cmp( arg1, "class" ) )
   {

      arg2[0] = toupper( arg2[0] );
      for( pban = first_ban_class; pban; pban = pban->next )
      {
         /* Need to make sure we dispose properly of the ban_data Or memory problems will be created. Shaddai */
         if( value == 1 || !str_cmp( pban->name, arg2 ) )
         {
            fMatch = true;
            dispose_ban( pban, BAN_CLASS );
            break;
         }
         if( value > 1 )
            value--;
      }
   }
   else
      goto Usage_message;

   if( fMatch )
   {
      save_banlist( );
      ch_printf( ch, "%s is now allowed.\r\n", arg2 );
   }
   else
      ch_printf( ch, "%s was not banned.\r\n", arg2 );
   return;

   /* Make sure that return above stays in! */
   Usage_message:
      send_to_char( "Usage: allow site/race/class <address/race/class>\r\n", ch );
      return;
}

/* Sets the warn flag on bans. */
CMDF( do_warn )
{
   BAN_DATA *pban, *start, *end;
   char arg1[MSL], arg2[MSL], *name;
   int count = -1, type;

   /* Don't want mobs or link-deads doing this. */
   if( is_npc( ch ) )
   {
      send_to_char( "Monsters are too dumb to do that!\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
      goto Usage_message;

   if( arg2[0] == '#' )
   {
      name = arg2;
      name++;
      if( !is_number( name ) )
         goto Usage_message;
      if( ( count = atoi( name ) ) < 1 )
      {
         send_to_char( "The number has to be above 0.\r\n", ch );
         return;
      }
   }
   else if( is_number( arg2 ) )
   {
      if( ( count = atoi( arg2 ) ) < 1 )
      {
         send_to_char( "The number has to be above 0.\r\n", ch );
         return;
      }
   }

   /* We simply set up which ban list we will be looking at here. */
   if( !str_cmp( arg1, "class" ) )
      type = BAN_CLASS;
   else if( !str_cmp( arg1, "race" ) )
      type = BAN_RACE;
   else if( !str_cmp( arg1, "site" ) )
      type = BAN_SITE;
   else
      type = -1;

   if( type == BAN_CLASS )
   {
      pban = first_ban_class;
      start = first_ban_class;
      end = last_ban_class;
      arg2[0] = toupper( arg2[0] );
   }
   else if( type == BAN_RACE )
   {
      pban = first_ban_race;
      start = first_ban_race;
      end = last_ban_race;
      arg2[0] = toupper( arg2[0] );
   }
   else if( type == BAN_SITE )
   {
      pban = first_ban;
      start = first_ban;
      end = last_ban;
   }
   else
      goto Usage_message;
   for( ; pban && count != 0; count--, pban = pban->next )
      if( count == -1 && !str_cmp( pban->name, arg2 ) )
         break;
   if( pban )
   {
      /* If it is just a warn delete it, otherwise remove the warn flag. */
      if( pban->warn )
      {
         if( pban->level == BAN_WARN )
         {
            dispose_ban( pban, type );
            send_to_char( "Warn has been deleted.\r\n", ch );
         }
         else
         {
            pban->warn = false;
            send_to_char( "Warn turned off.\r\n", ch );
         }
      }
      else
      {
         pban->warn = true;
         send_to_char( "Warn turned on.\r\n", ch );
      }
      save_banlist( );
   }
   else
   {
      ch_printf( ch, "%s was not found in the ban list.\r\n", arg2 );
      return;
   }
   return;

   /* The above return has to stay in! */
   Usage_message:
      send_to_char( "Usage: warn site/race/class <address/race/class/#>\r\n", ch );
      return;
}

/* This actually puts the new ban into the proper linked list and initializes its data.  Shaddai */
int add_ban( CHAR_DATA *ch, char *arg1, char *arg2, int btime, int type )
{
   BAN_DATA *pban, *temp;
   char arg[MSL], *name;
   int level, i, value;

   /*
    * Should we check to see if they have dropped link sometime in between 
    * writing the note and now?  Not sure but for right now we won't since
    * do_ban checks for that.  Shaddai
    */
   switch( ch->substate )
   {
      default:
         bug( "%s: illegal substate", __FUNCTION__ );
         return 0;

      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return 0;

      case SUB_NONE:
      {
         one_argument( arg1, arg );

         if( arg[0] == '\0' || arg2[0] == '\0' )
            return 0;

         if( is_number( arg2 ) )
         {
            level = atoi( arg2 );
            if( level < 0 || level > MAX_LEVEL )
            {
               ch_printf( ch, "Level range is from 0 to %d.\r\n", MAX_LEVEL );
               return 0;
            }
         }
         else if( !str_cmp( arg2, "all" ) )
            level = MAX_LEVEL;
         else if( !str_cmp( arg2, "newbie" ) )
            level = 1;
         else if( !str_cmp( arg2, "mortal" ) )
            level = MAX_LEVEL;
         else if( !str_cmp( arg2, "warn" ) )
            level = BAN_WARN;
         else
         {
            bug( "%s: Bad string [%s] for flag.", __FUNCTION__, arg2 );
            return 0;
         }

         switch( type )
         {
            case BAN_CLASS:
               if( arg[0] == '\0' )
                  return 0;
               if( is_number( arg ) )
                  value = atoi( arg );
               else
               {
                  for( i = 0; i < MAX_CLASS; i++ )
                     if( !str_cmp( class_table[i]->name, arg ) )
                        break;
                  value = i;
               }
               if( value < 0 || value >= MAX_CLASS )
               {
                  send_to_char( "Unknown class.\r\n", ch );
                  return 0;
               }
               for( temp = first_ban_class; temp; temp = temp->next )
               {
                  if( temp->flag == value )
                  {
                     if( temp->level == level )
                     {
                        send_to_char( "That entry already exists.\r\n", ch );
                        return 0;
                     }
                     else
                     {
                        temp->level = level;
                        if( temp->level == BAN_WARN )
                           temp->warn = true;
                        temp->ban_time = STRALLOC( distime( current_time ) );
                        if( btime > 0 )
                        {
                           temp->duration = btime;
                           temp->unban_date = ( current_time + ( 86400 * btime ) );
                        }
                        else
                        {
                           temp->duration = -1;
                           temp->unban_date = -1;
                        }
                        STRSET( temp->ban_by, ch->name );
                        send_to_char( "Updated entry.\r\n", ch );
                        return 1;
                     }
                  }
               }
               CREATE( pban, BAN_DATA, 1 );
               pban->name = STRALLOC( class_table[value]->name );
               pban->flag = value;
               pban->level = level;
               pban->ban_by = STRALLOC( ch->name );
               LINK( pban, first_ban_class, last_ban_class, next, prev );
               break;

            case BAN_RACE:
               if( is_number( arg ) )
                  value = atoi( arg );
               else
               {
                  for( i = 0; i < MAX_RACE; i++ )
                     if( !str_cmp( race_table[i]->name, arg ) )
                        break;
                  value = i;
               }
               if( value < 0 || value >= MAX_RACE )
               {
                  send_to_char( "Unknown race.\r\n", ch );
                  return 0;
               }
               for( temp = first_ban_race; temp; temp = temp->next )
               {
                  if( temp->flag == value )
                  {
                     if( temp->level == level )
                     {
                        send_to_char( "That entry already exists.\r\n", ch );
                        return 0;
                     }
                     else
                     {
                        temp->level = level;
                        if( temp->level == BAN_WARN )
                           temp->warn = true;
                        temp->ban_time = STRALLOC( distime( current_time ) );
                        if( btime > 0 )
                        {
                           temp->duration = btime;
                           temp->unban_date = ( current_time + ( 86400 * btime ) );
                        }
                        else
                        {
                           temp->duration = -1;
                           temp->unban_date = -1;
                        }
                        STRSET( temp->ban_by, ch->name );
                        send_to_char( "Updated entry.\r\n", ch );
                        return 1;
                     }
                  }
               }
               CREATE( pban, BAN_DATA, 1 );
               pban->name = STRALLOC( race_table[value]->name );
               pban->flag = value;
               pban->level = level;
               pban->ban_by = STRALLOC( ch->name );
               LINK( pban, first_ban_race, last_ban_race, next, prev );
               break;

            case BAN_SITE:
            {
               bool prefix = false, suffix = false, user_name = false;
               char *temp_host = NULL, *temp_user = NULL;
               size_t x;

               for( x = 0; x < strlen( arg ); x++ )
               {
                  if( arg[x] == '@' )
                  {
                     user_name = true;
                     temp_host = STRALLOC( &arg[x + 1] );
                     arg[x] = '\0';
                     temp_user = STRALLOC( arg );
                     break;
                  }
               }
               if( !user_name )
                  name = arg;
               else
                  name = temp_host;

               if( !name ) /* Double check to make sure name isnt null */
               {
                  /* Free this stuff if its there */
                  if( user_name )
                  {
                     STRFREE( temp_host );
                     STRFREE( temp_user );
                  }
                  send_to_char( "Name was null.\r\n", ch );
                  return 0;
               }

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
               for( temp = first_ban; temp; temp = temp->next )
               {
                  if( !str_cmp( temp->name, name ) )
                  {
                     if( temp->level == level && ( prefix && temp->prefix ) && ( suffix && temp->suffix ) )
                     {
                        /* Free this stuff if its there */
                        if( user_name )
                        {
                           STRFREE( temp_host );
                           STRFREE( temp_user );
                        }
                        send_to_char( "That entry already exists.\r\n", ch );
                        return 0;
                     }
                     else
                     {
                        temp->suffix = suffix;
                        temp->prefix = prefix;
                        if( temp->level == BAN_WARN )
                           temp->warn = true;
                        temp->level = level;
                        temp->ban_time = STRALLOC( distime( current_time ) );
                        if( btime > 0 )
                        {
                           temp->duration = btime;
                           temp->unban_date = ( current_time + ( 86400 * btime ) );
                        }
                        else
                        {
                           temp->duration = -1;
                           temp->unban_date = -1;
                        }
                        if( user_name )
                        {
                           STRFREE( temp_host );
                           STRFREE( temp_user );
                        }
                        STRSET( temp->ban_by, ch->name );
                        send_to_char( "Updated entry.\r\n", ch );
                        return 1;
                     }
                  }
               }
               CREATE( pban, BAN_DATA, 1 );
               pban->ban_by = STRALLOC( ch->name );
               pban->suffix = suffix;
               pban->prefix = prefix;
               pban->name = STRALLOC( name );
               pban->level = level;
               LINK( pban, first_ban, last_ban, next, prev );
               if( user_name )
               {
                  STRFREE( temp_host );
                  STRFREE( temp_user );
               }
               break;
            }
            default:
               bug( "%s: Bad type %d.", __FUNCTION__, type );
               return 0;
         }
         pban->ban_time = STRALLOC( distime( current_time ) );
         if( btime > 0 )
         {
            pban->duration = btime;
            pban->unban_date = ( current_time + ( 86400 * btime ) );
         }
         else
         {
            pban->duration = -1;
            pban->unban_date = -1;
         }
         if( pban->level == BAN_WARN )
            pban->warn = true;
         ch->substate = SUB_BAN_DESC;
         ch->dest_buf = pban;
         start_editing( ch, pban->note );
         return 1;
      }
      case SUB_BAN_DESC:
         pban = ( BAN_DATA * ) ch->dest_buf;
         if( !pban )
         {
            bug( "%s: sub_ban_desc: NULL ch->dest_buf", __FUNCTION__ );
            ch->substate = SUB_NONE;
            return 0;
         }
         STRFREE( pban->note );
         pban->note = copy_buffer( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         save_banlist( );
         if( pban->duration > 0 )
            ch_printf( ch, "%s banned for %d days.\r\n", pban->name, pban->duration );
         else
            ch_printf( ch, "%s banned forever.\r\n", pban->name );
         return 1;
   }
}

/* Check for totally banned sites.  Need this because we don't have a char struct yet.  Shaddai */
bool check_total_bans( DESCRIPTOR_DATA *d )
{
   BAN_DATA *pban;
   char new_host[MSL];
   int i;

   for( i = 0; i < ( int )strlen( d->host ); i++ )
      new_host[i] = LOWER( d->host[i] );
   new_host[i] = '\0';

   for( pban = first_ban; pban; pban = pban->next )
   {
      if( pban->level != MAX_LEVEL )
         continue;
      if( pban->prefix && pban->suffix && strstr( new_host, pban->name ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_SITE );
            save_banlist( );
            return false;
         }
         else
            return true;
      }

      /* Bug of switched checks noticed by Cronel */
      if( pban->suffix && !str_prefix( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_SITE );
            save_banlist( );
            return false;
         }
         else
            return true;
      }
      if( pban->prefix && !str_suffix( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_SITE );
            save_banlist( );
            return false;
         }
         else
            return true;
      }
      if( !str_cmp( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_SITE );
            save_banlist( );
            return false;
         }
         else
            return true;
      }
   }
   return false;
}

bool check_expire( BAN_DATA *pban )
{
   if( pban->unban_date < 0 )
      return false;
   if( pban->unban_date <= current_time )
   {
      log_printf_plus( LOG_WARN, sysdata.perm_log, "%s ban has expired.", pban->name );
      return true;
   }
   return false;
}

void dispose_ban( BAN_DATA *pban, int type )
{
   if( !pban )
      return;

   if( type != BAN_SITE && type != BAN_CLASS && type != BAN_RACE )
   {
      bug( "%s: Unknown Ban Type %d.", __FUNCTION__, type );
      return;
   }

   switch( type )
   {
      case BAN_SITE:
         UNLINK( pban, first_ban, last_ban, next, prev );
         break;
      case BAN_CLASS:
         UNLINK( pban, first_ban_class, last_ban_class, next, prev );
         break;
      case BAN_RACE:
         UNLINK( pban, first_ban_race, last_ban_race, next, prev );
         break;
   }
   free_ban( pban );
}

void free_bans( void )
{
   BAN_DATA *ban, *ban_next;

   for( ban = first_ban; ban; ban = ban_next )
   {
      ban_next = ban->next;
      dispose_ban( ban, BAN_SITE );
   }
   for( ban = first_ban_race; ban; ban = ban_next )
   {
      ban_next = ban->next;
      dispose_ban( ban, BAN_RACE );
   }
   for( ban = first_ban_class; ban; ban = ban_next )
   {
      ban_next = ban->next;
      dispose_ban( ban, BAN_CLASS );
   }
}

bool check_race_bans( int iRace )
{
   BAN_DATA *pban;

   for( pban = first_ban_race; pban; pban = pban->next )
   {
      if( pban->flag == iRace )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_RACE );
            save_banlist( );
            return false;
         }
         return true;
      }
   }
   return false;
}

bool check_class_bans( int iClass )
{
   BAN_DATA *pban;

   for( pban = first_ban_class; pban; pban = pban->next )
   {
      if( pban->flag == iClass )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban, BAN_CLASS );
            save_banlist( );
            return false;
         }
         return true;
      }
   }
   return false;
}
