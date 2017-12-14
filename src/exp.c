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
#include <math.h>
#include "h/mud.h"

/* Calculate roughly how much experience a character is worth */
double get_exp_worth( CHAR_DATA *ch )
{
   double wexp;
   int ulevel = urange( 1, ch->level, MAX_LEVEL );

   wexp = ( ulevel * ulevel * ulevel );
   return durange( MIN_EXP_WORTH, wexp, MAX_EXP_WORTH );
}

double exp_level( CHAR_DATA *ch, int level )
{
   double lvl;

   lvl = durange( 1, level, MAX_LEVEL );
   return ( 20 + ( lvl * lvl * lvl ) );
}

double xp_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
   double xp;
   int ldiff;

   ldiff = ( gch->level - victim->level );

   /* if there is more then a 10 level difference either way return 0 */
   if( ldiff > 10 || ldiff < -10 )
      return 0;

   xp = get_exp_worth( victim );

   /* This will take the level difference and increase or decrease exp accordingly */
   if( xp > 0 && ldiff != 0 )
   {
      if( ldiff < 0 )
      {
         ldiff = ( 0 - ldiff );
         xp *= ldiff;
      }
      else
         xp /= ldiff;
   }

   if( !is_npc( gch ) && is_npc( victim ) )
   {
      int times = times_killed( gch, victim );

      if( times >= 20 )
         xp = 0;
      else if( times )
      {
         xp = ( xp * ( 20 - times ) ) / 20;
         if( times > 15 )
            xp /= 3;
         else if( times > 10 )
            xp /= 1.5;
      }
   }

   /* No more then half the exp you needed to level gained off 1 kill */
   return durange( 0, xp, ( exp_level( gch, gch->level ) / 2 ) );
}

void update_level( CHAR_DATA *ch )
{
   MCLASS_DATA *mclass;
   int mcount = 0, mlevel = 0, oldlevel = 0;

   if( !ch || is_npc( ch ) )
      return;
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      mcount++;
      mlevel += mclass->level;
   }
   if( mcount && mlevel )
   {
      oldlevel = ch->level;
      ch->level = ( mlevel / mcount );

      if( ch->level > ch->perm_stats[STAT_CHA] )
         ch->perm_stats[STAT_CHA] = ch->level; /* Perm charisma should always be equal to level, but if higher let it be */
      if( ch->level > oldlevel )
      {
         /* Chance of stat increases */
         if( number_percent( ) > 60 )
            ch->max_hit = UMAX( ch->max_hit, ( ch->max_hit + number_range( 4, 12 ) ) );
         if( number_percent( ) > 60 )
            ch->max_move = UMAX( ch->max_move, ( ch->max_move + number_range( 2, 8 ) ) );
         if( number_percent( ) > 60 )
            ch->max_mana = UMAX( ch->max_mana, ( ch->max_mana + number_range( 2, 8 ) ) );
      }
   }
}

/* Advancement stuff. */
void advance_level( CHAR_DATA *ch )
{
   if( !ch || is_npc( ch ) )
      return;
   ch->practice += 1;

   update_level( ch );

   /* Restore them if needed */
   if( ch->hit < ch->max_hit )
      ch->hit = ch->max_hit;
   if( ch->mana < ch->max_mana )
      ch->mana = ch->max_mana;
   if( ch->move < ch->max_move )
      ch->move = ch->max_move;
}

void gain_exp( CHAR_DATA *ch, double gain )
{
   MCLASS_DATA *mclass;
   double modgain, oldexp = 0.0, gained = 0.0, umodgain, increase;
   short upercent = 0;

   if( !ch )
      return;

   if( gain > 0 )
      modgain = ( gain * sysdata.expmulti );
   else
      modgain = gain;

   /* Handle npcs and anyone with no class */
   if( !ch->pcdata || !ch->pcdata->first_mclass )
   {
      if( ch->level >= MAX_LEVEL )
      {
         ch->exp = 0.0;
         return;
      }

      if( ch->level >= 2 && not_authorized( ch ) )
      {
         send_to_char( "You can't gain any more experience until you change your name.\r\n", ch );
         return;
      }
      umodgain = dumin( modgain, exp_level( ch, ch->level ) );
      oldexp = ch->exp;
      increase = umodgain;
      upercent = 100;
      ch->exp = dumax( 0, ch->exp + ( int ) increase );
      gained += ( ch->exp - oldexp );
      if( ch->level < MAX_LEVEL && ch->exp >= exp_level( ch, ( ch->level + 1 ) ) )
      {
         set_char_color( AT_WHITE + AT_BLINK, ch );
         ch->level += 1;
         ch->exp -= exp_level( ch, ch->level );
         if( gained != 0 )
         {
            ch_printf( ch, "You %s %s experience points.\r\n", gained > 0 ? "received" : "lost", double_punct( fabs( gained ) ) );
            gained = 0.0;
         }
         ch_printf( ch, "You have obtained level %d!\r\n", ch->level );
         if( ch->level >= MAX_LEVEL )
            ch->exp = 0.0;
         advance_level( ch );
         set_char_color( AT_PLAIN, ch );
      }
   }
   else
   {
      if( ch->level >= 2 && not_authorized( ch ) )
      {
         send_to_char( "You can't gain any more experience until you change your name.\r\n", ch );
         return;
      }

      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         if( mclass->level >= MAX_LEVEL )
         {
            mclass->exp = 0.0;
            continue;
         }
         umodgain = dumin( modgain, exp_level( ch, mclass->level ) );
         oldexp = mclass->exp;

         upercent += mclass->cpercent;
         if( modgain > 0 )
            increase = ( ( umodgain * mclass->cpercent ) / 100 );
         else
            increase = umodgain;

         mclass->exp = dumax( 0, ( mclass->exp + ( int ) increase ) );
         gained += ( mclass->exp - oldexp );
         if( mclass->level < MAX_LEVEL && mclass->exp >= exp_level( ch, ( mclass->level + 1 ) ) )
         {
            set_char_color( AT_WHITE + AT_BLINK, ch );
            mclass->level += 1;
            mclass->exp -= exp_level( ch, mclass->level );
            if( gained != 0 )
            {
               ch_printf( ch, "You %s %s experience points.\r\n", gained > 0 ? "received" : "lost", double_punct( fabs( gained ) ) );
               gained = 0.0;
            }
            ch_printf( ch, "You have obtained level %d in %s!\r\n", mclass->level, dis_class_name( mclass->wclass ) );
            if( mclass->level >= MAX_LEVEL )
               mclass->exp = 0.0;
            advance_level( ch );
            set_char_color( AT_PLAIN, ch );
         }
      }
   }

   if( upercent == 0 && gain > 0 )
      send_to_char( "You can't gain any exp because all of your class percent gains are set to 0.\r\n", ch );

   if( gained != 0 )
      ch_printf( ch, "You %s %s experience points.\r\n", gained > 0 ? "received" : "lost", double_punct( fabs( gained ) ) );
}

/* Display your current exp, level, and surrounding level exp requirements - Thoric */
CMDF( do_level )
{
   MCLASS_DATA *mclass;
   const char *s1, *s2;

   s1 = color_str( AT_SCORE, ch );
   s2 = color_str( AT_SCORE2, ch );

   ch_printf( ch, "%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", s1 );
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      ch_printf( ch, "%s|%s%12.12s%s|%s%3d%s|", s1, s2, dis_class_name( mclass->wclass ), s1, s2, mclass->level, s1 );
      if( mclass->level < MAX_LEVEL )
      {
         ch_printf( ch, "%sCurrent: %s%12s%s|", s1, s2, double_punct( mclass->exp ), s1 );
         ch_printf( ch, "%sStill Need: %s%12s%s|%s%3d%s|", s1, s2, double_punct( exp_level( ch, mclass->level + 1 ) - mclass->exp ), s1,
            s2, mclass->cpercent, s1 );
      }
      else
         send_to_char( "                                                  |", ch );
      send_to_char( "\r\n", ch );
   }
   ch_printf( ch, "%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", s1 );
}

CMDF( do_classpercent )
{
   MCLASS_DATA *mclass, *tmclass = NULL;
   char arg[MSL];
   int tmpcount = 0, mcount = 0;

   if( !ch || is_npc( ch ) )
      return;
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !argument || argument[0] == '\0' || !is_number( argument ) )
   {
      send_to_char( "Usage: classpercent <class> <percent>\r\n", ch );
      return;
   }
   if( ( tmpcount = atoi( argument ) ) < 0 || tmpcount > 100 )
   {
      send_to_char( "A valid percent is 0 to 100.\r\n", ch );
      return;
   }
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      if( !str_cmp( dis_class_name( mclass->wclass ), arg ) )
      {
         mcount = mclass->cpercent;
         mclass->cpercent = tmpcount;
         tmclass = mclass;
         break;
      }
   }
   if( !tmclass )
   {
      send_to_char( "No such class to change the percent on.\r\n", ch );
      return;
   }
   tmpcount = 0;
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      tmpcount += mclass->cpercent;
      if( tmpcount < 0 || tmpcount > 100 )
      {
         tmclass->cpercent = mcount; /* Set it back to what it was */
         send_to_char( "Sorry, but the percents on the classes can't go over 100% combined.\r\n", ch );
         return;
      }
   }
   ch_printf( ch, "%s percent has been set to %d.\r\n", dis_class_name( tmclass->wclass ), tmclass->cpercent );
}
