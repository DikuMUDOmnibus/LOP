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
 *                                 Storages                                  *
 *****************************************************************************/

#include <stdio.h>
#include <dirent.h>
#include "h/mud.h"

static OBJ_DATA *rgObjNest[MAX_NEST];

void load_storage_file( char *filename )
{
   ROOM_INDEX_DATA *room;
   OBJ_DATA *tobj, *tobj_next;
   FILE *fp;
   char buf[MIL];
   int iNest, vnum = atoi( filename );

   if( !( room = get_room_index( vnum ) ) )
   {
      bug( "%s: Room %d doesn't exist", __FUNCTION__, vnum );
      return;
   }

   snprintf( buf, sizeof( buf ), "%s%s", STORAGE_DIR, filename );
   if( !( fp = fopen( buf, "r" ) ) )
   {
      perror( buf );
      return;
   }

   log_printf( "Loading storage room %s", buf );
   rset_supermob( room );

   for( iNest = 0; iNest < MAX_NEST; iNest++ )
      rgObjNest[iNest] = NULL;
   for( ;; )
   {
      char *word;
      char letter;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      if( letter != '#' )
      {
         bug( "%s: # not found for %s.", __FUNCTION__, filename );
         break;
      }
      word = fread_word( fp );
      if( !str_cmp( word, "OBJECT" ) ) /* Objects  */
         fread_obj( supermob, NULL, fp, OS_CARRY );
      else if( !str_cmp( word, "END" ) )  /* Done     */
         break;
      else
      {
         bug( "%s: bad section (%s) for %s.", __FUNCTION__, word, filename );
         break;
      }
   }
   fclose( fp );
   fp = NULL;

   for( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
   {
      tobj_next = tobj->next_content;
      obj_from_char( tobj );
      obj_to_room( tobj, room );
   }
   release_supermob( );
}

void load_storages( void )
{
   DIR *dp;
   struct dirent *dentry;

   dp = opendir( STORAGE_DIR );
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
         load_storage_file( dentry->d_name );
      dentry = readdir( dp );
   }
   closedir( dp );
}

void save_storage( ROOM_INDEX_DATA *room )
{
   FILE *fp;
   char filename[MIL];

   if( !room )
      return;
   snprintf( filename, sizeof( filename ), "%s%d", STORAGE_DIR, room->vnum );
   if( !room->last_content )
   {
      remove_file( filename );
      return;
   }
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: cant open %s", __FUNCTION__, filename );
      perror( filename );
      return;
   }
   rset_supermob( room );
   fwrite_obj( supermob, room->last_content, fp, 0, OS_CARRY, false );
   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
   release_supermob( );
}
