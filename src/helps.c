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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "h/mud.h"

int top_help;
char *help_greeting;
void save_helps( bool autosave );

typedef struct help_data HELP_DATA;
struct help_data
{
   HELP_DATA *next, *prev;
   char *keyword;
   char *text;
   time_t updated;
   char *updated_by;
   short perm;
   bool nosave;
};

HELP_DATA *first_help, *last_help;

void free_help( HELP_DATA *pHelp )
{
   STRFREE( pHelp->text );
   STRFREE( pHelp->keyword );
   STRFREE( pHelp->updated_by );
   DISPOSE( pHelp );
   top_help--;
}

void free_helps( void )
{
   HELP_DATA *pHelp, *pHelp_next;

   for( pHelp = first_help; pHelp; pHelp = pHelp_next )
   {
      pHelp_next = pHelp->next;
      UNLINK( pHelp, first_help, last_help, next, prev );
      free_help( pHelp );
   }
}

/*
 * Adds a help page to the list if it is not a duplicate of an existing page.
 * Page is insert-sorted by keyword. -Thoric
 * (The reason for sorting is to keep do_hlist looking nice)
 */
void add_help( HELP_DATA *pHelp )
{
   HELP_DATA *tHelp;
   int match;

   for( tHelp = first_help; tHelp; tHelp = tHelp->next )
   {
      if( pHelp->perm == tHelp->perm && strcmp( pHelp->keyword, tHelp->keyword ) == 0 )
      {
         bug( "%s: duplicate: %s. Deleting.", __FUNCTION__, pHelp->keyword );
         free_help( pHelp );
         return;
      }
      else if( pHelp->keyword
      && ( ( match = strcmp( pHelp->keyword[0] == '\'' ? pHelp->keyword + 1 : pHelp->keyword,
      tHelp->keyword[0] == '\'' ? tHelp->keyword + 1 : tHelp->keyword ) ) < 0
      || ( match == 0 && pHelp->perm > tHelp->perm ) ) )
      {
         INSERT( pHelp, tHelp, first_help, next, prev );
         break;
      }
   }
   if( !tHelp )
      LINK( pHelp, first_help, last_help, next, prev );

   top_help++;
}

void add_skill_help( SKILLTYPE *skill, bool update )
{
   HELP_DATA *pHelp;
   char buf[MSL];

   if( !skill || !skill->name || !skill->htext )
      return;
   snprintf( buf, sizeof( buf ), "'%s'", strupper( skill->name ) );
   pHelp = get_help( NULL, buf );
   if( pHelp && !update )
      return;
   if( !pHelp )
   {
      CREATE( pHelp, HELP_DATA, 1 );
      STRSET( pHelp->keyword, buf );
      STRSET( pHelp->text, skill->htext );
      STRSET( pHelp->updated_by, "Skill" );
      pHelp->updated = current_time;
      pHelp->nosave = true;
      pHelp->perm = PERM_ALL;
      add_help( pHelp );
   }
   else
   {
      STRSET( pHelp->keyword, buf );
      STRSET( pHelp->text, skill->htext );
      STRSET( pHelp->updated_by, "Skill" );
      pHelp->updated = current_time;
      pHelp->nosave = true;
      pHelp->perm = PERM_ALL;
   }
}

void add_command_help( CMDTYPE *cmd, bool update )
{
   HELP_DATA *pHelp;
   char buf[MSL];

   if( !cmd || !cmd->name || !cmd->htext )
      return;
   snprintf( buf, sizeof( buf ), "'%s'", strupper( cmd->name ) );
   pHelp = get_help( NULL, buf );
   if( pHelp && !update )
      return;
   if( !pHelp ) /* New help file */
   {
      CREATE( pHelp, HELP_DATA, 1 );
      STRSET( pHelp->keyword, buf );
      STRSET( pHelp->text, cmd->htext );
      STRSET( pHelp->updated_by, "Command" );
      pHelp->updated = current_time;
      pHelp->nosave = true;
      pHelp->perm = cmd->perm;
      add_help( pHelp );
   }
   else /* Modify existing help file */
   {
      STRSET( pHelp->keyword, buf );
      STRSET( pHelp->text, cmd->htext );
      pHelp->updated = current_time;
      pHelp->nosave = true;
      pHelp->perm = cmd->perm;
   }
}

HELP_DATA *new_help( void )
{
   HELP_DATA *phelp = NULL;

   CREATE( phelp, HELP_DATA, 1 );
   if( !phelp )
   {
      bug( "%s: phelp is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   phelp->keyword = NULL;
   phelp->text = NULL;
   phelp->updated_by = NULL;
   phelp->nosave = false;
   return phelp;
}

void fread_help( FILE *fp )
{
   const char *word;
   bool fMatch;
   HELP_DATA *pHelp = NULL;

   pHelp = new_help( );

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
               if( !pHelp->keyword || pHelp->keyword[0] == '\0' )
               {
                  free_help( pHelp );
                  return;
               }
               if( pHelp->text && pHelp->text[0] == '.' && pHelp->text[1] == ' ' )
               {
                  char *tmptext = un_fix_help( pHelp->text );
                  STRSET( pHelp->text, tmptext );
               }
               if( !str_cmp( pHelp->keyword, "greeting" ) )
                  help_greeting = pHelp->text;
               add_help( pHelp );
               return;
	    }
	    break;

         case 'K':
            KEY( "Keyword", pHelp->keyword, fread_string( fp ) );
            break;

         case 'P':
            KEY( "Perm", pHelp->perm, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Text", pHelp->text, fread_string( fp ) );
            break;

         case 'U':
            KEY( "Updated", pHelp->updated, fread_time( fp ) );
            KEY( "UpdatedBy", pHelp->updated_by, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   if( pHelp )
      free_help( pHelp );
}

void load_helps( void )
{
   FILE *fp;

   top_help = 0;

   if( !( fp = fopen( HELP_FILE, "r" ) ) )
   {
      bug( "%s: Can't open %s", __FUNCTION__, HELP_FILE );
      perror( HELP_FILE );
      return;
   }

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
      if( !str_cmp( word, "HELP" ) )
      {
         fread_help( fp );
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

HELP_DATA *get_help( CHAR_DATA *ch, char *argument )
{
   char argall[MIL], argone[MIL], argnew[MIL];
   HELP_DATA *pHelp;
   int lev;

   if( !argument || argument[0] == '\0' )
      argument = (char *)"summary";

   if( isdigit( argument[0] ) && !is_number( argument ) )
   {
      lev = number_argument( argument, argnew );
      argument = argnew;
   }
   else
      lev = -2;

   /* Tricky argument handling so 'help a b' doesn't match a. */
   argall[0] = '\0';
   while( argument && argument[0] != '\0' )
   {
      argument = one_argument( argument, argone );
      if( argall[0] != '\0' )
         mudstrlcat( argall, " ", sizeof( argall ) );
      mudstrlcat( argall, argone, sizeof( argall ) );
   }

   for( pHelp = first_help; pHelp; pHelp = pHelp->next )
   {
      if( ch && pHelp->perm > get_trust( ch ) )
         continue;
      if( lev != -2 && pHelp->perm != lev )
         continue;
      if( is_name( argall, pHelp->keyword ) )
         return pHelp;
   }
   return NULL;
}

/* LAWS command */
CMDF( do_laws )
{
   char buf[1024];

   if( !argument || argument[0] == '\0' )
      do_help( ch, (char *)"laws" );
   else
   {
      snprintf( buf, sizeof( buf ), "law %s", argument );
      do_help( ch, buf );
   }
}

void show_help( CHAR_DATA *ch, HELP_DATA *pHelp )
{
   const char *h1, *h2;

   if( !ch || !pHelp )
      return;

   h1 = color_str( AT_HELP, ch );
   h2 = color_str( AT_HELP2, ch );

   set_pager_color( AT_HELP, ch );
   if( is_immortal( ch ) )
   {
      pager_printf( ch, "%sHelp permission: %s%s%s\r\n", h1, h2, perms_flag[pHelp->perm], h1 );
      pager_printf( ch, "%sHelp Keyword:    %s%s&D%s\r\n", h1, h2, pHelp->keyword, h1 );
   }

   if( pHelp->updated )
      pager_printf( ch, "%sLast updated on %s%s&D %sby %s%s\r\n", h1, h2, distime( pHelp->updated ),
         h1, h2, pHelp->updated_by ? pHelp->updated_by : "(Unknown)" );

   if( !pHelp->text )
      return;
   pager_printf( ch, "%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n%s", h1, h2 );
   send_to_pager( pHelp->text, ch );
   pager_printf( ch, "%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", h1 );
}

CMDF( do_help )
{
   HELP_DATA *pHelp, *lHelp = NULL;
   char *keyword, arg[MIL], oneword[MSL], lastmatch[MSL];
   int value = -1;
   short matched = 0, checked = 0, totalmatched = 0, found = 0;
   bool uselevel = false;

   set_pager_color( AT_NOTE, ch );

   if( !argument || argument[0] == '\0' )
      argument = (char *)"summary";

   if( ( pHelp = get_help( ch, argument ) ) )
   {
      show_help( ch, pHelp );
      return;
   }

   pager_printf( ch, "No help on \'%s\' found.\r\n", argument );

   /* Get an arg incase they do a number seperate */
   one_argument( argument, arg );

   /* See if arg is a number if so update argument */
   if( is_number( arg ) )
   {
     argument = one_argument( argument, arg );
     if( argument && argument[0] != '\0' )
     {
         value = URANGE( 0, atoi( arg ), PERM_IMP );
         uselevel = true;
     }
     else /* If no more argument put arg as argument */
        argument = arg;
   }

   if( value >= 0 )
      pager_printf( ch, "Checking for suggested helps that are permission %s.\r\n", perms_flag[value] );

   send_to_pager( "Suggested Help Files:\r\n", ch );
   strncpy( lastmatch, " ", sizeof( lastmatch ) );

   /* Check helps first */
   for( pHelp = first_help; pHelp; pHelp = pHelp->next )
   {
      matched = 0;
      if( !pHelp || !pHelp->keyword || pHelp->keyword[0] == '\0' || pHelp->perm > get_trust( ch ) )
         continue;

      /* Check arg if its avaliable */
      if( uselevel && pHelp->perm != value )
         continue;

      keyword = pHelp->keyword;
      while( keyword && keyword[0] != '\0' )
      {
         matched = 0;   /* Set to 0 for each time we check lol */
         keyword = one_argument( keyword, oneword );

         /* Lets check only up to 10 spots */
         for( checked = 0; checked <= 10; checked++ )
         {
            if( !oneword[checked] || !argument[checked] )
               break;
            if( LOWER( oneword[checked] ) == LOWER( argument[checked] ) )
               matched++;
         }

         if( ( matched > 1 && matched > ( checked / 2 ) ) || ( matched > 0 && checked < 2 ) )
         {
            pager_printf( ch, " %-20s ", oneword );
            if( ++found == 4 )
            {
               found = 0;
               send_to_pager( "\r\n", ch );
            }
            strncpy( lastmatch, oneword, MSL );
            totalmatched++;
            lHelp = pHelp;
            break;
         }
      }
   }
   if( found != 0 )
      send_to_pager( "\r\n", ch );

   if( totalmatched == 0 )
      send_to_pager( "No suggested help files.\r\n", ch );
   else if( totalmatched == 1 && lHelp )
   {
      send_to_pager( "Opening only suggested helpfile.\r\n", ch );
      show_help( ch, lHelp );
   }
}

CMDF( do_hedit )
{
   HELP_DATA *pHelp;

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor.\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;
      case SUB_HELP_EDIT:
         if( !( pHelp = ( HELP_DATA * ) ch->dest_buf ) )
         {
            bug( "%s: sub_help_edit: NULL ch->dest_buf", __FUNCTION__ );
            stop_editing( ch );
            return;
         }
         if( help_greeting == pHelp->text )
            help_greeting = NULL;
         STRFREE( pHelp->text );
         pHelp->text = copy_buffer( ch );
         if( !help_greeting )
            help_greeting = pHelp->text;
         stop_editing( ch );
         STRSET( pHelp->updated_by, ch->name );
         pHelp->updated = current_time;
         save_helps( true );
         return;
   }
   if( !( pHelp = get_help( ch, argument ) ) ) /* new help */
   {
      HELP_DATA *tHelp;
      char argnew[MIL];
      int lev;
      bool n_help = true;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "If you would like to create a new helpfile need to supply a keyword.\r\n", ch );
         return;
      }
      for( tHelp = first_help; tHelp; tHelp = tHelp->next )
      {
         if( !str_cmp( argument, tHelp->keyword ) )
         {
            pHelp = tHelp;
            n_help = false;
            break;
         }
      }

      if( pHelp && pHelp->nosave )
      {
         send_to_char( "This is a temporary help file that is associated with a skill or command.\r\n", ch );
         send_to_char( "To edit this help file you will need to use cedit or sset.\r\n", ch );
         return;
      }
      if( n_help )
      {
         if( isdigit( argument[0] ) )
         {
            lev = number_argument( argument, argnew );
            argument = argnew;
         }
         else
            lev = get_trust( ch );
         lev = URANGE( 0, lev, PERM_IMP );
         pHelp = new_help( );
         pHelp->keyword = STRALLOC( strupper( argument ) );
         pHelp->perm = lev;
         add_help( pHelp );
      }
   }

   if( pHelp && pHelp->nosave )
   {
      send_to_char( "This is a temporary help file that is associated with a skill or command.\r\n", ch );
      send_to_char( "To edit this help file you will need to use cedit or sset.\r\n", ch );
      return;
   }

   ch->substate = SUB_HELP_EDIT;
   ch->dest_buf = pHelp;
   start_editing( ch, pHelp->text );
}

/* Remove . if needed */
char *un_fix_help( char *text )
{
   static char newstr[MSL];
   int i = 0, j = 0;

   if( !text || text[0] == '\0' )
      return (char *)"";

   if( text[0] != '.' || text[1] != ' ' )
      return text;
   if( text[0] == '.' )
      i++;
   for( ; text[i] != '\0'; i++ )
      newstr[j++] = text[i];
   newstr[j] = '\0';
   return newstr;
}

/*
 * Remove carriage returns from a line
 * add in a . if needed
 * preserve the normal spaces
 */
char *help_fix( char *text )
{
   static char newstr[MSL];
   int i = 0, j = 0;

   if( !text || text[0] == '\0' )
      return (char *)"";

   if( text[0] == ' ' )
      newstr[j++] = '.';
   for( ; text[i] != '\0'; i++ )
   {
      if( text[i] != '\r' )
         newstr[j++] = text[i];
   }
   newstr[j] = '\0';
   return newstr;
}

void save_helps( bool autosave )
{
   FILE *fp;
   HELP_DATA *pHelp;
   char filename[MIL];

   if( autosave && !sysdata.autosavehelps )
      return;

   log_printf( "Saving %s", HELP_FILE );

   snprintf( filename, sizeof( filename ), "%s.temp", HELP_FILE );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: cant open %s", __FUNCTION__, filename );
      perror( filename );
      return;
   }

   for( pHelp = first_help; pHelp; pHelp = pHelp->next )
   {
      if( !pHelp->keyword || pHelp->nosave )
         continue;
      fprintf( fp, "#HELP\n" );
      if( pHelp->perm )
         fprintf( fp, "Perm      %d\n", pHelp->perm );
      fprintf( fp, "Keyword   %s~\n", pHelp->keyword );
      if( pHelp->updated )
         fprintf( fp, "Updated   %ld\n", pHelp->updated );
      if( pHelp->updated_by )
         fprintf( fp, "UpdatedBy %s~\n", pHelp->updated_by );
      if( pHelp->text )
         fprintf( fp, "Text      %s~\n", help_fix( pHelp->text ) );
      fprintf( fp, "%s\n\n", "End" );
   }
   fprintf( fp, "%s\n", "#END" );
   fclose( fp );
   fp = NULL;
   if( rename( filename, HELP_FILE ) )
      bug( "%s: Couldn't rename (%s) to (%s).", __FUNCTION__, filename, HELP_FILE );
   else
      chmod( HELP_FILE, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH );
}

CMDF( do_hset )
{
   HELP_DATA *pHelp;
   char arg1[MIL], arg2[MIL];

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: hset <field> [value] [help page]\r\n\r\n", ch );
      send_to_char( "Field being one of:\r\n", ch );
      send_to_char( "  perm  keyword  remove  save\r\n", ch );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_helps( false );
      send_to_char( "Saved.\r\n", ch );
      return;
   }

   if( str_cmp( arg1, "remove" ) )
      argument = one_argument( argument, arg2 );

   if( !argument || argument[0] == '\0' || !( pHelp = get_help( ch, argument ) ) )
   {
      send_to_char( "Can't find help on that subject.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "remove" ) )
   {
      UNLINK( pHelp, first_help, last_help, next, prev );
      free_help( pHelp );
      save_helps( true );
      send_to_char( "Removed.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "perm" ) )
   {
      int temp;

      if( !is_number( arg2 ) )
         temp = get_flag( arg2, perms_flag, PERM_MAX );
      else
         temp = atoi( arg2 );
      if( temp < 0 || temp > get_trust( ch ) || temp >= PERM_MAX )
      {
         send_to_char( "Invalid permission.\r\n", ch );
         return;
      }
      pHelp->perm = temp;
      save_helps( true );
      send_to_char( "Done.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "keyword" ) )
   {
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "You can't set a keyword to nothing.\r\n", ch );
         return;
      }
      STRSET( pHelp->keyword, strupper( arg2 ) );
      save_helps( true );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   do_hset( ch, (char *)"" );
}

/*
 * Show help topics in a level range - Thoric
 * Idea suggested by Gorog
 * prefix keyword indexing added by Fireblade
 */
CMDF( do_hlist )
{
   HELP_DATA *help;
   char *keywords, *idx = NULL, arg[MIL], keyword[MIL];
   const char *h1, *h2;
   int min, max, perm = -1, cnt = 0, level, col = 0, keycount = 0;
   bool hfound, kfound;

   min = 0;
   max = get_trust( ch );

   argument = one_argument( argument, arg );
   if( arg != NULL && arg[0] != '\0' )
   {
      if( !isdigit( arg[0] ) && !is_number( arg ) )
      {
         if( perm == -1 )
            perm = get_flag( arg, perms_flag, PERM_MAX );
         if( perm == -1 )
            idx = arg;
      }
      else
         perm = URANGE( min, atoi( arg ), max );
   }

   set_pager_color( AT_HELP, ch );
   h1 = color_str( AT_HELP, ch );
   h2 = color_str( AT_HELP2, ch );

   if( perm == -1 )
      pager_printf( ch, "%sHelp Topics in perm range %s%s %sto %s%s%s:\r\n\r\n",
         h1, h2, perms_flag[min], h1, h2, perms_flag[max], h1 );
   else
      pager_printf( ch, "%sHelp Topics in perm %s%s\r\n\r\n", h1, h2, perms_flag[perm] );
   for( level = min; level <= max; level++ )
   {
      hfound = false;
      col = 0;
      for( help = first_help; help; help = help->next )
      {
         kfound = false;
         if( help->perm == level && ( !idx || nifty_is_name_prefix( idx, help->keyword ) )
         && ( perm == -1 || help->perm == perm ) )
         {
            keywords = help->keyword;
            while( keywords && keywords[0] != '\0' )
            {
               keywords = one_argument( keywords, keyword );
               if( idx && !nifty_is_name_prefix( idx, keyword ) )
                  continue;
               if( !hfound )
                  pager_printf( ch, " %sPermission [%s%s%s]\r\n", h1, h2, perms_flag[level], h1 );
               hfound = true;
               pager_printf( ch, "%s%20.20s&D", h2, keyword );
               if( ++col == 3 )
               {
                  send_to_pager( "\r\n", ch );
                  col = 0;
               }
               else
                  send_to_pager( " ", ch );
               keycount++;
               kfound = true;
            }
            if( kfound )
               ++cnt;
         }
      }
      if( col != 0 )
         send_to_pager( "\r\n", ch );
   }
   if( cnt )
      pager_printf( ch, "\r\n%s%d %spage%s found with a total of %s%d %skeyword%s.\r\n", h2, cnt, h1,
         cnt != 1 ? "s" : "", h2, keycount, h1, keycount != 1 ? "s" : "" );
   else
      pager_printf( ch, "%sNone found.\r\n", h1 );
}

CMDF( do_credits )
{
   do_help( ch, (char *)"credits" );
}

CMDF( do_check_helps )
{
   CMDTYPE *command;
   int col = 0, skcnt = 0, cmdcnt = 0, hash, sn, cmdttl = 0, skttl = 0;

   for( hash = 0; hash < 126; hash++ )
   {
      for( command = command_hash[hash]; command; command = command->next )
      {
         if( !command || !command->name )
            continue;
         ++cmdttl;
         if( command->htext )
            continue;
         if( ++cmdcnt == 1 )
            send_to_pager( "\r\nCommands that have no help file:\r\n", ch );
         pager_printf( ch, "%s%15.15s", col != 0 ? "  " : "", command->name );
         if( ++col == 5 )
         {
            col = 0;
            send_to_pager( "\r\n", ch );
         }
      }
   }
   if( col != 0 )
      send_to_pager( "\r\n", ch );

   col = 0;
   for( sn = 0; sn < top_sn; sn++ )
   {
      if( !skill_table[sn] || !skill_table[sn]->name )
         continue;
      ++skttl;
      if( skill_table[sn]->htext )
         continue;
      if( ++skcnt == 1 )
         send_to_pager( "\r\nSkills that have no help file:\r\n", ch );
      pager_printf( ch, "%s%15.15s", col != 0 ? "  " : "", skill_table[sn]->name );
      if( ++col == 5 )
      {
         col = 0;
         send_to_pager( "\r\n", ch );
      }
   }
   if( col != 0 )
      send_to_pager( "\r\n", ch );

   if( !cmdcnt )
      send_to_pager( "\r\nAll of your commands have matching help files.\r\n", ch );
   else
      pager_printf( ch, "\r\n%d of %d command%s were found to not have a matching help file.\r\n", cmdcnt, cmdttl, cmdttl == 1 ? "" : "s" );
   if( !skcnt )
      send_to_pager( "All of your skills have matching help files.\r\n", ch );
   else
      pager_printf( ch, "%d of %d skill%s were found to not have a matching help file.\r\n", skcnt, skttl, skttl == 1 ? "" : "s" );
}

bool valid_help( char *argument )
{
   if( !argument || !get_help( NULL, argument ) )
      return false;
   return true;
}

void update_skill_helps( void )
{
   HELP_DATA *phelp;
   int sn;

   for( sn = 0; sn < top_sn; sn++ )
   {
      if( !skill_table[sn] || !skill_table[sn]->name || skill_table[sn]->htext )
         continue;
      if( !( phelp = get_help( NULL, skill_table[sn]->name ) ) )
         continue;
      STRSET( skill_table[sn]->htext, phelp->text );
      phelp->nosave = true;
      bug( "%s: '%s' had no help text, found matching help file.", __FUNCTION__, skill_table[sn]->name );
   }
}

void update_command_helps( void )
{
   HELP_DATA *phelp;
   CMDTYPE *command;
   int hash;

   for( hash = 0; hash < 126; hash++ )
   {
      for( command = command_hash[hash]; command; command = command->next )
      {
         if( !command || !command->name || command->htext )
            continue;
         if( !( phelp = get_help( NULL, command->name ) ) )
            continue;
         STRSET( command->htext, phelp->text );
         phelp->nosave = true;
         bug( "%s: '%s' had no help text, found matching help file.", __FUNCTION__, command->name );
      }
   }
}
