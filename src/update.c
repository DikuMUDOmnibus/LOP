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
 *			      Regular update module			     *
 *****************************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "h/mud.h"
#include "h/trivia.h"

void friend_update( CHAR_DATA *ch );
bool is_same_cords( OBJ_DATA *obj, CHAR_DATA *ch );
void found_prey( CHAR_DATA *ch, CHAR_DATA *victim );
void tele_update( void );
void trivia_update( void );
void room_act_update( void );
void mprog_random_trigger( CHAR_DATA *mob );
void oprog_random_trigger( OBJ_DATA *obj );
void rprog_random_trigger( CHAR_DATA *ch );
void hunt_victim( CHAR_DATA *ch );
void better_mental_state( CHAR_DATA *ch, int mod );
void mprog_time_trigger( CHAR_DATA *mob );
void mprog_hour_trigger( CHAR_DATA *mob );
void rprog_time_trigger( ROOM_INDEX_DATA *room );
void rprog_hour_trigger( ROOM_INDEX_DATA *room );
void drunk_randoms( CHAR_DATA *ch );
void hallucinations( CHAR_DATA *ch );
void quest_update( void );
void time_update( void );
void send_hint( CHAR_DATA *ch );
void mprog_script_trigger( CHAR_DATA *mob );
void mprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type );
void pfile_cleanup( void );
void obj_act_update( void );

/* Global Variables */
CHAR_DATA *gch_prev;
OBJ_DATA *gobj_prev;

CHAR_DATA *timechar;

const char *corpse_descs[] =
{
   "The corpse of %s is in the last stages of decay.",
   "The corpse of %s is crawling with vermin.",
   "The corpse of %s fills the air with a foul stench.",
   "The corpse of %s is buzzing with flies.",
   "The corpse of %s lies here."
};

void stat_gain( CHAR_DATA *ch )
{
   int hitgain = 0, managain = 0, stamgain = 0;

   if( !is_npc( ch ) && ch->position >= POS_SLEEPING ) /* Handled by pc_regen */
      return;

   hitgain = ( ( ch->level * 3 ) / 2 );
   managain = ch->level;
   stamgain = ch->level;

   if( IS_AFFECTED( ch, AFF_POISON ) )
   {
      hitgain /= 4;
      managain /= 4;
      stamgain /= 4;
   }

   if( ch->position < POS_SLEEPING )
   {
      managain = 0;
      stamgain = 0;
      switch( ch->position )
      {
         case POS_DEAD:
            hitgain = 0;
            break;

         case POS_MORTAL:
         case POS_INCAP:
            hitgain = -1;
            break;

         case POS_STUNNED:
            hitgain = 1;
            break;
      }
   }

   hitgain = UMIN( hitgain, ( ch->max_hit - ch->hit ) );
   ch->hit += hitgain;

   managain = UMIN( managain, ( ch->max_mana - ch->mana ) );
   ch->mana += managain;

   stamgain = UMIN( stamgain, ( ch->max_move - ch->move ) );
   ch->move += stamgain;
}

/* Sometime make it so spells/skills can affect the time */
void pc_regen( void )
{
   CHAR_DATA *ch;
   PC_DATA *player;
   int gain = 0, tmpgain, nextregen;

   for( player = first_pc; player; player = player->next )
   {
      if( !( ch = player->character ) || is_npc( ch ) )
         continue;
      if( --player->nextregen > 0 )
         continue;
      if( ch->fighting || ch->position < POS_SLEEPING )
         continue;

      gain = 1;

      switch( ch->position )
      {
         default:
            nextregen = 22;
            break;

         case POS_SLEEPING:
            gain += 2;
            nextregen = 18;
            break;

         case POS_RESTING:
            gain += 1;
            nextregen = 20;
            break;
      }

      if( is_vampire( ch ) )
      {
         if( ch->mana <= 1 )
         {
            gain /= 2;
            nextregen += 2;
         }
         else if( ch->mana >= ( ch->max_mana / 2 ) )
         {
            gain += 2;
            nextregen -= 2;
         }
         if( is_outside( ch ) )
         {
            switch( time_info.sunlight )
            {
               case SUN_RISE:
               case SUN_SET:
                  gain /= 2;
                  nextregen += 2;
                  break;

               case SUN_LIGHT:
                  gain /= 4;
                  nextregen += 4;
                  break;
            }
         }
      }

      if( ch->pcdata->condition[COND_FULL] <= 0 )
      {
         gain /= 2;
         nextregen += 2;
      }

      if( ch->pcdata->condition[COND_THIRST] <= 0 )
      {
         gain /= 2;
         nextregen += 2;
      }

      if( IS_AFFECTED( ch, AFF_POISON ) )
      {
         gain /= 4;
         nextregen += 4;
      }

      player->nextregen = nextregen;

      tmpgain = 0;
      if( get_curr_con( ch ) >= ch->level )
         tmpgain += 1;
      if( ch->hit < ch->max_hit )
         ch->hit += UMIN( ( gain + tmpgain ), ( ch->max_hit - ch->hit ) );

      tmpgain = 0;
      if( get_curr_int( ch ) >= ch->level )
         tmpgain += 1;
      if( ch->mana < ch->max_mana )
         if( !is_vampire( ch ) ) /* Vampires have to feed for their blood */
            ch->mana += UMIN( ( gain + tmpgain ), ( ch->max_mana - ch->mana ) );

      tmpgain = 0;
      if( get_curr_dex( ch ) >= ch->level )
         tmpgain += 1;
      if( ch->move < ch->max_move )
         ch->move += UMIN( ( gain + tmpgain ), ( ch->max_move - ch->move ) );
   }
}

void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
   int condition;

   if( value == 0 || is_npc( ch ) || get_trust( ch ) >= PERM_IMM )
      return;

   condition = ch->pcdata->condition[iCond];
   ch->pcdata->condition[iCond] = URANGE( 0, condition + value, 50 );
   if( ch->level < MAX_LEVEL )
   {
      switch( iCond )
      {
         case COND_FULL:
            if( condition < ch->pcdata->condition[iCond] )
            {
               if( ch->pcdata->condition[iCond] == 50 )
                  act( AT_THIRSTY, "Your stomach has reached its capacity.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] >= 45 && ch->pcdata->condition[iCond] < 50 )
                  act( AT_THIRSTY, "Your stomach is nearing its capacity.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] > 0 && ch->pcdata->condition[iCond] <= 10 )
                  act( AT_THIRSTY, "You're still peckish.", ch, NULL, NULL, TO_CHAR );
            }
            if( condition > ch->pcdata->condition[iCond] )
            {
               if( ch->pcdata->condition[iCond] > 5 && ch->pcdata->condition[iCond] <= 10 )
                  act( AT_HUNGRY, "You're a mite peckish.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] > 3 && ch->pcdata->condition[iCond] <= 5 )
                  act( AT_HUNGRY, "You're hungry.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] > 0 && ch->pcdata->condition[iCond] <= 3 )
               {
                  act( AT_HUNGRY, "You're really hungry.", ch, NULL, NULL, TO_CHAR );
                  act( AT_HUNGRY, "You can hear $n's stomach growling.", ch, NULL, NULL, TO_ROOM );
                  if( number_bits( 1 ) == 0 )
                     worsen_mental_state( ch, 1 );
               }               
            }
            if( ch->pcdata->condition[iCond] == 0 )
            {
               act( AT_HUNGRY, "You're STARVING!", ch, NULL, NULL, TO_CHAR );
               act( AT_HUNGRY, "$n is starved half to death!", ch, NULL, NULL, TO_ROOM );
               if( number_bits( 1 ) == 0 )
                  worsen_mental_state( ch, 1 );
               damage( ch, ch, NULL, 1, TYPE_UNDEFINED, false );
            }
            break;

         case COND_THIRST:
            if( condition < ch->pcdata->condition[iCond] )
            {
               if( ch->pcdata->condition[iCond] == 50 )
                  act( AT_THIRSTY, "Your stomach has reached its capacity.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] >= 45 && ch->pcdata->condition[iCond] < 50 )
                  act( AT_THIRSTY, "Your stomach is nearing its capacity.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] > 0 && ch->pcdata->condition[iCond] <= 10 )
                  act( AT_THIRSTY, "You could still use a sip of something refreshing.", ch, NULL, NULL, TO_CHAR );
            }
            if( condition > ch->pcdata->condition[iCond] )
            {
               if( ch->pcdata->condition[iCond] > 5 && ch->pcdata->condition[iCond] <= 10 )
                  act( AT_THIRSTY, "You could use a sip of something refreshing.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] > 3 && ch->pcdata->condition[iCond] <= 5 )
                  act( AT_THIRSTY, "You're thirsty.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] > 0 && ch->pcdata->condition[iCond] <= 3 )
               {
                  act( AT_THIRSTY, "You're really thirsty.", ch, NULL, NULL, TO_CHAR );
                  act( AT_THIRSTY, "$n looks a little parched.", ch, NULL, NULL, TO_ROOM );
                  if( number_bits( 1 ) == 0 )
                     worsen_mental_state( ch, 1 );
               }
            }
            if( ch->pcdata->condition[iCond] == 0 )
            {
               act( AT_THIRSTY, "You're DYING of THIRST!", ch, NULL, NULL, TO_CHAR );
               act( AT_THIRSTY, "$n is dying of thirst!", ch, NULL, NULL, TO_ROOM );
               if( number_bits( 1 ) == 0 )
                  worsen_mental_state( ch, 1 );
               damage( ch, ch, NULL, 1, TYPE_UNDEFINED, false );
            }
            break;

         case COND_DRUNK:
            if( condition > ch->pcdata->condition[iCond] )
            {
               if( ch->pcdata->condition[iCond] >= 1 && ch->pcdata->condition[iCond] <= 5 )
                  act( AT_SOBER, "You're feeling a little less light headed.", ch, NULL, NULL, TO_CHAR );
               if( ch->pcdata->condition[iCond] == 0 )
                  act( AT_SOBER, "You're sober.", ch, NULL, NULL, TO_CHAR );
            }
            if( condition < ch->pcdata->condition[iCond] )
            {
               if( ch->pcdata->condition[iCond] == 1 )
                  act( AT_SOBER, "You're feeling light headed.", ch, NULL, NULL, TO_CHAR );
            }
            break;

         default:
            bug( "%s: invalid condition type %d", __FUNCTION__, iCond );
            break;
      }
   }
}

/*
 * Put this in a seperate function so it isn't called three times per tick
 * This was added after a suggestion from Cronel	--Shaddai
 */
void check_alignment( CHAR_DATA *ch )
{
   /* Race alignment restrictions */
   if( ch->alignment < race_table[ch->race]->minalign || ch->alignment > race_table[ch->race]->maxalign )
   {
      set_char_color( AT_BLOOD, ch );
      send_to_char( "Your actions have been incompatible with the ideals of your race.", ch );
      worsen_mental_state( ch, 6 );
   }
}

/* Update all the rooms */
void room_update( void )
{
   ROOM_INDEX_DATA *pRoomIndex;
   int iHash;

   for( iHash = 0; iHash < MKH; iHash++ )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         rprog_hour_trigger( pRoomIndex );
         rprog_time_trigger( pRoomIndex );
      }
   }
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Mud cpu time.
 */
void mobile_update( void )
{
   CHAR_DATA *ch;
   EXIT_DATA *pexit;
   char buf[MSL];
   int door;
   ch_ret retcode;

   retcode = rNONE;

   /* Examine all characters. */
   for( ch = last_char; ch; ch = gch_prev )
   {
      set_cur_char( ch );
      gch_prev = ch->prev;

      if( !is_npc( ch ) )
      {
         drunk_randoms( ch );
         hallucinations( ch );
         continue;
      }

      /* Never know might want to allow pets to have special functions */
      if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) && xIS_SET( ch->act, ACT_PET ) && ch->spec_fun )
      {
         if( ( *ch->spec_fun ) ( ch ) )
            continue;
         if( char_died( ch ) )
            continue;
      }

      if( !ch->in_room || IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_PARALYSIS ) )
         continue;

      /* Clean up 'animated corpses' that aren't charmed' - Scryn */
      if( ch->pIndexData->vnum == MOB_VNUM_ANIMATED_CORPSE && !IS_AFFECTED( ch, AFF_CHARM ) )
      {
         act( AT_MAGIC, "$n returns to the dust from whence $e came.", ch, NULL, NULL, TO_ROOM );
         extract_char( ch, true );
         continue;
      }

      if( !xIS_SET( ch->act, ACT_RUNNING ) && !xIS_SET( ch->act, ACT_SENTINEL ) && !ch->fighting && ch->first_hunting )
      {
         wait_state( ch, 2 * PULSE_VIOLENCE );
         hunt_victim( ch );
         continue;
      }

      /* Examine call for special procedure */
      if( !xIS_SET( ch->act, ACT_RUNNING ) && ch->spec_fun )
      {
         if( ( *ch->spec_fun ) ( ch ) )
            continue;
         if( char_died( ch ) )
            continue;
      }

      /* If there is no players in the area don't do anything below this */
      if( ch->in_room && ch->in_room->area->nplayer <= 0 )
         continue;

      /* Check for mudprogram script on mob */
      if( HAS_PROG( ch->pIndexData, SCRIPT_PROG ) )
      {
         mprog_script_trigger( ch );
         continue;
      }

      if( ch != cur_char )
      {
         bug( "%s: ch != cur_char after spec_fun", __FUNCTION__ );
         continue;
      }

      /* That's all for sleeping / busy monster */
      if( ch->position != POS_STANDING )
         continue;

      if( xIS_SET( ch->act, ACT_MOUNTED ) )
      {
         if( xIS_SET( ch->act, ACT_AGGRESSIVE ) || xIS_SET( ch->act, ACT_META_AGGR ) )
            do_emote( ch, (char *)"snarls and growls." );
         continue;
      }

      if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE )
      && ( xIS_SET( ch->act, ACT_AGGRESSIVE ) || xIS_SET( ch->act, ACT_META_AGGR ) ) )
         do_emote( ch, (char *)"glares around and snarls." );

      /* MOBprogram random trigger */
      if( ch->in_room->area->nplayer > 0 )
      {
         mprog_random_trigger( ch );
         if( char_died( ch ) )
            continue;
         if( ch->position < POS_STANDING )
            continue;
      }

      /* MOBprogram hour trigger: do something for an hour */
      mprog_hour_trigger( ch );
      if( char_died( ch ) )
         continue;

      if( ch->position < POS_STANDING )
         continue;

      /* Scavenge */
      if( xIS_SET( ch->act, ACT_SCAVENGER ) && ch->in_room->first_content && number_bits( 2 ) == 0 )
      {
         OBJ_DATA *obj, *obj_best = NULL;
         int max = 1;

         for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
         {
            if( is_obj_stat( obj, ITEM_PROTOTYPE ) && !xIS_SET( ch->act, ACT_PROTOTYPE ) )
               continue;
            if( !can_wear( obj, ITEM_NO_TAKE ) && obj->cost > max && !is_obj_stat( obj, ITEM_BURIED ) )
            {
               obj_best = obj;
               max = obj->cost;
            }
         }

         if( obj_best )
         {
            obj_from_room( obj_best );
            obj_to_char( obj_best, ch );
            act( AT_ACTION, "$n gets $p.", ch, obj_best, NULL, TO_ROOM );
         }
      }

      /* Wander */
      if( !xIS_SET( ch->act, ACT_RUNNING ) && !xIS_SET( ch->act, ACT_SENTINEL ) && !xIS_SET( ch->act, ACT_PROTOTYPE ) )
      {
         if( is_in_wilderness( ch ) )
            move_around_wilderness( ch, number_range( 0, 7 ), ch->in_room->description );
         else if( ( door = number_bits( 5 ) ) <= 9
         && ( pexit = get_exit( ch->in_room, door ) )
         && pexit->to_room
         && !xIS_SET( pexit->exit_info, EX_CLOSED )
         && !xIS_SET( pexit->exit_info, EX_WINDOW )
         && !xIS_SET( pexit->to_room->room_flags, ROOM_NO_MOB )
         && !xIS_SET( pexit->to_room->room_flags, ROOM_DEATH )
         && ( !xIS_SET( ch->act, ACT_STAY_AREA ) || pexit->to_room->area == ch->in_room->area ) )
         {
            retcode = move_char( ch, pexit, 0, false );
            if( char_died( ch ) )
               continue;
            if( retcode == rNONE || retcode == rSTOP || xIS_SET( ch->act, ACT_SENTINEL ) || ch->position < POS_STANDING )
               continue;
         }
      }

      /* Flee */
      if( ch->hit < ( ch->max_hit / 2 )
      && ( door = number_bits( 4 ) ) <= 9
      && ( pexit = get_exit( ch->in_room, door ) )
      && pexit->to_room
      && !xIS_SET( pexit->exit_info, EX_WINDOW )
      && !xIS_SET( pexit->exit_info, EX_CLOSED )
      && !xIS_SET( pexit->to_room->room_flags, ROOM_NO_MOB )
      && !xIS_SET( pexit->to_room->room_flags, ROOM_DEATH ) )
      {
         CHAR_DATA *rch;
         bool found;

         found = false;
         for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
         {
            if( is_fearing( ch, rch ) )
            {
               switch( number_bits( 2 ) )
               {
                  case 0:
                     snprintf( buf, sizeof( buf ), "yell Get away from me, %s!", rch->name );
                     break;
                  case 1:
                     snprintf( buf, sizeof( buf ), "yell Leave me be, %s!", rch->name );
                     break;
                  case 2:
                     snprintf( buf, sizeof( buf ), "yell %s is trying to kill me!  Help!", rch->name );
                     break;
                  case 3:
                     snprintf( buf, sizeof( buf ), "yell Someone save me from %s!", rch->name );
                     break;
               }
               interpret( ch, buf );
               found = true;
               break;
            }
         }
         if( found )
            retcode = move_char( ch, pexit, 0, false );
      }
   }
}

/* Update all chars, including mobs. This function is performance sensitive. */
void char_update( void )
{
   CHAR_DATA *ch, *ch_save;
   short save_count = 0;

   ch_save = NULL;
   for( ch = last_char; ch; ch = gch_prev )
   {
      gch_prev = ch->prev;
      set_cur_char( ch );

      if( !is_npc( ch ) )
      {
         rprog_random_trigger( ch );
         if( char_died( ch ) )
            continue;
      }

      if( is_npc( ch ) )
      {
         mprog_time_trigger( ch );
         if( char_died( ch ) )
            continue;
      }

      if( !is_npc( ch ) )
      {
         friend_update( ch );
         if( !xIS_SET( ch->act, PLR_NOHINTS ) )
            send_hint( ch );
         check_auction( ch );
      }

      /* See if player should be auto-saved. */
      if( !is_npc( ch )
      && ( !ch->desc || ch->desc->connected == CON_PLAYING )
      && ch->level >= 2 && ( current_time - ch->save_time ) > ( sysdata.save_frequency * 60 ) )
         ch_save = ch;
      else
         ch_save = NULL;

      if( ch->position >= POS_STUNNED )
         stat_gain( ch );

      if( ch->position == POS_STUNNED )
         update_pos( ch );

      /* Morph timer expires */
      if( ch->morph && ch->morph->timer > 0 )
      {
         if( --ch->morph->timer <= 0 )
            do_unmorph_char( ch );
      }

      if( !is_npc( ch ) )
      {
         /* Handle a timer for mortals and link dead immortals */
         if( get_trust( ch ) < PERM_IMM || !ch->desc )
         {
            if( ++ch->timer >= 12 )
            {
               if( !is_idle( ch ) )
               {
                  if( ch->fighting )
                     stop_fighting( ch, true );
                  act( AT_ACTION, "$n disappears into the void.", ch, NULL, NULL, TO_ROOM );
                  send_to_char( "You disappear into the void.\r\n", ch );
                  if( xIS_SET( sysdata.save_flags, SV_IDLE ) )
                     save_char_obj( ch );
                  xSET_BIT( ch->pcdata->flags, PCFLAG_IDLE );
                  char_from_room( ch );
                  char_to_room( ch, get_room_index( sysdata.room_limbo ) );
               }
            }
         }
      }

      if( !is_npc( ch ) && get_trust( ch ) < PERM_IMM )
      {
         OBJ_DATA *obj;

         for( obj = ch->first_carrying; obj; obj = obj->next_content )
         {
            if( obj->wear_loc != WEAR_NONE && is_obj_stat( obj, ITEM_LODGED ) )
            {
               int dam;

               dam = number_range( ( 2 * obj->value[1] ), ( 2 * obj->value[2] ) );
               act( AT_CARNAGE, "$n suffers damage from $p being stuck in $m.", ch, obj, NULL, TO_ROOM );
               act( AT_CARNAGE, "You suffer damage from $p being stuck in you.", ch, obj, NULL, TO_CHAR );
               damage( ch, ch, NULL, dam, TYPE_UNDEFINED, false );
               if( char_died( ch ) )
                  continue;
            }
            if( obj->wear_loc != WEAR_NONE && obj->item_type == ITEM_LIGHT && obj->value[2] > 0 )
            {
               if( --obj->value[2] == 0 && ch->in_room )
               {
                  ch->in_room->light -= obj->count;
                  if( ch->in_room->light < 0 )
                     ch->in_room->light = 0;
                  act( AT_ACTION, "$p goes out.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "$p goes out.", ch, obj, NULL, TO_CHAR );
               }
            }
         }

         if( ch->pcdata->condition[COND_DRUNK] > 8 )
            worsen_mental_state( ch, ch->pcdata->condition[COND_DRUNK] / 8 );
         if( ch->pcdata->condition[COND_FULL] > 1 )
         {
            switch( ch->position )
            {
               case POS_SLEEPING:
                  better_mental_state( ch, 4 );
                  break;

               case POS_RESTING:
                  better_mental_state( ch, 3 );
                  break;

               case POS_SITTING:
               case POS_MOUNTED:
                  better_mental_state( ch, 2 );
                  break;

               case POS_STANDING:
                  better_mental_state( ch, 1 );
                  break;

               case POS_FIGHTING:
               case POS_EVASIVE:
               case POS_DEFENSIVE:
               case POS_AGGRESSIVE:
               case POS_BERSERK:
                  if( number_bits( 2 ) == 0 )
                     better_mental_state( ch, 1 );
                  break;
            }
         }

         if( ch->pcdata->condition[COND_THIRST] > 1 )
         {
            switch( ch->position )
            {
               case POS_SLEEPING:
                  better_mental_state( ch, 5 );
                  break;

               case POS_RESTING:
                  better_mental_state( ch, 3 );
                  break;

               case POS_SITTING:
               case POS_MOUNTED:
                  better_mental_state( ch, 2 );
                  break;

               case POS_STANDING:
                  better_mental_state( ch, 1 );
                  break;

               case POS_FIGHTING:
               case POS_EVASIVE:
               case POS_DEFENSIVE:
               case POS_AGGRESSIVE:
               case POS_BERSERK:
                  if( number_bits( 2 ) == 0 )
                     better_mental_state( ch, 1 );
                  break;
            }
         }

         /* Function added on suggestion from Cronel */
         check_alignment( ch );
         gain_condition( ch, COND_DRUNK, -1 );
         gain_condition( ch, COND_FULL, -1 + race_table[ch->race]->hunger_mod );

         if( ch->in_room )
         {
            if( ch->in_room->sector_type == SECT_DESERT )
               gain_condition( ch, COND_THIRST, -3 + race_table[ch->race]->thirst_mod );
            else if( ch->in_room->sector_type == SECT_UNDERWATER
            || ch->in_room->sector_type == SECT_OCEANFLOOR )
            {
               if( number_bits( 1 ) == 0 )
                  gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod );
            }
            else
               gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod );
         }
      }

      if( !char_died( ch ) )
      {
         /*
          * Careful with the damages here,
          * MUST NOT refer to ch after damage taken, without checking
          * return code and/or char_died as it may be lethal damage.
          */
         if( IS_AFFECTED( ch, AFF_POISON ) )
         {
            act( AT_POISON, "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM );
            act( AT_POISON, "You shiver and suffer.", ch, NULL, NULL, TO_CHAR );
            ch->mental_state = URANGE( 20, ch->mental_state + ( is_npc( ch ) ? 2 : 3 ), 100 );
            damage( ch, ch, NULL, 6, gsn_poison, false );
         }
         else if( ch->position == POS_INCAP )
            damage( ch, ch, NULL, 1, TYPE_UNDEFINED, false );
         else if( ch->position == POS_MORTAL )
            damage( ch, ch, NULL, 4, TYPE_UNDEFINED, false );

         if( char_died( ch ) )
            continue;

         if( ch->mental_state >= 30 )
         {
            switch( ( ch->mental_state + 5 ) / 10 )
            {
               case 3:
                  send_to_char( "You feel feverish.\r\n", ch );
                  act( AT_ACTION, "$n looks kind of out of it.", ch, NULL, NULL, TO_ROOM );
                  break;

               case 4:
                  send_to_char( "You don't feel well at all.\r\n", ch );
                  act( AT_ACTION, "$n doesn't look too good.", ch, NULL, NULL, TO_ROOM );
                  break;

               case 5:
                  send_to_char( "You need help!\r\n", ch );
                  act( AT_ACTION, "$n looks like $e could use your help.", ch, NULL, NULL, TO_ROOM );
                  break;

               case 6:
                  send_to_char( "Seekest thou a cleric.\r\n", ch );
                  act( AT_ACTION, "Someone should fetch a healer for $n.", ch, NULL, NULL, TO_ROOM );
                  break;

               case 7:
                  send_to_char( "You feel reality slipping away...\r\n", ch );
                  act( AT_ACTION, "$n doesn't appear to be aware of what's going on.", ch, NULL, NULL, TO_ROOM );
                  break;

               case 8:
                  send_to_char( "You begin to understand... everything.\r\n", ch );
                  act( AT_ACTION, "$n starts ranting like a madman!", ch, NULL, NULL, TO_ROOM );
                  break;

               case 9:
                  send_to_char( "You're ONE with the universe.\r\n", ch );
                  act( AT_ACTION, "$n is ranting on about 'the answer', 'ONE' and other mumbo-jumbo...", ch, NULL, NULL, TO_ROOM );
                  break;

               case 10:
                  send_to_char( "You feel the end is near.\r\n", ch );
                  act( AT_ACTION, "$n is muttering and ranting in tongues...", ch, NULL, NULL, TO_ROOM );
                  break;
            }
         }

         if( ch->mental_state <= -30 )
         {
            switch( ( abs( ch->mental_state ) + 5 ) / 10 )
            {
               case 10:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ( ch->position == POS_STANDING || ch->position < POS_FIGHTING )
                     && number_percent( ) + 10 < abs( ch->mental_state ) )
                        do_sleep( ch, (char *)"" );
                     else
                        send_to_char( "You're barely conscious.\r\n", ch );
                  }
                  break;

               case 9:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ( ch->position == POS_STANDING || ch->position < POS_FIGHTING )
                     && ( number_percent( ) + 20 ) < abs( ch->mental_state ) )
                        do_sleep( ch, (char *)"" );
                     else
                        send_to_char( "You can barely keep your eyes open.\r\n", ch );
                  }
                  break;

               case 8:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ch->position < POS_SITTING && ( number_percent( ) + 30 ) < abs( ch->mental_state ) )
                        do_sleep( ch, (char *)"" );
                     else
                        send_to_char( "You're extremely drowsy.\r\n", ch );
                  }
                  break;

               case 7:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel very unmotivated.\r\n", ch );
                  break;

               case 6:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel sedated.\r\n", ch );
                  break;

               case 5:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel sleepy.\r\n", ch );
                  break;

               case 4:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel tired.\r\n", ch );
                  break;

               case 3:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You could use a rest.\r\n", ch );
                  break;

               default:
                  break;
            }
         }

         /* Just a random thing to once in awhile give them luck */
         handle_stat( ch, STAT_LCK, true, 1 );

         if( ch->timer > 24 )
            do_quit( ch, (char *)"" );
         else if( ch == ch_save && xIS_SET( sysdata.save_flags, SV_AUTO ) && ++save_count < 10 ) /* save max of 10 per tick */
            save_char_obj( ch );
      }
   }
}

/* Update all objs. This function is performance sensitive. */
void obj_update( void )
{
   OBJ_DATA *obj, *iobj;
   short AT_TEMP;

   for( obj = last_object; obj; obj = gobj_prev )
   {
      CHAR_DATA *rch;
      const char *message;

      gobj_prev = obj->prev;

      falling = 1;
      obj_fall( obj, false );
      falling = 0;

      if( !obj )
         continue;

      /* Everything should be somewhere if not need to find where objects are getting lost at */
      if( !obj->carried_by && !obj->in_room )
      {
         if( obj->auctioned )
            continue;

         if( ( iobj = obj->in_obj ) ) /* Have to get top object and see if it is carried by someone or in a room */
         {
            while( iobj->in_obj )
               iobj = iobj->in_obj;
         }

         if( iobj && iobj->auctioned )
            continue;

         if( !iobj || ( iobj && !iobj->carried_by && !iobj->in_room ) )
         {
            if( !iobj )
               bug( "%s: %d isn't being carried by anyone and isn't in a room.", __FUNCTION__, obj->pIndexData->vnum );
            else
               bug( "%s: %d isn't being carried by anyone and isn't in a room.", __FUNCTION__, iobj->pIndexData->vnum );
            continue;
         }
      }

      if( obj->carried_by || ( obj->in_room && obj->in_room->area->nplayer > 0 ) )
         oprog_random_trigger( obj );

      if( !obj )
         continue;

      if( obj->item_type == ITEM_PIPE )
      {
         if( IS_SET( obj->value[3], PIPE_LIT ) )
         {
            if( --obj->value[1] <= 0 )
            {
               obj->value[1] = 0;
               REMOVE_BIT( obj->value[3], PIPE_LIT );
               SET_BIT( obj->value[3], PIPE_FULLOFASH );
               if( obj->carried_by )
                  act( AT_ACTION, "$p finishes burning and goes out.", obj->carried_by, obj, NULL, TO_CHAR );
            }
            else if( IS_SET( obj->value[3], PIPE_GOINGOUT ) )
            {
               REMOVE_BIT( obj->value[3], PIPE_LIT );
               REMOVE_BIT( obj->value[3], PIPE_GOINGOUT );
               if( obj->carried_by )
                  act( AT_ACTION, "$p goes out.", obj->carried_by, obj, NULL, TO_CHAR );
            }
            else
               SET_BIT( obj->value[3], PIPE_GOINGOUT );
         }
      }

      if( obj->bsplatter > 0 )
      {
         obj->bsplatter--;
         obj->bstain++;
         if( obj->carried_by )
            act( AT_ACTION, "$p soaks up some blood leaving a stain where the blood was.", obj->carried_by, obj, NULL, TO_CHAR );
      }

      /* Corpse decay (npc corpses decay at 8 times the rate of pc corpses) - Narn */
      if( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC )
      {
         short timerfrac = UMAX( 1, obj->timer - 1 );
         if( obj->item_type == ITEM_CORPSE_PC )
            timerfrac = ( int )( obj->timer / 8 + 1 );
         /* If its empty go ahead and remove it already */
         if( !obj->first_content )
            obj->timer = 1;
         if( obj->timer > 0 && obj->value[2] > timerfrac )
         {
            char *bufptr, buf[MSL], name[MSL];

            bufptr = one_argument( obj->short_descr, name );
            bufptr = one_argument( bufptr, name );
            bufptr = one_argument( bufptr, name );

            separate_obj( obj );
            obj->value[2] = timerfrac;
            snprintf( buf, sizeof( buf ), corpse_descs[UMIN( timerfrac - 1, 4 )], bufptr );
            STRSET( obj->description, buf );
         }
      }

      /* don't let inventory decay */
      if( is_obj_stat( obj, ITEM_INVENTORY ) )
         continue;

      /* groundrot items only decay on the ground */
      if( is_obj_stat( obj, ITEM_GROUNDROT ) && !obj->in_room )
         continue;

      if( ( obj->timer <= 0 || --obj->timer > 0 ) )
         continue;

      /* if we get this far, object's timer has expired. */
      AT_TEMP = AT_PLAIN;
      switch( obj->item_type )
      {
         default:
            message = "$p mysteriously vanishes.";
            AT_TEMP = AT_PLAIN;
            break;

         case ITEM_CONTAINER:
            message = "$p falls apart, tattered from age.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_PORTAL:
            message = "$p unravels and winks from existence.";
            remove_portal( obj );
            obj->item_type = ITEM_TRASH;  /* so extract_obj doesn't remove_portal */
            AT_TEMP = AT_MAGIC;
            break;

         case ITEM_FOUNTAIN:
            message = "$p dries up.";
            AT_TEMP = AT_BLUE;
            break;

         case ITEM_CORPSE_NPC:
            message = "$p decays into dust and blows away.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_CORPSE_PC:
            message = "$p is sucked into a swirling vortex of colors...";
            AT_TEMP = AT_MAGIC;
            break;

         case ITEM_COOK:
         case ITEM_FOOD:
         case ITEM_FISH:
            message = "$p is devoured by a swarm of maggots.";
            AT_TEMP = AT_HUNGRY;
            break;

         case ITEM_BLOOD:
            message = "$p slowly seeps into the ground.";
            AT_TEMP = AT_BLOOD;
            break;

         case ITEM_BLOODSTAIN:
            message = "$p dries up into flakes and blows away.";
            AT_TEMP = AT_BLOOD;
            break;

         case ITEM_SCRAPS:
            message = "$p crumble and decay into nothing.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_FIRE:
            message = "$p burns out.";
            AT_TEMP = AT_FIRE;
            break;
      }

      if( ( iobj = obj->in_obj ) )
      {
         while( iobj->in_obj )
            iobj = iobj->in_obj;
      }

      /* Empty corpses into the room */
      if( obj->in_room && ( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC ) )
         empty_obj( obj, NULL, obj->in_room );

      if( iobj && iobj->carried_by )
         act( AT_TEMP, message, iobj->carried_by, obj, NULL, TO_CHAR );
      else if( iobj && iobj->in_room && !is_obj_stat( iobj, ITEM_BURIED ) )
      {
         for( rch = iobj->in_room->first_person; rch; rch = rch->next_in_room )
            if( is_same_cords( iobj, rch ) )
               act( AT_TEMP, message, rch, obj, NULL, TO_CHAR );
      }
      else if( obj->carried_by )
         act( AT_TEMP, message, obj->carried_by, obj, NULL, TO_CHAR );
      else if( obj->in_room  && !is_obj_stat( obj, ITEM_BURIED ) )
      {
         for( rch = obj->in_room->first_person; rch; rch = rch->next_in_room )
            if( is_same_cords( obj, rch ) )
               act( AT_TEMP, message, rch, obj, NULL, TO_CHAR );
      }

      extract_obj( obj );
   }
}

/*
 * Function to check important stuff happening to a player
 * This function should take about 5% of mud cpu time
 */
void char_check( void )
{
   CHAR_DATA *ch, *ch_next;
   OBJ_DATA *obj;
   EXIT_DATA *pexit;
   static int cnt = 0;
   int door, retcode;

   /* This little counter can be used to handle periodic events */
   cnt = ( cnt + 1 ) % SECONDS_PER_TICK;

   for( ch = first_char; ch; ch = ch_next )
   {
      set_cur_char( ch );
      ch_next = ch->next;
      will_fall( ch, 0 );

      if( char_died( ch ) )
         continue;

      if( is_npc( ch ) )
      {
         if( ( cnt & 1 ) )
            continue;

         /* running mobs  -Thoric */
         if( xIS_SET( ch->act, ACT_RUNNING ) )
         {
            if( !xIS_SET( ch->act, ACT_SENTINEL ) && !ch->fighting && ch->first_hunting )
            {
               wait_state( ch, 2 * PULSE_VIOLENCE );
               hunt_victim( ch );
               continue;
            }

            if( ch->spec_fun )
            {
               if( ( *ch->spec_fun ) ( ch ) )
                  continue;
               if( char_died( ch ) )
                  continue;
            }

            if( !xIS_SET( ch->act, ACT_SENTINEL )
            && !xIS_SET( ch->act, ACT_PROTOTYPE )
            && ( door = number_bits( 4 ) ) <= 9
            && ( pexit = get_exit( ch->in_room, door ) )
            && pexit->to_room
            && !xIS_SET( pexit->exit_info, EX_WINDOW )
            && !xIS_SET( pexit->exit_info, EX_CLOSED )
            && !xIS_SET( pexit->to_room->room_flags, ROOM_NO_MOB )
            && !xIS_SET( pexit->to_room->room_flags, ROOM_DEATH )
            && ( !xIS_SET( ch->act, ACT_STAY_AREA ) || pexit->to_room->area == ch->in_room->area ) )
            {
               retcode = move_char( ch, pexit, 0, false );
               if( char_died( ch ) )
                  continue;
               if( retcode == rNONE || retcode == rSTOP || xIS_SET( ch->act, ACT_SENTINEL ) || ch->position < POS_STANDING )
                  continue;
            }
         }
         continue;
      }
      else
      {
         if( ch->mount && ch->in_room != ch->mount->in_room )
         {
            xREMOVE_BIT( ch->mount->act, ACT_MOUNTED );
            ch->mount = NULL;
            ch->position = POS_STANDING;
            send_to_char( "No longer upon your mount, you fall to the ground...\r\nOUCH!\r\n", ch );
         }

         if( ch->in_room
         && ( ch->in_room->sector_type == SECT_UNDERWATER || ch->in_room->sector_type == SECT_OCEANFLOOR ) )
         {
            if( !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            {
               if( get_trust( ch ) < PERM_IMM && number_bits( 3 ) == 0 )
               {
                  int dam;

                  dam = number_range( ( ch->max_hit / 100 ), ( ch->max_hit / 50 ) );
                  dam = UMAX( 1, dam );
                  send_to_char( "You cough and choke as you try to breathe water!\r\n", ch );
                  damage( ch, ch, NULL, dam, TYPE_UNDEFINED, false );
               }
            }
         }

         if( char_died( ch ) )
            continue;

         if( ch->in_room
         && ( ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM ) )
         {
            if( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLOATING ) && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) && !ch->mount )
            {
               for( obj = ch->first_carrying; obj; obj = obj->next_content )
                  if( obj->item_type == ITEM_BOAT )
                     break;

               if( !obj )
               {
                  if( get_trust( ch ) < PERM_IMM && number_bits( 3 ) == 0 )
                  {
                     int mov, dam;

                     if( ch->move > 0 )
                     {
                        mov = number_range( ( ch->max_move / 100 ), ( ch->max_move / 50 ) );
                        mov = UMAX( 1, mov );
                        ch->move = URANGE( 0, ( ch->move - mov ), ch->max_move );
                     }
                     else
                     {
                        dam = number_range( ( ch->max_hit / 100 ), ( ch->max_hit / 50 ) );
                        dam = UMAX( 1, dam );
                        send_to_char( "Struggling with exhaustion, you choke on a mouthful of water.\r\n", ch );
                        damage( ch, ch, NULL, dam, TYPE_UNDEFINED, false );
                     }
                  }
               }
            }
         }

         /* beat up on link dead players */
         if( !ch->desc )
         {
            CHAR_DATA *wch, *wch_next;

            for( wch = ch->in_room->first_person; wch; wch = wch_next )
            {
               wch_next = wch->next_in_room;

               if( !is_npc( wch )
               || wch->fighting
               || IS_AFFECTED( wch, AFF_CHARM )
               || !is_awake( wch )
               || ( xIS_SET( wch->act, ACT_WIMPY ) && is_awake( ch ) )
               || !can_see( wch, ch ) )
                  continue;

               if( is_hating( wch, ch ) )
               {
                  found_prey( wch, ch );
                  continue;
               }

               if( ( !xIS_SET( wch->act, ACT_AGGRESSIVE ) && !xIS_SET( wch->act, ACT_META_AGGR ) )
               || xIS_SET( wch->act, ACT_MOUNTED )
               || xIS_SET( wch->in_room->room_flags, ROOM_SAFE ) )
                  continue;
               global_retcode = multi_hit( wch, ch, TYPE_UNDEFINED );
            }
         }
      }
   }
}

/*
 * Aggress.
 *
 * for each descriptor
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function should take 5% to 10% of ALL mud cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't want the mob to just attack the first PC
 *   who leads the party into the room.
 *
 */
void aggr_room_update( CHAR_DATA *wch, ROOM_INDEX_DATA *room )
{
   CHAR_DATA *ch, *ch_next, *vch, *vch_next, *victim;

   if( !wch || char_died( wch ) || is_npc( wch ) || get_trust( wch ) >= PERM_IMM || !wch->in_room || !room )
      return;

   for( ch = room->first_person; ch; ch = ch_next )
   {
      int count;

      ch_next = ch->next_in_room;

      if( !is_npc( ch )
      || ch->fighting
      || IS_AFFECTED( ch, AFF_CHARM )
      || !is_awake( ch )
      || ( xIS_SET( ch->act, ACT_WIMPY ) && is_awake( wch ) )
      || !can_see( ch, wch )
      || !can_see_character( ch, wch ) )
         continue;

      if( is_hating( ch, wch ) )
      {
         found_prey( ch, wch );
         continue;
      }

      if( ( !xIS_SET( ch->act, ACT_AGGRESSIVE ) && !xIS_SET( ch->act, ACT_META_AGGR ) )
      || xIS_SET( ch->act, ACT_MOUNTED )
      || xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
         continue;

      /*
       * Ok we have a 'wch' player character and a 'ch' npc aggressor.
       * Now make the aggressor fight a RANDOM pc victim in the room,
       *   giving each 'vch' an equal chance of selection.
       *
       * Depending on flags set, the mob may attack another mob
       */
      count = 0;
      victim = NULL;
      for( vch = room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;

         if( ( !is_npc( vch ) || xIS_SET( ch->act, ACT_META_AGGR ) || xIS_SET( vch->act, ACT_ANNOYING ) )
         && get_trust( vch ) < PERM_IMM
         && ( !xIS_SET( ch->act, ACT_WIMPY ) || !is_awake( vch ) ) && can_see( ch, vch ) )
         {
            if( number_range( 0, count ) == 0 )
               victim = vch;
            count++;
         }
      }

      if( !victim )
      {
         bug( "%s: null victim. %d", __FUNCTION__, count );
         continue;
      }

      /* backstabbing mobs (Thoric) */
      if( is_npc( ch ) && xIS_SET( ch->attacks, ATCK_BACKSTAB ) )
      {
         OBJ_DATA *obj;

         if( !ch->mount
         && ( obj = get_eq_hold( ch, ITEM_WEAPON ) )
         && ( obj->value[3] == DAM_PIERCE || obj->value[3] == DAM_STAB )
         && !victim->fighting && victim->hit >= victim->max_hit )
         {
            wait_state( ch, skill_table[gsn_backstab]->beats );
            if( !is_awake( victim ) || number_percent( ) + 5 < ch->level )
            {
               global_retcode = multi_hit( ch, victim, gsn_backstab );
               continue;
            }
            else
            {
               global_retcode = damage( ch, victim, NULL, 0, gsn_backstab, true );
               continue;
            }
         }
      }
      global_retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
   }
}

void aggr_update( void )
{
   DESCRIPTOR_DATA *d, *dnext;
   CHAR_DATA *wch;
   struct act_prog_data *apdtmp;

   /* check mobprog act queue */
   while( ( apdtmp = mob_act_list ) )
   {
      wch = ( CHAR_DATA * ) mob_act_list->vo;
      if( !char_died( wch ) && wch->mpactnum > 0 )
      {
         MPROG_ACT_LIST *tmp_act;

         while( ( tmp_act = wch->mpact ) )
         {
            if( tmp_act->obj )
               tmp_act->obj = NULL;
            if( tmp_act->ch && !char_died( tmp_act->ch ) )
               mprog_wordlist_check( tmp_act->buf, wch, tmp_act->ch, tmp_act->obj, tmp_act->vo, ACT_PROG );
            wch->mpact = tmp_act->next;
            STRFREE( tmp_act->buf );
            DISPOSE( tmp_act );
         }
         wch->mpactnum = 0;
         wch->mpact = NULL;
      }
      mob_act_list = apdtmp->next;
      DISPOSE( apdtmp );
   }

   /*
    * Just check descriptors here for victims to aggressive mobs
    * We can check for linkdead victims in char_check   -Thoric
    */
   for( d = first_descriptor; d; d = dnext )
   {
      dnext = d->next;
      if( d->connected != CON_PLAYING || !( wch = d->character ) )
         continue;

      if( char_died( wch ) || is_npc( wch ) || get_trust( wch ) >= PERM_IMM || !wch->in_room )
         continue;

      aggr_room_update( wch, wch->in_room );
   }
}

/* drunk randoms - Tricops (Made part of mobile_update -Thoric) */
void drunk_randoms( CHAR_DATA *ch )
{
   CHAR_DATA *rvch = NULL, *vch;
   short drunk, position;

   if( is_npc( ch ) || ch->pcdata->condition[COND_DRUNK] <= 0 )
      return;

   if( number_percent( ) < 30 )
      return;

   drunk = ch->pcdata->condition[COND_DRUNK];
   position = ch->position;
   ch->position = POS_STANDING;

   if( number_percent( ) < ( 2 * drunk / 20 ) )
      check_social( ch, "burp", (char *)"" );
   else if( number_percent( ) < ( 2 * drunk / 20 ) )
      check_social( ch, "hiccup", (char *)"" );
   else if( number_percent( ) < ( 2 * drunk / 20 ) )
      check_social( ch, "drool", (char *)"" );
   else if( number_percent( ) < ( 2 * drunk / 20 ) )
      check_social( ch, "fart", (char *)"" );
   else if( drunk > ( 10 + ( get_curr_con( ch ) / 5 ) ) && number_percent( ) < ( 2 * drunk / 18 ) )
   {
      for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
         if( number_percent( ) < 10 )
            rvch = vch;
      check_social( ch, "puke", ( rvch ? rvch->name : ( char * )"" ) );
   }

   ch->position = position;
}

/*
 * Random hallucinations for those suffering from an overly high mentalstate
 * (Hats off to Albert Hoffman's "problem child") -Thoric
 */
void hallucinations( CHAR_DATA *ch )
{
   if( ch->mental_state >= 30 && number_bits( 5 - ( ch->mental_state >= 50 ) - ( ch->mental_state >= 75 ) ) == 0 )
   {
      const char *t;

      switch( number_range( 1, UMIN( 21, ( ch->mental_state + 5 ) / 5 ) ) )
      {
         default:
         case 1:
            t = "You feel very restless... you can't sit still.\r\n";
            break;

         case 2:
            t = "You're tingling all over.\r\n";
            break;

         case 3:
            t = "Your skin is crawling.\r\n";
            break;

         case 4:
            t = "You suddenly feel that something is terribly wrong.\r\n";
            break;

         case 5:
            t = "Those damn little fairies keep laughing at you!\r\n";
            break;

         case 6:
            t = "You can hear your mother crying...\r\n";
            break;

         case 7:
            t = "Have you been here before, or not?  You're not sure...\r\n";
            break;

         case 8:
            t = "Painful childhood memories flash through your mind.\r\n";
            break;

         case 9:
            t = "You hear someone call your name in the distance...\r\n";
            break;

         case 10:
            t = "Your head is pulsating... you can't think straight.\r\n";
            break;

         case 11:
            t = "The ground... seems to be squirming...\r\n";
            break;

         case 12:
            t = "You're not quite sure what is real anymore.\r\n";
            break;

         case 13:
            t = "It's all a dream... or is it?\r\n";
            break;

         case 14:
            t = "You hear your grandchildren praying for you to watch over them.\r\n";
            break;

         case 15:
            t = "They're coming to get you... coming to take you away...\r\n";
            break;

         case 16:
            t = "You begin to feel all powerful!\r\n";
            break;

         case 17:
            t = "You're light as air... the heavens are yours for the taking.\r\n";
            break;

         case 18:
            t = "Your whole life flashes by... and your future...\r\n";
            break;

         case 19:
            t = "You're everywhere and everything... you know all and are all!\r\n";
            break;

         case 20:
            t = "You feel immortal!\r\n";
            break;

         case 21:
            t = "Ahh... the power of a Supreme Entity... what to do...\r\n";
            break;
      }
      send_to_char( t, ch );
   }
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */
void update_handler( void )
{
   static int pulse_area;
   static int pulse_mobile;
   static int pulse_violence;
   static int pulse_point;
   static int pulse_second;
   static int pulse_trivia;
   static int pulse_room;
   static int pulse_rnum; /* Lets have it redo the random numbers every so often */
   static int pulse_time; /* Make it easier to change the time if you want to */
   struct timeval sttime;
   struct timeval etime;

   pfile_cleanup( );

   update_fishing( );

   quest_update( );

   if( --pulse_rnum <= 0 ) /* Don't have to do it alot but at least once in awhile */
   {
      init_mm( );
      pulse_rnum = number_range( 100, 200 );
   }

   if( timechar )
   {
      set_char_color( AT_PLAIN, timechar );
      send_to_char( "Starting update timer.\r\n", timechar );
      gettimeofday( &sttime, NULL );
   }

   if( --pulse_area <= 0 )
   {
      pulse_area = number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 );
      area_update( );
   }

   if( --pulse_mobile <= 0 )
   {
      pulse_mobile = PULSE_MOBILE;
      mobile_update( );
   }

   if( --pulse_room <= 0 )
   {
      pulse_room = PULSE_ROOM;
      room_update( );
   }

   if( --pulse_violence <= 0 )
   {
      pulse_violence = PULSE_VIOLENCE;
      violence_update( );
   }

   /* Easier to modify time this way. Also increased it by .50 each to slow it down a little */
   if( -- pulse_time <= 0 )
   {
      pulse_time = number_range( ( int )( PULSE_TICK * 1.25 ), ( int )( PULSE_TICK * 1.75 ) );

      time_update( );
   }

   if( --pulse_point <= 0 )
   {
      pulse_point = number_range( ( int )( PULSE_TICK * 0.75 ), ( int )( PULSE_TICK * 1.25 ) );

      auth_update( );  /* Gorog */
      weather_update( );
      char_update( );
      obj_update( );
   }

   pc_regen( ); /* Allow pcs to regen differently */

   if( --pulse_second <= 0 )
   {
      pulse_second = PULSE_PER_SECOND;
      char_check( );
   }

   if( --pulse_trivia <= 0 )
   {
      pulse_trivia = PULSE_TRIVIA;
      if( utrivia && utrivia->running )
         trivia_update( );
   }

   tele_update( );
   aggr_update( );
   obj_act_update( );
   room_act_update( );
   clean_char_queue( );   /* dispose of dead mobs/quitting chars */
   if( timechar )
   {
      gettimeofday( &etime, NULL );
      set_char_color( AT_PLAIN, timechar );
      send_to_char( "Update timing complete.\r\n", timechar );
      subtract_times( &etime, &sttime );
      ch_printf( timechar, "Timing took %ld.%06ld seconds.\r\n", (time_t)etime.tv_sec, (time_t)etime.tv_usec );
      timechar = NULL;
   }
   tail_chain( );
}

void remove_portal( OBJ_DATA *portal )
{
   ROOM_INDEX_DATA *fromRoom, *toRoom;
   EXIT_DATA *pexit;
   bool found;

   if( !portal )
   {
      bug( "%s: portal is NULL", __FUNCTION__ );
      return;
   }

   if( !( fromRoom = portal->in_room ) )
   {
      bug( "%s: portal->in_room is NULL", __FUNCTION__ );
      return;
   }

   found = false;
   for( pexit = fromRoom->first_exit; pexit; pexit = pexit->next )
   {
      if( xIS_SET( pexit->exit_info, EX_PORTAL ) && pexit->vdir == DIR_PORTAL )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      if( pexit && pexit->vdir != DIR_PORTAL )
         bug( "%s: exit in dir %d != DIR_PORTAL", __FUNCTION__, pexit->vdir );
      else
         bug( "%s: portal exit not found in room %d!", __FUNCTION__, fromRoom->vnum );
      return;
   }

   if( !( toRoom = pexit->to_room ) )
      bug( "%s: toRoom is NULL", __FUNCTION__ );

   extract_exit( fromRoom, pexit );
}

/* Check to see if the ch is still able to hold the item if not put it in the room. */
/* If they can hold it return true, else return false */
/* This is now being used by check_chareq for items not being worn. */
bool weight_check( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( !obj )
   {
      bug( "%s: NULL obj", __FUNCTION__ );
      return false;
   }
   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return false;
   }
   if( !ch->in_room )
   {
      bug( "%s: NULL ch->in_room", __FUNCTION__ );
      return false;
   }

   if( ( ch->carry_weight + get_obj_weight( obj ) ) > can_carry_w( ch ) )
   {
      act( AT_PLAIN, "$p is too heavy for you to carry with your current inventory.", ch, obj, NULL, TO_CHAR );
      act( AT_PLAIN, "$n is carrying too much to also carry $p, and $e drops it.", ch, obj, NULL, TO_ROOM );
      if( obj->carried_by )
         obj_from_char( obj );
      obj_to_room( obj, ch->in_room );
      return false;
   }

   if( !obj->carried_by )
      obj_to_char( obj, ch );
   return true;
}

void subtract_times( struct timeval *etime, struct timeval *sttime )
{
   etime->tv_sec -= sttime->tv_sec;
   etime->tv_usec -= sttime->tv_usec;
   while( etime->tv_usec < 0 )
   {
      etime->tv_usec += 1000000;
      etime->tv_sec--;
   }
}

/*
 * Function to update weather vectors according to climate
 * settings, random effects, and neighboring areas.
 * Last modified: July 18, 1997 - Fireblade
 */
void adjust_vectors( WEATHER_DATA *weather )
{
   NEIGHBOR_DATA *neigh;
   double dT, dP, dW;

   if( !weather )
   {
      bug( "%s: NULL weather data.", __FUNCTION__ );
      return;
   }

   dT = 0;
   dP = 0;
   dW = 0;

   /* Add in random effects */
   dT += number_range( -rand_factor, rand_factor );
   dP += number_range( -rand_factor, rand_factor );
   dW += number_range( -rand_factor, rand_factor );

   /* Add in climate effects */
   dT += climate_factor * ( ( ( weather->climate_temp   - 2 ) * weath_unit ) - ( weather->temp   ) ) / weath_unit;
   dP += climate_factor * ( ( ( weather->climate_precip - 2 ) * weath_unit ) - ( weather->precip ) ) / weath_unit;
   dW += climate_factor * ( ( ( weather->climate_wind   - 2 ) * weath_unit ) - ( weather->wind   ) ) / weath_unit;

   /* Add in effects from neighboring areas */
   for( neigh = weather->first_neighbor; neigh; neigh = neigh->next )
   {
      /* see if we have the area cache'd already */
      if( !neigh->address )
      {
         /* try and find address for area */
         neigh->address = get_area( neigh->name );

         /* if couldn't find area ditch the neigh */
         if( !neigh->address )
         {
            NEIGHBOR_DATA *temp;

            bug( "%s: invalid area name.", __FUNCTION__ );
            temp = neigh->prev;
            UNLINK( neigh, weather->first_neighbor, weather->last_neighbor, next, prev );
            STRFREE( neigh->name );
            DISPOSE( neigh );
            neigh = temp;
            continue;
         }
      }

      dT += ( neigh->address->weather->temp - weather->temp ) / neigh_factor;
      dP += ( neigh->address->weather->precip - weather->precip ) / neigh_factor;
      dW += ( neigh->address->weather->wind - weather->wind ) / neigh_factor;
   }

   /* now apply the effects to the vectors */
   weather->temp_vector += ( int )dT;
   weather->precip_vector += ( int )dP;
   weather->wind_vector += ( int )dW;

   /* Make sure they are within the right range */
   weather->temp_vector = URANGE( -max_vector, weather->temp_vector, max_vector );
   weather->precip_vector = URANGE( -max_vector, weather->precip_vector, max_vector );
   weather->wind_vector = URANGE( -max_vector, weather->wind_vector, max_vector );
}

/*
 * get weather echo messages according to area weather...
 * stores echo message in weath_data.... must be called before
 * the vectors are adjusted
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_weather_echo( WEATHER_DATA *weath )
{
   int n;
   int temp, precip, wind;
   int dT, dP, dW;
   int tindex, pindex, windex;

   /* set echo to be nothing */
   weath->echo = NULL;
   weath->echo_color = AT_GRAY;

   /* get the random number */
   n = number_bits( 2 );

   /* variables for convenience */
   temp = weath->temp;
   precip = weath->precip;
   wind = weath->wind;

   dT = weath->temp_vector;
   dP = weath->precip_vector;
   dW = weath->wind_vector;

   tindex = ( temp + 3 * weath_unit - 1 ) / weath_unit;
   pindex = ( precip + 3 * weath_unit - 1 ) / weath_unit;
   windex = ( wind + 3 * weath_unit - 1 ) / weath_unit;

   /* get the echo string... mainly based on precip */
   switch( pindex )
   {
      case 0:
         if( precip - dP > -2 * weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The clouds disappear.\r\n",
               "The clouds disappear.\r\n",
               "The sky begins to break through the clouds.\r\n",
               "The clouds are slowly evaporating.\r\n"
            };

            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         break;

      case 1:
         if( precip - dP <= -2 * weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The sky is getting cloudy.\r\n",
               "The sky is getting cloudy.\r\n",
               "Light clouds cast a haze over the sky.\r\n",
               "Billows of clouds spread through the sky.\r\n"
            };
            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_GRAY;
         }
         break;

      case 2:
         if( precip - dP > 0 )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] =
               {
                  "The rain stops.\r\n",
                  "The rain stops.\r\n",
                  "The rainstorm tapers off.\r\n",
                  "The rain's intensity breaks.\r\n"
               };
               weath->echo = ( char * ) echo_strings[n];
               weath->echo_color = AT_CYAN;
            }
            else
            {
               const char *echo_strings[4] =
               {
                  "The snow stops.\r\n",
                  "The snow stops.\r\n",
                  "The snow showers taper off.\r\n",
                  "The snow flakes disappear from the sky.\r\n"
               };
               weath->echo = ( char * ) echo_strings[n];
               weath->echo_color = AT_WHITE;
            }
         }
         break;

      case 3:
         if( precip - dP <= 0 )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] =
               {
                  "It starts to rain.\r\n",
                  "It starts to rain.\r\n",
                  "A droplet of rain falls upon you.\r\n",
                  "The rain begins to patter.\r\n"
               };
               weath->echo = ( char * ) echo_strings[n];
               weath->echo_color = AT_CYAN;
            }
            else
            {
               const char *echo_strings[4] =
               {
                  "It starts to snow.\r\n",
                  "It starts to snow.\r\n",
                  "Crystal flakes begin to fall from the sky.\r\n",
                  "Snow flakes drift down from the clouds.\r\n"
               };
               weath->echo = ( char * ) echo_strings[n];
               weath->echo_color = AT_WHITE;
            }
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The temperature drops and the rain becomes a light snow.\r\n",
               "The temperature drops and the rain becomes a light snow.\r\n",
               "Flurries form as the rain freezes.\r\n",
               "Large snow flakes begin to fall with the rain.\r\n"
            };
            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The snow flurries are gradually replaced by pockets of rain.\r\n",
               "The snow flurries are gradually replaced by pockets of rain.\r\n",
               "The falling snow turns to a cold drizzle.\r\n",
               "The snow turns to rain as the air warms.\r\n"
            };
            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      case 4:
         if( precip - dP > 2 * weath_unit )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] =
               {
                  "The lightning has stopped.\r\n",
                  "The lightning has stopped.\r\n",
                  "The sky settles, and the thunder surrenders.\r\n",
                  "The lightning bursts fade as the storm weakens.\r\n"
               };
               weath->echo = ( char * ) echo_strings[n];
               weath->echo_color = AT_GRAY;
            }
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The cold rain turns to snow.\r\n",
               "The cold rain turns to snow.\r\n",
               "Snow flakes begin to fall amidst the rain.\r\n",
               "The driving rain begins to freeze.\r\n"
            };
            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The snow becomes a freezing rain.\r\n",
               "The snow becomes a freezing rain.\r\n",
               "A cold rain beats down on you as the snow begins to melt.\r\n",
               "The snow is slowly replaced by a heavy rain.\r\n"
            };
            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      case 5:
         if( precip - dP <= 2 * weath_unit )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] =
               {
                  "Lightning flashes in the sky.\r\n",
                  "Lightning flashes in the sky.\r\n",
                  "A flash of lightning splits the sky.\r\n",
                  "The sky flashes, and the ground trembles with thunder.\r\n"
               };
               weath->echo = ( char * ) echo_strings[n];
               weath->echo_color = AT_YELLOW;
            }
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The sky rumbles with thunder as the snow changes to rain.\r\n",
               "The sky rumbles with thunder as the snow changes to rain.\r\n",
               "The falling turns to freezing rain amidst flashes of lightning.\r\n",
               "The falling snow begins to melt as thunder crashes overhead.\r\n"
            };
            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            const char *echo_strings[4] =
            {
               "The lightning stops as the rainstorm becomes a blinding blizzard.\r\n",
               "The lightning stops as the rainstorm becomes a blinding blizzard.\r\n",
               "The thunder dies off as the pounding rain turns to heavy snow.\r\n",
               "The cold rain turns to snow and the lightning stops.\r\n"
            };
            weath->echo = ( char * ) echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      default:
         bug( "%s: invalid precip index", __FUNCTION__ );
         weath->precip = 0;
         break;
   }
}

void update_area_weather( AREA_DATA *pArea )
{
   int limit = ( 3 * weath_unit );

   if( !pArea )
      return;

   /* Apply vectors to fields */
   pArea->weather->temp += pArea->weather->temp_vector;
   pArea->weather->precip += pArea->weather->precip_vector;
   pArea->weather->wind += pArea->weather->wind_vector;

   /* Make sure they are within the proper range */
   pArea->weather->temp = URANGE( -limit, pArea->weather->temp, limit );
   pArea->weather->precip = URANGE( -limit, pArea->weather->precip, limit );
   pArea->weather->wind = URANGE( -limit, pArea->weather->wind, limit );

   /* get an appropriate echo for the area */
   get_weather_echo( pArea->weather );
   adjust_vectors( pArea->weather );
}

void single_weather_update( AREA_DATA *pArea )
{
   DESCRIPTOR_DATA *d;

   if( !pArea )
      return;
   update_area_weather( pArea );
   for( d = first_descriptor; d; d = d->next )
   {
      WEATHER_DATA *weath;

      if( d->connected == CON_PLAYING
      && is_outside( d->character )
      && !no_weather_sect( d->character->in_room )
      && is_awake( d->character )
      && d->character->in_room->area == pArea )
      {
         weath = d->character->in_room->area->weather;

         if( !weath->echo )
            continue;
         set_char_color( weath->echo_color, d->character );
         send_to_char( weath->echo, d->character );
      }
   }
}

void weather_update( void )
{
   AREA_DATA *pArea;
   DESCRIPTOR_DATA *d;

   for( pArea = first_area; pArea; pArea = ( pArea == last_area ) ? first_build : pArea->next )
      update_area_weather( pArea );

   /* display the echo strings to the appropriate players */
   for( d = first_descriptor; d; d = d->next )
   {
      WEATHER_DATA *weath;

      if( d->connected == CON_PLAYING
      && is_outside( d->character )
      && !no_weather_sect( d->character->in_room )
      && is_awake( d->character ) )
      {
         weath = d->character->in_room->area->weather;
         if( !weath->echo )
            continue;
         set_char_color( weath->echo_color, d->character );
         send_to_char( weath->echo, d->character );
      }
   }
}
