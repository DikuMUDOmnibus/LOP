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
 *                        "Special procedure" module                         *
 *****************************************************************************/

#include <stdio.h>
#if !defined(WIN32)
  #include <dlfcn.h>
#else
  #include <windows.h>
  #define dlsym( handle, name ) ( (void*)GetProcAddress( (HINSTANCE) (handle), (name) ) )
  #define dlerror() GetLastError()
#endif
#include "h/mud.h"

NM ch_ret spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo );

#define SPECFUN_FILE SYSTEM_DIR "specfuns.dat"
typedef struct specfun_list SPEC_LIST;
struct specfun_list
{
   SPEC_LIST *next, *prev;
   char *name;
};

/* The following special functions are available for mobiles. */
DECLARE_SPEC_FUN( spec_breath_any );
DECLARE_SPEC_FUN( spec_breath_acid );
DECLARE_SPEC_FUN( spec_breath_fire );
DECLARE_SPEC_FUN( spec_breath_frost );
DECLARE_SPEC_FUN( spec_breath_gas );
DECLARE_SPEC_FUN( spec_breath_lightning );
DECLARE_SPEC_FUN( spec_cast_adept );
DECLARE_SPEC_FUN( spec_cast_cleric );
DECLARE_SPEC_FUN( spec_cast_mage );
DECLARE_SPEC_FUN( spec_cast_undead );
DECLARE_SPEC_FUN( spec_fido );
DECLARE_SPEC_FUN( spec_janitor );
DECLARE_SPEC_FUN( spec_poison );
DECLARE_SPEC_FUN( spec_thief );

SPEC_LIST *first_specfun;
SPEC_LIST *last_specfun;

void free_specfuns( void )
{
   SPEC_LIST *specfun, *next_specfun;

   for( specfun = first_specfun; specfun; specfun = next_specfun )
   {
      next_specfun = specfun->next;
      UNLINK( specfun, first_specfun, last_specfun, next, prev ); 
      STRFREE( specfun->name );
      DISPOSE( specfun );
   }
}

/*
 * Simple load function - no OLC support for now.
 * This is probably something you DONT want builders playing with.
 */
void load_specfuns( void )
{
   SPEC_LIST *specfun;
   FILE *fp;
   char *word;

   first_specfun = last_specfun = NULL;

   if( !( fp = fopen( SPECFUN_FILE, "r" ) ) )
   {
      bug( "%s: can't read %s, exiting.", __FUNCTION__, SPECFUN_FILE );
      perror( SPECFUN_FILE );
      exit( 1 );
   }
   else
   {
      for( ; ; )
      {
         if( feof( fp ) )
            break;

         word = fread_word( fp );
         if( !str_cmp( word, "$" ) )
            break;

         CREATE( specfun, SPEC_LIST, 1 );
         specfun->name = STRALLOC( word );
         LINK( specfun, first_specfun, last_specfun, next, prev );
      }
      fclose( fp );
      fp = NULL;
   }
}

/* Simple validation function to be sure a function can be used on mobs */
bool validate_spec_fun( char *name )
{
   SPEC_LIST *specfun;

   for( specfun = first_specfun; specfun; specfun = specfun->next )
   {
      if( !str_cmp( specfun->name, name ) )
         return true;
   }
   return false;
}

/* Given a name, return the appropriate spec_fun. */
SPEC_FUN *spec_lookup( char *name )
{
   void *funHandle;
#if !defined(WIN32)
   const char *error;
#else
   DWORD error;
#endif

   funHandle = dlsym( sysdata.dlHandle, name );
   if( ( error = dlerror() ) )
   {
      bug( "%s: Error locating function %s in symbol table.", __FUNCTION__, name );
      return NULL;
   }
   return ( SPEC_FUN * ) funHandle;
}

/* if a spell casting mob is hating someone... try and summon them */
void summon_if_hating( CHAR_DATA *ch )
{
   CHAR_DATA *victim;
   HHF_DATA *hate = NULL;
   char buf[MSL], name[MIL];

   if( ch->position <= POS_SLEEPING )
   {
      ch->summoning = NULL;
      return;
   }
   if( ch->fighting || !ch->first_hating || xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      ch->summoning = NULL;
      return;
   }
   for( hate = ch->first_hating; hate; hate = hate->next )
   {
      if( !hate->who )
         continue;
      /* Make it a little random when possible */
      if( hate->next && number_range( 1, 6 ) > 3 )
         continue;
      /* Allow it to change who its summoning as it goes */
      if( hate->next && ch->summoning && ch->summoning == hate->who )
         continue;
      break;
   }
   if( !ch->summoning && !hate )
   {
      ch->summoning = NULL;
      return;
   }

   if( hate )
      victim = hate->who;
   else
      victim = ch->summoning;

   if( ch->in_room == victim->in_room )
      return;

   ch->summoning = victim;
   snprintf( buf, sizeof( buf ), "summon %s%s", !is_npc( victim ) ? "0." : "", name );
   do_cast( ch, buf );
}

bool is_fighting( CHAR_DATA *ch )
{
   if( ch && ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
   || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
   || ch->position == POS_BERSERK ) )
      return true;
   return false;
}

/* Core procedure for dragons. */
bool dragon( CHAR_DATA *ch, const char *fspell_name )
{
   CHAR_DATA *victim, *v_next;
   int sn;

   if( !is_fighting( ch ) )
      return false;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim )
      return false;

   if( ( sn = skill_lookup( fspell_name ) ) < 0 )
      return false;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
   return true;
}

/* Special procedures for mobiles. */
bool spec_breath_any( CHAR_DATA *ch )
{
   if( !is_fighting( ch ) )
      return false;

   switch( number_bits( 3 ) )
   {
      case 0:
         return spec_breath_fire( ch );
      case 1:
      case 2:
         return spec_breath_lightning( ch );
      case 3:
         return spec_breath_gas( ch );
      case 4:
         return spec_breath_acid( ch );
      case 5:
      case 6:
      case 7:
         return spec_breath_frost( ch );
   }

   return false;
}

bool spec_breath_acid( CHAR_DATA *ch )
{
   return dragon( ch, "acid breath" );
}

bool spec_breath_fire( CHAR_DATA *ch )
{
   return dragon( ch, "fire breath" );
}

bool spec_breath_frost( CHAR_DATA *ch )
{
   return dragon( ch, "frost breath" );
}

bool spec_breath_gas( CHAR_DATA *ch )
{
   int sn;

   if( !is_fighting( ch ) )
      return false;

   if( ( sn = skill_lookup( "gas breath" ) ) < 0 )
      return false;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, NULL );
   return true;
}

bool spec_breath_lightning( CHAR_DATA *ch )
{
   return dragon( ch, "lightning breath" );
}

bool spec_cast_adept( CHAR_DATA *ch )
{
   CHAR_DATA *victim, *v_next;

   if( !is_awake( ch ) || ch->fighting )
      return false;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( victim != ch && can_see( ch, victim ) && number_bits( 1 ) == 0 )
         break;
   }

   if( !victim )
      return false;

   switch( number_bits( 3 ) )
   {
      case 0:
         act( AT_MAGIC, "$n utters the word 'suah'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "cure blindness" ), ch->level, ch, victim );
         return true;

      case 1:
         act( AT_MAGIC, "$n utters the word 'nran'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "cure light" ), ch->level, ch, victim );
         return true;

      case 2:
         act( AT_MAGIC, "$n utters the word 'nyrcs'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "cure poison" ), ch->level, ch, victim );
         return true;

      case 3:
         act( AT_MAGIC, "$n utters the word 'naimad'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, victim );
         return true;

      case 4:
         act( AT_MAGIC, "$n utters the word 'gorog'.", ch, NULL, NULL, TO_ROOM );
         spell_remove_curse( skill_lookup( "remove curse" ), ch->level, ch, victim );
         return true;

      case 5:
         act( AT_MAGIC, "$n utters the word 'rogo'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, victim );
         return true;

      case 6:
         act( AT_MAGIC, "$n utters the word 'floro'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "heal" ), ch->level, ch, victim );
         return true;
   }

   return false;
}

bool spec_cast_cleric( CHAR_DATA *ch )
{
   CHAR_DATA *victim, *v_next;
   const char *spell;
   int sn;

   summon_if_hating( ch );
   if( !is_fighting( ch ) )
      return false;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return false;

   for( ;; )
   {
      int min_level;

      switch( number_bits( 4 ) )
      {
         case 0:
            min_level = 0;
            spell = "cause light";
            break;

         case 1:
            min_level = 3;
            spell = "cause serious";
            break;

         case 2:
            min_level = 6;
            spell = "earthquake";
            break;

         case 3:
            min_level = 7;
            spell = "blindness";
            break;

         case 4:
            min_level = 9;
            spell = "cause critical";
            break;

         case 5:
            min_level = 10;
            spell = "dispel evil";
            break;

         case 6:
            min_level = 12;
            spell = "curse";
            break;

         default:
            min_level = 16;
            spell = "dispel magic";
            break;
      }

      if( ch->level >= min_level )
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
   return true;
}

bool spec_cast_mage( CHAR_DATA *ch )
{
   CHAR_DATA *victim, *v_next;
   const char *spell;
   int sn;

   summon_if_hating( ch );
   if( !is_fighting( ch ) )
      return false;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return false;

   for( ;; )
   {
      int min_level;

      switch( number_bits( 4 ) )
      {
         case 0:
            min_level = 0;
            spell = "energy drain";
            break;

         default:
            min_level = 15;
            spell = "fireball";
            break;
      }

      if( ch->level >= min_level )
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
   return true;
}

bool spec_cast_undead( CHAR_DATA *ch )
{
   CHAR_DATA *victim, *v_next;
   const char *spell;
   int sn;

   summon_if_hating( ch );
   if( !is_fighting( ch ) )
      return false;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return false;

   for( ;; )
   {
      int min_level;

      switch( number_bits( 4 ) )
      {
         case 0:
            min_level = 0;
            spell = "curse";
            break;

         case 1:
            min_level = 13;
            spell = "blindness";
            break;

         case 2:
            min_level = 14;
            spell = "poison";
            break;

         case 3:
            min_level = 15;
            spell = "energy drain";
            break;

         default:
            min_level = 40;
            spell = "gate";
            break;
      }

      if( ch->level >= min_level )
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
   return true;
}

bool spec_fido( CHAR_DATA *ch )
{
   OBJ_DATA *corpse, *c_next, *obj, *obj_next;

   if( !is_awake( ch ) )
      return false;

   for( corpse = ch->in_room->first_content; corpse; corpse = c_next )
   {
      c_next = corpse->next_content;
      if( corpse->item_type != ITEM_CORPSE_NPC )
         continue;

      act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );
      for( obj = corpse->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         obj_from_obj( obj );
         obj_to_room( obj, ch->in_room );
      }
      extract_obj( corpse );
      return true;
   }

   return false;
}

bool spec_janitor( CHAR_DATA *ch )
{
   OBJ_DATA *trash, *trash_next;

   if( !is_awake( ch ) )
      return false;

   for( trash = ch->in_room->first_content; trash; trash = trash_next )
   {
      trash_next = trash->next_content;
      if( is_obj_stat( trash, ITEM_PROTOTYPE ) && ( !is_npc( ch ) || !xIS_SET( ch->act, ACT_PROTOTYPE ) ) )
         continue;
      if( xIS_SET( trash->wear_flags, ITEM_NO_TAKE ) || is_obj_stat( trash, ITEM_BURIED ) )
         continue;
      if( trash->item_type == ITEM_DRINK_CON || trash->item_type == ITEM_TRASH
      || trash->cost < 10 || ( trash->pIndexData->vnum == OBJ_VNUM_SHOPPING_BAG && !trash->first_content ) )
      {
         act( AT_ACTION, "$n picks up some trash.", ch, NULL, NULL, TO_ROOM );
         obj_from_room( trash );
         obj_to_char( trash, ch );
         return true;
      }
   }
   return false;
}

bool spec_poison( CHAR_DATA *ch )
{
   CHAR_DATA *victim;

   if( !is_fighting( ch ) )
      return false;

   if( !( victim = who_fighting( ch ) ) || number_percent( ) > 2 * ch->level )
      return false;

   act( AT_HIT, "You bite $N!", ch, NULL, victim, TO_CHAR );
   act( AT_ACTION, "$n bites $N!", ch, NULL, victim, TO_NOTVICT );
   act( AT_POISON, "$n bites you!", ch, NULL, victim, TO_VICT );
   spell_smaug( gsn_poison, ch->level, ch, victim );
   return true;
}

bool spec_thief( CHAR_DATA *ch )
{
   CHAR_DATA *victim, *v_next;
   int gold;

   if( ch->position != POS_STANDING )
      return false;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;

      if( is_npc( victim ) || get_trust( victim ) >= PERM_IMM || number_bits( 2 ) != 0 || !can_see( ch, victim ) )
         continue;
      if( victim->gold <= 0 )
         continue;
      if( is_awake( victim ) && number_range( 0, ch->level ) == 0 )
      {
         act( AT_ACTION, "You discover $n's hands in your sack of gold!", ch, NULL, victim, TO_VICT );
         act( AT_ACTION, "$N discovers $n's hands in $S sack of gold!", ch, NULL, victim, TO_NOTVICT );
         return true;
      }
      else
      {
         gold = ( victim->gold / number_range( 10, 100 ) );
         if( gold <= 0 )
            return false;
         decrease_gold( victim, gold );
         increase_gold( ch, gold );
         return true;
      }
   }

   return false;
}
