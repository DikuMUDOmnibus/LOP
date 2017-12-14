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
#include <string.h>
#include <stdio.h>
#include "h/mud.h"

#define LOCKER_FILE SYSTEM_DIR "lockers.dat"

typedef struct lshare_data LSHARE_DATA;
struct lshare_data
{
   LSHARE_DATA *next, *prev;
   char *name;
};

typedef struct lockershare_data LOCKERSHARE_DATA;
struct lockershare_data
{
   LOCKERSHARE_DATA *next, *prev;
   LSHARE_DATA *first_lshare, *last_lshare;
   char *locker;
};

LOCKERSHARE_DATA *first_locker, *last_locker;

void free_lshare( LSHARE_DATA *lshare )
{
   if( !lshare )
      return;
   STRFREE( lshare->name );
   DISPOSE( lshare );
}

void free_all_lshare( LOCKERSHARE_DATA *locker )
{
   LSHARE_DATA *lshare, *lshare_next;

   if( !locker )
      return;
   for( lshare = locker->first_lshare; lshare; lshare = lshare_next )
   {
      lshare_next = lshare->next;
      UNLINK( lshare, locker->first_lshare, locker->last_lshare, next, prev );
      free_lshare( lshare );
   }
}

void free_lockershare( LOCKERSHARE_DATA *locker )
{
   if( !locker )
      return;
   STRFREE( locker->locker );
   free_all_lshare( locker );
   DISPOSE( locker );
}

void free_all_lockershare( void )
{
   LOCKERSHARE_DATA *locker, *locker_next;

   for( locker = first_locker; locker; locker = locker_next )
   {
      locker_next = locker->next;
      UNLINK( locker, first_locker, last_locker, next, prev );
      free_lockershare( locker );
   }
}

void save_lockers( void )
{
   LOCKERSHARE_DATA *locker;
   LSHARE_DATA *lshare;
   FILE *fp;

   if( !first_locker )
   {
      remove( LOCKER_FILE );
      return;
   }
   if( !( fp = fopen( LOCKER_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, LOCKER_FILE );
      perror( LOCKER_FILE );
      return;
   }
   for( locker = first_locker; locker; locker = locker->next )
   {
      fprintf( fp, "%s", "#LOCKER\n" );
      fprintf( fp, "Name      %s~\n", locker->locker );
      for( lshare = locker->first_lshare; lshare; lshare = lshare->next )
         if( lshare->name )
            fprintf( fp, "Shared    %s~\n", lshare->name );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

LOCKERSHARE_DATA *new_lockershare( void )
{
   LOCKERSHARE_DATA *locker = NULL;

   CREATE( locker, LOCKERSHARE_DATA, 1 );
   if( !locker )
   {
      bug( "%s: locker is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   locker->locker = NULL;
   locker->first_lshare = locker->last_lshare = NULL;
   return locker;
}

void fread_locker( FILE *fp )
{
   LOCKERSHARE_DATA *locker;
   const char *word;
   bool fMatch;

   locker = new_lockershare( );

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
               LINK( locker, first_locker, last_locker, next, prev );
               return;
	    }
	    break;

         case 'N':
            KEY( "Name", locker->locker, fread_string( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Shared" ) )
            {
               LSHARE_DATA *shared;
               char *tmpstr = fread_flagstring( fp );

               CREATE( shared, LSHARE_DATA, 1 );
               shared->name = STRALLOC( tmpstr );
               LINK( shared, locker->first_lshare, locker->last_lshare, next, prev );

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
   free_lockershare( locker );
}

/* Check all lockers and make sure everything is valid if not update and save */
void check_lockers( void )
{
   LOCKERSHARE_DATA *locker, *locker_next;
   LSHARE_DATA *lshare, *lshare_next;
   bool changed = false;

   for( locker = first_locker; locker; locker = locker_next )
   {
      locker_next = locker->next;

      /* First toss through the lshare data and free any that no longer exist */
      for( lshare = locker->first_lshare; lshare; lshare = lshare_next )
      {
         lshare_next = lshare->next;

         if( !valid_pfile( lshare->name ) )
         {
            UNLINK( lshare, locker->first_lshare, locker->last_lshare, next, prev );
            free_lshare( lshare );
            changed = true;
         }
      }

      /* Now if not being shared anymore remove it all from the list */
      if( !locker->first_lshare )
      {
         UNLINK( locker, first_locker, last_locker, next, prev );
         free_lockershare( locker );
         changed = true;
      }
   }

   if( changed )
      save_lockers( );
}

void load_lockershares( void )
{
   FILE *fp;

   first_locker = last_locker = NULL;
   if( !( fp = fopen( LOCKER_FILE, "r" ) ) )
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
      if( !str_cmp( word, "LOCKER" ) )
      {
         fread_locker( fp );
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
   check_lockers( );
}

/* Check to see if a locker is shared */
bool is_locker_shared( char *name )
{
   LOCKERSHARE_DATA *locker;

   if( !name || name[0] == '\0' )
      return false;
   for( locker = first_locker; locker; locker = locker->next )
   {
      if( !str_cmp( locker->locker, name ) )
         return true;
   }
   return false;
}

LOCKERSHARE_DATA *find_locker( char *name )
{
   LOCKERSHARE_DATA *locker;

   if( !name || name[0] == '\0' )
      return NULL;
   for( locker = first_locker; locker; locker = locker->next )
   {
      if( !str_cmp( locker->locker, name ) )
         return locker;
   }
   return NULL;
}

void rename_lockershare( char *name, char *newname )
{
   LOCKERSHARE_DATA *locker;

   if( !name || !newname || name[0] == '\0' || newname[0] == '\0' )
      return;
   if( !( locker = find_locker( name ) ) )
      return;
   STRSET( locker->locker, newname ); /* Set it to the newname */
   save_lockers( );
}

/* Can the ch access the locker? */
bool can_use_locker( CHAR_DATA *ch, char *name )
{
   LOCKERSHARE_DATA *locker;
   LSHARE_DATA *lshare;

   if( !ch || !name || name[0] == '\0' || is_npc( ch ) )
      return false;
   if( is_immortal( ch ) ) /* Immortals can access all lockers, only use for checking etc... */
      return true;
   if( !str_cmp( ch->name, name ) ) /* Allow them to access their own lockers */
      return true;
   if( !( locker = find_locker( name ) ) ) /* Not shared so can't access it */
      return false;
   for( lshare = locker->first_lshare; lshare; lshare = lshare->next )
   {
      if( !str_cmp( lshare->name, ch->name ) ) /* Is shared with them so allow them access */
         return true;
   }
   return false; /* Shared but not with them */
}

void fwrite_locker( CHAR_DATA *ch, OBJ_DATA *locker, char *uname )
{
   FILE *fp = NULL;
   char lockerfile[MIL], *name;

   if( !ch || is_npc( ch ) )
      return;

   if( !locker )
   {
      bug( "%s: NULL object. for character %s", __FUNCTION__, ch->name);
      send_to_char( "There was a problem trying to write the locker file!\r\n", ch );
      return;
   }

   name = capitalize( uname );
   snprintf( lockerfile, sizeof( lockerfile ), "%s%s", LOCKER_DIR, name );
   if( !( fp = fopen( lockerfile, "w" ) ) )
   {
      bug( "%s: Couldn't open %s for write", __FUNCTION__, lockerfile );
      send_to_char( "There was some problem in writing your locker file!\r\n", ch );
      return;
   }
   fwrite_obj( ch, locker, fp, 0, OS_LOCKER, false );
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
}

CMDF( do_locker )
{
   OBJ_DATA *locker, *locker_next;
   OBJ_INDEX_DATA *ilocker;
   LOCKERSHARE_DATA *slocker = NULL;
   LSHARE_DATA *lshare;
   FILE *fp = NULL;
   char buf[MIL], arg[MIL], *name = NULL;
   bool nlocker = false;

   if( !ch || is_npc( ch ) )
      return;

   argument = one_argument( argument, arg );

   /* Allow it to be shared */
   if( arg != NULL && arg[0] != '\0' && !str_cmp( arg, "share" ) )
   {
      char tmparg[MIL];

      one_argument( argument, tmparg ); /* Lets see if maybe there is someone named share lol and if it should go on if so let it */
      /* If no arg or arg isn't put, get or list */
      if( tmparg == NULL || tmparg[0] == '\0' || ( str_cmp( tmparg, "put" ) && str_cmp( tmparg, "get" ) && str_cmp( tmparg, "list" )  ) )
      {
         /* Show shared list */
         if( tmparg == NULL || tmparg[0] == '\0' )
         {
            if( !( slocker = find_locker( ch->name ) ) )
            {
               send_to_char( "You aren't currently sharing your locker with anyone.\r\n", ch );
               return;
            }
            send_to_char( "You are currently sharing your locker with:\r\n", ch );
            for( lshare = slocker->first_lshare; lshare; lshare = lshare->next )
               ch_printf( ch, "%s\r\n", lshare->name );
            return;
         }

         /* No slocker so create one */
         if( !( slocker = find_locker( ch->name ) ) )
         {
            slocker = new_lockershare( );
            slocker->locker = STRALLOC( ch->name );
            nlocker = true;
         }

         /* Check to see if it is already in the list */
         for( lshare = slocker->first_lshare; lshare; lshare = lshare->next )
         {
            if( !str_cmp( lshare->name, tmparg ) )
            {
               UNLINK( lshare, slocker->first_lshare, slocker->last_lshare, next, prev );
               free_lshare( lshare );
               if( !slocker->first_lshare ) /* If no more on shared list remove them all */
               {
                  UNLINK( slocker, first_locker, last_locker, next, prev );
                  free_lockershare( slocker );
               }
               ch_printf( ch, "Removed %s from your locker share list.\r\n", tmparg );
               return;
            }
         }

         if( !valid_pfile( tmparg ) || !can_use_path( NULL, LOCKER_DIR, tmparg ) )
         {
            send_to_char( "You can't share your locker with them currently.\r\n", ch );
            if( nlocker ) /* New lockers don't have any yet so free them up */
               free_lockershare( slocker );
            return;
         }

         if( !str_cmp( ch->name, tmparg ) )
         {
            send_to_char( "You already have access to your locker.\r\n", ch );
            if( nlocker ) /* New lockers don't have any yet so free them up */
               free_lockershare( slocker );
            return;
         }

         /* Ok so not on the list yet so lets add it */
         CREATE( lshare, LSHARE_DATA, 1 );
         lshare->name = STRALLOC( capitalize( tmparg ) );
         lshare->next = NULL;
         lshare->prev = NULL;
         LINK( lshare, slocker->first_lshare, slocker->last_lshare, next, prev );
         if( nlocker ) /* Need to add new lockers to the list */
            LINK( slocker, first_locker, last_locker, next, prev );
         save_lockers( );
         ch_printf( ch, "You are now sharing your locker with %s.\r\n", lshare->name );
         return;
      }
   }

   /* Lets see if we are specifying what locker to open */
   if( arg != NULL && arg[0] != '\0' && str_cmp( arg, "put" ) && str_cmp( arg, "get" ) && str_cmp( arg, "list" )
   && check_parse_name( capitalize( arg ), false ) )
   {
      name = capitalize( arg );
      argument = one_argument( argument, arg );
      if( !can_use_locker( ch, name ) )
      {
         send_to_char( "You don't have permission to access that locker.\r\n", ch );
         return;
      }
   }

   if( arg == NULL || arg[0] == '\0' || ( str_cmp( arg, "put" ) && str_cmp( arg, "get" ) && str_cmp( arg, "list" ) ) )
   {
      send_to_char( "Usage: locker put/get all\r\nUsage: locker put/get <object name>\r\nUsage: locker list\r\n", ch );
      return;
   }

   if( !xIS_SET( ch->in_room->room_flags, ROOM_LOCKER ) )
   {
      send_to_char( "You need to be in a locker room!\r\n", ch );
      return;
   }

   if( !name || name[0] == '\0' )
      name = capitalize( ch->name );

   snprintf( buf, sizeof( buf ), "%s%s", LOCKER_DIR, name );
   if( ( fp = fopen( buf, "r" ) ) != NULL )
   {
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '#' )
         {
            word = fread_word( fp );
            if( !strcmp( word, "LOCKER" ) )
               fread_obj( ch, NULL, fp, OS_LOCKER );
            else if( !strcmp( word, "OBJECT" ) )
               fread_obj( ch, NULL, fp, OS_CARRY );
            else if( !strcmp( word, "END" ) )
               break;
            else
            {
               bug( "%s: bad section (%s).", __FUNCTION__, word );
               break;
            }
         }
      }
      fclose( fp );
      fp = NULL;
   }
   else
   {
      if( !( ilocker = get_obj_index( OBJ_VNUM_LOCKER ) ) )
      {
         send_to_char( "There is no locker to use currently.\r\n", ch );
         bug( "%s: Can't find the index locker! Vnum %d.", __FUNCTION__, OBJ_VNUM_LOCKER );
         return;
      }
      if( !( locker = create_object( ilocker, 0 ) ) )
      {
         send_to_char( "There is no locker to use currently.\r\n", ch );
         bug( "%s: Failed to create a locker! Vnum %d.", __FUNCTION__, OBJ_VNUM_LOCKER );
         return;
      }
      snprintf( buf, sizeof( buf ), locker->name, name );
      STRSET( locker->name, buf );
      snprintf( buf, sizeof( buf ), locker->short_descr, name );
      STRSET( locker->short_descr, buf );
      snprintf( buf, sizeof( buf ), locker->description, name );
      STRSET( locker->description, buf );
      obj_to_room( locker, ch->in_room );
   }

   if( !str_cmp( arg, "put" ) )
   {
      interpret_printf( ch, "put %s %s", argument, name );
   }
   else if( !str_cmp( arg, "get" ) )
   {
      interpret_printf( ch, "get %s %s", argument, name );
   }
   else if( !str_cmp( arg, "list" ) )
   {
      interpret_printf( ch, "look in %s", name );
   }

   for( locker = ch->in_room->first_content; locker; locker = locker_next )
   {
      locker_next = locker->next_content;

      if( locker->pIndexData->vnum != OBJ_VNUM_LOCKER )
         continue;
      if( locker->first_content )
         fwrite_locker( ch, locker, locker->name );
      else
      {
         snprintf( buf, sizeof( buf ), "%s%s", LOCKER_DIR, locker->name );
         remove_file( buf );
      }
      extract_obj( locker );
   }
}

void rename_locker( CHAR_DATA *ch, char *newname )
{
   OBJ_DATA *locker;
   FILE *fp;
   char name[MSL], uname[MSL], buf[MSL], filename[MIL];

   snprintf( name, sizeof( name ), "%s", capitalize( ch->name ) );
   snprintf( uname, sizeof( uname ), "%s", capitalize( newname ) );

   snprintf( filename, sizeof( filename ), "%s%s", LOCKER_DIR, name );
   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '#' )
         {
            word = fread_word( fp );
            if( !strcmp( word, "END" ) )
               break;
            if( !strcmp( word, "OBJECT" ) )
               fread_obj( ch, NULL, fp, OS_LOCKER );
         }
      }
      fclose( fp );
      fp = NULL;
   }

   for( locker = ch->in_room->first_content; locker; locker = locker->next_content )
      if( locker->pIndexData->vnum == OBJ_VNUM_LOCKER && !str_cmp( locker->name, ch->name ) )
         break;

   if( locker )
   {
      if( locker->first_content )
      {
         snprintf( buf, sizeof( buf ), locker->pIndexData->name, uname );
         STRSET( locker->name, buf );
         snprintf( buf, sizeof( buf ), locker->pIndexData->short_descr, uname );
         STRSET( locker->short_descr, buf );
         snprintf( buf, sizeof( buf ), locker->pIndexData->description, uname );
         STRSET( locker->description, buf );
         fwrite_locker( ch, locker, uname );
      }
      else
      {
         snprintf( filename, sizeof( filename ), "%s%s", LOCKER_DIR, name );
         remove_file( filename );
      }
      extract_obj( locker );
   }
}
