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
 *			Specific object creation module			     *
 *****************************************************************************/

#include <stdio.h>
#include "h/mud.h"

/* Make a trap. */
OBJ_DATA *make_trap( int v0, int v1, int v2, int v3 )
{
   OBJ_INDEX_DATA *otrap;
   OBJ_DATA *trap;

   if( !( otrap = get_obj_index( OBJ_VNUM_TRAP ) ) )
   {
      bug( "%s: object vnum %d doesn't exist.", __FUNCTION__, OBJ_VNUM_TRAP );
      return NULL;
   }

   if( !( trap = create_object( otrap, 0 ) ) )
   {
      bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_TRAP );
      return NULL;
   }
   trap->timer = 0;
   trap->value[0] = v0;
   trap->value[1] = v1;
   trap->value[2] = v2;
   trap->value[3] = v3;
   return trap;
}

/* Turn an object into scraps. */
void make_scraps( OBJ_DATA *obj )
{
   char buf[MSL];
   OBJ_DATA *scraps;
   CHAR_DATA *ch = NULL;

   separate_obj( obj );
   if( !( scraps = create_object( get_obj_index( OBJ_VNUM_SCRAPS ), 0 ) ) )
   {
      bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_SCRAPS );
      return;
   }
   scraps->timer = number_range( 5, 15 );

   /* don't make scraps of scraps of scraps of ... */
   if( obj->pIndexData->vnum == OBJ_VNUM_SCRAPS )
   {
      STRSET( scraps->short_descr, "some debris" );
      STRSET( scraps->description, "Bits of debris lie on the ground here." );
   }
   else
   {
      if( obj->short_descr )
      {
         if( scraps->short_descr )
         {
            snprintf( buf, sizeof( buf ), scraps->short_descr, obj->short_descr );
            STRSET( scraps->short_descr, buf );
         }
         else
            scraps->short_descr = STRALLOC( obj->short_descr );
         if( scraps->description )
         {
            snprintf( buf, sizeof( buf ), scraps->description, obj->short_descr );
            STRSET( scraps->description, buf );
         }
         else
            scraps->description = STRALLOC( obj->short_descr );
      }
   }

   if( obj->carried_by )
   {
      ch = obj->carried_by;
      act( AT_OBJECT, "$p falls to the ground in scraps!", obj->carried_by, obj, NULL, TO_CHAR );
      obj_to_room( scraps, obj->carried_by->in_room );
   }
   else if( obj->in_room )
   {
      if( ( ch = obj->in_room->first_person ) )
      {
         act( AT_OBJECT, "$p is reduced to little more than scraps.", ch, obj, NULL, TO_ROOM );
         act( AT_OBJECT, "$p is reduced to little more than scraps.", ch, obj, NULL, TO_CHAR );
      }
      obj_to_room( scraps, obj->in_room );
   }
   if( ( obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_KEYRING
   || obj->item_type == ITEM_QUIVER || obj->item_type == ITEM_CORPSE_PC ) && obj->first_content )
   {
      if( ch && ch->in_room )
      {
         act( AT_OBJECT, "The contents of $p fall to the ground.", ch, obj, NULL, TO_ROOM );
         act( AT_OBJECT, "The contents of $p fall to the ground.", ch, obj, NULL, TO_CHAR );
      }
      if( obj->carried_by )
         empty_obj( obj, NULL, obj->carried_by->in_room );
      else if( obj->in_room )
         empty_obj( obj, NULL, obj->in_room );
      else if( obj->in_obj )
         empty_obj( obj, obj->in_obj, NULL );
   }
   oprog_scrap_trigger( ch, obj );
   extract_obj( obj );
}

/* Make a corpse out of a character. */
OBJ_DATA *make_corpse( CHAR_DATA *ch, CHAR_DATA *killer )
{
   char buf[MSL], *name;
   OBJ_DATA *corpse, *obj, *obj_next;
   int rest = 0;

   if( is_npc( ch ) )
   {
      name = ch->short_descr;
      if( !( corpse = create_object( get_obj_index( OBJ_VNUM_CORPSE_NPC ), 0 ) ) )
      {
         bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_CORPSE_NPC );
         return NULL;
      }
      handle_mwreset( corpse );
      corpse->timer = 6;
      rest = ch->gold;
      ch->gold = 0;
      obj_to_obj( create_money( rest ), corpse );

      corpse->cost = ( -( int )ch->pIndexData->vnum );
      corpse->value[2] = corpse->timer;
      if( !xIS_SET( ch->act, ACT_NOSLICE ) )
         corpse->value[0] = 1; /* How many slices do you wish to allow per corpse? */
      corpse->value[1] = ch->pIndexData->vnum;
   }
   else
   {
      name = ch->name;
      if( !( corpse = create_object( get_obj_index( OBJ_VNUM_CORPSE_PC ), 0 ) ) )
      {
         bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_CORPSE_PC );
         return NULL;
      }
      if( in_arena( ch ) )
         corpse->timer = 0;
      else
         corpse->timer = 40;
      corpse->value[2] = ( int )( corpse->timer / 8 );
      corpse->value[4] = ch->level;
      if( can_pkill( ch ) && sysdata.pk_loot )
         xSET_BIT( corpse->extra_flags, ITEM_CLANCORPSE );
      /*
       * Pkill corpses get save timers, in ticks (approx 70 seconds)
       * This should be anough for the killer to type 'get all corpse'. 
       */
      if( !is_npc( ch ) && !is_npc( killer ) && ch != killer )
         corpse->value[3] = 1;
      else
         corpse->value[3] = 0;
   }

   if( ch != killer )
   {
      snprintf( buf, sizeof( buf ), "%s", killer->name );
      STRSET( corpse->action_desc, buf );
   }

   snprintf( buf, sizeof( buf ), "corpse %s", name );
   STRSET( corpse->name, buf );

   if( corpse->short_descr )
   {
      snprintf( buf, sizeof( buf ), corpse->short_descr, name );
      STRSET( corpse->short_descr, buf );
   }
   else
      corpse->short_descr = STRALLOC( name );

   if( corpse->description )
   {
      snprintf( buf, sizeof( buf ), corpse->description, name );
      STRSET( corpse->description, buf );
   }
   else
      corpse->description = STRALLOC( name );

   for( obj = ch->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;
      obj_from_char( obj );
      if( is_obj_stat( obj, ITEM_INVENTORY ) || is_obj_stat( obj, ITEM_DEATHROT )
      || ( is_npc( ch ) && xIS_SET( ch->act, ACT_AUTOPURGE ) ) ) /* Instead of having to use programs to purge inventory on death */
         extract_obj( obj );
      else
      {
         if( is_obj_stat( obj, ITEM_PIERCED ) )
            xREMOVE_BIT( obj->extra_flags, ITEM_PIERCED );
         if( is_obj_stat( obj, ITEM_LODGED ) ) /* Probably better to remove these instead of unlodging and letting them get them */
         {
            xREMOVE_BIT( obj->extra_flags, ITEM_LODGED );
            extract_obj( obj );
            continue;
         }
         obj_to_obj( obj, corpse );
      }
   }

   obj_to_char_cords( corpse, ch );
   return obj_to_room( corpse, ch->in_room );
}

void make_blood( CHAR_DATA *ch )
{
   OBJ_DATA *obj;

   if( !( obj = create_object( get_obj_index( OBJ_VNUM_BLOOD ), 0 ) ) )
   {
      bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_BLOOD );
      return;
   }
   obj->timer = number_range( 2, 4 );
   obj->value[1] = number_range( 3, UMIN( 5, ch->level ) );
   obj_to_room( obj, ch->in_room );
   obj_to_char_cords( obj, ch );
}

void make_bloodstain( OBJ_DATA *obj, ROOM_INDEX_DATA *room )
{
   OBJ_INDEX_DATA *bindex;
   OBJ_DATA *bstain;

   if( !room )
   {
      bug( "%s: NULL room", __FUNCTION__ );
      return;
   }
   if( !( bindex = get_obj_index( OBJ_VNUM_BLOODSTAIN ) ) )
   {
      bug( "%s: couldn't find object vnum %d", __FUNCTION__, OBJ_VNUM_BLOODSTAIN );
      return;
   }
   if( !( bstain = create_object( bindex, 0 ) ) )
   {
      bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_BLOODSTAIN );
      return;
   }
   bstain->timer = number_range( 1, 3 );
   obj_to_room( bstain, room );
   if( obj && is_obj_stat( obj, ITEM_WILDERNESS ) )
   {
      xSET_BIT( bstain->extra_flags, ITEM_WILDERNESS );
      bstain->cords[0] = obj->cords[0];
      bstain->cords[1] = obj->cords[1];
   }
}

void set_money( OBJ_DATA *obj, int amount )
{
   char buf[MSL], *bignum;

   bignum = num_punct( amount );

   if( obj->pIndexData->short_descr )
   {
      snprintf( buf, sizeof( buf ), obj->pIndexData->short_descr, bignum );
      STRSET( obj->short_descr, buf );
   }
   else
   {
      snprintf( buf, sizeof( buf ), "%s gold coins", bignum );
      obj->short_descr = STRALLOC( buf );
   }

   if( obj->pIndexData->description )
   {
      snprintf( buf, sizeof( buf ), obj->pIndexData->description, bignum );
      STRSET( obj->description, buf );
   }
   else
   {
      snprintf( buf, sizeof( buf ), "%s gold coins", bignum );
      obj->description = STRALLOC( buf );
   }

   obj->value[0] = amount;
}

/* make some coinage */
OBJ_DATA *create_money( int amount )
{
   OBJ_DATA *obj;

   if( amount == 0 ) /* No point in creating nothing */
      return NULL;

   if( amount < 0 )
   {
      bug( "%s: negative money (Amount)%d.", __FUNCTION__, amount );
      amount = 1;
   }

   if( amount == 1 )
   {
      if( !( obj = create_object( get_obj_index( OBJ_VNUM_MONEY_ONE ), 0 ) ) )
      {
         bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_MONEY_ONE );
         return NULL;
      }
   }
   else
   {
      if( !( obj = create_object( get_obj_index( OBJ_VNUM_MONEY_SOME ), 0 ) ) )
      {
         bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_MONEY_SOME );
         return NULL;
      }

      set_money( obj, amount );
   }

   return obj;
}
