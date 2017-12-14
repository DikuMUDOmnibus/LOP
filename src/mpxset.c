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

#include <stdio.h>
#include <string.h>
#include "h/mud.h"

CMDF( do_mpmset )
{
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MSL];
   CHAR_DATA *victim;
   int value = 0, v2, stat;
   int minattr, maxattr;

   /* A desc means switched.. too many loopholes if we allow that.. */
   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   mudstrlcpy( arg3, argument, sizeof( arg3 ) );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      progbug( "MpMset: no victim argument", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' )
   {
      progbug( "MpMset: Null 2nd argument", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      progbug_printf( ch, "MpMset: victim (%s) not in same room", arg1 );
      return;
   }

   if( is_immortal( victim ) )
   {
      progbug_printf( ch, "MpMset: victim (%s) is immortal", victim->name );
      return;
   }

   if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
   {
      progbug_printf( ch, "MpMset: victim (%s) is proto", victim->name );
      return;
   }

   if( is_npc( victim ) )
   {
      minattr = 1;
      maxattr = ( MAX_LEVEL + 25 );
   }
   else
   {
      minattr = 3;
      maxattr = MAX_LEVEL;
   }

   if( is_number( arg3 ) )
      value = atoi( arg3 );

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( !str_cmp( arg2, stattypes[stat] ) )
      {
         if( value < minattr || value > maxattr )
         {
            progbug_printf( ch, "MpMset: Invalid (%s) value (%d)", capitalize( stattypes[stat] ), value );
            return;
         }
         victim->perm_stats[stat] = value;
         return;
      }
   }

   if( !str_cmp( arg2, "sav1" ) )
   {
      victim->saving_poison_death = URANGE( -30, value, 30 );
      return;
   }

   if( !str_cmp( arg2, "sav2" ) )
   {
      victim->saving_wand = URANGE( -30, value, 30 );
      return;
   }

   if( !str_cmp( arg2, "sav3" ) )
   {
      victim->saving_para_petri = URANGE( -30, value, 30 );
      return;
   }

   if( !str_cmp( arg2, "sav4" ) )
   {
      victim->saving_breath = URANGE( -30, value, 30 );
      return;
   }

   if( !str_cmp( arg2, "sav5" ) )
   {
      victim->saving_spell_staff = URANGE( -30, value, 30 );
      return;
   }

   if( !str_cmp( arg2, "sex" ) )
   {
      value = get_flag( arg3, sex_names, SEX_MAX );
      if( value < 0 && is_number( arg3 ) )
         value = atoi( arg3 );
      if( value < 0 || value >= SEX_MAX )
      {
         progbug_printf( ch, "MpMset: Invalid sex (%s)", arg3 );
         return;
      }
      victim->sex = value;
      return;
   }

   if( !str_cmp( arg2, "class" ) )
   {
      MCLASS_DATA *mclass;
      int uclass, mcount = 0;
      bool add = false;

      if( is_npc( victim ) )
      {
         send_to_char( "Mobiles have no reason to have a class.\r\n", ch );
         return;
      }
      value = -1;

      argument = one_argument( argument, arg2 );
      if( !str_cmp( arg2, "add" ) )
         add = true;
      else if( is_number( arg2 ) )
         value = atoi( arg2 );

      if( !add && value < 0 )
      {
         ch_printf( ch, "Invalid class number use a number equal to or above %d.\r\n", 0 );
         return;
      }

      uclass = get_pc_class( argument );
      if( uclass < 0 && is_number( argument ) )
         uclass = atoi( argument );
      if( uclass < -1 || uclass >= MAX_PC_CLASS )
      {
         ch_printf( ch, "Class range is -1 to %d.\r\n", ( MAX_PC_CLASS - 1 ) );
         return;
      }
      if( uclass >= 0 && char_is_class( victim, uclass ) )
      {
         ch_printf( ch, "They already have class [%d]%s.\r\n", uclass, dis_class_name( uclass ) );
         return;
      }

      if( add )
      {
         if( uclass >= 0 )
         {
            if( uclass >= 0 && char_is_class( victim, uclass ) )
            {
               ch_printf( ch, "They already have class [%d]%s.\r\n", uclass, dis_class_name( uclass ) );
               return;
            }
            if( !( mclass = add_mclass( victim, uclass, 2, 0, 0.0, false ) ) )
               ch_printf( ch, "Failed to add [%d]%s to them.\r\n", uclass, dis_class_name( uclass ) );
            else
               ch_printf( ch, "Class [%d]%s has been added to them.\r\n", mclass->wclass, dis_class_name( mclass->wclass ) );
         }
         else
            send_to_char( "Need to specify a valid class to be added.\r\n", ch );
         return;
      }

      for( mclass = victim->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( mcount++ == value )
         {
            mclass->wclass = uclass;
            ch_printf( ch, "Class %d has been set to [%d]%s.\r\n", value, mclass->wclass, dis_class_name( mclass->wclass ) );
            break;
         }
      }
      if( !mclass )
      {
         send_to_char( "There is no class in that spot on them to set.\r\n", ch );
         return;
      }
      if( mclass->wclass == -1 )
      {
         if( mcount > 1 ) /* They have more then one class set */
         {
            UNLINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
            DISPOSE( mclass );
            update_level( ch );
            send_to_char( "The class has been removed from them completely.\r\n", ch );
         }
         else
            send_to_char( "They only have 1 class and it has been set to none.\r\n", ch );
         return;
      }
      return;
   }

   if( !str_cmp( arg2, "race" ) )
   {
      if( is_npc( victim ) )
      {
         send_to_char( "Mobiles have no reason to have a race.\r\n", ch );
         return;
      }
      value = get_pc_race( arg3 );
      if( value < 0 && is_number( arg3 ) )
         value = atoi( arg3 );
      if( value < 0 || value >= MAX_PC_RACE )
      {
         ch_printf( ch, "Race range is 0 to %d.\r\n", MAX_PC_RACE - 1 );
         return;
      }
      victim->race = value;
      ch_printf( ch, "Victim's race set to %s.\r\n", dis_race_name( victim->race ) );
      return;
   }

   if( !str_cmp( arg2, "armor" ) )
   {
      victim->armor = URANGE( 0, value, 1000 );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc level", ch );
         return;
      }

      victim->level = URANGE( 1, value, MAX_LEVEL );
      return;
   }

   if( !str_cmp( arg2, "numattacks" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc numattacks", ch );
         return;
      }

      victim->numattacks = URANGE( 0, value, 20 );
      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      set_gold( victim, value );
      return;
   }

   if( !str_cmp( arg2, "hitroll" ) )
   {
      victim->hitroll = URANGE( 0, value, 85 );
      return;
   }

   if( !str_cmp( arg2, "damroll" ) )
   {
      victim->damroll = URANGE( 0, value, 65 );
      return;
   }

   if( !str_cmp( arg2, "hp" ) )
   {
      victim->max_hit = UMAX( 1, value );
      return;
   }

   if( !str_cmp( arg2, "mana" ) || !str_cmp( arg2, "blood" ) )
   {
      victim->max_mana = UMAX( 0, value );
      return;
   }

   if( !str_cmp( arg2, "move" ) )
   {
      victim->max_move = UMAX( 0, value );
      return;
   }

   if( !str_cmp( arg2, "practice" ) )
   {
      victim->practice = URANGE( 0, value, 100 );
      return;
   }

   if( !str_cmp( arg2, "align" ) )
   {
      victim->alignment = URANGE( -1000, value, 1000 );
      return;
   }

   if( !str_cmp( arg2, "questplus" ) )
   {
      if( is_npc( victim ) )
      {
         progbug( "MpMset: can't set npc qp", ch );
         return;
      }

      if( value < 0 || value > 200 )
      {
         progbug_printf( ch, "MpMset: Invalid questplus value (%d)", value );
         return;
      }
      log_printf( "%s raising glory of %s by %d ...", ch->name, victim->name, value );
      victim->pcdata->quest_curr += value;
      return;
   }

   if( !str_cmp( arg2, "favor" ) )
   {
      if( is_npc( victim ) )
      {
         progbug( "MpMset: can't set npc favor", ch );
         return;
      }

      victim->pcdata->favor = URANGE( -2500, value, 2500 );
      return;
   }

   if( !str_cmp( arg2, "mentalstate" ) )
   {
      victim->mental_state = URANGE( -100, value, 100 );
      return;
   }

   if( !str_cmp( arg2, "thirst" ) )
   {
      if( is_npc( victim ) )
      {
         progbug( "MpMset: can't set npc thirst", ch );
         return;
      }
      victim->pcdata->condition[COND_THIRST] = URANGE( 0, value, 100 );
      return;
   }

   if( !str_cmp( arg2, "drunk" ) )
   {
      if( is_npc( victim ) )
      {
         progbug( "MpMset: can't set npc drunk", ch );
         return;
      }
      victim->pcdata->condition[COND_DRUNK] = URANGE( 0, value, 100 );
      return;
   }

   if( !str_cmp( arg2, "full" ) )
   {
      if( is_npc( victim ) )
      {
         progbug( "MpMset: can't set npc full", ch );
         return;
      }
      victim->pcdata->condition[COND_FULL] = URANGE( 0, value, 100 );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc name", ch );
         return;
      }

      STRSET( victim->name, arg3 );
      return;
   }

   if( !str_cmp( arg2, "deity" ) )
   {
      DEITY_DATA *deity;

      if( is_npc( victim ) )
      {
         progbug( "MpMset: can't set npc deity", ch );
         return;
      }

      if( arg3 == NULL || arg3[0] == '\0' )
      {
         victim->pcdata->deity = NULL;
         return;
      }

      if( !( deity = get_deity( arg3 ) ) )
      {
         progbug( "MpMset: Invalid deity", ch );
         return;
      }
      victim->pcdata->deity = deity;
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      STRSET( victim->short_descr, arg3 );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      mudstrlcpy( buf, arg3, sizeof( buf ) );
      mudstrlcat( buf, "\r\n", sizeof( buf ) );
      STRSET( victim->long_descr, buf );
      return;
   }

   if( !str_cmp( arg2, "title" ) )
   {
      if( is_npc( victim ) )
      {
         progbug( "MpMset: can't set npc title", ch );
         return;
      }

      set_title( victim, arg3 );
      return;
   }

   if( !str_cmp( arg2, "spec" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc spec", ch );
         return;
      }

      if( !str_cmp( arg3, "none" ) )
      {
         victim->spec_fun = NULL;
         return;
      }

      if( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
      {
         progbug( "MpMset: Invalid spec", ch );
         return;
      }
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc flags", ch );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no flags", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, act_flags, ACT_MAX );
         if( value < 0 || value >= ACT_MAX )
            progbug_printf( ch, "MpMset: Invalid flag (%s)", arg3 );
         else
         {
            if( value == ACT_PROTOTYPE )
               progbug( "MpMset: can't set prototype flag", ch );
            else if( value == ACT_IS_NPC )
               progbug( "MpMset: can't remove npc flag", ch );
            else
               xTOGGLE_BIT( victim->act, value );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't modify pc affected", ch );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no affected", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, a_flags, AFF_MAX );
         if( value < 0 || value >= AFF_MAX )
            progbug_printf( ch, "MpMset: Invalid affected (%s)", arg3 );
         else
            xTOGGLE_BIT( victim->affected_by, value );
      }
      return;
   }

   if( !str_cmp( arg2, "resistant" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc resistant", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no resistant", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      value = get_flag( arg3, ris_flags, RIS_MAX );
      if( value < 0 || value >= RIS_MAX )
         progbug_printf( ch, "MpMset: Invalid resistant (%s)", arg3 );
      else
         victim->resistant[value] = atoi( argument );
      return;
   }

   if( !str_cmp( arg2, "part" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc part", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no part", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, part_flags, PART_MAX );
         if( value < 0 || value >= PART_MAX )
            progbug_printf( ch, "MpMset: Invalid part (%s)", arg3 );
         else
            xTOGGLE_BIT( victim->xflags, value );
      }
      return;
   }

   if( !str_cmp( arg2, "attack" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc attack", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no attack", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, attack_flags, ATCK_MAX );
         if( value < 0 || value >= ATCK_MAX )
            progbug_printf( ch, "MpMset: Invalid attack (%s)", arg3 );
         else
            xTOGGLE_BIT( victim->attacks, value );
      }
      return;
   }

   if( !str_cmp( arg2, "defense" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc defense", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no defense", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, defense_flags, DFND_MAX );
         if( value < 0 || value >= DFND_MAX )
            progbug_printf( ch, "MpMset: Invalid defense (%s)", arg3 );
         else
            xTOGGLE_BIT( victim->defenses, value );
      }
      return;
   }

   if( !str_cmp( arg2, "pos" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc pos", ch );
         return;
      }
      if( value < 0 || value > POS_STANDING )
      {
         progbug( "MpMset: Invalid pos", ch );
         return;
      }
      victim->position = value;
      return;
   }

   if( !str_cmp( arg2, "defpos" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc defpos", ch );
         return;
      }
      if( value < 0 || value > POS_STANDING )
      {
         progbug( "MpMset: Invalid defpos", ch );
         return;
      }
      victim->defposition = value;
      return;
   }

   if( !str_cmp( arg2, "speaks" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no speaks", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_langflag( arg3 );
         v2 = get_langnum( arg3 );
         if( value == LANG_UNKNOWN )
            progbug( "MpMset: Invalid speaks", ch );
         else if( !is_npc( victim ) )
         {
            if( !( value &= VALID_LANGS ) )
            {
               progbug_printf( ch, "MpMset: speaks: Invalid player language (%s)", arg3 );
               continue;
            }
            if( v2 == -1 )
               progbug_printf( ch, "MpMset: speaks: Unknown language (%s)", arg3 );
            else
               xTOGGLE_BIT( victim->speaks, v2 );
         }
         else
         {
            if( v2 == -1 )
               progbug_printf( ch, "MpMset: speaks: Unknown language (%s)", arg3 );
            else
               xTOGGLE_BIT( victim->speaks, v2 );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "speaking" ) )
   {
      if( !is_npc( victim ) )
      {
         progbug( "MpMset: can't set pc speaking", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpMset: no speaking", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_langflag( arg3 );
         if( value == LANG_UNKNOWN )
            progbug( "MpMset: Invalid speaking", ch );
         else
         {
            v2 = get_langnum( arg3 );
            if( v2 == -1 )
               progbug_printf( ch, "MpMset: speaking: Unknown language (%s)", arg3 );
            else
               xTOGGLE_BIT( victim->speaks, v2 );
         }
      }
      return;
   }

   progbug_printf( ch, "MpMset: Invalid field (%s)", arg2 );
}

CMDF( do_mposet )
{
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MSL], arg4[MIL];
   OBJ_DATA *obj;
   int value, tmp;

   /* A desc means switched.. too many loopholes if we allow that.. */
   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   mudstrlcpy( arg3, argument, sizeof( arg3 ) );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      progbug( "MpOset: no object argument", ch );
      return;
   }

   if( !( obj = get_obj_here( ch, arg1 ) ) )
   {
      progbug_printf( ch, "MpOset: object (%s) not here", arg1 );
      return;
   }

   if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
   {
      progbug_printf( ch, "MpOset: object (%s) is a prototype", arg1 );
      return;
   }

   separate_obj( obj );
   value = atoi( arg3 );

   if( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
   {
      if( is_number( arg3 ) )
         obj->value[0] = value;
      else
      {
         argument = one_argument( argument, arg3 );
         argument = one_argument( argument, arg4 );
         value = atoi( arg4 );
         if( !str_cmp( arg3, "+" ) )
            obj->value[0] += value;
         else if( !str_cmp( arg3, "-" ) )
            obj->value[0] -= value;
         else
            progbug_printf( ch, "MpOset: %s specified instead of a + or - for v0.", arg3 );
      }
      return;
   }

   if( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
   {
      if( is_number( arg3 ) )
         obj->value[1] = value;
      else
      {
         argument = one_argument( argument, arg3 );
         argument = one_argument( argument, arg4 );
         value = atoi( arg4 );
         if( !str_cmp( arg3, "+" ) )
            obj->value[1] += value;
         else if( !str_cmp( arg3, "-" ) )
            obj->value[1] -= value;
         else
            progbug_printf( ch, "MpOset: %s specified instead of a + or - for v1.", arg3 );
      }
      return;
   }

   if( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
   {
      if( is_number( arg3 ) )
         obj->value[2] = value;
      else
      {
         argument = one_argument( argument, arg3 );
         argument = one_argument( argument, arg4 );
         value = atoi( arg4 );
         if( !str_cmp( arg3, "+" ) )
            obj->value[2] += value;
         else if( !str_cmp( arg3, "-" ) )
            obj->value[2] -= value;
         else
            progbug_printf( ch, "MpOset: %s specified instead of a + or - for v2.", arg3 );
      }
      return;
   }

   if( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
   {
      if( is_number( arg3 ) )
         obj->value[3] = value;
      else
      {
         argument = one_argument( argument, arg3 );
         argument = one_argument( argument, arg4 );
         value = atoi( arg4 );
         if( !str_cmp( arg3, "+" ) )
            obj->value[3] += value;
         else if( !str_cmp( arg3, "-" ) )
            obj->value[3] -= value;
         else
            progbug_printf( ch, "MpOset: %s specified instead of a + or - for v3.", arg3 );
      }
      return;
   }

   if( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
   {
      if( is_number( arg3 ) )
         obj->value[4] = value;
      else
      {
         argument = one_argument( argument, arg3 );
         argument = one_argument( argument, arg4 );
         value = atoi( arg4 );
         if( !str_cmp( arg3, "+" ) )
            obj->value[4] += value;
         else if( !str_cmp( arg3, "-" ) )
            obj->value[4] -= value;
         else
            progbug_printf( ch, "MpOset: %s specified instead of a + or - for v4.", arg3 );
      }
      return;
   }

   if( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) )
   {
      if( is_number( arg3 ) )
         obj->value[5] = value;
      else
      {
         argument = one_argument( argument, arg3 );
         argument = one_argument( argument, arg4 );
         value = atoi( arg4 );
         if( !str_cmp( arg3, "+" ) )
            obj->value[5] += value;
         else if( !str_cmp( arg3, "-" ) )
            obj->value[5] -= value;
         else
            progbug_printf( ch, "MpOset: %s specified instead of a + or - for v5.", arg3 );
      }
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpOset: no type argument", ch );
         return;
      }
      value = get_flag( argument, o_types, ITEM_TYPE_MAX );
      if( value < 0 && is_number( argument ) )
         value = atoi( argument );
      if( value < 1 || value >= ITEM_TYPE_MAX )
      {
         progbug_printf( ch, "MpOset: Invalid type (%s)", argument );
         return;
      }
      obj->item_type = ( short )value;
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpOset: no flags", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, o_flags, ITEM_MAX );
         if( value < 0 || value >= ITEM_MAX )
            progbug_printf( ch, "MpOset: Invalid flag (%s)", arg3 );
         else
         {
            if( value == ITEM_PROTOTYPE )
               progbug( "MpOset: can't set prototype flag", ch );
            else
               xTOGGLE_BIT( obj->extra_flags, value );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "wear" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpOset: no wear", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, w_flags, ITEM_WEAR_MAX );
         if( value < 0 || value >= ITEM_WEAR_MAX )
            progbug_printf( ch, "MpOset: Invalid wear (%s)", arg3 );
         else
            xTOGGLE_BIT( obj->wear_flags, value );
      }
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      obj->level = URANGE( 1, value, MAX_LEVEL );
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      obj->weight = UMAX( 0, value );
      return;
   }

   if( !str_cmp( arg2, "cost" ) )
   {
      obj->cost = UMAX( 0, value );
      return;
   }

   if( !str_cmp( arg2, "timer" ) )
   {
      obj->timer = UMAX( -1, value );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( arg3 == NULL || arg3[0] == '\0' )
      {
         progbug( "MpOset: trying to set name to nothing.", ch );
         return;
      }
      STRSET( obj->name, arg3 );
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      if( arg3 == NULL || arg3[0] == '\0' )
      {
         progbug( "MpOset: trying to set short to nothing.", ch );
         return;
      }
      STRSET( obj->short_descr, arg3 );
      if( str_infix( "mprename", obj->name ) )
      {
         snprintf( buf, sizeof( buf ), "%s %s", obj->name, "mprename" );
         STRSET( obj->name, buf );
      }
      if( obj == supermob_obj )
         STRSET( supermob->short_descr, obj->short_descr );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      if( arg3 == NULL || arg3[0] == '\0' )
      {
         progbug( "MpOset: trying to set long to nothing.", ch );
         return;
      }
      STRSET( obj->description, buf );
      return;
   }

   if( !str_cmp( arg2, "actiondesc" ) )
   {
      if( strstr( arg3, "%n" ) || strstr( arg3, "%d" ) || strstr( arg3, "%l" ) )
      {
         progbug( "MpOset: Illegal actiondesc", ch );
         return;
      }
      STRSET( obj->action_desc, arg3 );
      return;
   }

   if( !str_cmp( arg2, "affect" ) )
   {
      AFFECT_DATA *paf;
      short loc;
      int bitv = -1;

      argument = one_argument( argument, arg2 );
      if( arg2[0] == '\0' || !argument || argument[0] == 0 )
      {
         progbug( "MpOset: Bad affect Usage", ch );
         return;
      }
      loc = get_flag( arg2, a_types, APPLY_MAX );
      if( loc < 1 || loc >= APPLY_MAX )
      {
         progbug( "MpOset: Invalid affect field", ch );
         return;
      }

      argument = one_argument( argument, arg3 );
      if( loc == APPLY_EXT_AFFECT )
      {
         value = get_flag( arg3, a_flags, AFF_MAX );
         if( value < 0 || value >= AFF_MAX )
         {
            progbug_printf( ch, "MpOset: bad affected (%s)", arg3 );
            return;
         }
      }
      else if( loc == APPLY_STAT )
      {
         bitv = get_flag( arg3, stattypes, STAT_MAX );
         if( bitv < 0 || bitv >= STAT_MAX )
         {
            progbug_printf( ch, "MpOset: Unknown stat: %s\r\n", arg3 );
            return;
         }
         argument = one_argument( argument, arg3 );
         value = atoi( arg3 );
      }
      else if( loc == APPLY_RESISTANT )
      {
         bitv = get_flag( arg3, ris_flags, RIS_MAX );
         if( bitv < 0 || bitv >= RIS_MAX )
         {
            progbug_printf( ch, "MpOset: Unknown ris: %s\r\n", arg3 );
            return;
         }
         argument = one_argument( argument, arg3 );
         value = atoi( arg3 );
      }
      else
         value = atoi( arg3 );
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = loc;
      paf->modifier = value;
      xCLEAR_BITS( paf->bitvector );
      if( loc == APPLY_STAT )
         xSET_BIT( paf->bitvector, bitv );
      paf->next = NULL;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      ++top_affect;
      return;
   }

   if( !str_cmp( arg2, "rmaffect" ) )
   {
      AFFECT_DATA *paf;
      short loc, count;

      if( !argument || argument[0] == '\0' )
      {
         progbug( "MpOset: no rmaffect", ch );
         return;
      }
      loc = atoi( argument );
      if( loc < 1 )
      {
         progbug( "MpOset: Invalid rmaffect", ch );
         return;
      }

      count = 0;

      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ++count == loc )
         {
            UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
            DISPOSE( paf );
            send_to_char( "Removed.\r\n", ch );
            --top_affect;
            return;
         }
      }
      progbug( "MpOset: rmaffect not found", ch );
      return;
   }

   /* Make it easier to set special object values by name than number -Thoric */
   tmp = -1;
   switch( obj->item_type )
   {
      case ITEM_WEAPON:
         if( !str_cmp( arg2, "weapontype" ) )
         {
            unsigned int x;

            value = -1;
            for( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               progbug( "MpOset: Invalid weapon type", ch );
               return;
            }
            tmp = 3;
            break;
         }
         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         break;

      case ITEM_ARMOR:
         if( !str_cmp( arg2, "condition" ) )
            tmp = 3;
         if( !str_cmp( arg2, "ac" ) )
            tmp = 1;
         break;

      case ITEM_SALVE:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "maxdoses" ) )
            tmp = 1;
         if( !str_cmp( arg2, "doses" ) )
            tmp = 2;
         if( !str_cmp( arg2, "delay" ) )
            tmp = 3;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 4;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 5;
         if( tmp >= 4 && tmp <= 5 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 1;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell3" ) )
            tmp = 3;
         if( tmp >= 1 && tmp <= 3 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell" ) )
         {
            tmp = 3;
            value = skill_lookup( arg3 );
         }
         if( !str_cmp( arg2, "maxcharges" ) )
            tmp = 1;
         if( !str_cmp( arg2, "charges" ) )
            tmp = 2;
         break;

      case ITEM_CONTAINER:
         if( !str_cmp( arg2, "capacity" ) )
            tmp = 0;
         if( !str_cmp( arg2, "cflags" ) )
            tmp = 1;
         if( !str_cmp( arg2, "key" ) )
            tmp = 2;
         break;

      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         if( !str_cmp( arg2, "tflags" ) )
         {
            tmp = 0;
            value = get_trigflag( arg3 );
         }
         break;
   }
   if( tmp >= 0 && tmp <= 3 )
   {
      obj->value[tmp] = value;
      return;
   }

   progbug_printf( ch, "MpOset: Invalid field (%s)", arg2 );
}
