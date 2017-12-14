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
 *	    Misc module for general commands: not skills or spells	     *
 *****************************************************************************
 * Note: Most of the stuff in here would go in act_obj.c, but act_obj was    *
 * getting big.								     *
 *****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "h/mud.h"

/*
 * Fill a container
 * Many enhancements added by Thoric (ie: filling non-drink containers)
 */
CMDF( do_fill )
{
   char arg1[MIL], arg2[MIL];
   OBJ_DATA *obj, *source;
   int diff = 0;
   short dest_item, src_item1, src_item2, src_item3;
   bool all = false;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Fill what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You don't have that item.\r\n", ch );
      return;
   }
   else
      dest_item = obj->item_type;

   src_item1 = src_item2 = src_item3 = -1;
   switch( dest_item )
   {
      default:
         act( AT_ACTION, "$n tries to fill $p... (Don't ask me how)", ch, obj, NULL, TO_ROOM );
         send_to_char( "You can't fill that.\r\n", ch );
         return;

      /* place all fillable item types here */
      case ITEM_DRINK_CON:
         src_item1 = ITEM_FOUNTAIN;
         src_item2 = ITEM_BLOOD;
         break;

      case ITEM_HERB_CON:
      case ITEM_PIPE:
         src_item1 = ITEM_HERB;
         src_item2 = ITEM_HERB_CON;
         break;

      case ITEM_CONTAINER:
         src_item1 = ITEM_CONTAINER;
         src_item2 = ITEM_CORPSE_NPC;
         src_item3 = ITEM_CORPSE_PC;
         break;
   }

   if( dest_item == ITEM_CONTAINER )
   {
      if( IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, obj->name, TO_CHAR );
         return;
      }
      if( get_real_obj_weight( obj ) / obj->count >= obj->value[0] )
      {
         send_to_char( "It's already full as it can be.\r\n", ch );
         return;
      }
   }
   else
   {
      diff = obj->value[0] - obj->value[1];
      if( diff < 1 || obj->value[1] >= obj->value[0] )
      {
         send_to_char( "It's already full as it can be.\r\n", ch );
         return;
      }
   }

   if( dest_item == ITEM_PIPE && IS_SET( obj->value[3], PIPE_FULLOFASH ) )
   {
      send_to_char( "It's full of ashes, and needs to be emptied first.\r\n", ch );
      return;
   }

   if( arg2 != NULL && arg2[0] != '\0' )
   {
      if( dest_item == ITEM_CONTAINER && ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) ) )
      {
         all = true;
         source = NULL;
      }
      else if( dest_item == ITEM_PIPE )
      {
         if( !( source = get_obj_carry( ch, arg2 ) ) )
         {
            send_to_char( "You don't have that item.\r\n", ch );
            return;
         }
         if( source->item_type != src_item1 && source->item_type != src_item2 && source->item_type != src_item3 )
         {
            act( AT_PLAIN, "You can't fill $p with $P!", ch, obj, source, TO_CHAR );
            return;
         }
      }
      else
      {
         int ocheck = 0;

         if( !str_cmp( argument, "room" ) )
            ocheck = 1;
         else if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
            ocheck = 2;
         else if( !str_cmp( argument, "worn" ) )
            ocheck = 3;

         if( !( source = new_get_obj_here( ocheck, ch, arg2 ) ) )
         {
            send_to_char( "You can't find that item.\r\n", ch );
            return;
         }
      }
   }
   else
      source = NULL;

   if( !source && dest_item == ITEM_PIPE )
   {
      send_to_char( "Fill it with what?\r\n", ch );
      return;
   }

   if( !source )
   {
      bool found = false;
      OBJ_DATA *src_next;

      found = false;
      separate_obj( obj );
      for( source = ch->in_room->first_content; source; source = src_next )
      {
         src_next = source->next_content;
         if( dest_item == ITEM_CONTAINER )
         {
            if( can_wear( source, ITEM_NO_TAKE )
            || is_obj_stat( source, ITEM_BURIED )
            || ( is_obj_stat( source, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
            || ch->carry_weight + get_obj_weight( source ) > can_carry_w( ch )
            || ( get_real_obj_weight( source ) + get_real_obj_weight( obj ) / obj->count ) > obj->value[0] )
               continue;

            if( all && arg2[3] == '.' && !nifty_is_name( &arg2[4], source->name ) )
               continue;

            if( source->item_type == ITEM_MONEY )
            {
               int ncount = source->count, uamount;
               bool dextract = false;

               if( source->value[0] < 0 || source->value[1] < 0 )
               {
                  send_to_char( "Getting this would decrease the amount of gold you have.\r\n", ch );
                  return;
               }

               while( ncount > 0 )
               {
                  if( !( can_hold_gold( ch, source->value[0] ) ) )
                  {
                     ncount--;
                     dextract = true;
                     continue;
                  }

                  if( --source->count < 1 )
                     source->count = 1;

                  uamount = source->value[0];

                  increase_gold( ch, uamount );

                  if( xIS_SET( ch->act, PLR_AUTOSPLIT ) )
                  {
                     char buf1[MSL];

                     snprintf( buf1, sizeof( buf1 ), "split auto %d", source->value[0] );
                     interpret( ch, buf1 );
                  }
                  ncount--;
               }

               obj_from_room( source );

               if( !dextract )
                  extract_obj( source );
               else
                  obj_to_obj( source, obj );
            }
            else
            {
               obj_from_room( source );
               obj_to_obj( source, obj );
            }
            found = true;
         }
         else if( source->item_type == src_item1 || source->item_type == src_item2 || source->item_type == src_item3 )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         switch( src_item1 )
         {
            default:
               send_to_char( "There is nothing appropriate here!\r\n", ch );
               return;

            case ITEM_FOUNTAIN:
               send_to_char( "There is no fountain here!\r\n", ch );
               return;

            case ITEM_BLOOD:
               send_to_char( "There is no blood here!\r\n", ch );
               return;

            case ITEM_HERB_CON:
               send_to_char( "There are no herbs here!\r\n", ch );
               return;

            case ITEM_HERB:
               send_to_char( "You can't find any smoking herbs.\r\n", ch );
               return;
         }
      }
      if( dest_item == ITEM_CONTAINER )
      {
         act( AT_ACTION, "You fill $p.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n fills $p.", ch, obj, NULL, TO_ROOM );
         return;
      }
   }

   if( dest_item == ITEM_CONTAINER )
   {
      OBJ_DATA *otmp, *otmp_next;
      CHAR_DATA *gch;
      char name[MIL], *pd;
      bool found = false;

      if( source == obj )
      {
         send_to_char( "You can't fill something with itself!\r\n", ch );
         return;
      }

      switch( source->item_type )
      {
         default: /* put something in container */
            if( !source->in_room /* disallow inventory items */
            || can_wear( source, ITEM_NO_TAKE )
            || ( is_obj_stat( source, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
            || ( ch->carry_weight + get_obj_weight( source ) ) > can_carry_w( ch )
            || ( get_real_obj_weight( source ) + get_real_obj_weight( obj ) / obj->count ) > obj->value[0] )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            separate_obj( obj );
            act( AT_ACTION, "You take $P and put it inside $p.", ch, obj, source, TO_CHAR );
            act( AT_ACTION, "$n takes $P and puts it inside $p.", ch, obj, source, TO_ROOM );
            obj_from_room( source );
            obj_to_obj( source, obj );
            break;

         case ITEM_MONEY:
            send_to_char( "You can't do that... yet.\r\n", ch );
            break;

         case ITEM_CORPSE_PC:
            if( is_npc( ch ) )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            if( is_obj_stat( source, ITEM_CLANCORPSE ) && !is_immortal( ch ) )
            {
               send_to_char( "Your hands fumble.  Maybe you better loot a different way.\r\n", ch );
               return;
            }
            if( !is_obj_stat( source, ITEM_CLANCORPSE ) || !xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
            {
               pd = source->short_descr;
               pd = one_argument( pd, name );
               pd = one_argument( pd, name );
               pd = one_argument( pd, name );
               pd = one_argument( pd, name );

               if( str_cmp( name, ch->name ) && !is_immortal( ch ) )
               {
                  bool fGroup;

                  fGroup = false;
                  for( gch = first_char; gch; gch = gch->next )
                  {
                     if( !is_npc( gch ) && is_same_group( ch, gch ) && !str_cmp( name, gch->name ) )
                     {
                        fGroup = true;
                        break;
                     }
                  }
                  if( !fGroup )
                  {
                     send_to_char( "That's someone else's corpse.\r\n", ch );
                     return;
                  }
               }
            }

         case ITEM_CONTAINER:
            if( source->item_type == ITEM_CONTAINER   /* don't remove */
            && IS_SET( source->value[1], CONT_CLOSED ) )
            {
               act( AT_PLAIN, "The $d is closed.", ch, NULL, source->name, TO_CHAR );
               return;
            }

         case ITEM_CORPSE_NPC:
            if( !( otmp = source->first_content ) )
            {
               send_to_char( "It's empty.\r\n", ch );
               return;
            }
            separate_obj( obj );
            for( ; otmp; otmp = otmp_next )
            {
               otmp_next = otmp->next_content;

               if( can_wear( otmp, ITEM_NO_TAKE )
               || ( is_obj_stat( otmp, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
               || ch->carry_number + otmp->count > can_carry_n( ch )
               || ch->carry_weight + get_obj_weight( otmp ) > can_carry_w( ch )
               || ( get_real_obj_weight( source ) + get_real_obj_weight( obj ) / obj->count ) > obj->value[0] )
                  continue;
               obj_from_obj( otmp );
               obj_to_obj( otmp, obj );
               found = true;
            }
            if( found )
            {
               act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
               act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
            }
            else
               send_to_char( "There is nothing appropriate in there.\r\n", ch );
            break;
      }
      return;
   }

   if( source->value[1] < 1 )
   {
      send_to_char( "There's none left!\r\n", ch );
      return;
   }

   separate_obj( source );
   separate_obj( obj );

   switch( source->item_type )
   {
      default:
         bug( "%s: got bad item type: %d", __FUNCTION__, source->item_type );
         send_to_char( "Something went wrong...\r\n", ch );
         return;

      case ITEM_FOUNTAIN:
         if( obj->value[1] > 0 && obj->value[2] != LIQ_WATER )
         {
            send_to_char( "There is already another liquid in it.\r\n", ch );
            return;
         }
         obj->value[2] = LIQ_WATER;
         obj->value[1] = obj->value[0];
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         return;

      case ITEM_BLOOD:
         if( obj->value[1] > 0 && obj->value[2] != LIQ_BLOOD )
         {
            send_to_char( "There is already another liquid in it.\r\n", ch );
            return;
         }
         obj->value[2] = LIQ_BLOOD;
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         if( ( source->value[1] -= diff ) < 1 )
         {
            make_bloodstain( source, ch->in_room );
            extract_obj( source );
         }
         return;

      case ITEM_HERB:
         if( obj->value[1] > 0 && obj->value[2] != source->value[2] )
         {
            send_to_char( "There is already another type of herb in it.\r\n", ch );
            return;
         }
         obj->value[2] = source->value[2];
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         act( AT_ACTION, "You fill $p with $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p with $P.", ch, obj, source, TO_ROOM );
         if( ( source->value[1] -= diff ) < 1 )
            extract_obj( source );
         return;

      case ITEM_HERB_CON:
         if( obj->value[1] > 0 && obj->value[2] != source->value[2] )
         {
            send_to_char( "There is already another type of herb in it.\r\n", ch );
            return;
         }
         obj->value[2] = source->value[2];
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         source->value[1] -= diff;
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         return;

      case ITEM_DRINK_CON:
         if( obj->value[1] > 0 && obj->value[2] != source->value[2] )
         {
            send_to_char( "There is already another liquid in it.\r\n", ch );
            return;
         }
         obj->value[2] = source->value[2];
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         source->value[1] -= diff;
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         return;
   }
}

CMDF( do_drink )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int liquid, ocheck = 0;

   if( !argument || argument[0] == '\0' )
   {
      for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
         if( can_see_obj( ch, obj ) && ( ( obj->item_type == ITEM_FOUNTAIN ) || ( obj->item_type == ITEM_BLOOD && is_vampire( ch ) ) ) )
            break;

      if( !obj )
      {
         send_to_char( "Drink what?\r\n", ch );
         return;
      }
   }
   else
   {
      argument = one_argument( argument, arg );

      if( !str_cmp( arg, "room" ) )
      {
         ocheck = 1;
         argument = one_argument( argument, arg );
      }
      else if( !str_cmp( arg, "inv" ) || !str_cmp( arg, "inventory" ) )
      {
         ocheck = 2;
         argument = one_argument( argument, arg );
      }
      else if( !str_cmp( arg, "worn" ) )
      {
         ocheck = 3;
         argument = one_argument( argument, arg );
      }

      if( !( obj = find_obj( ch, arg, false ) ) )
      {
         if( !( obj = new_get_obj_here( ocheck, ch, arg ) ) )
         {
//            send_to_char( "You can't find it.\r\n", ch );
            return;
         }
      }
   }

   if( obj->count > 1 && obj->item_type != ITEM_FOUNTAIN )
      separate_obj( obj );

   if( !is_npc( ch ) && ch->pcdata->condition[COND_DRUNK] > 40 )
   {
      send_to_char( "You fail to reach your mouth.  *Hic*\r\n", ch );
      return;
   }

   switch( obj->item_type )
   {
      default:
         if( obj->carried_by == ch )
         {
            act( AT_ACTION, "$n lifts $p up to $s mouth and tries to drink from it...", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You bring $p up to your mouth and try to drink from it...", ch, obj, NULL, TO_CHAR );
         }
         else
         {
            act( AT_ACTION, "$n gets down and tries to drink from $p... (Is $e feeling ok?)", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You get down on the ground and try to drink from $p...", ch, obj, NULL, TO_CHAR );
         }
         break;

      case ITEM_POTION:
         if( obj->carried_by == ch )
            do_quaff( ch, obj->name );
         else
            send_to_char( "You're not carrying that.\r\n", ch );
         break;

      case ITEM_BLOOD:
         if( is_npc( ch ) || !is_vampire( ch ) )
         {
            send_to_char( "It is not in your nature to do such things.\r\n", ch );
            return;
         }
         if( obj->timer > 0 && ch->level > 5 && ch->mana > ( ch->max_mana / 4 ) )
         {
            send_to_char( "It is beneath you to stoop to drinking blood from the ground!\r\n", ch );
            send_to_char( "Unless in dire need, you'd much rather have blood from a victim's neck!\r\n", ch );
            return;
         }
         if( ch->mana >= ch->max_mana )
         {
            send_to_char( "Alas... you can't consume any more blood.\r\n", ch );
            return;
         }
         if( ch->pcdata->condition[COND_FULL] >= 48 || ch->pcdata->condition[COND_THIRST] >= 48 )
         {
            send_to_char( "You're too full to drink any blood.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_BLOOD, "$n drinks from the spilled blood.", ch, NULL, NULL, TO_ROOM );
            act( AT_BLOOD, "You drink from the spilled blood.", ch, NULL, NULL, TO_CHAR );
            if( obj->value[1] <= 1 )
            {
               act( AT_BLOOD, "You drink the last drop of blood from the spill.", ch, NULL, NULL, TO_CHAR );
               act( AT_BLOOD, "$n drinks the last drop of blood from the spill.", ch, NULL, NULL, TO_ROOM );
            }
         }
         ch->mana += 1;
         gain_condition( ch, COND_FULL, 1 );
         gain_condition( ch, COND_THIRST, 1 );
         if( --obj->value[1] <= 0 )
         {
            make_bloodstain( obj, ch->in_room );
            extract_obj( obj );
         }
         break;

      case ITEM_FOUNTAIN:
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n drinks from the fountain.", ch, NULL, NULL, TO_ROOM );
            act( AT_ACTION, "You take a long thirst quenching drink.", ch, NULL, NULL, TO_CHAR );
         }

         if( !is_npc( ch ) )
            ch->pcdata->condition[COND_THIRST] = 40;
         break;

      case ITEM_DRINK_CON:
         if( obj->value[1] <= 0 )
         {
            send_to_char( "It is already empty.\r\n", ch );
            return;
         }

         if( ( liquid = obj->value[2] ) >= LIQ_MAX )
         {
            bug( "%s: bad liquid number %d.", __FUNCTION__, liquid );
            liquid = obj->value[2] = 0;
         }

         if( liquid == LIQ_BLOOD )
         {
            if( is_npc( ch ) || !is_vampire( ch ) )
            {
               send_to_char( "It is not in your nature to do such things.\r\n", ch );
               return;
            }
            if( ch->mana >= ch->max_mana )
            {
               send_to_char( "Alas... you can't consume any more blood.\r\n", ch );
               return;
            }
            ch->mana += 1;
         }

         if( obj->in_obj )
         {
            act( AT_PLAIN, "You take $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
            act( AT_PLAIN, "$n takes $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
         }

         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n drinks $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_ROOM );
            act( AT_ACTION, "You drink $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_CHAR );
         }

         gain_condition( ch, COND_DRUNK, liq_table[liquid].liq_affect[COND_DRUNK] );
         gain_condition( ch, COND_FULL, liq_table[liquid].liq_affect[COND_FULL] );
         gain_condition( ch, COND_THIRST, liq_table[liquid].liq_affect[COND_THIRST] );

         if( !is_npc( ch ) )
         {
            if( ch->pcdata->condition[COND_DRUNK] > 24 )
               send_to_char( "You feel quite sloshed.\r\n", ch );
            else if( ch->pcdata->condition[COND_DRUNK] > 18 )
               send_to_char( "You feel very drunk.\r\n", ch );
            else if( ch->pcdata->condition[COND_DRUNK] > 12 )
               send_to_char( "You feel drunk.\r\n", ch );
            else if( ch->pcdata->condition[COND_DRUNK] > 8 )
               send_to_char( "You feel a little drunk.\r\n", ch );
            else if( ch->pcdata->condition[COND_DRUNK] > 5 )
               send_to_char( "You feel light headed.\r\n", ch );

            if( ch->pcdata->condition[COND_FULL] >= 50 )
               send_to_char( "You're full.\r\n", ch );

            if( ch->pcdata->condition[COND_THIRST] >= 50 )
               send_to_char( "You feel bloated.\r\n", ch );
            else if( ch->pcdata->condition[COND_THIRST] > 36 )
               send_to_char( "Your stomach is sloshing around.\r\n", ch );
            else if( ch->pcdata->condition[COND_THIRST] > 30 )
               send_to_char( "You don't feel thirsty.\r\n", ch );
         }

         if( obj->value[3] > 0 )
         {
            /* The drink was poisoned! */
            AFFECT_DATA af;

            act( AT_POISON, "$n sputters and gags.", ch, NULL, NULL, TO_ROOM );
            act( AT_POISON, "You sputter and gag.", ch, NULL, NULL, TO_CHAR );
            ch->mental_state = URANGE( 20, ch->mental_state + 5, 100 );
            af.type = gsn_poison;
            af.duration = 3 * obj->value[3];
            af.location = APPLY_EXT_AFFECT;
            af.modifier = AFF_POISON;
            af.bitvector = meb( AFF_POISON );
            affect_join( ch, &af );
         }

         obj->value[1] -= 1;
         if( obj->value[1] <= 0 )
         {
            send_to_char( "The empty container vanishes.\r\n", ch );
            extract_obj( obj );
         }
         else if( obj->in_obj )
         {
            act( AT_PLAIN, "You put $p back in $P.", ch, obj, obj->in_obj, TO_CHAR );
            act( AT_PLAIN, "$n puts $p back in $P.", ch, obj, obj->in_obj, TO_ROOM );
         }
         break;
   }
   if( who_fighting( ch ) && is_pkill( ch ) )
      wait_state( ch, PULSE_PER_SECOND / 3 );
   else
      wait_state( ch, PULSE_PER_SECOND );
}

CMDF( do_eat )
{
   char buf[MSL];
   OBJ_DATA *obj;
   ch_ret retcode;
   int foodcond;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Eat what?\r\n", ch );
      return;
   }

   if( is_npc( ch ) || ch->pcdata->condition[COND_FULL] > 5 )
      if( ms_find_obj( ch ) )
         return;

   if( !( obj = find_obj( ch, argument, true ) ) )
      return;

   separate_obj( obj );

   if( !is_immortal( ch ) )
   {
      if( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL
      && obj->item_type != ITEM_COOK && obj->item_type != ITEM_FISH )
      {
         act( AT_ACTION, "$n starts to nibble on $p... ($e must really be hungry)", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You try to nibble on $p...", ch, obj, NULL, TO_CHAR );
         return;
      }

      if( !is_npc( ch ) && ch->pcdata->condition[COND_FULL] > 40 )
      {
         send_to_char( "You're too full to eat more.\r\n", ch );
         return;
      }
   }

   if( obj->in_obj )
   {
      act( AT_PLAIN, "You take $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
      act( AT_PLAIN, "$n takes $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
   }
   if( ch->fighting && number_percent( ) > ( get_curr_dex( ch ) * 2 + 47 ) )
   {
      snprintf( buf, sizeof( buf ), "%s",
         ( ch->in_room->sector_type == SECT_UNDERWATER
         || ch->in_room->sector_type == SECT_WATER_SWIM
         || ch->in_room->sector_type == SECT_WATER_NOSWIM )
            ? "dissolves in the water" : ( ch->in_room->sector_type == SECT_AIR
         || xIS_SET( ch->in_room->room_flags, ROOM_NOFLOOR ) )
            ? "falls far below" : "is trampled underfoot" );
      act( AT_MAGIC, "$n drops $p, and it $T.", ch, obj, buf, TO_ROOM );
      act( AT_MAGIC, "Oops, $p slips from your hand and $T!", ch, obj, buf, TO_CHAR );
   }
   else
   {
      if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
      {
         if( !obj->action_desc || obj->action_desc[0] == '\0' )
         {
            act( AT_ACTION, "$n eats $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You eat $p.", ch, obj, NULL, TO_CHAR );
         }
         else
            actiondesc( ch, obj );
      }

      switch( obj->item_type )
      {
         case ITEM_COOK:
         case ITEM_FOOD:
         case ITEM_FISH:
            wait_state( ch, PULSE_PER_SECOND / 3 );
            if( obj->timer > 0 && obj->value[1] > 0 )
               foodcond = ( obj->timer * 10 ) / obj->value[1];
            else
               foodcond = 10;

            if( !is_npc( ch ) )
               gain_condition( ch, COND_FULL, ( obj->value[0] * foodcond ) / 10 );

            if( obj->value[3] > 0
            || ( foodcond < 4 && number_range( 0, foodcond + 1 ) == 0 )
            || ( ( obj->item_type == ITEM_COOK || obj->item_type == ITEM_FISH ) && obj->value[2] == 0 ) )
            {
               /* The food was poisoned! */
               AFFECT_DATA af;

               if( obj->value[3] > 0 )
               {
                  act( AT_POISON, "$n chokes and gags.", ch, NULL, NULL, TO_ROOM );
                  act( AT_POISON, "You choke and gag.", ch, NULL, NULL, TO_CHAR );
                  ch->mental_state = URANGE( 20, ch->mental_state + 5, 100 );
               }
               else
               {
                  act( AT_POISON, "$n gags on $p.", ch, obj, NULL, TO_ROOM );
                  act( AT_POISON, "You gag on $p.", ch, obj, NULL, TO_CHAR );
                  ch->mental_state = URANGE( 15, ch->mental_state + 5, 100 );
               }

               af.type = gsn_poison;
               af.duration = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
               af.location = APPLY_EXT_AFFECT;
               af.modifier = AFF_POISON;
               af.bitvector = meb( AFF_POISON );
               affect_join( ch, &af );
            }
            break;

         case ITEM_PILL:
            if( who_fighting( ch ) && is_pkill( ch ) )
               wait_state( ch, PULSE_PER_SECOND / 4 );
            else
               wait_state( ch, PULSE_PER_SECOND / 3 );

            if( !is_npc( ch ) && obj->value[4] )
               gain_condition( ch, COND_FULL, obj->value[4] );
            retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
            if( retcode == rNONE )
               retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
            if( retcode == rNONE )
               retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
            break;
      }
   }
   extract_obj( obj );
}

CMDF( do_quaff )
{
   OBJ_DATA *obj;
   ch_ret retcode;
   short drop = 0;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Quaff what?\r\n", ch );
      return;
   }

   if( !is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
      return;

   if( !( obj = find_obj( ch, argument, true ) ) )
      return;

   separate_obj( obj );

   if( obj->item_type != ITEM_POTION )
   {
      if( obj->item_type == ITEM_DRINK_CON )
         do_drink( ch, obj->name );
      else
      {
         act( AT_ACTION, "$n lifts $p up to $s mouth and tries to drink from it...", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You bring $p up to your mouth and try to drink from it...", ch, obj, NULL, TO_CHAR );
      }
      return;
   }

   /* Empty container check -Shaddai */
   if( obj->value[1] == -1 && obj->value[2] == -1 && obj->value[3] == -1 )
   {
      send_to_char( "You suck in nothing but air.\r\n", ch );
      return;
   }

   if( !is_npc( ch ) && ( ch->pcdata->condition[COND_FULL] >= 50 || ch->pcdata->condition[COND_THIRST] >= 50 ) )
   {
      send_to_char( "Your stomach can't contain any more.\r\n", ch );
      return;
   }

   if( obj->in_obj )
   {
      act( AT_PLAIN, "You take $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
      act( AT_PLAIN, "$n takes $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
   }

   /* Always at least 5% chance of dropping it */
   drop = URANGE( 0, get_curr_dex( ch ), 95 );
   if( ch->fighting && number_percent( ) > drop )
   {
      act( AT_MAGIC, "$n fumbles $p and shatters it into fragments.", ch, obj, NULL, TO_ROOM );
      act( AT_MAGIC, "Oops... $p is knocked from your hand and shatters!", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
      {
         act( AT_ACTION, "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You quaff $p.", ch, obj, NULL, TO_CHAR );
      }

      wait_state( ch, PULSE_PER_SECOND / 3 );
      if( !is_npc( ch ) )
         gain_condition( ch, COND_THIRST, 1 );
      retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
      if( retcode == rNONE )
         retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
      if( retcode == rNONE )
         retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
   }
   extract_obj( obj );
}

void recite_scroll( CHAR_DATA *ch, OBJ_DATA *scroll, CHAR_DATA *victim, OBJ_DATA *obj )
{
   ch_ret retcode;

   if( !scroll )
   {
      send_to_char( "You don't have that scroll.\r\n", ch );
      return;
   }

   if( scroll->item_type != ITEM_SCROLL )
   {
      act( AT_ACTION, "$n holds up $p as if to recite something from it...", ch, scroll, NULL, TO_ROOM );
      act( AT_ACTION, "You hold up $p and stand there with your mouth open.  (Now what?)", ch, scroll, NULL, TO_CHAR );
      return;
   }

   separate_obj( scroll );

   if( scroll->in_obj )
   {
      act( AT_PLAIN, "You take $p from $P.", ch, scroll, scroll->in_obj, TO_CHAR );
      act( AT_PLAIN, "$n takes $p from $P.", ch, scroll, scroll->in_obj, TO_ROOM );
   }

   act( AT_MAGIC, "$n recites $p.", ch, scroll, NULL, TO_ROOM );
   act( AT_MAGIC, "You recite $p.", ch, scroll, NULL, TO_CHAR );

   if( victim != ch )
      wait_state( ch, 2 * PULSE_VIOLENCE );
   else
      wait_state( ch, PULSE_PER_SECOND / 2 );

   if( ( retcode = obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj ) ) == rNONE )
      if( ( retcode = obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj ) ) == rNONE )
         retcode = obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );

   extract_obj( scroll );
}

CMDF( do_recite )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *scroll = NULL, *obj = NULL;
   char buf[MSL];
   int ocheck = 0;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Recite what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( scroll = get_obj_carry( ch, arg1 ) ) )
   {
      snprintf( buf, sizeof( buf ), "%s %s", arg1, arg2 );
      if( !( scroll = find_obj( ch, buf, true ) ) )
         return;

      /* Ok so we found the scroll inside of a container and now we can get the rest of the info */
      argument = one_argument( argument, arg2 );
   }

   if( arg2 == NULL || arg2[0] == '\0' )
      victim = ch;
   else
   {
      if( !str_cmp( argument, "room" ) )
         ocheck = 1;
      else if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
         ocheck = 2;
      else if( !str_cmp( argument, "worn" ) )
         ocheck = 3;

      if( !( victim = get_char_room( ch, arg2 ) ) && !( obj = new_get_obj_here( ocheck, ch, arg2 ) ) )
      {
         send_to_char( "You can't find it.\r\n", ch );
         return;
      }
   }

   recite_scroll( ch, scroll, victim, obj );
}

/* Function to handle the state changing of a triggerobject (lever)  -Thoric */
void pullorpush( CHAR_DATA *ch, OBJ_DATA *obj, bool pull )
{
   ROOM_INDEX_DATA *room, *to_room;
   EXIT_DATA *pexit, *pexit_rev;
   CHAR_DATA *rch;
   char buf[MSL];
   bool isup;
   int edir;

   if( IS_SET( obj->value[0], TRIG_UP ) )
      isup = true;
   else
      isup = false;
   switch( obj->item_type )
   {
      default:
         ch_printf( ch, "You can't %s that!\r\n", pull ? "pull" : "push" );
         return;
         break;

      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
         if( ( !pull && isup ) || ( pull && !isup ) )
         {
            ch_printf( ch, "It is already %s.\r\n", isup ? "up" : "down" );
            return;
         }
         break;

      case ITEM_BUTTON:
         if( ( !pull && isup ) || ( pull && !isup ) )
         {
            ch_printf( ch, "It is already %s.\r\n", isup ? "in" : "out" );
            return;
         }
         break;
   }

   if( pull && HAS_PROG( obj->pIndexData, PULL_PROG ) )
   {
      if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
         REMOVE_BIT( obj->value[0], TRIG_UP );
      oprog_pull_trigger( ch, obj );
      return;
   }

   if( !pull && HAS_PROG( obj->pIndexData, PUSH_PROG ) )
   {
      if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
         SET_BIT( obj->value[0], TRIG_UP );
      oprog_push_trigger( ch, obj );
      return;
   }

   if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
   {
      snprintf( buf, sizeof( buf ), "$n %s $p.", pull ? "pulls" : "pushes" );
      act( AT_ACTION, buf, ch, obj, NULL, TO_ROOM );
      snprintf( buf, sizeof( buf ), "You %s $p.", pull ? "pull" : "push" );
      act( AT_ACTION, buf, ch, obj, NULL, TO_CHAR );
   }

   if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
   {
      if( pull )
         REMOVE_BIT( obj->value[0], TRIG_UP );
      else
         SET_BIT( obj->value[0], TRIG_UP );
   }

   if( IS_SET( obj->value[0], TRIG_TELEPORT )
   || IS_SET( obj->value[0], TRIG_TELEPORTALL ) || IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
   {
      int flags;

      if( !( room = get_room_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }

      /* No point in continuing if comming back to this room */
      if( ch && ch->in_room && ch->in_room == room )
      {
         bug( "%s: obj points back to room %d which is where it is being used at.", __FUNCTION__, obj->value[1] );
         return;
      }

      flags = 0;
      if( IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) )
         SET_BIT( flags, TELE_SHOWDESC );
      if( IS_SET( obj->value[0], TRIG_TELEPORTALL ) )
         SET_BIT( flags, TELE_TRANSALL );
      if( IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
         SET_BIT( flags, TELE_TRANSALLPLUS );

      teleport( ch, obj->value[1], flags );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 )
   || IS_SET( obj->value[0], TRIG_RAND10 ) || IS_SET( obj->value[0], TRIG_RAND11 ) )
   {
      int maxd;

      if( !( room = get_room_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }

      if( IS_SET( obj->value[0], TRIG_RAND4 ) )
         maxd = 3;
      else if( IS_SET( obj->value[0], TRIG_RAND6 ) )
         maxd = 5;
      else if( IS_SET( obj->value[0], TRIG_RAND10 ) )
         maxd = 9;
      else
         maxd = 10;

      randomize_exits( room, maxd );
      for( rch = room->first_person; rch; rch = rch->next_in_room )
      {
         send_to_char( "You hear a loud rumbling sound.\r\n", rch );
         send_to_char( "Something seems different...\r\n", rch );
      }

      return;
   }
   if( IS_SET( obj->value[0], TRIG_DEATH ) )
   {
      /* Should we really send a message to the room? */
      act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
      act( AT_DEAD, "Oopsie... you're dead!\r\n", ch, NULL, NULL, TO_CHAR );
      snprintf( buf, sizeof( buf ), "%s hit a DEATH TRIGGER in room %d!", ch->name, ch->in_room->vnum );
      log_string( buf );
      to_channel( buf, "monitor", PERM_IMM );

      /* Personaly I fiqured if we wanted it to be a full DT we could just have it send them into a DT. */
      set_cur_char( ch );
      raw_kill( ch, ch );

      /* If you want it to be more like a room deathtrap use this instead */
/*
      if( is_npc( ch ) )
         extract_char( ch, true );
      else
         extract_char( ch, false );
*/
      return;
   }

   if( IS_SET( obj->value[0], TRIG_MLOAD ) )
   {
      MOB_INDEX_DATA *pMobIndex;
      CHAR_DATA *mob;

      /* value[1] for the obj vnum */
      if( !( pMobIndex = get_mob_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid mob vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      /* Set room to NULL before hand */
      room = NULL;
      /* value[2] for the room vnum to put the mobile in if there is one, 0 for current room */
      if( obj->value[2] > 0 && !( room = get_room_index( obj->value[2] ) ) )
      {
         bug( "%s: obj points to invalid room vnum %d", __FUNCTION__, obj->value[2] );
         return;
      }
      if( !( mob = create_mobile( pMobIndex ) ) )
      {
         bug( "%s: obj couldnt create_mobile vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      if( room )
         char_to_room( mob, room );
      else
         char_to_room( mob, ch->in_room );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_OLOAD ) )
   {
      OBJ_INDEX_DATA *pObjIndex;
      OBJ_DATA *tobj;
      int ulevel = 1;

      /* value[1] for the obj vnum */
      if( !( pObjIndex = get_obj_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid object vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      /* Set room to NULL before hand */
      room = NULL;
      /* value[2] for the room vnum to put the object in if there is one, 0 for giving it to char or current room */
      if( obj->value[2] > 0 && !( room = get_room_index( obj->value[2] ) ) )
      {
         bug( "%s: obj points to invalid room vnum %d", __FUNCTION__, obj->value[2] );
         return;
      }
      /* Uses value[3] for level */
      if( obj->value[3] > 0 )
         ulevel = obj->value[3];
      if( !( tobj = create_object( pObjIndex, URANGE( 0, ulevel, MAX_LEVEL ) ) ) )
      {
         bug( "%s: obj couldnt create_obj vnum %d at level %d", __FUNCTION__, obj->value[1], ulevel );
         return;
      }
      if( room )
         obj_to_room( tobj, room );
      else
      {
         if( !can_wear( obj, ITEM_NO_TAKE ) )
            obj_to_char( tobj, ch );
         else
            obj_to_room( tobj, ch->in_room );
      }
      return;
   }

   /* Use to just use the sn directly, but that changes way to often to not use the slot instead */
   if( IS_SET( obj->value[0], TRIG_CAST ) )
   {
      int usesn = slot_lookup( obj->value[1] );

      if( usesn < 0 || !is_valid_sn( usesn ) )
      {
         bug( "%s: obj points to invalid slot [%d]", __FUNCTION__, obj->value[1] );
         return;
      }

      obj_cast_spell( usesn, URANGE( 1, ( obj->value[2] > 0 ) ? obj->value[2] : ch->level, MAX_LEVEL ), ch, ch, NULL );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_CONTAINER ) )
   {
      OBJ_DATA *container = NULL;

      if( !( room = get_room_index( obj->value[1] ) ) )
         room = obj->in_room;
      if( !room )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }

      for( container = room->first_content; container; container = container->next_content )
      {
         if( container->pIndexData->vnum == obj->value[2] )
            break;
      }
      if( !container )
      {
         bug( "%s: obj points to a container [%d] that's not in room [%d] where it should be?", __FUNCTION__, obj->value[2], obj->value[1] );
         return;
      }
      if( container->item_type != ITEM_CONTAINER )
      {
         bug( "%s: obj points to object [%d], but it isn't a container.", __FUNCTION__, obj->value[2] );
         return;
      }
      /* Could toss in some messages. Limit how it is handled etc... I'll leave that to each one to do */
      /* Started to use TRIG_OPEN, TRIG_CLOSE, TRIG_LOCK, and TRIG_UNLOCK like TRIG_DOOR does. */
      /* It limits it alot, but it wouldn't allow for an EATKEY change */
      if( IS_SET( obj->value[3], CONT_CLOSEABLE ) )
         TOGGLE_BIT( container->value[1], CONT_CLOSEABLE );
      if( IS_SET( obj->value[3], CONT_PICKPROOF ) )
         TOGGLE_BIT( container->value[1], CONT_PICKPROOF );
      if( IS_SET( obj->value[3], CONT_CLOSED ) )
         TOGGLE_BIT( container->value[1], CONT_CLOSED );
      if( IS_SET( obj->value[3], CONT_LOCKED ) )
         TOGGLE_BIT( container->value[1], CONT_LOCKED );
      if( IS_SET( obj->value[3], CONT_EATKEY ) )
         TOGGLE_BIT( container->value[1], CONT_EATKEY );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_DOOR ) )
   {
      if( !( room = get_room_index( obj->value[1] ) ) )
         room = obj->in_room;
      if( !room )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }

      if( IS_SET( obj->value[0], TRIG_D_NORTH ) )
         edir = DIR_NORTH;
      else if( IS_SET( obj->value[0], TRIG_D_SOUTH ) )
         edir = DIR_SOUTH;
      else if( IS_SET( obj->value[0], TRIG_D_EAST ) )
         edir = DIR_EAST;
      else if( IS_SET( obj->value[0], TRIG_D_WEST ) )
         edir = DIR_WEST;
      else if( IS_SET( obj->value[0], TRIG_D_UP ) )
         edir = DIR_UP;
      else if( IS_SET( obj->value[0], TRIG_D_DOWN ) )
         edir = DIR_DOWN;
      else if( IS_SET( obj->value[0], TRIG_D_NORTHEAST ) )
         edir = DIR_NORTHEAST;
      else if( IS_SET( obj->value[0], TRIG_D_NORTHWEST ) )
         edir = DIR_NORTHWEST;
      else if( IS_SET( obj->value[0], TRIG_D_SOUTHEAST ) )
         edir = DIR_SOUTHEAST;
      else if( IS_SET( obj->value[0], TRIG_D_SOUTHWEST ) )
         edir = DIR_SOUTHWEST;
      else if( IS_SET( obj->value[0], TRIG_D_SOMEWHERE ) )
         edir = DIR_SOMEWHERE;
      else
      {
         bug( "%s: door: no direction flag set.", __FUNCTION__ );
         return;
      }

      if( !( pexit = get_exit( room, edir ) ) )
      {
         if( !IS_SET( obj->value[0], TRIG_PASSAGE ) )
         {
            bug( "%s: obj points to non-exit %d", __FUNCTION__, obj->value[1] );
            return;
         }
         if( !( to_room = get_room_index( obj->value[2] ) ) )
         {
            bug( "%s: dest points to invalid room %d", __FUNCTION__, obj->value[2] );
            return;
         }
         pexit = make_exit( room, to_room, edir );
         pexit->keyword = NULL;
         pexit->description = NULL;
         pexit->key = -1;
         xCLEAR_BITS( pexit->exit_info );
         top_exit++;
         for( rch = room->first_person; rch; rch = rch->next_in_room )
            act( AT_ACTION, "A passage opens.", rch, NULL, NULL, TO_CHAR );
         if( ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
            for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_ACTION, "A passage opens.", rch, NULL, NULL, TO_CHAR );
         return;
      }
      if( ( IS_SET( obj->value[0], TRIG_UNLOCK ) && xIS_SET( pexit->exit_info, EX_LOCKED ) )
      || ( IS_SET( obj->value[0], TRIG_LOCK ) && !xIS_SET( pexit->exit_info, EX_LOCKED ) ) )
      {
         bool islocked = false;

         if( xIS_SET( pexit->exit_info, EX_LOCKED ) )
            islocked = true;

         if( islocked )
            xREMOVE_BIT( pexit->exit_info, EX_LOCKED );
         else
            xSET_BIT( pexit->exit_info, EX_LOCKED );
         for( rch = room->first_person; rch; rch = rch->next_in_room )
            act( AT_ACTION, "You hear a faint click.", rch, NULL, NULL, TO_CHAR );
         if( ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
         {
            if( islocked )
               xREMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
            else
               xSET_BIT( pexit_rev->exit_info, EX_LOCKED );
            for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_ACTION, "You hear a faint click.", rch, NULL, NULL, TO_CHAR );
         }
         return;
      }
      if( ( IS_SET( obj->value[0], TRIG_OPEN ) && xIS_SET( pexit->exit_info, EX_CLOSED ) )
      || ( IS_SET( obj->value[0], TRIG_CLOSE ) && !xIS_SET( pexit->exit_info, EX_CLOSED ) ) )
      {
         bool isclosed = false;
         char *exkey;

         if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
            isclosed = true;

         if( isclosed )
            xREMOVE_BIT( pexit->exit_info, EX_CLOSED );
         else
            xSET_BIT( pexit->exit_info, EX_CLOSED );

         if( pexit->keyword && pexit->keyword[0] != '\0' )
            exkey = pexit->keyword;
         else
            exkey = (char *)"door";

         for( rch = room->first_person; rch; rch = rch->next_in_room )
         {
            if( isclosed )
               act( AT_ACTION, "The $d opens.", rch, NULL, exkey, TO_CHAR );
            else
               act( AT_ACTION, "The $d closes.", rch, NULL, exkey, TO_CHAR );
         }
         if( ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
         {
            if( isclosed )
               xREMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
            else
               xSET_BIT( pexit_rev->exit_info, EX_CLOSED );
            if( pexit_rev->keyword && pexit_rev->keyword[0] != '\0' )
               exkey = pexit_rev->keyword;
            else
               exkey = (char *)"door";
            for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
            {
               if( isclosed )
                  act( AT_ACTION, "The $d opens.", rch, NULL, exkey, TO_CHAR );
               else
                  act( AT_ACTION, "The $d closes.", rch, NULL, exkey, TO_CHAR );
            }
         }
         check_room_for_traps( ch, trap_door[edir] );
         return;
      }
   }
}

CMDF( do_pull )
{
   OBJ_DATA *obj;
   char arg[MIL];
   int ocheck = 0;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Pull what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   argument = one_argument( argument, arg );

   if( !str_cmp( argument, "room" ) )
      ocheck = 1;
   else if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
      ocheck = 2;
   else if( !str_cmp( argument, "worn" ) )
      ocheck = 3;

   if( !( obj = new_get_obj_here( ocheck, ch, arg ) ) )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, arg, TO_CHAR );
      return;
   }

   pullorpush( ch, obj, true );
}

CMDF( do_push )
{
   OBJ_DATA *obj;
   char arg[MIL];
   int ocheck = 0;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Push what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   argument = one_argument( argument, arg );

   if( !str_cmp( argument, "room" ) )
      ocheck = 1;
   else if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
      ocheck = 2;
   else if( !str_cmp( argument, "worn" ) )
      ocheck = 3;

   if( !( obj = new_get_obj_here( ocheck, ch, arg ) ) )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }

   pullorpush( ch, obj, false );
}

CMDF( do_rap )
{
   EXIT_DATA *pexit, *pexit_rev;
   ROOM_INDEX_DATA *to_room, *from_room;
   char *keyword;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Rap on what?\r\n", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You have better things to do with your hands right now.\r\n", ch );
      return;
   }
   if( !( pexit = find_door( ch, argument, true ) ) )
   {
      act( AT_ACTION, "You make knocking motions through the air.", ch, NULL, NULL, TO_CHAR );
      act( AT_ACTION, "$n makes knocking motions through the air.", ch, NULL, NULL, TO_ROOM );
      return;
   }
   if( !xIS_SET( pexit->exit_info, EX_CLOSED ) )
   {
      send_to_char( "Why knock?  It's open.\r\n", ch );
      return;
   }
   if( xIS_SET( pexit->exit_info, EX_SECRET ) )
      keyword = (char *)"wall";
   else
      keyword = pexit->keyword;
   from_room = ch->in_room;
   act( AT_ACTION, "You rap loudly on the $d.", ch, NULL, keyword, TO_CHAR );
   act( AT_ACTION, "$n raps loudly on the $d.", ch, NULL, keyword, TO_ROOM );
   if( ( to_room = pexit->to_room ) && ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
   {
      char_from_room( ch );
      char_to_room( ch, to_room );
      if( xIS_SET( pexit_rev->exit_info, EX_SECRET ) )
         keyword = (char *)"wall";
      else
         keyword = pexit_rev->keyword;
      act( AT_ACTION, "Someone raps loudly from the other side of the $d.", ch, NULL, keyword, TO_ROOM );
      char_from_room( ch );
      char_to_room( ch, from_room );
   }
}

CMDF( do_smoke )
{
   OBJ_DATA *pipe;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Smoke what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( pipe = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that.\r\n", ch );
      return;
   }
   if( pipe->item_type != ITEM_PIPE )
   {
      act( AT_ACTION, "You try to smoke $p... but it doesn't seem to work.", ch, pipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to smoke $p... (I wonder what $e's been putting in $s pipe?)", ch, pipe, NULL, TO_ROOM );
      return;
   }
   if( !IS_SET( pipe->value[3], PIPE_LIT ) )
   {
      act( AT_ACTION, "You try to smoke $p, but it's not lit.", ch, pipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to smoke $p, but it's not lit.", ch, pipe, NULL, TO_ROOM );
      return;
   }
   if( pipe->value[1] <= 0 )
   {
      act( AT_ACTION, "You try to smoke $p, but it's empty.", ch, pipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to smoke $p, but it's empty.", ch, pipe, NULL, TO_ROOM );
      return;
   }
   if( !oprog_use_trigger( ch, pipe, NULL, NULL ) )
   {
      if( pipe->value[1] > 1 )
      {
         act( AT_ACTION, "You draw thoughtfully from $p.", ch, pipe, NULL, TO_CHAR );
         act( AT_ACTION, "$n draws thoughtfully from $p.", ch, pipe, NULL, TO_ROOM );
      }
      else
      {
         act( AT_ACTION, "You draw the last puff from $p.", ch, pipe, NULL, TO_CHAR );
         act( AT_ACTION, "$n draws the last puff from $p.", ch, pipe, NULL, TO_ROOM );
      }
   }
   if( is_valid_herb( pipe->value[2] ) && pipe->value[2] < top_herb )
   {
      int sn = ( pipe->value[2] + TYPE_HERB );
      SKILLTYPE *skill = get_skilltype( sn );

      wait_state( ch, skill->beats );
      if( skill->spell_fun )
         obj_cast_spell( sn, ch->level, ch, ch, NULL );
      if( !pipe )
         return;
   }
   else
      bug( "%s: bad herb type %d", __FUNCTION__, pipe->value[2] );
   if( --pipe->value[1] < 1 )
   {
      REMOVE_BIT( pipe->value[3], PIPE_LIT );
      SET_BIT( pipe->value[3], PIPE_FULLOFASH );
   }
}

CMDF( do_light )
{
   OBJ_DATA *pipe;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Light what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( pipe = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that.\r\n", ch );
      return;
   }
   if( pipe->item_type != ITEM_PIPE )
   {
      send_to_char( "You can't light that.\r\n", ch );
      return;
   }
   if( IS_SET( pipe->value[3], PIPE_LIT ) )
   {
      send_to_char( "It's already lit.\r\n", ch );
      return;
   }
   if( IS_SET( pipe->value[3], PIPE_FULLOFASH ) )
   {
      act( AT_ACTION, "You try to light $p, but the ashes won't ignight.", ch, pipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to light $p, but the ashes won't ignight.", ch, pipe, NULL, TO_ROOM );
      return;
   }
   if( pipe->value[1] < 1 )
   {
      act( AT_ACTION, "You try to light $p, but it's empty.", ch, pipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to light $p, but it's empty.", ch, pipe, NULL, TO_ROOM );
      return;
   }
   act( AT_ACTION, "You carefully light $p.", ch, pipe, NULL, TO_CHAR );
   act( AT_ACTION, "$n carefully lights $p.", ch, pipe, NULL, TO_ROOM );
   SET_BIT( pipe->value[3], PIPE_LIT );
}

CMDF( do_empty )
{
   OBJ_DATA *obj;
   char arg1[MIL], arg2[MIL];
   int ocheck = 0;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "into" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Empty what?\r\n", ch );
      return;
   }
   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You aren't carrying that.\r\n", ch );
      return;
   }

   if( obj->count > 1 )
      separate_obj( obj );

   switch( obj->item_type )
   {
      default:
         act( AT_ACTION, "You shake $p in an attempt to empty it...", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n begins to shake $p in an attempt to empty it...", ch, obj, NULL, TO_ROOM );
         return;

      case ITEM_PIPE:
         if( obj->value[1] <= 0 && !IS_SET( obj->value[3], PIPE_FULLOFASH ) )
         {
            act( AT_ACTION, "$p is already empty.", ch, obj, NULL, TO_CHAR );
            return;
         }
         act( AT_ACTION, "You gently tap $p and empty it out.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n gently taps $p and empties it out.", ch, obj, NULL, TO_ROOM );
         REMOVE_BIT( obj->value[3], PIPE_FULLOFASH );
         REMOVE_BIT( obj->value[3], PIPE_LIT );
         obj->value[1] = 0;
         return;

      case ITEM_DRINK_CON:
         if( obj->value[1] < 1 )
         {
            send_to_char( "It's already empty.\r\n", ch );
            return;
         }
         act( AT_ACTION, "You empty $p.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n empties $p.", ch, obj, NULL, TO_ROOM );
         obj->value[1] = 0;
         return;

      case ITEM_CONTAINER:
      case ITEM_QUIVER:
         if( IS_SET( obj->value[1], CONT_CLOSED ) )
         {
            act( AT_PLAIN, "The $d is closed.", ch, NULL, obj->name, TO_CHAR );
            return;
         }

      case ITEM_KEYRING:
         if( !obj->first_content )
         {
            send_to_char( "It's already empty.\r\n", ch );
            return;
         }
         if( arg2 == NULL || arg2[0] == '\0' )
         {
            if( !can_drop_room( ch ) )
               return;

            if( xIS_SET( ch->in_room->room_flags, ROOM_NODROPALL ) || xIS_SET( ch->in_room->room_flags, ROOM_STORAGEROOM ) )
            {
               send_to_char( "You can't seem to do that here...\r\n", ch );
               return;
            }
            if( empty_obj( obj, NULL, ch->in_room ) )
            {
               act( AT_ACTION, "You empty $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n empties $p.", ch, obj, NULL, TO_ROOM );
               if( xIS_SET( sysdata.save_flags, SV_EMPTY ) )
                  save_char_obj( ch );
            }
            else
               send_to_char( "Hmmm... didn't work.\r\n", ch );
         }
         else
         {
            OBJ_DATA *dest;

            /* Need some way to specify if we want to look at things in room or in inventory */
            if( !str_cmp( argument, "room" ) )
               ocheck = 1;
            if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
               ocheck = 2;
            if( !str_cmp( argument, "worn" ) )
               ocheck = 3;

            if( !( dest = new_get_obj_here( ocheck, ch, arg2 ) ) )
            {
               send_to_char( "You can't find it.\r\n", ch );
               return;
            }
            if( dest == obj )
            {
               send_to_char( "You can't empty something into itself!\r\n", ch );
               return;
            }
            if( dest->item_type != ITEM_CONTAINER && dest->item_type != ITEM_KEYRING && dest->item_type != ITEM_QUIVER )
            {
               send_to_char( "That's not a container!\r\n", ch );
               return;
            }
            if( IS_SET( dest->value[1], CONT_CLOSED ) )
            {
               act( AT_PLAIN, "The $d is closed.", ch, NULL, dest->name, TO_CHAR );
               return;
            }
            separate_obj( dest );
            if( empty_obj( obj, dest, NULL ) )
            {
               act( AT_ACTION, "You empty $p into $P.", ch, obj, dest, TO_CHAR );
               act( AT_ACTION, "$n empties $p into $P.", ch, obj, dest, TO_ROOM );
               if( !dest->carried_by && xIS_SET( sysdata.save_flags, SV_EMPTY ) )
                  save_char_obj( ch );
            }
            else
               act( AT_ACTION, "$P is too full.", ch, obj, dest, TO_CHAR );
         }
         return;
   }
}

/* Apply a salve/ointment - Thoric Support for applying to others. Pkill concerns dealt with elsewhere. */
CMDF( do_apply )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *salve, *obj;
   ch_ret retcode;
   int ocheck = 0;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Apply what?\r\n", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You're too busy fighting ...\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;
   if( !( salve = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You don't have that.\r\n", ch );
      return;
   }

   obj = NULL;
   if( arg2 == NULL || arg2[0] == '\0' )
      victim = ch;
   else
   {
      if( !str_cmp( argument, "room" ) )
         ocheck = 1;
      else if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
         ocheck = 2;
      else if( !str_cmp( argument, "worn" ) )
         ocheck = 3;

      if( !( victim = get_char_room( ch, arg2 ) ) && !( obj = new_get_obj_here( ocheck, ch, arg2 ) ) )
      {
         send_to_char( "Apply it to what or who?\r\n", ch );
         return;
      }
   }

   /* apply salve to another object */
   if( obj )
   {
      send_to_char( "You can't do that... yet.\r\n", ch );
      return;
   }

   if( victim && victim->fighting )
   {
      send_to_char( "Wouldn't work very well while they're fighting ...\r\n", ch );
      return;
   }

   separate_obj( salve );
   if( salve->item_type != ITEM_SALVE )
   {
      if( victim == ch )
      {
         act( AT_ACTION, "$n starts to rub $p on $mself...", ch, salve, NULL, TO_ROOM );
         act( AT_ACTION, "You try to rub $p on yourself...", ch, salve, NULL, TO_CHAR );
      }
      else
      {
         act( AT_ACTION, "$n starts to rub $p on $N...", ch, salve, victim, TO_NOTVICT );
         act( AT_ACTION, "$n starts to rub $p on you...", ch, salve, victim, TO_VICT );
         act( AT_ACTION, "You try to rub $p on $N...", ch, salve, victim, TO_CHAR );
      }
      return;
   }

   --salve->value[1];
   if( !oprog_use_trigger( ch, salve, NULL, NULL ) )
   {
      if( !salve->action_desc || salve->action_desc[0] == '\0' )
      {
         if( salve->value[1] < 1 )
         {
            if( victim != ch )
            {
               act( AT_ACTION, "$n rubs the last of $p onto $N.", ch, salve, victim, TO_NOTVICT );
               act( AT_ACTION, "$n rubs the last of $p onto you.", ch, salve, victim, TO_VICT );
               act( AT_ACTION, "You rub the last of $p onto $N.", ch, salve, victim, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "You rub the last of $p onto yourself.", ch, salve, NULL, TO_CHAR );
               act( AT_ACTION, "$n rubs the last of $p onto $mself.", ch, salve, NULL, TO_ROOM );
            }
         }
         else
         {
            if( victim != ch )
            {
               act( AT_ACTION, "$n rubs $p onto $N.", ch, salve, victim, TO_NOTVICT );
               act( AT_ACTION, "$n rubs $p onto you.", ch, salve, victim, TO_VICT );
               act( AT_ACTION, "You rub $p onto $N.", ch, salve, victim, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "You rub $p onto yourself.", ch, salve, NULL, TO_CHAR );
               act( AT_ACTION, "$n rubs $p onto $mself.", ch, salve, NULL, TO_ROOM );
            }
         }
      }
      else
         actiondesc( ch, salve );
   }

   wait_state( ch, salve->value[2] );
   retcode = obj_cast_spell( salve->value[3], salve->value[0], ch, victim, NULL );
   if( retcode == rNONE )
      retcode = obj_cast_spell( salve->value[4], salve->value[0], ch, victim, NULL );
   if( retcode == rNONE )
      retcode = obj_cast_spell( salve->value[5], salve->value[0], ch, victim, NULL );
   if( retcode == rCHAR_DIED || retcode == rBOTH_DIED )
   {
      bug( "%s: char died", __FUNCTION__ );
      return;
   }

   if( salve && salve->value[1] <= 0 )
      extract_obj( salve );
}

/* generate an action description message */
void actiondesc( CHAR_DATA *ch, OBJ_DATA *obj )
{
   char charbuf[MSL], roombuf[MSL];
   char *srcptr = obj->action_desc;
   char *charptr = charbuf;
   char *roomptr = roombuf;
   const char *ichar = "You";
   const char *iroom = "Someone";

   while( *srcptr != '\0' )
   {
      if( *srcptr == '$' )
      {
         srcptr++;
         switch( *srcptr )
         {
            case 'e':
               ichar = "you";
               iroom = "$e";
               break;

            case 'm':
               ichar = "you";
               iroom = "$m";
               break;

            case 'n':
               ichar = "you";
               iroom = "$n";
               break;

            case 's':
               ichar = "your";
               iroom = "$s";
               break;

            default:
               srcptr--;
               *charptr++ = *srcptr;
               *roomptr++ = *srcptr;
               break;
         }
      }
      else if( *srcptr == '%' && *++srcptr == 's' )
      {
         ichar = "You";
         iroom = "$n";
      }
      else
      {
         *charptr++ = *srcptr;
         *roomptr++ = *srcptr;
         srcptr++;
         continue;
      }

      while( ( *charptr = *ichar ) != '\0' )
      {
         charptr++;
         ichar++;
      }

      while( ( *roomptr = *iroom ) != '\0' )
      {
         roomptr++;
         iroom++;
      }
      srcptr++;
   }

   *charptr = '\0';
   *roomptr = '\0';

   switch( obj->item_type )
   {
      case ITEM_BLOOD:
      case ITEM_FOUNTAIN:
         act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
         return;

      case ITEM_DRINK_CON:
         act( AT_ACTION, charbuf, ch, obj, liq_table[obj->value[2]].liq_name, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, liq_table[obj->value[2]].liq_name, TO_ROOM );
         return;

      case ITEM_PIPE:
         return;

      case ITEM_ARMOR:
      case ITEM_WEAPON:
      case ITEM_LIGHT:
         return;

      case ITEM_COOK:
      case ITEM_FOOD:
      case ITEM_PILL:
         act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
         return;

      default:
         return;
   }
}

/* Extended Bitvector Routines - Thoric */

/* check to see if the extended bitvector is completely empty */
bool ext_is_empty( EXT_BV *bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      if( bits->bits[x] != 0 )
         return false;

   return true;
}

void ext_clear_bits( EXT_BV *bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      bits->bits[x] = 0;
}

/* for use by xHAS_BITS( ) -- works like IS_SET( ) */
int ext_has_bits( EXT_BV *var, EXT_BV *bits )
{
   int x, bit;

   for( x = 0; x < XBI; x++ )
      if( ( bit = ( var->bits[x] & bits->bits[x] ) ) != 0 )
         return bit;

   return 0;
}

/* I found has_bits to return on any match and I want one to only return if all bits the set ones on bits are set on var */
bool ext_has_all_bits( EXT_BV *var, EXT_BV *bits )
{
   int x, possible = 0, matched = 0;

   for( x = 0; x < MAX_BITS; x++ )
   {
      if( xIS_SET( *bits, x ) )
      {
         possible++;
         if( !xIS_SET( *var, x ) )
            return false;
         else
            matched++;
      }
   }

   if( possible == 0 || matched != possible )
      return false;
   return true;
}

/* for use by xSAME_BITS( ) -- works like == */
bool ext_same_bits( EXT_BV *var, EXT_BV *bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      if( var->bits[x] != bits->bits[x] )
         return false;

   return true;
}

/* for use by xSET_BITS( ) -- works like SET_BIT( ) */
void ext_set_bits( EXT_BV *var, EXT_BV *bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      var->bits[x] |= bits->bits[x];
}

/* for use by xREMOVE_BITS( ) -- works like REMOVE_BIT( ) */
void ext_remove_bits( EXT_BV *var, EXT_BV *bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      var->bits[x] &= ~( bits->bits[x] );
}

/* for use by xTOGGLE_BITS( ) -- works like TOGGLE_BIT( ) */
void ext_toggle_bits( EXT_BV *var, EXT_BV *bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      var->bits[x] ^= bits->bits[x];
}

/* Read an extended bitvector from a file. - Thoric */
EXT_BV fread_bitvector( FILE *fp )
{
   EXT_BV ret;
   int c, x = 0;
   int num = 0;

   memset( &ret, '\0', sizeof( ret ) );
   for( ;; )
   {
      num = fread_number( fp );
      if( x < XBI )
         ret.bits[x] = num;
      ++x;
      if( ( c = getc( fp ) ) != '&' )
      {
         ungetc( c, fp );
         break;
      }
   }

   return ret;
}

/* return a string for writing a bitvector to a file */
char *print_bitvector( EXT_BV *bits )
{
   static char buf[XBI * 12];
   char *p = buf;
   int x, cnt = 0;

   for( cnt = XBI - 1; cnt > 0; cnt-- )
      if( bits->bits[cnt] )
         break;
   for( x = 0; x <= cnt; x++ )
   {
      snprintf( p, sizeof( buf ) - ( p - buf ), "%d", bits->bits[x] );
      p += strlen( p );
      if( x < cnt )
         *p++ = '&';
   }
   *p = '\0';

   return buf;
}

/* Write an extended bitvector to a file - Thoric */
void fwrite_bitvector( EXT_BV *bits, FILE *fp )
{
   fputs( print_bitvector( bits ), fp );
}

EXT_BV meb( int bit )
{
   EXT_BV bits;

   xCLEAR_BITS( bits );
   if( bit >= 0 )
      xSET_BIT( bits, bit );

   return bits;
}

EXT_BV multimeb( int bit, ... )
{
   EXT_BV bits;
   va_list param;
   int b;

   xCLEAR_BITS( bits );
   if( bit < 0 )
      return bits;

   xSET_BIT( bits, bit );

   va_start( param, bit );

   while( ( b = va_arg( param, int ) ) != -1 )
        xSET_BIT( bits, b );

   va_end( param );

   return bits;
}
