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
 *                  Shop and repair shop module                              *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"

void oprog_repair_trigger( CHAR_DATA *ch, OBJ_DATA *obj );

bool canuseshop( CHAR_DATA *ch, CHAR_DATA *keeper )
{
   CHAR_DATA *whof;
   int speakswell, speaking = -1, lang;

   if( ( whof = who_fighting( keeper ) ) )
   {
      if( whof == ch )
         send_to_char( "I don't think that's a good idea...\r\n", ch );
      else
         interpret( keeper, (char *)"say I'm too busy for that!" );
      return false;
   }
   if( who_fighting( ch ) )
   {
      ch_printf( ch, "%s doesn't seem to want to get involved.\r\n", PERS( keeper, ch ) );
      return false;
   }
   if( keeper->position == POS_SLEEPING )
   {
      send_to_char( "While they're asleep?\r\n", ch );
      return false;
   }
   if( keeper->position < POS_SLEEPING )
   {
      send_to_char( "I don't think they can hear you...\r\n", ch );
      return false;
   }
   if( !can_see( keeper, ch ) )
   {
      interpret( keeper, (char *)"say I don't trade with folks I can't see." );
      return false;
   }
   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( xIS_SET( ch->speaking, lang_array[lang] ) )
      {
         speaking = lang;
         break;
      }
   }
   speakswell = UMIN( knows_language( keeper, speaking ), knows_language( ch, speaking ) );
   if( ( number_percent( ) % 65 ) > speakswell )
   {
      if( speakswell > 60 )
         interpret_printf( keeper, "tell 0.%s Could you repeat that?  I didn't quite catch it.", ch->name );
      else if( speakswell > 50 )
         interpret_printf( keeper, "tell 0.%s Could you say that a little more clearly please?", ch->name );
      else if( speakswell > 40 )
         interpret_printf( keeper, "tell 0.%s Sorry... What was that you wanted?", ch->name );
      else
         interpret_printf( keeper, "tell 0.%s I can't understand you.", ch->name );
      return false;
   }
   return true;
}

/* Shopping commands. */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
   CHAR_DATA *keeper;
   SHOP_DATA *pShop;

   pShop = NULL;
   for( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
      if( is_npc( keeper ) && ( pShop = keeper->pIndexData->pShop ) )
         break;

   if( !pShop )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return NULL;
   }

   if( pShop->open_hour > pShop->close_hour )
   {
      if( time_info.hour < pShop->open_hour && time_info.hour > pShop->close_hour )
      {
         interpret( keeper, (char *)"say Sorry, come back later." );
         return NULL;
      }
   }
   else
   {
      if( time_info.hour < pShop->open_hour )
      {
         interpret( keeper, (char *)"say Sorry, come back later." );
         return NULL;
      }
      if( time_info.hour > pShop->close_hour )
      {
         interpret( keeper, (char *)"say Sorry, come back tomorrow." );
         return NULL;
      }
   }

   if( !canuseshop( ch, keeper ) )
      return NULL;

   return keeper;
}

/* repair commands. */
CHAR_DATA *find_fixer( CHAR_DATA *ch )
{
   CHAR_DATA *keeper;
   REPAIR_DATA *rShop;

   rShop = NULL;
   for( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
      if( is_npc( keeper ) && ( rShop = keeper->pIndexData->rShop ) )
         break;

   if( !rShop )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return NULL;
   }

   if( rShop->open_hour > rShop->close_hour )
   {
      if( time_info.hour < rShop->open_hour && time_info.hour > rShop->close_hour )
      {
         interpret( keeper, (char *)"say Sorry, come back later." );
         return NULL;
      }
   }
   else
   {
      if( time_info.hour < rShop->open_hour )
      {
         interpret( keeper, (char *)"say Sorry, come back later." );
         return NULL;
      }
      if( time_info.hour > rShop->close_hour )
      {
         interpret( keeper, (char *)"say Sorry, come back tomorrow." );
         return NULL;
      }
   }

   if( !canuseshop( ch, keeper ) )
      return NULL;

   return keeper;
}

int get_cost( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
   SHOP_DATA *pShop;
   int cost, profitmod;

   if( !obj || !( pShop = keeper->pIndexData->pShop ) || obj->bsplatter != 0 || obj->bstain != 0 )
      return 0;

   if( fBuy )
   {
      profitmod = ch->level - get_curr_cha( ch ) + ( ( URANGE( 5, ch->level, MAX_LEVEL ) - 20 ) / 2 );
      cost = ( int )( obj->cost * UMAX( ( pShop->profit_sell + 1 ), pShop->profit_buy + profitmod ) ) / 100;
      cost = ( int )( cost * ( 80 + UMIN( ch->level, MAX_LEVEL ) ) ) / 100;
   }
   else
   {
      OBJ_DATA *obj2;
      int itype;

      profitmod = get_curr_cha( ch ) - ch->level;
      cost = 0;
      for( itype = 0; itype < ITEM_TYPE_MAX; itype++ )
      {
         if( itype == obj->item_type && pShop->buy_type[itype] )
         {
            cost = ( int )( obj->cost * UMIN( ( pShop->profit_buy - 1 ), pShop->profit_sell + profitmod ) ) / 100;
            break;
         }
      }

      for( obj2 = keeper->first_carrying; obj2; obj2 = obj2->next_content )
      {
         if( obj->pIndexData == obj2->pIndexData )
         {
            cost = 0;
            break;
         }
      }
   }

   if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
      cost = ( int )( cost * obj->value[2] / obj->value[1] );

   return cost;
}

int get_repaircost( CHAR_DATA *keeper, OBJ_DATA *obj )
{
   REPAIR_DATA *rShop;
   int cost = 0, itype;
   bool found;

   if( !obj || !( rShop = keeper->pIndexData->rShop ) )
      return 0;

   found = false;
   for( itype = 0; itype < ITEM_TYPE_MAX; itype++ )
   {
      if( !rShop->fix_type[itype] || obj->item_type != itype )
         continue;
      cost = ( ( obj->cost * rShop->profit_fix ) / 1000 );
      found = true;
      break;
   }

   if( !found )
      return -1;

   if( cost == 0 )
      cost = 1;

   switch( obj->item_type )
   {
      default:
         return -1;
         break;

      case ITEM_CONTAINER:
      case ITEM_KEYRING:
      case ITEM_QUIVER:
         if( obj->value[3] >= obj->value[4] )
            return 0;
         else
            cost *= ( obj->value[4] - obj->value[3] );
         break;

      case ITEM_ARMOR:
      case ITEM_LIGHT:
      case ITEM_AXE:
      case ITEM_LOCKPICK:
      case ITEM_SHOVEL:
         if( obj->value[0] >= obj->value[1] )
            return 0;
         else
            cost *= ( obj->value[1] - obj->value[0] );
         break;

      case ITEM_MISSILE_WEAPON:
      case ITEM_WEAPON:
         if( obj->value[0] >= INIT_WEAPON_CONDITION )
            return 0;
         else
            cost *= ( INIT_WEAPON_CONDITION - obj->value[0] );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         if( obj->value[2] >= obj->value[1] )
            return 0;
         else
            cost *= ( obj->value[1] - obj->value[2] );
         break;
   }

   return cost;
}

CMDF( do_buy )
{
   char arg[MIL], buf[MSL];
   int maxgold, count = 0;

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Buy what?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_PET_SHOP ) )
   {
      CHAR_DATA *pet;
      ROOM_INDEX_DATA *pRoomIndexNext, *in_room;

      if( is_npc( ch ) )
         return;

      for( pet = ch->pcdata->first_pet; pet; pet = pet->next_pet )
         count++;

      if( count >= sysdata.maxpet )
      {
         send_to_char( "Sorry, but you already have the max amount of pets you can have.\r\n", ch );
         return;
      }

      if( !( pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 ) ) )
      {
         bug( "%s: bad pet shop at vnum %d.", __FUNCTION__, ch->in_room->vnum );
         send_to_char( "Sorry, you can't buy that here.\r\n", ch );
         return;
      }

      in_room = ch->in_room;
      ch->in_room = pRoomIndexNext;
      pet = get_char_room( ch, arg );
      ch->in_room = in_room;

      if( !pet || !is_npc( pet ) || !xIS_SET( pet->act, ACT_PET ) )
      {
         send_to_char( "Sorry, you can't buy that here.\r\n", ch );
         return;
      }

      if( ch->level < pet->level )
      {
         send_to_char( "You're not ready for this pet.\r\n", ch );
         return;
      }

      maxgold = ( 10 * pet->level * pet->level );

      if( !has_gold( ch, maxgold ) )
      {
         send_to_char( "You can't afford it.\r\n", ch );
         return;
      }

      if( !( pet = create_mobile( pet->pIndexData ) ) )
      {
         send_to_char( "There was an issue creating this pet for you.\r\n", ch );
         bug( "%s: couldn't create a pet from room %d.", __FUNCTION__, pRoomIndexNext->vnum );
      }
      xSET_BIT( pet->act, ACT_PET );
      xSET_BIT( pet->affected_by, AFF_CHARM );

      decrease_gold( ch, maxgold );

      argument = one_argument( argument, arg );
      if( arg != NULL && arg[0] != '\0' )
      {
         snprintf( buf, sizeof( buf ), "%s %s", pet->name, arg );
         STRSET( pet->name, buf );
      }

      snprintf( buf, sizeof( buf ), "%sA neck tag says 'I belong to %s'.\r\n",
         pet->description ? pet->description : "", ch->name );
      STRSET( pet->description, buf );

      char_to_room( pet, ch->in_room );
      add_follower( pet, ch );

      send_to_char( "Enjoy your pet.\r\n", ch );
      act( AT_ACTION, "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
      return;
   }
   else
   {
      CHAR_DATA *keeper;
      OBJ_DATA *obj;
      int cost;
      int noi = 1;
      short mnoi = 20;  /* Max number of items to be bought at once */

      if( !( keeper = find_keeper( ch ) ) )
         return;

      maxgold = keeper->level * keeper->level * 50000;

      if( is_number( arg ) )
      {
         noi = atoi( arg );
         argument = one_argument( argument, arg );
         if( noi > mnoi )
         {
            act_tell( ch, keeper, "$n tells you 'I don't sell that many items at once.'", keeper, NULL, ch, TO_VICT );
            return;
         }
      }

      if( !( obj = get_obj_carry( keeper, arg ) ) )
      {
         act_tell( ch, keeper, "$n tells you 'I don't sell that -- try 'list'.'", keeper, NULL, ch, TO_VICT );
         return;
      }

      cost = ( get_cost( ch, keeper, obj, true ) * noi );
      if( cost <= 0 || !can_see_obj( ch, obj ) )
      {
         act_tell( ch, keeper, "$n tells you 'I don't sell that -- try 'list'.'", keeper, NULL, ch, TO_VICT );
         return;
      }

      if( !is_obj_stat( obj, ITEM_INVENTORY ) && ( noi > 1 ) )
      {
         interpret( keeper, (char *)"laugh" );
         act_tell( ch, keeper, "$n tells you 'I don't have enough of those in stock to sell more than one at a time.'", keeper, NULL, ch, TO_VICT );
         return;
      }

      if( !str_cmp( argument, "check" ) )
      {
         bool inventory = false;

         act_tell( ch, keeper, "$n tells you 'Here you are, it's always nice to check something out before you buy it.'", keeper, NULL, ch, TO_VICT );
         if( is_obj_stat( obj, ITEM_INVENTORY ) )
         {
            xREMOVE_BIT( obj->extra_flags, ITEM_INVENTORY );
            inventory = true;
         }
         show_obj( ch, obj );
         if( inventory )
            xSET_BIT( obj->extra_flags, ITEM_INVENTORY );
         return;
      }

      if( !has_gold( ch, cost ) )
      {
         act_tell( ch, keeper, "$n tells you 'You can't afford to buy $p.'", keeper, obj, ch, TO_VICT );
         return;
      }

      if( obj->level > ch->level )
      {
         act_tell( ch, keeper, "$n tells you 'You can't use $p yet.'", keeper, obj, ch, TO_VICT );
         return;
      }

      if( is_obj_stat( obj, ITEM_PROTOTYPE ) && get_trust( ch ) < PERM_IMM )
      {
         act_tell( ch, keeper, "$n tells you 'This is a only a prototype!  I can't sell you that...'", keeper, NULL, ch, TO_VICT );
         return;
      }

      if( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
      {
         send_to_char( "You can't carry that many items.\r\n", ch );
         return;
      }

      if( ch->carry_weight + ( get_obj_weight( obj ) * noi ) + ( noi > 1 ? 2 : 0 ) > can_carry_w( ch ) )
      {
         send_to_char( "You can't carry that much weight.\r\n", ch );
         return;
      }

      if( noi == 1 )
      {
         act( AT_ACTION, "$n buys $p.", ch, obj, NULL, TO_ROOM );
         act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "You buy $p for %s gold.", num_punct( cost ) );
      }
      else
      {
         act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n buys %d $p%s.", noi,
            ( obj->short_descr[strlen( obj->short_descr ) - 1] == 's' ? "" : "s" ) );
         act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "You buy %d $p%s for %s gold.", noi,
            ( obj->short_descr[strlen( obj->short_descr ) - 1] == 's' ? "" : "s" ), num_punct( cost ) );
         act( AT_ACTION, "$N puts them into a bag and hands it to you.", ch, NULL, keeper, TO_CHAR );
      }

      decrease_gold( ch, cost );

      /* If he can't hold all the gold show him putting some into a safe */
      if( !can_hold_gold( keeper, cost ) )
         act( AT_ACTION, "$n puts some gold into a large safe.", keeper, NULL, NULL, TO_ROOM );

      increase_gold( keeper, cost );

      if( is_obj_stat( obj, ITEM_INVENTORY ) )
      {
         OBJ_DATA *buy_obj, *bag;

         buy_obj = create_object( obj->pIndexData, obj->level );

         /*
          * Due to grouped objects and carry limitations in SMAUG
          * The shopkeeper gives you a bag with multiple-buy,
          * and also, only one object needs be created with a count
          * set to the number bought.    -Thoric
          */
         if( noi > 1 )
         {
            bag = create_object( get_obj_index( OBJ_VNUM_SHOPPING_BAG ), 1 );
            xSET_BIT( bag->extra_flags, ITEM_GROUNDROT );
            bag->timer = 10;  /* Blodkai, 4/97 */
            /* perfect size bag ;) */
            bag->value[0] = bag->weight + ( buy_obj->weight * noi );
            buy_obj->count = noi;
            obj->pIndexData->count += ( noi - 1 );
            numobjsloaded += ( noi - 1 );
            obj_to_obj( buy_obj, bag );
            obj_to_char( bag, ch );
         }
         else
            obj_to_char( buy_obj, ch );
      }
      else
      {
         obj_from_char( obj );
         obj_to_char( obj, ch );
      }
      return;
   }
}

/*
 * This is a new list function which allows level limits to follow as
 * arguments.  This code relies heavily on the items held by the shopkeeper
 * being sorted in descending order by level.  obj_to_char in handler.c was
 * modified to achieve this.  Anyways, this list command will now throw flags
 * at the levels passed as arguments.  This helps pick out equipment which is
 * usable by the char, etc.  This was done rather than just producing a list
 * of equip at desired level because there would be an inconsistency between
 * #.item on one list and #.item on the other.
 * Usage:
 *    list          -   list the items for sale
 *    list #        -   list items starting at specified level
 *    list #1 #2    -   list items within specified levels
 *    list <name>   -   list items matching <name>
 * Note that this won't work in pets stores. Since you can't control
 * the order in which the pets repop you can't guarantee a sorted list.
 * Last Modified: May 25, 1997 -- Fireblade
 */
CMDF( do_list )
{
   set_char_color( AT_OBJECT, ch );

   if( xIS_SET( ch->in_room->room_flags, ROOM_PET_SHOP ) )
   {
      ROOM_INDEX_DATA *pRoomIndexNext;
      CHAR_DATA *pet;
      bool found;

      if( !( pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 ) ) )
      {
         bug( "%s: bad pet shop at vnum %d.", __FUNCTION__, ch->in_room->vnum );
         send_to_char( "You can't do that here.\r\n", ch );
         return;
      }

      found = false;

      for( pet = pRoomIndexNext->first_person; pet; pet = pet->next_in_room )
      {
         if( xIS_SET( pet->act, ACT_PET ) && is_npc( pet ) )
         {
            if( !found )
            {
               found = true;
               send_to_pager( "Pets for sale:\r\n", ch );
            }
            pager_printf( ch, "[%2d] %8d - %s\r\n", pet->level, 10 * pet->level * pet->level, pet->short_descr );
         }
      }
      if( !found )
         send_to_char( "Sorry, we're out of pets right now.\r\n", ch );
   }
   else
   {
      CHAR_DATA *keeper;
      OBJ_DATA *obj;
      char arg[MIL], *rest;
      int cost, lower = 0, upper = MAX_LEVEL;
      bool found = false, fsomething = false;

      rest = one_argument( argument, arg );

      if( !( keeper = find_keeper( ch ) ) )
         return;

      /* Get the level limits for the flags */
      if( is_number( arg ) )
      {
         lower = atoi( arg );
         rest = one_argument( rest, arg );

         if( is_number( arg ) )
         {
            upper = atoi( arg );
            rest = one_argument( rest, arg );
         }
      }

      /* Fix the limits if reversed */
      if( lower >= upper )
      {
         int temp;
         temp = lower;
         lower = upper;
         upper = temp;
      }

      for( obj = keeper->first_carrying; obj; obj = obj->next_content )
      {
         if( obj->wear_loc != WEAR_NONE || !can_see_obj( ch, obj ) || ( cost = get_cost( ch, keeper, obj, true ) ) <= 0 )
            continue;

         fsomething = true; /* Something was found */

         if( ( obj->level >= lower && obj->level <= upper ) && ( arg == NULL || arg[0] == '\0' || nifty_is_name( arg, obj->name ) ) )
         {
            if( !found )
               send_to_pager( "[Lv   Price] Item\r\n", ch );
            found = true;
            pager_printf( ch, "&c[&C%2d &Y%7s&c] &[objects]%s&D.\r\n", obj->level, num_punct( cost ), capitalize( obj->short_descr ) );
         }
      }

      if( !found )
      {
         if( fsomething )
            send_to_char( "You can't find anything matching that.\r\n", ch );
         else if( arg != NULL && arg[0] != '\0' )
            send_to_char( "You can't buy that here.\r\n", ch );
         else
            send_to_char( "You can't buy anything here.\r\n", ch );
      }
   }
}

CMDF( do_sell )
{
   CHAR_DATA *keeper;
   OBJ_DATA *obj;
   char buf[MSL], arg[MIL];
   int cost;

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Sell what?\r\n", ch );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
      return;

   if( !( obj = get_obj_carry( ch, arg ) ) )
   {
      act_tell( ch, keeper, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      return;
   }

   separate_obj( obj );

   /* Bug report and solution thanks to animal@netwin.co.nz */
   if( !can_see_obj( keeper, obj ) )
   {
      send_to_char( "What are you trying to sell me? I don't buy thin air!\r\n", ch );
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it!\r\n", ch );
      return;
   }

   if( obj->bsplatter > 0 )
   {
      act_tell( ch, keeper, "$n tells you, '$p is covered in blood!!!'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( obj->bstain > 0 )
   {
      act_tell( ch, keeper, "$n tells you, '$p is stained with blood!!!'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( obj->timer > 0 )
   {
      act_tell( ch, keeper, "$n tells you, '$p is depreciating in value too quickly...'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( ( cost = get_cost( ch, keeper, obj, false ) ) <= 0 )
   {
      act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
      return;
   }

   /* Allow items to be sold for less then they are worth */
   if( argument && argument[0] != '\0' && is_number( argument ) )
   {
      int sfor = atoi( argument );

      if( sfor < 0 || sfor > cost )
      {
         send_to_char( "You can't sell it for less then 0 or more then it's worth.\r\n", ch );
         return;
      }

      cost = sfor;
   }

   if( !has_gold( keeper, cost ) )
   {
      act_tell( ch, keeper, "$n tells you, '$p is worth more than I can afford...'", keeper, obj, ch, TO_VICT );
      if( keeper->gold > 0 )
         act_printf( AT_TELL, keeper, obj, ch, TO_VICT, "$n tells you, 'The most I could give you for $p is %s gold pieces.'", show_char_gold( keeper ) ); 
      return;
   }

   if( !can_hold_gold( ch, cost ) )
   {
      act_tell( ch, keeper, "$n tells you, 'You can't hold that much gold anyways...'", keeper, obj, ch, TO_VICT );
      return;
   }

   act( AT_ACTION, "$n sells $p.", ch, obj, NULL, TO_ROOM );
   snprintf( buf, sizeof( buf ), "You sell $p for %d gold piece%s.", cost, cost == 1 ? "" : "s" );
   act( AT_ACTION, buf, ch, obj, NULL, TO_CHAR );
   decrease_gold( keeper, cost );
   increase_gold( ch, cost );

   if( obj->item_type == ITEM_TRASH )
      extract_obj( obj );
   else
   {
      obj_from_char( obj );
      obj_to_char( obj, keeper );
   }
}

CMDF( do_value )
{
   CHAR_DATA *keeper;
   OBJ_DATA *obj;
   char buf[MSL];
   int cost;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Value what?\r\n", ch );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
      return;

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      act_tell( ch, keeper, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      return;
   }

   separate_obj( obj );

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it!\r\n", ch );
      return;
   }

   if( obj->bsplatter > 0 )
   {
      act( AT_ACTION, "$n doesn't want to get blood all over $m!", keeper, obj, ch, TO_VICT );
      return;
   }

   if( obj->bstain > 0 )
   {
      act( AT_ACTION, "$n doesn't want something stained with blood!", keeper, obj, ch, TO_VICT );
      return;
   }

   if( ( cost = get_cost( ch, keeper, obj, false ) ) <= 0 )
   {
      act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
      return;
   }

   snprintf( buf, sizeof( buf ), "$n tells you 'I'll give you %d gold coins for $p.'", cost );
   act_tell( ch, keeper, buf, keeper, obj, ch, TO_VICT );
}

/* Repair a single object. Used when handling "repair all" - Gorog */
void repair_one_obj( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj, char *arg, int maxgold )
{
   char buf[MSL], *fixstr, *fixstr2;
   int cost;

   if( !obj || !ch || !keeper )
      return;

   if( obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF )
   {
      fixstr = (char *)"recharge";
      fixstr2 = (char *)"recharges";
   }
   else
   {
      fixstr = (char *)"repair";
      fixstr2 = (char *)"repairs";
   }

   if( !can_drop_obj( ch, obj ) )
      ch_printf( ch, "You can't let go of %s.\r\n", obj->name );
   else if( obj->bsplatter > 0 )
   {
      act_tell( ch, keeper, "$n tells you, 'Sorry, I don't want to get blood on me.'", keeper, obj, ch, TO_VICT );
   }
   else if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
   {
      act_tell( ch, keeper, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
   }
   /* "repair all" gets a 10% surcharge - Gorog */
   else if( ( cost = strcmp( "all", arg ) ? cost : 11 * cost / 10 ) > ch->gold )
   {
      snprintf( buf, sizeof( buf ), "$N tells you, 'It will cost %d piece%s of gold to %s %s...'", cost,
         cost == 1 ? "" : "s", fixstr, obj->name );
      act_tell( ch, keeper, buf, ch, NULL, keeper, TO_CHAR );
      act_tell( ch, keeper, "$N tells you, 'Which I see you can't afford.'", ch, NULL, keeper, TO_CHAR );
   }
   else
   {
      if( cost == 0 )
      {
         act_tell( ch, keeper, "$n tells you, '$p looks fine to me.'", keeper, obj, ch, TO_VICT );
         return;
      }
      snprintf( buf, sizeof( buf ), "$n gives $p to $N, who quickly %s it.", fixstr2 );
      act( AT_ACTION, buf, ch, obj, keeper, TO_ROOM );
      snprintf( buf, sizeof( buf ), "$N charges you %d gold piece%s to %s $p.", cost, cost == 1 ? "" : "s", fixstr );
      act( AT_ACTION, buf, ch, obj, keeper, TO_CHAR );
      decrease_gold( ch, cost );
      increase_gold( keeper, cost );
      if( has_gold( keeper, maxgold ) )
      {
         decrease_gold( keeper, ( maxgold / 2 ) );
         act( AT_ACTION, "$n puts some gold into a large safe.", keeper, NULL, NULL, TO_ROOM );
      }

      switch( obj->item_type )
      {
         default:
            send_to_char( "For some reason, you think you got ripped off...\r\n", ch );
            break;

         case ITEM_CONTAINER:
         case ITEM_KEYRING:
         case ITEM_QUIVER:
            obj->value[3] = obj->value[4];
            break;

         case ITEM_LIGHT:
         case ITEM_AXE:
         case ITEM_LOCKPICK:
         case ITEM_SHOVEL:
         case ITEM_ARMOR:
            obj->value[0] = obj->value[1];
            break;

         case ITEM_MISSILE_WEAPON:
         case ITEM_WEAPON:
            obj->value[0] = INIT_WEAPON_CONDITION;
            break;

         case ITEM_WAND:
         case ITEM_STAFF:
            obj->value[2] = obj->value[1];
            break;
      }

      oprog_repair_trigger( ch, obj );
   }
}

CMDF( do_repair )
{
   CHAR_DATA *keeper;
   OBJ_DATA *obj;
   int maxgold;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Repair what?\r\n", ch );
      return;
   }

   if( !( keeper = find_fixer( ch ) ) )
      return;

   maxgold = keeper->level * keeper->level * 100000;

   if( !strcmp( argument, "all" ) )
   {
      for( obj = ch->first_carrying; obj; obj = obj->next_content )
      {
         separate_obj( obj );
         if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) && can_see_obj( keeper, obj ) )
            repair_one_obj( ch, keeper, obj, argument, maxgold );
      }
      return;
   }

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      act_tell( ch, keeper, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      return;
   }

   separate_obj( obj );
   repair_one_obj( ch, keeper, obj, argument, maxgold );
}

void appraise_all( CHAR_DATA *ch, CHAR_DATA *keeper )
{
   OBJ_DATA *obj;
   char buf[MSL], *fixstr;
   int cost = 0, total = 0;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      separate_obj( obj );

      if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
      {
         if( obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF )
            fixstr = (char *)"recharge";
         else
            fixstr = (char *)"repair";

         if( !can_drop_obj( ch, obj ) )
            ch_printf( ch, "You can't let go of %s.\r\n", obj->name );
         else if( obj->bsplatter > 0 )
         {
            act_tell( ch, keeper, "$n tells you, 'Sorry, I don't want to get blood on me.'", keeper, obj, ch, TO_VICT );
         }
         else if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
         {
            act_tell( ch, keeper, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
         }
         else if( cost == 0 )
         {
            act_tell( ch, keeper, "$n tells you, '$p looks fine to me.'", keeper, obj, ch, TO_VICT );
         }
         else
         {
            snprintf( buf, sizeof( buf ), "$N tells you, 'It will cost %d piece%s of gold to %s %s'",
               cost, cost == 1 ? "" : "s", fixstr, obj->name );
            act_tell( ch, keeper, buf, ch, NULL, keeper, TO_CHAR );
            total += cost;
         }
      }
   }
   if( total > 0 )
   {
      send_to_char( "\r\n", ch );
      snprintf( buf, sizeof( buf ), "$N tells you, 'It will cost %d piece%s of gold in total.'", total,
         cost == 1 ? "" : "s" );
      act_tell( ch, keeper, buf, ch, NULL, keeper, TO_CHAR );
      act_tell( ch, keeper, "$N tells you, 'Remember there is a 10% surcharge for repair all.'", ch, NULL, keeper, TO_CHAR );
   }
}

CMDF( do_appraise )
{
   CHAR_DATA *keeper;
   OBJ_DATA *obj;
   char buf[MSL], arg[MIL], *fixstr;
   int cost;

   one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Appraise what?\r\n", ch );
      return;
   }

   if( !( keeper = find_fixer( ch ) ) )
      return;

   if( !strcmp( arg, "all" ) )
   {
      appraise_all( ch, keeper );
      return;
   }

   if( !( obj = get_obj_carry( ch, arg ) ) )
   {
      act_tell( ch, keeper, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      return;
   }

   separate_obj( obj );
   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it.\r\n", ch );
      return;
   }

   if( obj->bsplatter > 0 )
   {
      act_tell( ch, keeper, "$n tells you, 'Sorry, I don't want to get blood on me.'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
   {
      act_tell( ch, keeper, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( cost == 0 )
   {
      act_tell( ch, keeper, "$n tells you, '$p looks fine to me.'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF )
      fixstr = (char *)"recharge";
   else
      fixstr = (char *)"repair";

   snprintf( buf, sizeof( buf ), "$N tells you, 'It will cost %d piece%s of gold to %s that...'", cost,
      cost == 1 ? "" : "s", fixstr );
   act_tell( ch, keeper, buf, ch, NULL, keeper, TO_CHAR );
   if( !has_gold( ch, cost ) )
      act_tell( ch, keeper, "$N tells you, 'Which I see you can't afford.'", ch, NULL, keeper, TO_CHAR );
}

/* ------------------ Shop Building and Editing Section ----------------- */
CMDF( do_makeshop )
{
   SHOP_DATA *shop;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makeshop <mobvnum>\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !can_medit( ch, mob ) )
      return;

   if( mob->pShop )
   {
      send_to_char( "This mobile already has a shop.\r\n", ch );
      return;
   }

   CREATE( shop, SHOP_DATA, 1 );

   LINK( shop, first_shop, last_shop, next, prev );
   shop->keeper = vnum;
   shop->profit_buy = 120;
   shop->profit_sell = 90;
   shop->open_hour = 0;
   shop->close_hour = 23;
   mob->pShop = shop;
   send_to_char( "Done.\r\n", ch );
}

CMDF( do_shopset )
{
   SHOP_DATA *shop;
   MOB_INDEX_DATA *mob, *mob2;
   char arg1[MIL], arg2[MIL];
   int vnum, value;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Usage: shopset <mob vnum> <field> <value>\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "  buytype buy sell open close keeper\r\n", ch );
      return;
   }

   vnum = atoi( arg1 );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !can_medit( ch, mob ) )
      return;

   if( !mob->pShop )
   {
      send_to_char( "This mobile doesn't keep a shop.\r\n", ch );
      return;
   }
   shop = mob->pShop;
   value = atoi( argument );

   if( !str_cmp( arg2, "buytype" ) )
   {
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         if( !is_number( arg1 ) )
            value = get_flag( arg1, o_types, ITEM_TYPE_MAX );
         if( value < 0 || value >= ITEM_TYPE_MAX )
            ch_printf( ch, "%s is an Invalid item type!\r\n", arg1 );
         else
            shop->buy_type[value] = !shop->buy_type[value];
      }
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "buy" ) )
   {
      if( value <= ( shop->profit_sell + 5 ) || value > 1000 )
      {
         send_to_char( "Out of range.\r\n", ch );
         return;
      }
      shop->profit_buy = value;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "sell" ) )
   {
      if( value < 0 || value >= ( shop->profit_buy - 5 ) )
      {
         send_to_char( "Out of range.\r\n", ch );
         return;
      }
      shop->profit_sell = value;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "open" ) )
   {
      if( value < 0 || value > 23 )
      {
         send_to_char( "Out of range.\r\n", ch );
         return;
      }
      shop->open_hour = value;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "close" ) )
   {
      if( value < 0 || value > 23 )
      {
         send_to_char( "Out of range.\r\n", ch );
         return;
      }
      shop->close_hour = value;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "keeper" ) )
   {
      if( !( mob2 = get_mob_index( vnum ) ) )
      {
         send_to_char( "Mobile not found.\r\n", ch );
         return;
      }
      if( !can_medit( ch, mob ) )
         return;
      if( mob2->pShop )
      {
         send_to_char( "That mobile already has a shop.\r\n", ch );
         return;
      }
      mob->pShop = NULL;
      mob2->pShop = shop;
      shop->keeper = value;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   do_shopset( ch, (char *)"" );
}

CMDF( do_shopstat )
{
   SHOP_DATA *shop;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: shopstat <keeper vnum>\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !mob->pShop )
   {
      send_to_char( "This mobile doesn't keep a shop.\r\n", ch );
      return;
   }
   shop = mob->pShop;

   ch_printf( ch, "Keeper: %d  %s\r\n", shop->keeper, mob->short_descr );
   send_to_char( "Buytype:", ch );
   for( vnum = 0; vnum < ITEM_TYPE_MAX; vnum++ )
      if( shop->buy_type[vnum] )
         ch_printf( ch, " %s", o_types[vnum] );
   send_to_char( "\r\n", ch );
   ch_printf( ch, "Profit:  buy %3d%%  sell %3d%%\r\n", shop->profit_buy, shop->profit_sell );
   ch_printf( ch, "Hours:   open %2d  close %2d\r\n", shop->open_hour, shop->close_hour );
}

CMDF( do_shops )
{
   SHOP_DATA *shop;
   int sbuy;

   if( !first_shop )
   {
      send_to_char( "There are no shops.\r\n", ch );
      return;
   }

   set_char_color( AT_NOTE, ch );
   for( shop = first_shop; shop; shop = shop->next )
   {
      ch_printf( ch, "Keeper: %5d Buy: %3d Sell: %3d Open: %2d Close: %2d Buytype:",
         shop->keeper, shop->profit_buy, shop->profit_sell, shop->open_hour, shop->close_hour );
      for( sbuy = 0; sbuy < ITEM_TYPE_MAX; sbuy++ )
         if( shop->buy_type[sbuy] )
            ch_printf( ch, " %s", o_types[sbuy] );
      send_to_char( "\r\n", ch );
   }
}

/* -------------- Repair Shop Building and Editing Section -------------- */
CMDF( do_makerepair )
{
   REPAIR_DATA *repair;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makerepair <mobvnum>\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !can_medit( ch, mob ) )
      return;

   if( mob->rShop )
   {
      send_to_char( "This mobile already has a repair shop.\r\n", ch );
      return;
   }

   CREATE( repair, REPAIR_DATA, 1 );

   LINK( repair, first_repair, last_repair, next, prev );
   repair->keeper = vnum;
   repair->profit_fix = 100;
   repair->open_hour = 0;
   repair->close_hour = 23;
   mob->rShop = repair;
   send_to_char( "Done.\r\n", ch );
}

CMDF( do_repairset )
{
   REPAIR_DATA *repair;
   MOB_INDEX_DATA *mob, *mob2;
   char arg1[MIL], arg2[MIL];
   int vnum, value;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Usage: repairset <mob vnum> <field> value\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "  fixtype profit open close keeper\r\n", ch );
      return;
   }

   vnum = atoi( arg1 );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !can_medit( ch, mob ) )
      return;

   if( !mob->rShop )
   {
      send_to_char( "This mobile doesn't keep a repair shop.\r\n", ch );
      return;
   }
   repair = mob->rShop;
   value = atoi( argument );

   if( !str_cmp( arg2, "fixtype" ) )
   {
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         if( !is_number( arg1 ) )
            value = get_flag( arg1, o_types, ITEM_TYPE_MAX );
         if( value < 0 || value >= ITEM_TYPE_MAX )
            ch_printf( ch, "%s is an Invalid item type!\r\n", arg1 );
         else
            repair->fix_type[value] = !repair->fix_type[value];
      }
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "profit" ) )
   {
      repair->profit_fix = URANGE( 1, value, 1000 );
      ch_printf( ch, "Profit set to %d.\r\n", repair->profit_fix );
      return;
   }

   if( !str_cmp( arg2, "open" ) )
   {
      repair->open_hour = URANGE( 0, value, 23 );
      ch_printf( ch, "Open set to %d.\r\n", repair->open_hour );
      return;
   }

   if( !str_cmp( arg2, "close" ) )
   {
      repair->close_hour = URANGE( 0, value, 23 );
      ch_printf( ch, "Close set to %d.\r\n", repair->close_hour );
      return;
   }

   if( !str_cmp( arg2, "keeper" ) )
   {
      if( !( mob2 = get_mob_index( vnum ) ) )
      {
         send_to_char( "Mobile not found.\r\n", ch );
         return;
      }
      if( !can_medit( ch, mob ) )
         return;
      if( mob2->rShop )
      {
         send_to_char( "That mobile already has a repair shop.\r\n", ch );
         return;
      }
      mob->rShop = NULL;
      mob2->rShop = repair;
      repair->keeper = value;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   do_repairset( ch, (char *)"" );
}

CMDF( do_repairstat )
{
   REPAIR_DATA *repair;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: repairstat <keeper vnum>\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !mob->rShop )
   {
      send_to_char( "This mobile doesn't keep a repair shop.\r\n", ch );
      return;
   }
   repair = mob->rShop;

   ch_printf( ch, "Keeper: %d  %s\r\n", repair->keeper, mob->short_descr );
   send_to_char( "Fixtype:", ch );
   for( vnum = 0; vnum < ITEM_TYPE_MAX; vnum++ )
      if( repair->fix_type[vnum] )
         ch_printf( ch, " %s", o_types[vnum] );
   send_to_char( "\r\n", ch );
   ch_printf( ch, "Profit: %3d%%\r\n", repair->profit_fix );
   ch_printf( ch, "Hours:   open %2d  close %2d\r\n", repair->open_hour, repair->close_hour );
}

CMDF( do_repairshops )
{
   REPAIR_DATA *repair;
   int srepair;

   set_char_color( AT_NOTE, ch );
   if( !first_repair )
   {
      send_to_char( "There are no repair shops.\r\n", ch );
      return;
   }

   for( repair = first_repair; repair; repair = repair->next )
   {
      ch_printf( ch, "Keeper: %5d Profit: %3d Open: %2d Close: %2d Fixtype:",
         repair->keeper, repair->profit_fix, repair->open_hour, repair->close_hour );
      for( srepair = 0; srepair < ITEM_TYPE_MAX; srepair++ )
         if( repair->fix_type[srepair] )
            ch_printf( ch, " %s", o_types[srepair] );
      send_to_char( "\r\n", ch );
   }
}

CMDF( do_removeshop )
{
   SHOP_DATA *shop;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: removeshop <mobvnum>\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !can_medit( ch, mob ) )
      return;

   if( !mob->pShop )
   {
      send_to_char( "This mobile doesn't have a shop.\r\n", ch );
      return;
   }

   shop = mob->pShop;
   UNLINK( shop, first_shop, last_shop, next, prev );
   DISPOSE( shop );
   mob->pShop = NULL;
   send_to_char( "Shop deleted.\r\n", ch );
}

CMDF( do_removerepairshop )
{
   REPAIR_DATA *rshop;
   MOB_INDEX_DATA *mob;
   int vnum;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: removerepairshop <mobvnum>\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "Mobile not found.\r\n", ch );
      return;
   }

   if( !can_medit( ch, mob ) )
      return;

   if( !mob->rShop )
   {
      send_to_char( "This mobile doesn't have a repair shop.\r\n", ch );
      return;
   }

   rshop = mob->rShop;
   UNLINK( rshop, first_repair, last_repair, next, prev );
   DISPOSE( rshop );
   mob->rShop = NULL;
   send_to_char( "Repair shop deleted.\r\n", ch );
}
