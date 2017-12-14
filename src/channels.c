/*****************************************************************************
 *---------------------------------------------------------------------------*
 * LoP (C) 2006 - 2012 by: the LoP team.                                     *
 *---------------------------------------------------------------------------*
 *                          Dynamic Channel Handler                          *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "h/mud.h"

#define CHANNEL_DIR "channels/"
#define CHANNEL_FILE CHANNEL_DIR "channels.dat"

typedef struct chistory_data CHISTORY_DATA;
typedef struct channel_data CHANNEL_DATA;

CHANNEL_DATA *first_channel, *last_channel;

struct chistory_data
{
   CHISTORY_DATA *next, *prev;
   char *name;
   char *text;
   char *restrict;
   time_t chtime;
   int speaking;
};

struct channel_data
{
   CHANNEL_DATA *next, *prev;
   CHISTORY_DATA *first_chistory, *last_chistory;
   char *name;
   char *color;
   int type;
   int permission;
   int currhistory;
   int maxhistory;
   bool scramble;
   bool adult; /* Adult only channel? */
};

bool check_is_ignoring( CHAR_DATA *ch, char *name )
{
   IGNORE_DATA *temp;

   if( is_npc( ch ) )
      return false;

   for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
   {
      if( nifty_is_name( temp->name, name ) )
         return true;
   }

   return false;
}

void save_chistory( CHANNEL_DATA *channel )
{
   CHISTORY_DATA *chistory;
   FILE *fp;
   char filename[MIL];

   if( !channel )
      return;

   if( !channel->name || channel->name[0] == '\0' )
   {
      bug( "%s: trying to save a channel with no name.", __FUNCTION__ );
      return;
   }

   snprintf( filename, sizeof( filename ), "%s%s", CHANNEL_DIR, channel->name );

   if( !channel->first_chistory )
   {
      remove_file( filename );
      return;
   }

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, filename );
      perror( filename );
      return;
   }

   for( chistory = channel->first_chistory; chistory; chistory = chistory->next )
   {
      if( !chistory->text )
         continue;
      fprintf( fp, "%s", "#CHISTORY\n" );
      if( chistory->name )
         fprintf( fp, "Name     %s~\n", chistory->name );
      fprintf( fp, "Time     %ld\n", chistory->chtime );
      if( chistory->speaking != -1 )
         fprintf( fp, "Speaking %d\n", chistory->speaking );
      fprintf( fp, "Text     %s~\n", chistory->text );
      if( chistory->restrict )
         fprintf( fp, "Restrict %s~\n", chistory->restrict );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

void save_channels( void )
{
   CHANNEL_DATA *channel;
   FILE *fp;

   if( !( fp = fopen( CHANNEL_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, CHANNEL_FILE );
      perror( CHANNEL_FILE );
      return;
   }
   for( channel = first_channel; channel; channel = channel->next )
   {
       fprintf( fp, "%s", "#CHANNEL\n" );
       fprintf( fp, "Name       %s~\n", channel->name );
       fprintf( fp, "Color      %s~\n", channel->color );
       if( channel->type >= 0 && channel->type < CHANNEL_MAX )
          fprintf( fp, "Type       %s~\n", channelflags[channel->type] );
       if( channel->permission >= 0 && channel->permission < PERM_MAX )
          fprintf( fp, "Permission %s~\n", perms_flag[channel->permission] );
       fprintf( fp, "MaxHistory %d\n", channel->maxhistory );
       if( channel->scramble )
          fprintf( fp, "%s", "Scramble\n" );
       if( channel->adult )
          fprintf( fp, "%s", "Adult\n" );
       fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

void free_chistory( CHISTORY_DATA *chistory )
{
   if( !chistory )
      return;
   STRFREE( chistory->name );
   STRFREE( chistory->text );
   STRFREE( chistory->restrict );
   DISPOSE( chistory );
}

void free_channel( CHANNEL_DATA *channel )
{
   CHISTORY_DATA *chistory, *chistory_next;

   if( !channel )
      return;
   STRFREE( channel->name );
   STRFREE( channel->color );
   for( chistory = channel->first_chistory; chistory; chistory = chistory_next )
   {
      chistory_next = chistory->next;
      UNLINK( chistory, channel->first_chistory, channel->last_chistory, next, prev );
      free_chistory( chistory );
   }
   DISPOSE( channel );
}

CHANNEL_DATA *get_channel( const char *argument )
{
   CHANNEL_DATA *channel;

   for( channel = first_channel; channel; channel = channel->next )
   {
      if( !str_cmp( channel->name, argument ) )
      {
         return channel;
      }
   }
   return NULL;
}

void show_chistory( CHAR_DATA *ch, CHANNEL_DATA *channel, bool showall )
{
   MCLASS_DATA *mclass;
   CHISTORY_DATA *chistory;
   bool found = false;

   if( !ch || !channel )
      return;
   for( chistory = channel->first_chistory; chistory; chistory = chistory->next )
   {
      if( !showall && chistory->restrict && ch->pcdata )
      {
         if( channel->type == CHANNEL_RACETALK
         && ( !race_table[ch->race] || str_cmp( chistory->restrict, race_table[ch->race]->name ) ) )
            continue;
         if( channel->type == CHANNEL_FCHAT && !find_friend( ch, chistory->restrict ) )
            continue;
         if( channel->type == CHANNEL_CLASS )
         {
            bool ccont = true;

            for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
            {
               if( mclass->wclass != -1 && !str_cmp( chistory->restrict, dis_class_name( mclass->wclass ) ) )
               {
                  ccont = false;
                  break;
               }
            }
            if( ccont )
               continue;
         }
         if( channel->type == CHANNEL_CLAN
         && ( !ch->pcdata || !ch->pcdata->clan || str_cmp( chistory->restrict, ch->pcdata->clan->name ) ) )
            continue;
         if( channel->type == CHANNEL_NATION
         && ( !ch->pcdata || !ch->pcdata->nation || str_cmp( chistory->restrict, ch->pcdata->nation->name ) ) )
            continue;
         if( channel->type == CHANNEL_COUNCIL
         && ( !ch->pcdata || !ch->pcdata->council || str_cmp( chistory->restrict, ch->pcdata->council->name ) ) )
            continue;
         if( channel->type == CHANNEL_LOG && get_trust( ch ) < atoi( chistory->restrict ) )
            continue;
      }

      if( channel->type == CHANNEL_YELL
      && ( !ch->in_room || !ch->in_room->area || str_cmp( chistory->restrict, ch->in_room->area->name ) ) )
        continue;

      if( !showall && chistory->name && check_is_ignoring( ch, chistory->name ) )
         continue;

      found = true;

      ch_printf( ch, "%s[%s]", channel->color ? channel->color : "&[plain]", distime( chistory->chtime ) );
      if( chistory->name )
      {
         if( !str_cmp( ch->name, chistory->name ) )
            ch_printf( ch, " You %s", channel->name );
         else
            ch_printf( ch, " %s %ss", chistory->name, channel->name );
      }

      send_to_char( " ' ", ch );

      if( chistory->speaking == -1 )
         send_to_char( chistory->text, ch );
      else
      {
         int speakswell = knows_language( ch, chistory->speaking );
         char *sbuf = chistory->text;

         if( speakswell < 85 )
            sbuf = translate( speakswell, chistory->text, lang_names[chistory->speaking] );
         send_to_char( sbuf, ch );
      }

      ch_printf( ch, " &D%s'\r\n", channel->color ? channel->color : "&[plain]" );
   }
   if( !found )
      ch_printf( ch, "Nothing has been said on %s.\r\n", channel->name );
}

CMDF( do_setchannel )
{
   CHANNEL_DATA *channel;
   char arg[MIL], arg2[MIL];
   int col = 0;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: setchannel <channel>\r\n", ch );
      send_to_char( "Usage: setchannel <channel> create/delete/clearhistory\r\n", ch );
      send_to_char( "Usage: setchannel <channel> name/color/type/perm/maxhistory <setting>\r\n", ch );
      for( channel = first_channel; channel; channel = channel->next )
      {
         ch_printf( ch, "%s[%3d]", channel->color ? channel->color : "&[plain]", channel->currhistory );
         ch_printf( ch, "%-20.20s", channel->name );
         if( ++col == 3 )
         {
            col = 0;
            send_to_char( "\r\n", ch );
         }
      }
      if( col != 0 )
         send_to_char( "\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   channel = get_channel( arg );
   if( arg2 == NULL || arg2[0] == '\0' )
   {
      if( !channel )
      {
         ch_printf( ch, "No channel by the name of %s.\r\n", arg );
         return;
      }
      ch_printf( ch, "&CName:       &W%s\r\n", channel->name );
      ch_printf( ch, "&CColor:      %sColored&D\r\n", channel->color );
      ch_printf( ch, "&CType:       &W%s\r\n", channelflags[channel->type] );
      ch_printf( ch, "&CPerm:       &W%s\r\n", perms_flag[channel->permission] );
      ch_printf( ch, "&CHistory:    &W%d\r\n", channel->currhistory );
      ch_printf( ch, "&CMaxhistory: &W%d&D\r\n", channel->maxhistory );
      ch_printf( ch, "&CScramble:   &W%s\r\n", channel->scramble ? "Yes" : "No" );
      ch_printf( ch, "&CAdult:      &W%s\r\n", channel->adult ? "Yes" : "No" );
      send_to_char( "Full channel history:\r\n", ch );
      show_chistory( ch, channel, true );
      return;
   }

   if( !str_cmp( arg2, "create" ) )
   {
      if( channel )
      {
         ch_printf( ch, "There is already a channel named %s.\r\n", channel->name );
         return;
      }
      CREATE( channel, CHANNEL_DATA, 1 );
      channel->name = STRALLOC( arg );
      channel->color = STRALLOC( "&[plain]" );
      channel->type = CHANNEL_GLOBAL;
      channel->permission = PERM_ALL;
      channel->maxhistory = 20;
      channel->currhistory = 0;
      channel->first_chistory = channel->last_chistory = NULL;
      LINK( channel, first_channel, last_channel, next, prev );
      save_channels( );
      ch_printf( ch, "%s channel created.\r\n", channel->name );
      return;
   }

   if( !channel )
   {
      ch_printf( ch, "No channel by the name of %s.\r\n", arg );
      return;
   }
   if( !str_cmp( arg2, "delete" ) )
   {
      UNLINK( channel, first_channel, last_channel, next, prev );
      free_channel( channel );
      save_channels( );
      ch_printf( ch, "%s channel has been deleted.\r\n", arg );
      return;
   }
   if( !str_cmp( arg2, "clearhistory" ) )
   {
      char filename[MSL];
      CHISTORY_DATA *chistory, *chistory_next;

      for( chistory = channel->first_chistory; chistory; chistory = chistory_next )
      {
         chistory_next = chistory->next;
         UNLINK( chistory, channel->first_chistory, channel->last_chistory, next, prev );
         free_chistory( chistory );
      }
      sprintf( filename, "%s%s", CHANNEL_DIR, channel->name );
      remove_file( filename );
      channel->currhistory = 0;
      ch_printf( ch, "%s's history has been cleared.\r\n", channel->name );
      return;
   }
   if( !str_cmp( arg2, "scramble" ) )
   {
      channel->scramble = !channel->scramble;
      save_channels( );
      ch_printf( ch, "%s's will %s be scrambled.\r\n", channel->name, channel->scramble ? "now" : "no longer" );
      return;
   }
   if( !str_cmp( arg2, "adult" ) )
   {
      channel->adult = !channel->adult;
      save_channels( );
      ch_printf( ch, "%s's is %s an adult only channel.\r\n", channel->name, channel->adult ? "now" : "no longer" );
      return;
   }
   if( !str_cmp( arg2, "color" ) )
   {
      STRSET( channel->color, argument );
      save_channels( );
      ch_printf( ch, "%s's color has been set.\r\n", channel->name );
      return;
   }
   if( !str_cmp( arg2, "type" ) )
   {
      int value = get_flag( argument, channelflags, CHANNEL_MAX );

      if( value < 0 || value >= CHANNEL_MAX )
      {
         send_to_char( "Invalid channel type.\r\n", ch );
         return;
      }
      channel->type = value;
      save_channels( );
      ch_printf( ch, "%s type has been set to %s.\r\n", channel->name, channelflags[channel->type] );
      return;
   }
   if( !str_cmp( arg2, "perm" ) )
   {
      int value = get_flag( argument, perms_flag, PERM_MAX );
      if( value < 0 || value >= PERM_MAX )
      {
         send_to_char( "Invalid permission.\r\n", ch );
         return;
      }
      channel->permission = value;
      save_channels( );
      ch_printf( ch, "%s permission has been set to %s.\r\n", channel->name, perms_flag[channel->permission] );
      return;
   }
   if( !str_cmp( arg2, "maxhistory" ) )
   {
      channel->maxhistory = atoi( argument );
      channel->maxhistory = URANGE( 0, channel->maxhistory, 100 );
      save_channels( );
      ch_printf( ch, "%s maxhistory has been set to %d.\r\n", channel->name, channel->maxhistory );
      return;
   }
   if( !str_cmp( arg2, "name" ) )
   {
      STRSET( channel->name, argument );
      save_channels( );
      ch_printf( ch, "%s channel has been changed to %s.\r\n", arg, channel->name );
      return;
   }
   do_setchannel( ch, (char *)"" );
}

void fread_chistory( CHANNEL_DATA *channel, FILE *fp )
{
   CHISTORY_DATA *chistory;
   const char *word;
   bool fMatch;

   CREATE( chistory, CHISTORY_DATA, 1 );
   if( !chistory )
   {
      bug( "%s: chistory is NULL after CREATE.", __FUNCTION__ );
      return;
   }
   chistory->name = NULL;
   chistory->text = NULL;
   chistory->restrict = NULL;
   chistory->speaking = -1;

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
               LINK( chistory, channel->first_chistory, channel->last_chistory, next, prev );
               channel->currhistory++;
               return;
            }
            break;

         case 'N':
            KEY( "Name", chistory->name, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Restrict", chistory->restrict, fread_string( fp ) );
            break;

         case 'S':
            KEY( "Speaking", chistory->speaking, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Text", chistory->text, fread_string( fp ) );
            KEY( "Time", chistory->chtime, fread_time( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_chistory( chistory );
}

void load_chistorys( CHANNEL_DATA *channel )
{
   FILE *fp;
   char filename[MIL];

   if( !channel || !channel->name )
      return;

   snprintf( filename, sizeof( filename ), "%s%s", CHANNEL_DIR, channel->name );

   if( !( fp = fopen( filename, "r" ) ) )
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
      if( !str_cmp( word, "CHISTORY" ) )
      {
         fread_chistory( channel, fp );
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

void fread_channel( FILE *fp )
{
   CHANNEL_DATA *channel;
   const char *word;
   char *infoflags, flag[MIL];
   int value;
   bool fMatch;

   CREATE( channel, CHANNEL_DATA, 1 );
   if( !channel )
   {
      bug( "%s: channel is NULL after CREATE.", __FUNCTION__ );
      return;
   }
   channel->name = NULL;
   channel->type = CHANNEL_GLOBAL;
   channel->permission = PERM_ALL;
   channel->maxhistory = 0;
   channel->currhistory = 0;
   channel->first_chistory = channel->last_chistory = NULL;
   channel->scramble = false;
   channel->adult = false;

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

         case 'A':
            if( !str_cmp( word, "Adult" ) )
            {
               channel->adult = true;
               fMatch = true;
               break;
            }
            break;

         case 'C':
            KEY( "Color", channel->color, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !channel->color )
                  channel->color = STRALLOC( "&[plain]" );
               LINK( channel, first_channel, last_channel, next, prev );
               load_chistorys( channel );
               return;
            }
            break;

         case 'M':
            KEY( "MaxHistory", channel->maxhistory, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", channel->name, fread_string( fp ) );
            break;

         case 'P':
            SKEY( "Permission", channel->permission, fp, perms_flag, PERM_MAX );
            break;

         case 'S':
            if( !str_cmp( word, "Scramble" ) )
            {
               channel->scramble = true;
               fMatch = true;
               break;
            }
            break;

         case 'T':
            SKEY( "Type", channel->type, fp, channelflags, CHANNEL_MAX );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_channel( channel );
}

void load_channels( void )
{
   FILE *fp;

   first_channel = last_channel = NULL;

   if( !( fp = fopen( CHANNEL_FILE, "r" ) ) )
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
      if( !str_cmp( word, "CHANNEL" ) )
      {
         fread_channel( fp );
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

void free_all_channels( void )
{
   CHANNEL_DATA *channel, *channel_next;

   for( channel = first_channel; channel; channel = channel_next )
   {
      channel_next = channel->next;
      UNLINK( channel, first_channel, last_channel, next, prev );
      free_channel( channel );
   }
}

void add_to_history( CHANNEL_DATA *channel, CHAR_DATA *ch, int trust, int speaking, const char *argument )
{
   MCLASS_DATA *mclass;
   CHISTORY_DATA *chistory, *chistory_next, *chistory_remove = NULL;
   int y = 0;

   if( !channel || !argument || argument[0] == '\0' )
      return;
   if( !channel->first_chistory && channel->maxhistory <= 0 )
      return;
   for( chistory = channel->first_chistory; chistory; chistory = chistory_next )
   {
      chistory_next = chistory->next;
      if( chistory->restrict && ch && ch->pcdata )
      {
         if( channel->type == CHANNEL_RACETALK
         && ( !race_table[ch->race] || str_cmp( chistory->restrict, race_table[ch->race]->name ) ) )
            continue;
         if( channel->type == CHANNEL_FCHAT && !find_friend( ch, chistory->restrict ) )
            continue;
         if( channel->type == CHANNEL_CLASS )
         {
            bool ccont = true;

            for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
            {
               if( mclass->wclass >= 0 && !str_cmp( chistory->restrict, dis_class_name( mclass->wclass ) ) )
               {
                  ccont = false;
                  break;
               }
            }
            if( ccont )
               continue;
         }
         if( channel->type == CHANNEL_CLAN
         && ( !ch->pcdata || !ch->pcdata->clan || str_cmp( chistory->restrict, ch->pcdata->clan->name ) ) )
            continue;
         if( channel->type == CHANNEL_NATION
         && ( !ch->pcdata || !ch->pcdata->nation || str_cmp( chistory->restrict, ch->pcdata->nation->name ) ) )
            continue;
         if( channel->type == CHANNEL_COUNCIL
         && ( !ch->pcdata || !ch->pcdata->council || str_cmp( chistory->restrict, ch->pcdata->council->name ) ) )
            continue;
      }

      if( channel->type == CHANNEL_YELL
      && ( !ch->in_room || !ch->in_room->area || str_cmp( chistory->restrict, ch->in_room->area->name ) ) )
         continue;

      if( !chistory_remove )
         chistory_remove = chistory;
      if( ++y >= channel->maxhistory )
      {
         UNLINK( chistory_remove, channel->first_chistory, channel->last_chistory, next, prev );
         free_chistory( chistory_remove );
         channel->currhistory--;
         chistory_remove = NULL;
         y--;
      }
   }
   if( channel->maxhistory <= 0 )
      return;
   chistory = NULL;
   CREATE( chistory, CHISTORY_DATA, 1 );
   chistory->speaking = speaking;
   smash_tilde( (char *)argument );
   chistory->text = STRALLOC( argument );
   chistory->restrict = NULL;
   if( ch )
   {
      chistory->name = STRALLOC( ch->name );
      if( channel->type == CHANNEL_RACETALK && race_table[ch->race] )
         chistory->restrict = STRALLOC( race_table[ch->race]->name );
      if( channel->type == CHANNEL_FCHAT )
         chistory->restrict = STRALLOC( ch->name );
      if( channel->type == CHANNEL_CLASS )
         chistory->restrict = STRALLOC( dis_main_class_name( ch ) );
      if( channel->type == CHANNEL_CLAN && ch->pcdata && ch->pcdata->clan && ch->pcdata->clan->name )
         chistory->restrict = STRALLOC( ch->pcdata->clan->name );
      if( channel->type == CHANNEL_NATION && ch->pcdata && ch->pcdata->nation && ch->pcdata->nation->name )
         chistory->restrict = STRALLOC( ch->pcdata->nation->name );
      if( channel->type == CHANNEL_COUNCIL && ch->pcdata && ch->pcdata->council && ch->pcdata->council->name )
         chistory->restrict = STRALLOC( ch->pcdata->council->name );
      if( channel->type == CHANNEL_YELL && ch->in_room && ch->in_room->area && ch->in_room->area->name )
         chistory->restrict = STRALLOC( ch->in_room->area->name );
      if( channel->type == CHANNEL_LOG )
      {
         char buf[MSL];

         snprintf( buf, sizeof( buf ), "%d", get_trust( ch ) );
         chistory->restrict = STRALLOC( buf );
      }
   }
   else if( channel->type == CHANNEL_LOG && trust )
   {
      char buf[MSL];

      snprintf( buf, sizeof( buf ), "%d", trust );
      chistory->restrict = STRALLOC( buf );
   }

   chistory->chtime = current_time;
   LINK( chistory, channel->first_chistory, channel->last_chistory, next, prev );
   channel->currhistory++;
   save_chistory( channel );
}

bool can_use_channel( CHAR_DATA *ch, CHANNEL_DATA *channel )
{
   if( !channel )
      return false;
   if( !ch )
      return true;
   if( get_trust( ch ) < channel->permission )
      return false;
   if( channel->type == CHANNEL_RACETALK && !race_table[ch->race] )
      return false;
   if( channel->type == CHANNEL_FCHAT ) /* FChat can be used by anyone */
      return true;
   if( channel->type == CHANNEL_CLASS ) /* Classtalk can be used by anyone even ones unknown are well an unknown class */
      return true;
   if( channel->type == CHANNEL_CLAN && ( !ch->pcdata || !ch->pcdata->clan ) )
      return false;
   if( channel->type == CHANNEL_NATION && ( !ch->pcdata || !ch->pcdata->nation ) )
      return false;
   if( channel->type == CHANNEL_COUNCIL && ( !ch->pcdata || !ch->pcdata->council ) )
      return false;
   if( channel->type == CHANNEL_YELL && ( !ch->in_room || !ch->in_room->area ) )
      return false;
   if( channel->adult && ( !ch->pcdata || !xIS_SET( ch->pcdata->flags, PCFLAG_ADULT ) ) )
      return false;
   return true;
}

CMDF( do_channels )
{
   CHANNEL_DATA *channel;
   int col = 0;

   if( !ch )
      return;
   for( channel = first_channel; channel; channel = channel->next )
   {
      if( !can_use_channel( ch, channel ) )
         continue;
      ch_printf( ch, "%s%20.20s", channel->color ? channel->color : "&[plain]", channel->name );
      if( ++col == 4 )
      {
         col = 0;
         send_to_char( "\r\n", ch );
      }
   }
   if( col != 0 )
      send_to_char( "\r\n", ch );
}

bool is_listening( CHAR_DATA *ch, CHANNEL_DATA *channel )
{
   char *listening, chan[MIL];

   if( !ch || !channel )
      return false;
   listening = ch->pcdata->channels;
   while( listening && listening[0] != '\0' )
   {
      listening = one_argument( listening, chan );
      if( !str_cmp( channel->name, chan ) )
         return true;
   }
   return false;
}

bool handle_channels( CHAR_DATA *ch, CHANNEL_DATA *channel, int trust, const char *argument )
{
   DESCRIPTOR_DATA *d, *d_next;
   PER_HISTORY *phistory;
   char buf[MSL], hbuf[MSL], thbuf[MSL];
   int speaking = -1, lang;

   if( !channel )
      return false;

   if( !ch && ( !argument || argument[0] == '\0' ) )
      return false;

   if( ch && !can_use_channel( ch, channel ) )
      return false;

   hbuf[0] = '\0';
   if( ch )
   {
      if( !argument || argument[0] == '\0' || channel->type == CHANNEL_LOG )
      {
         if( !is_npc( ch ) && channel->type == CHANNEL_YELL )
         {
            if( !ch->pcdata->first_yell )
               send_to_char( "You haven't heard anyone yell.\r\n", ch );
            else
            {
               for( phistory = ch->pcdata->first_yell; phistory; phistory = phistory->next )
                  send_to_char( phistory->text, ch );
            }
            return true;
         }
         if( !is_npc( ch ) && channel->type == CHANNEL_FCHAT )
         {
            if( !ch->pcdata->first_fchat )
               send_to_char( "You haven't heard anyone fchat.\r\n", ch );
            else
            {
               for( phistory = ch->pcdata->first_fchat; phistory; phistory = phistory->next )
                  send_to_char( phistory->text, ch );
            }
            return true;
         }
         show_chistory( ch, channel, false );
         return true;
      }
     
      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      {
         if( xIS_SET( ch->speaking, lang_array[lang] ) )
         {
            speaking = lang;
            break;
         }
      }

      ch_printf( ch, "%sYou %s ' %s &D%s'\r\n", channel->color ? channel->color : "&[plain]",
         channel->name, argument, channel->color ? channel->color : "&[plain]" );
      snprintf( buf, sizeof( buf ), "%s%s %ss ' %s &D%s'\r\n", channel->color ? channel->color : "&[plain]",
         capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), channel->name, argument, channel->color ? channel->color : "&[plain]" );
      if( channel->type == CHANNEL_YELL )
         snprintf( hbuf, sizeof( hbuf ), "%s%s yelled ' %s &D%s'\r\n", channel->color ? channel->color : "&[plain]",
            capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), argument, channel->color ? channel->color : "&[plain]" );
      if( channel->type == CHANNEL_FCHAT )
         snprintf( hbuf, sizeof( hbuf ), "%s%s fchated ' %s &D%s'\r\n", channel->color ? channel->color : "&[plain]",
            capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), argument, channel->color ? channel->color : "&[plain]" );
   }
   else
   {
      snprintf( buf, sizeof( buf ), "%s%s: %s\r\n", channel->color, capitalize( channel->name ), argument );
      if( channel->type == CHANNEL_YELL )
         snprintf( hbuf, sizeof( hbuf ), "%s%s yelled ' %s &D%s'\r\n", channel->color ? channel->color : "&[plain]",
            ch ? ( capitalize( is_npc( ch ) ? ch->short_descr : ch->name ) ) : "Unknown", argument, channel->color ? channel->color : "&[plain]" );
      if( channel->type == CHANNEL_FCHAT )
         snprintf( hbuf, sizeof( hbuf ), "%s%s fchated ' %s &D%s'\r\n", channel->color ? channel->color : "&[plain]",
            ch ? ( capitalize( is_npc( ch ) ? ch->short_descr : ch->name ) ) : "Unknown", argument, channel->color ? channel->color : "&[plain]" );
   }
   add_to_history( channel, ch, trust, speaking, argument );

   for( d = first_descriptor; d; d = d_next )
   {
      d_next = d->next;
      if( !d || !d->character )
          continue;
      if( d->connected != CON_PLAYING )
         continue;
      if( ch && d->character == ch )
         continue;
      if( !can_use_channel( d->character, channel ) )
         continue;
      if( ch && ch->pcdata && d->character && d->character->pcdata )
      {
         if( channel->type == CHANNEL_RACETALK && ch->race != d->character->race )
            continue;
         if( channel->type == CHANNEL_FCHAT && !find_friend( d->character, ch->name ) )
            continue;
         if( channel->type == CHANNEL_CLASS )
         {
            const char *uclass = dis_main_class_name( ch );
            const char *duclass = dis_main_class_name( d->character );

            if( str_cmp( duclass, uclass ) )
               continue;
         }
         if( channel->type == CHANNEL_CLAN && ch->pcdata->clan != d->character->pcdata->clan )
            continue;
         if( channel->type == CHANNEL_NATION && ch->pcdata->nation != d->character->pcdata->nation )
            continue;
         if( channel->type == CHANNEL_COUNCIL && ch->pcdata->council != d->character->pcdata->council )
            continue;
      }

      /* Lol should probably do this one here instead of only if they have pcdata */
      if( channel->type == CHANNEL_YELL && ch && d->character
      && ( !ch->in_room || !d->character->in_room || ch->in_room->area != d->character->in_room->area ) )
         continue;

      if( channel->type == CHANNEL_LOG && get_trust( d->character ) < ( ch ? get_trust( ch ) : trust ) )
         continue;

      if( !is_listening( d->character, channel ) )
         continue;

      /* Check to see if character is ignoring speaker */
      if( ch && d->character && is_ignoring( d->character, ch ) )
      {
         /* continue unless speaker is an immortal */
         if( !is_immortal( ch ) || get_trust( d->character ) > get_trust( ch ) )
            continue;
         else
         {
            set_char_color( AT_IGNORE, d->character );
            ch_printf( d->character, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
         }
      }

      if( speaking == -1 )
      {
         send_to_char( buf, d->character );
         snprintf( thbuf, sizeof( thbuf ), "%s", hbuf );
      }
      else
      {
         int speakswell = knows_language( d->character, speaking );
         char *sbuf = ( char * ) argument;

         if( channel->scramble && speakswell < 85 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );

         if( ch )
         {
            sbuf = drunk_speech( sbuf, ch );

            snprintf( buf, sizeof( buf ), "%s%s %ss ' %s &D%s'\r\n", channel->color ? channel->color : "&[plain]",
               capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), channel->name, sbuf, channel->color ? channel->color : "&[plain]" );
         }
         else
            snprintf( buf, sizeof( buf ), "%s%s: %s\r\n", channel->color, channel->name, sbuf );

         send_to_char( buf, d->character );

         if( channel->type == CHANNEL_YELL )
            snprintf( thbuf, sizeof( thbuf ), "%s%s yelled ' %s &D%s'", channel->color ? channel->color : "&[plain]",
               capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), sbuf, channel->color ? channel->color : "&[plain]" );
         if( channel->type == CHANNEL_FCHAT )
            snprintf( thbuf, sizeof( thbuf ), "%s%s fchated ' %s &D%s'", channel->color ? channel->color : "&[plain]",
               capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), sbuf, channel->color ? channel->color : "&[plain]" );
      }

      if( channel->type == CHANNEL_YELL )
      {
         if( !is_npc( d->character ) )
            add_phistory( 2, d->character, thbuf );
      }
      if( channel->type == CHANNEL_FCHAT )
      {
         if( !is_npc( d->character ) )
            add_phistory( 4, d->character, thbuf );
      }
   }
   return true;
}

bool check_channel( CHAR_DATA *ch, char *command, char *argument )
{
   CHANNEL_DATA *channel;

   for( channel = first_channel; channel; channel = channel->next )
   {
      if( !channel->name )
         continue;
      if( ch && !can_use_channel( ch, channel ) )
         continue;
      if( !str_prefix( command, channel->name ) )
      {
         handle_channels( ch, channel, get_trust( ch ), argument );
         return true;
      }
   }
   return false;
}

/* Writes a string to the log, extended version - Thoric */
void log_string_plus( const char *str, short log_type, int level )
{
   CHANNEL_DATA *channel;
   struct timeval now_time;

   /* Update time. */
   gettimeofday( &now_time, NULL );
   current_time = ( time_t ) now_time.tv_sec;
   current_time += ( time_t ) TIME_MODIFY;

   fprintf( stderr, "%s :: %s\n", distime( current_time ), str );
   switch( log_type )
   {
      default:
         if( ( channel = get_channel( "log" ) ) )
            handle_channels( NULL, channel, level, ( char * )str );
         break;

      case LOG_BUG:
         if( ( channel = get_channel( "bug" ) ) )
            handle_channels( NULL, channel, level, ( char * )str );
         break;

      case LOG_BUILD:
         if( ( channel = get_channel( "build" ) ) )
            handle_channels( NULL, channel, level, ( char * )str );
         break;

      case LOG_COMM:
         if( ( channel = get_channel( "comm" ) ) )
            handle_channels( NULL, channel, level, ( char * )str );
         break;

      case LOG_WARN:
         if( ( channel = get_channel( "warn" ) ) )
            handle_channels( NULL, channel, level, ( char * )str );
         break;

      case LOG_ALL:
         break;
   }
}

void log_printf_plus( short log_type, int level, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   log_string_plus( buf, log_type, level );
}

void log_printf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   log_string_plus( buf, LOG_NORMAL, PERM_LOG );
}

void to_channel( const char *argument, const char *channel, int level )
{
   CHANNEL_DATA *chandata;

   if( argument[0] == '\0' )
      return;

   if( ( chandata = get_channel( channel ) ) )
      handle_channels( NULL, chandata, level, argument );
}

void to_channel_printf( const char *channel, int level, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   to_channel( buf, channel, level );
}

CMDF( do_listen )
{
   CHANNEL_DATA *c;
   char arg[MIL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&cCurrently tuned into:\r\n", ch );
      if( ch->pcdata->channels )
      {
         char *listening = ch->pcdata->channels;
         int col = 0;

         while( listening && listening[0] != '\0' )
         {
            listening = one_argument( listening, arg );
            ch_printf( ch, "&W%20.20s", arg );
            if( ++col == 4 )
            {
               col = 0;
               send_to_char( "\r\n", ch );
            }
         }
         if( col != 0 )
            send_to_char( "\r\n", ch );
      }
      else
         send_to_char( "&WNone\r\n", ch );

      {
         char chan_new[MSL];
         char tmp_chan[MSL];
         int col = 0;

         chan_new[0] = '\0';
         for( c = first_channel; c; c = c->next )
         {
            if( !can_use_channel( ch, c ) )
               continue;

            /* Listening to this channel? */
            if( is_listening( ch, c ) )
               continue;

            snprintf( tmp_chan, sizeof( tmp_chan ), "&R%20.20s", c->name );
            mudstrlcat( chan_new, tmp_chan, sizeof( chan_new ) );
            if( ++col == 4 )
            {
               col = 0;
               mudstrlcat( chan_new, "\r\n", sizeof( chan_new ) );
            }
         }
         if( chan_new != NULL && chan_new[0] != '\0' )
         {
            if( col != 0 )
               mudstrlcat( chan_new, "\r\n", sizeof( chan_new ) );
            send_to_char( "&cCurrently not listening to:\r\n", ch );
            send_to_char( chan_new, ch );
         }
      }
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      char chan_new[MSL];

      chan_new[0] = '\0';
      for( c = first_channel; c; c = c->next )
      {
         if( !can_use_channel( ch, c ) )
            continue;

         if( chan_new != NULL && chan_new[0] != '\0' )
            mudstrlcat( chan_new, " ", sizeof( chan_new ) );
         mudstrlcat( chan_new, c->name, sizeof( chan_new ) );
      }
      if( chan_new == NULL || chan_new[0] == '\0' )
         send_to_char( "&YNo channels for you to listen to.\r\n", ch );
      else
      {
         STRSET( ch->pcdata->channels, chan_new );
         send_to_char( "&YYou're now listening to all available channels.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      STRFREE( ch->pcdata->channels );
      send_to_char( "&YYou no longer listen to any available channels.\r\n", ch );
      return;
   }

   while( argument && argument[0] != '\0' )
   {
      char *chan_buf, chan_tmp[MIL], chan_new[MSL];
      bool cremoved = false;

      argument = one_argument( argument, arg );

      if( !( c = get_channel( arg ) ) || !can_use_channel( ch, c ) )
      {
         ch_printf( ch, "No channel named %s.\r\n", arg );
         continue;;
      }

      chan_buf = ch->pcdata->channels;
      mudstrlcpy( chan_new, "", sizeof( chan_new ) );

      while( chan_buf && chan_buf[0] != '\0' )
      {
         chan_buf = one_argument( chan_buf, chan_tmp );
         if( !str_cmp( chan_tmp, c->name ) )
            cremoved = true;
         else
         {
            if( chan_new != NULL && chan_new[0] != '\0' )
               mudstrlcat( chan_new, " ", sizeof( chan_new ) );
            mudstrlcat( chan_new, chan_tmp, sizeof( chan_new ) );
         }
      }

      if( cremoved )
         ch_printf( ch, "You're no longer listening to %s.\r\n", c->name );
      else
      {
         if( chan_new != NULL && chan_new[0] != '\0' )
            mudstrlcat( chan_new, " ", sizeof( chan_new ) );
         mudstrlcat( chan_new, c->name, sizeof( chan_new ) );
         ch_printf( ch, "You're now listening to %s.\r\n", c->name );
      }

      STRSET( ch->pcdata->channels, chan_new );
   }
}
