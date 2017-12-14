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
 *			    Battle & death module			     *
 *****************************************************************************/

#include <stdio.h>
#include "h/mud.h"

OBJ_DATA *used_weapon;  /* Used to figure out which weapon later */
OBJ_DATA *ncorpse; /* Used to keep track of corpse */

HHF_DATA *first_hating, *last_hating;
HHF_DATA *first_hunting, *last_hunting;
HHF_DATA *first_fearing, *last_fearing;

int get_style_mod( CHAR_DATA *ch, CHAR_DATA *victim, int damage )
{
   int dam = damage;

   /* Calculate Damage Modifiers from Victim's Fighting Style  */
   if( victim->position == POS_BERSERK )
      dam = ( int )( 1.2 * dam );
   else if( victim->position == POS_AGGRESSIVE )
      dam = ( int )( 1.1 * dam );
   else if( victim->position == POS_DEFENSIVE )
      dam = ( int )( .85 * dam );
   else if( victim->position == POS_EVASIVE )
      dam = ( int )( .8 * dam );

   /* Calculate Damage Modifiers from Attacker's Fighting Style */
   if( ch->position == POS_BERSERK )
      dam = ( int )( 1.2 * dam );
   else if( ch->position == POS_AGGRESSIVE )
      dam = ( int )( 1.1 * dam );
   else if( ch->position == POS_DEFENSIVE )
      dam = ( int )( .85 * dam );
   else if( ch->position == POS_EVASIVE )
      dam = ( int )( .8 * dam );

   return dam;
}

/* Compare various things and decide if it should hit or not */
int hit_vict_chance( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield )
{
   /* Compare hitroll of character to victims armorclass */
   int chance = URANGE( 0, ( ( get_hitroll( ch ) / 5 ) - ( get_ac( victim ) / 10 ) ), 20 );

   /* If victim can't see what they are being hit by increase chance of being hit */
   if( wield && !can_see_obj( victim, wield ) )
      chance += 1;

   /* If can't see victim decrease chance */
   if( !can_see( ch, victim ) )
      chance -= 4;

   /* Compare intelligence for a bonus */
   if( ch->fighting && ch->fighting->who == victim )
      chance += URANGE( -4, ( ( get_curr_int( ch ) / 10 ) - ( get_curr_int( victim ) / 10 ) ), 4 );

   return chance;
}

/* Check to see if player's attacks are (still?) suppressed */
bool is_attack_suppressed( CHAR_DATA *ch )
{
   TIMER *timer;

   if( is_npc( ch ) )
      return false;

   timer = get_timerptr( ch, TIMER_ASUPPRESSED );

   if( !timer )
      return false;

   /* perma-suppression -- bard? (can be reset at end of fight, or spell, etc) */
   if( timer->value == -1 )
      return true;

   /* this is for timed suppressions */
   if( timer->count >= 1 )
      return true;

   return false;
}

/* Check to see if weapon is poisoned. */
bool is_wielding_poisoned( CHAR_DATA *ch )
{
   OBJ_DATA *obj;

   if( !used_weapon )
      return false;

   if( ( obj = get_eq_char( ch, WEAR_HOLD_B ) ) && used_weapon == obj && obj->item_type == ITEM_WEAPON
   && is_obj_stat( obj, ITEM_POISONED ) )
      return true;
   if( ( obj = get_eq_char( ch, WEAR_HOLD_L ) ) && used_weapon == obj && obj->item_type == ITEM_WEAPON
   && is_obj_stat( obj, ITEM_POISONED ) )
      return true;
   if( ( obj = get_eq_char( ch, WEAR_HOLD_R ) ) && used_weapon == obj && obj->item_type == ITEM_WEAPON
   && is_obj_stat( obj, ITEM_POISONED ) )
      return true;
   return false;
}

/*
 * Find a piece of eq on a character.
 * Use top layer if layered
 * Check compariable locations
 * How this works:
 * It wants to check arms
 *    First check arms and if nothing there check each arm separate
 * It wants to check the left arm
 *    First check the left arm and then check the arms
 * *NOTE* If it's checking the left arm and something is not on
 *        the left arm and not on both arms, but something is on
 *        the right arm it won't consider it being worn on the left arm.
 * Makes good sense right??? Does to me at least.
 */
OBJ_DATA *get_eq_location( CHAR_DATA *ch, int iWear )
{
   OBJ_DATA *obj = NULL;

   /* First check the actual wear location */
   if( !( obj = get_eq_char( ch, iWear ) ) )
   {
      /* Now check the similar locations */
      if( iWear == WEAR_EARS )
      {
         if( !( obj = get_eq_char( ch, WEAR_EAR_L ) ) )
            obj = get_eq_char( ch, WEAR_EAR_R );
      }
      if( iWear == WEAR_EAR_L || iWear == WEAR_EAR_R )
         obj = get_eq_char( ch, WEAR_EARS );

      if( iWear == WEAR_EYES )
      {
         if( !( obj = get_eq_char( ch, WEAR_EYE_L ) ) )
            obj = get_eq_char( ch, WEAR_EYE_R );
      }
      if( iWear == WEAR_EYE_L || iWear == WEAR_EYE_R )
         obj = get_eq_char( ch, WEAR_EYES );

      if( iWear == WEAR_SHOULDERS )
      {
         if( !( obj = get_eq_char( ch, WEAR_SHOULDER_L ) ) )
            obj = get_eq_char( ch, WEAR_SHOULDER_R );
      }
      if( iWear == WEAR_SHOULDER_L || iWear == WEAR_SHOULDER_R )
         obj = get_eq_char( ch, WEAR_SHOULDERS );

      if( iWear == WEAR_ARMS )
      {
         if( !( obj = get_eq_char( ch, WEAR_ARM_L ) ) )
            obj = get_eq_char( ch, WEAR_ARM_R );
      }
      if( iWear == WEAR_ARM_L || iWear == WEAR_ARM_R )
         obj = get_eq_char( ch, WEAR_ARMS );

      if( iWear == WEAR_WRISTS )
      {
         if( !( obj = get_eq_char( ch, WEAR_WRIST_L ) ) )
            obj = get_eq_char( ch, WEAR_WRIST_R );
      }
      if( iWear == WEAR_WRIST_L || iWear == WEAR_WRIST_R )
         obj = get_eq_char( ch, WEAR_WRISTS );

      if( iWear == WEAR_HANDS )
      {
         if( !( obj = get_eq_char( ch, WEAR_HAND_L ) ) )
            obj = get_eq_char( ch, WEAR_HAND_R );
      }
      if( iWear == WEAR_HAND_L || iWear == WEAR_HAND_R )
         obj = get_eq_char( ch, WEAR_HANDS );

      if( iWear == WEAR_FINGERS )
      {
         if( !( obj = get_eq_char( ch, WEAR_FINGER_L ) ) )
            obj = get_eq_char( ch, WEAR_FINGER_R );
      }
      if( iWear == WEAR_FINGER_L || iWear == WEAR_FINGER_R )
         obj = get_eq_char( ch, WEAR_FINGERS );

      if( iWear == WEAR_HOLD_B )
      {
         if( !( obj = get_eq_char( ch, WEAR_HOLD_L ) ) )
            obj = get_eq_char( ch, WEAR_HOLD_R );
      }
      if( iWear == WEAR_HOLD_L || iWear == WEAR_HOLD_R )
         obj = get_eq_char( ch, WEAR_HOLD_B );

      if( iWear == WEAR_LEGS )
      {
         if( !( obj = get_eq_char( ch, WEAR_LEG_L ) ) )
            obj = get_eq_char( ch, WEAR_LEG_R );
      }
      if( iWear == WEAR_LEG_L || iWear == WEAR_LEG_R )
         obj = get_eq_char( ch, WEAR_LEGS );

      if( iWear == WEAR_ANKLES )
      {
         if( !( obj = get_eq_char( ch, WEAR_ANKLE_L ) ) )
            obj = get_eq_char( ch, WEAR_ANKLE_R );
      }
      if( iWear == WEAR_ANKLE_L || iWear == WEAR_ANKLE_R )
         obj = get_eq_char( ch, WEAR_ANKLES );

      if( iWear == WEAR_FEET )
      {
         if( !( obj = get_eq_char( ch, WEAR_FOOT_L ) ) )
            obj = get_eq_char( ch, WEAR_FOOT_R );
      }
      if( iWear == WEAR_FOOT_L || iWear == WEAR_FOOT_R )
         obj = get_eq_char( ch, WEAR_FEET );
   }
   return obj;
}

bool is_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
   HHF_DATA *hunt, *hunt_next;

   if( !ch || !ch->first_hunting )
      return false;
   /* Check all characters they are hunting */
   for( hunt = ch->first_hunting; hunt; hunt = hunt_next )
   {
      hunt_next = hunt->next;

      if( hunt->who != NULL && hunt->who == victim )
         return true;
      if( hunt->who == NULL && hunt->name != NULL && hunt->name[0] != '\0' && !str_cmp( hunt->name, victim->name ) )
         return true;
   }

   return false;
}

void stop_hunting( CHAR_DATA *ch, CHAR_DATA *victim, bool complete )
{
   HHF_DATA *hunt, *hunt_next;

   if( !ch || !ch->first_hunting )
      return;
   for( hunt = ch->first_hunting; hunt; hunt = hunt_next )
   {
      hunt_next = hunt->next;

      /* Remove all or just Victim */
      if( complete || ( hunt->who != NULL && hunt->who == victim )
      || ( hunt->who == NULL && hunt->name != NULL && hunt->name[0] != '\0' && !str_cmp( hunt->name, victim->name ) ) )
      {
         UNLINK( hunt, first_hunting, last_hunting, lnext, lprev );
         UNLINK( hunt, ch->first_hunting, ch->last_hunting, next, prev );
         STRFREE( hunt->name );
         hunt->who = NULL;
         hunt->mob = NULL;
         DISPOSE( hunt );
      }
   }
}

void start_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
   HHF_DATA *hunt, *hunt_next;

   if( !ch )
      return;
   /* Make sure not adding repeat information */
   if( ch->first_hunting )
   {
      for( hunt = ch->first_hunting; hunt; hunt = hunt_next )
      {
         hunt_next = hunt->next;

         if( ( hunt->who != NULL && hunt->who == victim )
         || ( hunt->who == NULL && hunt->name != NULL && hunt->name[0] != '\0' && !str_cmp( hunt->name, victim->name ) ) )
            return;
      }
   }
   /* Ok, on to adding it all now */
   hunt = NULL;
   CREATE( hunt, HHF_DATA, 1 );
   LINK( hunt, first_hunting, last_hunting, lnext, lprev );
   LINK( hunt, ch->first_hunting, ch->last_hunting, next, prev );
   hunt->name = QUICKLINK( victim->name );
   hunt->who = victim;
   hunt->mob = ch;
}

bool is_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
   HHF_DATA *hate, *hate_next;

   if( !ch || !ch->first_hating )
      return false;
   /* Check all characters they are hating */
   for( hate = ch->first_hating; hate; hate = hate_next )
   {
      hate_next = hate->next;

      if( hate->who != NULL && hate->who == victim )
         return true;
      if( hate->who == NULL && hate->name != NULL && hate->name[0] != '\0' && !str_cmp( hate->name, victim->name ) )
         return true;
   }

   return false;
}

void stop_hating( CHAR_DATA *ch, CHAR_DATA *victim, bool complete )
{
   HHF_DATA *hate, *hate_next;

   if( !ch || !ch->first_hating )
      return;
   for( hate = ch->first_hating; hate; hate = hate_next )
   {
      hate_next = hate->next;

      /* Remove all or just Victim */
      if( complete || ( hate->who != NULL && hate->who == victim )
      || ( hate->who == NULL && hate->name != NULL && hate->name[0] != '\0' && !str_cmp( hate->name, victim->name ) ) )
      {
         if( ch->summoning == hate->who )
            ch->summoning = NULL;
         UNLINK( hate, first_hating, last_hating, lnext, lprev );
         UNLINK( hate, ch->first_hating, ch->last_hating, next, prev );
         STRFREE( hate->name );
         hate->who = NULL;
         hate->mob = NULL;
         DISPOSE( hate );
      }
   }
}

void start_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
   HHF_DATA *hate, *hate_next;

   if( !ch )
      return;
   /* Make sure not adding repeat information */
   if( ch->first_hating )
   {
      for( hate = ch->first_hating; hate; hate = hate_next )
      {
         hate_next = hate->next;

         if( ( hate->who != NULL && hate->who == victim )
         || ( hate->who == NULL && hate->name != NULL && hate->name[0] != '\0' && !str_cmp( hate->name, victim->name ) ) )
            return;
      }
   }
   /* Ok, on to adding it all now */
   hate = NULL;
   CREATE( hate, HHF_DATA, 1 );
   LINK( hate, first_hating, last_hating, lnext, lprev );
   LINK( hate, ch->first_hating, ch->last_hating, next, prev );
   hate->name = QUICKLINK( victim->name );
   hate->who = victim;
   hate->mob = ch;
}

void stop_summoning( CHAR_DATA *ch )
{
   HHF_DATA *hh, *hh_next;
   CHAR_DATA *vch, *vch_next;

   if( !ch || !ch->in_room )
      return;
   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;

      for( hh = vch->first_hunting; hh; hh = hh_next )
      {
         hh_next = hh->next;

         if( ( hh->who != NULL && hh->who == ch )
         || ( hh->who == NULL && hh->name != NULL && hh->name[0] != '\0' && !str_cmp( hh->name, ch->name ) ) )
            stop_hunting( vch, ch, false );
      }
      for( hh = vch->first_hating; hh; hh = hh_next )
      {
         hh_next = hh->next;
         if( ( hh->who != NULL && hh->who == ch )
         || ( hh->who == NULL && hh->name != NULL && hh->name[0] != '\0' && !str_cmp( hh->name, ch->name ) ) )
            stop_hating( vch, ch, false );
      }
   }
}

bool is_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
   HHF_DATA *fear, *fear_next;

   if( !ch || !ch->first_fearing )
      return false;
   /* Check all characters they are fearing */
   for( fear = ch->first_fearing; fear; fear = fear_next )
   {
      fear_next = fear->next;

      if( fear->who != NULL && fear->who == victim )
         return true;
      if( fear->who == NULL && fear->name != NULL && fear->name[0] != '\0' && !str_cmp( fear->name, victim->name ) )
         return true;
   }

   return false;
}

void stop_fearing( CHAR_DATA *ch, CHAR_DATA *victim, bool complete )
{
   HHF_DATA *fear, *fear_next;

   if( !ch || !ch->first_fearing )
      return;
   for( fear = ch->first_fearing; fear; fear = fear_next )
   {
      fear_next = fear->next;

      /* Remove all or just Victim */
      if( complete || ( fear->who != NULL && fear->who == victim )
      || ( fear->who == NULL && fear->name != NULL && fear->name[0] != '\0' && !str_cmp( fear->name, victim->name ) ) )
      {
         UNLINK( fear, first_fearing, last_fearing, lnext, lprev );
         UNLINK( fear, ch->first_fearing, ch->last_fearing, next, prev );
         STRFREE( fear->name );
         fear->who = NULL;
         fear->mob = NULL;
         DISPOSE( fear );
      }
   }
}

void start_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
   HHF_DATA *fear, *fear_next;

   if( !ch )
      return;
   /* Make sure not adding repeat information */
   if( ch->first_fearing )
   {
      for( fear = ch->first_fearing; fear; fear = fear_next )
      {
         fear_next = fear->next;

         if( ( fear->who != NULL && fear->who == victim )
         || ( fear->who == NULL && fear->name != NULL && fear->name[0] != '\0' && !str_cmp( fear->name, victim->name ) ) )
            return;
      }
   }
   /* Ok, on to adding it all now */
   fear = NULL;
   CREATE( fear, HHF_DATA, 1 );
   LINK( fear, first_fearing, last_fearing, lnext, lprev );
   LINK( fear, ch->first_fearing, ch->last_fearing, next, prev );
   fear->name = QUICKLINK( victim->name );
   fear->who = victim;
   fear->mob = ch;
}

/* Ok so lets see if we can't stop them from logging out to avoid hate, hunt, fear */
/* This will remove the soon to be invalid who pointer, fix_hhf fixes it when they log back in */
void stop_hhf( CHAR_DATA *ch )
{
   HHF_DATA *fhhf, *next_hhf, *tmp_fhhf;
   short tmp;

   if( !ch || ch->name == NULL || ch->name[0] == '\0' )
      return;

   for( tmp = 0; tmp < 3; tmp++ )
   {
      if( tmp == 0 )
         tmp_fhhf = first_hating;
      else if( tmp == 1 )
         tmp_fhhf = first_hunting;
      else if( tmp == 2 )
         tmp_fhhf = first_fearing;
      else
         return;

      for( fhhf = tmp_fhhf; fhhf; fhhf = next_hhf )
      {
         next_hhf = fhhf->lnext;
         if( !fhhf->who )
            continue;
         if( str_cmp( fhhf->name, ch->name ) )
            continue;
         fhhf->who = NULL;
         if( fhhf->mob && fhhf->mob->summoning && fhhf->mob->summoning == ch )
            fhhf->mob->summoning = NULL;
      }
   }
}

/* Check to see if they are being hunted, hated, feared and if so set to the new char */
void fix_hhf( CHAR_DATA *ch )
{
   HHF_DATA *fhhf, *next_hhf, *tmp_fhhf;
   short tmp;

   if( !ch || ch->name == NULL || ch->name[0] == '\0' )
      return;
   for( tmp = 0; tmp < 3; tmp++ )
   {
      if( tmp == 0 )
         tmp_fhhf = first_hating;
      else if( tmp == 1 )
         tmp_fhhf = first_hunting;
      else if( tmp == 2 )
         tmp_fhhf = first_fearing;
      else
         return;

      for( fhhf = tmp_fhhf; fhhf; fhhf = next_hhf )
      {
         next_hhf = fhhf->lnext;
         if( fhhf->who )
            continue;
         if( str_cmp( fhhf->name, ch->name ) )
            continue;
         fhhf->who = ch;
      }
   }
}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 * Many hours spent fixing bugs in here by Thoric, as noted by residual
 * debugging checks.  If you never get any of these error messages again
 * in your logs... then you can comment out some of the checks without
 * worry.
 *
 * Note:  This function also handles some non-violence updates.
 */
void violence_update( void )
{
   CHAR_DATA *ch, *lst_ch, *victim, *rch, *rch_next;
   AFFECT_DATA *paf, *paf_next;
   TIMER *timer, *timer_next;
   SKILLTYPE *skill;
   ch_ret retcode;
   static int pulse = 0;
   int attacktype, cnt;

   lst_ch = NULL;
   pulse = ( pulse + 1 ) % 100;

   for( ch = last_char; ch; lst_ch = ch, ch = gch_prev )
   {
      set_cur_char( ch );

      if( ch == first_char && ch->prev )
      {
         bug( "%s", "ERROR: first_char->prev isn't NULL, fixing..." );
         ch->prev = NULL;
      }

      gch_prev = ch->prev;

      if( gch_prev && gch_prev->next != ch )
      {
         bug( "FATAL: %s: %s->prev->next doesn't point to ch.", __FUNCTION__,ch->name );
         bug( "%s", "Short-cutting here" );
         ch->prev = NULL;
         gch_prev = NULL;
         interpret( ch, (char *)"chat The Immortals say, 'Prepare for the worst!'" );
      }

      /*
       * See if we got a pointer to someone who recently died...
       * if so, either the pointer is bad... or it's a player who
       * "died", and is back at the healer...
       * Since he/she's in the char_list, it's likely to be the later...
       * and shouldn't already be in another fight already
       */
      if( char_died( ch ) )
         continue;

      /* See if we got a pointer to some bad looking data... */
      if( !ch->in_room || !ch->name )
      {
         log_string( "violence_update: bad ch record!  (Shortcutting.)" );
         log_printf( "ch: %ld  ch->in_room: %ld  ch->prev: %ld  ch->next: %ld",
             ( long )ch, ( long )ch->in_room, ( long )ch->prev, ( long )ch->next );
         log_string( lastplayercmd );
         if( lst_ch )
            log_printf( "lst_ch: %ld  lst_ch->prev: %ld  lst_ch->next: %ld",
                ( long )lst_ch, ( long )lst_ch->prev, ( long )lst_ch->next );
         else
            log_string( "lst_ch: NULL" );
         gch_prev = NULL;
         continue;
      }

      for( timer = ch->first_timer; timer; timer = timer_next )
      {
         timer_next = timer->next;

         if( --timer->count <= 0 )
         {
            if( timer->type == TIMER_ASUPPRESSED )
            {
               if( timer->value == -1 )
               {
                  timer->count = 1000;
                  continue;
               }
            }

            if( timer->type == TIMER_DO_FUN )
            {
               int tempsub;

               tempsub = ch->substate;
               ch->substate = timer->value;
               ( timer->do_fun ) ( ch, (char *)"" );
               if( char_died( ch ) )
                  break;
               ch->substate = tempsub;
            }
            if( timer->count <= 0 )
               extract_timer( ch, timer );
         }
      }

      if( char_died( ch ) )
         continue;

      /*
       * We need spells that have shorter durations than an hour.
       * So a melee round sounds good to me... -Thoric
       */
      for( paf = ch->first_affect; paf; paf = paf_next )
      {
         paf_next = paf->next;
         if( paf->duration > 0 )
            paf->duration--;
         else if( paf->duration == 0 )
         {
            if( !paf_next || paf_next->type != paf->type || paf_next->duration > 0 )
            {
               skill = get_skilltype( paf->type );
               if( paf->type > 0 && skill && skill->msg_off )
               {
                  set_char_color( AT_WEAROFF, ch );
                  send_to_char( skill->msg_off, ch );
                  send_to_char( "\r\n", ch );
               }
            }
            affect_remove( ch, paf );
         }
      }

      if( char_died( ch ) )
         continue;

      /* check for exits moving players around */
      if( ( retcode = pullcheck( ch, pulse ) ) == rCHAR_DIED || char_died( ch ) )
         continue;

      /* Let the battle begin! */
      if( !( victim = who_fighting( ch ) ) || IS_AFFECTED( ch, AFF_PARALYSIS ) )
         continue;

      retcode = rNONE;

      if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
      {
         log_printf( "%s: %s fighting %s in a SAFE room.", __FUNCTION__, ch->name, victim->name );
         stop_fighting( ch, true );
      }
      else if( is_awake( ch ) && ch->in_room == victim->in_room )
      {
         retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
         if( char_died( ch ) || char_died( victim ) )
            stop_fighting( ch, false );
      }
      else
         stop_fighting( ch, false );

      if( char_died( ch ) )
         continue;

      if( retcode == rCHAR_DIED || !( victim = who_fighting( ch ) ) )
         continue;

      /*
       *  Mob triggers
       *  -- Added some victim death checks, because it IS possible.. -- Alty
       */
      rprog_rfight_trigger( ch );
      if( char_died( ch ) || char_died( victim ) )
         continue;

      mprog_hitprcnt_trigger( ch, victim );
      if( char_died( ch ) || char_died( victim ) )
         continue;

      mprog_fight_trigger( ch, victim );
      if( char_died( ch ) || char_died( victim ) )
         continue;

      oprog_fight_trigger( ch );
      if( char_died( ch ) || char_died( victim ) )
         continue;

      /* NPC special attack flags - Thoric */
      if( is_npc( ch ) )
      {
         if( !xIS_EMPTY( ch->attacks ) )
         {
            attacktype = -1;
            if( 30 + ( ch->level / 4 ) >= number_percent( ) )
            {
               cnt = 0;
               for( ;; )
               {
                  if( cnt++ > 10 )
                  {
                     attacktype = -1;
                     break;
                  }
                  attacktype = number_range( 7, ATCK_MAX - 1 );
                  if( xIS_SET( ch->attacks, attacktype ) )
                     break;
               }
               switch( attacktype )
               {
                  case ATCK_BASH:
                     do_bash( ch, (char *)"" );
                     retcode = global_retcode;
                     break;

                  case ATCK_STUN:
                     do_stun( ch, (char *)"" );
                     retcode = global_retcode;
                     break;

                  case ATCK_GOUGE:
                     do_gouge( ch, (char *)"" );
                     retcode = global_retcode;
                     break;

                  case ATCK_FEED:
                     do_feed( ch, (char *)"" );
                     retcode = global_retcode;
                     break;

                  case ATCK_DRAIN:
                     retcode = spell_energy_drain( skill_lookup( "energy drain" ), ch->level, ch, victim );
                     break;

                  case ATCK_FIREBREATH:
                     retcode = spell_smaug( skill_lookup( "fire breath" ), ch->level, ch, victim );
                     break;

                  case ATCK_FROSTBREATH:
                     retcode = spell_smaug( skill_lookup( "frost breath" ), ch->level, ch, victim );
                     break;

                  case ATCK_ACIDBREATH:
                     retcode = spell_smaug( skill_lookup( "acid breath" ), ch->level, ch, victim );
                     break;

                  case ATCK_LIGHTNBREATH:
                     retcode = spell_smaug( skill_lookup( "lightning breath" ), ch->level, ch, victim );
                     break;

                  case ATCK_GASBREATH:
                     retcode = spell_smaug( skill_lookup( "gas breath" ), ch->level, ch, victim );
                     break;

                  case ATCK_POISON:
                     retcode = spell_smaug( gsn_poison, ch->level, ch, victim );
                     break;

                  case ATCK_BLINDNESS:
                     retcode = spell_smaug( gsn_blindness, ch->level, ch, victim );
                     break;

                  case ATCK_CAUSESERIOUS:
                     retcode = spell_smaug( skill_lookup( "cause serious" ), ch->level, ch, victim );
                     break;

                  case ATCK_EARTHQUAKE:
                     retcode = spell_earthquake( skill_lookup( "earthquake" ), ch->level, ch, victim );
                     break;

                  case ATCK_CAUSECRITICAL:
                     retcode = spell_smaug( skill_lookup( "cause critical" ), ch->level, ch, victim );
                     break;

                  case ATCK_CURSE:
                     retcode = spell_smaug( skill_lookup( "curse" ), ch->level, ch, victim );
                     break;

                  case ATCK_FIREBALL:
                     retcode = spell_smaug( skill_lookup( "fireball" ), ch->level, ch, victim );
                     break;
               }
               if( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
                  continue;
            }
         }

         /* NPC special defense flags - Thoric */
         if( !xIS_EMPTY( ch->defenses ) )
         {
            attacktype = -1;
            if( 50 + ( ch->level / 4 ) > number_percent( ) )
            {
               cnt = 0;
               for( ;; )
               {
                  if( cnt++ > 10 )
                  {
                     attacktype = -1;
                     break;
                  }
                  attacktype = number_range( 2, DFND_MAX - 1 );
                  if( xIS_SET( ch->defenses, attacktype ) )
                     break;
               }

               switch( attacktype )
               {
                  case DFND_CURELIGHT:
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks a little better.", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "cure light" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_CURESERIOUS:
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks a bit better.", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_CURECRITICAL:
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks healthier.", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_HEAL:
                     if( ch->hit < ch->max_hit )
                     {
                        act( AT_MAGIC, "$n mutters a few incantations...and looks much healthier.", ch, NULL, NULL,
                             TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "heal" ), ch->level, ch, ch );
                     }
                     break;

                  case DFND_DISPELMAGIC:
                     if( victim->first_affect )
                     {
                        act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_dispel_magic( skill_lookup( "dispel magic" ), ch->level, ch, victim );
                     }
                     break;

                  case DFND_DISPELEVIL:
                     act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                     retcode = spell_dispel_evil( skill_lookup( "dispel evil" ), ch->level, ch, victim );
                     break;

                  case DFND_TELEPORT:
                     retcode = spell_teleport( skill_lookup( "teleport" ), ch->level, ch, ch );
                     break;

                  case DFND_SHOCKSHIELD:
                     if( !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "shockshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_VENOMSHIELD:
                     if( !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "venomshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_ACIDMIST:
                     if( !IS_AFFECTED( ch, AFF_ACIDMIST ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "acidmist" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_FIRESHIELD:
                     if( !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "fireshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_ICESHIELD:
                     if( !IS_AFFECTED( ch, AFF_ICESHIELD ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "iceshield" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;

                  case DFND_TRUESIGHT:
                     if( !IS_AFFECTED( ch, AFF_TRUESIGHT ) )
                        retcode = spell_smaug( skill_lookup( "true" ), ch->level, ch, ch );
                     else
                        retcode = rNONE;
                     break;

                  case DFND_SANCTUARY:
                     if( !IS_AFFECTED( ch, AFF_SANCTUARY ) )
                     {
                        act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                        retcode = spell_smaug( skill_lookup( "sanctuary" ), ch->level, ch, ch );
                     }
                     else
                        retcode = rNONE;
                     break;
               }
               if( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
                  continue;
            }
         }
      }

      /* Fun for the whole family! */
      for( rch = ch->in_room->first_person; rch; rch = rch_next )
      {
         rch_next = rch->next_in_room;

         if( ( !is_npc( ch ) && !is_npc( rch ) )
         && ( rch != ch )
         && ( rch->fighting )
         && ( who_fighting( rch->fighting->who ) == ch ) )
         {
            int curr, rcurr;

            rcurr = ( rch->level
                    + ( ( rch->hit > 0 ) ? ( ( 100 * rch->hit ) / rch->max_hit ) : 0 )
                    + ( ( rch->move > 0 ) ? ( ( 100 * rch->move ) / rch->max_move ) : 0 ) );

            curr =  ( ch->level
                    + ( ( ch->hit > 0 ) ? ( ( 100 * ch->hit ) / ch->max_hit ) : 0 )
                    + ( ( ch->move > 0 ) ? ( ( 100 * ch->move ) / ch->max_move ) : 0 ) );
            if( rcurr < curr )
            {
               rch->fighting->who->fighting->who = rch;
            }
         }

         if( is_awake( rch ) && !rch->fighting )
         {
            /* PC's auto-assist others in their group. */
            if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
            {
               if( ( ( !is_npc( rch ) && rch->desc ) || IS_AFFECTED( rch, AFF_CHARM ) )
               && is_same_group( ch, rch ) && !is_safe( rch, victim, true ) )
                  multi_hit( rch, victim, TYPE_UNDEFINED );
               continue;
            }

            /* NPC's assist NPC's of same type or 12.5% chance regardless. */
            if( is_npc( rch ) && !IS_AFFECTED( rch, AFF_CHARM ) && !xIS_SET( rch->act, ACT_NOASSIST )
            && !xIS_SET( rch->act, ACT_PET ) )
            {
               if( char_died( ch ) )
                  break;
               if( rch->pIndexData == ch->pIndexData || number_bits( 3 ) == 0 )
               {
                  CHAR_DATA *vch;
                  CHAR_DATA *target;
                  int number;

                  target = NULL;
                  number = 0;
                  for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
                  {
                     if( can_see( rch, vch ) && is_same_group( vch, victim ) && number_range( 0, number ) == 0 )
                     {
                        if( vch->mount && vch->mount == rch )
                           target = NULL;
                        else
                        {
                           target = vch;
                           number++;
                        }
                     }
                  }
                  if( target )
                     multi_hit( rch, target, TYPE_UNDEFINED );
               }
            }
         }
      }
   }
}

int get_num_attacks( CHAR_DATA *ch )
{
   int udex;

   if( !ch )
      return 0;

   if( is_npc( ch ) )
      return ch->numattacks;
  udex = get_curr_stat( STAT_DEX, ch );
  return URANGE( 0, ( ( udex > 0 ) ? ( udex / 5 ) : 2 ), 10 );
}

/* Do one group of attacks. */
ch_ret multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
   OBJ_DATA *obj;
   int schance, wearloc, numattacks;
   ch_ret retcode;

   /* add timer to pkillers */
   if( !is_npc( ch ) && !is_npc( victim ) )
   {
      if( xIS_SET( ch->act, PLR_NICE ) )
         return rNONE;
      add_timer( ch, TIMER_RECENTFIGHT, 11, NULL, 0 );
      add_timer( victim, TIMER_RECENTFIGHT, 11, NULL, 0 );
   }

   if( is_attack_suppressed( ch ) )
      return rNONE;

   if( is_npc( ch ) && xIS_SET( ch->act, ACT_NOATTACK ) )
      return rNONE;

   if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE )
      return retcode;

   if( who_fighting( ch ) != victim || dt == gsn_backstab || dt == gsn_circle )
      return rNONE;

   schance = is_npc( ch ) ? 100 : ( LEARNED( ch, gsn_berserk ) * 5 / 2 );
   if( IS_AFFECTED( ch, AFF_BERSERK ) && number_percent( ) < schance )
      if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE || who_fighting( ch ) != victim )
         return retcode;

   for( wearloc = 0; wearloc < WEAR_MAX; wearloc++ )
   {
      if( number_percent( ) > 25 )
         continue;
      if( ( obj = get_eq_char( ch, wearloc ) ) && obj->item_type == ITEM_WEAPON )
         if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE || who_fighting( ch ) != victim )
            return retcode;
   }

   numattacks = get_num_attacks( ch );
   for( schance = 0; schance < numattacks; schance++ )
   {
      if( is_npc( ch ) && ch->move < 10 )
         break;
      retcode = one_hit( ch, victim, dt );
      if( retcode != rNONE || who_fighting( ch ) != victim )
         return retcode;
   }

   retcode = rNONE;

   schance = is_npc( ch ) ? ( int )( ch->level / 2 ) : 0;
   if( number_percent( ) < schance )
      retcode = one_hit( ch, victim, dt );

   if( retcode == rNONE )
   {
      int move = 0;

      if( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLOATING ) )
         move = encumbrance( ch, movement_loss[ URANGE( 0, ch->in_room->sector_type, SECT_MAX - 1 ) ] );
      else
         move = encumbrance( ch, 1 );
      if( ch->move )
         ch->move = UMAX( 0, ch->move - move );
   }
   return retcode;
}

int barehanded_prof_bonus_check( CHAR_DATA *ch, int *gsn_ptr )
{
   int bonus = 0;

   *gsn_ptr = -1;

   if( is_npc( ch ) )
      return bonus;

   *gsn_ptr = gsn_barehanded;
   if( *gsn_ptr != -1 )
      bonus = ( int )( ( LEARNED( ch, *gsn_ptr ) ) / 10 );
   return bonus;
}

/* Weapon types, haus */
int weapon_prof_bonus_check( CHAR_DATA *ch, OBJ_DATA *wield, int *gsn_ptr )
{
   int bonus;

   bonus = 0;
   *gsn_ptr = -1;
   if( !is_npc( ch ) && ch->level > 5 )
   {
      if( !wield )
         *gsn_ptr = gsn_barehanded;
      else if( wield )
      {
         switch( wield->value[3] )
         {
            default:
               *gsn_ptr = -1;
               break;

            case DAM_HIT:
            case DAM_SUCTION:
            case DAM_BITE:
            case DAM_BLAST:
               *gsn_ptr = gsn_pugilism;
               break;

            case DAM_SLASH:
            case DAM_SLICE:
               *gsn_ptr = gsn_long_blades;
               break;

            case DAM_PIERCE:
            case DAM_STAB:
               *gsn_ptr = gsn_short_blades;
               break;

            case DAM_WHIP:
               *gsn_ptr = gsn_flexible_arms;
               break;

            case DAM_CLAW:
               *gsn_ptr = gsn_talonous_arms;
               break;

            case DAM_POUND:
            case DAM_CRUSH:
               *gsn_ptr = gsn_bludgeons;
               break;

            case DAM_BOLT:
            case DAM_ARROW:
            case DAM_DART:
            case DAM_STONE:
            case DAM_PEA:
               *gsn_ptr = gsn_missile_weapons;
               break;
         }
      }
      if( *gsn_ptr != -1 )
         bonus = ( int )( ( LEARNED( ch, *gsn_ptr ) - 50 ) / 10 );
   }
   return bonus;
}

/* Offensive shield level modifier */
short off_shld_lvl( CHAR_DATA *ch, CHAR_DATA *victim )
{
   short lvl;

   if( !is_npc( ch ) )  /* players get much less effect */
   {
      lvl = UMAX( 1, ( ch->level - 10 ) / 2 );
      if( number_percent( ) + ( victim->level - lvl ) < 40 )
      {
         if( can_pkill( ch ) && can_pkill( victim ) )
            return ch->level;
         else
            return lvl;
      }
      else
         return 0;
   }
   else
   {
      lvl = ch->level / 2;
      if( number_percent( ) + ( victim->level - lvl ) < 70 )
         return lvl;
      else
         return 0;
   }
}

/* Hit one guy once. */
ch_ret one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
   OBJ_DATA *wield = NULL;
   static int lastwused = 0;
   int victim_ac, plusris, dam, diceroll, attacktype, cnt, prof_bonus, prof_gsn = -1, onwused;
   ch_ret retcode = rNONE;

   /*
    * Can't beat a dead char!
    * Guard against weird room-leavings.
    */
   if( victim->position == POS_DEAD || ch->in_room != victim->in_room )
      return rVICT_DIED;

   used_weapon = NULL;
   for( onwused = 0; onwused < WEAR_MAX; onwused++ )
   {
      if( onwused <= lastwused )
         continue;
      if( !( wield = get_eq_char( ch, onwused ) ) || wield->item_type != ITEM_WEAPON )
         continue;
      break;
   }
   /* No weapon to use? Use last one if possible */
   if( !wield )
   {
      if( ( wield = get_eq_char( ch, lastwused ) ) )
         if( wield->item_type != ITEM_WEAPON )
            wield = NULL;
   }
   lastwused = onwused;
   if( lastwused >= WEAR_MAX )
      lastwused = 0;
   used_weapon = wield;

   if( wield )
   {
      /* If it takes all your current strength to wield the weapon you can gain more strength over time */
      if( wield->stat_reqs[STAT_STR] >= ch->perm_stats[STAT_STR] )
         handle_stat( ch, STAT_STR, true, 1 );
      prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
   }
   else
      prof_bonus = barehanded_prof_bonus_check( ch, &prof_gsn );

   if( ch->fighting  /* make sure fight is already started */
   && dt == TYPE_UNDEFINED && is_npc( ch ) && !xIS_EMPTY( ch->attacks ) )
   {
      cnt = 0;
      for( ;; )
      {
         attacktype = number_range( 0, 6 );
         if( xIS_SET( ch->attacks, attacktype ) )
            break;
         if( cnt++ > 16 )
         {
            attacktype = -1;
            break;
         }
      }
      if( attacktype == ATCK_BACKSTAB )
         attacktype = -1;
      if( wield && number_percent( ) > 25 )
         attacktype = -1;
      if( !wield && number_percent( ) > 50 )
         attacktype = -1;

      switch( attacktype )
      {
         default:
            break;

         case ATCK_BITE:
            retcode = spell_smaug( gsn_bite, ch->level, ch, victim );
            break;

         case ATCK_CLAWS:
            retcode = spell_smaug( gsn_claw, ch->level, ch, victim );
            break;

         case ATCK_TAIL:
            retcode = spell_smaug( gsn_tail, ch->level, ch, victim );
            break;

         case ATCK_PUNCH:
            retcode = spell_smaug( gsn_punch, ch->level, ch, victim );
            break;

         case ATCK_KICK:
            retcode = spell_smaug( gsn_kick, ch->level, ch, victim );
            break;

         case ATCK_TRIP:
            attacktype = 0;
            break;
      }
      if( attacktype >= 0 )
         return retcode;
   }

   if( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if( wield && wield->item_type == ITEM_WEAPON )
         dt += wield->value[3];
   }

   victim_ac = hit_vict_chance( ch, victim, wield );

   /* Weapon proficiency bonus */
   victim_ac -= prof_bonus;

   /* The moment of excitement! */
   while( ( diceroll = number_bits( 5 ) ) >= 20 )
      ;

   if( diceroll != 19 ) /* 19 skips ac and hits */
   {
      /* 0 and lower is always a fail, victim_ac less then diceroll is a fail */
      if( diceroll <= 0 || victim_ac < diceroll )
      {
         /* Miss. */
         if( prof_gsn != -1 )
            learn_from_failure( ch, prof_gsn );
         damage( ch, victim, wield, 0, dt, true );
         tail_chain( );
         return rNONE;
      }
   }

   /* Hit. Calc damage. */
   dam = 0;

   /* Weapon damage */
   if( wield )
      dam += number_range( wield->value[1], wield->value[2] );

   /* Bonuses. */
   dam += get_damroll( ch );

   if( prof_bonus )
      dam += prof_bonus / 4;

   if( victim != ch )
   {
      dam = get_style_mod( ch, victim, dam );

      /* Auto increase styles */
      if( !is_npc( ch ) )
      {
         switch( ch->position )
         {
            case POS_EVASIVE:
               if( ch->pcdata->learned[gsn_style_evasive] > 0 && number_percent( ) < LEARNED( ch, gsn_style_evasive ) )
                  learn_from_success( ch, gsn_style_evasive );
               break;

            case POS_DEFENSIVE:
               if( ch->pcdata->learned[gsn_style_defensive] > 0 && number_percent( ) < LEARNED( ch, gsn_style_defensive ) )
                  learn_from_success( ch, gsn_style_defensive );
               break;

            case POS_FIGHTING:
               if( ch->pcdata->learned[gsn_style_standard] > 0 && number_percent( ) < LEARNED( ch, gsn_style_standard ) )
                  learn_from_success( ch, gsn_style_standard );
               break;

            case POS_AGGRESSIVE:
               if( ch->pcdata->learned[gsn_style_aggressive] > 0 && number_percent( ) < LEARNED( ch, gsn_style_aggressive ) )
                  learn_from_success( ch, gsn_style_aggressive );
               break;

           case POS_BERSERK:
              if( ch->pcdata->learned[gsn_style_berserk] > 0 && number_percent( ) < LEARNED( ch, gsn_style_berserk ) )
                 learn_from_success( ch, gsn_style_berserk );
              break;

           default:
              break;
         }
      }
   }

   if( !is_npc( ch ) && ch->pcdata->learned[gsn_enhanced_damage] > 0 )
   {
      int maxenhance = ( get_adept( ch, gsn_enhanced_damage ) + 20 );

      dam += ( int )( ( dam * LEARNED( ch, gsn_enhanced_damage ) ) / maxenhance );
      learn_from_success( ch, gsn_enhanced_damage );
   }

   if( !is_awake( victim ) )
      dam *= 2;
   if( dt == gsn_backstab )
      dam *= ( 2 + URANGE( 2, ch->level - ( victim->level / 4 ), 30 ) / 8 );
   if( dt == gsn_circle )
      dam *= ( 2 + URANGE( 2, ch->level - ( victim->level / 4 ), 30 ) / 16 );

   if( dam <= 0 )
      dam = 1;

   plusris = 0;

   if( prof_gsn != -1 )
   {
      if( dam > 0 )
         learn_from_success( ch, prof_gsn );
      else
         learn_from_failure( ch, prof_gsn );
   }

   if( ( retcode = damage( ch, victim, wield, dam, dt, true ) ) != rNONE )
      return retcode;
   if( char_died( ch ) )
      return rCHAR_DIED;
   if( char_died( victim ) )
      return rVICT_DIED;

   retcode = rNONE;
   if( dam == 0 )
      return retcode;

   /*
    * Weapon spell support            -Thoric
    * Each successful hit casts a spell
    */
   if( wield && !xIS_SET( victim->in_room->room_flags, ROOM_NO_MAGIC ) )
   {
      AFFECT_DATA *aff;
      int affx;

      for( affx = 0; affx < 2; affx++ )
      {
         if( affx == 0 )
            aff = wield->pIndexData->first_affect;
         else if( affx == 1 )
            aff = wield->first_affect;
         else
            break;
         for( ; aff; aff = aff->next )
         {
            if( ( aff->location % REVERSE_APPLY ) == APPLY_WEAPONSPELL && is_valid_sn( aff->modifier ) && skill_table[aff->modifier]->spell_fun )
               retcode = ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, ( wield->level + 3 ) / 3, ch, victim );
            if( ( retcode != rSPELL_FAILED && retcode != rNONE ) || char_died( ch ) || char_died( victim ) )
               return retcode;
         }
      }
   }

   /* magic shields that retaliate - Thoric */
   if( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
      retcode = spell_smaug( skill_lookup( "flare" ), off_shld_lvl( victim, ch ), victim, ch );
   if( ( retcode != rSPELL_FAILED && retcode != rNONE ) || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( ch, AFF_ICESHIELD ) )
      retcode = spell_smaug( skill_lookup( "iceshard" ), off_shld_lvl( victim, ch ), victim, ch );
   if( ( retcode != rSPELL_FAILED && retcode != rNONE ) || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
      retcode = spell_smaug( skill_lookup( "torrent" ), off_shld_lvl( victim, ch ), victim, ch );
   if( ( retcode != rSPELL_FAILED && retcode != rNONE ) || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_ACIDMIST ) && !IS_AFFECTED( ch, AFF_ACIDMIST ) )
      retcode = spell_smaug( skill_lookup( "acidshot" ), off_shld_lvl( victim, ch ), victim, ch );
   if( ( retcode != rSPELL_FAILED && retcode != rNONE ) || char_died( ch ) || char_died( victim ) )
      return retcode;

   if( IS_AFFECTED( victim, AFF_VENOMSHIELD ) && !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
      retcode = spell_smaug( skill_lookup( "venomshot" ), off_shld_lvl( victim, ch ), victim, ch );
   if( ( retcode != rSPELL_FAILED && retcode != rNONE ) || char_died( ch ) || char_died( victim ) )
      return retcode;

   tail_chain( );
   return retcode;
}

bool pierce_equipment( CHAR_DATA *ch, int iWear, int dam )
{
   OBJ_DATA *obj, *obj_next;

   for( obj = ch->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;

      if( obj->wear_loc == iWear
      || ( ( iWear == WEAR_EAR_L || iWear == WEAR_EAR_R ) && obj->wear_loc == WEAR_EARS )
      || ( ( iWear == WEAR_EYE_L || iWear == WEAR_EYE_R ) && obj->wear_loc == WEAR_EYES )
      || ( ( iWear == WEAR_SHOULDER_L || iWear == WEAR_SHOULDER_R ) && obj->wear_loc == WEAR_SHOULDERS )
      || ( ( iWear == WEAR_ARM_L || iWear == WEAR_ARM_R ) && obj->wear_loc == WEAR_ARMS )
      || ( ( iWear == WEAR_WRIST_L || iWear == WEAR_WRIST_R ) && obj->wear_loc == WEAR_WRISTS )
      || ( ( iWear == WEAR_HAND_L || iWear == WEAR_HAND_R ) && obj->wear_loc == WEAR_HANDS )
      || ( ( iWear == WEAR_LEG_L || iWear == WEAR_LEG_R ) && obj->wear_loc == WEAR_LEGS )
      || ( ( iWear == WEAR_ANKLE_L || iWear == WEAR_ANKLE_R ) && obj->wear_loc == WEAR_ANKLES )
      || ( ( iWear == WEAR_FOOT_L || iWear == WEAR_FOOT_R ) && obj->wear_loc == WEAR_FEET ) )
      {
         if( dam < get_obj_resistance( obj ) )
            return false;
         if( is_obj_stat( obj, ITEM_PIERCED ) || is_obj_stat( obj, ITEM_LODGED ) )
            return false;
         xSET_BIT( obj->extra_flags, ITEM_PIERCED );
         damage_obj( obj );
      }
   }
   return true;
}

void lodge_projectile( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *projectile, int dam )
{
   OBJ_DATA *eqloc;
   short dameq;

   /* Lets allow an arrow to get stuck in a random eq part if not protected */
   dameq = number_range( 0, WEAR_MAX - 1 );

   /* Lets limit what positions it can be stuck in */
   if( dameq == WEAR_EARS || dameq == WEAR_EYES || dameq == WEAR_SHOULDERS
   || dameq == WEAR_ABOUT || dameq == WEAR_ARMS || dameq == WEAR_WRISTS
   || dameq == WEAR_HANDS || dameq == WEAR_FINGERS || dameq == WEAR_FINGER_L
   || dameq == WEAR_FINGER_R || dameq == WEAR_HOLD_B
   || dameq == WEAR_HOLD_L || dameq == WEAR_HOLD_R || dameq == WEAR_LEGS
   || dameq == WEAR_ANKLES || dameq == WEAR_FEET )
   {
      extract_obj( projectile );
      return;
   }

   /* Block it with a shield */
   if( check_shieldblock( ch, victim ) )
   {
      extract_obj( projectile );
      return;
   }

   if( ( eqloc = get_eq_location( victim, dameq ) ) )
   {
      if( pierce_equipment( victim, dameq, dam ) )
      {
         act( AT_CARNAGE, "$n suffers as $p gets stuck in $m.", victim, projectile, NULL, TO_ROOM );
         act( AT_CARNAGE, "You suffer as $p gets stuck in you.", victim, projectile, NULL, TO_CHAR );
         if( victim->in_room != ch->in_room )
            act( AT_CARNAGE, "$n suffers as $p gets stuck in $m.", victim, projectile, ch, TO_VICT );
         xSET_BIT( projectile->extra_flags, ITEM_LODGED );
         obj_to_char( projectile, victim );
         equip_char( victim, projectile, dameq );
      }
      else
      {
         depierce_equipment( victim, dameq );
         extract_obj( projectile );
      }
      return;
   }

   /* Now lets lodge it in the location */
   act( AT_CARNAGE, "$n suffers as $p gets stuck in $m.", victim, projectile, NULL, TO_ROOM );
   act( AT_CARNAGE, "You suffer as $p gets stuck in you.", victim, projectile, NULL, TO_CHAR );
   if( victim->in_room != ch->in_room )
      act( AT_CARNAGE, "$n suffers as $p gets stuck in $m.", victim, projectile, ch, TO_VICT );
   xSET_BIT( projectile->extra_flags, ITEM_LODGED );
   obj_to_char( projectile, victim );
   equip_char( victim, projectile, dameq );
}

/*
 * Hit one guy with a projectile.
 * Handles use of missile weapons (wield = missile weapon)
 * or thrown items/weapons
 */
ch_ret projectile_hit( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, OBJ_DATA *projectile, short dist )
{
   int victim_ac, plusris, dam, diceroll, prof_bonus, prof_gsn = -1, proj_bonus, dt;
   ch_ret retcode;

   if( !projectile )
      return rNONE;

   if( projectile->item_type == ITEM_PROJECTILE || projectile->item_type == ITEM_WEAPON )
   {
      dt = TYPE_HIT + projectile->value[3];
      proj_bonus = number_range( projectile->value[1], projectile->value[2] );
   }
   else
   {
      dt = TYPE_UNDEFINED;
      proj_bonus = number_range( 1, URANGE( 2, get_obj_weight( projectile ), 100 ) );
   }

   /* Can't beat a dead char! */
   if( victim->position == POS_DEAD || char_died( victim ) )
   {
      if( projectile->in_obj )
         obj_from_obj( projectile );
      if( projectile->carried_by )
         obj_from_char( projectile );
      extract_obj( projectile );
      return rVICT_DIED;
   }

   if( wield )
      prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
   else
      prof_bonus = barehanded_prof_bonus_check( ch, &prof_gsn );

   if( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if( wield && wield->item_type == ITEM_MISSILE_WEAPON )
         dt += wield->value[3];
   }

   victim_ac = hit_vict_chance( ch, victim, wield );

   /* Weapon proficiency bonus */
   victim_ac -= prof_bonus;

   /* The moment of excitement! */
   while( ( diceroll = number_bits( 5 ) ) >= 20 )
      ;

   if( diceroll == 0 || ( diceroll != 19 && victim_ac < diceroll ) )
   {
      /* Miss. */
      if( prof_gsn != -1 )
         learn_from_failure( ch, prof_gsn );

      if( projectile->in_obj )
         obj_from_obj( projectile );
      if( projectile->carried_by )
         obj_from_char( projectile );
      extract_obj( projectile );
      damage( ch, victim, wield, 0, dt, false ); /* Projectile is already gone so just using wield, 0 damage anyways */
      tail_chain( );
      return rNONE;
   }

   /* Hit. Calc damage. */
   if( !wield )   /* dice formula fixed by Thoric */
      dam = proj_bonus;
   else
      dam = number_range( wield->value[1], wield->value[2] ) + ( proj_bonus / 10 );

   /* Bonuses. */
   dam += get_damroll( ch );

   if( prof_bonus )
      dam += prof_bonus / 4;

   dam = get_style_mod( ch, victim, dam );

   if( !is_npc( ch ) && ch->pcdata->learned[gsn_enhanced_damage] > 0 )
   {
      int maxenhance = ( get_adept( ch, gsn_enhanced_damage ) + 20 );

      dam += ( int )( ( dam * LEARNED( ch, gsn_enhanced_damage ) ) / maxenhance );
      learn_from_success( ch, gsn_enhanced_damage );
   }

   if( !is_awake( victim ) )
      dam *= 2;

   if( dam <= 0 )
      dam = 1;

   plusris = 0;

   if( prof_gsn != -1 )
   {
      if( dam > 0 )
         learn_from_success( ch, prof_gsn );
      else
         learn_from_failure( ch, prof_gsn );
   }

   if( ( retcode = damage( ch, victim, projectile, dam, dt, true ) ) != rNONE )
   {
      if( projectile->in_obj )
         obj_from_obj( projectile );
      if( projectile->carried_by )
         obj_from_char( projectile );
      extract_obj( projectile );
      return retcode;
   }

   if( char_died( ch ) )
   {
      if( projectile->in_obj )
         obj_from_obj( projectile );
      if( projectile->carried_by )
         obj_from_char( projectile );
      extract_obj( projectile );
      return rCHAR_DIED;
   }

   if( char_died( victim ) )
   {
      if( projectile->in_obj )
         obj_from_obj( projectile );
      if( projectile->carried_by )
         obj_from_char( projectile );
      extract_obj( projectile );
      return rVICT_DIED;
   }

   retcode = rNONE;
   if( dam == 0 )
   {
      if( projectile->in_obj )
         obj_from_obj( projectile );
      if( projectile->carried_by )
         obj_from_char( projectile );
      extract_obj( projectile );
      return retcode;
   }

   if( wield && !xIS_SET( victim->in_room->room_flags, ROOM_NO_MAGIC ) )
   {
      AFFECT_DATA *aff;
      int affx;

      for( affx = 0; affx < 2; affx++ )
      {
         if( affx == 0 )
            aff = wield->pIndexData->first_affect;
         else if( affx == 1 )
            aff = wield->first_affect;
         else
            break;
         for( ; aff; aff = aff->next )
         {
            if( ( aff->location % REVERSE_APPLY ) == APPLY_WEAPONSPELL && is_valid_sn( aff->modifier ) && skill_table[aff->modifier]->spell_fun )
               retcode = ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, ( wield->level + 3 ) / 3, ch, victim );
            if( ( retcode != rSPELL_FAILED && retcode != rNONE ) || char_died( ch ) || char_died( victim ) )
            {
               if( projectile->in_obj )
                  obj_from_obj( projectile );
               if( projectile->carried_by )
                  obj_from_char( projectile );
               extract_obj( projectile );
               return retcode;
            }
         }
      }
   }

   if( projectile->in_obj )
      obj_from_obj( projectile );
   if( projectile->carried_by )
      obj_from_char( projectile );
   if( projectile && wield )
      lodge_projectile( ch, victim, projectile, dam );
   else
      extract_obj( projectile );

   tail_chain( );
   return retcode;
}

/* Calculate damage based on resistances, immunities and suceptibilities - Thoric */
int ris_damage( CHAR_DATA *ch, short dam, int ris )
{
   return ch->resistant[ris];
}

/* Inflict damage from a hit. This is one damn big function. */
ch_ret damage( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon, int dam, int dt, bool candameq )
{
   OBJ_DATA *damobj = NULL;
   CHAR_DATA *gch /*, *lch */ ;
   char log_buf[MSL];
   int olddam;
   short dameq, maxdam, anopc = 0, bnopc = 0, rismod = 0;
   ch_ret retcode;
   bool npcvict, loot, endfight = false;

   retcode = rNONE;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return rERROR;
   }
   if( !victim )
   {
      bug( "%s: null victim!", __FUNCTION__ );
      return rVICT_DIED;
   }

   if( victim->position == POS_DEAD )
      return rVICT_DIED;

   npcvict = is_npc( victim );

   /* Check damage types for RIS -Thoric */
   if( dam && dt != TYPE_UNDEFINED )
   {
      olddam = dam;

      if( IS_FIRE( dt ) )
         rismod = ris_damage( victim, dam, RIS_FIRE );
      else if( IS_COLD( dt ) )
         rismod = ris_damage( victim, dam, RIS_COLD );
      else if( IS_ACID( dt ) )
         rismod = ris_damage( victim, dam, RIS_ACID );
      else if( IS_ELECTRICITY( dt ) )
         rismod = ris_damage( victim, dam, RIS_ELECTRICITY );
      else if( IS_ENERGY( dt ) )
         rismod = ris_damage( victim, dam, RIS_ENERGY );
      else if( IS_DRAIN( dt ) )
         rismod = ris_damage( victim, dam, RIS_DRAIN );
      else if( IS_WIND( dt ) )
         rismod = ris_damage( victim, dam, RIS_WIND );
      else if( IS_HOLY( dt ) )
      {
         rismod = ris_damage( victim, dam, RIS_HOLY );
         if( is_good( victim ) )
            dam /= 2;
      }
      else if( IS_SHADOW( dt ) )
      {
         rismod = ris_damage( victim, dam, RIS_SHADOW );
         if( is_evil( victim ) )
            dam /= 2;
      }
      else if( IS_ICE( dt ) )
         rismod = ris_damage( victim, dam, RIS_ICE );
      else if( IS_WATER( dt ) )
         rismod = ris_damage( victim, dam, RIS_WATER );
      else if( IS_EARTH( dt ) )
         rismod = ris_damage( victim, dam, RIS_EARTH );
      else if( dt == gsn_poison || IS_POISON( dt ) )
         rismod = ris_damage( victim, dam, RIS_POISON );
      else if( dt == ( TYPE_HIT + DAM_POUND ) || dt == ( TYPE_HIT + DAM_CRUSH )
      || dt == ( TYPE_HIT + DAM_STONE ) || dt == ( TYPE_HIT + DAM_PEA ) )
         rismod = ris_damage( victim, dam, RIS_BLUNT );
      else if( dt == ( TYPE_HIT + DAM_STAB ) || dt == ( TYPE_HIT + DAM_PIERCE )
      || dt == ( TYPE_HIT + DAM_BITE ) || dt == ( TYPE_HIT + DAM_BOLT )
      || dt == ( TYPE_HIT + DAM_DART ) || dt == ( TYPE_HIT + DAM_ARROW ) )
         rismod = ris_damage( victim, dam, RIS_PIERCE );
      else if( dt == ( TYPE_HIT + DAM_SLICE ) || dt == ( TYPE_HIT + DAM_SLASH )
      || dt == ( TYPE_HIT + DAM_WHIP ) || dt == ( TYPE_HIT + DAM_CLAW ) )
         rismod = ris_damage( victim, dam, RIS_SLASH );

      if( IS_MAGIC( dt ) || ( weapon && is_obj_stat( weapon, ITEM_MAGIC ) ) )
         rismod += ris_damage( victim, dam, RIS_MAGIC );
      else
         rismod += ris_damage( victim, dam, RIS_NONMAGIC );

      if( rismod == 100 )
      {
         if( dt >= 0 && dt < top_sn )
         {
            bool found = false;
            SKILLTYPE *skill = skill_table[dt];

            if( skill->imm_char && skill->imm_char[0] != '\0' )
            {
               act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
               found = true;
            }
            if( skill->imm_vict && skill->imm_vict[0] != '\0' )
            {
               act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
               found = true;
            }
            if( skill->imm_room && skill->imm_room[0] != '\0' )
            {
               act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
               found = true;
            }
            if( found )
               return rNONE;
         }
         dam = 0;
      }
      else if( rismod > 100 )
      {
         bool found = false;

         if( dt >= 0 && dt < top_sn )
         {
            SKILLTYPE *skill = skill_table[dt];

            if( skill->abs_char && skill->abs_char[0] != '\0' )
            {
               act( AT_HIT, skill->abs_char, ch, NULL, victim, TO_CHAR );
               found = true;
            }
            if( skill->abs_vict && skill->abs_vict[0] != '\0' )
            {
               act( AT_HITME, skill->abs_vict, ch, NULL, victim, TO_VICT );
               found = true;
            }
            if( skill->abs_room && skill->abs_room[0] != '\0' )
            {
               act( AT_ACTION, skill->abs_room, ch, NULL, victim, TO_NOTVICT );
               found = true;
            }
         }

         if( dam > 0 )
         {
            double risper =  ( dam * ( ( rismod - 100 ) * .01 ) );
            int hchange = ( int )risper;

            if( hchange > 0 )
            {
               victim->hit = URANGE( 0, victim->hit + hchange, victim->max_hit );
               update_pos( victim );
               if( !found )
                  new_dam_message( ch, victim, -hchange, dt, NULL );
               found = true;
            }
         }

         if( found )
            return rNONE;

         dam = 0;
      }
      else if( rismod < 0 ) /* Susceptible, increase damage */
      {
         int nrismod = -rismod;
         double risper = ( dam * ( nrismod * .01 ) );

         dam += ( int )risper;
      }
      else if( rismod > 0 ) /* Ok so lets reduce the damage if resistant */
      {
         double risper = ( dam * ( rismod * .01 ) );

         dam -= ( int )risper;
      }
   }

   /* Precautionary step mainly to prevent people in Hell from finding a way out. --Shaddai */
   if( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
      dam = 0;

   if( dam && npcvict && ch != victim )
   {
      if( !xIS_SET( victim->act, ACT_PACIFIST ) )
      {
         start_hating( victim, ch );
         if( !xIS_SET( victim->act, ACT_SENTINEL ) )
            start_hunting( victim, ch );
      }
   }

   /* Stop up any residual loopholes. */
   maxdam = ch->level * 80;

   if( dam > maxdam )
   {
      bug( "%s: %d more than %d points!", __FUNCTION__, dam, maxdam );
      bug( "** %s (lvl %d) -> %s **", ch->name, ch->level, victim->name );
      dam = maxdam;
   }

   if( victim != ch )
   {
      /*
       * Certain attacks are forbidden.
       * Most other attacks are returned.
       */
      if( is_safe( ch, victim, true ) )
         return rNONE;

      if( ch != supermob && victim->position > POS_STUNNED )
      {
         if( !victim->fighting && victim->in_room == ch->in_room && can_see_character( victim, ch ) )
            set_fighting( victim, ch );

         if( is_npc( victim ) && victim->fighting )
            victim->position = POS_FIGHTING;
         else if( victim->fighting )
         {
            switch( victim->style )
            {
               case ( STYLE_EVASIVE ):
                  victim->position = POS_EVASIVE;
                  break;

               case ( STYLE_DEFENSIVE ):
                  victim->position = POS_DEFENSIVE;
                  break;

               case ( STYLE_AGGRESSIVE ):
                  victim->position = POS_AGGRESSIVE;
                  break;

               case ( STYLE_BERSERK ):
                  victim->position = POS_BERSERK;
                  break;

               default:
                  victim->position = POS_FIGHTING;
                  break;
            }
         }
      }

      if( ch != supermob && victim->position > POS_STUNNED )
      {
         if( !ch->fighting && victim->in_room == ch->in_room && can_see_character( ch, victim ) )
            set_fighting( ch, victim );

         /* If victim is charmed, ch might attack victim's master. */
         if( is_npc( ch ) && npcvict
         && IS_AFFECTED( victim, AFF_CHARM )
         && victim->master && victim->master->in_room == ch->in_room && number_bits( 3 ) == 0 )
         {
            stop_fighting( ch, false );
            retcode = multi_hit( ch, victim->master, TYPE_UNDEFINED );
            return retcode;
         }
      }

      /*
       * Pkill stuff.  If a deadly attacks another deadly or is attacked by
       * one, then ungroup any nondealies.  Disabled untill I can figure out
       * the right way to do it.
       */
      /* count the # of non-pkill pc in a ( not including == ch ) */
      for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
         if( is_same_group( ch, gch ) && !is_npc( gch ) && !is_pkill( gch ) && ( ch != gch ) )
            anopc++;

      /* count the # of non-pkill pc in b ( not including == victim ) */
      for( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
         if( is_same_group( victim, gch ) && !is_npc( gch ) && !is_pkill( gch ) && ( victim != gch ) )
            bnopc++;

      /*
       * only consider disbanding if both groups have 1(+) non-pk pc,
       * or when one participant is pc, and the other group has 1(+)
       * pk pc's (in the case that participant is only pk pc in group) 
       */
      if( ( bnopc > 0 && anopc > 0 ) || ( bnopc > 0 && !is_npc( ch ) ) || ( anopc > 0 && !is_npc( victim ) ) )
      {
         /* Disband from same group first */
         if( is_same_group( ch, victim ) )
         {
            /* Messages to char and master handled in stop_follower */
            act( AT_ACTION, "$n disbands from $N's group.",
               ( ch->leader == victim ) ? victim : ch, NULL,
               ( ch->leader == victim ) ? victim->master : ch->master, TO_NOTVICT );
            if( ch->leader == victim )
            {
               stop_follower( victim );
               if( victim->group )
                  remove_char_from_group( victim );
            }
            else
            {
               stop_follower( ch );
               if( ch->group )
                  remove_char_from_group( ch );
            }
         }
         /* if leader isnt pkill, leave the group and disband ch */
         if( ch->leader && !is_npc( ch->leader ) && !is_pkill( ch->leader ) )
         {
            act( AT_ACTION, "$n disbands from $N's group.", ch, NULL, ch->master, TO_NOTVICT );
            stop_follower( ch );
            if( ch->group )
               remove_char_from_group( ch );
         }
         else
         {
            for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
               if( is_same_group( gch, ch ) && !is_npc( gch ) && !is_pkill( gch ) && gch != ch )
               {
                  act( AT_ACTION, "$n disbands from $N's group.", ch, NULL, gch->master, TO_NOTVICT );
                  stop_follower( gch );
                  if( gch->group )
                     remove_char_from_group( gch );
               }
         }
         /* if leader isnt pkill, leave the group and disband victim */
         if( victim->leader && !is_npc( victim->leader ) && !is_pkill( victim->leader ) )
         {
            act( AT_ACTION, "$n disbands from $N's group.", victim, NULL, victim->master, TO_NOTVICT );
            stop_follower( victim );
            if( victim->group )
               remove_char_from_group( victim );
         }
         else
         {
            for( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
               if( is_same_group( gch, victim ) && !is_npc( gch ) && !is_pkill( gch ) && gch != victim )
               {
                  act( AT_ACTION, "$n disbands from $N's group.", gch, NULL, gch->master, TO_NOTVICT );
                  stop_follower( gch );
                  if( gch->group )
                     remove_char_from_group( gch );
               }
         }

         /* More charm stuff. */
         if( victim->master == ch )
         {
            stop_follower( victim );
            if( victim->group )
               remove_char_from_group( victim );
         }
      }

      /* Inviso attacks ... not. */
      if( IS_AFFECTED( ch, AFF_INVISIBLE ) )
      {
         affect_strip( ch, gsn_invis );
         xREMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
         act( AT_MAGIC, "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
      }

      /* Take away Hide */
      if( IS_AFFECTED( ch, AFF_HIDE ) )
         xREMOVE_BIT( ch->affected_by, AFF_HIDE );

      /* Damage modifiers. */
      if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
         dam /= 2;

      if( IS_AFFECTED( victim, AFF_PROTECT ) && is_evil( ch ) )
         dam -= ( int )( dam / 4 );

      if( dam < 0 )
         dam = 0;

      /* Check for disarm, trip, parry, dodge and tumble. */
      if( dt >= TYPE_HIT && ch->in_room == victim->in_room )
      {
         if( is_npc( ch ) && xIS_SET( ch->defenses, DFND_DISARM )
         && number_percent( ) < ( URANGE( 3, ch->level, MAX_LEVEL ) / 3 ) )
            disarm( ch, victim );

         if( is_npc( ch ) && xIS_SET( ch->attacks, ATCK_TRIP )
         && number_percent( ) < ( URANGE( 2, ch->level, MAX_LEVEL ) / 2 ) )
            trip( ch, victim );

         if( check_counter( ch, victim, dam ) )
            return rNONE;
         if( check_parry( ch, victim ) )
            return rNONE;
         if( check_dodge( ch, victim ) )
            return rNONE;
         if( check_duck( ch, victim ) )
            return rNONE;
         if( check_shieldblock( ch, victim ) )
            return rNONE;
         if( check_block( ch, victim ) )
            return rNONE;
         if( check_tumble( ch, victim ) )
            return rNONE;
      }
   }

   /*
    * Code to handle equipment getting damaged, and also support  -Thoric
    * bonuses/penalties for having or not having equipment where hit
    */
   if( candameq && dam > 10 && dt != TYPE_UNDEFINED )
   {
      /* get a random body eq part */
      dameq = number_range( 0, WEAR_MAX - 1 );
      if( ( damobj = get_eq_location( victim, dameq ) ) )
         dam -= 5;   /* add a bonus for having something to block the blow */
      else
         dam += 5;   /* add penalty for bare skin! */
   }

   if( ch != victim && dt != TYPE_UNDEFINED )
      new_dam_message( ch, victim, dam, dt, NULL );
   if( damobj && dam > get_obj_resistance( damobj ) && number_bits( 1 ) == 0 )
      damage_obj( damobj );

   /* Hurt the victim. Inform the victim of his new state. */
   victim->hit -= dam;

   if( !is_npc( victim ) && get_trust( victim ) >= PERM_IMM && victim->hit < 1 )
      victim->hit = 1;

   /* Make sure newbies dont die */
   if( !is_npc( victim ) && victim->level < 2 && victim->hit < 1 )
      victim->hit = 1;

   /* Make sure sparing partners don't die */
   if( !is_npc( ch ) && !is_npc( victim ) )
   {
      if( xIS_SET( ch->act, PLR_SPARING ) && !xIS_SET( victim->act, PLR_SPARING ) )
         ch_printf( ch, "%s looks like they are out to kill you, even though your taking it easy.\r\n", victim->name );
      if( xIS_SET( ch->act, PLR_SPARING ) && victim->hit < 1 )
         endfight = true;
   }

   /* Make it so we can have a mobile that will fight, but not kill */
   if( is_npc( ch ) )
   {
      if( xIS_SET( ch->act, ACT_NOKILL ) && victim->hit < 1 )
         endfight = true;
   }

   /* Make it so we can have a mobile that will fight, but not die */
   if( is_npc( victim ) )
   {
      if( xIS_SET( victim->act, ACT_NODEATH ) && victim->hit < 1 )
         endfight = true;
   }

   if( endfight )
   {
      ch_printf( ch, "%s has fallen to you.\r\n", victim->name );
      ch_printf( victim, "You have fallen to %s.\r\n", ch->name );
      stop_fighting( victim, true );
      stop_hunting( ch, victim, false );
      stop_hating( ch, victim, false );
      stop_hunting( victim, NULL, true );
      stop_hating( victim, NULL, true );
      victim->hit = 1;
   }

   if( dam > 0 && dt > TYPE_HIT
   && !IS_AFFECTED( victim, AFF_POISON )
   && is_wielding_poisoned( ch )
   && !saves_poison_death( ch->level, victim ) )
   {
      AFFECT_DATA af;

      af.type = gsn_poison;
      af.duration = 20;
      af.location = APPLY_EXT_AFFECT;
      af.modifier = AFF_POISON;
      af.bitvector = meb( AFF_POISON );
      affect_join( victim, &af );
      victim->mental_state = URANGE( 20, victim->mental_state + ( is_pkill( victim ) ? 1 : 2 ), 100 );
   }

   /* Vampire self preservation - Thoric */
   if( is_vampire( victim ) && victim->max_hit > 0 )
   {
      if( dam >= ( victim->max_hit / 10 ) )  /* get hit hard, lose blood */
         victim->mana = URANGE( 0, ( victim->mana - ( dam / 20 ) ), victim->max_mana );
      if( victim->hit <= ( victim->max_hit / 8 ) && victim->mana > 8 )
      {
         victim->mana = URANGE( 0, ( victim->mana - number_range( 3, 8 ) ), victim->max_mana );
         victim->hit += URANGE( 4, ( victim->max_hit / 30 ), 15 );
         set_char_color( AT_BLOOD, victim );
         send_to_char( "You howl with rage as the beast within stirs!\r\n", victim );
      }
   }

   if( !npcvict && get_trust( victim ) >= PERM_IMM && get_trust( ch ) >= PERM_IMM && victim->hit < 1 )
      victim->hit = 1;

   /* Allow character to absorb or be damaged based on their ris */
   if( ch != victim && victim->hit > 1 && dam > 0 && ch->resistant[RIS_ABDAMAGE] != 0 )
   {
      if( ch->resistant[RIS_ABDAMAGE] < 0 ) /* Susceptible, damage the ch some */
      {
         int nrismod = -ch->resistant[RIS_ABDAMAGE];
         int absorbed;
         double risper = ( dam * ( nrismod * .01 ) );

         absorbed = ch->hit;
         ch->hit = URANGE( 1, ( ch->hit - ( int )risper ), ch->max_hit );
         if( ch->hit <= 0 )
            ch->hit = 1;
         if( absorbed > ch->hit )
         {
            absorbed = ( absorbed - ch->hit );
            ch_printf( ch, "You lost an additional %d hp.\r\n", absorbed );
         }
         else if( absorbed < ch->hit )
         {
            absorbed = ( ch->hit - absorbed );
            ch_printf( ch, "You absorbed %d hp.\r\n", absorbed );
         }
      }
      else if( ch->resistant[RIS_ABDAMAGE] > 0 ) /* Ok so lets let ch absorb some of the damage */
      {
         int absorbed;
         double risper = ( dam * ( ch->resistant[RIS_ABDAMAGE] * .01 ) );

         absorbed = ch->hit;
         ch->hit = URANGE( ch->hit, ( ch->hit + ( int )risper ), ch->max_hit );
         if( ch->hit <= 0 )
            ch->hit = 1;
         if( absorbed > ch->hit )
         {
            absorbed = ( absorbed - ch->hit );
            ch_printf( ch, "You lost an additional %d hp.\r\n", absorbed );
         }
         else if( absorbed < ch->hit )
         {
            absorbed = ( ch->hit - absorbed );
            ch_printf( ch, "You absorbed %d hp.\r\n", absorbed );
         }
      }
   }

   update_pos( victim );

   switch( victim->position )
   {
      case POS_MORTAL:
         act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You're mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_INCAP:
         act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You're incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_STUNNED:
         if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
         {
            act( AT_ACTION, "$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM );
            act( AT_HURT, "You're stunned, but will probably recover.", victim, NULL, NULL, TO_CHAR );
         }
         break;

      case POS_DEAD:
         if( dt >= 0 && dt < top_sn )
         {
            SKILLTYPE *skill = skill_table[dt];

            if( skill->die_char && skill->die_char[0] != '\0' )
               act( AT_DEAD, skill->die_char, ch, NULL, victim, TO_CHAR );
            if( skill->die_vict && skill->die_vict[0] != '\0' )
               act( AT_DEAD, skill->die_vict, ch, NULL, victim, TO_VICT );
            if( skill->die_room && skill->die_room[0] != '\0' )
               act( AT_DEAD, skill->die_room, ch, NULL, victim, TO_NOTVICT );
         }
         act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
         act( AT_DEAD, "You have been KILLED!!\r\n", victim, 0, 0, TO_CHAR );
         break;

      default:
         /*
          * Victim mentalstate affected, not attacker -- oops ;)
          * Thanks to gfinello@mail.karmanet.it for finding this bug
          */
         if( victim->max_hit <= 0 )
            break;
         if( dam > ( victim->max_hit / 4 ) )
         {
            act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
            if( number_bits( 3 ) == 0 )
               worsen_mental_state( victim, 1 );
         }
         if( victim->hit < ( victim->max_hit / 4 ) )
         {
            act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0, TO_CHAR );
            if( number_bits( 2 ) == 0 )
               worsen_mental_state( victim, 1 );
         }
         break;
   }

   /* Sleep spells and extremely wounded folks. */
   if( !is_awake( victim ) && !IS_AFFECTED( victim, AFF_PARALYSIS ) )
   {
      stop_summoning( victim );

      if( !npcvict && is_npc( ch ) )
         stop_fighting( victim, true );
      else
         stop_fighting( victim, false );
   }

   /* Payoff for killing things. */
   if( victim->position == POS_DEAD )
   {
      group_gain( ch, victim );

      if( !npcvict )
      {
         snprintf( log_buf, sizeof( log_buf ), "[%d]%s killed by [%d]%s at %d",
            victim->level, victim->name, ch->level, ( is_npc( ch ) ? ch->short_descr : ch->name ), victim->in_room->vnum );
         log_string( log_buf );
         to_channel( log_buf, "monitor", PERM_IMM );

         if( !is_npc( ch ) && !is_immortal( ch ) && ch->pcdata->clan
         && ch->pcdata->clan->clan_type == CLAN_PLAIN && victim != ch )
         {
            if( !victim->pcdata->clan || victim->pcdata->clan != ch->pcdata->clan )
               update_clan_victory( ch->pcdata->clan, ch, victim );
         }

         gain_exp( victim, ( 0 - number_range( 0, 500 ) ) );
      }
      else if( !is_npc( ch ) && is_npc( victim ) ) /* keep track of mob vnum killed */
      {
         add_kill( ch, victim );

         /* Add to kill tracker for grouped chars, as well. -Halcyon  */
         for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
            if( is_same_group( gch, ch ) && !is_npc( gch ) && gch != ch )
               add_kill( gch, victim );
      }

      check_killer( ch, victim );

      if( ch != victim && ch->in_room == victim->in_room )
         loot = legal_loot( ch, victim );
      else
         loot = false;

      /* If the victim was smarter then you, maybe you learned something from the battle */
      if( victim->perm_stats[STAT_WIS] >= ch->perm_stats[STAT_WIS] )
         handle_stat( ch, STAT_WIS, true, 1 );
      if( victim->perm_stats[STAT_INT] >= ch->perm_stats[STAT_INT] )
         handle_stat( ch, STAT_INT, true, 1 );

      set_cur_char( victim );
      raw_kill( ch, victim );
      victim = NULL;

      /* Sometimes the corpse and character aren't in the same room so why bother */
      if( !is_npc( ch ) && loot && ncorpse && ncorpse->in_room == ch->in_room )
      {
         /* No point in trying to get gold or items from a corpse if nothing in it */
         if( ncorpse->first_content && victim != ch )
         {
            if( xIS_SET( ch->act, PLR_AUTOLOOT ) )
               interpret( ch, (char *)"get all corpse" );
            else if( xIS_SET( ch->act, PLR_AUTOGOLD ) )
               interpret( ch, (char *)"get coin corpse" );

            if( !xIS_SET( ch->act, PLR_AUTOLOOT ) )
               interpret( ch, (char *)"look in corpse" );
         }

         if( xIS_SET( ch->act, PLR_AUTOSAC ) )
         {
            if( !xIS_SET( ch->act, PLR_SMARTSAC ) || !ncorpse->first_content )
               interpret( ch, (char *)"sacrifice corpse" );
         }
      }

      if( xIS_SET( sysdata.save_flags, SV_KILL ) )
         save_char_obj( ch );

      ncorpse = NULL;

      return rVICT_DIED;
   }

   if( victim == ch )
      return rNONE;

   /* Take care of link dead people. */
   if( !npcvict && !victim->desc && !xIS_SET( victim->pcdata->flags, PCFLAG_NORECALL ) )
   {
      if( number_range( 0, victim->wait ) == 0 )
      {
         do_recall( victim, (char *)"" );
         return rNONE;
      }
   }

   /* Wimp out? */
   if( npcvict && dam > 0 && victim->max_hit > 0 )
   {
      if( ( xIS_SET( victim->act, ACT_WIMPY ) && number_bits( 1 ) == 0
      && victim->hit < ( victim->max_hit / 2 ) )
      || ( IS_AFFECTED( victim, AFF_CHARM ) && victim->master && victim->master->in_room != victim->in_room ) )
      {
         start_fearing( victim, ch );
         stop_hunting( victim, ch, false );
         do_flee( victim, (char *)"" );
      }
   }

   if( !npcvict && victim->hit > 0 && victim->hit <= victim->wimpy && victim->wait == 0 )
      do_flee( victim, (char *)"" );
   else if( !npcvict && xIS_SET( victim->act, PLR_FLEE ) )
      do_flee( victim, (char *)"" );

   tail_chain( );
   return rNONE;
}

/*
 * Changed is_safe to have the show_messg boolian.  This is so if you don't
 * want to show why you can't kill someone you can't turn it off.  This is
 * useful for things like area attacks.  --Shaddai
 */
bool is_safe( CHAR_DATA *ch, CHAR_DATA *victim, bool show_messg )
{
   if( char_died( victim ) || char_died( ch ) )
      return true;

   if( who_fighting( ch ) == ch )
      return false;

   if( !victim )
   {
      bug( "%s: %s opponent does not exist!", __FUNCTION__, ch->name );
      return true;
   }

   if( !victim->in_room )
   {
      bug( "%s: %s has no physical location!", __FUNCTION__, victim->name );
      return true;
   }

   if( xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
   {
      if( show_messg )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "A magical force prevents you from attacking.\r\n", ch );
      }
      return true;
   }

   if( is_pacifist( ch ) )
   {
      if( show_messg )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "You're a pacifist and won't fight.\r\n", ch );
      }
      return true;
   }

   if( is_pacifist( victim ) )
   {
      if( show_messg )
      {
         set_char_color( AT_MAGIC, ch );
         ch_printf( ch, "%s is a pacifist and won't fight.\r\n", capitalize( victim->short_descr ) );
      }
      return true;
   }

   if( victim->fighting && victim != ch && ch != victim->fighting->who && !is_same_group( ch, victim->fighting->who ) )
   {
      if( !is_npc( victim->fighting->who ) && xIS_SET( victim->fighting->who->act, PLR_NOASSIST ) )
      {
         if( show_messg )
            send_to_char( "You're not able to join in the fight.\n\r", ch );
         return true;
      }
   }

   if( !is_npc( ch ) && get_trust( ch ) >= PERM_IMM )
      return false;

   if( !is_npc( ch ) && !is_npc( victim ) && ch != victim && xIS_SET( victim->in_room->area->flags, AFLAG_NOPKILL ) )
   {
      if( show_messg )
      {
         set_char_color( AT_IMMORT, ch );
         send_to_char( "The gods have forbidden player killing in this area.\r\n", ch );
      }
      return true;
   }

   if( is_npc( ch ) || is_npc( victim ) )
      return false;

   if( !is_pkill( ch ) )
   {
      if( !xIS_SET( ch->act, PLR_SPARING ) )
      {
         if( show_messg )
         {
            set_char_color( AT_YELLOW, ch );
            send_to_char( "You choose a peaceful path and can't kill another player.\r\n", ch );
         }
         return true;
      }
   }

   if( !is_pkill( victim ) )
   {
      if( !xIS_SET( victim->act, PLR_SPARING ) )
      {
         if( show_messg )
         {
            set_char_color( AT_GREEN, ch );
            send_to_char( "That character choose a peaceful path and can't be killed by another player.\r\n", ch );
         }
         return true;
      }
   }

   if( ch->level < 5 )
   {
      if( show_messg )
      {
         set_char_color( AT_WHITE, ch );
         send_to_char( "You aren't high enough in level yet. \r\n", ch );
      }
      return true;
   }

   if( victim->level < 5 )
   {
      if( show_messg )
      {
         set_char_color( AT_WHITE, ch );
         send_to_char( "They aren't high enough in level yet.\r\n", ch );
      }
      return true;
   }

   if( ( ch->level - victim->level ) > 5 || ( victim->level - ch->level ) > 5 )
   {
      if( show_messg )
      {
         set_char_color( AT_IMMORT, ch );
         send_to_char( "The gods don't allow murder when there is such a difference in level.\r\n", ch );
      }
      return true;
   }

   if( get_timer( victim, TIMER_PKILLED ) > 0 )
   {
      if( show_messg )
      {
         set_char_color( AT_GREEN, ch );
         send_to_char( "That character has died within the last 5 minutes.\r\n", ch );
      }
      return true;
   }

   if( get_timer( ch, TIMER_PKILLED ) > 0 )
   {
      if( show_messg )
      {
         set_char_color( AT_GREEN, ch );
         send_to_char( "You have been killed within the last 5 minutes.\r\n", ch );
      }
      return true;
   }

   return false;
}

/* just verify that a corpse looting is legal */
bool legal_loot( CHAR_DATA *ch, CHAR_DATA *victim )
{
   /* anyone can loot mobs */
   if( is_npc( victim ) )
      return true;

   /* non-charmed mobs can loot anything */
   if( is_npc( ch ) && !ch->master )
      return true;

   /* members of different clans can loot too! -Thoric */
   if( !is_npc( ch ) && !is_npc( victim )
   && xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
   && xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
      return true;

   return false;
}

void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
   /* NPC's are fair game. */
   if( is_npc( victim ) )
   {
      if( !is_npc( ch ) )
      {
         int level_ratio;

         level_ratio = URANGE( 1, ch->level, MAX_LEVEL );
         if( ch->pcdata->deity )
            adjust_favor( ch, 1, level_ratio );
         ch->pcdata->mkills++;
      }
      return;
   }

   /* If you kill yourself nothing happens. */
   if( ch == victim || get_trust( ch ) >= PERM_IMM )
      return;

   /* Any character in the arena is ok to kill. */
   if( ( in_arena( ch ) && in_arena( victim ) )
   || ( !is_npc( ch ) && !is_npc( victim ) ) )
   {
      ch->pcdata->pkills++;
      update_pos( victim );
      victim->pcdata->pdeaths++;
      adjust_favor( victim, 9, 1 );
      adjust_favor( ch, 1, 1 );
      add_timer( victim, TIMER_PKILLED, 115, NULL, 0 );
      wait_state( victim, 3 * PULSE_VIOLENCE );
      return;
   }

   /* Charm-o-rama. */
   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( !ch->master )
      {
         bug( "Check_killer: %s bad AFF_CHARM", is_npc( ch ) ? ch->short_descr : ch->name );
         affect_strip( ch, gsn_charm_person );
         xREMOVE_BIT( ch->affected_by, AFF_CHARM );
         return;
      }

      /* stop_follower( ch ); */
      if( ch->master )
         check_killer( ch->master, victim );
      return;
   }

   /*
    * NPC's are cool of course (as long as not charmed).
    * Hitting yourself is cool too (bleeding).
    * So is being immortal (Alander's idea).
    * And current killers stay as they are.
    */
   if( is_npc( ch ) )
   {
      if( !is_npc( victim ) )
      {
         int level_ratio;

         victim->pcdata->mdeaths++;
         level_ratio = URANGE( 1, ch->level, MAX_LEVEL );
         if( victim->pcdata->deity )
            adjust_favor( victim, 9, level_ratio );
      }
      return;
   }
}

/* Set position of a victim. */
void update_pos( CHAR_DATA *victim )
{
   if( !victim )
   {
      bug( "%s: null victim", __FUNCTION__ );
      return;
   }

   if( victim->hit > 0 )
   {
      if( victim->position <= POS_STUNNED )
         victim->position = POS_STANDING;
      if( IS_AFFECTED( victim, AFF_PARALYSIS ) )
         victim->position = POS_STUNNED;
      return;
   }

   if( is_npc( victim ) || victim->hit <= -11 )
   {
      if( victim->mount )
      {
         act( AT_ACTION, "$n falls from $N.", victim, NULL, victim->mount, TO_ROOM );
         xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
         victim->mount->mounter = NULL;
         victim->mount = NULL;
      }
      victim->position = POS_DEAD;
      return;
   }

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_SUICIDE ) )
   {
      if( victim->hit <= 0 )
         victim->position = POS_DEAD;
   }
   else
   {
      if( victim->hit <= -6 )
         victim->position = POS_MORTAL;
      else if( victim->hit <= -3 )
         victim->position = POS_INCAP;
      else
         victim->position = POS_STUNNED;
   }

   if( victim->position > POS_STUNNED && IS_AFFECTED( victim, AFF_PARALYSIS ) )
      victim->position = POS_STUNNED;

   if( victim->mount )
   {
      act( AT_ACTION, "$n falls unconscious from $N.", victim, NULL, victim->mount, TO_ROOM );
      xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
      victim->mount->mounter = NULL;
      victim->mount = NULL;
   }
}

/* Start fights. */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
   FIGHT_DATA *fight;

   if( ch->fighting )
   {
      bug( "%s: %s -> %s (already fighting %s)", __FUNCTION__, ch->name, victim->name, ch->fighting->who->name );
      return;
   }

   if( IS_AFFECTED( ch, AFF_SLEEP ) )
      affect_strip( ch, gsn_sleep );

   if( victim->num_fighting > MAX_FIGHT )
   {
      send_to_char( "There are too many people fighting for you to join in.\r\n", ch );
      return;
   }

   CREATE( fight, FIGHT_DATA, 1 );
   fight->who = victim;
   fight->align = align_compute( ch, victim );
   if( !is_npc( ch ) && is_npc( victim ) )
      fight->timeskilled = times_killed( ch, victim );
   ch->num_fighting = 1;
   ch->fighting = fight;

   if( is_npc( ch ) )
      ch->position = POS_FIGHTING;
   else
   {
      switch( ch->style )
      {
         case ( STYLE_EVASIVE ):
            ch->position = POS_EVASIVE;
            break;

         case ( STYLE_DEFENSIVE ):
            ch->position = POS_DEFENSIVE;
            break;

         case ( STYLE_AGGRESSIVE ):
            ch->position = POS_AGGRESSIVE;
            break;

         case ( STYLE_BERSERK ):
            ch->position = POS_BERSERK;
            break;

         default:
            ch->position = POS_FIGHTING;
            break;
      }
   }
   victim->num_fighting++;
}

CHAR_DATA *who_fighting( CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s: null ch", __FUNCTION__ );
      return NULL;
   }
   if( !ch->fighting )
      return NULL;
   return ch->fighting->who;
}

void free_fight( CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return;
   }
   if( ch->fighting )
   {
      if( !char_died( ch->fighting->who ) )
         --ch->fighting->who->num_fighting;
      DISPOSE( ch->fighting );
   }
   ch->fighting = NULL;
   if( ch->mount )
      ch->position = POS_MOUNTED;
   else
      ch->position = POS_STANDING;
   /* Berserk wears off after combat. -- Altrag */
   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      affect_strip( ch, gsn_berserk );
      set_char_color( AT_WEAROFF, ch );
      send_to_char( skill_table[gsn_berserk]->msg_off, ch );
      send_to_char( "\r\n", ch );
   }
}

/* Stop fights. */
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
   CHAR_DATA *fch;

   if( !ch || !ch->fighting )
      return;

   free_fight( ch );
   update_pos( ch );

   if( !fBoth )  /* major short cut here by Thoric */
      return;

   for( fch = first_char; fch; fch = fch->next )
   {
      if( who_fighting( fch ) == ch )
      {
         free_fight( fch );
         update_pos( fch );
      }
   }
}

/* Lets allow blood to splatter on armor */
void splatter_blood( CHAR_DATA *ch )
{
   CHAR_DATA *rch, *rch_next;
   OBJ_DATA *eqloc;
   short dameq;

   for( rch = ch->in_room->first_person; rch; rch = rch_next )
   {
      rch_next = rch->next_in_room;

      if( number_percent( ) > 25 )
      {
         act( AT_CARNAGE, "You hear $n's death cry.", ch, NULL, rch, TO_VICT );
         continue;
      }

      dameq = number_range( 0, WEAR_MAX - 1 );

      /* If no armor splatter blood on their body */
      if( !( eqloc = get_eq_location( rch, dameq ) ) )
      {
         rch->bsplatter = UMAX( 0, ( rch->bsplatter + 1 ) );
         act( AT_CARNAGE, "$n splatters blood on you.", ch, NULL, rch, TO_VICT );
      }
      else /* Splatter on the armor */
      {
         eqloc->bsplatter = UMAX( 0, ( eqloc->bsplatter + 1 ) );
         act( AT_CARNAGE, "$n splatters blood on $p.", ch, eqloc, rch, TO_VICT );
      }
   }
}

/*
 * Improved Death_cry contributed by Diavolo.
 * Additional improvement by Thoric (and removal of turds... sheesh!)  
 * Support for additional bodyparts by Fireblade
 */
void death_cry( CHAR_DATA *ch )
{
   ROOM_INDEX_DATA *was_in_room;
   EXIT_DATA *pexit;
   const char *msg;
   int vnum, i;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return;
   }

   vnum = 0;
   msg = NULL;

   switch( number_range( 0, 5 ) )
   {
      default:
         msg = "You hear $n's death cry.";
         break;

      case 0:
         msg = "$n screams furiously as $e falls to the ground in a heap!";
         break;

      case 1:
         msg = "$n hits the ground ... DEAD.";
         break;

      case 2:
         msg = "$n catches $s guts in $s hands as they pour through $s fatal wound!";
         break;

      case 3:
         splatter_blood( ch );
         break;

      case 4:
         msg = "$n gasps $s last breath and blood spurts out of $s mouth and ears.";
         break;

      case 5:
         if( !xIS_EMPTY( ch->xflags ) )
         {
            for( i = 0; i < PART_MAX; i++ )
            {
               if( number_percent( ) > 15 || !HAS_BODYPART( ch, i ) )
                  continue;
               msg = part_messages[i];
               vnum = part_vnums[i];
               break;
            }
         }
         if( !msg )
            msg = "You hear $n's death cry.";
         break;
   }

   if( msg )
      act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );

   if( vnum )
   {
      char buf[MSL];
      OBJ_DATA *obj;
      char *name;

      if( !get_obj_index( vnum ) )
      {
         bug( "%s: invalid vnum [%d]", __FUNCTION__, vnum );
         return;
      }

      name = is_npc( ch ) ? ch->short_descr : ch->name;
      if( !( obj = create_object( get_obj_index( vnum ), 0 ) ) )
      {
         bug( "%s: object [%d] couldn't be created.", __FUNCTION__, vnum );
         return;
      }
      obj->timer = number_range( 4, 7 );
      if( IS_AFFECTED( ch, AFF_POISON ) )
         obj->value[3] = 10;

      snprintf( buf, sizeof( buf ), obj->short_descr, name );
      STRFREE( obj->short_descr );
      obj->short_descr = STRALLOC( buf );

      snprintf( buf, sizeof( buf ), obj->description, name );
      STRFREE( obj->description );
      obj->description = STRALLOC( buf );

      obj = obj_to_room( obj, ch->in_room );
   }

   if( is_npc( ch ) )
      msg = "You hear something's death cry.";
   else
      msg = "You hear someone's death cry.";

   was_in_room = ch->in_room;
   for( pexit = was_in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( pexit->to_room && pexit->to_room != was_in_room )
      {
         ch->in_room = pexit->to_room;
         act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );
      }
   }
   ch->in_room = was_in_room;
}

void raw_kill( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !victim )
   {
      bug( "%s: null victim!", __FUNCTION__ );
      return;
   }

   /* backup in case hp goes below 1 */
   if( !is_npc( victim ) && victim->level < 2 )
   {
      bug( "%s: killing newbie: %s", __FUNCTION__, victim->name );
      return;
   }

   /* Let's not let a person sparing die */
   if( ch && !is_npc( victim ) && !is_npc( ch ) && xIS_SET( ch->act, PLR_SPARING ) && xIS_SET( victim->act, PLR_SPARING ) )
   {
      bug( "%s: killing sparing: %s by %s", __FUNCTION__, victim->name, ch->name );
      return;
   }

   stop_hunting( victim, ch, false );
   stop_fighting( victim, true );
   stop_fighting( ch, false );

   /* Take care of morphed characters */
   if( victim->morph )
   {
      do_unmorph_char( victim );
      raw_kill( ch, victim );
      return;
   }

   mprog_death_trigger( ch, victim );
   if( char_died( victim ) )
      return;

   rprog_death_trigger( victim );
   if( char_died( victim ) )
      return;

   ncorpse = make_corpse( victim, ch );

   if( !is_npc( victim ) || !xIS_SET( victim->act, ACT_NOBLOOD ) )
   {
      if( victim->in_room->sector_type == SECT_OCEANFLOOR
      || victim->in_room->sector_type == SECT_UNDERWATER
      || victim->in_room->sector_type == SECT_WATER_SWIM
      || victim->in_room->sector_type == SECT_WATER_NOSWIM )
         act( AT_BLOOD, "$n's blood slowly clouds the surrounding water.", victim, NULL, NULL, TO_ROOM );
      else if( victim->in_room->sector_type == SECT_AIR )
         act( AT_BLOOD, "$n's blood sprays wildly through the air.", victim, NULL, NULL, TO_ROOM );
      else
         make_blood( victim );
   }

   if( is_npc( victim ) )
   {
      victim->pIndexData->killed++;
      extract_char( victim, true );
      victim = NULL;
      return;
   }

   set_char_color( AT_DIEMSG, victim );
   if( victim->pcdata->mdeaths + victim->pcdata->pdeaths < 3 )
      do_help( victim, (char *)"new_death" );
   else
      do_help( victim, (char *)"_DIEMSG_" );

   extract_char( victim, false );

   if( !victim )
   {
      bug( "%s: oops! extract_char destroyed pc char", __FUNCTION__ );
      return;
   }
   while( victim->first_affect )
      affect_remove( victim, victim->first_affect );
   victim->affected_by = race_table[victim->race]->affected;
   {
      int stat;

      for( stat = 0; stat < RIS_MAX; stat++ )
         victim->resistant[stat] = 0;
   }
   victim->carry_weight = 0;
   victim->armor = 0;
   victim->armor += race_table[victim->race]->ac_plus;
   victim->bsplatter = 0;
   victim->damroll = 0;
   victim->hitroll = 0;
   victim->mental_state = -10;
   victim->alignment = URANGE( -1000, victim->alignment, 1000 );

   victim->position = POS_RESTING;
   victim->hit = UMAX( 1, victim->hit );

   if( victim->level < MAX_LEVEL )
      victim->mana = UMAX( 1, victim->mana );
   else
      victim->mana = 1;
   victim->move = UMAX( 1, victim->move );

   victim->pcdata->condition[COND_FULL] = 12;
   victim->pcdata->condition[COND_THIRST] = 12;

   if( xIS_SET( sysdata.save_flags, SV_DEATH ) )
      save_char_obj( victim );
}

void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
   CHAR_DATA *gch, *gch_next;
   GROUP_DATA *group;
   double xp;
   int members;

   /*
    * Dying of mortal wounds or poison doesn't give xp to anyone!
    */
   if( victim == ch )
      return;

   /* Handle groups if needed */
   if( ( group = ch->group ) )
   {
      members = 0;
      for( gch = group->first_char; gch; gch = gch->next_in_group )
      {
         if( gch->in_room == ch->in_room ) /* Only give exp if in same room */
            members++;
      }

      if( members == 0 )
      {
         bug( "%s: members %d", __FUNCTION__, members );
         members = 1;
      }

      for( gch = group->first_char; gch; gch = gch_next )
      {
         gch_next = gch->next_in_group;

         if( gch->in_room != ch->in_room )
            continue;

         xp = ( ( xp_compute( gch, victim ) ) / members );

         gch->alignment = align_compute( gch, victim );
         if( xp > 0 )
            gain_exp( gch, xp );

         check_chareq( gch );
      }
      return;
   }

   /* If not in a group handle solo */
   xp = ( ( xp_compute( ch, victim ) ) );
   ch->alignment = align_compute( ch, victim );
   if( xp > 0 )
      gain_exp( ch, xp );
   check_chareq( ch );
}

int align_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
   int align, newalign, divalign;

   align = gch->alignment - victim->alignment;

   /*
    * slowed movement in good & evil ranges by a factor of 5, h 
    * Added divalign to keep neutral chars shifting faster -- Blodkai 
    * This is obviously gonna take a lot more thought 
    */

   if( gch->alignment > -350 && gch->alignment < 350 )
      divalign = 4;
   else
      divalign = 20;

   if( align > 500 )
      newalign = UMIN( gch->alignment + ( align - 500 ) / divalign, 1000 );
   else if( align < -500 )
      newalign = UMAX( gch->alignment + ( align + 500 ) / divalign, -1000 );
   else
      newalign = gch->alignment - ( int )( gch->alignment / divalign );

   return newalign;
}

/*
 * Revamped by Thoric to be more realistic
 * Added code to produce different messages based on weapon type - FB
 * Added better bug message so you can track down the bad dt's -Shaddai
 */
void new_dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, unsigned int dt, OBJ_DATA *obj )
{
   SKILLTYPE *skill = NULL;
   ROOM_INDEX_DATA *was_in_room;
   const char *attack;
   char buf1[256], buf2[256], buf3[256], vs[MSL], vp[MSL], tmp[MSL], punct;
   int pdam = 0, d_index, w_index;
   bool gcflag = false, gvflag = false, absorbed = false;
   bool vfound = false, cfound = false, ofound = false;

   if( ch->in_room != victim->in_room )
   {
      was_in_room = ch->in_room;
      char_from_room( ch );
      char_to_room( ch, victim->in_room );
   }
   else
      was_in_room = NULL;

   /* Get the weapon index */
   /* Unsigned int is always 0 and higher so removed first part of this check */
   if( dt < ( unsigned int )top_sn )
      w_index = 0;
   else if( dt >= TYPE_PERS && dt < ( unsigned int )( top_pers + TYPE_PERS ) )
      w_index = 0;
   else if( dt >= TYPE_HIT && dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
      w_index = dt - TYPE_HIT;
   else
   {
      bug( "%s: bad dt %d from %s to %s in %d.", __FUNCTION__, dt, ch->name, victim->name, ch->in_room->vnum );
      dt = TYPE_HIT;
      w_index = 0;
   }

   if( !dam || victim->max_hit <= 0 )
      dam = 0;

   /* get the damage percent */
   if( dam > 0 && victim->hit > 0 )
   {
      pdam = ( ( 100 * dam ) / victim->hit );
      if( pdam <= 0 )
         pdam = 1;
   }

   /* get the damage index */
   if( pdam == 0 )
      d_index = 0;
   else if( pdam < 0 )
      d_index = 1;
   else if( pdam <= 10 )
      d_index = 1 + ( pdam / 2 );
   else if( pdam <= 20 )
      d_index = 6 + ( pdam / 4 );
   else if( pdam <= 40 )
      d_index = 11 + ( pdam / 8 );
   else if( pdam <= 80 )
      d_index = 16 + ( pdam / 16 );
   else if( pdam <= 100 )
      d_index = 21 + ( pdam / 50 );
   else
      d_index = 23;

   /* Lookup the damage message */
   if( dam < 0 )
   {
      dam = -dam;
      absorbed = true;
   }
   else
   {
      snprintf( vs, sizeof( vs ), "%s", s_message_table[w_index][d_index] );
      snprintf( vp, sizeof( vp ), "%s", p_message_table[w_index][d_index] );
   }
   
   if( d_index >= 10 )
   {
      if( d_index <= 15 )
      {
         snprintf( tmp, sizeof( tmp ), "&[yellow]_%s_&D", vs );
         snprintf( vs, sizeof( vs ), "%s", tmp );
         snprintf( tmp, sizeof( tmp ), "&[yellow]_%s_&D", vp );
         snprintf( vp, sizeof( vp ), "%s", tmp );
      }
      else if( d_index <= 20 )
      {
         snprintf( tmp, sizeof( tmp ), "&[pink]%s&D", strupper( vs ) );
         snprintf( vs, sizeof( vs ), "%s", tmp );
         snprintf( tmp, sizeof( tmp ), "&[pink]%s&D", strupper( vp ) );
         snprintf( vp, sizeof( vp ), "%s", tmp );
      }
      else
      {
         snprintf( tmp, sizeof( tmp ), "&[red]*%s*&D", strupper( vs ) );
         snprintf( vs, sizeof( vs ), "%s", tmp );
         snprintf( tmp, sizeof( tmp ), "&[red]*%s*&D", strupper( vp ) );
         snprintf( vp, sizeof( vp ), "%s", tmp );
      }
   }
   punct = ( d_index <= 15 ) ? '.' : '!';

   if( dam == 0 && ( !is_npc( ch ) && ( xIS_SET( ch->pcdata->flags, PCFLAG_GAG ) ) ) )
      gcflag = true;

   if( dam == 0 && ( !is_npc( victim ) && ( xIS_SET( victim->pcdata->flags, PCFLAG_GAG ) ) ) )
      gvflag = true;

   skill = get_skilltype( dt );

   if( dt == TYPE_HIT )
   {
      if( !absorbed )
      {
         snprintf( buf1, sizeof( buf1 ), "$n %s $N%c", vp, punct );
         snprintf( buf2, sizeof( buf2 ), "You %s $N%c [%d]", vs, punct, dam );
         snprintf( buf3, sizeof( buf3 ), "$n %s you%c [%d]", vp, punct, dam );
      }
      else
      {
         snprintf( buf1, sizeof( buf1 ), "$N absorbs some damage from $n%c", punct );
         snprintf( buf2, sizeof( buf2 ), "$N absorbs some damage from you%c [%d]", punct, dam );
         snprintf( buf3, sizeof( buf3 ), "You absorb some damage from $n%c [%d]", punct, dam );
      }
   }
   else if( dt > TYPE_HIT && is_wielding_poisoned( ch ) )
   {
      if( dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
         attack = attack_table[dt - TYPE_HIT];
      else
      {
         bug( "%s: bad dt %d from %s to %s in %d.", __FUNCTION__, dt, ch->name, victim->name, ch->in_room->vnum );
         dt = TYPE_HIT;
         attack = attack_table[0];
      }

      if( !absorbed )
      {
         snprintf( buf1, sizeof( buf1 ), "$n's poisoned %s %s $N%c", attack, vp, punct );
         snprintf( buf2, sizeof( buf2 ), "Your poisoned %s %s $N%c [%d]", attack, vp, punct, dam );
         snprintf( buf3, sizeof( buf3 ), "$n's poisoned %s %s you%c [%d]", attack, vp, punct, dam );
      }
      else
      {
         snprintf( buf1, sizeof( buf1 ), "$n's poisoned %s is absorbed by $N%c", attack, punct );
         snprintf( buf2, sizeof( buf2 ), "Your poisoned %s is absorbed by $N%c [%d]", attack, punct, dam );
         snprintf( buf3, sizeof( buf3 ), "$n's poisoned %s is absorbed by you%c [%d]", attack, punct, dam );
      }
   }
   else
   {
      if( skill )
      {
         attack = skill->noun_damage;
         if( dam == 0 )
         {
            if( skill->miss_char && skill->miss_char[0] != '\0' )
            {
               act( AT_HIT, skill->miss_char, ch, NULL, victim, TO_CHAR );
               cfound = true;
            }
            if( skill->miss_vict && skill->miss_vict[0] != '\0' )
            {
               act( AT_HITME, skill->miss_vict, ch, NULL, victim, TO_VICT );
               vfound = true;
            }
            if( skill->miss_room && skill->miss_room[0] != '\0' )
            {
               act( AT_ACTION, skill->miss_room, ch, NULL, victim, TO_NOTVICT );
               ofound = true;
            }
         }
         else
         {
            if( skill->hit_char && skill->hit_char[0] != '\0' )
            {
               act( AT_HIT, skill->hit_char, ch, NULL, victim, TO_CHAR );
               cfound = true;
            }
            if( skill->hit_vict && skill->hit_vict[0] != '\0' )
            {
               act( AT_HITME, skill->hit_vict, ch, NULL, victim, TO_VICT );
               vfound = true;
            }
            if( skill->hit_room && skill->hit_room[0] != '\0' )
            {
               act( AT_ACTION, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
               ofound = true;
            }
         }
      }
      else if( dt >= TYPE_HIT && dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
      {
         if( obj )
            attack = obj->short_descr;
         else
            attack = attack_table[dt - TYPE_HIT];
      }
      else
      {
         bug( "%s: bad dt %d from %s to %s in %d.", __FUNCTION__, dt, ch->name, victim->name, ch->in_room->vnum );
         dt = TYPE_HIT;
         attack = attack_table[0];
      }

      if( !absorbed )
      {
         snprintf( buf1, sizeof( buf1 ), "$n's %s %s $N%c", attack, vp, punct );
         snprintf( buf2, sizeof( buf2 ), "Your %s %s $N%c [%d]", attack, vp, punct, dam );
         snprintf( buf3, sizeof( buf3 ), "$n's %s %s you%c [%d]", attack, vp, punct, dam );
      }
      else
      {
         snprintf( buf1, sizeof( buf1 ), "$n's %s is absorbed by $N%c", attack, punct );
         snprintf( buf2, sizeof( buf3 ), "Your %s is absorbed by $N%c [%d]", attack, punct, dam );
         snprintf( buf3, sizeof( buf2 ), "$n's %s is absorbed by you%c [%d]", attack, punct, dam );
      }
   }

   /* Only show the message if a message for it wasn't already shown and if not gagged */
   if( dam != 0 && !ofound )
      act( AT_ACTION, buf1, ch, NULL, victim, TO_NOTVICT );
   if( !gcflag && !cfound )
      act( AT_HIT, buf2, ch, NULL, victim, TO_CHAR );
   if( !gvflag && !vfound )
      act( AT_HITME, buf3, ch, NULL, victim, TO_VICT );

   if( was_in_room )
   {
      char_from_room( ch );
      char_to_room( ch, was_in_room );
   }
}

CMDF( do_kill )
{
   CHAR_DATA *victim;
   char arg[MIL];

   one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Kill whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) && victim->morph )
   {
      send_to_char( "This creature appears strange to you.  Look upon it more closely before attempting to murder it.", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You hit yourself.  Ouch!\r\n", ch );
      multi_hit( ch, ch, TYPE_UNDEFINED );
      return;
   }

   if( is_safe( ch, victim, true ) )
      return;

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
   {
      act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
      return;
   }

   /* Handle if they are already fighting */
   if( ch->fighting )
   {
      /* No point in fighting someone you're already fighting */
      if( ch->fighting->who == victim )
      {
         send_to_char( "You do the best you can!\r\n", ch );
         return;
      }
      /* Stop attacking current victim and attack a new one */
      stop_fighting( ch, false );
   }
   
   wait_state( ch, 1 * PULSE_VIOLENCE );
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

CMDF( do_murder )
{
   CHAR_DATA *victim;
   char buf[MSL], arg[MIL];

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Murder whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Suicide is a mortal sin.\r\n", ch );
      return;
   }

   if( is_safe( ch, victim, true ) )
      return;

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
   {
      act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !is_npc( victim ) && xIS_SET( ch->act, PLR_NICE ) )
   {
      send_to_char( "You feel too nice to do that!\r\n", ch );
      return;
   }

   /* Handle if they are already fighting */
   if( ch->fighting )
   {
      /* No point in fighting someone you're already fighting */
      if( ch->fighting->who == victim )
      {
         send_to_char( "You do the best you can!\r\n", ch );
         return;
      }
      /* Stop attacking current victim and attack a new one */
      stop_fighting( ch, false );
   }

   if( !is_npc( victim ) )
      log_printf_plus( LOG_NORMAL, get_trust( ch ), "%s: murder %s.", ch->name, victim->name );

   wait_state( ch, 1 * PULSE_VIOLENCE );
   snprintf( buf, sizeof( buf ), "%s Help!  I am being attacked by %s!",
      is_pkill( victim ) ? "wartalk" : "yell", is_npc( ch ) ? ch->short_descr : ch->name );
   interpret( victim, buf );
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

/* Check to see if the player is in an "Arena". */
bool in_arena( CHAR_DATA *ch )
{
   if( xIS_SET( ch->in_room->room_flags, ROOM_ARENA ) )
      return true;
   if( xIS_SET( ch->in_room->area->flags, AFLAG_FREEKILL ) )
      return true;
   if( ch->in_room->vnum >= 29 && ch->in_room->vnum <= 43 )
      return true;
   if( !str_cmp( ch->in_room->area->filename, "arena.are" ) )
      return true;

   return false;
}

CMDF( do_flee )
{
   ROOM_INDEX_DATA *was_in, *now_in;
   EXIT_DATA *pexit;
   double los;
   int attempt;
   short door;

   if( !who_fighting( ch ) )
   {
      if( is_fighting( ch ) )
      {
         if( ch->mount )
            ch->position = POS_MOUNTED;
         else
            ch->position = POS_STANDING;
      }
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   if( ch->move <= 0 )
   {
      send_to_char( "You're too exhausted to flee from combat!\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "You can't flee while fighting berserk!\r\n", ch );
      return;
   }

   /* No fleeing while more aggressive than standard or hurt. - Haus */
   if( ch->position < POS_FIGHTING )
   {
      if( ch->position <= POS_SLEEPING )
         return;
      if( ch->position == POS_BERSERK )
         send_to_char( "You can't flee while fighting berserk!\r\n", ch );
      if( ch->position == POS_RESTING )
         send_to_char( "You can't flee while your resting!\r\n", ch );
      if( ch->position == POS_AGGRESSIVE )
         send_to_char( "You can't flee while your fighting aggressively!\r\n", ch );
      if( ch->position == POS_SITTING )
         send_to_char( "You can't flee while your sitting!\r\n", ch );
      return;      
   }

   was_in = ch->in_room;
   for( attempt = 0; attempt < 8; attempt++ )
   {
      door = number_door( );
      if( !( pexit = get_exit( was_in, door ) )
      || !pexit->to_room
      || xIS_SET( pexit->exit_info, EX_NOFLEE )
      || ( xIS_SET( pexit->exit_info, EX_CLOSED ) && !IS_AFFECTED( ch, AFF_PASS_DOOR ) )
      || ( is_npc( ch ) && xIS_SET( pexit->to_room->room_flags, ROOM_NO_MOB ) ) 
      || ( is_npc( ch ) && xIS_SET( pexit->to_room->room_flags, ROOM_DEATH ) ) )
         continue;
      affect_strip( ch, gsn_sneak );
      xREMOVE_BIT( ch->affected_by, AFF_SNEAK );
      if( ch->mount )
         stop_fighting( ch->mount, true );
      move_char( ch, pexit, 0, false );
      if( ( now_in = ch->in_room ) == was_in )
         continue;
      ch->in_room = was_in;
      act( AT_FLEE, "$n flees head over heels!", ch, NULL, NULL, TO_ROOM );
      ch->in_room = now_in;
      act( AT_FLEE, "$n glances around for signs of pursuit.", ch, NULL, NULL, TO_ROOM );
      if( !is_npc( ch ) )
      {
         CHAR_DATA *wf = who_fighting( ch );
         act( AT_FLEE, "You flee head over heels from combat!", ch, NULL, NULL, TO_CHAR );
         los = ( number_range( 20, 200 ) );
         if( ch->level < MAX_LEVEL )
            gain_exp( ch, 0 - los );

         if( wf && ch->pcdata->deity )
         {
            int level_ratio = URANGE( 1, wf->level / ch->level, MAX_LEVEL );
            adjust_favor( ch, 0, level_ratio );
         }
      }
      stop_fighting( ch, true );
      return;
   }

   los = ( number_range( 20, 100 ) );
   act( AT_FLEE, "You attempt to flee from combat but can't escape!", ch, NULL, NULL, TO_CHAR );
   if( ch->level < MAX_LEVEL && number_bits( 3 ) == 1 )
      gain_exp( ch, 0 - los );
}

CMDF( do_slay )
{
   CHAR_DATA *victim;
   char arg[MIL];

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Slay whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( ch == victim )
   {
      send_to_char( "Suicide is a mortal sin.\r\n", ch );
      return;
   }

   if( !is_npc( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "inferno" ) )
   {
      act( AT_FIRE, "You release a searing fireball in $N's direction that turns $S into a blazing inferno upon impact.", ch, NULL, victim, TO_CHAR );
      act( AT_FIRE, "$n releases a searing fireball in your direction that turns you into a blazing inferno upon impact.", ch, NULL, victim, TO_VICT );
      act( AT_FIRE, "$n releases a searing fireball in $N's direction that turns $S into a blazing inferno upon impact.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg, "shatter" ) )
   {
      act( AT_LBLUE, "You freeze $N with a glance and shatter the frozen body into tiny shards.", ch, NULL, victim, TO_CHAR );
      act( AT_LBLUE, "$n freezes you with a glance and shatters your frozen body into tiny shards.", ch, NULL, victim, TO_VICT );
      act( AT_LBLUE, "$n freezes $N with a glance and shatters the frozen body into tiny shards.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg, "demon" ) )
   {
      act( AT_IMMORT, "You gesture, and a slavering demon appears.  With a horrible grin, the", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  With a horrible grin, the", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "foul creature turns on you.  You scream in panic before being eaten alive.", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  With a horrible grin, the", ch, NULL, victim, TO_NOTVICT );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg, "leap" ) )
   {
      act( AT_BLOOD, "You leap upon $N with bared fangs and tear open $S throat.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n leaps upon you with bared fangs and tears open your throat.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n leaps upon $N with bared fangs and tears open $N's throat.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg, "slit" ) )
   {
      act( AT_BLOOD, "You reach out with a clawed finger and calmly slit $N's throat.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n reaches out with a clawed finger and calmly slits your throat.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n reaches out with a clawed finger and calmly slits $N's throat.", ch, NULL, victim, TO_NOTVICT );
   }
   else if( !str_cmp( arg, "dogs" ) )
   {
      act( AT_BLOOD, "You order your dogs to rip $N to shreds.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n orders $s dogs to rip you to shreds.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n orders $s dogs to rip $N to shreds.", ch, NULL, victim, TO_NOTVICT );
   }
   else
   {
      act( AT_IMMORT, "You slay $N in cold blood!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT );
   }

   set_cur_char( victim );
   raw_kill( ch, victim );
}

CMDF( do_suicide )
{
   if( ch->hit >= 0 )
   {
      act( AT_BLOOD, "You can't commit suicide unless your below 0 hp.", ch, NULL, NULL, TO_CHAR );
      act( AT_BLOOD, "$n tries to commit suicide but fails.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   act( AT_BLOOD, "You commit suicide.", ch, NULL, NULL, TO_CHAR );
   act( AT_BLOOD, "$n commits suicide.", ch, NULL, NULL, TO_ROOM );
   set_cur_char( ch );
   raw_kill( ch, ch );
}
