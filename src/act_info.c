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
 *                           Informational module                            *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "h/mud.h"
#include "h/sha256.h"

bool EXA_prog_trigger = true;

int get_ip_logins( void );

char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
   static char buf[MSL];
   bool glowsee = false;

   if( is_obj_stat( obj, ITEM_GLOW ) && is_obj_stat( obj, ITEM_INVIS )
   && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
      glowsee = true;

   buf[0] = '\0';
   if( is_immortal( ch ) )
      snprintf( buf, sizeof( buf ), "[%d]", obj->pIndexData->vnum );
   if( is_obj_stat( obj, ITEM_INVIS ) )
      mudstrlcat( buf, "(Invis) ", sizeof( buf ) );
   if( IS_AFFECTED( ch, AFF_DETECT_EVIL ) || xIS_SET( ch->act, PLR_HOLYLIGHT ) )
   {
      if( is_obj_stat( obj, ITEM_ANTI_EVIL ) )
      {
         if( !is_obj_stat( obj, ITEM_ANTI_NEUTRAL ) )
         {
            if( is_obj_stat( obj, ITEM_ANTI_GOOD ) ) /* Antievil and Antigood */
               mudstrlcat( buf, "(Smouldering Red-White) ", sizeof( buf ) );
            else /* Antievil */
               mudstrlcat( buf, "(Flaming Red) ", sizeof( buf ) );
         }
         else if( !is_obj_stat( obj, ITEM_ANTI_GOOD ) ) /* Antievil and Antineutral */
            mudstrlcat( buf, "(Smouldering Red-Gray) ", sizeof( buf ) );
      }
      else
      {
         if( is_obj_stat( obj, ITEM_ANTI_NEUTRAL ) )
         {
            if( !is_obj_stat( obj, ITEM_ANTI_GOOD ) ) /* Antineutral */
               mudstrlcat( buf, "(Flaming Gray) ", sizeof( buf ) );
            else /* Antineutral and Antigood */
               mudstrlcat( buf, "(Smouldering Gray-White) ", sizeof( buf ) );
         }
         else if( is_obj_stat( obj, ITEM_ANTI_GOOD ) ) /* Antigood */
            mudstrlcat( buf, "(Flaming White) ", sizeof( buf ) );
      }
   }

   if( ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) || xIS_SET( ch->act, PLR_HOLYLIGHT ) ) && is_obj_stat( obj, ITEM_MAGIC ) )
      mudstrlcat( buf, "(Magical) ", sizeof( buf ) );
   if( !glowsee && is_obj_stat( obj, ITEM_GLOW ) )
      mudstrlcat( buf, "(Glowing) ", sizeof( buf ) );
   if( is_obj_stat( obj, ITEM_HIDDEN ) )
      mudstrlcat( buf, "(Hidden) ", sizeof( buf ) );
   if( is_obj_stat( obj, ITEM_BURIED ) )
      mudstrlcat( buf, "(Buried) ", sizeof( buf ) );
   if( is_immortal( ch ) && is_obj_stat( obj, ITEM_PROTOTYPE ) )
      mudstrlcat( buf, "(PROTO) ", sizeof( buf ) );
   if( ( IS_AFFECTED( ch, AFF_DETECTTRAPS ) || xIS_SET( ch->act, PLR_HOLYLIGHT ) ) && is_trapped( obj ) )
      mudstrlcat( buf, "(Trap) ", sizeof( buf ) );

   if( fShort )
   {
      if( glowsee && !is_immortal( ch ) )
         mudstrlcpy( buf, "the faint glow of something", sizeof( buf ) );
      else if( obj->short_descr )
         mudstrlcat( buf, obj->short_descr, sizeof( buf ) );
   }
   else
   {
      if( glowsee && !is_immortal( ch ) )
         mudstrlcpy( buf, "You see the faint glow of something nearby.", sizeof( buf ) );
      else if( obj->description )
         mudstrlcat( buf, obj->description, sizeof( buf ) );
   }

   if( obj->owner && obj->owner[0] != '\0' )
   {
      char buf2[MSL];

      sprintf( buf2, " owned by %s", obj->owner );
      strcat( buf, buf2 );
   }
   return buf;
}

/*
 * Some increasingly freaky hallucinated objects -Thoric
 * (Hats off to Albert Hoffman's "problem child")
 */
const char *hallucinated_object( int ms, bool fShort )
{
   int sms = URANGE( 1, ( ms + 10 ) / 5, 20 );

   if( fShort )
   {
      switch( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) )
      {
         case 1:  return "a sword";
         case 2:  return "a stick";
         case 3:  return "something shiny";
         case 4:  return "something";
         case 5:  return "something interesting";
         case 6:  return "something colorful";
         case 7:  return "something that looks cool";
         case 8:  return "a nifty thing";
         case 9:  return "a cloak of flowing colors";
         case 10: return "a mystical flaming sword";
         case 11: return "a swarm of insects";
         case 12: return "a deathbane";
         case 13: return "a figment of your imagination";
         case 14: return "your gravestone";
         case 15: return "the long lost boots of Ranger Thoric";
         case 16: return "a glowing tome of arcane knowledge";
         case 17: return "a long sought secret";
         case 18: return "the meaning of it all";
         case 19: return "the answer";
         case 20: return "the key to life, the universe and everything";
      }
   }

   switch( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) )
   {
      case 1:  return "A nice looking sword catches your eye.";
      case 2:  return "The ground is covered in small sticks.";
      case 3:  return "Something shiny catches your eye.";
      case 4:  return "Something catches your attention.";
      case 5:  return "Something interesting catches your eye.";
      case 6:  return "Something colorful flows by.";
      case 7:  return "Something that looks cool calls out to you.";
      case 8:  return "A nifty thing of great importance stands here.";
      case 9:  return "A cloak of flowing colors asks you to wear it.";
      case 10: return "A mystical flaming sword awaits your grasp.";
      case 11: return "A swarm of insects buzzes in your face!";
      case 12: return "The extremely rare Deathbane lies at your feet.";
      case 13: return "A figment of your imagination is at your command.";
      case 14: return "You notice a gravestone here... upon closer examination, it reads your name.";
      case 15: return "The long lost boots of Ranger Thoric lie off to the side.";
      case 16: return "A glowing tome of arcane knowledge hovers in the air before you.";
      case 17: return "A long sought secret of all mankind is now clear to you.";
      case 18: return "The meaning of it all, so simple, so clear... of course!";
      case 19: return "The answer.  One.  It's always been One.";
      case 20: return "The key to life, the universe and everything awaits your hand.";
   }

   return "Whoa!!!";
}

/* Copy of num_punct, only changed to handle doubles */
char *double_punct( double foo )
{
   char buf[MIL];
   static char buf_new[MIL];
   unsigned int nindex;
   int index_new, rest, x;

   snprintf( buf, sizeof( buf ), "%.f", foo );
   rest = strlen( buf ) % 3;

   for( nindex = index_new = 0; nindex < strlen( buf ); nindex++, index_new++ )
   {
      x = nindex - rest;
      if( nindex != 0 && ( x % 3 ) == 0 )
         buf_new[index_new++] = ',';
      buf_new[index_new] = buf[nindex];
   }
   buf_new[index_new] = '\0';
   return buf_new;
}

/*
 * This is the punct snippet from Desden el Chaman Tibetano - Nov 1998
 *  Email: jlalbatros@mx2.redestb.es
 */
char *num_punct( int foo )
{
   char buf[16];
   static char buf_new[16];
   unsigned int nindex;
   int index_new, rest, x;

   snprintf( buf, sizeof( buf ), "%d", foo );
   rest = strlen( buf ) % 3;

   for( nindex = index_new = 0; nindex < strlen( buf ); nindex++, index_new++ )
   {
      x = nindex - rest;
      if( nindex != 0 && ( x % 3 ) == 0 )
         buf_new[index_new++] = ',';
      buf_new[index_new] = buf[nindex];
   }
   buf_new[index_new] = '\0';
   return buf_new;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
   OBJ_DATA *obj;
   char **prgpstrShow;
   char *pstrShow;
   int count, offcount, tmp, ms, cnt;
   int *prgnShow, *pitShow, nShow, iShow;
   bool fCombine;

   if( !ch->desc )
      return;

   /* if there's no list... then don't do all this crap! - Thoric */
   if( !list )
   {
      if( fShowNothing )
      {
         if( is_npc( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
            send_to_char( "     ", ch );
         set_char_color( AT_OBJECT, ch );
         send_to_char( "Nothing.\r\n", ch );
      }
      return;
   }

   /* Alloc space for output lines. */
   count = 0;
   for( obj = list; obj; obj = obj->next_content )
      count++;

   ms = ( ch->mental_state ? ch->mental_state : 1 )
      * ( is_npc( ch ) ? 1 : ( ch->pcdata->condition[COND_DRUNK] ? ( ch->pcdata->condition[COND_DRUNK] / 12 ) : 1 ) );

   /* If not mentally stable... */
   if( abs( ms ) > 40 )
   {
      offcount = URANGE( -( count ), ( count * ms ) / 100, count * 2 );
      if( offcount < 0 )
         offcount += number_range( 0, abs( offcount ) );
      else if( offcount > 0 )
         offcount -= number_range( 0, offcount );
   }
   else
      offcount = 0;

   if( count + offcount <= 0 )
   {
      if( fShowNothing )
      {
         if( is_npc( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
            send_to_char( "     ", ch );
         set_char_color( AT_OBJECT, ch );
         send_to_char( "Nothing.\r\n", ch );
      }
      return;
   }

   CREATE( prgpstrShow, char *, count + ( ( offcount > 0 ) ? offcount : 0 ) );
   CREATE( prgnShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
   CREATE( pitShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
   nShow = 0;
   tmp = ( offcount > 0 ) ? offcount : 0;
   cnt = 0;

   /* Format the list of objects. */
   for( obj = list; obj; obj = obj->next_content )
   {
      if( offcount < 0 && ++cnt > ( count + offcount ) )
         break;
      if( tmp > 0 && number_bits( 1 ) == 0 )
      {
         prgpstrShow[nShow] = STRALLOC( hallucinated_object( ms, fShort ) );
         prgnShow[nShow] = 1;
         pitShow[nShow] = number_range( 0, ITEM_TYPE_MAX - 1 );
         nShow++;
         --tmp;
      }
      if( obj->wear_loc == WEAR_NONE
      && ( can_see_obj( ch, obj )
      || ( is_obj_stat( obj, ITEM_INVIS ) && is_obj_stat( obj, ITEM_GLOW )
      && !is_obj_stat( obj, ITEM_BURIED ) && !is_obj_stat( obj, ITEM_HIDDEN ) ) )
      && ( obj->item_type != ITEM_TRAP || obj->value[3] == 0 || IS_AFFECTED( ch, AFF_DETECTTRAPS ) ) )
      {
         pstrShow = format_obj_to_char( obj, ch, fShort );
         fCombine = false;

         if( is_npc( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
         {
            /*
             * Look for duplicates, case sensitive.
             * Matches tend to be near end so run loop backwords.
             */
            for( iShow = nShow - 1; iShow >= 0; iShow-- )
            {
               if( !strcmp( prgpstrShow[iShow], pstrShow ) )
               {
                  prgnShow[iShow] += obj->count;
                  fCombine = true;
                  break;
               }
            }
         }

         pitShow[nShow] = obj->item_type;

         /* Couldn't combine, or didn't want to. */
         if( !fCombine )
         {
            prgpstrShow[nShow] = STRALLOC( pstrShow );
            prgnShow[nShow] = obj->count;
            nShow++;
         }
      }
   }
   if( tmp > 0 )
   {
      int x;
      for( x = 0; x < tmp; x++ )
      {
         prgpstrShow[nShow] = STRALLOC( hallucinated_object( ms, fShort ) );
         prgnShow[nShow] = 1;
         pitShow[nShow] = number_range( 0, ITEM_TYPE_MAX - 1 );
         nShow++;
      }
   }

   /* Output the formatted list. -Color support by Thoric */
   for( iShow = 0; iShow < nShow; iShow++ )
   {
      switch( pitShow[iShow] )
      {
         default:
            set_char_color( AT_OBJECT, ch );
            break;

         case ITEM_BLOOD:
            set_char_color( AT_BLOOD, ch );
            break;

         case ITEM_MONEY:
         case ITEM_TREASURE:
            set_char_color( AT_YELLOW, ch );
            break;

         case ITEM_COOK:
         case ITEM_FOOD:
         case ITEM_FISH:
            set_char_color( AT_HUNGRY, ch );
            break;

         case ITEM_DRINK_CON:
         case ITEM_FOUNTAIN:
            set_char_color( AT_THIRSTY, ch );
            break;

         case ITEM_FIRE:
            set_char_color( AT_FIRE, ch );
            break;

         case ITEM_SCROLL:
         case ITEM_WAND:
         case ITEM_STAFF:
            set_char_color( AT_MAGIC, ch );
            break;
      }
      if( fShowNothing )
         send_to_char( "     ", ch );
      send_to_char( prgpstrShow[iShow], ch );
      if( prgnShow[iShow] != 1 )
         ch_printf( ch, " (%d)", prgnShow[iShow] );
      send_to_char( "\r\n", ch );
      STRFREE( prgpstrShow[iShow] );
   }

   if( fShowNothing && nShow == 0 )
   {
      if( is_npc( ch ) || xIS_SET( ch->act, PLR_COMBINE ) )
         send_to_char( "     ", ch );
      set_char_color( AT_OBJECT, ch );
      send_to_char( "Nothing.\r\n", ch );
   }
   DISPOSE( prgpstrShow );
   DISPOSE( prgnShow );
   DISPOSE( pitShow );
}

/* Show fancy descriptions for certain spell affects - Thoric */
void show_visible_affects_to_char( CHAR_DATA *victim, CHAR_DATA *ch )
{
   char name[MSL];

   if( is_npc( victim ) )
      mudstrlcpy( name, victim->short_descr, sizeof( name ) );
   else
      mudstrlcpy( name, victim->name, sizeof( name ) );
   name[0] = toupper( name[0] );

   if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
   {
      if( is_good( victim ) )
         ch_printf( ch, "&[white]%s glows with an aura of divine radiance.&d\r\n", name );
      else if( is_evil( victim ) )
         ch_printf( ch, "&[white]%s shimmers beneath an aura of dark energy.&d\r\n", name );
      else
         ch_printf( ch, "&[white]%s is shrouded in flowing shadow and light.&d\r\n", name );
   }
   if( IS_AFFECTED( victim, AFF_FIRESHIELD ) )
      ch_printf( ch, "&[fire]%s is engulfed within a blaze of mystical flame.&d\r\n", name );
   if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) )
      ch_printf( ch, "&[blue]%s is surrounded by cascading torrents of energy.&d\r\n", name );
   if( IS_AFFECTED( victim, AFF_ACIDMIST ) )
      ch_printf( ch, "&[green]%s is visible through a cloud of churning mist.&d\r\n", name );
   /*Scryn 8/13*/
   if( IS_AFFECTED( victim, AFF_ICESHIELD ) )
      ch_printf( ch, "&[lblue]%s is ensphered by shards of glistening ice.&d\r\n", name );
   if( IS_AFFECTED( victim, AFF_VENOMSHIELD ) )
      ch_printf( ch, "&[green]%s is enshrouded in a choking cloud of gas.&d\r\n", name );
   if( IS_AFFECTED( victim, AFF_CHARM ) )
      ch_printf( ch, "&[magic]%s wanders in a dazed, zombie-like state.&d\r\n", name );
}

void show_affects_to_char( CHAR_DATA *victim, CHAR_DATA *ch )
{
   char name[MSL];
   int cnt = 0;

   if( is_npc( victim ) )
      mudstrlcpy( name, victim->short_descr, sizeof( name ) );
   else
      mudstrlcpy( name, victim->name, sizeof( name ) );
   name[0] = toupper( name[0] );

   if( is_npc( ch ) || xIS_SET( ch->act, PLR_GROUPAFFECTS ) )
   {
      /* Show a single line message */
      if( !IS_AFFECTED( victim, AFF_SANCTUARY ) && !IS_AFFECTED( victim, AFF_FIRESHIELD )
      && !IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( victim, AFF_ACIDMIST )
      && !IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( victim, AFF_VENOMSHIELD )
      && !IS_AFFECTED( victim, AFF_CHARM ) )
         return;

      ch_printf( ch, "%s%s &[magic]is affected by: ",
         victim->sex == SEX_MALE ? color_str( AT_MALE, ch ) :
         victim->sex == SEX_FEMALE ? color_str( AT_FEMALE, ch ) :
         color_str( AT_PERSON, ch ), name );

      if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
         ch_printf( ch, "%s&[white]Sanctuary", (++cnt > 1) ? " " : "" );
      if( IS_AFFECTED( victim, AFF_FIRESHIELD ) )
         ch_printf( ch, "%s&[fire]Fireshield", (++cnt > 1) ? " " : "" );
      if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) )
         ch_printf( ch, "%s&[blue]Shockshield", (++cnt > 1) ? " " : "" );
      if( IS_AFFECTED( victim, AFF_ACIDMIST ) )
         ch_printf( ch, "%s&[green]Acidmist", (++cnt > 1) ? " " : "" );
      if( IS_AFFECTED( victim, AFF_ICESHIELD ) )
         ch_printf( ch, "%s&[lblue]Iceshield", (++cnt > 1) ? " " : "" );
      if( IS_AFFECTED( victim, AFF_VENOMSHIELD ) )
         ch_printf( ch, "%s&[green]Venomshield", (++cnt > 1) ? " " : "" );
      if( IS_AFFECTED( victim, AFF_CHARM ) )
         ch_printf( ch, "%s&[magic]Charm", (++cnt > 1) ? " " : "" );
      send_to_char( "&D\r\n", ch );

      return;
   }
   show_visible_affects_to_char( victim, ch );
}

void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
   TIMER *timer;
   const char *person;
   char buf[MSL], buf1[MSL];

   buf[0] = '\0';

   person = color_str( AT_PERSON, ch );
   if( victim->sex == SEX_MALE )
      person = color_str( AT_MALE, ch );
   if( victim->sex == SEX_FEMALE )
      person = color_str( AT_FEMALE, ch );

   set_char_color( AT_PERSON, ch );
   if( is_immortal( ch ) && is_npc( victim ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s[&W%d%s]", person, victim->pIndexData->vnum, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( !is_npc( victim ) && !victim->desc )
   {
      snprintf( buf1, sizeof( buf1 ), "%s[&W(Link Dead)%s] ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_AFK ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s[&WAFK%s] ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( ( !is_npc( victim ) && xIS_SET( victim->act, PLR_WIZINVIS ) )
   || ( is_npc( victim ) && xIS_SET( victim->act, ACT_MOBINVIS ) ) )
   {
      if( !is_npc( victim ) )
         snprintf( buf1, sizeof( buf1 ), "%s(Invis &W%d%s) ", person, victim->pcdata->wizinvis, person );
      else
         snprintf( buf1, sizeof( buf1 ), "%s(Mobinvis &W%d%s) ", person, victim->mobinvis, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( !is_npc( victim ) )
   {
      if( is_immortal( victim ) )
      {
         if( is_retired( victim ) )
            snprintf( buf1, sizeof( buf1 ), "%s(&WRetired%s) ", person, person );
         else if( is_guest( victim ) )
            snprintf( buf1, sizeof( buf1 ), "%s(&WGuest%s) ", person, person );
         else
            snprintf( buf1, sizeof( buf1 ), "%s(&WImmortal%s) ", person, person );
         mudstrlcat( buf, buf1, sizeof( buf ) );
      }

      if( victim->pcdata->clan
      && xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY )
      && victim->pcdata->clan->badge
      && victim->pcdata->clan->clan_type == CLAN_PLAIN )
      {
         snprintf( buf1, sizeof( buf1 ), "%s%s%s ", person, victim->pcdata->clan->badge, person );
         mudstrlcat( buf, buf1, sizeof( buf ) );
      }
      else if( can_pkill( victim ) && !is_immortal( victim ) )
      {
         snprintf( buf1, sizeof( buf1 ), "%s(&WUnclanned%s) ", person, person );
         mudstrlcat( buf, buf1, sizeof( buf ) );
      }
   }

   if( IS_AFFECTED( victim, AFF_INVISIBLE ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&WInvis%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }
   if( IS_AFFECTED( victim, AFF_HIDE ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&WHide%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }
   if( IS_AFFECTED( victim, AFF_PASS_DOOR ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&WTranslucent%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }
   if( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&PPink Aura%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( IS_AFFECTED( ch, AFF_DETECT_EVIL ) )
   {
      if( is_evil( victim ) )
         snprintf( buf1, sizeof( buf1 ), "%s(&RRed Aura%s) ", person, person );
      if( is_neutral( victim ) )
         snprintf( buf1, sizeof( buf1 ), "%s(&zGray Aura%s) ", person, person );
      if( is_good( victim ) )
         snprintf( buf1, sizeof( buf1 ), "%s(&WWhite Aura%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( IS_AFFECTED( victim, AFF_BERSERK ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&RWild-eyed%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( is_npc( victim ) && is_immortal( ch ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&WPROTO%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( is_npc( victim ) && ch->mount && ch->mount == victim && ch->in_room == ch->mount->in_room )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&WMount%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( victim->desc && victim->desc->connected == CON_EDITING )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&WWriting%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( victim->morph )
   {
      snprintf( buf1, sizeof( buf1 ), "%s(&WMorphed%s) ", person, person );
      mudstrlcat( buf, buf1, sizeof( buf ) );
   }

   if( ( victim->position == victim->defposition && victim->long_descr[0] != '\0' )
   || ( victim->morph && victim->morph->morph && victim->morph->morph->defpos == victim->position ) )
   {
      if( victim->morph )
      {
         if( !is_immortal( ch ) )
         {
            if( victim->morph->morph )
               mudstrlcat( buf, victim->morph->morph->long_desc, sizeof( buf ) );
            else
               mudstrlcat( buf, victim->long_descr, sizeof( buf ) );
         }
         else
         {
            mudstrlcat( buf, PERS( victim, ch ), sizeof( buf ) );
            if( !is_npc( victim ) && !xIS_SET( ch->act, PLR_BRIEF ) )
               mudstrlcat( buf, victim->pcdata->title, sizeof( buf ) );
            mudstrlcat( buf, ".\r\n", sizeof( buf ) );
         }
      }
      else
         mudstrlcat( buf, victim->long_descr, sizeof( buf ) );
      send_to_char( buf, ch );
      show_affects_to_char( victim, ch );
      return;
   }
   else
   {
      if( victim->morph && victim->morph->morph && !is_immortal( ch ) )
         mudstrlcat( buf, MORPHPERS( victim, ch ), sizeof( buf ) );
      else
         mudstrlcat( buf, PERS( victim, ch ), sizeof( buf ) );
   }

   if( !is_npc( victim ) && !xIS_SET( ch->act, PLR_BRIEF ) )
      mudstrlcat( buf, victim->pcdata->title, sizeof( buf ) );

   mudstrlcat( buf, person, sizeof( buf ) );

   if( ( timer = get_timerptr( victim, TIMER_DO_FUN ) ) )
   {
      if( timer->do_fun == do_cast )
         mudstrlcat( buf, " is chanting.", sizeof( buf ) );
      else if( timer->do_fun == do_dig )
         mudstrlcat( buf, " is digging.", sizeof( buf ) );
      else if( timer->do_fun == do_search )
         mudstrlcat( buf, " is searching for something.", sizeof( buf ) );
      else if( timer->do_fun == do_detrap )
         mudstrlcat( buf, " is working with a trap here.", sizeof( buf ) );
      else if( timer->do_fun == do_pushup )
         mudstrlcat( buf, " is doing pushups.", sizeof( buf ) );
      else if( timer->do_fun == do_situp )
         mudstrlcat( buf, " is doing situps.", sizeof( buf ) );
      else
         mudstrlcat( buf, " is doing something.", sizeof( buf ) );
   }
   else
   {
      switch( victim->position )
      {
         case POS_DEAD:
            mudstrlcat( buf, " is DEAD!!", sizeof( buf ) );
            break;

         case POS_MORTAL:
            mudstrlcat( buf, " is mortally wounded.", sizeof( buf ) );
            break;

         case POS_INCAP:
            mudstrlcat( buf, " is incapacitated.", sizeof( buf ) );
            break;

         case POS_STUNNED:
            mudstrlcat( buf, " is lying here stunned.", sizeof( buf ) );
            break;

         case POS_SLEEPING:
            if( ch->position == POS_SITTING || ch->position == POS_RESTING )
               mudstrlcat( buf, " is sleeping nearby.", sizeof( buf ) );
            else
               mudstrlcat( buf, " is deep in slumber here.", sizeof( buf ) );
            break;

         case POS_RESTING:
            if( ch->position == POS_RESTING )
               mudstrlcat( buf, " is sprawled out alongside you.", sizeof( buf ) );
            else if( ch->position == POS_MOUNTED )
               mudstrlcat( buf, " is sprawled out at the foot of your mount.", sizeof( buf ) );
            else
               mudstrlcat( buf, " is sprawled out here.", sizeof( buf ) );
            break;

         case POS_SITTING:
            if( ch->position == POS_SITTING )
               mudstrlcat( buf, " sits here with you.", sizeof( buf ) );
            else if( ch->position == POS_RESTING )
               mudstrlcat( buf, " sits nearby as you lie around.", sizeof( buf ) );
            else
               mudstrlcat( buf, " sits upright here.", sizeof( buf ) );
            break;

         case POS_STANDING:
            if( is_immortal( victim ) )
               mudstrlcat( buf, " is here before you.", sizeof( buf ) );
            else if( ( victim->in_room->sector_type == SECT_UNDERWATER || victim->in_room->sector_type == SECT_OCEANFLOOR )
            && !IS_AFFECTED( victim, AFF_AQUA_BREATH ) && !is_npc( victim ) )
               mudstrlcat( buf, " is drowning here.", sizeof( buf ) );
            else if( victim->in_room->sector_type == SECT_UNDERWATER )
               mudstrlcat( buf, " is here in the water.", sizeof( buf ) );
            else if( victim->in_room->sector_type == SECT_OCEANFLOOR )
               mudstrlcat( buf, " is standing here in the water.", sizeof( buf ) );
            else if( IS_AFFECTED( victim, AFF_FLOATING ) || IS_AFFECTED( victim, AFF_FLYING ) )
               mudstrlcat( buf, " is hovering here.", sizeof( buf ) );
            else
               mudstrlcat( buf, " is standing here.", sizeof( buf ) );
            break;

         case POS_SHOVE:
            mudstrlcat( buf, " is being shoved around.", sizeof( buf ) );
            break;

         case POS_DRAG:
            mudstrlcat( buf, " is being dragged around.", sizeof( buf ) );
            break;

         case POS_MOUNTED:
            mudstrlcat( buf, " is here, upon ", sizeof( buf ) );
            if( !victim->mount )
               mudstrlcat( buf, "thin air???", sizeof( buf ) );
            else if( victim->mount == ch )
               mudstrlcat( buf, "your back.", sizeof( buf ) );
            else if( victim->in_room == victim->mount->in_room )
            {
               mudstrlcat( buf, PERS( victim->mount, ch ), sizeof( buf ) );
               mudstrlcat( buf, ".", sizeof( buf ) );
            }
            else
               mudstrlcat( buf, "someone who left??", sizeof( buf ) );
            break;

         case POS_FIGHTING:   case POS_EVASIVE:
         case POS_DEFENSIVE:  case POS_AGGRESSIVE:
         case POS_BERSERK:
            mudstrlcat( buf, " is here, fighting ", sizeof( buf ) );
            if( !victim->fighting )
            {
               mudstrlcat( buf, "thin air???", sizeof( buf ) );

               /* some bug somewhere.... kinda hackey fix -h */
               if( !victim->mount )
                  victim->position = POS_STANDING;
               else
                  victim->position = POS_MOUNTED;
            }
            else if( who_fighting( victim ) == ch )
               mudstrlcat( buf, "YOU!", sizeof( buf ) );
            else if( victim->in_room == victim->fighting->who->in_room )
            {
               mudstrlcat( buf, PERS( victim->fighting->who, ch ), sizeof( buf ) );
               mudstrlcat( buf, ".", sizeof( buf ) );
            }
            else
               mudstrlcat( buf, "someone who left??", sizeof( buf ) );
            break;
      }
   }
   mudstrlcat( buf, "\r\n", sizeof( buf ) );
   buf[0] = UPPER( buf[0] );
   send_to_char( buf, ch );
   show_affects_to_char( victim, ch );
}

void show_race_line( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int feet, inches;

   if( is_npc( victim ) )
      return;

   feet = victim->height / 12;
   inches = victim->height % 12;

   if( victim == ch )
      ch_printf( ch, "You're %d'%d\" and weigh %d pounds.\r\n", feet, inches, victim->weight );
   else
      ch_printf( ch, "%s is %d'%d\" and weighs %d pounds.\r\n", PERS( victim, ch ), feet, inches, victim->weight );
}

void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
{
   OBJ_DATA *obj;
   int iWear;
   bool found;

   if( can_see( victim, ch ) && !is_npc( ch ) && !xIS_SET( ch->act, PLR_WIZINVIS ) )
   {
      act( AT_ACTION, "$n looks at you.", ch, NULL, victim, TO_VICT );
      act_printf( AT_ACTION, ch, NULL, victim, TO_NOTVICT, "$n looks at %s.", ( victim == ch ) ? "$mself" : "$N" );
   }

   if( victim->description )
   {
      if( victim->morph && victim->morph->morph && victim->morph->morph->description )
         send_to_char( victim->morph->morph->description, ch );
      else
         send_to_char( victim->description, ch );
   }
   else
   {
      if( victim->morph && victim->morph->morph && victim->morph->morph->description )
         send_to_char( victim->morph->morph->description, ch );
      else if( is_npc( victim ) )
         act( AT_PLAIN, "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
      else if( ch != victim )
         act( AT_PLAIN, "$E isn't much to look at...", ch, NULL, victim, TO_CHAR );
      else
         act( AT_PLAIN, "You're not much to look at...", ch, NULL, NULL, TO_CHAR );
   }

   show_race_line( ch, victim );
   show_condition( ch, victim );

   found = false;
   for( iWear = 0; iWear < WEAR_MAX; iWear++ )
   {
      if( ( obj = get_eq_char( victim, iWear ) ) && can_see_obj( ch, obj ) )
      {
         if( !found )
         {
            send_to_char( "\r\n", ch );
            if( victim != ch )
               act( AT_PLAIN, "$N is using:", ch, NULL, victim, TO_CHAR );
            else
               act( AT_PLAIN, "You're using:", ch, NULL, NULL, TO_CHAR );
            found = true;
         }
         if( !is_obj_stat( obj, ITEM_LODGED ) )
         {
            if( ( !is_npc( victim ) ) && ( victim->race > 0 ) && ( victim->race < MAX_PC_RACE )
            && race_table[victim->race] && race_table[victim->race]->where_name[iWear] )
               send_to_char( race_table[victim->race]->where_name[iWear], ch );
            else
               send_to_char( where_name[iWear], ch );
         }
         else
         {
            if( ( !is_npc( victim ) ) && ( victim->race > 0 ) && ( victim->race < MAX_PC_RACE )
            && race_table[victim->race] && race_table[victim->race]->lodge_name[iWear] )
               send_to_char( race_table[victim->race]->lodge_name[iWear], ch );
            else
               send_to_char( lodge_name[iWear], ch );
         }
         send_to_char( format_obj_to_char( obj, ch, true ), ch );
         send_to_char( "\r\n", ch );
      }
   }

   /* Crash fix here by Thoric */
   if( is_npc( ch ) || victim == ch )
      return;

   if( is_immortal( ch ) )
   {
      if( is_npc( victim ) )
         ch_printf( ch, "\r\nMobile #%d '%s' ", victim->pIndexData->vnum, victim->name );
      else
         ch_printf( ch, "\r\n%s ", victim->name );
      ch_printf( ch, "is level %d", victim->level );
      if( !is_npc( victim ) )
         ch_printf( ch, " and a %s %s", dis_race_name( victim->race ), dis_main_class_name( victim ) );
      send_to_char( ".\r\n", ch );
   }

   if( number_percent( ) < LEARNED( ch, gsn_peek ) )
   {
      ch_printf( ch, "\r\nYou peek at %s inventory:\r\n", victim->sex == 1 ? "his" : victim->sex == 2 ? "her" : "its" );
      show_list_to_char( victim->first_carrying, ch, true, true );
      learn_from_success( ch, gsn_peek );
   }
   else if( ch->pcdata->learned[gsn_peek] > 0 )
      learn_from_failure( ch, gsn_peek );
}

void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
   CHAR_DATA *rch;

   for( rch = list; rch; rch = rch->next_in_room )
   {
      if( rch == ch )
         continue;

      if( !can_see_character( ch, rch ) )
         continue;

      if( can_see( ch, rch ) )
         show_char_to_char_0( rch, ch );
      else if( room_is_dark( ch->in_room ) && IS_AFFECTED( ch, AFF_INFRARED ) && is_npc( rch ) && !is_immortal( rch ) )
      {
         set_char_color( AT_BLOOD, ch );
         send_to_char( "The red form of a living creature is here.\r\n", ch );
      }
   }
}

/* Used to display the list of characters in a room grouped */
void show_chars_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
   CHAR_DATA *rch;
   char tmpbuf[MSL], buf[MSL];
   int currcolor = AT_PERSON, lastcolor = AT_PERSON, strtrack = 0;
   bool sendspace = false;

   set_char_color( AT_PERSON, ch );
   for( rch = list; rch; rch = rch->next_in_room )
   {
      if( rch == ch || !can_see( ch, rch ) )
         continue;

      if( !can_see_character( ch, rch ) )
         continue;

      currcolor = AT_PERSON;
      if( rch->sex == SEX_MALE )
         currcolor = AT_MALE;
      if( rch->sex == SEX_FEMALE )
         currcolor = AT_FEMALE;
      snprintf( tmpbuf, sizeof( tmpbuf ), "[%s]", PERS( rch, ch ) );
      strtrack += strlen( tmpbuf );
      if( sendspace )
         strtrack++;
      if( strtrack >= 80 )
      {
         strtrack = strlen( tmpbuf );
         snprintf( buf, sizeof( buf ), "\r\n%s%s", lastcolor != currcolor ? color_str( currcolor, ch ) : "", tmpbuf );
      }
      else
         snprintf( buf, sizeof( buf ), "%s%s%s", sendspace ? " " : "", lastcolor != currcolor ? color_str( currcolor, ch ) : "", tmpbuf );
      sendspace = true;
      send_to_char( buf, ch );
      lastcolor = currcolor;
   }
   if( strtrack != 0 )
      send_to_char( "\r\n", ch );
}

bool check_blind( CHAR_DATA *ch )
{
   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_HOLYLIGHT ) )
      return true;

   if( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
      return true;

   if( IS_AFFECTED( ch, AFF_BLIND ) )
   {
      send_to_char( "You can't see a thing!\r\n", ch );
      return false;
   }

   return true;
}

/* Returns classical DIKU door direction based on text in arg - Thoric */
int get_door( char *arg )
{
   int door = -1;

   switch( UPPER( arg[0] ) )
   {
      case 'D':
         if( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
            door = 5;
         break;

      case 'E':
         if( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
            door = 1;
         break;

      case 'N':
         if( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
            door = 0;
         else if( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
            door = 6;
         else if( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
            door = 7;
         break;

      case 'S':
         if( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
            door = 2;
         else if( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
            door = 8;
         else if( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
            door = 9;
         break;

      case 'U':
         if( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
            door = 4;
         break;

      case 'W':
         if( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
            door = 3;
         break;
   }

   return door;
}

void show_compass( CHAR_DATA *ch )
{
   EXIT_DATA *pexit;
   char first[50], last[50];
   char dir_n[50], dir_e[50], dir_s[50], dir_w[50], dir_u[50], dir_d[50], dir_ne[50];
   char dir_nw[50], dir_se[50], dir_sw[50];
   bool found = false;

   if( !ch || !ch->in_room )
   {
      bug( "%s: NULL ch or NULL ch->in_room", __FUNCTION__ );
      return;
   }

   if( is_npc( ch ) || !xIS_SET( ch->act, PLR_COMPASS ) || !ch->in_room->first_exit )
      return;

   dir_nw[0] = dir_n[0] = dir_ne[0] = '\0';
   dir_e[0] = dir_u[0] = dir_d[0] = dir_w[0] = '\0';
   dir_sw[0] = dir_s[0] = dir_se[0] = '\0';

   strcpy( dir_nw, "----" );
   strcpy( dir_n, "---" );
   strcpy( dir_ne, "----" );

   strcpy( dir_w, "---" );
   strcpy( dir_u, "---" );
   strcpy( dir_d, "---" );
   strcpy( dir_e, "---" );

   strcpy( dir_sw, "----" );
   strcpy( dir_s, "---" );
   strcpy( dir_se, "----" );

   for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( !pexit->to_room )
         continue;

      /* Don't show closed and hidden/secret/dig doors */
      if( xIS_SET( pexit->exit_info, EX_CLOSED )
      && ( xIS_SET( pexit->exit_info, EX_HIDDEN ) || xIS_SET( pexit->exit_info, EX_SECRET ) || xIS_SET( pexit->exit_info, EX_DIG ) ) )
         continue;

      found = true;

      /* Default */
      snprintf( first, sizeof( first ), "%s", "-" );
      snprintf( last, sizeof( last ), "%s", "-" );

      if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         snprintf( first, sizeof( first ), "%s", "(" );
         snprintf( last, sizeof( last ), "%s", ")" );
      }
      if( xIS_SET( pexit->exit_info, EX_WINDOW ) )
      {
         snprintf( first, sizeof( first ), "%s", "[" );
         snprintf( last, sizeof( last ), "%s", "]" );
      }

      if( pexit->vdir == DIR_NORTHWEST )
         sprintf( dir_nw, "%sNW%s", first, last );
      if( pexit->vdir == DIR_NORTH )
         sprintf( dir_n, "%sN%s", first, last );
      if( pexit->vdir == DIR_NORTHEAST )
         sprintf( dir_ne, "%sNE%s", first, last );

      if( pexit->vdir == DIR_EAST )
         sprintf( dir_e, "%sE%s", first, last );
      if( pexit->vdir == DIR_UP )
         sprintf( dir_u, "%sU%s", first, last );
      if( pexit->vdir == DIR_DOWN )
         sprintf( dir_d, "%sD%s", first, last );
      if( pexit->vdir == DIR_WEST )
         sprintf( dir_w, "%sW%s", first, last );

      if( pexit->vdir == DIR_SOUTHWEST )
         sprintf( dir_sw, "%sSW%s", first, last );
      if( pexit->vdir == DIR_SOUTH )
         sprintf( dir_s, "%sS%s", first, last );
      if( pexit->vdir == DIR_SOUTHEAST )
         sprintf( dir_se, "%sSE%s", first, last );
    }

    if( !found )
       return;

    ch_printf( ch, "&W%s&w------&W%s&w------&W%s\r\n", dir_nw, dir_n, dir_ne );
    ch_printf( ch, "&W%s&w---&W%s&w<-&G*&w->&W%s&w---&W%s\r\n", dir_w, dir_u, dir_d, dir_e );
    ch_printf( ch, "&W%s&w------&W%s&w------&W%s\r\n", dir_sw, dir_s, dir_se );
}

bool compare_str_desc( char *str, char operation, char *cstr )
{
   bool ismatch = true;

   if( !str || !cstr )
      return false;

   if( operation == '!' )
      ismatch = false;

   if( !ismatch )
   {
      if( str_cmp( str, cstr ) )
         return true;
      return false;
   }

   if( !str_cmp( str, cstr ) )
      return true;

   return false;
}

/* Send two numbers, fnum is what we are comparing against cnum, operation is to know how to check it */
bool compare_for_desc( int fnum, char operation, char *compare )
{
   int ccnum = 0;

   if( compare && is_number( compare ) )
      ccnum = atoi( compare );
   else /* If nothing to compare or isn't a number return false */
      return false;

   switch( operation )
   {
      default:
         return false;

      case '<':
         if( fnum < ccnum )
            return true;
         return false;

      case '>':
         if( fnum > ccnum )
            return true;
         return false;

      case '=':
         if( fnum == ccnum )
            return true;
         return false;

      case '!':
         if( fnum != ccnum )
            return true;
         return false;
   }

   return false;
}

/* Take the information and send back adddesc if it needs added if not send NULL */
char *handle_check( CHAR_DATA *ch, char operation, char *check, char *compare, char *adddesc )
{
   if( !ch )
      return NULL;

   /* Make it be as fast as possible */
   switch( UPPER( check[0] ) )
   {
      default:
         break;

      case 'A':
         if( !str_cmp( check, "Absorb" ) ) /* Are they able to absorb it */
         {
            int value;

            value = get_flag( compare, ris_flags, RIS_MAX );
            if( value < 0 || value >= RIS_MAX )
               return NULL;
            if( operation == '!' ) /* Checking to see if not absorbant to it */
            {
               if( ch->resistant[value] > 100 )
                  return NULL;
               return adddesc;
            }

            if( ch->resistant[value] > 100 )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Affected" ) ) /* Are they affected by it */
         {
            int value;

            value = get_flag( compare, a_flags, AFF_MAX );
            if( value < 0 || value >= AFF_MAX )
               return NULL;
            if( operation == '!' ) /* Checking to see if not affected by it */
            {
               if( xIS_SET( ch->affected_by, value ) )
                  return NULL;
               return adddesc;
            }

            if( xIS_SET( ch->affected_by, value ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'C':
         if( !str_cmp( check, "Constitution" ) )
         {
            if( compare_for_desc( get_curr_stat( STAT_CON, ch ), operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Charisma" ) )
         {
            if( compare_for_desc( get_curr_stat( STAT_CHA, ch ), operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Carry" ) ) /* Are they carrying it? */
         {
            if( operation == '!' ) /* Not carrying it */
            {
               if( get_obj_carry( ch, compare ) )
                  return NULL;
               return adddesc;
            }

            if( get_obj_carry( ch, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'D':
         if( !str_cmp( check, "Dexterity" ) )
         {
            if( compare_for_desc( get_curr_stat( STAT_DEX, ch ), operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'E':
         if( !str_cmp( check, "Exit" ) ) /* Is there an exit in the specified direction? */
         {
            int edir;

            if( !is_number( compare ) ) /* If they specified a name */
               edir = get_dir( compare );
            else
               edir = atoi( compare );

            if( !ch->in_room ) /* Safty Check */
               return NULL;

            if( operation == '!' ) /* Checking to make sure no exit in this room */
            {
               if( get_exit( ch->in_room, edir ) )
                  return NULL;
               return adddesc;
            }

            /* Only should matter if the exit is or isn't there */
            if( get_exit( ch->in_room, edir ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'G':
         if( !str_cmp( check, "Gold" ) )
         {
            if( compare_for_desc( ch->gold, operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'H':
         if( !str_cmp( check, "HP" ) )
         {
            if( compare_for_desc( ch->hit, operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'I':
         if( !str_cmp( check, "Intelligence" ) )
         {
            if( compare_for_desc( get_curr_stat( STAT_INT, ch ), operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Immune" ) ) /* Are they immune to it */
         {
            int value;

            value = get_flag( compare, ris_flags, RIS_MAX );
            if( value < 0 || value >= RIS_MAX )
               return NULL;
            if( operation == '!' ) /* Checking to see if not immune to it */
            {
               if( ch->resistant[value] == 100 )
                  return NULL;
               return adddesc;
            }

            if( ch->resistant[value] == 100 )
               return adddesc;
            return NULL;
         }
         break;

      case 'L':
         if( !str_cmp( check, "Level" ) )
         {
            if( compare_for_desc( ch->level, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Luck" ) )
         {
            if( compare_for_desc( get_curr_stat( STAT_LCK, ch ), operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Light" ) ) /* How much added light is in the room */
         {
            if( !ch->in_room )
               return NULL;

            if( compare_for_desc( ch->in_room->light, operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'M':
         if( !str_cmp( check, "MHour" ) ) /* Mud Hour (Military Time 0 - 23) */
         {
            if( compare_for_desc( time_info.hour, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "MWDay" ) ) /* Mud Weekday */
         {
            if( compare_for_desc( time_info.wday, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "MDay" ) ) /* Mud Day (Of the month) */
         {
            if( compare_for_desc( time_info.day, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "MMonth" ) ) /* Mud Month */
         {
            if( compare_for_desc( time_info.month, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "MYear" ) ) /* Mud Year */
         {
            if( compare_for_desc( time_info.year, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Mana" ) )
         {
            if( compare_for_desc( ch->mana, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Move" ) )
         {
            if( compare_for_desc( ch->move, operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'N':
         if( !str_cmp( check, "Name" ) )
         {
            if( compare_str_desc( ch->name, operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'O':
         if( !str_cmp( check, "Ohere" ) ) /* Is the item somewhere in the room? */
         {
            if( !ch->in_room ) /* Safty Check */
               return NULL;

            if( operation == '!' ) /* Not here */
            {
               if( get_obj_list_rev( ch, compare, ch->in_room->last_content ) )
                  return NULL;
               return adddesc;
            }

            if( get_obj_list_rev( ch, compare, ch->in_room->last_content ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'P':
         if( !str_cmp( check, "Precipitation" ) ) /* Check current precipitation */
         {
            if( !ch->in_room || !ch->in_room->area || !ch->in_room->area->weather )
               return NULL;

            if( compare_for_desc( ch->in_room->area->weather->precip, operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'R':
         if( !str_cmp( check, "Resistant" ) ) /* Are they resistant to it */
         {
            int value;

            value = get_flag( compare, ris_flags, RIS_MAX );
            if( value < 0 || value >= RIS_MAX )
               return NULL;
            if( operation == '!' ) /* Checking to see if not resistant to it */
            {
               if( ch->resistant[value] > 0 )
                  return NULL;
               return adddesc;
            }

            if( ch->resistant[value] > 0 )
               return adddesc;
            return NULL;
         }
         break;

      case 'S':
         if( !str_cmp( check, "Strength" ) )
         {
            if( compare_for_desc( get_curr_stat( STAT_STR, ch ), operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Susceptible" ) ) /* Are they susceptible to it */
         {
            int value;

            value = get_flag( compare, ris_flags, RIS_MAX );
            if( value < 0 || value >= RIS_MAX )
               return NULL;
            if( operation == '!' ) /* Checking to see if not susceptible to it */
            {
               if( ch->resistant[value] < 0 )
                  return NULL;
               return adddesc;
            }

            if( ch->resistant[value] < 0 )
               return adddesc;
            return NULL;
         }
         break;

      case 'T':
         if( !str_cmp( check, "Temperature" ) ) /* Check current temperature */
         {
            if( !ch->in_room || !ch->in_room->area || !ch->in_room->area->weather )
               return NULL;

            if( compare_for_desc( ch->in_room->area->weather->temp, operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;

      case 'W':
         if( !str_cmp( check, "Worn" ) ) /* Are they wearing it? */
         {
            if( operation == '!' ) /* Not wearing it */
            {
               if( get_obj_wear( ch, compare ) )
                  return NULL;
               return adddesc;
            }

            if( get_obj_wear( ch, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Wind" ) ) /* Check current wind */
         {
            if( !ch->in_room || !ch->in_room->area || !ch->in_room->area->weather )
               return NULL;

            if( compare_for_desc( ch->in_room->area->weather->wind, operation, compare ) )
               return adddesc;
            return NULL;
         }
         if( !str_cmp( check, "Wisdom" ) )
         {
            if( compare_for_desc( get_curr_stat( STAT_WIS, ch ), operation, compare ) )
               return adddesc;
            return NULL;
         }
         break;
   }

   return NULL;
}

char showdescription[MSL];
int desccount;
int desccheck;

/* Strip the tags and return whats left */
char *get_tag( CHAR_DATA *ch, char *str )
{
   char nadddesc[MSL], ncheck[MSL], ncompare[MSL], operation, *adddesc;
   int i;
   bool foundtag, foundextra, sendnewline, shouldignoreextra = false;

   /* Set this to true to ignore extras if previous was invalid */
   shouldignoreextra = true;

   if( !ch || !str || *str == '\0' )
      return (char *)"";

   while( *str != '\0' )
   {
      foundtag = false;
      foundextra = false;
      sendnewline = false;

      /* See if we need to parse it for the command */
      if( *str == '[' )
      {
         str++;
         if( *str == '#' )
         {
            foundtag = true;
            str++;
         }
         else
            str--;
      }
      if( foundtag )
      {
         foundtag = false;

         /* Get the check */
         i = 0;
         desccheck++;

         while( *str != ' ' && *str != '\0' && *str != '\r' && *str != '\n' )
         {
            ncheck[i++] = *str;
            str++;
         }
         ncheck[i] = '\0';

         /* Skip the spaces */
         while( *str == ' ' )
            str++;

         /* Get the operation if it's there */
         operation = '\0';
         if( *str == '<' || *str == '>' || *str == '=' || *str == '!' )
         {
            operation = *str;
            str++;
         }

         /* Skip the spaces */
         while( *str == ' ' )
            str++;

         /* Get what we are comparing */
         i = 0;
         while( *str != ' ' && *str != '\0' && *str != ':' && *str != '\r' && *str != '\n' )
         {
            ncompare[i++] = *str;
            str++;
         }
         ncompare[i] = '\0';

         /* Need to skip the : and any spaces */
         while( *str == ':' || *str == ' ' )
            str++;

         /* Go ahead and get what else could be shown */
         i = 0;
         while( *str != ']' && *str != '\0' )
         {
            /* Ignore the normal lines here */
            if( *str == '\r' || *str == '\n' )
            {
               /* Have to add in a space or it runs stuff together */
               if( *str == '\n' && nadddesc[i - 1] != ' ' && nadddesc[i - 1] != '\n' && nadddesc[i - 1] != '\r' )
                  nadddesc[i++] = ' ';
               str++;
               continue;
            }

            /* If this is a space and there is already a space or on a new line skip spaces */
            if( *str == ' ' )
            {
               if( nadddesc[i - 1] == ' ' || nadddesc[i - 1] == '\n' || nadddesc[i - 1] == '\r' )
                  while( *str == ' ' )
                     str++;
            }

            /* Look and see if trying to start another check while in a check */
            if( *str == '[' )
            {
               str++;
               /* Extra tags in this one? */
               if( *str == '#' )
                  foundextra = true;
               /* Allow them to add new lines where they want using [] */
               if( *str == ']' )
               {
                  nadddesc[i++] = '\r';
                  nadddesc[i++] = '\n';
                  str++;
                  continue;
               }
               str--;
            }
            if( foundextra )
            {
               /* If its a valid check need to put whats currently in there in */
               nadddesc[i] = '\0';
               i = 0;
               adddesc = handle_check( ch, operation, ncheck, ncompare, nadddesc );

               if( shouldignoreextra && !adddesc ) /* Only ignore if shouldignoreextra is true and check was NULL */
               {
                  int excount = 2; /* We are in 2 right now, and want to exit out of it fully */

                  foundextra = false;
                  while( *str != '\0' )
                  {
                     str++;
                     if( *str == '[' )
                        excount++;
                     if( *str == ']' )
                        excount--;
                     if( excount == 0 )
                        break;
                  }
                  continue;
               }
               
               if( adddesc && *adddesc != '\0' )
               {
                  while( *adddesc != '\0' )
                  {
                     showdescription[desccount++] = *adddesc;
                     adddesc++;
                  }
               }

               foundextra = false;
               str = get_tag( ch, str );
               continue;
            }
            nadddesc[i++] = *str;
            str++;
         }

         /* Need to skip the ], return and new line */
         while( *str == ']' || *str == '\r' || *str == '\n' )
         {
            if( *str == ']' && --desccheck == 0 )
            {
               sendnewline = true;
               nadddesc[i++] = '\r';
               nadddesc[i++] = '\n';
            }
            str++;
         }

         nadddesc[i] = '\0';

         adddesc = handle_check( ch, operation, ncheck, ncompare, nadddesc );
         if( adddesc && *adddesc != '\0' )
         {
            while( *adddesc != '\0' )
            {
               if( *adddesc != ' '
               || ( desccount != 0 && showdescription[desccount - 1] != ' '
               && showdescription[desccount - 1] != '\r' && showdescription[desccount - 1] != '\n' ) )
                  showdescription[desccount++] = *adddesc;
               adddesc++;
            }
         }
         else if( sendnewline )
         {
            if( desccount != 0 && showdescription[desccount - 1] != '\n' )
            {
               showdescription[desccount++] = '\r';
               showdescription[desccount++] = '\n';
            }
         }
         return str;
      }
      return str;
   }
   return str;
}

void parse_description( CHAR_DATA *ch, char *str )
{
   bool foundtag;

   if( !ch || !str || str[0] == '\0' )
      return;

   desccount = 0;
   desccheck = 0;
   for( ; *str != '\0'; )
   {
      foundtag = false;

      /* See if we need to parse it */
      if( *str == '[' )
      {
         str++;
         if( *str == '#' )
            foundtag = true;
         str--;
      }
      /* Tag was found so handle it */
      if( foundtag )
      {
         str = get_tag( ch, str );
         continue;
      }
      showdescription[desccount++] = *str;
      str++;
   }
   showdescription[desccount] = '\0';

   send_to_char( showdescription, ch );
}

/* Used to send the room description to the character */
void show_room_description( CHAR_DATA *ch )
{
   /* Don't bother doing anything */
   if( !ch || !ch->in_room || !ch->in_room->description )
      return;

   if( xIS_SET( ch->in_room->room_flags, ROOM_WILDERNESS ) )
      parse_wilderness_description( ch, ch->in_room->description );
   else
      parse_description( ch, ch->in_room->description );
}

CMDF( do_look )
{
   EXIT_DATA *pexit;
   CHAR_DATA *victim = NULL;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *original;
   char arg[MIL], arg1[MIL], arg2[MIL], arg3[MIL];
   char *pdesc;
   int number, cnt, ocheck = 0;
   short door;
   bool darkroom = false;

   if( !ch->desc )
      return;

   if( ch->position < POS_SLEEPING )
   {
      send_to_char( "You can't see anything but stars!\r\n", ch );
      return;
   }

   if( ch->position == POS_SLEEPING )
   {
      send_to_char( "You can't see anything, you're sleeping!\r\n", ch );
      return;
   }

   if( !check_blind( ch ) )
      return;

   if( !ch->in_room )
   {
      send_to_char( "You're in a NULL room?\r\n", ch );
      bug( "%s: %s is in a NULL room!", __FUNCTION__, ch->name );
      return;
   }

   if( !is_npc( ch ) && !xIS_SET( ch->act, PLR_HOLYLIGHT )
   && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && room_is_dark( ch->in_room ) )
      darkroom = true;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1 == NULL || arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
   {
      if( !darkroom )
      {
         set_char_color( AT_RMNAME, ch );
         send_to_char( ch->in_room->name, ch );
         send_to_char( "\r\n", ch );
         set_char_color( AT_RMDESC, ch );
         if( arg1[0] == '\0' || ( !is_npc( ch ) && !xIS_SET( ch->act, PLR_BRIEF ) ) )
            show_room_description( ch );
         if( !is_in_wilderness( ch ) )
         {
            show_compass( ch );
            if( !is_npc( ch ) && xIS_SET( ch->act, PLR_AUTOEXIT ) )
               do_exits( ch, (char *)"auto" );
         }
         else if( !is_npc( ch ) && is_immortal( ch ) )
            ch_printf( ch, "&cCurrent Cords X[&w%d&c] Y[&w%d&c]\r\n", ch->cords[0], ch->cords[1] );
         if( !is_npc( ch ) && is_immortal( ch ) && !xIS_EMPTY( ch->in_room->room_flags ) )
            ch_printf( ch, "&cRoom flags: &w%s\r\n", ext_flag_string( &ch->in_room->room_flags, r_flags ) );
      }
      else
      {
         set_char_color( AT_DGRAY, ch );
         send_to_char( "It is pitch black...\r\n", ch );
      }
      show_list_to_char( ch->in_room->first_content, ch, false, false );
      if( ch->in_room->charcount < 10 )
         show_char_to_char( ch->in_room->first_person, ch );
      else
         show_chars_to_char( ch->in_room->first_person, ch );
      return;
   }

   /* Need some way to specify if we want to look at things in room or in inventory */
   if( !str_cmp( arg1, "room" ) )
   {
      ocheck = 1;
      snprintf( arg1, sizeof( arg1 ), "%s", arg2 );
      snprintf( arg2, sizeof( arg2 ), "%s", arg3 );
      argument = one_argument( argument, arg3 );
   }
   else if( !str_cmp( arg1, "inv" ) || !str_cmp( arg1, "inventory" ) )
   {
      ocheck = 2;
      snprintf( arg1, sizeof( arg1 ), "%s", arg2 );
      snprintf( arg2, sizeof( arg2 ), "%s", arg3 );
      argument = one_argument( argument, arg3 );
   }
   else if( !str_cmp( arg1, "worn" ) )
   {
      ocheck = 3;
      snprintf( arg1, sizeof( arg1 ), "%s", arg2 );
      snprintf( arg2, sizeof( arg2 ), "%s", arg3 );
      argument = one_argument( argument, arg3 );
   }

   if( !str_cmp( arg1, "under" ) )
   {
      int count;

      if( arg2[0] == '\0' )
      {
         send_to_char( "Look beneath what?\r\n", ch );
         return;
      }

      if( !( obj = new_get_obj_here( ocheck, ch, arg2 ) ) )
      {
         send_to_char( "You don't see that here.\r\n", ch );
         return;
      }

      if( can_wear( obj, ITEM_NO_TAKE ) && get_trust( ch ) < sysdata.perm_getobjnotake )
      {
         send_to_char( "You can't seem to get a grip on it.\r\n", ch );
         return;
      }

      if( ch->carry_weight + obj->weight > can_carry_w( ch ) )
      {
         send_to_char( "It's too heavy for you to look under.\r\n", ch );
         return;
      }
      count = obj->count;
      obj->count = 1;
      act( AT_PLAIN, "You lift $p and look beneath it:", ch, obj, NULL, TO_CHAR );
      act( AT_PLAIN, "$n lifts $p and looks beneath it:", ch, obj, NULL, TO_ROOM );
      obj->count = count;
      if( is_obj_stat( obj, ITEM_COVERING ) )
         show_list_to_char( obj->first_content, ch, true, true );
      else
         send_to_char( "Nothing.\r\n", ch );
      if( EXA_prog_trigger )
         oprog_examine_trigger( ch, obj );
      return;
   }

   if( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
   {
      int count;

      if( arg2[0] == '\0' )
      {
         send_to_char( "Look in what?\r\n", ch );
         return;
      }

      if( !( obj = new_get_obj_here( ocheck, ch, arg2 ) ) )
      {
         send_to_char( "You don't see that here.\r\n", ch );
         return;
      }

      switch( obj->item_type )
      {
         default:
            send_to_char( "That is not a container.\r\n", ch );
            break;

         case ITEM_DRINK_CON:
            if( obj->value[1] <= 0 )
            {
               send_to_char( "It is empty.\r\n", ch );
               if( EXA_prog_trigger )
                  oprog_examine_trigger( ch, obj );
               break;
            }

            ch_printf( ch, "It's %s of a %s liquid.\r\n",
               ( obj->value[1] < ( obj->value[0] / 4 ) && obj->value[1] < 5 ) ? "almost out"
               : obj->value[1] < ( obj->value[0] / 2 ) ? "less then half full"
               : obj->value[1] < obj->value[0]         ? "less then full"
               : "full",
               ( obj->value[2] >= 0 && obj->value[2] < LIQ_MAX ) ? liq_table[obj->value[2]].liq_color : "?Unknown?" );

            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            break;

         case ITEM_PORTAL:
            for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            {
               if( pexit->vdir == DIR_PORTAL && xIS_SET( pexit->exit_info, EX_PORTAL ) )
               {
                  if( room_is_private( pexit->to_room ) && get_trust( ch ) < sysdata.perm_override_private )
                  {
                     set_char_color( AT_WHITE, ch );
                     send_to_char( "That room is private buster!\r\n", ch );
                     return;
                  }
                  original = ch->in_room;
                  char_from_room( ch );
                  char_to_room( ch, pexit->to_room );
                  do_look( ch, (char *)"auto" );
                  char_from_room( ch );
                  char_to_room( ch, original );
                  return;
               }
            }
            send_to_char( "You see swirling chaos...\r\n", ch );
            break;

         case ITEM_CONTAINER:
         case ITEM_QUIVER:
            if( IS_SET( obj->value[1], CONT_CLOSED ) )
            {
               send_to_char( "It is closed.\r\n", ch );
               break;
            }

         case ITEM_CORPSE_NPC:
         case ITEM_CORPSE_PC:
         case ITEM_KEYRING:
            count = obj->count;
            obj->count = 1;
            if( obj->item_type == ITEM_CONTAINER )
               act( AT_PLAIN, "$p contains:", ch, obj, NULL, TO_CHAR );
            else
               act( AT_PLAIN, "$p holds:", ch, obj, NULL, TO_CHAR );
            obj->count = count;
            show_list_to_char( obj->first_content, ch, true, true );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            break;
      }
      return;
   }

   if( ( pdesc = get_extra_descr( arg1, ch->in_room->first_extradesc ) ) )
   {
      send_to_char( pdesc, ch );
      return;
   }

   door = get_door( arg1 );
   if( ( pexit = find_door( ch, arg1, true ) ) )
   {
      /* Exits in wilderness are handled differently */
      if( is_in_wilderness( ch ) )
      {
         send_to_char( "You can't look at the exits while in the wilderness.\r\n", ch );
         return;
      }

      if( xIS_SET( pexit->exit_info, EX_CLOSED ) && !xIS_SET( pexit->exit_info, EX_WINDOW ) )
      {
         if( ( xIS_SET( pexit->exit_info, EX_SECRET ) || xIS_SET( pexit->exit_info, EX_DIG ) ) && door != -1 )
            send_to_char( "Nothing special there.\r\n", ch );
         else
            act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
         return;
      }
      if( xIS_SET( pexit->exit_info, EX_BASHED ) )
         act( AT_RED, "The $d has been bashed from its hinges!", ch, NULL, pexit->keyword, TO_CHAR );

      if( pexit->description && pexit->description[0] != '\0' )
         send_to_char( pexit->description, ch );
      else
         send_to_char( "Nothing special there.\r\n", ch );

      /* Ability to look into the next room - Thoric */
      if( pexit->to_room
      && ( IS_AFFECTED( ch, AFF_SCRYING )
      || xIS_SET( pexit->exit_info, EX_xLOOK )
      || get_trust( ch ) >= PERM_IMM ) )
      {
         if( !xIS_SET( pexit->exit_info, EX_xLOOK ) && get_trust( ch ) < PERM_IMM )
         {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You attempt to scry...\r\n", ch );
            /*
             * Change by Narn, Sept 96 to allow characters who don't have the
             * scry spell to benefit from objects that are affected by scry.
             */
            if( !is_npc( ch ) )
            {
               int percent = LEARNED( ch, skill_lookup( "scry" ) );
               if( !percent )
                  percent = 55;

               if( number_percent( ) > percent )
               {
                  send_to_char( "You fail.\r\n", ch );
                  return;
               }
            }
         }
         if( room_is_private( pexit->to_room ) && get_trust( ch ) < sysdata.perm_override_private )
         {
            set_char_color( AT_WHITE, ch );
            send_to_char( "That room is private buster!\r\n", ch );
            return;
         }
         original = ch->in_room;
         char_from_room( ch );
         char_to_room( ch, pexit->to_room );
         do_look( ch, (char *)"auto" );
         char_from_room( ch );
         char_to_room( ch, original );
      }
      return;
   }
   else if( door != -1 )
   {
      send_to_char( "Nothing special there.\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) )
   {
      show_char_to_char_1( victim, ch );
      return;
   }

   /* finally fixed the annoying look 2.obj desc bug -Thoric */
   number = number_argument( arg1, arg );
   for( cnt = 0, obj = ch->last_carrying; obj; obj = obj->prev_content )
   {
      if( !can_see_obj( ch, obj ) )
         continue;

      if( ( pdesc = get_extra_descr( arg, obj->first_extradesc ) ) )
      {
         if( ( cnt += obj->count ) < number )
            continue;
         send_to_char( pdesc, ch );

         if( obj->bsplatter > 0 && obj->bstain > 0 )
            act( AT_BLOOD, "You can see stains with fresh blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
         else if( obj->bsplatter > 0 )
            act( AT_BLOOD, "You can see blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
         else if( obj->bstain > 0 )
            act( AT_BLOOD, "You can see blood stains on $p.\r\n", ch, obj, NULL, TO_CHAR );

         if( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );
         return;
      }

      if( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc ) ) )
      {
         if( ( cnt += obj->count ) < number )
            continue;
         send_to_char( pdesc, ch );

         if( obj->bsplatter > 0 && obj->bstain > 0 )
            act( AT_BLOOD, "You can see stains with fresh blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
         else if( obj->bsplatter > 0 )
            act( AT_BLOOD, "You can see blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
         else if( obj->bstain > 0 )
            act( AT_BLOOD, "You can see blood stains on $p.\r\n", ch, obj, NULL, TO_CHAR );

         if( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );
         return;
      }

      if( nifty_is_name_prefix( arg, obj->name ) )
      {
         if( ( cnt += obj->count ) < number )
            continue;

         pdesc = obj->desc;
         if( !pdesc )
            pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
         if( !pdesc )
            pdesc = get_extra_descr( obj->name, obj->first_extradesc );
         if( pdesc )
            send_to_char( pdesc, ch );
         else
            send_to_char( "You see nothing special.\r\n", ch );

         if( obj->bsplatter > 0 && obj->bstain > 0 )
            act( AT_BLOOD, "You can see stains with fresh blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
         else if( obj->bsplatter > 0 )
            act( AT_BLOOD, "You can see blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
         else if( obj->bstain > 0 )
            act( AT_BLOOD, "You can see blood stains on $p.\r\n", ch, obj, NULL, TO_CHAR );

         if( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );
         return;
      }
   }

   for( obj = ch->in_room->last_content; obj; obj = obj->prev_content )
   {
      if( !can_see_obj( ch, obj ) )
         continue;

      if( ( pdesc = get_extra_descr( arg, obj->first_extradesc ) ) )
      {
         if( ( cnt += obj->count ) < number )
            continue;
         send_to_char( pdesc, ch );
         if( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );
         return;
      }

      if( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc ) ) )
      {
         if( ( cnt += obj->count ) < number )
            continue;
         send_to_char( pdesc, ch );
         if( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );
         return;
      }

      if( nifty_is_name_prefix( arg, obj->name ) )
      {
         if( ( cnt += obj->count ) < number )
            continue;
         pdesc = obj->desc;
         if( !pdesc )
            pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
         if( !pdesc )
            pdesc = get_extra_descr( obj->name, obj->first_extradesc );
         if( !pdesc )
         {
            if( obj->bsplatter > 0 && obj->bstain > 0 )
               act( AT_BLOOD, "You can see stains with fresh blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
            else if( obj->bsplatter > 0 )
               act( AT_BLOOD, "You can see blood on $p.\r\n", ch, obj, NULL, TO_CHAR );
            else if( obj->bstain > 0 )
               act( AT_BLOOD, "You can see blood stains on $p.\r\n", ch, obj, NULL, TO_CHAR );
            else
               send_to_char( "You see nothing special.\r\n", ch );
         }
         else
            send_to_char( pdesc, ch );
         if( EXA_prog_trigger )
            oprog_examine_trigger( ch, obj );
         return;
      }
   }

   send_to_char( "You don't see that here.\r\n", ch );
}

void show_condition( CHAR_DATA *ch, CHAR_DATA *victim )
{
   char buf[MSL];
   double percent;

   if( victim->hit > 0 && victim->max_hit > 0 )
      percent = get_percent( victim->hit, victim->max_hit );
   else
      percent = 0;

   if( victim != ch )
   {
      mudstrlcpy( buf, PERS( victim, ch ), sizeof( buf ) );
      mudstrlcat( buf, " ", sizeof( buf ) );
      if( percent >= 100 )
         mudstrlcat( buf, "is in perfect health.\r\n", sizeof( buf ) );
      else if( percent >= 90 )
         mudstrlcat( buf, "is slightly scratched.\r\n", sizeof( buf ) );
      else if( percent >= 80 )
         mudstrlcat( buf, "has a few bruises.\r\n", sizeof( buf ) );
      else if( percent >= 70 )
         mudstrlcat( buf, "has some cuts.\r\n", sizeof( buf ) );
      else if( percent >= 60 )
         mudstrlcat( buf, "has several wounds.\r\n", sizeof( buf ) );
      else if( percent >= 50 )
         mudstrlcat( buf, "has many nasty wounds.\r\n", sizeof( buf ) );
      else if( percent >= 40 )
         mudstrlcat( buf, "is bleeding freely.\r\n", sizeof( buf ) );
      else if( percent >= 30 )
         mudstrlcat( buf, "is covered in blood.\r\n", sizeof( buf ) );
      else if( percent >= 20 )
         mudstrlcat( buf, "is leaking guts.\r\n", sizeof( buf ) );
      else if( percent >= 10 )
         mudstrlcat( buf, "is almost dead.\r\n", sizeof( buf ) );
      else if( percent >= 1 )
         mudstrlcat( buf, "is very close to death.\r\n", sizeof( buf ) );
      else
         mudstrlcat( buf, "is DYING.\r\n", sizeof( buf ) );
   }
   else
   {
      mudstrlcpy( buf, "You ", sizeof( buf ) );
      if( percent >= 100 )
         mudstrlcat( buf, "are in perfect health.\r\n", sizeof( buf ) );
      else if( percent >= 90 )
         mudstrlcat( buf, "are slightly scratched.\r\n", sizeof( buf ) );
      else if( percent >= 80 )
         mudstrlcat( buf, "have a few bruises.\r\n", sizeof( buf ) );
      else if( percent >= 70 )
         mudstrlcat( buf, "have some cuts.\r\n", sizeof( buf ) );
      else if( percent >= 60 )
         mudstrlcat( buf, "have several wounds.\r\n", sizeof( buf ) );
      else if( percent >= 50 )
         mudstrlcat( buf, "have many nasty wounds.\r\n", sizeof( buf ) );
      else if( percent >= 40 )
         mudstrlcat( buf, "are bleeding freely.\r\n", sizeof( buf ) );
      else if( percent >= 30 )
         mudstrlcat( buf, "are covered in blood.\r\n", sizeof( buf ) );
      else if( percent >= 20 )
         mudstrlcat( buf, "are leaking guts.\r\n", sizeof( buf ) );
      else if( percent >= 10 )
         mudstrlcat( buf, "are almost dead.\r\n", sizeof( buf ) );
      else if( percent >= 1 )
         mudstrlcat( buf, "are very close to death.\r\n", sizeof( buf ) );
      else
         mudstrlcat( buf, "are DYING.\r\n", sizeof( buf ) );
   }

   buf[0] = UPPER( buf[0] );
   send_to_char( buf, ch );
}

CMDF( do_glance )
{
   CHAR_DATA *victim;
   char arg1[MIL];
   bool brief = true;

   if( !ch->desc )
      return;

   if( ch->position < POS_SLEEPING )
   {
      send_to_char( "You can't see anything but stars!\r\n", ch );
      return;
   }

   if( ch->position == POS_SLEEPING )
   {
      send_to_char( "You can't see anything, you're sleeping!\r\n", ch );
      return;
   }

   if( !check_blind( ch ) )
      return;

   set_char_color( AT_ACTION, ch );
   argument = one_argument( argument, arg1 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      if( !xIS_SET( ch->act, PLR_BRIEF ) )
         brief = false;
      xSET_BIT( ch->act, PLR_BRIEF );
      do_look( ch, (char *)"auto" );
      if( !brief )
         xREMOVE_BIT( ch->act, PLR_BRIEF );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "They're not here.\r\n", ch );
      return;
   }

   if( can_see( victim, ch ) )
   {
      act( AT_ACTION, "$n glances at you.", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n glances at $N.", ch, NULL, victim, TO_NOTVICT );
   }
   if( is_immortal( ch ) && victim != ch )
   {
      if( is_npc( victim ) )
         ch_printf( ch, "Mobile #%d '%s' ", victim->pIndexData->vnum, victim->name );
      else
         ch_printf( ch, "%s ", victim->name );
      ch_printf( ch, "is level %d", victim->level );
      if( !is_npc( victim ) )
         ch_printf( ch, " and a %s %s", dis_race_name( victim->race ), dis_main_class_name( victim ) );
      send_to_char( ".\r\n", ch );
   }
   show_condition( ch, victim );
}

CMDF( do_examine )
{
   OBJ_DATA *obj;
   char buf[MSL], arg[MIL];
   int ocheck = 0;
   short dam;

   if( !ch )
   {
      bug( "%s: null ch.", __FUNCTION__ );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Examine what?\r\n", ch );
      return;
   }

   EXA_prog_trigger = false;
   do_look( ch, arg );
   EXA_prog_trigger = true;

   /* Need some way to specify if we want to look at things in room or in inventory */
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

   /*
    * Support for checking equipment conditions and support for trigger positions by Thoric
    */
   if( ( obj = new_get_obj_here( ocheck, ch, arg ) ) )
   {
      switch( obj->item_type )
      {
         default:
            break;

         case ITEM_LIGHT:
         case ITEM_ARMOR:
            if( obj->value[1] == 0 )
               obj->value[1] = obj->value[0];
            if( obj->value[1] == 0 )
               obj->value[1] = 1;
            dam = ( short )( ( obj->value[0] * 10 ) / obj->value[1] );
            send_to_char( "As you look more closely, you notice that it is ", ch );
            if( dam >= 10 )
               send_to_char( "in superb condition.", ch );
            else if( dam == 9 )
               send_to_char( "in very good condition.", ch );
            else if( dam == 8 )
               send_to_char( "in good shape.", ch );
            else if( dam == 7 )
               send_to_char( "showing a bit of wear.", ch );
            else if( dam == 6 )
               send_to_char( "a little run down.", ch );
            else if( dam == 5 )
               send_to_char( "in need of repair.", ch );
            else if( dam == 4 )
               send_to_char( "in great need of repair.", ch );
            else if( dam == 3 )
               send_to_char( "in dire need of repair.", ch );
            else if( dam == 2 )
               send_to_char( "very badly worn.", ch );
            else if( dam == 1 )
               send_to_char( "practically worthless.", ch );
            else if( dam <= 0 )
               send_to_char( "broken.", ch );
            send_to_char( "\r\n", ch );
            break;

         case ITEM_MISSILE_WEAPON:
         case ITEM_WEAPON:
            dam = INIT_WEAPON_CONDITION - obj->value[0];
            send_to_char( "As you look more closely, you notice that it is ", ch );
            if( dam == 0 )
               send_to_char( "in superb condition.", ch );
            else if( dam == 1 )
               send_to_char( "in excellent condition.", ch );
            else if( dam == 2 )
               send_to_char( "in very good condition.", ch );
            else if( dam == 3 )
               send_to_char( "in good shape.", ch );
            else if( dam == 4 )
               send_to_char( "showing a bit of wear.", ch );
            else if( dam == 5 )
               send_to_char( "a little run down.", ch );
            else if( dam == 6 )
               send_to_char( "in need of repair.", ch );
            else if( dam == 7 )
               send_to_char( "in great need of repair.", ch );
            else if( dam == 8 )
               send_to_char( "in dire need of repair.", ch );
            else if( dam == 9 )
               send_to_char( "very badly worn.", ch );
            else if( dam == 10 )
               send_to_char( "practically worthless.", ch );
            else if( dam == 11 )
               send_to_char( "almost broken.", ch );
            else if( dam == 12 )
               send_to_char( "broken.", ch );
            send_to_char( "\r\n", ch );
            break;

         case ITEM_FISH:
         case ITEM_COOK:
            send_to_char( "As you examine it carefully you notice that it ", ch );
            dam = obj->value[2];
            if( dam >= 3 )
               send_to_char( "is burned to a crisp.", ch );
            else if( dam == 2 )
               send_to_char( "is a little over cooked.", ch );
            else if( dam == 1 )
               send_to_char( "is perfectly roasted.", ch );
            else
               send_to_char( "is raw.", ch );
            send_to_char( "\r\n", ch );
            break;

         case ITEM_FOOD:
            send_to_char( "As you examine it carefully you notice that it ", ch );
            if( obj->timer > 0 && obj->value[1] > 0 )
               dam = ( obj->timer * 10 ) / obj->value[1];
            else
               dam = 10;
            if( dam >= 10 )
               send_to_char( "is fresh.", ch );
            else if( dam == 9 )
               send_to_char( "is nearly fresh.", ch );
            else if( dam == 8 )
               send_to_char( "is perfectly fine.", ch );
            else if( dam == 7 )
               send_to_char( "looks good.", ch );
            else if( dam == 6 )
               send_to_char( "looks ok.", ch );
            else if( dam == 5 )
               send_to_char( "is a little stale.", ch );
            else if( dam == 4 )
               send_to_char( "is a bit stale.", ch );
            else if( dam == 3 )
               send_to_char( "smells slightly off.", ch );
            else if( dam == 2 )
               send_to_char( "smells quite rank.", ch );
            else if( dam == 1 )
               send_to_char( "smells revolting!", ch );
            else if( dam <= 0 )
               send_to_char( "is crawling with maggots!", ch );
            send_to_char( "\r\n", ch );
            break;

         case ITEM_SWITCH:
         case ITEM_LEVER:
         case ITEM_PULLCHAIN:
            if( IS_SET( obj->value[0], TRIG_UP ) )
               send_to_char( "You notice that it is in the up position.\r\n", ch );
            else
               send_to_char( "You notice that it is in the down position.\r\n", ch );
            break;

         case ITEM_BUTTON:
            if( IS_SET( obj->value[0], TRIG_UP ) )
               send_to_char( "You notice that it is depressed.\r\n", ch );
            else
               send_to_char( "You notice that it isn't depressed.\r\n", ch );
            break;

         case ITEM_CORPSE_PC:
         case ITEM_CORPSE_NPC:
         {
            short timerfrac = obj->timer;
            if( obj->item_type == ITEM_CORPSE_PC )
               timerfrac = ( int )obj->timer / 8 + 1;

            switch( timerfrac )
            {
               default:
                  send_to_char( "This corpse has recently been slain.\r\n", ch );
                  break;

               case 4:
                  send_to_char( "This corpse was slain a little while ago.\r\n", ch );
                  break;

               case 3:
                  send_to_char( "A foul smell rises from the corpse, and it is covered in flies.\r\n", ch );
                  break;

               case 2:
                  send_to_char( "A writhing mass of maggots and decay, you can barely go near this corpse.\r\n", ch );
                  break;

               case 1:
               case 0:
                  send_to_char( "Little more than bones, there isn't much left of this corpse.\r\n", ch );
                  break;
            }
         }

         case ITEM_CONTAINER:
            if( is_obj_stat( obj, ITEM_COVERING ) )
               break;

         case ITEM_DRINK_CON:
         case ITEM_QUIVER:
            send_to_char( "When you look inside, you see:\r\n", ch );

         case ITEM_KEYRING:
            EXA_prog_trigger = false;
            snprintf( buf, sizeof( buf ), "in %s", arg );
            do_look( ch, buf );
            EXA_prog_trigger = true;
            break;
      }
      if( is_obj_stat( obj, ITEM_COVERING ) )
      {
         EXA_prog_trigger = false;
         snprintf( buf, sizeof( buf ), "under %s", arg );
         do_look( ch, buf );
         EXA_prog_trigger = true;
      }

      oprog_examine_trigger( ch, obj );
      if( char_died( ch ) )
         return;

      check_for_trap( ch, obj, TRAP_EXAMINE );
   }
}

CMDF( do_exits )
{
   EXIT_DATA *pexit;
   bool found, fAuto, door, window;

   set_char_color( AT_EXITS, ch );

   if( !check_blind( ch ) )
      return;

   if( !ch->in_room )
   {
      send_to_char( "You're in a NULL room?\r\n", ch );
      bug( "%s: %s is in a NULL room!", __FUNCTION__, ch->name );
      return;
   }

   if( !is_npc( ch ) && !xIS_SET( ch->act, PLR_HOLYLIGHT )
   && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && room_is_dark( ch->in_room ) )
   {
      set_char_color( AT_DGRAY, ch );
      send_to_char( "It is pitch black ... \r\n", ch );
      return;
   }

   fAuto = !str_cmp( argument, "auto" );
   send_to_char( fAuto ? "Exits:" : "Obvious exits:\r\n", ch );
   found = false;
   for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( !pexit->to_room )
         continue;
      if( xIS_SET( pexit->exit_info, EX_CLOSED ) 
      && ( xIS_SET( pexit->exit_info, EX_HIDDEN ) || xIS_SET( pexit->exit_info, EX_SECRET ) || xIS_SET( pexit->exit_info, EX_DIG ) ) )
         continue;

      door = window = false;
      if( xIS_SET( pexit->exit_info, EX_CLOSED ) )
         door = true;
      if( xIS_SET( pexit->exit_info, EX_WINDOW ) )
         window = true;
      found = true;
      ch_printf( ch, "%s%s", fAuto ? " " : "", door ? "(" : window ? "[" : "" );
      if( pexit->keyword && pexit->keyword[0] != '\0' )
      {
         ch_printf( ch, "%s", capitalize( pexit->keyword ) );
         if( !door && !window )
            ch_printf( ch, "->%s", capitalize( dir_name[pexit->vdir] ) );
      }
      else
         ch_printf( ch, "%s", capitalize( dir_name[pexit->vdir] ) );
      ch_printf( ch, "%s", door ? ")" : window ? "]" : "" );
      if( !fAuto )
         ch_printf( ch, " - %s\r\n", room_is_dark( pexit->to_room ) ? "Too dark to tell" : pexit->to_room->name );
   }
   if( !found )
      ch_printf( ch, "%sNone.\r\n", fAuto ? " " : "" );
   else if( fAuto )
      send_to_char( ".\r\n", ch );
}

const char *show_weather( AREA_DATA *area )
{
   const char *combo, *single;
   static char buf[MSL];
   int temp, precip, wind;

   temp = ( area->weather->temp + ( 3 * weath_unit ) - 1 ) / weath_unit;
   precip = ( area->weather->precip + ( 3 * weath_unit ) - 1 ) / weath_unit;
   wind = ( area->weather->wind + ( 3 * weath_unit ) - 1 ) / weath_unit;

   if( temp < 0 || temp > 6 )
   {
      bug( "%s: temp is %d when it should be 0 - 6.", __FUNCTION__, temp );
      return "(Bad Temp)";
   }
   if( precip < 0 || precip > 6 )
   {
      bug( "%s: precip is %d when it should be 0 - 6.", __FUNCTION__, precip );
      return "(Bad Precip)";
   }
   if( wind < 0 || wind > 6 )
   {
      bug( "%s: wind is %d when it should be 0 - 6.", __FUNCTION__, wind );
      return "(Bad Wind)";
   }

   if( precip >= 3 )
   {
      combo = preciptemp_msg[precip][temp];
      single = wind_msg[wind];
   }
   else
   {
      combo = windtemp_msg[wind][temp];
      single = precip_msg[precip];
   }

   snprintf( buf, sizeof( buf ), "%s and %s.", combo, single );
   return buf;
}

CMDF( do_weather )
{
   if( !is_outside( ch ) )
   {
      send_to_char( "You can't see the sky from here.\r\n", ch );
      return;
   }

   set_char_color( AT_BLUE, ch );
   ch_printf( ch, "%s\r\n", show_weather( ch->in_room->area ) );
}

/* 
 * New do_who with WHO REQUEST, clan, race and homepage support.  -Thoric
 *
 * Latest version of do_who eliminates redundant code by using linked lists.
 * Shows imms separately, indicates guest and retired immortals.
 * Narn, Oct/96
 *
 * Who group by Altrag, Feb 28/97
 */
typedef struct whogr_s WHOGR_S;
struct whogr_s
{
   WHOGR_S *next;
   WHOGR_S *follower;
   WHOGR_S *l_follow;
   DESCRIPTOR_DATA *d;
   int indent;
} *first_whogr, *last_whogr;

typedef struct who_data WHO_DATA;
struct who_data
{
   WHO_DATA *prev, *next;
   char *text;
   int type;
};

#define WT_MORTAL   0
#define WT_DEADLY   1
#define WT_IMM      2
#define WT_GROUPED  3
#define WT_GROUPWHO 4

WHOGR_S *find_whogr( DESCRIPTOR_DATA *d, WHOGR_S *first )
{
   WHOGR_S *whogr, *whogr_t;

   for( whogr = first; whogr; whogr = whogr->next )
   {
      if( whogr->d == d )
         return whogr;
      else if( whogr->follower && ( whogr_t = find_whogr( d, whogr->follower ) ) )
         return whogr_t;
   }
   return NULL;
}

void indent_whogr( CHAR_DATA *looker, WHOGR_S *whogr, int ilev )
{
   for( ; whogr; whogr = whogr->next )
   {
      if( whogr->follower )
      {
         int nlev = ilev;
         CHAR_DATA *wch = whogr->d->character;

         if( can_see( looker, wch ) && !is_immortal( wch ) )
            nlev += 3;
         indent_whogr( looker, whogr->follower, nlev );
      }
      whogr->indent = ilev;
   }
}

void delete_whogr( void )
{
   WHOGR_S *whogr;

   while( ( whogr = first_whogr ) )
   {
      first_whogr = whogr->next;
      DISPOSE( whogr );
   }

   first_whogr = last_whogr = NULL;
}

/* This a great big mess to backwards-structure the ->leader character fields */
void create_whogr( CHAR_DATA *looker )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *wch;
   WHOGR_S *whogr, *whogr_t;
   int dc = 0, wc = 0;

   delete_whogr( );

   /* Link in the ones without leaders first */
   for( d = last_descriptor; d; d = d->prev )
   {
      if( d->connected != CON_PLAYING && d->connected != CON_EDITING )
         continue;
      ++dc;
      wch = d->character;
      if( !wch->leader || wch->leader == wch || !wch->leader->desc
      || is_npc( wch->leader ) || is_immortal( wch ) || is_immortal( wch->leader ) )
      {
         CREATE( whogr, WHOGR_S, 1 );
         if( !last_whogr )
            first_whogr = last_whogr = whogr;
         else
         {
            last_whogr->next = whogr;
            last_whogr = whogr;
         }
         whogr->next = NULL;
         whogr->follower = whogr->l_follow = NULL;
         whogr->d = d;
         whogr->indent = 0;
         ++wc;
      }
   }
   /* Now for those who have leaders.. */
   while( wc < dc )
   {
      for( d = last_descriptor; d; d = d->prev )
      {
         if( d->connected != CON_PLAYING && d->connected != CON_EDITING )
            continue;
         if( find_whogr( d, first_whogr ) )
            continue;
         wch = d->character;
         if( wch->leader && wch->leader != wch && wch->leader->desc
         && !is_npc( wch->leader ) && !is_immortal( wch )
         && !is_immortal( wch->leader ) && ( whogr_t = find_whogr( wch->leader->desc, first_whogr ) ) )
         {
            CREATE( whogr, WHOGR_S, 1 );
            if( !whogr_t->l_follow )
               whogr_t->follower = whogr_t->l_follow = whogr;
            else
            {
               whogr_t->l_follow->next = whogr;
               whogr_t->l_follow = whogr;
            }
            whogr->next = NULL;
            whogr->follower = whogr->l_follow = NULL;
            whogr->d = d;
            whogr->indent = 0;
            ++wc;
         }
      }
   }

   /* Set up indentation levels */
   indent_whogr( looker, first_whogr, 0 );

   /* And now to linear link them.. */
   for( whogr_t = NULL, whogr = first_whogr; whogr; )
   {
      if( whogr->l_follow )
      {
         whogr->l_follow->next = whogr;
         whogr->l_follow = NULL;
         if( whogr_t )
            whogr_t->next = whogr = whogr->follower;
         else
            first_whogr = whogr = whogr->follower;
      }
      else
      {
         whogr_t = whogr;
         whogr = whogr->next;
      }
   }
}

CMDF( do_who )
{
   CLAN_DATA *pClan = NULL;
   COUNCIL_DATA *pCouncil = NULL;
   DEITY_DATA *pDeity = NULL;
   DESCRIPTOR_DATA *d;
   WHO_DATA *cur_who = NULL, *next_who = NULL, *first_mortal = NULL, *first_imm = NULL;
   WHO_DATA *first_deadly = NULL, *first_grouped = NULL, *first_groupwho = NULL;
   WHOGR_S *whogr, *whogr_p;
   char buf[MSL], clan_name[MIL], nation_name[MIL], council_name[MIL], invis_str[MIL], char_name[MIL];
   char class_text[MIL];
   const char *w1, *w2;
   int iClass, iRace, iLevelLower = 0, iLevelUpper = MAX_LEVEL, nNumber, nMatch;
   bool rgfClass[MAX_CLASS], rgfRace[MAX_RACE], fClassRestrict = false, fRaceRestrict = false;
   bool fImmortalOnly = false, fLeader = false, fPkill = false, fGroup = false;
   bool fShowHomepage = false, fClanMatch = false, fCouncilMatch = false, fDeityMatch = false;
   bool fNationMatch = false;

   if( !ch )
      return;

   w1 = color_str( AT_WHO, ch );
   w2 = color_str( AT_WHO2, ch );

   /* Set default arguments. */
   for( iClass = 0; iClass < MAX_CLASS; iClass++ )
      rgfClass[iClass] = false;
   for( iRace = 0; iRace < MAX_RACE; iRace++ )
      rgfRace[iRace] = false;

   /* Parse arguments. */
   nNumber = 0;
   while( argument && argument[0] != '\0' )
   {
      char arg[MSL];

      argument = one_argument( argument, arg );
      if( arg[0] == '\0' )
         break;

      if( is_number( arg ) )
      {
         switch( ++nNumber )
         {
            case 1:
               iLevelLower = atoi( arg );
               break;

            case 2:
               iLevelUpper = atoi( arg );
               break;

            default:
               send_to_char( "Only two level numbers allowed.\r\n", ch );
               return;
         }
      }
      else
      {
         if( strlen( arg ) < 3 )
         {
            send_to_char( "Arguments must be longer than that.\r\n", ch );
            return;
         }

         /* Look for classes to turn on. */
         if( !str_cmp( arg, "deadly" ) || !str_cmp( arg, "pkill" ) )
            fPkill = true;
         else if( !str_cmp( arg, "imm" ) || !str_cmp( arg, "gods" ) )
            fImmortalOnly = true;
         else if( !str_cmp( arg, "leader" ) )
            fLeader = true;
         else if( !str_cmp( arg, "www" ) )
            fShowHomepage = true;
         else if( !str_cmp( arg, "group" ) && ch )
            fGroup = true;
         else if( ( pClan = get_clan( arg ) ) && pClan->clan_type == CLAN_PLAIN )
            fClanMatch = true;
         else if( ( pClan = get_clan( arg ) ) && pClan->clan_type == CLAN_NATION )
            fNationMatch = true;
         else if( ( pCouncil = get_council( arg ) ) )
            fCouncilMatch = true;
         else if( ( pDeity = get_deity( arg ) ) )
            fDeityMatch = true;
         else
         {
            for( iClass = 0; iClass < MAX_CLASS; iClass++ )
            {
               if( class_table[iClass] && !str_cmp( arg, class_table[iClass]->name ) )
               {
                  rgfClass[iClass] = true;
                  break;
               }
            }
            if( iClass != MAX_CLASS )
               fClassRestrict = true;

            for( iRace = 0; iRace < MAX_RACE; iRace++ )
            {
               if( race_table[iRace] && !str_cmp( arg, race_table[iRace]->name ) )
               {
                  rgfRace[iRace] = true;
                  break;
               }
            }
            if( iRace != MAX_RACE )
               fRaceRestrict = true;

            if( iClass == MAX_CLASS && iRace == MAX_RACE && !fClanMatch
            && !fCouncilMatch && !fDeityMatch && !fNationMatch )
            {
               send_to_char( "That's not a class, race, clan, nation, council or deity.\r\n", ch );
               return;
            }
         }
      }
   }

   /* Now find matching chars. */
   nMatch = 0;
   buf[0] = '\0';
   set_pager_color( AT_WHO, ch );

   /* start from last to first to get it in the proper order */
   if( fGroup )
   {
      create_whogr( ch );
      whogr = first_whogr;
      d = whogr->d;
   }
   else
   {
      whogr = NULL;
      d = last_descriptor;
   }

   whogr_p = NULL;
   for( ; d; whogr_p = whogr, whogr = ( fGroup ? whogr->next : NULL ), d = ( fGroup ? ( whogr ? whogr->d : NULL ) : d->prev ) )
   {
      CHAR_DATA *wch;
      char const *Class;

      if( ( d->connected != CON_PLAYING && d->connected != CON_EDITING ) || !can_see( ch, d->character ) )
         continue;
      wch = d->character;

      if( wch->level < iLevelLower
      || wch->level > iLevelUpper
      || ( fPkill && !can_pkill( wch ) )
      || ( fImmortalOnly && get_trust( wch ) < PERM_IMM )
      || ( fRaceRestrict && !rgfRace[wch->race] )
      || ( fClanMatch && ( pClan != wch->pcdata->clan ) )
      || ( fNationMatch && ( pClan != wch->pcdata->nation ) )
      || ( fCouncilMatch && ( pCouncil != wch->pcdata->council ) )
      || ( fDeityMatch && ( pDeity != wch->pcdata->deity ) ) )
         continue;

      if( fClassRestrict )
      {
         MCLASS_DATA *mclass;
         bool clcontinue = true;

         for( mclass = wch->pcdata->first_mclass; mclass; mclass = mclass->next )
            if( rgfClass[mclass->wclass] )
               clcontinue = false;
         if( clcontinue )
            continue;
      }

      if( fLeader
      && ( !wch->pcdata->council || ( str_cmp( wch->pcdata->council->head2, wch->name ) && str_cmp( wch->pcdata->council->head, wch->name ) ) )
      && ( !wch->pcdata->nation || ( str_cmp( wch->pcdata->nation->leader, wch->name )
      && str_cmp( wch->pcdata->nation->number1, wch->name ) && str_cmp( wch->pcdata->nation->number2, wch->name ) ) )
      && ( !wch->pcdata->clan || ( str_cmp( wch->pcdata->clan->leader, wch->name )
      && str_cmp( wch->pcdata->clan->number1, wch->name ) && str_cmp( wch->pcdata->clan->number2, wch->name ) ) ) )
         continue;

      if( fGroup && !wch->leader && !xIS_SET( wch->pcdata->flags, PCFLAG_GROUPWHO ) && ( !whogr_p || !whogr_p->indent ) )
         continue;

      nMatch++;

      mudstrlcpy( char_name, wch->name, sizeof( char_name ) );

      /* Base Info For Class Info */
      snprintf( class_text, sizeof( class_text ), "%2d %s", wch->level, dis_main_class_name( wch ) );
      Class = class_text;

      /* Some extra things that override base info */
      if( wch->level == MAX_LEVEL )
         Class = "Avatar";
      if( get_trust( wch ) == PERM_IMP )
         Class = "Implementor";
      else if( get_trust( wch ) == PERM_HEAD )
         Class = "Head Immortal";
      else if( get_trust( wch ) == PERM_LEADER )
         Class = "Leader";
      else if( get_trust( wch ) == PERM_BUILDER )
         Class = "Builder";
      else if( get_trust( wch ) == PERM_IMM )
         Class = "Immortal";

      /* These override all the base and extra for class info */
      if( is_retired( wch ) )
         Class = "Retired";
      else if( is_guest( wch ) )
         Class = "Guest";
      else if( wch->pcdata->clan && !str_cmp( wch->name, wch->pcdata->clan->leader ) && wch->pcdata->clan->leadrank )
         Class = wch->pcdata->clan->leadrank;
      else if( wch->pcdata->clan && !str_cmp( wch->name, wch->pcdata->clan->number1 ) && wch->pcdata->clan->onerank )
         Class = wch->pcdata->clan->onerank;
      else if( wch->pcdata->clan && !str_cmp( wch->name, wch->pcdata->clan->number2 ) && wch->pcdata->clan->tworank )
         Class = wch->pcdata->clan->tworank;
      else if( wch->pcdata->rank )
         Class = wch->pcdata->rank;

      if( wch->pcdata->clan )
      {
         CLAN_DATA *pclan = wch->pcdata->clan;

         mudstrlcpy( clan_name, w1, sizeof( clan_name ) );
         mudstrlcat( clan_name, " <", sizeof( clan_name ) );
         mudstrlcat( clan_name, w2, sizeof( clan_name ) );
         if( !str_cmp( wch->name, pclan->leader ) )
            mudstrlcat( clan_name, "Leader of ", sizeof( clan_name ) );
         else if( !str_cmp( wch->name, pclan->number1 ) )
            mudstrlcat( clan_name, "Number One ", sizeof( clan_name ) );
         else if( !str_cmp( wch->name, pclan->number2 ) )
            mudstrlcat( clan_name, "Number Two ", sizeof( clan_name ) );
         mudstrlcat( clan_name, pclan->name, sizeof( clan_name ) );
         mudstrlcat( clan_name, w1, sizeof( clan_name ) );
         mudstrlcat( clan_name, ">", sizeof( clan_name ) );
      }
      else
         clan_name[0] = '\0';

      if( wch->pcdata->nation )
      {
         CLAN_DATA *nation = wch->pcdata->nation;

         mudstrlcpy( nation_name, w1, sizeof( nation_name ) );
         mudstrlcat( nation_name, " [", sizeof( nation_name ) );
         mudstrlcat( nation_name, w2, sizeof( nation_name ) );
         mudstrlcat( nation_name, nation->name, sizeof( nation_name ) );
         mudstrlcat( nation_name, w1, sizeof( nation_name ) );
         mudstrlcat( nation_name, "]", sizeof( nation_name ) );
      }
      else
         nation_name[0] = '\0';

      if( wch->pcdata->council && wch->pcdata->council->name )
      {
         mudstrlcpy( council_name, w1, sizeof( council_name ) );
         mudstrlcat( council_name, " [", sizeof( council_name ) );
         mudstrlcat( council_name, w2, sizeof( council_name ) );
         if( !wch->pcdata->council->head2 )
         {
            if( !str_cmp( wch->name, wch->pcdata->council->head ) )
               mudstrlcat( council_name, "Head of ", sizeof( council_name ) );
         }
         else
         {
            if( !str_cmp( wch->name, wch->pcdata->council->head ) || !str_cmp( wch->name, wch->pcdata->council->head2 ) )
               mudstrlcat( council_name, "Co-Head of ", sizeof( council_name ) );
         }
         mudstrlcat( council_name, wch->pcdata->council->name, sizeof( council_name ) );
         mudstrlcat( council_name, w1, sizeof( council_name ) );
         mudstrlcat( council_name, "]", sizeof( council_name ) );
      }
      else
         council_name[0] = '\0';

      if( xIS_SET( wch->act, PLR_WIZINVIS ) )
         snprintf( invis_str, sizeof( invis_str ), "(%d) ", wch->pcdata->wizinvis );
      else
         invis_str[0] = '\0';

      snprintf( buf, sizeof( buf ), "%*s%-15s %s%s%s%s%s%s.&D%s%s%s\r\n",
         ( fGroup ? whogr->indent : 0 ), "",
         Class,
         invis_str,
         ( wch->desc && wch->desc->connected ) ? "[WRITING] " : "",
         xIS_SET( wch->act, PLR_AFK ) ? "[AFK] " : "",
         wch->sex == SEX_FEMALE ? color_str( AT_FEMALE, ch ) : wch->sex == SEX_MALE ? color_str( AT_MALE, ch ) : "",
         char_name, wch->pcdata->title, clan_name, nation_name, council_name );

      /*
       * This is where the old code would display the found player to the ch.
       * What we do instead is put the found data into a linked list
       */

      /* First make the structure. */
      CREATE( cur_who, WHO_DATA, 1 );
      cur_who->text = STRALLOC( buf );
      if( is_immortal( wch ) )
         cur_who->type = WT_IMM;
      else if( fGroup )
      {
         if( wch->leader || ( whogr_p && whogr_p->indent ) )
            cur_who->type = WT_GROUPED;
         else
            cur_who->type = WT_GROUPWHO;
      }
      else if( can_pkill( wch ) )
         cur_who->type = WT_DEADLY;
      else
         cur_who->type = WT_MORTAL;

      /* Then put it into the appropriate list. */
      switch( cur_who->type )
      {
         case WT_MORTAL:
            cur_who->next = first_mortal;
            first_mortal = cur_who;
            break;

         case WT_DEADLY:
            cur_who->next = first_deadly;
            first_deadly = cur_who;
            break;

         case WT_GROUPED:
            cur_who->next = first_grouped;
            first_grouped = cur_who;
            break;

         case WT_GROUPWHO:
            cur_who->next = first_groupwho;
            first_groupwho = cur_who;
            break;

         case WT_IMM:
            cur_who->next = first_imm;
            first_imm = cur_who;
            break;
      }
   }

   /*
    * Ok, now we have three separate linked lists and what remains is to 
    * display the information and clean up.
    * Two extras now for grouped and groupwho (wanting group). -- Alty
    */

   if( first_mortal )
      pager_printf( ch, "\r\n%s~`~.~`~.~`~.~`~.~`~.~`~.~`~[ %sPEACEFUL CHARACTERS %s]~`~.~`~.~`~.~`~.~`~.~`~.~`~\r\n\r\n", w1, w2, w1 );

   for( cur_who = first_mortal; cur_who; cur_who = next_who )
   {
      send_to_pager( cur_who->text, ch );
      next_who = cur_who->next;
      STRFREE( cur_who->text );
      DISPOSE( cur_who );
   }

   if( first_deadly )
      pager_printf( ch, "\r\n%s~`~.~`~.~`~.~`~.~`~.~`~.~`~[  %sDEADLY CHARACTERS  %s]~`~.~`~.~`~.~`~.~`~.~`~.~`~\r\n\r\n", w1, w2, w1 );

   for( cur_who = first_deadly; cur_who; cur_who = next_who )
   {
      send_to_pager( cur_who->text, ch );
      next_who = cur_who->next;
      STRFREE( cur_who->text );
      DISPOSE( cur_who );
   }

   if( first_grouped )
      pager_printf( ch, "\r\n%s~`~.~`~.~`~.~`~.~`~.~`~.~`~[  %sGROUPED CHARACTERS %s]~`~.~`~.~`~.~`~.~`~.~`~.~`~\r\n\r\n", w1, w2, w1 );

   for( cur_who = first_grouped; cur_who; cur_who = next_who )
   {
      send_to_pager( cur_who->text, ch );
      next_who = cur_who->next;
      STRFREE( cur_who->text );
      DISPOSE( cur_who );
   }

   if( first_groupwho )
      pager_printf( ch, "\r\n%s~`~.~`~.~`~.~`~.~`~.~`~.~`~[     %sWANTING GROUP   %s]~`~.~`~.~`~.~`~.~`~.~`~.~`~\r\n\r\n", w1, w2, w1 );

   for( cur_who = first_groupwho; cur_who; cur_who = next_who )
   {
      send_to_pager( cur_who->text, ch );
      next_who = cur_who->next;
      STRFREE( cur_who->text );
      DISPOSE( cur_who );
   }

   if( first_imm )
      pager_printf( ch, "\r\n%s~`~.~`~.~`~.~`~.~`~.~`~.~`~[     %sIMMORTALS       %s]~`~.~`~.~`~.~`~.~`~.~`~.~`~\r\n\r\n", w1, w2, w1 );

   for( cur_who = first_imm; cur_who; cur_who = next_who )
   {
      send_to_pager( cur_who->text, ch );
      next_who = cur_who->next;
      STRFREE( cur_who->text );
      DISPOSE( cur_who );
   }

   delete_whogr( );

   pager_printf( ch, "\r\n%sYou see [%s%d%s] player%s online.\r\n", w1, w2, nMatch, w1, nMatch != 1 ? "s" : "");
   pager_printf( ch, "%sThere have been a max of [%s%d%s] players on at one time since Reboot.\r\n", w1, w2, sysdata.maxplayers, w1);
   {
      int iplogins = get_ip_logins( );

      pager_printf( ch, "%sThere have been players on from [%s%d%s] different ip%s since Reboot.\r\n", w1, w2, iplogins, w1, iplogins == 1 ? "" : "s" );
   }
   pager_printf( ch, "%sMud has been up since %s%s\r\n", w1, w2, str_boot_time );
}

#undef WT_MORTAL
#undef WT_DEADLY
#undef WT_IMM
#undef WT_GROUPED
#undef WT_GROUPWHO

CMDF( do_compare )
{
   OBJ_DATA *obj1, *obj2;
   char arg1[MIL], arg2[MIL];
   const char *msg;
   int value1, value2;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Compare what to what?\r\n", ch );
      return;
   }

   if( !( obj1 = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You don't have that item.\r\n", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' )
   {
      for( obj2 = ch->first_carrying; obj2; obj2 = obj2->next_content )
      {
         if( obj2->wear_loc != WEAR_NONE
         && can_see_obj( ch, obj2 )
         && obj1->item_type == obj2->item_type
         && xSAME_BITS( obj1->wear_flags, obj2->wear_flags ) )
            break;
      }

      if( !obj2 )
      {
         send_to_char( "You aren't wearing anything comparable.\r\n", ch );
         return;
      }
   }
   else
   {
      if( !( obj2 = get_obj_carry( ch, arg2 ) ) )
      {
         send_to_char( "You don't have that item.\r\n", ch );
         return;
      }
   }

   msg = NULL;
   value1 = 0;
   value2 = 0;

   if( obj1 == obj2 )
      msg = "You compare $p to itself. It looks about the same.";
   else if( obj1->item_type != obj2->item_type )
      msg = "You can't compare $p and $P.";
   else
   {
      switch( obj1->item_type )
      {
         default:
            msg = "You can't compare $p and $P.";
            break;

         case ITEM_ARMOR:
            value1 = obj1->value[0];
            value2 = obj2->value[0];
            break;

         case ITEM_MISSILE_WEAPON:
         case ITEM_WEAPON:
            value1 = obj1->value[1] + obj1->value[2];
            value2 = obj2->value[1] + obj2->value[2];
            break;
      }
   }

   if( !msg )
   {
      if( value1 == value2 )
         msg = "$p and $P look about the same.";
      else if( value1 > value2 )
         msg = "$p looks better than $P.";
      else
         msg = "$p looks worse than $P.";
   }

   act( AT_PLAIN, msg, ch, obj1, obj2, TO_CHAR );
}

CMDF( do_where )
{
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   bool found = false;

   set_pager_color( AT_PERSON, ch );
   if( !argument || argument[0] == '\0' )
   {
      pager_printf( ch, "\r\nPlayers near you in %s:\r\n", ch->in_room->area->name );
      for( d = first_descriptor; d; d = d->next )
      {
         if( !d->character )
            continue;
         if( ( victim = d->character ) && !is_npc( victim )
         && victim != ch
         && victim->in_room
         && victim->in_room->area == ch->in_room->area
         && can_see( ch, victim ) && ( get_trust( ch ) >= get_trust( victim )
         || !xIS_SET( victim->pcdata->flags, PCFLAG_DND ) ) )
         {
            found = true;
            pager_printf( ch, "&P%-13s  ", victim->name );
            if( is_immortal( victim ) )
               send_to_pager( "&P(&WImmortal&P)          ", ch );
            else if( can_pkill( victim ) && victim->pcdata->clan && victim->pcdata->clan->clan_type == CLAN_PLAIN )
               pager_printf( ch, "%-18s  ", victim->pcdata->clan->badge );
            else if( can_pkill( victim ) )
               send_to_pager( "(&wUnclanned&P)         ", ch );
            else
               send_to_pager( "                    ", ch );
            pager_printf( ch, "&P%s\r\n", victim->in_room->name );
         }
      }
      if( !found )
         send_to_char( "None\r\n", ch );
   }
   else
   {
      for( victim = first_char; victim; victim = victim->next )
      {
         if( victim != ch
         && victim->in_room
         && victim->in_room->area == ch->in_room->area
         && can_see( ch, victim )
         && is_name( argument, victim->name ) )
         {
            found = true;
            pager_printf( ch, "%-28s %s\r\n", PERS( victim, ch ), victim->in_room->name );
            break;
         }
      }
      if( !found )
         act( AT_PLAIN, "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
   }
}

CMDF( do_consider )
{
   CHAR_DATA *victim;
   const char *msg;
   int diff;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Consider killing whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They're not here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You're pretty sure you could take yourself in a fight.\r\n", ch );
      return;
   }

   diff = ch->level - victim->level;

   msg = "";
   if( diff >= 10 )
      msg = "You're alot more experienced than $N.";
   else if( diff >= 5 )
      msg = "You're more experienced than $N.";
   else if( diff >= 1 )
      msg = "You're a little more experienced than $N.";
   else if( diff == 0 )
      msg = "You're as experienced as $N.";
   else if( diff <= -10 )
      msg = "$N is alot more experienced than you.";
   else if( diff <= -5 )
      msg = "$N is more experienced than you.";
   else if( diff <= -1 )
      msg = "$N is a little more experienced than you.";
   act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

   diff = ( int )get_hitroll( ch ) - get_hitroll( victim );
   if( diff >= 10 )
      msg = "You're able to hit alot more often than $N.";
   else if( diff >= 5 )
      msg = "You're able to hit more often than $N.";
   else if( diff >= 1 )
      msg = "You're able to hit a little more often than $N.";
   else if( diff == 0 )
      msg = "You're able to hit as often as $N.";
   else if( diff <= -10 )
      msg = "$N is able to hit alot more often than you.";
   else if( diff <= -5 )
      msg = "$N is able to hit more often than you.";
   else if( diff <= -1 )
      msg = "$N is able to hit a little more often than you.";
   act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

   diff = ( int )get_damroll( ch ) - get_damroll( victim );
   if( diff >= 10 )
      msg = "You're able to do alot more damage than $N.";
   else if( diff >= 5 )
      msg = "You're able to do more damage than $N.";
   else if( diff >= 1 )
      msg = "You're able to do a little more damage than $N.";
   else if( diff == 0 )
      msg = "You're able to do as much damage as $N.";
   else if( diff <= -10 )
      msg = "$N is able to do alot more damage than you.";
   else if( diff <= -5 )
      msg = "$N is able to do more damage than you.";
   else if( diff <= -1 )
      msg = "$N is able to do a little more damage than you.";
   act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

   diff = ( int )get_ac( ch ) - get_ac( victim );
   if( diff >= 500 )
      msg = "You're alot better armored than $N.";
   else if( diff >= 250 )
      msg = "You're better armored than $N.";
   else if( diff >= 1 )
      msg = "You're a little better armored than $N.";
   else if( diff == 0 )
      msg = "You're as armored as $N.";
   else if( diff <= -500 )
      msg = "$N is alot better armored than you.";
   else if( diff <= -250 )
      msg = "$N is better armored than you.";
   else if( diff <= -1 )
      msg = "$N is a little better armored than you.";
   act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

   diff = ( int )( ch->max_hit - victim->max_hit );
   if( diff >= 500 )
      msg = "You're able to take alot more damage than $N.";
   else if( diff >= 250 )
      msg = "You're able to take more damage than $N.";
   else if( diff >= 1 )
      msg = "You're able to take a little more damage than $N.";
   else if( diff == 0 )
      msg = "You're able to take as much damage as $N.";
   else if( diff <= -500 )
      msg = "$N is able to take alot more damage than you.";
   else if( diff <= -250 )
      msg = "$N is able to take more damage than you.";
   else if( diff <= -1 )
      msg = "$N is able to take a little more damage than you.";
   act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );
}

CMDF( do_wimpy )
{
   int wimpy;

   if( !argument || argument[0] == '\0' )
      wimpy = ( int )ch->max_hit / 5;
   else if( !str_cmp( argument, "max" ) )
      wimpy = ( int )( ch->max_hit / 2.25 );
   else
      wimpy = atoi( argument );

   set_char_color( AT_YELLOW, ch );
   if( wimpy < 0 || wimpy > ( ch->max_hit / 2.25 ) )
   {
      ch_printf( ch, "Wimpy range for you is 0 to %d\r\n", ( int )( ch->max_hit / 2.25 ) );
      return;
   }
   ch->wimpy = wimpy;
   ch_printf( ch, "Wimpy set to %d hit points.\r\n", wimpy );
}

CMDF( do_password )
{
   char arg1[MIL], arg2[MIL];
   char *pArg, *pwdnew;
   char cEnd;

   if( is_npc( ch ) )
      return;

   /* Can't use one_argument here because it smashes case. So we just steal all its code.  Bleagh. */
   pArg = arg1;
   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }
      *pArg++ = *argument++;
   }
   *pArg = '\0';

   pArg = arg2;
   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }
      *pArg++ = *argument++;
   }
   *pArg = '\0';

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Usage: password <new> <again>.\r\n", ch );
      return;
   }

   /* This should stop all the mistyped password problems --Shaddai */
   if( strcmp( arg1, arg2 ) )
   {
      send_to_char( "Passwords don't match try again.\r\n", ch );
      return;
   }
   if( strlen( arg2 ) < 5 )
   {
      send_to_char( "New password must be at least five characters long.\r\n", ch );
      return;
   }

   if( arg1[0] == '!' || arg2[0] == '!' )
   {
      send_to_char( "New password can't begin with the '!' character.", ch );
      return;
   }

   pwdnew = sha256_crypt( arg2 );   /* SHA-256 Encryption */

   STRSET( ch->pcdata->pwd, pwdnew );
   if( xIS_SET( sysdata.save_flags, SV_PASSCHG ) )
      save_char_obj( ch );
   if( ch->desc && ch->desc->host[0] != '\0' )
      log_printf( "%s changing password from site %s\n", ch->name, ch->desc->host );
   else
      log_printf( "%s changing thier password with no descriptor!", ch->name );
   send_to_char( "Ok.\r\n", ch );
}

/* display WIZLIST file - Thoric */
CMDF( do_wizlist )
{
   set_pager_color( AT_IMMORT, ch );
   show_file( ch, WIZLIST_FILE );
}

CMDF( do_config )
{
   char arg[MIL];

   if( !ch || is_npc( ch ) )
      return;

   one_argument( argument, arg );
   set_char_color( AT_GREEN, ch );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "\r\n&[dgreen]Configurations ", ch );
      send_to_char( "&[green](use 'config <keyword>' to toggle, see 'help config')\r\n\r\n", ch );
      send_to_char( "&[dgreen]Display:   ", ch );

      set_char_color( AT_GRAY, ch );

      ch_printf( ch, "Pager(%c)      Gag(%c)        Brief(%c)         Combine(%c)\r\n",
         xIS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) ? 'X' : ' ',
         xIS_SET( ch->pcdata->flags, PCFLAG_GAG )     ? 'X' : ' ',
         xIS_SET( ch->act, PLR_BRIEF )                ? 'X' : ' ',
         xIS_SET( ch->act, PLR_COMBINE )              ? 'X' : ' ' );

      ch_printf( ch, "           Blank(%c)      Prompt(%c)     Ansi(%c)          KeepAlive(%c)\r\n",
         xIS_SET( ch->act, PLR_BLANK )  ? 'X' : ' ',
         xIS_SET( ch->act, PLR_PROMPT ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_ANSI )   ? 'X' : ' ',
         xIS_SET( ch->act, PLR_K_LIVE ) ? 'X' : ' ' );

      ch_printf( ch, "           Hints(%c)      Compass(%c)    Groupaffects(%c)  FInfo(%c)\r\n",
         !xIS_SET( ch->act, PLR_NOHINTS ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_COMPASS ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_GROUPAFFECTS ) ? 'X' : ' ',
         !xIS_SET( ch->act, PLR_NOFINFO ) ? 'X' : ' ' );

      set_char_color( AT_DGREEN, ch );
      send_to_char( "\r\nAuto:      ", ch );
      set_char_color( AT_GRAY, ch );
      ch_printf( ch, "AutoSac(%c)    AutoGold(%c)   AutoLoot(%c)      AutoExit(%c)\r\n",
         xIS_SET( ch->act, PLR_AUTOSAC )  ? 'X' : ' ',
         xIS_SET( ch->act, PLR_AUTOGOLD ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_AUTOLOOT ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_AUTOEXIT ) ? 'X' : ' ' );

      ch_printf( ch, "           Suicide(%c)    AutoSplit(%c)\r\n",
         xIS_SET( ch->act, PLR_SUICIDE ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_AUTOSPLIT ) ? 'X' : ' ' );

      set_char_color( AT_DGREEN, ch );
      send_to_char( "\r\nSafeties:  ", ch );
      set_char_color( AT_GRAY, ch );
      ch_printf( ch, "NoRecall(%c)   NoSummon(%c)   Solo(%c)          NoAssist(%c)\r\n",
         xIS_SET( ch->pcdata->flags, PCFLAG_NORECALL ) ? 'X' : ' ',
         xIS_SET( ch->pcdata->flags, PCFLAG_NOSUMMON ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_SOLO ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_NOASSIST ) ? 'X' : ' ' );

      ch_printf( ch, "           SmartSac(%c)   NoInduct(%c)   Sparing(%c)",
         xIS_SET( ch->act, PLR_SMARTSAC ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_NOINDUCT ) ? 'X' : ' ',
         xIS_SET( ch->act, PLR_SPARING ) ? 'X' : ' ' );

      if( !xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
         ch_printf( ch, "       Drag(%c)\r\n           Nice(%c)        Private(%c)",
             xIS_SET( ch->act, PLR_SHOVEDRAG ) ? 'X' : ' ',
             xIS_SET( ch->act, PLR_NICE )      ? 'X' : ' ',
             xIS_SET( ch->pcdata->flags, PCFLAG_PRIVATE ) ? 'X' : ' ' );

      set_char_color( AT_DGREEN, ch );
      send_to_char( "\r\n\r\nMisc:      ", ch );
      set_char_color( AT_GRAY, ch );
      ch_printf( ch, "TelnetGA(%c)   GroupWho(%c)   NoIntro(%c)",
         xIS_SET( ch->act, PLR_TELNET_GA )             ? 'X' : ' ',
         xIS_SET( ch->pcdata->flags, PCFLAG_GROUPWHO ) ? 'X' : ' ',
         xIS_SET( ch->pcdata->flags, PCFLAG_NOINTRO )  ? 'X' : ' ' );

      set_char_color( AT_DGREEN, ch );
      send_to_char( "\r\n\r\nSettings:  ", ch );
      set_char_color( AT_GRAY, ch );
      ch_printf( ch, "KeepAlive Time (%d)  Pager Length (%d)  Wimpy (&W%d&w)",
         ch->pcdata->kltime, ch->pcdata->pagerlen, ch->wimpy );

      set_char_color( AT_DGREEN, ch );
      send_to_char( "\r\n\r\nSentences imposed on you (if any):", ch );
      set_char_color( AT_YELLOW, ch );
      ch_printf( ch, "\r\n%s%s%s",
         xIS_SET( ch->act, PLR_SILENCE ) ? " For your abuse of channels, you're currently silenced.\r\n" : "",
         xIS_SET( ch->act, PLR_NO_EMOTE ) ? " The gods have removed your emotes.\r\n" : "",
         xIS_SET( ch->act, PLR_NO_TELL ) ? " You aren't permitted to send 'tells' to others.\r\n" : "" );
      return;
   }

   switch( UPPER( arg[0] ) )
   {
      case 'A':
         if( !str_prefix( arg, "ansi" ) )
            xTOGGLE_BIT( ch->act, PLR_ANSI );
         if( !str_prefix( arg, "autoexit" ) )
            xTOGGLE_BIT( ch->act, PLR_AUTOEXIT );
         if( !str_prefix( arg, "autogold" ) )
            xTOGGLE_BIT( ch->act, PLR_AUTOGOLD );
         if( !str_prefix( arg, "autosplit" ) )
            xTOGGLE_BIT( ch->act, PLR_AUTOSPLIT );
         if( !str_prefix( arg, "autoloot" ) )
            xTOGGLE_BIT( ch->act, PLR_AUTOLOOT );
         if( !str_prefix( arg, "autosac" ) )
            xTOGGLE_BIT( ch->act, PLR_AUTOSAC );
         break;

      case 'B':
         if( !str_prefix( arg, "blank" ) )
            xTOGGLE_BIT( ch->act, PLR_BLANK );
         if( !str_prefix( arg, "brief" ) )
            xTOGGLE_BIT( ch->act, PLR_BRIEF );
         break;

      case 'C':
         if( !str_prefix( arg, "combine" ) )
            xTOGGLE_BIT( ch->act, PLR_COMBINE );
         if( !str_prefix( arg, "compass" ) )
            xTOGGLE_BIT( ch->act, PLR_COMPASS );
         break;

      case 'D':
         if( !str_prefix( arg, "drag" ) )
         {
            if( xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
            {
               send_to_char( "Pkill characters can't config that option.\r\n", ch );
               return;
            }
            else
               xTOGGLE_BIT( ch->act, PLR_SHOVEDRAG );
         }
         break;

      case 'F':
         if( !str_prefix( arg, "flee" ) )
         {
            if( xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
            {
               send_to_char( "Pkill characters can't config that option.\r\n", ch );
               return;
            }
            else
               xTOGGLE_BIT( ch->act, PLR_FLEE );
         }
         if( !str_prefix( arg, "finfo" ) )
            xTOGGLE_BIT( ch->act, PLR_NOFINFO );
         break;

      case 'G':
         if( !str_prefix( arg, "gag" ) )
            xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_GAG );
         if( !str_prefix( arg, "groupwho" ) )
            xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_GROUPWHO );
         if( !str_prefix( arg, "groupaffects" ) )
            xTOGGLE_BIT( ch->act, PLR_GROUPAFFECTS );
         break;

      case 'H':
         if( !str_prefix( arg, "hints" ) )
            xTOGGLE_BIT( ch->act, PLR_NOHINTS );
         break;

      case 'K':
         if( !str_prefix( arg, "keepalive" ) )
            xTOGGLE_BIT( ch->act, PLR_K_LIVE );
         break;

      case 'N':
         if( !str_prefix( arg, "norecall" ) )
            xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_NORECALL );
         if( !str_prefix( arg, "nointro" ) )
            xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_NOINTRO );
         if( !str_prefix( arg, "nosummon" ) )
            xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_NOSUMMON );
         if( !str_prefix( arg, "nice" ) )
         {
            if( xIS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
            {
               send_to_char( "Pkill characters can't config that option.\r\n", ch );
               return;
            }
            else
               xTOGGLE_BIT( ch->act, PLR_NICE );
         }
         if( !str_prefix( arg, "noassist" ) )
            xTOGGLE_BIT( ch->act, PLR_NOASSIST );
         if( !str_prefix( arg, "noinduct" ) )
            xTOGGLE_BIT( ch->act, PLR_NOINDUCT );
         break;

      case 'P':
         if( !str_prefix( arg, "pager" ) )
            xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_PAGERON );
         if( !str_prefix( arg, "private" ) )
            xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_PRIVATE );
         if( !str_prefix( arg, "prompt" ) )
            xTOGGLE_BIT( ch->act, PLR_PROMPT );
         break;

      case 'S':
         if( !str_prefix( arg, "solo" ) )
            xTOGGLE_BIT( ch->act, PLR_SOLO );
         if( !str_prefix( arg, "suicide" ) )
            xTOGGLE_BIT( ch->act, PLR_SUICIDE );
         if( !str_prefix( arg, "smartsac" ) )
            xTOGGLE_BIT( ch->act, PLR_SMARTSAC );
         if( !str_prefix( arg, "sparing" ) )
            xTOGGLE_BIT( ch->act, PLR_SPARING );
         break;

      case 'T':
         if( !str_prefix( arg, "telnetga" ) )
            xTOGGLE_BIT( ch->act, PLR_TELNET_GA );
         break;
   }

   send_to_char( "Ok.\r\n", ch );
}

/* New do_areas, written by Fireblade, last modified - 12/23/2007 */
CMDF( do_areas )
{
   AREA_DATA *pArea;
   const char *header_string1 = "\r\n   Author    |             Area                     | Recommended |  Enforced\r\n";
   const char *header_string2 = "-------------+--------------------------------------+-------------+-----------\r\n";
   const char *footer_string1 = "------------------------------------------------------------------------------\r\n";
   const char *print_string = "%-12.12s | %-36.36s | %4d - %-4d | %3d - %-3d \r\n";
   char arg[MSL];
   int lower_bound = 0;
   int upper_bound = MAX_LEVEL + 1;
   int acount = 0, ashown = 0;

   argument = one_argument( argument, arg );

   if( arg != NULL && arg[0] != '\0' )
   {
      if( is_number( arg ) )
      {
         upper_bound = atoi( arg );
         lower_bound = upper_bound;

         argument = one_argument( argument, arg );

         if( arg != NULL && arg[0] != '\0' )
         {
            if( is_number( arg ) )
            {
               upper_bound = atoi( arg );
               argument = one_argument( argument, arg );
            }
         }
      }
   }

   if( lower_bound > upper_bound )
   {
      int swap = lower_bound;

      lower_bound = upper_bound;
      upper_bound = swap;
   }

   set_pager_color( AT_PLAIN, ch );
   send_to_pager( header_string1, ch );
   send_to_pager( header_string2, ch );

   for( pArea = first_area_name; pArea; pArea = pArea->next_sort_name )
   {
      acount++;
      if( xIS_SET( pArea->flags, AFLAG_NOSHOW ) ) /* Should be able to flag areas so they aren't seen in do_areas */
         continue;
      if( arg != NULL && arg[0] != '\0' && !is_number( arg ) && pArea->name && pArea->name[0] != '\0'
      && str_prefix( arg, pArea->name ) )
         continue;
      if( pArea->hi_soft_range >= lower_bound && pArea->low_soft_range <= upper_bound )
      {
         pager_printf( ch, print_string, pArea->author, pArea->name,
            pArea->low_soft_range, pArea->hi_soft_range, pArea->low_hard_range, pArea->hi_hard_range );
         ashown++;
      }
   }
   send_to_pager( footer_string1, ch );
   pager_printf( ch, "%d of %d area%s displayed.\r\n", ashown, acount, acount != 1 ? "s" : "" );
}

CMDF( do_afk )
{
   if( is_npc( ch ) )
      return;

   xTOGGLE_BIT( ch->act, PLR_AFK );

   if( !xIS_SET( ch->act, PLR_AFK ) )
   {
      send_to_char( "You're no longer afk.\r\n", ch );
      act( AT_GRAY, "$n is no longer afk.", ch, NULL, NULL, TO_CANSEE );
#ifdef IMC
      if( IMCIS_SET( IMCFLAG( ch ), IMC_AFK ) )
      {
         IMCREMOVE_BIT( IMCFLAG( ch ), IMC_AFK );
         send_to_char( "You're no longer AFK to IMC2.\r\n", ch );
      }
#endif
   }
   else
   {
      send_to_char( "You're now afk.\r\n", ch );
      act( AT_GRAY, "$n is now afk.", ch, NULL, NULL, TO_CANSEE );
#ifdef IMC
      if( !IMCIS_SET( IMCFLAG( ch ), IMC_AFK ) )
      {
         IMCSET_BIT( IMCFLAG( ch ), IMC_AFK );
         send_to_char( "You're now AFK to IMC2.\r\n", ch );
      }
#endif
   }
}

CMDF( do_slist )
{
   char skn[MIL], buf[MIL], arg1[MIL], arg2[MIL];
   int sn, i, lowlev, hilev, max = 0;
   short lasttype = SKILL_SPELL;
   bool lfound;

   if( is_npc( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   lowlev = 1;
   hilev = MAX_LEVEL;

   if( arg1[0] != '\0' )
      lowlev = atoi( arg1 );

   if( ( lowlev < 1 ) || ( lowlev > MAX_LEVEL ) )
      lowlev = 1;

   if( arg2[0] != '\0' )
      hilev = atoi( arg2 );

   if( ( hilev < 0 ) || ( hilev > MAX_LEVEL ) )
      hilev = MAX_LEVEL;

   if( lowlev > hilev )
      lowlev = hilev;

   set_pager_color( AT_MAGIC, ch );
   send_to_pager( "SPELL & SKILL LIST\r\n", ch );
   send_to_pager( "------------------\r\n", ch );

   for( i = lowlev; i <= hilev; i++ )
   {
      lfound = false;
      snprintf( skn, sizeof( skn ), "%s", "Spell" );
      for( sn = 0; sn < top_sn; sn++ )
      {
         if( !skill_table[sn] || !skill_table[sn]->name )
            continue;

         if( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
            continue;

         if( i != first_learned( ch, sn ) )
            continue;

         if( skill_table[sn]->type != lasttype )
         {
            lasttype = skill_table[sn]->type;
            mudstrlcpy( skn, skill_tname[lasttype], sizeof( skn ) );
         }

         if( !lfound )
         {
            lfound = true;
            pager_printf( ch, "Level %d\r\n", i );
         }
         switch( skill_table[sn]->minimum_position )
         {
            default:
               snprintf( buf, sizeof( buf ), "%s", "Invalid" );
               bug( "%s: skill with invalid minpos, skill = %s", __FUNCTION__, skill_table[sn]->name );
               break;

            case POS_DEAD:
               snprintf( buf, sizeof( buf ), "%s", "any" );
               break;

            case POS_MORTAL:
               snprintf( buf, sizeof( buf ), "%s", "mortally wounded" );
               break;

            case POS_INCAP:
               snprintf( buf, sizeof( buf ), "%s", "incapacitated" );
               break;

            case POS_STUNNED:
               snprintf( buf, sizeof( buf ), "%s", "stunned" );
               break;

            case POS_SLEEPING:
               snprintf( buf, sizeof( buf ), "%s", "sleeping" );
               break;

            case POS_RESTING:
               snprintf( buf, sizeof( buf ), "%s", "resting" );
               break;

            case POS_STANDING:
               snprintf( buf, sizeof( buf ), "%s", "standing" );
               break;

            case POS_FIGHTING:
               snprintf( buf, sizeof( buf ), "%s", "fighting" );
               break;

            case POS_EVASIVE:
               snprintf( buf, sizeof( buf ), "%s", "fighting (evasive)" ); /* Fighting style support -haus */
               break;

            case POS_DEFENSIVE:
               snprintf( buf, sizeof( buf ), "%s", "fighting (defensive)" );
               break;

            case POS_AGGRESSIVE:
               snprintf( buf, sizeof( buf ), "%s", "fighting (aggressive)" );
               break;

            case POS_BERSERK:
               snprintf( buf, sizeof( buf ), "%s", "fighting (berserk)" );
               break;

            case POS_MOUNTED:
               snprintf( buf, sizeof( buf ), "%s", "mounted" );
               break;

            case POS_SITTING:
               snprintf( buf, sizeof( buf ), "%s", "sitting" );
               break;

            case POS_SHOVE:
               snprintf( buf, sizeof( buf ), "%s", "shoved" );
               break;

            case POS_DRAG:
               snprintf( buf, sizeof( buf ), "%s", "dragged" );
               break;
         }
         max = get_possible_adept( ch, sn );
         pager_printf( ch, "%7s: %20.20s Current: %-3d Max: %-3d MinPos: %s\r\n", skn, skill_table[sn]->name, ch->pcdata->learned[sn], max, buf );
      }
   }
}

CMDF( do_whois )
{
   CHAR_DATA *victim;
   const char *s1, *s2;
   char buf[MSL];

   buf[0] = '\0';

   if( is_npc( ch ) )
      return;

   s1 = color_str( AT_SCORE, ch );
   s2 = color_str( AT_SCORE2, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_pager( "You must input the name of an online character.\r\n", ch );
      return;
   }

   mudstrlcat( buf, "0.", sizeof( buf ) );
   mudstrlcat( buf, argument, sizeof( buf ) );
   if( !( victim = get_char_world( ch, buf ) ) )
   {
      send_to_pager( "No such character online.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_pager( "That's not a player!\r\n", ch );
      return;
   }

   set_pager_color( AT_SCORE, ch );
   pager_printf( ch, "\r\n%s%s%s.\r\n",
      victim->sex == SEX_MALE ? color_str( AT_MALE, ch ) : victim->sex == SEX_FEMALE ? color_str( AT_FEMALE, ch ) : "",
      victim->name, victim->pcdata->title );

   pager_printf( ch, "%s%s is a level %s%d %s",
      s1, victim->sex == SEX_MALE ? "He" : victim->sex == SEX_FEMALE ? "She" : "It",
      s2, victim->level, capitalize( dis_race_name( victim->race ) ) );

   pager_printf( ch, " %s%s, %s%d %syears of age.\r\n",
      capitalize( dis_main_class_name( victim ) ), s1, s2, get_age( victim ), s1 );

   pager_printf( ch, "%s%s is a %sdeadly player",
      s1, victim->sex == SEX_MALE ? "He" : victim->sex == SEX_FEMALE ? "She" : "It",
      xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ? "" : "non-" );

   if( victim->pcdata->clan )
      pager_printf( ch, ", and belongs to Clan %s%s%s", s2, victim->pcdata->clan->name ? victim->pcdata->clan->name : "(Unknown)", s1 );
   send_to_pager( ".\r\n", ch );

   if( victim->pcdata->council )
      pager_printf( ch, "%s%s holds a seat on:  %s%s\r\n",
         s1, victim->sex == SEX_MALE ? "He" : victim->sex == SEX_FEMALE ? "She" : "It",
         s2, victim->pcdata->council->name ? victim->pcdata->council->name : "(Unknown)" );

   if( victim->pcdata->deity )
      pager_printf( ch, "%s%s has found succor in the deity %s%s%s.\r\n",
         s1, victim->sex == SEX_MALE ? "He" : victim->sex == SEX_FEMALE ? "She" : "It",
         s2, victim->pcdata->deity->name ? victim->pcdata->deity->name : "(Unknown)", s1 );

   if( victim->pcdata->homepage )
      pager_printf( ch, "%s%s homepage can be found at %s%s\r\n",
         s1, victim->sex == SEX_MALE ? "His" : victim->sex == SEX_FEMALE ? "Her" : "Its",
         s2, show_tilde( victim->pcdata->homepage ) );

   if( victim->pcdata->bio )
      pager_printf( ch, "%s%s%s's personal bio:\r\n%s%s", s2, victim->name, s1, s2, victim->pcdata->bio );
   else
      pager_printf( ch, "%s%s %shas yet to create a bio.\r\n", s2, victim->name, s1 );

   if( is_immortal( ch ) || !xIS_SET( victim->pcdata->flags, PCFLAG_PRIVATE ) )
   {
      if( victim->pcdata->email )
         pager_printf( ch, "%sEmail: %s%s\r\n", s1, s2, victim->pcdata->email );
      if( victim->pcdata->msn )
         pager_printf( ch, "%sMSN:   %s%s\r\n", s1, s2, victim->pcdata->msn );
      if( victim->pcdata->yahoo )
         pager_printf( ch, "%sYahoo: %s%s\r\n", s1, s2, victim->pcdata->yahoo );
      if( victim->pcdata->gtalk )
         pager_printf( ch, "%sGTalk: %s%s\r\n", s1, s2, victim->pcdata->gtalk );
   }

   if( is_immortal( ch ) )
   {
      pager_printf( ch, "%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~%sImmortal Info%s~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", s1, s2, s1 );

      pager_printf( ch, "%s%s %shas killed %s%u %smobiles, and been killed by a mobile %s%u %stimes.\r\n",
         s2, victim->name, s1, s2, victim->pcdata->mkills, s1, s2, victim->pcdata->mdeaths, s1 );
      if( victim->pcdata->pkills || victim->pcdata->pdeaths )
         pager_printf( ch, "%s%s %shas killed %s%u %splayers, and been killed by a player %s%u %stimes.\r\n",
            s2, victim->name, s1, s2, victim->pcdata->pkills, s1, s2, victim->pcdata->pdeaths, s1 );

      if( xIS_SET( victim->act, PLR_SILENCE ) || xIS_SET( victim->act, PLR_NO_EMOTE )
      || xIS_SET( victim->act, PLR_NO_TELL ) )
      {
         pager_printf( ch, "%sThis player has the following flags set:%s", s1, s2 );
         if( xIS_SET( victim->act, PLR_SILENCE ) )
            send_to_pager( " silence", ch );
         if( xIS_SET( victim->act, PLR_NO_EMOTE ) )
            send_to_pager( " noemote", ch );
         if( xIS_SET( victim->act, PLR_NO_TELL ) )
            send_to_pager( " notell", ch );
         pager_printf( ch, "%s.\r\n", s1 );
      }
      if( victim->desc && victim->desc->host[0] != '\0' )   /* added by Gorog */
         pager_printf( ch, "%s%s%s's IP info: %s%s\r\n", s2, victim->name, s1, s2, victim->desc->host );
   }
}

CMDF( do_pager )
{
   char arg[MIL];

   if( is_npc( ch ) )
      return;
   set_char_color( AT_NOTE, ch );
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_PAGERON );
      if( xIS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) )
         ch_printf( ch, "Pager is now enabled at %d lines.\r\n", ch->pcdata->pagerlen );
      else
         send_to_char( "Pager disabled.\r\n", ch );
      return;
   }
   if( !is_number( arg ) )
   {
      send_to_char( "Set page pausing to how many lines?\r\n", ch );
      return;
   }
   ch->pcdata->pagerlen = UMAX( 5, atoi( arg ) );
   ch_printf( ch, "Page pausing set to %d lines.\r\n", ch->pcdata->pagerlen );
}

CMDF( do_klive )
{
   char arg[MIL];

   if( is_npc( ch ) )
      return;
   set_char_color( AT_NOTE, ch );
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      xTOGGLE_BIT( ch->act, PLR_K_LIVE );
      if( xIS_SET( ch->act, PLR_K_LIVE ) )
         ch_printf( ch, "KeepAlive time is now set to %d min%s.\r\n", ch->pcdata->kltime, ch->pcdata->kltime == 1  ? "" : "s" );
      else
         send_to_char( "KeepAlive disabled.\r\n", ch );
      return;
   }
   if( !is_number( arg ) )
   {
      send_to_char( "Set time to how long ( 1 - 10 )?\r\n", ch );
      return;
   }
   ch->pcdata->kltime = URANGE( 1, atoi( arg ), 10 );
   ch_printf( ch, "KeepAlive time is now set to %d min%s.\r\n", ch->pcdata->kltime, ch->pcdata->kltime == 1 ? "" : "s" );
}

/*
 * The ignore command allows players to ignore other players.
 * Players may ignore other characters whether
 * they are online or not. This is to prevent people from
 * spamming someone and then logging off quickly to evade
 * being ignored.
 * Usage:
 *	ignore		-	lists players currently ignored
 *	ignore none	-	sets it so no players are ignored
 *	ignore <player>	-	start ignoring player if not already
 *				ignored otherwise stop ignoring player
 *	ignore reply	-	start ignoring last player to send a
 *				tell to ch, to deal with invis spammers
 * - Fireblade
 */
CMDF( do_ignore )
{
   IGNORE_DATA *temp, *next, *inew;
   CHAR_DATA *victim = NULL;
   char arg[MIL];

   if( is_npc( ch ) )
      return;

   argument = one_argument( argument, arg );
   /* If no arguements, then list players currently ignored */
   if( arg == NULL || arg[0] == '\0' )
   {
      set_char_color( AT_DIVIDER, ch );
      send_to_char( "\r\n----------------------------------------\r\n", ch );
      set_char_color( AT_DGREEN, ch );
      send_to_char( "You're currently ignoring:\r\n", ch );
      set_char_color( AT_DIVIDER, ch );
      send_to_char( "----------------------------------------\r\n", ch );
      set_char_color( AT_IGNORE, ch );

      if( !ch->pcdata->first_ignored )
         send_to_char( "  - No one\r\n", ch );
      else
      {
         for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
            ch_printf( ch, "  - %s\r\n", temp->name );
      }
      return;
   }

   set_char_color( AT_IGNORE, ch );

   /* Clear players ignored if given arg "none" */
   if( !strcmp( arg, "none" ) )
   {
      for( temp = ch->pcdata->first_ignored; temp; temp = next )
      {
         next = temp->next;
         UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
         STRFREE( temp->name );
         DISPOSE( temp );
      }

      send_to_char( "You no longer ignore anyone.\r\n", ch );
      return;
   }
   /* Prevent someone from ignoring themself... */
   else if( !strcmp( arg, "self" ) || nifty_is_name( arg, ch->name ) )
   {
      send_to_char( "Did you type something?\r\n", ch );
      return;
   }

   /* get the name of the char who last sent tell to ch */
   if( !strcmp( arg, "reply" ) )
   {
      if( !ch->reply )
      {
         send_to_char( "They're not here.\r\n", ch );
         return;
      }
      else
         mudstrlcpy( arg, ch->reply->name, sizeof( arg ) );
   }

   /* Loop through the linked list of ignored players */
   for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
   {
      /* If the argument matches a name in list remove it */
      if( !strcmp( temp->name, capitalize( arg ) ) )
      {
         UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
         ch_printf( ch, "You no longer ignore %s.\r\n", temp->name );
         STRFREE( temp->name );
         DISPOSE( temp );
         return;
      }
   }

   if( !( victim = get_char_world( ch, arg ) ) || is_npc( victim ) || strcmp( capitalize( arg ), victim->name ) != 0 )
   {
      if( victim || !valid_pfile( arg ) )
      {
         send_to_char( "No player exists by that name.\r\n", ch );
         return;
      }
   }

   if( victim )
      mudstrlcpy( arg, capitalize( victim->name ), sizeof( arg ) );

   if( !check_parse_name( capitalize( arg ), true ) )
   {
      send_to_char( "No player exists by that name.\r\n", ch );
      return;
   }

   CREATE( inew, IGNORE_DATA, 1 );
   if( !inew )
   {
      bug( "%s: couldn't create a new ignore.\r\n", __FUNCTION__ );
      return;
   }
   inew->name = STRALLOC( capitalize( arg ) );
   inew->next = NULL;
   inew->prev = NULL;
   LINK( inew, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
   ch_printf( ch, "You now ignore %s.\r\n", inew->name );
}

/*
 * This function simply checks to see if ch is ignoring ign_ch.
 * Last Modified: October 10, 1997 - Fireblade
 */
bool is_ignoring( CHAR_DATA *ch, CHAR_DATA *ign_ch )
{
   IGNORE_DATA *temp;

   if( is_npc( ch ) || is_npc( ign_ch ) )
      return false;

   for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
   {
      if( nifty_is_name( temp->name, ign_ch->name ) )
         return true;
   }

   return false;
}

/* Version info -- Scryn */
CMDF( do_version )
{
   set_char_color( AT_YELLOW, ch );
   send_to_char( "DikuMUD\r\n", ch );
   send_to_char( "MERC  2.1\r\n", ch );
   send_to_char( "SMAUG 1.4\r\n", ch );
   send_to_char( "SMAUG 1.7 FUSS\r\n", ch );
   ch_printf( ch, "LOP   %d.%d\r\n", sysdata.version_major, sysdata.version_minor );
   ch_printf( ch, "Last Compiled on %s at %s.\r\n", __DATE__, __TIME__ );
}

/*
 * This code is so that if you're leading a group or have others following
 * you, you may use it to leave them behind while you do something without
 * them having to sit
 */
CMDF( do_solo )
{
   if( !ch || is_npc( ch ) )
      return;
   set_pager_color( AT_WHITE, ch );
   xTOGGLE_BIT( ch->act, PLR_SOLO );
   ch_printf( ch, "You will %s allow others to follow you.\r\n", xIS_SET( ch->act, PLR_SOLO ) ? "not" : "now" );
   act_printf( AT_GRAY, ch, NULL, NULL, TO_ROOM, "$n will %s allow others to follow $m.", xIS_SET( ch->act, PLR_SOLO ) ? "not" : "now");
}
