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
 *                          Player skills module                             *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"

OBJ_DATA *get_room_trap( ROOM_INDEX_DATA *room );
SPELL_FUN *spell_function( char *name );
void save_skill_table( bool autosave );
void save_herb_table( bool autosave );
void save_pers_table( bool autosave );
void asskills( short type );
void save_races( void );
void failed_casting( SKILLTYPE *skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj );
int get_skill( char *skilltype );
int ris_save( CHAR_DATA *ch, int schance, int ris );
int ch_pers_lookup( const char *pchar, const char *name );
int find_spell( CHAR_DATA *ch, const char *name, bool know );
bool saves_para_petri( int level, CHAR_DATA *victim );
bool can_use_skill( CHAR_DATA *ch, int percent, int gsn );
bool process_spell_components( CHAR_DATA *ch, int sn );
bool check_pos( CHAR_DATA *ch, short position );
bool check_grip( CHAR_DATA *ch, CHAR_DATA *victim );

int get_possible_adept( CHAR_DATA *ch, int sn )
{
   MCLASS_DATA *mclass;
   int adept = 0;

   if( !ch || !skill_table[sn] )
      return 0;

   if( is_immortal( ch ) )
      return 100;

   if( ch->race >= 0 && ch->race < MAX_PC_RACE && race_table[ch->race] && skill_table[sn]->race_level[ch->race] > 0 )
      adept = skill_table[sn]->race_adept[ch->race];

   if( ch->pcdata )
   {
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( mclass->wclass >= 0 && mclass->wclass < MAX_PC_CLASS && class_table[mclass->wclass]
         && skill_table[sn]->skill_level[mclass->wclass] > 0 && skill_table[sn]->skill_adept[mclass->wclass] > adept )
            adept = skill_table[sn]->skill_adept[mclass->wclass];
      }
   }

   return adept;
}

int get_adept( CHAR_DATA *ch, int sn )
{
   MCLASS_DATA *mclass;
   int adept = 0;

   if( !ch || !skill_table[sn] )
      return 0;

   if( is_immortal( ch ) )
      return 100;

   if( ch->race >= 0 && ch->race < MAX_PC_RACE && race_table[ch->race]
   && skill_table[sn]->race_level[ch->race] > 0 && ch->level >= skill_table[sn]->race_level[ch->race] )
      adept = skill_table[sn]->race_adept[ch->race];

   if( ch->pcdata )
   {
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( mclass->wclass >= 0 && mclass->wclass < MAX_PC_CLASS && class_table[mclass->wclass]
         && skill_table[sn]->skill_level[mclass->wclass] > 0 && mclass->level >= skill_table[sn]->skill_level[mclass->wclass]
         && skill_table[sn]->skill_adept[mclass->wclass] > adept )
            adept = skill_table[sn]->skill_adept[mclass->wclass];
      }
   }

   return adept;
}

/* Can we use the name for a skill, etc...? */
bool can_use_skill_name( char *name )
{
   int sn;

   for( sn = 0; sn < top_sn; sn++ )
   {
      if( skill_table[sn] && skill_table[sn]->name && !str_cmp( name, skill_table[sn]->name ) )
         return false;
   }

   for( sn = 0; sn < top_herb; sn++ )
   {
      if( herb_table[sn] && herb_table[sn]->name && !str_cmp( name, herb_table[sn]->name ) )
         return false;
   }

   for( sn = 0; sn < top_pers; sn++ )
   {
      if( pers_table[sn] && pers_table[sn]->name && !str_cmp( name, pers_table[sn]->name ) )
         return false;
   }

   return true;
}

const char *dis_skill_name( int sn )
{
   if( !is_valid_sn( sn ) || !skill_table[sn] || !skill_table[sn]->name )
      return "NONE";

   return skill_table[sn]->name;
}

/* Mana cost */
int mana_cost( CHAR_DATA *ch, int sn )
{
   MCLASS_DATA *mclass;
   SKILLTYPE *skill = NULL;
   int min = ( MAX_LEVEL + 1 ), cost = 0;

   if( !ch )
      return cost;

   /* We need to make sure we get a valid spell/skill to use */
   if( sn >= 0 && sn < MAX_SKILL && skill_table[sn] )
   {
      skill = skill_table[sn];
      if( ch->pcdata )
      {
         for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
         {
            if( mclass->wclass >= 0 && mclass->wclass < MAX_PC_CLASS && class_table[mclass->wclass]
            && skill->skill_level[mclass->wclass] > 0 && min > skill->skill_level[mclass->wclass] )
            {
               min = skill->skill_level[mclass->wclass];
               cost = ( MAX_LEVEL / ( 2 + ( mclass->level - min ) ) );
            }
         }
      }

      if( ch->race >= 0 && race_table[ch->race] && skill->race_level[ch->race] > 0 && min > skill->race_level[ch->race] )
         cost = ( MAX_LEVEL / ( 2 + ( ch->level - skill->race_level[ch->race] ) ) );
   }
   else if( sn >= TYPE_PERS && sn < ( TYPE_PERS + MAX_PERS ) && pers_table[( sn - TYPE_PERS )] )
   {
      sn -= TYPE_PERS;
      skill = pers_table[sn];

      /* For now we are just going to set it to use what ever the mana is set at */
      cost = skill->min_mana;
   }

   return cost;
}

/* What was the earliest they could have learned it? */
int first_learned( CHAR_DATA *ch, int sn )
{
   MCLASS_DATA *mclass;
   int min = ( MAX_LEVEL + 1 );

   if( !ch || !skill_table[sn] )
      return min;

   if( ch->pcdata )
   {
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( mclass->wclass >= 0 && mclass->wclass < MAX_PC_CLASS && class_table[mclass->wclass]
         && skill_table[sn]->skill_level[mclass->wclass] > 0 && min > skill_table[sn]->skill_level[mclass->wclass] )
            min = skill_table[sn]->skill_level[mclass->wclass];
      }
   }

   if( ch->race >= 0 && race_table[ch->race] && skill_table[sn]->race_level[ch->race] > 0 && min > skill_table[sn]->race_level[ch->race] )
      min = skill_table[sn]->race_level[ch->race];

   return min;
}

void free_skill( SKILLTYPE *skill )
{
   SMAUG_AFF *aff, *aff_next;

   if( skill->first_affect )
   {
      for( aff = skill->first_affect; aff; aff = aff_next )
      {
         aff_next = aff->next;
         UNLINK( aff, skill->first_affect, skill->last_affect, next, prev );
         STRFREE( aff->duration );
         STRFREE( aff->modifier );
         DISPOSE( aff );
      }
   }
   STRFREE( skill->skill_fun_name );
   STRFREE( skill->spell_fun_name );
   STRFREE( skill->name );
   STRFREE( skill->noun_damage );
   STRFREE( skill->msg_off );
   STRFREE( skill->hit_char );
   STRFREE( skill->hit_vict );
   STRFREE( skill->hit_room );
   STRFREE( skill->hit_dest );
   STRFREE( skill->miss_char );
   STRFREE( skill->miss_vict );
   STRFREE( skill->miss_room );
   STRFREE( skill->die_char );
   STRFREE( skill->die_vict );
   STRFREE( skill->die_room );
   STRFREE( skill->imm_char );
   STRFREE( skill->imm_vict );
   STRFREE( skill->imm_room );
   STRFREE( skill->abs_char );
   STRFREE( skill->abs_vict );
   STRFREE( skill->abs_room );
   STRFREE( skill->htext );
   STRFREE( skill->dice );
   STRFREE( skill->components );
   STRFREE( skill->teachers );
   STRFREE( skill->reqskillname );
   skill->spell_fun = NULL;
   skill->skill_fun = NULL;
   DISPOSE( skill );
}

void check_reqs( void )
{
   SKILLTYPE *skill;
   int hash = 0, sn;

   for( hash = 0; hash < top_sn; hash++ )
   {
      skill = skill_table[hash];

      if( !skill->reqskillname )
         continue;

      if( ( sn = skill_lookup( skill->reqskillname ) ) < 0 )
         bug( "%s: unknown skill (%s).", __FUNCTION__, skill->reqskillname );
      else
         skill->req_skill = sn;

      STRFREE( skill->reqskillname );
   }
}

void free_skills( void )
{
   SKILLTYPE *skill;
   int hash = 0;

   for( hash = 0; hash < top_sn; hash++ )
   {
      skill = skill_table[hash];
      free_skill( skill );
   }

   for( hash = 0; hash < top_herb; hash++ )
   {
      skill = herb_table[hash];
      free_skill( skill );
   }

   for( hash = 0; hash < top_pers; hash++ )
   {
      skill = pers_table[hash];
      free_skill( skill );
   }
}

/* Dummy function */
CMDF( skill_notfound )
{
   send_to_char( "Huh?\r\n", ch );
}

bool is_legal_kill( CHAR_DATA *ch, CHAR_DATA *vch )
{
   if( is_npc( ch ) || is_npc( vch ) )
      return true;
   if( !is_pkill( ch ) || !is_pkill( vch ) )
      return false;
   if( ch->pcdata->clan && ch->pcdata->clan == vch->pcdata->clan )
      return false;
   return true;
}

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows				-Thoric
 */
bool check_skill( CHAR_DATA *ch, char *command, char *argument )
{
   SKILLTYPE *skill;
   struct timeval time_used;
   int sn, mana = 0, blood = 0;
   int first = gsn_first_skill;
   int top = gsn_first_weapon - 1;

   /* bsearch for the skill */
   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( ( skill = skill_table[sn] )
      && LOWER( command[0] ) == LOWER( skill->name[0] )
      && !str_prefix( command, skill->name )
      && ( skill->skill_fun || skill->spell_fun != spell_null )
      && ( can_use_skill( ch, 0, sn ) ) )
         break;
      if( first >= top )
      {
         /* Wasn't found in skills so look in personals */
         if( is_npc( ch ) || is_immortal( ch ) )
         {
            if( ( sn = pers_lookup( command ) ) >= 0 )
            {
               sn += TYPE_PERS;

               if( ( skill = get_skilltype( sn ) ) && !skill->tmpspell )
                  break;
               return false;
            }
            else
               return false;
         }
         else if( !is_npc( ch ) )
         {
            if( ( sn = ch_pers_lookup( ch->name, command ) ) >= 0 )
            {
               sn += TYPE_PERS;

               if( ( skill = get_skilltype( sn ) ) && !skill->tmpspell )
                  break;
               return false;
            }
            else
               return false;
         }
         else
            return false;
      }
      if( skill && strcasecmp( command, skill->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }

   if( !check_pos( ch, skill->minimum_position ) )
      return true;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "For some reason, you seem unable to perform that...\r\n", ch );
      act( AT_GRAY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
      return true;
   }

   if( !ch->in_room
   || ( !xIS_EMPTY( skill->spell_sector ) && !xIS_SET( skill->spell_sector, ch->in_room->sector_type ) ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return true;
   }

   {
      int stat;

      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( skill->stats[stat] <= 0 )
            continue;
         if( skill->stats[stat] > get_curr_stat( stat, ch ) )
         {
            ch_printf( ch, "You don't have enough %s to do that.\r\n", stattypes[stat] );
            return true;
         }
      }
   }

   /* check if mana is required */
   if( skill->min_mana )
   {
      mana = is_npc( ch ) ? 0 : UMAX( skill->min_mana, mana_cost( ch, sn ) );
      blood = UMAX( 1, ( mana + 4 ) / 8 );
      if( is_vampire( ch ) )
      {
         if( ch->mana < blood )
         {
            send_to_char( "You don't have enough blood power.\r\n", ch );
            return true;
         }
      }
      else if( !is_npc( ch ) && ch->mana < mana )
      {
         send_to_char( "You don't have enough mana.\r\n", ch );
         return true;
      }
   }

   /* Is this a real do-fun, or a really a spell? */
   if( !skill->skill_fun )
   {
      CHAR_DATA *victim = NULL;
      OBJ_DATA *obj = NULL;
      ch_ret retcode = rNONE;
      void *vo = NULL;

      target_name = (char *)"";

      switch( skill->target )
      {
         default:
            bug( "%s: bad target for sn %d.", __FUNCTION__, sn );
            send_to_char( "Something went wrong...\r\n", ch );
            return true;

         case TAR_IGNORE:
            vo = NULL;
            if( argument[0] == '\0' )
            {
               if( ( victim = who_fighting( ch ) ) )
                  target_name = victim->name;
            }
            else
               target_name = argument;
            break;

         case TAR_CHAR_OFFENSIVE:
            if( argument[0] == '\0' && !( victim = who_fighting( ch ) ) )
            {
               ch_printf( ch, "Confusion overcomes you as your '%s' has no target.\r\n", skill->name );
               return true;
            }
            else if( argument[0] != '\0' && !( victim = get_char_room( ch, argument ) ) )
            {
               send_to_char( "They aren't here.\r\n", ch );
               return true;
            }

            if( is_safe( ch, victim, true ) )
               return true;

            if( ch == victim && SPELL_FLAG( skill, SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\r\n", ch );
               return true;
            }

            if( !is_npc( ch ) )
            {
               if( !is_npc( victim ) )
               {
                  if( get_timer( ch, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "You have been killed in the last 5 minutes.\r\n", ch );
                     return true;
                  }

                  if( get_timer( victim, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "This player has been killed in the last 5 minutes.\r\n", ch );
                     return true;
                  }

                  if( victim != ch )
                     send_to_char( "You really shouldn't do this to another player...\r\n", ch );
               }

               if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
               {
                  send_to_char( "You can't do that on your own follower.\r\n", ch );
                  return true;
               }
            }

            vo = ( void * )victim;
            break;

         case TAR_CHAR_DEFENSIVE:
            if( argument[0] != '\0' && !( victim = get_char_room( ch, argument ) ) )
            {
               send_to_char( "They aren't here.\r\n", ch );
               return true;
            }
            if( !victim )
               victim = ch;

            if( ch == victim && SPELL_FLAG( skill, SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\r\n", ch );
               return true;
            }

            vo = ( void * )victim;
            break;

         case TAR_CHAR_SELF:
            vo = ( void * )ch;
            break;

         case TAR_OBJ_INV:
            if( !( obj = get_obj_carry( ch, argument ) ) )
            {
               send_to_char( "You can't find that.\r\n", ch );
               return true;
            }
            vo = ( void * )obj;
            break;
      }

      /* waitstate */
      wait_state( ch, skill->beats );

      /* check for failure */
      if( skill->type != SKILL_PERSONAL && ( number_percent( ) + skill->difficulty ) > ( is_npc( ch ) ? 75 : LEARNED( ch, sn ) ) )
      {
         failed_casting( skill, ch, victim, obj );
         learn_from_failure( ch, sn );
         if( mana )
            ch->mana -= ( ( is_vampire( ch ) ? blood : mana ) / 2 );
         return true;
      }
      if( mana )
         ch->mana -= is_vampire( ch ) ? blood : mana;
      start_timer( &time_used );
      retcode = ( *skill->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );
      update_userec( &time_used, &skill->userec );

      if( retcode == rCHAR_DIED || retcode == rERROR )
         return true;

      if( char_died( ch ) )
         return true;

      if( retcode == rSPELL_FAILED )
      {
         learn_from_failure( ch, sn );
         retcode = rNONE;
      }
      else
         learn_from_success( ch, sn );

      if( skill->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) )
      {
         CHAR_DATA *vch, *vch_next;

         for( vch = ch->in_room->first_person; vch; vch = vch_next )
         {
            vch_next = vch->next_in_room;
            if( victim == vch && !victim->fighting && victim->master != ch )
            {
               retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
      return true;
   }

   if( mana )
      ch->mana -= is_vampire( ch ) ? blood : mana;

   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = skill->skill_fun;
   start_timer( &time_used );
   ( *skill->skill_fun ) ( ch, argument );
   end_timer( &time_used );
   update_userec( &time_used, &skill->userec );

   tail_chain( );
   return true;
}

CMDF( do_skin )
{
   OBJ_DATA *corpse, *obj, *skin;
   char *temp;
   char buf[MSL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Whose corpse do you wish to skin?\r\n", ch );
      return;
   }
   if( !( corpse = get_obj_here( ch, argument ) ) )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }
   if( !is_immortal( ch ) )
   {
      /*
       * This is restrictions for mortals only 
       * They have to hold a weapon and of a certian type
       * They can only skin corpses if they got final blow
       * And the corpse can only be skinned once
       */
      if( !( obj = get_eq_hold( ch, ITEM_WEAPON ) ) )
      {
         send_to_char( "You have no weapon with which to perform this deed.\r\n", ch );
         return;
      }
      if( obj->value[3] != DAM_SLICE && obj->value[3] != DAM_STAB
      && obj->value[3] != DAM_SLASH && obj->value[3] != DAM_PIERCE )
      {
         send_to_char( "You're not holding the right kind of weapon to skin a corpse.\r\n", ch );
         return;
      }
      if( !corpse->action_desc || corpse->action_desc[0] == '\0' || str_cmp( corpse->action_desc, ch->name ) )
      {
         send_to_char( "You can't skin a corpse if you didn't get the final blow on the player.\r\n", ch );
         return;
      }
      if( corpse->value[5] >= 1 )
      {
         send_to_char( "That corpse has already been skinned.\r\n", ch );
         return;
      }
   }
   /* Immortals can skin npc and pc corpses */
   else if( corpse->item_type != ITEM_CORPSE_PC && corpse->item_type != ITEM_CORPSE_NPC )
   {
      send_to_char( "You can only skin corpses.\r\n", ch );
      return;
   }

   if( !get_obj_index( OBJ_VNUM_SKIN ) )
   {
      bug( "%s: Vnum %d (OBJ_VNUM_SKIN) not found!", __FUNCTION__, OBJ_VNUM_SKIN );
      send_to_char( "The skin object couldn't be found.\r\n", ch );
      return;
   }
   if( !( skin = create_object( get_obj_index( OBJ_VNUM_SKIN ), 0 ) ) )
   {
      bug( "%s: couldn't create skin [%d]", __FUNCTION__, OBJ_VNUM_SKIN );
      send_to_char( "A skin object couldn't be created.\r\n", ch );
      return;
   }

   temp = corpse->short_descr;
   temp = one_argument( temp, buf );
   temp = one_argument( temp, buf );
   temp = one_argument( temp, buf );

   if( skin->short_descr )
   {
      snprintf( buf, sizeof( buf ), skin->short_descr, temp );
      STRSET( skin->short_descr, buf );
   }
   if( skin->description )
   {
      snprintf( buf, sizeof( buf ), skin->description, temp );
      STRSET( skin->description, buf );
   }
   corpse->value[5] = 1;
   act( AT_BLOOD, "$n strips the skin from $p.", ch, corpse, NULL, TO_ROOM );
   act( AT_BLOOD, "You strip the skin from $p.", ch, corpse, NULL, TO_CHAR );
   obj_to_char( skin, ch );
}

bool can_use_slot( int slot )
{
   int sn;

   for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
      if( skill_table[sn]->slot == slot )
         return false;
   return true;
}

/* Lookup a skills information */
CMDF( do_slookup )
{
   SKILLTYPE *skill = NULL;
   char buf[MSL], arg[MIL];
   int sn, iClass, stat;

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Slookup what?\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "slots" ) )
   {
      int firstfree = -1, lastfree = 0, found = 0, cnt = 0;
      bool firstloop = true;

      send_to_char( "Free slots:\r\n", ch );
      for( iClass = 0; iClass < MAX_SKILL; iClass++ )
      {
         found = 0;

         for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
         {
            if( skill_table[sn]->slot == iClass )
            {
               /* On the first loop lets check for slots higher then MAX_SKILL */
               if( firstloop && skill_table[sn]->slot > MAX_SKILL )
                  ch_printf( ch, "SN %d has a slot higher then %d (MAX_SKILL).\r\n", sn, MAX_SKILL );
               found++;
            }
         }
         firstloop = false;
         /* Keep track of free slots */
         if( found == 0 )
         {
            if( firstfree == -1 )
               firstfree = iClass;
            lastfree = iClass;
            if( iClass < ( MAX_SKILL - 1 ) )
               continue;
         }
         if( firstfree != -1 )
         {
            if( lastfree != firstfree )
               snprintf( buf, sizeof( buf ), "[%4d - %-4d]", firstfree, lastfree );
            else
               snprintf( buf, sizeof( buf ), "[   %4d    ]", lastfree );
            ch_printf( ch, "%14s", buf );
            if( ++cnt == 6 )
            {
               cnt = 0;
               send_to_char( "\r\n", ch );
            }
         }
         else if( found > 1 && iClass != 0 ) /* 0 is basicaly not set */
         {
            if( cnt != 0 )
            {
               cnt = 0;
               send_to_char( "\r\n", ch );
            }
            ch_printf( ch, "%4d is used on more then one spell/skill.\r\n", iClass );
         }
         firstfree = -1;
         lastfree = -1;
      }
      if( cnt != 0 )
         send_to_char( "\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "all" ) )
   {
      bool found = false;

      for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
      {
         found = true;
         pager_printf( ch, "Sn: %4d %8s: '%-24s'",
            sn, skill_tname[skill_table[sn]->type], skill_table[sn]->name );
         if( skill_table[sn]->slot != -1 )
            pager_printf( ch, " Slot: %4d", skill_table[sn]->slot );
         else
            pager_printf( ch, "%11s", "" );
         if( SPELL_DAMAGE( skill_table[sn] ) != 0 )
            pager_printf( ch, " Damtype: %s", spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
         send_to_pager( "\r\n", ch );
      }
      if( !found )
         send_to_pager( "Nothing found to display.\r\n", ch );
      send_to_pager( "\r\n", ch );
   }
   else if( !str_cmp( arg, "herbs" ) )
   {
      for( sn = 0; sn < top_herb && herb_table[sn] && herb_table[sn]->name; sn++ )
         pager_printf( ch, "%d) %s\r\n", sn, herb_table[sn]->name );
   }
   else if( !str_cmp( arg, "personal" ) )
   {
      for( sn = 0; sn < top_pers && pers_table[sn] && pers_table[sn]->name; sn++ )
         pager_printf( ch, "%d) %s\r\n", sn, pers_table[sn]->name );
   }
   else
   {
      SMAUG_AFF *aff;
      int cnt = 0;

      if( arg[0] == 'h' && is_number( arg + 1 ) )
      {
         sn = atoi( arg + 1 );
         if( !is_valid_herb( sn ) )
         {
            send_to_char( "Invalid herb.\r\n", ch );
            return;
         }
         skill = herb_table[sn];
      }
      else if( arg[0] == 'p' && is_number( arg + 1 ) )
      {
         sn = atoi( arg + 1 );
         if( !is_valid_pers( sn ) )
         {
            send_to_char( "Invalid personal.\r\n", ch );
            return;
         }
         skill = pers_table[sn];
      }
      else if( is_number( arg ) )
      {
         sn = atoi( arg );
         if( !( skill = get_skilltype( sn ) ) )
         {
            send_to_char( "Invalid sn.\r\n", ch );
            return;
         }
         sn %= 1000;
      }
      else if( ( sn = skill_lookup( argument ) ) >= 0 )
         skill = skill_table[sn];
      else if( ( sn = herb_lookup( argument ) ) >= 0 )
         skill = herb_table[sn];
      else if( ( sn = pers_lookup( argument ) ) >= 0 )
         skill = pers_table[sn];
      else
      {
         send_to_char( "No such skill, spell, weapon, tongue, herb or personal.\r\n", ch );
         return;
      }

      if( !skill )
      {
         send_to_char( "Not created yet.\r\n", ch );
         return;
      }

      ch_printf( ch, "Sn: %4d Slot: %4d %s: '%-20s'\r\n", sn, skill->slot, skill_tname[skill->type], skill->name );

      if( skill->damage || skill->action || skill->Class || skill->power || skill->save )
         ch_printf( ch, "DamType: %s  ActType: %s   ClassType: %s   PowerType: %s  SaveEffect: %s\r\n",
            spell_damage[SPELL_DAMAGE( skill )], spell_action[SPELL_ACTION( skill )],
            spell_class[SPELL_CLASS( skill )],   spell_power[SPELL_POWER( skill )],
            spell_save_effect[SPELL_SAVE( skill )] );

      if( !xIS_EMPTY( skill->flags ) )
         ch_printf( ch, "Flags: %s\r\n", ext_flag_string( &skill->flags, spell_flag ) );

      ch_printf( ch, "Saves: %s\r\n", spell_saves[skill->saves] );

      if( skill->difficulty != 0 )
         ch_printf( ch, "Difficulty: %d\r\n", skill->difficulty );

      ch_printf( ch, "Magical: %s\r\n", skill->magical ? "Yes" : "No" );

      ch_printf( ch, "Type: %s  Target: %s  Minpos: %d[%s]  Mana: %d  Beats: %d  Range: %d\r\n",
         skill_tname[skill->type],
         target_type[URANGE( TAR_IGNORE, skill->target, TAR_OBJ_INV )],
         skill->minimum_position, pos_names[skill->minimum_position],
         skill->min_mana, skill->beats, skill->range );
      ch_printf( ch, "Value: %d  Code: %s\r\n",
         skill->value, skill->skill_fun ? skill->skill_fun_name : skill->spell_fun_name );
      ch_printf( ch, "Sectors Allowed: %s\r\n",
         !xIS_EMPTY( skill->spell_sector ) ? ext_flag_string( &skill->spell_sector, sect_flags ) : "All" );
      ch_printf( ch, "Dammsg: %s\r\nWearoff: %s\r\n",
         skill->noun_damage ? skill->noun_damage : "(None Set)", skill->msg_off ? skill->msg_off : "(None Set)" );

      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( skill->stats[stat] <= 0 )
            continue;
         ch_printf( ch, "Requires %d %s\r\n", skill->stats[stat], stattypes[stat] );
      }

      if( skill->req_skill != -1 )
      {
         int reqsn = skill->req_skill;

         ch_printf( ch, "Reqskill: %d", skill->req_skill );
         if( is_valid_sn( reqsn ) && skill_table[ reqsn ] )
            ch_printf( ch, "(%s)", skill_table[ reqsn ]->name );
         else
            send_to_char( "(UNKNOWN)", ch );
         send_to_char( "\r\n", ch );
      }
      if( skill->dice )
         ch_printf( ch, "Dice: %s\r\n", skill->dice );
      if( skill->teachers )
         ch_printf( ch, "Teachers: %s\r\n", skill->teachers );
      if( skill->components )
         ch_printf( ch, "Components: %s\r\n", skill->components );
      if( skill->participants )
         ch_printf( ch, "Participants: %d\r\n", skill->participants );
      if( skill->userec.num_uses )
         send_timer( &skill->userec, ch );

      for( aff = skill->first_affect; aff; aff = aff->next )
      {
         int modifier;

         if( aff == skill->first_affect )
            send_to_char( "\r\n", ch );
         snprintf( buf, sizeof( buf ), "Affect %d", ++cnt );
         if( aff->location )
         {
            if( ( aff->location % REVERSE_APPLY ) == APPLY_STAT )
               snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), 
                  " modifies %s by", ( aff->bitvector >= 0 && aff->bitvector < STAT_MAX ) ? stattypes[aff->bitvector] : "Unknown" );
            else if( ( aff->location % REVERSE_APPLY ) == APPLY_RESISTANT )
               snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), 
                  " modifies %s by", ( aff->bitvector >= 0 && aff->bitvector < RIS_MAX ) ? ris_flags[aff->bitvector] : "Unknown" );
            else
               snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), 
                  " modifies %s by", a_types[aff->location % REVERSE_APPLY] );
         }
         modifier = atoi( aff->modifier );
         if( ( ( aff->location % REVERSE_APPLY ) >= APPLY_WEAPONSPELL
         && ( aff->location % REVERSE_APPLY ) <= APPLY_STRIPSN )
         && is_valid_sn( modifier ) )
            snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), " %d", skill_table[modifier]->slot );
         else if( ( aff->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT
         && modifier >= 0 && modifier < AFF_MAX )
            snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), " %s", a_flags[modifier] );
         else
            snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), " %s", aff->modifier );
         if( aff->bitvector != -1 )
         {
            if( ( aff->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
               snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), " and applies %s",
                  ( aff->bitvector >= 0 && aff->bitvector < AFF_MAX ) ? a_flags[aff->bitvector] : "Unknown" );
         }
         if( aff->duration && aff->duration[0] != '\0' && aff->duration[0] != '0' )
         {
            snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), 
               " for '%s' rounds", aff->duration );
         }
         if( aff->location >= REVERSE_APPLY )
            mudstrlcat( buf, " (affects caster only)", sizeof( buf ) );
         mudstrlcat( buf, "\r\n", sizeof( buf ) );
         send_to_char( buf, ch );

         if( !aff->next )
            send_to_char( "\r\n", ch );
      }

      if( skill->hit_char )
         ch_printf( ch, "Hitchar   : %s\r\n", skill->hit_char );
      if( skill->hit_vict )
         ch_printf( ch, "Hitvict   : %s\r\n", skill->hit_vict );
      if( skill->hit_room )
         ch_printf( ch, "Hitroom   : %s\r\n", skill->hit_room );
      if( skill->hit_dest )
         ch_printf( ch, "Hitdest   : %s\r\n", skill->hit_dest );
      if( skill->miss_char )
         ch_printf( ch, "Misschar  : %s\r\n", skill->miss_char );
      if( skill->miss_vict )
         ch_printf( ch, "Missvict  : %s\r\n", skill->miss_vict );
      if( skill->miss_room )
         ch_printf( ch, "Missroom  : %s\r\n", skill->miss_room );
      if( skill->die_char )
         ch_printf( ch, "Diechar   : %s\r\n", skill->die_char );
      if( skill->die_vict )
         ch_printf( ch, "Dievict   : %s\r\n", skill->die_vict );
      if( skill->die_room )
         ch_printf( ch, "Dieroom   : %s\r\n", skill->die_room );
      if( skill->imm_char )
         ch_printf( ch, "Immchar   : %s\r\n", skill->imm_char );
      if( skill->imm_vict )
         ch_printf( ch, "Immvict   : %s\r\n", skill->imm_vict );
      if( skill->imm_room )
         ch_printf( ch, "Immroom   : %s\r\n", skill->imm_room );
      if( skill->abs_char )
         ch_printf( ch, "Abschar   : %s\r\n", skill->abs_char );
      if( skill->abs_vict )
         ch_printf( ch, "Absvict   : %s\r\n", skill->abs_vict );
      if( skill->abs_room )
         ch_printf( ch, "Absroom   : %s\r\n", skill->abs_room );
      if( skill->type != SKILL_HERB && skill->type != SKILL_PERSONAL )
      {
         send_to_char( "---------------------------[CLASS USE]---------------------------\r\n", ch );
         for( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )
         {
            if( !class_table[iClass] )
               continue;
            snprintf( buf, sizeof( buf ), "%11.11s) lvl: %3d max: %3d%%",
               class_table[iClass]->name, skill->skill_level[iClass], skill->skill_adept[iClass] );
            if( ( iClass > 0 ) && ( iClass % 2 == 1 ) )
               mudstrlcat( buf, "\r\n", sizeof( buf ) );
            else
               mudstrlcat( buf, "  ", sizeof( buf ) );
            send_to_char( buf, ch );
         }

         send_to_char( "\r\n---------------------------[ RACE USE]---------------------------\r\n", ch );
         for( iClass = 0; iClass < MAX_PC_RACE; iClass++ )
         {
            if( !race_table[iClass] )
               continue;
            snprintf( buf, sizeof( buf ), "%11.11s) lvl: %3d max: %3d%%",
               race_table[iClass]->name, skill->race_level[iClass], skill->race_adept[iClass] );
            if( ( iClass > 0 ) && ( iClass % 2 == 1 ) )
               mudstrlcat( buf, "\r\n", sizeof( buf ) );
            else
               mudstrlcat( buf, "  ", sizeof( buf ) );
            send_to_char( buf, ch );
         }
      }
      send_to_char( "\r\n", ch );
      if( skill->htext )
      {
         send_to_char( ".~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", ch );
         send_to_char( skill->htext, ch );
         send_to_char( ".~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", ch );
      }
   }
}

/*
 * Set a skill's attributes or what skills a player has.
 * High god command, with support for creating skills/spells/herbs/etc
 */
CMDF( do_sset )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill;
   char arg1[MIL], arg2[MIL];
   int value, sn, adept, stat;
   short astype = 0;
   bool fAll;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !ch->desc )
      return;

   switch( ch->substate )
   {
      default:
         break;

      case SUB_SHELP_EDIT:
         if( !( skill = ( SKILLTYPE * ) ch->dest_buf ) )
         {
            bug( "%s: sub_shelp_edit: NULL ch->dest_buf", __FUNCTION__ );
            stop_editing( ch );
            return;
         }
         STRFREE( skill->htext );
         skill->htext = copy_buffer( ch );
         stop_editing( ch );
         add_skill_help( skill, true );
         asskills( 0 ); /* All */
         return;
   }

   if( arg1 == NULL || arg2 == NULL || !argument || arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Usage: sset <victim>  <skill>      <value>\r\n", ch );
      send_to_char( "Usage: sset <victim>  all          <value>\r\n", ch );
      if( get_trust( ch ) >= PERM_HEAD )
      {
         send_to_char( "Usage: sset save   skill/herb/personal table\r\n", ch );
         send_to_char( "Usage: sset create skill/spell/herb/personal <new skill/spell/herb/personal name>\r\n", ch );
         send_to_char( "Usage: sset <sn>   help\r\n", ch );
         send_to_char( "Usage: sset <sn>   delete\r\n", ch );
         send_to_char( "Usage: sset <sn>   magical\r\n", ch );
         send_to_char( "Usage: sset <sn>   <field>      <value>\r\n", ch );
         send_to_char( "\r\nField being one of:\r\n", ch );
         send_to_char( "  code    type   sector  diechar   immchar    missvict    difficulty\r\n", ch );
         send_to_char( "  dice   beats   target  dieroom   immroom    reqskill  participants\r\n", ch );
         send_to_char( "  flag   class  abschar  dievict   immvict    rmaffect\r\n", ch );
         send_to_char( "  mana   value  absroom  hitchar   seffect    teachers\r\n", ch );
         send_to_char( "  name  affect  absvict  hitdest   wearoff   classtype\r\n", ch );
         send_to_char( "  race  dammsg  acttype  hitroom  misschar   powertype\r\n", ch );
         send_to_char( "  slot  minpos  damtype  hitvict  missroom  components\r\n", ch );
         send_to_char( "Affect having the fields: <location> <modfifier> [duration] [bitvector]\r\n", ch );
         send_to_char( "(See AFFECTTYPES for location, and AFFECTED_BY for bitvector)\r\n", ch );
      }
      send_to_char( "Skill being any skill or spell.\r\n", ch );
      return;
   }

   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( arg1, "save" ) && !str_cmp( argument, "table" ) )
   {
      if( !str_cmp( arg2, "skill" ) )
      {
         send_to_char( "Saving skill table...\r\n", ch );
         save_skill_table( false );
         save_classes( );
         save_races( ); 
         return;
      }
      if( !str_cmp( arg2, "herb" ) )
      {
         send_to_char( "Saving herb table...\r\n", ch );
         save_herb_table( false );
         return;
      }
      if( !str_cmp( arg2, "personal" ) )
      {
         send_to_char( "Saving personal table...\r\n", ch );
         save_pers_table( false );
         return;
      }
   }

   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( arg1, "create" )
   && ( !str_cmp( arg2, "skill" ) || !str_cmp( arg2, "herb" ) || !str_cmp( arg2, "spell" ) || !str_cmp( arg2, "personal" ) ) )
   {
      short type = SKILL_SKILL;

      if( !str_cmp( arg2, "herb" ) )
      {
         type = SKILL_HERB;
         astype = 2;
         if( top_herb >= MAX_HERB )
         {
            ch_printf( ch, "The current top herb is %d, which is the maximum.  "
                           "To add more herbs,\r\nMAX_HERB will have to be "
                           "raised in mud.h, and the mud recompiled.\r\n", top_herb );
            return;
         }
      }
      else if( !str_cmp( arg2, "personal" ) )
      {
         type = SKILL_PERSONAL;
         astype = 3;
         if( top_pers >= MAX_PERS )
         {
            ch_printf( ch, "The current top personal is %d, which is the maximum.  "
                           "To add more personals,\r\nMAX_PERS will have to be "
                           "raised in mud.h, and the mud recompiled.\r\n", top_pers );
            return;
         }
      }
      else
      {
         if( !str_cmp( arg2, "spell" ) )
            type = SKILL_SPELL;
         astype = 1;
         if( top_sn >= MAX_SKILL )
         {
            ch_printf( ch, "The current top sn is %d, which is the maximum.  "
                           "To add more skills,\r\nMAX_SKILL will have to be "
                           "raised in mud.h, and the mud recompiled.\r\n", top_sn );
            return;
         }
      }

      if( !can_use_skill_name( argument ) )
      {
         send_to_char( "There is already a skill/spell/weapon/tongue/herb/personal using that name.\r\n", ch );
         return;
      }

      if( !( skill = new_skill( ) ) )
      {
         send_to_char( "Failed to create a new spell/skill/weapon/tongue/herb/personal.\r\n", ch );
         bug( "%s: skill is NULL after new_skill", __FUNCTION__ );
         return;
      }
      if( type == SKILL_HERB )
      {
         int max, x;

         herb_table[top_herb++] = skill;
         for( max = x = 0; x < top_herb - 1; x++ )
            if( herb_table[x] && herb_table[x]->slot > max )
               max = herb_table[x]->slot;
         skill->slot = max + 1;
      }
      else if( type == SKILL_PERSONAL )
      {
         int max, x;

         pers_table[top_pers++] = skill;
         for( max = x = 0; x < top_pers - 1; x++ )
            if( pers_table[x] && pers_table[x]->slot > max )
               max = pers_table[x]->slot;
         skill->slot = max + 1;
      }
      else
         skill_table[top_sn++] = skill;
      skill->name = STRALLOC( argument );
      skill->spell_fun = spell_smaug;
      STRSET( skill->spell_fun_name, "spell_smaug" );
      skill->type = type;
      asskills( astype );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( arg1[0] == 'h' || arg1[0] == 'p' )
      sn = atoi( arg1 + 1 );
   else
      sn = atoi( arg1 );
   if( get_trust( ch ) >= PERM_HEAD
   && ( ( ( arg1[0] == 'h' || arg1[0] == 'p' ) && is_number( arg1 + 1 ) && ( sn = atoi( arg1 + 1 ) ) >= 0 )
   || ( is_number( arg1 ) && ( sn = atoi( arg1 ) ) >= 0 ) ) )
   {
      if( arg1[0] == 'h' )
      {
         if( sn >= top_herb )
         {
            send_to_char( "Herb number out of range.\r\n", ch );
            return;
         }
         skill = herb_table[sn];
         astype = 2;
      }
      else if( arg1[0] == 'p' )
      {
         if( sn >= top_pers )
         {
            send_to_char( "Personal number out of range.\r\n", ch );
            return;
         }
         skill = pers_table[sn];
         astype = 3;
      }
      else
      {
         if( !( skill = get_skilltype( sn ) ) )
         {
            send_to_char( "Skill number out of range.\r\n", ch );
            return;
         }
         sn %= 1000;
         astype = 1;
      }

      /* Allow the stat requirements to be set */
      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         int uvalue = atoi( argument );

         if( !str_cmp( arg2, stattypes[stat] ) )
         {
            if( uvalue < 0 || uvalue > MAX_LEVEL )
            {
               ch_printf( ch, "%s range is %d to %d.\r\n", capitalize( stattypes[stat] ), 0, MAX_LEVEL );
               return;
            }
            skill->stats[stat] = uvalue;
            asskills( astype );
            ch_printf( ch, "%s set to %d\r\n", capitalize( stattypes[stat] ), uvalue );
            return;
         }
      }

      switch( UPPER( arg2[0] ) )
      {
         case 'A':
            if( !str_cmp( arg2, "abschar" ) )
            {
               STRSET( skill->abs_char, argument );
               asskills( astype );
               send_to_char( "Abschar set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "absvict" ) )
            {
               STRSET( skill->abs_vict, argument );
               asskills( astype );
               send_to_char( "Absvict set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "absroom" ) )
            {
               STRSET( skill->abs_room, argument );
               asskills( astype );
               send_to_char( "Absroom set.\r\n", ch );
               return;
            }
            /* affect <location> <modifier> <duration> <bitvector> */
            if( !str_cmp( arg2, "affect" ) )
            {
               SMAUG_AFF *aff;
               char location[MIL], modifier[MIL], duration[MIL];
               int loc, bit, tmpbit;

               argument = one_argument( argument, location );
               argument = one_argument( argument, modifier );
               argument = one_argument( argument, duration );

               if( location[0] == '!' )
                  loc = get_flag( location + 1, a_types, APPLY_MAX ) + REVERSE_APPLY;
               else
                  loc = get_flag( location, a_types, APPLY_MAX );
               if( ( loc % REVERSE_APPLY ) < 0 || ( loc % REVERSE_APPLY ) >= APPLY_MAX )
               {
                  send_to_char( "Unknown affect location.  See AFFECTTYPES.\r\n", ch );
                  return;
               }

               bit = -1;
               if( argument && argument[0] != '\0' )
               {
                  if( ( loc % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
                  {
                     if( ( tmpbit = get_flag( argument, a_flags, AFF_MAX ) ) == -1 )
                     {
                        ch_printf( ch, "Unknown affect: %s.  See OBJECTAFFECTED\r\n", argument );
                        return;
                     }
                     else
                        bit = tmpbit;
                  }
                  else if( ( loc % REVERSE_APPLY ) == APPLY_STAT )
                  {
                     if( ( tmpbit = get_flag( argument, stattypes, STAT_MAX ) ) == -1 )
                     {
                        ch_printf( ch, "Unknown stat: %s.\r\n", argument );
                        return;
                     }
                     else
                        bit = tmpbit;
                  }
                  else if( ( loc % REVERSE_APPLY ) == APPLY_RESISTANT )
                  {
                     if( ( tmpbit = get_flag( argument, ris_flags, RIS_MAX ) ) == -1 )
                     {
                        ch_printf( ch, "Unknown ris: %s.\r\n", argument );
                        return;
                     }
                     else
                        bit = tmpbit;
                  }
               }
               else if( ( loc % REVERSE_APPLY ) == APPLY_STAT )
               {
                  send_to_char( "You have to specify what stat you wish to have it increase.\r\n", ch );
                  return;
               }
               else if( ( loc % REVERSE_APPLY ) == APPLY_RESISTANT )
               {
                  send_to_char( "You have to specify what ris you wish to have it modify.\r\n", ch );
                  return;
               }

               if( !str_cmp( duration, "0" ) )
                  duration[0] = '\0';
               if( !str_cmp( modifier, "0" ) )
                  modifier[0] = '\0';

               if( ( loc % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
               {
                  int modval = get_flag( modifier, a_flags, AFF_MAX );

                  if( modval < 0 || modval >= AFF_MAX )
                  {
                     modval = 0;
                     ch_printf( ch, "Unknown affect: %s.\r\n", modifier );
                     return;
                  }
                  snprintf( modifier, sizeof( modifier ), "%d", modval );
               }

               if( ( loc % REVERSE_APPLY ) >= APPLY_STRIPSN && ( loc % REVERSE_APPLY ) < APPLY_STAT )
                  if( duration == NULL || duration[0] == '\0' )
                     snprintf( duration, sizeof( duration ), "%d", 0 );
               if( ( loc % REVERSE_APPLY ) == APPLY_WAITSTATE && ( modifier == NULL || modifier[0] == '\0' ) )
               {
                  send_to_char( "You can't affect waitsate by nothing.\r\n", ch );
                  return;
               }
               CREATE( aff, SMAUG_AFF, 1 );
               if( !aff )
               {
                  bug( "%s: couldn't create an aff.\r\n", __FUNCTION__ );
                  return;
               }
               aff->duration = STRALLOC( duration );
               aff->location = loc;
               aff->modifier = STRALLOC( modifier );
               aff->bitvector = bit;
               LINK( aff, skill->first_affect, skill->last_affect, next, prev );
               asskills( astype );
               send_to_char( "Affect was added.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "acttype" ) )
            {
               int x = get_flag( argument, spell_action, SA_MAX );

               if( x < 0 || x >= SA_MAX )
                  send_to_char( "Not a spell action type.\r\n", ch );
               else
               {
                  SET_SACT( skill, x );
                  asskills( astype );
                 ch_printf( ch, "Acttype set to %s.\r\n", spell_action[SPELL_ACTION( skill )] );
               }
               return;
            }
            break;

         case 'B':
            if( !str_cmp( arg2, "beats" ) )
            {
               skill->beats = URANGE( 0, atoi( argument ), 120 );
               asskills( astype );
               ch_printf( ch, "Beats set to %d.\r\n", skill->beats );
               return;
            }
            break;

         case 'C':
            if( !str_cmp( arg2, "class" ) )
            {
               char arg3[MIL], arg4[MIL];
               int Class;

               argument = one_argument( argument, arg3 );
               argument = one_argument( argument, arg4 );

               if( arg3 == NULL || arg4 == NULL || !argument )
               {
                  send_to_char( "Usage: sset <sn> class <class> <level> <adept>\r\n", ch );
                  return;
               }
               if( is_number( arg3 ) )
                  Class = atoi( arg3 );
               else
                  Class = get_pc_class( arg3 );
               if( Class < 0 || Class >= MAX_PC_CLASS )
                  send_to_char( "That isn't a valid class.\r\n", ch );
               else
               {
                  skill->skill_level[Class] = URANGE( -1, atoi( arg4 ), MAX_LEVEL );
                  ch_printf( ch, "You have set that class level to %d.\r\n", skill->skill_level[Class] );
                  if( argument && argument[0] != '\0' )
                  {
                     skill->skill_adept[Class] = URANGE( 0, atoi( argument ), 100 );
                     ch_printf( ch, "You have set that class adept to %d.\r\n", skill->skill_adept[Class] );
                  }
                  asskills( astype );
               }
               return;
            }
            if( !str_cmp( arg2, "components" ) )
            {
               STRSET( skill->components, argument );
               asskills( astype );
               send_to_char( "Components set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "code" ) )
            {
               SPELL_FUN *spellfun;
               DO_FUN *dofun;

               if( !str_prefix( "do_", argument ) && ( dofun = skill_function( argument ) ) != skill_notfound )
               {
                  skill->skill_fun = dofun;
                  STRSET( skill->skill_fun_name, argument );
                  skill->spell_fun = NULL;
                  STRFREE( skill->spell_fun_name );
               }		
               else if( ( spellfun = spell_function( argument ) ) != spell_notfound )
               {
                  skill->spell_fun = spellfun;
                  STRSET( skill->spell_fun_name, argument );
                  skill->skill_fun = NULL;
                  STRFREE( skill->skill_fun_name );
               }
               else if( validate_spec_fun( argument ) )
               {
                  send_to_char( "Can't use a spec_fun for skills or spells.\r\n", ch );
                  return;
               }
               else
               {
                  send_to_char( "Not a spell or skill.\r\n", ch );
                  return;
               }
               asskills( astype );
               send_to_char( "Ok.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "classtype" ) )
            {
               int x = get_flag( argument, spell_class, SC_MAX );

               if( x < 0 || x >= SC_MAX )
                  send_to_char( "Not a spell class type.\r\n", ch );
               else
               {
                  SET_SCLA( skill, x );
                  asskills( astype );
                  ch_printf( ch, "Classtype set to %s.\r\n", spell_class[SPELL_CLASS( skill )] );
               }
               return;
            }
            break;

         case 'D':
            if( !str_cmp( arg2, "dice" ) )
            {
               STRSET( skill->dice, argument );
               asskills( astype );
               send_to_char( "Dice set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "dammsg" ) )
            {
               STRSET( skill->noun_damage, argument );
               asskills( astype );
               send_to_char( "Dammsg set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "diechar" ) )
            {
               STRSET( skill->die_char, argument );
               asskills( astype );
               send_to_char( "Diechar set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "dievict" ) )
            {
               STRSET( skill->die_vict, argument );
               asskills( astype );
               send_to_char( "Dievict set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "dieroom" ) )
            {
               STRSET( skill->die_room, argument );
               asskills( astype );
               send_to_char( "Dieroom set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "delete" ) )
            {
               skill->type = SKILL_DELETED;
               asskills( astype );
               send_to_char( "Set to be deleted on hotboot/reboot.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "damtype" ) )
            {
               int x = get_flag( argument, spell_damage, SD_MAX );
               if( x < 0 || x >= SD_MAX )
                  send_to_char( "Not a spell damage type.\r\n", ch );
               else
               {
                  SET_SDAM( skill, x );
                  asskills( astype );
                  ch_printf( ch, "Damtype set to %s.\r\n", spell_damage[SPELL_DAMAGE( skill )] );
               }
               return;
            }
            if( !str_cmp( arg2, "difficulty" ) )
            {
               skill->difficulty = URANGE( 0, atoi( argument ), 100 );
               asskills( astype );
               ch_printf( ch, "Difficulty set to %d.\r\n", skill->difficulty );
               return;
            }
            break;

         case 'F':
            if( !str_cmp( arg2, "flags" ) )
            {
               int x;
               bool sasave = false;

               if( !argument || argument[0] == '\0' )
               {
                  send_to_char( "What flag would you like to set?\r\n", ch );
                  return;
               }
               while( argument && argument[0] != '\0' )
               {
                  argument = one_argument( argument, arg2 );
                  x = get_flag( arg2, spell_flag, SF_MAX );
                  if( x < 0 || x >= SF_MAX )
                     ch_printf( ch, "Unknown flag %s.\r\n", arg2 );
                  else
                  {
                     xTOGGLE_BIT( skill->flags, x );
                     sasave = true;
                     ch_printf( ch, "Flag %s has been %s.\r\n", arg2, xIS_SET( skill->flags, x ) ? "set" : "unset" );
                  }
               }
               if( sasave )
                  asskills( astype );
               return;
            }
            break;

         case 'H':
            if( !str_cmp( arg2, "help" ) )
            {
               ch->substate = SUB_SHELP_EDIT;
               ch->dest_buf = skill;
               start_editing( ch, skill->htext );
               return;
            }
            if( !str_cmp( arg2, "hitchar" ) )
            {
               STRSET( skill->hit_char, argument );
               asskills( astype );
               send_to_char( "Hitchar set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "hitvict" ) )
            {
               STRSET( skill->hit_vict, argument );
               asskills( astype );
               send_to_char( "Hitvict set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "hitroom" ) )
            {
               STRSET( skill->hit_room, argument );
               asskills( astype );
               send_to_char( "Hitroom set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "hitdest" ) )
            {
               STRSET( skill->hit_dest, argument );
               asskills( astype );
               send_to_char( "Hitdest set.\r\n", ch );
               return;
            }
            break;

         case 'I':
            if( !str_cmp( arg2, "immchar" ) )
            {
               STRSET( skill->imm_char, argument );
               asskills( astype );
               send_to_char( "Immchar set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "immvict" ) )
            {
               STRSET( skill->imm_vict, argument );
               asskills( astype );
               send_to_char( "Immvict set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "immroom" ) )
            {
               STRSET( skill->imm_room, argument );
               asskills( astype );
               send_to_char( "Immroom set.\r\n", ch );
               return;
            }
            break;

         case 'M':
            if( !str_cmp( arg2, "misschar" ) )
            {
               STRSET( skill->miss_char, argument );
               asskills( astype );
               send_to_char( "Misschar set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "missvict" ) )
            {
               STRSET( skill->miss_vict, argument );
               asskills( astype );
               send_to_char( "Missvict set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "missroom" ) )
            {
               STRSET( skill->miss_room, argument );
               asskills( astype );
               send_to_char( "Missroom set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "mana" ) )
            {
               skill->min_mana = URANGE( 0, atoi( argument ), 2000 );
               asskills( astype );
               ch_printf( ch, "Mana set to %d.\r\n", skill->min_mana );
               return;
            }
            if( !str_cmp( arg2, "minpos" ) )
            {
               int x = get_flag( argument, pos_names, POS_MAX );

               if( x < 0 && is_number( argument ) )
                  x = atoi( argument );

               if( x < 0 || x >= POS_MAX )
                  ch_printf( ch, "Not a valid position (%s).\r\n", argument );
               else
               {
                  skill->minimum_position = x;
                  asskills( astype );
                  ch_printf( ch, "Minpos set to %d[%s].\r\n", skill->minimum_position, pos_names[skill->minimum_position] );
               }
               return;
            }
            if( !str_cmp( arg2, "magical" ) )
            {
               skill->magical = !skill->magical;
               asskills( astype );
               ch_printf( ch, "That skill is now %s.\r\n", skill->magical ? "magical" : "non magical" );
               return;
            }
            break;

         case 'N':
            if( !str_cmp( arg2, "name" ) )
            {
               if( !argument || argument[0] == '\0' )
               {
                  send_to_char( "You can't set the name field to nothing.\r\n", ch );
                  return;
               }
               if( !can_use_skill_name( argument ) )
               {
                  send_to_char( "There is already a skill/spell/weapon/tongue/herb/personal using that name.\r\n", ch );
                  return;
               }
               STRSET( skill->name, argument );
               asskills( astype );
               send_to_char( "Name set.\r\n", ch );
               return;
            }
            break;

         case 'P':
            if( !str_cmp( arg2, "participants" ) )
            {
               skill->participants = URANGE( 0, atoi( argument ), 100 );
               asskills( astype );
               ch_printf( ch, "Participants set to %d.\r\n", skill->participants );
               return;
            }
            if( !str_cmp( arg2, "powertype" ) )
            {
               int x = get_flag( argument, spell_power, SP_MAX );

               if( x < 0 || x >= SP_MAX )
                  send_to_char( "Not a spell power type.\r\n", ch );
               else
               {
                  SET_SPOW( skill, x );
                  asskills( astype );
                  ch_printf( ch, "Powertype set to %s.\r\n", spell_power[SPELL_POWER( skill )] );
               }
               return;
            }
            break;

         case 'R':
            if( !str_cmp( arg2, "rmaffect" ) )
            {
               SMAUG_AFF *aff = skill->first_affect;
               int num = atoi( argument );
               int cnt = 0;

               if( !aff )
               {
                  send_to_char( "This spell has no special affects to remove.\r\n", ch );
                  return;
               }
               for( ; aff; aff = aff->next )
               {
                  if( ++cnt == num )
                  {
                     UNLINK( aff, skill->first_affect, skill->last_affect, next, prev );
                     STRFREE( aff->duration );
                     STRFREE( aff->modifier );
                     DISPOSE( aff );
                     asskills( astype );
                     send_to_char( "Removed.\r\n", ch );
                     return;
                  }
               }
               send_to_char( "Not found.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "range" ) )
            {
               skill->range = URANGE( 0, atoi( argument ), 20 );
               asskills( astype );
               ch_printf( ch, "Range set to %d.\r\n", skill->range );
               return;
            }
            if( !str_cmp( arg2, "race" ) )
            {
               char arg3[MIL], arg4[MIL];
               int Class;

               argument = one_argument( argument, arg3 );
               argument = one_argument( argument, arg4 );

               if( arg3 == NULL || arg4 == NULL || !argument )
               {
                  send_to_char( "Usage: sset <sn> race <race> <level> <adept>\r\n", ch );
                  return;
               }
               if( is_number( arg3 ) )
                  Class = atoi( arg3 );
               else
                  Class = get_pc_race( arg3 );
               if( Class < 0 || Class >= MAX_PC_RACE )
                  send_to_char( "That isn't a valid race.\r\n", ch );
               else
               {
                  skill->race_level[Class] = URANGE( -1, atoi( arg4 ), MAX_LEVEL );
                  ch_printf( ch, "You have set that race level to %d.\r\n", skill->race_level[Class] );
                  if( argument && argument[0] != '\0' )
                  {
                     skill->race_adept[Class] = URANGE( 0, atoi( argument ), 100 );
                     ch_printf( ch, "You have set that race adept to %d.\r\n", skill->race_adept[Class] );
                  }
                  asskills( astype );
               }
               return;
            }
            if( !str_cmp( arg2, "reqskill" ) )
            {
               int x;

               if( is_number( argument ) )
                  x = atoi( argument );
               else
               {
                  if( ( x = skill_lookup( argument ) ) < 0 )
                  {
                     ch_printf( ch, "No skill called (%s) to use.\r\n", argument );
                     return;
                  }
               }
               if( x != -1 && !is_valid_sn( x ) )
               {
                  send_to_char( "You can't set it to that.\r\n", ch );
                  return;
               }
               skill->req_skill = URANGE( -1, x, top_sn );
               asskills( astype );
               if( skill->req_skill == -1 )
                  send_to_char( "Reqskill set to -1(Nothing).\r\n", ch );
               else
                  ch_printf( ch, "Reqskill set to %d(%s).\r\n", skill->req_skill, skill_table[ skill->req_skill ]->name );
               return;
            }
            break;

         case 'S':
            if( !str_cmp( arg2, "seffect" ) )
            {
               int x = get_flag( argument, spell_save_effect, SE_MAX );

               if( x < 0 || x >= SE_MAX )
                  send_to_char( "Not a spell save effect type.\r\n", ch );
               else
               {
                  SET_SSAV( skill, x );
                  asskills( astype );
                  ch_printf( ch, "Seffect set to %s.\r\n", spell_save_effect[SPELL_SAVE( skill )] );
               }
               return;
            }
            if( !str_cmp( arg2, "sector" ) )
            {
               char tmp_arg[MSL];
               bool asave = false;

               while( argument[0] != '\0' )
               {
                  argument = one_argument( argument, tmp_arg );
                  value = get_flag( tmp_arg, sect_flags, SECT_MAX );
                  if( value < 0 || value >= SECT_MAX )
                     ch_printf( ch, "Unknown sector: %s\r\n", tmp_arg );
                  else
                  {
                     xTOGGLE_BIT( skill->spell_sector, value );
                     asave = true;
                  }
               }
               if( asave )
                  asskills( astype );
               ch_printf( ch, "Sector set to %s.\r\n", ext_flag_string( &skill->spell_sector, sect_flags ) );
               return;
            }
            if( !str_cmp( arg2, "slot" ) )
            {
               int x = atoi( argument );

               if( x < -1 )
               {
                  send_to_char( "You can't set a slot to less then -1.\r\n", ch );
                  return;
               }
               if( x != -1 && !can_use_slot( x ) )
               {
                  send_to_char( "That slot is already being used.\r\n", ch );
                  return;
               }
               skill->slot = URANGE( -1, x, 30000 );
               asskills( astype );
               ch_printf( ch, "Slot set to %d.\r\n", skill->slot );
               return;
            }
            if( !str_cmp( arg2, "saves" ) )
            {
               int x = get_flag( argument, spell_saves, SA_MAX );

               if( x < 0 || x >= SA_MAX)
                  send_to_char( "Not a saving type.\r\n", ch );
               else
               {
                  skill->saves = x;
                  asskills( astype );
                  ch_printf( ch, "Saves set to %s.\r\n", spell_saves[skill->saves] );
               }
               return;
            }
            break;

         case 'T':
            if( !str_cmp( arg2, "teachers" ) )
            {
               STRSET( skill->teachers, argument );
               asskills( astype );
               send_to_char( "Teachers set.\r\n", ch );
               return;
            }
            if( !str_cmp( arg2, "type" ) )
            {
               skill->type = get_skill( argument );
               asskills( astype );
               ch_printf( ch, "Type set to %s.\r\n", skill_tname[skill->type] );
               return;
            }
            if( !str_cmp( arg2, "target" ) )
            {
               int x = get_flag( argument, target_type, TAR_MAX );

               if( x < 0 || x >= TAR_MAX )
                  send_to_char( "Not a valid target type.\r\n", ch );
               else
               {
                  skill->target = x;
                  asskills( astype );
                  ch_printf( ch, "Target set to %s.\r\n", target_type[URANGE( TAR_IGNORE, skill->target, TAR_OBJ_INV )] );
               }
               return;
            }
            break;

         case 'V':
            if( !str_cmp( arg2, "value" ) )
            {
               skill->value = atoi( argument );
               asskills( astype );
               ch_printf( ch, "Value set to %d.\r\n", skill->value );
               return;
            }
            break;

         case 'W':
            if( !str_cmp( arg2, "wearoff" ) )
            {
               STRSET( skill->msg_off, argument );
               asskills( astype );
               send_to_char( "Wearoff set.\r\n", ch );
               return;
            }
            break;
      }

      do_sset( ch, (char *)"" );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      if( ( sn = skill_lookup( arg1 ) ) >= 0 )
      {
         snprintf( arg1, sizeof( arg1 ), "%d %s %s", sn, arg2, argument );
         do_sset( ch, arg1 );
      }
      else
         send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   fAll = !str_cmp( arg2, "all" );
   sn = 0;
   if( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      send_to_char( "No such skill or spell.\r\n", ch );
      return;
   }

   /* Snarf the value. */
   if( !is_number( argument ) )
   {
      send_to_char( "Value must be numeric.\r\n", ch );
      return;
   }

   value = atoi( argument );
   if( value < 0 || value > 100 )
   {
      send_to_char( "Value range is 0 to 100.\r\n", ch );
      return;
   }

   if( fAll )
   {
      for( sn = 0; sn < top_sn; sn++ )
      {
         if( !skill_table[sn] || !skill_table[sn]->name )
            continue;
         adept = get_adept( victim, sn );
         victim->pcdata->learned[sn] = ( value > adept ) ? adept : value;
      }
      send_to_char( "You have set all of their skills.\r\n", ch );
      send_to_char( "All of your skills have been set.\r\n", victim );
   }
   else
   {
      adept = get_adept( victim, sn );
      victim->pcdata->learned[sn] = ( value > adept ) ? adept : value;
      send_to_char( "You have set their skill.\r\n", ch );
      send_to_char( "One of your skills has been set.\r\n", victim );
   }
}

void learn_from_success( CHAR_DATA *ch, int sn )
{
   int adept, learn, percent, schance;

   if( is_npc( ch ) || ch->pcdata->learned[sn] <= 0 || !skill_table[sn] )
      return;
   adept = get_adept( ch, sn );
   if( ch->pcdata->learned[sn] < adept )
   {
      schance = ( ch->pcdata->learned[sn] + skill_table[sn]->difficulty );
      percent = number_percent( );
      if( percent >= schance )
         learn = 2;
      else if( schance - percent > 25 )
         return;
      else
         learn = 1;
      ch->pcdata->learned[sn] = UMIN( adept, ch->pcdata->learned[sn] + learn );
      if( ch->pcdata->learned[sn] == adept ) /* fully learned! */
      {
         int exp = ( 100 * urange( 1, first_learned( ch, sn ), MAX_LEVEL ) );

         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You're now an adept of %s!\r\n", skill_table[sn]->name );
         gain_exp( ch, exp );
      }
   }
}

void learn_from_failure( CHAR_DATA *ch, int sn )
{
   int adept, schance;

   if( is_npc( ch ) || ch->pcdata->learned[sn] <= 0 || !skill_table[sn] )
      return;
   schance = ch->pcdata->learned[sn] + skill_table[sn]->difficulty;
   if( schance - number_percent( ) > 25 )
      return;
   adept = get_adept( ch, sn );
   if( ch->pcdata->learned[sn] < ( adept - 1 ) )
      ch->pcdata->learned[sn] = UMIN( adept, ch->pcdata->learned[sn] + 1 );
}

CMDF( do_gouge )
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   int schance;
   short dam;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !can_use_skill( ch, 0, gsn_gouge ) )
   {
      send_to_char( "You don't know of this skill.\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't get close enough while mounted.\r\n", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   schance = ( ( get_curr_dex( victim ) - get_curr_dex( ch ) ) * 10 ) + 10;
   if( can_use_skill( ch, ( number_percent( ) + schance ), gsn_gouge ) )
   {
      dam = number_range( 5, ch->level );
      global_retcode = damage( ch, victim, NULL, dam, gsn_gouge, false );
      if( global_retcode == rNONE )
      {
         if( !IS_AFFECTED( victim, AFF_BLIND ) )
         {
            af.type = gsn_blindness;
            af.location = APPLY_EXT_AFFECT;
            af.modifier = AFF_BLIND;
            if( !is_npc( victim ) && !is_npc( ch ) )
               af.duration = ( ch->level + 10 ) / get_curr_con( victim );
            else
               af.duration = 3 + ( ch->level / 15 );
            af.bitvector = meb( AFF_BLIND );
            affect_to_char( victim, &af );
            act( AT_SKILL, "You can't see a thing!", victim, NULL, NULL, TO_CHAR );
         }
         wait_state( ch, PULSE_VIOLENCE );
         if( !is_npc( ch ) && !is_npc( victim ) )
         {
            if( number_bits( 1 ) == 0 )
            {
               ch_printf( ch, "%s looks momentarily dazed.\r\n", victim->name );
               send_to_char( "You're momentarily dazed ...\r\n", victim );
               wait_state( victim, PULSE_VIOLENCE );
            }
         }
         else
            wait_state( victim, PULSE_VIOLENCE );
         /*
          * Taken out by request - put back in by Thoric
          * * This is how it was designed.  You'd be a tad stunned
          * * if someone gouged you in the eye.
          * * Mildly modified by Blodkai, Feb 1998 at request of
          * * of pkill Conclave (peaceful use remains the same)
          */
      }
      else if( global_retcode == rVICT_DIED )
      {
         act( AT_BLOOD, "Your fingers plunge into your victim's brain, causing immediate death!", ch, NULL, NULL, TO_CHAR );
      }
      if( global_retcode != rCHAR_DIED && global_retcode != rBOTH_DIED )
         learn_from_success( ch, gsn_gouge );
   }
   else
   {
      wait_state( ch, skill_table[gsn_gouge]->beats );
      global_retcode = damage( ch, victim, NULL, 0, gsn_gouge, false );
      learn_from_failure( ch, gsn_gouge );
   }
}

CMDF( do_detrap )
{
   OBJ_DATA *obj, *trap;
   char arg[MIL];
   int percent;
   bool found = false;

   switch( ch->substate )
   {
      default:
         if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         if( !can_use_skill( ch, 0, gsn_detrap ) )
         {
            send_to_char( "You don't know of this skill.\r\n", ch );
            return;
         }
         if( arg == NULL || arg[0] == '\0' )
         {
            send_to_char( "Detrap what?\r\n", ch );
            return;
         }
         if( ms_find_obj( ch ) )
            return;
         found = false;
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         if( !ch->in_room->first_content )
         {
            send_to_char( "You can't find that here.\r\n", ch );
            return;
         }
         for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
         {
            if( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
            {
               found = true;
               break;
            }
         }
         if( !found )
         {
            send_to_char( "You can't find that here.\r\n", ch );
            return;
         }
         act( AT_ACTION, "You carefully begin your attempt to remove a trap from $p...", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n carefully attempts to remove a trap from $p...", ch, obj, NULL, TO_ROOM );
         ch->alloc_ptr = STRALLOC( obj->name );
         add_timer( ch, TIMER_DO_FUN, 3, do_detrap, 1 );
         return;

      case 1:
         if( ch->alloc_ptr )
         {
            mudstrlcpy( arg, ch->alloc_ptr, sizeof( arg ) );
            STRFREE( ch->alloc_ptr );
            ch->alloc_ptr = NULL;
         }
         ch->substate = SUB_NONE;
         break;

      case SUB_TIMER_DO_ABORT:
         STRFREE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You carefully stop what you were doing.\r\n", ch );
         return;
   }

   if( !ch->in_room->first_content )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }
   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
      {
         found = true;
         break;
      }
   }
   if( !found )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }
   if( !( trap = get_trap( obj ) ) )
   {
      send_to_char( "You find no trap on that.\r\n", ch );
      return;
   }

   percent = number_percent( ) - ( ch->level / 15 ) - ( get_curr_lck( ch ) - 16 );

   separate_obj( obj );
   if( !can_use_skill( ch, percent, gsn_detrap ) )
   {
      send_to_char( "Ooops!\r\n", ch );
      spring_trap( ch, trap );
      learn_from_failure( ch, gsn_detrap );
      return;
   }

   extract_obj( trap );

   send_to_char( "You successfully remove a trap.\r\n", ch );
   learn_from_success( ch, gsn_detrap );
}

CMDF( do_trapset )
{
   OBJ_DATA *obj = NULL, *trap = NULL, *otrap = NULL;
   char arg[MIL];
   int percent;
   bool found = false, uroom = false;

   switch( ch->substate )
   {
      default:
         if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         if( !can_use_skill( ch, 0, gsn_trapset ) )
         {
            send_to_char( "You don't know of this skill.\r\n", ch );
            return;
         }
         if( arg == NULL || arg[0] == '\0' )
         {
            send_to_char( "Trapset what?\r\n", ch );
            return;
         }
         if( ms_find_obj( ch ) )
            return;
         found = false;
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         if( !str_cmp( arg, "room" ) )
            uroom = true;
         for( obj = ch->first_carrying; obj; obj = obj->next_content )
         {
            if( can_see_obj( ch, obj ) && obj->item_type == ITEM_TRAP )
            {
               trap = obj;
               break;
            }
         }
         if( !trap )
         {
            send_to_char( "You aren't carrying any trap to set.\r\n", ch );
            return;
         }
         if( !uroom )
         {
            if( !ch->in_room->first_content )
            {
               send_to_char( "You can't find that here.\r\n", ch );
               return;
            }
            for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
            {
               if( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
               {
                  found = true;
                  break;
               }
            }
            if( !found )
            {
               send_to_char( "You can't find that here.\r\n", ch );
               return;
            }
            act( AT_ACTION, "You carefully begin your attempt to trap $p...", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n carefully attempts to trap $p...", ch, obj, NULL, TO_ROOM );
            ch->alloc_ptr = STRALLOC( obj->name );
         }
         else
         {
            act( AT_ACTION, "You carefully begin your attempt to trap the room...", ch, NULL, NULL, TO_CHAR );
            act( AT_ACTION, "$n carefully attempts to trap the room...", ch, NULL, NULL, TO_ROOM );
            ch->alloc_ptr = STRALLOC( "room" );
         }
         add_timer( ch, TIMER_DO_FUN, 3, do_trapset, 1 );
         return;

      case 1:
         if( ch->alloc_ptr )
         {
            mudstrlcpy( arg, ch->alloc_ptr, sizeof( arg ) );
            STRFREE( ch->alloc_ptr );
            ch->alloc_ptr = NULL;
         }
         ch->substate = SUB_NONE;
         break;

      case SUB_TIMER_DO_ABORT:
         STRFREE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You carefully stop what you were doing.\r\n", ch );
         return;
   }

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( can_see_obj( ch, obj ) && obj->item_type == ITEM_TRAP )
      {
         trap = obj;
         break;
      }
   }
   if( !trap )
   {
      send_to_char( "You aren't carrying any trap to set.\r\n", ch );
      return;
   }
   separate_obj( trap );

   if( !str_cmp( arg, "room" ) )
      uroom = true;

   if( !uroom )
   {
      if( !ch->in_room->first_content )
      {
         send_to_char( "You can't find that here.\r\n", ch );
         return;
      }

      for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
      {
         if( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "You can't find that here.\r\n", ch );
         return;
      }

      separate_obj( obj );

      if( ( otrap = get_trap( obj ) ) )
      {
         if( number_percent( ) < 5 ) /* 5% chance of setting off the trap before you realize it is there */
         {
            send_to_char( "Ooops!\r\n", ch );
            spring_trap( ch, otrap );
            learn_from_failure( ch, gsn_trapset );
            return;
         }
         send_to_char( "You realize that object is already trapped.\r\n", ch );
         return;
      }

      /* Set it to be a trap in an object */
      SET_BIT( trap->value[3], TRAP_OBJ );
      /* Set it to go off for these reasons */
      SET_BIT( trap->value[3], TRAP_OPEN );
      SET_BIT( trap->value[3], TRAP_CLOSE );
      SET_BIT( trap->value[3], TRAP_GET );
      SET_BIT( trap->value[3], TRAP_PUT );
      SET_BIT( trap->value[3], TRAP_PICK );
      SET_BIT( trap->value[3], TRAP_UNLOCK );
      xSET_BIT( trap->extra_flags, ITEM_PTRAP );
      obj_from_char( trap );
      obj_to_obj( trap, obj );
   }
   else
   {
      if( ( otrap = get_room_trap( ch->in_room ) ) )
      {
         if( number_percent( ) < 5 ) /* 5% chance of setting off the trap before you realize it is there */
         {
            send_to_char( "Ooops!\r\n", ch );
            spring_trap( ch, otrap );
            learn_from_failure( ch, gsn_trapset );
            return;
         }
         send_to_char( "You realize that the room is already trapped.\r\n", ch );
         return;
      }
      /* Set it to be a trap in the room */
      SET_BIT( trap->value[3], TRAP_ROOM );
      /* Set it to go off for these reasons */
      SET_BIT( trap->value[3], TRAP_ENTER_ROOM );
      xSET_BIT( trap->extra_flags, ITEM_PTRAP );
      obj_from_char( trap );
      obj_to_room( trap, ch->in_room );
   }

   percent = number_percent( ) - ( ch->level / 15 ) - ( get_curr_lck( ch ) - 16 );
   if( !can_use_skill( ch, percent, gsn_trapset ) )
   {
      send_to_char( "Ooops!\r\n", ch );
      spring_trap( ch, trap );
      learn_from_failure( ch, gsn_trapset );
      return;
   }

   send_to_char( "You successfully set a trap.\r\n", ch );
   learn_from_success( ch, gsn_trapset );
}

CMDF( do_dig )
{
   OBJ_DATA *obj, *startobj, *shovel = NULL;
   EXIT_DATA *pexit;
   char arg[MIL];
   int dir = -1;
   bool found = false;

   arg[0] = '\0';

   switch( ch->substate )
   {
      default:
         if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         one_argument( argument, arg );
         if( arg != NULL && arg[0] != '\0' )
         {
            if( is_in_wilderness( ch ) )
            {
               send_to_char( "You can't dig out an exit while in the wilderness.\r\n", ch );
               return;
            }

            if( !( pexit = find_door( ch, arg, true ) ) && ( dir = get_dir( arg ) ) == -1 )
            {
               send_to_char( "What direction is that?\r\n", ch );
               return;
            }
            if( pexit )
            {
               if( !xIS_SET( pexit->exit_info, EX_DIG ) && !xIS_SET( pexit->exit_info, EX_CLOSED ) )
               {
                  send_to_char( "There is no need to dig out that exit.\r\n", ch );
                  return;
               }
            }

            if( dir != -1 )
            {
               ch_printf( ch, "You begin digging %s...\r\n", capitalize( dir_name[dir] ) );
               act_printf( AT_PLAIN, ch, NULL, NULL, TO_ROOM, "$n begins digging %s...", capitalize( dir_name[dir] ) );
               found = true;
            }
         }
         else
         {
            if( ch->in_room->sector_type == SECT_CITY || ch->in_room->sector_type == SECT_INSIDE )
            {
               send_to_char( "The floor is too hard to dig through.\r\n", ch );
               return;
            }
            else if( ch->in_room->sector_type == SECT_WATER_SWIM
            || ch->in_room->sector_type == SECT_WATER_NOSWIM
            || ch->in_room->sector_type == SECT_UNDERWATER )
            {
               send_to_char( "You can't dig here.\r\n", ch );
               return;
            }
            else if( ch->in_room->sector_type == SECT_AIR )
            {
               send_to_char( "What?  In the air?!\r\n", ch );
               return;
            }
         }

         if( !found )
         {
            send_to_char( "You begin digging...\r\n", ch );
            act( AT_PLAIN, "$n begins digging...", ch, NULL, NULL, TO_ROOM );
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_dig]->beats / 10, 3 ), do_dig, 1 );
         if( arg != NULL && arg[0] != '\0' )
            ch->alloc_ptr = STRALLOC( arg );
         return;

      case 1:
         if( ch->alloc_ptr )
         {
            mudstrlcpy( arg, ch->alloc_ptr, sizeof( arg ) );
            STRFREE( ch->alloc_ptr );
         }
         break;

      case SUB_TIMER_DO_ABORT:
         if( ch->alloc_ptr )
         {
            if( ( dir = get_dir( ch->alloc_ptr ) ) != -1 )
            {
               ch_printf( ch, "You stop digging %s...\r\n", capitalize( dir_name[dir] ) );
               act_printf( AT_PLAIN, ch, NULL, NULL, TO_ROOM, "$n stops digging %s...", capitalize( dir_name[dir] ) );
               found = true;
            }
            STRFREE( ch->alloc_ptr );
         }

         if( !found )
         {
            send_to_char( "You stop digging...\r\n", ch );
            act( AT_PLAIN, "$n stops digging...", ch, NULL, NULL, TO_ROOM );
         }
         ch->substate = SUB_NONE;
         return;
   }

   ch->substate = SUB_NONE;

   shovel = get_eq_hold( ch, ITEM_SHOVEL );

   if( arg != NULL && arg[0] != '\0' )
   {
      if( ( pexit = find_door( ch, arg, true ) )
      && xIS_SET( pexit->exit_info, EX_DIG )
      && xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         if( can_use_skill( ch, ( number_percent( ) * ( shovel ? 1 : 4 ) ), gsn_dig ) )
         {
            /* Don't remove the dig if it is on default, so when the area resets it closes it and it has to be redug */
            if( !xIS_SET( pexit->base_info, EX_DIG ) )
               xREMOVE_BIT( pexit->exit_info, EX_DIG );
            xREMOVE_BIT( pexit->exit_info, EX_CLOSED );
            send_to_char( "You dig open a passageway!\r\n", ch );
            act( AT_PLAIN, "$n digs open a passageway!", ch, NULL, NULL, TO_ROOM );
            learn_from_success( ch, gsn_dig );

            if( shovel && --shovel->value[0] <= 0 )
            {
               if( is_obj_stat( shovel, ITEM_NOSCRAP ) )
                  shovel->value[0] = 1;
               else
               {
                  act( AT_SKILL, "Your $p breaks as you finish digging!", ch, shovel, NULL, TO_CHAR );
                  extract_obj( shovel );
               }
            }

            return;
         }
      }

      learn_from_failure( ch, gsn_dig );

      if( ( dir = get_dir( arg ) ) != -1 )
      {
         ch_printf( ch, "Your dig did not discover any exit %s...\r\n", capitalize( dir_name[dir] ) );
         act_printf( AT_PLAIN, ch, NULL, NULL, TO_ROOM, "$n's dig did not discover any exit %s...", capitalize( dir_name[dir] ) );
      }
      else
      {
         send_to_char( "Your dig did not discover any exit...\r\n", ch );
         act( AT_PLAIN, "$n's dig did not discover any exit...", ch, NULL, NULL, TO_ROOM );
      }

      if( shovel && --shovel->value[0] <= 0 )
      {
         if( is_obj_stat( shovel, ITEM_NOSCRAP ) )
            shovel->value[0] = 1;
         else
         {
            act( AT_SKILL, "Your $p breaks as you finish digging!", ch, shovel, NULL, TO_CHAR );
            extract_obj( shovel );
         }
      }

      return;
   }

   startobj = ch->in_room->first_content;
   found = false;

   for( obj = startobj; obj; obj = obj->next_content )
   {
      if( is_obj_stat( obj, ITEM_WILDERNESS ) )
      {
         if( !is_in_wilderness( ch ) )
            continue;
         if( obj->cords[0] != ch->cords[0] || obj->cords[1] != ch->cords[1] )
            continue;
      }
      if( is_obj_stat( obj, ITEM_BURIED ) && ( can_use_skill( ch, ( number_percent( ) * ( shovel ? 1 : 2 ) ), gsn_dig ) ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "Your dig uncovered nothing.\r\n", ch );
      act( AT_PLAIN, "$n's dig uncovered nothing.", ch, NULL, NULL, TO_ROOM );

      if( shovel && --shovel->value[0] <= 0 )
      {
         if( is_obj_stat( shovel, ITEM_NOSCRAP ) )
            shovel->value[0] = 1;
         else
         {
            act( AT_SKILL, "Your $p breaks as you finish digging!", ch, shovel, NULL, TO_CHAR );
            extract_obj( shovel );
         }
      }

      learn_from_failure( ch, gsn_dig );
      return;
   }

   separate_obj( obj );
   xREMOVE_BIT( obj->extra_flags, ITEM_BURIED );
   act( AT_SKILL, "Your dig uncovered $p!", ch, obj, NULL, TO_CHAR );
   act( AT_SKILL, "$n's dig uncovered $p!", ch, obj, NULL, TO_ROOM );

   if( shovel && --shovel->value[0] <= 0 )
   {
      if( is_obj_stat( shovel, ITEM_NOSCRAP ) )
         shovel->value[0] = 1;
      else
      {
         act( AT_SKILL, "Your $p breaks as you finish digging!", ch, shovel, NULL, TO_CHAR );
         extract_obj( shovel );
      }
   }

   learn_from_success( ch, gsn_dig );
   if( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC )
      adjust_favor( ch, 11, 1 );
}

CMDF( do_search )
{
   OBJ_DATA *obj, *container, *startobj;
   char arg[MIL];
   int percent, door;

   door = -1;
   arg[0] = '\0';
   switch( ch->substate )
   {
      default:
         if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         if( arg != NULL && arg[0] != '\0' && ( door = get_door( arg ) ) == -1 )
         {
            if( !( container = get_obj_here( ch, arg ) ) )
            {
               send_to_char( "You can't find that here.\r\n", ch );
               return;
            }
            if( container->item_type != ITEM_CONTAINER )
            {
               send_to_char( "You can't search in that!\r\n", ch );
               return;
            }
            if( IS_SET( container->value[1], CONT_CLOSED ) )
            {
               send_to_char( "It is closed.\r\n", ch );
               return;
            }
            ch->alloc_ptr = STRALLOC( arg );
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_search]->beats / 10, 3 ), do_search, 1 );
         send_to_char( "You begin your search...\r\n", ch );
         return;

      case 1:
         if( ch->alloc_ptr )
         {
            mudstrlcpy( arg, ch->alloc_ptr, sizeof( arg ) );
            STRFREE( ch->alloc_ptr );
         }
         break;

      case SUB_TIMER_DO_ABORT:
         STRFREE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop your search...\r\n", ch );
         return;
   }
   ch->substate = SUB_NONE;
   if( arg == NULL || arg[0] == '\0' )
      startobj = ch->in_room->first_content;
   else
   {
      if( ( door = get_door( arg ) ) != -1 )
         startobj = NULL;
      else
      {
         if( !( container = get_obj_here( ch, arg ) ) )
         {
            send_to_char( "You can't find that here.\r\n", ch );
            return;
         }
         startobj = container->first_content;
      }
   }

   if( ( !startobj && door == -1 ) || is_npc( ch ) )
   {
      send_to_char( "You find nothing.\r\n", ch );
      learn_from_failure( ch, gsn_search );
      return;
   }

   percent = number_percent( ) + number_percent( ) - ( ch->level / 10 );

   if( door != -1 )
   {
      EXIT_DATA *pexit;

      if( ( pexit = get_exit( ch->in_room, door ) )
      && !is_in_wilderness( ch )
      && xIS_SET( pexit->exit_info, EX_SECRET )
      && xIS_SET( pexit->exit_info, EX_xSEARCHABLE ) && can_use_skill( ch, percent, gsn_search ) )
      {
         act( AT_SKILL, "Your search reveals the $d!", ch, NULL, pexit->keyword, TO_CHAR );
         act( AT_SKILL, "$n finds the $d!", ch, NULL, pexit->keyword, TO_ROOM );
         xREMOVE_BIT( pexit->exit_info, EX_SECRET );
         learn_from_success( ch, gsn_search );
         return;
      }
   }
   else
   {
      for( obj = startobj; obj; obj = obj->next_content )
      {
         if( is_obj_stat( obj, ITEM_WILDERNESS ) )
         {
            if( !is_in_wilderness( ch ) )
               continue;
            if( obj->cords[0] != ch->cords[0] || obj->cords[1] != ch->cords[1] )
               continue;
         }
         if( is_obj_stat( obj, ITEM_HIDDEN ) && can_use_skill( ch, percent, gsn_search ) )
         {
            separate_obj( obj );
            xREMOVE_BIT( obj->extra_flags, ITEM_HIDDEN );
            act( AT_SKILL, "Your search reveals $p!", ch, obj, NULL, TO_CHAR );
            act( AT_SKILL, "$n finds $p!", ch, obj, NULL, TO_ROOM );
            learn_from_success( ch, gsn_search );
            return;
         }
      }
   }
   send_to_char( "You find nothing.\r\n", ch );
   learn_from_failure( ch, gsn_search );
}

CMDF( do_steal )
{
   CHAR_DATA *victim, *mst;
   OBJ_DATA *obj;
   char buf[MSL], arg1[MIL], arg2[MIL];
   int percent;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( arg1 == NULL || arg2 == NULL || arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Steal what from whom?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( victim = get_char_room( ch, arg2 ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "That's pointless.\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "A magical force interrupts you.\r\n", ch );
      return;
   }

   if( !is_npc( ch ) && !is_npc( victim ) )
   {
      set_char_color( AT_IMMORT, ch );
      send_to_char( "The gods forbid theft between players.\r\n", ch );
      return;
   }

   if( is_npc( victim ) && ( victim->pIndexData->pShop || victim->pIndexData->rShop ) )
   {
      set_char_color( AT_IMMORT, ch );
      send_to_char( "The gods forbid theft from stores.\r\n", ch );
      return;
   }

   if( xIS_SET( victim->act, ACT_PACIFIST ) )   /* Gorog */
   {
      send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_steal]->beats );
   percent = number_percent( ) + ( is_awake( victim ) ? 10 : -50 )
      - ( get_curr_lck( ch ) - 15 ) + ( get_curr_lck( victim ) - 13 );

   /*
    * Changed the level check, made it 10 levels instead of five and made the 
    * victim not attack in the case of a too high level difference.  This is 
    * to allow mobprogs where the mob steals eq without having to put level 
    * checks into the progs.  Also gave the mobs a 10% chance of failure.
    */
   if( ch->level + 10 < victim->level )
   {
      send_to_char( "You really don't want to try that!\r\n", ch );
      return;
   }

   if( victim->position == POS_FIGHTING || !can_use_skill( ch, percent, gsn_steal ) )
   {
      /* Failure. */
      send_to_char( "Oops...\r\n", ch );
      act( AT_ACTION, "$n tried to steal from you!\r\n", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n tried to steal from $N.\r\n", ch, NULL, victim, TO_NOTVICT );

      snprintf( buf, sizeof( buf ), "yell %s is a bloody thief!", ch->name );
      interpret( victim, buf );

      learn_from_failure( ch, gsn_steal );
      if( !is_npc( ch ) )
      {
         if( legal_loot( ch, victim ) )
         {
            global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
         }
         else
         {
            if( is_npc( ch ) )
            {
               if( !( mst = ch->master ) )
                  return;
            }
            else
               mst = ch;
            if( is_npc( mst ) )
               return;
         }
      }
      return;
   }

   if( !str_cmp( arg1, "coin" ) || !str_cmp( arg1, "coins" ) || !str_cmp( arg1, "gold" ) )
   {
      int amount;

      amount = ( int )( victim->gold * number_range( 1, 10 ) / 100 );
      if( amount <= 0 )
      {
         send_to_char( "You couldn't get any gold.\r\n", ch );
         learn_from_failure( ch, gsn_steal );
         return;
      }

      decrease_gold( victim, amount );
      increase_gold( ch, amount );
      ch_printf( ch, "Aha!  You got %s gold coins.\r\n", num_punct( amount ) );
      learn_from_success( ch, gsn_steal );
      return;
   }

   if( !( obj = get_obj_carry( victim, arg1 ) ) )
   {
      send_to_char( "You can't seem to find it.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( !can_drop_obj( victim, obj ) || is_obj_stat( obj, ITEM_INVENTORY )
   || is_obj_stat( obj, ITEM_PROTOTYPE ) || obj->level > ch->level )
   {
      send_to_char( "You can't manage to pry it away.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( ch->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( ch ) )
   {
      send_to_char( "You have your hands full.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( ch->carry_weight + ( get_obj_weight( obj ) / obj->count ) > can_carry_w( ch ) )
   {
      send_to_char( "You can't carry that much weight.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   separate_obj( obj );
   obj_from_char( obj );
   obj_to_char( obj, ch );
   send_to_char( "Ok.\r\n", ch );
   learn_from_success( ch, gsn_steal );
   adjust_favor( ch, 7, 1 );
}

CMDF( do_backstab )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   char arg[MIL];
   int percent;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't do that right now.\r\n", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Backstab whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't get close enough while mounted.\r\n", ch );
      return;
   }

   if( is_safe( ch, victim, true ) )
      return;

   if( !is_npc( ch ) && !is_npc( victim ) && xIS_SET( ch->act, PLR_NICE ) )
   {
      send_to_char( "You're too nice to do that.\r\n", ch );
      return;
   }

   if( !( obj = get_eq_hold( ch, ITEM_WEAPON ) )
   || ( obj->value[3] != DAM_PIERCE && obj->value[3] != DAM_STAB ) )
   {
      send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
      return;
   }

   if( victim->fighting )
   {
      send_to_char( "You can't backstab someone who is in combat.\r\n", ch );
      return;
   }

   /* Can backstab a char even if it's hurt as long as it's sleeping. -Narn */
   if( victim->hit < victim->max_hit && is_awake( victim ) )
   {
      act( AT_PLAIN, "$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent( ) - ( get_curr_lck( ch ) - 14 ) + ( get_curr_lck( victim ) - 13 );

   wait_state( ch, skill_table[gsn_backstab]->beats );
   if( !is_awake( victim ) || can_use_skill( ch, percent, gsn_backstab ) )
   {
      learn_from_success( ch, gsn_backstab );
      global_retcode = multi_hit( ch, victim, gsn_backstab );
      adjust_favor( ch, 8, 1 );

   }
   else
   {
      learn_from_failure( ch, gsn_backstab );
      global_retcode = damage( ch, victim, NULL, 0, gsn_backstab, true );
   }
}

CMDF( do_rescue )
{
   CHAR_DATA *victim, *fch;
   int percent;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "You aren't thinking clearly...\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Rescue whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How about fleeing instead?\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( !is_npc( ch ) && is_npc( victim ) )
   {
      send_to_char( "They don't need your help!\r\n", ch );
      return;
   }

   if( !ch->fighting )
   {
      send_to_char( "Too late...\r\n", ch );
      return;
   }

   if( !( fch = who_fighting( victim ) ) )
   {
      send_to_char( "They aren't fighting right now.\r\n", ch );
      return;
   }

   if( who_fighting( victim ) == ch )
   {
      send_to_char( "Just running away would be better...\r\n", ch );
      return;
   }

   if( IS_AFFECTED( victim, AFF_BERSERK ) )
   {
      send_to_char( "Stepping in front of a berserker would not be an intelligent decision.\r\n", ch );
      return;
   }

   percent = number_percent( ) - ( get_curr_lck( ch ) - 14 ) - ( get_curr_lck( victim ) - 16 );

   wait_state( ch, skill_table[gsn_rescue]->beats );
   if( !can_use_skill( ch, percent, gsn_rescue ) )
   {
      send_to_char( "You fail the rescue.\r\n", ch );
      act( AT_SKILL, "$n tries to rescue you!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "$n tries to rescue $N!", ch, NULL, victim, TO_NOTVICT );
      learn_from_failure( ch, gsn_rescue );
      return;
   }

   act( AT_SKILL, "You rescue $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n rescues you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$n moves in front of $N!", ch, NULL, victim, TO_NOTVICT );

   learn_from_success( ch, gsn_rescue );
   adjust_favor( ch, 6, 1 );
   stop_fighting( fch, false );
   stop_fighting( victim, false );
   if( ch->fighting )
      stop_fighting( ch, false );
   set_fighting( ch, fch );
   set_fighting( fch, ch );
}

CMDF( do_bash )
{
   CHAR_DATA *victim;
   int schance;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   schance = ( ( ( get_curr_dex( victim ) + get_curr_str( victim ) )
                 - ( get_curr_dex( ch ) + get_curr_str( ch ) ) ) * 10 ) + 10;
   wait_state( ch, skill_table[gsn_bash]->beats );
   if( can_use_skill( ch, ( number_percent( ) + schance ), gsn_bash ) )
   {
      learn_from_success( ch, gsn_bash );
      wait_state( victim, 2 * PULSE_VIOLENCE );
      victim->position = POS_SITTING;
      global_retcode = damage( ch, victim, NULL, number_range( 1, ch->level ), gsn_bash, true );
   }
   else
   {
      learn_from_failure( ch, gsn_bash );
      global_retcode = damage( ch, victim, NULL, 0, gsn_bash, true );
   }
}

CMDF( do_stun )
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   int schance;
   bool fail;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   if( !is_npc( ch ) && ch->move < ch->max_move / 10 )
   {
      set_char_color( AT_SKILL, ch );
      send_to_char( "You're far too tired to do that.\r\n", ch );
      return;  /* missing return fixed March 11/96 */
   }

   wait_state( ch, skill_table[gsn_stun]->beats );
   fail = false;
   schance = ris_save( victim, ch->level, RIS_PARALYSIS );
   if( schance >= 1000 )
      fail = true;
   else
      fail = saves_para_petri( schance, victim );

   schance = ( ( ( get_curr_dex( victim ) + get_curr_str( victim ) )
                 - ( get_curr_dex( ch ) + get_curr_str( ch ) ) ) * 10 ) + 10;
   if( !fail && can_use_skill( ch, ( number_percent( ) + schance ), gsn_stun ) )
   {
      learn_from_success( ch, gsn_stun );
      if( !is_npc( ch ) )
         ch->move -= ch->max_move / 10;
      wait_state( ch, 2 * PULSE_VIOLENCE );
      wait_state( victim, PULSE_VIOLENCE );
      act( AT_SKILL, "$N smashes into you, leaving you stunned!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, leaving $M stunned!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, leaving $M stunned!", ch, NULL, victim, TO_NOTVICT );
      if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
      {
         af.type = gsn_stun;
         af.location = APPLY_EXT_AFFECT;
         af.modifier = AFF_PARALYSIS;
         af.duration = 3;
         af.bitvector = meb( AFF_PARALYSIS );
         affect_to_char( victim, &af );
         update_pos( victim );
      }
   }
   else
   {
      wait_state( ch, 2 * PULSE_VIOLENCE );
      if( !is_npc( ch ) )
         ch->move -= ch->max_move / 15;
      learn_from_failure( ch, gsn_stun );
      act( AT_SKILL, "$n charges at you screaming, but you dodge out of the way.", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You try to stun $N, but $E dodges out of the way.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n charges screaming at $N, but keeps going right on past.", ch, NULL, victim, TO_NOTVICT );
   }
}

CMDF( do_bloodlet )
{
   OBJ_DATA *obj;

   if( is_npc( ch ) || !is_vampire( ch ) )
      return;

   if( ch->fighting )
   {
      send_to_char( "You're too busy fighting...\r\n", ch );
      return;
   }

   if( ch->mana < 10 )
   {
      send_to_char( "You're too drained to offer any blood...\r\n", ch );
      return;
   }

   wait_state( ch, PULSE_VIOLENCE );
   if( !can_use_skill( ch, number_percent( ), gsn_bloodlet ) )
   {
      act( AT_BLOOD, "You can't manage to draw much blood...", ch, NULL, NULL, TO_CHAR );
      act( AT_BLOOD, "$n slices open $s skin, but no blood is spilled...", ch, NULL, NULL, TO_ROOM );
      learn_from_failure( ch, gsn_bloodlet );
      return;
   }

   ch->mana -= 7;
   act( AT_BLOOD, "Tracing a sharp nail over your skin, you let your blood spill.", ch, NULL, NULL, TO_CHAR );
   act( AT_BLOOD, "$n traces a sharp nail over $s skin, spilling a quantity of blood to the ground.", ch, NULL, NULL, TO_ROOM );
   learn_from_success( ch, gsn_bloodlet );
   if( !( obj = create_object( get_obj_index( OBJ_VNUM_BLOODLET ), 0 ) ) )
   {
      send_to_char( "Failed to create blood.\r\n", ch );
      bug( "%s: object is NULL after create_object.\r\n", __FUNCTION__ );
      return;
   }
   obj->timer = 1;
   obj->value[1] = 6;
   obj_to_room( obj, ch->in_room );
}

CMDF( do_feed )
{
   CHAR_DATA *victim;
   short dam;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !is_npc( ch ) && !is_vampire( ch ) )
   {
      send_to_char( "It is not of your nature to feed on living creatures.\r\n", ch );
      return;
   }
   if( !can_use_skill( ch, 0, gsn_feed ) )
   {
      send_to_char( "You haven't yet practiced your new teeth.\r\n", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_feed]->beats );
   if( !can_use_skill( ch, number_percent( ), gsn_feed ) )
   {
      global_retcode = damage( ch, victim, NULL, 0, gsn_feed, false );
      if( global_retcode == rNONE && !is_npc( ch )
      && ch->fighting && ch->mana < ch->max_mana )
      {
         act( AT_BLOOD, "The smell of $N's blood is driving you insane!", ch, NULL, victim, TO_CHAR );
         act( AT_BLOOD, "$n is lusting after your blood!", ch, NULL, victim, TO_VICT );
         learn_from_failure( ch, gsn_feed );
      }
      return;
   }

   dam = number_range( 1, ch->level );
   global_retcode = damage( ch, victim, NULL, dam, gsn_feed, false );
   if( global_retcode == rNONE && !is_npc( ch ) && dam && ch->fighting && ch->mana < ch->max_mana )
   {
      ch->mana = URANGE( 0, ch->mana + ( UMIN( number_range( 1, ( ch->level + victim->level / 20 ) + 3 ), ch->max_mana - ch->mana ) ), ch->max_mana );
      if( ch->pcdata->condition[COND_FULL] <= 37 )
         gain_condition( ch, COND_FULL, 2 );
      gain_condition( ch, COND_THIRST, 2 );
      act( AT_BLOOD, "You manage to suck a little life out of $N.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n sucks some of your blood!", ch, NULL, victim, TO_VICT );
      learn_from_success( ch, gsn_feed );
   }
}

/*
 * Disarm someone.
 * Caller must check for successful attack.
 * Check for loyalty flag (weapon disarms to inventory)
 */
void disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
   OBJ_DATA *obj;

   if( !( obj = get_eq_hold( victim, ITEM_WEAPON ) ) )
      return;

   if( !get_eq_hold( ch, ITEM_WEAPON ) && number_bits( 1 ) == 0 )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   if( is_npc( ch ) && !can_see_obj( ch, obj ) && number_bits( 1 ) == 0 )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   if( check_grip( ch, victim ) )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   act( AT_SKILL, "$n DISARMS you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "You disarm $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n disarms $N!", ch, NULL, victim, TO_NOTVICT );
   learn_from_success( ch, gsn_disarm );

   obj_from_char( obj );
   if( is_npc( victim ) || is_obj_stat( obj, ITEM_LOYAL ) )
      obj_to_char( obj, victim );
   else
      obj_to_room( obj, victim->in_room );
}

CMDF( do_disarm )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int percent;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   if( !get_eq_hold( ch, ITEM_WEAPON ) )
   {
      send_to_char( "You must wield a weapon to disarm.\r\n", ch );
      return;
   }

   if( !( obj = get_eq_hold( victim, ITEM_WEAPON ) ) )
   {
      send_to_char( "Your opponent is not wielding a weapon.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_disarm]->beats );
   percent = number_percent( ) + victim->level - ch->level - ( get_curr_lck( ch ) - 15 ) + ( get_curr_lck( victim ) - 15 );
   if( !can_see_obj( ch, obj ) )
      percent += 10;
   if( can_use_skill( ch, ( percent * 3 / 2 ), gsn_disarm ) )
      disarm( ch, victim );
   else
   {
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_disarm );
   }
}

/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( IS_AFFECTED( victim, AFF_FLYING ) || IS_AFFECTED( victim, AFF_FLOATING ) )
      return;

   if( victim->mount )
   {
      if( IS_AFFECTED( victim->mount, AFF_FLYING ) || IS_AFFECTED( victim->mount, AFF_FLOATING ) )
         return;
      act( AT_SKILL, "$n trips your mount and you fall off!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N's mount and $N falls off!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N's mount and $N falls off!", ch, NULL, victim, TO_NOTVICT );
      xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
      victim->mount = NULL;
      wait_state( ch, 2 * PULSE_VIOLENCE );
      wait_state( victim, 2 * PULSE_VIOLENCE );
      victim->position = POS_RESTING;
      return;
   }

   if( victim->wait == 0 )
   {
      act( AT_SKILL, "$n trips you and you go down!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N and $N goes down!", ch, NULL, victim, TO_NOTVICT );

      wait_state( ch, 2 * PULSE_VIOLENCE );
      wait_state( victim, 2 * PULSE_VIOLENCE );
      victim->position = POS_RESTING;
   }
}

CMDF( do_pick )
{
   CHAR_DATA *gch;
   OBJ_DATA *obj = NULL, *lockpick;
   EXIT_DATA *pexit;
   char arg[MIL];

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Pick what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_pick_lock]->beats );

   /* Look for lockpicking tools */
   if( !( lockpick = get_eq_hold( ch, ITEM_LOCKPICK ) ) )
   {
      send_to_char( "You must be holding a lockpicking tool.\r\n", ch );
      return;
   }

   /* look for guards */
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( is_npc( gch ) && is_awake( gch ) && ( ch->level + 5 ) < gch->level )
      {
         act( AT_PLAIN, "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
         return;
      }
   }

   if( !can_use_skill( ch, number_percent( ), gsn_pick_lock ) )
   {
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_pick_lock );
      return;
   }

   if( ( pexit = find_door( ch, arg, true ) ) )
   {
      EXIT_DATA *pexit_rev;

      if( !xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\r\n", ch );
         return;
      }

      if( pexit->key < 0 )
      {
         send_to_char( "It can't be picked.\r\n", ch );
         return;
      }

      if( !xIS_SET( pexit->exit_info, EX_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\r\n", ch );
         return;
      }

      if( xIS_SET( pexit->exit_info, EX_PICKPROOF ) )
      {
         send_to_char( "You failed.\r\n", ch );
         learn_from_failure( ch, gsn_pick_lock );
         check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
         return;
      }

      xREMOVE_BIT( pexit->exit_info, EX_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      act( AT_ACTION, "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

      if( --lockpick->value[0] <= 0 )
      {
         if( is_obj_stat( lockpick, ITEM_NOSCRAP ) )
            lockpick->value[0] = 1;
         else
         {
            act( AT_ACTION, "Your $p breaks as you pull it out of the lock.", ch, lockpick, NULL, TO_ROOM );
            extract_obj( lockpick );
         }
      }

      learn_from_success( ch, gsn_pick_lock );
      adjust_favor( ch, 7, 1 );
      /* pick the other side */
      if( ( pexit_rev = pexit->rexit ) && pexit_rev->to_room == ch->in_room )
         xREMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
      check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
      return;
   }

   if( ( obj = get_obj_here( ch, arg ) ) )
   {
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

      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\r\n", ch );
         return;
      }

      if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
      {
         send_to_char( "You failed.\r\n", ch );
         learn_from_failure( ch, gsn_pick_lock );
         check_for_trap( ch, obj, TRAP_PICK );
         return;
      }

      separate_obj( obj );
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      act( AT_ACTION, "$n picks $p.", ch, obj, NULL, TO_ROOM );

      if( --lockpick->value[0] <= 0 )
      {
         if( is_obj_stat( lockpick, ITEM_NOSCRAP ) )
            lockpick->value[0] = 1;
         else
         {
            act( AT_ACTION, "Your $p breaks as you pull it out of the lock.", ch, lockpick, NULL, TO_ROOM );
            extract_obj( lockpick );
         }
      }

      learn_from_success( ch, gsn_pick_lock );
      adjust_favor( ch, 7, 1 );
      check_for_trap( ch, obj, TRAP_PICK );
      return;
   }

   ch_printf( ch, "You see no %s here.\r\n", arg );
}

/* Contributed by Alander. */
CMDF( do_visible )
{
   affect_strip( ch, gsn_invis );
   affect_strip( ch, gsn_sneak );
   affect_strip( ch, gsn_hide );
   xREMOVE_BIT( ch->affected_by, AFF_HIDE );
   xREMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
   xREMOVE_BIT( ch->affected_by, AFF_SNEAK );
   send_to_char( "Ok.\r\n", ch );
}

bool handle_recall( CHAR_DATA *ch )
{
   ROOM_INDEX_DATA *location = NULL;
   CHAR_DATA *opponent;

   if( !is_npc( ch ) && ch->pcdata->clan )
      location = get_room_index( ch->pcdata->clan->recall );

   if( !location && !is_npc( ch ) && ch->pcdata->nation )
      location = get_room_index( ch->pcdata->nation->recall );

   if( !is_npc( ch ) && !location && ch->level >= 5 && xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
      location = get_room_index( sysdata.room_deadly );

   if( !location )
      location = get_room_index( race_table[ch->race]->race_recall );

   if( !location )
      location = get_room_index( sysdata.room_temple );

   if( !location )
   {
      send_to_char( "You're completely lost.\r\n", ch );
      return false;
   }

   if( ch->in_room == location )
      return false;

   if( xIS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) )
   {
      send_to_char( "For some strange reason... nothing happens.\r\n", ch );
      return false;
   }

   if( IS_AFFECTED( ch, AFF_CURSE ) )
   {
      send_to_char( "You're cursed and can't recall!\r\n", ch );
      return false;
   }

   if( ( opponent = who_fighting( ch ) ) )
   {
      int lose;

      if( number_bits( 1 ) == 0 || ( !is_npc( opponent ) && number_bits( 3 ) > 1 ) )
      {
         wait_state( ch, 4 );
         lose = ( int )( exp_level( ch, ch->level ) / 10 );
         gain_exp( ch, 0 - lose );
         ch_printf( ch, "You failed!  You lose %d exps.\r\n", lose );
         return false;
      }

      lose = ( int )( exp_level( ch, ch->level ) / 8 );
      gain_exp( ch, 0 - lose );
      ch_printf( ch, "You recall from combat!  You lose %d exps.\r\n", lose );
      stop_fighting( ch, true );
   }

   act( AT_ACTION, "$n disappears in a swirl of smoke.", ch, NULL, NULL, TO_ROOM );
   char_from_room( ch );
   char_to_room( ch, location );
   if( ch->mount )
   {
      char_from_room( ch->mount );
      char_to_room( ch->mount, location );
   }
   act( AT_ACTION, "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
   do_look( ch, (char *)"auto" );
   return true;
}

CMDF( do_recall )
{
   handle_recall( ch );
}

CMDF( do_aid )
{
   CHAR_DATA *victim;
   char arg[MIL];
   int percent;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Aid whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )  /* Gorog */
   {
      send_to_char( "Not on mobs.\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Aid yourself?\r\n", ch );
      return;
   }

   if( victim->position > POS_STUNNED )
   {
      act( AT_PLAIN, "$N doesn't need your help.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->hit <= -6 )
   {
      act( AT_PLAIN, "$N's condition is beyond your aiding ability.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent( ) - ( get_curr_lck( ch ) - 13 );
   wait_state( ch, skill_table[gsn_aid]->beats );
   if( !can_use_skill( ch, percent, gsn_aid ) )
   {
      send_to_char( "You fail.\r\n", ch );
      learn_from_failure( ch, gsn_aid );
      return;
   }

   act( AT_SKILL, "You aid $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n aids $N!", ch, NULL, victim, TO_NOTVICT );
   learn_from_success( ch, gsn_aid );
   adjust_favor( ch, 6, 1 );
   if( victim->hit < 1 )
      victim->hit = 1;

   update_pos( victim );
   act( AT_SKILL, "$n aids you!", ch, NULL, victim, TO_VICT );
}

CMDF( do_mount )
{
   CHAR_DATA *victim;

   if( ch->mount )
   {
      send_to_char( "You're already mounted!\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }

   if( !is_npc( victim ) || !xIS_SET( victim->act, ACT_MOUNTABLE ) )
   {
      send_to_char( "You can't mount that!\r\n", ch );
      return;
   }

   if( xIS_SET( victim->act, ACT_MOUNTED ) || victim->mounter )
   {
      send_to_char( "That mount already has a rider.\r\n", ch );
      return;
   }

   if( victim->position < POS_STANDING )
   {
      send_to_char( "Your mount must be standing.\r\n", ch );
      return;
   }

   if( victim->position == POS_FIGHTING || victim->fighting )
   {
      send_to_char( "Your mount is moving around too much.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_mount]->beats );
   if( can_use_skill( ch, number_percent( ), gsn_mount ) )
   {
      xSET_BIT( victim->act, ACT_MOUNTED );
      ch->mount = victim;
      victim->mounter = ch;
      act( AT_SKILL, "You mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully mounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n mounts you.", ch, NULL, victim, TO_VICT );
      learn_from_success( ch, gsn_mount );
      ch->position = POS_MOUNTED;
   }
   else
   {
      act( AT_SKILL, "You unsuccessfully try to mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n unsuccessfully attempts to mount $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n tries to mount you.", ch, NULL, victim, TO_VICT );
      learn_from_failure( ch, gsn_mount );
   }
}

CMDF( do_dismount )
{
   CHAR_DATA *victim;

   if( !( victim = ch->mount ) )
   {
      send_to_char( "You're not mounted.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_mount]->beats );
   xREMOVE_BIT( victim->act, ACT_MOUNTED );
   victim->mounter = NULL;
   ch->mount = NULL;
   if( can_use_skill( ch, number_percent( ), gsn_mount ) )
   {
      act( AT_SKILL, "You dismount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully dismounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n dismounts you.  Whew!", ch, NULL, victim, TO_VICT );
      ch->position = POS_STANDING;
      learn_from_success( ch, gsn_mount );
   }
   else
   {
      act( AT_SKILL, "You fall off while dismounting $N.  Ouch!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n falls off of $N while dismounting.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n falls off your back.", ch, NULL, victim, TO_VICT );
      learn_from_failure( ch, gsn_mount );
      ch->position = POS_SITTING;
      global_retcode = damage( ch, ch, NULL, 1, TYPE_UNDEFINED, true );
   }
}

/* Check for parry. */
bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chances;

   if( !is_awake( victim ) )
      return false;

   if( is_npc( victim ) && !xIS_SET( victim->defenses, DFND_PARRY ) )
      return false;

   if( is_npc( victim ) )
   {
      chances = UMIN( 60, 2 * victim->level );
   }
   else
   {
      /* Don't need a weapon unless the one attacking is using one */
      if( get_eq_hold( ch, ITEM_WEAPON ) && !get_eq_hold( victim, ITEM_WEAPON ) )
         return false;
      chances = ( int )( LEARNED( victim, gsn_parry ) );
   }

   if( chances != 0 && victim->morph )
      chances += victim->morph->parry;

   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_parry );
      return false;
   }

   if( !is_npc( victim ) && !xIS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
       act( AT_SKILL, "You parry $n's attack.", ch, NULL, victim, TO_VICT );
   if( !is_npc( ch ) && !xIS_SET( ch->pcdata->flags, PCFLAG_GAG ) ) 
      act( AT_SKILL, "$N parries your attack.", ch, NULL, victim, TO_CHAR );

   learn_from_success( victim, gsn_parry );
   return true;
}

/* Check for dodge. */
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chances;

   if( !is_awake( victim ) )
      return false;

   if( is_npc( victim ) && !xIS_SET( victim->defenses, DFND_DODGE ) )
      return false;

   if( is_npc( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( LEARNED( victim, gsn_dodge ) );

   if( chances != 0 && victim->morph )
      chances += victim->morph->dodge;

   /* Consider luck as a factor */
   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_dodge );
      return false;
   }

   if( !is_npc( victim ) && !xIS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "You dodge $n's attack.", ch, NULL, victim, TO_VICT );

   if( !is_npc( ch ) && !xIS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "$N dodges your attack.", ch, NULL, victim, TO_CHAR );

   learn_from_success( victim, gsn_dodge );
   return true;
}

bool check_duck( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chances;

   if( !is_awake( victim ) )
      return false;
   if( !is_npc( victim ) && !victim->pcdata->learned[gsn_duck] > 0 )
      return false;
   if( is_npc( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( LEARNED( victim, gsn_duck ) + ( get_curr_dex( victim ) - 13 ) );
   if( !chance( victim, chances + victim->level - ch->level ) )
      return false;
   if( !is_npc( victim ) && !xIS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "You duck $n's attack.", ch, NULL, victim, TO_VICT );
   if( !is_npc( ch ) && !xIS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "$N ducks your attack.", ch, NULL, victim, TO_CHAR );
   learn_from_success( victim, gsn_duck );
   return true;
}

bool check_block( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chances;

   if( !is_awake( victim ) )
      return false;
   if( !is_npc( victim ) && !victim->pcdata->learned[gsn_block] > 0 )
      return false;
   if( is_npc( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
   {
      /* Don't need a weapon / shield unless the one attacking is using one */
      if( get_eq_hold( ch, ITEM_WEAPON ) && !get_eq_hold( victim, ITEM_WEAPON ) && !get_eq_hold( victim, ITEM_ARMOR ) )
         return false;
      chances = ( int )( LEARNED( victim, gsn_block ) + ( get_curr_dex( victim ) - 13 ) );
   }
   if( !chance( victim, chances + victim->level - ch->level ) )
      return false;
   if( !is_npc( victim ) && !xIS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "You block $n's attack.", ch, NULL, victim, TO_VICT );
   if( !is_npc( ch ) && !xIS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "$N blocks your attack.", ch, NULL, victim, TO_CHAR );
   learn_from_success( victim, gsn_block );
   return true;
}

bool check_tumble( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chances;

   if( !is_awake( victim ) )
      return false;
   if( !is_npc( victim ) && !victim->pcdata->learned[gsn_tumble] > 0 )
      return false;
   if( is_npc( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( LEARNED( victim, gsn_tumble ) + ( get_curr_dex( victim ) - 13 ) );
   if( chances != 0 && victim->morph )
      chances += victim->morph->tumble;
   if( !chance( victim, chances + victim->level - ch->level ) )
      return false;
   if( !is_npc( victim ) && !xIS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "You tumble away from $n's attack.", ch, NULL, victim, TO_VICT );
   if( !is_npc( ch ) && !xIS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "$N tumbles away from your attack.", ch, NULL, victim, TO_CHAR );
   learn_from_success( victim, gsn_tumble );
   return true;
}

CMDF( do_poison_weapon )
{
   OBJ_DATA *obj, *cobj, *pobj = NULL, *wobj = NULL;
   int percent;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What are you trying to poison?\r\n", ch );
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
      send_to_char( "You don't have that weapon.\r\n", ch );
      return;
   }
   if( obj->item_type != ITEM_WEAPON )
   {
      send_to_char( "That item is not a weapon.\r\n", ch );
      return;
   }
   if( is_obj_stat( obj, ITEM_POISONED ) )
   {
      send_to_char( "That weapon is already poisoned.\r\n", ch );
      return;
   }
   if( is_obj_stat( obj, ITEM_CLANOBJECT ) )
   {
      send_to_char( "It doesn't appear to be fashioned of a poisonable material.\r\n", ch );
      return;
   }
   if( !is_npc( ch ) && get_curr_wis( ch ) < 16 )
   {
      send_to_char( "You can't quite remember what to do...\r\n", ch );
      return;
   }
   if( !is_npc( ch ) && ( ( get_curr_dex( ch ) < 17 ) || ch->pcdata->condition[COND_DRUNK] > 0 ) )
   {
      send_to_char( "Your hands aren't steady enough to properly mix the poison.\r\n", ch );
      return;
   }

   /* Now we have a valid weapon...check to see if we have the powder. */
   for( cobj = ch->first_carrying; cobj; cobj = cobj->next_content )
   {
      if( !pobj && cobj->pIndexData->vnum == OBJ_VNUM_BLACK_POWDER )
         pobj = cobj;
      if( !wobj && cobj->item_type == ITEM_DRINK_CON && cobj->value[1] > 0 && cobj->value[2] == LIQ_WATER )
         wobj = cobj;
      if( pobj && wobj )
         break;
   }
   if( !pobj )
   {
      send_to_char( "You don't have the black poison powder.\r\n", ch );
      return;
   }
   if( !wobj )
   {
      send_to_char( "You have no water to mix with the powder.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_poison_weapon]->beats );

   percent = ( number_percent( ) - get_curr_lck( ch ) - 14 );

   separate_obj( pobj );
   separate_obj( wobj );
   if( !can_use_skill( ch, percent, gsn_poison_weapon ) )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "You failed and spill some on yourself.  Ouch!\r\n", ch );
      set_char_color( AT_GRAY, ch );
      act( AT_RED, "$n spills the poison all over!", ch, NULL, NULL, TO_ROOM );
      extract_obj( pobj );
      extract_obj( wobj );
      learn_from_failure( ch, gsn_poison_weapon );
      damage( ch, ch, NULL, number_range( 10, ch->level ), gsn_poison_weapon, true );
      return;
   }
   separate_obj( obj );
   act( AT_RED, "You mix $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_CHAR );
   act( AT_RED, "$n mixes $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_ROOM );
   act( AT_GREEN, "You pour the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_CHAR );
   act( AT_GREEN, "$n pours the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_ROOM );
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_CHAR );
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_ROOM );
   extract_obj( pobj );
   extract_obj( wobj );
   xSET_BIT( obj->extra_flags, ITEM_POISONED );
   obj->cost *= 2;
   obj->timer = UMIN( obj->level, ch->level );

   if( is_obj_stat( obj, ITEM_BLESS ) )
      obj->timer *= 2;

   if( is_obj_stat( obj, ITEM_MAGIC ) )
      obj->timer *= 2;

   learn_from_success( ch, gsn_poison_weapon );
}

CMDF( do_scribe )
{
   OBJ_DATA *scroll;
   char buf1[MSL], buf2[MSL], buf3[MSL];
   int sn, mana;

   if( is_npc( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Scribe what?\r\n", ch );
      return;
   }

   if( ms_find_obj(ch) )
      return;

   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      send_to_char( "You haven't learned that spell.\r\n", ch );
      return;
   }

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\r\n", ch );
      return;
   }

   if( !SPELL_FLAG( skill_table[sn], SF_CANSCRIBE ) && !SPELL_FLAG( skill_table[sn], SF_CANDOALL ) )
   {
      send_to_char( "You can't scribe that spell.\r\n", ch );
      return;
   }

   mana = is_npc( ch ) ? 0 : UMAX( skill_table[sn]->min_mana, mana_cost( ch, sn ) );
   mana *= 5;

   if( !is_npc(ch) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   if( !( scroll = get_eq_hold( ch, ITEM_SCROLL ) ) )
   {
      send_to_char( "You must be holding a scroll to scribe it.\r\n", ch );
      return;
   }

   if( scroll->value[5] != 1 )
   {
      send_to_char( "That scroll can't be scribed.\r\n", ch );
      return;
   }

   if( ( scroll->value[1] != -1 ) && ( scroll->value[2] != -1 ) && ( scroll->value[3] != -1 ) )
   {
      send_to_char( "That scroll has already been fully scribed.\r\n", ch);
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_scribe );
      ch->mana -= (mana / 2);
      return;
   }

   if( !can_use_skill( ch, number_percent( ), gsn_scribe ) )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_scribe );
      ch->mana -= (mana / 2);
      return;
   }

   if( scroll->value[1] == -1 )
   {
      scroll->value[1] = sn;
   }
   else if( scroll->value[2] == -1 )
   {
      if( number_percent( ) > 50 )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "The magic surges out of control and destroys the scroll!.\r\n", ch );
         learn_from_failure( ch, gsn_scribe );
         ch->mana -= ( mana / 2 );
         extract_obj( scroll );
         return;
      }
      scroll->value[2] = sn;
   }
   else if( scroll->value[3] == -1 )
   {
      if( number_percent( ) > 30 )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "The magic surges outof control and destroys the scroll!.\r\n", ch );
         learn_from_failure( ch, gsn_scribe );
         ch->mana -= ( mana / 2 );
         extract_obj( scroll );
         return;
      }
      scroll->value[3] = sn;
   }

   if( scroll->value[1] != -1 && scroll->value[2] != -1 && scroll->value[3] != -1 )
   {
      sprintf( buf1, "%s, %s, %s scroll",
         skill_table[scroll->value[1]]->name,
         skill_table[scroll->value[2]]->name,
         skill_table[scroll->value[3]]->name );
      sprintf( buf2, "A scroll inscribed with '%s' '%s' '%s' lies in the dust.",
         skill_table[scroll->value[1]]->name,
         skill_table[scroll->value[2]]->name,
         skill_table[scroll->value[3]]->name );
      sprintf( buf3, "scroll scribing %s %s %s",
         skill_table[scroll->value[1]]->name, 
         skill_table[scroll->value[2]]->name,
         skill_table[scroll->value[3]]->name );
   }
   else if( scroll->value[1] != -1 && scroll->value[2] != -1 )
   {
      sprintf( buf1, "%s, %s scroll",
         skill_table[scroll->value[1]]->name,
         skill_table[scroll->value[2]]->name );
      sprintf( buf2, "A scroll inscribed with '%s' '%s' lies in the dust.",
         skill_table[scroll->value[1]]->name,
         skill_table[scroll->value[2]]->name );
      sprintf( buf3, "scroll scribing %s %s",
         skill_table[scroll->value[1]]->name, 
         skill_table[scroll->value[2]]->name );
   }
   else if( scroll->value[1] != -1 )
   {
      sprintf( buf1, "%s scroll",
         skill_table[scroll->value[1]]->name );
      sprintf( buf2, "A scroll inscribed with '%s' lies in the dust.",
         skill_table[scroll->value[1]]->name );
      sprintf( buf3, "scroll scribing %s",
         skill_table[scroll->value[1]]->name );
   }

   if( scroll->value[0] == 0 || scroll->value[0] > ch->level )
      scroll->value[0] = ch->level;

   STRSET( scroll->short_descr, aoran( buf1 ) );
   STRSET( scroll->description, buf2 );
   STRSET( scroll->name, buf3 );

   act( AT_MAGIC, "$n magically scribes $p.", ch, scroll, NULL, TO_ROOM );
   act( AT_MAGIC, "You magically scribe $p.", ch, scroll, NULL, TO_CHAR );

   learn_from_success( ch, gsn_scribe );
   ch->mana -= mana;
}

CMDF( do_brew )
{
   OBJ_DATA *potion, *fire;
   char buf1[MSL], buf2[MSL], buf3[MSL];
   int sn, mana;
   bool found;

   if( is_npc( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Brew what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      send_to_char( "You haven't learned that spell.\r\n", ch );
      return;
   }

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\r\n", ch );
      return;
   }

   if( !SPELL_FLAG( skill_table[sn], SF_CANBREW ) && !SPELL_FLAG( skill_table[sn], SF_CANDOALL ) )
   {
      send_to_char( "You can't brew that spell.\r\n", ch );
      return;
   }

   mana = is_npc( ch ) ? 0 : UMAX( skill_table[sn]->min_mana, mana_cost( ch, sn ) );
   mana *= 4;

   if( !is_npc( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   found = false;

   for( fire = ch->in_room->first_content; fire; fire = fire->next_content )
   {
      if( fire->item_type == ITEM_FIRE )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "There must be a fire in the room to brew a potion.\r\n", ch );
      return;
   }

   if( !( potion = get_eq_hold( ch, ITEM_POTION ) ) )
   {
      send_to_char( "You must be holding a flask to brew a potion.\r\n", ch );
      return;
   }

   if( potion->value[5] != 1 )
   {
      send_to_char( "You can't brew anything on that flask.\r\n", ch );
      return;
   }

   if( ( potion->value[1] != -1 ) && ( potion->value[2] != -1 ) && ( potion->value[3] != -1 ) )
   {
      send_to_char( "That flask has already been fully brewed.\r\n", ch);
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_brew );
      ch->mana -= (mana / 2);
      return;
   }

   if( !can_use_skill( ch, number_percent( ), gsn_brew ) )
   {
      set_char_color ( AT_MAGIC, ch );
      send_to_char("You failed.\r\n", ch);
      learn_from_failure( ch, gsn_brew );
      ch->mana -= (mana / 2);
      return;
   }

   if( potion->value[1] == -1 )
   {
      potion->value[1] = sn;
   }
   else if( potion->value[2] == -1 )
   {
      if( number_percent( ) > 50 )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "The magic surges out of control and destroys the potion!.\r\n", ch );
         learn_from_failure( ch, gsn_brew );
         ch->mana -= ( mana / 2 );
         extract_obj( potion );
         return;
      }
      potion->value[2] = sn;
   }
   else if( potion->value[3] == -1 )
   {
      if( number_percent( ) > 30 )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "The magic surges out of control and destroys the potion!.\r\n", ch );
         learn_from_failure( ch, gsn_brew );
         ch->mana -= ( mana / 2 );
         extract_obj( potion );
         return;
      }
      potion->value[3] = sn;
   }

   if( potion->value[1] != -1 && potion->value[2] != -1 && potion->value[3] != -1 )
   {
      sprintf( buf1, "%s, %s, %s potion",
         skill_table[potion->value[1]]->name,
         skill_table[potion->value[2]]->name,
         skill_table[potion->value[3]]->name );
      sprintf( buf2, "A strange potion labelled '%s' '%s' '%s' sizzles in a glass flask.",
         skill_table[potion->value[1]]->name,
         skill_table[potion->value[2]]->name,
         skill_table[potion->value[3]]->name );
      sprintf( buf3, "flask potion %s %s %s",
         skill_table[potion->value[1]]->name, 
         skill_table[potion->value[2]]->name,
         skill_table[potion->value[3]]->name );
   }
   else if( potion->value[1] != -1 && potion->value[2] != -1 )
   {
      sprintf( buf1, "%s, %s potion",
         skill_table[potion->value[1]]->name,
         skill_table[potion->value[2]]->name );
      sprintf( buf2, "A strange potion labelled '%s' '%s' sizzles in a glass flask.",
         skill_table[potion->value[1]]->name,
         skill_table[potion->value[2]]->name );
      sprintf( buf3, "flask potion %s %s",
         skill_table[potion->value[1]]->name, 
         skill_table[potion->value[2]]->name );
   }
   else if( potion->value[1] != -1 )
   {
      sprintf( buf1, "%s potion",
         skill_table[potion->value[1]]->name );
      sprintf( buf2, "A strange potion labelled '%s' sizzles in a glass flask.",
         skill_table[potion->value[1]]->name );
      sprintf( buf3, "flask potion %s",
         skill_table[potion->value[1]]->name );
   }

   if( potion->value[0] == 0 || potion->value[0] > ch->level )
      potion->value[0] = ch->level;

   STRSET( potion->short_descr, aoran( buf1 ) );
   STRSET( potion->description, buf2 );
   STRSET( potion->name, buf3 );

   act( AT_MAGIC, "$n magically brews $p.", ch, potion, NULL, TO_ROOM );
   act( AT_MAGIC, "You magically brew $p.", ch, potion, NULL, TO_CHAR );

   learn_from_success( ch, gsn_brew );
   ch->mana -= mana;
}

bool check_grip( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int schance;

   if( !is_awake( victim ) )
      return false;

   if( is_npc( victim ) && !xIS_SET( victim->defenses, DFND_GRIP ) )
      return false;

   if( is_npc( victim ) )
      schance = UMIN( 60, 2 * victim->level );
   else
      schance = ( int )( LEARNED( victim, gsn_grip ) / 2 );

   /* Consider luck as a factor */
   schance += ( 2 * ( get_curr_lck( victim ) - 13 ) );

   if( number_percent( ) >= schance + victim->level - ch->level )
   {
      learn_from_failure( victim, gsn_grip );
      return false;
   }
   act( AT_SKILL, "You evade $n's attempt to disarm you.", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$N holds $S weapon strongly, and is not disarmed.", ch, NULL, victim, TO_CHAR );
   learn_from_success( victim, gsn_grip );
   return true;
}

CMDF( do_circle )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   char arg[MIL];
   int percent;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   one_argument( argument, arg );

   if( ch->mount )
   {
      send_to_char( "You can't circle while mounted.\r\n", ch );
      return;
   }

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Circle around whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\r\n", ch );
      return;
   }

   if( is_safe( ch, victim, true ) )
      return;

   if( !( obj = get_eq_hold( ch, ITEM_WEAPON ) )
   || ( obj->value[3] != DAM_PIERCE && obj->value[3] != DAM_STAB ) )
   {
      send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
      return;
   }

   if( !ch->fighting )
   {
      send_to_char( "You can't circle around someone when you aren't fighting.\r\n", ch );
      return;
   }

   if( !victim->fighting )
   {
      send_to_char( "You can't circle around a person who is not fighting.\r\n", ch );
      return;
   }

   if( victim->num_fighting < 2 )
   {
      act( AT_PLAIN, "You can't circle around them without a distraction.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent( ) - ( get_curr_lck( ch ) - 16 ) + ( get_curr_lck( victim ) - 13 );

   wait_state( ch, skill_table[gsn_circle]->beats );
   if( can_use_skill( ch, percent, gsn_circle ) )
   {
      learn_from_success( ch, gsn_circle );
      wait_state( ch, 2 * PULSE_VIOLENCE );
      global_retcode = multi_hit( ch, victim, gsn_circle );
      adjust_favor( ch, 8, 1 );
   }
   else
   {
      learn_from_failure( ch, gsn_circle );
      wait_state( ch, 2 * PULSE_VIOLENCE );
      global_retcode = damage( ch, victim, NULL, 0, gsn_circle, true );
   }
}

/* Berserk and HitAll. -- Altrag */
CMDF( do_berserk )
{
   AFFECT_DATA af;
   short percent;

   if( !ch->fighting )
   {
      send_to_char( "But you aren't fighting!\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "Your rage is already at its peak!\r\n", ch );
      return;
   }

   percent = LEARNED( ch, gsn_berserk );
   wait_state( ch, skill_table[gsn_berserk]->beats );
   if( !chance( ch, percent ) )
   {
      send_to_char( "You couldn't build up enough rage.\r\n", ch );
      learn_from_failure( ch, gsn_berserk );
      return;
   }
   af.type = gsn_berserk;
   /*
    * Hmmm.. 10-20 combat rounds at level 50.. good enough for most mobs,
    * and if not they can always go berserk again.. shrug.. maybe even
    * too high. -- Altrag 
    */
   af.duration = number_range( ch->level / 5, ch->level * 2 / 5 );
   /* Hmm.. you get stronger when yer really enraged.. mind over matter type thing.. */
   af.location = APPLY_EXT_AFFECT;
   af.modifier = AFF_BERSERK;
   af.bitvector = meb( AFF_BERSERK );
   affect_to_char( ch, &af );
   send_to_char( "You start to lose control..\r\n", ch );
   learn_from_success( ch, gsn_berserk );
}

CMDF( do_scan )
{
   ROOM_INDEX_DATA *was_in_room;
   EXIT_DATA *pexit;
   short dir = -1, dist, max_dist = 8;
   bool found = false;

   set_char_color( AT_ACTION, ch );

   if( IS_AFFECTED( ch, AFF_BLIND ) )
   {
      send_to_char( "Not very effective when you're blind...\r\n", ch );
      return;
   }

   if( is_vampire( ch ) )
   {
      if( time_info.hour < 21 && time_info.hour > 5 )
      {
         send_to_char( "You have trouble seeing clearly through all the light.\r\n", ch );
         max_dist = 1;
      }
   }

   if( !argument || argument[0] == '\0' )
   {
      act( AT_GRAY, "Scanning in all directions...", ch, NULL, NULL, TO_CHAR );
      act( AT_GRAY, "$n scans in all directions.", ch, NULL, NULL, TO_ROOM );
      was_in_room = ch->in_room;
      for( pexit = was_in_room->first_exit; pexit; pexit = pexit->next )
      {
         if( !pexit->to_room )
            continue;
         if( xIS_SET( pexit->exit_info, EX_HIDDEN ) )
            continue;
         if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
            continue;
         if( room_is_private( pexit->to_room ) && get_trust( ch ) < PERM_LEADER )
            continue;
         act( AT_GRAY, "Scanning $t...", ch, (char *)dir_name[pexit->vdir], NULL, TO_CHAR );
         found = true;
         char_from_room( ch );
         char_to_room( ch, pexit->to_room );
         set_char_color( AT_RMNAME, ch );
         send_to_char( ch->in_room->name, ch );
         send_to_char( "\r\n", ch );
         show_list_to_char( ch->in_room->first_content, ch, false, false );
         show_char_to_char( ch->in_room->first_person, ch );
         char_from_room( ch );
         char_to_room( ch, was_in_room );
      }
      if( found )
         learn_from_success( ch, gsn_scan );
      else
         send_to_char( "Nothing to show...\r\n", ch );
      return;
   }

   if( ( dir = get_door( argument ) ) == -1 )
   {
      send_to_char( "Scan in WHAT direction?\r\n", ch );
      return;
   }

   was_in_room = ch->in_room;
   act( AT_GRAY, "Scanning $t...", ch, (char *)dir_name[dir], NULL, TO_CHAR );
   act( AT_GRAY, "$n scans $t.", ch, (char *)dir_name[dir], NULL, TO_ROOM );

   if( !can_use_skill( ch, number_percent( ), gsn_scan ) )
   {
      act( AT_GRAY, "You stop scanning $t as your vision blurs.", ch, (char *)dir_name[dir], NULL, TO_CHAR );
      learn_from_failure( ch, gsn_scan );
      return;
   }

   if( !( pexit = get_exit( ch->in_room, dir ) ) )
   {
      act( AT_GRAY, "You can't see $t.", ch, (char *)dir_name[dir], NULL, TO_CHAR );
      return;
   }

   if( ch->level < 50 )
      --max_dist;
   if( ch->level < 40 )
      --max_dist;
   if( ch->level < 30 )
      --max_dist;

   for( dist = 1; dist <= max_dist; )
   {
      if( !pexit->to_room || xIS_SET( pexit->exit_info, EX_HIDDEN ) || xIS_SET( pexit->exit_info, EX_SECRET )
      || xIS_SET( pexit->exit_info, EX_DIG ) )
      {
          act( AT_GRAY, "Your view $t is blocked by a wall.", ch, (char *)dir_name[dir], NULL, TO_CHAR );
          break;
      }
      if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         act( AT_GRAY, "Your view $t is blocked by a door.", ch, (char *)dir_name[dir], NULL, TO_CHAR );
         break;
      }
      if( room_is_private( pexit->to_room ) && get_trust( ch ) < PERM_LEADER )
      {
         act( AT_GRAY, "Your view $t is blocked by a private room.", ch, (char *)dir_name[dir], NULL, TO_CHAR );
         break;
      }
      char_from_room( ch );
      char_to_room( ch, pexit->to_room );
      set_char_color( AT_RMNAME, ch );
      send_to_char( ch->in_room->name, ch );
      send_to_char( "\r\n", ch );
      show_list_to_char( ch->in_room->first_content, ch, false, false );
      show_char_to_char( ch->in_room->first_person, ch );

      if( ch->in_room->sector_type == SECT_AIR && number_percent( ) < 80 )
         dist++;
      else if( ch->in_room->sector_type == SECT_INSIDE
      || ch->in_room->sector_type == SECT_FIELD
      || ch->in_room->sector_type == SECT_UNDERGROUND )
         dist++;
      else if( ch->in_room->sector_type == SECT_FOREST
      || ch->in_room->sector_type == SECT_CITY
      || ch->in_room->sector_type == SECT_DESERT
      || ch->in_room->sector_type == SECT_HILLS )
         dist += 2;
      else if( ch->in_room->sector_type == SECT_WATER_SWIM
      || ch->in_room->sector_type == SECT_WATER_NOSWIM )
         dist += 3;
      else if( ch->in_room->sector_type == SECT_MOUNTAIN
      || ch->in_room->sector_type == SECT_UNDERWATER
      || ch->in_room->sector_type == SECT_OCEANFLOOR )
         dist += 4;
      else
         dist++;

      if( dist >= max_dist )
      {
         act( AT_GRAY, "Your vision blurs with distance and you see no farther $t.", ch, (char *)dir_name[dir], NULL, TO_CHAR );
         break;
      }
      if( !( pexit = get_exit( ch->in_room, dir ) ) )
      {
         act( AT_GRAY, "Your view $t is blocked by a wall.", ch, (char *)dir_name[dir], NULL, TO_CHAR );
         break;
      }
   }
   char_from_room( ch );
   char_to_room( ch, was_in_room );
   learn_from_success( ch, gsn_scan );
}

/*
 * Basically the same guts as do_scan() from above (please keep them in
 * sync) used to find the victim we're firing at.	-Thoric
 */
CHAR_DATA *scan_for_victim( CHAR_DATA *ch, EXIT_DATA *pexit, char *name )
{
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *was_in_room;
   short dist, dir, max_dist = 8;

   if( IS_AFFECTED( ch, AFF_BLIND ) || !pexit )
      return NULL;

   was_in_room = ch->in_room;
   if( is_vampire( ch ) && time_info.hour < 21 && time_info.hour > 5 )
      max_dist = 1;

   if( ch->level < 50 )
      --max_dist;
   if( ch->level < 40 )
      --max_dist;
   if( ch->level < 30 )
      --max_dist;

   for( dist = 1; dist <= max_dist; )
   {
      if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
         break;

      if( room_is_private( pexit->to_room ) && get_trust( ch ) < PERM_LEADER )
         break;

      char_from_room( ch );
      char_to_room( ch, pexit->to_room );
      if( ( victim = get_char_room( ch, name ) ) )
      {
         char_from_room( ch );
         char_to_room( ch, was_in_room );
         return victim;
      }

      if( ch->in_room->sector_type == SECT_AIR && number_percent( ) < 80 )
         dist++;
      else if( ch->in_room->sector_type == SECT_INSIDE
      || ch->in_room->sector_type == SECT_FIELD
      || ch->in_room->sector_type == SECT_UNDERGROUND )
         dist++;
      else if( ch->in_room->sector_type == SECT_FOREST
      || ch->in_room->sector_type == SECT_CITY
      || ch->in_room->sector_type == SECT_DESERT
      || ch->in_room->sector_type == SECT_HILLS )
         dist += 2;
      else if( ch->in_room->sector_type == SECT_WATER_SWIM
      || ch->in_room->sector_type == SECT_WATER_NOSWIM )
         dist += 3;
      else if( ch->in_room->sector_type == SECT_MOUNTAIN
      || ch->in_room->sector_type == SECT_UNDERWATER
      || ch->in_room->sector_type == SECT_OCEANFLOOR )
         dist += 4;
      else
         dist++;

      if( dist >= max_dist )
         break;

      dir = pexit->vdir;
      if( !( pexit = get_exit( ch->in_room, dir ) ) )
         break;
   }
   char_from_room( ch );
   char_to_room( ch, was_in_room );

   return NULL;
}

/*
 * Search inventory for an appropriate projectile to fire.
 * Also search open quivers. - Thoric
 */
OBJ_DATA *find_projectile( CHAR_DATA *ch, int type )
{
   OBJ_DATA *obj, *obj2;

   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
   {
      if( can_see_obj( ch, obj ) )
      {
         if( obj->item_type == ITEM_QUIVER && !IS_SET( obj->value[1], CONT_CLOSED ) )
         {
            for( obj2 = obj->last_content; obj2; obj2 = obj2->prev_content )
            {
               if( obj2->item_type == ITEM_PROJECTILE && obj2->value[3] == type && !is_obj_stat( obj2, ITEM_LODGED ) )
                  return obj2;
            }
         }
         if( obj->item_type == ITEM_PROJECTILE && obj->value[3] == type && !is_obj_stat( obj, ITEM_LODGED ) )
            return obj;
      }
   }

   return NULL;
}

ch_ret spell_attack( int, int, CHAR_DATA *, void * );

/* Perform the actual attack on a victim - Thoric */
ch_ret ranged_got_target( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon, OBJ_DATA *projectile, short dist, short dt, char *stxt, short color )
{
   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      /* safe room, bubye projectile */
      if( projectile )
      {
         ch_printf( ch, "Your %s is blasted from existance by a godly presense.", myobj( projectile ) );
         act( color, "A godly presence smites $p!", ch, projectile, NULL, TO_ROOM );
         extract_obj( projectile );
      }
      else
      {
         ch_printf( ch, "Your %s is blasted from existance by a godly presense.", stxt );
         act( color, "A godly presence smites $t!", ch, aoran( stxt ), NULL, TO_ROOM );
      }
      return rNONE;
   }

   if( is_npc( victim ) && xIS_SET( victim->act, ACT_SENTINEL ) && ch->in_room != victim->in_room )
   {
      /* always miss if not in the same room */
      if( projectile )
      {
         if( weapon && projectile && can_use_skill( ch, 1, gsn_missile_weapons ) )
            learn_from_failure( ch, gsn_missile_weapons );

         /* 50% chance of projectile getting lost */
         if( number_percent( ) < 50 )
         {
            extract_obj( projectile );
         }
         else
         {
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, victim->in_room );
          }
      }
      return damage( ch, victim, NULL, 0, dt, true );
   }

   if( number_percent( ) > 25 || ( projectile && weapon && can_use_skill( ch, number_percent( ), gsn_missile_weapons ) ) )
   {
      if( projectile )
      {
         global_retcode = projectile_hit( ch, victim, weapon, projectile, dist );
         if( weapon && projectile && can_use_skill( ch, 1, gsn_missile_weapons ) )
            learn_from_success( ch, gsn_missile_weapons );
      }
      else
         global_retcode = spell_attack( dt, ch->level, ch, victim );
   }
   else
   {
      /* Should only be increased if they are useing a weapon and projectile and know the skill */
      if( weapon && projectile && can_use_skill( ch, 1, gsn_missile_weapons ) )
         learn_from_failure( ch, gsn_missile_weapons );

      global_retcode = damage( ch, victim, NULL, 0, dt, true );

      if( projectile )
      {
         /* 50% chance of getting lost */
         if( number_percent( ) < 50 )
         {
            extract_obj( projectile );
         }
         else
         {
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, victim->in_room );
         }
      }
   }
   return global_retcode;
}

/* Generic use ranged attack function - Thoric & Tricops */
ch_ret ranged_attack( CHAR_DATA *ch, char *argument, OBJ_DATA *weapon, OBJ_DATA *projectile, short dt, short range )
{
   CHAR_DATA *victim, *vch;
   OBJ_DATA *use_projectile = NULL; /* The projectile to use, current or a duplicated one */
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *was_in_room;
   SKILLTYPE *skill = NULL;
   char arg[MIL], arg1[MIL], temp[MIL], buf[MSL];
   const char *dtxt = "somewhere";
   const char *stxt = "burst of energy";
   int count;
   short dir = -1, dist = 0, color = AT_GRAY;

   if( argument && argument[0] != '\0' && argument[0] == '\'' )
   {
      one_argument( argument, temp );
      argument = temp;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg1 );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Where?  At who?\r\n", ch );
      return rNONE;
   }

   victim = NULL;

   /* get an exit or a victim */
   if( !( pexit = find_door( ch, arg, true ) ) )
   {
      if( !( victim = get_char_room( ch, arg ) ) )
      {
         send_to_char( "Aim in what direction?\r\n", ch );
         return rNONE;
      }
      else
      {
         if( who_fighting( ch ) == victim )
         {
            send_to_char( "They are too close to release that type of attack!\r\n", ch );
            return rNONE;
         }
      }
   }
   else
      dir = pexit->vdir;

   /* check for ranged attacks from private rooms, etc */
   if( !victim )
   {
      if( xIS_SET( ch->in_room->room_flags, ROOM_PRIVATE ) || xIS_SET( ch->in_room->room_flags, ROOM_SOLITARY ) )
      {
         send_to_char( "You can't perform a ranged attack from a private room.\r\n", ch );
         return rNONE;
      }
      if( ch->in_room->tunnel > 0 )
      {
         count = 0;
         for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
            ++count;
         if( count >= ch->in_room->tunnel )
         {
            send_to_char( "You're too cramped to perform such an attack.\r\n", ch );
            return rNONE;
         }
      }
   }

   if( is_valid_sn( dt ) )
      skill = skill_table[dt];

   if( pexit && !pexit->to_room )
   {
      send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
      return rNONE;
   }

   /* Check for obstruction */
   if( pexit && xIS_SET( pexit->exit_info, EX_CLOSED ) )
   {
      if( xIS_SET( pexit->exit_info, EX_SECRET ) || xIS_SET( pexit->exit_info, EX_DIG ) )
         send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
      else
         send_to_char( "Are you expecting to fire through a door!?\r\n", ch );
      return rNONE;
   }

   vch = NULL;
   if( pexit && arg1[0] != '\0' )
   {
      if( !( vch = scan_for_victim( ch, pexit, arg1 ) ) )
      {
         send_to_char( "You can't see your target.\r\n", ch );
         return rNONE;
      }

      /* don't allow attacks on mobs that are in a no-missile room --Shaddai */
      if( xIS_SET( vch->in_room->room_flags, ROOM_NOMISSILE ) )
      {
         send_to_char( "You can't get a clean shot off.\r\n", ch );
         return rNONE;
      }

      /* can't properly target someone heavily in battle */
      if( vch->num_fighting > MAX_FIGHT )
      {
         send_to_char( "There is too much activity there for you to get a clear shot.\r\n", ch );
         return rNONE;
      }
   }
   if( vch )
   {
      if( !is_npc( vch ) && !is_npc( ch ) && xIS_SET( ch->act, PLR_NICE ) )
      {
         send_to_char( "You're too nice to do that!\r\n", ch );
         return rNONE;
      }
      if( vch && is_safe( ch, vch, true ) )
         return rNONE;
   }

   was_in_room = ch->in_room;

   if( projectile )
   {
      /* Used to make a new projectile to use, and leave the old one where it was */
      if( is_npc( ch ) || xIS_SET( projectile->extra_flags, ITEM_CONTINUOUS_FIRE ) )
      {
         if( !( use_projectile = create_object( projectile->pIndexData, projectile->level ) ) )
         {
            bug( "%s: failed to create a new projectile for [%d]%s.", __FUNCTION__, projectile->pIndexData->vnum, projectile->name );
            send_to_char( "Failed to create a new projectile to fire.\r\n", ch );
            return rNONE;
         }

         /* Take continuous fire off the new one */
         xREMOVE_BIT( use_projectile->extra_flags, ITEM_CONTINUOUS_FIRE );
      }
      else
         use_projectile = projectile;

      separate_obj( use_projectile );
      if( pexit )
      {
         if( weapon )
         {
            act( AT_GRAY, "You fire $p $T.", ch, use_projectile, (char *)dir_name[dir], TO_CHAR );
            act( AT_GRAY, "$n fires $p $T.", ch, use_projectile, (char *)dir_name[dir], TO_ROOM );
         }
         else
         {
            act( AT_GRAY, "You throw $p $T.", ch, use_projectile, (char *)dir_name[dir], TO_CHAR );
            act( AT_GRAY, "$n throw $p $T.", ch, use_projectile, (char *)dir_name[dir], TO_ROOM );
         }
      }
      else
      {
         if( weapon )
         {
            act( AT_GRAY, "You fire $p at $N.", ch, use_projectile, victim, TO_CHAR );
            act( AT_GRAY, "$n fires $p at $N.", ch, use_projectile, victim, TO_NOTVICT );
            act( AT_GRAY, "$n fires $p at you!", ch, use_projectile, victim, TO_VICT );
         }
         else
         {
            act( AT_GRAY, "You throw $p at $N.", ch, use_projectile, victim, TO_CHAR );
            act( AT_GRAY, "$n throws $p at $N.", ch, use_projectile, victim, TO_NOTVICT );
            act( AT_GRAY, "$n throws $p at you!", ch, use_projectile, victim, TO_VICT );
         }
      }
   }
   else if( skill )
   {
      if( skill->noun_damage && skill->noun_damage[0] != '\0' )
         stxt = skill->noun_damage;
      else
         stxt = skill->name;
      /* a plain "spell" flying around seems boring */
      if( !str_cmp( stxt, "spell" ) )
         stxt = (char *)"magical burst of energy";
      if( skill->type == SKILL_SPELL )
      {
         color = AT_MAGIC;
         if( pexit )
         {
            act( AT_MAGIC, "You release $t $T.", ch, aoran( stxt ), (char *)dir_name[dir], TO_CHAR );
            act( AT_MAGIC, "$n releases $s $t $T.", ch, (char *)stxt, (char *)dir_name[dir], TO_ROOM );
         }
         else
         {
            act( AT_MAGIC, "You release $t at $N.", ch, aoran( stxt ), victim, TO_CHAR );
            act( AT_MAGIC, "$n releases $s $t at $N.", ch, (char *)stxt, victim, TO_NOTVICT );
            act( AT_MAGIC, "$n releases $s $t at you!", ch, (char *)stxt, victim, TO_VICT );
         }
      }
   }
   else
   {
      bug( "%s: no projectile, no skill dt %d", __FUNCTION__, dt );
      return rNONE;
   }

   /* victim in same room */
   if( victim )
   {
      if( use_projectile )
         return ranged_got_target( ch, victim, weapon, use_projectile, 0, dt, (char *)stxt, color );
      else
         return ranged_got_target( ch, victim, weapon, projectile, 0, dt, (char *)stxt, color );
   }

   /* assign scanned victim */
   victim = vch;

   /* reverse direction text from move_char */
   dtxt = rev_exit( pexit->vdir );

   while( dist <= range )
   {
      char_from_room( ch );
      char_to_room( ch, pexit->to_room );

      if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         /* what do you know, the door's closed */
         if( use_projectile )
            snprintf( buf, sizeof( buf ), "You see your %s pierce a door in the distance to the %s.",
               myobj( use_projectile ), dir_name[dir] );
         else if( projectile )
            snprintf( buf, sizeof( buf ), "You see your %s pierce a door in the distance to the %s.",
               myobj( projectile ), dir_name[dir] );
         else
            snprintf( buf, sizeof( buf ), "You see your %s hit a door in the distance to the %s.", stxt, dir_name[dir] );
         act( color, buf, ch, NULL, NULL, TO_CHAR );
         if( use_projectile )
         {
            snprintf( buf, sizeof( buf ), "$p flies in from %s and implants itself solidly in the %sern door.", dtxt,
               dir_name[dir] );
            act( color, buf, ch, use_projectile, NULL, TO_ROOM );
            extract_obj( use_projectile );
         }
         else if( projectile )
         {
            snprintf( buf, sizeof( buf ), "$p flies in from %s and implants itself solidly in the %sern door.", dtxt,
               dir_name[dir] );
            act( color, buf, ch, projectile, NULL, TO_ROOM );
            extract_obj( projectile );
         }
         else
         {
            snprintf( buf, sizeof( buf ), "%s flies in from %s and implants itself solidly in the %sern door.",
               aoran( stxt ), dtxt, dir_name[dir] );
            buf[0] = UPPER( buf[0] );
            act( color, buf, ch, NULL, NULL, TO_ROOM );
         }
         break;
      }

      /* no victim? pick a random one */
      if( !victim )
      {
         for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
         {
            if( ( ( is_npc( ch ) && !is_npc( vch ) ) || ( !is_npc( ch ) && is_npc( vch ) ) ) && number_bits( 1 ) == 0 )
            {
               victim = vch;
               break;
            }
         }
         if( victim && is_safe( ch, victim, true ) )
         {
            char_from_room( ch );
            char_to_room( ch, was_in_room );
            return rNONE;
         }
      }

      /* In the same room as our victim? */
      if( victim && ch->in_room == victim->in_room )
      {
         if( use_projectile )
            act( color, "$p flies in from $T.", ch, use_projectile, (char *)dtxt, TO_ROOM );
         else if( projectile )
            act( color, "$p flies in from $T.", ch, projectile, (char *)dtxt, TO_ROOM );
         else
            act( color, "$t flies in from $T.", ch, aoran( stxt ), (char *)dtxt, TO_ROOM );

         /* get back before the action starts */
         char_from_room( ch );
         char_to_room( ch, was_in_room );

         if( use_projectile )
            return ranged_got_target( ch, victim, weapon, use_projectile, dist, dt, (char *)stxt, color );
         else
            return ranged_got_target( ch, victim, weapon, projectile, dist, dt, (char *)stxt, color );
      }

      if( dist == range )
      {
         if( use_projectile )
         {
            act( color, "Your $t falls harmlessly to the ground to the $T.", ch, myobj( use_projectile ), (char *)dir_name[dir], TO_CHAR );
            act( color, "$p flies in from $T and falls harmlessly to the ground here.", ch, use_projectile, (char *)dtxt, TO_ROOM );
            if( use_projectile->in_obj )
               obj_from_obj( use_projectile );
            if( use_projectile->carried_by )
               obj_from_char( use_projectile );
            obj_to_room( use_projectile, ch->in_room );
         }
         else if( projectile )
         {
            act( color, "Your $t falls harmlessly to the ground to the $T.", ch, myobj( projectile ), (char *)dir_name[dir], TO_CHAR );
            act( color, "$p flies in from $T and falls harmlessly to the ground here.", ch, projectile, (char *)dtxt, TO_ROOM );
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, ch->in_room );
         }
         else
         {
            act( color, "Your $t fizzles out harmlessly to the $T.", ch, (char *)stxt, (char *)dir_name[dir], TO_CHAR );
            act( color, "$t flies in from $T and fizzles out harmlessly.", ch, aoran( (char *)stxt ), (char *)dtxt, TO_ROOM );
         }
         break;
      }

      if( !( pexit = get_exit( ch->in_room, dir ) ) )
      {
         if( use_projectile )
         {
            act( color, "Your $t hits a wall and bounces harmlessly to the ground to the $T.", ch, myobj( use_projectile ), (char *)dir_name[dir], TO_CHAR );
            act( color, "$p strikes the $Tsern wall and falls harmlessly to the ground.", ch, use_projectile, (char *)dir_name[dir], TO_ROOM );
            if( use_projectile->in_obj )
               obj_from_obj( use_projectile );
            if( use_projectile->carried_by )
               obj_from_char( use_projectile );
            obj_to_room( use_projectile, ch->in_room );
         }
         else if( projectile )
         {
            act( color, "Your $t hits a wall and bounces harmlessly to the ground to the $T.", ch, myobj( projectile ), (char *)dir_name[dir], TO_CHAR );
            act( color, "$p strikes the $Tsern wall and falls harmlessly to the ground.", ch, projectile, (char *)dir_name[dir], TO_ROOM );
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, ch->in_room );
         }
         else
         {
            act( color, "Your $t harmlessly hits a wall to the $T.", ch, (char *)stxt, (char *)dir_name[dir], TO_CHAR );
            act( color, "$t strikes the $Tsern wall and falls harmlessly to the ground.",
                 ch, aoran( (char *)stxt ), (char *)dir_name[dir], TO_ROOM );
         }
         break;
      }
      if( use_projectile )
         act( color, "$p flies in from $T.", ch, use_projectile, (char *)dtxt, TO_ROOM );
      else if( projectile )
         act( color, "$p flies in from $T.", ch, projectile, (char *)dtxt, TO_ROOM );
      else
         act( color, "$t flies in from $T.", ch, aoran( (char *)stxt ), (char *)dtxt, TO_ROOM );
      dist++;
   }

   char_from_room( ch );
   char_to_room( ch, was_in_room );

   return rNONE;
}

/*
 * Fire <direction> <target>
 * Fire a projectile from a missile weapon (bow, crossbow, etc)
 * Design by Thoric, coding by Thoric and Tricops.
 * Support code (see projectile_hit(), quiver support, other changes to
 * fight.c, etc by Thoric.
 */
CMDF( do_fire )
{
   OBJ_DATA *arrow, *bow;
   short max_dist;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Fire where at who?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
   {
      send_to_char( "&[magic]You can't fire anything in the wilderness.\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      send_to_char( "&[magic]A magical force prevents you from attacking.\r\n", ch );
      return;
   }

   /* Probably to busy in water to fire anything */
   if( ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_WATER_NOSWIM
   || ch->in_room->sector_type == SECT_UNDERWATER || ch->in_room->sector_type == SECT_OCEANFLOOR )
   {
      send_to_char( "You're to busy dealing with all the water to fire anything.\r\n", ch );
      return;
   }

   /* Find the projectile weapon */
   if( !( bow = get_eq_hold( ch, ITEM_MISSILE_WEAPON ) ) )
   {
      send_to_char( "You aren't wielding a missile weapon.\r\n", ch );
      return;
   }

   /* modify maximum distance based on bow-type and ch's class/str/etc */
   max_dist = URANGE( 1, bow->value[4], 10 );

   if( !( arrow = find_projectile( ch, bow->value[3] ) ) )
   {
      const char *msg = "You have nothing to fire...\r\n";

      switch( bow->value[3] )
      {
         case DAM_BOLT:
            msg = "You have no bolts...\r\n";
            break;

         case DAM_ARROW:
            msg = "You have no arrows...\r\n";
            break;

         case DAM_DART:
            msg = "You have no darts...\r\n";
            break;

         case DAM_STONE:
            msg = "You have no slingstones...\r\n";
            break;

         case DAM_PEA:
            msg = "You have no peas...\r\n";
            break;
      }
      send_to_char( msg, ch );
      return;
   }

   /* Add wait state to fire for pkill, etc... */
   wait_state( ch, 6 );

   /* handle the ranged attack */
   ranged_attack( ch, argument, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );
}

/*
 * Attempt to fire at a victim.
 * Returns false if no attempt was made
 */
bool mob_fire( CHAR_DATA *ch, char *name )
{
   OBJ_DATA *arrow, *bow;
   short max_dist;

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
      return false;

   if( xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
      return false;

   if( !( bow = get_eq_hold( ch, ITEM_MISSILE_WEAPON ) ) )
      return false;

   /* modify maximum distance based on bow-type and ch's class/str/etc */
   max_dist = URANGE( 1, bow->value[4], 10 );

   if( !( arrow = find_projectile( ch, bow->value[3] ) ) )
      return false;

   ranged_attack( ch, name, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );

   return true;
}

CMDF( do_slice )
{
   OBJ_DATA *corpse, *obj, *slice;
   MOB_INDEX_DATA *pMobIndex;
   char buf[MSL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "From what do you wish to slice meat?\r\n", ch );
      return;
   }

   if( !( obj = get_eq_hold( ch, ITEM_WEAPON ) )
   || ( obj->value[3] != DAM_SLICE && obj->value[3] != DAM_STAB
   && obj->value[3] != DAM_SLASH && obj->value[3] != DAM_PIERCE ) )
   {
      send_to_char( "You need to have a sharp weapon in hand.\r\n", ch );
      return;
   }

   if( !( corpse = get_obj_here( ch, argument ) ) )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }

   if( corpse->item_type != ITEM_CORPSE_NPC || corpse->value[0] < 1 )
   {
      send_to_char( "That is not a suitable source of meat.\r\n", ch );
      return;
   }

   if( !( pMobIndex = get_mob_index( corpse->value[1] ) ) )
   {
      bug( "%s: Can not find mob for value[1] of corpse", __FUNCTION__ );
      return;
   }

   if( !get_obj_index( OBJ_VNUM_SLICE ) )
   {
      bug( "%s: Vnum %d not found!", __FUNCTION__, OBJ_VNUM_SLICE );
      return;
   }

   if( !can_use_skill( ch, number_percent( ), gsn_slice ) && !is_immortal( ch ) )
   {
      send_to_char( "You fail to slice the meat properly.\r\n", ch );
      learn_from_failure( ch, gsn_slice );   /* Just in case they die :> */
      if( number_percent( ) + ( get_curr_dex( ch ) - 13 ) < 10 )
      {
         act( AT_BLOOD, "You cut yourself!", ch, NULL, NULL, TO_CHAR );
         damage( ch, ch, NULL, ch->level, gsn_slice, true );
      }
      return;
   }

   if( !( slice = create_object( get_obj_index( OBJ_VNUM_SLICE ), 0 ) ) )
   {
      bug( "%s: Couldn't create an object from index vnum %d.", __FUNCTION__, OBJ_VNUM_SLICE );
      return;
   }

   slice->value[3] = 1;

   snprintf( buf, sizeof( buf ), "meat fresh slice %s", pMobIndex->name );
   STRSET( slice->name, buf );

   snprintf( buf, sizeof( buf ), "a slice of raw meat from %s", pMobIndex->short_descr );
   STRSET( slice->short_descr, buf );

   snprintf( buf, sizeof( buf ), "A slice of raw meat from %s lies on the ground.", pMobIndex->short_descr );
   STRSET( slice->description, buf );

   act( AT_BLOOD, "$n cuts a slice of meat from $p.", ch, corpse, NULL, TO_ROOM );
   act( AT_BLOOD, "You cut a slice of meat from $p.", ch, corpse, NULL, TO_CHAR );

   obj_to_char( slice, ch );
   corpse->value[0] -= 1;
   learn_from_success( ch, gsn_slice );
}

/*  Fighting Styles - haus */
CMDF( do_style )
{
   char arg[MIL];

   if( !ch || is_npc( ch ) )
      return;

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      ch_printf( ch, "&wAdopt which fighting style?  (current:  %s&w)\r\n",
         capitalize( style_names[ch->style] ) );
      return;
   }

   if( !str_prefix( arg, "evasive" ) )
   {
      if( !can_practice( ch, gsn_style_evasive ) )
      {
         send_to_char( "You haven't yet learned enough to fight evasively.\r\n", ch );
         return;
      }
      wait_state( ch, skill_table[gsn_style_evasive]->beats );
      if( number_percent( ) < LEARNED( ch, gsn_style_evasive ) )
      {
         if( ch->fighting )
         {
            ch->position = POS_EVASIVE;
            learn_from_success( ch, gsn_style_evasive );
            act( AT_ACTION, "$n falls back into an evasive stance.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_EVASIVE;
         send_to_char( "You adopt an evasive fighting style.\r\n", ch );
         return;
      }
      else
      {
         send_to_char( "You nearly trip in a lame attempt to adopt an evasive fighting style.\r\n", ch );
         return;
      }
   }
   else if( !str_prefix( arg, "defensive" ) )
   {
      if( !can_practice( ch, gsn_style_defensive ) )
      {
         send_to_char( "You haven't learned enough to fight defensively.\r\n", ch );
         return;
      }
      wait_state( ch, skill_table[gsn_style_defensive]->beats );
      if( number_percent( ) < LEARNED( ch, gsn_style_defensive ) )
      {
         if( ch->fighting )
         {
            ch->position = POS_DEFENSIVE;
            learn_from_success( ch, gsn_style_defensive );
            act( AT_ACTION, "$n moves into a defensive posture.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_DEFENSIVE;
         send_to_char( "You adopt a defensive fighting style.\r\n", ch );
         return;
      }
      else
      {
         send_to_char( "You nearly trip in a lame attempt to adopt a defensive fighting style.\r\n", ch );
         return;
      }
   }
   else if( !str_prefix( arg, "standard" ) )
   {
      if( !can_practice( ch, gsn_style_standard ) )
      {
         send_to_char( "You haven't learned enough to fight in the standard style.\r\n", ch );
         return;
      }
      wait_state( ch, skill_table[gsn_style_standard]->beats );
      if( number_percent( ) < LEARNED( ch, gsn_style_standard ) )
      {
         if( ch->fighting )
         {
            ch->position = POS_FIGHTING;
            learn_from_success( ch, gsn_style_standard );
            act( AT_ACTION, "$n switches to a standard fighting style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_FIGHTING;
         send_to_char( "You adopt a standard fighting style.\r\n", ch );
         return;
      }
      else
      {
         send_to_char( "You nearly trip in a lame attempt to adopt a standard fighting style.\r\n", ch );
         return;
      }
   }
   else if( !str_prefix( arg, "aggressive" ) )
   {
      if( !can_practice( ch, gsn_style_aggressive ) )
      {
         send_to_char( "You haven't learned enough to fight aggressively.\r\n", ch );
         return;
      }
      wait_state( ch, skill_table[gsn_style_aggressive]->beats );
      if( number_percent( ) < LEARNED( ch, gsn_style_aggressive ) )
      {
         if( ch->fighting )
         {
            ch->position = POS_AGGRESSIVE;
            learn_from_success( ch, gsn_style_aggressive );
            act( AT_ACTION, "$n assumes an aggressive stance.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_AGGRESSIVE;
         send_to_char( "You adopt an aggressive fighting style.\r\n", ch );
         return;
      }
      else
      {
         send_to_char( "You nearly trip in a lame attempt to adopt an aggressive fighting style.\r\n", ch );
         return;
      }
   }
   else if( !str_prefix( arg, "berserk" ) )
   {
      if( !can_practice( ch, gsn_style_berserk ) )
      {
         send_to_char( "You haven't learned enough to fight as a berserker.\r\n", ch );
         return;
      }
      wait_state( ch, skill_table[gsn_style_berserk]->beats );
      if( number_percent( ) < LEARNED( ch, gsn_style_berserk ) )
      {
         if( ch->fighting )
         {
            ch->position = POS_BERSERK;
            learn_from_success( ch, gsn_style_berserk );
            act( AT_ACTION, "$n enters a wildly aggressive style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_BERSERK;
         send_to_char( "You adopt a berserk fighting style.\r\n", ch );
         return;
      }
      else
      {
         send_to_char( "You nearly trip in a lame attempt to adopt a berserk fighting style.\r\n", ch );
         return;
      }
   }

   send_to_char( "Adopt which fighting style?\r\n", ch );
}

/*  New check to see if you can use skills to support morphs --Shaddai */
bool can_use_skill( CHAR_DATA *ch, int percent, int gsn )
{
   bool check = false;

   if( is_npc( ch ) && percent < 85 )
      check = true;
   else if( !is_npc( ch ) && percent < LEARNED( ch, gsn ) )
      check = true;
   if( ch->morph && ch->morph->morph )
   {
      if( ch->morph->morph->skills && is_name( skill_table[gsn]->name, ch->morph->morph->skills ) && percent < 85 )
         check = true;
      if( ch->morph->morph->no_skills && is_name( skill_table[gsn]->name, ch->morph->morph->no_skills ) )
         check = false;
   }
   return check;
}

/* Cook was coded by Blackmane and heavily modified by Shaddai */
CMDF( do_cook )
{
   OBJ_DATA *food, *fire;
   char buf[MSL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Cook what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( food = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You don't have that item.\r\n", ch );
      return;
   }

   if( food->item_type != ITEM_COOK && food->item_type != ITEM_FISH )
   {
      send_to_char( "How can you cook that?\r\n", ch );
      return;
   }

   if( food->value[2] > 2 )
   {
      send_to_char( "That is already burnt to a crisp.\r\n", ch );
      return;
   }

   if( food->value[2] > 1 )
   {
      send_to_char( "That is already overcooked.\r\n", ch );
      return;
   }

   if( food->value[2] > 0 )
   {
      send_to_char( "That is already roasted.\r\n", ch );
      return;
   }

   for( fire = ch->in_room->first_content; fire; fire = fire->next_content )
   {
      if( fire->item_type == ITEM_FIRE )
         break;
   }

   if( !fire )
   {
      send_to_char( "There is no fire here!\r\n", ch );
      return;
   }

   separate_obj( food );   /* Bug catch by Tchaika from SMAUG list */
   if( number_percent( ) > LEARNED( ch, gsn_cook ) )
   {
      if( food->timer )
         food->timer /= 2;
      food->value[0] = 0;
      food->value[2] = 3;
      food->value[3] = 0;
      act( AT_MAGIC, "$p catches on fire burning it to a crisp!\r\n", ch, food, NULL, TO_CHAR );
      act( AT_MAGIC, "$n catches $p on fire burning it to a crisp.", ch, food, NULL, TO_ROOM );

      snprintf( buf, sizeof( buf ), "a burnt %s", food->name );
      STRSET( food->short_descr, buf );
      snprintf( buf, sizeof( buf ), "A burnt %s.", food->name );
      STRSET( food->description, buf );
      learn_from_failure( ch, gsn_cook );
      return;
   }

   if( number_percent( ) > 85 )
   {
      if( food->timer )
         food->timer *= 3;
      food->value[2] = 2;
      food->value[0] += 1;
      food->value[3] = 0;
      act( AT_MAGIC, "$n overcooks $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You overcook $p.", ch, food, NULL, TO_CHAR );
      snprintf( buf, sizeof( buf ), "an overcooked %s", food->name );
      STRSET( food->short_descr, buf );
      snprintf( buf, sizeof( buf ), "An overcooked %s.", food->name );
      STRSET( food->description, buf );
   }
   else
   {
      if( food->timer )
         food->timer *= 4;
      if( food->value[0] )
         food->value[0] *= 2;
      else
         food->value[0] += 4;
      act( AT_MAGIC, "$n roasts $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You roast $p.", ch, food, NULL, TO_CHAR );
      snprintf( buf, sizeof( buf ), "a roasted %s", food->name );
      STRSET( food->short_descr, buf );
      snprintf( buf, sizeof( buf ), "A roasted %s.", food->name );
      STRSET( food->description, buf );
      food->value[2] = 1;
      food->value[3] = 0;
   }
   learn_from_success( ch, gsn_cook );
}

/* Currently does a number_range( 1, 5 ) for number of charges. */
CMDF( do_imbue )
{
   OBJ_DATA *wand;
   char buf[MSL];
   int mana, sn;

   if( is_npc( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Imbue what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      send_to_char( "You haven't learned that spell.\r\n", ch );
      return;
   }

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\r\n", ch );
      return;
   }

   if( !SPELL_FLAG( skill_table[sn], SF_CANIMBUE ) && !SPELL_FLAG( skill_table[sn], SF_CANDOALL ) )
   {
      send_to_char( "You can't imbue that spell.\r\n", ch );
      return;
   }

   mana = is_npc( ch ) ? 0 : UMAX( skill_table[sn]->min_mana, mana_cost( ch, sn ) );
   mana *= 4;

   if( !is_npc( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   if( !( wand = get_eq_hold( ch, ITEM_WAND ) ) || wand->value[3] != -1 )
   {
      send_to_char( "You must be holding a wand to imbue.\r\n", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_imbue );
      ch->mana -= (mana / 2);
      return;
   }

   if( !can_use_skill( ch, number_percent( ), gsn_imbue ) )
   {
      set_char_color ( AT_MAGIC, ch );
      send_to_char("You failed.\r\n", ch);
      learn_from_failure( ch, gsn_imbue );
      ch->mana -= (mana / 2);
      return;
   }

   wand->value[3] = sn;

   snprintf( buf, sizeof( buf ), "%s wand", skill_table[wand->value[3]]->name );
   STRSET( wand->short_descr, aoran( buf ) );

   snprintf( buf, sizeof( buf ), "A small wand labelled '%s' is laying here.", skill_table[wand->value[3]]->name );
   STRSET( wand->description, buf );

   snprintf( buf, sizeof( buf ), "wand %s", skill_table[wand->value[3]]->name );
   STRSET( wand->name, buf );

   if( wand->value[0] == 0 || wand->value[0] > ch->level )
      wand->value[0] = ch->level;

   wand->value[1] = number_range( 1, 5 );
   wand->value[2] = wand->value[1];

   act( AT_MAGIC, "$n imbues $p.", ch, wand, NULL, TO_ROOM );
   act( AT_MAGIC, "You imbue $p.", ch, wand, NULL, TO_CHAR );

   learn_from_success( ch, gsn_imbue );
   ch->mana -= mana;
}

/* Currently does a number_range( 1, 5 ) for number of charges. */
CMDF( do_carve )
{
   OBJ_DATA *staff, *gem;
   char buf[MSL], gname[MSL];
   int sn, mana;
   bool found;

   if( is_npc( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Carve what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      send_to_char( "You haven't learned that spell.\r\n", ch );
      return;
   }

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\r\n", ch );
      return;
   }

   if( !SPELL_FLAG( skill_table[sn], SF_CANCARVE ) && !SPELL_FLAG( skill_table[sn], SF_CANDOALL ) )
   {
      send_to_char( "You can't carve that spell.\r\n", ch );
      return;
   }

   mana = is_npc( ch ) ? 0 : UMAX( skill_table[sn]->min_mana, mana_cost( ch, sn ) );
   mana *= 4;

   if( !is_npc( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   found = false;

   if( !( staff = get_eq_hold( ch, ITEM_STAFF ) ) || staff->value[3] != -1 )
   {
      send_to_char( "You must be holding an empty staff to carve.\r\n", ch );
      return;
   }

   for( gem = ch->first_carrying; gem; gem = gem->next_content )
   {
      if( gem->item_type == ITEM_GEM )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "There must be a gem in your inventory to use on the staff.\r\n", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_carve );
      ch->mana -= (mana / 2);
      return;
   }

   if( !can_use_skill( ch, number_percent( ), gsn_carve ) )
   {
      set_char_color ( AT_MAGIC, ch );
      send_to_char("You failed.\r\n", ch);
      learn_from_failure( ch, gsn_carve );
      ch->mana -= (mana / 2);
      return;
   }

   staff->value[3] = sn;

   separate_obj( gem );

   snprintf( gname, sizeof( gname ), "%s", gem->name ? gem->name : "gem" );

   snprintf( buf, sizeof( buf ), "%s encrusted staff of %s", gname, skill_table[staff->value[3]]->name );
   STRSET( staff->short_descr, aoran( buf ) );

   snprintf( buf, sizeof( buf ), "A %s encrusted staff labelled '%s' is laying here.", gname, skill_table[staff->value[3]]->name );
   STRSET( staff->description, buf );

   snprintf( buf, sizeof( buf ), "%s staff %s", gname, skill_table[staff->value[3]]->name );
   STRSET( staff->name, buf );

   if( staff->value[0] == 0 || staff->value[0] > ch->level )
      staff->value[0] = ch->level;

   staff->value[1] = number_range( 1, 5 );
   staff->value[2] = staff->value[1];

   act( AT_MAGIC, "$n carves $p.", ch, staff, NULL, TO_ROOM );
   act( AT_MAGIC, "You carve $p.", ch, staff, NULL, TO_CHAR );

   learn_from_success( ch, gsn_carve );
   ch->mana -= mana;

   extract_obj( gem );
}

CMDF( do_concoct )
{
   OBJ_DATA *obj, *pill = NULL, *powder = NULL, *mortar = NULL;
   char buf1[MSL], buf2[MSL], buf3[MSL];
   int sn, mana;

   if( is_npc( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Concoct what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      send_to_char( "You haven't learned that spell.\r\n", ch );
      return;
   }

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\r\n", ch );
      return;
   }

   if( !SPELL_FLAG( skill_table[sn], SF_CANCONCOCT ) && !SPELL_FLAG( skill_table[sn], SF_CANDOALL ) )
   {
      send_to_char( "You can't concoct that spell.\r\n", ch );
      return;
   }

   mana = is_npc( ch ) ? 0 : UMAX( skill_table[sn]->min_mana, mana_cost( ch, sn ) );
   mana *= 4;

   if( !is_npc( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( !pill && obj->item_type == ITEM_PILL
      && ( obj->value[1] == -1 || obj->value[2] == -1 || obj->value[3] == -1 )
      && obj->value[5] == 1 )
         pill = obj;
      if( !mortar && obj->item_type == ITEM_MORTAR )
         mortar = obj;
      if( !powder && obj->item_type == ITEM_POWDER )
         powder = obj;
      if( pill && powder && mortar )
         break;
   }

   if( !pill )
   {
      send_to_char( "You have no casing to use for concocting.\r\n", ch );
      return;
   }

   if( !powder )
   {
      send_to_char( "You have no powder to use for concocting.\r\n", ch );
      return;
   }

   if( !mortar )
   {
      send_to_char( "You have no mortar to use for concocting.\r\n", ch );
      return;
   }

   if( pill->value[1] != -1 && pill->value[2] != -1 && pill->value[3] != -1 )
   {
      send_to_char( "That casing has already been used all it can.\r\n", ch );
      return;
   }

   separate_obj( mortar );
   if( --mortar->value[0] <= 0 )
      extract_obj( mortar );

   separate_obj( powder );
   extract_obj( powder );

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_concoct );
      ch->mana -= (mana / 2);
      return;
   }

   if( !can_use_skill( ch, number_percent( ), gsn_concoct ) )
   {
      set_char_color ( AT_MAGIC, ch );
      send_to_char("You failed.\r\n", ch);
      learn_from_failure( ch, gsn_concoct );
      ch->mana -= (mana / 2);
      return;
   }

   if( pill->value[1] == -1 )
   {
      pill->value[1] = sn;
   }
   else if( pill->value[2] == -1 )
   {
      if( number_percent( ) > 50 )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "The magic surges out of control and destroys the pill!.\r\n", ch );
         learn_from_failure( ch, gsn_concoct );
         ch->mana -= ( mana / 2 );
         extract_obj( pill );
         return;
      }
      pill->value[2] = sn;
   }
   else if( pill->value[3] == -1 )
   {
      if( number_percent( ) > 30 )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "The magic surges out of control and destroys the pill!.\r\n", ch );
         learn_from_failure( ch, gsn_concoct );
         ch->mana -= ( mana / 2 );
         extract_obj( pill );
         return;
      }
      pill->value[3] = sn;
   }

   if( pill->value[1] != -1 && pill->value[2] != -1 && pill->value[3] != -1 )
   {
      sprintf( buf1, "%s, %s, %s pill",
         skill_table[pill->value[1]]->name,
         skill_table[pill->value[2]]->name,
         skill_table[pill->value[3]]->name );
      sprintf( buf2, "A strange pill labelled '%s' '%s' '%s' lays here.",
         skill_table[pill->value[1]]->name,
         skill_table[pill->value[2]]->name,
         skill_table[pill->value[3]]->name );
      sprintf( buf3, "pill %s %s %s",
         skill_table[pill->value[1]]->name, 
         skill_table[pill->value[2]]->name,
         skill_table[pill->value[3]]->name );
   }
   else if( pill->value[1] != -1 && pill->value[2] != -1 )
   {
      sprintf( buf1, "%s, %s pill",
         skill_table[pill->value[1]]->name,
         skill_table[pill->value[2]]->name );
      sprintf( buf2, "A strange pill labelled '%s' '%s' lays here.",
         skill_table[pill->value[1]]->name,
         skill_table[pill->value[2]]->name );
      sprintf( buf3, "pill %s %s",
         skill_table[pill->value[1]]->name, 
         skill_table[pill->value[2]]->name );
   }
   else if( pill->value[1] != -1 )
   {
      sprintf( buf1, "%s pill",
         skill_table[pill->value[1]]->name );
      sprintf( buf2, "A strange pill labelled '%s' lays here.",
         skill_table[pill->value[1]]->name );
      sprintf( buf3, "pill %s",
         skill_table[pill->value[1]]->name );
   }

   if( pill->value[0] == 0 || pill->value[0] > ch->level )
      pill->value[0] = ch->level;

   pill->value[4] = 1;

   STRSET( pill->short_descr, aoran( buf1 ) );
   STRSET( pill->description, buf2 );
   STRSET( pill->name, buf3 );

   act( AT_MAGIC, "$n concocts $p.", ch, pill, NULL, TO_ROOM );
   act( AT_MAGIC, "You concoct $p.", ch, pill, NULL, TO_CHAR );

   learn_from_success( ch, gsn_concoct );
   ch->mana -= mana;
}

CMDF( do_mix )
{
   OBJ_DATA *obj, *tin = NULL, *fire = NULL, *powder = NULL, *mortar = NULL, *mortar2 = NULL, *mortar3 = NULL;
   OBJ_DATA *powder2 = NULL, *powder3 = NULL;
   char arg1[MIL], arg2[MIL], arg3[MIL], buf1[MSL], buf2[MSL], buf3[MSL];
   int sn = -1, sn2 = -1, sn3 = -1, mana = 0, powdercount = 0, mortarcount = 0, spells = 0;

   if( is_npc( ch ) )
      return;

   if( ms_find_obj( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Mix what?\r\n", ch );
      return;
   }

   if( ( sn = find_spell( ch, arg1, true ) ) >= 0 )
   {
      if( skill_table[sn]->spell_fun == spell_null )
      {
         ch_printf( ch, "%s is not a spell!\r\n", skill_table[sn]->name );
         return;
      }

      if( !SPELL_FLAG( skill_table[sn], SF_CANMIX ) && !SPELL_FLAG( skill_table[sn], SF_CANDOALL ) )
      {
         ch_printf( ch, "You can't mix %s.\r\n", skill_table[sn]->name );
         return;
      }

     spells++;
      mana += ( 4 * ( is_npc( ch ) ? 0 : UMAX( skill_table[sn]->min_mana, mana_cost( ch, sn ) ) ) );
   }

   if( arg2 != NULL && arg2[0] != '\0' )
   {
      if( ( sn2 = find_spell( ch, arg2, true ) ) >= 0 )
      {
         if( skill_table[sn2]->spell_fun == spell_null )
         {
            ch_printf( ch, "%s is not a spell!\r\n", skill_table[sn2]->name );
            return;
         }

         if( !SPELL_FLAG( skill_table[sn2], SF_CANMIX ) && !SPELL_FLAG( skill_table[sn2], SF_CANDOALL ) )
         {
            ch_printf( ch, "You can't mix %s.\r\n", skill_table[sn2]->name );
            return;
         }

         spells++;
         mana += ( 4 * ( is_npc( ch ) ? 0 : UMAX( skill_table[sn2]->min_mana, mana_cost( ch, sn2 ) ) ) );
      }
   }

   if( arg3 != NULL && arg3[0] != '\0' )
   {
      if( ( sn3 = find_spell( ch, arg3, true ) ) >= 0 )
      {
         if( skill_table[sn3]->spell_fun == spell_null )
         {
            ch_printf( ch, "%s is not a spell!\r\n", skill_table[sn3]->name );
            return;
         }

         if( !SPELL_FLAG( skill_table[sn3], SF_CANMIX ) && !SPELL_FLAG( skill_table[sn3], SF_CANDOALL ) )
         {
            ch_printf( ch, "You can't mix %s.\r\n", skill_table[sn3]->name );
            return;
         }

         spells++;
         mana += ( 4 * ( is_npc( ch ) ? 0 : UMAX( skill_table[sn3]->min_mana, mana_cost( ch, sn3 ) ) ) );
      }
   }

   if( !is_npc( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( !tin && obj->item_type == ITEM_SALVE
      && ( obj->value[3] == -1 || obj->value[4] == -1 || obj->value[5] == -1 )
      && obj->value[1] == -1 ) /* Using -1 for value[1] as a way to know it can be used to mix */
         tin = obj;
      if( obj->item_type == ITEM_MORTAR )
      {
         if( !mortar )
         {
            mortar = obj;
            mortarcount += ( obj->value[0] * obj->count );
         }
         else if( !mortar2 && mortarcount < spells )
         {
            mortar2 = obj;
            mortarcount += ( obj->value[0] * obj->count );
         }
         else if( !mortar3 && mortarcount < spells )
         {
            mortar3 = obj;
            mortarcount += ( obj->value[0] * obj->count );
         }
      }
      if( obj->item_type == ITEM_POWDER )
      {
         if( !powder )
         {
            powder = obj;
            powdercount += ( obj->value[0] * obj->count );
         }
         else if( !powder2 )
         {
            powder2 = obj;
            powdercount += ( obj->value[0] * obj->count );
         }
         else if( !powder3 )
         {
            powder3 = obj;
            powdercount += ( obj->value[0] * obj->count );
         }
      }
      if( tin && powder && mortar && powdercount >= spells && mortarcount >= spells )
         break;
   }

   if( !tin )
   {
      send_to_char( "You have no tin to use for mixing.\r\n", ch );
      return;
   }

   if( !powder )
   {
      send_to_char( "You have no powder to use for mixing.\r\n", ch );
      return;
   }

   if( !mortar )
   {
      send_to_char( "You have no mortar to use for mixing.\r\n", ch );
      return;
   }

   if( spells > mortarcount )
   {
      ch_printf( ch, "You need %d mortar, but you only have %d.\r\n", spells, mortarcount );
      return;
   }

   if( spells > powdercount )
   {
      ch_printf( ch, "You need %d powder, but you only have %d.\r\n", spells, powdercount );
      return;
   }

   if( tin->value[3] != -1 && tin->value[4] != -1 && tin->value[5] != -1 )
   {
      send_to_char( "You can't mix anything else in this tin.\r\n", ch );
      return;
   }

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( obj->item_type == ITEM_FIRE )
      {
         fire = obj;
         break;
      }
   }

   if( !fire )
   {
      send_to_char( "You need a fire nearby for mixing.\r\n", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_mix );
      ch->mana -= (mana / 2);
      return;
   }

   {
      while( spells-- > 0 )
      {
         if( mortar )
         {
            separate_obj( mortar );
            if( --mortar->value[0] <= 0 )
            {
               extract_obj( mortar );
               mortar = NULL;
            }
         }
         else if( mortar2 )
         {
            separate_obj( mortar2 );
            if( --mortar2->value[0] <= 0 )
            {
               extract_obj( mortar2 );
               mortar2 = NULL;
            }
         }
         else if( mortar3 )
         {
            separate_obj( mortar3 );
            if( --mortar3->value[0] <= 0 )
            {
               extract_obj( mortar3 );
               mortar3 = NULL;
            }
         }
         else
            bug( "%s: mortar, mortar2 and mortar3 are NULL.", __FUNCTION__ );

         if( powder )
         {
            separate_obj( powder );
            if( --powder->value[0] <= 0 )
            {
               extract_obj( powder );
               powder = NULL;
            }
         }
         else if( powder2 )
         {
            separate_obj( powder2 );
            if( --powder2->value[0] <= 0 )
            {
               extract_obj( powder2 );
               powder2 = NULL;
            }
         }
         else if( powder3 )
         {
            separate_obj( powder3 );
            if( --powder3->value[0] <= 0 )
            {
               extract_obj( powder3 );
               powder3 = NULL;
            }
         }
         else
            bug( "%s: powder, powder2 and powder3 are NULL.", __FUNCTION__ );
      }
   }

   if( !can_use_skill( ch, number_percent( ), gsn_mix ) )
   {
      set_char_color ( AT_MAGIC, ch );
      send_to_char("You failed.\r\n", ch);
      learn_from_failure( ch, gsn_mix );
      ch->mana -= (mana / 2);
      return;
   }

   separate_obj( tin );
   if( tin->value[3] == -1 )
      tin->value[3] = sn;

   if( sn2 != -1 && tin->value[4] == -1 )
   {
      if( number_percent( ) <= 75 )
         tin->value[4] = sn2;
      else
         ch_printf( ch, "You failed to add %s into the mix, but used up the ingredients trying.\r\n", dis_skill_name( sn2 ) );
   }

   if( sn3 != -1 && ( tin->value[5] == -1 || tin->value[4] == -1 ) )
   {
      if( number_percent( ) <= 50 )
      {
         if( tin->value[4] == -1 )
            tin->value[4] = sn3;
         else
            tin->value[5] = sn3;
      }
      else
         ch_printf( ch, "You failed to add %s into the mix, but used up the ingredients trying.\r\n", dis_skill_name( sn3 ) );
   }

   if( tin->value[3] != -1 && tin->value[4] != -1 && tin->value[5] != -1 )
   {
      sprintf( buf1, "%s, %s, %s salve",
         dis_skill_name( tin->value[3] ), dis_skill_name( tin->value[4] ), dis_skill_name( tin->value[5] ) );
      sprintf( buf2, "A strange tin labelled '%s' '%s' '%s' lays here.",
         dis_skill_name( tin->value[3] ), dis_skill_name( tin->value[4] ), dis_skill_name( tin->value[5] ) );
      sprintf( buf3, "salve %s %s %s",
         dis_skill_name( tin->value[3] ), dis_skill_name( tin->value[4] ), dis_skill_name( tin->value[5] ) );
   }
   else if( tin->value[3] != -1 && tin->value[4] != -1 )
   {
      sprintf( buf1, "%s, %s salve", dis_skill_name( tin->value[3] ), dis_skill_name( tin->value[4] ) );
      sprintf( buf2, "A strange tin labelled '%s' '%s' lays here.",
         dis_skill_name( tin->value[3] ), dis_skill_name( tin->value[4] ) );
      sprintf( buf3, "salve %s %s", dis_skill_name( tin->value[3] ), dis_skill_name( tin->value[4] ) );
   }
   else if( tin->value[3] != -1 )
   {
      sprintf( buf1, "%s salve", dis_skill_name( tin->value[3] ) );
      sprintf( buf2, "A strange tin labelled '%s' lays here.", dis_skill_name( tin->value[3] ) );
      sprintf( buf3, "salve %s", dis_skill_name( tin->value[3] ) );
   }

   tin->value[0] = UMAX( tin->value[0], ch->level );

   tin->value[2] = 1; /* delay */
   tin->value[1] = number_range( 1, 5 ); /* charges */

   STRSET( tin->short_descr, aoran( buf1 ) );
   STRSET( tin->description, buf2 );
   STRSET( tin->name, buf3 );

   act( AT_MAGIC, "$n mixes $p.", ch, tin, NULL, TO_ROOM );
   act( AT_MAGIC, "You mix $p.", ch, tin, NULL, TO_CHAR );

   learn_from_success( ch, gsn_mix );
   ch->mana -= mana;
}

bool can_practice( CHAR_DATA *ch, int sn )
{
   SKILLTYPE *skill = skill_table[sn];
   MCLASS_DATA *mclass;
   bool canpract = false;

   if( !ch || !skill || is_npc( ch ) )
      return false;

   if( is_immortal( ch ) )
      return true;

   if( ch->pcdata )
   {
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
         if( mclass->wclass >= 0 && skill->skill_level[mclass->wclass] > 0 && mclass->level >= skill->skill_level[mclass->wclass] )
            canpract = true;
   }

   if( ch->race >= 0 && ch->race < MAX_PC_RACE )
   {
      if( skill->race_level[ch->race] > 0 && ch->level >= skill->race_level[ch->race] )
         canpract = true;
   }

   if( ch->pcdata->learned[sn] > 0 ) /* Already learned it? */
      canpract = true;

   /* If it requires a skill && they haven't learned and adepted it return false */
   if( skill->req_skill != -1
   && ( ch->pcdata->learned[ skill->req_skill ] <= 0 || ch->pcdata->learned[ skill->req_skill ] < get_adept( ch, sn ) ) )
      return false;

   return canpract;
}

CMDF( do_practice )
{
   SKILLTYPE *skill;
   CHAR_DATA *mob;
   char buf[MSL];
   int sn, adept = 0, tshow = SKILL_UNKNOWN;
   bool resetcolor = true;

   if( is_npc( ch ) )
      return;

   if( !argument || argument[0] == '\0' || ( tshow = get_skill( argument ) ) != SKILL_UNKNOWN )
   {
      int col;
      short lasttype, cnt;

      col = cnt = 0;
      lasttype = SKILL_UNKNOWN;
      set_pager_color( AT_PRAC, ch );
      for( sn = 0; sn < top_sn; sn++ )
      {
         if( !skill_table[sn] || !skill_table[sn]->name )
            continue;

         skill = skill_table[sn];

         if( tshow != SKILL_UNKNOWN && skill->type != tshow )
            continue;

         if( !can_practice( ch, sn ) )
            continue;

         if( skill->type == SKILL_DELETED )
            continue;

         if( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill, SF_SECRETSKILL ) )
            continue;

         if( skill->type != lasttype )
         {
            if( lasttype != SKILL_UNKNOWN )
            {
               if( !cnt )
                  send_to_pager( "                                   (none)\r\n", ch );
               else if( col != 0 )
                  send_to_pager( "\r\n", ch );
            }
            pager_printf( ch, "&[prac] .~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~[&[prac2]%7ss&[prac]]~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.&D\r\n",
               skill_tname[skill->type] );
            col = cnt = 0;
         }
         lasttype = skill->type;

         ++cnt;
         if( resetcolor )
            send_to_pager( "&[prac]", ch );
         pager_printf( ch, "%20.20s", skill->name );
         if( ch->pcdata->learned[sn] <= 0 )
         {
            send_to_pager( "&[prac3]", ch );
            resetcolor = true;
         }
         else if( ch->pcdata->learned[sn] >= get_adept( ch, sn ) )
         {
            send_to_pager( "&[prac2]", ch );
            resetcolor = true;
         }
         pager_printf( ch, " %3d%% ", ch->pcdata->learned[sn] );
         if( ++col == 3 )
         {
            col = 0;
            send_to_pager( "\r\n", ch );
         }
      }
      if( col != 0 )
      {
         send_to_pager( "\r\n", ch );
         col = 0;
      }
      for( sn = 0; sn < MAX_PC_PERS; sn++ )
      {
         int tmpsn;

         if( is_npc( ch ) || ch->pcdata->personal[sn] == -1 )
            continue;
         tmpsn = ch->pcdata->personal[sn];
         if( !pers_table[tmpsn] || !pers_table[tmpsn]->name )
            continue;
         if( !pers_table[tmpsn]->teachers || str_cmp( pers_table[tmpsn]->teachers, ch->name ) )
            continue;
         skill = pers_table[tmpsn];
         if( tshow != SKILL_UNKNOWN && skill->type != tshow )
            continue;
         if( skill->type != lasttype )
         {
            pager_printf( ch, "&[prac] .~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~[&[prac2]%7ss&[prac]]~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.&D\r\n",
               skill_tname[skill->type] );
         }
         lasttype = skill->type;
         ++cnt;
         send_to_pager( "&[prac]", ch );
         pager_printf( ch, "%20.20s", skill->name );
         send_to_pager( "&[prac2]", ch );
         pager_printf( ch, " %3d%% ", 100 );
         if( ++col == 3 )
         {
            col = 0;
            send_to_pager( "\r\n", ch );
         }
      }
      if( col != 0 )
      {
         send_to_pager( "\r\n", ch );
         col = 0;
      }
      pager_printf( ch, "&[prac]You have &[prac2]%d &[prac]practice session%s left.\r\n", ch->practice, ch->practice == 1 ? "" : "s" );
      return;
   }

   if( !is_awake( ch ) )
   {
      send_to_char( "In your dreams, or what?\r\n", ch );
      return;
   }

   for( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
      if( is_npc( mob ) && xIS_SET( mob->act, ACT_PRACTICE ) )
         break;

   if( !mob )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( ch->practice <= 0 )
   {
      act_tell( ch, mob, "$n tells you 'You must earn some more practice sessions.'", mob, NULL, ch, TO_VICT );
      return;
   }

   if( ( sn = skill_lookup( argument ) ) < 0 )
   {
      act_tell( ch, mob, "$n tells you 'You're not ready to learn that yet...'", mob, NULL, ch, TO_VICT );
      return;
   }

   skill = skill_table[sn];

   if( !can_practice( ch, sn ) )
   {
      act_tell( ch, mob, "$n tells you 'You're not ready to learn that yet...'", mob, NULL, ch, TO_VICT );
      return;
   }

   adept = URANGE( 5, get_adept( ch, sn ), 100 );

   /* Skill requires a special teacher */
   if( skill_table[sn]->teachers )
   {
      snprintf( buf, sizeof( buf ), "%d", mob->pIndexData->vnum );
      if( !is_name( buf, skill_table[sn]->teachers ) )
      {
         act_tell( ch, mob, "$n tells you, 'I don't know how to teach that.'", mob, NULL, ch, TO_VICT );
         return;
      }
   }

   if( skill_table[sn]->type == SKILL_TONGUE )
   {
      act_tell( ch, mob, "$n tells you 'I don't know how to teach that.'", mob, NULL, ch, TO_VICT );
      return;
   }

   /* Already learned it? */
   if( ch->pcdata->learned[sn] > 0 )
   {
      act_printf( AT_TELL, mob, NULL, ch, TO_VICT, "$n tells you, 'I've taught you everything I can about %s.'", skill_table[sn]->name );
      act_tell( ch, mob, "$n tells you, 'You'll have to practice it on your own now...'", mob, NULL, ch, TO_VICT );
      return;
   }

   ch->practice--;
   ch->pcdata->learned[sn] = URANGE( 5, ( get_curr_int( ch ) / 5 ), adept );
   act( AT_ACTION, "You practice $T.", ch, NULL, skill_table[sn]->name, TO_CHAR );
   act( AT_ACTION, "$n practices $T.", ch, NULL, skill_table[sn]->name, TO_ROOM );
   act_tell( ch, mob, "$n tells you. 'You'll have to practice it on your own now...'", mob, NULL, ch, TO_VICT );
}

CMDF( do_makefire )
{
   OBJ_INDEX_DATA *findex = NULL;
   OBJ_DATA *fire = NULL, *obj, *wood = NULL;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You're to busy at this moment.\r\n", ch );
      return;
   }
   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_WATER_NOSWIM 
   || ch->in_room->sector_type == SECT_UNDERWATER || ch->in_room->sector_type == SECT_OCEANFLOOR )
   {
      send_to_char( "You can't create a fire here.\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) )
   {
      send_to_char( "A magical force prevents you from creating that here.\r\n", ch );
      return;
   }

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc != WEAR_NONE )
         continue;
      if( obj->item_type == ITEM_WOOD )
      {
         wood = obj;
         break;
      }
   }
   if( !wood )
   {
      send_to_char( "You have no wood to use.\r\n", ch );
      return;
   }

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == OBJ_VNUM_WOODFIRE
      || obj->pIndexData->vnum == OBJ_VNUM_FIRE )
      {
         fire = obj;
         break;
      }
   }

   separate_obj( wood );
   if( fire )
   {
      act( AT_MAGIC, "$n tosses another peice of wood on the fire!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You toss another peice of wood on the fire!", ch, NULL, NULL, TO_CHAR );
      fire->timer = UMAX( fire->timer, ( fire->timer + wood->value[0] ) );
   }
   else
   {
      if( !can_use_skill( ch, number_percent( ), gsn_makefire ) )
      {
         act( AT_ACTION, "You use up the wood, but fail to make a fire.", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n uses up the wood, but fails to make a fire.", ch, NULL, NULL, TO_ROOM );
         learn_from_failure( ch, gsn_makefire );
         extract_obj( wood );
         return;
      }

      if( !( findex = get_obj_index( OBJ_VNUM_WOODFIRE ) ) )
      {
         send_to_char( "Something happened in makefire and the fire couldn't be created.\r\n", ch );
         bug( "%s: Object vnum %d couldn't be found", __FUNCTION__, OBJ_VNUM_WOODFIRE );
         return;
      }

      if( !( fire = create_object( findex, 0 ) ) )
      {
         send_to_char( "Something happened in makefire and the fire couldn't be created.\r\n", ch );
         bug( "%s: Object vnum %d couldn't be created", __FUNCTION__, OBJ_VNUM_WOODFIRE );
         return;
      }
      learn_from_success( ch, gsn_makefire );
      fire->timer = wood->value[0];
      act( AT_MAGIC, "$n uses a peice of wood to start a fire.", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You use a peice of wood to start a fire.", ch, NULL, NULL, TO_CHAR );
      obj_to_room( fire, ch->in_room );
   }
   extract_obj( wood );
}

CMDF( do_chop )
{
   OBJ_INDEX_DATA *windex = NULL;
   OBJ_DATA *obj, *axe;
   int moveloss = 10;

   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You're to busy at this moment.\r\n", ch );
      return;
   }
   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }
   if( !( axe = get_eq_hold( ch, ITEM_AXE ) ) )
   {
      send_to_char( "You aren't holding an axe!\r\n", ch );
      return;
   }
   if( ch->in_room->sector_type != SECT_FOREST )
   {
      send_to_char( "You aren't in a forest!\r\n", ch );
      return;
   }
   if( ch->move < moveloss )
   {
      send_to_char( "You're to exhausted to chop wood.\r\n", ch );
      return;
   }
   separate_obj( axe );
   if( axe->value[0] <= 0 )
   {
      send_to_char( "The axe is to dull to cut.\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         add_timer( ch, TIMER_DO_FUN, number_range( 2, 4 ), do_chop, 1 );
         act( AT_ACTION, "You begin chopping some wood...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n begins chopping some wood...", ch, NULL, NULL, TO_ROOM );
         return;

      case 1:
         ch->substate = SUB_NONE;
         ch->move -= moveloss;
         axe->value[0]--;
         if( !can_use_skill( ch, number_percent( ), gsn_chop ) )
         {
            act( AT_ACTION, "You slip and hit yourself with the axe.", ch, NULL, NULL, TO_CHAR );
            act( AT_ACTION, "$n slips and hits $mself with the axe.", ch, NULL, NULL, TO_ROOM );
            learn_from_failure( ch, gsn_chop );
            damage( ch, ch, NULL, number_range( 1, ch->level ), gsn_chop, true );
            return;
         }
         if( !( windex = get_obj_index( OBJ_VNUM_WOOD ) ) )
         {
            send_to_char( "There was an issue in chopping, so no wood was created.\r\n", ch );
            bug( "%s: couldn't find object vnum %d.", __FUNCTION__, OBJ_VNUM_WOOD );
            return;
         }
         if( !( obj = create_object( windex, 0 ) ) )
         {
            send_to_char( "There was an issue in chopping, so no wood was created.\r\n", ch );
            bug( "%s: couldn't create an object using vnum %d.", __FUNCTION__, OBJ_VNUM_WOOD );
            return;
         }
         act( AT_ACTION, "You finish chopping wood.", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n finishes chopping wood.", ch, NULL, NULL, TO_ROOM );
         learn_from_success( ch, gsn_chop );
         obj->value[0] = number_range( 5, 50 );
         obj_to_room( obj, ch->in_room );
         return;

      case SUB_TIMER_DO_ABORT:
         ch->substate = SUB_NONE;
         act( AT_ACTION, "You stop chopping wood...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n stops chopping wood...", ch, NULL, NULL, TO_ROOM );
         return;
   }
}

/*
 * Lets counter an attack - Remcon
 * Remember the victim is the actual one doing the countering here
 * It's the characters attacks that are being countered
 */
bool check_counter( CHAR_DATA *ch, CHAR_DATA *victim, int dam )
{
   int chances;
   int chance2 = number_range( 0, 100 );
   int chance3 = number_range( 0, 100 );
   int chance4 = number_range( 0, 100 );

   if( dam <= 0 )
      return false;
   if( !is_awake( victim ) )
      return false;
   if( ch->position == POS_DEAD || victim->in_room != ch->in_room || ch->hit < 0 )
      return false;
   if( !is_npc( victim ) && LEARNED( victim, gsn_counter ) == 0 )
      return false;
   if( chance3 < 50 || chance4 > 50 )
      return false;
   if( is_npc( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = (int) ( LEARNED( victim, gsn_counter ) );
   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_counter );
      return false;
   }
   if( chance2 > 75 )
      dam = dam;
   else if( chance2 > 50 && chance2 < 75 && dam > 0 )
      dam = (dam / 2);
   else if( chance2 > 25 && chance2 < 50 && dam > 0 )
      dam = (dam / 3);
   else
      dam = 0;
   if( dam == 0 )
   {
      learn_from_failure( victim, gsn_counter );
      return false;
   }
   if( ( ch->hit - dam ) > 0 )
   {
      if( !is_npc( victim ) )
         act_printf( AT_HIT, ch, NULL, victim, TO_VICT, "You counter $n's attack. &C[&B%d&C]&D", dam );
      else
         act( AT_HIT, "You counter $n's attack.&D", ch, NULL, victim, TO_VICT );
      if( !is_npc( ch ) )
         act_printf( AT_HITME, ch, NULL, victim, TO_CHAR, "$N counters your attack. &R[&B%d&R]&D", dam );
      else
         act( AT_HITME, "$N counters your attack.&D", ch, NULL, victim, TO_CHAR );
   }
   if( ( ch->hit - dam ) <= 0 )
   {
      learn_from_failure( victim, gsn_counter );
      return false;
   }
   ch->hit -= dam;
   learn_from_success( victim, gsn_counter );
   return true;
}

/* Remember the victim is the one using a shield to block */
bool check_shieldblock( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chances;

   if( !is_awake( victim ) )
      return false;

   /* Armor held in hands is likely a shield no...? */
   if( !( get_eq_hold( victim, ITEM_ARMOR ) ) )
      return false;

   if( is_npc( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = (int) ( LEARNED( victim, gsn_shieldblock ) );

   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_shieldblock );
      return false;
   }

   learn_from_success( victim, gsn_shieldblock );
   return true;
}

CMDF( do_hide )
{
   OBJ_DATA *obj, *container = NULL;
   AFFECT_DATA af;
   char arg1[MIL];

   if( !ch )
      return;
   if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      if( ch->mount )
      {
         send_to_char( "You can't hide while mounted.\r\n", ch );
         return;
      }

      if( IS_AFFECTED( ch, AFF_HIDE ) )
      {
         send_to_char( "You're already hidden.\r\n", ch );
         return;
      }

      if( can_use_skill( ch, number_percent( ), gsn_hide ) )
      {
         af.type = gsn_hide;
         af.location = APPLY_EXT_AFFECT;
         af.modifier = AFF_HIDE;
         af.duration = ( ch->level * 23 );
         af.bitvector = meb( AFF_HIDE );
         affect_to_char( ch, &af );
         learn_from_success( ch, gsn_hide );
         send_to_char( "You hide.\r\n", ch );
      }
      else
      {
         learn_from_failure( ch, gsn_hide );
         send_to_char( "You attempt to hide.\r\n", ch );
      }
   }
   else
   {
      argument = one_argument( argument, arg1 );
      if( !( obj = get_obj_carry( ch, arg1 ) ) )
      {
         send_to_char( "You don't have that item.\r\n", ch );
         return;
      }

      if( !can_drop_room( ch ) )
         return;

      separate_obj( obj );
      if( !can_drop_obj( ch, obj ) )
      {
         send_to_char( "You can't let go of it.\r\n", ch );
         return;
      }
      if( argument && argument[0] != '\0' && !( container = get_obj_here( ch, argument ) ) )
      {
         send_to_char( "You can't find that to hide it in.\r\n", ch );
         return;
      }
      if( container )
      {
         separate_obj( container );
         if( container->item_type != ITEM_CONTAINER )
         {
            send_to_char( "That isn't a container.\r\n", ch );
            return;
         }
         if( container == obj )
         {
            send_to_char( "You can't hide something in itself.\r\n", ch );
            return;
         }

         if( ( get_real_obj_weight( obj ) / obj->count ) + ( get_real_obj_weight( container ) / container->count ) > container->value[0] )
         {
            send_to_char( "It won't fit.\r\n", ch );
            return;
         }
      }
      if( can_use_skill( ch, number_percent( ), gsn_hide ) )
      {
         obj_from_char( obj );
         if( !container )
         {
            obj_to_room( obj, ch->in_room );
            ch_printf( ch, "You hide [%s] in the room.\r\n", obj->name );
         }
         else
         {
            obj_to_obj( obj, container );
            ch_printf( ch, "You hide [%s] in [%s].\r\n", obj->name, container->name );
         }
         xSET_BIT( obj->extra_flags, ITEM_HIDDEN );
         learn_from_success( ch, gsn_hide );
      }
      else
      {
         learn_from_failure( ch, gsn_hide );
         ch_printf( ch, "You fail to hide [%s] in the room.\r\n", obj->name );
      }
   }
   save_char_obj( ch );
}

CMDF( do_throw )
{
   OBJ_DATA *tobj;
   char arg[MIL];
   short max_dist = 3;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Throw what at who?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
   {
      send_to_char( "&[magic]You can't throw anything in the wilderness.\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      send_to_char( "&[magic]A magical force prevents you from attacking.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   for( tobj = ch->last_carrying; tobj; tobj = tobj->prev_content )
   {
      if( tobj->wear_loc != WEAR_HOLD_B && tobj->wear_loc != WEAR_HOLD_L && tobj->wear_loc != WEAR_HOLD_R )
         continue;
      if( can_see_obj( ch, tobj ) && nifty_is_name( arg, tobj->name ) )
        break;
   }

   if( !tobj )
   {
      send_to_char( "You aren't holding anything like that.\r\n", ch );
      return;
   }

   if( tobj->item_type != ITEM_WEAPON )
   {
      send_to_char( "You can only throw weapons.\r\n", ch );
      return;
   }

   wait_state( ch, 6 );

   ranged_attack( ch, argument, NULL, tobj, TYPE_HIT + tobj->value[3], max_dist );
}

void depierce_equipment( CHAR_DATA *ch, int iWear )
{
   OBJ_DATA *obj;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
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
         if( is_obj_stat( obj, ITEM_PIERCED ) )
            xREMOVE_BIT( obj->extra_flags, ITEM_PIERCED );
      }
   }
}

CMDF( do_dislodge )
{
   OBJ_DATA * arrow = NULL;
   int dam = 0;

   if( !ch )
      return;
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Dislodge what?\r\n", ch );
      return;
   }

   if( !( arrow = get_obj_wear( ch, argument ) ) || !is_obj_stat( arrow, ITEM_LODGED ) )
   {
      send_to_char( "You don't have anything like that lodged in your body.\r\n", ch );
      return;
   }

   act( AT_CARNAGE, "With a wrenching pull, you dislodge $p.", ch, arrow, NULL, TO_CHAR );
   act( AT_CARNAGE, "$n winces in pain as $e dislodges $p.", ch, arrow, NULL, TO_ROOM );
   depierce_equipment( ch, arrow->wear_loc );
   xREMOVE_BIT( arrow->extra_flags, ITEM_LODGED);
   dam = number_range( ( 3 * arrow->value[1] ), ( 3 * arrow->value[2] ) );
   unequip_char( ch, arrow );
   extract_obj( arrow );
   damage( ch, ch, NULL, dam, TYPE_UNDEFINED, true );
}

SKILLTYPE *new_skill( void )
{
   SKILLTYPE *skill = NULL;
   int i, stat;

   CREATE( skill, SKILLTYPE, 1 );
   if( !skill )
   {
      bug( "%s: skill is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }

   skill->first_affect = skill->last_affect = NULL;
   skill->hit_char = NULL;
   skill->hit_vict = NULL;
   skill->hit_room = NULL;
   skill->hit_dest = NULL;
   skill->miss_char = NULL;
   skill->miss_vict = NULL;
   skill->miss_room = NULL;
   skill->die_char = NULL;
   skill->die_vict = NULL;
   skill->die_room = NULL;
   skill->imm_char = NULL;
   skill->imm_vict = NULL;
   skill->imm_room = NULL;
   skill->abs_char = NULL;
   skill->abs_vict = NULL;
   skill->abs_room = NULL;
   skill->magical = true;
   skill->htext = NULL;
   skill->dice = NULL;
   skill->components = NULL;
   skill->name = NULL;
   skill->teachers = NULL;
   skill->noun_damage = NULL;
   skill->msg_off = NULL;
   skill->skill_fun = NULL;
   skill->skill_fun_name = NULL;
   skill->spell_fun = NULL;
   skill->spell_fun_name = NULL;
   skill->slot = -1;
   skill->req_skill = -1;
   skill->reqskillname = NULL;
   skill->min_mana = 0;
   skill->tmpspell = false;
   skill->type = SKILL_UNKNOWN;
   skill->target = TAR_IGNORE;
   xCLEAR_BITS( skill->spell_sector );
   for( i = 0; i < MAX_CLASS; i++ )
   {
      skill->skill_level[i] = -1;
      skill->skill_adept[i] = 95;
   }
   for( i = 0; i < MAX_RACE; i++ )
   {
      skill->race_level[i] = -1;
      skill->race_adept[i] = 95;
   }
   for( stat = 0; stat < STAT_MAX; stat++ )
      skill->stats[stat] = 0;
   return skill;
}

CMDF( do_personal )
{
   SKILLTYPE *skill = NULL;
   char arg[MSL];
   int max, x;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: personal create spell/skill offensive/defensive <type> <name>\r\n", ch );
      send_to_char( "Usage: personal <name> delete\r\n", ch );
      send_to_char( "Offensive Types: damage <DTYPE>, <RIS>\r\n", ch );
      send_to_char( "Defensive Types: <RIS>\r\n", ch );
      send_to_char( "  RIS:   ice  fire  blunt   earth        water       poison\r\n", ch );
      send_to_char( "        acid  holy  charm   slash       energy       shadow\r\n", ch );
      send_to_char( "        cold  wind  drain   sleep       pierce  electricity\r\n", ch );
      send_to_char( "DTYPE:   ice  fire   wind   water       shadow\r\n", ch );
      send_to_char( "        acid  holy  drain  energy  electricity\r\n", ch );
      send_to_char( "        cold  none  earth  poison\r\n", ch );

      return;
   }

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "create" ) )
   {
      int pccheck = 0, ristype = -1;
      bool sused = false, sspell = false, soffensive = false, sdamage = false, setmess = false;

      if( top_pers >= MAX_PERS )
      {
         send_to_char( "The max personal skills have already been created and no more can be created currently.\r\n", ch );
         return;
      }

      /* Check to see if they have already named one this */
      for( x = 0; x < MAX_PC_PERS; x++ )
      {
         if( ch->pcdata->personal[x] == -1 )
            continue;
         pccheck++;
         if( !str_cmp( pers_table[ch->pcdata->personal[x]]->name, argument ) )
         {
            send_to_char( "You already have a personal skill by that name.\r\n", ch );
            return;
         }
      }

      if( pccheck >= MAX_PC_PERS )
      {
         send_to_char( "You already have all the personal spells/skills you can have.\r\n", ch );
         return;
      }

      argument = one_argument( argument, arg );
      if( !str_cmp( arg, "skill" ) )
         sspell = false;
      else if( !str_cmp( arg, "spell" ) )
         sspell = true;
      else
      {
         send_to_char( "You can only create a skill or spell.\r\n", ch );
         return;
      }

      argument = one_argument( argument, arg );
      if( !str_cmp( arg, "offensive" ) )
      {
         soffensive = true;
         argument = one_argument( argument, arg );

         if( !str_cmp( arg, "damage" ) )
         {
            argument = one_argument( argument, arg );
            ristype = get_flag( arg, spell_damage, SD_MAX );
            sdamage = true;
            if( ristype < 0 || ristype >= SD_MAX )
            {
               send_to_char( "Invalid SDTYPE.\r\n", ch );
               return;
            }
         }
         else
         {
            ristype = get_flag( arg, ris_flags, RIS_MAX );
            if( ristype < 0 || ristype >= RIS_MAX || ristype == RIS_NONMAGIC || ristype == RIS_MAGIC || ristype == RIS_PARALYSIS )
            {
               send_to_char( "Invalid RIS.\r\n", ch );
               return;
            }
            setmess = true;
         }
      }
      else if( !str_cmp( arg, "defensive" ) )
      {
         soffensive = false;
         setmess = true;
         argument = one_argument( argument, arg );
         ristype = get_flag( arg, ris_flags, RIS_MAX );
         if( ristype < 0 || ristype >= RIS_MAX || ristype == RIS_NONMAGIC || ristype == RIS_MAGIC || ristype == RIS_PARALYSIS )
         {
            send_to_char( "Invalid RIS.\r\n", ch );
            return;
         }
      }
      else
      {
         send_to_char( "You can only create an offensive or defensive one.\r\n", ch );
         return;
      }

      if( !can_use_skill_name( argument ) )
      {
         send_to_char( "There is already a skill/spell/weapon/tongue/herb/personal using that name.\r\n", ch );
         return;
      }

      /* Insert it where you can */
      for( x = 0; x < MAX_PC_PERS; x++ )
      {
         if( ch->pcdata->personal[x] != -1 )
            continue;
         ch->pcdata->personal[x] = top_pers;
         sused = true;
         break;
      }

      if( !sused )
      {
         send_to_char( "You already have all the personal spells/skills you can have.\r\n", ch );
         return;
      }

      if( !( skill = new_skill( ) ) )
      {
         send_to_char( "Failed to create a new personal spell/skill for you.\r\n", ch );
         bug( "%s: skill is NULL after new_skill.", __FUNCTION__ );
         ch->pcdata->personal[x] = -1;
         return;
      }

      pers_table[top_pers++] = skill;
      for( max = x = 0; x < top_pers - 1; x++ )
         if( pers_table[x] && pers_table[x]->slot > max )
            max = pers_table[x]->slot;
      skill->slot = max + 1;
      skill->min_mana = UMAX( 25, ( MAX_LEVEL - ch->level ) ); /* A good reason to spend gold to redo it at higher levels */
      /* Another good reason */
      {
         int bdiff = ( MAX_LEVEL - ch->level );

         if( bdiff > 0 )
            bdiff /= 10;
         skill->beats = UMAX( 4, bdiff );
      }
      skill->name = STRALLOC( argument );
      skill->teachers = STRALLOC( ch->name );
      skill->spell_fun = spell_smaug;
      STRSET( skill->spell_fun_name, "spell_smaug" );
      if( setmess )
      {
         char smessage[MSL];

         if( sspell )
         {
            snprintf( smessage, sizeof( smessage ), "You cast %s on $N.", skill->name );
            STRSET( skill->hit_char, smessage );
            snprintf( smessage, sizeof( smessage ), "$n cast %s on you.", skill->name );
            STRSET( skill->hit_vict, smessage );
         }
         else
         {
            snprintf( smessage, sizeof( smessage ), "You %s $N.", skill->name );
            STRSET( skill->hit_char, smessage );
            snprintf( smessage, sizeof( smessage ), "$n %s you.", skill->name );
            STRSET( skill->hit_vict, smessage );
         }
      }
      skill->type = SKILL_PERSONAL;
      skill->tmpspell = sspell;
      if( soffensive )
      {
         char modifier[MSL];

         skill->target = TAR_CHAR_OFFENSIVE;
         if( sdamage )
         {
            SET_SACT( skill, SA_CREATE );
            SET_SCLA( skill, SC_DEATH );
            SET_SDAM( skill, ristype );
            if( sspell )
            {
               snprintf( modifier, sizeof( modifier ), "%d", ( ch->level + get_curr_int( ch ) ) );
               STRSET( skill->dice, modifier );
            }
            else
            {
               snprintf( modifier, sizeof( modifier ), "%d", ( ch->level + get_curr_str( ch ) ) );
               STRSET( skill->dice, modifier );
            }
            STRSET( skill->noun_damage, skill->name );
         }
         else
         {
            SMAUG_AFF *aff;

            CREATE( aff, SMAUG_AFF, 1 );
            if( !aff )
            {
               bug( "%s: couldn't create an aff.\r\n", __FUNCTION__ );
               return;
            }
            aff->location = APPLY_RESISTANT;
            snprintf( modifier, sizeof( modifier ), "%d", ( ch->level + get_curr_wis( ch ) ) );
            aff->duration = STRALLOC( modifier );
            snprintf( modifier, sizeof( modifier ), "%d", URANGE( -1, -ch->level, -10 ) );
            aff->modifier = STRALLOC( modifier );
            aff->bitvector = ristype;
            LINK( aff, skill->first_affect, skill->last_affect, next, prev );
         }
      }
      else
      {
         SMAUG_AFF *aff;
         char modifier[MSL];

         CREATE( aff, SMAUG_AFF, 1 );
         if( !aff )
         {
            bug( "%s: couldn't create an aff.\r\n", __FUNCTION__ );
            return;
         }
         aff->location = APPLY_RESISTANT;
         snprintf( modifier, sizeof( modifier ), "%d", ( ch->level + get_curr_wis( ch ) ) );
         aff->duration = STRALLOC( modifier );
         snprintf( modifier, sizeof( modifier ), "%d", URANGE( 1, ch->level, 10 ) );
         aff->modifier = STRALLOC( modifier );
         aff->bitvector = ristype;
         LINK( aff, skill->first_affect, skill->last_affect, next, prev );

         skill->target = TAR_CHAR_DEFENSIVE;
      }
      save_pers_table( false );
      ch_printf( ch, "Created %s.\r\n", skill->name );
      return;
   }

   if( ( x = ch_pers_lookup( ch->name, arg ) ) >= 0 )
      skill = pers_table[x];
   else
      skill = NULL;
   if( !skill )
   {
      send_to_char( "No such personal to modify.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "delete" ) )
   {
      for( max = 0; max < MAX_PC_PERS; max++ )
         if( ch->pcdata->personal[max] == x )
            ch->pcdata->personal[max] = -1;
      skill->type = SKILL_DELETED;
      STRFREE( skill->teachers );
      save_pers_table( false );
      ch_printf( ch, "%s has been set to be deleted.\r\n", skill->name );
      return;
   }

   do_personal( ch, (char *)"" );
}

/* Designed to auto save some things */
void asskills( short type )
{
   if( type == 0 || type == 1 )
   {
      save_skill_table( true );
      save_classes( );
      save_races( ); 
   }

   if( type == 0 || type == 2 )
      save_herb_table( true );

   if( type == 0 || type == 3 )
      save_pers_table( true );
}

CMDF( do_skilltable )
{
   SKILLTYPE *skill;
   int sn, cnt = 0;

   set_pager_color( AT_IMMORT, ch );
   send_to_char( "Skills and Number of Uses This Run:\r\n", ch );
   set_pager_color( AT_PLAIN, ch );

   for( sn = 0; sn < top_sn; sn++ )
   {
      if( !( skill = skill_table[sn] ) || !skill->name )
         continue;

      if( !skill->userec.num_uses )
         continue;

      pager_printf( ch, "%-10.10s %4d ", skill->name, skill->userec.num_uses );
      if( ++cnt == 4 )
      {
         send_to_pager( "\r\n", ch );
         cnt = 0;
      }
   }
   if( cnt != 0 )
      send_to_char( "\r\n", ch );
}

CMDF( do_taste )
{
   OBJ_DATA *obj;

   if( !can_use_skill( ch, 0, gsn_taste ) )
   {
      send_to_char( "You don't know of this skill.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Taste what?\r\n", ch );
      return;
   }
   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that!\r\n", ch );
      return;
   }

   set_char_color( AT_MAGIC, ch );

   if( obj->item_type != ITEM_DRINK_CON && obj->item_type != ITEM_FOOD
   && obj->item_type != ITEM_COOK && obj->item_type != ITEM_FISH )
   {
      send_to_char( "You can't taste that.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_taste]->beats );

   if( !can_use_skill( ch, number_percent( ), gsn_taste ) )
   {
      send_to_char( "You couldn't tell much from that taste.\r\n", ch );
      learn_from_failure( ch, gsn_taste );
      return;
   }

   if( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
      send_to_char( "It taste a little undercooked.\r\n", ch );
   else if( obj->value[3] > 0 )
      send_to_char( "It taste a little poisonous.\r\n", ch );
   else
      send_to_char( "It taste great.\r\n", ch );

   learn_from_success( ch, gsn_taste );
}

CMDF( do_smell )
{
   OBJ_DATA *obj;

   if( !can_use_skill( ch, 0, gsn_smell ) )
   {
      send_to_char( "You don't know of this skill.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Smell what?\r\n", ch );
      return;
   }

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that!\r\n", ch );
      return;
   }

   set_char_color( AT_MAGIC, ch );

   if( obj->item_type != ITEM_DRINK_CON && obj->item_type != ITEM_FOOD
   && obj->item_type != ITEM_COOK && obj->item_type != ITEM_FISH )
   {
      send_to_char( "You can't smell that.\r\n", ch );
      return;
   }

   wait_state( ch, skill_table[gsn_smell]->beats );

   if( !can_use_skill( ch, number_percent( ), gsn_smell ) )
   {
      send_to_char( "You couldn't tell much from that sniff.\r\n", ch );
      learn_from_failure( ch, gsn_smell );
      return;
   }

   if( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
      send_to_char( "It smells a little bloody.\r\n", ch );
   else if( obj->value[3] > 0 )
      send_to_char( "It smells acidic.\r\n", ch );
   else
      send_to_char( "It smells great.\r\n", ch );

   learn_from_success( ch, gsn_smell );
}
