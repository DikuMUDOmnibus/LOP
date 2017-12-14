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
 *		        Main structure manipulation module		     *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "h/mud.h"

CHAR_DATA *cur_char;
bool cur_char_died;
ch_ret global_retcode;

void remove_file( const char *filename )
{
   unlink( filename );
}

bool valid_pfile( const char *filename )
{
   char buf[MSL];
   struct stat fst;

   if( !filename || filename[0] == '\0' )
      return false;

   snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( filename[0] ), capitalize( filename ) );
   if( stat( buf, &fst ) == -1 || !check_parse_name( capitalize( filename ), false ) )
      return false;
   return true;
}

char *shorttime( time_t updated )
{
   static char buf[MIL];
   struct tm *time;

   if( !updated )
      return (char *)"";
   time = localtime( &updated );
   snprintf( buf, sizeof( buf ), "%2d/%2d/%4.4d", ( time->tm_mon + 1 ), time->tm_mday, ( time->tm_year + 1900 ) );
   return buf;
}

char *distime( time_t updated )
{
   char wday_name[7][4] =
   {
      "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
   };
   char month_name[12][4] =
   {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
   };
   static char buf[MIL];
   struct tm *time;
   bool am = true;
   int hour;

   if( !updated )
      return (char *)"";
   time = localtime( &updated );
   hour = time->tm_hour;
   if( hour >= 12 )
   {
      am = false;
      hour -= 12;
   }
   if( hour == 0 )
      hour = 12;
   snprintf( buf, sizeof( buf ), "%3.3s %3.3s %2d %2d:%.2d:%.2d%2.2s %4.4d",
      wday_name[time->tm_wday], month_name[time->tm_mon], time->tm_mday,
      hour, time->tm_min, time->tm_sec,
      am ? "AM" : "PM",
      ( time->tm_year + 1900 ) );
   return buf;
}

void explore_room( CHAR_DATA *ch, ROOM_INDEX_DATA *room )
{
   EXP_DATA *fexp;
   int exp, cnt = 0;

   if( !ch || !ch->pcdata || is_npc( ch ) || !room || !xIS_SET( room->room_flags, ROOM_EXPLORER ) )
      return;

   for( fexp = ch->pcdata->first_explored; fexp; fexp = fexp->next )
   {
      if( fexp->vnum == room->vnum )
         return;
      cnt++; /* The more of them they find the more exp they gain */
   }

   CREATE( fexp, EXP_DATA, 1 );
   if( !fexp )
   {
      bug( "%s: fexp is NULL after CREATE.", __FUNCTION__ );
      return;
   }
   fexp->vnum = room->vnum;
   LINK( fexp, ch->pcdata->first_explored, ch->pcdata->last_explored, next, prev );

   exp = ( 100 * cnt );
   gain_exp( ch, exp );
}

/* Retrieve a character's permission for checking. */
short get_trust( CHAR_DATA *ch )
{
   if( !ch )
      return 0;
   return ch->trust;
}

/* Retrieve a character's age. */
short get_age( CHAR_DATA *ch )
{
   if( is_npc( ch ) )
      return 0;
   if( time_info.month > ch->pcdata->birth_month )
      return ( time_info.year - ch->pcdata->birth_year );
   if( time_info.month == ch->pcdata->birth_month && time_info.day >= ch->pcdata->birth_day )
      return ( time_info.year - ch->pcdata->birth_year );
   return ( ( time_info.year - 1 ) - ch->pcdata->birth_year );
}

/* Retrieve what year someones birth_year should be set to for changing their age. */
short get_birth_year( CHAR_DATA *ch, short age )
{
   if( is_npc( ch ) )
      return 0;
   if( age < 18 )
      return ( ch->pcdata->birth_year );
   if( time_info.month > ch->pcdata->birth_month )
      return ( time_info.year - age );
   if( time_info.month == ch->pcdata->birth_month && time_info.day >= ch->pcdata->birth_day )
      return ( time_info.year - age );
   return ( ( time_info.year - 1 ) - age );
}


/* Update characters maxes */
void update_maxes( CHAR_DATA *ch )
{
   /* Start again with race's base hit */
   ch->max_hit = race_table[ch->race]->hit;
   /* Toss in affect from constitution */
   ch->max_hit += get_curr_con( ch );

   /* Start again with race's base move */
   ch->max_move = race_table[ch->race]->move;
   /* Toss in affect from constitution and dexterity */
   ch->max_move += ( ( get_curr_con( ch ) + get_curr_dex( ch ) ) / 4 );

   /* Start again with race's base mana */
   ch->max_mana = race_table[ch->race]->mana;
   /* Toss in affect from intelligence and wisdom */
   ch->max_mana += ( ( ( get_curr_int( ch ) * 2 ) + get_curr_wis( ch ) ) / 8 );
}

short get_perm_stat( int stat, CHAR_DATA *ch )
{
   if( stat < 0 || stat >= STAT_MAX )
      return 0;
   return ch->perm_stats[stat];
}

short get_mod_stat( int stat, CHAR_DATA *ch )
{
   if( stat < 0 || stat >= STAT_MAX )
      return 0;
   return ch->mod_stats[stat];
}

short get_curr_stat( int stat, CHAR_DATA *ch )
{
   OBJ_DATA *obj;
   short eqcheck, cha;

   if( stat < 0 || stat >= STAT_MAX )
      return 0;
   if( stat == STAT_CHA )
   {
      cha = ( ch->perm_stats[stat] + ch->mod_stats[stat] );

      /* Decrease based on blood on character */
      cha = UMAX( 0, ( cha - ch->bsplatter ) );

      /* Decrease for no/bloody/stained equipment on locations */
      for( eqcheck = 0; eqcheck < WEAR_MAX; eqcheck++ )
      {
         if( !( obj = get_eq_location( ch, eqcheck ) ) ) /* Decrease 1 if no eq on the location */
         {
            /* Limit the locations we check to see if they are naked */
            if( eqcheck != WEAR_BODY && eqcheck != WEAR_LEGS )
               continue;

            cha = UMAX( 0, ( cha - 1 ) );
         }
         else /* Decrease if blood/stain on any worn object, the more blood/stains the uglier they are */
            cha = UMAX( 0, ( cha - ( obj->bsplatter + obj->bstain ) ) );
      }

      return cha;
   }
   return ( ch->perm_stats[stat] + ch->mod_stats[stat] );
}

/* Retrieve character's current strength. */
short get_curr_str( CHAR_DATA *ch )
{
   return get_curr_stat( STAT_STR, ch );
}

/* Retrieve character's current intelligence. */
short get_curr_int( CHAR_DATA *ch )
{
   return get_curr_stat( STAT_INT, ch );
}

/* Retrieve character's current wisdom. */
short get_curr_wis( CHAR_DATA *ch )
{
   return get_curr_stat( STAT_WIS, ch );
}

/* Retrieve character's current dexterity. */
short get_curr_dex( CHAR_DATA *ch )
{
   return get_curr_stat( STAT_DEX, ch );
}

/* Retrieve character's current constitution. */
short get_curr_con( CHAR_DATA *ch )
{
   return get_curr_stat( STAT_CON, ch );
}

/* Retrieve character's current charisma. */
short get_curr_cha( CHAR_DATA *ch )
{
   return get_curr_stat( STAT_CHA, ch );
}

/* Retrieve character's current luck. */
short get_curr_lck( CHAR_DATA *ch )
{
   return get_curr_stat( STAT_LCK, ch );
}

/* Retrieve the number of items a character can carry */
int can_carry_n( CHAR_DATA *ch )
{
   int penalty = 0;

   if( !is_npc( ch ) && get_trust( ch ) >= PERM_IMM )
      return get_trust( ch ) * 200;

   if( is_npc( ch ) && xIS_SET( ch->act, ACT_IMMORTAL ) )
      return ch->level * 200;

   if( get_eq_char( ch, WEAR_HOLD_B ) )
      ++penalty;
   if( get_eq_char( ch, WEAR_HOLD_L ) )
      ++penalty;
   if( get_eq_char( ch, WEAR_HOLD_R ) )
      ++penalty;
   return URANGE( 5, ( ch->level + 15 ) / 5 + get_curr_dex( ch ) - 13 - penalty, 20 );
}

/* Retrieve a character's carry capacity. */
int can_carry_w( CHAR_DATA *ch )
{
   int carry;

   if( !is_npc( ch ) && get_trust( ch ) >= PERM_IMM )
      return 1000000;

   if( is_npc( ch ) && xIS_SET( ch->act, ACT_IMMORTAL ) )
      return 1000000;

   carry = get_curr_str( ch );
   return URANGE( carry, carry * ch->level, 1000000 );
}

/* See if a player/mob can take a piece of prototype eq - Thoric */
bool can_take_proto( CHAR_DATA *ch )
{
   if( is_immortal( ch ) )
      return true;
   else if( is_npc( ch ) && xIS_SET( ch->act, ACT_PROTOTYPE ) )
      return true;
   else
      return false;
}

/* See if a string is one of the names of an object. */
bool is_name( const char *str, char *namelist )
{
   char name[MIL];

   while( namelist && namelist[0] != '\0' )
   {
      namelist = one_argument( namelist, name );
      if( !str_cmp( str, name ) )
         return true;
   }
   return false;
}

bool is_name_prefix( const char *str, char *namelist )
{
   char name[MIL];

   while( namelist && namelist[0] != '\0' )
   {
      namelist = one_argument( namelist, name );
      if( !str_prefix( str, name ) )
         return true;
   }
   return false;
}

/*
 * See if a string is one of the names of an object. - Thoric
 * Treats a dash as a word delimiter as well as a space
 */
bool is_name2( const char *str, char *namelist )
{
   char name[MIL];

   while( namelist && namelist[0] != '\0' )
   {
      namelist = one_argument2( namelist, name );
      if( !str_cmp( str, name ) )
         return true;
   }
   return false;
}

bool is_name2_prefix( const char *str, char *namelist )
{
   char name[MIL];


   while( namelist && namelist[0] != '\0' )
   {
      namelist = one_argument2( namelist, name );
      if( !str_prefix( str, name ) )
         return true;
   }
   return false;
}

/* Checks if str is a name in namelist supporting multiple keywords - Thoric */
bool nifty_is_name( char *str, char *namelist )
{
   char name[MIL];

   while( str && str[0] != '\0' )
   {
      str = one_argument2( str, name );
      if( !is_name2( name, namelist ) )
         return false;
   }
   return true;
}

bool nifty_is_name_prefix( char *str, char *namelist )
{
   char name[MIL];

   while( str && str[0] != '\0' )
   {
      str = one_argument2( str, name );
      if( !is_name2_prefix( name, namelist ) )
         return false;
   }
   return true;
}

/* Apply or remove an affect to a character. */
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd )
{
   SKILLTYPE *skill;
   int mod, x;
   ch_ret retcode;

   mod = paf->modifier;

   if( fAdd )
   {
      if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
      {
         if( !xIS_EMPTY( paf->bitvector ) )
            xSET_BITS( ch->affected_by, paf->bitvector );
      }
   }
   else
   {
      if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
         xREMOVE_BITS( ch->affected_by, paf->bitvector );

      switch( paf->location % REVERSE_APPLY )
      {
         case APPLY_EXT_AFFECT:
            xREMOVE_BIT( ch->affected_by, mod );
            return;

         default:
            break;
      }
      mod = 0 - mod;
   }

   switch( paf->location % REVERSE_APPLY )
   {
      default:
         bug( "%s: unknown location %d.", __FUNCTION__, paf->location );
         return;

      case APPLY_NONE:
         break;

      case APPLY_HIT:
         ch->max_hit += mod;
         break;

      case APPLY_MOVE:
         ch->max_move += mod;
         break;

      case APPLY_MANA:
         ch->max_mana += mod;
         break;

      case APPLY_ARMOR:
         ch->armor += mod;
         break;

      case APPLY_HITROLL:
         ch->hitroll += mod;
         break;

      case APPLY_DAMROLL:
         ch->damroll += mod;
         break;

      case APPLY_WAITSTATE:
         wait_state( ch, mod );
         break;

      case APPLY_STAT:
         for( x = 0; x < STAT_MAX; x++ )
         {
            if( xIS_SET( paf->bitvector, x ) )
               ch->mod_stats[x] += mod;
         }
         break;

      /* Bitvector modifying apply types */
      case APPLY_EXT_AFFECT:
         if( mod < 0 || mod >= AFF_MAX )
            bug( "%s: unknown affect: mod (%d) paf->modifier (%d)", __FUNCTION__, mod, paf->modifier );
         else if( xIS_EMPTY( paf->bitvector ) || ( !xIS_EMPTY( paf->bitvector ) && mod != 0 ) )
            xSET_BIT( ch->affected_by, mod );
         break;

      case APPLY_RESISTANT:
         for( x = 0; x < RIS_MAX; x++ )
         {
            if( xIS_SET( paf->bitvector, x ) )
               ch->resistant[x] += mod;
         }
         break;

      case APPLY_WEAPONSPELL:   /* see fight.c */
         break;

      case APPLY_STRIPSN:
         if( is_valid_sn( mod ) )
            affect_strip( ch, mod );
         else
            bug( "%s: APPLY_STRIPSN invalid sn %d", __FUNCTION__, mod );
         break;

      case APPLY_WEARSPELL:
      case APPLY_REMOVESPELL:
         if( xIS_SET( ch->in_room->room_flags, ROOM_NO_MAGIC )
         || ( ( paf->location % REVERSE_APPLY ) == APPLY_WEARSPELL && !fAdd )
         || ( ( paf->location % REVERSE_APPLY ) == APPLY_REMOVESPELL && fAdd )
         || saving_char == ch /* so save/quit doesn't trigger */
         || loading_char == ch )   /* so loading doesn't trigger */
            return;

         mod = abs( mod );
         if( is_valid_sn( mod ) && ( skill = skill_table[mod] ) && skill->type == SKILL_SPELL )
         {
            if( skill->target == TAR_IGNORE || skill->target == TAR_OBJ_INV )
            {
               bug( "APPLY_WEARSPELL/APPLY_REMOVESPELL trying to apply bad target spell.  SN is %d.", mod );
               return;
            }
            if( ( retcode = ( *skill->spell_fun ) ( mod, ch->level, ch, ch ) ) == rCHAR_DIED || char_died( ch ) )
               return;
         }
         break;
   }

   check_chareq( ch );
}

/* Give an affect to a char. */
void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
   AFFECT_DATA *paf_new, *paf_check;
   SKILLTYPE *skill, *skill_new;
   bool inserted = false;

   if( !ch )
   {
      bug( "%s: (NULL, %d)", __FUNCTION__, paf ? paf->type : 0 );
      return;
   }

   if( !paf )
   {
      bug( "%s: (%s, NULL)", __FUNCTION__, ch->name );
      return;
   }

   CREATE( paf_new, AFFECT_DATA, 1 );
   if( !paf_new )
   {
      bug( "%s: paf_new is NULL after CREATE.", __FUNCTION__ );
      return;
   }
   paf_new->type = paf->type;
   paf_new->duration = paf->duration;
   paf_new->location = paf->location;
   paf_new->modifier = paf->modifier;
   paf_new->bitvector = paf->bitvector;

   skill_new = get_skilltype( paf_new->type );

   for( paf_check = ch->first_affect; paf_check; paf_check = paf_check->next )
   {
      skill = get_skilltype( paf_check->type );
      if( paf_new->duration < paf_check->duration )
      {
         INSERT( paf_new, paf_check, ch->first_affect, next, prev );
         inserted = true;
         break;
      }
      else if( paf_new->duration == paf_check->duration && skill_new && skill
      && strcmp( skill_new->name, skill->name ) <= 0 )
      {
         INSERT( paf_new, paf_check, ch->first_affect, next, prev );
         inserted = true;
         break;
      }
   }
   if( !inserted )
      LINK( paf_new, ch->first_affect, ch->last_affect, next, prev );

   affect_modify( ch, paf_new, true );
}

/* Remove an affect from a char. */
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf )
{
   if( !ch->first_affect )
   {
      bug( "%s: (%s, %d): no affect.", __FUNCTION__, ch->name, paf ? paf->type : 0 );
      return;
   }

   affect_modify( ch, paf, false );

   UNLINK( paf, ch->first_affect, ch->last_affect, next, prev );
   DISPOSE( paf );
}

/* Strip all affects of a given sn. */
void affect_strip( CHAR_DATA *ch, int sn )
{
   AFFECT_DATA *paf, *paf_next;

   for( paf = ch->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      if( paf->type == sn )
         affect_remove( ch, paf );
   }
}

/* Return true if a char is affected by a spell. */
bool is_affected( CHAR_DATA *ch, int sn )
{
   AFFECT_DATA *paf;

   for( paf = ch->first_affect; paf; paf = paf->next )
      if( paf->type == sn )
         return true;

   return false;
}

/*
 * Add or enhance an affect.
 * Limitations put in place by Thoric, they may be high... but at least they're there :)
 */
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf )
{
   AFFECT_DATA *paf_old;

   for( paf_old = ch->first_affect; paf_old; paf_old = paf_old->next )
   {
      if( paf_old->type != paf->type ) /* Check same type */
         continue;
      if( ( paf_old->location % REVERSE_APPLY ) != ( paf->location % REVERSE_APPLY ) ) /* Check same location */
         continue;
      if( !xSAME_BITS( paf_old->bitvector, paf->bitvector ) ) /* Check bitvectors */
         continue;
      if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT && paf->modifier != paf_old->modifier ) /* Check for same modifier */
         continue;

      paf->duration = URANGE( paf_old->duration, ( paf->duration + paf_old->duration ), 1000000 );
      /* Don't increase APPLY_EXT_AFFECT modifier */
      if( ( paf->location % REVERSE_APPLY ) != APPLY_EXT_AFFECT )
      {
         if( paf->modifier )
            paf->modifier = URANGE( paf->modifier, ( paf->modifier + paf_old->modifier ), 50000 );
         else
            paf->modifier = paf_old->modifier;
      }
      affect_remove( ch, paf_old );
      break;
   }
   affect_to_char( ch, paf );
}

/* Apply only affected on a char */
void aris_affect( CHAR_DATA *ch, AFFECT_DATA *paf )
{
   if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
   {
      if( !xIS_EMPTY( paf->bitvector ) )
         xSET_BITS( ch->affected_by, paf->bitvector );
   }

   switch( paf->location % REVERSE_APPLY )
   {
      case APPLY_EXT_AFFECT:
         xSET_BIT( ch->affected_by, paf->modifier );
         break;
   }
}

/*
 * Update affecteds and RIS for a character in case things get messed.
 * This should only really be used as a quick fix until the cause
 * of the problem can be hunted down. - FB
 * Last modified: June 30, 1997
 *
 * Quick fix?  Looks like a good solution for a lot of problems.
 */

/* Temp mod to bypass immortals so they can keep their mset affects,
 * just a band-aid until we get more time to look at it -- Blodkai */
void update_aris( CHAR_DATA *ch )
{
   AFFECT_DATA *paf;
   OBJ_DATA *obj;
   int stat;

   if( is_npc( ch ) || is_immortal( ch ) )
      return;

   xCLEAR_BITS( ch->affected_by );
   xCLEAR_BITS( ch->no_affected_by );

   for( stat = 0; stat < RIS_MAX; stat++ )
      ch->resistant[stat] = 0;

   /* Add in effects from race */
   if( race_table[ch->race] )
   {
      if( !xIS_EMPTY( race_table[ch->race]->affected ) )
         xSET_BITS( ch->affected_by, race_table[ch->race]->affected );
      for( stat = 0; stat < RIS_MAX; stat++ )
         ch->resistant[stat] += race_table[ch->race]->resistant[stat];
   }

   /* Add in effects from deities */
   if( ch->pcdata->deity )
   {
      if( ch->pcdata->favor > ch->pcdata->deity->affectednum )
         if( !xIS_EMPTY( ch->pcdata->deity->affected ) )
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected );
      if( ch->pcdata->favor > ch->pcdata->deity->resistnum )
         for( stat = 0; stat < RIS_MAX; stat++ )
            ch->resistant[stat] += race_table[ch->race]->resistant[stat];
   }

   /* Add in effect from spells */
   for( paf = ch->first_affect; paf; paf = paf->next )
      aris_affect( ch, paf );

   /* Add in effects from equipment */
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc != WEAR_NONE )
      {
         for( paf = obj->first_affect; paf; paf = paf->next )
            aris_affect( ch, paf );

         for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
            aris_affect( ch, paf );
      }
   }

   /* Add in effects for polymorph */
   if( ch->morph )
   {
      if( !xIS_EMPTY( ch->morph->affected_by ) )
         xSET_BITS( ch->affected_by, ch->morph->affected_by );
      if( !xIS_EMPTY( ch->morph->no_affected_by ) )
         xSET_BITS( ch->no_affected_by, ch->morph->no_affected_by );
      for( stat = 0; stat < RIS_MAX; stat++ )
         ch->resistant[stat] += ch->morph->resistant[stat];
   }
}

/* Adjust the rooms light for this ch */
void adjust_room_light( CHAR_DATA *ch, bool increase )
{
   OBJ_DATA *obj;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc != WEAR_NONE && obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
      {
         if( increase )
            ++ch->in_room->light;
         else if( ch->in_room->light > 0 )
            --ch->in_room->light;
      }
   }
}

/* Move a char out of a room. */
void char_from_room( CHAR_DATA *ch )
{
   if( !ch->in_room )
   {
      bug( "%s: NULL.", __FUNCTION__ );
      return;
   }

   if( !is_npc( ch ) )
      --ch->in_room->area->nplayer;

   if( is_npc( ch ) )
      xREMOVE_BIT( ch->act, ACT_WILDERNESS );
   else
      xREMOVE_BIT( ch->act, PLR_WILDERNESS );
   ch->cords[0] = 0;
   ch->cords[1] = 0;

   adjust_room_light( ch, false );

   ch->in_room->charcount--;
   UNLINK( ch, ch->in_room->first_person, ch->in_room->last_person, next_in_room, prev_in_room );
   if( ch->in_room->area )
      UNLINK( ch, ch->in_room->area->first_person, ch->in_room->area->last_person, next_in_area, prev_in_area );
   ch->was_in_room = ch->in_room;
   ch->in_room = NULL;
   ch->next_in_room = NULL;
   ch->prev_in_room = NULL;

   if( !is_npc( ch ) && get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
      remove_timer( ch, TIMER_SHOVEDRAG );
}

typedef struct teleport_data TELEPORT_DATA;

/* Delayed teleport type. */
struct teleport_data
{
   TELEPORT_DATA *next, *prev;
   ROOM_INDEX_DATA *room;
   short timer;
};

TELEPORT_DATA *first_teleport, *last_teleport;

void tele_update( void )
{
   TELEPORT_DATA *tele, *tele_next;

   if( !first_teleport )
      return;

   for( tele = first_teleport; tele; tele = tele_next )
   {
      tele_next = tele->next;
      if( --tele->timer <= 0 )
      {
         if( tele->room->first_person )
         {
            if( xIS_SET( tele->room->room_flags, ROOM_TELESHOWDESC ) )
               teleport( tele->room->first_person, tele->room->tele_vnum, TELE_SHOWDESC | TELE_TRANSALL );
            else
               teleport( tele->room->first_person, tele->room->tele_vnum, TELE_TRANSALL );
         }
         UNLINK( tele, first_teleport, last_teleport, next, prev );
         DISPOSE( tele );
      }
   }
}

/* Move a char into a room. */
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
   if( !ch )
   {
      bug( "%s: NULL ch!", __FUNCTION__ );
      return;
   }

   if( !pRoomIndex || !get_room_index( pRoomIndex->vnum ) )
   {
      if( ch->was_in_room && get_room_index( ch->was_in_room->vnum ) )
      {
         bug( "%s: Trying to put (%s) in a NULL room! Putting char back in previous room!", __FUNCTION__, ch->name );
         pRoomIndex = ch->was_in_room;
      }
      else if( !( pRoomIndex = get_room_index( sysdata.room_limbo ) ) )
         bug( "%s: Trying to put %s in a NULL room! Putting char in limbo (%d)", __FUNCTION__, ch->name, sysdata.room_limbo );
      else
         bug( "%s: Trying to put %s in a NULL room! Limbo [%d] doesn't exist prepare for a crash!", __FUNCTION__, ch->name, sysdata.room_limbo );
   }

   stop_fishing( ch );

   ch->in_room = pRoomIndex;

   LINK( ch, pRoomIndex->first_person, pRoomIndex->last_person, next_in_room, prev_in_room );
   if( pRoomIndex->area )
      LINK( ch, pRoomIndex->area->first_person, pRoomIndex->area->last_person, next_in_area, prev_in_area );
   pRoomIndex->charcount++;

   if( !is_npc( ch ) )
      if( ++pRoomIndex->area->nplayer > pRoomIndex->area->max_players )
         pRoomIndex->area->max_players = pRoomIndex->area->nplayer;

   adjust_room_light( ch, true );

   explore_room( ch, pRoomIndex );

   if( !is_npc( ch ) && xIS_SET( pRoomIndex->room_flags, ROOM_SAFE ) && get_timer( ch, TIMER_SHOVEDRAG ) <= 0 )
      add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );

   /*
    * Delayed Teleport rooms             -Thoric
    * Should be the last thing checked in this function
    */
   if( xIS_SET( pRoomIndex->room_flags, ROOM_TELEPORT ) && pRoomIndex->tele_delay > 0 )
   {
      TELEPORT_DATA *tele;

      for( tele = first_teleport; tele; tele = tele->next )
         if( tele->room == pRoomIndex )
            return;

      CREATE( tele, TELEPORT_DATA, 1 );
      LINK( tele, first_teleport, last_teleport, next, prev );
      tele->room = pRoomIndex;
      tele->timer = pRoomIndex->tele_delay;
   }

   if( !ch->was_in_room )
      ch->was_in_room = ch->in_room;
}

void free_teleports( void )
{
   TELEPORT_DATA *tele, *tele_next;

   for( tele = first_teleport; tele; tele = tele_next )
   {
      tele_next = tele->next;

      UNLINK( tele, first_teleport, last_teleport, next, prev );
      DISPOSE( tele );
   }
}

/* Give an obj to a char. */
OBJ_DATA *obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
   OBJ_DATA *otmp, *oret = obj;
   bool skipgroup = false, grouped = false;
   int oweight = get_obj_weight( obj );
   int onum = get_obj_number( obj );
   int wear_loc = obj->wear_loc;
   EXT_BV extra_flags = obj->extra_flags;

   if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
   {
      if( !is_immortal( ch ) && ( !is_npc( ch ) || !xIS_SET( ch->act, ACT_PROTOTYPE ) ) )
         return obj_to_room( obj, ch->in_room );
   }

   /* Reset information should be set after the object is given to who it needs to be given to, so here lets free it up so it can reset later */
   if( obj->reset )
      obj->reset->obj = NULL;
   obj->reset = NULL;

   if( loading_char == ch )
      if( obj->wear_loc != -1 || obj->t_wear_loc != -1 )
         skipgroup = true;

   if( is_npc( ch ) && ch->pIndexData->pShop )
      skipgroup = true;

   if( xIS_SET( obj->extra_flags, ITEM_NOGROUP ) )
      skipgroup = true;

   if( !skipgroup )
   {
      for( otmp = ch->first_carrying; otmp; otmp = otmp->next_content )
      {
         if( ( oret = group_object( otmp, obj ) ) == otmp )
         {
            grouped = true;
            break;
         }
      }
   }
   if( !grouped )
   {
      if( !is_npc( ch ) || !ch->pIndexData->pShop )
      {
         LINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );
         obj->carried_by = ch;
         obj->in_room = NULL;
         obj->in_obj = NULL;
      }
      else
      {
         /* If ch is a shopkeeper, add the obj using an insert sort */
         for( otmp = ch->first_carrying; otmp; otmp = otmp->next_content )
         {
            if( obj->level > otmp->level )
            {
               INSERT( obj, otmp, ch->first_carrying, next_content, prev_content );
               break;
            }
            else if( obj->level == otmp->level && strcmp( obj->short_descr, otmp->short_descr ) < 0 )
            {
               INSERT( obj, otmp, ch->first_carrying, next_content, prev_content );
               break;
            }
         }

         if( !otmp )
            LINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );

         obj->carried_by = ch;
         obj->in_room = NULL;
         obj->in_obj = NULL;
      }
   }
   if( wear_loc == WEAR_NONE )
   {
      ch->carry_number += onum;
      ch->carry_weight += oweight;
   }
   else if( !xIS_SET( extra_flags, ITEM_MAGIC ) )
      ch->carry_weight += oweight;
   return ( oret ? oret : obj );
}

/* Take an obj from its character. */
void obj_from_char( OBJ_DATA *obj )
{
   CHAR_DATA *ch;

   if( !( ch = obj->carried_by ) )
   {
      bug( "%s: null ch.", __FUNCTION__ );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
      unequip_char( ch, obj );

   /* obj may drop during unequip... */
   if( !obj || !obj->carried_by )
      return;

   UNLINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );

   if( is_obj_stat( obj, ITEM_COVERING ) && obj->first_content )
      empty_obj( obj, NULL, NULL );

   obj->in_room = NULL;
   obj->carried_by = NULL;
   ch->carry_number -= get_obj_number( obj );
   ch->carry_weight -= get_obj_weight( obj );
}

/* Uses a 0 and higher now, higher is better */
void apply_ac( CHAR_DATA *ch, OBJ_DATA *obj, int iWear, bool remove )
{
   if( obj->item_type != ITEM_ARMOR )
      return;

   switch( iWear )
   {
      case WEAR_HEAD:
      case WEAR_EARS:      case WEAR_EAR_L:      case WEAR_EAR_R:
      case WEAR_EYES:      case WEAR_EYE_L:      case WEAR_EYE_R:
      case WEAR_FACE:
      case WEAR_NECK:
      case WEAR_SHOULDERS: case WEAR_SHOULDER_L: case WEAR_SHOULDER_R:
      case WEAR_ABOUT:
      case WEAR_BODY:
      case WEAR_BACK:
      case WEAR_ARMS:      case WEAR_ARM_L:      case WEAR_ARM_R:
      case WEAR_HANDS:     case WEAR_HAND_L:     case WEAR_HAND_R:
      case WEAR_FINGERS:   case WEAR_FINGER_L:   case WEAR_FINGER_R:
      case WEAR_HOLD_B:    case WEAR_HOLD_L:     case WEAR_HOLD_R:
      case WEAR_WAIST:
      case WEAR_LEGS:      case WEAR_LEG_L:      case WEAR_LEG_R:
      case WEAR_ANKLES:    case WEAR_ANKLE_L:    case WEAR_ANKLE_R:
      case WEAR_FEET:      case WEAR_FOOT_L:     case WEAR_FOOT_R:
         if( remove )
            ch->armor -= obj->value[0];
         else
            ch->armor += obj->value[0];
         if( ch->armor < 0 )
            ch->armor = 0;
         break;
   }
}

/*
 * Find a piece of eq on a character.
 * Will pick the top layer if clothing is layered.		-Thoric
 */
OBJ_DATA *get_eq_char( CHAR_DATA *ch, int iWear )
{
   OBJ_DATA *obj, *maxobj = NULL;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->wear_loc == iWear )
      {
         if( !obj->pIndexData->layers )
            return obj;
         else if( !maxobj || obj->pIndexData->layers > maxobj->pIndexData->layers )
            maxobj = obj;
      }

   return maxobj;
}

/* Equip a char with an obj. */
void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear )
{
   MCLASS_DATA *mclass;
   AFFECT_DATA *paf;
   OBJ_DATA *otmp;
   int stat;

   if( obj->carried_by != ch )
   {
      bug( "%s: obj not being carried by (%s)[%d]!", __FUNCTION__, ch->name,
         ch->pIndexData ? ch->pIndexData->vnum : 0 );
      return;
   }

   if( !is_obj_stat( obj, ITEM_LODGED )
   && ( otmp = get_eq_char( ch, iWear ) )
   && ( !otmp->pIndexData->layers || !obj->pIndexData->layers ) )
   {
      bug( "%s: already equipped on (%d) (%s) by (%s)[%d].", __FUNCTION__, iWear, wear_locs[iWear], ch->name,
         ch->pIndexData ? ch->pIndexData->vnum : 0 );
      return;
   }

   separate_obj( obj );
   if( !is_obj_stat( obj, ITEM_LODGED ) && ch->level < obj->level )
   {
      ch_printf( ch, "You must be level %d to use this object.\r\n", obj->level );
      act( AT_ACTION, "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM );
      return;
   }
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( !is_obj_stat( obj, ITEM_LODGED ) && get_curr_stat( stat, ch ) < obj->stat_reqs[stat] )
      {
         ch_printf( ch, "You need to have %d %s to use this object.\r\n", obj->stat_reqs[STAT_STR], stattypes[stat] );
         act( AT_ACTION, "$n tries to use $p, but lacks something to do so.", ch, obj, NULL, TO_ROOM );
         return;
      }
   }
   if( !is_obj_stat( obj, ITEM_LODGED ) && !is_npc( ch ) && xIS_SET( obj->race_restrict, ch->race ) )
   {
      send_to_char( "Your race can't wear this object.\r\n", ch );
      act( AT_ACTION, "$n tries to use $p, but is forbidden to do so.", ch, obj, NULL, TO_ROOM );
      return;
   }

   if( !is_obj_stat( obj, ITEM_LODGED ) && !is_npc( ch ) )
   {
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( mclass->wclass >= 0 && xIS_SET( obj->class_restrict, mclass->wclass ) )
         {
            send_to_char( "Your class can't wear this object.\r\n", ch );
            act( AT_ACTION, "$n tries to use $p, but is forbidden to do so.", ch, obj, NULL, TO_ROOM );
            return;
         }
      }
   }

   if( !is_npc( ch ) && ch->race >= 0 && ch->race < MAX_PC_RACE && race_table[ch->race] && xIS_SET( race_table[ch->race]->where_restrict, iWear ) )
   {
      send_to_char( "You can't wear that correctly.\r\n", ch );
      act( AT_ACTION, "$n tries to use $p, but isn't able to wear it correctly.", ch, obj, NULL, TO_ROOM );
      return;      
   }

   if( !is_obj_stat( obj, ITEM_LODGED )
   && ( ( is_obj_stat( obj, ITEM_ANTI_EVIL ) && is_evil( ch ) )
   || ( is_obj_stat( obj, ITEM_ANTI_GOOD ) && is_good( ch ) )
   || ( is_obj_stat( obj, ITEM_ANTI_NEUTRAL ) && is_neutral( ch ) ) ) )
   {
      if( loading_char != ch )
      {
         act( AT_MAGIC, "You're zapped by $p.", ch, obj, NULL, TO_CHAR );
         act( AT_MAGIC, "$n is zapped by $p.", ch, obj, NULL, TO_ROOM );
      }
      oprog_zap_trigger( ch, obj );
      if( xIS_SET( sysdata.save_flags, SV_ZAPDROP ) && !char_died( ch ) )
         save_char_obj( ch );
      return;
   }

   apply_ac( ch, obj, iWear, false );
   obj->wear_loc = iWear;

   ch->carry_number -= get_obj_number( obj );
   if( is_obj_stat( obj, ITEM_MAGIC ) )
      ch->carry_weight -= get_obj_weight( obj );

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      affect_modify( ch, paf, true );
   for( paf = obj->first_affect; paf; paf = paf->next )
      affect_modify( ch, paf, true );

   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room )
      ++ch->in_room->light;
}

/* Unequip a char with an obj. */
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj )
{
   AFFECT_DATA *paf;

   if( obj->wear_loc == WEAR_NONE )
   {
      bug( "%s: already unequipped.", __FUNCTION__ );
      return;
   }

   ch->carry_number += get_obj_number( obj );
   if( is_obj_stat( obj, ITEM_MAGIC ) )
      ch->carry_weight += get_obj_weight( obj );

   apply_ac( ch, obj, obj->wear_loc, true );
   obj->wear_loc = -1;
   obj->t_wear_loc = -1;

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      affect_modify( ch, paf, false );
   if( obj->carried_by )
      for( paf = obj->first_affect; paf; paf = paf->next )
         affect_modify( ch, paf, false );

   update_aris( ch );

   if( !obj->carried_by )
      return;

   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room && ch->in_room->light > 0 )
      --ch->in_room->light;
}

/* Move an obj out of a room. */

int falling;

void obj_from_room( OBJ_DATA *obj )
{
   ROOM_INDEX_DATA *in_room;

   if( !( in_room = obj->in_room ) )
   {
      bug( "%s: NULL in_room.", __FUNCTION__ );
      return;
   }

   in_room->objcount--;
   UNLINK( obj, in_room->first_content, in_room->last_content, next_content, prev_content );

   /* Remove the wilderness stuff */
   obj->cords[0] = 0;
   obj->cords[1] = 0;
   xREMOVE_BIT( obj->extra_flags, ITEM_WILDERNESS );

   /* uncover contents */
   if( is_obj_stat( obj, ITEM_COVERING ) && obj->first_content )
      empty_obj( obj, NULL, obj->in_room );

   if( obj->item_type == ITEM_FIRE )
      obj->in_room->light -= obj->count;

   obj->carried_by = NULL;
   obj->in_obj = NULL;
   obj->in_room = NULL;
   if( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
      write_corpses( obj, true );
}

/* Move an obj into a room. */
OBJ_DATA *obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex )
{
   OBJ_DATA *otmp, *oret, *otmp_next;
   short count = obj->count;
   short item_type = obj->item_type;
   bool skipgroup = false;

   if( xIS_SET( obj->extra_flags, ITEM_NOGROUP ) )
      skipgroup = true;

   if( !skipgroup )
   {
      for( otmp = pRoomIndex->first_content; otmp; otmp = otmp_next )
      {
         otmp_next = otmp->next_content;

         if( ( oret = group_object( otmp, obj ) ) == otmp )
         {
            if( item_type == ITEM_FIRE )
               pRoomIndex->light += count;
            return oret;
         }
      }
   }

   /* Reset information should be set after the object is put in room, so here lets free it up so it can reset later */
   if( obj->reset )
      obj->reset->obj = NULL;
   obj->reset = NULL;
   LINK( obj, pRoomIndex->first_content, pRoomIndex->last_content, next_content, prev_content );
   pRoomIndex->objcount++;
   obj->in_room = pRoomIndex;
   obj->carried_by = NULL;
   obj->in_obj = NULL;
   obj->room_vnum = pRoomIndex->vnum;  /* hotboot tracker */
   if( item_type == ITEM_FIRE )
      pRoomIndex->light += count;
   falling++;
   obj_fall( obj, false );
   falling--;
   if( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
      write_corpses( obj, false );
   return obj;
}

/* Who's carrying an item -- recursive for nested objects - Thoric */
CHAR_DATA *carried_by( OBJ_DATA *obj )
{
   if( obj->in_obj )
      return carried_by( obj->in_obj );

   return obj->carried_by;
}

/* Move an object into an object. */
OBJ_DATA *obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to )
{
   OBJ_DATA *otmp, *oret;
   CHAR_DATA *who;
   bool skipgroup = false;

   if( !obj )
      return NULL;

   if( obj == obj_to )
   {
      bug( "%s: trying to put object inside itself: vnum %d", __FUNCTION__, obj->pIndexData->vnum );
      return obj;
   }

   /* Reset information should be set after the object is put where it needs to be, so here lets free it up so it can reset later */
   if( obj->reset )
      obj->reset->obj = NULL;
   obj->reset = NULL;

   if( !in_magic_container( obj_to ) && ( who = carried_by( obj_to ) ) )
      who->carry_weight += get_obj_weight( obj );

   if( xIS_SET( obj->extra_flags, ITEM_NOGROUP ) )
      skipgroup = true;

   if( !skipgroup )
      for( otmp = obj_to->first_content; otmp; otmp = otmp->next_content )
         if( ( oret = group_object( otmp, obj ) ) == otmp )
            return oret;

   LINK( obj, obj_to->first_content, obj_to->last_content, next_content, prev_content );

   obj->in_obj = obj_to;
   obj->in_room = NULL;
   obj->carried_by = NULL;

   return obj;
}

/* Move an object out of an object. */
void obj_from_obj( OBJ_DATA *obj )
{
   OBJ_DATA *obj_from;
   bool magic;

   if( !( obj_from = obj->in_obj ) )
   {
      bug( "%s: null obj_from.", __FUNCTION__ );
      return;
   }

   magic = in_magic_container( obj_from );

   UNLINK( obj, obj_from->first_content, obj_from->last_content, next_content, prev_content );

   /* uncover contents */
   if( is_obj_stat( obj, ITEM_COVERING ) && obj->first_content )
      empty_obj( obj, obj->in_obj, NULL );

   obj->in_obj = NULL;
   obj->in_room = NULL;
   obj->carried_by = NULL;

   if( !magic )
      for( ; obj_from; obj_from = obj_from->in_obj )
         if( obj_from->carried_by )
            obj_from->carried_by->carry_weight -= get_obj_weight( obj );
}

/* Free an object's data */
void free_obj( OBJ_DATA *obj, bool unlink )
{
   OBJ_DATA *obj_content;
   REL_DATA *RQueue, *rq_next;

   if( !obj )
      return;

   if( obj->item_type == ITEM_FISHINGPOLE )
      stop_obj_fishing( obj );

   if( obj->item_type == ITEM_PORTAL )
      remove_portal( obj );

   if( obj->reset )
      obj->reset->obj = NULL;
   obj->reset = NULL;

   if( obj->carried_by )
      obj_from_char( obj );
   if( obj->in_room )
      obj_from_room( obj );
   if( obj->in_obj )
      obj_from_obj( obj );

   while( ( obj_content = obj->last_content ) )
      extract_obj( obj_content );

   /* remove affects */
   {
      AFFECT_DATA *paf, *paf_next;

      for( paf = obj->first_affect; paf; paf = paf_next )
      {
         paf_next = paf->next;
         DISPOSE( paf );
         top_affect--;
      }
      obj->first_affect = obj->last_affect = NULL;
   }

   /* remove extra descriptions */
   {
      EXTRA_DESCR_DATA *ed, *ed_next;

      for( ed = obj->first_extradesc; ed; ed = ed_next )
      {
         ed_next = ed->next;
         STRFREE( ed->description );
         STRFREE( ed->keyword );
         DISPOSE( ed );
      }
      obj->first_extradesc = obj->last_extradesc = NULL;
   }

   if( obj == gobj_prev )
      gobj_prev = obj->prev;

   for( RQueue = first_relation; RQueue; RQueue = rq_next )
   {
      rq_next = RQueue->next;
      if( RQueue->Type == relOSET_ON )
      {
         if( obj == RQueue->Subject )
            ( ( CHAR_DATA * ) RQueue->Actor )->dest_buf = NULL;
         else
            continue;
         UNLINK( RQueue, first_relation, last_relation, next, prev );
         DISPOSE( RQueue );
      }
   }

   if( unlink )
   {
      UNLINK( obj, first_object, last_object, next, prev );
      if( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC )
      {
         UNLINK( obj, first_corpse, last_corpse, next_corpse, prev_corpse );
         --num_corpses;
      }
      UNLINK( obj, obj->pIndexData->first_copy, obj->pIndexData->last_copy, next_index, prev_index );

      obj->pIndexData->count -= obj->count;
      numobjsloaded -= obj->count;
      --physicalobjects;
   }

   STRFREE( obj->name );
   STRFREE( obj->short_descr );
   STRFREE( obj->description );
   STRFREE( obj->action_desc );
   STRFREE( obj->desc );
   STRFREE( obj->owner );
   DISPOSE( obj );
}

/* Free the object data and unlink it from the world. */
void extract_obj( OBJ_DATA *obj )
{
   free_obj( obj, true );
}

void extract_character_obj( OBJ_DATA *obj )
{
   OBJ_DATA *obj_content, *obj_content_next;

   for( obj_content = obj->first_content; obj_content != NULL; obj_content = obj_content_next )
   {
      obj_content_next = obj_content->next_content;

      extract_character_obj( obj_content );
   }

   if( is_obj_stat( obj, ITEM_QUEST ) )
      return;

   if( obj->item_type != ITEM_CONTAINER || !obj->first_content )
      extract_obj( obj );
}

/* Extract a char from the world. */
void extract_char( CHAR_DATA *ch, bool fPull )
{
   CHAR_DATA *wch;
   OBJ_DATA *obj, *nextobj;
   char buf[MSL];
   ROOM_INDEX_DATA *location;
   REL_DATA *RQueue, *rq_next;

   if( !ch )
   {
      bug( "%s: NULL ch.", __FUNCTION__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: %s in NULL room.", __FUNCTION__, ch->name ? ch->name : "???" );
      return;
   }

   if( ch == supermob )
   {
      bug( "%s: ch == supermob!", __FUNCTION__ );
      return;
   }

   if( char_died( ch ) )
   {
      bug( "%s: %s already died!", __FUNCTION__, ch->name );
      return;
   }

   if( ch == cur_char )
      cur_char_died = true;

   /* shove onto extraction queue */
   queue_extracted_char( ch, fPull );

   for( RQueue = first_relation; RQueue; RQueue = rq_next )
   {
      rq_next = RQueue->next;
      if( fPull && RQueue->Type == relMSET_ON )
      {
         if( ch == RQueue->Subject )
            ( ( CHAR_DATA * ) RQueue->Actor )->dest_buf = NULL;
         else if( ch != RQueue->Actor )
            continue;
         UNLINK( RQueue, first_relation, last_relation, next, prev );
         DISPOSE( RQueue );
      }
   }

   if( gch_prev == ch )
      gch_prev = ch->prev;

   if( fPull )
      die_follower( ch );

   stop_fighting( ch, true );

   if( ch->mount )
   {
      xREMOVE_BIT( ch->mount->act, ACT_MOUNTED );
      ch->mount = NULL;
      ch->position = POS_STANDING;
   }

   if( ch->mounter )
   {
      ch->mounter = NULL;
   }

   /* If player goes link dead and was editing something update it all */
   if( ch->dest_buf )
   {
      if( ( wch = ( CHAR_DATA * )ch->dest_buf ) )
      {
         if( wch->editing )
         {
            stop_editing( ch ); /* This is correct as ch and not wch */
            wch->editing = NULL;
         }
      }
      ch->dest_buf = NULL;
      ch->spare_ptr = NULL;
   }

   /* Update the one who was editing the ch */
   if( ch->editing )
   {
      stop_editing( ch->editing );
      ch->editing->substate = SUB_NONE;
      ch->editing->dest_buf = NULL;
      ch->editing->spare_ptr = NULL;
      send_to_char( "What you were editing has been extracted.\r\n", ch->editing );
      ch->editing = NULL;
   }

   /* check if this NPC was a mount, pet or in a group */
   if( is_npc( ch ) )
   {
      if( ch->reset )
         ch->reset->ch = NULL;
      ch->reset = NULL;
      for( wch = first_char; wch; wch = wch->next )
      {
         if( wch->mount == ch && wch != quitting_char )
         {
            wch->mount = NULL;
            wch->position = POS_STANDING;
            if( wch->in_room == ch->in_room )
            {
               act( AT_SOCIAL, "Your faithful mount, $N collapses beneath you...", wch, NULL, ch, TO_CHAR );
               act( AT_SOCIAL, "Sadly you dismount $M for the last time.", wch, NULL, ch, TO_CHAR );
               act( AT_PLAIN, "$n sadly dismounts $N for the last time.", wch, NULL, ch, TO_ROOM );
            }
         }
         if( wch->pcdata )
         {
            CHAR_DATA *pet;

            for( pet = wch->pcdata->first_pet; pet; pet = pet->next_pet )
            {
               if( pet != ch )
                  continue;
               UNLINK( pet, wch->pcdata->first_pet, wch->pcdata->last_pet, next_pet, prev_pet );
               if( wch->in_room == ch->in_room )
                  act( AT_SOCIAL, "You mourn for the loss of $N.", wch, NULL, ch, TO_CHAR );
            }
         }
      }
      xREMOVE_BIT( ch->act, ACT_MOUNTED );
      if( ch->group )
         remove_char_from_group( ch );
   }

   if( fPull )
   {
      while( ( obj = ch->last_carrying ) )
         extract_obj( obj );
   }
   else
   {
      for( obj = ch->first_carrying; obj; obj = nextobj )
      {
         nextobj = obj->next_content;
         extract_character_obj( obj );
      }
   }

   char_from_room( ch );

   if( !fPull )
   {
      location = NULL;

      if( !is_npc( ch ) && ch->pcdata->nation )
         location = get_room_index( ch->pcdata->nation->recall );

      if( !location && !is_npc( ch ) && ch->pcdata->clan )
         location = get_room_index( ch->pcdata->clan->recall );

      if( !location )
         location = get_room_index( sysdata.room_altar );

      if( !location )
         location = get_room_index( sysdata.room_limbo );

      if( !location )
         bug( "No location to send (%s) to...this could cause alot of issues!!!!", ch->name );

      char_to_room( ch, location );

      /* Make things a little fancier - Thoric */
      if( ( wch = get_char_room( ch, (char *)"healer" ) ) )
      {
         act( AT_MAGIC, "$n mutters a few incantations, waves $s hands and points $s finger.", wch, NULL, NULL, TO_ROOM );
         act( AT_MAGIC, "$n appears from some strange swirling mists!", ch, NULL, NULL, TO_ROOM );
         snprintf( buf, sizeof( buf ), "say Welcome back to the land of the living, %s", capitalize( ch->name ) );
         interpret( wch, buf );
      }
      else
         act( AT_MAGIC, "$n appears from some strange swirling mists!", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_RESTING;
      return;
   }

   if( is_npc( ch ) )
   {
      --ch->pIndexData->count;
      --nummobsloaded;
      UNLINK( ch, ch->pIndexData->first_copy, ch->pIndexData->last_copy, next_index, prev_index );
   }

   for( wch = first_char; wch; wch = wch->next )
   {
      if( wch->reply == ch )
         wch->reply = NULL;
      if( wch->retell == ch )
         wch->retell = NULL;
   }

   UNLINK( ch, first_char, last_char, next, prev );

   if( ch->desc )
   {
      if( ch->desc->character != ch )
         bug( "%s: char's descriptor points to another char", __FUNCTION__ );
      else
      {
         ch->desc->character = NULL;
         close_socket( ch->desc, false );
         ch->desc = NULL;
      }
   }
}

/* Find a char in the room. */
CHAR_DATA *get_char_room( CHAR_DATA *ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *rch;
   int number, count, vnum;

   number = number_argument( argument, arg );
   if( !str_cmp( arg, "self" ) )
      return ch;

   if( get_trust( ch ) >= PERM_BUILDER && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;

   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( !can_see( ch, rch ) )
         continue;

      if( !can_see_character( ch, rch ) )
         continue;

      if( vnum != -1 )
      {
         if( !is_npc( rch ) || vnum != rch->pIndexData->vnum )
            continue;
      }
      else if( !nifty_is_name( arg, rch->name ) )
         continue;

      if( number == 0 && !is_npc( rch ) )
         return rch;
      else if( ++count == number )
         return rch;
   }

   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of characters
    * again looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( !can_see( ch, rch ) )
         continue;

      if( !can_see_character( ch, rch ) )
         continue;

      if( !nifty_is_name_prefix( arg, rch->name ) )
         continue;

      if( number == 0 && !is_npc( rch ) )
         return rch;
      else if( ++count == number )
         return rch;
   }

   return NULL;
}

/* Get a character in the same area */
CHAR_DATA *get_char_area( CHAR_DATA *ch, char *argument )
{
   char arg[MIL];
   AREA_DATA *parea = NULL;
   CHAR_DATA *wch;
   int number, count, vnum;

   number = number_argument( argument, arg );
   count = 0;
   if( !str_cmp( arg, "self" ) )
      return ch;

   if( ch && ch->in_room && ch->in_room->area )
      parea = ch->in_room->area;

   /* Allow reference by vnum for saints+ -Thoric */
   if( ch && get_trust( ch ) >= PERM_BUILDER && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   /* check the room for an exact match */
   if( ch )
   {
      for( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
      {
         if( can_see( ch, wch ) && ( nifty_is_name( arg, wch->name ) || ( is_npc( wch ) && vnum == wch->pIndexData->vnum ) ) )
         {
            if( number == 0 && !is_npc( wch ) )
               return wch;
            else if( ++count == number )
               return wch;
         }
      }
   }
   count = 0;

   /* check the area for an exact match */
   if( parea )
   {
      for( wch = parea->first_person; wch; wch = wch->next_in_area )
      {
         if( ( !ch || can_see( ch, wch ) )
         && ( nifty_is_name( arg, wch->name ) || ( is_npc( wch ) && vnum == wch->pIndexData->vnum ) ) )
         {
            if( number == 0 && !is_npc( wch ) )
               return wch;
            else if( ++count == number )
               return wch;
         }
      }
   }
   else /* If no ch or no parea check full list for ones in area */
   {
      for( wch = first_char; wch; wch = wch->next )
      {
         if( ( !ch || ( can_see( ch, wch ) && ch->in_room->area == wch->in_room->area ) )
         && ( nifty_is_name( arg, wch->name ) || ( is_npc( wch ) && vnum == wch->pIndexData->vnum ) ) )
         {
            if( number == 0 && !is_npc( wch ) )
               return wch;
            else if( ++count == number )
               return wch;
         }
      }
   }

   /* bail out if looking for a vnum match */
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, check the room for
    * for a prefix match, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   if( ch )
   {
      for( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
      {
         if( !can_see( ch, wch ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
         if( ch->in_room->area != wch->in_room->area )
            continue;
         if( number == 0 && !is_npc( wch ) )
            return wch;
         else if( ++count == number )
            return wch;
      }
   }

   /*
    * If we didn't find a prefix match in the room, run through the full list
    * of characters looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   if( parea )
   {
      for( wch = parea->first_person; wch; wch = wch->next_in_area )
      {
         if( ( ch && !can_see( ch, wch ) ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
         if( number == 0 && !is_npc( wch ) )
            return wch;
         else if( ++count == number )
            return wch;
      }
   }
   else
   {
      for( wch = first_char; wch; wch = wch->next )
      {
         if( ( ch && !can_see( ch, wch ) ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
         if( ch && ch->in_room->area != wch->in_room->area )
            continue;
         if( number == 0 && !is_npc( wch ) )
            return wch;
         else if( ++count == number )
            return wch;
      }
   }

   return NULL;
}

/* Find a char in the world. */
CHAR_DATA *get_char_world( CHAR_DATA *ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *wch;
   int number, count, vnum;

   number = number_argument( argument, arg );
   count = 0;
   if( !str_cmp( arg, "self" ) )
      return ch;

   /* Allow reference by vnum for saints+ -Thoric */
   if( ch && get_trust( ch ) >= PERM_BUILDER && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   /* check the room for an exact match */
   if( ch )
      for( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
         if( can_see( ch, wch ) && ( nifty_is_name( arg, wch->name ) || ( is_npc( wch ) && vnum == wch->pIndexData->vnum ) ) )
         {
            if( number == 0 && !is_npc( wch ) )
               return wch;
            else if( ++count == number )
               return wch;
         }

   count = 0;

   /* check the world for an exact match */
   for( wch = first_char; wch; wch = wch->next )
      if( ( !ch || can_see( ch, wch ) ) && ( nifty_is_name( arg, wch->name ) || ( is_npc( wch ) && vnum == wch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !is_npc( wch ) )
            return wch;
         else if( ++count == number )
            return wch;
      }

   /* bail out if looking for a vnum match */
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, check the room for
    * for a prefix match, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   if( ch )
      for( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
      {
         if( !can_see( ch, wch ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
         if( number == 0 && !is_npc( wch ) )
            return wch;
         else if( ++count == number )
            return wch;
      }

   /*
    * If we didn't find a prefix match in the room, run through the full list
    * of characters looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( wch = first_char; wch; wch = wch->next )
   {
      if( ( ch && !can_see( ch, wch ) ) || !nifty_is_name_prefix( arg, wch->name ) )
         continue;
      if( number == 0 && !is_npc( wch ) )
         return wch;
      else if( ++count == number )
         return wch;
   }

   return NULL;
}

/* Find an obj in a list. */
OBJ_DATA *get_obj_list( CHAR_DATA *ch, char *argument, OBJ_DATA *list )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   number = number_argument( argument, arg );
   if( is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = list; obj; obj = obj->next_content )
      if( can_see_obj( ch, obj ) && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = list; obj; obj = obj->next_content )
      if( can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}

/* Find an obj in a list...going the other way -Thoric */
OBJ_DATA *get_obj_list_rev( CHAR_DATA *ch, char *argument, OBJ_DATA *list )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   number = number_argument( argument, arg );
   if( is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = list; obj; obj = obj->prev_content )
      if( can_see_obj( ch, obj ) && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = list; obj; obj = obj->prev_content )
      if( can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}

/* Find an obj in player's inventory or wearing via a vnum -Shaddai */
OBJ_DATA *get_obj_vnum( CHAR_DATA *ch, int vnum )
{
   OBJ_DATA *obj;

   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( can_see_obj( ch, obj ) && obj->pIndexData->vnum == vnum )
         return obj;
   return NULL;
}

/* Find an obj in player's inventory. */
OBJ_DATA *get_obj_carry( CHAR_DATA *ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   number = number_argument( argument, arg );
   if( is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
      && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;

   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}

/* Find an obj in player's equipment. */
OBJ_DATA *get_obj_wear( CHAR_DATA *ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   number = number_argument( argument, arg );

   if( is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj ) && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ++count == number )
            return obj;

   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      if( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ++count == number )
            return obj;

   return NULL;
}

/* This way we can specify what list to look in */
/* 0 - Check all the list */
/* 1 - Check just what is in the room where the character is */
/* 2 - Check just what is in the inventory of the character */
/* 3 - Check just what is being worn by the character */
OBJ_DATA *new_get_obj_here( int type, CHAR_DATA *ch, char *argument )
{
   OBJ_DATA *obj;

   if( type == 0 || type == 1 )
      if( ( obj = get_obj_list_rev( ch, argument, ch->in_room->last_content ) ) )
         return obj;

   if( type == 0 || type == 2 )
      if( ( obj = get_obj_carry( ch, argument ) ) )
         return obj;

   if( type == 0 || type == 3 )
      if( ( obj = get_obj_wear( ch, argument ) ) )
         return obj;

   return NULL;
}

/* Find an obj in the room or in inventory. */
OBJ_DATA *get_obj_here( CHAR_DATA *ch, char *argument )
{
   /* Just check all the list if old way is used */
   return new_get_obj_here( 0, ch, argument );
}

/* Find an obj in the world. */
OBJ_DATA *get_obj_world( CHAR_DATA *ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   int number, count, vnum;

   if( ( obj = get_obj_here( ch, argument ) ) )
      return obj;

   number = number_argument( argument, arg );

   /* Allow reference by vnum for saints+ -Thoric */
   if( get_trust( ch ) >= PERM_BUILDER && is_number( arg ) )
      vnum = atoi( arg );
   else
      vnum = -1;

   count = 0;
   for( obj = first_object; obj; obj = obj->next )
      if( can_see_obj( ch, obj ) && ( nifty_is_name( arg, obj->name ) || vnum == obj->pIndexData->vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;

   /* bail out if looking for a vnum */
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( obj = first_object; obj; obj = obj->next )
      if( can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;

   return NULL;
}


/*
 * How mental state could affect finding an object		-Thoric
 * Used by get/drop/put/quaff/recite/etc
 * Increasingly freaky based on mental state and drunkeness
 */
bool ms_find_obj( CHAR_DATA *ch )
{
   const char *t;
   int ms = ch->mental_state;
   int drunk = is_npc( ch ) ? 0 : ch->pcdata->condition[COND_DRUNK];

   /*
    * we're going to be nice and let nothing weird happen unless
    * you're a tad messed up
    */
   drunk = UMAX( 1, drunk );
   if( abs( ms ) + ( drunk / 3 ) < 30 )
      return false;
   if( ( number_percent( ) + ( ms < 0 ? 15 : 5 ) ) > abs( ms ) / 2 + drunk / 4 )
      return false;
   if( ms > 15 )  /* range 1 to 20 -- feel free to add more */
   {
      switch( number_range( UMAX( 1, ( ms / 5 - 15 ) ), ( ms + 4 ) / 5 ) )
      {
         default:
         case 1:
            t = "As you reach for it, you forgot what it was...\r\n";
            break;

         case 2:
            t = "As you reach for it, something inside stops you...\r\n";
            break;

         case 3:
            t = "As you reach for it, it seems to move out of the way...\r\n";
            break;

         case 4:
            t = "You grab frantically for it, but can't seem to get a hold of it...\r\n";
            break;

         case 5:
            t = "It disappears as soon as you touch it!\r\n";
            break;

         case 6:
            t = "You would if it would stay still!\r\n";
            break;

         case 7:
            t = "Whoa!  It's covered in blood!  Ack!  Ick!\r\n";
            break;

         case 8:
            t = "Wow... trails!\r\n";
            break;

         case 9:
            t = "You reach for it, then notice the back of your hand is growing something!\r\n";
            break;

         case 10:
            t = "As you grasp it, it shatters into tiny shards which bite into your flesh!\r\n";
            break;

         case 11:
            t = "What about that huge dragon flying over your head?!?!?\r\n";
            break;

         case 12:
            t = "You stratch yourself instead...\r\n";
            break;

         case 13:
            t = "You hold the universe in the palm of your hand!\r\n";
            break;

         case 14:
            t = "You're too scared.\r\n";
            break;

         case 15:
            t = "Your mother smacks your hand... 'NO!'\r\n";
            break;

         case 16:
            t = "Your hand grasps the worst pile of revoltingness that you could ever imagine!\r\n";
            break;

         case 17:
            t = "You stop reaching for it as it screams out at you in pain!\r\n";
            break;

         case 18:
            t = "What about the millions of burrow-maggots feasting on your arm?!?!\r\n";
            break;

         case 19:
            t = "That doesn't matter anymore... you've found the true answer to everything!\r\n";
            break;

         case 20:
            t = "A supreme entity has no need for that.\r\n";
            break;
      }
   }
   else
   {
      int sub = URANGE( 1, abs( ms ) / 2 + drunk, 60 );

      switch( number_range( 1, sub / 10 ) )
      {
         default:
         case 1:
            t = "In just a second...\r\n";
            break;

         case 2:
            t = "You can't find that...\r\n";
            break;

         case 3:
            t = "It's just beyond your grasp...\r\n";
            break;

         case 4:
            t = "...but it's under a pile of other stuff...\r\n";
            break;

         case 5:
            t = "You go to reach for it, but pick your nose instead.\r\n";
            break;

         case 6:
            t = "Which one?!?  I see two... no three...\r\n";
            break;
      }
   }
   send_to_char( t, ch );
   return true;
}

/*
 * Generic get obj function that supports optional containers.	-Thoric
 * currently only used for "eat" and "quaff".
 */
OBJ_DATA *find_obj( CHAR_DATA *ch, char *argument, bool carryonly )
{
   char arg1[MIL];
   char arg2[MIL];
   OBJ_DATA *obj = NULL;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !str_cmp( arg2, "from" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg2[0] == '\0' )
   {
      if( carryonly && !( obj = get_obj_carry( ch, arg1 ) ) )
      {
         send_to_char( "You don't have that item.\r\n", ch );
         return NULL;
      }
      else if( !carryonly && !( obj = get_obj_here( ch, arg1 ) ) )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR );
         return NULL;
      }
      return obj;
   }
   else
   {
      OBJ_DATA *container = NULL;

      if( carryonly && !( container = get_obj_carry( ch, arg2 ) ) && !( container = get_obj_wear( ch, arg2 ) ) )
      {
         send_to_char( "You don't have that item.\r\n", ch );
         return NULL;
      }
      if( !carryonly && !( container = get_obj_here( ch, arg2 ) ) )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
         return NULL;
      }

      if( !is_obj_stat( container, ITEM_COVERING ) && IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return NULL;
      }

      obj = get_obj_list( ch, arg1, container->first_content );
      if( !obj )
         act( AT_PLAIN, is_obj_stat( container, ITEM_COVERING ) ?
              "I see nothing like that beneath $p." : "I see nothing like that in $p.", ch, container, NULL, TO_CHAR );
      return obj;
   }
}

int get_obj_number( OBJ_DATA *obj )
{
   return obj->count;
}

/* Return true if an object is, or nested inside a magic container */
bool in_magic_container( OBJ_DATA *obj )
{
   if( obj->item_type == ITEM_CONTAINER && is_obj_stat( obj, ITEM_MAGIC ) )
      return true;
   if( obj->in_obj )
      return in_magic_container( obj->in_obj );
   return false;
}

/* Return weight of an object, including weight of contents (unless magic). */
int get_obj_weight( OBJ_DATA *obj )
{
   int weight;

   weight = obj->count * obj->weight;

   /* magic containers */
   if( obj->item_type != ITEM_CONTAINER || !is_obj_stat( obj, ITEM_MAGIC ) )
      for( obj = obj->first_content; obj; obj = obj->next_content )
         weight += get_obj_weight( obj );

   return weight;
}

const char *dis_class_name( int Class )
{
   if( Class < 0 || Class >= MAX_PC_CLASS || !class_table[Class] || !class_table[Class]->name )
      return "Unknown";
   return class_table[Class]->name;
}

/* Display the main class name for the character */
const char *dis_main_class_name( CHAR_DATA *ch )
{
   MCLASS_DATA *mclass;
   int uclass = -1, ulevel = -1;
   double uexp = 0;

   if( !ch || is_npc( ch ) )
      return (char *)"Unknown";
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      if( mclass->wclass < 0 || mclass->wclass >= MAX_PC_CLASS || !class_table[mclass->wclass] || !class_table[mclass->wclass]->name )
         continue;
      if( uclass == -1 || ulevel < mclass->level || ( ulevel == mclass->level && uexp < mclass->exp ) )
      {
         uclass = mclass->wclass;
         ulevel = mclass->level;
         uexp = mclass->exp;
      }
   }
   if( uclass < 0 || uclass >= MAX_PC_CLASS || !class_table[uclass] || !class_table[uclass]->name )
      return "Unknown";
   return class_table[uclass]->name;
}

const char *dis_race_name( int race )
{
   if( race < 0 || race >= MAX_PC_RACE || !race_table[race] || !race_table[race]->name )
      return (char *)"Unknown";
   return race_table[race]->name;
}

/* Return real weight of an object, including weight of contents. */
int get_real_obj_weight( OBJ_DATA *obj )
{
   int weight;

   weight = obj->count * obj->weight;

   for( obj = obj->first_content; obj; obj = obj->next_content )
      weight += get_real_obj_weight( obj );

   return weight;
}

/* True if room is dark. */
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
   if( !pRoomIndex )
   {
      bug( "%s:: NULL pRoomIndex", __FUNCTION__ );
      return true;
   }

   if( pRoomIndex->light > 0 || xIS_SET( pRoomIndex->room_flags, ROOM_LIGHT ) )
      return false;

   if( xIS_SET( pRoomIndex->room_flags, ROOM_DARK ) )
      return true;

   if( pRoomIndex->sector_type == SECT_INSIDE || pRoomIndex->sector_type == SECT_CITY )
      return false;

   if( time_info.sunlight == SUN_SET || time_info.sunlight == SUN_DARK )
      return true;

   return false;
}

/*
 * If room is "do not disturb" return the pointer to the imm with dnd flag
 * NULL if room is not "do not disturb".
 */
CHAR_DATA *room_is_dnd( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
   CHAR_DATA *rch;

   if( !pRoomIndex )
   {
      bug( "%s: NULL pRoomIndex", __FUNCTION__ );
      return NULL;
   }

   if( !xIS_SET( pRoomIndex->room_flags, ROOM_DND ) )
      return NULL;

   for( rch = pRoomIndex->first_person; rch; rch = rch->next_in_room )
   {
      if( !is_npc( rch ) && rch->pcdata && is_immortal( rch )
      && xIS_SET( rch->pcdata->flags, PCFLAG_DND )
      && get_trust( ch ) < get_trust( rch ) && can_see( ch, rch ) )
         return rch;
   }
   return NULL;
}

/* True if room is private. */
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex )
{
   CHAR_DATA *rch;
   int count;

   if( !pRoomIndex )
   {
      bug( "%s: NULL pRoomIndex", __FUNCTION__ );
      return false;
   }

   count = 0;
   for( rch = pRoomIndex->first_person; rch; rch = rch->next_in_room )
      count++;

   if( xIS_SET( pRoomIndex->room_flags, ROOM_PRIVATE ) && count >= 2 )
      return true;

   if( xIS_SET( pRoomIndex->room_flags, ROOM_SOLITARY ) && count >= 1 )
      return true;

   return false;
}

/* True if char can see victim. */
bool can_see( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !victim )
      return false;

   if( !ch )
   {
      if( IS_AFFECTED( victim, AFF_INVISIBLE ) || IS_AFFECTED( victim, AFF_HIDE ) || xIS_SET( victim->act, PLR_WIZINVIS ) )
         return false;
      else
         return true;
   }

   if( ch == victim )
      return true;

   /* Wizinvis goes by get_trust */
   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_WIZINVIS ) && get_trust( ch ) < victim->pcdata->wizinvis )
      return false;

   /* Mobinvis goes by level but shows to all immortals */
   if( is_npc( victim ) && xIS_SET( victim->act, ACT_MOBINVIS ) && ch->level < victim->mobinvis && !is_immortal( ch ) )
      return false;

   /* Deadlies link-dead over 2 ticks aren't seen by mortals -- Blodkai */
   if( !is_immortal( ch ) && !is_npc( ch ) && !is_npc( victim ) && is_pkill( victim ) && victim->timer > 1 && !victim->desc )
      return false;

   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_HOLYLIGHT ) )
      return true;

   /* The miracle cure for blindness? -- Altrag */
   if( !IS_AFFECTED( ch, AFF_TRUESIGHT ) )
   {
      if( IS_AFFECTED( ch, AFF_BLIND ) )
         return false;

      if( room_is_dark( ch->in_room ) && !IS_AFFECTED( ch, AFF_INFRARED ) )
         return false;

      if( IS_AFFECTED( victim, AFF_INVISIBLE ) && !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
         return false;

      if( IS_AFFECTED( victim, AFF_HIDE )
          && !IS_AFFECTED( ch, AFF_DETECT_HIDDEN )
          && !victim->fighting && ( is_npc( ch ) ? !is_npc( victim ) : is_npc( victim ) ) )
         return false;
   }

   return true;
}

/* True if char can see obj. */
bool can_see_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( obj->in_room && obj->in_room == ch->in_room && xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
   {
      if( is_obj_stat( obj, ITEM_WILDERNESS ) ) /* In the wilderness */
      {
         if( is_npc( ch ) && ch->pIndexData->vnum == MOB_VNUM_SUPERMOB )
            return true;

         if( !obj->in_room  || !ch->in_room || obj->in_room != ch->in_room
         || !xIS_SET( obj->in_room->room_flags, ROOM_WILDERNESS ) )
            return false;

         if( !is_in_wilderness( ch ) )
            return false;

         if( obj->cords[0] != ch->cords[0] || obj->cords[1] != ch->cords[1] )
            return false;
      }
      else /* Not in the wilderness */
      {
         if( is_in_wilderness( ch ) )
            return false;
      }
   }

   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_HOLYLIGHT ) )
      return true;

   if( is_npc( ch ) && ch->pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return true;

   if( is_obj_stat( obj, ITEM_BURIED ) )
      return false;

   if( is_obj_stat( obj, ITEM_HIDDEN ) )
      return false;

   if( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
      return true;

   if( IS_AFFECTED( ch, AFF_BLIND ) )
      return false;

   /* can see lights in the dark */
   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
      return true;

   if( room_is_dark( ch->in_room ) )
   {
      if( is_obj_stat( obj, ITEM_GLOW ) )
         return true;
      if( !IS_AFFECTED( ch, AFF_INFRARED ) )
         return false;
   }

   if( is_obj_stat( obj, ITEM_INVIS ) && !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
      return false;

   return true;
}

/* True if char can drop obj. */
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( !is_obj_stat( obj, ITEM_NODROP ) )
      return true;

   if( !is_npc( ch ) && get_trust( ch ) >= PERM_IMM )
      return true;

   if( is_npc( ch ) && ch->pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return true;

   return false;
}

bool can_drop_room( CHAR_DATA *ch )
{
   if( !ch || !ch->in_room )
      return false;

   if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) && ch != supermob )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "A magical force stops you!\r\n", ch );
      set_char_color( AT_TELL, ch );
      send_to_char( "Someone tells you, 'No littering here!'\r\n", ch );
      return false;
   }

   return true;
}

/* Return ascii name of an affect location. */
char *affect_loc_name( int location )
{
   if( location >= 0 && location < APPLY_MAX )
      return capitalize( a_types[location % REVERSE_APPLY] );

   bug( "%s: unknown location %d.", __FUNCTION__, location );
   return (char *)"(Unknown)";
}

/* Return ascii name of pulltype exit setting. */
const char *pull_type_name( int pulltype )
{
   if( pulltype >= PT_FIRE )
      return ex_pfire[pulltype - PT_FIRE];
   if( pulltype >= PT_AIR )
      return ex_pair[pulltype - PT_AIR];
   if( pulltype >= PT_EARTH )
      return ex_pearth[pulltype - PT_EARTH];
   if( pulltype >= PT_WATER )
      return ex_pwater[pulltype - PT_WATER];
   if( pulltype < 0 )
      return "ERROR";

   return ex_pmisc[pulltype];
}

/* Set off a trap (obj) upon character (ch) -Thoric */
ch_ret spring_trap( CHAR_DATA *ch, OBJ_DATA *obj )
{
   int dam, typ, lev;
   const char *txt;
   char buf[MSL];
   ch_ret retcode;

   typ = obj->value[1];
   lev = obj->value[2];

   retcode = rNONE;

   switch( typ )
   {
      default:
         txt = "hit by a trap";
         break;

      case TRAP_TYPE_POISON_GAS:
         txt = "surrounded by a green cloud of gas";
         break;

      case TRAP_TYPE_POISON_DART:
         txt = "hit by a dart";
         break;

      case TRAP_TYPE_POISON_NEEDLE:
         txt = "pricked by a needle";
         break;

      case TRAP_TYPE_POISON_DAGGER:
         txt = "stabbed by a dagger";
         break;

      case TRAP_TYPE_POISON_ARROW:
         txt = "struck with an arrow";
         break;

      case TRAP_TYPE_BLINDNESS_GAS:
         txt = "surrounded by a red cloud of gas";
         break;

      case TRAP_TYPE_SLEEPING_GAS:
         txt = "surrounded by a yellow cloud of gas";
         break;

      case TRAP_TYPE_FLAME:
         txt = "struck by a burst of flame";
         break;

      case TRAP_TYPE_EXPLOSION:
         txt = "hit by an explosion";
         break;

      case TRAP_TYPE_ACID_SPRAY:
         txt = "covered by a spray of acid";
         break;

      case TRAP_TYPE_ELECTRIC_SHOCK:
         txt = "suddenly shocked";
         break;

      case TRAP_TYPE_BLADE:
         txt = "sliced by a razor sharp blade";
         break;
   }

   dam = number_range( obj->value[2], obj->value[2] * 2 );
   snprintf( buf, sizeof( buf ), "You're %s!", txt );
   act( AT_HITME, buf, ch, NULL, NULL, TO_CHAR );
   snprintf( buf, sizeof( buf ), "$n is %s.", txt );
   act( AT_ACTION, buf, ch, NULL, NULL, TO_ROOM );

   if( --obj->value[0] <= 0 )
      extract_obj( obj );

   switch( typ )
   {
      default:
      case TRAP_TYPE_POISON_DART:
      case TRAP_TYPE_POISON_NEEDLE:
      case TRAP_TYPE_POISON_DAGGER:
      case TRAP_TYPE_POISON_ARROW:
         retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
         if( retcode == rNONE )
            retcode = damage( ch, ch, NULL, dam, TYPE_UNDEFINED, true );
         break;

      case TRAP_TYPE_POISON_GAS:
         retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_BLINDNESS_GAS:
         retcode = obj_cast_spell( gsn_blindness, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_SLEEPING_GAS:
         retcode = obj_cast_spell( skill_lookup( "sleep" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_ACID_SPRAY:
         retcode = obj_cast_spell( skill_lookup( "acidshot" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_FLAME:
      case TRAP_TYPE_EXPLOSION:
         retcode = obj_cast_spell( skill_lookup( "fireball" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_ELECTRIC_SHOCK:
      case TRAP_TYPE_BLADE:
         retcode = damage( ch, ch, NULL, dam, TYPE_UNDEFINED, true );
         break;
   }
   return retcode;
}

/* Check an object for a trap - Thoric */
ch_ret check_for_trap( CHAR_DATA *ch, OBJ_DATA *obj, int flag )
{
   OBJ_DATA *check;
   ch_ret retcode = rNONE;

   if( !obj->first_content )
      return rNONE;

   for( check = obj->first_content; check; check = check->next_content )
   {
      if( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) )
      {
         retcode = spring_trap( ch, check );
         if( retcode != rNONE )
            return retcode;
      }
   }
   return retcode;
}

ch_ret check_room_for_traps( CHAR_DATA *ch, int flag )
{
   OBJ_DATA *check;
   ch_ret retcode = rNONE;

   if( !ch )
      return rERROR;

   if( !ch->in_room || !ch->in_room->first_content )
      return rNONE;

   for( check = ch->in_room->first_content; check; check = check->next_content )
   {
      if( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) )
      {
         retcode = spring_trap( ch, check );
         if( retcode != rNONE )
            return retcode;
      }
   }
   return retcode;
}

/* return true if an object contains a trap - Thoric */
bool is_trapped( OBJ_DATA *obj )
{
   OBJ_DATA *check;

   if( !obj->first_content )
      return false;

   for( check = obj->first_content; check; check = check->next_content )
      if( check->item_type == ITEM_TRAP && check->value[3] != 0 )
         return true;

   return false;
}

/* If a room contains a trap, return the pointer to the trap */
OBJ_DATA *get_room_trap( ROOM_INDEX_DATA *room )
{
   OBJ_DATA *check;

   if( !room || !room->first_content )
      return NULL;

   for( check = room->first_content; check; check = check->next_content )
      if( check->item_type == ITEM_TRAP && check->value[3] != 0 )
         return check;

   return NULL;
}

/* If an object contains a trap, return the pointer to the trap - Thoric */
OBJ_DATA *get_trap( OBJ_DATA *obj )
{
   OBJ_DATA *check;

   if( !obj->first_content )
      return NULL;

   for( check = obj->first_content; check; check = check->next_content )
      if( check->item_type == ITEM_TRAP && check->value[3] != 0 )
         return check;

   return NULL;
}

/*
 * Return a pointer to the first object of a certain type found that
 * a player is carrying/wearing
 */
OBJ_DATA *get_objtype( CHAR_DATA *ch, short type )
{
   OBJ_DATA *obj;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->item_type == type )
         return obj;

   return NULL;
}

/* Remove an exit from a room - Thoric */
void extract_exit( ROOM_INDEX_DATA *room, EXIT_DATA *pexit )
{
   UNLINK( pexit, room->first_exit, room->last_exit, next, prev );
   if( pexit->rexit )
      pexit->rexit->rexit = NULL;
   STRFREE( pexit->keyword );
   STRFREE( pexit->description );
   DISPOSE( pexit );
}

void clean_room( ROOM_INDEX_DATA *room )
{
   EXTRA_DESCR_DATA *ed, *ed_next;
   EXIT_DATA *pexit, *pexit_next;
   MPROG_DATA *mprog, *mprog_next;

   STRFREE( room->description );
   STRFREE( room->name );
   for( mprog = room->mudprogs; mprog; mprog = mprog_next )
   {
      mprog_next = mprog->next;
      STRFREE( mprog->arglist );
      STRFREE( mprog->comlist );
      DISPOSE( mprog );
   }
   for( ed = room->first_extradesc; ed; ed = ed_next )
   {
      ed_next = ed->next;
      STRFREE( ed->description );
      STRFREE( ed->keyword );
      DISPOSE( ed );
      top_ed--;
   }
   room->first_extradesc = NULL;
   room->last_extradesc = NULL;
   for( pexit = room->first_exit; pexit; pexit = pexit_next )
   {
      pexit_next = pexit->next;
      extract_exit( room, pexit );
      top_exit--;
   }
   room->first_exit = NULL;
   room->last_exit = NULL;
   xCLEAR_BITS( room->room_flags );
   room->sector_type = 0;
   room->light = 0;
}

void clean_obj( OBJ_INDEX_DATA *obj )
{
   AFFECT_DATA *paf, *paf_next;
   EXTRA_DESCR_DATA *ed, *ed_next;
   MPROG_DATA *mprog, *mprog_next;

   STRFREE( obj->name );
   STRFREE( obj->short_descr );
   STRFREE( obj->description );
   STRFREE( obj->action_desc );
   obj->item_type = 0;
   xCLEAR_BITS( obj->extra_flags );
   xCLEAR_BITS( obj->wear_flags );
   obj->count = 0;
   obj->weight = 0;
   obj->cost = 0;
   obj->value[0] = 0;
   obj->value[1] = 0;
   obj->value[2] = 0;
   obj->value[3] = 0;
   obj->value[4] = 0;
   obj->value[5] = 0;
   for( paf = obj->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      DISPOSE( paf );
      top_affect--;
   }
   obj->first_affect = NULL;
   obj->last_affect = NULL;
   for( ed = obj->first_extradesc; ed; ed = ed_next )
   {
      ed_next = ed->next;
      STRFREE( ed->description );
      STRFREE( ed->keyword );
      DISPOSE( ed );
      top_ed--;
   }
   obj->first_extradesc = NULL;
   obj->last_extradesc = NULL;
   for( mprog = obj->mudprogs; mprog; mprog = mprog_next )
   {
      mprog_next = mprog->next;
      STRFREE( mprog->arglist );
      STRFREE( mprog->comlist );
      DISPOSE( mprog );
   }
}

/* clean out a mobile (index) (leave list pointers intact ) - Thoric */
void clean_mob( MOB_INDEX_DATA *mob )
{
   MPROG_DATA *mprog, *mprog_next;

   STRFREE( mob->name );
   STRFREE( mob->short_descr );
   STRFREE( mob->long_descr );
   STRFREE( mob->description );
   mob->spec_fun = NULL;
   mob->pShop = NULL;
   mob->rShop = NULL;
   xCLEAR_BITS( mob->progtypes );

   for( mprog = mob->mudprogs; mprog; mprog = mprog_next )
   {
      mprog_next = mprog->next;
      STRFREE( mprog->arglist );
      STRFREE( mprog->comlist );
      DISPOSE( mprog );
   }
   mob->count = 0;
   mob->killed = 0;
   mob->sex = 0;
   mob->level = 0;
   xCLEAR_BITS( mob->act );
   xCLEAR_BITS( mob->affected_by );
   mob->alignment = 0;
   mob->ac = 0;
   mob->minhit = 0;
   mob->maxhit = 0;
   mob->gold = 0;
   mob->defposition = 0;
   mob->height = 0;
   mob->weight = 0;
   xCLEAR_BITS( mob->attacks );
   xCLEAR_BITS( mob->defenses );
}

/* Set up players base stats */
void set_base_stats( CHAR_DATA *ch )
{
   int stat;

   if( !ch )
      return;
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( race_table[ch->race] )
         ch->perm_stats[stat] = race_table[ch->race]->base_stats[stat];
      else
         ch->perm_stats[stat] = 13;
   }
   if( race_table[ch->race] )
   {
      ch->max_hit = race_table[ch->race]->hit;
      ch->max_mana = race_table[ch->race]->mana;
      ch->max_move = race_table[ch->race]->move;
      ch->height = number_range( race_table[ch->race]->minheight, race_table[ch->race]->maxheight );
      ch->weight = number_range( race_table[ch->race]->minweight, race_table[ch->race]->maxweight );
      ch->alignment = URANGE( race_table[ch->race]->minalign, race_table[ch->race]->alignment, race_table[ch->race]->maxalign );
      ch->armor = race_table[ch->race]->ac_plus;
   }
   else
   {
      ch->max_hit = 100;
      ch->max_mana = 100;
      ch->max_move = 100;
      ch->height = number_range( 50, 100 );
      ch->weight = number_range( 150, 300 );
      ch->alignment = 0;
      ch->armor = 0;
   }

   ch->hit = UMAX( 1, ch->max_hit );
   ch->mana = UMAX( 1, ch->max_mana );
   ch->move = UMAX( 1, ch->max_move );
}

bool is_obj_stat( OBJ_DATA *obj, int stat )
{
   if( xIS_SET( obj->extra_flags, stat ) )
      return true;
   return false;
}

bool can_wear( OBJ_DATA *obj, int part )
{
   if( xIS_SET( obj->wear_flags, part ) )
      return true;
   return false;
}

bool is_retired( CHAR_DATA *ch )
{
   if( ch->pcdata && xIS_SET( ch->pcdata->flags, PCFLAG_RETIRED ) )
      return true;
   return false;
}

bool is_guest( CHAR_DATA *ch )
{
   if( ch->pcdata && xIS_SET( ch->pcdata->flags, PCFLAG_GUEST ) )
      return true;
   return false;
}

bool is_idle( CHAR_DATA *ch )
{
   if( ch->pcdata && xIS_SET( ch->pcdata->flags, PCFLAG_IDLE ) )
      return true;
   return false;
}

bool is_pkill( CHAR_DATA *ch )
{
   if( ch->pcdata && xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
      return true;
   return false;
}

bool can_pkill( CHAR_DATA *ch )
{
   if( is_pkill( ch ) && ch->level >= 5 && get_age( ch ) >= 18 )
      return true;
   return false;
}

bool is_awake( CHAR_DATA *ch )
{
   if( ch->position > POS_SLEEPING )
      return true;
   return false;
}

bool is_outside( CHAR_DATA *ch )
{
   if( !ch || !ch->in_room )
      return false;

   if( ch->in_room->sector_type != SECT_INSIDE
   && !xIS_SET( ch->in_room->room_flags, ROOM_INDOORS )
   && !xIS_SET( ch->in_room->room_flags, ROOM_TUNNEL ) )
      return true;

   return false;
}

bool no_weather_sect( ROOM_INDEX_DATA *room )
{
   if( !room )
      return false;

   if( room->sector_type == SECT_INSIDE || room->sector_type == SECT_UNDERWATER
   || room->sector_type == SECT_OCEANFLOOR || room->sector_type == SECT_UNDERGROUND )
      return true;

   return false;
}

bool is_drunk( CHAR_DATA *ch, int drunk )
{
   if( number_percent( ) < ( ( ch->pcdata->condition[COND_DRUNK] * 2 ) / drunk ) )
      return true;
   return false;
}

bool is_devoted( CHAR_DATA *ch )
{
   if( !is_npc( ch ) && ch->pcdata->deity )
      return true;
   return false;
}

bool is_vampire( CHAR_DATA *ch )
{
   if( !is_npc( ch ) && race_table[ch->race] && race_table[ch->race]->uses == 2 )
      return true;
   return false;
}

bool is_good( CHAR_DATA *ch )
{
   if( ch->alignment >= 350 )
      return true;
   return false;
}

bool is_evil( CHAR_DATA *ch )
{
   if( ch->alignment <= -350 )
      return true;
   return false;
}

bool is_neutral( CHAR_DATA *ch )
{
   if( !is_good( ch ) && !is_evil( ch ) )
      return true;
   return false;
}

bool can_cast( CHAR_DATA *ch )
{
   if( race_table[ch->race] && ( race_table[ch->race]->uses == 1 || race_table[ch->race]->uses == 2 ) )
      return true;
   return false;
}

bool in_clan( CHAR_DATA *ch )
{
   if( !is_npc( ch ) && ch->pcdata->clan )
      return true;
   return false;
}

bool is_clanned( CHAR_DATA *ch )
{
   if( in_clan( ch ) && ch->pcdata->clan->clan_type == CLAN_PLAIN )
      return true;
   return false;
}

bool in_nation( CHAR_DATA *ch )
{
   if( !is_npc( ch ) && ch->pcdata->nation )
      return true;
   return false;
}

bool is_nationed( CHAR_DATA *ch )
{
   if( in_nation( ch ) && ch->pcdata->nation->clan_type == CLAN_NATION )
      return true;
   return false;
}

bool in_council( CHAR_DATA *ch )
{
   if( !is_npc( ch ) && ch->pcdata->council )
      return true;
   return false;
}

bool is_counciled( CHAR_DATA *ch )
{
   if( in_council( ch ) )
      return true;
   return false;
}

bool is_pacifist( CHAR_DATA *ch )
{
   if( is_npc( ch ) && xIS_SET( ch->act, ACT_PACIFIST ) )
      return true;
   return false;
}

bool is_valid_herb( int sn )
{
   if( sn < 0 || sn >= MAX_HERB )
      return false;
   if( !herb_table[sn] || !herb_table[sn]->name )
      return false;
   return true;
}

bool is_valid_pers( int sn )
{
   if( sn < 0 || sn >= MAX_PERS )
      return false;
   if( !pers_table[sn] || !pers_table[sn]->name )
      return false;
   return true;
}

bool is_valid_sn( int sn )
{
   if( sn < 0 || sn >= MAX_SKILL )
      return false;
   if( !skill_table[sn] || !skill_table[sn]->name )
      return false;
   return true;
}

void wait_state( CHAR_DATA *ch, int npulse )
{
   if( !is_npc( ch ) && is_immortal( ch ) )
      ch->wait = 1;
   else
      ch->wait = UMAX( ch->wait, npulse );
}

bool can_go( ROOM_INDEX_DATA *room, int door )
{
   EXIT_DATA *exit;

   if( !room || !( exit = get_exit( room, door ) ) )
      return false;
   if( !exit->to_room || xIS_SET( exit->exit_info, EX_CLOSED ) )
      return false;
   return true;
}

bool is_floating( CHAR_DATA *ch )
{
   if( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLOATING ) )
      return false;
   return true;
}

/* Set the current global character to ch - Thoric */
void set_cur_char( CHAR_DATA *ch )
{
   cur_char = ch;
   cur_char_died = false;
   global_retcode = rNONE;
}

/* Check to see if ch died recently - Thoric */
bool char_died( CHAR_DATA *ch )
{
   EXTRACT_CHAR_DATA *ccd;

   if( ch == cur_char && cur_char_died )
      return true;

   for( ccd = extracted_char_queue; ccd; ccd = ccd->next )
      if( ccd->ch == ch )
         return true;
   return false;
}

/* Add ch to the queue of recently extracted characters - Thoric */
void queue_extracted_char( CHAR_DATA *ch, bool extract )
{
   EXTRACT_CHAR_DATA *ccd;

   if( !ch )
   {
      bug( "%s: ch = NULL", __FUNCTION__ );
      return;
   }
   CREATE( ccd, EXTRACT_CHAR_DATA, 1 );
   ccd->ch = ch;
   ccd->room = ch->in_room;
   ccd->extract = extract;
   if( ch == cur_char )
      ccd->retcode = global_retcode;
   else
      ccd->retcode = rCHAR_DIED;
   ccd->next = extracted_char_queue;
   extracted_char_queue = ccd;
   cur_qchars++;
}

/* clean out the extracted character queue */
void clean_char_queue( void )
{
   EXTRACT_CHAR_DATA *ccd;

   for( ccd = extracted_char_queue; ccd; ccd = extracted_char_queue )
   {
      extracted_char_queue = ccd->next;
      if( ccd->extract )
         free_char( ccd->ch );
      DISPOSE( ccd );
      --cur_qchars;
   }
}

/*
 * Add a timer to ch						-Thoric
 * Support for "call back" time delayed commands
 */
void add_timer( CHAR_DATA *ch, short type, int count, DO_FUN *fun, int value )
{
   TIMER *timer;
    
   for( timer = ch->first_timer; timer; timer = timer->next )
   {
      if( timer->type == type )
      {
         timer->count = count;
         timer->do_fun = fun;
         timer->value = value;
         break;
      }
   }
   if( !timer )
   {
      CREATE( timer, TIMER, 1 );
      timer->count = count;
      timer->type = type;
      timer->do_fun = fun;
      timer->value = value;
      LINK( timer, ch->first_timer, ch->last_timer, next, prev );
   }
}

TIMER *get_timerptr( CHAR_DATA *ch, short type )
{
   TIMER *timer;

   for( timer = ch->first_timer; timer; timer = timer->next )
      if( timer->type == type )
         return timer;
   return NULL;
}

short get_timer( CHAR_DATA *ch, short type )
{
   TIMER *timer;

   if( ( timer = get_timerptr( ch, type ) ) )
      return timer->count;
   else
      return 0;
}

void extract_timer( CHAR_DATA *ch, TIMER *timer )
{
   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return;
   }
   if( !timer )
   {
      bug( "%s: NULL timer", __FUNCTION__ );
      return;
   }

   UNLINK( timer, ch->first_timer, ch->last_timer, next, prev );
   DISPOSE( timer );
}

void remove_timer( CHAR_DATA *ch, short type )
{
   TIMER *timer;

   for( timer = ch->first_timer; timer; timer = timer->next )
      if( timer->type == type )
         break;

   if( timer )
      extract_timer( ch, timer );
}

bool in_soft_range( CHAR_DATA *ch, AREA_DATA *tarea )
{
   if( is_immortal( ch ) )
      return true;
   else if( is_npc( ch ) )
      return true;
   else if( ch->level >= tarea->low_soft_range || ch->level <= tarea->hi_soft_range )
      return true;
   else
      return false;
}

bool can_astral( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( victim == ch
   || !victim->in_room
   || xIS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
   || xIS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
   || xIS_SET( victim->in_room->room_flags, ROOM_DEATH )
   || xIS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
   || victim->level >= ch->level + 15
   || ( can_pkill( victim ) && !is_npc( ch ) && !can_pkill( ch ) )
   || ( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
   || ( is_npc( victim ) && saves_spell_staff( ch->level, victim ) )
   || ( xIS_SET( victim->in_room->area->flags, AFLAG_NOPKILL ) && is_pkill( ch ) ) )
      return false;
   else
      return true;
}

bool in_hard_range( CHAR_DATA *ch, AREA_DATA *tarea )
{
   if( is_immortal( ch ) )
      return true;
   else if( is_npc( ch ) )
      return true;
   else if( ch->level >= tarea->low_hard_range && ch->level <= tarea->hi_hard_range )
      return true;
   else
      return false;
}

/* Scryn, standard luck check 2/2/96 */
bool chance( CHAR_DATA *ch, short percent )
{
   short deity_factor, ms;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return false;
   }

   /*
    * Mental state bonus/penalty:  Your mental state is a ranged value with
    * zero (0) being at a perfect mental state (bonus of 10).
    * negative values would reflect how sedated one is, and
    * positive values would reflect how stimulated one is.
    * In most circumstances you'd do best at a perfectly balanced state.
    */
   if( is_devoted( ch ) )
      deity_factor = ch->pcdata->favor / -500;
   else
      deity_factor = 0;

   ms = 10 - abs( ch->mental_state );

   if( ( ( number_percent( ) * 3 ) - get_curr_lck( ch ) - ms ) + deity_factor <= percent )
      return true;
   else
      return false;
}

void clone_extradescs( OBJ_DATA *obj, OBJ_DATA *cobj )
{
   EXTRA_DESCR_DATA *ed, *ced;

   for( ced = cobj->first_extradesc; ced; ced = ced->next )
   {
      CREATE ( ed, EXTRA_DESCR_DATA, 1 );

      ed->keyword = STRALLOC( ced->keyword );
      ed->description = STRALLOC( ced->description );
      LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
      top_ed++;
   }
}

void clone_affects( OBJ_DATA *obj, OBJ_DATA *cobj )
{
   AFFECT_DATA *paf, *cpaf;

   for( cpaf = cobj->first_affect; cpaf; cpaf = cpaf->next )
   {
      CREATE ( paf, AFFECT_DATA, 1 );

      paf->type = cpaf->type;
      paf->duration = cpaf->duration;
      paf->location = cpaf->location;
      paf->modifier = cpaf->modifier;
      paf->bitvector = cpaf->bitvector;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      top_affect++;
   }
}

/* Make a simple clone of an object (no extras...yet) -Thoric */
OBJ_DATA *clone_object( OBJ_DATA *obj )
{
   OBJ_DATA *clone;

   CREATE( clone, OBJ_DATA, 1 );
   clone->pIndexData = obj->pIndexData;
   clone->name = QUICKLINK( obj->name );
   clone->short_descr = QUICKLINK( obj->short_descr );
   clone->description = QUICKLINK( obj->description );
   clone->action_desc = QUICKLINK( obj->action_desc );
   clone->owner = QUICKLINK( obj->owner );
   clone->item_type = obj->item_type;
   clone->extra_flags = obj->extra_flags;
   clone->wear_flags = obj->wear_flags;
   clone->wear_loc = obj->wear_loc;
   clone->t_wear_loc = obj->t_wear_loc;
   clone->weight = obj->weight;
   clone->cost = obj->cost;
   clone->level = obj->level;
   clone->timer = obj->timer;
   clone->value[0] = obj->value[0];
   clone->value[1] = obj->value[1];
   clone->value[2] = obj->value[2];
   clone->value[3] = obj->value[3];
   clone->value[4] = obj->value[4];
   clone->value[5] = obj->value[5];
   clone->count = 1;
   ++obj->pIndexData->count;
   ++numobjsloaded;
   ++physicalobjects;
   clone_extradescs( clone, obj );
   clone_affects( clone, obj );
   LINK( clone, first_object, last_object, next, prev );
   if( clone->pIndexData->vnum == OBJ_VNUM_CORPSE_PC )
   {
      LINK( clone, first_corpse, last_corpse, next_corpse, prev_corpse );
      ++num_corpses;
   }
   LINK( clone, clone->pIndexData->first_copy, clone->pIndexData->last_copy, next_index, prev_index );
   return clone;
}

bool has_same_affects( OBJ_DATA *obj, OBJ_DATA *cobj )
{
   AFFECT_DATA *paf, *cpaf;

   paf = obj->first_affect;
   cpaf = cobj->first_affect;

   if( !paf && !cpaf )
      return true;
   if( paf && !cpaf )
      return false;
   if( !paf && cpaf )
      return false;
   for( ; paf && cpaf; paf = paf->next, cpaf = cpaf->next )
   {
      if( !paf->next && cpaf->next )
         return false;
      if( paf->next && !cpaf->next )
         return false;
      if( paf->type != cpaf->type )
         return false;
      if( paf->duration != cpaf->duration )
         return false;
      if( paf->location != cpaf->location )
         return false;
      if( paf->modifier != cpaf->modifier )
         return false;
      if( !xSAME_BITS( paf->bitvector, cpaf->bitvector ) )
         return false;
   }

   return true;
}

/* Compare the extra descriptions of two objects and see if they match */
bool has_same_extradescs( OBJ_DATA *obj, OBJ_DATA *nobj )
{
   EXTRA_DESCR_DATA *ed, *ned;
   char *argument, *nargument;
   char arg[MSL], narg[MSL];

   if( !obj->first_extradesc && !nobj->first_extradesc )
      return true;
   if( !obj->first_extradesc && nobj->first_extradesc )
      return false;
   if( obj->first_extradesc && !nobj->first_extradesc )
      return false;

   /* Check each extra description */
   for( ed = obj->first_extradesc, ned = nobj->first_extradesc; ed && ned; ed = ed->next, ned = ned->next )
   {
      /* If this is it for one and not the other return FALSE */
      if( ed->next && !ned->next )
         return false;
      if( !ed->next && ned->next )
         return false;

      /* Check to make sure keywords match and are in the same places etc... */
      argument = ed->keyword;
      nargument = ned->keyword;
      for( ;; )
      {
         argument = one_argument( argument, arg );
         nargument = one_argument( nargument, narg );
         if( arg[0] == '\0' && narg[0] == '\0' )
            break;
         if( arg[0] == '\0' && narg[0] != '\0' )
            return false;
         if( arg[0] != '\0' && narg[0] == '\0' )
            return false;
         if( str_cmp( arg, narg ) )
            return false;
      }

      /* Check to make sure the descriptions line up exactly */
      argument = ed->description;
      nargument = ned->description;
      for( ;; )
      {
         argument = one_argument( argument, arg );
         nargument = one_argument( nargument, narg );
         if( arg[0] == '\0' && narg[0] == '\0' )
            break;
         if( arg[0] == '\0' && narg[0] != '\0' )
            return false;
         if( arg[0] != '\0' && narg[0] == '\0' )
            return false;
         if( str_cmp( arg, narg ) )
            return false;
      }
   }

   /* Well looks like it all matches up so allow them to be groupped */
   return true;
}

/*
 * If possible group obj2 into obj1				-Thoric
 * This code, along with clone_object, obj->count, and special support
 * for it implemented throughout handler.c and save.c should show improved
 * performance on MUDs with players that hoard tons of potions and scrolls
 * as this will allow them to be grouped together both in memory, and in
 * the player files.
 */
OBJ_DATA *group_object( OBJ_DATA *obj1, OBJ_DATA *obj2 )
{
   if( !obj1 || !obj2 )
      return NULL;
   if( obj1 == obj2 )
      return obj1;
   if( obj1->pIndexData == obj2->pIndexData
   && !str_cmp( obj1->name, obj2->name )
   && !str_cmp( obj1->short_descr, obj2->short_descr )
   && !str_cmp( obj1->description, obj2->description )
   && !str_cmp( obj1->action_desc, obj2->action_desc )
   && !str_cmp( obj1->owner, obj2->owner )
   && obj1->bsplatter == obj2->bsplatter
   && obj1->bstain == obj2->bstain
   && obj1->item_type == obj2->item_type 
   && !xIS_SET( obj1->extra_flags, ITEM_NOGROUP )
   && !xIS_SET( obj2->extra_flags, ITEM_NOGROUP )
   && xSAME_BITS( obj1->extra_flags, obj2->extra_flags )
   && xSAME_BITS( obj1->wear_flags, obj2->wear_flags )
   && obj1->wear_loc == obj2->wear_loc
   && obj1->t_wear_loc == obj2->t_wear_loc
   && obj1->weight == obj2->weight
   && obj1->cost == obj2->cost
   && obj1->level == obj2->level
   && obj1->timer == obj2->timer
   && obj1->value[0] == obj2->value[0]
   && obj1->value[1] == obj2->value[1]
   && obj1->value[2] == obj2->value[2]
   && obj1->value[3] == obj2->value[3]
   && obj1->value[4] == obj2->value[4]
   && obj1->value[5] == obj2->value[5]
   && !obj1->first_affect && !obj2->first_affect
   && !obj1->first_content && !obj2->first_content
   && has_same_extradescs( obj1, obj2 )
   && ( ( !xIS_SET( obj1->extra_flags, ITEM_WILDERNESS ) && !xIS_SET( obj2->extra_flags, ITEM_WILDERNESS ) )
      || ( obj1->cords[0] == obj2->cords[0] && obj1->cords[1] == obj2->cords[1] ) )
   && obj1->count + obj2->count > 0 )   /* prevent count overflow */
   {
      obj1->count += obj2->count;
      obj1->pIndexData->count += obj2->count;   /* to be decremented in */
      numobjsloaded += obj2->count; /* extract_obj */
      extract_obj( obj2 );
      return obj1;
   }
   return obj2;
}

/*
 * Split off a grouped object - Thoric
 * decreased obj's count to num, and creates a new object containing the rest
 */
void split_obj( OBJ_DATA *obj, int num )
{
   int count;
   OBJ_DATA *rest;

   if( !obj )
      return;

   count = obj->count;
   if( count <= num || num == 0 )
      return;

   rest = clone_object( obj );
   --obj->pIndexData->count;  /* since clone_object() ups this value */
   --numobjsloaded;
   rest->count = obj->count - num;
   obj->count = num;

   if( obj->carried_by )
   {
      LINK( rest, obj->carried_by->first_carrying, obj->carried_by->last_carrying, next_content, prev_content );
      rest->carried_by = obj->carried_by;
      rest->in_room = NULL;
      rest->in_obj = NULL;
   }
   else if( obj->in_room )
   {
      LINK( rest, obj->in_room->first_content, obj->in_room->last_content, next_content, prev_content );
      obj->in_room->objcount++;
      rest->carried_by = NULL;
      rest->in_room = obj->in_room;
      rest->in_obj = NULL;
   }
   else if( obj->in_obj )
   {
      LINK( rest, obj->in_obj->first_content, obj->in_obj->last_content, next_content, prev_content );
      rest->in_obj = obj->in_obj;
      rest->in_room = NULL;
      rest->carried_by = NULL;
   }
}

void separate_obj( OBJ_DATA *obj )
{
   if( !obj )
      return;
   split_obj( obj, 1 );
}

/* Empty an obj's contents... optionally into another obj, or a room */
bool empty_obj( OBJ_DATA *obj, OBJ_DATA *destobj, ROOM_INDEX_DATA *destroom )
{
   OBJ_DATA *otmp, *otmp_next;
   CHAR_DATA *ch = obj->carried_by;
   bool movedsome = false;

   if( !obj )
   {
      bug( "%s: NULL obj", __FUNCTION__ );
      return false;
   }
   if( destobj || ( !destroom && !ch && ( destobj = obj->in_obj ) ) )
   {
      for( otmp = obj->first_content; otmp; otmp = otmp_next )
      {
         otmp_next = otmp->next_content;
         /* only keys on a keyring */
         if( destobj->item_type == ITEM_KEYRING && otmp->item_type != ITEM_KEY )
            continue;
         if( destobj->item_type == ITEM_QUIVER && otmp->item_type != ITEM_PROJECTILE )
            continue;
         if( ( destobj->item_type == ITEM_CONTAINER || destobj->item_type == ITEM_KEYRING
         || destobj->item_type == ITEM_QUIVER )
         && get_real_obj_weight( otmp ) + get_real_obj_weight( destobj ) > destobj->value[0] )
            continue;
         obj_from_obj( otmp );
         obj_to_obj( otmp, destobj );
         movedsome = true;
      }
      return movedsome;
   }
   if( destroom || ( !ch && ( destroom = obj->in_room ) ) )
   {
      for( otmp = obj->first_content; otmp; otmp = otmp_next )
      {
         otmp_next = otmp->next_content;
         if( ch && HAS_PROG( otmp->pIndexData, DROP_PROG ) && otmp->count > 1 )
         {
            separate_obj( otmp );
            obj_from_obj( otmp );
            if( !otmp_next )
               otmp_next = obj->first_content;
         }
         else
            obj_from_obj( otmp );
         otmp = obj_to_room( otmp, destroom );

         if( otmp )
         {
            if( is_obj_stat( obj, ITEM_WILDERNESS ) )
            {
               xSET_BIT( otmp->extra_flags, ITEM_WILDERNESS );
               otmp->cords[0] = obj->cords[0];
               otmp->cords[1] = obj->cords[1];
            }
            else if( ch && is_in_wilderness( ch ) )
            {
               xSET_BIT( otmp->extra_flags, ITEM_WILDERNESS );
               otmp->cords[0] = ch->cords[0];
               otmp->cords[1] = ch->cords[1];
            }
         }

         if( ch )
         {
            oprog_drop_trigger( ch, otmp );  /* mudprogs */
            if( char_died( ch ) )
               ch = NULL;
         }
         movedsome = true;
      }
      return movedsome;
   }
   if( ch )
   {
      for( otmp = obj->first_content; otmp; otmp = otmp_next )
      {
         otmp_next = otmp->next_content;
         obj_from_obj( otmp );
         obj_to_char( otmp, ch );
         movedsome = true;
      }
      return movedsome;
   }
   bug( "%s: could not determine a destination for vnum %d", __FUNCTION__, obj->pIndexData->vnum );
   return false;
}

/* Improve mental state - Thoric */
void better_mental_state( CHAR_DATA *ch, int mod )
{
   int c = URANGE( 0, abs( mod ), 20 );
   int con = get_curr_con( ch );

   c += number_percent( ) < con ? 1 : 0;

   if( ch->mental_state < 0 )
      ch->mental_state = URANGE( -100, ch->mental_state + c, 0 );
   else if( ch->mental_state > 0 )
      ch->mental_state = URANGE( 0, ch->mental_state - c, 100 );
}

/* Deteriorate mental state - Thoric */
void worsen_mental_state( CHAR_DATA *ch, int mod )
{
   int c = URANGE( 0, abs( mod ), 20 );
   int con = get_curr_con( ch );

   c -= number_percent( ) < con ? 1 : 0;
   if( c < 1 )
      return;

   if( ch->mental_state < 0 )
      ch->mental_state = URANGE( -100, ch->mental_state - c, 100 );
   else if( ch->mental_state > 0 )
      ch->mental_state = URANGE( -100, ch->mental_state + c, 100 );
   else
      ch->mental_state -= c;
}

/*
 * Add another notch on that there belt... ;)
 * Keep track of the last so many kills by vnum - Thoric
 */
void add_kill( CHAR_DATA *ch, CHAR_DATA *mob )
{
   KILLED_DATA *killed, *killed_next;
   int vnum, count = 0;

   if( is_npc( ch ) )
   {
      bug( "%s: trying to add kill to npc", __FUNCTION__ );
      return;
   }

   if( !is_npc( mob ) )
   {
      bug( "%s: trying to add kill non-npc", __FUNCTION__ );
      return;
   }

   vnum = mob->pIndexData->vnum;

   /* Already in the history? */
   for( killed = ch->pcdata->first_killed; killed; killed = killed_next )
   {
      killed_next = killed->next;

      ++count;
      if( killed->vnum == vnum )
      {
         ++killed->count;
         return;
      }
   }

   /* If you have more then you need remove extras from the start before adding new one */
   while( count >= sysdata.maxkillhistory )
   {
      killed = ch->pcdata->first_killed;
      UNLINK( killed, ch->pcdata->first_killed, ch->pcdata->last_killed, next, prev );
      DISPOSE( killed );
      --count;
   }

   /* New so add it to the list */
   CREATE( killed, KILLED_DATA, 1 );
   killed->vnum = vnum;
   killed->count = 1;
   LINK( killed, ch->pcdata->first_killed, ch->pcdata->last_killed, next, prev );
}

/*
 * Return how many times this player has killed this mob	-Thoric
 * Only keeps track of so many, and keeps track by vnum
 */
int times_killed( CHAR_DATA *ch, CHAR_DATA *mob )
{
   KILLED_DATA *killed;
   int vnum;

   if( is_npc( ch ) )
   {
      bug( "%s: ch is not a player", __FUNCTION__ );
      return 0;
   }

   if( !is_npc( mob ) )
   {
      bug( "%s: mob is not a mobile", __FUNCTION__ );
      return 0;
   }

   vnum = mob->pIndexData->vnum;
   for( killed = ch->pcdata->first_killed; killed; killed = killed->next )
   {
      if( killed->vnum == vnum )
         return killed->count;
   }
   return 0;
}

/*
 * returns area with name matching input string
 * Last Modified : July 21, 1997
 * Fireblade
 */
AREA_DATA *get_area( char *name )
{
   AREA_DATA *pArea;

   if( !name )
   {
      bug( "%s: NULL input string.", __FUNCTION__ );
      return NULL;
   }

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      if( nifty_is_name( name, pArea->name ) )
         break;
   }

   if( !pArea )
   {
      for( pArea = first_build; pArea; pArea = pArea->next )
      {
         if( nifty_is_name( name, pArea->name ) )
            break;
      }
   }

   return pArea;
}

AREA_DATA *get_area_obj( OBJ_INDEX_DATA *pObjIndex )
{
   AREA_DATA *pArea;

   if( !pObjIndex )
   {
      bug( "%s: pObjIndex is NULL.", __FUNCTION__ );
      return NULL;
   }
   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      if( pObjIndex->vnum >= pArea->low_vnum && pObjIndex->vnum <= pArea->hi_vnum )
         break;
   }
   return pArea;
}

int umin( int check, int ncheck )
{
   if( check < ncheck )
      return check;
   return ncheck;
}

int umax( int check, int ncheck )
{
   if( check > ncheck )
      return check;
   return ncheck;
}

int urange( int mincheck, int check, int maxcheck )
{
   if( check < mincheck )
      return mincheck;
   if( check > maxcheck )
      return maxcheck;
   return check;
}

double dumin( double check, double ncheck )
{
   if( check < ncheck )
      return check;
   return ncheck;
}

double dumax( double check, double ncheck )
{
   if( check > ncheck )
      return check;
   return ncheck;
}

double durange( double mincheck, double check, double maxcheck )
{
   if( check < mincheck )
      return mincheck;
   if( check > maxcheck )
      return maxcheck;
   return check;
}

char *pers( CHAR_DATA *ch, CHAR_DATA *looker )
{
   if( !ch || !looker )
      return (char *)"Someone";
   if( can_see( looker, ch ) )
   {
      if( is_npc( ch ) )
         return ch->short_descr;
      else
         return ch->name;
   }
   return (char *)"Someone";
}

char *morphpers( CHAR_DATA *ch, CHAR_DATA *looker )
{
   if( ch && looker && ch->morph && ch->morph->morph && ch->morph->morph->short_desc )
      return ch->morph->morph->short_desc;
   return (char *)"Someone";
}

/* Things change sometimes and need to be rechecked as they go */
void check_chareq( CHAR_DATA *ch )
{
   MCLASS_DATA *mclass;
   OBJ_DATA *obj, *obj_next;
   int stat, check = 0, count = 0;
   bool scontinue;

   /* Just incase sometimes it puts stuff back in inventory need to limit the check */
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      check++;

   for( obj = ch->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;

      if( ++count > check )
         break;

      if( obj->wear_loc == WEAR_NONE || is_obj_stat( obj, ITEM_LODGED ) )
      {
         if( obj->wear_loc == WEAR_NONE )
            weight_check( ch, obj );
         continue;
      }

      /* Check to see if it should zap them */
      if( ( is_obj_stat( obj, ITEM_ANTI_EVIL ) && is_evil( ch ) )
      || ( is_obj_stat( obj, ITEM_ANTI_GOOD ) && is_good( ch ) )
      || ( is_obj_stat( obj, ITEM_ANTI_NEUTRAL ) && is_neutral( ch ) ) )
      {
         act( AT_MAGIC, "You're zapped by $p.", ch, obj, NULL, TO_CHAR );
         act( AT_MAGIC, "$n is zapped by $p.", ch, obj, NULL, TO_ROOM );
         unequip_char( ch, obj );
         oprog_zap_trigger( ch, obj );   /* mudprogs */
         if( char_died( ch ) )
            break;
         continue;
      }

      /* Check level to see if they can still use it */
      if( ch->level < obj->level )
      {
         act( AT_OBJECT, "Your level is no longer high enough to use $p.", ch, obj, NULL, TO_CHAR );
         act( AT_OBJECT, "$n's level is no longer high enough to use $p.", ch, obj, NULL, TO_ROOM );
         unequip_char( ch, obj );
         continue;
      }

      /* Check all stats to see if they can still use it */
      scontinue = false;
      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( get_curr_stat( stat, ch ) < obj->stat_reqs[stat] )
         {
            act_printf( AT_OBJECT, ch, obj, NULL, TO_CHAR, "Your %s is no longer high enough to use $p.", stattypes[stat] );
            act_printf( AT_OBJECT, ch, obj, NULL, TO_ROOM, "$n no longer has the %s needed to use $p.", stattypes[stat] );
            unequip_char( ch, obj );
            scontinue = true;
            break;
         }
      }
      if( scontinue )
         continue;

      /* Only check pc's after this */
      if( is_npc( ch ) )
         continue;

      /* Check PC race */
      if( ch->race >= 0 && ch->race < MAX_PC_RACE && race_table[ch->race] )
      {
         if( xIS_SET( obj->race_restrict, ch->race ) )
         {
            act( AT_OBJECT, "Your race isn't able to use $p.", ch, obj, NULL, TO_CHAR );
            act( AT_OBJECT, "$n can no longer use $p because of race.", ch, obj, NULL, TO_ROOM );
            unequip_char( ch, obj );
            continue;
         }

         if( xIS_SET( race_table[ch->race]->where_restrict, obj->wear_loc ) )
         {
            act( AT_OBJECT, "Your not able to wear $p correctly.", ch, obj, NULL, TO_CHAR );
            act( AT_OBJECT, "$n can no longer wear $p correctly.", ch, obj, NULL, TO_ROOM );
            unequip_char( ch, obj );
            continue;
         }
      }

      /* Check all PC classes */
      scontinue = false;
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( mclass->wclass >= 0 && xIS_SET( obj->class_restrict, mclass->wclass ) )
         {
            act( AT_OBJECT, "Your class isn't able to wear $p.", ch, obj, NULL, TO_CHAR );
            act( AT_OBJECT, "$n can no longer use $p because of class.", ch, obj, NULL, TO_ROOM );
            unequip_char( ch, obj );
            scontinue = true;
            break;
         }
      }
      if( scontinue )
         continue;
   }
}

bool check_subrestricted( CHAR_DATA *ch )
{
   if( !ch )
      return true;
   if( ch->substate == SUB_RESTRICTED )
   {
      send_to_char( "You can't use this command from within another command.\r\n", ch );
      return true;
   }

   return false;
}

char lower( char c )
{
   if( c >= 'A' && c <= 'Z' )
      return ( c + 'a' - 'A' );
   return c;
}

char upper( char c )
{
   if( c >= 'a' && c <= 'z' )
      return ( c + 'A' - 'a' );
   return c;
}
