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
 *			     Spell handling module			     *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "h/mud.h"

/* Lookup an herb by name. */
int herb_lookup( const char *name )
{
   int sn;

   for( sn = 0; sn < top_herb; sn++ )
   {
      if( !herb_table[sn] || !herb_table[sn]->name )
         return -1;
      if( LOWER( name[0] ) != LOWER( herb_table[sn]->name[0] ) )
         continue;
      if( str_prefix( name, herb_table[sn]->name ) )
         continue;
      return sn;
   }
   return -1;
}

void set_ch_personal( PC_DATA *pc )
{
   int sn, count = 0;

   if( !pc || !pc->character )
      return;

   /* Set them all to nothing */
   for( sn = 0; sn < MAX_PC_PERS; sn++ )
      pc->personal[sn] = -1;

   for( sn = 0; sn < top_pers; sn++ )
   {
      if( !pers_table[sn] || !pers_table[sn]->name || !pers_table[sn]->teachers || str_cmp( pers_table[sn]->teachers, pc->character->name ) )
         continue;
      /* Lets make sure now that we keep it in limit */
      if( count >= MAX_PC_PERS )
         send_to_char( "Your already at your max personal skills/spells.\r\n", pc->character );
      else
         pc->personal[count++] = sn; /* Set them if valid and is listed as the teacher */
   }
}

/* Lookup personals only for this character */
int ch_pers_lookup( const char *pchar, const char *name )
{
   int sn;

   for( sn = 0; sn < top_pers; sn++ )
   {
      if( !pers_table[sn] || !pers_table[sn]->name )
         return -1;
      if( LOWER( name[0] ) != LOWER( pers_table[sn]->name[0] ) )
         continue;
      if( str_prefix( name, pers_table[sn]->name ) )
         continue;
      if( str_cmp( pers_table[sn]->teachers, pchar ) )
         continue;
      return sn;
   }
   return -1;
}

int pers_lookup( const char *name )
{
   int sn;

   for( sn = 0; sn < top_pers; sn++ )
   {
      if( !pers_table[sn] || !pers_table[sn]->name )
         return -1;
      if( LOWER( name[0] ) != LOWER( pers_table[sn]->name[0] ) )
         continue;
      if( str_prefix( name, pers_table[sn]->name ) )
         continue;
      return sn;
   }
   return -1;
}

/* Lookup a skill by name. */
int skill_lookup( const char *name )
{
   int sn;
   extern bool assigning_gsns;

   if( ( sn = bsearch_skill_exact( name, gsn_first_spell, gsn_first_skill - 1 ) ) == -1 )
      if( ( sn = bsearch_skill_exact( name, gsn_first_skill, gsn_first_weapon - 1 ) ) == -1 )
         if( ( sn = bsearch_skill_exact( name, gsn_first_weapon, gsn_first_tongue - 1 ) ) == -1 )
            if( ( sn = bsearch_skill_exact( name, gsn_first_tongue, gsn_top_sn - 1 ) ) == -1 )
               if( ( sn = bsearch_skill_prefix( name, gsn_first_spell, gsn_first_skill - 1 ) ) == -1 )
                  if( ( sn = bsearch_skill_prefix( name, gsn_first_skill, gsn_first_weapon - 1 ) ) == -1 )
                     if( ( sn = bsearch_skill_prefix( name, gsn_first_weapon, gsn_first_tongue - 1 ) ) == -1 )
                        if( ( sn = bsearch_skill_prefix( name, gsn_first_tongue, gsn_top_sn - 1 ) ) == -1 && gsn_top_sn < top_sn )
                        {
                           for( sn = gsn_top_sn; sn < top_sn; sn++ )
                           {
                              if( !skill_table[sn] || !skill_table[sn]->name )
                              {
                                 if( assigning_gsns )
                                    fprintf( stderr, "ASSIGN_GSN: Skill %s not found.\n", name ); \
                                 return -1;
                              }
                              if( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] )
                              && !str_prefix( name, skill_table[sn]->name ) )
                                 return sn;
                           }
                           if( assigning_gsns )
                              fprintf( stderr, "ASSIGN_GSN: Skill %s not found.\n", name ); \
                           return -1;
                        }

   if( assigning_gsns && sn == -1 )
      fprintf( stderr, "ASSIGN_GSN: Skill %s not found.\n", name ); \
   return sn;
}

/* This is to allow it to just check for spells so you see less "That isn't a spell" lines for like 'cast trollish' */
int spell_lookup( const char *name )
{
   int sn;

   if( ( sn = bsearch_skill_exact( name, gsn_first_spell, gsn_first_skill - 1 ) ) == -1 )
      if( ( sn = bsearch_skill_prefix( name, gsn_first_spell, gsn_first_skill - 1 ) ) == -1 )
         return skill_lookup( name );
   return sn;
}

/*
 * Return a skilltype pointer based on sn - Thoric
 * Returns NULL if bad or unused sn.
 */
SKILLTYPE *get_skilltype( int sn )
{
   if( sn >= TYPE_PERS )
      return is_valid_pers( sn - TYPE_PERS ) ? pers_table[sn - TYPE_PERS] : NULL;
   if( sn >= TYPE_HERB )
      return is_valid_herb( sn - TYPE_HERB ) ? herb_table[sn - TYPE_HERB] : NULL;
   if( sn >= TYPE_HIT )
      return NULL;
   return is_valid_sn( sn ) ? skill_table[sn] : NULL;
}

/*
 * Perform a binary search on a section of the skill table	-Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check for prefix matches
 */
int bsearch_skill_prefix( const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;
      if( !is_valid_sn( sn ) )
         return -1;
      if( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] ) && !str_prefix( name, skill_table[sn]->name ) )
         return sn;
      if( first >= top )
         return -1;
      if( strcasecmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

/*
 * Perform a binary search on a section of the skill table	-Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check for exact matches only
 */
int bsearch_skill_exact( const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;
      if( !is_valid_sn( sn ) )
         return -1;
      if( !strcasecmp( name, skill_table[sn]->name ) )
         return sn;
      if( first >= top )
         return -1;
      if( strcasecmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

/*
 * Perform a binary search on a section of the skill table	-Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check exact match first, then a prefix match
 */
int bsearch_skill( const char *name, int first, int top )
{
   int sn = bsearch_skill_exact( name, first, top );

   return ( sn == -1 ) ? bsearch_skill_prefix( name, first, top ) : sn;
}

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows				-Thoric
 */
int ch_bsearch_skill_prefix( CHAR_DATA *ch, const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( ch->pcdata->learned[sn] > 0 )
         if( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] ) )
            if( !str_prefix( name, skill_table[sn]->name ) )
               if( can_practice( ch, sn ) )
                  return sn;
      if( first >= top )
         return -1;
      if( strcmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

int ch_bsearch_skill_exact( CHAR_DATA *ch, const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( ch->pcdata->learned[sn] > 0 )
         if( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] ) )
            if( !str_cmp( name, skill_table[sn]->name ) )
               if( can_practice( ch, sn ) )
                  return sn;
      if( first >= top )
         return -1;
      if( strcmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

int ch_bsearch_skill( CHAR_DATA *ch, const char *name, int first, int top )
{
   int sn = ch_bsearch_skill_exact( ch, name, first, top );

   return ( sn == -1 ) ? ch_bsearch_skill_prefix( ch, name, first, top ) : sn;
}

int find_spell( CHAR_DATA *ch, const char *name, bool know )
{
   if( is_npc( ch ) || !know )
      return bsearch_skill( name, gsn_first_spell, gsn_first_skill - 1 );
   else
      return ch_bsearch_skill( ch, name, gsn_first_spell, gsn_first_skill - 1 );
}

int find_skill( CHAR_DATA *ch, const char *name, bool know )
{
   if( is_npc( ch ) || !know )
      return bsearch_skill( name, gsn_first_skill, gsn_first_weapon - 1 );
   else
      return ch_bsearch_skill( ch, name, gsn_first_skill, gsn_first_weapon - 1 );
}

int find_weapon( CHAR_DATA *ch, const char *name, bool know )
{
   if( is_npc( ch ) || !know )
      return bsearch_skill( name, gsn_first_weapon, gsn_first_tongue - 1 );
   else
      return ch_bsearch_skill( ch, name, gsn_first_weapon, gsn_first_tongue - 1 );
}

int find_tongue( CHAR_DATA *ch, const char *name, bool know )
{
   if( is_npc( ch ) || !know )
      return bsearch_skill( name, gsn_first_tongue, gsn_top_sn - 1 );
   else
      return ch_bsearch_skill( ch, name, gsn_first_tongue, gsn_top_sn - 1 );
}

/* Lookup a skill by slot number. Used for object loading. */
int slot_lookup( int slot )
{
   int sn;

   if( slot <= 0 )
      return -1;

   for( sn = 0; sn < top_sn; sn++ )
      if( slot == skill_table[sn]->slot )
         return sn;

   if( fBootDb )
   {
      bug( "%s: bad slot %d.", __FUNCTION__, slot );
      abort( );
   }

   return -1;
}

/* Handler to tell the victim which spell is being affected. - Shaddai */
int dispel_casting( AFFECT_DATA *paf, CHAR_DATA *ch, CHAR_DATA *victim, int affect, bool dispel )
{
   char buf[MSL], *spell;
   SKILLTYPE *sktmp;
   bool is_mage = false, has_detect = false;
   EXT_BV ext_bv = meb( affect );

   if( is_npc( ch ) )
      is_mage = true;
   if( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) )
      has_detect = true;

   if( paf )
   {
      if( !( sktmp = get_skilltype( paf->type ) ) )
         return 0;
      spell = sktmp->name;
   }
   else
      spell = ext_flag_string( &ext_bv, a_flags );

   set_char_color( AT_MAGIC, ch );
   set_char_color( AT_HITME, victim );

   if( !can_see( ch, victim ) )
      mudstrlcpy( buf, "Someone", sizeof( buf ) );
   else
   {
      mudstrlcpy( buf, ( is_npc( victim ) ? victim->short_descr : victim->name ), sizeof( buf ) );
      buf[0] = toupper( buf[0] );
   }

   if( dispel )
   {
      ch_printf( victim, "Your %s vanishes.\r\n", spell );
      if( is_mage && has_detect )
         ch_printf( ch, "%s's %s vanishes.\r\n", buf, spell );
      else
         return 0;   /* So we give the default Ok. Message */
   }
   else
   {
      if( is_mage && has_detect )
         ch_printf( ch, "%s's %s wavers but holds.\r\n", buf, spell );
      else
         return 0;   /* The wonderful Failed. Message */
   }
   return 1;
}

/* Fancy message handling for a successful casting - Thoric */
void successful_casting( SKILLTYPE *skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( !ch )
      return;

   if( !victim )
      victim = ch;

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch != victim )
   {
      if( skill->hit_char )
         act( chit, skill->hit_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "Ok.", ch, NULL, NULL, TO_CHAR );
   }

   if( skill->hit_vict )
      act( chitme, skill->hit_vict, ch, obj, victim, ch == victim ? TO_CHAR : TO_VICT );
   else if( ch == victim && ( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL ) )
      act( chitme, "Ok.", ch, NULL, victim, TO_CHAR );

   if( skill->hit_room )
       act( chitroom, skill->hit_room, ch, obj, victim, TO_NOTVICT );
}


/* Fancy message handling for a failed casting - Thoric */
void failed_casting( SKILLTYPE *skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( !ch )
      return;

   if( !victim )
      victim = ch;

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch != victim )
   {
      if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "You failed.", ch, NULL, NULL, TO_CHAR );
   }

   if( skill->miss_vict )
      act( chitme, skill->miss_vict, ch, obj, victim, ch == victim ? TO_CHAR : TO_VICT );
   else if( ch == victim && ( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL ) )
      act( chitme, "You failed.", ch, NULL, victim, TO_CHAR );

   if( skill->miss_room )
      act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );
}

/* Fancy message handling for being immune to something -Thoric */
void immune_casting( SKILLTYPE *skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( !ch )
      return;
   if( !victim )
      victim = ch;

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch != victim )
   {
      if( skill->imm_char )
         act( chit, skill->imm_char, ch, obj, victim, TO_CHAR );
      else if( skill->miss_char )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "That appears to have no effect.", ch, NULL, NULL, TO_CHAR );
   }

   if( skill->imm_vict )
      act( chitme, skill->imm_vict, ch, obj, victim, ch == victim ? TO_CHAR : TO_VICT );
   else if( skill->miss_vict )
      act( chitme, skill->miss_vict, ch, obj, victim, ch == victim ? TO_CHAR : TO_VICT );
   else if( ch == victim && ( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL ) )
      act( chitme, "That appears to have no affect.", ch, NULL, NULL, TO_CHAR );

   if( skill->imm_room )
      act( chitroom, skill->imm_room, ch, obj, victim, TO_NOTVICT );
   else if( skill->miss_room )
      act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );
}

void absorb_casting( SKILLTYPE *skill, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( !ch )
      return;
   if( !victim )
      victim = ch;

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch != victim )
   {
      if( skill->abs_char )
         act( chit, skill->abs_char, ch, obj, victim, TO_CHAR );
      else if( skill->miss_char )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "That appears to have been absorbed.", ch, NULL, NULL, TO_CHAR );
   }

   if( skill->abs_vict )
      act( chitme, skill->abs_vict, ch, obj, victim, ch == victim ? TO_CHAR : TO_VICT );
   else if( skill->miss_vict )
      act( chitme, skill->miss_vict, ch, obj, victim, ch == victim ? TO_CHAR : TO_VICT );
   else if( ch == victim && ( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL ) )
      act( chitme, "That appears to have been absorbed.", ch, NULL, NULL, TO_CHAR );

   if( skill->abs_room )
      act( chitroom, skill->abs_room, ch, obj, victim, TO_NOTVICT );
   else if( skill->miss_room )
      act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );
}

/* Utter mystical words for an sn. */
void say_spell( CHAR_DATA *ch, int sn )
{
   CHAR_DATA *rch;
   SKILLTYPE *skill = get_skilltype( sn );
   char buf[MSL], buf2[MSL], *pName;
   const char *uclass, *ruclass; 
  int iSyl, length;

   struct syl_type
   {
      const char *old;
      const char *cnew;
   };

   static const struct syl_type syl_table[] =
   {
      { "a",  "a" },  { "b",  "b" },  { "c",  "q" },  { "d",  "e" },
      { "e",  "z" },  { "f",  "y" },  { "g",  "o" },  { "h",  "p" },
      { "i",  "u" },  { "j",  "y" },  { "k",  "t" },  { "l",  "r" },
      { "m",  "w" },  { "n",  "i" },  { "o",  "a" },  { "p",  "s" },
      { "q",  "d" },  { "r",  "f" },  { "s",  "g" },  { "t",  "h" },
      { "u",  "j" },  { "v",  "z" },  { "w",  "x" },  { "x",  "n" },
      { "y",  "l" },  { "z",  "k" },  { "",   ""}
   };

   buf[0] = '\0';
   for( pName = skill->name; *pName != '\0'; pName += length )
   {
      for( iSyl = 0; ( length = strlen( syl_table[iSyl].old ) ) != 0; iSyl++ )
      {
         if( !str_prefix( syl_table[iSyl].old, pName ) )
         {
            mudstrlcat( buf, syl_table[iSyl].cnew, sizeof( buf ) );
            break;
         }
      }

      if( length == 0 )
         length = 1;
   }

   snprintf( buf2, sizeof( buf2 ), "$n utters the words, '%s'.", buf );
   snprintf( buf, sizeof( buf ), "$n utters the words, '%s'.", skill->name );

   uclass = dis_main_class_name( ch );
   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( rch == ch || !can_see_character( rch, ch ) )
         continue;
      ruclass = dis_main_class_name( rch );
      if( !str_cmp( uclass, ruclass ) )
         act( AT_MAGIC, buf, ch, NULL, rch, TO_VICT );
      else
         act( AT_MAGIC, buf2, ch, NULL, rch, TO_VICT );
   }
}

/* Make adjustments to saving throw based in RIS -Thoric */
/* This is normaly used for Magic stuff so should also check RIS_MAGIC on all of it */
int ris_save( CHAR_DATA *ch, int schance, int ris )
{
   return ch->resistant[ris];
}


/* -Thoric
 * Fancy dice expression parsing complete with order of operations,
 * simple exponent support, dice support as well as a few extra
 * variables: L = level, H = hp, M = mana, V = move, S = str, X = dex
 *            I = int, W = wis, C = con, A = cha, U = luck, A = age
 *
 * Used for spell dice parsing, ie: 3d8+L-6
 */
int rd_parse( CHAR_DATA *ch, CHAR_DATA *victim, int level, char *texp )
{
   unsigned int x;
   int lop = 0, gop = 0, eop = 0, tmp = 0;
   char operation;
   char *sexp[2];
   int total = 0;
   unsigned int len = 0;

   /* take care of nulls coming in */
   if( !texp || !strlen( texp ) )
      return 0;

   if( !victim )
      victim = ch;

   /* get rid of brackets if they surround the entire expresion */
   if( ( *texp == '(' ) && texp[strlen( texp ) - 1] == ')' )
   {
      texp[strlen( texp ) - 1] = '\0';
      texp++;
   }

   /* check if the expresion is just a number */
   len = strlen( texp );
   if( len == 1 && isalpha( texp[0] ) )
   {
      switch( texp[0] )
      {
         case 'A':
            return get_curr_cha( victim );

         case 'a':
            return get_curr_cha( ch );

         case 'B':
            return get_hitroll( victim );

         case 'b':
            return get_hitroll( ch );

         case 'C':
            return get_curr_con( victim );

         case 'c':
            return get_curr_con( ch );

         case 'E':
            return get_damroll( victim );

         case 'e':
            return get_damroll( ch );

         case 'H':
            return victim->hit;

         case 'h':
            return ch->hit;

         case 'I':
            return get_curr_int( victim );

         case 'i':
            return get_curr_int( ch );

         case 'L':
         case 'l':
            return level;

         case 'M':
            return victim->mana;

         case 'm':
            return ch->mana;

         case 'S':
            return get_curr_str( victim );

         case 's':
            return get_curr_str( ch );

         case 'U':
            return get_curr_lck( victim );

         case 'u':
            return get_curr_lck( ch );

         case 'V':
            return victim->move;

         case 'v':
            return ch->move;

         case 'W':
            return get_curr_wis( victim );

         case 'w':
            return get_curr_wis( ch );

         case 'X':
            return get_curr_dex( victim );

         case 'x':
            return get_curr_dex( ch );

         case 'Y':
            return get_age( victim );

         case 'y':
            return get_age( ch );
      }
   }

   for( x = 0; x < len; ++x )
      if( !isdigit( texp[x] ) && !isspace( texp[x] ) )
         break;

   if( x == len )
      return atoi( texp );

   /* break it into 2 parts */
   for( x = 0; x < strlen( texp ); ++x )
   {
      switch( texp[x] )
      {
         case '^':
            if( !total )
               eop = x;
            break;

         case '-':
         case '+':
            if( !total )
               lop = x;
            break;

         case '*':
         case '/':
         case '%':
         case 'd':
         case 'D':
         case 'R':
         case '<':
         case '>':
         case '{':
         case '}':
         case '=':
            if( !total )
               gop = x;
            break;

         case '(':
            ++total;
            break;

         case ')':
            --total;
            break;
      }
   }
   if( lop )
      x = lop;
   else if( gop )
      x = gop;
   else
      x = eop;
   operation = texp[x];
   texp[x] = '\0';
   sexp[0] = texp;
   sexp[1] = ( char * )( texp + x + 1 );

   /* work it out */
   total = rd_parse( ch, victim, level, sexp[0] );
   
   switch( operation )
   {
      case '-':
         total -= rd_parse( ch, victim, level, sexp[1] );
         break;

      case '+':
         total += rd_parse( ch, victim, level, sexp[1] );
         break;

      case '*':
         tmp = rd_parse( ch, victim, level, sexp[1] );
         total *= tmp != 0 ? tmp : 1;
         break;

      case '/':
         tmp = rd_parse( ch, victim, level, sexp[1] );
         total /= tmp != 0 ? tmp : 1;
         break;

      case '%':
         total %= rd_parse( ch, victim, level, sexp[1] );
         break;

      case 'd':
      case 'D':
         total = dice( total, rd_parse( ch, victim, level, sexp[1] ) );
         break;

      case 'R':
         total = number_range( total, rd_parse( ch, victim, level, sexp[1] ) );
         break;

      case '<':
         total = ( total < rd_parse( ch, victim, level, sexp[1] ) );
         break;

      case '>':
         total = ( total > rd_parse( ch, victim, level, sexp[1] ) );
         break;

      case '=':
         total = ( total == rd_parse( ch, victim, level, sexp[1] ) );
         break;

      case '{':
         total = UMIN( total, rd_parse( ch, victim, level, sexp[1] ) );
         break;

      case '}':
         total = UMAX( total, rd_parse( ch, victim, level, sexp[1] ) );
         break;

      case '^':
      {
         unsigned int y = rd_parse( ch, victim, level, sexp[1] ), z = total;

         for( x = 1; x < y; ++x, z *= total );
         total = z;
         break;
      }
   }
   return total;
}

/* wrapper function so as not to destroy exp */
int dice_parse( CHAR_DATA *ch, CHAR_DATA *victim, int level, char *texp )
{
   char buf[MIL];

   mudstrlcpy( buf, texp, sizeof( buf ) );
   return rd_parse( ch, victim, level, buf );
}

CMDF( do_checkdice )
{
   ch_printf( ch, "%s returned %d from dice_parse\r\n", argument, dice_parse( ch, ch, ch->level, argument ) );
}

bool save_chance( CHAR_DATA *ch, short percent )
{
   short ms;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return false;
   }

   ms = 10 - abs( ch->mental_state );
   if( ( number_percent( ) - ms ) <= percent )
      return true;
   else
      return false;
}

/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_poison_death( int level, CHAR_DATA *victim )
{
   int save;

   save = ( victim->level - level - victim->saving_poison_death );
   save = URANGE( 5, save, 95 );
   return save_chance( victim, save );
}

bool saves_wands( int level, CHAR_DATA *victim )
{
   int save;

   save = ( victim->level - level - victim->saving_wand );
   save = URANGE( 5, save, 95 );
   return save_chance( victim, save );
}

bool saves_para_petri( int level, CHAR_DATA *victim )
{
   int save;

   save = ( victim->level - level - victim->saving_para_petri );
   save = URANGE( 5, save, 95 );
   return save_chance( victim, save );
}

bool saves_breath( int level, CHAR_DATA *victim )
{
   int save;

   save = ( victim->level - level - victim->saving_breath );
   save = URANGE( 5, save, 95 );
   return save_chance( victim, save );
}

bool saves_spell_staff( int level, CHAR_DATA *victim )
{
   int save;

   if( is_npc( victim ) && level > 10 )
      level -= 5;
   save = ( victim->level - level - victim->saving_spell_staff );
   save = URANGE( 5, save, 95 );
   return save_chance( victim, save );
}

/*
 * Process the spell's required components, if any		-Thoric
 * -----------------------------------------------
 *    T#  check for item of type #
 *    V#  check for item of vnum #
 * Kword  check for item with keyword 'word'
 *    G#  check if player has # amount of gold
 *    H#  check if player has # amount of hitpoints
 *    M#  check if player has # amount of movement
 *
 * Special operators:
 * ! spell fails if player has this
 * + don't consume this component
 * @ decrease component's value[0], and extract if it reaches 0
 * # decrease component's value[1], and extract if it reaches 0
 * $ decrease component's value[2], and extract if it reaches 0
 * % decrease component's value[3], and extract if it reaches 0
 * ^ decrease component's value[4], and extract if it reaches 0
 * & decrease component's value[5], and extract if it reaches 0
 */
bool process_spell_components( CHAR_DATA *ch, int sn )
{
   SKILLTYPE *skill = get_skilltype( sn );
   char *comp = skill->components;
   char *check;
   char arg[MIL];
   bool consume, fail, found;
   int val, value;
   OBJ_DATA *obj;

   /* if no components necessary, then everything is cool */
   if( !comp || comp[0] == '\0' )
      return true;

   while( comp[0] != '\0' )
   {
      comp = one_argument( comp, arg );
      consume = true;
      fail = found = false;
      val = -1;
      switch( arg[1] )
      {
         default:
            check = arg + 1;
            break;

         case '!':
            check = arg + 2;
            fail = true;
            break;

         case '+':
            check = arg + 2;
            consume = false;
            break;

         case '@':
            check = arg + 2;
            val = 0;
            break;

         case '#':
            check = arg + 2;
            val = 1;
            break;

         case '$':
            check = arg + 2;
            val = 2;
            break;

         case '%':
            check = arg + 2;
            val = 3;
            break;

         case '^':
            check = arg + 2;
            val = 4;
            break;

         case '&':
            check = arg + 2;
            val = 5;
            break;
            /* reserve '*', '(' and ')' for v6, v7 and v8 */
      }
      value = atoi( check );
      obj = NULL;
      switch( UPPER( arg[0] ) )
      {
         case 'T':
            for( obj = ch->first_carrying; obj; obj = obj->next_content )
               if( obj->item_type == value )
               {
                  if( fail )
                  {
                     send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                     return false;
                  }
                  found = true;
                  break;
               }
            break;

         case 'V':
            for( obj = ch->first_carrying; obj; obj = obj->next_content )
               if( obj->pIndexData->vnum == value )
               {
                  if( fail )
                  {
                     send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                     return false;
                  }
                  found = true;
                  break;
               }
            break;

         case 'K':
            for( obj = ch->first_carrying; obj; obj = obj->next_content )
               if( nifty_is_name( check, obj->name ) )
               {
                  if( fail )
                  {
                     send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                     return false;
                  }
                  found = true;
                  break;
               }
            break;

         case 'G':
            if( has_gold( ch, value ) )
            {
               if( fail )
               {
                  send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                  return false;
               }
               else
               {
                  if( consume )
                  {
                     set_char_color( AT_GOLD, ch );
                     send_to_char( "You feel a little lighter...\r\n", ch );
                     decrease_gold( ch, value );
                  }
                  continue;
               }
            }
            break;

         case 'H':
            if( ch->hit >= value )
            {
               if( fail )
               {
                  send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                  return false;
               }
               else
               {
                  if( consume )
                  {
                     set_char_color( AT_BLOOD, ch );
                     send_to_char( "You feel a little weaker...\r\n", ch );
                     ch->hit -= value;
                     update_pos( ch );
                  }
                  continue;
               }
            }
            break;

         case 'M':
            if( ch->move >= value )
            {
               if( fail )
               {
                  send_to_char( "Something disrupts the casting of this spell...\r\n", ch );
                  return false;
               }
               else
               {
                  if( consume )
                  {
                     set_char_color( AT_BLOOD, ch );
                     send_to_char( "You feel a little slower...\r\n", ch );
                     ch->move -= value;
                     update_pos( ch );
                  }
                  continue;
               }
            }
            break;
      }
      /*
       * having this component would make the spell fail... if we get
       * here, then the caster didn't have that component 
       */
      if( fail )
         continue;
      if( !found )
      {
         send_to_char( "Something is missing...\r\n", ch );
         return false;
      }
      if( obj )
      {
         if( val >= 0 && val < 6 )
         {
            separate_obj( obj );
            if( obj->value[val] <= 0 )
            {
               act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
               act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
               extract_obj( obj );
               return false;
            }
            else if( --obj->value[val] == 0 )
            {
               act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
               act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
               extract_obj( obj );
            }
            else
               act( AT_MAGIC, "$p glows briefly and a whisp of smoke rises from it.", ch, obj, NULL, TO_CHAR );
         }
         else if( consume )
         {
            separate_obj( obj );
            act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
            act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
            extract_obj( obj );
         }
         else
         {
            int count = obj->count;

            obj->count = 1;
            act( AT_MAGIC, "$p glows briefly.", ch, obj, NULL, TO_CHAR );
            obj->count = count;
         }
      }
   }
   return true;
}

int pAbort;

/* Locate targets. */
/* Turn off annoying message and just abort if needed */
bool silence_locate_targets;

void *locate_targets( CHAR_DATA *ch, char *arg, int sn, CHAR_DATA **victim, OBJ_DATA **obj )
{
   SKILLTYPE *skill = get_skilltype( sn );
   void *vo = NULL;

   *victim = NULL;
   *obj = NULL;

   switch( skill->target )
   {
      default:
         bug( "%s: bad target for sn %d.", __FUNCTION__, sn );
         return &pAbort;

      case TAR_IGNORE:
         break;

      case TAR_CHAR_OFFENSIVE:
         if( arg[0] == '\0' )
         {
            if( !( *victim = who_fighting( ch ) ) )
            {
               if( !silence_locate_targets )
                  send_to_char( "Cast the spell on whom?\r\n", ch );
               return &pAbort;
            }
         }
         else
         {
            if( !( *victim = get_char_room( ch, arg ) ) )
            {
               if( !silence_locate_targets )
                  send_to_char( "They aren't here.\r\n", ch );
               return &pAbort;
            }
         }

         if( is_safe( ch, *victim, true ) )
            return &pAbort;

         if( ch == *victim )
         {
            if( SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) )
            {
               if( !silence_locate_targets )
                  send_to_char( "You can't cast this on yourself!\r\n", ch );
               return &pAbort;
            }
            if( !silence_locate_targets )
               send_to_char( "Cast this on yourself?  Okay...\r\n", ch );
            /*
             * send_to_char( "You can't do that to yourself.\r\n", ch );
             * return &pAbort;
             */
         }

         if( !is_npc( ch ) )
         {
            if( !is_npc( *victim ) )
            {
               if( get_timer( ch, TIMER_PKILLED ) > 0 )
               {
                  if( !silence_locate_targets )
                     send_to_char( "You have been killed in the last 5 minutes.\r\n", ch );
                  return &pAbort;
               }

               if( get_timer( *victim, TIMER_PKILLED ) > 0 )
               {
                  if( !silence_locate_targets )
                     send_to_char( "This player has been killed in the last 5 minutes.\r\n", ch );
                  return &pAbort;
               }
               if( xIS_SET( ch->act, PLR_NICE ) && ch != *victim )
               {
                  if( !silence_locate_targets )
                     send_to_char( "You're too nice to attack another player.\r\n", ch );
                  return &pAbort;
               }
               if( *victim != ch )
               {
                  if( !silence_locate_targets )
                     send_to_char( "You really shouldn't do this to another player...\r\n", ch );
                  else if( who_fighting( *victim ) != ch )
                  {
                     /*
                      * Only auto-attack those that are hitting you. 
                      */
                     return &pAbort;
                  }
               }
            }

            if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == *victim )
            {
               if( !silence_locate_targets )
                  send_to_char( "You can't do that on your own follower.\r\n", ch );
               return &pAbort;
            }
         }

         vo = ( void * )*victim;
         break;

      case TAR_CHAR_DEFENSIVE:
         {
            if( arg[0] == '\0' )
               *victim = ch;
            else
            {
               if( !( *victim = get_char_room( ch, arg ) ) )
               {
                  if( !silence_locate_targets )
                     send_to_char( "They aren't here.\r\n", ch );
                  return &pAbort;
               }
            }
         }

         if( ch == *victim && SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) )
         {
            if( !silence_locate_targets )
               send_to_char( "You can't cast this on yourself!\r\n", ch );
            return &pAbort;
         }

         vo = ( void * )*victim;
         break;

      case TAR_CHAR_SELF:
         if( arg[0] != '\0' && !nifty_is_name( arg, ch->name ) )
         {
            if( !silence_locate_targets )
               send_to_char( "You can't cast this spell on another.\r\n", ch );
            return &pAbort;
         }

         vo = ( void * )ch;
         break;

      case TAR_OBJ_INV:
      {
         if( !arg || arg[0] == '\0' )
         {
            if( !silence_locate_targets )
               send_to_char( "What should the spell be cast upon?\r\n", ch );
            return &pAbort;
         }

         if( !( *obj = get_obj_carry( ch, arg ) ) )
         {
            if( !silence_locate_targets )
               send_to_char( "You aren't carrying that.\r\n", ch );
            return &pAbort;
         }
      }

         vo = ( void * )*obj;
         break;
   }

   return vo;
}

/* The kludgy global is for spells who want more stuff from command line. */
char *target_name;
char *ranged_target_name = NULL;

/* Cast a spell.  Multi-caster and component support by Thoric */
CMDF( do_cast )
{
   char arg1[MIL], arg2[MIL];
   static char staticbuf[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   void *vo = NULL;
   int mana, sn;
   ch_ret retcode;
   bool dont_wait = false;
   SKILLTYPE *skill = NULL;
   struct timeval time_used;

   retcode = rNONE;

   if( IS_AFFECTED( ch, AFF_SILENCE ) )
   {
      send_to_char( "You are currently silenced and can't cast anything.\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         /* no ordering charmed mobs to cast spells */
         if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't seem to do that right now...\r\n", ch );
            return;
         }

         if( xIS_SET( ch->in_room->room_flags, ROOM_NO_MAGIC ) )
         {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You failed.\r\n", ch );
            return;
         }

         target_name = one_argument( argument, arg1 );
         one_argument( target_name, arg2 );
         STRFREE( ranged_target_name );
         ranged_target_name = STRALLOC( target_name );

         if( arg1 == NULL || arg1[0] == '\0' )
         {
            send_to_char( "Cast which what where?\r\n", ch );
            return;
         }

         /* Regular mortal spell casting */
         if( get_trust( ch ) < PERM_LEADER )
         {
            /* Check normal spells */
            if( ( sn = find_spell( ch, arg1, true ) ) >= 0 )
            {
               if( !is_npc( ch ) && !can_practice( ch, sn ) )
               {
                  send_to_char( "You can't do that.\r\n", ch );
                  return;
               }
               if( !( skill = get_skilltype( sn ) ) )
               {
                  send_to_char( "You can't do that right now...\r\n", ch );
                  return;
               }
               if( skill->type != SKILL_SPELL )
               {
                  send_to_char( "That isn't a spell.\r\n", ch );
                  return;
               }
            }
            else if( ( sn = ch_pers_lookup( ch->name, arg1 ) ) >= 0 )
            {
               sn += TYPE_PERS;
               if( !( skill = get_skilltype( sn ) ) )
               {
                  send_to_char( "You can't do that right now...\r\n", ch );
                  return;
               }
               if( !skill->tmpspell )
               {
                  send_to_char( "That isn't a spell.\r\n", ch );
                  return;
               }
            }
            else
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
         }
         else /* Godly "spell builder" spell casting with debugging messages */
         {
            /* First check normal spells */
            if( ( sn = spell_lookup( arg1 ) ) >= 0 )
            {
               if( !( skill = get_skilltype( sn ) ) )
               {
                  send_to_char( "Something is severely wrong with that one...\r\n", ch );
                  return;
               }
               if( skill->type != SKILL_SPELL )
               {
                  send_to_char( "That isn't a spell.\r\n", ch );
                  return;
               }
            }
            else if( ( sn = pers_lookup( arg1 ) ) >= 0 )
            {
               sn += TYPE_PERS;
               if( !( skill = get_skilltype( sn ) ) )
               {
                  send_to_char( "You can't do that right now...\r\n", ch );
                  return;
               }
               if( !skill->tmpspell )
               {
                  send_to_char( "That isn't a spell.\r\n", ch );
                  return;
               }
            }
            else
            {
               send_to_char( "We didn't create that yet...\r\n", ch );
               return;
            }

            if( !skill->spell_fun )
            {
               send_to_char( "We didn't finish that one yet...\r\n", ch );
               return;
            }
         }

         /*
          * Something else removed by Merc         -Thoric
          * Band-aid alert!  !is_npc check -- Blod 
          */
         if( ch->position < skill->minimum_position && !is_npc( ch ) )
         {
            switch( ch->position )
            {
               default:
                  send_to_char( "You can't concentrate enough.\r\n", ch );
                  break;

               case POS_SITTING:
                  send_to_char( "You can't summon enough energy sitting down.\r\n", ch );
                  break;

               case POS_RESTING:
                  send_to_char( "You're too relaxed to cast that spell.\r\n", ch );
                  break;

               case POS_FIGHTING:
                  if( skill->minimum_position <= POS_EVASIVE )
                     send_to_char( "This fighting style is too demanding for that!\r\n", ch );
                  else
                     send_to_char( "No way!  You're still fighting!\r\n", ch );
                  break;

               case POS_DEFENSIVE:
                  if( skill->minimum_position <= POS_EVASIVE )
                     send_to_char( "This fighting style is too demanding for that!\r\n", ch );
                  else
                     send_to_char( "No way!  You're still fighting!\r\n", ch );
                  break;

               case POS_AGGRESSIVE:
                  if( skill->minimum_position <= POS_EVASIVE )
                     send_to_char( "This fighting style is too demanding for that!\r\n", ch );
                  else
                     send_to_char( "No way!  You're still fighting!\r\n", ch );
                  break;

               case POS_BERSERK:
                  if( skill->minimum_position <= POS_EVASIVE )
                     send_to_char( "This fighting style is too demanding for that!\r\n", ch );
                  else
                     send_to_char( "No way!  You're still fighting!\r\n", ch );
                  break;

               case POS_EVASIVE:
                  send_to_char( "No way!  You're still fighting!\r\n", ch );
                  break;

               case POS_SLEEPING:
                  send_to_char( "You dream about great feats of magic.\r\n", ch );
                  break;
            }
            return;
         }

         if( skill->spell_fun == spell_null )
         {
            send_to_char( "That's not a spell!\r\n", ch );
            return;
         }

         if( !skill->spell_fun )
         {
            send_to_char( "You can't cast that... yet.\r\n", ch );
            return;
         }

         /* Mystaric, 980908 - Added checks for spell sector type */
         if( !ch->in_room
         || ( !xIS_EMPTY( skill->spell_sector ) && !xIS_SET( skill->spell_sector, ch->in_room->sector_type ) ) )
         {
            send_to_char( "You can't cast that here.\r\n", ch );
            return;
         }

         {
            int stat;

            for( stat = 0; stat < STAT_MAX; stat++ )
            {
               if( skill->stats[stat] <= 0 )
                  continue;
               if( skill->stats[stat] > get_curr_stat( stat, ch ) )
               {
                  ch_printf( ch, "You don't have enough %s to cast that.\r\n", stattypes[stat] );
                  return;
               }
            }
         }

         mana = is_npc( ch ) ? 0 : UMAX( skill->min_mana, mana_cost( ch, sn ) );

         /* Locate targets. */
         vo = locate_targets( ch, arg2, sn, &victim, &obj );
         if( vo == &pAbort )
            return;

         if( !is_npc( ch ) && victim && !is_npc( victim )
         && can_pkill( victim ) && !can_pkill( ch ) && !in_arena( ch ) && !in_arena( victim ) )
         {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "The gods won't permit you to cast spells on that character.\r\n", ch );
            return;
         }

         if( !is_immortal( ch ) )
         {
            if( ch->mana < mana )
            {
               ch_printf( ch, "You don't have enough %s.\r\n",
                  is_vampire( ch ) ? "blood power" : "mana" );
               return;
            }
         }

         if( skill->participants <= 1 )
            break;

         /*
          * multi-participant spells         -Thoric 
          */
         add_timer( ch, TIMER_DO_FUN, UMIN( skill->beats / 10, 3 ), do_cast, 1 );
         act( AT_MAGIC, "You begin to chant...", ch, NULL, NULL, TO_CHAR );
         act( AT_MAGIC, "$n begins to chant...", ch, NULL, NULL, TO_ROOM );
         snprintf( staticbuf, sizeof( staticbuf ), "%s %s", arg2, target_name );
         ch->alloc_ptr = STRALLOC( staticbuf );
         ch->tempnum = sn;
         return;

      case SUB_TIMER_DO_ABORT:
         STRFREE( ch->alloc_ptr );
         if( is_valid_sn( ( sn = ch->tempnum ) ) )
         {
            if( !( skill = get_skilltype( sn ) ) )
            {
               send_to_char( "Something went wrong...\r\n", ch );
               bug( "do_cast: SUB_TIMER_DO_ABORT: bad sn %d", sn );
               return;
            }
            mana = is_npc( ch ) ? 0 : UMAX( skill->min_mana, mana_cost( ch, sn ) );
            if( !is_immortal( ch ) )
               ch->mana -= mana / 3;
         }
         set_char_color( AT_MAGIC, ch );
         send_to_char( "You stop chanting...\r\n", ch );
         /*
          * should add chance of backfire here 
          */
         return;

      case 1:
         sn = ch->tempnum;
         if( !( skill = get_skilltype( sn ) ) )
         {
            send_to_char( "Something went wrong...\r\n", ch );
            bug( "do_cast: substate 1: bad sn %d", sn );
            return;
         }
         if( !ch->alloc_ptr || !is_valid_sn( sn ) || skill->type != SKILL_SPELL )
         {
            send_to_char( "Something cancels out the spell!\r\n", ch );
            bug( "do_cast: ch->alloc_ptr NULL or bad sn (%d)", sn );
            return;
         }
         mana = is_npc( ch ) ? 0 : UMAX( skill->min_mana, mana_cost( ch, sn ) );
         mudstrlcpy( staticbuf, ch->alloc_ptr, sizeof( staticbuf ) );
         target_name = one_argument( staticbuf, arg2 );
         STRFREE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         if( skill->participants > 1 )
         {
            int cnt = 1;
            CHAR_DATA *tmp;
            TIMER *t;

            for( tmp = ch->in_room->first_person; tmp; tmp = tmp->next_in_room )
               if( tmp != ch
                   && ( t = get_timerptr( tmp, TIMER_DO_FUN ) )
                   && t->count >= 1 && t->do_fun == do_cast
                   && tmp->tempnum == sn && tmp->alloc_ptr && !str_cmp( tmp->alloc_ptr, staticbuf ) )
                  ++cnt;
            if( cnt >= skill->participants )
            {
               for( tmp = ch->in_room->first_person; tmp; tmp = tmp->next_in_room )
                  if( tmp != ch
                  && ( t = get_timerptr( tmp, TIMER_DO_FUN ) )
                  && t->count >= 1 && t->do_fun == do_cast
                  && tmp->tempnum == sn && tmp->alloc_ptr && !str_cmp( tmp->alloc_ptr, staticbuf ) )
                  {
                     extract_timer( tmp, t );
                     act( AT_MAGIC, "Channeling your energy into $n, you help cast the spell!", ch, NULL, tmp, TO_VICT );
                     act( AT_MAGIC, "$N channels $S energy into you!", ch, NULL, tmp, TO_CHAR );
                     act( AT_MAGIC, "$N channels $S energy into $n!", ch, NULL, tmp, TO_NOTVICT );
                     learn_from_success( tmp, sn );
                     if( !is_immortal( tmp ) )
                        tmp->mana -= mana;
                     tmp->substate = SUB_NONE;
                     tmp->tempnum = -1;
                     STRFREE( tmp->alloc_ptr );
                  }
               dont_wait = true;
               send_to_char( "You concentrate all the energy into a burst of mystical words!\r\n", ch );
               vo = locate_targets( ch, arg2, sn, &victim, &obj );
               if( vo == &pAbort )
                  return;
            }
            else
            {
               set_char_color( AT_MAGIC, ch );
               send_to_char( "There was not enough power for the spell to succeed...\r\n", ch );
               if( !is_immortal( ch ) )
                  ch->mana -= mana / 2;
               learn_from_failure( ch, sn );
               return;
            }
         }
   }

   say_spell( ch, sn );

   if( !dont_wait )
      wait_state( ch, skill->beats );

   /* Getting ready to cast... check for spell components  -Thoric */
   if( !process_spell_components( ch, sn ) )
   {
      if( !is_immortal( ch ) )
         ch->mana -= mana / 2;
      learn_from_failure( ch, sn );
      return;
   }

   if( skill->type != SKILL_PERSONAL && !is_npc( ch ) && ( number_percent( ) + skill->difficulty ) > ch->pcdata->learned[sn] )
   {
      /* Some more interesting loss of concentration messages  -Thoric */
      switch( number_bits( 2 ) )
      {
         case 0: /* too busy */
            if( ch->fighting )
               send_to_char( "This round of battle is too hectic to concentrate properly.\r\n", ch );
            else
               send_to_char( "You lost your concentration.\r\n", ch );
            break;

         case 1: /* irritation */
            if( number_bits( 2 ) == 0 )
            {
               switch( number_bits( 2 ) )
               {
                  case 0:
                     send_to_char( "A tickle in your nose prevents you from keeping your concentration.\r\n", ch );
                     break;
                  case 1:
                     send_to_char( "An itch on your leg keeps you from properly casting your spell.\r\n", ch );
                     break;
                  case 2:
                     send_to_char( "Something in your throat prevents you from uttering the proper phrase.\r\n", ch );
                     break;
                  case 3:
                     send_to_char( "A twitch in your eye disrupts your concentration for a moment.\r\n", ch );
                     break;
               }
            }
            else
               send_to_char( "Something distracts you, and you lose your concentration.\r\n", ch );
            break;

         case 2: /* not enough time */
            if( ch->fighting )
               send_to_char( "There wasn't enough time this round to complete the casting.\r\n", ch );
            else
               send_to_char( "You lost your concentration.\r\n", ch );
            break;

         case 3:
            send_to_char( "You get a mental block mid-way through the casting.\r\n", ch );
            break;
      }
      if( !is_immortal( ch ) )
         ch->mana -= mana / 2;
      learn_from_failure( ch, sn );
      return;
   }
   else
   {
      if( !is_immortal( ch ) )
         ch->mana -= mana;

      start_timer( &time_used );
      retcode = ( *skill->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );
      update_userec( &time_used, &skill->userec );
   }

   if( retcode == rCHAR_DIED || retcode == rERROR || char_died( ch ) )
      return;

   /* learning */
   if( retcode != rSPELL_FAILED )
      learn_from_success( ch, sn );
   else
      learn_from_failure( ch, sn );

   /* favor adjustments */
   if( victim && victim != ch && !is_npc( victim ) && skill->target == TAR_CHAR_DEFENSIVE )
      adjust_favor( ch, 5, 1 );

   if( victim && victim != ch && !is_npc( ch ) && skill->target == TAR_CHAR_DEFENSIVE )
      adjust_favor( victim, 10, 1 );

   if( victim && victim != ch && !is_npc( ch ) && skill->target == TAR_CHAR_OFFENSIVE )
      adjust_favor( ch, 2, 1 );

   /* Fixed up a weird mess here, and added double safeguards -Thoric */
   if( skill->target == TAR_CHAR_OFFENSIVE && victim && !char_died( victim ) && victim != ch )
   {
      CHAR_DATA *vch, *vch_next;

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;

         if( vch == victim )
         {
            if( vch->master != ch && !vch->fighting )
               retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
            break;
         }
      }
   }
}

/* Cast spells at targets using a magical object. */
ch_ret obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
   void *vo;
   ch_ret retcode = rNONE;
   int levdiff = ch->level - level;
   SKILLTYPE *skill = get_skilltype( sn );
   struct timeval time_used;

   if( sn == -1 )
      return retcode;
   if( !skill || !skill->spell_fun )
   {
      bug( "Obj_cast_spell: bad sn %d.", sn );
      return rERROR;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_NO_MAGIC ) )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "Nothing seems to happen...\r\n", ch );
      return rNONE;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) && skill->target == TAR_CHAR_OFFENSIVE )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "Nothing seems to happen...\r\n", ch );
      return rNONE;
   }

   /*
    * Basically this was added to cut down on level 5 players using level
    * 40 scrolls in battle too often ;)     -Thoric
    */
   if( ( skill->target == TAR_CHAR_OFFENSIVE || number_bits( 7 ) == 1 ) /* 1/128 chance if non-offensive */
   && skill->type != SKILL_HERB && skill->type != SKILL_PERSONAL && !chance( ch, 95 + levdiff ) )
   {
      switch( number_bits( 2 ) )
      {
         case 0:
            failed_casting( skill, ch, victim, NULL );
            break;

         case 1:
            act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
            if( victim )
               act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
            act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
            return damage( ch, ch, NULL, number_range( 1, level ), TYPE_UNDEFINED, true );

         case 2:
            failed_casting( skill, ch, victim, NULL );
            break;

         case 3:
            act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
            if( victim )
               act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
            act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
            return damage( ch, ch, NULL, number_range( 1, level ), TYPE_UNDEFINED, true );
      }
      return rNONE;
   }

   target_name = (char *)"";
   switch( skill->target )
   {
      default:
         bug( "Obj_cast_spell: bad target for sn %d.", sn );
         return rERROR;

      case TAR_IGNORE:
         vo = NULL;
         if( victim )
            target_name = victim->name;
         else if( obj )
            target_name = obj->name;
         break;

      case TAR_CHAR_OFFENSIVE:
         if( victim != ch )
         {
            if( !victim )
               victim = who_fighting( ch );
            if( !victim || ( !is_npc( victim ) && !in_arena( victim ) ) )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return rNONE;
            }
         }
         if( ch != victim && is_safe( ch, victim, true ) )
            return rNONE;
         vo = ( void * )victim;
         break;

      case TAR_CHAR_DEFENSIVE:
         if( !victim )
            victim = ch;
         vo = ( void * )victim;
         break;

      case TAR_CHAR_SELF:
         vo = ( void * )ch;
         break;

      case TAR_OBJ_INV:
         if( !obj )
         {
            send_to_char( "You can't do that.\r\n", ch );
            return rNONE;
         }
         vo = ( void * )obj;
         break;
   }

   start_timer( &time_used );
   retcode = ( *skill->spell_fun ) ( sn, level, ch, vo );
   end_timer( &time_used );
   update_userec( &time_used, &skill->userec );

   if( retcode == rSPELL_FAILED )
      retcode = rNONE;

   if( retcode == rCHAR_DIED || retcode == rERROR )
      return retcode;

   if( char_died( ch ) )
      return rCHAR_DIED;

   if( skill->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) )
   {
      CHAR_DATA *vch;
      CHAR_DATA *vch_next;

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;
         if( victim == vch && !vch->fighting && vch->master != ch )
         {
            retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
            break;
         }
      }
   }

   return retcode;
}

NM ch_ret spell_call_lightning( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *vch, *vch_next;
   int dam;
   bool ch_died;
   ch_ret retcode = rNONE;

   if( !is_outside( ch ) )
   {
      send_to_char( "You must be outside.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( no_weather_sect( ch->in_room ) )
   {
      send_to_char( "You aren't able to use the weather from here.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( ch->in_room->area->weather->precip <= 0 )
   {
      send_to_char( "You need bad weather.\r\n", ch );
      return rSPELL_FAILED;
   }

   dam = dice( level / 2, 8 );

   set_char_color( AT_MAGIC, ch );
   send_to_char( "You call lightning to strike your foes!\r\n", ch );
   act( AT_MAGIC, "$n calls lightning to strike $s foes!", ch, NULL, NULL, TO_ROOM );

   ch_died = false;
   for( vch = first_char; vch; vch = vch_next )
   {
      vch_next = vch->next;
      if( !vch->in_room )
         continue;
      if( vch->in_room == ch->in_room )
      {
         if( !is_npc( vch ) && xIS_SET( vch->act, PLR_WIZINVIS ) && vch->pcdata->wizinvis >= get_trust( ch ) )
            continue;

         if( vch != ch && ( is_npc( ch ) ? !is_npc( vch ) : is_npc( vch ) ) )
            retcode = damage( ch, vch, NULL, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn, true );
         if( retcode == rCHAR_DIED || char_died( ch ) )
            ch_died = true;
         continue;
      }

      if( !ch_died && vch->in_room->area == ch->in_room->area && is_outside( vch ) && is_awake( vch ) )
      {
         if( number_bits( 3 ) == 0 )
            send_to_char( "&BLightning flashes in the sky.\r\n", vch );
      }
   }

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

bool can_charm( CHAR_DATA *ch )
{
   if( is_npc( ch ) || is_immortal( ch ) )
      return true;
   if( ( ( get_curr_cha( ch ) / 3 ) + 1 ) > ch->pcdata->charmies )
      return true;
   return false;
}

NM ch_ret spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   AFFECT_DATA af;
   int schance;
   SKILLTYPE *skill = get_skilltype( sn );

   if( victim == ch )
   {
      send_to_char( "You like yourself even better!\r\n", ch );
      return rSPELL_FAILED;
   }

   if( !is_npc( victim ) && !is_npc( ch ) )
   {
      send_to_char( "I don't think so...\r\n", ch );
      send_to_char( "You feel charmed...\r\n", victim );
      return rSPELL_FAILED;
   }

   schance = ris_save( victim, level, RIS_CHARM );

   if( schance > 100 )
   {
      absorb_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( schance == 100 )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( IS_AFFECTED( victim, AFF_CHARM ) || IS_AFFECTED( ch, AFF_CHARM )
   || level < victim->level || circle_follow( victim, ch )
   || !can_charm( ch ) || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->master )
      stop_follower( victim );
   add_follower( victim, ch );
   af.type = sn;
   af.duration = ( int )( ( ( ( level + 1 ) / 5 ) + 1 ) * DUR_CONV );
   af.location = APPLY_EXT_AFFECT;
   af.modifier = AFF_CHARM;
   af.bitvector = meb( AFF_CHARM );
   affect_to_char( victim, &af );
   successful_casting( skill, ch, victim, NULL );

   log_printf_plus( LOG_NORMAL, get_trust( ch ), "%s has charmed %s.", ch->name, victim->name );
   if( !is_npc( ch ) )
      ch->pcdata->charmies++;
   if( is_npc( victim ) )
   {
      if( !xIS_SET( victim->act, ACT_PACIFIST ) )
      {
         start_hating( victim, ch );
         if( !xIS_SET( victim->act, ACT_SENTINEL ) )
            start_hunting( victim, ch );
      }
   }
   return rNONE;
}

void single_weather_update( AREA_DATA *pArea );

NM ch_ret spell_control_weather( int sn, int level, CHAR_DATA *ch, void *vo )
{
   SKILLTYPE *skill = get_skilltype( sn );
   WEATHER_DATA *weath;
   int change;
   weath = ch->in_room->area->weather;

   if( !is_outside( ch ) )
   {
      send_to_char( "You must be outside to control the weather.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( no_weather_sect( ch->in_room ) )
   {
      send_to_char( "You aren't able to change the weather from here.\r\n", ch );
      return rSPELL_FAILED;
   }

   change = number_range( -rand_factor, rand_factor ) + ( ch->level * 3 ) / ( 2 * max_vector );

   if( !str_cmp( target_name, "warmer" ) )
      weath->temp_vector += change;
   else if( !str_cmp( target_name, "colder" ) )
      weath->temp_vector -= change;
   else if( !str_cmp( target_name, "wetter" ) )
      weath->precip_vector += change;
   else if( !str_cmp( target_name, "drier" ) )
      weath->precip_vector -= change;
   else if( !str_cmp( target_name, "windier" ) )
      weath->wind_vector += change;
   else if( !str_cmp( target_name, "calmer" ) )
      weath->wind_vector -= change;
   else
   {
      send_to_char( "Do you want it to get warmer, colder, wetter, drier, windier, or calmer?\r\n", ch );
      return rSPELL_FAILED;
   }

   weath->temp_vector = URANGE( -max_vector, weath->temp_vector, max_vector );
   weath->precip_vector = URANGE( -max_vector, weath->precip_vector, max_vector );
   weath->wind_vector = URANGE( -max_vector, weath->wind_vector, max_vector );

   successful_casting( skill, ch, NULL, NULL );

   /* Go ahead and update the weather */
   single_weather_update( ch->in_room->area );

   return rNONE;
}

NM ch_ret spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *mushroom;

   if( !( mushroom = create_object( get_obj_index( OBJ_VNUM_MUSHROOM ), 0 ) ) )
   {
      bug( "%s: Object vnum %d couldn't be created", __FUNCTION__, OBJ_VNUM_MUSHROOM );
      return rNONE;
   }
   if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) )
   {
      send_to_char( "A magical force prevents you from casting that here.\r\n", ch );
      return rSPELL_FAILED;
   }   
   mushroom->value[0] = 5 + level;
   act( AT_MAGIC, "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM );
   act( AT_MAGIC, "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR );
   obj_to_room( mushroom, ch->in_room );
   if( mushroom )
      obj_to_char_cords( mushroom, ch );
   return rNONE;
}

NM ch_ret spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   WEATHER_DATA *weath;
   int water;

   if( obj->item_type != ITEM_DRINK_CON )
   {
      send_to_char( "It is unable to hold water.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( obj->value[2] != LIQ_WATER && obj->value[1] > 0 )
   {
      send_to_char( "It contains some other liquid.\r\n", ch );
      return rSPELL_FAILED;
   }

   weath = ch->in_room->area->weather;

   water = UMIN( level * ( weath->precip >= 0 ? 4 : 2 ), obj->value[0] - obj->value[1] );

   if( water > 0 )
   {
      separate_obj( obj );
      obj->value[2] = LIQ_WATER;
      obj->value[1] += water;
      if( !is_name( "water", obj->name ) )
      {
         char buf[MSL];

         snprintf( buf, sizeof( buf ), "%s water", obj->name );
         STRFREE( obj->name );
         obj->name = STRALLOC( buf );
      }
      act( AT_MAGIC, "$p is filled.", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      send_to_char( "It failed to hold any water.\r\n", ch );
      return rSPELL_FAILED;
   }

   return rNONE;
}

NM ch_ret spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;

   set_char_color( AT_MAGIC, ch );
   if( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD
   || obj->item_type == ITEM_COOK || obj->item_type == ITEM_FISH )
   {
      if( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
         send_to_char( "It looks undercooked.\r\n", ch );
      else if( obj->value[3] > 0 )
         send_to_char( "You smell poisonous fumes.\r\n", ch );
      else
         send_to_char( "It looks very delicious.\r\n", ch );
   }
   else
      send_to_char( "It doesn't look poisoned.\r\n", ch );

   return rNONE;
}

ch_ret spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam;

   if( !is_npc( ch ) && is_evil( ch ) )
      victim = ch;

   if( is_good( victim ) )
   {
      act( AT_MAGIC, "Thoric protects $N.", ch, NULL, victim, TO_ROOM );
      return rSPELL_FAILED;
   }

   if( is_neutral( victim ) )
   {
      act( AT_MAGIC, "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
      return rSPELL_FAILED;
   }

   dam = dice( level, 4 );
   if( saves_spell_staff( level, victim ) )
      dam /= 2;

   return damage( ch, victim, NULL, dam, sn, true );
}

/*
 * New version of dispel magic fixes alot of bugs, and allows players
 * to not lose thie affects if they have the spell and the affect.
 * Also prints a message to the victim, and does various other things :)
 * Shaddai
 */
ch_ret spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int cnt = 0, affect_num, affected_by = 0, times = 0;
   int schance;
   SKILLTYPE *skill = get_skilltype( sn );
   AFFECT_DATA *paf;
   bool found = false, twice = false, three = false;
   bool is_mage = false;

   set_char_color( AT_MAGIC, ch );

   schance = ( get_curr_int( ch ) - get_curr_int( victim ) );

   if( is_npc( ch ) )
      is_mage = true;

   if( is_mage )
      schance += 5;
   else
      schance -= 15;

   if( ch == victim )
   {
      send_to_char( "You pass your hands around your body...\r\n", ch );
      if( ch->first_affect )
      {
         while( ch->first_affect )
            affect_remove( ch, ch->first_affect );
         if( !is_npc( ch ) )  /* Stop the NPC bug  Shaddai */
            update_aris( victim );
      }
      return rNONE;
   }

   if( !is_mage && !IS_AFFECTED( ch, AFF_DETECT_MAGIC ) )
   {
      send_to_char( "You don't sense a magical aura to dispel.\r\n", ch );
      return rERROR; /* You don't cast it so don't attack */
   }

   if( number_percent( ) > ( 75 - schance ) )
   {
      twice = true;
      if( number_percent( ) > ( 75 - schance ) )
         three = true;
   }

 start_loop:

   /* Grab affected_by from mobs first */
   if( is_npc( victim ) && !xIS_EMPTY( victim->affected_by ) )
   {
      for( ;; )
      {
         affected_by = number_range( 1, AFF_MAX - 1 );
         if( xIS_SET( victim->affected_by, affected_by ) )
         {
            found = true;
            break;
         }
         if( cnt++ > 30 )
         {
            found = false;
            break;
         }
      }
      if( found ) /* Ok lets see if it is a spell */
      {
         for( paf = victim->first_affect; paf; paf = paf->next )
            if( xIS_SET( paf->bitvector, affected_by ) )
               break;
         if( paf ) /* It is a spell lets remove the spell too */
         {
            if( level < victim->level || saves_spell_staff( level, victim ) )
            {
               if( !dispel_casting( paf, ch, victim, false, false ) )
                  failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            if( SPELL_FLAG( get_skilltype( paf->type ), SF_NODISPEL ) )
            {
               if( !dispel_casting( paf, ch, victim, false, false ) )
                  failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            if( !dispel_casting( paf, ch, victim, false, true ) && times == 0 )
               successful_casting( skill, ch, victim, NULL );
            affect_remove( victim, paf );
            if( ( twice && times < 1 ) || ( three && times < 2 ) )
            {
               times++;
               goto start_loop;
            }
            return rNONE;
         }
         else  /* Nope not a spell just remove the bit *For Mobs Only* */
         {
            if( level < victim->level || saves_spell_staff( level, victim ) )
            {
               if( !dispel_casting( NULL, ch, victim, affected_by, false ) )
                  failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            if( !dispel_casting( NULL, ch, victim, affected_by, true ) && times == 0 )
               successful_casting( skill, ch, victim, NULL );
            xREMOVE_BIT( victim->affected_by, affected_by );
            if( ( twice && times < 1 ) || ( three && times < 2 ) )
            {
               times++;
               goto start_loop;
            }
            return rNONE;
         }
      }
   }

   /* Ok mob has no affected_by's or we didn't catch them lets go to first_affect. SHADDAI */
   if( !victim->first_affect )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   cnt = 0;

   /*
    * Need to randomize the affects, yes you have to loop on average 1.5 times
    * but dispel magic only takes at worst case 256 uSecs so who cares :)
    * Shaddai
    */
   for( paf = victim->first_affect; paf; paf = paf->next )
      cnt++;

   paf = victim->first_affect;

   for( affect_num = number_range( 0, ( cnt - 1 ) ); affect_num > 0; affect_num-- )
      paf = paf->next;

   if( level < victim->level || saves_spell_staff( level, victim ) )
   {
      if( !dispel_casting( paf, ch, victim, false, false ) )
         failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   /* Need to make sure we have an affect and it isn't no dispel */
   if( !paf || SPELL_FLAG( get_skilltype( paf->type ), SF_NODISPEL ) )
   {
      if( !dispel_casting( paf, ch, victim, false, false ) )
         failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( !dispel_casting( paf, ch, victim, false, true ) && times == 0 )
      successful_casting( skill, ch, victim, NULL );
   affect_remove( victim, paf );
   if( ( twice && times < 1 ) || ( three && times < 2 ) )
   {
      times++;
      goto start_loop;
   }

   /* Have to reset victim affects */
   if( !is_npc( victim ) )
      update_aris( victim );
   return rNONE;
}

NM ch_ret spell_polymorph( int sn, int level, CHAR_DATA *ch, void *vo )
{
   MORPH_DATA *morph;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !target_name || target_name[0] == '\0' )
   {
      send_to_char( "What would you like to morph into?\r\n", ch );
      show_morphs( ch );
      return rSPELL_FAILED;
   }
   if( !( morph = find_morph( ch, target_name, true ) ) )
   {
      send_to_char( "You can't morph into anything like that!\r\n", ch );
      return rSPELL_FAILED;
   }
   if( !do_morph_char( ch, morph ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }
   return rNONE;
}

ch_ret spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *vch, *vch_next;
   ROOM_INDEX_DATA *room;
   bool ch_died;
   ch_ret retcode;
   SKILLTYPE *skill = get_skilltype( sn );

   ch_died = false;
   retcode = rNONE;

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   act( AT_MAGIC, "The earth trembles beneath your feet!", ch, NULL, NULL, TO_CHAR );
   act( AT_MAGIC, "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

   room = ch->in_room;
   for( vch = room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;
      if( !is_npc( vch ) && xIS_SET( vch->act, PLR_WIZINVIS ) && vch->pcdata->wizinvis >= get_trust( ch ) )
         continue;
      if( IS_AFFECTED( vch, AFF_FLYING ) || IS_AFFECTED( vch, AFF_FLOATING ) )
         continue;
      if( !can_see_character( ch, vch ) )
         continue;
      if( vch != ch && ( is_npc( ch ) ? !is_npc( vch ) : is_npc( vch ) ) )
         retcode = damage( ch, vch, NULL, ( level + dice( 2, 8 ) ), sn, true );
      if( retcode == rCHAR_DIED || char_died( ch ) )
      {
         ch_died = true;
         continue;
      }
   }

   if( !ch_died )
   {
      for( vch = first_char; vch; vch = vch_next )
      {
         vch_next = vch->next;

         if( is_npc( vch ) || vch == ch || vch->in_room == room || vch->in_room->area != room->area )
            continue;
         if( number_bits( 3 ) == 0 )
            send_to_char( "&BThe earth trembles and shivers.\r\n", vch );
      }
      return rNONE;
   }

   return rCHAR_DIED;
}


/* Drain MANA, MOVE, HP. Caster gains HP. */
ch_ret spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam;
   int schance;
   SKILLTYPE *skill = get_skilltype( sn );

   dam = dice( 1, UMAX( 1, level ) );

   schance = ris_save( victim, victim->level, RIS_DRAIN );
   if( schance == 100 )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( schance > 100 )
   {
      victim->hit = URANGE( 0, victim->hit + dam, victim->max_hit );
      update_pos( victim );
      absorb_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );   /* SB */
      return rSPELL_FAILED;
   }

   ch->alignment = UMAX( -1000, ch->alignment - 200 );

   if( victim->mana > 0 )
      victim->mana /= 2;
   if( victim->move > 0 )
      victim->move /= 2;
   schance = victim->hit;
   damage( ch, victim, NULL, dam, sn, true );
   if( victim->hit < schance )
      ch->hit = URANGE( 0, ch->hit + ( schance - victim->hit ), ch->max_hit );
   return rNONE;
}

NM ch_ret spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   AFFECT_DATA af;
   SKILLTYPE *skill = get_skilltype( sn );

   if( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   af.type = sn;
   af.duration = ( int )( level * DUR_CONV );
   af.location = APPLY_ARMOR;
   af.modifier = 2 * level;
   af.bitvector = meb( AFF_FAERIE_FIRE );
   affect_to_char( victim, &af );
   act( AT_PINK, "You're surrounded by a pink outline.", victim, NULL, NULL, TO_CHAR );
   act( AT_PINK, "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM );
   return rNONE;
}

NM ch_ret spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *ich;

   act( AT_MAGIC, "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
   act( AT_MAGIC, "You conjure a cloud of purple smoke.", ch, NULL, NULL, TO_CHAR );

   for( ich = ch->in_room->first_person; ich; ich = ich->next_in_room )
   {
      if( !is_npc( ich ) && xIS_SET( ich->act, PLR_WIZINVIS ) )
         continue;

      if( ich == ch || saves_spell_staff( level, ich ) )
         continue;

      affect_strip( ich, gsn_hide );
      affect_strip( ich, gsn_invis );
      affect_strip( ich, gsn_sneak );
      xREMOVE_BIT( ich->affected_by, AFF_HIDE );
      xREMOVE_BIT( ich->affected_by, AFF_INVISIBLE );
      xREMOVE_BIT( ich->affected_by, AFF_SNEAK );
      act( AT_MAGIC, "$n is revealed!", ich, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You're revealed!", ich, NULL, NULL, TO_CHAR );
   }
   return rNONE;
}

NM ch_ret spell_gate( int sn, int level, CHAR_DATA *ch, void *vo )
{
   MOB_INDEX_DATA *temp;
   CHAR_DATA *vamp = NULL, *v_next = NULL;
   int count = 0;

   if( !( temp = get_mob_index( MOB_VNUM_VAMPIRE ) ) )
   {
      bug( "%s: Vampire vnum %d doesn't exist.", __FUNCTION__, MOB_VNUM_VAMPIRE );
      return rSPELL_FAILED;
   }
   for( vamp = ch->in_room->first_person; vamp; vamp = v_next )
   {
      v_next = vamp->next_in_room;

      if( is_npc( vamp ) && vamp->pIndexData->vnum == MOB_VNUM_VAMPIRE && ++count >= 5 )
      {
         send_to_char( "There are already enough guardian vampires here.\r\n", ch );
         return rSPELL_FAILED;
      }
   }
   if( !( vamp = create_mobile( temp ) ) )
   {
      bug( "%s: failed to create_mobile for vnum %d.", __FUNCTION__, MOB_VNUM_VAMPIRE );
      return rSPELL_FAILED;
   }
   char_to_room( vamp, ch->in_room );
   act( AT_MAGIC, "$N is gated in to do $n's bidding.", ch, NULL, vamp, TO_ROOM );
   act( AT_MAGIC, "$N is gated in to do your bidding.", ch, NULL, vamp, TO_CHAR );
   return rNONE;
}

NM ch_ret spell_harm( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam;

   dam = UMAX( 20, victim->hit - dice( 1, 4 ) );

   if( saves_spell_staff( level, victim ) )
      dam = UMIN( 50, dam / 4 );
   dam = UMIN( 100, dam );

   return damage( ch, victim, NULL, dam, sn, true );
}

NM ch_ret spell_identify( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj;
   CHAR_DATA *victim;
   AFFECT_DATA *paf;
   SKILLTYPE *sktmp;
   char *name;

   if( !target_name || target_name[0] == '\0' )
   {
      send_to_char( "What should the spell be cast upon?\r\n", ch );
      return rSPELL_FAILED;
   }

   if( ( obj = get_obj_carry( ch, target_name ) ) )
   {
      show_obj( ch, obj );
      return rNONE;
   }
   else if( ( victim = get_char_room( ch, target_name ) ) )
   {
      if( victim->morph && victim->morph->morph )
         name = capitalize( victim->morph->morph->short_desc );
      else if( is_npc( victim ) )
         name = capitalize( victim->short_descr );
      else
         name = victim->name;

      ch_printf( ch, "%s appears to be between level %d and %d.\r\n",
         name, victim->level - ( victim->level % 5 ), victim->level - ( victim->level % 5 ) + 5 );

      if( is_npc( victim ) && victim->morph )
         ch_printf( ch, "%s appears to truly be %s.\r\n",
            name, ( ch->level > victim->level + 10 ) ? victim->name : "someone else" );

      if( !is_npc( victim ) )
         ch_printf( ch, "%s looks like %s, and follows the ways of the %s.\r\n",
            name, aoran( dis_race_name( victim->race ) ), dis_main_class_name( victim ) );

      if( ( chance( ch, 50 ) && ch->level >= victim->level + 10 ) || is_immortal( ch ) )
      {
         if( !victim->first_affect )
         {
            ch_printf( ch, "%s isn't affected by anything.\r\n", name );
            return rNONE;
         }

         ch_printf( ch, "%s appears to be affected by: ", name );

         for( paf = victim->first_affect; paf; paf = paf->next )
         {
            if( victim->first_affect != victim->last_affect )
            {
               if( paf != victim->last_affect && ( sktmp = get_skilltype( paf->type ) ) )
                  ch_printf( ch, "%s, ", sktmp->name );

               if( paf == victim->last_affect && ( sktmp = get_skilltype( paf->type ) ) )
               {
                  ch_printf( ch, "and %s.\r\n", sktmp->name );
                  return rNONE;
               }
            }
            else
            {
               if( ( sktmp = get_skilltype( paf->type ) ) )
                  ch_printf( ch, "%s.\r\n", sktmp->name );
               else
                  send_to_char( "\r\n", ch );
               return rNONE;
            }
         }
      }
   }
   else
   {
      ch_printf( ch, "You can't find %s!\r\n", target_name );
      return rSPELL_FAILED;
   }
   return rNONE;
}

/* Scryn 2/2/96 */
NM ch_ret spell_remove_invis( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !target_name || target_name[0] == '\0' )
   {
      send_to_char( "What should the spell be cast upon?\r\n", ch );
      return rSPELL_FAILED;
   }

   if( ( obj = get_obj_carry( ch, target_name ) ) )
   {
      if( !is_obj_stat( obj, ITEM_INVIS ) )
      {
         send_to_char( "Its not invisible!\r\n", ch );
         return rSPELL_FAILED;
      }

      xREMOVE_BIT( obj->extra_flags, ITEM_INVIS );
      act( AT_MAGIC, "$p becomes visible again.", ch, obj, NULL, TO_CHAR );

      send_to_char( "Ok.\r\n", ch );
      return rNONE;
   }
   else
   {
      CHAR_DATA *victim;

      victim = get_char_room( ch, target_name );

      if( victim )
      {
         if( !can_see( ch, victim ) )
         {
            ch_printf( ch, "You don't see %s!\r\n", target_name );
            return rSPELL_FAILED;
         }

         if( !IS_AFFECTED( victim, AFF_INVISIBLE ) )
         {
            send_to_char( "They aren't invisible!\r\n", ch );
            return rSPELL_FAILED;
         }

         if( is_safe( ch, victim, true ) )
         {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
         }

         if( !is_npc( victim ) )
         {
            if( chance( ch, 50 ) && ch->level + 10 < victim->level )
            {
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
         }
         else
         {
            if( chance( ch, URANGE( 1, level, 75 ) ) && ch->level + 15 < victim->level )
            {
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
         }

         affect_strip( victim, gsn_invis );
         xREMOVE_BIT( victim->affected_by, AFF_INVISIBLE );
         successful_casting( skill, ch, victim, NULL );
         return rNONE;
      }

      ch_printf( ch, "You can't find %s!\r\n", target_name );
      return rSPELL_FAILED;
   }
}

NM ch_ret spell_invis( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !target_name || target_name[0] == '\0' )
      victim = ch;
   else
      victim = get_char_room( ch, target_name );

   if( victim )
   {
      AFFECT_DATA af;

      if( IS_AFFECTED( victim, AFF_INVISIBLE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      act( AT_MAGIC, "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );
      af.type = sn;
      af.duration = ( int )( ( ( level / 4 ) + 12 ) * DUR_CONV );
      af.location = APPLY_EXT_AFFECT;
      af.modifier = AFF_INVISIBLE;
      af.bitvector = meb( AFF_INVISIBLE );
      affect_to_char( victim, &af );
      act( AT_MAGIC, "You fade out of existence.", victim, NULL, NULL, TO_CHAR );
      return rNONE;
   }
   else
   {
      OBJ_DATA *obj;

      if( ( obj = get_obj_carry( ch, target_name ) ) )
      {
         separate_obj( obj ); /* Fix multi-invis bug --Blod */
         if( is_obj_stat( obj, ITEM_INVIS ) )
         {
            failed_casting( skill, ch, NULL, NULL );
            return rSPELL_FAILED;
         }

         act( AT_MAGIC, "$p fades out of existence.", ch, obj, NULL, TO_CHAR );
         xSET_BIT( obj->extra_flags, ITEM_INVIS );
         return rNONE;
      }
   }
   ch_printf( ch, "You can't find %s!\r\n", target_name );
   return rSPELL_FAILED;
}

NM ch_ret spell_know_alignment( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   const char *msg;
   int ap;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !victim )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   ap = victim->alignment;

   if( ap > 700 )
      msg = "$N has an aura as white as the driven snow.";
   else if( ap > 350 )
      msg = "$N is of excellent moral character.";
   else if( ap > 100 )
      msg = "$N is often kind and thoughtful.";
   else if( ap > -100 )
      msg = "$N doesn't have a firm moral commitment.";
   else if( ap > -350 )
      msg = "$N lies to $S friends.";
   else if( ap > -700 )
      msg = "$N would just as soon kill you as look at you.";
   else
      msg = "I'd rather just not say anything at all about $N.";

   act( AT_MAGIC, msg, ch, NULL, victim, TO_CHAR );
   return rNONE;
}

NM ch_ret spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj;
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );
   bool removed = false;

   if( !victim )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( is_affected( victim, gsn_curse ) )
   {
      affect_strip( victim, gsn_curse );
      set_char_color( AT_MAGIC, victim );
      removed = true;
      send_to_char( "The weight of your curse is lifted.\r\n", victim );
      if( ch != victim )
      {
         act( AT_MAGIC, "You dispel the curses afflicting $N.", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$n's dispels the curses afflicting $N.", ch, NULL, victim, TO_NOTVICT );
      }
   }
   else if( victim->first_carrying )
   {
      for( obj = victim->first_carrying; obj; obj = obj->next_content )
      {
         if( !obj->in_obj && ( is_obj_stat( obj, ITEM_NOREMOVE ) || is_obj_stat( obj, ITEM_NODROP ) ) )
         {
            removed = true;
            separate_obj( obj );
            xREMOVE_BIT( obj->extra_flags, ITEM_NOREMOVE );
            xREMOVE_BIT( obj->extra_flags, ITEM_NODROP );
            set_char_color( AT_MAGIC, victim );
            act( AT_MAGIC, "You feel a burden released on $p.", ch, obj, victim, ch != victim ? TO_VICT : TO_CHAR );
            if( ch != victim )
            {
               act( AT_MAGIC, "You dispel a curse afflicting $N.", ch, NULL, victim, TO_CHAR );
               act( AT_MAGIC, "$n's dispels a curse afflicting $N.", ch, NULL, victim, TO_NOTVICT );
            }
            return rNONE;
         }
      }
   }
   if( !removed )
   {
      if( ch != victim )
         act( AT_MAGIC, "$N doesn't have any curses that need removed.", ch, NULL, victim, TO_CHAR );
      else
         act( AT_MAGIC, "You don't have any curses that need removed.", ch, NULL, NULL, TO_CHAR );
   }
   return rNONE;
}

NM ch_ret spell_remove_trap( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj, *trap;
   bool found;
   int retcode;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !target_name || target_name[0] == '\0' )
   {
      send_to_char( "Remove trap on what?\r\n", ch );
      return rSPELL_FAILED;
   }

   found = false;

   if( !ch->in_room || !ch->in_room->first_content )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return rNONE;
   }

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( can_see_obj( ch, obj ) && nifty_is_name( target_name, obj->name ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( !( trap = get_trap( obj ) ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   if( !chance( ch, 70 + get_curr_wis( ch ) ) )
   {
      send_to_char( "Ooops!\r\n", ch );
      retcode = spring_trap( ch, trap );
      if( retcode == rNONE )
         retcode = rSPELL_FAILED;
      return retcode;
   }

   extract_obj( trap );

   successful_casting( skill, ch, NULL, NULL );
   return rNONE;
}

NM ch_ret spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo )
{
   AFFECT_DATA af;
   int retcode, schance, tmp;
   CHAR_DATA *victim;
   char log_buf[MSL];
   SKILLTYPE *skill = get_skilltype( sn );

   if( !( victim = get_char_room( ch, target_name ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( !is_npc( victim ) && victim->fighting )
   {
      send_to_char( "You can't put a fighting player to sleep.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( is_safe( ch, victim, true ) )
      return rSPELL_FAILED;

   if( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !is_npc( ch ) && !is_npc( victim ) )
      tmp = level / 2;
   else
      tmp = level;

   schance = ris_save( victim, tmp, RIS_SLEEP );
   if( schance == 100 )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( schance > 100 )
   {
      absorb_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( IS_AFFECTED( victim, AFF_SLEEP )
   || level < victim->level
   || ( victim != ch && xIS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
   || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );
      if( ch == victim )
         return rSPELL_FAILED;
      if( !victim->fighting )
      {
         retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
         if( retcode == rNONE )
            retcode = rSPELL_FAILED;
         return retcode;
      }
   }
   af.type = sn;
   af.duration = ( int )( ( 4 + level ) * DUR_CONV );
   af.location = APPLY_EXT_AFFECT;
   af.modifier = AFF_SLEEP;
   af.bitvector = meb( AFF_SLEEP );
   affect_join( victim, &af );

   if( !is_npc( victim ) )
   {
      snprintf( log_buf, sizeof( log_buf ), "%s has cast sleep on %s.", ch->name, victim->name );
      log_string_plus( log_buf, LOG_NORMAL, get_trust( ch ) );
      to_channel( log_buf, "monitor", UMAX( PERM_IMM, get_trust( ch ) ) );
   }

   if( is_awake( victim ) )
   {
      act( AT_MAGIC, "You feel very sleepy ..... zzzzzz.", victim, NULL, NULL, TO_CHAR );
      act( AT_MAGIC, "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
      victim->position = POS_SLEEPING;
   }
   if( is_npc( victim ) )
   {
      if( !xIS_SET( victim->act, ACT_PACIFIST ) )
      {
         start_hating( victim, ch );
         if( !xIS_SET( victim->act, ACT_SENTINEL ) )
            start_hunting( victim, ch );
      }
   }
   return rNONE;
}

NM ch_ret spell_summon( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim;
   char buf[MSL];
   SKILLTYPE *skill = get_skilltype( sn );

   if( !( victim = get_char_world( ch, target_name ) )
   || victim == ch
   || !victim->in_room
   || xIS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
   || xIS_SET( victim->in_room->room_flags, ROOM_SAFE )
   || xIS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
   || xIS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_SUMMON )
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
   || victim->level >= ( level + 5 )
   || victim->fighting
   || ( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
   || ( is_npc( victim ) && saves_spell_staff( level, victim ) )
   || !in_hard_range( victim, ch->in_room->area )
   || ( !is_npc( ch ) && !can_pkill( ch ) && is_pkill( victim ) )
   || ( xIS_SET( ch->in_room->area->flags, AFLAG_NOPKILL ) && is_pkill( victim ) )
   || ( !is_npc( ch ) && !is_npc( victim ) && xIS_SET( victim->pcdata->flags, PCFLAG_NOSUMMON ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( ch->in_room->area != victim->in_room->area )
   {
      if( ( ( is_npc( ch ) != is_npc( victim ) ) && chance( ch, 30 ) )
      || ( ( is_npc( ch ) == is_npc( victim ) ) && chance( ch, 60 ) ) )
      {
         failed_casting( skill, ch, victim, NULL );
         set_char_color( AT_MAGIC, victim );
         send_to_char( "You feel a strange pulling sensation...\r\n", victim );
         return rSPELL_FAILED;
      }
   }

   if( !is_npc( ch ) )
   {
      act( AT_MAGIC, "You feel a wave of nausea overcome you...", ch, NULL, NULL, TO_CHAR );
      act( AT_MAGIC, "$n collapses, stunned!", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_STUNNED;

      snprintf( buf, sizeof( buf ), "%s summoned %s to room %d.", ch->name, victim->name, ch->in_room->vnum );
      log_string_plus( buf, LOG_NORMAL, get_trust( ch ) );
      to_channel( buf, "monitor", UMAX( PERM_IMM, get_trust( ch ) ) );
   }

   act( AT_MAGIC, "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
   char_from_room( victim );
   char_to_room( victim, ch->in_room );
   if( is_in_wilderness( ch ) )
   {
      if( is_npc( victim ) )
         xSET_BIT( victim->act, ACT_WILDERNESS );
      else
         xSET_BIT( victim->act, PLR_WILDERNESS );
      victim->cords[0] = ch->cords[0];
      victim->cords[1] = ch->cords[1];
   }
   act( AT_MAGIC, "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
   act( AT_MAGIC, "$N has summoned you!", victim, NULL, ch, TO_CHAR );
   do_look( victim, (char *)"auto" );
   return rNONE;
}

/*
 * Travel via the astral plains to quickly travel to desired location - Thoric
 * Uses SMAUG spell messages is available to allow use as a SMAUG spell
 */
NM ch_ret spell_astral_walk( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !( victim = get_char_world( ch, target_name ) )
   || !can_astral( ch, victim ) || !in_hard_range( ch, victim->in_room->area ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( skill->hit_char && skill->hit_char[0] != '\0' )
      act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
   if( skill->hit_vict && skill->hit_vict[0] != '\0' )
      act( AT_MAGIC, skill->hit_vict, ch, NULL, victim, TO_VICT );

   if( skill->hit_room && skill->hit_room[0] != '\0' )
      act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
   else
      act( AT_MAGIC, "$n disappears in a flash of light!", ch, NULL, NULL, TO_ROOM );
   char_from_room( ch );
   char_to_room( ch, victim->in_room );
   if( skill->hit_dest && skill->hit_dest[0] != '\0' )
      act( AT_MAGIC, skill->hit_dest, ch, NULL, victim, TO_NOTVICT );
   else
      act( AT_MAGIC, "$n appears in a flash of light!", ch, NULL, NULL, TO_ROOM );
   do_look( ch, (char *)"auto" );
   return rNONE;
}

ch_ret spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   ROOM_INDEX_DATA *pRoomIndex;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !victim->in_room
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
   || ( !is_npc( ch ) && victim->fighting )
   || ( victim != ch && ( saves_spell_staff( level, victim )
   || saves_wands( level, victim ) ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   for( ;; )
   {
      pRoomIndex = get_room_index( number_range( 0, MAX_VNUM ) );
      if( pRoomIndex )
         if( !xIS_SET( pRoomIndex->room_flags, ROOM_PRIVATE )
         && !xIS_SET( pRoomIndex->room_flags, ROOM_SOLITARY )
         && !xIS_SET( pRoomIndex->room_flags, ROOM_NO_ASTRAL )
         && !xIS_SET( pRoomIndex->area->flags, AFLAG_NOTELEPORT )
         && !xIS_SET( pRoomIndex->room_flags, ROOM_NO_RECALL )
         && in_hard_range( ch, pRoomIndex->area ) )
            break;
   }

   act( AT_MAGIC, "$n slowly fades out of view.", victim, NULL, NULL, TO_ROOM );
   char_from_room( victim );
   char_to_room( victim, pRoomIndex );
   if( !is_npc( victim ) )
      act( AT_MAGIC, "$n slowly fades into view.", victim, NULL, NULL, TO_ROOM );
   do_look( victim, (char *)"auto" );
   return rNONE;
}

/* Don't remove */
ch_ret spell_null( int sn, int level, CHAR_DATA *ch, void *vo )
{
   send_to_char( "That's not a spell!\r\n", ch );
   return rNONE;
}

/* Don't remove */
ch_ret spell_notfound( int sn, int level, CHAR_DATA *ch, void *vo )
{
   send_to_char( "That's not a spell!\r\n", ch );
   return rNONE;
}

/*   Haus' Spell Additions */

/* to do: portal           (like mpcreatepassage)
 *        sharpness        (makes weapon of caster's level)
 *        repair           (repairs armor)
 *        blood burn       (offensive)  * name: net book of spells *
 *        spirit scream    (offensive)  * name: net book of spells *
 *        something about saltpeter or brimstone
 */

/* Working on DM's transport eq suggestion - Scryn 8/13 */
NM ch_ret spell_transport( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim;
   char arg3[MSL];
   OBJ_DATA *obj;
   SKILLTYPE *skill = get_skilltype( sn );

   target_name = one_argument( target_name, arg3 );

   if( !( victim = get_char_world( ch, target_name ) )
   || victim == ch
   || xIS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
   || xIS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
   || xIS_SET( victim->in_room->room_flags, ROOM_DEATH )
   || xIS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
   || victim->level >= level + 15
   || ( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
   || ( is_npc( victim ) && saves_spell_staff( level, victim ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }


   if( victim->in_room == ch->in_room )
   {
      send_to_char( "They are right beside you!\r\n", ch );
      return rSPELL_FAILED;
   }

   if( !( obj = get_obj_carry( ch, arg3 ) )
   || ( victim->carry_weight + get_obj_weight( obj ) ) > can_carry_w( victim )
   || ( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   separate_obj( obj ); /* altrag shoots, haus alley-oops! */

   if( is_obj_stat( obj, ITEM_NODROP ) )
   {
      send_to_char( "You can't seem to let go of it.\r\n", ch );
      return rSPELL_FAILED;   /* nice catch, caine */
   }

   if( is_obj_stat( obj, ITEM_PROTOTYPE ) && get_trust( victim ) < PERM_IMM )
   {
      send_to_char( "That item is not for mortal hands to touch!\r\n", ch );
      return rSPELL_FAILED;   /* Thoric */
   }

   act( AT_MAGIC, "$p slowly dematerializes...", ch, obj, NULL, TO_CHAR );
   act( AT_MAGIC, "$p slowly dematerializes from $n's hands..", ch, obj, NULL, TO_ROOM );
   obj_from_char( obj );
   obj_to_char( obj, victim );
   act( AT_MAGIC, "$p from $n appears in your hands!", ch, obj, victim, TO_VICT );
   act( AT_MAGIC, "$p appears in $n's hands!", victim, obj, NULL, TO_ROOM );
   save_char_obj( ch );
   save_char_obj( victim );
   return rNONE;
}

/*
 * Usage portal (mob/char) 
 * opens a 2-way EX_PORTAL from caster's room to room inhabited by  
 *  mob or character won't mess with existing exits
 *
 * do_mp_open_passage, combined with spell_astral
 */
NM ch_ret spell_portal( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *targetRoom, *fromRoom;
   int targetRoomVnum, fromRoomVnum;
   OBJ_DATA *portalObj;
   EXIT_DATA *pexit;
   char buf[MSL];
   SKILLTYPE *skill = get_skilltype( sn );

   /*
    * No go if all kinds of things aren't just right, including the caster
    * and victim aren't both pkill or both peaceful. -- Narn
    */
   if( !( victim = get_char_world( ch, target_name ) )
   || victim == ch
   || !victim->in_room
   || xIS_SET( victim->in_room->room_flags, ROOM_WILDERNESS )
   || xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS )
   || xIS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
   || xIS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
   || xIS_SET( victim->in_room->room_flags, ROOM_DEATH )
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
   || xIS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
   || xIS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
   || victim->level >= level + 15
   || ( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
   || ( is_npc( victim ) && saves_spell_staff( level, victim ) )
   || ( !is_npc( victim ) && can_pkill( ch ) != can_pkill( victim ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->in_room == ch->in_room )
   {
      send_to_char( "They are right beside you!\r\n", ch );
      return rSPELL_FAILED;
   }

   fromRoomVnum = ch->in_room->vnum;
   targetRoomVnum = victim->in_room->vnum;
   fromRoom = ch->in_room;
   targetRoom = victim->in_room;

   /* Check if there already is a portal in either room. */
   for( pexit = fromRoom->first_exit; pexit; pexit = pexit->next )
   {
      if( xIS_SET( pexit->exit_info, EX_PORTAL ) )
      {
         send_to_char( "There is already a portal in this room.\r\n", ch );
         return rSPELL_FAILED;
      }

      if( pexit->vdir == DIR_PORTAL )
      {
         send_to_char( "You may not create a portal in this room.\r\n", ch );
         return rSPELL_FAILED;
      }
   }

   for( pexit = targetRoom->first_exit; pexit; pexit = pexit->next )
   {
      if( xIS_SET( pexit->exit_info, EX_PORTAL ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }
      if( pexit->vdir == DIR_PORTAL )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }
   }

   if( !( pexit = make_exit( fromRoom, targetRoom, DIR_PORTAL ) ) )
   {
      bug( "%s: couldn't make exit from room %d to room %d.", __FUNCTION__, fromRoom->vnum, targetRoom->vnum );
      return rNONE;
   }
   pexit->keyword = STRALLOC( "portal" );
   pexit->description = STRALLOC( "You gaze into the shimmering portal...\r\n" );
   pexit->key = -1;
   xSET_BIT( pexit->exit_info, EX_PORTAL );
   xSET_BIT( pexit->exit_info, EX_xENTER );
   xSET_BIT( pexit->exit_info, EX_HIDDEN );
   xSET_BIT( pexit->exit_info, EX_xLOOK );
   pexit->vnum = targetRoomVnum;

   if( !( portalObj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 ) ) )
   {
      bug( "%s: couldn't create object %d.", __FUNCTION__, OBJ_VNUM_PORTAL );
      return rNONE;
   }
   portalObj->timer = 3;

   snprintf( buf, sizeof( buf ), "a portal created by %s", ch->name );
   STRFREE( portalObj->short_descr );
   portalObj->short_descr = STRALLOC( buf );
   portalObj = obj_to_room( portalObj, ch->in_room );

   /* support for new casting messages */
   if( !skill->hit_char || skill->hit_char[0] == '\0' )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "You utter an incantation, and a portal forms in front of you!\r\n", ch );
   }
   else
      act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
   if( !skill->hit_room || skill->hit_room[0] == '\0' )
      act( AT_MAGIC, "$n utters an incantation, and a portal forms in front of you!", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_ROOM );
   if( !skill->hit_vict || skill->hit_vict[0] == '\0' )
      act( AT_MAGIC, "A shimmering portal forms in front of you!", victim, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, skill->hit_vict, victim, NULL, victim, TO_ROOM );

   if( !( pexit = make_exit( targetRoom, fromRoom, DIR_PORTAL ) ) )
   {
      bug( "%s: couldn't make exit to room %d from room %d.", __FUNCTION__, targetRoom->vnum, fromRoom->vnum );
      return rNONE;
   }
   pexit->keyword = STRALLOC( "portal" );
   pexit->description = STRALLOC( "You gaze into the shimmering portal...\r\n" );
   pexit->key = -1;
   xSET_BIT( pexit->exit_info, EX_PORTAL );
   xSET_BIT( pexit->exit_info, EX_xENTER );
   xSET_BIT( pexit->exit_info, EX_HIDDEN );
   xSET_BIT( pexit->exit_info, EX_xLOOK );
   pexit->vnum = fromRoomVnum;

   if( !( portalObj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 ) ) )
   {
      bug( "%s: couldn't create object %d.", __FUNCTION__, OBJ_VNUM_PORTAL );
      return rNONE;
   }
   portalObj->timer = 3;
   STRFREE( portalObj->short_descr );
   portalObj->short_descr = STRALLOC( buf );
   portalObj = obj_to_room( portalObj, targetRoom );
   return rNONE;
}

NM ch_ret spell_farsight( int sn, int level, CHAR_DATA *ch, void *vo )
{
   ROOM_INDEX_DATA *location, *original;
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   /*
    * The spell fails if the victim isn't playing, the victim is the caster,
    * the target room has private, solitary, noastral, death or proto flags,
    * the caster's room is norecall, the victim is too high in level, the 
    * victim is a proto mob, the victim makes the saving throw or the pkill 
    * flag on the caster is not the same as on the victim.  Got it?
    */
   if( !( victim = get_char_world( ch, target_name ) )
   || victim == ch
   || !victim->in_room
   || xIS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
   || xIS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
   || xIS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
   || xIS_SET( victim->in_room->room_flags, ROOM_DEATH )
   || xIS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
   || victim->level >= level + 15
   || ( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
   || ( is_npc( victim ) && saves_spell_staff( level, victim ) )
   || ( !is_npc( victim ) && can_pkill( victim ) && !can_pkill( ch ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   location = victim->in_room;
   if( !location )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   successful_casting( skill, ch, victim, NULL );
   original = ch->in_room;
   char_from_room( ch );
   char_to_room( ch, location );
   do_look( ch, (char *)"auto" );
   char_from_room( ch );
   char_to_room( ch, original );
   return rNONE;
}

NM ch_ret spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;

   if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
   {
      separate_obj( obj );
      if( obj->value[2] == obj->value[1] || obj->value[1] > ( obj->pIndexData->value[1] * 4 ) )
      {
         act( AT_FIRE, "$p bursts into flames, injuring you!", ch, obj, NULL, TO_CHAR );
         act( AT_FIRE, "$p bursts into flames, charring $n!", ch, obj, NULL, TO_ROOM );
         extract_obj( obj );
         if( damage( ch, ch, NULL, obj->level * 2, TYPE_UNDEFINED, true ) == rCHAR_DIED || char_died( ch ) )
            return rCHAR_DIED;
         else
            return rSPELL_FAILED;
      }

      if( chance( ch, 2 ) )
      {
         act( AT_YELLOW, "$p glows with a blinding magical luminescence.", ch, obj, NULL, TO_CHAR );
         obj->value[1] *= 2;
         obj->value[2] = obj->value[1];
         return rNONE;
      }
      else if( chance( ch, 5 ) )
      {
         act( AT_YELLOW, "$p glows brightly for a few seconds...", ch, obj, NULL, TO_CHAR );
         obj->value[2] = obj->value[1];
         return rNONE;
      }
      else if( chance( ch, 10 ) )
      {
         act( AT_WHITE, "$p disintegrates into a void.", ch, obj, NULL, TO_CHAR );
         act( AT_WHITE, "$n's attempt at recharging fails, and $p disintegrates.", ch, obj, NULL, TO_ROOM );
         extract_obj( obj );
         return rSPELL_FAILED;
      }
      else if( chance( ch, 50 - ( ch->level / 2 ) ) )
      {
         send_to_char( "Nothing happens.\r\n", ch );
         return rSPELL_FAILED;
      }
      else
      {
         act( AT_MAGIC, "$p feels warm to the touch.", ch, obj, NULL, TO_CHAR );
         --obj->value[1];
         obj->value[2] = obj->value[1];
         return rNONE;
      }
   }
   else
   {
      send_to_char( "You can't recharge that!\r\n", ch );
      return rSPELL_FAILED;
   }
}

/*
 * Animate Dead: Scryn 3/2/96
 * Modifications by Altrag 16/2/96
 */
NM ch_ret spell_animate_dead( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *mob;
   OBJ_DATA *corpse, *corpse_next, *obj, *obj_next;
   bool found;
   MOB_INDEX_DATA *pMobIndex, *acorpse;
   AFFECT_DATA af;
   char buf[MSL];
   SKILLTYPE *skill = get_skilltype( sn );

   found = false;

   for( corpse = ch->in_room->first_content; corpse; corpse = corpse_next )
   {
      corpse_next = corpse->next_content;

      if( corpse->item_type == ITEM_CORPSE_NPC && corpse->cost != -5 )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "You can't find a suitable corpse here.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( !( acorpse = get_mob_index( MOB_VNUM_ANIMATED_CORPSE ) ) )
   {
      bug( "%s: Vnum %d not found!", __FUNCTION__, MOB_VNUM_ANIMATED_CORPSE );
      return rNONE;
   }

   if( !( pMobIndex = get_mob_index( abs( corpse->cost ) ) ) )
   {
      bug( "%s: Can't find mob for cost [%d] of corpse", __FUNCTION__, abs( corpse->cost ) );
      return rNONE;
   }

   if( !is_npc( ch ) && !is_immortal( ch ) )
   {
      if( ch->mana < ( pMobIndex->level * 4 ) )
      {
         ch_printf( ch, "You don't have enough %s to reanimate this corpse.\r\n", is_vampire( ch ) ? "blood power" : "mana" );
         return rSPELL_FAILED;
      }
      ch->mana -= ( pMobIndex->level * 4 );
   }

   if( is_immortal( ch ) || ( chance( ch, 75 ) && pMobIndex->level - ch->level < 10 ) )
   {
      if( !( mob = create_mobile( acorpse ) ) )
      {
         bug( "%s: couldn't create_mobile for vnum %d.\r\n", __FUNCTION__, MOB_VNUM_ANIMATED_CORPSE );
         return rNONE;
      }
      char_to_room( mob, ch->in_room );
      mob->level = UMIN( ch->level / 2, pMobIndex->level );

      mob->max_hit = number_range( pMobIndex->minhit, pMobIndex->maxhit );
      mob->hit = mob->max_hit;
      mob->damroll = ch->level / 8;
      mob->hitroll = ch->level / 6;
      mob->alignment = ch->alignment;

      act( AT_MAGIC, "$n makes $T rise from the grave!", ch, NULL, pMobIndex->short_descr, TO_ROOM );
      act( AT_MAGIC, "You make $T rise from the grave!", ch, NULL, pMobIndex->short_descr, TO_CHAR );

      snprintf( buf, sizeof( buf ), "animated corpse %s", pMobIndex->name );
      STRSET( mob->name, buf );

      snprintf( buf, sizeof( buf ), "The animated corpse of %s", pMobIndex->short_descr );
      STRSET( mob->short_descr, buf );

      snprintf( buf, sizeof( buf ), "An animated corpse of %s struggles with the horror of its undeath.\r\n", pMobIndex->short_descr );
      STRSET( mob->long_descr, buf );

      add_follower( mob, ch );
      af.type = sn;
      af.duration = ( int )( ( ( ( level + 1 ) / 4 ) + 1 ) * DUR_CONV );
      af.location = APPLY_EXT_AFFECT;
      af.modifier = AFF_CHARM;
      af.bitvector = meb( AFF_CHARM );
      affect_to_char( mob, &af );

      if( corpse->first_content )
      {
         for( obj = corpse->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            obj_from_obj( obj );
            obj_to_room( obj, corpse->in_room );
         }
      }
      separate_obj( corpse );
      extract_obj( corpse );
      return rNONE;
   }

   failed_casting( skill, ch, NULL, NULL );
   return rSPELL_FAILED;
}

/* Ignores pickproofs, but can't unlock containers. -- Altrag 17/2/96 */
NM ch_ret spell_knock( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = NULL;
   EXIT_DATA *pexit = NULL;
   SKILLTYPE *skill = get_skilltype( sn );

   set_char_color( AT_MAGIC, ch );

   if( ms_find_obj( ch ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   if( !( pexit = find_door( ch, target_name, true ) ) && !( obj = get_obj_here( ch, target_name ) ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   if( pexit )
   {
      if( !xIS_SET( pexit->exit_info, EX_CLOSED )
      || !xIS_SET( pexit->exit_info, EX_LOCKED )
      || xIS_SET( pexit->exit_info, EX_PICKPROOF ) )
      {
         failed_casting( skill, ch, NULL, NULL );
         return rSPELL_FAILED;
      }
      xREMOVE_BIT( pexit->exit_info, EX_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      if( pexit->rexit && pexit->rexit->to_room == ch->in_room )
         xREMOVE_BIT( pexit->rexit->exit_info, EX_LOCKED );
      check_room_for_traps( ch, TRAP_UNLOCK | trap_door[pexit->vdir] );
      return rNONE;
   }

   if( obj )
   {
      if( obj->item_type != ITEM_CONTAINER
      || !IS_SET( obj->value[1], CONT_CLOSED )
      || obj->value[2] < 0
      || !IS_SET( obj->value[1], CONT_LOCKED )
      || IS_SET( obj->value[1], CONT_PICKPROOF ) )
      {
         failed_casting( skill, ch, NULL, NULL );
         return rSPELL_FAILED;
      }

      separate_obj( obj );
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      check_for_trap( ch, obj, TRAP_PICK );
      return rNONE;
   }

   return rNONE;
}

/* Tells to sleepers in area. -- Altrag 17/2/96 */
NM ch_ret spell_dream( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim;
   char arg[MIL];

   target_name = one_argument( target_name, arg );
   set_char_color( AT_MAGIC, ch );
   if( !( victim = get_char_world( ch, arg ) ) || victim->in_room->area != ch->in_room->area )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return rSPELL_FAILED;
   }
   if( victim->position != POS_SLEEPING )
   {
      send_to_char( "They aren't asleep.\r\n", ch );
      return rSPELL_FAILED;
   }
   if( !target_name )
   {
      send_to_char( "What do you want them to dream about?\r\n", ch );
      return rSPELL_FAILED;
   }

   set_char_color( AT_TELL, victim );
   ch_printf( victim, "You have dreams about %s telling you '%s'.\r\n", PERS( ch, victim ), target_name );
   successful_casting( get_skilltype( sn ), ch, victim, NULL );
   return rNONE;
}

 /*******************************************************
  * Everything after this point is part of SMAUG SPELLS *
  *******************************************************/

/* saving throw check - Thoric */
bool check_save( int sn, int level, CHAR_DATA *ch, CHAR_DATA * victim )
{
   SKILLTYPE *skill = get_skilltype( sn );
   bool saved = false;

   if( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !is_npc( ch ) && !is_npc( victim ) )
      level /= 2;

   if( skill->saves )
   {
      switch( skill->saves )
      {
         case SS_POISON_DEATH:
            saved = saves_poison_death( level, victim );
            break;
         case SS_ROD_WANDS:
            saved = saves_wands( level, victim );
            break;
         case SS_PARA_PETRI:
            saved = saves_para_petri( level, victim );
            break;
         case SS_BREATH:
            saved = saves_breath( level, victim );
            break;
         case SS_SPELL_STAFF:
            saved = saves_spell_staff( level, victim );
            break;
      }
   }
   return saved;
}

/* Generic offensive spell damage attack - Thoric */
ch_ret spell_attack( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );
   bool saved = check_save( sn, level, ch, victim );
   int dam;
   ch_ret retcode = rNONE;

   if( saved && SPELL_SAVE( skill ) == SE_NEGATE )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( skill->dice )
      dam = UMAX( 0, dice_parse( ch, victim, level, skill->dice ) );
   else
      dam = dice( 1, level / 2 );
   if( saved )
   {
      switch( SPELL_SAVE( skill ) )
      {
         case SE_3QTRDAM:
            dam = ( dam * 3 ) / 4;
            break;
         case SE_HALFDAM:
            dam >>= 1;
            break;
         case SE_QUARTERDAM:
            dam >>= 2;
            break;
         case SE_EIGHTHDAM:
            dam >>= 3;
            break;

         case SE_ABSORB:  /* victim absorbs spell for hp's */
            act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, victim, TO_CHAR );
            act( AT_MAGIC, "You absorb $N's $t!", victim, skill->noun_damage, ch, TO_CHAR );
            act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, victim, TO_NOTVICT );
            victim->hit = URANGE( 0, victim->hit + dam, victim->max_hit );
            update_pos( victim );
            if( skill->first_affect )
               retcode = spell_affectchar( sn, level, ch, victim );
            return retcode;

         case SE_REFLECT: /* reflect the spell to the caster */
            return spell_attack( sn, level, victim, ch );
      }
   }
   retcode = damage( ch, victim, NULL, dam, sn, true );
   if( retcode == rNONE && skill->first_affect
   && !char_died( ch ) && !char_died( victim )
   && ( !is_affected( victim, sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE ) || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
      retcode = spell_affectchar( sn, level, ch, victim );
   return retcode;
}

/* Generic area attack - Thoric */
ch_ret spell_area_attack( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *vch, *vch_next;
   SKILLTYPE *skill = get_skilltype( sn );
   bool saved;
   bool affects;
   int dam;
   bool ch_died = false;
   ch_ret retcode = rNONE;

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   affects = ( skill->first_affect ? true : false );
   if( skill->hit_char && skill->hit_char[0] != '\0' )
      act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
   if( skill->hit_room && skill->hit_room[0] != '\0' )
      act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );

   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;

      if( !is_npc( vch ) && xIS_SET( vch->act, PLR_WIZINVIS ) && vch->pcdata->wizinvis >= get_trust( ch ) )
         continue;

      if( vch == ch )
         continue;

      if( is_safe( ch, vch, false ) )
         continue;

      if( !is_npc( ch ) && !is_npc( vch ) && !in_arena( ch ) && ( !is_pkill( ch ) || !is_pkill( vch ) ) )
         continue;

      if( !can_see_character( ch, vch ) )
         continue;

      saved = check_save( sn, level, ch, vch );
      if( saved && SPELL_SAVE( skill ) == SE_NEGATE )
      {
         failed_casting( skill, ch, vch, NULL );
         continue;
      }
      else if( skill->dice )
         dam = dice_parse( ch, vch, level, skill->dice );
      else
         dam = dice( 1, level / 2 );
      if( saved )
      {
         switch( SPELL_SAVE( skill ) )
         {
            case SE_3QTRDAM:
               dam = ( dam * 3 ) / 4;
               break;
            case SE_HALFDAM:
               dam >>= 1;
               break;
            case SE_QUARTERDAM:
               dam >>= 2;
               break;
            case SE_EIGHTHDAM:
               dam >>= 3;
               break;

            case SE_ABSORB:  /* victim absorbs spell for hp's */
               act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, vch, TO_CHAR );
               act( AT_MAGIC, "You absorb $N's $t!", vch, skill->noun_damage, ch, TO_CHAR );
               act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, vch, TO_NOTVICT );
               vch->hit = URANGE( 0, vch->hit + dam, vch->max_hit );
               update_pos( vch );
               continue;

            case SE_REFLECT: /* reflect the spell to the caster */
               retcode = spell_attack( sn, level, vch, ch );
               if( char_died( ch ) )
               {
                  ch_died = true;
                  break;
               }
               continue;
         }
      }
      retcode = damage( ch, vch, NULL, dam, sn, true );
      if( retcode == rNONE && affects && !char_died( ch ) && !char_died( vch )
      && ( !is_affected( vch, sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE ) || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
         retcode = spell_affectchar( sn, level, ch, vch );
      if( retcode == rCHAR_DIED || char_died( ch ) )
      {
         ch_died = true;
         break;
      }
   }
   return retcode;
}

ch_ret spell_affectchar( int sn, int level, CHAR_DATA *ch, void *vo )
{
   AFFECT_DATA af;
   SMAUG_AFF *saf;
   SKILLTYPE *skill = get_skilltype( sn );
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int schance;
   bool affected = false, first = true;
   ch_ret retcode = rNONE;

   if( SPELL_FLAG( skill, SF_RECASTABLE ) )
      affect_strip( victim, sn );
   for( saf = skill->first_affect; saf; saf = saf->next )
   {
      if( saf->location >= REVERSE_APPLY )
      {
         if( !SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         {
            if( first == true )
            {
               if( SPELL_FLAG( skill, SF_RECASTABLE ) )
                  affect_strip( ch, sn );
               if( is_affected( ch, sn ) )
                  affected = true;
            }
            first = false;
            if( affected == true )
               continue;
         }
         victim = ch;
      }
      else
         victim = ( CHAR_DATA * ) vo;

      /* Check if char has this bitvector already */
      af.bitvector = meb( saf->bitvector );
      if( saf->bitvector >= 0 && xIS_SET( victim->affected_by, saf->bitvector ) && !SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         continue;

      /* necessary for affect_strip to work properly... */
      switch( saf->bitvector )
      {
         default:
            af.type = sn;
            break;

         case AFF_POISON:
            if( ( saf->location % REVERSE_APPLY ) == APPLY_STAT || ( saf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
               af.type = sn;
            else
               af.type = gsn_poison;
            schance = ris_save( victim, level, RIS_POISON );

            if( schance == 100 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }

            if( schance > 100 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }

            if( saves_poison_death( schance, victim ) )
            {
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }

            victim->mental_state = URANGE( 30, victim->mental_state + 2, 100 );
            break;

         case AFF_BLIND:
            if( ( saf->location % REVERSE_APPLY ) == APPLY_STAT || ( saf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
               af.type = sn;
            else
               af.type = gsn_blindness;
            break;

         case AFF_CURSE:
            if( ( saf->location % REVERSE_APPLY ) == APPLY_STAT || ( saf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
               af.type = sn;
            else
               af.type = gsn_curse;
            break;

         case AFF_INVISIBLE:
            if( ( saf->location % REVERSE_APPLY ) == APPLY_STAT || ( saf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
               af.type = sn;
            else
               af.type = gsn_invis;
            break;

         case AFF_SLEEP:
            if( ( saf->location % REVERSE_APPLY ) == APPLY_STAT || ( saf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
               af.type = sn;
            else
               af.type = gsn_sleep;
            schance = ris_save( victim, level, RIS_SLEEP );
            if( schance == 100 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            if( schance > 100 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;

         case AFF_CHARM:
            if( ( saf->location % REVERSE_APPLY ) == APPLY_STAT || ( saf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
               af.type = sn;
            else
               af.type = gsn_charm_person;
            schance = ris_save( victim, level, RIS_CHARM );
            if( schance == 100 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            if( schance > 100 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;
      }
      if( saf->duration )
         af.duration = dice_parse( ch, victim, level, saf->duration );
      af.modifier = dice_parse( ch, victim, level, saf->modifier );
      af.location = ( saf->location % REVERSE_APPLY );

      if( af.duration == 0 )
      {
         switch( ( af.location % REVERSE_APPLY ) )
         {
            case APPLY_HIT:
               victim->hit = URANGE( 0, victim->hit + af.modifier, victim->max_hit );
               update_pos( victim );
               if( is_npc( victim ) && victim->hit <= 0 )
                  damage( ch, victim, NULL, 5, TYPE_UNDEFINED, true );
               break;

            case APPLY_MANA:
               victim->mana = URANGE( 0, victim->mana + af.modifier, victim->max_mana );
               update_pos( victim );
               break;

            case APPLY_MOVE:
               victim->move = URANGE( 0, victim->move + af.modifier, victim->max_move );
               update_pos( victim );
               break;

            default:
               affect_modify( victim, &af, true );
               break;
         }
      }
      else if( SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         affect_join( victim, &af );
      else
         affect_to_char( victim, &af );
   }
   update_pos( victim );
   return retcode;
}

/* Generic spell affect - Thoric */
ch_ret spell_affect( int sn, int level, CHAR_DATA *ch, void *vo )
{
   SMAUG_AFF *saf;
   SKILLTYPE *skill = get_skilltype( sn );
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   bool groupsp, areasp;
   bool hitchar = false, hitroom = false, hitvict = false;
   ch_ret retcode;

   if( !skill->first_affect )
   {
      bug( "%s: has no affects sn %d", __FUNCTION__, sn );
      return rNONE;
   }
   if( SPELL_FLAG( skill, SF_GROUPSPELL ) )
      groupsp = true;
   else
      groupsp = false;

   if( SPELL_FLAG( skill, SF_AREA ) )
      areasp = true;
   else
      areasp = false;

   if( !groupsp && !areasp )
   {
      /* Can't find a victim */
      if( !victim )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      /* Spell is already on this guy */
      if( is_affected( victim, sn ) && !SPELL_FLAG( skill, SF_ACCUMULATIVE ) && !SPELL_FLAG( skill, SF_RECASTABLE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( ( saf = skill->first_affect ) && !saf->next
      && ( saf->location % REVERSE_APPLY ) == APPLY_STRIPSN && !is_affected( victim, dice_parse( ch, victim, level, saf->modifier ) ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( check_save( sn, level, ch, victim ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }
   }
   else
   {
      if( skill->hit_char && skill->hit_char[0] != '\0' )
      {
         if( strstr( skill->hit_char, "$N" ) )
            hitchar = true;
         else
            act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
      }

      if( skill->hit_room && skill->hit_room[0] != '\0' )
      {
         if( strstr( skill->hit_room, "$N" ) )
            hitroom = true;
         else
            act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );
      }

      if( skill->hit_vict && skill->hit_vict[0] != '\0' )
         hitvict = true;

      if( victim )
         victim = victim->in_room->first_person;
      else
         victim = ch->in_room->first_person;
   }
   if( !victim )
   {
      bug( "%s: could not find victim: sn %d", __FUNCTION__, sn );
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   for( ; victim; victim = victim->next_in_room )
   {
      if( groupsp || areasp )
      {
         if( ( groupsp && !is_same_group( victim, ch ) )
         || check_save( sn, level, ch, victim ) || ( !SPELL_FLAG( skill, SF_RECASTABLE ) && is_affected( victim, sn ) ) )
            continue;

         if( hitvict && ch != victim )
         {
            act( AT_MAGIC, skill->hit_vict, ch, NULL, victim, TO_VICT );
            if( hitroom )
            {
               act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
               act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_CHAR );
            }
         }
         else if( hitroom )
            act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_ROOM );

         if( ch == victim )
         {
            if( hitvict )
               act( AT_MAGIC, skill->hit_vict, ch, NULL, ch, TO_CHAR );
            else if( hitchar )
               act( AT_MAGIC, skill->hit_char, ch, NULL, ch, TO_CHAR );
         }
         else if( hitchar )
            act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
      }

      retcode = spell_affectchar( sn, level, ch, victim );

      if( !groupsp && !areasp )
      {
         if( retcode == rVICT_IMMUNE )
            immune_casting( skill, ch, victim, NULL );
         else
            successful_casting( skill, ch, victim, NULL );
         break;
      }
   }
   return rNONE;
}

/* Generic inventory object spell - Thoric */
ch_ret spell_obj_inv( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !obj )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }

   switch( SPELL_ACTION( skill ) )
   {
      default:
      case SA_NONE:
         return rNONE;

      case SA_CREATE:
         if( SPELL_FLAG( skill, SF_WATER ) ) /* create water */
         {
            int water;
            WEATHER_DATA *weath = ch->in_room->area->weather;

            if( obj->item_type != ITEM_DRINK_CON )
            {
               send_to_char( "It is unable to hold water.\r\n", ch );
               return rSPELL_FAILED;
            }

            if( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
            {
               send_to_char( "It contains some other liquid.\r\n", ch );
               return rSPELL_FAILED;
            }

            water = UMIN( ( skill->dice ? dice_parse( ch, ch, level, skill->dice ) : level )
                    * ( weath->precip >= 0 ? 2 : 1 ), obj->value[0] - obj->value[1] );

            if( water > 0 )
            {
               separate_obj( obj );
               obj->value[2] = LIQ_WATER;
               obj->value[1] += water;
               if( !is_name( "water", obj->name ) )
               {
                  char buf[MSL];

                  snprintf( buf, sizeof( buf ), "%s water", obj->name );
                  STRFREE( obj->name );
                  obj->name = STRALLOC( buf );
               }
            }
            successful_casting( skill, ch, NULL, obj );
            return rNONE;
         }
         if( SPELL_DAMAGE( skill ) == SD_FIRE ) /* burn object */
         {
            /* return rNONE; */
         }
         if( SPELL_DAMAGE( skill ) == SD_POISON /* poison object */
         || SPELL_CLASS( skill ) == SC_DEATH )
         {
            switch( obj->item_type )
            {
               default:
                  failed_casting( skill, ch, NULL, obj );
                  break;
               case ITEM_COOK:
               case ITEM_FOOD:
               case ITEM_DRINK_CON:
               case ITEM_FISH:
                  separate_obj( obj );
                  obj->value[3] = 1;
                  successful_casting( skill, ch, NULL, obj );
                  break;
            }
            return rNONE;
         }
         if( SPELL_CLASS( skill ) == SC_LIFE /* purify food/water */
         && ( obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON
         || obj->item_type == ITEM_COOK || obj->item_type == ITEM_FISH ) )
         {
            switch( obj->item_type )
            {
               default:
                  failed_casting( skill, ch, NULL, obj );
                  break;
               case ITEM_COOK:
               case ITEM_FOOD:
               case ITEM_DRINK_CON:
               case ITEM_FISH:
                  separate_obj( obj );
                  obj->value[3] = 0;
                  successful_casting( skill, ch, NULL, obj );
                  break;
            }
            return rNONE;
         }

         if( SPELL_CLASS( skill ) != SC_NONE )
         {
            failed_casting( skill, ch, NULL, obj );
            return rNONE;
         }
         switch( SPELL_POWER( skill ) )  /* clone object */
         {
               OBJ_DATA *clone;

            default:
            case SP_NONE:
               if( ch->level - obj->level < 10 || obj->cost > ch->level * get_curr_int( ch ) * get_curr_wis( ch ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;

            case SP_MINOR:
               if( ch->level - obj->level < 20 || obj->cost > ch->level * get_curr_int( ch ) / 5 )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;

            case SP_GREATER:
               if( ch->level - obj->level < 5 || obj->cost > ch->level * 10 * get_curr_int( ch ) * get_curr_wis( ch ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;

            case SP_MAJOR:
               if( ch->level - obj->level < 0 || obj->cost > ch->level * 50 * get_curr_int( ch ) * get_curr_wis( ch ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               clone = clone_object( obj );
               clone->timer = skill->dice ? dice_parse( ch, ch, level, skill->dice ) : 0;
               obj_to_char( clone, ch );
               successful_casting( skill, ch, NULL, obj );
               break;
         }
         return rNONE;

      case SA_DESTROY:
      case SA_RESIST:
      case SA_SUSCEPT:
      case SA_DIVINATE:
         if( SPELL_DAMAGE( skill ) == SD_POISON )  /* detect poison */
         {
            if( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD
            || obj->item_type == ITEM_COOK || obj->item_type == ITEM_FISH )
            {
               if( ( obj->item_type == ITEM_COOK || obj->item_type == ITEM_FISH ) && obj->value[2] == 0 )
                  send_to_char( "It looks undercooked.\r\n", ch );
               else if( obj->value[3] != 0 )
                  send_to_char( "You smell poisonous fumes.\r\n", ch );
               else
                  send_to_char( "It looks very delicious.\r\n", ch );
            }
            else
               send_to_char( "It doesn't look poisoned.\r\n", ch );
            return rNONE;
         }
         return rNONE;
      case SA_OBSCURE: /* make obj invis */
         if( is_obj_stat( obj, ITEM_INVIS ) || chance( ch, skill->dice ? dice_parse( ch, ch, level, skill->dice ) : 20 ) )
         {
            failed_casting( skill, ch, NULL, NULL );
            return rSPELL_FAILED;
         }
         successful_casting( skill, ch, NULL, obj );
         xSET_BIT( obj->extra_flags, ITEM_INVIS );
         return rNONE;

      case SA_CHANGE:
         return rNONE;
   }
}

/* Generic object creating spell - Thoric */
ch_ret spell_create_obj( int sn, int level, CHAR_DATA *ch, void *vo )
{
   SKILLTYPE *skill = get_skilltype( sn );
   int lvl, vnum = skill->value;
   OBJ_DATA *obj;
   OBJ_INDEX_DATA *oi;

   switch( SPELL_POWER( skill ) )
   {
      default:
      case SP_NONE:
         lvl = 10;
         break;
      case SP_MINOR:
         lvl = 0;
         break;
      case SP_GREATER:
         lvl = level / 2;
         break;
      case SP_MAJOR:
         lvl = level;
         break;
   }

   /* Add predetermined objects here */
   if( vnum == 0 )
   {
   }

   if( !( oi = get_obj_index( vnum ) ) || !( obj = create_object( oi, lvl ) ) )
   {
      bug( "%s: either no index data or couldn't create_object for vnum [%d].", __FUNCTION__, vnum );
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }
   obj->timer = skill->dice ? dice_parse( ch, ch, level, skill->dice ) : 0;
   if( !can_wear( obj, ITEM_NO_TAKE ) )
      obj_to_char( obj, ch );
   else
   {
      if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) )
      {
         send_to_char( "A magical force prevents you from casting that here.\r\n", ch );
         extract_obj( obj );
         return rNONE;
      }
      obj_to_room( obj, ch->in_room );
   }
   successful_casting( skill, ch, NULL, obj );
   return rNONE;
}

/* Generic mob creating spell - Thoric */
ch_ret spell_create_mob( int sn, int level, CHAR_DATA *ch, void *vo )
{
   SKILLTYPE *skill = get_skilltype( sn );
   int lvl, vnum = skill->value;
   CHAR_DATA *mob;
   MOB_INDEX_DATA *mi;
   AFFECT_DATA af;

   /* set maximum mob level */
   switch( SPELL_POWER( skill ) )
   {
      default:
      case SP_NONE:
         lvl = 20;
         break;
      case SP_MINOR:
         lvl = 5;
         break;
      case SP_GREATER:
         lvl = level / 2;
         break;
      case SP_MAJOR:
         lvl = level;
         break;
   }

   /* Add predetermined mobiles here */
   if( vnum == 0 )
   {
      if( !str_cmp( target_name, "cityguard" ) )
         vnum = MOB_VNUM_CITYGUARD;
      if( !str_cmp( target_name, "vampire" ) )
         vnum = MOB_VNUM_VAMPIRE;
   }

   if( !( mi = get_mob_index( vnum ) ) || !( mob = create_mobile( mi ) ) )
   {
      bug( "%s: either no index data or couldn't create_mobile for vnum [%d].", __FUNCTION__, vnum );
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }
   mob->level = UMIN( lvl, skill->dice ? dice_parse( ch, ch, level, skill->dice ) : mob->level );
   mob->armor = interpolate( mob->level, 100, -100 );

   mob->max_hit = mob->level * 8 + number_range( mob->level * mob->level / 4, mob->level * mob->level );
   mob->hit = mob->max_hit;
   mob->gold = 0;
   successful_casting( skill, ch, mob, NULL );
   char_to_room( mob, ch->in_room );
   add_follower( mob, ch );
   af.type = sn;
   af.duration = ( int )( ( ( ( level + 1 ) / 3 ) + 1 ) * DUR_CONV );
   af.location = APPLY_EXT_AFFECT;
   af.modifier = AFF_CHARM;
   af.bitvector = meb( AFF_CHARM );
   affect_to_char( mob, &af );
   return rNONE;
}

ch_ret ranged_attack( CHAR_DATA *, char *, OBJ_DATA *, OBJ_DATA *, short, short );

/* Generic handler for new "SMAUG" spells - Thoric */
ch_ret spell_smaug( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   /* Put this check in to prevent crashes from this getting a bad skill */
   if( !skill )
   {
      bug( "%s: Called with a null skill for sn %d", __FUNCTION__, sn );
      return rERROR;
   }

   switch( skill->target )
   {
      case TAR_IGNORE:
         /* offensive area spell */
         if( SPELL_FLAG( skill, SF_AREA )
         && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
         || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) ) )
            return spell_area_attack( sn, level, ch, vo );

         if( SPELL_ACTION( skill ) == SA_CREATE )
         {
            if( SPELL_FLAG( skill, SF_OBJECT ) )   /* create object */
               return spell_create_obj( sn, level, ch, vo );
            if( SPELL_CLASS( skill ) == SC_LIFE )  /* create mob */
               return spell_create_mob( sn, level, ch, vo );
         }

         /* affect a distant player */
         if( SPELL_FLAG( skill, SF_DISTANT )
         && ( victim = get_char_world( ch, target_name ) )
         && !xIS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         && SPELL_FLAG( skill, SF_CHARACTER ) )
            return spell_affect( sn, level, ch, get_char_world( ch, target_name ) );

         /* affect a player in this room (should have been TAR_CHAR_XXX) */
         if( SPELL_FLAG( skill, SF_CHARACTER ) )
            return spell_affect( sn, level, ch, get_char_room( ch, target_name ) );

         if( skill->range > 0
         && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
         || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) ) )
            return ranged_attack( ch, ranged_target_name, NULL, NULL, sn, skill->range );
         /* will fail, or be an area/group affect */
         return spell_affect( sn, level, ch, vo );

      case TAR_CHAR_OFFENSIVE:
         if( SPELL_FLAG( skill, SF_NOFIGHT )
         && ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE || ch->position == POS_BERSERK ) )
         {
            send_to_char( "You can't concentrate enough for that!\r\n", ch );
            return rNONE;
         }

         if( SPELL_FLAG( skill, SF_NOMOUNT ) && ch->position == POS_MOUNTED )
         {
            send_to_char( "You can't do that while you are mounted!\r\n", ch );
            return rNONE;
         }

         /* a regular damage inflicting spell attack */
         if( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
         || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) )
            return spell_attack( sn, level, ch, vo );

         /* a nasty spell affect */
         return spell_affect( sn, level, ch, vo );

      case TAR_CHAR_DEFENSIVE:
      case TAR_CHAR_SELF:
         if( SPELL_FLAG( skill, SF_NOFIGHT )
         && ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE || ch->position == POS_BERSERK ) )
         {
            send_to_char( "You can't concentrate enough for that!\r\n", ch );
            return rNONE;
         }

         if( SPELL_FLAG( skill, SF_NOMOUNT ) && ch->position == POS_MOUNTED )
         {
            send_to_char( "You can't do that while you are mounted!\r\n", ch );
            return rNONE;
         }

         if( vo && SPELL_ACTION( skill ) == SA_DESTROY )
         {
            victim = ( CHAR_DATA * ) vo;

            /* cure poison */
            if( SPELL_DAMAGE( skill ) == SD_POISON )
            {
               if( is_affected( victim, gsn_poison ) )
               {
                  affect_strip( victim, gsn_poison );
                  victim->mental_state = URANGE( -100, victim->mental_state, -10 );
                  successful_casting( skill, ch, victim, NULL );
                  return rNONE;
               }
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            /* cure blindness */
            if( SPELL_CLASS( skill ) == SC_ILLUSION )
            {
               if( is_affected( victim, gsn_blindness ) )
               {
                  affect_strip( victim, gsn_blindness );
                  successful_casting( skill, ch, victim, NULL );
                  return rNONE;
               }
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
         }
         return spell_affect( sn, level, ch, vo );

      case TAR_OBJ_INV:
         return spell_obj_inv( sn, level, ch, vo );
   }
   return rNONE;
}

NM ch_ret spell_midas_touch( int sn, int level, CHAR_DATA *ch, void *vo )
{
   int val;
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;

   if( is_obj_stat( obj, ITEM_NODROP ) )
   {
      send_to_char( "You can't seem to let go of it.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( is_obj_stat( obj, ITEM_PROTOTYPE ) && get_trust( ch ) < PERM_IMM )
   {
      send_to_char( "That item is not for mortal hands to touch!\r\n", ch );
      return rSPELL_FAILED;   /* Thoric */
   }

   if( can_wear( obj, ITEM_NO_TAKE ) || ( obj->item_type == ITEM_CORPSE_NPC ) || ( obj->item_type == ITEM_CORPSE_PC ) )
   {
      send_to_char( "You can't seem to turn this item to gold!\r\n", ch );
      return rNONE;
   }

   separate_obj( obj ); /* nice, alty :) */

   val = obj->cost / 2;
   val = UMAX( 0, val );

   increase_gold( ch, val );

   if( obj )
      extract_obj( obj );
   send_to_char( "You transmogrify the item to gold!\r\n", ch );

   return rNONE;
}

NM ch_ret spell_create_fire( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *fire = NULL, *obj;

   if( ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_WATER_NOSWIM 
   || ch->in_room->sector_type == SECT_UNDERWATER || ch->in_room->sector_type == SECT_OCEANFLOOR )
   {
      send_to_char( "You can't create a fire here.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) )
   {
      send_to_char( "A magical force prevents you from casting that here.\r\n", ch );
      return rSPELL_FAILED;
   }

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == OBJ_VNUM_FIRE
      || obj->pIndexData->vnum == OBJ_VNUM_WOODFIRE )
      {
         fire = obj;
         break;
      }
   }
   if( fire )
   {
      act( AT_MAGIC, "$n's magic successfully stokes the fire!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "Your magic successfully stokes the fire!", ch, NULL, NULL, TO_CHAR );
      fire->timer = UMAX( fire->timer, ( fire->timer + ch->level ) );
   }
   else
   {
      if( !( fire = create_object( get_obj_index( OBJ_VNUM_FIRE ), 0 ) ) )
      {
         bug( "%s: Object vnum %d couldn't be created", __FUNCTION__, OBJ_VNUM_FIRE );
         return rNONE;
      }
      fire->timer = ch->level;
      act( AT_MAGIC, "A small cloud of vaporous flame bursts forth before $n.", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "A small cloud of vaporous flame bursts forth before you.", ch, NULL, NULL, TO_CHAR );
      obj_to_room( fire, ch->in_room );
   }
   return rNONE;
}

NM ch_ret spell_create_spring( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *spring = NULL, *obj;

   if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) )
   {
      send_to_char( "A magical force prevents you from casting that here.\r\n", ch );
      return rSPELL_FAILED;
   }

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == OBJ_VNUM_SPRING )
      {
         spring = obj;
         break;
      }
   }
   if( spring )
   {
      act( AT_MAGIC, "$n's magic successfully increases the flow of the spring!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "Your magic successfully increases the flow of the spring!", ch, NULL, NULL, TO_CHAR );
      spring->timer = UMAX( spring->timer, ( spring->timer + ch->level ) );
   }
   else
   {
      if( !( spring = create_object( get_obj_index( OBJ_VNUM_SPRING ), 0 ) ) )
      {
         bug( "%s: Object vnum %d couldn't be created", __FUNCTION__, OBJ_VNUM_SPRING );
         return rNONE;
      }
      spring->timer = ch->level;
      act( AT_MAGIC, "As $n traces a ring through the air, the flow of a mystical spring emerges.", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "Tracing a ring before you, the graceful flow of a mystical spring emerges.", ch, NULL, NULL, TO_CHAR );
      obj_to_room( spring, ch->in_room );
   }
   return rNONE;
}

bool handle_recall( CHAR_DATA *ch );

NM ch_ret spell_word_of_recall( int sn, int level, CHAR_DATA *ch, void *vo )
{
   SKILLTYPE *skill = get_skilltype( sn );

   if( !handle_recall( ch ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }
   return rNONE;
}

/* Get a previous enchantment to modify */
AFFECT_DATA *has_enchantment( OBJ_DATA *obj, int ench, int mod )
{
   AFFECT_DATA *paf;

   for( paf = obj->first_affect; paf; paf = paf->next )
   {
      if( ( paf->location % REVERSE_APPLY ) == ench && paf->enchantment )
      {
         if( ( paf->location % REVERSE_APPLY ) != APPLY_STAT ) /* If not stat return paf */
            return paf;
         if( xIS_SET( paf->bitvector, mod ) ) /* If stat and is same stat return paf */
            return paf;
      }
   }

   return NULL;
}

bool add_enchantment( OBJ_DATA *obj, int location, int mod )
{
   AFFECT_DATA *paf;

   if( !obj || mod == 0 )
      return false;
   if( !( paf = has_enchantment( obj, location, mod ) ) )
   {
      CREATE( paf, AFFECT_DATA, 1 );
      if( !paf )
      {
         bug( "%s: paf NULL after CREATE.", __FUNCTION__ );
         return false;
      }
      paf->type = -1;
      paf->duration = -1;
      paf->location = location;
      paf->modifier = 0;
      if( location == APPLY_STAT )
      {
         if( number_percent( ) >= 50 ) /* 50% chance of adding otherwise subtract */
            paf->modifier += 1;
         else
            paf->modifier -= 1;
      }
      else
         paf->modifier = mod;
      xCLEAR_BITS( paf->bitvector );
      if( location == APPLY_STAT )
         xSET_BIT( paf->bitvector, mod );
      paf->enchantment = true;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      ++top_affect;
   }
   else
   {
      /* Toss in a limit to how high and low these things can get */
      if( ( paf->modifier + mod ) >= 100 || ( paf->modifier + mod ) <= -100 )
         return false;
      if( location == APPLY_STAT )
      {
         if( number_percent( ) >= 50 ) /* 50% chance of adding otherwise subtract */
            paf->modifier += 1;
         else
            paf->modifier -= 1;
      }
      else
         paf->modifier += mod;
   }
   return true;
}

/* Lets see if an object is resistant, immune, or susceptable to something */
bool is_ris( OBJ_DATA *obj, int ris )
{
   AFFECT_DATA *paf;

   for( paf = obj->first_affect; paf; paf = paf->next )
   {
      if( ( ( paf->location % REVERSE_APPLY ) == APPLY_RESISTANT ) && xIS_SET( paf->bitvector, ris ) )
         return true;
   }
   return false;
}

/* This will decide if the ris can be changed from resistant to susceptable etc...will increase/decrease some too */
bool change_ris( OBJ_DATA *obj, bool resistant, int ris )
{
   AFFECT_DATA *paf;

   for( paf = obj->first_affect; paf; paf = paf->next )
   {
      /* Only change enchantments */
      if( !paf->enchantment )
         continue;

      /* Only bother with the correct one */
      if( !xIS_SET( paf->bitvector, ris ) )
         continue;

      if( ( paf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
      {
         if( !resistant )
         {
            if( paf->modifier > 1 )
               paf->modifier -= 1;
            else if( paf->modifier > 0 )
               paf->modifier = -1;
            else if( number_percent( ) > 50 )
               paf->modifier -= 1;
            else
               return false;
         }
         else
         {
            if( paf->modifier < -1 )
               paf->modifier += 1;
            else if( paf->modifier < 0 )
               paf->modifier = 1;
            else if( number_percent( ) > 50 )
               paf->modifier += 1;
            else
               return false;
         }
         return true;
      }
   }

   return false;
}

/* Add a resistance or susceptible */
bool add_ris( OBJ_DATA *obj, bool resistant, int ris )
{
   AFFECT_DATA *paf;

   /* Is it already got this one? if so change if possible if not just return */
   if( is_ris( obj, ris ) )
   {
      if( change_ris( obj, resistant, ris ) )
         return true;
      return false;
   }

   /* Ok lets add it */
   CREATE( paf, AFFECT_DATA, 1 );
   if( !paf )
      return false;
   paf->type = -1;
   paf->duration = -1;
   paf->location = APPLY_RESISTANT;
   if( resistant )
      paf->modifier = 1;
   else
      paf->modifier = -1;
   xCLEAR_BITS( paf->bitvector );
   xSET_BIT( paf->bitvector, ris );
   paf->next = NULL;
   paf->enchantment = true;
   LINK( paf, obj->first_affect, obj->last_affect, next, prev );
   ++top_affect;
   return true;
}

NM ch_ret spell_enchant( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   AFFECT_DATA *paf, *paf_next;
   int uchance;
   bool failed = false, goodaff, added = false;

   if( !obj )
   {
      send_to_char( "No object to enchant.\r\n", ch );
      return rSPELL_FAILED;
   }

   separate_obj( obj );

   if( ( obj->item_type != ITEM_WEAPON && obj->item_type != ITEM_ARMOR ) )
      failed = true;

   if( !failed && is_obj_stat( obj, ITEM_MAGIC ) )
      failed = true;

   if( failed )
   {
      act( AT_MAGIC, "Your magic twists and winds around $p but can't take hold.", ch, obj, NULL, TO_CHAR );
      act( AT_MAGIC, "$n's magic twists and winds around $p but can't take hold.", ch, obj, NULL, TO_NOTVICT );
      return rSPELL_FAILED;
   }

   /* Very small chance of removing all enchantments */
   if( xIS_SET( obj->extra_flags, ITEM_ENCHANTED ) && number_percent( ) <= 2 )
   {
      for( paf = obj->first_affect; paf; paf = paf_next )
      {
         paf_next = paf->next;
         if( !paf->enchantment )
            continue;
         UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
         DISPOSE( paf );
         --top_affect;
      }

      xREMOVE_BIT( obj->extra_flags, ITEM_ENCHANTED );

      if( is_obj_stat( obj, ITEM_ANTI_GOOD ) && is_obj_stat( obj, ITEM_ANTI_EVIL ) )
      {
         xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
         xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
         act( AT_YELLOW, "$p momentarily absorbs all light around it.", ch, obj, NULL, TO_CHAR );
      }

      if( is_obj_stat( obj, ITEM_ANTI_GOOD ) )
      {
         xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
         act( AT_BLUE, "$p momentarily absorbs all blue light around it.", ch, obj, NULL, TO_CHAR );
      }

      if( is_obj_stat( obj, ITEM_ANTI_EVIL ) )
      {
         xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
         act( AT_RED, "$p momentarily absorbs all red light around it.", ch, obj, NULL, TO_CHAR );
      }
      return rNONE;
   }

   failed = true;

   /* Only put armor on armor */
   if( obj->item_type == ITEM_ARMOR )
   {
      uchance = number_percent( );
      goodaff = ( number_percent( ) > 25 );
      if( uchance > 75 )
      {
         uchance = UMAX( 1, number_range( 1, ( level / 10 ) ) );
         if( !goodaff )
            uchance = ( 0 - uchance );
         added = add_enchantment( obj, APPLY_ARMOR, uchance );
         if( added )
            failed = false;
      }
   }

   /* Only put hitroll and damroll on weapons */
   if( obj->item_type == ITEM_WEAPON )
   {
      uchance = number_percent( );
      goodaff = ( number_percent( ) > 25 );
      if( uchance > 75 )
      {
         uchance = UMAX( 1, number_range( 1, ( level / 15 ) ) );
         if( !goodaff )
            uchance = ( 0 - uchance );
         added = add_enchantment( obj, APPLY_HITROLL, uchance );
         if( added )
            failed = false;
      }
   
      uchance = number_percent( );
      goodaff = ( number_percent( ) > 25 );
      if( number_percent( ) > 75 )
      {
         uchance = UMAX( 1, number_range( 1, ( level / 15 ) ) );
         if( !goodaff )
            uchance = ( 0 - uchance );
         added = add_enchantment( obj, APPLY_DAMROLL, uchance );
         if( added )
            failed = false;
      }
   }

   if( ( uchance = number_percent( ) ) >= 98 )
   {
      added = add_enchantment( obj, APPLY_STAT, number_range( 0, ( STAT_MAX - 1 ) ) );
      if( added )
         failed = false;
   }

   uchance = number_percent( );
   if( uchance > 90 )
   {
      goodaff = ( number_percent( ) > 25 );
      added = add_ris( obj, goodaff, number_range( 0, ( RIS_MAX - 1 ) ) );
      if( added )
         failed = false;
   }

   if( failed )
   {
      act( AT_MAGIC, "Your magic twists and winds around $p but can't take hold.", ch, obj, NULL, TO_CHAR );
      act( AT_MAGIC, "$n's magic twists and winds around $p but can't take hold.", ch, obj, NULL, TO_NOTVICT );
      return rSPELL_FAILED;
   }

   xSET_BIT( obj->extra_flags, ITEM_ENCHANTED );

   if( is_good( ch ) )
   {
      xSET_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
      act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_ROOM );
      act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_CHAR );
   }
   else if( is_evil( ch ) )
   {
      xSET_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
      act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_CHAR );
      act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_ROOM );
   }
   else
   {
      xSET_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
      xSET_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
      act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_ROOM );
      act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_CHAR );
   }
   return rNONE;
}

NM ch_ret spell_disenchant( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   AFFECT_DATA *paf, *paf_next;
   bool rmenchant = false;

   if( !obj )
   {
      send_to_char( "No object to disenchant.\r\n", ch );
      return rSPELL_FAILED;
   }

   separate_obj( obj );

   if( obj->item_type != ITEM_ARMOR && obj->item_type != ITEM_WEAPON )
   {
      send_to_char( "You can only disenchant armors and weapons.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( !obj->first_affect )
   {
      send_to_char( "That object appears to have no enchantments on it.\r\n", ch );
      return rSPELL_FAILED;
   }

   if( !is_obj_stat( obj, ITEM_ENCHANTED ) )
   {
      send_to_char( "You can't disenchant something that's not enchanted.\r\n", ch );
      return rSPELL_FAILED;
   }

   for( paf = obj->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      if( !paf->enchantment )
         continue;
      UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
      DISPOSE( paf );
      --top_affect;
      rmenchant = true;
   }

   if( !rmenchant )
   {
      send_to_char( "No enchantment found to remove.\r\n", ch );
      return rSPELL_FAILED;
   }

   xREMOVE_BIT( obj->extra_flags, ITEM_ENCHANTED );

   if( is_obj_stat( obj, ITEM_ANTI_GOOD ) && is_obj_stat( obj, ITEM_ANTI_EVIL ) )
   {
      xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
      xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
      act( AT_YELLOW, "$p momentarily absorbs all light around it.", ch, obj, NULL, TO_CHAR );
   }

   if( is_obj_stat( obj, ITEM_ANTI_GOOD ) )
   {
      xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_GOOD );
      act( AT_BLUE, "$p momentarily absorbs all blue light around it.", ch, obj, NULL, TO_CHAR );
   }

   if( is_obj_stat( obj, ITEM_ANTI_EVIL ) )
   {
      xREMOVE_BIT( obj->extra_flags, ITEM_ANTI_EVIL );
      act( AT_RED, "$p momentarily absorbs all red light around it.", ch, obj, NULL, TO_CHAR );
   }

   successful_casting( get_skilltype( sn ), ch, NULL, obj );
   return rNONE;
}
