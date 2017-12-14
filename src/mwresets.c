/*****************************************************************************
 *---------------------------------------------------------------------------*
 * LoP (C) 2006 - 2012 by: the LoP team.                                     *
 *---------------------------------------------------------------------------*
 *                              Mud Wide Resets                              *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "h/mud.h"

#define MWRESET_FILE SYSTEM_DIR "mwresets.dat"

typedef struct mwreset_data MWRESET_DATA;
struct mwreset_data
{
   MWRESET_DATA *next, *prev;
   short type;
   int vnum;
   int percent;
};

MWRESET_DATA *first_mwreset, *last_mwreset;

typedef enum
{
   MW_OBJECT, MW_MOBILE, MW_MAX
} mw_types;

const char *mw_type[] =
{
   "object", "mobile", "max"
};

void free_mwreset( MWRESET_DATA *reset )
{
   if( !reset )
      return;
   DISPOSE( reset );
}

void free_all_mwresets( void )
{
   MWRESET_DATA *reset, *reset_next;

   for( reset = first_mwreset; reset; reset = reset_next )
   {
      reset_next = reset->next;
      UNLINK( reset, first_mwreset, last_mwreset, next, prev );
      free_mwreset( reset );
   }
}

void save_mwresets( void )
{
   MWRESET_DATA *reset;
   FILE *fp;

   if( !( fp = fopen( MWRESET_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, MWRESET_FILE );
      perror( MWRESET_FILE );
      return;
   }
   for( reset = first_mwreset; reset; reset = reset->next )
   {
      fprintf( fp, "%s", "#MWRESET\n" );
      if( reset->type >= 0 && reset->type < MW_MAX )
         fprintf( fp, "Type     %s~", mw_type[reset->type] );
      fprintf( fp, "Vnum     %d\n", reset->vnum );
      fprintf( fp, "Percent  %d\n", reset->percent );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

void fread_mwreset( FILE *fp )
{
   const char *word;
   char *infoflags, flag[MIL];
   bool fMatch;
   MWRESET_DATA *reset;
   int value;

   CREATE( reset, MWRESET_DATA, 1 );
   reset->vnum = 0;
   reset->percent = 0;
   reset->type = 0;

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
               LINK( reset, first_mwreset, last_mwreset, next, prev );
               return;
            }
            break;

         case 'P':
            KEY( "Percent", reset->percent, fread_number( fp ) );
            break;

         case 'T':
            SKEY( "Type", reset->type, fp, mw_type, MW_MAX );
            break;

         case 'V':
            KEY( "Vnum", reset->vnum, fread_number( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_mwreset( reset );
}

MWRESET_DATA *find_mwreset( int check )
{
   MWRESET_DATA *reset;
   int count = 0;

   for( reset = first_mwreset; reset; reset = reset->next )
   {
      if( ++count == check )
         return reset;
   }
   return NULL;
}

CMDF( do_mwreset )
{
   MWRESET_DATA *reset;
   char arg[MIL];
   int count = 0, value;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: mwreset create <vnum> <percent(1-100)>\r\n", ch );
      send_to_char( "Usage: mwreset <#> delete\r\n", ch );
      send_to_char( "Usage: mwreset <#> vnum/percent/type <value>\r\n", ch );
      for( reset = first_mwreset; reset; reset = reset->next )
      {
         if( ++count == 1 )
         {
            send_to_char( "Mud Wide RESETs currently in the game:\r\n", ch );
            send_to_char( "--------------------------------------\r\n", ch );
         }
         ch_printf( ch, "%2d>  Type: %6s  Vnum: %d  Percent: %d\r\n", count,
            mw_type[reset->type], reset->vnum, reset->percent );
      }
      if( !count )
         send_to_char( "No mud wide resets yet.\r\n", ch );
      return;
   }
   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "create" ) )
   {
      int vnum, percent;

      argument = one_argument( argument, arg );
      vnum = atoi( arg );
      argument = one_argument( argument, arg );
      percent = atoi( arg );
      if( !vnum || !percent || percent > 100 )
      {
         send_to_char( "Usage: mwreset create <vnum> <percent(1-100)>\r\n", ch );
         return;
      }
      CREATE( reset, MWRESET_DATA, 1 );
      reset->vnum = vnum;
      reset->percent = percent;
      reset->type = MW_OBJECT;
      LINK( reset, first_mwreset, last_mwreset, next, prev );
      save_mwresets( );
      ch_printf( ch, "Reset created for object vnum %d with a percent of %d.\r\n", reset->vnum, reset->percent );
      return;
   }
   if( !( reset = find_mwreset( atoi( arg ) ) ) )
   {
      send_to_char( "No such reset to modify.\r\n", ch );
      return;
   }
   argument = one_argument( argument, arg );
   value = atoi( argument );
   
   if( !str_cmp( arg, "delete" ) )
   {
      UNLINK( reset, first_mwreset, last_mwreset, next, prev );
      free_mwreset( reset );
      save_mwresets( );
      send_to_char( "Reset deleted.\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "vnum" ) )
   {
      if( value <= 0 || value > MAX_VNUM )
      {
         send_to_char( "Vnum out of range.\r\n", ch );
         return;
      }
      reset->vnum = value;
      save_mwresets( );
      send_to_char( "Vnum set.\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "percent" ) )
   {
      if( value <= 0 || value > 100 )
      {
         send_to_char( "Percent out of range.\r\n", ch );
         return;
      }
      reset->percent = value;
      save_mwresets( );
      send_to_char( "Percent set.\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "type" ) )
   {
      if( !is_number( argument ) )
         value = get_flag( argument, mw_type, MW_MAX );

      if( value < 0 || value >= MW_MAX )
      {
         ch_printf( ch, "Type can be set from 0 to %d.\r\n", ( MW_MAX - 1 ) );
         for( value = 0; value < MW_MAX; value++ )
            ch_printf( ch, "%2d = %s\r\n", value, mw_type[value] );
         return;
      }
      reset->type = value;
      save_mwresets( );
      ch_printf( ch, "Type set to [%d]%s.\r\n", reset->type, mw_type[reset->type] );
      return;
   }
}

void handle_mwmobilereset( ROOM_INDEX_DATA *room )
{
   MWRESET_DATA *reset;
   MOB_INDEX_DATA *mindex;
   CHAR_DATA *mob;

   if( !room )
      return;

   for( reset = first_mwreset; reset; reset = reset->next )
   {
      if( reset->type != MW_MOBILE )
         continue;
      /* 75% of it skiping anyways */
      if( number_percent( ) > 25 )
         continue;
      /* Then take into consideration the percent it has */
      if( number_percent( ) > reset->percent )
         continue;
      if( !( mindex = get_mob_index( reset->vnum ) ) )
         continue;
      if( !( mob = create_mobile( mindex ) ) )
         continue;
      char_to_room( mob, room );
   }
}

void handle_mwreset( OBJ_DATA *corpse )
{
   MWRESET_DATA *reset;
   OBJ_INDEX_DATA *oindex;
   OBJ_DATA *obj;

   if( !corpse )
      return;
   for( reset = first_mwreset; reset; reset = reset->next )
   {
      if( reset->type != MW_OBJECT )
         continue;
      /* 50% of it skiping anyways */
      if( number_percent( ) > 50 )
         continue;
      /* Then take into consideration the percent it has */
      if( number_percent( ) > reset->percent )
         continue;
      if( !( oindex = get_obj_index( reset->vnum ) ) )
         continue;
      if( !( obj = create_object( oindex, 0 ) ) )
         continue;
      obj_to_obj( obj, corpse );
   }
}

void load_mwresets( void )
{
   FILE *fp;

   first_mwreset = last_mwreset = NULL;
   if( !( fp = fopen( MWRESET_FILE, "r" ) ) )
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
      if( !str_cmp( word, "MWRESET" ) )
      {
         fread_mwreset( fp );
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
