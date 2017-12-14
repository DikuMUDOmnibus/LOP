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
 *			 Tracking/hunting module			     *
 *****************************************************************************/

#include <stdio.h>
#include "h/mud.h"

#define BFS_ERROR	   -1
#define BFS_ALREADY_THERE  -2
#define BFS_NO_PATH	   -3
#define BFS_MARK           ROOM_BFS_MARK

CHAR_DATA *get_char_area( CHAR_DATA *ch, char *argument );
CHAR_DATA *scan_for_victim( CHAR_DATA *ch, EXIT_DATA *pexit, char *name );
bool mob_fire( CHAR_DATA *ch, char *name );

#define TRACK_THROUGH_DOORS

/*
 * You can define or not define TRACK_THOUGH_DOORS, above, depending on
 * whether or not you want track to find paths which lead through closed
 * or hidden doors.
 */
typedef struct bfs_queue_struct BFS_DATA;
struct bfs_queue_struct
{
   BFS_DATA *next;
   ROOM_INDEX_DATA *room;
   char dir;
};

static BFS_DATA *queue_head = NULL, *queue_tail = NULL, *room_queue = NULL;

/* Utility macros */
#define MARK( room )      ( xSET_BIT( (room)->room_flags, BFS_MARK ) )
#define UNMARK( room )    ( xREMOVE_BIT( (room)->room_flags, BFS_MARK ) )
#define IS_MARKED( room ) ( xIS_SET( (room)->room_flags, BFS_MARK ) )

bool valid_edge( EXIT_DATA *pexit )
{
   if( pexit->to_room
#ifndef TRACK_THROUGH_DOORS
   && !xIS_SET( pexit->exit_info, EX_CLOSED )
#endif
   && !IS_MARKED( pexit->to_room ) )
      return true;
   else
      return false;
}

void bfs_enqueue( ROOM_INDEX_DATA *room, char dir )
{
   BFS_DATA *curr;

   curr = ( BFS_DATA * ) malloc( sizeof( BFS_DATA ) );
   curr->room = room;
   curr->dir = dir;
   curr->next = NULL;

   if( queue_tail )
   {
      queue_tail->next = curr;
      queue_tail = curr;
   }
   else
      queue_head = queue_tail = curr;
}

void bfs_dequeue( void )
{
   BFS_DATA *curr;

   curr = queue_head;

   if( !( queue_head = queue_head->next ) )
      queue_tail = NULL;
   free( curr );
}

void bfs_clear_queue( void )
{
   while( queue_head )
      bfs_dequeue( );
}

void room_enqueue( ROOM_INDEX_DATA *room )
{
   BFS_DATA *curr;

   curr = ( BFS_DATA * ) malloc( sizeof( BFS_DATA ) );
   curr->room = room;
   curr->next = room_queue;

   room_queue = curr;
}

void clean_room_queue( void )
{
   BFS_DATA *curr, *curr_next;

   for( curr = room_queue; curr; curr = curr_next )
   {
      UNMARK( curr->room );
      curr_next = curr->next;
      free( curr );
   }
   room_queue = NULL;
}

int find_first_step( ROOM_INDEX_DATA *src, ROOM_INDEX_DATA *target, int maxdist )
{
   EXIT_DATA *pexit;
   int curr_dir, count;

   if( !src || !target )
   {
      bug( "%s: Illegal value passed to find_first_step (track.c)", __FUNCTION__ );
      return BFS_ERROR;
   }

   if( src == target )
      return BFS_ALREADY_THERE;

   if( src->area != target->area )
      return BFS_NO_PATH;

   room_enqueue( src );
   MARK( src );

   /* first, enqueue the first steps, saving which direction we're going. */
   for( pexit = src->first_exit; pexit; pexit = pexit->next )
   {
      if( valid_edge( pexit ) )
      {
         curr_dir = pexit->vdir;
         MARK( pexit->to_room );
         room_enqueue( pexit->to_room );
         bfs_enqueue( pexit->to_room, curr_dir );
      }
   }

   count = 0;
   while( queue_head )
   {
      if( ++count > maxdist )
      {
         bfs_clear_queue( );
         clean_room_queue( );
         return BFS_NO_PATH;
      }
      if( queue_head->room == target )
      {
         curr_dir = queue_head->dir;
         bfs_clear_queue( );
         clean_room_queue( );
         return curr_dir;
      }
      else
      {
         for( pexit = queue_head->room->first_exit; pexit; pexit = pexit->next )
            if( valid_edge( pexit ) )
            {
               curr_dir = pexit->vdir;
               MARK( pexit->to_room );
               room_enqueue( pexit->to_room );
               bfs_enqueue( pexit->to_room, queue_head->dir );
            }
         bfs_dequeue( );
      }
   }
   clean_room_queue( );

   return BFS_NO_PATH;
}

CMDF( do_track )
{
   CHAR_DATA *vict;
   char arg[MIL], fpath[MSL*2];
   int dir, maxdist, lastdir = 0, lastnumber = 0;
   bool firststep = true;

   if( !is_npc( ch ) && ch->pcdata->learned[gsn_track] <= 0 )
   {
      send_to_char( "You don't know of this skill yet.\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Whom are you trying to track?\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_track]->beats );

   if( !( vict = get_char_area( ch, arg ) ) )
   {
      send_to_char( "You can't find a trail of anyone like that.\r\n", ch );
      return;
   }

   maxdist = 100 + ch->level * 30;

   if( !is_npc( ch ) )
      maxdist = ( maxdist * LEARNED( ch, gsn_track ) ) / 100;

   /* Ok I want it to give a nice quick overview to how to get there */
   {
      ROOM_INDEX_DATA *fromroom = ch->in_room;
      int steps = 0;

      fpath[0] = '\0';

      for( steps = 0; steps < 1000; steps++ )
      {
         dir = find_first_step( fromroom, vict->in_room, maxdist );
         if( steps == 0 )
         {
            if( dir == BFS_ERROR )
            {
               send_to_char( "Hmm... At some point something went wrong.\r\n", ch );
               return;
            }
            else if( dir == BFS_ALREADY_THERE )
            {
               send_to_char( "You're already in the same room!\r\n", ch );
               return;
            }
            else if( dir == BFS_NO_PATH )
            {
               send_to_char( "You can't sense a trail from here.\r\n", ch );
               return;
            }

            lastdir = dir;
         }
         else
         {
            if( dir == BFS_ERROR )
            {
               send_to_char( "Hmm... At some point something went wrong.\r\nsomething seems to be wrong.\r\n", ch );
               return;
            }
         }

         /* Ok now for the fun stuff we want to get the direction name and number of times and then put them all together */
         if( lastdir != dir )
         {
            char snum[MIL];

            snprintf( snum, sizeof( snum ), "%s%d %s", !firststep ? ", " : "", lastnumber, dir_name[lastdir] );
            mudstrlcat( fpath, snum, sizeof( fpath ) );

            lastnumber = 1;
            firststep = false;
            lastdir = dir;
         }
         else
            lastnumber++;

         if( dir == BFS_ALREADY_THERE || dir == BFS_NO_PATH )
            break;

         fromroom = get_exit( fromroom, dir )->to_room; /* Change fromroom information */
      }   
   }

   ch_printf( ch, "You sense a trail %s from here...\r\n", fpath );
   learn_from_success( ch, gsn_track );
}

void found_prey( CHAR_DATA *ch, CHAR_DATA *victim )
{
   char victname[MSL];

   if( !victim )
   {
      bug( "%s: null victim", __FUNCTION__ );
      return;
   }

   if( !victim->in_room )
   {
      bug( "%s: null victim->in_room", __FUNCTION__ );
      return;
   }

   mudstrlcpy( victname, is_npc( victim ) ? victim->short_descr : victim->name, sizeof( victname ) );

   if( !can_see( ch, victim ) )
   {
      if( number_percent( ) < 90 )
         return;
      switch( number_bits( 2 ) )
      {
         case 0:
            interpret_printf( ch, "say Don't make me find you, %s!", victname );
            break;

         case 1:
            act( AT_ACTION, "$n sniffs around the room for $N.", ch, NULL, victim, TO_NOTVICT );
            act( AT_ACTION, "You sniff around the room for $N.", ch, NULL, victim, TO_CHAR );
            act( AT_ACTION, "$n sniffs around the room for you.", ch, NULL, victim, TO_VICT );
            interpret( ch, (char *)"say I can smell your blood!" );
            break;

         case 2:
            interpret_printf( ch, "yell I'm going to tear %s apart!", victname );
            break;

         case 3:
            interpret( ch, (char *)"say Just wait until I find you..." );
            break;
      }
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      if( number_percent( ) < 90 )
         return;
      switch( number_bits( 2 ) )
      {
         case 0:
            interpret( ch, (char *)"say C'mon out, you coward!" );
            interpret_printf( ch, "yell %s is a bloody coward!", victname );
            break;

         case 1:
            interpret_printf( ch, "say Let's take this outside, %s", victname );
            break;

         case 2:
            interpret_printf( ch, "yell %s is a yellow-bellied wimp!", victname );
            break;

         case 3:
            act( AT_ACTION, "$n takes a few swipes at $N.", ch, NULL, victim, TO_NOTVICT );
            act( AT_ACTION, "You try to take a few swipes $N.", ch, NULL, victim, TO_CHAR );
            act( AT_ACTION, "$n takes a few swipes at you.", ch, NULL, victim, TO_VICT );
            break;
      }
      return;
   }

   switch( number_bits( 2 ) )
   {
      case 0:
         interpret_printf( ch, "yell Your blood is mine, %s!", victname );
         break;

      case 1:
         interpret_printf( ch, "say Alas, we meet again, %s!", victname );
         break;

      case 2:
         interpret_printf( ch, "say What do you want on your tombstone, %s?", victname );
         break;

      case 3:
         act( AT_ACTION, "$n lunges at $N from out of nowhere!", ch, NULL, victim, TO_NOTVICT );
         act( AT_ACTION, "You lunge at $N catching $M off guard!", ch, NULL, victim, TO_CHAR );
         act( AT_ACTION, "$n lunges at you from out of nowhere!", ch, NULL, victim, TO_VICT );
         break;
   }
   stop_hunting( ch, victim, false );
   set_fighting( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

void hunt_victim( CHAR_DATA *ch )
{
   CHAR_DATA *tmp, *vch;
   EXIT_DATA *pexit;
   HHF_DATA *hunt;
   short ret;
   bool found = false;

   if( !ch || !ch->first_hunting || ch->position < 5 )
      return;

   for( hunt = ch->first_hunting; hunt; hunt = hunt->next )
   {
      if( !hunt->who )
         continue;
      found = true;
      break;
   }
   if( !hunt || !found )
      return;

   tmp = hunt->who;
   if( ch->in_room == tmp->in_room )
   {
      if( ch->fighting )
         return;
      found_prey( ch, tmp );
      return;
   }

   ret = find_first_step( ch->in_room, tmp->in_room, 500 + ch->level * 25 );
   if( ret < 0 )
   {
      /* Remove and readd so can hunt them later */
      stop_hunting( ch, tmp, false );
      start_hunting( ch, tmp );
      return;
   }

   if( !( pexit = get_exit( ch->in_room, ret ) ) )
   {
      bug( "%s: lost exit?", __FUNCTION__ );
      return;
   }
   move_char( ch, pexit, 0, false );

   /* Crash bug fix by Shaddai */
   if( char_died( ch ) )
      return;

   if( !ch->in_room )
   {
      bug( "%s: no ch->in_room!  Mob #%d, name: %s.  Placing mob in limbo.", __FUNCTION__, ch->pIndexData->vnum, ch->name );
      char_to_room( ch, get_room_index( sysdata.room_limbo ) );
      return;
   }

   if( ch->in_room == tmp->in_room )
   {
      found_prey( ch, tmp );
      return;
   }

   /*
    * perform a ranged attack if possible 
    * Changed who to name as scan_for_victim expects the name and Not the char struct. --Shaddai
    */
   if( ( vch = scan_for_victim( ch, pexit, tmp->name ) ) )
   {
      if( !mob_fire( ch, tmp->name ) )
      {
         /* ranged spell attacks go here */
      }
   }
}
