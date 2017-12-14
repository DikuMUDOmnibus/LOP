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
 *****************************************************************************
 *  The MUDprograms are heavily based on the original MOBprogram code that   *
 *  was written by N'Atas-ha.						     *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"

const char *mprog_type_to_name( int type )
{
   switch( type )
   {
      case IN_FILE_PROG:
         return "in_file_prog";
      case ACT_PROG:
         return "act_prog";
      case SPEECH_PROG:
         return "speech_prog";
      case RAND_PROG:
         return "rand_prog";
      case FIGHT_PROG:
         return "fight_prog";
      case HITPRCNT_PROG:
         return "hitprcnt_prog";
      case DEATH_PROG:
         return "death_prog";
      case ENTRY_PROG:
         return "entry_prog";
      case GREET_PROG:
         return "greet_prog";
      case ALL_GREET_PROG:
         return "all_greet_prog";
      case GIVE_PROG:
         return "give_prog";
      case BRIBE_PROG:
         return "bribe_prog";
      case HOUR_PROG:
         return "hour_prog";
      case TIME_PROG:
         return "time_prog";
      case WEAR_PROG:
         return "wear_prog";
      case REMOVE_PROG:
         return "remove_prog";
      case SAC_PROG:
         return "sac_prog";
      case LOOK_PROG:
         return "look_prog";
      case EXA_PROG:
         return "exa_prog";
      case ZAP_PROG:
         return "zap_prog";
      case OPEN_PROG:
         return "open_prog";
      case CLOSE_PROG:
         return "close_prog";
      case GET_PROG:
         return "get_prog";
      case DROP_PROG:
         return "drop_prog";
      case REPAIR_PROG:
         return "repair_prog";
      case DAMAGE_PROG:
         return "damage_prog";
      case SCRAP_PROG:
         return "scrap_prog";
      case PUT_PROG:
         return "put_prog";
      case PULL_PROG:
         return "pull_prog";
      case PUSH_PROG:
         return "push_prog";
      case SCRIPT_PROG:
         return "script_prog";
      case SLEEP_PROG:
         return "sleep_prog";
      case REST_PROG:
         return "rest_prog";
      case LEAVE_PROG:
         return "leave_prog";
      case USE_PROG:
         return "use_prog";
      default:
         return "ERROR_PROG";
   }
}

/*
 * A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MUDprograms which are set.
 */
CMDF( do_mpstat )
{
   char arg[MIL];
   MPROG_DATA *mprg;
   CHAR_DATA *victim;
   int cnt = 0, show = 0;
   bool full = false;

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Mpstat whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( !is_npc( victim ) )
   {
      send_to_char( "Only Mobiles can have MobPrograms!\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_HEAD && xIS_SET( victim->act, ACT_STATSHIELD ) )
   {
      set_pager_color( AT_IMMORT, ch );
      send_to_pager( "Their godly glow prevents you from getting a good look.\r\n", ch );
      return;
   }

   if( xIS_EMPTY( victim->pIndexData->progtypes ) )
   {
      send_to_char( "That Mobile has no Programs set.\r\n", ch );
      return;
   }
   ch_printf( ch, "Name: %s.  Vnum: %d.\r\n", victim->name, victim->pIndexData->vnum );
   ch_printf( ch, "Short description: %s.\r\nLong  description: %s",
      victim->short_descr, victim->long_descr[0] != '\0' ? victim->long_descr : "(none).\r\n" );
   ch_printf( ch, "Hp: %d/%d.  Mana: %d/%d.  Move: %d/%d. \r\n",
      victim->hit, victim->max_hit, victim->mana, victim->max_mana, victim->move, victim->max_move );
   ch_printf( ch, "Lv: %d.  Align: %d.  AC: %d.  Gold: %s.\r\n",
      victim->level, victim->alignment, get_ac( victim ),
      show_char_gold( victim ) );

   if( !str_cmp( argument, "full" ) )
      full = true;
   else if( is_number( argument ) )
      show = UMAX( 0, atoi( argument ) );
   for( mprg = victim->pIndexData->mudprogs; mprg; mprg = mprg->next )
   {
      ++cnt;
      if( show > 0 && cnt != show )
         continue;

      ch_printf( ch, "%d>%s %s\r\n", cnt, mprog_type_to_name( mprg->type ), mprg->arglist );
      if( full || show > 0 )
         ch_printf( ch, "%s\r\n", mprg->comlist );

      if( show > 0 && cnt == show )
         return;
   }
}

/* Opstat - Scryn 8/12*/
CMDF( do_opstat )
{
   char arg[MIL];
   MPROG_DATA *mprg;
   OBJ_DATA *obj;
   int cnt = 0, show = 0;
   bool full = false;

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "OProg stat what?\r\n", ch );
      return;
   }

   if( !( obj = get_obj_world( ch, arg ) ) )
   {
      send_to_char( "You can't find that.\r\n", ch );
      return;
   }

   if( xIS_EMPTY( obj->pIndexData->progtypes ) )
   {
      send_to_char( "That object has no programs set.\r\n", ch );
      return;
   }

   ch_printf( ch, "Name: %s.  Vnum: %d.\r\n", obj->name, obj->pIndexData->vnum );

   ch_printf( ch, "Short description: %s.\r\n", obj->short_descr );

   if( !str_cmp( argument, "full" ) )
      full = true;
   else if( is_number( argument ) )
      show = UMAX( 0, atoi( argument ) );
   for( mprg = obj->pIndexData->mudprogs; mprg; mprg = mprg->next )
   {
      ++cnt;
      if( show > 0 && cnt != show )
         continue;

      ch_printf( ch, "%d>%s %s\r\n", cnt, mprog_type_to_name( mprg->type ), mprg->arglist );
      if( full || show > 0 )
         ch_printf( ch, "%s\r\n", mprg->comlist );

      if( show > 0 && cnt == show )
         return;
   }
}

/* Rpstat - Scryn 8/12 */
CMDF( do_rpstat )
{
   MPROG_DATA *mprg;
   int cnt = 0, show = 0;
   bool full = false;

   if( xIS_EMPTY( ch->in_room->progtypes ) )
   {
      send_to_char( "This room has no programs set.\r\n", ch );
      return;
   }

   ch_printf( ch, "Name: %s.  Vnum: %d.\r\n", ch->in_room->name, ch->in_room->vnum );

   if( !str_cmp( argument, "full" ) )
      full = true;
   else if( is_number( argument ) )
      show = UMAX( 0, atoi( argument ) );
   for( mprg = ch->in_room->mudprogs; mprg; mprg = mprg->next )
   {
      ++cnt;
      if( show > 0 && cnt != show )
         continue;

      ch_printf( ch, "%d>%s %s\r\n", cnt, mprog_type_to_name( mprg->type ), mprg->arglist );
      if( full || show > 0 )
         ch_printf( ch, "%s\r\n", mprg->comlist );

      if( show > 0 && cnt == show )
         return;
   }
}

/* Woowoo - Blodkai, November 1997 */
CMDF( do_mpasuppress )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   int rnds;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Mpasuppress who?\r\n", ch );
      progbug( "Mpasuppress: argument for victim wasn't found", ch );
      return;
   }
   if( arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Suppress their attacks for how many rounds?\r\n", ch );
      progbug( "Mpasuppress: argument for round wasn't found", ch );
      return;
   }
   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "No such victim in the room.\r\n", ch );
      progbug( "Mpasuppress:  victim not in room", ch );
      return;
   }
   if( ( rnds = atoi( arg2 ) ) <= 0 || rnds > 32000 )
   {
      send_to_char( "Invalid number of rounds to suppress attacks.\r\n", ch );
      progbug( "Mpsuppress:  invalid number of rounds, valid range is 1 - 32000", ch );
      return;
   }
   add_timer( victim, TIMER_ASUPPRESSED, rnds, NULL, 0 );
}

/* lets the mobile kill any player or mobile without murder*/
CMDF( do_mpkill )
{
   char arg[MIL];
   CHAR_DATA *victim;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   if( !ch )
   {
      bug( "%s: NULL ch!", __FUNCTION__ );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "MpKill - no argument", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "MpKill - Victim not in room", ch );
      return;
   }

   if( victim == ch )
   {
      progbug( "MpKill - Bad victim to attack", ch );
      return;
   }

   if( is_fighting( ch ) )
   {
      progbug( "MpKill - Already fighting", ch );
      return;
   }

   multi_hit( ch, victim, TYPE_UNDEFINED );
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
CMDF( do_mpjunk )
{
   char arg[MIL];
   OBJ_DATA *obj, *obj_next;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpjunk - No argument", ch );
      return;
   }

   if( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
   {
      if( ( obj = get_obj_wear( ch, arg ) ) )
      {
         unequip_char( ch, obj );
         extract_obj( obj );
         return;
      }
      if( !( obj = get_obj_carry( ch, arg ) ) )
         return;
      extract_obj( obj );
   }
   else
   {
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( arg[3] == '\0' || is_name( &arg[4], obj->name ) )
         {
            if( obj->wear_loc != WEAR_NONE )
               unequip_char( ch, obj );
            extract_obj( obj );
         }
      }
   }
}

/*
 * This function examines a text string to see if the first "word" is a
 * color indicator (e.g. _red, _whi_, _blu).  -  Gorog
 */
int get_color( char *argument )  /* get color code from command string */
{
   char color[MIL];
   const char *cptr;
   static char const *color_list = "_bla_red_dgr_bro_dbl_pur_cya_cha_dch_ora_gre_yel_blu_pin_lbl_whi";
   static char const *blink_list = "*bla*red*dgr*bro*dbl*pur*cya*cha*dch*ora*gre*yel*blu*pin*lbl*whi";

   one_argument( argument, color );
   if( color[0] != '_' && color[0] != '*' )
      return 0;
   if( ( cptr = strstr( color_list, color ) ) )
      return ( cptr - color_list ) / 4;
   if( ( cptr = strstr( blink_list, color ) ) )
      return ( cptr - blink_list ) / 4 + AT_BLINK;
   return 0;
}

/* Prints the argument to all the rooms around the mobile */
CMDF( do_mpasound )
{
   char arg1[MIL];
   ROOM_INDEX_DATA *was_in_room;
   EXIT_DATA *pexit;
   short color;
   EXT_BV actflags;

   if( !ch )
   {
      bug( "%s: NULL ch!", __FUNCTION__ );
      return;
   }
   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbug( "Mpasound - No argument", ch );
      return;
   }
   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   if( ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg1 );
   was_in_room = ch->in_room;
   for( pexit = was_in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( pexit->to_room && pexit->to_room != was_in_room )
      {
         ch->in_room = pexit->to_room;
         MOBtrigger = false;
         if( color )
            act( color, argument, ch, NULL, NULL, TO_ROOM );
         else
            act( AT_SAY, argument, ch, NULL, NULL, TO_ROOM );
      }
   }
   ch->act = actflags;
   ch->in_room = was_in_room;
}

/* prints the message to all in the room other than the mob and victim */
CMDF( do_mpechoaround )
{
   char arg[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;
   short color;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpechoaround - No argument", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpechoaround - victim does not exist", ch );
      return;
   }

   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   /* DONT_UPPER prevents argument[0] from being captilized. --Shaddai */
   DONT_UPPER = true;
   if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg );
      act( color, argument, ch, NULL, victim, TO_NOTVICT );
   }
   else
      act( AT_ACTION, argument, ch, NULL, victim, TO_NOTVICT );

   DONT_UPPER = false;  /* Always set it back to false */
   ch->act = actflags;
}

/* prints message only to victim */
CMDF( do_mpechoat )
{
   char arg[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;
   short color;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpechoat - No argument", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpechoat - victim does not exist", ch );
      return;
   }

   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   DONT_UPPER = true;
   if( argument[0] == '\0' )
      act( AT_ACTION, " ", ch, NULL, victim, TO_VICT );
   else if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg );
      act( color, argument, ch, NULL, victim, TO_VICT );
   }
   else
      act( AT_ACTION, argument, ch, NULL, victim, TO_VICT );

   DONT_UPPER = false;

   ch->act = actflags;
}

/* prints message to room at large. */
CMDF( do_mpecho )
{
   char arg1[MIL];
   short color;
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   DONT_UPPER = true;
   if( argument[0] == '\0' )
      act( AT_ACTION, " ", ch, NULL, NULL, TO_ROOM );
   else if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg1 );
      act( color, argument, ch, NULL, NULL, TO_ROOM );
   }
   else
      act( AT_ACTION, argument, ch, NULL, NULL, TO_ROOM );
   DONT_UPPER = false;
   ch->act = actflags;
}

/* sound support -haus */
CMDF( do_mpsoundaround )
{
   char arg[MIL], sound[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpsoundaround - No argument", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpsoundaround - victim does not exist", ch );
      return;
   }

   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   snprintf( sound, sizeof( sound ), "!!SOUND(%s)\n", argument );
   act( AT_ACTION, sound, ch, NULL, victim, TO_NOTVICT );

   ch->act = actflags;
}

/* prints message only to victim */
CMDF( do_mpsoundat )
{
   char arg[MIL], sound[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      progbug( "Mpsoundat - No argument", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpsoundat - victim does not exist", ch );
      return;
   }

   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   snprintf( sound, sizeof( sound ), "!!SOUND(%s)\n", argument );
   act( AT_ACTION, sound, ch, NULL, victim, TO_VICT );

   ch->act = actflags;
}

/* prints message to room at large. */
CMDF( do_mpsound )
{
   char sound[MIL];
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbug( "Mpsound - called w/o argument", ch );
      return;
   }

   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   snprintf( sound, sizeof( sound ), "!!SOUND(%s)\n", argument );
   act( AT_ACTION, sound, ch, NULL, NULL, TO_ROOM );

   ch->act = actflags;
}
/* end sound stuff ----------------------------------------*/

/* Music stuff, same as above, at zMUD coders' request -- Blodkai */
CMDF( do_mpmusicaround )
{
   char arg[MIL], music[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpmusicaround - No argument", ch );
      return;
   }
   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpmusicaround - victim does not exist", ch );
      return;
   }
   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   snprintf( music, sizeof( music ), "!!MUSIC(%s)\n", argument );
   act( AT_ACTION, music, ch, NULL, victim, TO_NOTVICT );
   ch->act = actflags;
}

CMDF( do_mpmusic )
{
   char arg[MIL], music[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpmusic - No argument", ch );
      return;
   }
   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpmusic - victim does not exist", ch );
      return;
   }
   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   snprintf( music, sizeof( music ), "!!MUSIC(%s)\n", argument );
   act( AT_ACTION, music, ch, NULL, victim, TO_ROOM );
   ch->act = actflags;
}

CMDF( do_mpmusicat )
{
   char arg[MIL], music[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpmusicat - No argument", ch );
      return;
   }
   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpmusicat - victim does not exist", ch );
      return;
   }
   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   snprintf( music, sizeof( music ), "!!MUSIC(%s)\n", argument );
   act( AT_ACTION, music, ch, NULL, victim, TO_VICT );
   ch->act = actflags;
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory.  you can specify a level with
 * the load object portion as well.
 */
CMDF( do_mpmload )
{
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *victim;
   char arg[MSL];
   int vnum = 0, max = 10;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbug_printf( ch, "%s: NULL argument", __FUNCTION__ );
      return;
   }

   argument = one_argument( argument, arg );
   if( !is_number( arg ) )
   {
      progbug_printf( ch, "%s: Arg (%s) isn't a number.", __FUNCTION__, arg );
      return;
   }

   vnum = atoi( arg );
   if( !( pMobIndex = get_mob_index( vnum ) ) )
   {
      progbug_printf( ch, "%s: No mob using vnum (%d)", __FUNCTION__, vnum );
      return;
   }

   if( argument && argument[0] != '\0' )
      if( is_number( argument ) )
         max = atoi( argument );

   if( max > 50 ) /* Should never need to have a max of more then 50, allowing to many to be invoked crazy like can cause major problems */
      max = 50;

   if( pMobIndex->count >= max )
      return;

   if( !( victim = create_mobile( pMobIndex ) ) )
   {
      progbug_printf( ch, "%s: couldn't create mobile for %d.", __FUNCTION__, vnum );
      return;
   }

   char_to_room( victim, ch->in_room );
   if( is_in_wilderness( ch ) )
      put_in_wilderness( victim, ch->cords[0], ch->cords[1] );
}

CMDF( do_mpoload )
{
   char arg1[MIL], arg2[MIL];
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   int level, timer = 0;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' || !is_number( arg1 ) )
   {
      progbug( "Mpoload - Bad Usage", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' )
      level = get_trust( ch );
   else
   {
      /* New feature from Alander. */
      if( !is_number( arg2 ) )
      {
         progbug_printf( ch, "Mpoload - (%s) should be a number for the level", arg2 );
         return;
      }
      if( ( level = atoi( arg2 ) ) < 0 || level > get_trust( ch ) )
      {
         progbug_printf( ch, "Mpoload - Bad level (%d)", level );
         return;
      }

      if( ( timer = atoi( argument ) ) < 0 )
      {
         progbug_printf( ch, "Mpoload - Bad timer argument (%s)", argument );
         return;
      }
   }

   if( !( pObjIndex = get_obj_index( atoi( arg1 ) ) ) )
   {
      progbug_printf( ch, "Mpoload - Bad vnum (%s)", arg1 );
      return;
   }

   if( !( obj = create_object( pObjIndex, level ) ) )
   {
      progbug( "MpOLoad - obj still NULL after create_object.", ch );
      return;
   }

   obj->timer = timer;
   if( !can_wear( obj, ITEM_NO_TAKE ) )
      obj_to_char( obj, ch );
   else
   {
      obj_to_room( obj, ch->in_room );
      if( is_in_wilderness( ch ) )
         obj_to_char_cords( obj, ch );
   }
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 * itself, but this had best be the last command in the MUDprogram
 * otherwise ugly stuff will happen
 */
CMDF( do_mppurge )
{
   char arg[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      /* 'purge' */
      CHAR_DATA *vnext;

      for( victim = ch->in_room->first_person; victim; victim = vnext )
      {
         vnext = victim->next_in_room;
         if( is_npc( victim ) && victim != ch )
            extract_char( victim, true );
      }
      while( ch->in_room->first_content )
         extract_obj( ch->in_room->first_content );

      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      if( ( obj = get_obj_here( ch, arg ) ) )
         extract_obj( obj );
      else
         progbug( "Mppurge - Bad argument", ch );
      return;
   }

   if( !is_npc( victim ) )
   {
      progbug( "Mppurge - Trying to purge a PC", ch );
      return;
   }

   if( victim == ch )
   {
      progbug( "Mppurge - Trying to purge oneself", ch );
      return;
   }

   if( is_npc( victim ) && victim->pIndexData->vnum == MOB_VNUM_SUPERMOB )
   {
      progbug( "Mppurge: trying to purge supermob", ch );
      return;
   }

   extract_char( victim, true );
}

/* Allow mobiles to go wizinvis with programs -- SB */
CMDF( do_mpinvis )
{
   char arg[MIL];
   short level;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg != NULL && arg[0] != '\0' )
   {
      if( !is_number( arg ) )
      {
         progbug( "Mpinvis - Non numeric argument ", ch );
         return;
      }
      level = atoi( arg );
      /* If perm all might as well just say its invalid since all can see them */
      if( level < 1 || level > ch->level )
      {
         progbug( "MPinvis - Invalid level ", ch );
         return;
      }

      ch->mobinvis = level;
      ch_printf( ch, "Mobinvis level set to %d.\r\n", level );
      return;
   }

   if( ch->mobinvis < 1 )
      ch->mobinvis = 1;

   xTOGGLE_BIT( ch->act, ACT_MOBINVIS );
   if( !xIS_SET( ch->act, ACT_MOBINVIS ) )
   {
      act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "You slowly fade back into existence.\r\n", ch );
   }
   else
   {
      act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "You slowly vanish into thin air.\r\n", ch );
   }
}

/* lets the mobile goto any location it wishes that is not private */
/* Mounted chars follow their mobiles now - Blod, 11/97 */
CMDF( do_mpgoto )
{
   char arg[MIL];
   ROOM_INDEX_DATA *location, *in_room;
   CHAR_DATA *fch, *fch_next;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpgoto - No argument", ch );
      return;
   }

   if( !( location = find_location( ch, arg ) ) )
   {
      progbug( "Mpgoto - No such location", ch );
      return;
   }

   in_room = ch->in_room;
   stop_fighting( ch, true );
   char_from_room( ch );
   char_to_room( ch, location );
   set_loc_cords( ch );
   for( fch = in_room->first_person; fch; fch = fch_next )
   {
      fch_next = fch->next_in_room;
      if( fch->mount && fch->mount == ch )
      {
         char_from_room( fch );
         char_to_room( fch, location );
         set_loc_cords( fch );
      }
   }
}

/* lets the mobile do a command at another location. Very useful */
CMDF( do_mpat )
{
   ROOM_INDEX_DATA *location, *original;
   char arg[MIL];
   short oldx = 0, oldy = 0;
   bool wasinwild = false;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      progbug( "Mpat - Bad argument", ch );
      return;
   }

   if( !( location = find_location( ch, arg ) ) )
   {
      progbug( "Mpat - No such location", ch );
      return;
   }

   original = ch->in_room;
   if( is_in_wilderness( ch ) )
   {
      wasinwild = true;
      oldx = ch->cords[0];
      oldy = ch->cords[1];
   }
   char_from_room( ch );
   char_to_room( ch, location );
   set_loc_cords( ch );
   interpret( ch, argument );

   if( !char_died( ch ) )
   {
      char_from_room( ch );
      char_to_room( ch, original );
      if( wasinwild )
         put_in_wilderness( ch, oldx, oldy );
   }
}

/* This will only advance someone to level 2 */
CMDF( do_mpadvance )
{
   CHAR_DATA *victim;
   MCLASS_DATA *mclass;
   char arg[MIL];

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      progbug( "Mpadvance - Victim not specified", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mpadvance - Victim not there", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      progbug( "Mpadvance - Victim is NPC", ch );
      return;
   }

   if( victim->level >= 2 )
      return;

   for( mclass = victim->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      if( mclass->level >= 2 )
         continue;
      ++mclass->level;
      mclass->exp = 0;
   }
   advance_level( victim );
}

/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location 
 * the area argument transfers everyone in the current area to the
 * specified location
 */
CMDF( do_mptransfer )
{
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim, *nextinroom;
   DESCRIPTOR_DATA *d;
   char arg1[MIL], arg2[MIL];

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      progbug( "Mptransfer - No argument", ch );
      return;
   }

   if( arg2 != NULL && arg2[0] != '\0' )
   {
      if( !( location = find_location( ch, arg2 ) ) )
      {
         progbug_printf( ch, "Mptransfer - (%s) isn't a valid location", arg2 );
         return;
      }
   }
   else
      location = ch->in_room;

   if( !location )
   {
      progbug( "Mptransfer - no valid location", ch );
      return;
   }

   /* Put in the variable nextinroom to make this work right. -Narn */
   if( !str_cmp( arg1, "all" ) )
   {
      for( victim = ch->in_room->first_person; victim; victim = nextinroom )
      {
         nextinroom = victim->next_in_room;

         if( ch == victim )
            continue;

         transfer_char( ch, victim, location );
      }
      return;
   }

   /* This will only transfer PC's in the area not Mobs --Shaddai */
   if( !str_cmp( arg1, "area" ) )
   {
      for( d = first_descriptor; d; d = d->next )
      {
         if( !d->character || ( d->connected != CON_PLAYING &&  d->connected != CON_EDITING )
         || ch->in_room->area != d->character->in_room->area )
            continue;

         if( ch == d->character )
            continue;

         transfer_char( ch, d->character, location );
      }
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      progbug_printf( ch, "Mptransfer - (%s) doesn't exist", arg1 );
      return;
   }

   transfer_char( ch, victim, location );
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
CMDF( do_mpforce )
{
   char arg[MIL];

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      progbug( "Mpforce - Bad Usage", ch );
      return;
   }

   if( !str_cmp( arg, "all" ) )
   {
      CHAR_DATA *vch, *vch_next;

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;
         if( get_trust( vch ) < get_trust( ch ) && can_see( ch, vch ) )
            interpret( vch, argument );
      }
   }
   else
   {
      CHAR_DATA *victim;

      if( !( victim = get_char_room( ch, arg ) ) )
      {
         progbug( "Mpforce - No such victim", ch );
         return;
      }

      if( victim == ch )
      {
         progbug( "Mpforce - Forcing oneself", ch );
         return;
      }

      if( !is_npc( victim ) && ( !victim->desc ) && is_immortal( victim ) )
      {
         progbug( "Mpforce - Attempting to force link dead immortal", ch );
         return;
      }

      if( get_trust( victim ) <= PERM_IMM )
         interpret( victim, argument );
      else
         progbug( "Mpforce - Attempting to force an immortal! Cheater detected!", ch );
   }
}

/* mpmorph and mpunmorph for morphing people with mobs. --Shaddai */
CMDF( do_mpmorph )
{
   CHAR_DATA *victim;
   MORPH_DATA *morph;
   char arg1[MIL], arg2[MIL];

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      progbug( "Mpmorph - called w/o enough argument(s)", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "Victim must be in room.\r\n", ch );
      progbug( "Mpmorph: victim not in room", ch );
      return;
   }

   if( !is_number( arg2 ) )
      morph = get_morph( arg2 );
   else
      morph = get_morph_vnum( atoi( arg2 ) );
   if( !morph )
   {
      progbug( "Mpmorph - unknown morph", ch );
      return;
   }
   if( victim->morph )
   {
      progbug( "Mpmorph - victim already morphed", ch );
      return;
   }
   do_morph_char( victim, morph );
}

CMDF( do_mpunmorph )
{
   CHAR_DATA *victim;

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbug( "Mpmorph - called w/o an argument", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "Victim must be in room.\r\n", ch );
      progbug( "Mpunmorph: victim not in room", ch );
      return;
   }
   if( !victim->morph )
   {
      progbug( "Mpunmorph: victim not morphed", ch );
      return;
   }
   do_unmorph_char( victim );
}

CMDF( do_mpechozone )  /* Blod, late 97 */
{
   char arg1[MIL];
   CHAR_DATA *vch, *vch_next;
   short color;
   EXT_BV actflags;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   actflags = ch->act;
   xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   if( ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg1 );
   DONT_UPPER = true;
   for( vch = first_char; vch; vch = vch_next )
   {
      vch_next = vch->next;
      if( vch->in_room->area == ch->in_room->area && !is_npc( vch ) && is_awake( vch ) )
      {
         if( argument[0] == '\0' )
            act( AT_ACTION, " ", vch, NULL, NULL, TO_CHAR );
         else if( color )
            act( color, argument, vch, NULL, NULL, TO_CHAR );
         else
            act( AT_ACTION, argument, vch, NULL, NULL, TO_CHAR );
      }
   }
   DONT_UPPER = false;
   ch->act = actflags;
}

/*
 * Haus' toys follow:
 * Usage:  mppractice victim spell_name max%
 */
CMDF( do_mp_practice )
{
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MIL], log_buf[MSL];
   CHAR_DATA *victim;
   int sn, max, tmp, adept;
   char *fskill_name;

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' || arg3 == NULL || arg3[0] == '\0' )
   {
      send_to_char( "Mppractice: bad Usage", ch );
      progbug( "Mppractice - Bad Usage", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "Mppractice: Student not in room? Invis?", ch );
      progbug( "Mppractice: Invalid student not in room", ch );
      return;
   }

   if( ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      send_to_char( "Mppractice: Invalid spell/skill name", ch );
      progbug( "Mppractice: Invalid spell/skill name", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Mppractice: Can't train a mob", ch );
      progbug( "Mppractice: Can't train a mob", ch );
      return;
   }

   fskill_name = skill_table[sn]->name;

   if( ( ( max = atoi( arg3 ) ) < 0 ) || ( max > 100 ) )
   {
      snprintf( log_buf, sizeof( log_buf ), "%s: Invalid maxpercent: %d", __FUNCTION__, max );
      send_to_char( log_buf, ch );
      progbug( log_buf, ch );
      return;
   }

   if( !can_practice( victim, sn ) )
   {
      snprintf( buf, sizeof( buf ), "$n attempts to tutor you in %s, but it's beyond your comprehension.", fskill_name );
      act_tell( victim, ch, buf, ch, NULL, victim, TO_VICT );
      return;
   }

   /* adept is how high the player can learn it */
   adept = get_adept( victim, sn );

   if( ( victim->pcdata->learned[sn] >= adept ) || ( victim->pcdata->learned[sn] >= max ) )
   {
      snprintf( buf, sizeof( buf ), "$n shows some knowledge of %s, but yours is clearly superior.", fskill_name );
      act_tell( victim, ch, buf, ch, NULL, victim, TO_VICT );
      return;
   }

   tmp = UMIN( adept, max );
   act( AT_ACTION, "$N demonstrates $t to you.  You feel more learned in this subject.", victim, skill_table[sn]->name, ch,
        TO_CHAR );

   victim->pcdata->learned[sn] = tmp;

   if( victim->pcdata->learned[sn] >= adept )
   {
      victim->pcdata->learned[sn] = adept;
      act_tell( victim, ch, "$n tells you, 'You have learned all I know on this subject...'", ch, NULL, victim, TO_VICT );
   }
}

CMDF( do_mpscatter )
{
   char arg1[MSL], arg2[MSL];
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *pRoomIndex;
   int low_vnum, high_vnum, rvnum;

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Mpscatter whom?\r\n", ch );
      progbug( "Mpscatter: invalid (nonexistent?) argument", ch );
      return;
   }
   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "Victim must be in room.\r\n", ch );
      progbug( "Mpscatter: victim not in room", ch );
      return;
   }
   if( is_immortal( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You haven't the power to succeed against this victim.\r\n", ch );
      progbug( "Mpscatter: victim level too high", ch );
      return;
   }
   if( arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "You must specify a low vnum.\r\n", ch );
      progbug( "Mpscatter:  missing low vnum", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "You must specify a high vnum.\r\n", ch );
      progbug( "Mpscatter:  missing high vnum", ch );
      return;
   }
   if( ( low_vnum = atoi( arg2 ) ) < 1 || ( high_vnum = atoi( argument ) ) < low_vnum
   || low_vnum > high_vnum || low_vnum == high_vnum || high_vnum > MAX_VNUM )
   {
      send_to_char( "Invalid range.\r\n", ch );
      progbug( "Mpscatter:  invalid range", ch );
      return;
   }
   while( 1 )
   {
      rvnum = number_range( low_vnum, high_vnum );
      pRoomIndex = get_room_index( rvnum );
      if( pRoomIndex )
         break;
   }
   stop_fighting( victim, true );
   char_from_room( victim );
   char_to_room( victim, pRoomIndex );
   victim->position = POS_RESTING;
   do_look( victim, (char *)"auto" );
}

/* Usage: mpslay (character) */
CMDF( do_mp_slay )
{
   CHAR_DATA *victim;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "mpslay whom?\r\n", ch );
      progbug( "Mpslay: victim not specified", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "Victim must be in room.\r\n", ch );
      progbug( "Mpslay: victim not in room", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You try to slay yourself.  You fail.\r\n", ch );
      progbug( "Mpslay: trying to slay self", ch );
      return;
   }

   if( is_npc( victim ) && victim->pIndexData->vnum == MOB_VNUM_SUPERMOB )
   {
      send_to_char( "You can't slay supermob!\r\n", ch );
      progbug( "Mpslay: trying to slay supermob", ch );
      return;
   }

   if( get_trust( victim ) < PERM_IMM )
   {
      act( AT_IMMORT, "You slay $M in cold blood!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT );
      set_cur_char( victim );
      raw_kill( ch, victim );
      stop_fighting( ch, false );
      stop_hating( ch, victim, false );
      stop_fearing( ch, victim, false );
      stop_hunting( ch, victim, false );
   }
   else
   {
      act( AT_IMMORT, "You attempt to slay $M and fail!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n attempts to slay you.  What a kneebiter!", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n attempts to slay $N.  Needless to say $e fails.", ch, NULL, victim, TO_NOTVICT );
   }
}

MPDAMAGE_DATA *first_mpdamage, *last_mpdamage;

void free_mpdamage( MPDAMAGE_DATA *mpdam )
{
   if( !mpdam )
      return;
   STRFREE( mpdam->keyword );
   STRFREE( mpdam->charmsg );
   STRFREE( mpdam->roommsg );
   STRFREE( mpdam->victmsg );
   STRFREE( mpdam->immroom );
   STRFREE( mpdam->immchar );
   STRFREE( mpdam->immvict );
   STRFREE( mpdam->absroom );
   STRFREE( mpdam->abschar );
   STRFREE( mpdam->absvict );
   DISPOSE( mpdam );
}

void free_all_mpdamages( void )
{
   MPDAMAGE_DATA *mpdam, *mpdam_next;

   for( mpdam = first_mpdamage; mpdam; mpdam = mpdam_next )
   {
      mpdam_next = mpdam->next;
      UNLINK( mpdam, first_mpdamage, last_mpdamage, next, prev );
      free_mpdamage( mpdam );
   }
}

MPDAMAGE_DATA *get_mpdamage( char *keyword )
{
   MPDAMAGE_DATA *mpdam = NULL;

   if( !keyword || keyword[0] == '\0' )
      return NULL;

   for( mpdam = first_mpdamage; mpdam; mpdam = mpdam->next )
   {
      if( !str_cmp( mpdam->keyword, keyword ) )
         return mpdam;
   }

   return NULL;
}

void save_mpdamages( void )
{
   MPDAMAGE_DATA *mpdam;
   FILE *fp;
   int x;
   bool first = true;

   if( !first_mpdamage )
   {
      remove_file( MPDAMAGE_FILE );
      return;
   }
   if( !( fp = fopen( MPDAMAGE_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, MPDAMAGE_FILE );
      perror( MPDAMAGE_FILE );
      return;
   }
   for( mpdam = first_mpdamage; mpdam; mpdam = mpdam->next )
   {
      if( !mpdam->keyword )
         continue;
      fprintf( fp, "%s", "#MPDAMAGE\n" );
      fprintf( fp, "Keyword   %s~\n", mpdam->keyword );
      if( mpdam->charmsg )
         fprintf( fp, "CharMsg   %s~\n", mpdam->charmsg );
      if( mpdam->roommsg )
         fprintf( fp, "RoomMsg   %s~\n", mpdam->roommsg );
      if( mpdam->victmsg )
         fprintf( fp, "VictMsg   %s~\n", mpdam->victmsg );
      if( mpdam->immchar )
         fprintf( fp, "ImmChar   %s~\n", mpdam->immchar );
      if( mpdam->immroom )
         fprintf( fp, "ImmRoom   %s~\n", mpdam->immroom );
      if( mpdam->immvict )
         fprintf( fp, "ImmVict   %s~\n", mpdam->immvict );
      if( mpdam->abschar )
         fprintf( fp, "AbsChar   %s~\n", mpdam->abschar );
      if( mpdam->absroom )
         fprintf( fp, "AbsRoom   %s~\n", mpdam->absroom );
      if( mpdam->absvict )
         fprintf( fp, "AbsVict   %s~\n", mpdam->absvict );
      for( x = 0; x < RIS_MAX; x++ )
      {
         if( !mpdam->resistant[x] )
            continue;
         if( first )
            fprintf( fp, "%s", "Resistant " );
         first = false;
         fprintf( fp, "'%s'", ris_flags[x] );
      }
      if( !first )
         fprintf( fp, "%s", "~\n" );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

MPDAMAGE_DATA *new_mpdamage( void )
{
   MPDAMAGE_DATA *mpdam;
   int x;

   CREATE( mpdam, MPDAMAGE_DATA, 1 );
   if( !mpdam )
   {
      bug( "%s: mpdam is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   mpdam->victmsg = NULL;
   mpdam->charmsg = NULL;
   mpdam->keyword = NULL;
   mpdam->roommsg = NULL;
   mpdam->immroom = NULL;
   mpdam->immchar = NULL;
   mpdam->immvict = NULL;
   mpdam->absroom = NULL;
   mpdam->abschar = NULL;
   mpdam->absvict = NULL;
   mpdam->next = NULL;
   mpdam->prev = NULL;
   for( x = 0; x < RIS_MAX; x++ )
      mpdam->resistant[x] = false;
   return mpdam;
}

void fread_mpdamage( FILE *fp )
{
   MPDAMAGE_DATA *mpdam;
   const char *word;
   bool fMatch;

   mpdam = new_mpdamage( );

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;

      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "AbsRoom", mpdam->absroom, fread_string( fp ) );
            KEY( "AbsChar", mpdam->abschar, fread_string( fp ) );
            KEY( "AbsVict", mpdam->absvict, fread_string( fp ) );
            break;

         case 'C':
            KEY( "CharMsg", mpdam->charmsg, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               LINK( mpdam, first_mpdamage, last_mpdamage, next, prev );
               return;
	    }
	    break;

         case 'I':
            KEY( "ImmRoom", mpdam->immroom, fread_string( fp ) );
            KEY( "ImmChar", mpdam->immchar, fread_string( fp ) );
            KEY( "ImmVict", mpdam->immvict, fread_string( fp ) );
            break;

         case 'K':
            KEY( "Keyword", mpdam->keyword, fread_string( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Resistant" ) )
            {
               char *infoflags = fread_flagstring( fp );
               char res[MSL];
               int value;

               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, res );
                  value = get_flag( res, ris_flags, RIS_MAX );
                  if( value < 0 || value >= RIS_MAX )
                     bug( "%s: Unknown %s: %s", __FUNCTION__, word, res );
                  else
                     mpdam->resistant[value] = true;
               }
               fMatch = true;
               break;
            }
            KEY( "RoomMsg", mpdam->roommsg, fread_string( fp ) );
            break;

         case 'V':
            KEY( "VictMsg", mpdam->victmsg, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_mpdamage( mpdam );
}

void load_mpdamages( void )
{
   FILE *fp;

   first_mpdamage = last_mpdamage = NULL;
   if( !( fp = fopen( MPDAMAGE_FILE, "r" ) ) )
      return;
   for( ;; )
   {
      char letter;
      char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      if( letter != '#' )
      {
         bug( "%s: # not found.", __FUNCTION__ );
         break;
      }
      word = fread_word( fp );
      if( !str_cmp( word, "MPDAMAGE" ) )
      {
         fread_mpdamage( fp );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section (%s).", __FUNCTION__, word );
         fread_to_eol( fp );
         continue;
      }
   }
   fclose( fp );
   fp = NULL;
}

void send_setmpdamage_syntax( CHAR_DATA *ch )
{
   if( !ch )
      return;
   send_to_char( "Usage: setmpdamage save\r\n", ch );
   send_to_char( "Usage: setmpdamage create <keyword>\r\n", ch );
   send_to_char( "Usage: setmpdamage <keyword> delete\r\n", ch );
   send_to_char( "Usage: setmpdamage <keyword> resistant <resistant>\r\n", ch );
   send_to_char( "Usage: setmpdamage <keyword> <msgtype> <new message>\r\n", ch );
   send_to_char( "MsgTypes: charmsg roommsg victmsg immroom immchar immvict absroom abschar absvict\r\n", ch );
}

/* Dynamic mpdamage messages */
CMDF( do_setmpdamage )
{
   MPDAMAGE_DATA *mpdam = NULL;
   char arg1[MIL];
   int count = 0;
   bool found = false;

   if( is_npc( ch ) || !is_immortal( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      for( mpdam = first_mpdamage; mpdam; mpdam = mpdam->next )
      {
         if( !found )
            send_to_char( "MpDamages:\r\n", ch );
         found = true;
         ch_printf( ch, "%20s", mpdam->keyword );
         if( ++count == 4 )
         {
            count = 0;
            send_to_char( "\r\n", ch );
         }
      }
      if( count != 0 )
         send_to_char( "\r\n", ch );
      send_setmpdamage_syntax( ch );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_mpdamages( );
      send_to_char( "The mpdamages have been saved.\r\n", ch );
      return;
   }

   if( !str_cmp( arg1, "create" ) )
   {
      if( get_mpdamage( argument ) )
      {
         send_to_char( "There is already an mpdamage using that keyword.\r\n", ch );
         return;
      }
      mpdam = new_mpdamage( );
      mpdam->keyword = STRALLOC( argument );
      LINK( mpdam, first_mpdamage, last_mpdamage, next, prev );
      ch_printf( ch, "New mpdamage %s created.\r\n", mpdam->keyword );
      return;
   }

   if( !( mpdam = get_mpdamage( arg1 ) ) )
   {
      send_to_char( "There is no such mpdamage to modify.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      int x;

      ch_printf( ch, "Keyword: %s\r\n", mpdam->keyword );
      ch_printf( ch, "RoomMsg: %s\r\n", mpdam->roommsg ? mpdam->roommsg : "Not Set" );
      ch_printf( ch, "CharMsg: %s\r\n", mpdam->charmsg ? mpdam->charmsg : "Not Set" );
      ch_printf( ch, "VictMsg: %s\r\n", mpdam->victmsg ? mpdam->victmsg : "Not Set" );
      ch_printf( ch, "ImmRoom: %s\r\n", mpdam->immroom ? mpdam->immroom : "Not Set" );
      ch_printf( ch, "ImmChar: %s\r\n", mpdam->immchar ? mpdam->immchar : "Not Set" );
      ch_printf( ch, "ImmVict: %s\r\n", mpdam->immvict ? mpdam->immvict : "Not Set" );
      ch_printf( ch, "AbsRoom: %s\r\n", mpdam->absroom ? mpdam->absroom : "Not Set" );
      ch_printf( ch, "AbsChar: %s\r\n", mpdam->abschar ? mpdam->abschar : "Not Set" );
      ch_printf( ch, "AbsVict: %s\r\n", mpdam->absvict ? mpdam->absvict : "Not Set" );
      for( x = 0; x < RIS_MAX; x++ )
      {
         if( !mpdam->resistant[x] )
            continue;
         ch_printf( ch, "Resistant %s is checked.\r\n", ris_flags[x] );
      }
      return;
   }

   if( !str_cmp( arg1, "delete" ) )
   {
      UNLINK( mpdam, first_mpdamage, last_mpdamage, next, prev );
      free_mpdamage( mpdam );
      send_to_char( "That mpdamage has been deleted.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "resistant" ) )
   {
      int value;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "What resistant(s) would you like to set/unset?\r\n", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         value = get_flag( arg1, ris_flags, RIS_MAX );
         if( value < 0 || value >= RIS_MAX )
            ch_printf( ch, "%s isn't a valid resistant.\r\n", arg1 );
         else
         {
            mpdam->resistant[value] = !mpdam->resistant[value];
            ch_printf( ch, "%s has been %s.\r\n", arg1, mpdam->resistant[value] ? "set" : "removed" );
         }
      }
      return;
   }
   if( !str_cmp( arg1, "immchar" ) )
   {
      STRSET( mpdam->immchar, argument );
      ch_printf( ch, "That immchar has been set to %s\r\n", mpdam->immchar ? mpdam->immchar : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "immroom" ) )
   {
      STRSET( mpdam->immroom, argument );
      ch_printf( ch, "That immroom has been set to %s\r\n", mpdam->immroom ? mpdam->immroom : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "immvict" ) )
   {
      STRSET( mpdam->immvict, argument );
      ch_printf( ch, "That immvict has been set to %s\r\n", mpdam->immvict ? mpdam->immvict : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "abschar" ) )
   {
      STRSET( mpdam->abschar, argument );
      ch_printf( ch, "That abschar has been set to %s\r\n", mpdam->abschar ? mpdam->abschar : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "absroom" ) )
   {
      STRSET( mpdam->absroom, argument );
      ch_printf( ch, "That absroom has been set to %s\r\n", mpdam->absroom ? mpdam->absroom : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "absvict" ) )
   {
      STRSET( mpdam->absvict, argument );
      ch_printf( ch, "That absvict has been set to %s\r\n", mpdam->absvict ? mpdam->absvict : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "charmsg" ) )
   {
      STRSET( mpdam->charmsg, argument );
      ch_printf( ch, "That charmsg has been set to %s\r\n", mpdam->charmsg ? mpdam->charmsg : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "roommsg" ) )
   {
      STRSET( mpdam->roommsg, argument );
      ch_printf( ch, "That roommsg has been set to %s\r\n", mpdam->roommsg ? mpdam->roommsg : "Not Set" );
      return;
   }
   if( !str_cmp( arg1, "victmsg" ) )
   {
      STRSET( mpdam->victmsg, argument );
      ch_printf( ch, "That victmsg has been set to %s\r\n", mpdam->victmsg ? mpdam->victmsg : "Not Set" );
      return;
   }
}

void handle_mp_damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, MPDAMAGE_DATA *mpdam )
{
   int x, rismod = 0;
   char *charmsg, *roommsg, *victmsg;

   if( !ch || !victim || dam < 0 || dam > 32000 || ch == victim )
      return;

   /* Ok lets take into account resistants */
   if( mpdam )
   {
      for( x = 0; x < RIS_MAX; x++ )
      {
         if( !mpdam->resistant[x] )
            continue;
         rismod += ris_damage( victim, dam, x );
      }
      if( rismod == 100 )
         dam = 0;
      else if( rismod > 100 )
      {
         if( dam > 0 )
         {
            double risper =  ( dam * ( ( rismod - 100 ) * .01 ) );
            int hchange = ( int )risper;

            if( hchange > 0 )
            {
               victim->hit = URANGE( 0, victim->hit + hchange, victim->max_hit );
               update_pos( victim );
            }
         }
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

   if( dam > 0 )
   {
      if( damage( ch, victim, NULL, dam, TYPE_UNDEFINED, true ) == rVICT_DIED )
      {
         stop_fighting( ch, false );
         stop_hating( ch, victim, false );
         stop_fearing( ch, victim, false );
         stop_hunting( ch, victim, false );
      }
   }

   /* Send the message after damage so they see the change */
   if( mpdam )
   {
      if( rismod > 100 ) /* If its absorbing try absorb, then immune then finaly normal */
      {
         charmsg = mpdam->abschar ? mpdam->abschar : mpdam->immchar ? mpdam->immchar : mpdam->charmsg;
         roommsg = mpdam->absroom ? mpdam->absroom : mpdam->immroom ? mpdam->immroom : mpdam->roommsg;
         victmsg = mpdam->absvict ? mpdam->absvict : mpdam->immvict ? mpdam->immvict : mpdam->victmsg;
      }
      else if( rismod == 100 ) /* If immune try immune then normal */
      {
         charmsg = mpdam->immchar ? mpdam->immchar : mpdam->charmsg;
         roommsg = mpdam->immroom ? mpdam->immroom : mpdam->roommsg;
         victmsg = mpdam->immvict ? mpdam->immvict : mpdam->victmsg;
      }
      else /* Try normal */
      {
         charmsg = mpdam->charmsg;
         roommsg = mpdam->roommsg;
         victmsg = mpdam->victmsg;
      }

      /* Now send if something to send */
      if( charmsg )
         act( AT_HIT, charmsg, ch, NULL, victim, TO_CHAR );
      if( roommsg )
         act( AT_HIT, roommsg, ch, NULL, victim, TO_NOTVICT );
      if( victmsg )
         act( AT_HITME, victmsg, ch, NULL, victim, TO_VICT );
   }
}

/* Usage: mpdamage (character) (#hps) */
CMDF( do_mp_damage )
{
   CHAR_DATA *victim, *nextinroom;
   MPDAMAGE_DATA *mpdam = NULL;
   char arg1[MIL], arg2[MIL];
   int dam;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "mpdamage whom?\r\n", ch );
      progbug( "Mpdamage: invalid victim", ch );
      return;
   }

   if( arg2 != NULL && arg2[0] != '\0' && !is_number( arg2 ) )
   {
      if( !( mpdam = get_mpdamage( arg2 ) ) )
      {
         ch_printf( ch, "mpdamage: unknown mpdamage %s.\r\n", arg2 );
         progbug( "Mpdamage: invalid mpdamage keyword.\r\n", ch );
         return;
      }

      argument = one_argument( argument, arg2 );
   }

   if( arg2 == NULL || arg2[0] == '\0' || !is_number( arg2 ) )
   {
      send_to_char( "mpdamage inflict how many hps?\r\n", ch );
      progbug( "Mpdamage: invalid damage", ch );
      return;
   }

   if( ( dam = atoi( arg2 ) ) <= 0 || dam > 32000 )
   {
      send_to_char( "Mpdamage how much?\r\n", ch );
      progbug( "Mpdamage: invalid (nonexistent?) damage", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      if( !str_cmp( arg1, "all" ) )
      {
         for( victim = ch->in_room->first_person; victim; victim = nextinroom )
         {
            nextinroom = victim->next_in_room;
            if( victim != ch && can_see( ch, victim ) )  /* Could go either way */
               handle_mp_damage( ch, victim, dam, mpdam );
         }
         return;
      }
      else
      {
         send_to_char( "Victim must be in room.\r\n", ch );
         progbug( "Mpdamage: victim not in room", ch );
         return;
      }
   }

   if( victim == ch )
   {
      send_to_char( "You can't mpdamage yourself.\r\n", ch );
      progbug( "Mpdamage: trying to damage self", ch );
      return;
   }

   handle_mp_damage( ch, victim, dam, mpdam );
}

CMDF( do_mp_log )
{
   char buf[MSL];

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbug( "Mp_log:  non-existent entry", ch );
      return;
   }
   snprintf( buf, sizeof( buf ), "%s  %s:  %s &D", distime( current_time ), ch->short_descr, argument );
   append_to_file( MOBLOG_FILE, buf );
}

/* Usage: mprestore (character) (#hps) Gorog */
CMDF( do_mp_restore )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   int hp;

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "mprestore whom?\r\n", ch );
      progbug( "Mprestore: invalid argument1", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "mprestore how many hps?\r\n", ch );
      progbug( "Mprestore: invalid argument2", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "Victim must be in room.\r\n", ch );
      progbug( "Mprestore: victim not in room", ch );
      return;
   }

   if( ( ( hp = atoi( arg2 ) ) < 0 ) || ( hp > 32000 ) )
   {
      send_to_char( "Mprestore how much?\r\n", ch );
      progbug( "Mprestore: invalid (nonexistent?) argument", ch );
      return;
   }
   hp += victim->hit;
   victim->hit = ( hp > 32000 || hp < 0 || hp > victim->max_hit ) ? victim->max_hit : hp;
}

/*
 * Usage mpfavor target number
 * Raise a player's favor in progs.
 */
CMDF( do_mpfavor )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   int favor;
   char *tmp;
   bool plus = false, minus = false;

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "mpfavor whom?\r\n", ch );
      progbug( "Mpfavor: invalid argument1", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "mpfavor how much favor?\r\n", ch );
      progbug( "Mpfavor: invalid argument2", ch );
      return;
   }

   tmp = arg2;
   if( tmp[0] == '+' )
   {
      plus = true;
      tmp++;
      if( tmp[0] == '\0' )
      {
         send_to_char( "mpfavor how much favor?\r\n", ch );
         progbug( "Mpfavor: invalid argument2", ch );
         return;
      }
   }
   else if( tmp[0] == '-' )
   {
      minus = true;
      tmp++;
      if( tmp[0] == '\0' )
      {
         send_to_char( "mpfavor how much favor?\r\n", ch );
         progbug( "Mpfavor: invalid argument2", ch );
         return;
      }
   }
   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "Victim must be in room.\r\n", ch );
      progbug( "Mpfavor: victim not in room", ch );
      return;
   }

   favor = atoi( tmp );
   if( plus )
      victim->pcdata->favor = URANGE( -2500, victim->pcdata->favor + favor, 2500 );
   else if( minus )
      victim->pcdata->favor = URANGE( -2500, victim->pcdata->favor - favor, 2500 );
   else
      victim->pcdata->favor = URANGE( -2500, favor, 2500 );
}

/*
 * Usage mp_open_passage x y z
 * opens a 1-way passage from room x to room y in direction z
 * won't mess with existing exits
 */
CMDF( do_mp_open_passage )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   ROOM_INDEX_DATA *targetRoom, *fromRoom;
   int targetRoomVnum, fromRoomVnum, exit_num;
   EXIT_DATA *pexit;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' || arg3 == NULL || arg3[0] == '\0' )
   {
      progbug( "MpOpenPassage - Bad Usage", ch );
      return;
   }

   if( !is_number( arg1 ) )
   {
      progbug( "MpOpenPassage - Bad Usage", ch );
      return;
   }

   fromRoomVnum = atoi( arg1 );
   if( !( fromRoom = get_room_index( fromRoomVnum ) ) )
   {
      progbug( "MpOpenPassage - Bad Usage", ch );
      return;
   }

   if( !is_number( arg2 ) )
   {
      progbug( "MpOpenPassage - Bad Usage", ch );
      return;
   }

   targetRoomVnum = atoi( arg2 );
   if( !( targetRoom = get_room_index( targetRoomVnum ) ) )
   {
      progbug( "MpOpenPassage - Bad Usage", ch );
      return;
   }

   if( !is_number( arg3 ) )
      exit_num = get_dir( arg3 );
   else
      exit_num = atoi( arg3 );
   if( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) )
   {
      progbug( "MpOpenPassage - Bad Usage", ch );
      return;
   }

   if( ( pexit = get_exit( fromRoom, exit_num ) ) )
   {
      if( !xIS_SET( pexit->exit_info, EX_PASSAGE ) )
         return;
      progbug( "MpOpenPassage - Exit exists", ch );
      return;
   }

   pexit = make_exit( fromRoom, targetRoom, exit_num );
   pexit->keyword = NULL;
   pexit->description = NULL;
   pexit->key = -1;
   xSET_BIT( pexit->exit_info, EX_PASSAGE );
}

/* Usage mp_fillin x. Simply closes the door */
CMDF( do_mp_fill_in )
{
   EXIT_DATA *pexit;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !( pexit = find_door( ch, argument, true ) ) )
   {
      progbug( "MpFillIn - Exit does not exist", ch );
      return;
   }
   xSET_BIT( pexit->exit_info, EX_CLOSED );
}

/*
 * Usage mp_close_passage x y 
 * closes a passage in room x leading in direction y
 * the exit must have EX_PASSAGE set
 */
CMDF( do_mp_close_passage )
{
   char arg1[MIL], arg2[MIL];
   ROOM_INDEX_DATA *fromRoom;
   int exit_num;
   EXIT_DATA *pexit;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      progbug( "MpClosePassage - No first argument or second argument", ch );
      return;
   }

   if( !is_number( arg1 ) )
   {
      progbug( "MpClosePassage - first argument isn't a number", ch );
      return;
   }

   if( !( fromRoom = get_room_index( atoi( arg1 ) ) ) )
   {
      progbug_printf( ch, "MpClosePassage - invalid room vnum [%s]", arg1 );
      return;
   }

   if( !is_number( arg2 ) )
      exit_num = get_dir( arg2 );
   else
      exit_num = atoi( arg2 );
   if( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) )
   {
      progbug( "MpClosePassage - invalid exit direction", ch );
      return;
   }

   if( !( pexit = get_exit( fromRoom, exit_num ) ) )
      return;

   if( !xIS_SET( pexit->exit_info, EX_PASSAGE ) )
   {
      progbug( "MpClosePassage - Exit not a passage", ch );
      return;
   }

   extract_exit( fromRoom, pexit );
}

/* Does nothing.  Used for scripts. */
CMDF( do_mpnothing )
{
   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
      send_to_char( "Huh?\r\n", ch );
}

/* Sends a message to sleeping character.  Should be fun with room sleep_progs */
CMDF( do_mpdream )
{
   CHAR_DATA *vict;

   if( !is_npc( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !( vict = get_char_world( ch, argument ) ) )
   {
      progbug( "Mpdream: No such character", ch );
      return;
   }

   if( vict->position <= POS_SLEEPING )
      ch_printf( vict, "%s\r\n", argument );
}

CMDF( do_mpdelay )
{
   char arg[MIL];
   CHAR_DATA *victim;
   int delay;

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Delay for how many rounds?n\r", ch );
      progbug( "Mpdelay: no duration specified", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      progbug( "Mpdelay: target not in room", ch );
      return;
   }

   if( is_immortal( victim ) )
   {
      send_to_char( "Not against immortals.\r\n", ch );
      progbug( "Mpdelay: target is immortal", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !*arg || !is_number( arg ) )
   {
      send_to_char( "Delay them for how many rounds?\r\n", ch );
      progbug( "Mpdelay: invalid (nonexistant?) argument", ch );
      return;
   }

   if( ( delay = atoi( arg ) ) < 1 || delay > 30 )
   {
      send_to_char( "Argument out of range.\r\n", ch );
      progbug( "Mpdelay:  argument out of range (1 to 30)", ch );
      return;
   }

   wait_state( victim, delay * PULSE_VIOLENCE );
   send_to_char( "Mpdelay applied.\r\n", ch );
}

CMDF( do_mppeace )
{
   CHAR_DATA *rch, *victim;

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Who do you want to mppeace?\r\n", ch );
      progbug( "Mppeace: invalid (nonexistent?) argument", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
      {
         if( rch->fighting )
         {
            stop_fighting( rch, true );
            do_sit( rch, (char *)"" );
         }
         stop_hating( rch, NULL, true );
         stop_hunting( rch, NULL, true );
         stop_fearing( rch, NULL, true );
      }
      send_to_char( "Ok.\r\n", ch );
      return;
   }
   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They must be in the room.n\r", ch );
      progbug( "Mppeace: target not in room", ch );
      return;
   }
   stop_fighting( victim, true );
   stop_hating( ch, NULL, true );
   stop_hunting( ch, NULL, true );
   stop_fearing( ch, NULL, true );
   stop_hating( victim, NULL, true );
   stop_hunting( victim, NULL, true );
   stop_fearing( victim, NULL, true );
   send_to_char( "Ok.\r\n", ch );
}

CMDF( do_mppkset )
{
   CHAR_DATA *victim;
   char arg[MSL];

   if( !is_npc( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      progbug( "Mppkset - bad Usage", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbug( "Mppkset - no such player in room.", ch );
      return;
   }

   if( !str_cmp( argument, "yes" ) || !str_cmp( argument, "y" ) )
   {
      if( !xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
         xSET_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
   }
   else if( !str_cmp( argument, "no" ) || !str_cmp( argument, "n" ) )
   {
      if( xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
         xREMOVE_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
   }
   else
   {
      progbug( "Mppkset - bad Usage", ch );
      return;
   }
}
