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
 *			   Player movement module			     *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "h/mud.h"

bool sneaking_char;

const short movement_loss[SECT_MAX] =
{
   1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6, 5, 7, 4, 15, 8
};

const char *dir_name[] =
{
   "north",       "east",        "south",       "west",
   "up",          "down",        "northeast",   "northwest",
   "southeast",   "southwest",   "somewhere"
};

const int trap_door[] =
{
   TRAP_N,    TRAP_E,    TRAP_S,    TRAP_W,
   TRAP_U,    TRAP_D,    TRAP_NE,   TRAP_NW,
   TRAP_SE,   TRAP_SW
};

const short rev_dir[] =
{
   2, 3, 0, 1, 5, 4, 9, 8, 7, 6, 10
};

const char *rev_exit( short vdir )
{
   switch( vdir )
   {
      default: return "somewhere";
      case 0:  return "the south";
      case 1:  return "the west";
      case 2:  return "the north";
      case 3:  return "the east";
      case 4:  return "below";
      case 5:  return "above";
      case 6:  return "the southwest";
      case 7:  return "the southeast";
      case 8:  return "the northwest";
      case 9:  return "the northeast";
   }
}

/*
 * Function to get the equivelant exit of DIR 0-MAXDIR out of linked list.
 * Made to allow old-style diku-merc exit functions to work.	-Thoric
 */
EXIT_DATA *get_exit( ROOM_INDEX_DATA *room, short dir )
{
   EXIT_DATA *xit;

   if( !room )
   {
      bug( "%s: NULL room", __FUNCTION__ );
      return NULL;
   }

   for( xit = room->first_exit; xit; xit = xit->next )
      if( xit->vdir == dir )
         return xit;
   return NULL;
}

/* Function to get an exit, leading the the specified room */
EXIT_DATA *get_exit_to( ROOM_INDEX_DATA *room, short dir, int vnum )
{
   EXIT_DATA *xit;

   if( !room )
   {
      bug( "%s: NULL room", __FUNCTION__ );
      return NULL;
   }

   for( xit = room->first_exit; xit; xit = xit->next )
      if( xit->vdir == dir && xit->vnum == vnum )
         return xit;
   return NULL;
}

/* Function to get the nth exit of a room - Thoric */
EXIT_DATA *get_exit_num( ROOM_INDEX_DATA *room, short count )
{
   EXIT_DATA *xit;
   int cnt;

   if( !room )
   {
      bug( "%s: NULL room", __FUNCTION__ );
      return NULL;
   }

   for( cnt = 0, xit = room->first_exit; xit; xit = xit->next )
      if( ++cnt == count )
         return xit;
   return NULL;
}

/* Modify movement due to encumbrance - Thoric */
short encumbrance( CHAR_DATA *ch, short move )
{
   int cur, max;

   max = can_carry_w( ch );
   cur = ch->carry_weight;
   if( cur >= max )
      return move * 4;
   else if( cur >= max * 0.95 )
      return ( short )( move * 3.5 );
   else if( cur >= max * 0.90 )
      return move * 3;
   else if( cur >= max * 0.85 )
      return ( short )( move * 2.5 );
   else if( cur >= max * 0.80 )
      return move * 2;
   else if( cur >= max * 0.75 )
      return ( short )( move * 1.5 );
   else
      return move;
}

/* Check to see if a character can fall down, checks for looping - Thoric */
bool will_fall( CHAR_DATA *ch, int fall )
{
   if( xIS_SET( ch->in_room->room_flags, ROOM_NOFLOOR )
   && can_go( ch->in_room, DIR_DOWN )
   && ( !IS_AFFECTED( ch, AFF_FLYING ) || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) ) ) )
   {
      if( fall > 80 )
      {
         bug( "Falling (in a loop?) more than 80 rooms: vnum %d", ch->in_room->vnum );
         char_from_room( ch );
         char_to_room( ch, get_room_index( sysdata.room_temple ) );
         fall = 0;
         return true;
      }
      set_char_color( AT_FALLING, ch );
      send_to_char( "You're falling down...\r\n", ch );
      move_char( ch, get_exit( ch->in_room, DIR_DOWN ), ++fall, false );
      return true;
   }
   return false;
}

ch_ret move_char( CHAR_DATA *ch, EXIT_DATA *pexit, int fall, bool running )
{
   ROOM_INDEX_DATA *in_room, *to_room, *from_room;
   OBJ_DATA *boat;
   const char *txt, *dtxt;
   char buf[MSL];
   ch_ret retcode;
   short door;
   bool drunk = false, brief = false;

   if( !is_npc( ch ) )
   {
      if( is_drunk( ch, 2 ) && ( ch->position != POS_SHOVE ) && ( ch->position != POS_DRAG ) )
         drunk = true;
   }

   if( drunk && !fall )
   {
      door = number_door( );
      pexit = get_exit( ch->in_room, door );
   }

   retcode = rNONE;
   txt = NULL;

   if( IS_AFFECTED( ch, AFF_NOMOVE ) )
   {
      act( AT_ACTION, "You aren't currently able to move anywhere.", ch, NULL, NULL, TO_CHAR );
      return rSTOP;
   }

   if( is_npc( ch ) && xIS_SET( ch->act, ACT_MOUNTED ) )
      return retcode;

   in_room = ch->in_room;
   from_room = in_room;
   if( !pexit || !( to_room = pexit->to_room ) )
   {
      if( drunk && ch->position != POS_MOUNTED
      && ch->in_room->sector_type != SECT_WATER_SWIM
      && ch->in_room->sector_type != SECT_WATER_NOSWIM
      && ch->in_room->sector_type != SECT_UNDERWATER
      && ch->in_room->sector_type != SECT_OCEANFLOOR )
      {
         switch( number_bits( 4 ) )
         {
            default:
               act( AT_ACTION, "You drunkenly stumble into some obstacle.", ch, NULL, NULL, TO_CHAR );
               act( AT_ACTION, "$n drunkenly stumbles into a nearby obstacle.", ch, NULL, NULL, TO_ROOM );
               break;

            case 3:
               act( AT_ACTION, "In your drunken stupor you trip over your own feet and tumble to the ground.", ch, NULL, NULL, TO_CHAR );
               act( AT_ACTION, "$n stumbles drunkenly, trips and tumbles to the ground.", ch, NULL, NULL, TO_ROOM );
               ch->position = POS_RESTING;
               break;

            case 4:
               act( AT_SOCIAL, "You utter a string of slurred obscenities.", ch, NULL, NULL, TO_CHAR );
               act( AT_ACTION, "Something blurry and immovable has intercepted you as you stagger along.", ch, NULL, NULL, TO_CHAR );
               act( AT_HURT, "Oh geez... THAT really hurt.  Everything slowly goes dark and numb...", ch, NULL, NULL, TO_CHAR );

               act( AT_ACTION, "$n drunkenly staggers into something.", ch, NULL, NULL, TO_ROOM );
               act( AT_SOCIAL, "$n utters a string of slurred obscenities: @*&^%@*&!", ch, NULL, NULL, TO_ROOM );
               act( AT_ACTION, "$n topples to the ground with a thud.", ch, NULL, NULL, TO_ROOM );
               ch->position = POS_INCAP;
               break;
         }
      }
      else if( drunk )
         act( AT_ACTION, "You stare around trying to make sense of things through your drunken stupor.", ch, NULL, NULL, TO_CHAR );
      else
         send_to_char( "Alas, you can't go that way.\r\n", ch );
      return rSTOP;
   }

   door = pexit->vdir;

   /*
    * Exit is only a "window", there is no way to travel in that direction
    * unless it's a door with a window in it      -Thoric
    */
   if( xIS_SET( pexit->exit_info, EX_WINDOW ) && !xIS_SET( pexit->exit_info, EX_ISDOOR ) )
   {
      send_to_char( "Alas, you can't go that way.\r\n", ch );
      return rSTOP;
   }

   if( is_npc( ch ) )
   {
      if( xIS_SET( pexit->exit_info, EX_PORTAL ) )
      {
         act( AT_PLAIN, "Mobs can't use portals.", ch, NULL, NULL, TO_CHAR );
         return rSTOP;
      }
      if( xIS_SET( pexit->exit_info, EX_NOMOB ) || xIS_SET( to_room->room_flags, ROOM_NO_MOB ) )
      {
         act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
         return rSTOP;
      }
   }

   if( xIS_SET( pexit->exit_info, EX_CLOSED ) && ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) || xIS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
   {
      if( !xIS_SET( pexit->exit_info, EX_SECRET ) && !xIS_SET( pexit->exit_info, EX_DIG ) )
      {
         if( drunk )
         {
            act( AT_PLAIN, "$n runs into the $d in $s drunken state.", ch, NULL, pexit->keyword, TO_ROOM );
            act( AT_PLAIN, "You run into the $d in your drunken state.", ch, NULL, pexit->keyword, TO_CHAR );
         }
         else
            act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
      }
      else
      {
         if( drunk )
            send_to_char( "You stagger around in your drunken state.\r\n", ch );
         else
            send_to_char( "Alas, you can't go that way.\r\n", ch );
      }

      return rSTOP;
   }

   if( !fall && IS_AFFECTED( ch, AFF_CHARM ) && ch->master && in_room == ch->master->in_room )
   {
      send_to_char( "What?  And leave your beloved master?\r\n", ch );
      return rSTOP;
   }

   if( ch->position ==  POS_INCAP || ch->position == POS_STUNNED )
   {
      send_to_char( "No way, you're too stunned for that!\r\n", ch );
      return rSTOP;
   }

   if( ch->position == POS_SLEEPING )
   {
      send_to_char( "In your dreams, or what?\r\n", ch );
      return rSTOP;
   }

   if( ch->position == POS_SITTING || ch->position == POS_RESTING )
   {
      send_to_char( "You can't do that sitting down.\r\n", ch );
      return rSTOP;
   }

   if( room_is_private( to_room ) )
   {
      send_to_char( "That room is private right now.\r\n", ch );
      return rSTOP;
   }

   if( room_is_dnd( ch, to_room ) )
   {
      send_to_char( "That room is \"do not disturb\" right now.\r\n", ch );
      return rSTOP;
   }

   if( !is_immortal( ch ) && !is_npc( ch ) && ch->in_room->area != to_room->area )
   {
      if( ch->level < to_room->area->low_hard_range )
      {
         set_char_color( AT_TELL, ch );
         switch( to_room->area->low_hard_range - ch->level )
         {
            case 1:
               send_to_char( "A voice in your mind says, 'You're nearly ready to go that way...'\r\n", ch );
               break;

            case 2:
               send_to_char( "A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'\r\n", ch );
               break;

            case 3:
               send_to_char( "A voice in your mind says, 'You aren't ready to go down that path... yet.'\r\n", ch );
               break;

            default:
               send_to_char( "A voice in your mind says, 'You aren't ready to go down that path.'\r\n", ch );
               break;
         }
         return rSTOP;
      }
      else if( ch->level > to_room->area->hi_hard_range )
      {
         set_char_color( AT_TELL, ch );
         send_to_char( "A voice in your mind says, 'There is nothing more for you down that path.'\r\n", ch );
         return rSTOP;
      }
   }

   if( !fall && !is_npc( ch ) )
   {
      int move;

      if( xIS_SET( to_room->area->flags, AFLAG_NOPKILL )
      && !xIS_SET( ch->in_room->area->flags, AFLAG_NOPKILL )
      && ( is_pkill( ch ) && !is_immortal( ch ) ) )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "\r\nA godly force forbids deadly characters from entering that area...\r\n", ch );
         return rSTOP;
      }

      if( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR
      || xIS_SET( pexit->exit_info, EX_FLY ) )
      {
         if( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) )
         {
            send_to_char( "Your mount can't fly.\r\n", ch );
            return rSTOP;
         }
         if( !ch->mount && !IS_AFFECTED( ch, AFF_FLYING ) )
         {
            send_to_char( "You'd need to fly to go there.\r\n", ch );
            return rSTOP;
         }
      }

      if( in_room->sector_type == SECT_WATER_NOSWIM || to_room->sector_type == SECT_WATER_NOSWIM )
      {
         if( ( ch->mount && !is_floating( ch->mount ) ) || !is_floating( ch ) )
         {
            /*
             * Look for a boat.
             * We can use the boat obj for a more detailed description.
             */
            if( ( boat = get_objtype( ch, ITEM_BOAT ) ) )
            {
               if( drunk )
                  txt = "paddles unevenly";
               else
                  txt = "paddles";
            }
            else
            {
               if( ch->mount )
                  send_to_char( "Your mount would drown!\r\n", ch );
               else
                  send_to_char( "You'd need a boat to go there.\r\n", ch );
               return rSTOP;
            }
         }
      }

      if( xIS_SET( pexit->exit_info, EX_CLIMB ) )
      {
         bool found;

         found = false;
         if( ch->mount && IS_AFFECTED( ch->mount, AFF_FLYING ) )
            found = true;
         else if( IS_AFFECTED( ch, AFF_FLYING ) )
            found = true;

         if( !found && !ch->mount )
         {
            if( ( !is_npc( ch ) && number_percent( ) > LEARNED( ch, gsn_climb ) ) || drunk || ch->mental_state < -90 )
            {
               send_to_char( "You start to climb... but lose your grip and fall!\r\n", ch );
               learn_from_failure( ch, gsn_climb );
               if( pexit->vdir == DIR_DOWN )
               {
                  retcode = move_char( ch, pexit, 1, running );
                  return retcode;
               }
               set_char_color( AT_HURT, ch );
               send_to_char( "OUCH! You hit the ground!\r\n", ch );
               wait_state( ch, 20 );
               retcode = damage( ch, ch, NULL, ( pexit->vdir == DIR_UP ? 10 : 5 ), TYPE_UNDEFINED, true );
               if( running )
                  return rSTOP;
               return retcode;
            }
            found = true;
            learn_from_success( ch, gsn_climb );
            wait_state( ch, skill_table[gsn_climb]->beats );
            txt = "climbs";
         }

         if( !found )
         {
            send_to_char( "You can't climb.\r\n", ch );
            return rSTOP;
         }
      }

      if( ch->mount )
      {
         if( is_npc( ch->mount ) )
         {
            if( xIS_SET( pexit->exit_info, EX_PORTAL ) )
            {
               act( AT_PLAIN, "Your mount can't use portals.", ch, NULL, NULL, TO_CHAR );
               return rSTOP;
            }
            if( xIS_SET( pexit->exit_info, EX_NOMOB ) || xIS_SET( to_room->room_flags, ROOM_NO_MOB ) )
            {
               act( AT_PLAIN, "Your mount can't enter there.", ch, NULL, NULL, TO_CHAR );
               return rSTOP;
            }
         }

         switch( ch->mount->position )
         {
            case POS_DEAD:
               send_to_char( "Your mount is dead!\r\n", ch );
               return rSTOP;
               break;

            case POS_MORTAL:
            case POS_INCAP:
               send_to_char( "Your mount is hurt far too badly to move.\r\n", ch );
               return rSTOP;
               break;

            case POS_STUNNED:
               send_to_char( "Your mount is too stunned to do that.\r\n", ch );
               return rSTOP;
               break;

            case POS_SLEEPING:
               send_to_char( "Your mount is sleeping.\r\n", ch );
               return rSTOP;
               break;

            case POS_RESTING:
               send_to_char( "Your mount is resting.\r\n", ch );
               return rSTOP;
               break;

            case POS_SITTING:
               send_to_char( "Your mount is sitting down.\r\n", ch );
               return rSTOP;
               break;

            default:
               break;
         }

         if( !is_floating( ch->mount ) )
            move = movement_loss[ URANGE( 0, in_room->sector_type, SECT_MAX - 1 ) ];
         else
            move = 1;
         if( ch->mount->move < move )
         {
            send_to_char( "Your mount is too exhausted.\r\n", ch );
            return rSTOP;
         }
      }
      else
      {
         if( !is_floating( ch ) )
            move = encumbrance( ch, movement_loss[ URANGE( 0, in_room->sector_type, SECT_MAX - 1 ) ] );
         else
            move = 1;
         if( ch->move < move )
         {
            send_to_char( "You're too exhausted.\r\n", ch );
            return rSTOP;
         }
      }

      wait_state( ch, move );
      if( ch->mount )
         ch->mount->move -= move;
      else
         ch->move -= move;
   }

   /* Check if player can fit in the room */
   if( to_room->tunnel > 0 )
   {
      CHAR_DATA *ctmp;
      int count = ch->mount ? 1 : 0;

      for( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
      {
         if( ++count >= to_room->tunnel )
         {
            if( ch->mount && count == to_room->tunnel )
               send_to_char( "There is no room for both you and your mount there.\r\n", ch );
            else
               send_to_char( "There is no room for you there.\r\n", ch );
            return rSTOP;
         }
      }
   }

   if( stop_fishing( ch ) )
      send_to_char( "You pull your line out of the water.\r\n", ch );

   if( is_npc( ch ) || !xIS_SET( ch->act, PLR_WIZINVIS ) )
   {
      if( fall )
         txt = "falls";
      else if( !txt )
      {
         if( ch->mount )
         {
            if( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
               txt = "floats";
            else if( IS_AFFECTED( ch->mount, AFF_FLYING ) )
               txt = "flies";
            else
               txt = "rides";
         }
         else
         {
            if( IS_AFFECTED( ch, AFF_FLOATING ) )
            {
               if( drunk )
                  txt = "floats unsteadily";
               else
                  txt = "floats";
            }
            else if( IS_AFFECTED( ch, AFF_FLYING ) )
            {
               if( drunk )
                  txt = "flies shakily";
               else
                  txt = "flies";
            }
            else if( ch->position == POS_SHOVE )
               txt = "is shoved";
            else if( ch->position == POS_DRAG )
               txt = "is dragged";
            else
            {
               if( drunk )
                  txt = "stumbles drunkenly";
               else
                  txt = "leaves";
            }
         }
      }
      if( ch->mount )
      {
         snprintf( buf, sizeof( buf ), "$n %s %s upon $N.", txt, dir_name[door] );
         if( IS_AFFECTED( ch->mount, AFF_SNEAK ) )
            sneaking_char = true;
         act( AT_ACTION, buf, ch, NULL, ch->mount, TO_NOTVICT );
         sneaking_char = false;
      }
      else
      {
         snprintf( buf, sizeof( buf ), "$n %s $T.", txt );
         if( IS_AFFECTED( ch, AFF_SNEAK ) )
            sneaking_char = true;
         act( AT_ACTION, buf, ch, NULL, (char *)dir_name[door], TO_ROOM );
         sneaking_char = false;
      }
   }

   rprog_leave_trigger( ch );
   if( char_died( ch ) )
      return global_retcode;

   if( ch->in_room->first_content )
      retcode = check_room_for_traps( ch, TRAP_LEAVE_ROOM );
   if( retcode != rNONE )
      return retcode;

   char_from_room( ch );
   char_to_room( ch, to_room );
   if( ch->mount )
   {
      rprog_leave_trigger( ch->mount );

      if( char_died( ch->mount ) )
         return global_retcode;

      if( ch->mount->in_room->first_content )
         retcode = check_room_for_traps( ch->mount, TRAP_LEAVE_ROOM );
      if( retcode != rNONE )
         return retcode;

      if( ch->mount )
      {
         char_from_room( ch->mount );
         char_to_room( ch->mount, to_room );
      }
   }

   if( is_npc( ch ) || !xIS_SET( ch->act, PLR_WIZINVIS ) )
   {
      if( fall )
         txt = "falls";
      else if( ch->mount )
      {
         if( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
            txt = "floats in";
         else if( IS_AFFECTED( ch->mount, AFF_FLYING ) )
            txt = "flies in";
         else
            txt = "rides in";
      }
      else
      {
         if( IS_AFFECTED( ch, AFF_FLOATING ) )
         {
            if( drunk )
               txt = "floats in unsteadily";
            else
               txt = "floats in";
         }
         else if( IS_AFFECTED( ch, AFF_FLYING ) )
         {
            if( drunk )
               txt = "flies in shakily";
            else
               txt = "flies in";
         }
         else if( ch->position == POS_SHOVE )
            txt = "is shoved in";
         else if( ch->position == POS_DRAG )
            txt = "is dragged in";
         else
         {
            if( drunk )
               txt = "stumbles drunkenly in";
            else
               txt = "arrives";
         }
      }
      dtxt = rev_exit( door );
      if( ch->mount )
      {
         snprintf( buf, sizeof( buf ), "$n %s from %s upon $N.", txt, dtxt );
         if( IS_AFFECTED( ch->mount, AFF_SNEAK ) )
            sneaking_char = true;
         act( AT_ACTION, buf, ch, NULL, ch->mount, TO_ROOM );
         sneaking_char = false;
      }
      else
      {
         snprintf( buf, sizeof( buf ), "$n %s from %s.", txt, dtxt );
         if( IS_AFFECTED( ch, AFF_SNEAK ) )
            sneaking_char = true;
         act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );
         sneaking_char = false;
      }
   }

   if( !is_immortal( ch ) && !is_npc( ch ) && ch->in_room->area != to_room->area )
   {
      if( ch->level < to_room->area->low_soft_range )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "You feel uncomfortable being in this strange land...\r\n", ch );
      }
      else if( ch->level > to_room->area->hi_soft_range )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "You feel there is not much to gain visiting this place...\r\n", ch );
      }
   }

   /* Make sure everyone sees the room description of death traps. */
   if( xIS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !is_immortal( ch ) )
   {
      if( xIS_SET( ch->act, PLR_BRIEF ) )
         brief = true;
      xREMOVE_BIT( ch->act, PLR_BRIEF );
   }

   if( !running || brief )
      do_look( ch, (char *)"auto" );

   if( brief )
      xSET_BIT( ch->act, PLR_BRIEF );

   /* Put good-old EQ-munching death traps back in! - Thoric */
   if( xIS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !is_immortal( ch ) )
   {
      act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
      act( AT_DEAD, "Oopsie... you're dead!\r\n", ch, NULL, NULL, TO_CHAR );
      snprintf( buf, sizeof( buf ), "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
      log_string( buf );
      to_channel( buf, "monitor", PERM_IMM );
      if( is_npc( ch ) )
         extract_char( ch, true );
      else
         extract_char( ch, false );
      return rCHAR_DIED;
   }

   /*
    * BIG ugly looping problem here when the character is mptransed back
    * to the starting room.  To avoid this, check how many chars are in 
    * the room at the start and stop processing followers after doing
    * the right number of them.  -- Narn
    */
   if( !fall )
   {
      CHAR_DATA *fch, *nextinroom, *master;
      int chars = 0, count = 0;

      for( fch = from_room->first_person; fch; fch = fch->next_in_room )
         chars++;

      for( fch = from_room->first_person; fch && ( count < ( chars + 5 ) ); fch = nextinroom )
      {
         nextinroom = fch->next_in_room;
         count++;

         /*
          * Since the old way would sometimes loose people if not following right,
          * it is now done this way, if you are following John and he's following Bill
          * you are also following bill to keep things right it will go with you following Bill.
          */
         if( fch == ch || !fch->master )
            continue;

         master = fch->master;
         while( master->master )
            master = master->master;

         if( master != ch || xIS_SET( ch->act, PLR_SOLO ) || xIS_SET( fch->act, PLR_SOLO ) )
            continue;

         if( fch->position != POS_STANDING && fch->position != POS_MOUNTED )
            continue;

         act( AT_ACTION, "You follow $N.", fch, NULL, master, TO_CHAR );
         move_char( fch, get_exit( from_room, door ), 0, running );
      }
   }


   if( ch->in_room->first_content )
      retcode = check_room_for_traps( ch, TRAP_ENTER_ROOM );
   if( retcode != rNONE || char_died( ch ) )
      return retcode;

   mprog_entry_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   rprog_enter_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   mprog_greet_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   oprog_greet_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   if( !will_fall( ch, fall ) && fall > 0 )
   {
      if( !IS_AFFECTED( ch, AFF_FLOATING ) || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLOATING ) ) )
      {
         if( ch->mount )
         {
            set_char_color( AT_HURT, ch->mount );
            send_to_char( "OUCH! You hit the ground!\r\n", ch->mount );
            wait_state( ch->mount, 20 );
            retcode = damage( ch->mount, ch->mount, NULL, 20 * fall, TYPE_UNDEFINED, true );
         }
         else
         {
            set_char_color( AT_HURT, ch );
            send_to_char( "OUCH! You hit the ground!\r\n", ch );
            wait_state( ch, 20 );
            retcode = damage( ch, ch, NULL, 20 * fall, TYPE_UNDEFINED, true );
         }
      }
      else
      {
         set_char_color( AT_MAGIC, ch );
         if( ch->mount )
            send_to_char( "You and your mount lightly float down to the ground.\r\n", ch );
         else
            send_to_char( "You lightly float down to the ground.\r\n", ch );
      }

      if( running )
         retcode = rSTOP;
   }

   /* Very small chance of getting a dex increase through moving around */
   handle_stat( ch, STAT_DEX, true, 1 );

   return retcode;
}

CMDF( do_north )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 0, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_NORTH ), 0, false );
}

CMDF( do_east )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 2, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_EAST ), 0, false );
}

CMDF( do_south )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 1, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_SOUTH ), 0, false );
}

CMDF( do_west )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 3, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_WEST ), 0, false );
}

CMDF( do_up )
{
   if( is_in_wilderness( ch ) )
      send_to_char( "You can't go up in the wilderness.\r\n", ch );
   else
      move_char( ch, get_exit( ch->in_room, DIR_UP ), 0, false );
}

CMDF( do_down )
{
   if( is_in_wilderness( ch ) )
      send_to_char( "You can't go down in the wilderness.\r\n", ch );
   else
      move_char( ch, get_exit( ch->in_room, DIR_DOWN ), 0, false );
}

CMDF( do_northeast )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 4, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_NORTHEAST ), 0, false );
}

CMDF( do_northwest )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 5, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_NORTHWEST ), 0, false );
}

CMDF( do_southeast )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 6, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_SOUTHEAST ), 0, false );
}

CMDF( do_southwest )
{
   if( is_in_wilderness( ch ) )
      move_around_wilderness( ch, 7, ch->in_room->description );
   else
      move_char( ch, get_exit( ch->in_room, DIR_SOUTHWEST ), 0, false );
}

EXIT_DATA *find_door( CHAR_DATA *ch, char *arg, bool quiet )
{
   EXIT_DATA *pexit;
   int door = -1;

   if( !arg || !str_cmp( arg, "" ) )
      return NULL;

   pexit = NULL;

   switch( UPPER( arg[0] ) )
   {
      case 'D':
         if( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
            door = 5;
         break;

      case 'E':
         if( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
            door = 1;
         break;

      case 'N':
         if( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
            door = 0;
         if( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
            door = 6;
         if( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
            door = 7;
         break;

      case 'S':
         if( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
            door = 2;
         if( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
            door = 8;
         if( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
            door = 9;
         break;

      case 'U':
         if( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
            door = 4;
         break;

      case 'W':
         if( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
            door = 3;
         break;
   }

   if( door == -1 )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      {
         if( ( quiet || xIS_SET( pexit->exit_info, EX_ISDOOR ) ) && pexit->keyword && nifty_is_name( arg, pexit->keyword ) )
            return pexit;
      }
      if( !quiet )
         act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
      return NULL;
   }

   if( !( pexit = get_exit( ch->in_room, door ) ) )
   {
      if( !quiet )
         act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
      return NULL;
   }

   if( quiet )
      return pexit;

   if( xIS_SET( pexit->exit_info, EX_SECRET ) )
   {
      act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
      return NULL;
   }

   if( !xIS_SET( pexit->exit_info, EX_ISDOOR ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return NULL;
   }

   return pexit;
}

void set_bexit_flag( EXIT_DATA *pexit, int flag )
{
   EXIT_DATA *pexit_rev;

   xSET_BIT( pexit->exit_info, flag );
   if( ( pexit_rev = pexit->rexit ) && pexit_rev != pexit )
      xSET_BIT( pexit_rev->exit_info, flag );
}

void remove_bexit_flag( EXIT_DATA *pexit, int flag )
{
   EXIT_DATA *pexit_rev;

   xREMOVE_BIT( pexit->exit_info, flag );
   if( ( pexit_rev = pexit->rexit ) && pexit_rev != pexit )
      xREMOVE_BIT( pexit_rev->exit_info, flag );
}

CMDF( do_open )
{
   OBJ_DATA *obj;
   EXIT_DATA *pexit;
   char arg[MIL];
   int door;

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Open what?\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, arg, true ) ) )
   {
      /* 'open door' */
      EXIT_DATA *pexit_rev;

      if( xIS_SET( pexit->exit_info, EX_SECRET ) && pexit->keyword && !nifty_is_name( arg, pexit->keyword ) )
      {
         ch_printf( ch, "You see no %s here.\r\n", arg );
         return;
      }
      if( !xIS_SET( pexit->exit_info, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\r\n", ch );
         return;
      }
      if( !xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "It's already open.\r\n", ch );
         return;
      }
      if( xIS_SET( pexit->exit_info, EX_LOCKED ) && xIS_SET( pexit->exit_info, EX_BOLTED ) )
      {
         send_to_char( "The bolts locked.\r\n", ch );
         return;
      }
      if( xIS_SET( pexit->exit_info, EX_BOLTED ) )
      {
         send_to_char( "It's bolted.\r\n", ch );
         return;
      }
      if( xIS_SET( pexit->exit_info, EX_LOCKED ) )
      {
         send_to_char( "It's locked.\r\n", ch );
         return;
      }

      if( !xIS_SET( pexit->exit_info, EX_SECRET ) || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) )
      {
         act( AT_ACTION, "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
         act( AT_ACTION, "You open the $d.", ch, NULL, pexit->keyword, TO_CHAR );
         if( ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
         {
            CHAR_DATA *rch;

            for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_ACTION, "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
         }
         remove_bexit_flag( pexit, EX_CLOSED );
         if( ( door = pexit->vdir ) >= 0 && door < 10 )
            check_room_for_traps( ch, trap_door[door] );
         return;
      }
   }

   if( ( obj = get_obj_here( ch, arg ) ) )
   {
      /* 'open object' */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch_printf( ch, "%s is not a container.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch_printf( ch, "%s is already open.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
      {
         ch_printf( ch, "%s can't be opened or closed.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         ch_printf( ch, "%s is locked.\r\n", capitalize( obj->short_descr ) );
         return;
      }

      REMOVE_BIT( obj->value[1], CONT_CLOSED );
      act( AT_ACTION, "You open $p.", ch, obj, NULL, TO_CHAR );
      act( AT_ACTION, "$n opens $p.", ch, obj, NULL, TO_ROOM );
      oprog_open_trigger( ch, obj );
      if( !char_died( ch ) )
         check_for_trap( ch, obj, TRAP_OPEN );
      return;
   }

   ch_printf( ch, "You see no %s here.\r\n", arg );
}

CMDF( do_close )
{
   OBJ_DATA *obj;
   EXIT_DATA *pexit;
   char arg[MIL];
   int door;

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Close what?\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, arg, true ) ) )
   {
      /* 'close door' */
      EXIT_DATA *pexit_rev;

      if( !xIS_SET( pexit->exit_info, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\r\n", ch );
         return;
      }
      if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "It's already closed.\r\n", ch );
         return;
      }

      act( AT_ACTION, "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
      act( AT_ACTION, "You close the $d.", ch, NULL, pexit->keyword, TO_CHAR );

      /* close the other side */
      if( ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
      {
         CHAR_DATA *rch;

         xSET_BIT( pexit_rev->exit_info, EX_CLOSED );
         for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
            act( AT_ACTION, "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
      }
      set_bexit_flag( pexit, EX_CLOSED );
      if( ( door = pexit->vdir ) >= 0 && door < 10 )
         check_room_for_traps( ch, trap_door[door] );
      return;
   }

   if( ( obj = get_obj_here( ch, arg ) ) )
   {
      /* 'close object' */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch_printf( ch, "%s is not a container.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch_printf( ch, "%s is already closed.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
      {
         ch_printf( ch, "%s can't be opened or closed.\r\n", capitalize( obj->short_descr ) );
         return;
      }

      SET_BIT( obj->value[1], CONT_CLOSED );
      act( AT_ACTION, "You close $p.", ch, obj, NULL, TO_CHAR );
      act( AT_ACTION, "$n closes $p.", ch, obj, NULL, TO_ROOM );
      oprog_close_trigger( ch, obj );
      if( !char_died( ch ) )
         check_for_trap( ch, obj, TRAP_CLOSE );
      return;
   }

   ch_printf( ch, "You see no %s here.\r\n", arg );
}

/*
 * Keyring support added by Thoric
 * Idea suggested by Onyx <MtRicmer@worldnet.att.net> of Eldarion
 *
 * New: returns pointer to key/NULL instead of true/false
 *
 * If you want a feature like having immortals always have a key... you'll
 * need to code in a generic key, and make sure extract_obj doesn't extract it
 */
OBJ_DATA *has_key( CHAR_DATA *ch, int key )
{
   OBJ_DATA *obj, *obj2;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == key || ( obj->item_type == ITEM_KEY && obj->value[0] == key ) )
         return obj;
      else if( obj->item_type == ITEM_KEYRING )
      {
         for( obj2 = obj->first_content; obj2; obj2 = obj2->next_content )
            if( obj2->pIndexData->vnum == key || ( obj2->item_type == ITEM_KEY && obj2->value[0] == key ) )
               return obj2;
      }
   }

   return NULL;
}

CMDF( do_lock )
{
   OBJ_DATA *obj, *key;
   EXIT_DATA *pexit;
   char arg[MIL];
   int count;

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Lock what?\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, arg, true ) ) )
   {
      /* 'lock door' */
      if( !xIS_SET( pexit->exit_info, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\r\n", ch );
         return;
      }
      if( !xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\r\n", ch );
         return;
      }
      if( pexit->key < 0 )
      {
         send_to_char( "It can't be locked.\r\n", ch );
         return;
      }
      if( !( key = has_key( ch, pexit->key ) ) )
      {
         send_to_char( "You lack the key.\r\n", ch );
         return;
      }
      if( xIS_SET( pexit->exit_info, EX_LOCKED ) )
      {
         send_to_char( "It's already locked.\r\n", ch );
         return;
      }

      if( !xIS_SET( pexit->exit_info, EX_SECRET ) || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) )
      {
         send_to_char( "*Click*\r\n", ch );
         count = key->count;
         key->count = 1;
         act( AT_ACTION, "$n locks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
         key->count = count;
         set_bexit_flag( pexit, EX_LOCKED );
         return;
      }
   }

   if( ( obj = get_obj_here( ch, arg ) ) )
   {
      /* 'lock object' */
      if( obj->item_type != ITEM_CONTAINER )
      {
         send_to_char( "That's not a container.\r\n", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         send_to_char( "It's not closed.\r\n", ch );
         return;
      }
      if( obj->value[2] < 0 )
      {
         send_to_char( "It can't be locked.\r\n", ch );
         return;
      }
      if( !( key = has_key( ch, obj->value[2] ) ) )
      {
         send_to_char( "You lack the key.\r\n", ch );
         return;
      }
      if( IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already locked.\r\n", ch );
         return;
      }

      SET_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      count = key->count;
      key->count = 1;
      act( AT_ACTION, "$n locks $p with $P.", ch, obj, key, TO_ROOM );
      key->count = count;
      return;
   }

   ch_printf( ch, "You see no %s here.\r\n", arg );
}

CMDF( do_unlock )
{
   OBJ_DATA *obj, *key;
   EXIT_DATA *pexit;
   char arg[MIL];
   int count;

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Unlock what?\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, arg, true ) ) )
   {
      /* 'unlock door' */
      if( !xIS_SET( pexit->exit_info, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\r\n", ch );
         return;
      }
      if( !xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\r\n", ch );
         return;
      }
      if( pexit->key < 0 )
      {
         send_to_char( "It can't be unlocked.\r\n", ch );
         return;
      }
      if( !( key = has_key( ch, pexit->key ) ) )
      {
         send_to_char( "You lack the key.\r\n", ch );
         return;
      }
      if( !xIS_SET( pexit->exit_info, EX_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\r\n", ch );
         return;
      }

      if( !xIS_SET( pexit->exit_info, EX_SECRET ) || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) )
      {
         send_to_char( "*Click*\r\n", ch );
         count = key->count;
         key->count = 1;
         act( AT_ACTION, "$n unlocks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
         key->count = count;
         if( xIS_SET( pexit->exit_info, EX_EATKEY ) )
         {
            separate_obj( key );
            extract_obj( key );
         }
         remove_bexit_flag( pexit, EX_LOCKED );
         return;
      }
   }

   if( ( obj = get_obj_here( ch, arg ) ) )
   {
      /* 'unlock object' */
      if( obj->item_type != ITEM_CONTAINER )
      {
         send_to_char( "That's not a container.\r\n", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         send_to_char( "It's not closed.\r\n", ch );
         return;
      }
      if( obj->value[2] < 0 )
      {
         send_to_char( "It can't be unlocked.\r\n", ch );
         return;
      }
      if( !( key = has_key( ch, obj->value[2] ) ) )
      {
         send_to_char( "You lack the key.\r\n", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\r\n", ch );
         return;
      }

      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      count = key->count;
      key->count = 1;
      act( AT_ACTION, "$n unlocks $p with $P.", ch, obj, key, TO_ROOM );
      key->count = count;
      if( IS_SET( obj->value[1], CONT_EATKEY ) )
      {
         separate_obj( key );
         extract_obj( key );
      }
      return;
   }

   ch_printf( ch, "You see no %s here.\r\n", arg );
}

CMDF( do_bashdoor )
{
   EXIT_DATA *pexit;
   char *keyword = (char *)"wall";
   char arg[MIL];

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Bash what?\r\n", ch );
      return;
   }

   if( ch->fighting )
   {
      send_to_char( "You can't break off your fight.\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, arg, true ) ) )
   {
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit_rev;
      int schance;

      if( !xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "Calm down.  It is already open.\r\n", ch );
         return;
      }

      wait_state( ch, skill_table[gsn_bashdoor]->beats );

      if( !xIS_SET( pexit->exit_info, EX_SECRET ) )
         keyword = pexit->keyword;
      if( !is_npc( ch ) )
         schance = LEARNED( ch, gsn_bashdoor ) / 2;
      else
         schance = 90;
      if( xIS_SET( pexit->exit_info, EX_LOCKED ) )
         schance /= 3;

      if( !xIS_SET( pexit->exit_info, EX_BASHPROOF )
      && ch->move >= 15 && number_percent( ) < ( schance + 4 * ( get_curr_str( ch ) - 19 ) ) )
      {
         xREMOVE_BIT( pexit->exit_info, EX_CLOSED );
         if( xIS_SET( pexit->exit_info, EX_LOCKED ) )
            xREMOVE_BIT( pexit->exit_info, EX_LOCKED );
         xSET_BIT( pexit->exit_info, EX_BASHED );

         act( AT_SKILL, "Crash!  You bashed open the $d!", ch, NULL, keyword, TO_CHAR );
         act( AT_SKILL, "$n bashes open the $d!", ch, NULL, keyword, TO_ROOM );
         learn_from_success( ch, gsn_bashdoor );

         if( ( to_room = pexit->to_room ) && ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
         {
            CHAR_DATA *rch;

            xREMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
            if( xIS_SET( pexit_rev->exit_info, EX_LOCKED ) )
               xREMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
            xSET_BIT( pexit_rev->exit_info, EX_BASHED );

            for( rch = to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_SKILL, "The $d crashes open!", rch, NULL, pexit_rev->keyword, TO_CHAR );
         }
         damage( ch, ch, NULL, ( ch->max_hit / 20 ), gsn_bashdoor, true );
         return;
      }
   }

   act( AT_SKILL, "WHAAAAM!!!  You bash against the $d, but it doesn't budge.", ch, NULL, keyword, TO_CHAR );
   act( AT_SKILL, "WHAAAAM!!!  $n bashes against the $d, but it holds strong.", ch, NULL, keyword, TO_ROOM );
   damage( ch, ch, NULL, ( ch->max_hit / 20 ) + 10, gsn_bashdoor, true );
   learn_from_failure( ch, gsn_bashdoor );
}

CMDF( do_stand )
{
   switch( ch->position )
   {
      case POS_SLEEPING:
         if( IS_AFFECTED( ch, AFF_SLEEP ) )
         {
            send_to_char( "You can't seem to wake up!\r\n", ch );
            return;
         }

         send_to_char( "You wake and climb quickly to your feet.\r\n", ch );
         act( AT_ACTION, "$n arises from $s slumber.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_STANDING;
         break;

      case POS_RESTING:
         send_to_char( "You gather yourself and stand up.\r\n", ch );
         act( AT_ACTION, "$n rises from $s rest.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_STANDING;
         break;

      case POS_SITTING:
         send_to_char( "You move quickly to your feet.\r\n", ch );
         act( AT_ACTION, "$n rises up.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_STANDING;
         break;

      case POS_STANDING:
         send_to_char( "You're already standing.\r\n", ch );
         break;

      case POS_FIGHTING:
      case POS_EVASIVE:
      case POS_DEFENSIVE:
      case POS_AGGRESSIVE:
      case POS_BERSERK:
         send_to_char( "You're already fighting!\r\n", ch );
         break;
   }
}

CMDF( do_sit )
{
   switch( ch->position )
   {
      case POS_SLEEPING:
         if( IS_AFFECTED( ch, AFF_SLEEP ) )
         {
            send_to_char( "You can't seem to wake up!\r\n", ch );
            return;
         }

         send_to_char( "You wake and sit up.\r\n", ch );
         act( AT_ACTION, "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_SITTING;
         break;

      case POS_RESTING:
         send_to_char( "You stop resting and sit up.\r\n", ch );
         act( AT_ACTION, "$n stops resting and sits up.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_SITTING;
         break;

      case POS_STANDING:
         send_to_char( "You sit down.\r\n", ch );
         act( AT_ACTION, "$n sits down.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_SITTING;
         break;

      case POS_SITTING:
         send_to_char( "You're already sitting.\r\n", ch );
         return;

      case POS_FIGHTING:
      case POS_EVASIVE:
      case POS_DEFENSIVE:
      case POS_AGGRESSIVE:
      case POS_BERSERK:
         send_to_char( "You're busy fighting!\r\n", ch );
         return;

      case POS_MOUNTED:
         send_to_char( "You're already sitting - on your mount.\r\n", ch );
         return;
   }
}

CMDF( do_rest )
{
   switch( ch->position )
   {
      case POS_SLEEPING:
         if( IS_AFFECTED( ch, AFF_SLEEP ) )
         {
            send_to_char( "You can't seem to wake up!\r\n", ch );
            return;
         }

         send_to_char( "You rouse from your slumber.\r\n", ch );
         act( AT_ACTION, "$n rouses from $s slumber.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_RESTING;
         break;

      case POS_RESTING:
         send_to_char( "You're already resting.\r\n", ch );
         return;

      case POS_STANDING:
         send_to_char( "You sprawl out haphazardly.\r\n", ch );
         act( AT_ACTION, "$n sprawls out haphazardly.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_RESTING;
         break;

      case POS_SITTING:
         send_to_char( "You lie back and sprawl out to rest.\r\n", ch );
         act( AT_ACTION, "$n lies back and sprawls out to rest.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_RESTING;
         break;

      case POS_FIGHTING:
      case POS_EVASIVE:
      case POS_DEFENSIVE:
      case POS_AGGRESSIVE:
      case POS_BERSERK:
         send_to_char( "You're busy fighting!\r\n", ch );
         return;

      case POS_MOUNTED:
         send_to_char( "You'd better dismount first.\r\n", ch );
         return;
   }

   if( is_npc( ch ) && ch->mounter ) /* This char has a rider */
   {
      act( AT_SKILL, "You dismount $N.", ch->mounter, NULL, ch, TO_CHAR );
      xREMOVE_BIT( ch->act, ACT_MOUNTED );
      ch->mounter->position = POS_STANDING;
      ch->mounter->mount = NULL;
      ch->mounter = NULL;
   }

   rprog_rest_trigger( ch );
}

CMDF( do_sleep )
{
   switch( ch->position )
   {
      case POS_SLEEPING:
         send_to_char( "You're already sleeping.\r\n", ch );
         return;

      case POS_RESTING:
         if( ch->mental_state > 30 && ( number_percent( ) + 10 ) < ch->mental_state )
         {
            send_to_char( "You just can't seem to calm yourself down enough to sleep.\r\n", ch );
            act( AT_ACTION, "$n closes $s eyes for a few moments, but just can't seem to go to sleep.", ch, NULL, NULL,
                 TO_ROOM );
            return;
         }
         send_to_char( "You close your eyes and drift into slumber.\r\n", ch );
         act( AT_ACTION, "$n closes $s eyes and drifts into a deep slumber.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_SLEEPING;
         break;

      case POS_SITTING:
         if( ch->mental_state > 30 && ( number_percent( ) + 5 ) < ch->mental_state )
         {
            send_to_char( "You just can't seem to calm yourself down enough to sleep.\r\n", ch );
            act( AT_ACTION, "$n closes $s eyes for a few moments, but just can't seem to go to sleep.", ch, NULL, NULL,
                 TO_ROOM );
            return;
         }
         send_to_char( "You slump over and fall dead asleep.\r\n", ch );
         act( AT_ACTION, "$n nods off and slowly slumps over, dead asleep.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_SLEEPING;
         break;

      case POS_STANDING:
         if( ch->mental_state > 30 && number_percent( ) < ch->mental_state )
         {
            send_to_char( "You just can't seem to calm yourself down enough to sleep.\r\n", ch );
            act( AT_ACTION, "$n closes $s eyes for a few moments, but just can't seem to go to sleep.", ch, NULL, NULL,
                 TO_ROOM );
            return;
         }
         send_to_char( "You collapse into a deep sleep.\r\n", ch );
         act( AT_ACTION, "$n collapses into a deep sleep.", ch, NULL, NULL, TO_ROOM );
         ch->position = POS_SLEEPING;
         break;

      case POS_FIGHTING:
      case POS_EVASIVE:
      case POS_DEFENSIVE:
      case POS_AGGRESSIVE:
      case POS_BERSERK:
         send_to_char( "You're busy fighting!\r\n", ch );
         return;

      case POS_MOUNTED:
         send_to_char( "You really should dismount first.\r\n", ch );
         return;
   }

   if( is_npc( ch ) && ch->mounter ) /* This char has a rider */
   {
      act( AT_SKILL, "You dismount $N.", ch->mounter, NULL, ch, TO_CHAR );
      xREMOVE_BIT( ch->act, ACT_MOUNTED );
      ch->mounter->position = POS_STANDING;
      ch->mounter->mount = NULL;
      ch->mounter = NULL;
   }
   rprog_sleep_trigger( ch );
}

CMDF( do_wake )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      do_stand( ch, (char *)"" );
      return;
   }

   if( !is_awake( ch ) )
   {
      send_to_char( "You're asleep yourself!\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( is_awake( victim ) )
   {
      act( AT_PLAIN, "$N is already awake.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( IS_AFFECTED( victim, AFF_SLEEP ) || victim->position < POS_SLEEPING )
   {
      act( AT_PLAIN, "You can't seem to wake $M!", ch, NULL, victim, TO_CHAR );
      return;
   }

   act( AT_ACTION, "You wake $M.", ch, NULL, victim, TO_CHAR );
   victim->position = POS_STANDING;
   act( AT_ACTION, "$n wakes you.", ch, NULL, victim, TO_VICT );
   act( AT_ACTION, "$n wakes $N.", ch, NULL, victim, TO_ROOM );
}

/* teleport a character to another room */
void teleportch( CHAR_DATA *ch, ROOM_INDEX_DATA *room, bool show )
{
   char buf[MSL];

   if( room_is_private( room ) )
      return;
   act( AT_ACTION, "$n disappears suddenly!", ch, NULL, NULL, TO_ROOM );
   char_from_room( ch );
   char_to_room( ch, room );
   act( AT_ACTION, "$n arrives suddenly!", ch, NULL, NULL, TO_ROOM );
   if( show )
      do_look( ch, (char *)"auto" );
   if( xIS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !is_immortal( ch ) )
   {
      act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
      set_char_color( AT_DEAD, ch );
      send_to_char( "Oopsie... you're dead!\r\n", ch );
      snprintf( buf, sizeof( buf ), "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
      log_string( buf );
      to_channel( buf, "monitor", PERM_IMM );
      extract_char( ch, false );
   }
}

void teleport( CHAR_DATA *ch, int room, int flags )
{
   CHAR_DATA *nch, *nch_next;
   ROOM_INDEX_DATA *start = ch->in_room, *dest;
   bool show;

   if( !( dest = get_room_index( room ) ) )
   {
      bug( "%s: bad room vnum %d", __FUNCTION__, room );
      return;
   }

   /* No point in transporting them if already there */
   if( dest->vnum == start->vnum )
      return;

   if( IS_SET( flags, TELE_SHOWDESC ) )
      show = true;
   else
      show = false;

   if( !IS_SET( flags, TELE_TRANSALL ) )
   {
      teleportch( ch, dest, show );
      return;
   }

   /* teleport everybody in the room */
   for( nch = start->first_person; nch; nch = nch_next )
   {
      nch_next = nch->next_in_room;
      teleportch( nch, dest, show );
   }

   /* teleport the objects on the ground too */
   if( IS_SET( flags, TELE_TRANSALLPLUS ) )
   {
      OBJ_DATA *obj, *obj_next;

      for( obj = start->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         obj_from_room( obj );
         obj_to_room( obj, dest );
      }
   }
}

/* "Climb" in a certain direction. - Thoric */
CMDF( do_climb )
{
   EXIT_DATA *pexit;
   bool found;

   found = false;
   if( !argument || argument[0] == '\0' )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      {
         if( xIS_SET( pexit->exit_info, EX_xCLIMB ) )
         {
            move_char( ch, pexit, 0, false );
            return;
         }
      }
      send_to_char( "You can't climb here.\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) && xIS_SET( pexit->exit_info, EX_xCLIMB ) )
   {
      move_char( ch, pexit, 0, false );
      return;
   }
   send_to_char( "You can't climb there.\r\n", ch );
}

/* "enter" something (moves through an exit) - Thoric */
CMDF( do_enter )
{
   EXIT_DATA *pexit;
   bool found;

   if( is_in_wilderness( ch ) )
   {
      send_to_char( "You can't enter anything while in the wilderness.\r\n", ch );
      return;
   }

   found = false;
   if( !argument || argument[0] == '\0' )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      {
         if( xIS_SET( pexit->exit_info, EX_xENTER ) )
         {
            move_char( ch, pexit, 0, false );
            return;
         }
      }
      if( ch->in_room->sector_type != SECT_INSIDE && is_outside( ch ) )
      {
         for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
         {
            if( pexit->to_room
            && ( pexit->to_room->sector_type == SECT_INSIDE
            || xIS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) ) )
            {
               move_char( ch, pexit, 0, false );
               return;
            }
         }
      }
      send_to_char( "You can't find an entrance here.\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) && xIS_SET( pexit->exit_info, EX_xENTER ) )
   {
      move_char( ch, pexit, 0, false );
      return;
   }
   send_to_char( "You can't enter that.\r\n", ch );
}

/* Leave through an exit. - Thoric */
CMDF( do_leave )
{
   EXIT_DATA *pexit;
   bool found;

   if( is_in_wilderness( ch ) )
   {
      send_to_char( "You can't leave anything while in the wilderness.\r\n", ch );
      return;
   }

   found = false;
   if( !argument || argument[0] == '\0' )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      {
         if( xIS_SET( pexit->exit_info, EX_xLEAVE ) )
         {
            move_char( ch, pexit, 0, false );
            return;
         }
      }
      if( ch->in_room->sector_type == SECT_INSIDE || !is_outside( ch ) )
      {
         for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
         {
            if( pexit->to_room && pexit->to_room->sector_type != SECT_INSIDE
            && !xIS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) )
            {
               move_char( ch, pexit, 0, false );
               return;
            }
         }
      }
      send_to_char( "You can't find an exit here.\r\n", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) && xIS_SET( pexit->exit_info, EX_xLEAVE ) )
   {
      move_char( ch, pexit, 0, false );
      return;
   }
   send_to_char( "You can't leave that way.\r\n", ch );
}

/*
 * Check to see if an exit in the room is pulling (or pushing) players around.
 * Some types may cause damage.					-Thoric
 *
 * People kept requesting currents (like SillyMUD has), so I went all out
 * and added the ability for an exit to have a "pull" or a "push" force
 * and to handle different types much beyond a simple water current.
 *
 * This check is called by violence_update().  I'm not sure if this is the
 * best way to do it, or if it should be handled by a special queue.
 *
 * Future additions to this code may include equipment being blown away in
 * the wind (mostly headwear), and people being hit by flying objects
 *
 * TODO:
 *	handle more pulltypes
 *	give "entrance" messages for players and objects
 *	proper handling of player resistance to push/pulling
 */
ch_ret pullcheck( CHAR_DATA *ch, int pulse )
{
   ROOM_INDEX_DATA *room;
   EXIT_DATA *xtmp, *xit = NULL;
   OBJ_DATA *obj, *obj_next;
   const char *tochar = NULL, *toroom = NULL, *objmsg = NULL;
   const char *destrm = NULL, *destob = NULL;
   const char *dtxt = "somewhere";
   int pullfact, pull, resistance;
   bool move = false, moveobj = true, showroom = true;

   if( !( room = ch->in_room ) )
   {
      bug( "pullcheck: %s not in a room?!?", ch->name );
      return rNONE;
   }

   /* Find the exit with the strongest force (if any) */
   for( xtmp = room->first_exit; xtmp; xtmp = xtmp->next )
      if( xtmp->pull && xtmp->to_room && ( !xit || abs( xtmp->pull ) > abs( xit->pull ) ) )
         xit = xtmp;

   if( !xit )
      return rNONE;

   pull = xit->pull;

   /* strength also determines frequency */
   pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );

   /* strongest pull not ready yet... check for one that is */
   if( ( pulse % pullfact ) != 0 )
   {
      for( xit = room->first_exit; xit; xit = xit->next )
         if( xit->pull && xit->to_room )
         {
            pull = xit->pull;
            pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );
            if( ( pulse % pullfact ) != 0 )
               break;
         }

      if( !xit )
         return rNONE;
   }

   /* negative pull = push... get the reverse exit if any */
   if( pull < 0 )
      if( !( xit = get_exit( room, rev_dir[xit->vdir] ) ) )
         return rNONE;

   dtxt = rev_exit( xit->vdir );

   /*
    * First determine if the player should be moved or not
    * Check various flags, spells, the players position and strength vs.
    * the pull, etc... any kind of checks you like.
    */
   switch( xit->pulltype )
   {
      case PULL_CURRENT:
      case PULL_WHIRLPOOL:
         if( room->sector_type == SECT_WATER_SWIM || room->sector_type == SECT_WATER_NOSWIM )
            move = true;
         else if( room->sector_type == SECT_UNDERWATER || room->sector_type == SECT_OCEANFLOOR )
            move = true;
         break;

      case PULL_GEYSER:
      case PULL_WAVE:
         move = true;
         break;

      case PULL_WIND:
      case PULL_STORM:
         move = true;
         break;

      case PULL_COLDWIND:
         move = true;
         break;

      case PULL_HOTAIR:
         move = true;
         break;

      case PULL_BREEZE:
         move = false;
         break;

         /*
          * exits with these pulltypes should also be blocked from movement
          * ie: a secret locked pickproof door with the name "_sinkhole_", etc
          */
      case PULL_EARTHQUAKE:
      case PULL_SINKHOLE:
      case PULL_QUICKSAND:
      case PULL_LANDSLIDE:
      case PULL_SLIP:
      case PULL_LAVA:
         if( ( ch->mount && !is_floating( ch->mount ) ) || ( !ch->mount && !is_floating( ch ) ) )
            move = true;
         break;

         /* as if player moved in that direction him/herself */
      case PULL_UNDEFINED:
         return move_char( ch, xit, 0, false );

         /* all other cases ALWAYS move */
      default:
         move = true;
         break;
   }

   /* assign some nice text messages */
   switch( xit->pulltype )
   {
      case PULL_MYSTERIOUS:
         /* no messages to anyone */
         showroom = false;
         break;

      case PULL_WHIRLPOOL:
      case PULL_VACUUM:
         tochar = "You're sucked $T!";
         toroom = "$n is sucked $T!";
         destrm = "$n is sucked in from $T!";
         objmsg = "$p is sucked $T.";
         destob = "$p is sucked in from $T!";
         break;

      case PULL_CURRENT:
      case PULL_LAVA:
         tochar = "You drift $T.";
         toroom = "$n drifts $T.";
         destrm = "$n drifts in from $T.";
         objmsg = "$p drifts $T.";
         destob = "$p drifts in from $T.";
         break;

      case PULL_BREEZE:
         tochar = "You drift $T.";
         toroom = "$n drifts $T.";
         destrm = "$n drifts in from $T.";
         objmsg = "$p drifts $T in the breeze.";
         destob = "$p drifts in from $T.";
         break;

      case PULL_GEYSER:
      case PULL_WAVE:
         tochar = "You're pushed $T!";
         toroom = "$n is pushed $T!";
         destrm = "$n is pushed in from $T!";
         destob = "$p floats in from $T.";
         break;

      case PULL_EARTHQUAKE:
         tochar = "The earth opens up and you fall $T!";
         toroom = "The earth opens up and $n falls $T!";
         destrm = "$n falls from $T!";
         objmsg = "$p falls $T.";
         destob = "$p falls from $T.";
         break;

      case PULL_SINKHOLE:
         tochar = "The ground suddenly gives way and you fall $T!";
         toroom = "The ground suddenly gives way beneath $n!";
         destrm = "$n falls from $T!";
         objmsg = "$p falls $T.";
         destob = "$p falls from $T.";
         break;

      case PULL_QUICKSAND:
         tochar = "You begin to sink $T into the quicksand!";
         toroom = "$n begins to sink $T into the quicksand!";
         destrm = "$n sinks in from $T.";
         objmsg = "$p begins to sink $T into the quicksand.";
         destob = "$p sinks in from $T.";
         break;

      case PULL_LANDSLIDE:
         tochar = "The ground starts to slide $T, taking you with it!";
         toroom = "The ground starts to slide $T, taking $n with it!";
         destrm = "$n slides in from $T.";
         objmsg = "$p slides $T.";
         destob = "$p slides in from $T.";
         break;

      case PULL_SLIP:
         tochar = "You lose your footing!";
         toroom = "$n loses $s footing!";
         destrm = "$n slides in from $T.";
         objmsg = "$p slides $T.";
         destob = "$p slides in from $T.";
         break;

      case PULL_VORTEX:
         tochar = "You're sucked into a swirling vortex of colors!";
         toroom = "$n is sucked into a swirling vortex of colors!";
         toroom = "$n appears from a swirling vortex of colors!";
         objmsg = "$p is sucked into a swirling vortex of colors!";
         objmsg = "$p appears from a swirling vortex of colors!";
         break;

      case PULL_HOTAIR:
         tochar = "A blast of hot air blows you $T!";
         toroom = "$n is blown $T by a blast of hot air!";
         destrm = "$n is blown in from $T by a blast of hot air!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;

      case PULL_COLDWIND:
         tochar = "A bitter cold wind forces you $T!";
         toroom = "$n is forced $T by a bitter cold wind!";
         destrm = "$n is forced in from $T by a bitter cold wind!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;

      case PULL_WIND:
         tochar = "A strong wind pushes you $T!";
         toroom = "$n is blown $T by a strong wind!";
         destrm = "$n is blown in from $T by a strong wind!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;

      case PULL_STORM:
         tochar = "The raging storm drives you $T!";
         toroom = "$n is driven $T by the raging storm!";
         destrm = "$n is driven in from $T by a raging storm!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;

      default:
         if( pull > 0 )
         {
            tochar = "You're pulled $T!";
            toroom = "$n is pulled $T.";
            destrm = "$n is pulled in from $T.";
            objmsg = "$p is pulled $T.";
            objmsg = "$p is pulled in from $T.";
         }
         else
         {
            tochar = "You're pushed $T!";
            toroom = "$n is pushed $T.";
            destrm = "$n is pushed in from $T.";
            objmsg = "$p is pushed $T.";
            objmsg = "$p is pushed in from $T.";
         }
         break;
   }

   /* Do the moving */
   if( move )
   {
      /* display an appropriate exit message */
      if( tochar )
      {
         act( AT_PLAIN, tochar, ch, NULL, (char *)dir_name[xit->vdir], TO_CHAR );
         send_to_char( "\r\n", ch );
      }
      if( toroom )
         act( AT_PLAIN, toroom, ch, NULL, (char *)dir_name[xit->vdir], TO_ROOM );

      /* display an appropriate entrance message */
      if( destrm && xit->to_room->first_person )
      {
         act( AT_PLAIN, (char *)destrm, xit->to_room->first_person, NULL, (char *)dtxt, TO_CHAR );
         act( AT_PLAIN, (char *)destrm, xit->to_room->first_person, NULL, (char *)dtxt, TO_ROOM );
      }

      /* move the char */
      if( xit->pulltype == PULL_SLIP )
         return move_char( ch, xit, 1, false );
      char_from_room( ch );
      char_to_room( ch, xit->to_room );

      if( showroom )
         do_look( ch, (char *)"auto" );

      /* move the mount too */
      if( ch->mount )
      {
         char_from_room( ch->mount );
         char_to_room( ch->mount, xit->to_room );
         if( showroom )
            do_look( ch->mount, (char *)"auto" );
      }
   }

   /* move objects in the room */
   if( moveobj )
   {
      for( obj = room->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( is_obj_stat( obj, ITEM_BURIED ) || can_wear( obj, ITEM_NO_TAKE ) )
            continue;

         resistance = get_obj_weight( obj );
         if( is_obj_stat( obj, ITEM_METAL ) )
            resistance = ( resistance * 6 ) / 5;
         switch( obj->item_type )
         {
            case ITEM_SCROLL:
            case ITEM_TRASH:
               resistance >>= 2;
               break;

            case ITEM_SCRAPS:
            case ITEM_CONTAINER:
               resistance >>= 1;
               break;

            case ITEM_WAND:
               resistance = ( resistance * 5 ) / 6;
               break;

            case ITEM_CORPSE_PC:
            case ITEM_CORPSE_NPC:
            case ITEM_FOUNTAIN:
               resistance <<= 2;
               break;
         }

         /* is the pull greater than the resistance of the object? */
         if( ( abs( pull ) * 10 ) > resistance )
         {
            if( objmsg && room->first_person )
            {
               act( AT_PLAIN, (char *)objmsg, room->first_person, obj, (char *)dir_name[xit->vdir], TO_CHAR );
               act( AT_PLAIN, (char *)objmsg, room->first_person, obj, (char *)dir_name[xit->vdir], TO_ROOM );
            }
            if( destob && xit->to_room->first_person )
            {
               act( AT_PLAIN, (char *)destob, xit->to_room->first_person, obj, (char *)dtxt, TO_CHAR );
               act( AT_PLAIN, (char *)destob, xit->to_room->first_person, obj, (char *)dtxt, TO_ROOM );
            }
            obj_from_room( obj );
            obj_to_room( obj, xit->to_room );
         }
      }
   }
   return rNONE;
}

bool check_bolt( CHAR_DATA *ch, EXIT_DATA *pexit )
{
   if( !ch || !pexit )
      return false;
   if( !xIS_SET( pexit->exit_info, EX_ISDOOR ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return false;
   }
   if( !xIS_SET( pexit->exit_info, EX_CLOSED ) )
   {
      send_to_char( "It's not closed.\r\n", ch );
      return false;
   }
   if( !xIS_SET( pexit->exit_info, EX_ISBOLT ) )
   {
      send_to_char( "You don't see a bolt.\r\n", ch );
      return false;
   }
   return true;
}

/* This function bolts a door. Written by Blackmane */
CMDF( do_bolt )
{
   EXIT_DATA *pexit;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Bolt what?\r\n", ch );
      return;
   }
   if( !( pexit = find_door( ch, argument, true ) ) )
   {
      ch_printf( ch, "You see no %s here.\r\n", argument );
      return;
   }
   if( !check_bolt( ch, pexit ) )
      return;
   if( xIS_SET( pexit->exit_info, EX_BOLTED ) )
   {
      send_to_char( "It's already bolted.\r\n", ch );
      return;
   }
   if( !xIS_SET( pexit->exit_info, EX_SECRET ) || ( pexit->keyword && nifty_is_name( argument, pexit->keyword ) ) )
   {
      send_to_char( "*Clunk*\r\n", ch );
      act( AT_ACTION, "$n bolts the $d.", ch, NULL, pexit->keyword, TO_ROOM );
      set_bexit_flag( pexit, EX_BOLTED );
   }
}

/* This function unbolts a door.  Written by Blackmane */
CMDF( do_unbolt )
{
   EXIT_DATA *pexit;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Unbolt what?\r\n", ch );
      return;
   }
   if( !( pexit = find_door( ch, argument, true ) ) )
   {
      ch_printf( ch, "You see no %s here.\r\n", argument );
      return;
   }
   if( !check_bolt( ch, pexit ) )
      return;
   if( !xIS_SET( pexit->exit_info, EX_BOLTED ) )
   {
      send_to_char( "It's already unbolted.\r\n", ch );
      return;
   }
   if( !xIS_SET( pexit->exit_info, EX_SECRET )  || ( pexit->keyword && nifty_is_name( argument, pexit->keyword ) ) )
   {
      send_to_char( "*Clunk*\r\n", ch );
      act( AT_ACTION, "$n unbolts the $d.", ch, NULL, pexit->keyword, TO_ROOM );
      remove_bexit_flag( pexit, EX_BOLTED );
   }
}

CMDF( do_run )
{
   ROOM_INDEX_DATA *from_room, *prevroom;
   EXIT_DATA *pexit;
   char arg[MIL];
   ch_ret retcode;
   int amount = 0, x = 0, toroom;
   bool limited = false;

   if( !ch )
      return;
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Run where?\r\n", ch );
      return;
   }

   if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
   {
      send_to_char( "You are not in the correct position for that.\r\n", ch );
      return;
   }

   if( argument && argument[0] != '\0' && is_number( argument ) )
   {
      limited = true;
      amount = atoi( argument );
   }

   if( is_in_wilderness( ch ) )
   {
      send_to_char( "Sorry, you can't currently run in the wilderness.\r\n", ch );
      return;
   }

   from_room = ch->in_room;
   while( ( pexit = find_door( ch, arg, true ) ) != NULL )
   {
      if( pexit && xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         if( xIS_SET( pexit->exit_info, EX_SECRET ) || xIS_SET( pexit->exit_info, EX_DIG ) )
            send_to_char( "You can't go that way!\r\n", ch );
         else
            send_to_char( "You can't run through a door!\r\n", ch );
         break;
      }
      if( ch->move < 1 )
      {
         send_to_char( "You are too exhausted to run anymore.\r\n", ch );
         ch->move = 0;
         return;
      }
      if( ch->mount && ch->mount->move < 1 )
      {
         send_to_char( "Your mount is too exhausted to run anymore.\r\n", ch );
         ch->mount->move = 0;
         return;
      }
      toroom = pexit->vnum;
      prevroom = ch->in_room;
      retcode = move_char( ch, pexit, 0, true );

      if( retcode == rSTOP )
         break;

      if( retcode == rCHAR_DIED )
         return;

      aggr_room_update( ch, ch->in_room );

      if( ch->in_room && ch->in_room->vnum != toroom )
         return;
      if( ch->in_room == prevroom ) /* Didn't go anywhere? Don't want it getting in a loop sending back to the same room */
      {
         send_to_char( "You notice you are starting to run in circles and stop running.\r\n", ch );
         act( AT_ACTION, "$n notices $e is starting to run in circles and stops running.", ch, NULL, NULL, TO_ROOM );
         return;
      }
      if( ch->fighting )
      {
         act( AT_ACTION, "You are stopped dead in your tracks by $N.\r\n", ch, NULL, ch->fighting->who, TO_CHAR );
         act( AT_ACTION, "$n is stopped dead in $s tracks by $N.", ch, NULL, ch->fighting->who, TO_NOTVICT );
         do_look( ch, (char *)"auto" );
         return;
      }
      if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
         return;
      if( limited && ++x == amount )
         break;
   }

   if( ch->in_room == from_room )
   {
      send_to_char( "You try to run but don't get anywhere.\r\n", ch );
      act( AT_ACTION, "$n tries to run but doesn't get anywhere.", ch, NULL, NULL, TO_ROOM );
      return;
   }
   send_to_char( "You slow down after your run.\r\n", ch );
   act( AT_ACTION, "$n slows down after $s run.", ch, NULL, NULL, TO_ROOM );
   do_look( ch, (char *)"auto" );
}
