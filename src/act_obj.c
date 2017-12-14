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
 *			   Object manipulation module			     *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "h/mud.h"

/* how resistant an object is to damage - Thoric */
short get_obj_resistance( OBJ_DATA *obj )
{
   short resist;

   resist = number_range( 1, MAX_ITEM_IMPACT );

   /* magical items are more resistant */
   if( is_obj_stat( obj, ITEM_MAGIC ) )
      resist += number_range( 1, 12 );

   /* metal objects are definately stronger */
   if( is_obj_stat( obj, ITEM_METAL ) )
      resist += number_range( 1, 5 );

   /* organic objects are most likely weaker */
   if( is_obj_stat( obj, ITEM_ORGANIC ) )
      resist -= number_range( 1, 5 );

   /* blessed objects should have a little bonus */
   if( is_obj_stat( obj, ITEM_BLESS ) )
      resist += number_range( 1, 5 );

   /* lets make store inventory pretty tough */
   if( is_obj_stat( obj, ITEM_INVENTORY ) )
      resist += 20;

   /* okay... let's add some bonus/penalty for item level... */
   resist += ( obj->level / 10 ) - 2;

   /* and lasty... take armor or weapon's condition into consideration */
   if( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON )
      resist += ( obj->value[0] / 2 ) - 2;

   return URANGE( 10, resist, 99 );
}

void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
   OBJ_DATA *oldobj = NULL;
   ROOM_INDEX_DATA *oldroom = NULL;
   int weight;

   if( can_wear( obj, ITEM_NO_TAKE ) && ( get_trust( ch ) < sysdata.perm_getobjnotake ) )
   {
      send_to_char( "You can't take that.\r\n", ch );
      return;
   }

   if( xIS_SET( obj->extra_flags, ITEM_PKDISARMED ) && !is_npc( ch ) )
   {
      if( can_pkill( ch ) && !get_timer( ch, TIMER_PKILLED ) )
      {
         if( ch->level - obj->value[5] > 5 || obj->value[5] - ch->level > 5 )
         {
            send_to_char( "\r\n&bA godly force freezes your outstretched hand.\r\n", ch );
            return;
         }
         else
         {
            xREMOVE_BIT( obj->extra_flags, ITEM_PKDISARMED );
            obj->value[5] = 0;
         }
      }
      else
      {
         send_to_char( "\r\n&BA godly force freezes your outstretched hand.\r\n", ch );
         return;
      }
   }

   if( is_obj_stat( obj, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
   {
      send_to_char( "A godly force prevents you from getting close to it.\r\n", ch );
      return;
   }

   if( obj->item_type != ITEM_MONEY && ( ch->carry_number + get_obj_number( obj ) ) > can_carry_n( ch ) )
   {
      act( AT_PLAIN, "$T: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR );
      return;
   }

   if( is_obj_stat( obj, ITEM_COVERING ) )
      weight = obj->weight;
   else
      weight = get_obj_weight( obj );

   /* Money weight shouldn't count */
   if( obj->item_type != ITEM_MONEY )
   {
      if( obj->in_obj )
      {
         OBJ_DATA *tobj = obj->in_obj;
         int inobj = 1;
         bool checkweight = false;

         /* need to make it check weight if its in a magic container */
         if( tobj->item_type == ITEM_CONTAINER && is_obj_stat( tobj, ITEM_MAGIC ) )
            checkweight = true;

         while( tobj->in_obj )
         {
            tobj = tobj->in_obj;
            inobj++;
            /* need to make it check weight if its in a magic container */
            if( tobj->item_type == ITEM_CONTAINER && is_obj_stat( tobj, ITEM_MAGIC ) )
               checkweight = true;
         }

         /* need to check weight if not carried by ch or in a magic container. */
         if( !tobj->carried_by || tobj->carried_by != ch || checkweight )
         {
            if( ( ch->carry_weight + weight ) > can_carry_w( ch ) )
            {
               act( AT_PLAIN, "$T: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR );
               return;
            }
         }
      }
      else if( ( ch->carry_weight + weight ) > can_carry_w( ch ) )
      {
         act( AT_PLAIN, "$T: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR );
         return;
      }

      if( obj->owner && strlen( obj->owner ) > 1 && str_cmp( obj->owner, ch->name ) )
      {
         send_to_char( "You can't get this object because you're not the owner\r\n", ch );
         return;
      }
   }

   if( container )
   {
      if( container->item_type == ITEM_KEYRING && !is_obj_stat( container, ITEM_COVERING ) )
      {
         act( AT_ACTION, "You remove $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, "$n removes $p from $P", ch, obj, container, TO_ROOM );
      }
      else
      {
         act( AT_ACTION, is_obj_stat( container, ITEM_COVERING ) ?
              "You get $p from beneath $P." : "You get $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, is_obj_stat( container, ITEM_COVERING ) ?
              "$n gets $p from beneath $P." : "$n gets $p from $P", ch, obj, container, TO_ROOM );
      }
      if( is_obj_stat( container, ITEM_CLANCORPSE ) && !is_npc( ch ) && str_cmp( container->name + 7, ch->name ) )
         container->value[5]++;
      oldobj = obj->in_obj;
      obj_from_obj( obj );
   }
   else
   {
      act( AT_ACTION, "You get $p.", ch, obj, container, TO_CHAR );
      act( AT_ACTION, "$n gets $p.", ch, obj, container, TO_ROOM );
      oldroom = obj->in_room;
      obj_from_room( obj );
   }

   /* Storage check */
   if( xIS_SET( ch->in_room->room_flags, ROOM_STORAGEROOM ) && ( !container || !container->carried_by ) )
      save_storage( ch->in_room );

   if( obj->item_type != ITEM_CONTAINER )
      check_for_trap( ch, obj, TRAP_GET );
   if( char_died( ch ) )
   {
      /* Wow we were just returning after taking stuff out the room/container and leaving stuff hanging around, thats bad */
      /* I figure they died in getting it so put it back where it was */
      if( oldobj )
         obj_to_obj( obj, oldobj );
      else if( oldroom )
         obj_to_room( obj, oldroom );
      else
         extract_obj( obj ); /* If nothing else remove it */

      /* Not going to do trigger check here since character has died */
      return;
   }

   if( obj->item_type == ITEM_MONEY )
   {
      int ncount = obj->count;
      bool dextract = false;

      if( obj->value[0] < 0 )
      {
         send_to_char( "Getting this would decrease the amount of gold you have.\r\n", ch );

         /* I figure if getting it would decrease their gold put it back where it was */
         if( oldobj )
            obj_to_obj( obj, oldobj );
         else if( oldroom )
            obj_to_room( obj, oldroom );
         else
            extract_obj( obj ); /* If nothing else remove it */

         if( obj ) /* If it still exist check for triggers */
            oprog_get_trigger( ch, obj );
         return;
      }

      while( ncount > 0 )
      {
         if( !( can_hold_gold( ch, obj->value[0] ) ) )
         {
            ncount--;
            dextract = true;
            continue;
         }

         if( --obj->count < 1 )
            obj->count = 1;

         increase_gold( ch, obj->value[0] );

         if( xIS_SET( ch->act, PLR_AUTOSPLIT ) )
         {
            interpret_printf( ch, "split auto %d", obj->value[0] );
         }
         ncount--;
      }

      if( !dextract )
         extract_obj( obj );
      else
         obj = obj_to_char( obj, ch );
   }
   else
      obj = obj_to_char( obj, ch );

   if( char_died( ch ) ) /* By here it should have already been put somewhere so should be good */
      return;

   if( obj ) /* Wow we didn't see if it was NULL by this point? */
      oprog_get_trigger( ch, obj );
}

void show_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
   SKILLTYPE *sktmp;
   AFFECT_DATA *paf;
   int stat, cnt = 0;

   if( !ch || !obj )
   {
      bug( "%s: NULL ch or NULL obj", __FUNCTION__ );
      return;
   }

   set_char_color( AT_OBJECT, ch );
   ch_printf( ch, "\r\nObject '%s' is %s",
      obj->short_descr, aoran( o_types[ obj->item_type ] ) );
   if( !xIS_EMPTY( obj->wear_flags ) )
      ch_printf( ch, ", with wear location: %s\r\n", ext_flag_string( &obj->wear_flags, w_flags ) );
   else
      send_to_char( ".\r\n", ch );

   ch_printf( ch, "Special properties: %s\r\nIts weight is %d, value is %d.\r\n",
      ext_flag_string( &obj->extra_flags, o_flags ), obj->weight, obj->cost );

   if( obj->bsplatter > 0 )
      send_to_char( "&RBloody&D.\r\n", ch );
   if( obj->bstain > 0 )
      send_to_char( "&RStained&D.\r\n", ch );

   ch_printf( ch, "Requirements:\r\n%12s: %3d ", "Level", obj->level );
   cnt = 1;
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( obj->stat_reqs[stat] <= 0 )
         continue;
      ch_printf( ch, "%12s: %3d", capitalize( stattypes[stat] ), obj->stat_reqs[stat] );
      if( ++cnt == 3 )
      {
         send_to_char( "\r\n", ch );
         cnt = 0;
      }
      else
         send_to_char( " ", ch );
   }
   if( cnt != 0 )
      send_to_char( "\r\n", ch );

   if( !xIS_EMPTY( obj->class_restrict ) )
   {
      cnt = 0;
      send_to_char( "&cClass Restrictions:\r\n&w ", ch );
      for( stat = 0; stat < MAX_PC_CLASS; stat++ )
      {
         if( class_table[stat] && xIS_SET( obj->class_restrict, stat ) )
         {
            ch_printf( ch, "%12s", class_table[stat]->name );
            if( ++cnt == 4 )
            {
               cnt = 0;
               send_to_char( "\r\n", ch );
            }
            else
               send_to_char( " ", ch );
         }
      }
      if( cnt != 0 )
         send_to_char( "\r\n", ch );
   }
   if( !xIS_EMPTY( obj->race_restrict ) )
   {
      cnt = 0;
      send_to_char( "&cRace Restrictions:\r\n&w ", ch );
      for( stat = 0; stat < MAX_PC_RACE; stat++ )
      {
         if( race_table[stat] && xIS_SET( obj->race_restrict, stat ) )
         {
            ch_printf( ch, "%12s", race_table[stat]->name );
            if( ++cnt == 4 )
            {
               cnt = 0;
               send_to_char( "\r\n", ch );
            }
            else
               send_to_char( " ", ch );
         }
      }
      if( cnt != 0 )
         send_to_char( "\r\n", ch );
   }

   set_char_color( AT_MAGIC, ch );

   switch( obj->item_type )
   {
      case ITEM_CONTAINER:
         ch_printf( ch, "%s appears to be %s.\r\n", capitalize( obj->short_descr ),
            obj->value[0] < 76 ? "of a small capacity" :
            obj->value[0] < 150 ? "of a small to medium capacity" :
            obj->value[0] < 300 ? "of a medium capacity" :
            obj->value[0] < 550 ? "of a medium to large capacity" :
            obj->value[0] < 751 ? "of a large capacity" : "of a giant capacity" );
         ch_printf( ch, "&cContainer Flags: &w%s\r\n", flag_string( obj->value[1], cont_flags ) );
         break;

      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         if( obj->value[1] >= 0 || obj->value[2] >= 0 || obj->value[3] >= 0 )
         {
            ch_printf( ch, "Level %d spells of:", obj->value[0] );
            if( obj->value[1] >= 0 && ( sktmp = get_skilltype( obj->value[1] ) ) )
               ch_printf( ch, " '%s'", sktmp->name );
            if( obj->value[2] >= 0 && ( sktmp = get_skilltype( obj->value[2] ) ) )
               ch_printf( ch, " '%s'", sktmp->name );
            if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) )
               ch_printf( ch, " '%s'", sktmp->name );
            send_to_char( ".\r\n", ch );
         }
         else
            send_to_char( "Has no spells.\r\n", ch );
         break;

      case ITEM_SALVE:
         if( obj->value[3] >= 0 || obj->value[4] >= 0 || obj->value[5] >= 0 )
         {
            ch_printf( ch, "Has %d(%d) applications of level %d", obj->value[1], obj->value[2], obj->value[0] );
            if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) )
               ch_printf( ch, " '%s'", sktmp->name );
            if( obj->value[4] >= 0 && ( sktmp = get_skilltype( obj->value[4] ) ) )
               ch_printf( ch, " '%s'", sktmp->name );
            if( obj->value[5] >= 0 && ( sktmp = get_skilltype( obj->value[5] ) ) )
               ch_printf( ch, " '%s'", sktmp->name );
            send_to_char( ".\r\n", ch );
         }
         else
            send_to_char( "Has no applications.\r\n", ch );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         if( obj->value[3] >= 0 )
         {
            ch_printf( ch, "Has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0] );
            if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) )
               ch_printf( ch, " '%s'", sktmp->name );
            send_to_char( ".\r\n", ch );
         }
         else
            send_to_char( "Has no charges.\r\n", ch );
         break;

      case ITEM_MISSILE_WEAPON:
      case ITEM_WEAPON:
         ch_printf( ch, "&cWeapon Type: &w%s\r\n", ( obj->value[3] >= 0 && obj->value[3] < DAM_MAX ) ? attack_table[obj->value[3]] : "Unknown" );
         ch_printf( ch, "Damage is %d to %d (average %d)%s\r\n",
            obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2,
            is_obj_stat( obj, ITEM_POISONED ) ? ", and is poisonous." : "." );
         break;

      case ITEM_ARMOR:
         ch_printf( ch, "Armor class is %d.\r\n", obj->value[0] );
         break;
   }

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      showaffect( ch, paf, false );

   for( paf = obj->first_affect; paf; paf = paf->next )
      showaffect( ch, paf, false );
}

CMDF( do_get )
{
   OBJ_DATA *obj, *obj_next, *container;
   char arg1[MIL], arg2[MIL];
   int ocheck = 0;
   short number;
   bool found;

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Get what?\r\n", ch );
      return;
   }
   if( is_number( arg1 ) )
   {
      number = atoi( arg1 );
      if( number < 1 )
      {
         send_to_char( "That was easy...\r\n", ch );
         return;
      }
      if( ( ch->carry_number + number ) > can_carry_n( ch ) )
      {
         send_to_char( "You can't carry that many.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg1 );
   }
   else
      number = 0;

   argument = one_argument( argument, arg2 );

   /* Get type. */
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Get what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( arg2 == NULL || arg2[0] == '\0' )
   {
      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /* 'get obj' */
         if( !( obj = get_obj_list( ch, arg1, ch->in_room->first_content ) ) )
         {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR );
            return;
         }
         separate_obj( obj );
         get_obj( ch, obj, NULL );
         if( char_died( ch ) )
            return;
         if( xIS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
      else
      {
         char *chk;
         short cnt = 0;
         bool fAll;

         if( xIS_SET( ch->in_room->room_flags, ROOM_DONATION ) )
         {
            send_to_char( "The gods frown upon such a display of greed!\r\n", ch );
            return;
         }
         if( !str_cmp( arg1, "all" ) )
            fAll = true;
         else
            fAll = false;
         if( number > 1 )
            chk = arg1;
         else
            chk = &arg1[4];
         /* 'get all' or 'get all.obj' */
         found = false;
         for( obj = ch->in_room->last_content; obj; obj = obj_next )
         {
            obj_next = obj->prev_content;
            if( ( fAll || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj ) )
            {
               found = true;
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, NULL );
               if( char_died( ch ) || ch->carry_number >= can_carry_n( ch )
               || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) )
               {
                  if( !char_died( ch ) )
                  {
                     if( ch->carry_number >= can_carry_n( ch ) )
                        act( AT_PLAIN, "You're now carrying as many items as you can.", ch, NULL, NULL, TO_CHAR );
                     if( ch->carry_weight >= can_carry_w( ch ) )
                        act( AT_PLAIN, "You're now carrying all the weight you can.", ch, NULL, NULL, TO_CHAR );
                  }
                  if( xIS_SET( sysdata.save_flags, SV_GET ) && !char_died( ch ) )
                     save_char_obj( ch );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
               send_to_char( "I see nothing here.\r\n", ch );
            else
               act( AT_PLAIN, "I see no $T here.", ch, NULL, chk, TO_CHAR );
         }
         else if( xIS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
   }
   else
   {
      /* 'get ... container' */
      if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
      {
         send_to_char( "You can't do that.\r\n", ch );
         return;
      }

      /* Need some way to specify if we want to look at things in room or in inventory */
      if( !str_cmp( argument, "room" ) )
         ocheck = 1;
      else if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
         ocheck = 2;
      else if( !str_cmp( argument, "worn" ) )
         ocheck = 3;

      if( !( container = new_get_obj_here( ocheck, ch, arg2 ) ) )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
         return;
      }

      switch( container->item_type )
      {
         default:
            if( !is_obj_stat( container, ITEM_COVERING ) )
            {
               send_to_char( "That's not a container.\r\n", ch );
               return;
            }
            if( ch->carry_weight + container->weight > can_carry_w( ch ) )
            {
               send_to_char( "It's too heavy for you to lift.\r\n", ch );
               return;
            }
            break;

         case ITEM_CONTAINER:
         case ITEM_CORPSE_NPC:
         case ITEM_KEYRING:
         case ITEM_QUIVER:
            break;

         case ITEM_CORPSE_PC:
         {
            CHAR_DATA *gch;
            char *pd;
            char name[MIL];

            if( is_npc( ch ) )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }

            pd = container->short_descr;
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );

            if( is_obj_stat( container, ITEM_CLANCORPSE )
            && !is_npc( ch ) && ( get_timer( ch, TIMER_PKILLED ) > 0 ) && str_cmp( name, ch->name ) )
            {
               send_to_char( "You can't loot from that corpse...yet.\r\n", ch );
               return;
            }

            /*
             * Killer/owner loot only if die to pkill blow --Blod 
             * Added check for immortal so IMMS can get things out of
             * corpses --Shaddai 
             */
            if( is_obj_stat( container, ITEM_CLANCORPSE ) && !is_npc( ch ) && !is_immortal( ch ) )
            {
               if( str_cmp( name, ch->name ) )
               {
                  if( container->action_desc[0] != '\0' && str_cmp( container->action_desc, ch->name ) )
                  {
                     send_to_char( "You did not inflict the death blow upon this corpse.\r\n", ch );
                     return;
                  }

                  if( container->value[5] >= 3 )
                  {
                     send_to_char( "Frequent looting has left this corpse protected by the gods.\r\n", ch );
                     return;
                  }
               }

               if( xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
               && ( container->value[4] - ch->level ) < 6 && ( container->value[4] - ch->level ) > -6 )
                  break;
            }

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
      }

      if( !is_obj_stat( container, ITEM_COVERING ) && IS_SET( container->value[1], CONT_CLOSED )
      && container->item_type != ITEM_CORPSE_PC && container->item_type != ITEM_CORPSE_NPC )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return;
      }

      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /* 'get obj container' */
         obj = get_obj_list( ch, arg1, container->first_content );
         if( !obj )
         {
            act( AT_PLAIN, is_obj_stat( container, ITEM_COVERING ) ?
                 "I see nothing like that beneath the $T." : "I see nothing like that in the $T.", ch, NULL, container->short_descr, TO_CHAR );
            return;
         }
         separate_obj( obj );
         get_obj( ch, obj, container );
         /* Oops no wonder corpses were duping oopsie did I do that --Shaddai */
         if( container->item_type == ITEM_CORPSE_PC )
            write_corpses( container, false );
         check_for_trap( ch, container, TRAP_GET );
         if( char_died( ch ) )
            return;
         if( xIS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
      else
      {
         char *chk;
         int cnt = 0;
         bool fAll;

         /* 'get all container' or 'get all.obj container' */
         if( is_obj_stat( container, ITEM_DONATION ) )
         {
            send_to_char( "The gods frown upon such an act of greed!\r\n", ch );
            return;
         }

         if( is_obj_stat( container, ITEM_CLANCORPSE )
         && !is_immortal( ch ) && !is_npc( ch ) && str_cmp( ch->name, container->name + 7 ) )
         {
            send_to_char( "The gods frown upon such wanton greed!\r\n", ch );
            return;
         }

         if( !str_cmp( arg1, "all" ) )
            fAll = true;
         else
            fAll = false;
         if( number > 1 )
            chk = arg1;
         else
            chk = &arg1[4];
         found = false;
         for( obj = container->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            if( ( fAll || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj ) )
            {
               found = true;
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, container );
               if( char_died( ch )
               || ch->carry_number >= can_carry_n( ch )
               || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) )
               {
                  check_for_trap( ch, container, TRAP_GET );
                  if( !char_died( ch ) )
                  {
                     if( ch->carry_number >= can_carry_n( ch ) )
                        act( AT_PLAIN, "You're now carrying as many items as you can.", ch, NULL, NULL, TO_CHAR );
                     if( ch->carry_weight >= can_carry_w( ch ) )
                        act( AT_PLAIN, "You're now carrying all the weight you can.", ch, NULL, NULL, TO_CHAR );
                  }
                  if( container->item_type == ITEM_CORPSE_PC )
                     write_corpses( container, false );
                  if( found && xIS_SET( sysdata.save_flags, SV_GET ) )
                     save_char_obj( ch );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
            {
               if( container->item_type == ITEM_KEYRING && !is_obj_stat( container, ITEM_COVERING ) )
                  act( AT_PLAIN, "The $T holds no keys.", ch, NULL, container->short_descr, TO_CHAR );
               else
                  act( AT_PLAIN, is_obj_stat( container, ITEM_COVERING ) ?
                       "I see nothing beneath the $T." : "I see nothing in the $T.", ch, NULL, container->short_descr, TO_CHAR );
            }
            else
            {
               if( container->item_type == ITEM_KEYRING && !is_obj_stat( container, ITEM_COVERING ) )
                  act( AT_PLAIN, "The $T does not hold that key.", ch, NULL, container->short_descr, TO_CHAR );
               else
                  act( AT_PLAIN, is_obj_stat( container, ITEM_COVERING ) ?
                       "I see nothing like that beneath the $T." :
                       "I see nothing like that in the $T.", ch, NULL, container->short_descr, TO_CHAR );
            }
         }
         else
            check_for_trap( ch, container, TRAP_GET );
         /* Oops no wonder corpses were duping oopsie did I do that --Shaddai */
         if( container->item_type == ITEM_CORPSE_PC )
            write_corpses( container, false );
         if( char_died( ch ) )
            return;
         if( found && xIS_SET( sysdata.save_flags, SV_GET ) )
            save_char_obj( ch );
      }
   }
}

CMDF( do_put )
{
   OBJ_DATA *container, *obj, *obj_next;
   ROOM_INDEX_DATA *inroom;
   char arg1[MIL], arg2[MIL];
   int number, ocheck = 0;
   short count;
   bool save_char = false;

   argument = one_argument( argument, arg1 );
   if( is_number( arg1 ) )
   {
      number = atoi( arg1 );
      if( number < 1 )
      {
         send_to_char( "That was easy...\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg1 );
   }
   else
      number = 0;

   argument = one_argument( argument, arg2 );

   /* munch optional words */
   if( ( !str_cmp( arg2, "into" ) || !str_cmp( arg2, "inside" )
   || !str_cmp( arg2, "in" ) || !str_cmp( arg2, "under" )
   || !str_cmp( arg2, "onto" ) || !str_cmp( arg2, "on" ) )
   && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Put what in what?\r\n", ch );
      return;
   }

   /* Need some way to specify if we want to look at things in room or in inventory */
   if( !str_cmp( argument, "room" ) )
      ocheck = 1;
   else if( !str_cmp( argument, "inv" ) || !str_cmp( argument, "inventory" ) )
      ocheck = 2;
   else if( !str_cmp( argument, "worn" ) )
      ocheck = 3;

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   if( !( container = new_get_obj_here( ocheck, ch, arg2 ) ) )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
      return;
   }

   if( !container->carried_by && xIS_SET( sysdata.save_flags, SV_PUT ) )
      save_char = true;

   if( is_obj_stat( container, ITEM_COVERING ) )
   {
      if( ch->carry_weight + container->weight > can_carry_w( ch ) )
      {
         send_to_char( "It's too heavy for you to lift.\r\n", ch );
         return;
      }
   }
   else
   {
      if( container->item_type != ITEM_CONTAINER
      && container->item_type != ITEM_KEYRING
      && container->item_type != ITEM_QUIVER
      && container->item_type != ITEM_FIRE )
      {
         send_to_char( "That's not a container.\r\n", ch );
         return;
      }

      if( IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return;
      }
   }

   if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
   {
      /* 'put obj container' */
      if( !( obj = get_obj_carry( ch, arg1 ) ) )
      {
         send_to_char( "You don't have that item.\r\n", ch );
         return;
      }

      if( obj == container )
      {
         send_to_char( "You can't fold it into itself.\r\n", ch );
         return;
      }

      if( !can_drop_obj( ch, obj ) )
      {
         send_to_char( "You can't let go of it.\r\n", ch );
         return;
      }

      if( is_obj_stat( obj, ITEM_NOCONTAINER ) )
      {
         send_to_char( "You can't put that in a container.\r\n", ch );
         return;
      }

      if( container->item_type == ITEM_KEYRING && obj->item_type != ITEM_KEY )
      {
         send_to_char( "That's not a key.\r\n", ch );
         return;
      }

      if( container->item_type == ITEM_QUIVER && obj->item_type != ITEM_PROJECTILE )
      {
         send_to_char( "That's not a projectile.\r\n", ch );
         return;
      }

      if( ( is_obj_stat( container, ITEM_COVERING )
      && ( get_obj_weight( obj ) / obj->count )
      > ( ( get_obj_weight( container ) / container->count ) - container->weight ) ) )
      {
         send_to_char( "It won't fit under there.\r\n", ch );
         return;
      }

      /* note use of get_real_obj_weight */
      if( container->item_type != ITEM_FIRE
      && ( get_real_obj_weight( obj ) / obj->count )
      + ( get_real_obj_weight( container ) / container->count ) > container->value[0] )
      {
         send_to_char( "It won't fit.\r\n", ch );
         return;
      }

      inroom = ch->in_room; /* Since programs can change the room the object or character are in need to know where it started */
      separate_obj( obj );
      separate_obj( container );
      obj_from_char( obj );
      count = obj->count;
      obj->count = 1;
      if( container->item_type == ITEM_KEYRING && !is_obj_stat( container, ITEM_COVERING ) )
      {
         act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
      }
      else
      {
         act( AT_ACTION, is_obj_stat( container, ITEM_COVERING )
              ? "$n hides $p beneath $P." : "$n puts $p in $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, is_obj_stat( container, ITEM_COVERING )
              ? "You hide $p beneath $P." : "You put $p in $P.", ch, obj, container, TO_CHAR );
      }

      /* handle put triggers for container and obj */
      oprog_put_trigger( ch, obj, container );
      check_for_trap( ch, container, TRAP_PUT );
      obj->count = count;
      if( container->item_type == ITEM_FIRE )
      {
         container->timer += number_range( 1, 5 );
         extract_obj( obj );
      }
      else
         obj = obj_to_obj( obj, container );
      if( char_died( ch ) )
         return;

      /* Oops no wonder corpses were duping oopsie did I do that --Shaddai */
      if( container->item_type == ITEM_CORPSE_PC )
         write_corpses( container, false );

      if( save_char )
         save_char_obj( ch );

      /* Storage check */
      if( inroom && container && xIS_SET( inroom->room_flags, ROOM_STORAGEROOM ) && !container->carried_by )
         save_storage( inroom );
   }
   else
   {
      char *chk;
      int cnt = 0;
      bool found = false, fAll = false;

      if( !str_cmp( arg1, "all" ) )
         fAll = true;

      if( number > 1 )
         chk = arg1;
      else
         chk = &arg1[4];

      inroom = ch->in_room; /* Since programs can change the room the object or character are in need to know where it started */
      separate_obj( container );
      /* 'put all container' or 'put all.obj container' */
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( ( fAll || nifty_is_name( chk, obj->name ) )
         && can_see_obj( ch, obj )
         && obj->wear_loc == WEAR_NONE
         && obj != container
         && can_drop_obj( ch, obj )
         && !is_obj_stat( obj, ITEM_NOCONTAINER )
         && ( container->item_type != ITEM_KEYRING || obj->item_type == ITEM_KEY )
         && ( container->item_type != ITEM_QUIVER || obj->item_type == ITEM_PROJECTILE )
         && ( container->item_type == ITEM_FIRE || ( get_obj_weight( obj ) + get_obj_weight( container ) ) <= container->value[0] ) )
         {
            if( number && ( cnt + obj->count ) > number )
               split_obj( obj, number - cnt );
            cnt += obj->count;
            obj_from_char( obj );
            if( container->item_type == ITEM_KEYRING )
            {
               act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "$n puts $p in $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You put $p in $P.", ch, obj, container, TO_CHAR );
            }
            found = true;
            oprog_put_trigger( ch, obj, container );
            check_for_trap( ch, container, TRAP_PUT );
            if( container->item_type == ITEM_FIRE )
            {
               container->timer += number_range( 1, 5 );
               extract_obj( obj );
            }
            else
               obj = obj_to_obj( obj, container );
            if( char_died( ch ) )
               return;
            if( number && cnt >= number )
               break;
         }
      }

      /* Don't bother to save anything if nothing was dropped - Thoric */
      if( !found )
      {
         if( fAll )
            act( AT_PLAIN, "You aren't carrying anything.", ch, NULL, NULL, TO_CHAR );
         else
            act( AT_PLAIN, "You aren't carrying any $T.", ch, NULL, chk, TO_CHAR );
         return;
      }

      if( save_char )
         save_char_obj( ch );

      /* Oops no wonder corpses were duping oopsie did I do that -- Shaddai */
      if( container->item_type == ITEM_CORPSE_PC )
         write_corpses( container, false );

      /* Storage check */
      if( inroom && container && xIS_SET( inroom->room_flags, ROOM_STORAGEROOM ) && !container->carried_by )
         save_storage( inroom );
   }
}

void drop_coins( CHAR_DATA *ch, bool all, int number )
{
   ROOM_INDEX_DATA *inroom;
   OBJ_DATA *obj, *obj_next;

   if( !ch || ( number <= 0 && !all ) )
      return;

   if( all )
   {
      number = ch->gold;
      ch->gold = 0;
   }
   else
   {
      if( !has_gold( ch, number ) )
      {
         send_to_char( "You haven't got that many coins.\r\n", ch );
         return;
      }

      decrease_gold( ch, number );
   }

   inroom = ch->in_room;

   /* Used to combine all gold on the ground (When possible) */
   for( obj = ch->in_room->first_content; obj; obj = obj_next )
   {
      obj_next = obj->next_content;

      if( number <= 0 )
         break;

      switch( obj->pIndexData->vnum )
      {
         case OBJ_VNUM_MONEY_ONE:
            number += 1;
            extract_obj( obj );
            break;

         case OBJ_VNUM_MONEY_SOME:
            /* If already at max skip */
            if( obj->value[0] >= 2000000000 )
               continue;
            /* Fill it up to max and subtract what was used */
            if( ( obj->value[0] + number ) >= 2000000000 || ( obj->value[0] + number ) < obj->value[0] )
            {
               number -= ( 2000000000 - obj->value[0] );
               obj->value[0] = 2000000000;
               set_money( obj, obj->value[0] );
               continue;
            }
            obj->value[0] += number;
            number = 0;
            set_money( obj, obj->value[0] );
            break;
      }
   }

   act( AT_ACTION, "$n drops some gold.", ch, NULL, NULL, TO_ROOM );
   send_to_char( "You let the gold slip from your hand.\r\n", ch );

   if( number > 0 )
   {
      if( !( obj = create_money( number ) ) )
      {
         bug( "%s: failed to create money.", __FUNCTION__ );
         return;
      }

      obj_to_room( obj, ch->in_room );
      obj_to_char_cords( obj, ch );
   }

   if( xIS_SET( sysdata.save_flags, SV_DROP ) )
      save_char_obj( ch );

   /* Storage check - Should update the storage rooms here too */
   if( inroom && xIS_SET( inroom->room_flags, ROOM_STORAGEROOM ) )
      save_storage( inroom );
}

CMDF( do_drop )
{
   ROOM_INDEX_DATA *inroom;
   OBJ_DATA *obj, *obj_next;
   char arg[MIL];
   int number = 0;
   bool found;

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Drop what?\r\n", ch );
      return;
   }

   if( is_number( arg ) )
   {
      number = atoi( arg );

      if( number <= 0 )
      {
         send_to_char( "That was easy...\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg );
   }

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Drop what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !can_drop_room( ch ) )
      return;

   if( number > 0 )
   {
      /* 'drop NNNN coins' */
      if( !str_cmp( arg, "coins" ) || !str_cmp( arg, "coin" ) || !str_cmp( argument, "gold" ) )
      {
         drop_coins( ch, false, number );
         return;
      }
   }

   if( !str_cmp( arg, "all" ) && ( !str_cmp( arg, "coin" ) || !str_cmp( argument, "coins" ) || !str_cmp( argument, "gold" ) ) )
   {
      drop_coins( ch, true, 0 );
      return;
   }

   inroom = ch->in_room;
   if( number <= 1 && str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
   {
      /* 'drop obj' */
      if( !( obj = get_obj_carry( ch, arg ) ) )
      {
         send_to_char( "You don't have that item.\r\n", ch );
         return;
      }

      if( !can_drop_obj( ch, obj ) )
      {
         send_to_char( "You can't let go of it.\r\n", ch );
         return;
      }

      separate_obj( obj );
      act( AT_ACTION, "$n drops $p.", ch, obj, NULL, TO_ROOM );
      act( AT_ACTION, "You drop $p.", ch, obj, NULL, TO_CHAR );
      obj_from_char( obj );
      obj = obj_to_room( obj, ch->in_room );
      obj_to_char_cords( obj, ch );
      oprog_drop_trigger( ch, obj );   /* mudprogs */

      if( char_died( ch ) )
      {
         /* Storage check */
         if( inroom && xIS_SET( inroom->room_flags, ROOM_STORAGEROOM ) )
            save_storage( inroom );
         return;
      }
   }
   else
   {
      char *chk;
      int cnt = 0;
      bool fAll = false;

      if( !str_cmp( arg, "all" ) )
         fAll = true;

      if( number > 1 )
         chk = arg;
      else
         chk = &arg[4];

      /* 'drop all' or 'drop all.obj' */
      if( xIS_SET( ch->in_room->room_flags, ROOM_NODROPALL ) )
      {
         send_to_char( "You can't seem to do that here...\r\n", ch );
         return;
      }

      found = false;
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( ( fAll || nifty_is_name( chk, obj->name ) )
         && can_see_obj( ch, obj ) && obj->wear_loc == WEAR_NONE && can_drop_obj( ch, obj ) )
         {
            found = true;
            if( HAS_PROG( obj->pIndexData, DROP_PROG ) && obj->count > 1 )
            {
               ++cnt;
               separate_obj( obj );
               obj_from_char( obj );
               if( !obj_next )
                  obj_next = ch->first_carrying;
            }
            else
            {
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               obj_from_char( obj );
            }
            act( AT_ACTION, "$n drops $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You drop $p.", ch, obj, NULL, TO_CHAR );
            obj = obj_to_room( obj, ch->in_room );
            obj_to_char_cords( obj, ch );
            oprog_drop_trigger( ch, obj );   /* mudprogs */
            if( char_died( ch ) )
            {
               /* Storage check */
               if( inroom && xIS_SET( inroom->room_flags, ROOM_STORAGEROOM ) )
                  save_storage( inroom );
               return;
            }
            if( inroom && ch->in_room != inroom ) /* Don't continue if room changed */
            {
               /* Storage check */
               if( inroom && xIS_SET( inroom->room_flags, ROOM_STORAGEROOM ) )
                  save_storage( inroom );
               return;
            }
            if( number && cnt >= number )
               break;
         }
      }

      if( !found )
      {
         if( fAll )
            act( AT_PLAIN, "You aren't carrying anything.", ch, NULL, NULL, TO_CHAR );
         else
            act( AT_PLAIN, "You aren't carrying any $T.", ch, NULL, chk, TO_CHAR );
         return;
      }
   }

   if( xIS_SET( sysdata.save_flags, SV_DROP ) )
      save_char_obj( ch ); /* duping protector */

   /* Storage check */
   if( inroom && xIS_SET( inroom->room_flags, ROOM_STORAGEROOM ) )
      save_storage( inroom );
}

CMDF( do_give )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   char arg1[MIL], arg2[MIL], buf[MSL];

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Give what to whom?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( is_number( arg1 ) )
   {
      /* 'give NNNN coins victim' */
      int amount = 0;

      amount = atoi( arg1 );
      if( amount <= 0 || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) ) )
      {
         send_to_char( "Sorry, you can't do that.\r\n", ch );
         return;
      }

      argument = one_argument( argument, arg2 );
      if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
         argument = one_argument( argument, arg2 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Give what to whom?\r\n", ch );
         return;
      }

      if( !( victim = get_char_room( ch, arg2 ) ) )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }

      if( !has_gold( ch, amount ) )
      {
         send_to_char( "Very generous of you, but you haven't got that much gold.\r\n", ch );
         return;
      }

      if( !can_hold_gold( victim, amount ) )
      {
         send_to_char( "Very generous of you, but they can't hold that much gold.\r\n", ch );
         return;
      }

      increase_gold( victim, amount );
      decrease_gold( ch, amount );
      mudstrlcpy( buf, "$n gives you ", sizeof( buf ) );
      mudstrlcat( buf, arg1, sizeof( buf ) );
      mudstrlcat( buf, amount > 1 ? " coins." : " coin.", sizeof( buf ) );

      act( AT_ACTION, buf, ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n gives $N some gold.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You give $N some gold.", ch, NULL, victim, TO_CHAR );
      mprog_bribe_trigger( victim, ch, amount );
      if( xIS_SET( sysdata.save_flags, SV_GIVE ) && !char_died( ch ) )
         save_char_obj( ch );
      if( xIS_SET( sysdata.save_flags, SV_RECEIVE ) && !char_died( victim ) )
         save_char_obj( victim );
      return;
   }

   if( !( obj = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You don't have that item.\r\n", ch );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
   {
      send_to_char( "You must remove it first.\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg2 ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( obj->owner && strlen( obj->owner ) > 1 && str_cmp( obj->owner, victim->name ) )
   {
      send_to_char("You can't give this object them because they aren't the owner\r\n", ch);
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it.\r\n", ch );
      return;
   }

   if( victim->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( victim ) )
   {
      act( AT_PLAIN, "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->carry_weight + ( get_obj_weight( obj ) / obj->count ) > can_carry_w( victim ) )
   {
      act( AT_PLAIN, "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !can_see_obj( victim, obj ) )
   {
      act( AT_PLAIN, "$N can't see it.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( is_obj_stat( obj, ITEM_PROTOTYPE ) && !can_take_proto( victim ) )
   {
      act( AT_PLAIN, "You can't give that to $N!", ch, NULL, victim, TO_CHAR );
      return;
   }

   separate_obj( obj );
   obj_from_char( obj );
   act( AT_ACTION, "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
   act( AT_ACTION, "$n gives you $p.", ch, obj, victim, TO_VICT );
   act( AT_ACTION, "You give $p to $N.", ch, obj, victim, TO_CHAR );
   obj = obj_to_char( obj, victim );

   mprog_give_trigger( victim, ch, obj );
   if( xIS_SET( sysdata.save_flags, SV_GIVE ) && !char_died( ch ) )
      save_char_obj( ch );
   if( xIS_SET( sysdata.save_flags, SV_RECEIVE ) && !char_died( victim ) )
      save_char_obj( victim );
}

/*
 * Damage an object -Thoric
 * Affect player's AC if necessary.
 * Make object into scraps if necessary.
 * Send message about damaged object.
 */
void damage_obj( OBJ_DATA *obj )
{
   CHAR_DATA *ch;
   bool scrapobj = false;

   ch = obj->carried_by;

   /* No point in bothering with this if in_arena */
   if( ch && in_arena( ch ) )
      return;

   separate_obj( obj );

   if( ch && !is_npc( ch ) )
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR );
   else if( obj->in_room && ( ch = obj->in_room->first_person ) )
   {
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_ROOM );
      act( AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR );
      ch = NULL;
   }

   oprog_damage_trigger( ch, obj );

   switch( obj->item_type )
   {
      default:
         if( !is_obj_stat( obj, ITEM_NOSCRAP ) )
            scrapobj = true;
         break;

      case ITEM_CONTAINER:
      case ITEM_KEYRING:
      case ITEM_QUIVER:
         if( --obj->value[3] <= 0 )
         {
            if( is_obj_stat( obj, ITEM_NOSCRAP ) )
               obj->value[3] = 1;
            else
               scrapobj = true;
         }
         break;

      case ITEM_LOCKPICK:
      case ITEM_AXE:
      case ITEM_LIGHT:
      case ITEM_WEAPON:
         if( --obj->value[0] <= 0 )
         {
            if( is_obj_stat( obj, ITEM_NOSCRAP ) )
               obj->value[0] = 1;
            else
               scrapobj = true;
         }
         break;

      case ITEM_ARMOR:
         if( ch && obj->value[0] >= 1 )
            apply_ac( ch, obj, obj->wear_loc, true );
         if( --obj->value[0] <= 0 )
         {
            if( is_obj_stat( obj, ITEM_NOSCRAP ) )
               obj->value[0] = 1;
            else
               scrapobj = true;
         }
         if( ch && obj->value[0] >= 1 )
            apply_ac( ch, obj, obj->wear_loc, false );
         break;
   }
   if( scrapobj )
      make_scraps( obj );

   if( ch )
      save_char_obj( ch ); /* Stop scrap duping - Samson 1-2-00 */
}

/* Remove an object. */
bool remove_obj( CHAR_DATA *ch, int iWear, bool fReplace )
{
   OBJ_DATA *obj;

   if( !( obj = get_eq_char( ch, iWear ) ) )
      return true;

   if( !fReplace )
      return false;

   if( !fReplace && ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
   {
      act( AT_PLAIN, "$T: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR );
      return false;
   }

   if( is_obj_stat( obj, ITEM_NOREMOVE ) )
   {
      act( AT_PLAIN, "You can't remove $p.", ch, obj, NULL, TO_CHAR );
      return false;
   }

   if( is_obj_stat( obj, ITEM_LODGED ) )
   {
      act( AT_PLAIN, "You will have to dislodge $p.", ch, obj, NULL, TO_CHAR );
      return false;
   }

   if( is_obj_stat( obj, ITEM_PIERCED ) )
   {
      act( AT_PLAIN, "You will have to dislodge what pierced $p first.", ch, obj, NULL, TO_CHAR );
      return false;
   }

   if( obj->item_type == ITEM_FISHINGPOLE )
      if( stop_obj_fishing( obj ) )
         send_to_char( "You pull your line out of the water.\r\n", ch );

   unequip_char( ch, obj );

   /* This can change in unequip_char now */
   if( obj->carried_by && obj->carried_by == ch )
   {
      act( AT_ACTION, "$n stops using $p.", ch, obj, NULL, TO_ROOM );
      act( AT_ACTION, "You stop using $p.", ch, obj, NULL, TO_CHAR );
   }
   oprog_remove_trigger( ch, obj );
   return true;
}

/* Have a hand free to hold something? */
bool can_hold( CHAR_DATA *ch )
{
   if( get_eq_char( ch, WEAR_HOLD_B ) )
   {
      send_to_char( "You're already holding something that requires both hands!\r\n", ch );
      return false;
   }
   if( get_eq_char( ch, WEAR_HOLD_L ) && get_eq_char( ch, WEAR_HOLD_R ) )
   {
      send_to_char( "You're already holding something in each hand!\r\n", ch );
      return false;
   }
   return true;
}

/*
 * Check to see if there is room to wear another object on this location
 * (Layered clothing support)
 */
bool can_layer( CHAR_DATA *ch, OBJ_DATA *obj, short wear_loc )
{
   OBJ_DATA *otmp;
   short bitlayers = 0;
   short objlayers = obj->pIndexData->layers;

   for( otmp = ch->first_carrying; otmp; otmp = otmp->next_content )
   {
      if( otmp->wear_loc == wear_loc )
      {
         if( !otmp->pIndexData->layers )
            return false;
         else
            bitlayers |= otmp->pIndexData->layers;
      }
   }
   if( ( bitlayers && !objlayers ) || bitlayers > objlayers )
      return false;
   if( !bitlayers || ( ( bitlayers & ~objlayers ) == bitlayers ) )
      return true;
   return false;
}

void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, short wear_bit )
{
   OBJ_DATA *dw, *lh, *rh;
   short bit, tmp;

   separate_obj( obj );
   if( wear_bit > -1 )
   {
      bit = wear_bit;
      if( !can_wear( obj, bit ) )
      {
         if( fReplace )
         {
            switch( bit )
            {
               case ITEM_WEAR_HOLD_B:
               case ITEM_WEAR_HOLD:
                  send_to_char( "You can't hold that.\r\n", ch );
                  break;

               default:
                  ch_printf( ch, "You can't wear that on your %s.\r\n", w_flags[bit] );
                  break;
            }
         }
         return;
      }
   }
   else
   {
      for( bit = -1, tmp = 1; tmp < ITEM_WEAR_MAX; tmp++ )
      {
         if( can_wear( obj, tmp ) )
         {
            bit = tmp;
            break;
         }
      }
   }

   if( bit == -1 )
   {
      if( fReplace )
         send_to_char( "You can't wear that.\r\n", ch );
      return;
   }

   switch( bit )
   {
      default:
         bug( "%s: uknown/unused item_wear bit %d", __FUNCTION__, bit );
         if( fReplace )
            send_to_char( "You can't wear that.\r\n", ch );
         return;

      case ITEM_WEAR_HEAD:
         if( !can_layer( ch, obj, WEAR_HEAD ) && !remove_obj( ch, WEAR_HEAD, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n dons $p upon $s head.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You don $p upon your head.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_HEAD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_EARS:
      case ITEM_WEAR_EAR:
         if( ( get_eq_char( ch, WEAR_EARS ) && !can_layer( ch, obj, WEAR_EARS ) && !remove_obj( ch, WEAR_EARS, fReplace ) )
         || ( get_eq_char( ch, WEAR_EAR_L ) && get_eq_char( ch, WEAR_EAR_R )
         && !can_layer( ch, obj, WEAR_EAR_L ) && !can_layer( ch, obj, WEAR_EAR_R )
         && !remove_obj( ch, WEAR_EAR_L, fReplace ) && !remove_obj( ch, WEAR_EAR_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_EARS );
         lh = get_eq_char( ch, WEAR_EAR_L );
         rh = get_eq_char( ch, WEAR_EAR_R );

         if( ( dw && !can_layer( ch, obj, WEAR_EARS ) )
         || ( lh && !can_layer( ch, obj, WEAR_EAR_L ) && rh && !can_layer( ch, obj, WEAR_EAR_R ) ) )
         {
            send_to_char( "You're already wearing something on both ears.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_EARS )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_EAR_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_EAR_R ) ) )
            {
               send_to_char( "You need to have both your ears free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s ears.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your ears.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_EARS );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_EARS ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_EAR_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left ear.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left ear.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_EAR_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_EAR_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right ear.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right ear.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_EAR_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free ears.", __FUNCTION__ );
         send_to_char( "You already wear something on both ears.\r\n", ch );
         return;

      case ITEM_WEAR_EYES:
      case ITEM_WEAR_EYE:
         if( ( get_eq_char( ch, WEAR_EYES ) && !can_layer( ch, obj, WEAR_EYES ) && !remove_obj( ch, WEAR_EYES, fReplace ) )
         || ( get_eq_char( ch, WEAR_EYE_L ) && get_eq_char( ch, WEAR_EYE_R )
         && !can_layer( ch, obj, WEAR_EYE_L ) && !can_layer( ch, obj, WEAR_EYE_R )
         && !remove_obj( ch, WEAR_EYE_L, fReplace ) && !remove_obj( ch, WEAR_EYE_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_EYES );
         lh = get_eq_char( ch, WEAR_EYE_L );
         rh = get_eq_char( ch, WEAR_EYE_R );

         if( ( dw && !can_layer( ch, obj, WEAR_EYES ) )
         || ( lh && !can_layer( ch, obj, WEAR_EYE_L ) && rh && !can_layer( ch, obj, WEAR_EYE_R ) ) )
         {
            send_to_char( "You're already wearing something on both eyes.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_EYES )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_EYE_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_EYE_R ) ) )
            {
               send_to_char( "You need to have both your eyes free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s eyes.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your eyes.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_EYES );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_EYES ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_EYE_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left eye.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left eye.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_EYE_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_EYE_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right eye.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right eye.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_EYE_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free eyes.", __FUNCTION__ );
         send_to_char( "You already wear something on both eyes.\r\n", ch );
         return;

      case ITEM_WEAR_FACE:
         if( !can_layer( ch, obj, WEAR_FACE ) && !remove_obj( ch, WEAR_FACE, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n places $p on $s face.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You place $p on your face.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_FACE );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_NECK:
         if( !can_layer( ch, obj, WEAR_NECK ) && !remove_obj( ch, WEAR_NECK, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_NECK );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_SHOULDERS:
      case ITEM_WEAR_SHOULDER:
         if( ( get_eq_char( ch, WEAR_SHOULDERS ) && !can_layer( ch, obj, WEAR_SHOULDERS ) && !remove_obj( ch, WEAR_SHOULDERS, fReplace ) )
         || ( get_eq_char( ch, WEAR_SHOULDER_L ) && get_eq_char( ch, WEAR_SHOULDER_R )
         && !can_layer( ch, obj, WEAR_SHOULDER_L ) && !can_layer( ch, obj, WEAR_SHOULDER_R )
         && !remove_obj( ch, WEAR_SHOULDER_L, fReplace ) && !remove_obj( ch, WEAR_SHOULDER_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_SHOULDERS );
         lh = get_eq_char( ch, WEAR_SHOULDER_L );
         rh = get_eq_char( ch, WEAR_SHOULDER_R );

         if( ( dw && !can_layer( ch, obj, WEAR_SHOULDERS ) )
         || ( lh && !can_layer( ch, obj, WEAR_SHOULDER_L ) && rh && !can_layer( ch, obj, WEAR_SHOULDER_R ) ) )
         {
            send_to_char( "You're already wearing something on both shoulders.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_SHOULDERS )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_SHOULDER_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_SHOULDER_R ) ) )
            {
               send_to_char( "You need to have both your shoulders free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s shoulders.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your shoulders.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_SHOULDERS );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_SHOULDERS ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_SHOULDER_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left shoulder.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left shoulder.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_SHOULDER_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_SHOULDER_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right shoulder.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right shoulder.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_SHOULDER_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free shoulders.", __FUNCTION__ );
         send_to_char( "You already wear something on both shoulders.\r\n", ch );
         return;

      case ITEM_WEAR_ABOUT:
         if( !can_layer( ch, obj, WEAR_ABOUT ) && !remove_obj( ch, WEAR_ABOUT, fReplace ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your body.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_ABOUT );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_BODY:
         if( !can_layer( ch, obj, WEAR_BODY ) && !remove_obj( ch, WEAR_BODY, fReplace ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n fits $p on $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You fit $p on your body.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_BODY );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_BACK:
         if( !can_layer( ch, obj, WEAR_BACK ) && !remove_obj( ch, WEAR_BACK, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n slings $p on $s back.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You sling $p on your back.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_BACK );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_ARMS:
      case ITEM_WEAR_ARM:
         if( ( get_eq_char( ch, WEAR_ARMS ) && !can_layer( ch, obj, WEAR_ARMS ) && !remove_obj( ch, WEAR_ARMS, fReplace ) )
         || ( get_eq_char( ch, WEAR_ARM_L ) && get_eq_char( ch, WEAR_ARM_R )
         && !can_layer( ch, obj, WEAR_ARM_L ) && !can_layer( ch, obj, WEAR_ARM_R )
         && !remove_obj( ch, WEAR_ARM_L, fReplace ) && !remove_obj( ch, WEAR_ARM_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_ARMS );
         lh = get_eq_char( ch, WEAR_ARM_L );
         rh = get_eq_char( ch, WEAR_ARM_R );

         if( ( dw && !can_layer( ch, obj, WEAR_ARMS ) )
         || ( lh && !can_layer( ch, obj, WEAR_ARM_L ) && rh && !can_layer( ch, obj, WEAR_ARM_R ) ) )
         {
            send_to_char( "You're already wearing something on both arms.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_ARMS )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_ARM_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_ARM_R ) ) )
            {
               send_to_char( "You need to have both your arms free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ARMS );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_ARMS ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_ARM_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left arm.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left arm.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_ARM_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_ARM_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right arm.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right arm.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_ARM_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free arms.", __FUNCTION__ );
         send_to_char( "You already wear something on both arms.\r\n", ch );
         return;

      case ITEM_WEAR_WRISTS:
      case ITEM_WEAR_WRIST:
         if( ( get_eq_char( ch, WEAR_WRISTS ) && !can_layer( ch, obj, WEAR_WRISTS ) && !remove_obj( ch, WEAR_WRISTS, fReplace ) )
         || ( get_eq_char( ch, WEAR_WRIST_L ) && get_eq_char( ch, WEAR_WRIST_R )
         && !can_layer( ch, obj, WEAR_WRIST_L ) && !can_layer( ch, obj, WEAR_WRIST_R )
         && !remove_obj( ch, WEAR_WRIST_L, fReplace ) && !remove_obj( ch, WEAR_WRIST_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_WRISTS );
         lh = get_eq_char( ch, WEAR_WRIST_L );
         rh = get_eq_char( ch, WEAR_WRIST_R );

         if( ( dw && !can_layer( ch, obj, WEAR_WRISTS ) )
         || ( lh && !can_layer( ch, obj, WEAR_WRIST_L ) && rh && !can_layer( ch, obj, WEAR_WRIST_R ) ) )
         {
            send_to_char( "You're already wearing something on both wrists.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_WRISTS )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_WRIST_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_WRIST_R ) ) )
            {
               send_to_char( "You need to have both your wrists free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s wrists.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your wrists.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_WRISTS );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_WRISTS ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_WRIST_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left wrist.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left wrist.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_WRIST_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_WRIST_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right wrist.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right wrist.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_WRIST_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free wrists.", __FUNCTION__ );
         send_to_char( "You already wear something on both wrists.\r\n", ch );
         return;

      case ITEM_WEAR_HANDS:
      case ITEM_WEAR_HAND:
         if( ( get_eq_char( ch, WEAR_HANDS ) && !can_layer( ch, obj, WEAR_HANDS ) && !remove_obj( ch, WEAR_HANDS, fReplace ) )
         || ( get_eq_char( ch, WEAR_HAND_L ) && get_eq_char( ch, WEAR_HAND_R )
         && !can_layer( ch, obj, WEAR_HAND_L ) && !can_layer( ch, obj, WEAR_HAND_R )
         && !remove_obj( ch, WEAR_HAND_L, fReplace ) && !remove_obj( ch, WEAR_HAND_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_HANDS );
         lh = get_eq_char( ch, WEAR_HAND_L );
         rh = get_eq_char( ch, WEAR_HAND_R );

         if( ( dw && !can_layer( ch, obj, WEAR_HANDS ) )
         || ( lh && !can_layer( ch, obj, WEAR_HAND_L ) && rh && !can_layer( ch, obj, WEAR_HAND_R ) ) )
         {
            send_to_char( "You're already wearing something on both hands.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_HANDS )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_HAND_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_HAND_R ) ) )
            {
               send_to_char( "You need to have both your hands free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_HANDS );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_HANDS ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_HAND_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left hand.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left hand.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_HAND_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_HAND_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right hand.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right hand.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_HAND_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free ears.", __FUNCTION__ );
         send_to_char( "You already wear something on both hands.\r\n", ch );
         return;

      case ITEM_WEAR_FINGERS:
      case ITEM_WEAR_FINGER:
         if( ( get_eq_char( ch, WEAR_FINGERS ) && !can_layer( ch, obj, WEAR_FINGERS ) && !remove_obj( ch, WEAR_FINGERS, fReplace ) )
         || ( get_eq_char( ch, WEAR_FINGER_L ) && get_eq_char( ch, WEAR_FINGER_R )
         && !can_layer( ch, obj, WEAR_FINGER_L ) && !can_layer( ch, obj, WEAR_FINGER_R )
         && !remove_obj( ch, WEAR_FINGER_L, fReplace ) && !remove_obj( ch, WEAR_FINGER_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_FINGERS );
         lh = get_eq_char( ch, WEAR_FINGER_L );
         rh = get_eq_char( ch, WEAR_FINGER_R );

         if( ( dw && !can_layer( ch, obj, WEAR_FINGERS ) )
         || ( lh && !can_layer( ch, obj, WEAR_FINGER_L ) && rh && !can_layer( ch, obj, WEAR_FINGER_R ) ) )
         {
            send_to_char( "You're already wearing something on both fingers.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_FINGERS )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_FINGER_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_FINGER_R ) ) )
            {
               send_to_char( "You need to have both your fingers free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s fingers.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your fingers.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FINGERS );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_FINGERS ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_FINGER_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left finger.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left finger.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_FINGER_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_FINGER_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right finger.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_FINGER_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free fingers.", __FUNCTION__ );
         send_to_char( "You already wear something on both fingers.\r\n", ch );
         return;

      /* Don't think we need layering here...do we? */
      case ITEM_WEAR_HOLD_B:
      case ITEM_WEAR_HOLD:
         if( !can_hold( ch ) )
            return;
         if( ( get_eq_char( ch, WEAR_HOLD_B ) && !remove_obj( ch, WEAR_HOLD_B, fReplace ) )
         || ( get_eq_char( ch, WEAR_HOLD_L ) && get_eq_char( ch, WEAR_HOLD_R )
         && !remove_obj( ch, WEAR_HOLD_L, fReplace ) && !remove_obj( ch, WEAR_HOLD_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_HOLD_B );
         lh = get_eq_char( ch, WEAR_HOLD_L );
         rh = get_eq_char( ch, WEAR_HOLD_R );

         if( dw )
         {
            send_to_char( "You're already holding something that requires both hands.\r\n", ch );
            return;
         }
         if( lh && rh )
         {
            send_to_char( "You're already holding something in each hand.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_HOLD_B )
         {
            if( lh || rh )
            {
               send_to_char( "You need to have both your hands free for this.\r\n", ch );
               return;
            }
            if( get_obj_weight( obj ) > ( get_curr_str( ch ) / 8 ) )
            {
               send_to_char( "It is to heavy for you to hold...even using both hands.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n holds $p in both $s hands.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You hold $p in both your hands.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_HOLD_B );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( ( lh && ( get_obj_weight( obj ) + get_obj_weight( lh ) ) > ( get_curr_str( ch ) / 4 ) )
         || ( rh && ( get_obj_weight( obj ) + get_obj_weight( rh ) ) > ( get_curr_str( ch ) / 4 ) ) )
         {
            send_to_char( "It is too heavy for you to hold.\r\n", ch );
            return;
         }
         if( lh )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n holds $p in $s right hand.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You hold $p in your right hand.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_HOLD_R );
         }
         else
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n holds $p in $s left hand.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You hold $p in your left hand.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_HOLD_L );
         }
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_WAIST:
         if( !can_layer( ch, obj, WEAR_WAIST ) && !remove_obj( ch, WEAR_WAIST, fReplace ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\r\n", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_WAIST );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_LEGS:
      case ITEM_WEAR_LEG:
         if( ( get_eq_char( ch, WEAR_LEGS ) && !can_layer( ch, obj, WEAR_LEGS ) && !remove_obj( ch, WEAR_LEGS, fReplace ) )
         || ( get_eq_char( ch, WEAR_LEG_L ) && get_eq_char( ch, WEAR_LEG_R )
         && !can_layer( ch, obj, WEAR_LEG_L ) && !can_layer( ch, obj, WEAR_LEG_R )
         && !remove_obj( ch, WEAR_LEG_L, fReplace ) && !remove_obj( ch, WEAR_LEG_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_LEGS );
         lh = get_eq_char( ch, WEAR_LEG_L );
         rh = get_eq_char( ch, WEAR_LEG_R );

         if( ( dw && !can_layer( ch, obj, WEAR_LEGS ) )
         || ( lh && !can_layer( ch, obj, WEAR_LEG_L ) && rh && !can_layer( ch, obj, WEAR_LEG_R ) ) )
         {
            send_to_char( "You're already wearing something on both legs.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_LEGS )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_LEG_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_LEG_R ) ) )
            {
               send_to_char( "You need to have both your legs free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s legs.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your legs.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_LEGS );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_LEGS ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_LEG_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left leg.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left leg.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_LEG_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_LEG_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right leg.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right leg.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_LEG_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free legs.", __FUNCTION__ );
         send_to_char( "You already wear something on both legs.\r\n", ch );
         return;

      case ITEM_WEAR_ANKLES:
      case ITEM_WEAR_ANKLE:
         if( ( get_eq_char( ch, WEAR_ANKLES ) && !can_layer( ch, obj, WEAR_ANKLES ) && !remove_obj( ch, WEAR_ANKLES, fReplace ) )
         || ( get_eq_char( ch, WEAR_ANKLE_L ) && get_eq_char( ch, WEAR_ANKLE_R )
         && !can_layer( ch, obj, WEAR_ANKLE_L ) && !can_layer( ch, obj, WEAR_ANKLE_R )
         && !remove_obj( ch, WEAR_ANKLE_L, fReplace ) && !remove_obj( ch, WEAR_ANKLE_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_ANKLES );
         lh = get_eq_char( ch, WEAR_ANKLE_L );
         rh = get_eq_char( ch, WEAR_ANKLE_R );

         if( ( dw && !can_layer( ch, obj, WEAR_ANKLES ) )
         || ( lh && !can_layer( ch, obj, WEAR_ANKLE_L ) && rh && !can_layer( ch, obj, WEAR_ANKLE_R ) ) )
         {
            send_to_char( "You're already wearing something on both ankles.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_ANKLES )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_ANKLE_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_ANKLE_R ) ) )
            {
               send_to_char( "You need to have both your ankles free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s ankles.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your ankles.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ANKLES );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_ANKLES ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_ANKLE_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left ankle.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left ankle.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_ANKLE_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_ANKLE_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right ankle.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right ankle.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_ANKLE_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free ankles.", __FUNCTION__ );
         send_to_char( "You already wear something on both ankles.\r\n", ch );
         return;

      case ITEM_WEAR_FEET:
      case ITEM_WEAR_FOOT:
         if( ( get_eq_char( ch, WEAR_FEET ) && !can_layer( ch, obj, WEAR_FEET ) && !remove_obj( ch, WEAR_FEET, fReplace ) )
         || ( get_eq_char( ch, WEAR_FOOT_L ) && get_eq_char( ch, WEAR_FOOT_R )
         && !can_layer( ch, obj, WEAR_FOOT_L ) && !can_layer( ch, obj, WEAR_FOOT_R )
         && !remove_obj( ch, WEAR_FOOT_L, fReplace ) && !remove_obj( ch, WEAR_FOOT_R, fReplace ) ) )
            return;

         dw = get_eq_char( ch, WEAR_FEET );
         lh = get_eq_char( ch, WEAR_FOOT_L );
         rh = get_eq_char( ch, WEAR_FOOT_R );

         if( ( dw && !can_layer( ch, obj, WEAR_FEET ) )
         || ( lh && !can_layer( ch, obj, WEAR_FOOT_L ) && rh && !can_layer( ch, obj, WEAR_FOOT_R ) ) )
         {
            send_to_char( "You're already wearing something on both feet.\r\n", ch );
            return;
         }
         if( bit == ITEM_WEAR_FEET )
         {
            if( ( lh && !can_layer( ch, obj, WEAR_FOOT_L ) )
            || ( rh && !can_layer( ch, obj, WEAR_FOOT_R ) ) )
            {
               send_to_char( "You need to have both your feet free for this.\r\n", ch );
               return;
            }
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FEET );
            oprog_wear_trigger( ch, obj );
            return;
         }
         if( !dw || can_layer( ch, obj, WEAR_FEET ) )
         {
            if( !lh || can_layer( ch, obj, WEAR_FOOT_L ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s left foot.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your left foot.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_FOOT_L );
               oprog_wear_trigger( ch, obj );
               return;
            }
            if( !rh || can_layer( ch, obj, WEAR_FOOT_R ) )
            {
               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wears $p on $s right foot.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wear $p on your right foot.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_FOOT_R );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }
         bug( "%s: no free feet.", __FUNCTION__ );
         send_to_char( "You already wear something on both feet.\r\n", ch );
         return;
   }
}

CMDF( do_wear )
{
   OBJ_DATA *obj;
   char arg1[MIL], arg2[MIL];
   short wear_bit;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( ( !str_cmp( arg2, "on" ) || !str_cmp( arg2, "upon" ) || !str_cmp( arg2, "around" ) ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Wear what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg1, "all" ) )
   {
      OBJ_DATA *obj_next;

      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
         {
            wear_obj( ch, obj, false, -1 );
            if( char_died( ch ) )
               return;
         }
      }
   }
   else
   {
      if( !( obj = get_obj_carry( ch, arg1 ) ) )
      {
         send_to_char( "You don't have that item.\r\n", ch );
         return;
      }
      if( arg2 != NULL || arg2[0] != '\0' )
         wear_bit = get_flag( arg2, w_flags, ITEM_WEAR_MAX );
      else
         wear_bit = -1;
      wear_obj( ch, obj, true, wear_bit );
   }
}

CMDF( do_remove )
{
   OBJ_DATA *obj, *obj_next;
   char arg[MIL];

   one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Remove what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg, "all" ) )  /* SB Remove all */
   {
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj ) )
            remove_obj( ch, obj->wear_loc, true );
      }
      return;
   }

   if( !( obj = get_obj_wear( ch, arg ) ) )
   {
      send_to_char( "You aren't using that item.\r\n", ch );
      return;
   }
   if( ( obj_next = get_eq_char( ch, obj->wear_loc ) ) != obj )
   {
      act( AT_PLAIN, "You must remove $p first.", ch, obj_next, NULL, TO_CHAR );
      return;
   }

   remove_obj( ch, obj->wear_loc, true );
}

CMDF( do_bury )
{
   OBJ_DATA *obj = NULL, *shovel = NULL;
   EXIT_DATA *pexit = NULL;
   char arg[MIL];
   int dir = -1;
   short move;

   one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "What do you wish to bury?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->item_type == ITEM_SHOVEL )
      {
         shovel = obj;
         break;
      }
   }

   if( ch->in_room->sector_type == SECT_CITY || ch->in_room->sector_type == SECT_INSIDE )
   {
      send_to_char( "The floor is too hard to dig through.\r\n", ch );
      return;
   }

   if( ch->in_room->sector_type == SECT_WATER_SWIM
   || ch->in_room->sector_type == SECT_WATER_NOSWIM
   || ch->in_room->sector_type == SECT_UNDERWATER )
   {
      send_to_char( "You can't bury something here.\r\n", ch );
      return;
   }

   if( ch->in_room->sector_type == SECT_AIR )
   {
      send_to_char( "What?  In the air?!\r\n", ch );
      return;
   }

   if( !( obj = get_obj_list_rev( ch, arg, ch->in_room->last_content ) )
   && !( pexit = find_door( ch, arg, true ) ) && ( dir = get_dir( arg ) ) == -1 )
   {
      send_to_char( "You can't find it.\r\n", ch );
      return;
   }

   if( pexit )
   {
      if( !shovel )
      {
         send_to_char( "You will need a shovel to bury an exit.\r\n", ch );
         return;
      }

      if( xIS_SET( pexit->exit_info, EX_DIG ) && xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "That exit is already buried.\r\n", ch );
         return;
      }

      move = 250;
      if( move > ch->move )
      {
         send_to_char( "You don't have the energy to bury an exit.\r\n", ch );
         return;
      }
      ch->move -= move;

      xSET_BIT( pexit->exit_info, EX_CLOSED );
      xSET_BIT( pexit->exit_info, EX_DIG );
      send_to_char( "You bury a passageway!\r\n", ch );
      act( AT_PLAIN, "$n buries a passageway!", ch, NULL, NULL, TO_ROOM );

      if( shovel && --shovel->value[0] <= 0 )
      {
         if( is_obj_stat( shovel, ITEM_NOSCRAP ) )
            shovel->value[0] = 1;
         else
         {
            act( AT_SKILL, "Your $p breaks as you finish buring!", ch, shovel, NULL, TO_CHAR );
            extract_obj( shovel );
         }
      }

      wait_state( ch, 50 );
      return;
   }

   separate_obj( obj );
   if( can_wear( obj, ITEM_NO_TAKE ) )
   {
      if( !is_obj_stat( obj, ITEM_CLANCORPSE ) || is_npc( ch )
      || !xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
      {
         act( AT_PLAIN, "You can't bury $p.", ch, obj, 0, TO_CHAR );
         return;
      }
   }

   if( obj->weight > ( UMAX( 5, ( can_carry_w( ch ) / 10 ) ) ) && !shovel )
   {
      send_to_char( "You'd need a shovel to bury something that big.\r\n", ch );
      return;
   }

   move = ( obj->weight * 50 * ( shovel ? 1 : 5 ) ) / UMAX( 1, can_carry_w( ch ) );
   move = URANGE( 2, move, 1000 );
   if( move > ch->move )
   {
      send_to_char( "You don't have the energy to bury something of that size.\r\n", ch );
      return;
   }
   ch->move -= move;
   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
      adjust_favor( ch, 4, 1 );

   act( AT_ACTION, "You solemnly bury $p...", ch, obj, NULL, TO_CHAR );
   act( AT_ACTION, "$n solemnly buries $p...", ch, obj, NULL, TO_ROOM );
   xSET_BIT( obj->extra_flags, ITEM_BURIED );
   wait_state( ch, URANGE( 10, move / 2, 100 ) );
}

int sacrifice_content( CHAR_DATA *ch, OBJ_DATA *obj )
{
   OBJ_DATA *tobj, *tobj_next;
   int ocount = 0;

   if( obj )
      ocount = obj->count;
   if( obj && obj->first_content )
   {
      for( tobj = obj->first_content; tobj; tobj = tobj_next )
      {
         tobj_next = tobj->next_content;

         ocount += sacrifice_content( ch, tobj );
      }
   }
   return ocount;
}

void sacrifice_object( CHAR_DATA *ch, OBJ_DATA *obj )
{
   char buf[MSL], name[50];
   int objcount = 0, uamount;

   if( can_wear( obj, ITEM_NO_TAKE ) )
   {
      act( AT_OBJECT, "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
      return;
   }
   if( xIS_SET( obj->extra_flags, ITEM_PKDISARMED ) && !is_npc( ch ) )
   {
      if( can_pkill( ch ) && !get_timer( ch, TIMER_PKILLED ) )
      {
         if( ch->level - obj->value[5] > 5 || obj->value[5] - ch->level > 5 )
         {
            send_to_char( "\r\n&bA godly force freezes your outstretched hand.\r\n", ch );
            return;
         }
      }
   }
   if( !is_npc( ch ) && ch->pcdata->deity && ch->pcdata->deity->name[0] != '\0' )
      mudstrlcpy( name, ch->pcdata->deity->name, sizeof( name ) );
   else
      mudstrlcpy( name, "Thoric", sizeof( name ) );
   objcount += sacrifice_content( ch, obj );
   uamount = objcount;
   increase_gold( ch, uamount );
   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
      adjust_favor( ch, 3, objcount );
   ch_printf( ch, "%s gives you %d gold coin%s for your sacrifice.\r\n", name, objcount, objcount != 1 ? "s" : "" );
   snprintf( buf, sizeof( buf ), "$n sacrifices $p to %s.", name );
   act( AT_ACTION, buf, ch, obj, NULL, TO_ROOM );
   oprog_sac_trigger( ch, obj );
   extract_obj( obj );
}

CMDF( do_sacrifice )
{
   OBJ_DATA *obj, *obj_next;
   char arg[MIL];

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !str_cmp( arg, ch->name ) )
   {
      act( AT_ACTION, "$n offers $mself to $s deity, who graciously declines.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "Your deity appreciates your offer and may accept it later.\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg, "all" ) )
   {
      for( obj = ch->in_room->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         sacrifice_object( ch, obj );
         if( char_died( ch ) )
            return;
      }
      return;
   }
   if( !( obj = get_obj_list_rev( ch, arg, ch->in_room->last_content ) ) )
   {
      send_to_char( "You can't find it.\r\n", ch );
      return;
   }
   separate_obj( obj );
   sacrifice_object( ch, obj );
}

void brandish( CHAR_DATA *ch, OBJ_DATA *staff )
{
   CHAR_DATA *vch, *vch_next;
   ch_ret retcode;
   int sn;

   if( ( sn = staff->value[3] ) < 0 || sn >= top_sn || !skill_table[sn]->spell_fun )
   {
      bug( "%s: bad sn (%d) for value[3] on object vnum [%d].", __FUNCTION__, sn, staff->pIndexData->vnum );
      return;
   }

   wait_state( ch, 2 * PULSE_VIOLENCE );

   if( staff->value[2] > 0 )
   {
      if( !oprog_use_trigger( ch, staff, NULL, NULL ) )
      {
         act( AT_MAGIC, "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
         act( AT_MAGIC, "You brandish $p.", ch, staff, NULL, TO_CHAR );
      }
      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;
         if( !is_npc( vch ) && xIS_SET( vch->act, PLR_WIZINVIS ) && get_trust( ch ) < get_trust( vch ) )
            continue;
         else if( !can_see_character( ch, vch ) )
            continue;
         else
         {
            switch( skill_table[sn]->target )
            {
               default:
                  bug( "%s: bad target for sn %d.", __FUNCTION__, sn );
                  return;

               case TAR_IGNORE:
                  if( vch != ch )
                     continue;
                  break;

               case TAR_CHAR_OFFENSIVE:
                  if( is_npc( ch ) ? is_npc( vch ) : !is_npc( vch ) )
                     continue;
                  break;

               case TAR_CHAR_DEFENSIVE:
                  if( is_npc( ch ) ? !is_npc( vch ) : is_npc( vch ) )
                     continue;
                  break;

               case TAR_CHAR_SELF:
                  if( vch != ch )
                     continue;
                  break;
            }
         }

         retcode = obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
         if( retcode == rCHAR_DIED || retcode == rBOTH_DIED )
         {
            bug( "%s: char died", __FUNCTION__ );
            return;
         }
      }
   }

   if( --staff->value[2] <= 0 )
   {
      act( AT_MAGIC, "$p blazes bright and vanishes from $n's hands!", ch, staff, NULL, TO_ROOM );
      act( AT_MAGIC, "$p blazes bright and is gone!", ch, staff, NULL, TO_CHAR );
      extract_obj( staff );
   }
}

CMDF( do_brandish )
{
   OBJ_DATA *staff;

   if( !( staff = get_eq_hold( ch, ITEM_STAFF ) ) )
   {
      send_to_char( "You need to be holding a staff to brandish.\r\n", ch );
      return;
   }

   brandish( ch, staff );

   /* See if there is another one to brandish */
   if( ( staff = get_next_eq_hold( ch, staff, ITEM_STAFF ) ) )
      brandish( ch, staff );
}

void zap( CHAR_DATA *ch, OBJ_DATA *wand, CHAR_DATA *victim, OBJ_DATA *obj )
{
   ch_ret retcode;

   wait_state( ch, 1 * PULSE_VIOLENCE );

   if( wand->value[2] > 0 )
   {
      if( victim )
      {
         if( !oprog_use_trigger( ch, wand, victim, NULL ) )
         {
            act( AT_MAGIC, "$n aims $p at $N.", ch, wand, victim, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $N.", ch, wand, victim, TO_CHAR );
         }
      }
      else
      {
         if( !oprog_use_trigger( ch, wand, NULL, obj ) )
         {
            act( AT_MAGIC, "$n aims $p at $P.", ch, wand, obj, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $P.", ch, wand, obj, TO_CHAR );
         }
      }

      retcode = obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
      if( retcode == rCHAR_DIED || retcode == rBOTH_DIED )
      {
         bug( "%s: char died", __FUNCTION__ );
         return;
      }
   }

   if( --wand->value[2] <= 0 )
   {
      act( AT_MAGIC, "$p explodes into fragments.", ch, wand, NULL, TO_ROOM );
      act( AT_MAGIC, "$p explodes into fragments.", ch, wand, NULL, TO_CHAR );
      extract_obj( wand );
   }
}

CMDF( do_zap )
{
   CHAR_DATA *victim;
   OBJ_DATA *wand, *obj = NULL;
   char arg[MIL];
   int ocheck = 0;

   if( ( !argument || argument[0] == '\0' ) && !ch->fighting )
   {
      send_to_char( "Zap whom or what?\r\n", ch );
      return;
   }

   if( !( wand = get_eq_hold( ch, ITEM_WAND ) ) )
   {
      send_to_char( "You need to be holding a wand to zap.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      if( ch->fighting )
         victim = who_fighting( ch );
      else
      {
         send_to_char( "Zap whom or what?\r\n", ch );
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

      if( !( victim = get_char_room( ch, arg ) ) && !( obj = new_get_obj_here( ocheck, ch, arg ) ) )
      {
         send_to_char( "You can't find it.\r\n", ch );
         return;
      }
   }

   zap( ch, wand, victim, obj );

   /* See if there is another wand to zap with */
   if( ( wand = get_next_eq_hold( ch, wand, ITEM_WAND ) ) )
      zap( ch, wand, victim, obj );
}

/* Make objects in rooms that are nofloor fall - Scryn 1/23/96 */
void obj_fall( OBJ_DATA *obj, bool through )
{
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *to_room;
   static int fall_count;
   static bool is_falling; /* Stop loops from the call to obj_to_room()  -- Altrag */

   if( !obj->in_room || is_falling )
      return;

   if( fall_count > 30 )
   {
      bug( "%s", "object falling in loop more than 30 times" );
      extract_obj( obj );
      fall_count = 0;
      return;
   }

   if( xIS_SET( obj->in_room->room_flags, ROOM_NOFLOOR ) && can_go( obj->in_room, DIR_DOWN ) && !is_obj_stat( obj, ITEM_MAGIC ) )
   {
      pexit = get_exit( obj->in_room, DIR_DOWN );
      to_room = pexit->to_room;

      if( through )
         fall_count++;
      else
         fall_count = 0;

      if( obj->in_room == to_room )
      {
         bug( "Object falling into same room, room %d", to_room->vnum );
         extract_obj( obj );
         return;
      }

      if( obj->in_room->first_person )
      {
         act( AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_ROOM );
         act( AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_CHAR );
      }
      obj_from_room( obj );
      is_falling = true;
      obj = obj_to_room( obj, to_room );
      is_falling = false;

      if( obj->in_room->first_person )
      {
         act( AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_ROOM );
         act( AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_CHAR );
      }

      if( !xIS_SET( obj->in_room->room_flags, ROOM_NOFLOOR ) && through )
      {
         int dam = fall_count * obj->weight / 2;

         /* Damage players */
         if( obj->in_room->first_person && number_percent( ) > 15 )
         {
            CHAR_DATA *rch;
            CHAR_DATA *vch = NULL;
            int chcnt = 0;

            for( rch = obj->in_room->first_person; rch; rch = rch->next_in_room, chcnt++ )
               if( number_range( 0, chcnt ) == 0 )
                  vch = rch;
            act( AT_WHITE, "$p falls on $n!", vch, obj, NULL, TO_ROOM );
            act( AT_WHITE, "$p falls on you!", vch, obj, NULL, TO_CHAR );

            if( is_npc( vch ) && xIS_SET( vch->act, ACT_HARDHAT ) )
               act( AT_WHITE, "$p bounces harmlessly off your head!", vch, obj, NULL, TO_CHAR );
            else
               damage( vch, vch, obj, dam * vch->level, TYPE_UNDEFINED, true );
         }

         /* Damage objects */
         switch( obj->item_type )
         {
            case ITEM_WEAPON:
            case ITEM_ARMOR:
               if( ( obj->value[0] - dam ) <= 0 )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               else
                  obj->value[0] -= dam;
               break;

            default:
               if( ( dam * 15 ) > get_obj_resistance( obj ) )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               break;
         }
      }
      obj_fall( obj, true );
   }
}

CMDF( do_locate )
{
   OBJ_DATA *obj, *in_obj;
   bool found = false;

   if( !ch || is_npc( ch ) )
      return;
   for( obj = first_object; obj; obj = obj->next )
   {
      if( !can_see_obj( ch, obj ) || !obj->owner || str_cmp( ch->name, obj->owner ) )
         continue;
      found = true;
      for( in_obj = obj; in_obj->in_obj; in_obj = in_obj->in_obj );
      {
         separate_obj( obj );
         ch_printf( ch, "&W%s &D", obj->short_descr );
         if( in_obj->carried_by )
            ch_printf( ch, "&Wcarried by %s.&D\r\n", PERS( in_obj->carried_by, ch ) );
         else
            ch_printf( ch, "&Win %s.&D\r\n", in_obj->in_room == NULL ? "somewhere" : in_obj->in_room->name );
      }
   }
   if( !found )
      send_to_char( "You can't locate any items belonging to you.\r\n", ch );
}

CMDF( do_claim )
{
   OBJ_DATA *obj;

   if( !ch || is_npc( ch ) )
      return;
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What object do you wish to claim ownership of?\r\n", ch );
      return;
   }
   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that item.\r\n", ch );
      return;
   }
   separate_obj( obj );
   if( obj->item_type == ITEM_MONEY || obj->item_type == ITEM_TREASURE )
   {
      send_to_char( "You can't claim that item.\r\n", ch );
      return;
   }
   if( obj->owner && strlen( obj->owner ) > 1 )
   {
      if( !str_cmp( ch->name, obj->owner ) )
         send_to_char( "But you already own it!\r\n", ch );
      else
         send_to_char( "Someone else has already claimed ownership of it.\r\n", ch );
      return;
   }
   STRSET( obj->owner, ch->name );
   save_char_obj( ch );
   act( AT_PLAIN, "You're now the owner of $p.", ch, obj, NULL, TO_CHAR );
   act( AT_PLAIN, "$n is now the owner of $p.", ch, obj, NULL, TO_ROOM );
}

CMDF( do_gift )
{
   OBJ_DATA *obj;
   CHAR_DATA *victim;
   char arg1[MIL], arg2[MIL];

   if( !ch || is_npc( ch ) )
      return;
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Make a gift of which object to whom?\r\n", ch );
      return;
   }
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Make a gift of which object to whom?\r\n", ch );
      return;
   }
   if( !( obj = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You aren't carrying that item.\r\n", ch );
      return;
   }
   if( !( victim = get_char_room( ch, arg2 ) ) )
   {
      send_to_char( "Nobody here by that name.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( !obj->owner || strlen( obj->owner ) < 2 )
   {
      send_to_char( "That item has not yet been claimed.\r\n", ch );
      return;
   }
   if( str_cmp( ch->name,obj->owner ) )
   {
      send_to_char( "But you don't own it!\r\n", ch );
      return;
   }
   separate_obj( obj );
   STRSET( obj->owner, victim->name );
   act( AT_PLAIN, "You grant ownership of $p to $N.", ch, obj, victim, TO_CHAR );
   act( AT_PLAIN, "$n grants ownership of $p to $N.", ch, obj, victim, TO_NOTVICT );
   act( AT_PLAIN, "$n grants ownership of $p to you.", ch, obj, victim, TO_VICT );
   obj_from_char( obj );
   obj = obj_to_char( obj, victim );
   if( !is_npc( ch ) && !char_died( ch ) )
      save_char_obj( ch );
   if( !is_npc( victim ) && !char_died( victim ) )
      save_char_obj( victim );
}

CMDF( do_call )
{
   OBJ_DATA *obj, *in_obj;
   CHAR_DATA *victim = NULL;
   DESCRIPTOR_DATA *d;
   ROOM_INDEX_DATA *chroom, *objroom;
   bool found = false;

   if( !ch || is_npc( ch ) )
      return;
   wait_state( ch, 12 );
   for( obj = first_object; obj; obj = obj->next )
   {
      separate_obj( obj );

      if( !obj->owner || strlen( obj->owner ) < 2 || str_cmp( ch->name, obj->owner )
      || obj->item_type == ITEM_CORPSE_PC )
         continue;
      for( in_obj = obj; in_obj->in_obj; in_obj = in_obj->in_obj );
         if( in_obj->carried_by )
            if( in_obj->carried_by == ch )
               continue;
      found = true;
      if( obj->carried_by )
      {
         if( obj->carried_by == ch || !obj->carried_by->desc || obj->carried_by->desc->connected != CON_PLAYING )
            if( !is_npc( obj->carried_by ) )
               return;
         act( AT_PLAIN, "$p suddenly vanishes from your hands!", obj->carried_by, obj, NULL, TO_CHAR );
         act( AT_PLAIN, "$p suddenly vanishes from $n's hands!", obj->carried_by, obj, NULL, TO_ROOM );
         obj_from_char( obj );
      }
      else if( obj->in_room )
      {
         chroom = ch->in_room;
         objroom = obj->in_room;
         char_from_room( ch );
         char_to_room( ch, objroom );
         act( AT_PLAIN, "$p vanishes from the ground!", ch, obj, NULL, TO_ROOM );
         if( chroom == objroom )
            act( AT_PLAIN, "$p vanishes from the ground!", ch, obj, NULL, TO_CHAR );
         char_from_room( ch );
         char_to_room( ch, chroom );
         obj_from_room( obj );
      }
      else if( obj->in_obj )
         obj_from_obj( obj );
      else
         continue;
      obj_to_char( obj, ch );
      act( AT_PLAIN, "$p materializes in your hands.", ch, obj, NULL, TO_CHAR );
      act( AT_PLAIN, "$p materializes in $n's hands.", ch, obj, NULL, TO_ROOM );
   }

   if( !found )
   {
      send_to_char( "Nothing you own needs to be called back to you.\r\n", ch );
      return;
   }

   /* Save everyone */
   for( d = first_descriptor; d; d = d->next )
   {
      if( d->connected != CON_PLAYING )
         continue;
      if( !( victim = d->character ) )
         continue;
      if( is_npc( victim ) )
         continue;
      save_char_obj( victim );
   }
}

CHAR_DATA *find_undertaker( CHAR_DATA *ch )
{
   CHAR_DATA *undertaker = NULL;

   for( undertaker = ch->in_room->first_person; undertaker; undertaker = undertaker->next_in_room )
   {
      if( is_npc( undertaker ) && xIS_SET( undertaker->act, ACT_UNDERTAKER ) )
         break;
   }
   return undertaker;
}

CMDF( do_retrieve )
{
   OBJ_DATA *obj;
   CHAR_DATA *ut, *rch;
   char buf2[MSL];
   int cost = 0;
   bool found, althere, charge;

   if( is_npc( ch ) )
   {
      send_to_char( "Npcs have no reason to have someone else retrieve their corpse(s).\r\n", ch );
      return;
   }

   if( !( ut = find_undertaker( ch ) ) )
   {
      send_to_char( "You can't find anyone to retrieve your corpse.\r\n", ch );
      return;
   }

   cost = ( ch->level * 10 );
   if( !has_gold( ch, cost ) )
   {
      send_to_char( "You don't have enough gold to have your corpse(s) retrieved.\r\n", ch );
      return;
   }

   found = false;
   althere = false;
   charge = false;

   snprintf( buf2, sizeof( buf2 ), "the corpse of %s", ch->name );
   for( obj = first_corpse; obj; obj = obj->next_corpse )
   {
      if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         if( obj->in_room == ch->in_room )
         {
            althere = true;
            continue;
         }
         found = true;
         charge = true;
         if( ( rch = obj->in_room->first_person ) && !is_obj_stat( obj, ITEM_BURIED ) )
         {
            act( AT_ACTION, "$T is grabbed by an undertaker.", rch, NULL, buf2, TO_ROOM );
            act( AT_ACTION, "$T is grabbed by an undertaker.", rch, NULL, buf2, TO_CHAR );
         }
         act( AT_ACTION, "$N strolls in carrying your corpse and quickly drops it.", ch, NULL, ut, TO_CHAR );
         act( AT_ACTION, "$N strolls in carrying $n's corpse and quickly drops it.", ch, NULL, ut, TO_ROOM );
         obj_from_room( obj );
         obj = obj_to_room( obj, ch->in_room );
         obj_to_char_cords( obj, ch );
      }
   }
   if( !found )
   {
      if( !althere )
         send_to_char( "No corpse of yours litters the world...\r\n", ch );
      else
         send_to_char( "Your corpse(s) are already in this room...\r\n", ch );
      return;
   }

   if( charge )
      decrease_gold( ch, cost );
}

CMDF( do_connect )
{
   OBJ_DATA *first_obj, *second_obj, *new_obj;
   OBJ_INDEX_DATA *inobj;
   char arg1[MSL], arg2[MSL];

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg2 == NULL || arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Usage: connect <first object> <second object>.\r\n", ch );
      return;
   }
   if( !( first_obj = get_obj_carry( ch, arg1 ) ) || !( second_obj = get_obj_carry( ch, arg2 ) ) )
   {
      send_to_char( "You must have both parts in your inventory!\r\n", ch );
      return;
   }
   separate_obj( first_obj );
   separate_obj( second_obj );
   if( first_obj->item_type != ITEM_PIECE || second_obj->item_type != ITEM_PIECE )
   {
      send_to_char( "Both items must be pieces of another item!\r\n", ch );
      return;
   }
   if( first_obj->value[0] != second_obj->pIndexData->vnum
   || second_obj->value[0] != first_obj->pIndexData->vnum
   || first_obj->value[1] != second_obj->value[1] )
   {
      act( AT_ACTION, "$n jiggles some pieces together, but can't seem to make them connect.", ch, NULL, NULL, TO_ROOM );
      act( AT_ACTION, "You try to fit them together every which way, but they just won't fit together.", ch, NULL, NULL, TO_CHAR );
      return;
   }
   if( !( inobj = get_obj_index( first_obj->value[1] ) ) )
   {
      send_to_char( "While these two items combine correctly the item they make doesn't exist.\r\n", ch );
      bug( "%s: Can't find object %d", __FUNCTION__, first_obj->value[1] );
      return;
   }
   if( !( new_obj = create_object( inobj, ch->level ) ) )
   {
      send_to_char( "While these two items combine correctly the item they make failed to be created.\r\n", ch );
      bug( "%s: Couldn't create_object [%d]", __FUNCTION__, inobj->vnum );
      return;
   }
   extract_obj( first_obj );
   extract_obj( second_obj );
   new_obj->level = new_obj->pIndexData->level;
   obj_to_char( new_obj, ch );
   act( AT_ACTION, "$n jiggles some pieces until they snap in place, creating $p!", ch, new_obj, NULL, TO_ROOM );
   act( AT_ACTION, "You jiggle the pieces until they snap into place, creating $p!", ch, new_obj, NULL, TO_CHAR );
}

/*
 * Make it so we can wash blood off stuff
 * bsplatter = Splattered blood
 * bstain = Blood stains
 */
CMDF( do_wash )
{
   OBJ_DATA *obj = NULL, *cobj, *uobj = NULL, *wobj = NULL;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What are you trying to wash?\r\n", ch );
      return;
   }

   if( ch->fighting )
   {
      send_to_char( "While you're fighting?  Nice try.\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      if( str_cmp( argument, "self" ) )
      {
         send_to_char( "You don't have that.\r\n", ch );
         return;
      }
   }

   /* Now we have what we want washed...check to see if we have something to get a deeper clean. */
   for( cobj = ch->first_carrying; cobj; cobj = cobj->next_content )
   {
      if( !wobj && cobj->item_type == ITEM_DRINK_CON && cobj->value[1] > 0 && cobj->value[2] == LIQ_WATER )
         wobj = cobj;
      if( !uobj && cobj->item_type == ITEM_CLEANER ) /* Something used to clean it */
         uobj = cobj;
      if( wobj && uobj )
         break;
   }

   wait_state( ch, number_range( 5, 10 ) );

   if( !obj )
   {
      if( !wobj )
      {
         send_to_char( "You need some water to wash up with.\r\n", ch );
         return;
      }
      separate_obj( wobj );
      wobj->value[1]--;
      ch->bsplatter = 0;
      act( AT_ACTION, "You pour water from $p on yourself to clean off all the blood.", ch, wobj, NULL, TO_CHAR );
      act( AT_ACTION, "$n pours water from $p on $mself to clean off all the blood.", ch, wobj, NULL, TO_ROOM );
      return;
   }

   separate_obj( obj );

   if( !uobj && !wobj ) /* Nothing to use to clean it */
      send_to_char( "You don't have anything you can use to wash it.\r\n", ch );
   else if( obj->bsplatter == 0 && obj->bstain == 0 ) /* Doesn't need cleaned */
      send_to_char( "It doesn't need cleaned.\r\n", ch );
   else if( uobj && obj->bstain > 0 ) /* No point in using cleaner if not needed */
   {
      /* This will be used to give it a good deep clean */
      separate_obj( uobj );
      act( AT_OBJECT, "You pour liquid from $p on $P, and watch the blood and stains vanish!", ch, uobj, obj, TO_CHAR );
      act( AT_OBJECT, "$n pours liquid from $p on $P, and watches the blood and stains vanish!", ch, uobj, obj, TO_ROOM );
      extract_obj( uobj );
      obj->bstain = 0;
      obj->bsplatter = 0;
   }
   else if( wobj && obj->bsplatter > 0 ) /* Just using water */
   {
      separate_obj( wobj );
      act( AT_OBJECT, "You pour water from $p on $P, and watch the blood run off!", ch, wobj, obj, TO_CHAR );
      act( AT_OBJECT, "$n pours water from $p on $P, and watches the blood run off!", ch, wobj, obj, TO_ROOM );
      wobj->value[1]--;
      obj->bsplatter = 0;
      if( obj->bstain > 0 )
         send_to_char( "You will need something stronger then water to take out the blood stains.\r\n", ch );
   }
   else if( !uobj && obj->bstain > 0 )
      send_to_char( "You will need something stronger then water to take out the blood stains.\r\n", ch );
   else if( !wobj && obj->bsplatter > 0 )
      send_to_char( "You will need some water to wash the blood off!\r\n", ch );
   else
      send_to_char( "Interesting that you got here, do share with someone how you got to this point.\r\n", ch );
}

/* Done to randomize an objects stats/requirements */
void randomize_obj( OBJ_DATA *obj )
{
   int stat, use;

   if( !obj )
   {
      bug( "%s: NULL obj", __FUNCTION__ );
      return;
   }

   /* Only set if the obj is set as a random */
   if( !xIS_SET( obj->extra_flags, ITEM_RANDOM ) )
      return;

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( obj->stat_reqs[stat] <= 0 )
         continue;
      /* Keep them in a range of 0 - MAX_LEVEL */
      obj->stat_reqs[stat] = urange( 0, number_range( ( obj->stat_reqs[stat] - 5 ), ( obj->stat_reqs[stat] + 5 ) ), MAX_LEVEL );
   }

   switch( obj->item_type )
   {
      case ITEM_CONTAINER:
         if( obj->value[0] > 0 )
            obj->value[0] = umax( 0, number_range( ( obj->value[0] - 10 ), ( obj->value[0] + 10 ) ) );
         break;

      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         if( obj->value[1] >= 0 || obj->value[2] >= 0 || obj->value[3] >= 0 )
            obj->value[0] = urange( 0, number_range( ( obj->value[0] - 5 ), ( obj->value[0] + 5 ) ), MAX_LEVEL );
         break;

      case ITEM_SALVE:
         if( obj->value[4] >= 0 || obj->value[5] >= 0 )
            obj->value[0] = urange( 0, number_range( ( obj->value[0] - 5 ), ( obj->value[0] + 5 ) ), MAX_LEVEL );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         if( obj->value[3] >= 0 )
            obj->value[0] = urange( 0, number_range( ( obj->value[0] - 5 ), ( obj->value[0] + 5 ) ), MAX_LEVEL );
         break;

      case ITEM_WEAPON:
         use = number_range( ( obj->value[1] - 5 ), ( obj->value[1] + 5 ) );
         if( use > 0 )
            obj->value[1] = use;
         /* have to make sure obj->value[2] is higher then obj->value[1] to make it look nice */
         obj->value[2] = umax( ( obj->value[1] + 2 ), number_range( ( obj->value[2] - 5 ), ( obj->value[2] + 5 ) ) );
         break;

      case ITEM_ARMOR:
         obj->value[0] = number_range( ( obj->value[0] - 20 ), ( obj->value[0] + 20 ) );
         break;
   }

   /* Well we have set it, so now remove the flag */
   xREMOVE_BIT( obj->extra_flags, ITEM_RANDOM );
}
