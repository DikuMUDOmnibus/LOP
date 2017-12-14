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
 *                              Hint System                                  *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "h/mud.h"

#define HINT_FILE SYSTEM_DIR "hints.dat"
typedef struct hint_data HINT_DATA;
struct hint_data
{
   HINT_DATA *next, *prev;
   char *hint;
   /* Restrictions below */
   int minlevel, maxlevel; /* Levels */
   char *rname;            /* Race name */
   char *cname;            /* Class name */
   char *aname;            /* Area name */
};

HINT_DATA *first_hint, *last_hint;

void save_hints( void )
{
   HINT_DATA *hint;
   FILE *fp;

   if( !first_hint )
   {
      remove_file( HINT_FILE );
      return;
   }
   if( !( fp = fopen( HINT_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, HINT_FILE );
      perror( HINT_FILE );
      return;
   }

   for( hint = first_hint; hint; hint = hint->next )
   {
      if( !hint->hint )
         continue;
      fprintf( fp, "Hint      %s~\n", hint->hint );
      if( hint->aname )
         fprintf( fp, "AName     %s~\n", hint->aname );
      if( hint->cname )
         fprintf( fp, "CName     %s~\n", hint->cname );
      if( hint->rname )
         fprintf( fp, "RName     %s~\n", hint->rname );
      if( hint->minlevel )
         fprintf( fp, "MinLevel  %d\n", hint->minlevel );
      if( hint->maxlevel != MAX_LEVEL )
         fprintf( fp, "MaxLevel  %d\n", hint->maxlevel );
   }
   fprintf( fp, "%s", "End\n" );
   fclose( fp );
   fp = NULL;
}

void free_hint( HINT_DATA *hint )
{
   if( !hint )
      return;
   STRFREE( hint->hint );
   STRFREE( hint->aname );
   STRFREE( hint->rname );
   STRFREE( hint->cname );
   DISPOSE( hint );
}

void free_hints( void )
{
   HINT_DATA *hint, *hint_next;

   for( hint = first_hint; hint; hint = hint_next )
   {
      hint_next = hint->next;
      UNLINK( hint, first_hint, last_hint, next, prev );
      free_hint( hint );
   }
}

HINT_DATA *add_hint( char *newhint )
{
   HINT_DATA *hint;

   if( !newhint || newhint[0] == '\0' )
      return NULL;

   CREATE( hint, HINT_DATA, 1 );
   STRSET( hint->hint, newhint );
   hint->minlevel = 0;
   hint->maxlevel = MAX_LEVEL;
   hint->aname = NULL;
   hint->cname = NULL;
   hint->rname = NULL;
   LINK( hint, first_hint, last_hint, next, prev );
   return hint;
}

void load_hints( void )
{
   FILE *fp;
   HINT_DATA *hint = NULL;
   char *name;
   int value;
   bool fMatch;

   first_hint = last_hint = NULL;
   if( !( fp = fopen( HINT_FILE, "r" ) ) )
      return;
   for( ;; )
   {
      char *word;

      fMatch = false;
      if( feof( fp ) )
         break;
      word = fread_word( fp );
      if( word[0] == EOF )
         break;

      switch( UPPER( word[0] ) )
      {
         default:
            break;

         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "AName" ) )
            {
               name = fread_flagstring( fp );
               if( hint )
                  STRSET( hint->aname, name );
               fMatch = true;
               break;
            }   
            break;

         case 'C':
            if( !str_cmp( word, "CName" ) )
            {
               name = fread_flagstring( fp );
               value = get_pc_class( name );

               if( value < 0 || value >= MAX_PC_CLASS )
                  continue;
               if( hint )
                  STRSET( hint->cname, name );
               fMatch = true;
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               fclose( fp );
               fp = NULL;
               return;
            }
            break;

         case 'H':
            if( !str_cmp( word, "Hint" ) )
            {
               name = fread_flagstring( fp );
               hint = add_hint( name );
               fMatch = true;
               break;
            }
            break;

         case 'M':
            if( !str_cmp( word, "MinLevel" ) )
            {
               value = fread_number( fp );
               if( value < 0 || value > MAX_LEVEL )
                  value = 0;
               if( hint )
                  hint->minlevel = value;
               fMatch = true;
               break;
            }
            else if( !str_cmp( word, "MaxLevel" ) )
            {
               value = fread_number( fp );
               if( value < 0 || value > MAX_LEVEL )
                  value = MAX_LEVEL;
               if( hint )
                  hint->maxlevel = value;
               fMatch = true;
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "RName" ) )
            {
               name = fread_flagstring( fp );
               value = get_pc_race( name );

               if( value < 0 || value >= MAX_PC_RACE )
                  continue;
               if( hint )
                  STRSET( hint->rname, name );
               fMatch = true;
               break;
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
}

HINT_DATA *find_hint( int nhint )
{
   HINT_DATA *hint;
   int count = 0;

   for( hint = first_hint; hint; hint = hint->next )
      if( ++count == nhint )
         return hint;
   return NULL;
}

CMDF( do_hintset )
{
   HINT_DATA *hint;
   char arg[MSL];
   int count = 0, value = 0;

   set_char_color( AT_HINT, ch );
   if( !argument || argument[0] == '\0' )
   {
      if( !first_hint )
      {
         send_to_char( "No hints currently in the mud.\r\n", ch );
         return;
      }
      for( hint = first_hint; hint; hint = hint->next )
      {
         ch_printf( ch, "&[hint2]%3d&[hint]> &[hint2]%s\r\n", ++count, hint->hint );
         if( hint->aname || hint->cname || hint->rname || hint->minlevel || hint->maxlevel != MAX_LEVEL )
         {
            send_to_char( "&[hint]Restrictions:", ch );
            if( hint->aname )
               ch_printf( ch, " &[hint]Area: &[hint2]%s", hint->aname );
            if( hint->rname )
               ch_printf( ch, " &[hint]Race: &[hint2]%s", hint->rname );
            if( hint->cname )
               ch_printf( ch, " &[hint]Class: &[hint2]%s", hint->cname );
            ch_printf( ch, "  &[hint]Levels: &[hint2]%d &[hint]to &[hint2]%d\r\n", hint->minlevel, hint->maxlevel );
         }
      }
      return;
   }

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "create" ) )
   {
      if( !( hint = add_hint( argument ) ) )
      {
         send_to_char( "Hint couldn't be created.\r\n", ch );
         return;
      }
      save_hints( );
      send_to_char( "Hint created.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "delete" ) )
   {
      if( !( hint = find_hint( atoi( argument ) ) ) )
      {
         send_to_char( "No such hint to delete.\r\n", ch );
         return;
      }
      UNLINK( hint, first_hint, last_hint, next, prev );
      free_hint( hint );
      save_hints( );
      send_to_char( "Hint deleted.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "class" ) )
   {
      argument = one_argument( argument, arg );
      if( !( hint = find_hint( atoi( arg ) ) ) )
      {
         send_to_char( "No such hint to restrict to one class.\r\n", ch );
         return;
      }
      if( !str_cmp( argument, "clear" ) )
      {
         STRFREE( hint->cname );
         hint->cname = NULL;
         save_hints( );
         send_to_char( "Hint class cleared.\r\n", ch );
         return;
      }
      value = get_pc_class( argument );
      if( value < 0 || value >= MAX_PC_CLASS )
      {
         send_to_char( "Not a valid pc class.\r\n", ch );
         return;
      }
      STRSET( hint->cname, argument );
      save_hints( );
      send_to_char( "Hint class set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "race" ) )
   {
      argument = one_argument( argument, arg );
      if( !( hint = find_hint( atoi( arg ) ) ) )
      {
         send_to_char( "No such hint to restrict to one race.\r\n", ch );
         return;
      }
      if( !str_cmp( argument, "clear" ) )
      {
         STRFREE( hint->rname );
         hint->rname = NULL;
         save_hints( );
         send_to_char( "Hint race cleared.\r\n", ch );
         return;
      }
      value = get_pc_race( argument );
      if( value < 0 || value >= MAX_PC_RACE )
      {
         send_to_char( "Not a valid pc race.\r\n", ch );
         return;
      }
      STRSET( hint->rname, argument );
      save_hints( );
      send_to_char( "Hint race set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "area" ) )
   {
      argument = one_argument( argument, arg );
      if( !( hint = find_hint( atoi( arg ) ) ) )
      {
         send_to_char( "No such hint to restrict to one area.\r\n", ch );
         return;
      }
      if( !str_cmp( argument, "clear" ) )
      {
         STRFREE( hint->aname );
         hint->aname = NULL;
         save_hints( );
         send_to_char( "Hint area cleared.\r\n", ch );
         return;
      }
      STRSET( hint->aname, argument );
      save_hints( );
      send_to_char( "Hint area set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "minlevel" ) )
   {
      argument = one_argument( argument, arg );
      if( !( hint = find_hint( atoi( arg ) ) ) )
      {
         send_to_char( "No such hint to restrict to a minlevel.\r\n", ch );
         return;
      }
      value = atoi( argument );
      if( value < 0 || value > MAX_LEVEL )
      {
         send_to_char( "Invalid level.\r\n", ch );
         return;
      }
      hint->minlevel = value;
      save_hints( );
      send_to_char( "Hint minlevel set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "maxlevel" ) )
   {
      argument = one_argument( argument, arg );
      if( !( hint = find_hint( atoi( arg ) ) ) )
      {
         send_to_char( "No such hint to restrict to a maxlevel.\r\n", ch );
         return;
      }
      value = atoi( argument );
      if( value < 0 || value > MAX_LEVEL )
      {
         send_to_char( "Invalid level.\r\n", ch );
         return;
      }
      hint->maxlevel = value;
      save_hints( );
      send_to_char( "Hint maxlevel set.\r\n", ch );
      return;
   }

   send_to_char( "Usage: hintset create <hint>\r\n", ch );
   send_to_char( "Usage: hintset delete <hint #>\r\n", ch );
   send_to_char( "Usage: hintset class/race/area <hint #> <class/race/area name>/clear\r\n", ch );
   send_to_char( "Usage: hintset minlevel/maxlevel <hint #> <level>\r\n", ch );
}

/* Should we send the hint to the character? */
bool can_send_hint( CHAR_DATA *ch, HINT_DATA *hint )
{
   MCLASS_DATA *mclass;
   bool cshint;

   if( hint->cname )
   {
      /* Have to do this a bit different, check to see if they have a matching class if so can send hint, else can't send hint */
      cshint = false;
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( !str_cmp( dis_class_name( mclass->wclass ), hint->cname ) )
         {
            cshint = true;
            break;
         }
      }
      if( !cshint )
         return false;
   }
   if( hint->rname && str_cmp( dis_race_name( ch->race ), hint->rname ) )
      return false;
   if( hint->aname && ( !ch->in_room || !ch->in_room->area ||  str_cmp( ch->in_room->area->name, hint->aname ) ) )
      return false;
   if( ch->level < hint->minlevel || ch->level > hint->maxlevel )
      return false;
   return true;
}

void send_hint( CHAR_DATA *ch )
{
   HINT_DATA *hint;

   for( hint = first_hint; hint; hint = hint->next )
   {
      if( number_percent( ) > 10 )
         continue;
      if( !can_send_hint( ch, hint ) )
         continue;
      ch_printf( ch, "&[hint]HINT: &[hint2]%s\r\n", hint->hint );
      return;
   }
}
