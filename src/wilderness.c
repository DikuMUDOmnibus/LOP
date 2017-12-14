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
 *                         Wilderness Code                                   *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "h/mud.h"

bool loc_in_wilderness; /* Used to know if they should enter the wilderness */
int loc_cords[2]; /* Used to get the cords for goto etc... */
extern int desccount;
extern int desccheck;
extern char showdescription[MSL];

bool should_follow_wilderness( CHAR_DATA *ch, CHAR_DATA *rch );

void put_in_wilderness( CHAR_DATA *ch, short x, short y )
{
   if( !ch )
      return;
   if( is_npc( ch ) )
      xSET_BIT( ch->act, ACT_WILDERNESS );
   else
      xSET_BIT( ch->act, PLR_WILDERNESS );
   ch->cords[0] = x;
   ch->cords[1] = y;
}

/* Allow them to move around the wilderness */
void move_around_wilderness( CHAR_DATA *ch, short dir, char *str )
{
   CHAR_DATA *fch, *nextinroom, *master;
   int x = 0, y = 0, chx = 0, chy = 0, moveloss = 0, sector = 0, oldchx, oldchy;

   if( !ch || !str || str[0] == '\0' )
      return;

   desccount = 0;
   desccheck = 0;

   chx = ch->cords[0];
   oldchx = ch->cords[0];
   chy = ch->cords[1];
   oldchy = ch->cords[1];

   if( dir == 0 ) /* North */
      chy--; /* Line before */
   else if( dir == 1 ) /* South */
      chy++; /* Line below */
   else if( dir == 2 ) /* East */
      chx++; /* Next character */
   else if( dir == 3 ) /* West */
      chx--; /* Previous character */
   else if( dir == 4 ) /* NE */
   {
      chy--; /* Line before */
      chx++; /* Next character */
   }
   else if( dir == 5 ) /* NW */
   {
      chy--; /* Line before */
      chx--; /* Previous character */
   }
   else if( dir == 6 ) /* SE */
   {
      chy++; /* Line below */
      chx++; /* Next character */
   }
   else if( dir == 7 ) /* SW */
   {
      chy++; /* Line below */
      chx--; /* Previous character */
   }

   if( chy < 0 || chx < 0 )
   {
      send_to_char( "You can't go that way.\r\n", ch );
      return;
   }

   /* Parse the description and add in color etc... for where they are */
   for( ; *str != '\0'; )
   {
      if( *str != '\r' && *str != '\n' && x == chx && y == chy ) /* Ok so it is there and valid */
      {
         /* These you can't go to */
         if( *str == ' ' )
         {
            send_to_char( "You can't go that way.\r\n", ch );
            return;
         }

         /* Since we have the character now lets set up the movement loss */
         /* Only loose movement if not on a mount */
         if( *str == '^' ) /* Mountain */
            sector = SECT_MOUNTAIN;
         else if( *str == '*' ) /* Field */
            sector = SECT_FIELD;
         else if( *str == '~' )
            sector = SECT_WATER_SWIM;
         else /* Default of the Inside */
            sector = SECT_INSIDE;

         moveloss = 1;

         if( ( ch->mount && ( !should_follow_wilderness( ch, ch->mount ) || !is_floating( ch->mount ) ) ) || !is_floating( ch ) )
            moveloss = movement_loss[ URANGE( 0, sector, SECT_MAX - 1 ) ];;

         if( ch->mount && should_follow_wilderness( ch, ch->mount ) )
         {
            if( ch->mount->move < moveloss )
            {
               send_to_char( "Your mount is to exhausted.\r\n", ch );
               return;
            }
            move_around_wilderness( ch->mount, dir, ch->in_room->description );
         }
         else /* Only take movement if no mount since mount will loose when it is done */
         {
            if( ch->move < moveloss )
            {
               send_to_char( "Your to exhausted.\r\n", ch );
               return;
            }
            ch->move -= moveloss;
         }

         ch->cords[0] = chx;
         ch->cords[1] = chy;
         do_look( ch, (char *)"auto" );
         ch->cords[0] = oldchx;
         ch->cords[1] = oldchy;

         for( fch = ch->in_room->first_person; fch; fch = nextinroom )
         {
            nextinroom = fch->next_in_room;

            if( fch == ch || !fch->master )
               continue;

            master = fch->master;
            while( master->master )
               master = master->master;

            if( master != ch || xIS_SET( ch->act, PLR_SOLO ) || xIS_SET( fch->act, PLR_SOLO ) )
               continue;

            if( fch->position != POS_STANDING && fch->position != POS_MOUNTED )
               continue;

            if( !should_follow_wilderness( ch, fch ) )
               continue;

            act( AT_ACTION, "You follow $N.", fch, NULL, master, TO_CHAR );
            move_around_wilderness( fch, dir, fch->in_room->description );
         }

         ch->cords[0] = chx;
         ch->cords[1] = chy;

         return;
      }

      /* Should do this after the other checks */
      if( *str != '\r' && *str != '\n' )
         x++;
      if( *str == '\n' )
      {
         x = 0;
         y++;
      }

      str++;
   }

   send_to_char( "You can't go that way.\r\n", ch );
}

void parse_wilderness_description( CHAR_DATA *ch, char *str )
{
   int x = 0, y = 0;

   if( !ch || !str || str[0] == '\0' )
      return;

   desccount = 0;
   desccheck = 0;

   /* Parse the description and add in color etc... for where they are */
   for( ; *str != '\0'; )
   {
      if( x == ch->cords[0] && y == ch->cords[1] && is_in_wilderness( ch ) )
      {
         showdescription[desccount++] = '&';
         showdescription[desccount++] = 'r';
         if( *str == '&' || *str == '^' || *str == '}' )
            showdescription[desccount++] = *str;
         showdescription[desccount++] = *str;
         showdescription[desccount++] = '&';
         showdescription[desccount++] = 'D';
      }
      else
      {
         if( *str == '&' || *str == '^' || *str == '}' )
            showdescription[desccount++] = *str;
         showdescription[desccount++] = *str;
      }

      if( *str != '\n' && *str != '\r' )
         x++;
      if( *str == '\n' )
      {
         x = 0;
         y++;
      }

      str++;
   }
   showdescription[desccount] = '\0';

   send_to_char( showdescription, ch );
}

bool is_same_cords( OBJ_DATA *obj, CHAR_DATA *ch )
{
   if( !obj || !ch )
      return false;
   if( !xIS_SET( obj->extra_flags, ITEM_WILDERNESS ) && !is_in_wilderness( ch ) )
      return true;
   if( xIS_SET( obj->extra_flags, ITEM_WILDERNESS ) && is_in_wilderness( ch )
   && obj->cords[0] == ch->cords[0] && obj->cords[1] == ch->cords[1] )
      return true;
   return false;
}

bool should_follow_wilderness( CHAR_DATA *ch, CHAR_DATA *rch )
{
   if( !ch || !rch )
      return false;

   if( is_in_wilderness( ch ) )
   {
      if( is_in_wilderness( rch ) && ch->cords[0] == rch->cords[0] && ch->cords[1] == rch->cords[1] )
         return true;
   }

   if( !is_in_wilderness( ch ) )
   {
      if( !is_in_wilderness( rch ) )
         return true;
   }

   return false;
}

CMDF( do_wilderness )
{
   CHAR_DATA *fch, *nextinroom, *master;

   if( !ch || !ch->in_room )
      return;

   if( !xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
   {
      /* Just incase for some reason not in the room any more */
      if( is_npc( ch ) )
         xREMOVE_BIT( ch->act, ACT_WILDERNESS );
      else
         xREMOVE_BIT( ch->act, PLR_WILDERNESS );
      ch->cords[0] = 0;
      ch->cords[1] = 0;
      send_to_char( "You aren't currently in a wilderness room.\r\n", ch );
      return;
   }

   if( ch->mount && should_follow_wilderness( ch, ch->mount ) )
      do_wilderness( ch->mount, (char *)"" );

   for( fch = ch->in_room->first_person; fch; fch = nextinroom )
   {
      nextinroom = fch->next_in_room;
 
      if( fch == ch || !fch->master )
         continue;

      master = fch->master;
      while( master->master )
         master = master->master;

      if( master != ch || xIS_SET( ch->act, PLR_SOLO ) || xIS_SET( fch->act, PLR_SOLO ) )
         continue;

      if( fch->position != POS_STANDING && fch->position != POS_MOUNTED )
         continue;

      if( !should_follow_wilderness( ch, fch ) )
         continue;

      do_wilderness( fch, (char *)"" );
   }

   ch->cords[0] = 0;
   ch->cords[1] = 0;

   if( is_npc( ch ) )
   {
      xTOGGLE_BIT( ch->act, ACT_WILDERNESS );
      if( xIS_SET( ch->act, ACT_WILDERNESS ) )
         send_to_char( "You enter the wilderness.\r\n", ch );
      else
         send_to_char( "You emerge from the wilderness.\r\n", ch );
   }
   else
   {
      xTOGGLE_BIT( ch->act, PLR_WILDERNESS );
      if( xIS_SET( ch->act, PLR_WILDERNESS ) )
         send_to_char( "You enter the wilderness.\r\n", ch );
      else
         send_to_char( "You emerge from the wilderness.\r\n", ch );
   }
}

void obj_to_char_cords( OBJ_DATA *obj, CHAR_DATA *ch )
{
   if( !obj || !ch || !ch->in_room )
      return;
   if( !xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
      return;
   if( !is_in_wilderness( ch ) )
      return;
   /* Should be good to set it now */
   xSET_BIT( obj->extra_flags, ITEM_WILDERNESS );
   obj->cords[0] = ch->cords[0];
   obj->cords[1] = ch->cords[1];
}

bool can_see_character( CHAR_DATA *ch, CHAR_DATA *rch )
{
   if( !ch || !rch || !ch->in_room || !rch->in_room )
      return false;

   if( !xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
      return true;

   /* If not set to wilderness */
   if( !is_in_wilderness( ch ) )
   {
      /* This one is so just skip them */
      if( is_in_wilderness( rch ) )
         return false;
   }

   /* Is set to wilderness */
   if( is_in_wilderness( ch ) )
   {
      /* This one isn't so just skip them */
      if( !is_in_wilderness( rch ) )
         return false;

      /* If not in the same spot continue */
      if( ch->cords[0] != rch->cords[0] || ch->cords[1] != rch->cords[1] )
         return false;
   }

   return true;
}

bool is_in_wilderness( CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return false;
   }
   if( !ch->in_room || !xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
      return false;
   if( is_npc( ch ) && xIS_SET( ch->act, ACT_WILDERNESS ) )
      return true;
   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_WILDERNESS ) )
      return true;
   return false;
}

/* Lets set them to the cords if we need to */
void set_loc_cords( CHAR_DATA *ch )
{
   if( !loc_in_wilderness || !ch )
      return;
   if( is_npc( ch ) )
      xSET_BIT( ch->act, ACT_WILDERNESS );
   else
      xSET_BIT( ch->act, PLR_WILDERNESS );
   ch->cords[0] = loc_cords[0];
   ch->cords[1] = loc_cords[1];
}

void char_to_char_cords( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !ch || !victim )
      return;
   if( !is_in_wilderness( ch ) )
      return;
   if( is_npc( ch ) )
      xSET_BIT( victim->act, ACT_WILDERNESS );
   else
      xSET_BIT( victim->act, PLR_WILDERNESS );
   victim->cords[0] = ch->cords[0];
   victim->cords[1] = ch->cords[1];
}
