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

int top_social;

/* Only does the insert when changing name or adding a new one */
void add_social( SOCIALTYPE *social, bool hinsert )
{
   SOCIALTYPE *tmp, *prev;
   int x;

   if( !social )
   {
      bug( "%s: NULL social", __FUNCTION__ );
      return;
   }

   if( !social->name )
   {
      bug( "%s: NULL social->name", __FUNCTION__ );
      return;
   }

   if( !social->char_no_arg )
   {
      bug( "%s: NULL social->char_no_arg", __FUNCTION__ );
      return;
   }

   for( x = 0; social->name[x] != '\0'; x++ )
      social->name[x] = LOWER( social->name[x] );

   top_social++;
   HASH_LINK( social, social_index, tmp, prev, hinsert );
}

SOCIALTYPE *find_social( const char *command, bool exact )
{
   SOCIALTYPE *social;
   int hash;

   hash = LOWER( command[0] ) % 126;

   for( social = social_index[hash]; social; social = social->next )
   {
      if( ( !exact && !str_prefix( command, social->name ) )
      || ( exact && !str_cmp( command, social->name ) ) )
         return social;
   }
   return NULL;
}

bool check_social( CHAR_DATA *ch, const char *command, char *argument )
{
   CHAR_DATA *victim = NULL;
   SOCIALTYPE *social;
   char arg[MIL];
   bool gsocial = false;
   bool pmobtrigger = MOBtrigger;

   if( !( social = find_social( command, false ) ) )
      return false;

   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_NO_EMOTE ) )
   {
      send_to_char( "You're anti-social!\r\n", ch );
      return true;
   }

   switch( ch->position )
   {
      case POS_DEAD:
         send_to_char( "Lie still; you're DEAD.\r\n", ch );
         return true;

      case POS_INCAP:
      case POS_MORTAL:
         send_to_char( "You're hurt far too bad for that.\r\n", ch );
         return true;

      case POS_STUNNED:
         send_to_char( "You're too stunned to do that.\r\n", ch );
         return true;

      case POS_SLEEPING:
         /*
          * I just know this is the path to a 12" 'if' statement.  :(
          * But two players asked for it already!  -- Furey
          */
         if( !str_cmp( social->name, "snore" ) )
            break;
         send_to_char( "In your dreams, or what?\r\n", ch );
         return true;

   }

   one_argument( argument, arg );
   victim = NULL;

   if( arg != NULL && arg[0] != '\0' )
   {
      if( !( victim = get_char_room( ch, arg ) ) )
      {
         if( ( victim = get_char_world( ch, arg ) ) )
            gsocial = true;
      }
   }

   if( gsocial )
      MOBtrigger = false;
   if( victim && ch && is_ignoring( victim, ch ) )
   {
      set_char_color( AT_IGNORE, ch );
      if( is_immortal( ch ) )
         ch_printf( ch, "%s is trying to ignore you.\r\n", victim->name );
      else
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
   }
   if( arg == NULL || arg[0] == '\0' )
   {
      act_printf( AT_SOCIAL, ch, NULL, victim, TO_ROOM, "%s%s", gsocial ? "[From afar] " : "", social->others_no_arg );
      act_printf( AT_SOCIAL, ch, NULL, victim, TO_CHAR, "%s%s", gsocial ? "[From afar] " : "", social->char_no_arg );
   }
   else if( !victim )
      send_to_char( "They aren't here.\r\n", ch );
   else if( victim == ch )
   {
      act_printf( AT_SOCIAL, ch, NULL, victim, gsocial ? TO_OTHERS : TO_ROOM, "%s%s", gsocial ? "[From afar] " : "", social->others_auto );
      act_printf( AT_SOCIAL, ch, NULL, victim, TO_CHAR, "%s%s", gsocial ? "[From afar] " : "", social->char_auto );
   }
   else
   {
      act_printf( AT_SOCIAL, ch, NULL, victim, gsocial ? TO_OTHERS : TO_NOTVICT, "%s%s", gsocial ? "[From afar] " : "", social->others_found );
      act_printf( AT_SOCIAL, ch, NULL, victim, TO_CHAR, "%s%s", gsocial ? "[From afar] " : "", social->char_found );
      act_printf( AT_SOCIAL, ch, NULL, victim, TO_VICT, "%s%s", gsocial ? "[From afar] " : "", social->vict_found );

      /*
       * Make mobs do random socials back
       * Also removed being drawn into a fight for socials
       */
      if( !is_npc( ch ) && is_npc( victim ) && !IS_AFFECTED( victim, AFF_CHARM ) && is_awake( victim )
      && !HAS_PROG( victim->pIndexData, ACT_PROG ) )
      {
         SOCIALTYPE *resocial, *resocial_next;
         int hash;
         bool found = false;

         /* Preform a random social */
         for( hash = 0; hash < 126; hash++ )
         {
            for( resocial = social_index[hash]; resocial; resocial = resocial_next )
            {
               resocial_next = resocial->next;
               /* Continue as long as percent is not equal to 50 */
               if( number_percent( ) != 50 )
                  continue;
               /* If it doesn't have some of the data continue */
               if( !resocial->others_found || !resocial->char_found || !resocial->vict_found )
                 continue;
               /* Send the social data */
               act_printf( AT_SOCIAL, victim, NULL, ch, gsocial ? TO_OTHERS : TO_NOTVICT, "%s%s", gsocial ? "[From afar] " : "", resocial->others_found );
               act_printf( AT_SOCIAL, victim, NULL, ch, TO_CHAR, "%s%s", gsocial ? "[From afar] " : "", resocial->char_found );
               act_printf( AT_SOCIAL, victim, NULL, ch, TO_VICT, "%s%s", gsocial ? "[From afar] " : "", resocial->vict_found );
               found = true;
               break;
            }
            /* If one was found and used break */
            if( found )
              break;
         }

         /* If found is false then do the same social back */
         if( !found )
         {
            if( social->others_found )
               act_printf( AT_SOCIAL, victim, NULL, ch, gsocial ? TO_OTHERS : TO_NOTVICT, "%s%s", gsocial ? "[From afar] " : "", social->others_found );
            if( social->char_found )
               act_printf( AT_SOCIAL, victim, NULL, ch, TO_CHAR, "%s%s", gsocial ? "[From afar] " : "", social->char_found );
            if( social->vict_found )
               act_printf( AT_SOCIAL, victim, NULL, ch, TO_VICT, "%s%s", gsocial ? "[From afar] " : "", social->vict_found );
         }
      }
   }

   if( gsocial )
      MOBtrigger = pmobtrigger;

   return true;
}

SOCIALTYPE *new_social( void )
{
   SOCIALTYPE *social = NULL;

   CREATE( social, SOCIALTYPE, 1 );
   if( !social )
   {
      bug( "%s: social is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   social->name = NULL;
   social->char_no_arg = NULL;
   social->char_found = NULL;
   social->char_auto = NULL;
   social->others_no_arg = NULL;
   social->others_found = NULL;
   social->others_auto = NULL;
   social->vict_found = NULL;
   return social;
}

void free_social( SOCIALTYPE *social )
{
   if( !social )
      return;
   STRFREE( social->name );
   STRFREE( social->char_no_arg );
   STRFREE( social->others_no_arg );
   STRFREE( social->char_found );
   STRFREE( social->others_found );
   STRFREE( social->vict_found );
   STRFREE( social->char_auto );
   STRFREE( social->others_auto );
   DISPOSE( social );
   top_social--;
}

void fread_social( FILE *fp )
{
   SOCIALTYPE *social;
   const char *word;
   bool fMatch;

   social = new_social( );

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
            KEY( "CharNoArg", social->char_no_arg, fread_string( fp ) );
            KEY( "CharFound", social->char_found, fread_string( fp ) );
            KEY( "CharAuto", social->char_auto, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !social->name )
               {
                  bug( "%s: Name not found", __FUNCTION__ );
                  free_social( social );
                  return;
               }
               if( !social->char_no_arg )
               {
                  bug( "%s: CharNoArg not found", __FUNCTION__ );
                  free_social( social );
                  return;
               }
               add_social( social, false );
               return;
            }
            break;

         case 'N':
            KEY( "Name", social->name, fread_string( fp ) );
            break;

         case 'O':
            KEY( "OthersNoArg", social->others_no_arg, fread_string( fp ) );
            KEY( "OthersFound", social->others_found, fread_string( fp ) );
            KEY( "OthersAuto", social->others_auto, fread_string( fp ) );
            break;

         case 'V':
            KEY( "VictFound", social->vict_found, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_social( social );
}

void load_socials( void )
{
   FILE *fp;

   top_social = 0;

   if( !( fp = fopen( SOCIAL_FILE, "r" ) ) )
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
      if( !str_cmp( word, "SOCIAL" ) )
      {
         fread_social( fp );
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

void save_socials( bool autosave )
{
   FILE *fp;
   SOCIALTYPE *social;
   int x;
   bool found = false;

   if( autosave && !sysdata.autosavesocials )
      return;

   if( !( fp = fopen( SOCIAL_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writting", __FUNCTION__, SOCIAL_FILE );
      perror( SOCIAL_FILE );
      return;
   }

   for( x = 0; x < 126; x++ )
   {
      for( social = social_index[x]; social; social = social->next )
      {
         if( !social->name || social->name[0] == '\0' )
         {
            bug( "%s: blank social in hash bucket %d", __FUNCTION__, x );
            continue;
         }
         if( !social->char_no_arg || social->char_no_arg[0] == '\0' )
         {
            bug( "%s: social with NULL char_no_arg in hash bucket %d", __FUNCTION__, x );
            continue;
         }
         found = true;
         fprintf( fp, "#SOCIAL\n" );
         fprintf( fp, "Name        %s~\n", social->name );
         if( social->char_no_arg )
            fprintf( fp, "CharNoArg   %s~\n", social->char_no_arg );
         if( social->others_no_arg )
            fprintf( fp, "OthersNoArg %s~\n", social->others_no_arg );
         if( social->char_found )
            fprintf( fp, "CharFound   %s~\n", social->char_found );
         if( social->others_found )
            fprintf( fp, "OthersFound %s~\n", social->others_found );
         if( social->vict_found )
            fprintf( fp, "VictFound   %s~\n", social->vict_found );
         if( social->char_auto )
            fprintf( fp, "CharAuto    %s~\n", social->char_auto );
         if( social->others_auto )
            fprintf( fp, "OthersAuto  %s~\n", social->others_auto );
         fprintf( fp, "End\n\n" );
      }
   }
   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
   if( !found )
      remove_file( SOCIAL_FILE );
}

void free_socials( void )
{
   SOCIALTYPE *social, *social_next;
   int hash;

   for( hash = 0; hash < 126; hash++ )
   {
      for( social = social_index[hash]; social; social = social_next )
      {
         social_next = social->next;
         free_social( social );
      }
   }
}

void unlink_social( SOCIALTYPE *social )
{
   SOCIALTYPE *tmp;

   if( !social )
   {
      bug( "%s: NULL social", __FUNCTION__ );
      return;
   }

   if( !social->name || social->name[0] == '\0' )
   {
      bug( "%s: social with invalid name", __FUNCTION__ );
      return;
   }

   HASH_UNLINK( social, social_index, tmp );
}

CMDF( do_sedit )
{
   SOCIALTYPE *social, *tmp, *tmp_next;
   char arg1[MIL], arg2[MIL];

   set_char_color( AT_SOCIAL, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      if( get_trust( ch ) >= PERM_HEAD )
         send_to_char( "Usage: sedit <save>\r\n", ch );
      send_to_char( "Usage: sedit <social> [field]\r\n", ch );
      send_to_char( "Fields:\r\n   ", ch );
      if( get_trust( ch ) >= PERM_HEAD )
         send_to_char( "name    delete  ", ch );
      send_to_char( "create  cnoarg  onoarg  cfound  ofound  vfound  cauto  oauto\r\n", ch );
      send_to_char( "   raise   lower   list\r\n", ch );
      return;
   }

   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( arg1, "save" ) )
   {
      save_socials( false );
      send_to_char( "Saved.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "create" ) )
   {
      if( ( social = find_social( arg1, true ) ) )
      {
         send_to_char( "That social already exists!\r\n", ch );
         return;
      }
      if( !( social = new_social( ) ) )
         return;
      social->name = STRALLOC( arg1 );
      snprintf( arg2, sizeof( arg2 ), "You %s.", arg1 );
      social->char_no_arg = STRALLOC( arg2 );
      add_social( social, true );
      save_socials( true );
      send_to_char( "Social added.\r\n", ch );
      return;
   }

   if( !( social = find_social( arg1, false ) ) )
   {
      send_to_char( "Social not found.\r\n", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
   {
      ch_printf( ch, "Social: %s\r\n", social->name ? social->name : "(Not Set)" );
      ch_printf( ch, "CNoArg: %s\r\n", social->char_no_arg ? social->char_no_arg : "(Not Set)" );
      ch_printf( ch, "ONoArg: %s\r\n", social->others_no_arg ? social->others_no_arg : "(Not Set)" );
      ch_printf( ch, "CFound: %s\r\n", social->char_found ? social->char_found : "(Not Set)" );
      ch_printf( ch, "OFound: %s\r\n", social->others_found ? social->others_found : "(Not Set)" );
      ch_printf( ch, "VFound: %s\r\n", social->vict_found ? social->vict_found : "(Not Set)" );
      ch_printf( ch, "CAuto : %s\r\n", social->char_auto ? social->char_auto : "(Not Set)" );
      ch_printf( ch, "OAuto : %s\r\n", social->others_auto ? social->others_auto : "(Not Set)" );
      return;
   }

   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( arg2, "delete" ) )
   {
      unlink_social( social );
      free_social( social );
      save_socials( true );
      send_to_char( "Deleted.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "cnoarg" ) )
   {
      if( !argument || argument[0] == '\0' || !str_cmp( argument, "clear" ) )
      {
         send_to_char( "You can't clear this field.  It must have a message.\r\n", ch );
         return;
      }
      STRSET( social->char_no_arg, argument );
      save_socials( true );
      send_to_char( "Cnoarg set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "onoarg" ) )
   {
      STRSET( social->others_no_arg, argument );
      save_socials( true );
      send_to_char( "Onoarg set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "cfound" ) )
   {
      STRSET( social->char_found, argument );
      save_socials( true );
      send_to_char( "Cfound set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "ofound" ) )
   {
      STRSET( social->others_found, argument );
      save_socials( true );
      send_to_char( "Ofound set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "vfound" ) )
   {
      STRSET( social->vict_found, argument );
      save_socials( true );
      send_to_char( "Vfound set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "cauto" ) )
   {
      STRSET( social->char_auto, argument );
      save_socials( true );
      send_to_char( "Cauto set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "oauto" ) )
   {
      STRSET( social->others_auto, argument );
      save_socials( true );
      send_to_char( "Oauto set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      int hash = social->name[0] % 126;

      if( ( tmp = social_index[hash] ) == social )
      {
         send_to_char( "That social is already at the top.\r\n", ch );
         return;
      }
      if( tmp->next == social )
      {
         social_index[hash] = social;
         tmp_next = tmp->next;
         tmp->next = social->next;
         social->next = tmp;
         save_socials( true );
         ch_printf( ch, "Moved %s above %s.\r\n", social->name, social->next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         tmp_next = tmp->next;
         if( tmp_next->next == social )
         {
            tmp->next = social;
            tmp_next->next = social->next;
            social->next = tmp_next;
            save_socials( true );
            ch_printf( ch, "Moved %s above %s.\r\n", social->name, tmp_next->name );
            return;
         }
      }
      send_to_char( "ERROR -- Not Found!\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "lower" ) )
   {
      int hash = social->name[0] % 126;

      if( !social->next )
      {
         send_to_char( "That social is already at the bottom.\r\n", ch );
         return;
      }
      tmp = social_index[hash];
      if( tmp == social )
      {
         tmp_next = tmp->next;
         social_index[hash] = social->next;
         social->next = tmp_next->next;
         tmp_next->next = social;
         save_socials( true );
         ch_printf( ch, "Moved %s below %s.\r\n", social->name, tmp_next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         if( tmp->next == social )
         {
            tmp_next = social->next;
            tmp->next = tmp_next;
            social->next = tmp_next->next;
            tmp_next->next = social;
            save_socials( true );
            ch_printf( ch, "Moved %s below %s.\r\n", social->name, tmp_next->name );
            return;
         }
      }
      send_to_char( "ERROR -- Not Found!\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "list" ) )
   {
      int hash = social->name[0] % 126;
      int col = 0;

      pager_printf( ch, "Priority placement for [%s]:\r\n", social->name );
      for( tmp = social_index[hash]; tmp; tmp = tmp->next )
      {
         pager_printf( ch, "  %s%-12s", tmp == social ? "&[green]" : "&[plain]", tmp->name );
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
   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( arg2, "name" ) )
   {
      SOCIALTYPE *checksocial;
      bool relocate;

      one_argument( argument, arg1 );
      if( arg1 == NULL || arg1[0] == '\0' )
      {
         send_to_char( "Can't clear name field!\r\n", ch );
         return;
      }
      if( ( checksocial = find_social( arg1, true ) ) )
      {
         ch_printf( ch, "There is already a social named %s.\r\n", arg1 );
         return;
      }
      if( arg1[0] != social->name[0] )
      {
         unlink_social( social );
         relocate = true;
      }
      else
         relocate = false;
      STRSET( social->name, arg1 );
      if( relocate )
         add_social( social, true );
      save_socials( true );
      send_to_char( "Done.\r\n", ch );
      return;
   }
   do_sedit( ch, (char *)"" );
}

CMDF( do_socials )
{
   SOCIALTYPE *social;
   char arg[MIL];
   int iHash, col = 0, cnt = 0;

   argument = one_argument( argument, arg );
   set_pager_color( AT_PLAIN, ch );
   for( iHash = 0; iHash < 126; iHash++ )
   {
      for( social = social_index[iHash]; social; social = social->next )
      {
         if( !social || !social->name )
            continue;
         if( arg != NULL && arg[0] != '\0' && str_prefix( arg, social->name ) )
            continue;
         cnt++;
         pager_printf( ch, "%-12s", social->name );
         if( ++col == 6 )
         {
            send_to_pager( "\r\n", ch );
            col = 0;
         }
      }
   }
   if( col != 0 )
      send_to_pager( "\r\n", ch );
   if( cnt == 0 )
   {
      if( arg != NULL && arg[0] != '\0' )
         pager_printf( ch, "&WNo social found under &C%s&W.&D\r\n", arg );
      else
         send_to_pager( "&WNo socials found at all.&D\r\n", ch );
   }
   else
   {
      if( arg != NULL && arg[0] != '\0' )
         pager_printf( ch, "&C%d &Wsocials found under &C%s&W.&D\r\n", cnt, arg );
      else
         pager_printf( ch, "&C%d &Wsocials found in all.&D\r\n", cnt );
   }
}
