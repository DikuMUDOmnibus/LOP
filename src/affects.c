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

void fwrite_paflocation( FILE *fp, AFFECT_DATA *paf )
{
   if( !paf || !fp )
      return;

   if( ( paf->location % REVERSE_APPLY ) >= 0 && ( paf->location % REVERSE_APPLY ) < APPLY_MAX )
      fprintf( fp, " '%s'", a_types[paf->location % REVERSE_APPLY] );
   else
      fprintf( fp, " '%d'", paf->location );
}

void fwrite_pafmodifier( FILE *fp, AFFECT_DATA *paf )
{
   if( !paf || !fp )
      return;

   if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT && paf->modifier >= 0 && paf->modifier < AFF_MAX )
      fprintf( fp, " '%s'", a_flags[paf->modifier] );
   else if( ( ( paf->location % REVERSE_APPLY ) == APPLY_WEAPONSPELL || ( paf->location % REVERSE_APPLY ) == APPLY_WEARSPELL
   || ( paf->location % REVERSE_APPLY ) == APPLY_REMOVESPELL || ( paf->location % REVERSE_APPLY ) == APPLY_STRIPSN )
   && is_valid_sn( paf->modifier ) )
      fprintf( fp, " '%s'", skill_table[paf->modifier]->name );
   else
      fprintf( fp, " '%d'", paf->modifier );
}

void fwrite_pafbitvector( FILE *fp, AFFECT_DATA *paf )
{
   if( !paf || !fp )
      return;

   if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT && !xIS_EMPTY( paf->bitvector ) )
      fprintf( fp, " '%s'", ext_flag_string( &paf->bitvector, a_flags ) );
   else if( ( paf->location % REVERSE_APPLY ) == APPLY_STAT && !xIS_EMPTY( paf->bitvector ) )
      fprintf( fp, " '%s'", ext_flag_string( &paf->bitvector, stattypes ) );
   else if( ( paf->location % REVERSE_APPLY ) == APPLY_RESISTANT && !xIS_EMPTY( paf->bitvector ) )
      fprintf( fp, " '%s'", ext_flag_string( &paf->bitvector, ris_flags ) );
   else
      fprintf( fp, " %s", "'-1'" );
}

void fwrite_objaffects( FILE *fp, AFFECT_DATA *paf )
{
   SKILLTYPE *skill = NULL;

   if( !paf || !fp )
      return;

   for( ; paf; paf = paf->next )
   {
      if( paf->type >= 0 && !( skill = get_skilltype( paf->type ) ) )
         continue;

      /* type/name */
      if( paf->type >= 0 )
         fprintf( fp, "NAffectN     '%s'", skill->name );
      else
         fprintf( fp, "NAffectN     '%d'", paf->type );

      fprintf( fp, " %d", paf->duration );

      fwrite_paflocation( fp, paf );
      fwrite_pafmodifier( fp, paf );
      fwrite_pafbitvector( fp, paf );

      /* Enchantment or extra, Extra is from quest / immortal addition and enchantment is added by enchant spell */
      fprintf( fp, " '%s'", paf->enchantment ? "enchantment" : "extra" );

      fprintf( fp, "%s", "\n" );
   }
}

void fwrite_chaffect( FILE *fp, AFFECT_DATA *paf )
{
   SKILLTYPE *skill = NULL;

   if( !fp || !paf )
      return;

   for( ; paf; paf = paf->next )
   {
      if( paf->type >= 0 && !( skill = get_skilltype( paf->type ) ) )
         continue;

      /* type/name */
      if( paf->type >= 0 )
         fprintf( fp, "Affect         '%s'", skill->name );
      else
         fprintf( fp, "Affect         '%d'", paf->type );

      fprintf( fp, " %d", paf->duration );

      fwrite_paflocation( fp, paf );
      fwrite_pafmodifier( fp, paf );
      fwrite_pafbitvector( fp, paf );

      fprintf( fp, "%s", "\n" );
   }
}

void fwrite_oiaffect( FILE *fp, AFFECT_DATA *paf )
{
   if( !fp || !paf )
      return;

   for( ; paf; paf = paf->next )
   {
      fprintf( fp, "%s", "NAffect   " );

      fwrite_paflocation( fp, paf );
      fwrite_pafmodifier( fp, paf );
      fwrite_pafbitvector( fp, paf );

      fprintf( fp, "%s", "\n" );
   }
}

/* This will read in a ch->affect */
/* The atype is to have it load only what you are wanting it to load */
/*
 * atype == 0 (Loads it all except for enchantment/extra)
 * atype == 1 (Loads it all and enchantment/extra)
 * atype == 2 (Skips loading the type and duration and enchantment/extra)
 */
AFFECT_DATA *fread_chaffect( FILE *fp, int atype, const char *filename, int line )
{
   AFFECT_DATA *paf;
   char *infoflags;
   int sn, value;

   if( !fp )
      return NULL;

   CREATE( paf, AFFECT_DATA, 1 );

   if( atype == 2 )
   {
      paf->type = -1;
      paf->duration = -1;
   }
   else
   {
      infoflags = fread_word( fp );
      if( is_number( infoflags ) )
         paf->type = atoi( infoflags );
      else
      {
         if( ( sn = skill_lookup( infoflags ) ) < 0 )
         {
            if( ( sn = herb_lookup( infoflags ) ) < 0 )
            {
               if( ( sn = pers_lookup( infoflags ) ) < 0 )
               {
                  bug( "%s:%d unknown skill [%s] for char affect.", filename, line, infoflags );
                  fread_to_eol( fp );
                  DISPOSE( paf );
                  return NULL;
               }
               else
                  sn += TYPE_PERS;
            }
            else
               sn += TYPE_HERB;
         }
         paf->type = sn;
      }
      paf->duration = fread_number( fp );
   }


   infoflags = fread_word( fp );
   if( str_cmp( infoflags, "0" ) )
   {
      /* Change blood to mana */
      if( !str_cmp( infoflags, "Blood" ) )
         infoflags = (char *)"Mana";
      value = get_flag( infoflags, a_types, APPLY_MAX );
      if( value < 0 || value >= APPLY_MAX )
      {
         bug( "%s:%d Unknown apply %s",  filename, line, infoflags );
         fread_to_eol( fp );
         DISPOSE( paf );
         return NULL;
      }
      else
         paf->location = value;
   }
   else
      paf->location = 0;

   infoflags = fread_word( fp );
   if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
   {
      value = get_flag( infoflags, a_flags, AFF_MAX );
      if( value < 0 || value >= AFF_MAX )
      {
         bug( "%s:%d Unknown affected %s", filename, line, infoflags );
         fread_to_eol( fp );
         DISPOSE( paf );
         return NULL;
      }
      else
         paf->modifier = value;
   }
   else if( ( paf->location % REVERSE_APPLY ) == APPLY_WEAPONSPELL || ( paf->location % REVERSE_APPLY ) == APPLY_WEARSPELL
   || ( paf->location % REVERSE_APPLY ) == APPLY_REMOVESPELL || (paf->location % REVERSE_APPLY ) == APPLY_STRIPSN )
   {
      if( is_number( infoflags ) )
      {
         paf->modifier = slot_lookup( atoi( infoflags ) );
         if( paf->modifier < 0 )
         {
            bug( "%s:%d unknown slot [%s] for %s.", filename, line, infoflags, affect_loc_name( ( paf->location % REVERSE_APPLY ) ) );
            fread_to_eol( fp );
            DISPOSE( paf );
            return NULL;
         }
      }
      else
      {
         if( ( paf->modifier = skill_lookup( infoflags ) ) < 0 )
         {
            if( ( paf->modifier = herb_lookup( infoflags ) ) < 0 )
            {
               if( ( paf->modifier = pers_lookup( infoflags ) ) < 0 )
               {
                  bug( "%s:%d unknown skill [%s] for %s.", filename, line, infoflags, affect_loc_name( ( paf->location % REVERSE_APPLY ) ) );
                  fread_to_eol( fp );
                  DISPOSE( paf );
                  return NULL;
               }
            }
         }
      }
   }
   else
      paf->modifier = atoi( infoflags );

   xCLEAR_BITS( paf->bitvector );

   infoflags = fread_word( fp );
   if( str_cmp( infoflags, "-1" ) )
   {
      if( ( paf->location % REVERSE_APPLY ) == APPLY_STAT )
      {
         value = get_flag( infoflags, stattypes, STAT_MAX );
         if( value < 0 || value >= STAT_MAX )
         {
            bug( "%s:%d Unknown stat %s", filename, line, infoflags );
            fread_to_eol( fp );
            DISPOSE( paf );
            return NULL;
         }
         else
            xSET_BIT( paf->bitvector, value );
      }
      else if( ( paf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
      {
         value = get_flag( infoflags, ris_flags, RIS_MAX );
         if( value < 0 || value >= RIS_MAX )
         {
            bug( "%s:%d Unknown ris %s", filename, line, infoflags );
            fread_to_eol( fp );
            DISPOSE( paf );
            return NULL;
         }
         else
            xSET_BIT( paf->bitvector, value );
      }
      else if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
      {
         value = get_flag( infoflags, a_flags, AFF_MAX );
         if( value < 0 || value >= AFF_MAX )
         {
            bug( "%s:%d Unknown affect %s", filename, line, infoflags );
            fread_to_eol( fp );
            DISPOSE( paf );
            return NULL;
         }
         else
            xSET_BIT( paf->bitvector, value );
      }
   }

   paf->enchantment = false;

   if( atype == 1 )
   {
      infoflags = fread_word( fp );

      if( !str_cmp( infoflags, "enchantment" ) )
         paf->enchantment = true;
   }

   return paf;
}

/* Show an affect verbosely to a character -Thoric */
void showaffect( CHAR_DATA *ch, AFFECT_DATA *paf, bool extdisplay )
{
   char buf[MSL];

   if( !paf )
   {
      bug( "%s: NULL paf", __FUNCTION__ );
      return;
   }

   if( ( paf->location % REVERSE_APPLY ) != APPLY_NONE )
   {
      switch( ( paf->location % REVERSE_APPLY ) )
      {
         default:
            snprintf( buf, sizeof( buf ), "Affects %s by %d.", affect_loc_name( ( paf->location % REVERSE_APPLY ) ), paf->modifier );
            break;

         case APPLY_STAT:
            snprintf( buf, sizeof( buf ), "Affects %s stat by %d.", ext_flag_string( &paf->bitvector, stattypes ), paf->modifier );
            break;

         case APPLY_EXT_AFFECT:
            snprintf( buf, sizeof( buf ), "Affects %s by %s.", affect_loc_name( ( paf->location % REVERSE_APPLY ) ), a_flags[paf->modifier] );
            break;

         case APPLY_WEAPONSPELL:
         case APPLY_WEARSPELL:
         case APPLY_REMOVESPELL:
            snprintf( buf, sizeof( buf ), "Casts spell '%s'.", is_valid_sn( paf->modifier ) ? skill_table[paf->modifier]->name : "unknown" );
            break;

         case APPLY_RESISTANT:
            snprintf( buf, sizeof( buf ), "Affects %s resistance by %d.", ext_flag_string( &paf->bitvector, ris_flags ), paf->modifier );
            break;
      }
      if( extdisplay )
         mudstrlcat( buf, " (Extra)\r\n", sizeof( buf ) );
      else
         mudstrlcat( buf, "\r\n", sizeof( buf ) );
      send_to_char( buf, ch );
   }
}
