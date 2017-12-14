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
 *                  Game Reset Handler and Editing Module                    *
 *                         Smaug FUSS 1.6 Version                            *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"

/*
 * Find some object with a given index data.
 * Used by area-reset 'P', 'T' and 'H' commands.
 */
OBJ_DATA *get_obj_type( OBJ_INDEX_DATA *pObjIndex )
{
   OBJ_DATA *obj;

   for( obj = first_object; obj; obj = obj->next )
   {
      if( obj->pIndexData == pObjIndex )
         return obj;
   }
   return NULL;
}

/* Find an object in a room so we can check it's dependents. Used by 'O' resets. */
OBJ_DATA *get_obj_room( OBJ_INDEX_DATA *pObjIndex, ROOM_INDEX_DATA *pRoomIndex )
{
   OBJ_DATA *obj;

   for( obj = pRoomIndex->first_content; obj; obj = obj->next_content )
   {
      if( obj->pIndexData == pObjIndex )
         return obj;
   }
   return NULL;
}

char *sprint_reset( RESET_DATA *pReset, short *num )
{
   RESET_DATA *tReset, *gReset;
   static ROOM_INDEX_DATA *room;
   static OBJ_INDEX_DATA *obj, *obj2;
   static MOB_INDEX_DATA *mob;
   static char buf[MSL];
   char mobname[MSL], roomname[MSL], objname[MSL];

   switch( pReset->command )
   {
      default:
         snprintf( buf, sizeof( buf ), "%2d) *** BAD RESET: %c %d %d %d %d %d***\r\n",
            *num, pReset->command, pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3, pReset->rchance );
         break;

      case 'E':
         if( pReset->main_reset )
         {
            mob = get_mob_index( pReset->main_reset->arg1 );
            if( mob )
               strncpy( mobname, mob->name, sizeof( mobname ) );
            else
               strncpy( mobname, "Mobile: *BAD VNUM*", sizeof( mobname ) );
         }
         else
            strncpy( mobname, "Mobile: *UNKNOWN*", sizeof( mobname ) );

         if( !( obj = get_obj_index( pReset->arg1 ) ) )
            strncpy( objname, "Object: *BAD VNUM*", sizeof( objname ) );
         else
            strncpy( objname, obj->name, sizeof( objname ) );
         snprintf( buf, sizeof( buf ),
            "%2d) (equip) %s (%d) -> %s (%s) [%d] (%d)%s\r\n", *num, objname, pReset->arg1, mobname, wear_locs[pReset->arg3], pReset->arg2, pReset->rchance, pReset->obj ? " [Reseted]" : "" );
         break;

      case 'P':
         if( !( obj2 = get_obj_index( pReset->arg1 ) ) )
            strncpy( objname, "Object1: *BAD VNUM*", sizeof( objname ) );
         else
            strncpy( objname, obj2->name, sizeof( objname ) );
         if( pReset->arg3 > 0 && !( obj = get_obj_index( pReset->arg3 ) ) )
            strncpy( roomname, "Object2: *BAD VNUM*", sizeof( roomname ) );
         else if( !obj )
            strncpy( roomname, "Object2: *NULL obj*", sizeof( roomname ) );
         else
            strncpy( roomname, obj->name, sizeof( roomname ) );
         snprintf( buf, sizeof( buf ),
            "%2d) (put) %s (%d) -> %s (%d) [%d] (%d)%s\r\n", *num, objname, pReset->arg1, roomname,
            obj ? obj->vnum : pReset->arg3, pReset->arg2, pReset->rchance, pReset->obj ? " [Reseted]" : "" );
         break;

      case 'G':
         if( pReset->main_reset )
         {
            mob = get_mob_index( pReset->main_reset->arg1 );
            if( mob )
               strncpy( mobname, mob->name, sizeof( mobname ) );
            else
               strncpy( mobname, "Mobile: *BAD VNUM*", sizeof( mobname ) );
         }
         else
            strncpy( mobname, "Mobile: *UNKNOWN*", sizeof( mobname ) );

         if( !( obj = get_obj_index( pReset->arg1 ) ) )
            strncpy( objname, "Object: *BAD VNUM*", sizeof( objname ) );
         else
            strncpy( objname, obj->name, sizeof( objname ) );
         snprintf( buf, sizeof( buf ), "%2d) (carry) %s (%d) -> %s [%d] (%d)%s\r\n",
            *num, objname, pReset->arg1, mobname, pReset->arg2, pReset->rchance, pReset->obj ? " [Reseted]" : "" );
         break;

      case 'M':
         mob = get_mob_index( pReset->arg1 );
         room = get_room_index( pReset->arg3 );
         if( mob )
            strncpy( mobname, mob->name, sizeof( mobname ) );
         else
            strncpy( mobname, "Mobile: *BAD VNUM*", sizeof( mobname ) );
         if( room )
            strncpy( roomname, room->name, sizeof( roomname ) );
         else
            strncpy( roomname, "Room: *BAD VNUM*", sizeof( roomname ) );
         snprintf( buf, sizeof( buf ), "%2d) %s (%d) -> %s Room: %d [%d] (%d) X:[%d] Y:[%d]%s\r\n", *num, mobname, pReset->arg1,
            roomname, pReset->arg3, pReset->arg2, pReset->rchance, pReset->cords[0], pReset->cords[1], pReset->ch ? " [Reseted] " : "" );

         for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
         {
            ( *num )++;
            switch( tReset->command )
            {
               case 'E':
                  if( !mob )
                     strncpy( mobname, "* ERROR: NO MOBILE! *", sizeof( mobname ) );
                  if( !( obj = get_obj_index( tReset->arg1 ) ) )
                     strncpy( objname, "Object: *BAD VNUM*", sizeof( objname ) );
                  else
                     strncpy( objname, obj->name, sizeof( objname ) );
                  snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
                     "%2d) (equip) %s (%d) -> %s (%s) [%d] (%d)%s\r\n", *num, objname, tReset->arg1, mobname,
                     wear_locs[tReset->arg3], tReset->arg2, tReset->rchance, tReset->obj ? " [Reseted]" : "" );
                  break;

               case 'G':
                  if( !mob )
                     strncpy( mobname, "* ERROR: NO MOBILE! *", sizeof( mobname ) );
                  if( !( obj = get_obj_index( tReset->arg1 ) ) )
                     strncpy( objname, "Object: *BAD VNUM*", sizeof( objname ) );
                  else
                     strncpy( objname, obj->name, sizeof( objname ) );
                  snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), "%2d) (carry) %s (%d) -> %s [%d] (%d)%s\r\n",
                     *num, objname, tReset->arg1, mobname, tReset->arg2, tReset->rchance, tReset->obj ? " [Reseted]" : "" );
                  break;
            }
            if( tReset->first_reset )
            {
               for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
               {
                  ( *num )++;
                  switch( gReset->command )
                  {
                     case 'P':
                        if( !( obj2 = get_obj_index( gReset->arg1 ) ) )
                           strncpy( objname, "Object1: *BAD VNUM*", sizeof( objname ) );
                        else
                           strncpy( objname, obj2->name, sizeof( objname ) );
                        if( gReset->arg3 > 0 && !( obj = get_obj_index( gReset->arg3 ) ) )
                           strncpy( roomname, "Object2: *BAD VNUM*", sizeof( roomname ) );
                        else if( !obj )
                           strncpy( roomname, "Object2: *NULL obj*", sizeof( roomname ) );
                        else
                           strncpy( roomname, obj->name, sizeof( roomname ) );
                        snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
                           "%2d) (put) %s (%d) -> %s (%d) [%d] (%d)%s\r\n", *num, objname, gReset->arg1, roomname,
                           obj ? obj->vnum : gReset->arg3, gReset->arg2, gReset->rchance, gReset->obj ? " [Reseted]" : "" );
                        break;
                  }
               }
            }
         }
         break;

      case 'O':
         if( !( obj = get_obj_index( pReset->arg1 ) ) )
            strncpy( objname, "Object: *BAD VNUM*", sizeof( objname ) );
         else
            strncpy( objname, obj->name, sizeof( objname ) );
         room = get_room_index( pReset->arg3 );
         if( !room )
            strncpy( roomname, "Room: *BAD VNUM*", sizeof( roomname ) );
         else
            strncpy( roomname, room->name, sizeof( roomname ) );
         snprintf( buf, sizeof( buf ), "%2d) (object) %s (%d) -> %s Room: %d [%d] (%d) X:[%d] Y:[%d]%s\r\n",
            *num, objname, pReset->arg1, roomname, pReset->arg3, pReset->arg2, pReset->rchance, pReset->cords[0], pReset->cords[1], pReset->obj ? " [Reseted]" : "" );

         for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
         {
            ( *num )++;
            switch( tReset->command )
            {
               case 'P':
                  if( !( obj2 = get_obj_index( tReset->arg1 ) ) )
                     strncpy( objname, "Object1: *BAD VNUM*", sizeof( objname ) );
                  else
                     strncpy( objname, obj2->name, sizeof( objname ) );
                  if( tReset->arg3 > 0 && !( obj = get_obj_index( tReset->arg3 ) ) )
                     strncpy( roomname, "Object2: *BAD VNUM*", sizeof( roomname ) );
                  else if( !obj )
                     strncpy( roomname, "Object2: *NULL obj*", sizeof( roomname ) );
                  else
                     strncpy( roomname, obj->name, sizeof( roomname ) );
                  snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), "%2d) (put) %s (%d) -> %s (%d) [%d] (%d)%s\r\n",
                     *num, objname, tReset->arg1, roomname, obj ? obj->vnum : tReset->arg3, tReset->arg2, tReset->rchance, tReset->obj ? " [Reseted]" : "" );
                  break;

               case 'T':
                  snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
                     "%2d) (trap) %d %d %d %d (%s) -> %s (%d) (%d)\r\n", *num, tReset->extra, tReset->arg1, tReset->arg2,
                     tReset->arg3, flag_string( tReset->extra, trap_flags ), objname, obj ? obj->vnum : 0, tReset->rchance );
                  break;

               case 'H':
                  snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), "%2d) (hide) -> %s (%d)\r\n", *num, objname, tReset->rchance );
                  break;
            }
         }
         break;

      case 'D':
         break;

      case 'R':
         if( !( room = get_room_index( pReset->arg1 ) ) )
            strncpy( roomname, "Room: *BAD VNUM*", sizeof( roomname ) );
         else
            strncpy( roomname, room->name, sizeof( roomname ) );
         snprintf( buf, sizeof( buf ), "%2d) Randomize exits 0 to %d -> %s (%d) (%d)\r\n", *num, pReset->arg2, roomname,
            pReset->arg1, pReset->rchance );
         break;

      case 'T':
         if( !( room = get_room_index( pReset->arg3 ) ) )
            strncpy( roomname, "Room: *BAD VNUM*", sizeof( roomname ) );
         else
            strncpy( roomname, room->name, sizeof( roomname ) );
         snprintf( buf, sizeof( buf ), "%2d) Trap: %d %d %d %d (%s) -> %s (%d) (%d)\r\n",
            *num, pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3, flag_string( pReset->extra, trap_flags ),
            roomname, room ? room->vnum : 0, pReset->rchance );
         break;
   }
   return buf;
}

/* Create a new reset (for online building) - Thoric */
RESET_DATA *make_reset( char letter, int extra, int arg1, int arg2, int arg3, short rchance, short xcord, short ycord, bool wilderness )
{
   RESET_DATA *pReset;

   CREATE( pReset, RESET_DATA, 1 );
   pReset->command = letter;
   pReset->extra = extra;
   pReset->arg1 = arg1;
   pReset->arg2 = arg2;
   pReset->arg3 = arg3;
   pReset->rchance = rchance;
   pReset->ch = NULL;
   pReset->obj = NULL;
   pReset->cords[0] = xcord;
   pReset->cords[1] = ycord;
   pReset->wilderness = wilderness;
   top_reset++;
   return pReset;
}

void add_obj_reset( ROOM_INDEX_DATA *room, char cm, OBJ_DATA *obj, int v2, int v3 )
{
   RESET_DATA *reset;
   OBJ_DATA *inobj;
   static int iNest;

   if( xIS_SET( obj->extra_flags, ITEM_PTRAP ) ) /* Don't add player trap objects */
      return;

   if( ( cm == 'O' || cm == 'P' ) && obj->pIndexData->vnum == OBJ_VNUM_TRAP )
   {
      if( cm == 'O' )
      {
         reset = add_reset( room, 'T', obj->value[3], obj->value[1], obj->value[0], v3, 100, 0, 0, false );
         if( reset )
         {
            reset->obj = obj;
            obj->reset = reset;
         }
      }
      return;
   }
   if( is_obj_stat( obj, ITEM_WILDERNESS ) )
      reset = add_reset( room, cm, ( cm == 'P' ? iNest : 0 ), obj->pIndexData->vnum, v2, v3, 100, obj->cords[0], obj->cords[1], true );
   else
      reset = add_reset( room, cm, ( cm == 'P' ? iNest : 0 ), obj->pIndexData->vnum, v2, v3, 100, 0, 0, false );
   if( reset )
   {
      reset->obj = obj;
      obj->reset = reset;
   }
   if( cm == 'O' && is_obj_stat( obj, ITEM_HIDDEN ) && can_wear( obj, ITEM_NO_TAKE ) )
   {
      reset = add_reset( room, 'H', 1, 0, 0, 0, 100, 0, 0, false );
      if( reset )
      {
         reset->obj = obj;
         obj->reset = reset;
      }
   }
   for( inobj = obj->first_content; inobj; inobj = inobj->next_content )
   {
      if( inobj->pIndexData->vnum == OBJ_VNUM_TRAP )
         add_obj_reset( room, 'O', inobj, 0, 0 );
   }
   if( cm == 'P' )
      iNest++;
   for( inobj = obj->first_content; inobj; inobj = inobj->next_content )
      add_obj_reset( room, 'P', inobj, inobj->count, obj->pIndexData->vnum );
   if( cm == 'P' )
      iNest--;
}

void remove_track_reset( RESET_DATA *reset )
{
   OBJ_INDEX_DATA *oindex;
   MOB_INDEX_DATA *mindex;
   RESET_TRACK_DATA *rtrack, *rtrack_next;

   switch( UPPER( reset->command ) )
   {
      case 'M':
         if( ( mindex = get_mob_index( reset->arg1 ) ) )
         {
            for( rtrack = mindex->first_track; rtrack; rtrack = rtrack_next )
            {
               rtrack_next = rtrack->next;

               if( rtrack->reset != reset )
                  continue;
               UNLINK( rtrack, mindex->first_track, mindex->last_track, next, prev );
               DISPOSE( rtrack );
            }
         }
         break;

      case 'E':
      case 'G':
         if( ( oindex = get_obj_index( reset->arg1 ) ) )
         {
            for( rtrack = oindex->first_track; rtrack; rtrack = rtrack_next )
            {
               rtrack_next = rtrack->next;

               if( rtrack->reset != reset )
                  continue;
               UNLINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
               DISPOSE( rtrack );
            }
         }
         break;

      case 'P':
         if( ( oindex = get_obj_index( reset->arg1 ) ) )
         {
            for( rtrack = oindex->first_track; rtrack; rtrack = rtrack_next )
            {
               rtrack_next = rtrack->next;

               if( rtrack->reset != reset )
                  continue;
               UNLINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
               DISPOSE( rtrack );
            }
         }
         if( ( oindex = get_obj_index( reset->arg3 ) ) )
         {
            for( rtrack = oindex->first_track; rtrack; rtrack = rtrack_next )
            {
               rtrack_next = rtrack->next;

               if( rtrack->reset != reset )
                  continue;
               UNLINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
               DISPOSE( rtrack );
            }
         }
         break;

      case 'O':
         if( ( oindex = get_obj_index( reset->arg1 ) ) )
         {
            for( rtrack = oindex->first_track; rtrack; rtrack = rtrack_next )
            {
               rtrack_next = rtrack->next;

               if( rtrack->reset != reset )
                  continue;
               UNLINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
               DISPOSE( rtrack );
            }
         }
         break;
   }
}

void delete_reset( RESET_DATA *pReset )
{
   RESET_DATA *tReset, *tReset_next;

   for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
   {
      tReset_next = tReset->next_reset;

      UNLINK( tReset, pReset->first_reset, pReset->last_reset, next_reset, prev_reset );
      delete_reset( tReset );
   }
   if( pReset->ch )
      pReset->ch->reset = NULL;
   pReset->ch = NULL;
   if( pReset->obj )
      pReset->obj->reset = NULL;
   pReset->main_reset = NULL;
   pReset->obj = NULL;
   pReset->first_reset = pReset->last_reset = NULL;
   remove_track_reset( pReset );
   DISPOSE( pReset );
}

void instaroom( ROOM_INDEX_DATA *pRoom )
{
   CHAR_DATA *rch;
   OBJ_DATA *obj;
   RESET_DATA *uReset;

   for( rch = pRoom->first_person; rch; rch = rch->next_in_room )
   {
      if( !is_npc( rch ) || IS_AFFECTED( rch, AFF_CHARM ) ) /* Can be annoying if you have a pet following you around and doing instaroom/instazone */
         continue;

      if( is_in_wilderness( rch ) )
         uReset = add_reset( pRoom, 'M', 1, rch->pIndexData->vnum, 1, pRoom->vnum, 100, rch->cords[0], rch->cords[1], true );
      else
         uReset = add_reset( pRoom, 'M', 1, rch->pIndexData->vnum, 1, pRoom->vnum, 100, 0, 0, false );
      uReset->obj = NULL;
      if( rch->reset ) /* Set the old ch reset ch to NULL so it can reset later */
         rch->reset->ch = NULL;
      uReset->ch = rch;
      rch->reset = uReset;

      for( obj = rch->first_carrying; obj; obj = obj->next_content )
      {
         if( obj->wear_loc == WEAR_NONE )
            add_obj_reset( pRoom, 'G', obj, 1, 0 );
         else
            add_obj_reset( pRoom, 'E', obj, 1, obj->wear_loc );
      }
   }
   for( obj = pRoom->first_content; obj; obj = obj->next_content )
      add_obj_reset( pRoom, 'O', obj, obj->count, pRoom->vnum );
}

void wipe_resets( ROOM_INDEX_DATA *room )
{
   RESET_DATA *pReset, *pReset_next;

   for( pReset = room->first_reset; pReset; pReset = pReset_next )
   {
      pReset_next = pReset->next;

      UNLINK( pReset, room->first_reset, room->last_reset, next, prev );
      delete_reset( pReset );
   }
   room->first_reset = room->last_reset = NULL;
}

void wipe_area_resets( AREA_DATA *area )
{
   ROOM_INDEX_DATA *room;

   if( !mud_down )
   {
      for( room = area->first_room; room; room = room->next_aroom )
         wipe_resets( room );
   }
}

/* Function modified from original form - Samson */
CMDF( do_instaroom )
{
   if( is_npc( ch ) || get_trust( ch ) < PERM_BUILDER || !ch->pcdata->area )
   {
      send_to_char( "You don't have an assigned area to create resets for.\r\n", ch );
      return;
   }

   if( !can_rmodify( ch, ch->in_room ) )
      return;
   if( ch->in_room->area != ch->pcdata->area && get_trust( ch ) < PERM_HEAD )
   {
      send_to_char( "You can't reset this room.\r\n", ch );
      return;
   }
   if( ch->in_room->first_reset )
      wipe_resets( ch->in_room );
   instaroom( ch->in_room );
   send_to_char( "Room resets installed.\r\n", ch );
}

/* Function modified from original form - Samson */
CMDF( do_instazone )
{
   AREA_DATA *pArea;
   ROOM_INDEX_DATA *pRoom;

   if( is_npc( ch ) || get_trust( ch ) < PERM_BUILDER || !ch->pcdata->area )
   {
      send_to_char( "You don't have an assigned area to create resets for.\r\n", ch );
      return;
   }

   pArea = ch->pcdata->area;
   wipe_area_resets( pArea );
   for( pRoom = pArea->first_room; pRoom; pRoom = pRoom->next_aroom )
      instaroom( pRoom );
   send_to_char( "Area resets installed.\r\n", ch );
}

/* Count occurrences of an obj in a list. */
int count_obj_list( OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list )
{
   OBJ_DATA *obj;
   int nMatch = 0;

   for( obj = list; obj; obj = obj->next_content )
   {
      if( obj->pIndexData == pObjIndex )
      {
         if( obj->count > 1 )
            nMatch += obj->count;
         else
            nMatch++;
      }
   }
   return nMatch;
}

/* Reset all the doors for a room */
void reset_doors( ROOM_INDEX_DATA *room )
{
   EXIT_DATA *pexit;

   if( !room || !room->first_exit )
      return;
   for( pexit = room->first_exit; pexit; pexit = pexit->next )
      pexit->exit_info = pexit->base_info; /* Set all back to the default */
}

/* Reset one room. */
void reset_room( ROOM_INDEX_DATA *room )
{
   RESET_DATA *pReset, *tReset, *gReset;
   OBJ_DATA *nestmap[MAX_NEST];
   CHAR_DATA *mob = NULL;
   OBJ_DATA *obj = NULL, *lastobj = NULL, *to_obj;
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   MOB_INDEX_DATA *pMobIndex = NULL;
   OBJ_INDEX_DATA *pObjIndex = NULL, *pObjToIndex;
   char *filename = room->area->filename;
   int level = 0, n, num = 0, lastnest;

   if( !fBootDb )
      handle_mwmobilereset( room );

   reset_doors( room ); /* Reset all doors for the room */

   if( !room->first_reset )
      return;
   level = 0;
   for( pReset = room->first_reset; pReset; pReset = pReset->next )
   {
      if( pReset->rchance < number_percent( ) )
         continue;

      switch( pReset->command )
      {
         default:
            bug( "%s: %s: bad command %c.", __FUNCTION__, filename, pReset->command );
            break;

         case 'M':
            if( !( pMobIndex = get_mob_index( pReset->arg1 ) ) )
            {
               bug( "%s: %s: 'M': bad mob vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               continue;
            }
            if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
               bug( "%s: %s: 'M': bad room vnum %d.", __FUNCTION__, filename, pReset->arg3 );
               continue;
            }

            if( pReset->ch )
               mob = pReset->ch;
            else if( !( mob = create_mobile( pMobIndex ) ) )
            {
               bug( "%s: failed to create_mobile for mob vnum %d.", __FUNCTION__, pMobIndex->vnum );
               continue;
            }

            /* See if this is a pet room */
            {
               ROOM_INDEX_DATA *pRoomPrev = get_room_index( pRoomIndex->vnum - 1 );

               if( pRoomPrev && xIS_SET( pRoomPrev->room_flags, ROOM_PET_SHOP ) )
                  xSET_BIT( mob->act, ACT_PET );
            }

            mob->reset = pReset;
            pReset->ch = mob;
            if( room_is_dark( pRoomIndex ) )
               xSET_BIT( mob->affected_by, AFF_INFRARED );

            if( !mob->in_room )
            {
               char_to_room( mob, pRoomIndex );
               mob->cords[0] = pReset->cords[0];
               mob->cords[1] = pReset->cords[1];
               if( pReset->wilderness )
                  xSET_BIT( mob->act, ACT_WILDERNESS );
            }

            if( pReset->first_reset )
            {
               for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
               {
                  if( tReset->rchance < number_percent( ) )
                     continue;

                  switch( tReset->command )
                  {
                     case 'G':
                     case 'E':
                        if( !mob )
                        {
                           lastobj = NULL;
                           break;
                        }

                        if( !( pObjIndex = get_obj_index( tReset->arg1 ) ) )
                        {
                           bug( "%s: %s: 'E' or 'G': bad obj vnum %d.", __FUNCTION__, filename, tReset->arg1 );
                           continue;
                        }

                        if( tReset->obj )
                           obj = tReset->obj;
                        else
                        {
                           if( mob->pIndexData->pShop )
                           {
                              obj = create_object( pObjIndex, pObjIndex->level );
                              xSET_BIT( obj->extra_flags, ITEM_INVENTORY );
                           }
                           else
                              obj = create_object( pObjIndex, pObjIndex->level );
                        }

                        if( !obj )
                        {
                           bug( "%s: failed to create_object for obj vnum %d", __FUNCTION__, pObjIndex->vnum );
                           continue;
                        }
                        obj->level = URANGE( 0, obj->level, MAX_LEVEL );
                        obj->reset = tReset;
                        tReset->obj = obj;
                        if( !obj->carried_by )
                        {
                           xSET_BIT( obj->extra_flags, ITEM_NOGROUP );
                           obj = obj_to_char( obj, mob );
                           if( obj )
                           {
                              obj->reset = tReset;
                              tReset->obj = obj;
                           }
                           if( tReset->command == 'E' )
                           {
                              if( obj->carried_by != mob )
                              {
                                 bug( "'E' reset: can't give object %d to mob %d.", obj->pIndexData->vnum, mob->pIndexData->vnum );
                                 continue;
                              }
                              equip_char( mob, obj, tReset->arg3 );
                           }
                           xREMOVE_BIT( obj->extra_flags, ITEM_NOGROUP );
                        }
                        for( n = 0; n < MAX_NEST; n++ )
                           nestmap[n] = NULL;
                        nestmap[0] = obj;
                        lastobj = nestmap[0];
                        lastnest = 0;

                        if( tReset->first_reset )
                        {
                           for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
                           {
                              int iNest;
                              to_obj = lastobj;

                              if( gReset->rchance < number_percent( ) )
                                 continue;

                              switch( gReset->command )
                              {
                                 case 'H':
                                    if( !lastobj )
                                       break;
                                    xSET_BIT( lastobj->extra_flags, ITEM_HIDDEN );
                                    break;

                                 case 'P':
                                    if( !( pObjIndex = get_obj_index( gReset->arg1 ) ) )
                                    {
                                       bug( "%s: %s: 'P': bad obj vnum %d.", __FUNCTION__, filename, gReset->arg1 );
                                       continue;
                                    }
                                    iNest = gReset->extra;

                                    if( !( pObjToIndex = get_obj_index( gReset->arg3 ) ) )
                                    {
                                       bug( "%s: %s: 'P': bad objto vnum %d.", __FUNCTION__, filename, gReset->arg3 );
                                       continue;
                                    }
                                    if( iNest >= MAX_NEST )
                                    {
                                       bug( "%s: %s: 'P': Exceeded nesting limit of %d", __FUNCTION__, filename, MAX_NEST );
                                       obj = NULL;
                                       break;
                                    }

                                    if( count_obj_list( pObjIndex, to_obj->first_content ) > 0 )
                                    {
                                       obj = NULL;
                                       continue;
                                    }

                                    if( gReset->obj )
                                       obj = gReset->obj;
                                    else
                                    {
                                       if( iNest < lastnest )
                                          to_obj = nestmap[iNest];
                                       else if( iNest == lastnest )
                                          to_obj = nestmap[lastnest];
                                       else
                                          to_obj = lastobj;
                                       if( !( obj = create_object( pObjIndex, pObjIndex->level ) ) )
                                       {
                                          bug( "%s: failed to create_object for obj vnum %d", __FUNCTION__, pObjIndex->vnum );
                                          continue;
                                       }

                                       if( num > 1 )
                                          pObjIndex->count += ( num - 1 );
                                       xSET_BIT( obj->extra_flags, ITEM_NOGROUP );
                                       obj_to_obj( obj, to_obj );
                                       xREMOVE_BIT( obj->extra_flags, ITEM_NOGROUP );
                                       if( iNest > lastnest )
                                       {
                                          nestmap[iNest] = to_obj;
                                          lastnest = iNest;
                                       }
                                    }
                                    obj->level = UMIN( obj->level, MAX_LEVEL );
                                    obj->count = gReset->arg2;
                                    obj->reset = gReset;
                                    gReset->obj = obj;
                                    lastobj = obj;
                                    break;
                              }
                           }
                        }
                        break;
                  }
               }
            }
            break;

         case 'O':
            if( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
               bug( "%s: %s: 'O': bad obj vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               continue;
            }
            if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
               bug( "%s: %s: 'O': bad room vnum %d.", __FUNCTION__, filename, pReset->arg3 );
               continue;
            }

            if( pReset->obj )
               obj = pReset->obj;
            else
            {
               if( count_obj_list( pObjIndex, pRoomIndex->first_content ) < 1 )
               {
                  if( !( obj = create_object( pObjIndex, pObjIndex->level ) ) )
                  {
                     bug( "%s: failed to create_object for obj vnum %d", __FUNCTION__, pObjIndex->vnum );
                     continue;
                  }
                  if( num > 1 )
                     pObjIndex->count += ( num - 1 );
                  obj->count = pReset->arg2;
                  obj->level = UMIN( obj->level, MAX_LEVEL );
                  obj->cost = 0;
                  xSET_BIT( obj->extra_flags, ITEM_NOGROUP );
                  obj_to_room( obj, pRoomIndex );
                  xREMOVE_BIT( obj->extra_flags, ITEM_NOGROUP );
                  obj->cords[0] = pReset->cords[0];
                  obj->cords[1] = pReset->cords[1];
                  if( pReset->wilderness )
                     xSET_BIT( obj->extra_flags, ITEM_WILDERNESS );
               }
               else
               {
                  int x;

                  if( !( obj = get_obj_room( pObjIndex, pRoomIndex ) ) )
                  {
                     obj = NULL;
                     lastobj = NULL;
                     break;
                  }
                  obj->extra_flags = pObjIndex->extra_flags;
                  for( x = 0; x < 6; ++x )
                     obj->value[x] = pObjIndex->value[x];
               }
            }
            for( n = 0; n < MAX_NEST; n++ )
               nestmap[n] = NULL;
            nestmap[0] = obj;
            lastobj = nestmap[0];
            lastnest = 0;
            obj->reset = pReset;
            pReset->obj = obj;
            if( pReset->first_reset )
            {
               for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
               {
                  int iNest;

                  to_obj = lastobj;

                  if( tReset->rchance < number_percent( ) )
                     continue;

                  switch( tReset->command )
                  {
                     case 'H':
                        if( !lastobj )
                           break;
                        xSET_BIT( lastobj->extra_flags, ITEM_HIDDEN );
                        break;

                     case 'T':
                        if( !IS_SET( tReset->extra, TRAP_OBJ ) )
                        {
                           bug( "%s: Room reset found on object reset list", __FUNCTION__ );
                           break;
                        }
                        else
                        {
                           /*
                            * We need to preserve obj for future 'T' checks 
                            */
                           OBJ_DATA *pobj;

                           if( tReset->arg3 > 0 )
                           {
                              if( !( pObjToIndex = get_obj_index( tReset->arg3 ) ) )
                              {
                                 bug( "%s: %s: 'T': bad objto vnum %d.", __FUNCTION__, filename, tReset->arg3 );
                                 continue;
                              }
                              if( room->area->nplayer > 0 || !( to_obj = get_obj_type( pObjToIndex ) ) ||
                                  ( to_obj->carried_by && !is_npc( to_obj->carried_by ) ) || is_trapped( to_obj ) )
                                 break;
                           }
                           else
                           {
                              if( !lastobj || !obj )
                                 break;
                              to_obj = obj;
                           }
                           pobj = make_trap( tReset->arg2, tReset->arg1, to_obj->level, tReset->extra );
                           xSET_BIT( pobj->extra_flags, ITEM_NOGROUP );
                           obj_to_obj( pobj, to_obj );
                           xREMOVE_BIT( pobj->extra_flags, ITEM_NOGROUP );
                        }
                        break;

                     case 'P':
                        if( !( pObjIndex = get_obj_index( tReset->arg1 ) ) )
                        {
                           bug( "%s: %s: 'P': bad obj vnum %d.", __FUNCTION__, filename, tReset->arg1 );
                           continue;
                        }
                        iNest = tReset->extra;

                        if( !( pObjToIndex = get_obj_index( tReset->arg3 ) ) )
                        {
                           bug( "%s: %s: 'P': bad objto vnum %d.", __FUNCTION__, filename, tReset->arg3 );
                           continue;
                        }

                        if( iNest >= MAX_NEST )
                        {
                           bug( "%s: %s: 'P': Exceeded nesting limit of %d. Room %d.", __FUNCTION__, filename, MAX_NEST, room->vnum );
                           obj = NULL;
                           break;
                        }

                        if( count_obj_list( pObjIndex, to_obj->first_content ) > 0 )
                        {
                           obj = NULL;
                           continue;
                        }
   
                        if( tReset->obj )
                           obj = tReset->obj;
                        else
                        {
                           if( iNest < lastnest )
                              to_obj = nestmap[iNest];
                           else if( iNest == lastnest )
                              to_obj = nestmap[lastnest];
                           else
                              to_obj = lastobj;
                           obj = create_object( pObjIndex, pObjIndex->level );
                           if( num > 1 )
                              pObjIndex->count += ( num - 1 );
                           xSET_BIT( obj->extra_flags, ITEM_NOGROUP );
                           obj_to_obj( obj, to_obj );
                           xREMOVE_BIT( obj->extra_flags, ITEM_NOGROUP );
                           if( iNest > lastnest )
                           {
                              nestmap[iNest] = to_obj;
                              lastnest = iNest;
                           }
                        }
                        obj->level = UMIN( obj->level, MAX_LEVEL );
                        obj->count = tReset->arg2;
                        obj->reset = tReset;
                        tReset->obj = obj;
                        lastobj = obj;
                        break;
                  }
               }
            }
            break;

         case 'T':
            if( IS_SET( pReset->extra, TRAP_OBJ ) )
            {
               bug( "%s: Object trap found in room %d reset list", __FUNCTION__, room->vnum );
               break;
            }
            else
            {
               OBJ_INDEX_DATA *itrap;

               if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
               {
                  bug( "%s: %s: 'T': bad room %d.", __FUNCTION__, filename, pReset->arg3 );
                  continue;
               }
               if( !( itrap = get_obj_index( OBJ_VNUM_TRAP ) ) )
               {
                  bug( "%s: %s: 'T': no object found using vnum %d in room %d.", __FUNCTION__, filename, OBJ_VNUM_TRAP, pReset->arg3 );
               }
               if( room->area->nplayer <= 0 || count_obj_list( get_obj_index( OBJ_VNUM_TRAP ), pRoomIndex->first_content ) > 0 )
                  break;
               if( !( to_obj = make_trap( pReset->arg1, pReset->arg1, 10, pReset->extra ) ) )
               {
                  bug( "%s: %s: 'T': failed to make_trap in room %d.", __FUNCTION__, filename, pReset->arg3 );
                  continue;
               }
               xSET_BIT( to_obj->extra_flags, ITEM_NOGROUP );
               obj_to_room( to_obj, pRoomIndex );
               xSET_BIT( to_obj->extra_flags, ITEM_NOGROUP );
            }
            break;

         /* Use to handle doors so just kept around for now */
         case 'D':
            break;

         case 'R':
            if( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
               bug( "%s: %s: 'R': bad room vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               continue;
            }
            randomize_exits( pRoomIndex, pReset->arg2 );
            break;
      }
   }
}

void reset_area( AREA_DATA *area )
{
   ROOM_INDEX_DATA *room;

   if( !area->first_room )
      return;

   for( room = area->first_room; room; room = room->next_aroom )
      reset_room( room );
}

/* Setup put nesting levels, regardless of whether or not the resets will
   actually reset, or if they're bugged. */
void renumber_put_resets( ROOM_INDEX_DATA *room )
{
   RESET_DATA *pReset, *tReset, *lastobj = NULL;

   for( pReset = room->first_reset; pReset; pReset = pReset->next )
   {
      switch( pReset->command )
      {
         default:
            break;

         case 'O':
            lastobj = pReset;
            for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
            {
               switch( tReset->command )
               {
                  case 'P':
                     if( tReset->arg3 == 0 )
                     {
                        if( !lastobj )
                           tReset->extra = 1000000;
                        else if( lastobj->command != 'P' || lastobj->arg3 > 0 )
                           tReset->extra = 0;
                        else
                           tReset->extra = lastobj->extra + 1;
                        lastobj = tReset;
                     }
                     break;
               }
            }
            break;
      }
   }
}

/* Remove all resets associated with the index */
void remove_all_resets( OBJ_INDEX_DATA *oindex, MOB_INDEX_DATA *mindex )
{
   ROOM_INDEX_DATA *rid, *rid_next;
   RESET_DATA *pReset, *pReset_next, *tReset, *tReset_next;
   MOB_INDEX_DATA *mcindex;
   OBJ_INDEX_DATA *ocindex;
   int icnt;

   for( icnt = 0; icnt < MKH; icnt++ )
   {
      for( rid = room_index_hash[icnt]; rid; rid = rid_next )
      {
         rid_next = rid->next;

         for( pReset = rid->first_reset; pReset; pReset = pReset_next )
         {
            pReset_next = pReset->next;

            for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
            {
               tReset_next = tReset->next_reset;

               switch( UPPER( tReset->command ) )
               {
                  default:
                     break;

                  case 'G':
                  case 'E':
                     if( oindex && ( ocindex = get_obj_index( tReset->arg1 ) ) == oindex )
                     {
                        UNLINK( tReset, pReset->first_reset, pReset->last_reset, next_reset, prev_reset );
                        delete_reset( tReset );
                        --top_reset;
                     }
                     break;
               }
            }

            switch( UPPER( pReset->command ) )
            {
               default:
                  break;

               case 'M':
                  if( mindex && ( mcindex = get_mob_index( pReset->arg1 ) ) == mindex )
                  {
                     UNLINK( pReset, rid->first_reset, rid->last_reset, next, prev );
                     delete_reset( pReset );
                     --top_reset;
                  }
                  break;

               case 'O':
                  if( oindex && ( ocindex = get_obj_index( pReset->arg1 ) ) == oindex )
                  {
                     UNLINK( pReset, rid->first_reset, rid->last_reset, next, prev );
                     delete_reset( pReset );
                     --top_reset;
                  }
                  break;

               case 'P':
                  if( oindex && ( ocindex = get_obj_index( pReset->arg1 ) ) == oindex )
                  {
                     UNLINK( pReset, rid->first_reset, rid->last_reset, next, prev );
                     delete_reset( pReset );
                     --top_reset;
                  }

                  if( oindex && ( ocindex = get_obj_index( pReset->arg3 ) ) == oindex )
                  {
                     UNLINK( pReset, rid->first_reset, rid->last_reset, next, prev );
                     delete_reset( pReset );
                     --top_reset;
                  }
                  break;

               case 'G':
               case 'E':
                  if( oindex && ( ocindex = get_obj_index( pReset->arg1 ) ) == oindex )
                  {
                     UNLINK( pReset, rid->first_reset, rid->last_reset, next, prev );
                     delete_reset( pReset );
                     --top_reset;
                  }
                  break;
            }
         }
      }
   }
}

void free_track_resets( OBJ_INDEX_DATA *oindex, MOB_INDEX_DATA *mindex )
{
   RESET_TRACK_DATA *rtrack, *rtrack_next;

   if( !oindex && !mindex )
      return;

   remove_all_resets( oindex, mindex ); /* Might as well go ahead and free the resets for it since freeing these */

   if( oindex )
      rtrack = oindex->first_track;
   else
      rtrack = mindex->first_track;

   for( ; rtrack; rtrack = rtrack_next )
   {
      rtrack_next = rtrack->next;

      if( oindex )
         UNLINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
      else
         UNLINK( rtrack, mindex->first_track, mindex->last_track, next, prev );

      DISPOSE( rtrack );
   }
}

void show_track_resets( CHAR_DATA *ch, OBJ_INDEX_DATA *oindex, MOB_INDEX_DATA *mindex )
{
   RESET_TRACK_DATA *rtrack;
   short cnt = 0;
   char *rbuf;

   if( !oindex && !mindex )
   {
      send_to_char( "No object/mobile index data to show resets for.\r\n", ch );
      return;
   }

   if( oindex )
      rtrack = oindex->first_track;
   else
      rtrack = mindex->first_track;

   for( ; rtrack; rtrack = rtrack->next )
   {
      ++cnt;
      if( !( rbuf = sprint_reset( rtrack->reset, &cnt ) ) )
         continue;
      send_to_char( rbuf, ch );
   }

   if( cnt == 0 )
      send_to_char( "This object has no resets anywhere.\r\n", ch );
}

void add_track_reset( RESET_DATA *reset )
{
   OBJ_INDEX_DATA *oindex;
   MOB_INDEX_DATA *mindex;
   RESET_TRACK_DATA *rtrack;

   switch( UPPER( reset->command ) )
   {
      case 'M':
         if( ( mindex = get_mob_index( reset->arg1 ) ) )
         {
            CREATE( rtrack, RESET_TRACK_DATA, 1 );
            if( rtrack )
            {
               rtrack->reset = reset;
               LINK( rtrack, mindex->first_track, mindex->last_track, next, prev );
            }
         }
         break;

      case 'E':
      case 'G':
         if( ( oindex = get_obj_index( reset->arg1 ) ) )
         {
            CREATE( rtrack, RESET_TRACK_DATA, 1 );
            if( rtrack )
            {
               rtrack->reset = reset;
               LINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
            }
         }
         break;

      case 'P':
         if( ( oindex = get_obj_index( reset->arg1 ) ) )
         {
            CREATE( rtrack, RESET_TRACK_DATA, 1 );
            if( rtrack )
            {
               rtrack->reset = reset;
               LINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
            }
         }
         if( ( oindex = get_obj_index( reset->arg3 ) ) )
         {
            CREATE( rtrack, RESET_TRACK_DATA, 1 );
            if( rtrack )
            {
               rtrack->reset = reset;
               LINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
            }
         }
         break;

      case 'O':
         if( ( oindex = get_obj_index( reset->arg1 ) ) )
         {
            CREATE( rtrack, RESET_TRACK_DATA, 1 );
            if( rtrack )
            {
               rtrack->reset = reset;
               LINK( rtrack, oindex->first_track, oindex->last_track, next, prev );
            }
         }
         break;
   }
}

/* Add a reset to an area -Thoric */
RESET_DATA *add_reset( ROOM_INDEX_DATA *room, char letter, int extra, int arg1, int arg2, int arg3, short rchance, short xcord, short ycord, bool wilderness )
{
   RESET_DATA *pReset;

   if( !room )
   {
      bug( "%s: NULL room!", __FUNCTION__ );
      return NULL;
   }

   letter = UPPER( letter );
   pReset = make_reset( letter, extra, arg1, arg2, arg3, rchance, xcord, ycord, wilderness );

   switch( letter )
   {
      case 'M':
         room->last_mob_reset = pReset;
         add_track_reset( pReset );
         break;

      case 'E':
      case 'G':
         if( !room->last_mob_reset )
         {
            bug( "%s: Can't add '%c' reset to room: last_mob_reset is NULL.", __FUNCTION__, letter );
            return NULL;
         }
         pReset->main_reset = room->last_mob_reset;
         room->last_obj_reset = pReset;
         LINK( pReset, room->last_mob_reset->first_reset, room->last_mob_reset->last_reset, next_reset, prev_reset );
         add_track_reset( pReset );
         return pReset;

      case 'P':
         if( !room->last_obj_reset )
         {
            bug( "%s: Can't add '%c' reset to room: last_obj_reset is NULL.", __FUNCTION__, letter );
            return NULL;
         }
         LINK( pReset, room->last_obj_reset->first_reset, room->last_obj_reset->last_reset, next_reset, prev_reset );
         add_track_reset( pReset );
         return pReset;

      case 'O':
         room->last_obj_reset = pReset;
         add_track_reset( pReset );
         break;

      case 'T':
         if( IS_SET( extra, TRAP_OBJ ) )
         {
            pReset->prev_reset = NULL;
            pReset->next_reset = room->last_obj_reset->first_reset;
            if( room->last_obj_reset->first_reset )
               room->last_obj_reset->first_reset->prev_reset = pReset;
            room->last_obj_reset->first_reset = pReset;
            if( !room->last_obj_reset->last_reset )
               room->last_obj_reset->last_reset = pReset;
            return pReset;
         }
         break;

      case 'H':
         pReset->prev_reset = NULL;
         pReset->next_reset = room->last_obj_reset->first_reset;
         if( room->last_obj_reset->first_reset )
            room->last_obj_reset->first_reset->prev_reset = pReset;
         room->last_obj_reset->first_reset = pReset;
         if( !room->last_obj_reset->last_reset )
            room->last_obj_reset->last_reset = pReset;
         return pReset;
   }
   LINK( pReset, room->first_reset, room->last_reset, next, prev );
   return pReset;
}

RESET_DATA *find_oreset( ROOM_INDEX_DATA *room, char *oname )
{
   RESET_DATA *pReset;
   OBJ_INDEX_DATA *pobj;
   char arg[MIL];
   int cnt = 0, num = number_argument( oname, arg );

   for( pReset = room->first_reset; pReset; pReset = pReset->next )
   {
      /* Only going to allow traps/hides on room reset objects. Unless someone can come up with a better way to do this. */
      if( pReset->command != 'O' )
         continue;

      if( !( pobj = get_obj_index( pReset->arg1 ) ) )
         continue;

      if( is_name( arg, pobj->name ) && ++cnt == num )
         return pReset;
   }
   return NULL;
}

CMDF( do_reset )
{
   char arg[MIL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: reset area/room/rlist\r\n", ch );
      send_to_char( "Usage: reset random <# directions>\r\n", ch );
      send_to_char( "Usage: reset delete <#>\r\n", ch );
      send_to_char( "Usage: reset hide <objname>\r\n", ch );
      send_to_char( "Usage: reset add obj/mob/give <vnum>\r\n", ch );
      send_to_char( "Usage: reset add equip <vnum> <wearloc>\r\n", ch );
      send_to_char( "Usage: reset cords <#> <x> <y>\r\n", ch );
      send_to_char( "Usage: reset rchance <#> <chance>\r\n", ch );
      send_to_char( "Usage: reset trap room <type> <charges> [flags]\r\n", ch );
      send_to_char( "Usage: reset trap obj <name> <type> <charges> [flags]\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "area" ) )
   {
      reset_area( ch->in_room->area );
      send_to_char( "Area has been reset.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "room" ) )
   {
      reset_room( ch->in_room );
      send_to_char( "Room has been reset.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "list" ) )
   {
      RESET_DATA *pReset;
      char *rbuf;
      short num;
      ROOM_INDEX_DATA *room;
      bool found = false;

      for( room = ch->in_room->area->first_room; room; room = room->next_aroom )
      {
         num = 0;
         if( !room->first_reset )
            continue;

         for( pReset = room->first_reset; pReset; pReset = pReset->next )
         {
            found = true;
            if( ++num == 1 )
               ch_printf( ch, "Room:[%d]\r\n", room->vnum );
            if( !( rbuf = sprint_reset( pReset, &num ) ) )
               continue;
            send_to_char( rbuf, ch );
         }
      }
      if( !found )
         send_to_char( "The area your in has no resets.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "rlist" ) )
   {
      RESET_DATA *pReset;
      char *rbuf;
      short num;
      ROOM_INDEX_DATA *room;
      bool found = false;

      room = ch->in_room;
      num = 0;
      if( room->first_reset )
      {
         for( pReset = room->first_reset; pReset; pReset = pReset->next )
         {
            found = true;
            if( ++num == 1 )
               ch_printf( ch, "Room:[%d]\r\n", room->vnum );
            if( !( rbuf = sprint_reset( pReset, &num ) ) )
               continue;
            send_to_char( rbuf, ch );
         }
      }
      if( !found )
         send_to_char( "The room your in has no resets.\r\n", ch );
      return;
   }

   /* When adding equip or give they will be given to the mob listed before them when reset */
   if( !str_cmp( arg, "add" ) )
   {
      RESET_DATA *uReset;
      int vnum = -1, wearloc = -1;

      argument = one_argument( argument, arg );
      if( !str_cmp( arg, "mob" ) )
      {
         if( argument == NULL || argument[0] == '\0' || !is_number( argument ) )
         {
            send_to_char( "You must specify the mobile vnum you wish to add to the reset list.\r\n", ch );
            return;
         }
         vnum = atoi( argument );
         if( vnum < 0 || !get_mob_index( vnum ) )
         {
            send_to_char( "No such mobile exist.\r\n", ch );
            return;
         }
         uReset = add_reset( ch->in_room, 'M', 1, vnum, 1, ch->in_room->vnum, 100, 0, 0, false );
         if( !uReset )
            send_to_char( "The reset failed to be added to the room.\r\n", ch );
         else
            send_to_char( "The reset has been added to the room.\r\n", ch );
         return;
      }
      else if( !str_cmp( arg, "obj" ) )
      {
         if( argument == NULL || argument[0] == '\0' || !is_number( argument ) )
         {
            send_to_char( "You must specify the object vnum you wish to add to the reset list.\r\n", ch );
            return;
         }
         vnum = atoi( argument );
         if( vnum < 0 || !get_obj_index( vnum ) )
         {
            send_to_char( "No such object exist.\r\n", ch );
            return;
         }
         uReset = add_reset( ch->in_room, 'O', 1, vnum, 1, ch->in_room->vnum, 100, 0, 0, false );
         if( !uReset )
            send_to_char( "The reset failed to be added to the room.\r\n", ch );
         else
            send_to_char( "The reset has been added to the room.\r\n", ch );
         return;
      }
      else if( !str_cmp( arg, "give" ) )
      {
         if( argument == NULL || argument[0] == '\0' || !is_number( argument ) )
         {
            send_to_char( "You must specify the object vnum you wish to add to the reset list.\r\n", ch );
            return;
         }
         vnum = atoi( argument );
         if( vnum < 0 || !get_obj_index( vnum ) )
         {
            send_to_char( "No such object exist.\r\n", ch );
            return;
         }
         uReset = add_reset( ch->in_room, 'G', 1, vnum, 1, ch->in_room->vnum, 100, 0, 0, false );
         if( !uReset )
            send_to_char( "The reset failed to be added to the room.\r\n", ch );
         else
            send_to_char( "The reset has been added to the room.\r\n", ch );
         return;
      }
      else if( !str_cmp( arg, "equip" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "You must specify the object vnum you wish to add to the reset list.\r\n", ch );
            return;
         }
         vnum = atoi( arg );
         if( vnum < 0 || !get_obj_index( vnum ) )
         {
            send_to_char( "No such object exist.\r\n", ch );
            return;
         }

         if( argument == NULL || argument[0] == '\0' )
         {
            send_to_char( "You need to specify what location you want the object to be equipped on.\r\n", ch );
            return;
         }

         wearloc = get_flag( argument, wear_locs, WEAR_MAX );
         if( wearloc < 0 || wearloc >= WEAR_MAX )
         {
            ch_printf( ch, "Unknown wear location: %s\r\n", argument );
            return;
         }
         uReset = add_reset( ch->in_room, 'E', 1, vnum, 1, wearloc, 100, 0, 0, false );
         if( !uReset )
            send_to_char( "The reset failed to be added to the room.\r\n", ch );
         else
            send_to_char( "The reset has been added to the room.\r\n", ch );
         return;
      }
      send_to_char( "Usage: reset add obj/mob/give/equip <vnum> [<wearloc>]\r\n", ch );
      return;
   }

   /* Yeah, I know, this function is mucho ugly... but... */
   if( !str_cmp( arg, "delete" ) )
   {
      RESET_DATA *pReset, *tReset, *pReset_next, *tReset_next, *gReset, *gReset_next;
      int num, nfind = 0;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You must specify a reset # in this room to delete one.\r\n", ch );
         return;
      }

      if( !is_number( argument ) )
      {
         send_to_char( "Specified reset must be designated by number. See &Wredit rlist&D.\r\n", ch );
         return;
      }
      num = atoi( argument );

      for( pReset = ch->in_room->first_reset; pReset; pReset = pReset_next )
      {
         pReset_next = pReset->next;

         if( ++nfind == num )
         {
            UNLINK( pReset, ch->in_room->first_reset, ch->in_room->last_reset, next, prev );
            delete_reset( pReset );
            send_to_char( "Reset deleted.\r\n", ch );
            return;
         }

         for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
         {
            tReset_next = tReset->next_reset;

            if( ++nfind == num )
            {
               UNLINK( tReset, pReset->first_reset, pReset->last_reset, next_reset, prev_reset );
               delete_reset( tReset );
               send_to_char( "Reset deleted.\r\n", ch );
               return;
            }

            for( gReset = tReset->first_reset; gReset; gReset = gReset_next )
            {
               gReset_next = gReset->next_reset;

               if( ++nfind == num )
               {
                  UNLINK( gReset, tReset->first_reset, tReset->last_reset, next_reset, prev_reset );
                  delete_reset( gReset );
                  send_to_char( "Reset deleted.\r\n", ch );
                  return;
               }
            }
         }
      }
      send_to_char( "No reset matching that number was found in this room.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "rchance" ) )
   {
      RESET_DATA *pReset, *tReset, *pReset_next, *tReset_next, *gReset, *gReset_next;
      int num, nfind = 0, rchance = 0;
      char numarg[MIL];

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You must specify a reset # in this room to change.\r\n", ch );
         return;
      }

      argument = one_argument( argument, numarg );
      if( !is_number( numarg ) )
      {
         send_to_char( "Specified reset must be designated by number. See &Wredit rlist&D.\r\n", ch );
         return;
      }

      num = atoi( numarg );

      rchance = atoi( argument );
      if( rchance <= 0 || rchance > 100 )
      {
         send_to_char( "Reset Chance range is 1-100.\r\n", ch );
         return;
      }

      for( pReset = ch->in_room->first_reset; pReset; pReset = pReset_next )
      {
         pReset_next = pReset->next;

         if( ++nfind == num )
         {
            pReset->rchance = rchance;
            send_to_char( "Reset chance changed.\r\n", ch );
            return;
         }

         for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
         {
            tReset_next = tReset->next_reset;

            if( ++nfind == num )
            {
               tReset->rchance = rchance;
               send_to_char( "Reset chance changed.\r\n", ch );
               return;
            }

            for( gReset = tReset->first_reset; gReset; gReset = gReset_next )
            {
               gReset_next = gReset->next_reset;

               if( ++nfind == num )
               {
                  gReset->rchance = rchance;
                  send_to_char( "Reset chance changed.\r\n", ch );
                  return;
               }
            }
         }
      }
      send_to_char( "No reset matching that number was found in this room.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "cords" ) )
   {
      RESET_DATA *pReset, *tReset, *pReset_next, *tReset_next, *gReset, *gReset_next;
      int num, nfind = 0;
      short cordx = 0, cordy = 0;
      char numarg[MIL];

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You must specify a reset # in this room to change.\r\n", ch );
         return;
      }

      argument = one_argument( argument, numarg );
      if( !is_number( numarg ) )
      {
         send_to_char( "Specified reset must be designated by number. See &Wredit rlist&D.\r\n", ch );
         return;
      }

      num = atoi( numarg );

      argument = one_argument( argument, numarg );
      if( !is_number( numarg ) )
      {
         send_to_char( "You must specify the x and y cords. See &Wredit rlist&D.\r\n", ch );
         return;
      }
      cordx = atoi( numarg );

      argument = one_argument( argument, numarg );
      if( !is_number( numarg ) )
      {
         send_to_char( "You must specify the x and y cords. See &Wredit rlist&D.\r\n", ch );
         return;
      }
      cordy = atoi( numarg );

      for( pReset = ch->in_room->first_reset; pReset; pReset = pReset_next )
      {
         pReset_next = pReset->next;

         if( ++nfind == num )
         {
            pReset->cords[0] = cordx;
            pReset->cords[1] = cordy;
            pReset->wilderness = true;
            send_to_char( "Reset chance changed.\r\n", ch );
            return;
         }

         for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
         {
            tReset_next = tReset->next_reset;

            if( ++nfind == num )
            {
               tReset->cords[0] = cordx;
               tReset->cords[1] = cordy;
               tReset->wilderness = true;
               send_to_char( "Reset chance changed.\r\n", ch );
               return;
            }

            for( gReset = tReset->first_reset; gReset; gReset = gReset_next )
            {
               gReset_next = gReset->next_reset;

               if( ++nfind == num )
               {
                  gReset->cords[0] = cordx;
                  gReset->cords[1] = cordy;
                  gReset->wilderness = true;
                  send_to_char( "Reset chance changed.\r\n", ch );
                  return;
               }
            }
         }
      }
      send_to_char( "No reset matching that number was found in this room.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "random" ) )
   {
      RESET_DATA *pReset;
      int dir = -1;

      argument = one_argument( argument, arg );

      if( arg != NULL && arg[0] != '\0' && is_number( arg ) )
         dir = atoi( arg );

      if( dir < 0 || dir > 9 )
      {
         send_to_char( "Reset which directions randomly?\r\n", ch );
         send_to_char( "3 would randomize north, south, east, west.\r\n", ch );
         send_to_char( "5 would do those and up, down.\r\n", ch );
         send_to_char( "9 would do those and ne, nw, se, sw.\r\n", ch );
         return;
      }

      if( dir == 0 )
      {
         send_to_char( "There is no point in randomizing only one direction.\r\n", ch );
         return;
      }

      pReset = make_reset( 'R', 0, ch->in_room->vnum, dir, 0, 100, 0, 0, false );
      pReset->prev = NULL;
      pReset->next = ch->in_room->first_reset;
      if( ch->in_room->first_reset )
         ch->in_room->first_reset->prev = pReset;
      ch->in_room->first_reset = pReset;
      if( !ch->in_room->last_reset )
         ch->in_room->last_reset = pReset;
      ch_printf( ch, "Reset door(s) randomly 0 - %d in room %d has been created.\r\n", dir, ch->in_room->vnum );
      return;
   }

   if( !str_cmp( arg, "trap" ) )
   {
      RESET_DATA *pReset = NULL;
      RESET_DATA *tReset = NULL;
      char oname[MIL], arg2[MIL];
      int num, chrg, value, extra = 0, vnum;

      argument = one_argument( argument, arg2 );

      if( !str_cmp( arg2, "room" ) )
      {
         vnum = ch->in_room->vnum;
         extra = TRAP_ROOM;

         argument = one_argument( argument, arg );
         num = is_number( arg ) ? atoi( arg ) : -1;
         argument = one_argument( argument, arg );
         chrg = is_number( arg ) ? atoi( arg ) : -1;
      }
      else if( !str_cmp( arg2, "obj" ) )
      {
         argument = one_argument( argument, oname );
         if( !( pReset = find_oreset( ch->in_room, oname ) ) )
         {
            send_to_char( "No matching reset found to set a trap on.\r\n", ch );
            return;
         }
         vnum = 0;
         extra = TRAP_OBJ;

         argument = one_argument( argument, arg );
         num = is_number( arg ) ? atoi( arg ) : -1;
         argument = one_argument( argument, arg );
         chrg = is_number( arg ) ? atoi( arg ) : -1;
      }
      else
      {
         send_to_char( "Trap reset must be on 'room' or 'obj'\r\n", ch );
         return;
      }

      if( num < 1 || num >= TRAP_TYPE_MAX )
      {
         ch_printf( ch, "Invalid trap type (%d).\r\n", num );
         return;
      }

      if( chrg < 0 || chrg > 10000 )
      {
         send_to_char( "Invalid trap charges. Must be between 1 and 10000.\r\n", ch );
         return;
      }

      while( *argument )
      {
         argument = one_argument( argument, arg );
         value = get_flag( arg, trap_flags, TRAP_MAX );
         if( value < 0 || value >= TRAP_MAX )
         {
            ch_printf( ch, "Bad trap flag: %s\r\n", arg );
            continue;
         }
         SET_BIT( extra, 1 << value );
      }
      tReset = make_reset( 'T', extra, num, chrg, vnum, 100,0, 0, false );
      if( pReset )
      {
         tReset->prev_reset = NULL;
         tReset->next_reset = pReset->first_reset;
         if( pReset->first_reset )
            pReset->first_reset->prev_reset = tReset;
         pReset->first_reset = tReset;
         if( !pReset->last_reset )
            pReset->last_reset = tReset;
      }
      else
      {
         tReset->prev = NULL;
         tReset->next = ch->in_room->first_reset;
         if( ch->in_room->first_reset )
            ch->in_room->first_reset->prev = tReset;
         ch->in_room->first_reset = tReset;
         if( !ch->in_room->last_reset )
            ch->in_room->last_reset = tReset;
      }
      send_to_char( "Trap created.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "hide" ) )
   {
      RESET_DATA *pReset = NULL;
      RESET_DATA *tReset = NULL;

      if( !( pReset = find_oreset( ch->in_room, argument ) ) )
      {
         send_to_char( "No such object to hide in this room.\r\n", ch );
         return;
      }
      tReset = make_reset( 'H', 1, 0, 0, 0, 100, 0, 0, false );
      if( pReset )
      {
         tReset->prev_reset = NULL;
         tReset->next_reset = pReset->first_reset;
         if( pReset->first_reset )
            pReset->first_reset->prev_reset = tReset;
         pReset->first_reset = tReset;
         if( !pReset->last_reset )
            pReset->last_reset = tReset;
      }
      else
      {
         tReset->prev = NULL;
         tReset->next = ch->in_room->first_reset;
         if( ch->in_room->first_reset )
            ch->in_room->first_reset->prev = tReset;
         ch->in_room->first_reset = tReset;
         if( !ch->in_room->last_reset )
            ch->in_room->last_reset = tReset;
      }
      send_to_char( "Hide reset created.\r\n", ch );
      return;
   }
   do_reset( ch, (char *)"" );
}
