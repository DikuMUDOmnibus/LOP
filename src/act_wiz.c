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
 *                         Wizard/god command module                         *
 *****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "h/mud.h"
#include "h/sha256.h"

#define RESTORE_INTERVAL 21600

void remove_from_highscores( const char *name );
void rename_in_banks( const char *oplayer, const char *nplayer );
void show_track_resets( CHAR_DATA *ch, OBJ_INDEX_DATA *oindex, MOB_INDEX_DATA *mindex );

/*
 * Toggle "Do Not Disturb" flag. Used to prevent lower level imms from
 * using commands like "trans" and "goto" on higher level imms.
 */
CMDF( do_dnd )
{
   if( is_npc( ch ) || !ch->pcdata )
   {
      send_to_char( "huh?\r\n", ch );
      return;
   }
   xTOGGLE_BIT( ch->pcdata->flags, PCFLAG_DND );
   ch_printf( ch, "Your 'do not disturb' flag is now %s.\r\n",
      xIS_SET( ch->pcdata->flags, PCFLAG_DND ) ? "on" : "off" );
}

CMDF( do_bamfin )
{
   if( is_npc( ch ) )
      return;
   set_char_color( AT_IMMORT, ch );
   if( argument && argument[0] != '\0' && !nifty_is_name( ch->name, argument ) )
   {
      send_to_char( "Your bamfin must have your name in it somewhere.\r\n", ch );
      return;
   }
   STRSET( ch->pcdata->bamfin, argument );
   if( !ch->pcdata->bamfin ||  ch->pcdata->bamfin[0] == '\0' )
      send_to_char( "Bamfin set to nothing.\r\n", ch );
   else
      send_to_char( "Bamfin set.\r\n", ch );
}

CMDF( do_bamfout )
{
   if( is_npc( ch ) )
      return;
   set_char_color( AT_IMMORT, ch );
   if( argument && argument[0] != '\0' && !nifty_is_name( ch->name, argument ) )
   {
      send_to_char( "Your bamfout must have your name in it somewhere.\r\n", ch );
      return;
   }
   STRSET( ch->pcdata->bamfout, argument );
   if( !ch->pcdata->bamfout || ch->pcdata->bamfout[0] == '\0' )
      send_to_char( "Bamfout set to nothing.\r\n", ch );
   else
      send_to_char( "Bamfout set.\r\n", ch );
}

CMDF( do_rank )
{
   if( is_npc( ch ) )
      return;
   set_char_color( AT_IMMORT, ch );
   STRSET( ch->pcdata->rank, argument );
   if( !ch->pcdata->rank || ch->pcdata->rank[0] == '\0' )
      send_to_char( "Rank set to nothing.\r\n", ch );
   else
      send_to_char( "Rank set.\r\n", ch );
}

CMDF( do_retire )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Retire whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You can't succeed against them.\r\n", ch );
      return;
   }
   if( get_trust( victim ) < PERM_LEADER )
   {
      send_to_char( "The minimum level for retirement is leader.\r\n", ch );
      return;
   }
   xTOGGLE_BIT( victim->pcdata->flags, PCFLAG_RETIRED );
   if( is_retired( victim ) )
   {
      ch_printf( ch, "%s is now a retired immortal.\r\n", victim->name );
      ch_printf( victim, "Courtesy of %s, you're now a retired immortal.\r\n", ch->name );
   }
   else
   {
      ch_printf( ch, "%s returns from retirement.\r\n", victim->name );
      ch_printf( victim, "%s brings you back from retirement.\r\n", ch->name );
   }
}

CMDF( do_guest )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Whom would you like to make a guest?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You can't succeed against them.\r\n", ch );
      return;
   }
   xTOGGLE_BIT( victim->pcdata->flags, PCFLAG_GUEST );
   if( is_guest( victim ) )
   {
      ch_printf( ch, "%s is now a guest immortal.\r\n", victim->name );
      ch_printf( victim, "Courtesy of %s, you're now a guest immortal.\r\n", ch->name );
   }
   else
   {
      ch_printf( ch, "%s is no longer a guest.\r\n", victim->name );
      ch_printf( victim, "%s stops you from being a guest.\r\n", ch->name );
   }
}

CMDF( do_delay )
{
   CHAR_DATA *victim;
   char arg[MIL];
   int delay;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage:  delay <victim> <# of rounds>\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "No such character online.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Mobiles are unaffected by lag.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You haven't the power to succeed against them.\r\n", ch );
      return;
   }
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "For how long do you wish to delay them?\r\n", ch );
      return;
   }
   if( ( delay = atoi( arg ) ) < 0 || delay > 999 )
   {
      send_to_char( "Delay range is 0 to 999.\r\n", ch );
      return;
   }
   wait_state( victim, delay * PULSE_VIOLENCE );
   ch_printf( ch, "You've delayed %s for %d rounds.\r\n", victim->name, delay );
}

CMDF( do_deny )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Deny whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }
   xSET_BIT( victim->act, PLR_DENY );
   set_char_color( AT_IMMORT, victim );
   send_to_char( "You're denied access!\r\n", victim );
   ch_printf( ch, "You have denied access to %s.\r\n", victim->name );
   stop_fighting( victim, true );
   do_quit( victim, (char *)"" );
}

CMDF( do_disconnect )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Disconnect whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( !victim->desc )
   {
      act( AT_PLAIN, "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
      return;
   }
   if( get_trust( ch ) <= get_trust( victim ) )
   {
      send_to_char( "They might not like that...\r\n", ch );
      return;
   }
   close_socket( victim->desc, false );
   send_to_char( "Ok.\r\n", ch );
}

/* Force a level one player to quit. Gorog */
CMDF( do_fquit )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Force whom to quit?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( victim->level != 1 )
   {
      send_to_char( "They aren't level one!\r\n", ch );
      return;
   }
   set_char_color( AT_IMMORT, victim );
   send_to_char( "The MUD administrators force you to quit...\r\n", victim );
   stop_fighting( victim, true );
   do_quit( victim, (char *)"" );
   ch_printf( ch, "You have forced %s to quit.\r\n", victim->name );
}

CMDF( do_forceclose )
{
   DESCRIPTOR_DATA *d;
   int desc;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' || !is_number( argument ) )
   {
      send_to_char( "Usage: forceclose <descriptor #>\r\n", ch );
      return;
   }

   desc = atoi( argument );
   for( d = first_descriptor; d; d = d->next )
   {
      if( d->descriptor == desc )
      {
         if( d->character && get_trust( d->character ) >= get_trust( ch ) )
         {
            send_to_char( "They might not like that...\r\n", ch );
            return;
         }
         close_socket( d, false );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
   }
   send_to_char( "Not found!\r\n", ch );
}

void echo_to_all( short AT_COLOR, const char *argument, short tar )
{
   DESCRIPTOR_DATA *d;

   if( !argument || argument[0] == '\0' )
      return;

   for( d = first_descriptor; d; d = d->next )
   {
      if( d->connected == CON_PLAYING || d->connected == CON_EDITING )
      {
         if( tar == ECHOTAR_PC && is_npc( d->character ) )
            continue;
         else if( tar == ECHOTAR_IMM && !is_immortal( d->character ) )
            continue;
         set_char_color( AT_COLOR, d->character );
         ch_printf( d->character, "%s\r\n", argument );
      }
   }
}

void echo_to_all_printf( short AT_COLOR, short tar, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   echo_to_all( AT_COLOR, buf, tar );
}

CMDF( do_echo )
{
   char *parg;
   char arg[MIL];
   int target;
   short color;

   set_char_color( AT_IMMORT, ch );

   if( xIS_SET( ch->act, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't do that right now.\r\n", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Echo what?\r\n", ch );
      return;
   }

   if( ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg );
   parg = argument;
   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "pc" ) )
      target = ECHOTAR_PC;
   else if( !str_cmp( arg, "imm" ) )
      target = ECHOTAR_IMM;
   else
   {
      target = ECHOTAR_ALL;
      argument = parg;
   }
   if( !color && ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg );
   if( !color )
      color = AT_IMMORT;
   one_argument( argument, arg );
   echo_to_all( color, argument, target );
}

void echo_to_room( short AT_COLOR, ROOM_INDEX_DATA *room, char *argument )
{
   CHAR_DATA *vic;

   for( vic = room->first_person; vic; vic = vic->next_in_room )
   {
      set_char_color( AT_COLOR, vic );
      send_to_char( argument, vic );
      send_to_char( "\r\n", vic );
   }
}

CMDF( do_recho )
{
   char arg[MIL];
   short color;

   set_char_color( AT_IMMORT, ch );

   if( xIS_SET( ch->act, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't do that right now.\r\n", ch );
      return;
   }
   if( argument[0] == '\0' )
   {
      send_to_char( "Recho what?\r\n", ch );
      return;
   }
   one_argument( argument, arg );
   if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg );
      echo_to_room( color, ch->in_room, argument );
   }
   else
      echo_to_room( AT_IMMORT, ch->in_room, argument );
}

ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   loc_cords[0] = 0;
   loc_cords[1] = 0;
   loc_in_wilderness = false;

   if( is_number( arg ) )
      return get_room_index( atoi( arg ) );

   if( ( victim = get_char_world( ch, arg ) ) )
   {
      if( is_in_wilderness( victim ) )
      {
         loc_in_wilderness = true;
         loc_cords[0] = victim->cords[0];
         loc_cords[1] = victim->cords[1];
      }
      return victim->in_room;
   }

   if( ( obj = get_obj_world( ch, arg ) ) )
   {
      if( is_obj_stat( obj, ITEM_WILDERNESS ) && obj->in_room && xIS_SET( obj->in_room->room_flags, ROOM_WILDERNESS ) )
      {
         loc_in_wilderness = true;
         loc_cords[0] = obj->cords[0];
         loc_cords[1] = obj->cords[1];
      }
      return obj->in_room;
   }

   return NULL;
}

/*
 * This function shared by do_transfer and do_mptransfer
 *
 * Immortals bypass most restrictions on where to transfer victims.
 * NPCs can't transfer victims who are:
 * 1. Outside of the level range for the target room's area.
 * 2. Being sent to private rooms.
 */
void transfer_char( CHAR_DATA *ch, CHAR_DATA *victim, ROOM_INDEX_DATA *location )
{
   if( !victim->in_room )
   {
      bug( "%s: victim in NULL room: %s", __FUNCTION__, victim->name );
      return;
   }

   if( is_npc( ch ) && room_is_private( location ) )
   {
      progbug( "Mptransfer - Private room", ch );
      return;
   }

   if( !can_see( ch, victim ) )
      return;

   /* If victim not in area's level range, do not transfer */
   if( is_npc(ch) && !in_hard_range( victim, location->area ) )
      return;

   stop_fighting( victim, true );

   if( !is_npc(ch) )
   {
      act( AT_MAGIC, "$n disappears in a cloud of swirling colors.", victim, NULL, NULL, TO_ROOM );
      victim->retran = victim->in_room->vnum;
   }
   char_from_room( victim );
   char_to_room( victim, location );
   set_loc_cords( victim );
   if( !is_npc(ch) )
   {
      act( AT_MAGIC, "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
      if( ch != victim )
         act( AT_IMMORT, "$n has transferred you.", ch, NULL, victim, TO_VICT );
      do_look( victim, (char *)"auto" );
      if( !is_immortal( victim ) && !is_npc( victim ) && !in_hard_range( victim, location->area ) )
         act( AT_DANGER, "Warning:  this player's level is not within the area's level range.", ch, NULL, NULL, TO_CHAR );
   }
}

CMDF( do_transfer )
{
   ROOM_INDEX_DATA *location;
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;
   char arg1[MIL], arg2[MIL];

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Transfer whom (and where)?\r\n", ch );
      return;
   }

   loc_in_wilderness = false;

   if( arg2 != NULL && arg2[0] != '\0' )
   {
      if( !( location = find_location( ch, arg2 ) ) )
      {
         send_to_char( "That location does not exist.\r\n", ch );
         return;
      }
   }
   else
   {
      location = ch->in_room;
      if( is_in_wilderness( ch ) )
      {
         loc_in_wilderness = true;
         loc_cords[0] = ch->cords[0];
         loc_cords[1] = ch->cords[1];
      }
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      if( !str_cmp( arg1, "all" ) && get_trust( ch ) >= PERM_HEAD )
      {
         for( d = first_descriptor; d; d = d->next )
         {
            if( d->connected != CON_PLAYING || !d->character || d->character == ch )
               continue;
            if( !is_npc( d->character ) )
            {
               if( get_trust( ch ) < get_trust( d->character ) )
                  continue;
               if( xIS_SET( d->character->pcdata->flags, PCFLAG_DND ) )
                  continue;
            }
            transfer_char( ch, d->character, location );
         }
      }
      else
         send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( !is_npc( victim ) && get_trust( ch ) < get_trust( victim ) )
   {
      send_to_char( "You should ask them to come to you instead of transfering them around.\r\n", ch );
      return;
   }

   if( !is_npc( victim ) && get_trust( ch ) < get_trust( victim ) && victim->desc
   && ( victim->desc->connected == CON_PLAYING || victim->desc->connected == CON_EDITING )
   && xIS_SET( victim->pcdata->flags, PCFLAG_DND ) )
   {
      pager_printf( ch, "Sorry. %s does not wish to be disturbed currently.\r\n", victim->name );
      pager_printf( victim, "Your DND flag just foiled %s's transfer command.\r\n", ch->name );
      return;
   }

   transfer_char( ch, victim, location );
}

CMDF( do_retran )
{
   CHAR_DATA *victim;
   char buf[MSL];

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Retransfer whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( !victim->retran )
   {
      send_to_char( "No place to retransfer them to.\r\n", ch );
      return;
   }
   snprintf( buf, sizeof( buf ), "'%s' %d", victim->name, victim->retran );
   do_transfer( ch, buf );
}

CMDF( do_regoto )
{
   char buf[MSL];

   if( !ch->regoto )
   {
      send_to_char( "No place to go back to.\r\n", ch );
      return;
   }
   snprintf( buf, sizeof( buf ), "%d", ch->regoto );
   do_goto( ch, buf );
}

/*  Added do_at and do_atobj to reduce lag associated with at  --Shaddai */
CMDF( do_at )
{
   ROOM_INDEX_DATA *location = NULL, *original;
   CHAR_DATA *wch = NULL, *victim;
   char arg[MIL];
   short oldx = 0, oldy = 0;
   bool wasinwild = false;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || argument == NULL || arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "At where what?\r\n", ch );
      return;
   }
   if( is_number( arg ) )
      location = get_room_index( atoi( arg ) );
   else if( !( wch = get_char_world( ch, arg ) ) || !wch->in_room )
   {
      send_to_char( "No such mobile or player in existance.\r\n", ch );
      return;
   }
   if( !location && wch )
      location = wch->in_room;

   if( !location )
   {
      send_to_char( "No such location exists.\r\n", ch );
      return;
   }

   /*
    * The following mod is used to prevent players from using the 
    * at command on a higher level immortal who has a DND flag    
    */
   if( wch && !is_npc( wch )
   && xIS_SET( wch->pcdata->flags, PCFLAG_DND ) && get_trust( ch ) < get_trust( wch ) )
   {
      pager_printf( ch, "Sorry. %s does not wish to be disturbed.\r\n", wch->name );
      pager_printf( wch, "Your DND flag just foiled %s's at command.\r\n", ch->name );
      return;
   }

   if( room_is_private( location ) )
   {
      if( get_trust( ch ) < PERM_LEADER )
      {
         send_to_char( "That room is private right now.\r\n", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\r\n", ch );
   }

   if( ( victim = room_is_dnd( ch, location ) ) )
   {
      send_to_pager( "That room is \"do not disturb\" right now.\r\n", ch );
      pager_printf( victim, "Your DND flag just foiled %s's atmob command\r\n", ch->name );
      return;
   }

   set_char_color( AT_PLAIN, ch );
   original = ch->in_room;
   if( is_in_wilderness( ch ) )
   {
      wasinwild = true;
      oldx = ch->cords[0];
      oldy = ch->cords[1];
   }
   char_from_room( ch );
   char_to_room( ch, location );
   if( wch && is_in_wilderness( wch ) )
      put_in_wilderness( ch, wch->cords[0], wch->cords[1] );
   interpret( ch, argument );

   if( !char_died( ch ) )
   {
      char_from_room( ch );
      char_to_room( ch, original );
      if( wasinwild )
         put_in_wilderness( ch, oldx, oldy );
   }
}

CMDF( do_atobj )
{
   ROOM_INDEX_DATA *location = NULL, *original;
   OBJ_DATA *obj;
   CHAR_DATA *victim;
   char arg[MIL];
   short oldx = 0, oldy = 0;
   bool wasinwild = false;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || argument == NULL || arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "At where what?\r\n", ch );
      return;
   }

   if( !( obj = get_obj_world( ch, arg ) ) || !obj->in_room )
   {
      send_to_char( "No such object in existance.\r\n", ch );
      return;
   }
   location = obj->in_room;
   if( room_is_private( location ) )
   {
      if( get_trust( ch ) < PERM_LEADER )
      {
         send_to_char( "That room is private right now.\r\n", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\r\n", ch );
   }

   if( ( victim = room_is_dnd( ch, location ) ) )
   {
      send_to_pager( "That room is \"do not disturb\" right now.\r\n", ch );
      pager_printf( victim, "Your DND flag just foiled %s's atobj command\r\n", ch->name );
      return;
   }

   set_char_color( AT_PLAIN, ch );
   original = ch->in_room;
   if( is_in_wilderness( ch ) )
   {
      wasinwild = true;
      oldx = ch->cords[0];
      oldy = ch->cords[1];
   }
   char_from_room( ch );
   char_to_room( ch, location );
   if( obj && is_obj_stat( obj, ITEM_WILDERNESS ) )
      put_in_wilderness( ch, obj->cords[0], obj->cords[1] );
   interpret( ch, argument );

   if( !char_died( ch ) )
   {
      char_from_room( ch );
      char_to_room( ch, original );
      if( wasinwild )
         put_in_wilderness( ch, oldx, oldy );
   }
}

CMDF( do_rat )
{
   ROOM_INDEX_DATA *location, *original;
   char arg1[MIL], arg2[MIL];
   int Start, End, vnum;
   short oldx = 0, oldy = 0;
   bool wasinwild = false;

   set_char_color( AT_IMMORT, ch );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg2 == NULL || argument == NULL || arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Usage: rat <start> <end> <command>\r\n", ch );
      return;
   }

   Start = atoi( arg1 );
   End = atoi( arg2 );
   if( Start < 1 || End < Start || Start > End || Start == End || End > MAX_VNUM )
   {
      send_to_char( "Invalid range.\r\n", ch );
      return;
   }
   if( !str_cmp( argument, "quit" ) )
   {
      send_to_char( "I don't think so!\r\n", ch );
      return;
   }

   original = ch->in_room;
   if( is_in_wilderness( ch ) )
   {
      wasinwild = true;
      oldx = ch->cords[0];
      oldy = ch->cords[1];
   }
   for( vnum = Start; vnum <= End; vnum++ )
   {
      if( !( location = get_room_index( vnum ) ) )
         continue;
      char_from_room( ch );
      char_to_room( ch, location );
      interpret( ch, argument );
   }

   char_from_room( ch );
   char_to_room( ch, original );
   if( wasinwild )
      put_in_wilderness( ch, oldx, oldy );
   send_to_char( "Done.\r\n", ch );
}

CMDF( do_rstat )
{
   ROOM_INDEX_DATA *location;
   OBJ_DATA *obj;
   CHAR_DATA *rch;
   EXIT_DATA *pexit;
   static const char *dir_text[] = { "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?" };
   char buf[MSL], arg[MIL];
   int cnt;

   one_argument( argument, arg );
   if( !str_cmp( arg, "ex" ) || !str_cmp( arg, "exits" ) )
   {
      location = ch->in_room;

      ch_printf( ch, "&cExits for room '&W%s&c'  Vnum &W%d   &cAreaVnum &W%d\r\n", location->name, location->vnum, location->avnum );
      for( cnt = 0, pexit = location->first_exit; pexit; pexit = pexit->next )
      {
         ch_printf( ch, "&W%2d) &w%2s to %-5d  &cKey: &w%d  &cFlags: &w%s  ",
            ++cnt, dir_text[pexit->vdir],
            pexit->to_room ? pexit->to_room->vnum : 0,
            pexit->key, ext_flag_string( &pexit->exit_info, ex_flags ) );
         if( !xSAME_BITS( pexit->exit_info, pexit->base_info ) )
            ch_printf( ch, "&cIndex Flags: &w%s", ext_flag_string( &pexit->base_info, ex_flags ) );

         ch_printf( ch, "\r\n&cKeywords: '&w%s&c'\r\n     Exdesc: &w%s     &cBack link: &w%d  ",
            pexit->keyword ? pexit->keyword : "(Not Set)",
            pexit->description ? pexit->description : "(Not Set).\r\n",
            pexit->rexit ? pexit->rexit->vnum : 0 );
         ch_printf( ch, "&cVnum: &w%d  &cPulltype: &w%s  &cPull: &w%d\r\n",
            pexit->rvnum, pull_type_name( pexit->pulltype ), pexit->pull );
      }
      return;
   }
   location = ( arg == NULL || arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
   if( !location )
   {
      send_to_char( "No such location.\r\n", ch );
      return;
   }

   if( ch->in_room != location && room_is_private( location ) )
   {
      if( get_trust( ch ) < PERM_LEADER )
      {
         send_to_char( "That room is private right now.\r\n", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\r\n", ch );
   }

   ch_printf( ch, "&cName: &w%s\r\n&cArea: &w%s  &cFilename: &w%s\r\n",
      location->name, location->area ? location->area->name : "None????",
      location->area ? location->area->filename : "None????" );

   ch_printf( ch, "&cVnum: &w%d   &cSector: &w(%s)   &cLight: &w%d",
      location->vnum, sect_flags[ch->in_room->sector_type], location->light );
   if( location->tunnel > 0 )
      ch_printf( ch, "   &cTunnel: &W%d", location->tunnel );
   send_to_char( "\r\n", ch );
   ch_printf( ch, "&cChars: &w%d  &cObjs: &w%d\r\n", location->charcount, location->objcount );
   if( location->tele_delay > 0 || location->tele_vnum > 0 )
      ch_printf( ch, "&cTeleDelay: &R%d   &cTeleVnum: &R%d\r\n", location->tele_delay, location->tele_vnum );
   ch_printf( ch, "&cRoom flags: &w%s\r\n", ext_flag_string( &location->room_flags, r_flags ) );

   if( !xIS_EMPTY( location->progtypes ) )
      ch_printf( ch, "&cPrograms: &w%s\r\n", ext_flag_string( &location->progtypes, prog_names ) );

   ch_printf( ch, "&cDescription:\r\n&w%s", location->description ? location->description : "(Not Set)\r\n" );
   if( location->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;

      send_to_char( "&cExtra description keywords: &w'", ch );
      for( ed = location->first_extradesc; ed; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if( ed->next )
            send_to_char( " ", ch );
      }
      send_to_char( "'\r\n", ch );
   }

   send_to_char( "&cCharacters: &w", ch );
   for( rch = location->first_person; rch; rch = rch->next_in_room )
   {
      if( can_see( ch, rch ) )
      {
         send_to_char( " ", ch );
         one_argument( rch->name, buf );
         send_to_char( buf, ch );
      }
   }

   send_to_char( "\r\n&cObjects:&w\r\n", ch );
   for( obj = location->first_content; obj; obj = obj->next_content )
   {
      send_to_char( format_obj_to_char( obj, ch, true ), ch );
      send_to_char( "\r\n", ch );
   }

   if( location->first_exit )
      send_to_char( "&c------------------- &wEXITS &c-------------------\r\n", ch );
   for( cnt = 0, pexit = location->first_exit; pexit; pexit = pexit->next )
   {
      ++cnt;
      if( !pexit->to_room )
      {
         bug( "%s: NULL to_room for exit %2d) %-2s", __FUNCTION__, cnt, dir_text[pexit->vdir] );
         continue;
      }

      ch_printf( ch, "%2d) %-2s to %-5d. Name: %-10.10s",
         cnt, dir_text[pexit->vdir], pexit->to_room->vnum,
         pexit->to_room->name ? pexit->to_room->name : "(UNKNOWN)" );
      if( pexit->key > 0 )
         ch_printf( ch, " Key: %d", pexit->key );
      if( !xIS_EMPTY( pexit->exit_info ) )
         ch_printf( ch, " Flags: [%s]", ext_flag_string( &pexit->exit_info, ex_flags ) );
      if( !xSAME_BITS( pexit->exit_info, pexit->base_info ) )
         ch_printf( ch, " Index Flags: [%s]", ext_flag_string( &pexit->base_info, ex_flags ) );
      if( pexit->keyword )
         ch_printf( ch, " Keywords: %s", pexit->keyword );
      send_to_char( "\r\n", ch );
   }
}

/* Face-lift by Demora */
CMDF( do_ostat )
{
   AFFECT_DATA *paf;
   OBJ_DATA *obj;
   char arg[MIL];
   int stat, count = 0;

   set_char_color( AT_CYAN, ch );
   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Ostat what?\r\n", ch );
      return;
   }
   if( arg[0] != '\'' && arg[0] != '"' && strlen( argument ) > strlen( arg ) )
      mudstrlcpy( arg, argument, sizeof( arg ) );

   if( !( obj = get_obj_world( ch, arg ) ) )
   {
      send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
      return;
   }

   ch_printf( ch, "&cName: &w%s\r\n", obj->name ? obj->name : "Not Set" );
   if( str_cmp( obj->name, obj->pIndexData->name ) )
      ch_printf( ch, "&cIndex Name: &r%s\r\n", obj->pIndexData->name );

   ch_printf( ch, "&cVnum: &w%d &cAreaVnum: &w%d\r\n", obj->pIndexData->vnum, obj->pIndexData->avnum );

   ch_printf( ch, "&cType: &w%d&c[&w%s&c]\r\n", obj->item_type, o_types[obj->item_type] );
   if( obj->item_type != obj->pIndexData->item_type )
      ch_printf( ch, "&cIndex Type: &r%d&c[&r%s&c]\r\n", obj->pIndexData->item_type, o_types[obj->pIndexData->item_type] );

   ch_printf( ch, "&cCount: &w%d&c(&r%d&c)\r\n", obj->count, obj->pIndexData->count );

   ch_printf( ch, "&cShort description : &w%s\r\n", obj->short_descr ? obj->short_descr : "Not Set" );
   if( str_cmp( obj->short_descr, obj->pIndexData->short_descr ) )
      ch_printf( ch, "&cIndex Short description : &r%s\r\n", obj->pIndexData->short_descr ? obj->pIndexData->short_descr : "Not Set" );

   ch_printf( ch, "&cLong description  : &w%s\r\n", obj->description ? obj->description : "Not Set" );
   if( str_cmp( obj->description, obj->pIndexData->description ) )
      ch_printf( ch, "&cIndex Long description  : &r%s\r\n", obj->pIndexData->description ? obj->pIndexData->description : "Not Set" );

   ch_printf( ch, "&cDesc              : &w%s\r\n", obj->desc ? obj->desc : "Not Set" );
   if( str_cmp( obj->desc, obj->pIndexData->desc ) )
      ch_printf( ch, "&cIndex Desc              : &r%s\r\n", obj->pIndexData->desc ? obj->pIndexData->desc : "Not Set" );

   ch_printf( ch, "&cAction description: &w%s\r\n", obj->action_desc ? obj->action_desc : "Not Set" );
   if( str_cmp( obj->action_desc, obj->pIndexData->action_desc ) )
      ch_printf( ch, "&cIndex Action description: &r%s\r\n", obj->pIndexData->action_desc ? obj->pIndexData->action_desc : "Not Set" );

   ch_printf( ch, "&cWear flags : &w%s\r\n", ext_flag_string( &obj->wear_flags, w_flags ) );
   if( !xSAME_BITS( obj->wear_flags, obj->pIndexData->wear_flags ) )
      ch_printf( ch, "&cIndex Wear flags : &r%s\r\n", ext_flag_string( &obj->pIndexData->wear_flags, w_flags ) );

   ch_printf( ch, "&cExtra flags: &w%s\r\n", ext_flag_string( &obj->extra_flags, o_flags ) );
   if( !xSAME_BITS( obj->extra_flags, obj->pIndexData->extra_flags ) )
      ch_printf( ch, "&cIndex Extra flags: &r%s\r\n", ext_flag_string( &obj->pIndexData->extra_flags, o_flags ) );

   ch_printf( ch, "&cIndex Programs: &r%s\r\n", ext_flag_string( &obj->pIndexData->progtypes, prog_names ) );

   ch_printf( ch, "&cNumber: &w%d/%d   ", 1, get_obj_number( obj ) );

   ch_printf( ch, "&cWeight: &w%d/%d   ", obj->weight, get_obj_weight( obj ) );

   ch_printf( ch, "&cLayers: &w%d\r\n", obj->pIndexData->layers );

   ch_printf( ch, "&cWear_loc: &w%d(%s)\r\n", obj->wear_loc, obj->wear_loc != WEAR_NONE ? wear_locs[obj->wear_loc] : "Not Worn" );

   ch_printf( ch, "&cCost: &w%d", obj->cost );
   if( obj->cost != obj->pIndexData->cost )
      ch_printf( ch, "&c(&r%d&c)", obj->pIndexData->cost );
   send_to_char( "\r\n", ch );

   if( obj->bsplatter )
      ch_printf( ch, "&cBlood: &w%d&c\r\n", obj->bsplatter );

   if( obj->bstain )
      ch_printf( ch, "&cStain: &w%d&c\r\n", obj->bstain );

   if( obj->timer )
      ch_printf( ch, "&cTimer: &w%d&c\r\n", obj->timer );

   send_to_char( "&cRequirements:\r\n", ch );
   ch_printf( ch, "&c%12s: &w%3d&c(&r%3d&c) ", "Level", obj->level, obj->pIndexData->level );
   /* Since displaying level as a requirement put count to 1 before loop */
   count++;
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      ch_printf( ch, "&c%12s: &w%3d&c(&r%3d&c)", capitalize( stattypes[stat] ), obj->stat_reqs[stat],
         obj->pIndexData->stat_reqs[stat] );
      if( ++count == 3 )
      {
         count = 0;
         send_to_char( "\r\n", ch );
      }
      else
         send_to_char( " ", ch );
   }
   if( count != 0 )
      send_to_char( "\r\n", ch );

   if( !xIS_EMPTY( obj->class_restrict ) )
      ch_printf( ch, "&cClass Restrictions:\r\n&w%s\r\n", ext_class_string( &obj->class_restrict ) );
   if( !xSAME_BITS( obj->class_restrict, obj->pIndexData->class_restrict ) )
      ch_printf( ch, "&cIndex Class Restrictions:\r\n&r%s\r\n", ext_class_string( &obj->pIndexData->class_restrict ) );

   if( !xIS_EMPTY( obj->race_restrict ) )
      ch_printf( ch, "&cRace Restrictions:\r\n&w%s\r\n", ext_race_string( &obj->race_restrict ) );
   if( !xSAME_BITS( obj->race_restrict, obj->pIndexData->race_restrict ) )
      ch_printf( ch, "&cIndex Race Restriction:\r\n&r%s\r\n", ext_race_string( &obj->pIndexData->race_restrict ) );

   if( obj->in_room )
   {
      ch_printf( ch, "&cIn room: &w%d&c(&w%s&c)", obj->in_room->vnum,
         obj->in_room->name ? obj->in_room->name : "Not Set" );
      if( is_obj_stat( obj, ITEM_WILDERNESS ) )
         ch_printf( ch, " &cCords: X[&w%d&c] Y[&w%d&c]", obj->cords[0], obj->cords[1] );
      send_to_char( "\r\n", ch );
   }

   if( obj->carried_by )
      ch_printf( ch, "&cCarried by: &w%s\r\n", obj->carried_by->name );

   if( obj->in_obj )
   {
      OBJ_DATA *tobj = NULL;

      ch_printf( ch, "&cIn object: &w%s\r\n", obj->in_obj->short_descr );

      tobj = obj->in_obj;
      while( tobj->in_obj )
      {
         ch_printf( ch, "&cIn object: &w%s\r\n", tobj->in_obj->short_descr );
         tobj = tobj->in_obj;
      }
      if( tobj )
      {
         if( tobj->in_obj )
            ch_printf( ch, "&cIn object: &w%s\r\n", tobj->in_obj->short_descr );
         if( tobj->in_room )
         {
            ch_printf( ch, "&cIn room: &w%d&c(&w%s&c)", tobj->in_room->vnum,
               tobj->in_room->name ? tobj->in_room->name : "Not Set" );
            if( is_obj_stat( tobj, ITEM_WILDERNESS ) )
               ch_printf( ch, " &cCords: X[&w%d&c] Y[&w%d&c]", tobj->cords[0], tobj->cords[1] );
            send_to_char( "\r\n", ch );
         }
         if( tobj->carried_by )
            ch_printf( ch, "&cCarried by: &w%s\r\n", tobj->carried_by->name );
         if( tobj->auctioned )
            send_to_char( "&cBeing Auctioned\r\n", ch );
      }
   }

   if( obj->auctioned )
      send_to_char( "&cBeing Auctioned\r\n", ch );

   ch_printf( ch, "&cIndex Values : &r%d %d %d %d %d %d.\r\n",
      obj->pIndexData->value[0], obj->pIndexData->value[1],
      obj->pIndexData->value[2], obj->pIndexData->value[3],
      obj->pIndexData->value[4], obj->pIndexData->value[5] );
   ch_printf( ch, "&cObject Values: &w%d %d %d %d %d %d.\r\n",
      obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4], obj->value[5] );

   if( obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_QUIVER )
      ch_printf( ch, "&cContainer Flags: &w%s\r\n", flag_string( obj->value[1], cont_flags ) );

   if( ( obj->pIndexData->item_type == ITEM_CONTAINER || obj->pIndexData->item_type == ITEM_QUIVER )
   && ( obj->value[1] != obj->pIndexData->value[1] || obj->item_type != obj->pIndexData->item_type ) )
      ch_printf( ch, "&cIndex Container Flags: &r%s\r\n", flag_string( obj->pIndexData->value[1], cont_flags ) );

   if( obj->item_type == ITEM_MISSILE_WEAPON || obj->item_type == ITEM_WEAPON )
      ch_printf( ch, "&cWeapon Type: &w%s\r\n", ( obj->value[3] >= 0 && obj->value[3] < DAM_MAX ) ? attack_table[obj->value[3]] : "Unknown" );

   if( ( obj->pIndexData->item_type == ITEM_MISSILE_WEAPON || obj->pIndexData->item_type == ITEM_WEAPON )
   && ( obj->value[3] != obj->pIndexData->value[3] || obj->item_type != obj->pIndexData->item_type ) )
      ch_printf( ch, "&cIndex Weapon Type: &r%s\r\n",
        ( obj->pIndexData->value[3] >= 0 && obj->pIndexData->value[3] < DAM_MAX ) ? attack_table[obj->pIndexData->value[3]] : "Unknown" );

   if( obj->item_type == ITEM_SWITCH || obj->item_type == ITEM_LEVER || obj->item_type == ITEM_PULLCHAIN || obj->item_type == ITEM_BUTTON )
      ch_printf( ch, "&cLever Flags: &w%s\r\n", flag_string( obj->value[0], trig_flags ) );

   /* Now lets give some more info on what the other values will do based on everything so they know what to expect from it */
   if( obj->item_type == ITEM_SWITCH || obj->item_type == ITEM_LEVER || obj->item_type == ITEM_PULLCHAIN || obj->item_type == ITEM_BUTTON )
   {
      if( IS_SET( obj->value[0], TRIG_TELEPORT ) || IS_SET( obj->value[0], TRIG_TELEPORTALL ) || IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
         ch_printf( ch, "&cWants to teleport &w%s &cto room vnum &w%d%s%s&c.\r\n",
            IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) ? "everyone and everything" :
            IS_SET( obj->value[0], TRIG_TELEPORTALL ) ? "everyone" : "the actor",
            obj->value[1], get_room_index( obj->value[1] ) ? "" : "&r(Invalid)",
            IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) ? " and will show room description" : "" );

      if( IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 )
      || IS_SET( obj->value[0], TRIG_RAND10 ) || IS_SET( obj->value[0], TRIG_RAND11 ) )
         ch_printf( ch, "&cWants to randomize &w%d &cexits in room &w%d%s&c.\r\n", IS_SET( obj->value[0], TRIG_RAND4 ) ? 4 : IS_SET( obj->value[0], TRIG_RAND6 ) ? 6 :
            IS_SET( obj->value[0], TRIG_RAND10 ) ? 10 : 11, obj->value[1], get_room_index( obj->value[1] ) ? "" : "&r(Invalid)" );

      if( IS_SET( obj->value[0], TRIG_MLOAD ) )
         ch_printf( ch, "&cWants to load mobile &w%d%s &cin room &w%d%s&c.\r\n", obj->value[1], get_mob_index( obj->value[1] ) ? "" : "&r(Invalid)",
            obj->value[2], ( obj->value[2] > 0 ) ? ( get_room_index( obj->value[2] ) ? "" : "&r(Invalid)" ) : "(Room used in)" );

      if( IS_SET( obj->value[0], TRIG_OLOAD ) )
         ch_printf( ch, "&cWants to load a level &w%d &cobject &w%d%s &cin room &w%d%s&c.\r\n",  ( obj->value[3] > 0 ) ? obj->value[3] : 0,
            obj->value[1], get_obj_index( obj->value[1] ) ? "" : "&r(Invalid)",
            obj->value[2], ( obj->value[2] > 0 ) ? ( get_room_index( obj->value[2] ) ? "" : "&r(Invalid)" ) : "(to actor or room used in)" );

      if( IS_SET( obj->value[0], TRIG_CAST ) )
      {
         int usesn = slot_lookup( obj->value[1] );

         ch_printf( ch, "&cWants to cast the spell that is set to slot &w%d &cwhich is sn &w%d%s.\r\n", obj->value[1], usesn,
            ( usesn < 0 || !is_valid_sn( usesn ) ) ? "&r(Invalid)" : "" );
      }

      if( IS_SET( obj->value[0], TRIG_CONTAINER ) )
         ch_printf( ch, "&cWants to toggle&w%s%s%s%s%s&cflags on container &w%d &cin room &w%d%s&c.\r\n",
            IS_SET( obj->value[3], CONT_CLOSEABLE ) ? " closeable " : " ",
            IS_SET( obj->value[3], CONT_PICKPROOF ) ? " pickproof " : " ",
            IS_SET( obj->value[3], CONT_CLOSED ) ? " closed " : " ",
            IS_SET( obj->value[3], CONT_LOCKED ) ? " locked " : " ",
            IS_SET( obj->value[3], CONT_EATKEY ) ? " eatkey " : " ",
            obj->value[2], obj->value[1],
            get_room_index( obj->value[1] ) ? "" : "&r(Invalid)" );

      if( IS_SET( obj->value[0], TRIG_DOOR ) )
      {
         ch_printf( ch, "&cWants to modify &w%s &cexit's&w%s%s%s%s&cflags in room &w%d%s &cto room &w%d%s&c.\r\n",
            IS_SET( obj->value[0], TRIG_D_NORTH ) ? "North" :
            IS_SET( obj->value[0], TRIG_D_SOUTH ) ? "South" :
            IS_SET( obj->value[0], TRIG_D_EAST ) ? "East" :
            IS_SET( obj->value[0], TRIG_D_WEST ) ? "West" :
            IS_SET( obj->value[0], TRIG_D_UP ) ? "Up" :
            IS_SET( obj->value[0], TRIG_D_DOWN ) ? "Down" :
            IS_SET( obj->value[0], TRIG_D_NORTHEAST ) ? "Northeast" :
            IS_SET( obj->value[0], TRIG_D_NORTHWEST ) ? "Northwest" :
            IS_SET( obj->value[0], TRIG_D_SOUTHEAST ) ? "Southeast" :
            IS_SET( obj->value[0], TRIG_D_SOUTHWEST ) ? "Southwest" :
            IS_SET( obj->value[0], TRIG_D_SOMEWHERE ) ? "Somewhere" : "Unknown",
            IS_SET( obj->value[0], TRIG_UNLOCK ) ? " unlock " : " ",
            IS_SET( obj->value[0], TRIG_LOCK ) ? " lock " : " ",
            IS_SET( obj->value[0], TRIG_OPEN ) ? " open " : " ",
            IS_SET( obj->value[0], TRIG_CLOSE ) ? " close " : " ",
            obj->value[1],
            get_room_index( obj->value[1] ) ? "" : "&r(Invalid)",
            obj->value[2],
            ( obj->value[2] > 0 ) ? ( get_room_index( obj->value[2] ) ? "" : "&r(Invalid)" ) : "Unknown" );
      }
   }

   if( ( obj->pIndexData->item_type == ITEM_SWITCH || obj->pIndexData->item_type == ITEM_LEVER
   || obj->pIndexData->item_type == ITEM_PULLCHAIN || obj->pIndexData->item_type == ITEM_BUTTON )
   && ( obj->value[0] != obj->pIndexData->value[0] || obj->item_type != obj->pIndexData->item_type ) )
      ch_printf( ch, "&cIndex Lever Flags: &r%s\r\n", flag_string( obj->pIndexData->value[0], trig_flags ) );

   if( obj->item_type == ITEM_PILL || obj->item_type == ITEM_SCROLL || obj->item_type == ITEM_POTION )
   {
      SKILLTYPE *sktmp;

      ch_printf( ch, "&cCast level &w%d &cspells.\r\n", obj->value[0] );
      if( obj->value[1] >= 0 && ( sktmp = get_skilltype( obj->value[1] ) ) )
         ch_printf( ch, "&cValue 1 cast spell: &w%s\r\n", sktmp->name );
      if( obj->value[2] >= 0 && ( sktmp = get_skilltype( obj->value[2] ) ) )
         ch_printf( ch, "&cValue 2 cast spell: &w%s\r\n", sktmp->name );
      if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) )
         ch_printf( ch, "&cValue 3 cast spell: &w%s\r\n", sktmp->name );
   }

   if( ( obj->pIndexData->item_type == ITEM_PILL || obj->pIndexData->item_type == ITEM_SCROLL || obj->pIndexData->item_type == ITEM_POTION )
   && ( obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1] || obj->value[2] != obj->pIndexData->value[2]
   || obj->value[3] != obj->pIndexData->value[3] || obj->item_type != obj->pIndexData->item_type ) )
   {
      SKILLTYPE *sktmp;

      if( obj->value[0] != obj->pIndexData->value[0] )
         ch_printf( ch, "&cIndex Cast level &r%d &cspells.\r\n", obj->pIndexData->value[0] );
      if( obj->value[1] != obj->pIndexData->value[1] && obj->pIndexData->value[1] >= 0 && ( sktmp = get_skilltype( obj->pIndexData->value[1] ) ) )
         ch_printf( ch, "&cIndex Value 1 cast spell: &r%s\r\n", sktmp->name );
      if( obj->value[2] != obj->pIndexData->value[2] && obj->pIndexData->value[2] >= 0 && ( sktmp = get_skilltype( obj->pIndexData->value[2] ) ) )
         ch_printf( ch, "&cIndex Value 2 cast spell: &r%s\r\n", sktmp->name );
      if( obj->value[3] != obj->pIndexData->value[3] && obj->pIndexData->value[3] >= 0 && ( sktmp = get_skilltype( obj->pIndexData->value[3] ) ) )
         ch_printf( ch, "&cIndex Value 3 cast spell: &r%s\r\n", sktmp->name );
   }

   if( obj->item_type == ITEM_SALVE )
   {
      SKILLTYPE *sktmp;

      ch_printf( ch, "&cHas &w%d&c(&w%d&c) applications of level &w%d &cspells.\r\n", obj->value[1], obj->value[2], obj->value[0] );
      if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) )
         ch_printf( ch, "&cValue 3 cast spell: &w%s\r\n", sktmp->name );
      if( obj->value[4] >= 0 && ( sktmp = get_skilltype( obj->value[4] ) ) )
         ch_printf( ch, "&cValue 4 cast spell: &w%s\r\n", sktmp->name );
      if( obj->value[5] >= 0 && ( sktmp = get_skilltype( obj->value[5] ) ) )
         ch_printf( ch, "&cValue 5 cast spell: &w%s\r\n", sktmp->name );
   }

   if( obj->pIndexData->item_type == ITEM_SALVE && ( obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1]
   || obj->value[2] != obj->pIndexData->value[2] || obj->value[3] != obj->pIndexData->value[3] || obj->value[4] != obj->pIndexData->value[4]
   || obj->value[5] != obj->pIndexData->value[5] || obj->item_type != obj->pIndexData->item_type ) )
   {
      SKILLTYPE *sktmp;

      if( obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1] || obj->value[2] != obj->pIndexData->value[2] )
         ch_printf( ch, "&cIndex Has &r%d&c(&r%d&c) applications of level &r%d &cspells.\r\n", obj->pIndexData->value[1], obj->pIndexData->value[2], obj->pIndexData->value[0] );
      if( obj->value[3] != obj->pIndexData->value[3] && obj->pIndexData->value[3] >= 0 && ( sktmp = get_skilltype( obj->pIndexData->value[3] ) ) )
         ch_printf( ch, "&cIndex Value 3 cast spell: &r%s\r\n", sktmp->name );
      if( obj->value[4] != obj->pIndexData->value[4] && obj->pIndexData->value[4] >= 0 && ( sktmp = get_skilltype( obj->pIndexData->value[4] ) ) )
         ch_printf( ch, "&cIndex Value 4 cast spell: &r%s\r\n", sktmp->name );
      if( obj->value[5] != obj->pIndexData->value[5] && obj->pIndexData->value[5] >= 0 && ( sktmp = get_skilltype( obj->pIndexData->value[5] ) ) )
         ch_printf( ch, "&cIndex Value 5 cast spell: &r%s\r\n", sktmp->name );
   }

   if( obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF )
   {
      SKILLTYPE *sktmp;

      ch_printf( ch, "&cHas &w%d&c(&w%d&c) charges of level &w%d&c.\r\n", obj->value[1], obj->value[2], obj->value[0] );
      if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) )
         ch_printf( ch, "&cValue 3 cast spell: &w%s\r\n", sktmp->name );
   }

   if( ( obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF )
   && ( obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1]
   || obj->value[2] != obj->pIndexData->value[2] || obj->value[3] != obj->pIndexData->value[3]
   || obj->item_type != obj->pIndexData->item_type ) )
   {
      SKILLTYPE *sktmp;

      if( obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1] || obj->value[2] != obj->pIndexData->value[2] )
         ch_printf( ch, "&cIndex has &r%d&c(&r%d&c) charges of level &r%d&c.\r\n", obj->pIndexData->value[1], obj->pIndexData->value[2], obj->pIndexData->value[0] );
      if( obj->value[3] != obj->pIndexData->value[3] && obj->pIndexData->value[3] >= 0 && ( sktmp = get_skilltype( obj->pIndexData->value[3] ) ) )
         ch_printf( ch, "&cIndex value 3 cast spell: &r%s\r\n", sktmp->name );
   }

   if( obj->pIndexData->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;
      send_to_char( "Primary description keywords:   '", ch );
      for( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if( ed->next )
            send_to_char( " ", ch );
      }
      send_to_char( "'.\r\n", ch );
   }

   if( obj->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;
      send_to_char( "Secondary description keywords: '", ch );
      for( ed = obj->first_extradesc; ed; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if( ed->next )
            send_to_char( " ", ch );
      }
      send_to_char( "'.\r\n", ch );
   }

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      showaffect( ch, paf, false );

   for( paf = obj->first_affect; paf; paf = paf->next )
      showaffect( ch, paf, true );

   show_track_resets( ch, obj->pIndexData, NULL );
}

CMDF( do_mstat )
{
   AFFECT_DATA *paf;
   CHAR_DATA *victim;
   SKILLTYPE *skill;
   MOB_INDEX_DATA *mindex;
   char arg[MIL];
   int x, stat;

   set_pager_color( AT_CYAN, ch );
   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_pager( "Mstat whom?\r\n", ch );
      return;
   }
   if( arg[0] != '\'' && arg[0] != '"' && strlen( argument ) > strlen( arg ) )
      mudstrlcpy( arg, argument, sizeof( arg ) );

   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_pager( "They aren't here.\r\n", ch );
      return;
   }
   if( get_trust( ch ) < get_trust( victim ) && !is_npc( victim ) )
   {
      set_pager_color( AT_IMMORT, ch );
      send_to_pager( "Their godly glow prevents you from getting a good look.\r\n", ch );
      return;
   }
   if( is_npc( victim ) && get_trust( ch ) < PERM_LEADER && xIS_SET( victim->act, ACT_STATSHIELD ) )
   {
      set_pager_color( AT_IMMORT, ch );
      send_to_pager( "Their godly glow prevents you from getting a good look.\r\n", ch );
      return;
   }

   mindex = victim->pIndexData;

   send_to_pager( "&WAll Data:\r\n", ch );

   pager_printf( ch, "&c%s: &w%-20s\r\n", is_npc( victim ) ? "Mobile name" : "Name", victim->name );
   if( mindex && str_cmp( victim->name, mindex->name ) )
      pager_printf( ch, "&cIndex Mobile Name: &r%-20s\r\n", mindex->name ? mindex->name : "(Not Set)" );

   pager_printf( ch, "&cSex: &w%s", sex_names[victim->sex] );
   if( mindex && victim->sex != mindex->sex )
      pager_printf( ch, "&c(&r%s&c)", sex_names[mindex->sex] );

   pager_printf( ch, " &cRoom: &w%d", victim->in_room ? victim->in_room->vnum : 0 );
   if( is_in_wilderness( victim ) )
   pager_printf( ch, " &cCords X[&w%d&c] Y[&w%d&c]", victim->cords[0], victim->cords[1] );
   pager_printf( ch, " &cKilled: &w%d\r\n",
      mindex ? mindex->killed : ( victim->pcdata->mdeaths + victim->pcdata->pdeaths ) );

   pager_printf( ch, "&cLevel: &w%d", victim->level );
   if( mindex && victim->level != mindex->level )
      pager_printf( ch, "&c(&r%d&c)", mindex->level );
   send_to_pager( "\r\n", ch );

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      pager_printf( ch, "&c(%3.3s: &w%d+(%d)", capitalize( stattypes[stat] ), get_perm_stat( stat, victim ), get_mod_stat( stat, victim ) );
      if( stat == STAT_CHA )
      {
         short currcha = get_curr_stat( stat, victim );
         short scurrcha = ( get_perm_stat( stat, victim ) + get_mod_stat( stat, victim ) );

         if( currcha > scurrcha )
            pager_printf( ch, "+(%d)", ( currcha - scurrcha ) );
         else if( scurrcha > currcha )
            pager_printf( ch, "-(&r%d&w)", ( scurrcha - currcha ) );
      }
      if( mindex && get_perm_stat( stat, victim ) != mindex->perm_stats[stat] )
         pager_printf( ch, "&c[&r%d&c]", mindex->perm_stats[stat] );
      send_to_pager( "&c)", ch );
   }
   send_to_pager( "\r\n", ch );

   pager_printf( ch, "&cHps: &w%d&c/&w%d &c%s: &w%d&c/&w%d &cMove: &w%d&c/&w%d\r\n",
      victim->hit, victim->max_hit, is_vampire( victim ) ? "Blood" : "Mana",
      victim->mana, victim->max_mana, victim->move, victim->max_move );

   pager_printf( ch, "&cHitroll: &C%5d ", get_hitroll( victim ) );
   if( mindex && get_hitroll( victim ) != mindex->hitroll )
      pager_printf( ch, "&c[&R%5d&c] ", mindex->hitroll );
   pager_printf( ch, "&cAlign: &w%-5d &cArmorclass: &w%d\r\n", victim->alignment, get_ac( victim ) );

   pager_printf( ch, "&cDamroll: &C%5d ", get_damroll( victim ) );
   if( mindex && get_damroll( victim ) != mindex->damroll )
      pager_printf( ch, "&c[&R%5d&c] ", mindex->damroll );
   pager_printf( ch, "&cWimpy: &w%-5d &cPosition: &w%d&c[&w%s&c]",
      victim->wimpy, victim->position, pos_names[victim->position] );
   if( mindex && victim->position != mindex->defposition )
      pager_printf( ch, "&c(&R%d&c[&R%s&c])", mindex->defposition, pos_names[mindex->defposition] );
   send_to_pager( "\r\n", ch );

   pager_printf( ch, "&cFighting: &w%-13s   &cMaster: &w%-13s   &cLeader: &w%s\r\n",
      victim->fighting ? victim->fighting->who->name : "(none)",
      victim->master ? victim->master->name : "(none)", victim->leader ? victim->leader->name : "(none)" );

   pager_printf( ch, "&cMentalState: &w%-3d\r\n", victim->mental_state );
   pager_printf( ch, "&cSave versus: &w%d %d %d %d %d       &cItems: &w(%d/%d)  &cWeight &w(%d/%d)\r\n",
      victim->saving_poison_death, victim->saving_wand, victim->saving_para_petri,
      victim->saving_breath, victim->saving_spell_staff, victim->carry_number,
      can_carry_n( victim ), victim->carry_weight, can_carry_w( victim ) );
   pager_printf( ch, "&cYear: &w%-5d  &cSecs: &w%d  &cTimer: &w%d  &cGold: &w%s\r\n",
      get_age( victim ), ( int )victim->played, victim->timer, show_char_gold( victim ) );
   if( get_timer( victim, TIMER_PKILLED ) )
      pager_printf( ch, "&cTimerPkilled:  &w%d\r\n", get_timer( victim, TIMER_PKILLED ) );
   if( get_timer( victim, TIMER_RECENTFIGHT ) )
      pager_printf( ch, "&cTimerRecentfight:  &w%d\r\n", get_timer( victim, TIMER_RECENTFIGHT ) );
   if( get_timer( victim, TIMER_ASUPPRESSED ) )
      pager_printf( ch, "&cTimerAsuppressed:  &w%d\r\n", get_timer( victim, TIMER_ASUPPRESSED ) );

   if( victim->morph )
   {
      if( victim->morph->morph )
         pager_printf( ch, "&cMorphed as : (&w%d&c) &w%s    &cTimer: &w%d\r\n",
            victim->morph->morph->vnum, victim->morph->morph->short_desc, victim->morph->timer );
      else
         send_to_pager( "&cMorphed as: Morph was deleted.\r\n", ch );
   }
   pager_printf( ch, "&cAffected by: &w%s\r\n", ext_flag_string( &victim->affected_by, a_flags ) );
   if( mindex && !xSAME_BITS( victim->affected_by, mindex->affected_by ) )
      pager_printf( ch, "&cIndex Affected By: &r%s\r\n", ext_flag_string( &mindex->affected_by, a_flags ) );

   if( mindex )
      ch_printf( ch, "&cIndex Programs: &r%s\r\n", ext_flag_string( &mindex->progtypes, prog_names ) );

   pager_printf( ch, "&cSpeaks: &w%s", ext_flag_string( &victim->speaks, lang_names ) );
   if( mindex && !xSAME_BITS( victim->speaks, mindex->speaks ) )
      pager_printf( ch, "&c(&r%s&c)", ext_flag_string( &mindex->speaks, lang_names ) );
   pager_printf( ch, "\r\n&cSpeaking: &w%s", ext_flag_string( &victim->speaking, lang_names ) );
   if( mindex && !xSAME_BITS( victim->speaking, mindex->speaking ) )
      pager_printf( ch, "&c(&r%s&c)", ext_flag_string( &mindex->speaking, lang_names ) );
   send_to_pager( "\r\n", ch );

   send_to_pager( "&cLanguages: &w", ch );
   for( x = 0; lang_array[x] != LANG_UNKNOWN; x++ )
   {
      if( knows_language( victim, lang_array[x] ) )
      {
         if( xIS_SET( victim->speaking, lang_array[x] ) )
            send_to_pager( "&[red]", ch );
         send_to_pager( lang_names[x], ch );
         send_to_pager( "&D ", ch );
      }
      else if( xIS_SET( victim->speaking, lang_array[x] ) )
      {
         send_to_pager( "&[pink]", ch );
         send_to_pager( lang_names[x], ch );
         send_to_pager( "&D ", ch );
      }
   }
   send_to_pager( "&D\r\n", ch );

   if( victim->short_descr )
      pager_printf( ch, "&cShortdesc: &w%s\r\n", victim->short_descr );
   if( mindex && str_cmp( victim->short_descr, mindex->short_descr ) )
      pager_printf( ch, "&cIndex Shortdesc: &r%s\r\n", mindex->short_descr ? mindex->short_descr : "(Not Set)" );
   
   if( victim->long_descr )
      pager_printf( ch, "&cLongdesc: &w%s", victim->long_descr );
   if( mindex && str_cmp( victim->long_descr, mindex->long_descr ) )
      pager_printf( ch, "&cIndex Longdesc: &r%s", mindex->long_descr ? mindex->long_descr : "(Not Set)" );

   for( stat = 0; stat < RIS_MAX; stat++ )
   {
      if( victim->resistant[stat] == 0 && ( !mindex || victim->resistant[stat] == mindex->resistant[stat] ) )
         continue;
      pager_printf( ch, "&cResistant  : (&w%d&c)", victim->resistant[stat] );
      if( mindex && victim->resistant[stat] != mindex->resistant[stat] )
         pager_printf( ch, "&c(&r%d&c)\r\n", mindex->resistant[stat] );
      pager_printf( ch, "&w%s\r\n", ris_flags[stat] );
   }

   /* Should show all affects listed not just ones with matching skills */
   for( paf = victim->first_affect; paf; paf = paf->next )
   {
      if( ( skill = get_skilltype( paf->type ) ) )
         ch_printf( ch, "&c%s: &w'%s' ", skill_tname[skill->type], skill->name );
      else
         ch_printf( ch, "&c%d: &w'Unknown' ", paf->type );

      if( ( paf->location % REVERSE_APPLY ) == APPLY_STAT )
         ch_printf( ch, "Mods %s", ext_flag_string( &paf->bitvector, stattypes ) );
      else if( ( paf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
         ch_printf( ch, "Mods %s", ext_flag_string( &paf->bitvector, ris_flags ) );
      else
         ch_printf( ch, "Mods %s", a_types[paf->location % REVERSE_APPLY] );

      if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
      {
         if( paf->modifier == 0 )
            ch_printf( ch, " by %s", ext_flag_string( &paf->bitvector, a_flags ) );
         else if( paf->modifier > 0 && paf->modifier < AFF_MAX )
            ch_printf( ch, " by %s", a_flags[paf->modifier] );
         else
            ch_printf( ch, " by %d", paf->modifier );
      }
      else
         ch_printf( ch, " by %d", paf->modifier );
      ch_printf( ch, " for %d Seconds &D\r\n", paf->duration );
   }
   if( is_npc( victim ) )
   {
      HHF_DATA *hhf;

      pager_printf( ch, "&cVnum: &w%-5d &cAreaVnum: &w%-5d &cCount: &w%d\r\n",
         victim->pIndexData->vnum, victim->pIndexData->avnum, victim->pIndexData->count );

      if( victim->summoning )
         pager_printf( ch, "&cSummoning: &w%s\r\n", victim->summoning->name );
      if( victim->first_hating )
      {
         for( hhf = victim->first_hating; hhf; hhf = hhf->next )
            pager_printf( ch, "&cHating: &w%s\r\n", hhf->name );
      }
      if( victim->first_hunting )
      {
         for( hhf = victim->first_hunting; hhf; hhf = hhf->next )
            pager_printf( ch, "&cHunting: &w%s\r\n", hhf->name );
      }
      if( victim->first_fearing )
      {
         for( hhf = victim->first_fearing; hhf; hhf = hhf->next )
            pager_printf( ch, "&cFearing: &w%s\r\n", hhf->name );
      }

      pager_printf( ch, "&cMob hit min/max: &r%d&c-&r%d&c\r\n", mindex->minhit, mindex->maxhit );

      pager_printf( ch, "&cNumAttacks : &w%d", victim->numattacks );
      if( mindex->numattacks != victim->numattacks )
         pager_printf( ch, "(&r%d)", mindex->numattacks );
      send_to_pager( "\r\n", ch );

      if( victim->spec_fun )
         pager_printf( ch, "&cMobile has spec fun: &w%s\r\n", victim->spec_funname );
      if( str_cmp( mindex->spec_funname, victim->spec_funname ) )
         pager_printf( ch, "&cIndex spec fun: &r%s\r\n", mindex->spec_funname ? mindex->spec_funname : "(Not Set)" );

      if( !xIS_EMPTY( victim->xflags ) )
         pager_printf( ch, "&cBody Parts: &w%s\r\n", ext_flag_string( &victim->xflags, part_flags ) );
      if( !xSAME_BITS( mindex->xflags, victim->xflags ) )
         pager_printf( ch, "&cIndex Body Parts: &r%s\r\n", ext_flag_string( &mindex->xflags, part_flags ) );

      if( !xIS_EMPTY( victim->attacks ) )
         pager_printf( ch, "&cAttacks: &w%s\r\n", ext_flag_string( &victim->attacks, attack_flags ) );
      if( !xSAME_BITS( mindex->attacks, victim->attacks ) )
         pager_printf( ch, "&cIndex Attacks: &r%s\r\n", ext_flag_string( &mindex->attacks, attack_flags ) );

      if( !xIS_EMPTY( victim->defenses ) )
         pager_printf( ch, "&cDefenses: &w%s\r\n", ext_flag_string( &victim->defenses, defense_flags ) );
      if( !xSAME_BITS( mindex->defenses, victim->defenses ) )
         pager_printf( ch, "&cIndex Defenses: &r%s\r\n", ext_flag_string( &mindex->defenses, defense_flags ) );

      if( !xIS_EMPTY( victim->act ) )
         pager_printf( ch, "&cAct Flags: &w%s\r\n", ext_flag_string( &victim->act, act_flags ) );
      if( !xSAME_BITS( mindex->act, victim->act ) )
         pager_printf( ch, "&cIndex Act Flags: &r%s\r\n", ext_flag_string( &mindex->act, act_flags ) );
   }
   else
   {
      pager_printf( ch, "&cStatus: &w%-10s", can_pkill( victim ) ? "Deadly" :
         is_pkill( victim ) ? "Pre-Deadly" : "Non-Deadly" );
      if( victim->pcdata && victim->pcdata->bestowments )
         pager_printf( ch, "&cBestowments: &w%s\r\n", victim->pcdata->bestowments );
      if( victim->pcdata->clan )
      {
         pager_printf( ch, "   &c%s: &w%s",
            victim->pcdata->clan->clan_type == CLAN_NATION ? "Nation" : "Clan",
            victim->pcdata->clan->name );
      }
      send_to_pager( "\r\n", ch );
      if( get_trust( ch ) >= PERM_HEAD )
      {
         if( victim->desc )
         {
            pager_printf( ch, "&cHost: &w%s   &cDescriptor: &w%d   ",
               victim->desc->host, victim->desc->descriptor );
         }
         pager_printf( ch, "&cTrust: &w%d\r\n", victim->trust );
      }

      {
         MCLASS_DATA *mclass;

         for( mclass = victim->pcdata->first_mclass; mclass; mclass = mclass->next )
            pager_printf( ch, "&cClass: &w%-2.2d/%s Level: %d Exp: %.f Percent %d\r\n",
               mclass->wclass, dis_class_name( mclass->wclass ), mclass->level, mclass->exp, mclass->cpercent );
      }
      pager_printf( ch, "&cRace: &w%-2.2d/%-10s\r\n", victim->race, dis_race_name( victim->race ) );

      if( victim->pcdata->deity )
         pager_printf( ch, "&CDeity: &w%s   ", victim->pcdata->deity->name );
      pager_printf( ch, "&cFavor: &w%-5d   &cGlory: &w%u\r\n",
         victim->pcdata->favor, victim->pcdata->quest_curr );
      pager_printf( ch, "&cThirst: &w%d   &cFull: &w%d   &cDrunk: &w%d\r\n",
         victim->pcdata->condition[COND_THIRST],
         victim->pcdata->condition[COND_FULL],
         victim->pcdata->condition[COND_DRUNK] );
      pager_printf( ch, "&cPlayerFlags: &w%s\r\n", ext_flag_string( &victim->act, plr_flags ) );
      pager_printf( ch, "&cPcflags: &w%s\r\n", ext_flag_string( &victim->pcdata->flags, pc_flags ) );
      if( victim->wait )
         pager_printf( ch, "&cWaitState: &R%d\r\n", victim->wait / 12 );
   }

   if( victim->pIndexData )
      show_track_resets( ch, NULL, victim->pIndexData );
}

CMDF( do_mfind )
{
   MOB_INDEX_DATA *pMobIndex;
   char arg[MIL];
   int hash, nMatch = 0, vnum = 0;
   bool fAll = false;

   set_pager_color( AT_PLAIN, ch );

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Mfind whom?\r\n", ch );
      return;
   }

   if( is_number( arg ) )
      vnum = atoi( arg );
   else if( !str_cmp( arg, "all" ) )
      fAll = true;

   for( hash = 0; hash < MKH; hash++ )
   {
      for( pMobIndex = mob_index_hash[hash]; pMobIndex; pMobIndex = pMobIndex->next )
      {
         if( fAll || vnum == pMobIndex->vnum || nifty_is_name( arg, pMobIndex->name ) )
         {
            nMatch++;
            pager_printf( ch, "[%5d] %s\r\n", pMobIndex->vnum, capitalize( pMobIndex->short_descr ) );
         }
      }
   }

   if( nMatch )
      pager_printf( ch, "Number of matches: %d\n", nMatch );
   else
      send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
}

CMDF( do_ofind )
{
   OBJ_INDEX_DATA *pObjIndex;
   char arg[MIL];
   int hash, nMatch = 0, vnum = 0;
   bool fAll = false;

   set_pager_color( AT_PLAIN, ch );

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Ofind what?\r\n", ch );
      return;
   }

   if( is_number( arg ) )
      vnum = atoi( arg );
   else if( !str_cmp( arg, "all" ) )
      fAll = true;

   for( hash = 0; hash < MKH; hash++ )
   {
      for( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
      {
         if( fAll || vnum == pObjIndex->vnum || nifty_is_name( arg, pObjIndex->name ) )
         {
            nMatch++;
            pager_printf( ch, "[%5d] %s\r\n", pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
         }
      }
   }

   if( nMatch )
      pager_printf( ch, "Number of matches: %d\n", nMatch );
   else
      send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
}

CMDF( do_mwhere )
{
   CHAR_DATA *victim;
   char arg[MIL];
   int vnum = 0;
   bool found;

   set_pager_color( AT_PLAIN, ch );

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Mwhere whom?\r\n", ch );
      return;
   }

   if( is_number( arg ) )
      vnum = atoi( arg );

   found = false;
   for( victim = first_char; victim; victim = victim->next )
   {
      if( !is_npc( victim ) || !victim->in_room )
         continue;
      if( vnum > 0 && victim->pIndexData->vnum != vnum )
         continue;
      else if( vnum <= 0 && !nifty_is_name( arg, victim->name ) )
         continue;
      found = true;
      pager_printf( ch, "&c[&w%5d&c] &w%-28s &c[&w%5d&c] &w%s",
         victim->pIndexData->vnum, victim->short_descr, victim->in_room->vnum, victim->in_room->name );
      if( is_in_wilderness( victim ) )
         pager_printf( ch, " &cCords: X[&w%d&c] Y[&w%d&c]", victim->cords[0], victim->cords[1] );
      send_to_pager( "\r\n", ch );
   }

   if( !found )
      act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
}

CMDF( do_gwhere )
{
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int low = 1, high = MAX_LEVEL, count = 0;
   bool found = false, pmobs = false;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 != NULL && arg1[0] != '\0' )
   {
      if( arg1[0] == '\0' || arg2[0] == '\0' )
      {
         send_to_pager( "\r\n&wUsage:  gwhere | gwhere <low> <high> | gwhere <low> <high> mobs\r\n", ch );
         return;
      }
      low = atoi( arg1 );
      high = atoi( arg2 );
   }
   if( low < 1 || high < low || low > high || high > MAX_LEVEL )
   {
      send_to_pager( "&wInvalid level range.\r\n", ch );
      return;
   }
   argument = one_argument( argument, arg3 );
   if( !str_cmp( arg3, "mobs" ) )
      pmobs = true;

   pager_printf( ch, "\r\n&cGlobal %s locations:&w\r\n", pmobs ? "mob" : "player" );
   if( !pmobs )
   {
      for( d = first_descriptor; d; d = d->next )
      {
         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
         && ( victim = d->character ) && !is_npc( victim ) && victim->in_room
         && can_see( ch, victim ) && victim->level >= low && victim->level <= high )
         {
            found = true;
            pager_printf( ch, "&c(&C%2d&c) &w%-12.12s   [%-5d - %-19.19s]   &c%-25.25s\r\n",
               victim->level, victim->name, victim->in_room->vnum, victim->in_room->area->name, victim->in_room->name );
            count++;
         }
      }
   }
   else
   {
      for( victim = first_char; victim; victim = victim->next )
      {
         if( is_npc( victim ) && victim->in_room && can_see( ch, victim ) && victim->level >= low && victim->level <= high )
         {
            found = true;
            pager_printf( ch, "&c(&C%2d&c) &w%-12.12s   [%-5d - %-19.19s]   &c%-25.25s\r\n",
               victim->level, victim->name, victim->in_room->vnum, victim->in_room->area->name, victim->in_room->name );
            count++;
         }
      }
   }
   pager_printf( ch, "&c%d %s found.\r\n", count, pmobs ? "mobs" : "characters" );
}

CMDF( do_gfighting )
{
   HHF_DATA *hhf, *tmp_hhf = NULL;
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   char arg1[MIL];
   int low = 1, high = MAX_LEVEL, count = 0;
   bool found = false, pmobs = false, phating = false, phunting = false, pfearing = false, pall = false, psummoning = false;

   argument = one_argument( argument, arg1 );
   if( arg1 != NULL && arg1[0] != '\0' )
   {
      if( is_number( arg1 ) )
      {
         low = atoi( arg1 );
         argument = one_argument( argument, arg1 );
         if( arg1 != NULL && arg1[0] != '\0' )
         {
            if( is_number( arg1 ) )
               high = atoi( arg1 );
            argument = one_argument( argument, arg1 );
         }
      }
   }

   if( arg1 != NULL && arg1[0] != '\0' )
   {
      if( !str_cmp( arg1, "mobs" ) )
         pmobs = true;
      else if( !str_cmp( arg1, "all" ) )
         pall = true;
      else if( !str_cmp( arg1, "hating" ) )
         phating = true;
      else if( !str_cmp( arg1, "hunting" ) )
         phunting = true;
      else if( !str_cmp( arg1, "fearing" ) )
         pfearing = true;
      else if( !str_cmp( arg1, "summoning" ) )
         psummoning = true;
      else
      {
         send_to_pager( "&wUsage: gfighting [<low>] [<high>] [all/mobs/hating/hunting/fearing/summoning]\r\n", ch );
         return;
      }
   }

   if( low < 1 || high < low || low > high || high > MAX_LEVEL )
   {
      send_to_pager( "&wInvalid level range.\r\n", ch );
      return;
   }

   if( pall || ( !pmobs && !phating && !phunting && !pfearing && !psummoning ) )
   {
      send_to_pager( "&cGlobal player conflict(s):\r\n", ch );
      for( d = first_descriptor; d; d = d->next )
      {
         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
         && ( victim = d->character ) && !is_npc( victim ) && victim->in_room
         && can_see( ch, victim ) && victim->fighting && victim->level >= low && victim->level <= high )
         {
            found = true;
            pager_printf( ch, "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
               victim->name, victim->level, victim->fighting->who->level,
               ( is_npc( victim->fighting->who ) ? victim->fighting->who->short_descr 
               : victim->fighting->who->name ),
               ( is_npc( victim->fighting->who ) ? victim->fighting->who->pIndexData->vnum : 0 ),
               victim->in_room->area->name, !victim->in_room ? 0 : victim->in_room->vnum );
            count++;
         }
      }
   }

   if( pall || pmobs )
   {
      send_to_pager( "&cGlobal mob conflict(s):\r\n", ch );
      for( victim = first_char; victim; victim = victim->next )
      {
         if( is_npc( victim )
         && victim->in_room && can_see( ch, victim )
         && victim->fighting && victim->level >= low && victim->level <= high )
         {
            found = true;
            pager_printf( ch, "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
               victim->name, victim->level, victim->fighting->who->level,
               is_npc( victim->fighting->who ) ? victim->fighting->who->short_descr : victim->fighting->
               who->name, is_npc( victim->fighting->who ) ? victim->fighting->who->pIndexData->vnum : 0,
               victim->in_room->area->name, !victim->in_room ? 0 : victim->in_room->vnum );
            count++;
         }
      }
   }

   if( pall || pfearing || phating || phunting || psummoning )
   {
      short tmp;

      for( tmp = 0; tmp < 3; tmp++ )
      {
         if( pfearing || ( pall && tmp == 0 ) )
            tmp_hhf = first_fearing;
         else if( psummoning || phating || ( pall && tmp == 1 ) )
            tmp_hhf = first_hating;
         else if( phunting || ( pall && tmp == 2 ) )
            tmp_hhf = first_hunting;

         pager_printf( ch, "&cGlobal mob %s conflict(s):\r\n",
            ( pfearing || ( pall && tmp == 0 ) ) ? "fearing" : 
            ( psummoning || phating || ( pall && tmp == 1 ) ) ? ( psummoning ? "summoning" : "hating" ) : 
            ( phunting || ( pall && tmp == 2 ) ) ? "hunting" : "unknown" );
         for( hhf = tmp_hhf; hhf; hhf = hhf->lnext )
         {
            if( !(victim = hhf->mob) )
            {
               bug( "%s: %s list contains a NULL mob pointer.", __FUNCTION__, 
                  ( pfearing || ( pall && tmp == 0 ) ) ? "Fearing" : 
                  ( phating || ( pall && tmp == 1 ) ) ? "Hating" : 
                  ( phunting || ( pall && tmp == 2 ) ) ? "Hunting" : "Unknown" );
               continue;
            }
            if( !victim->in_room || !can_see( ch, victim ) || victim->level < low || victim->level > high )
               continue;
            if( psummoning && victim->summoning != hhf->who )
               continue;
            found = true;
            pager_printf( ch, "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]%s\r\n",
               victim->name, victim->level, hhf->who ? hhf->who->level : 0,
               hhf->who ? ( is_npc( hhf->who ) ?
               hhf->who->short_descr : hhf->who->name ) : hhf->name,
               hhf->who ? ( is_npc( hhf->who ) ? hhf->who->pIndexData->vnum : 0 ) : 0,
               victim->in_room->area->name, !victim->in_room ? 0 : victim->in_room->vnum,
               ( !psummoning && victim->summoning == hhf->who ) ? " &RSummoning" : "" );
            count++;
         }

         if( !pall ) /* If not in a pall stop after displaying one set */
            break;
      }
   }
   pager_printf( ch, "&c%d %s conflicts located.\r\n", count, pmobs ? "mob" : "character" );
}

/* Added 'show' argument for lowbie imms without ostat -- Blodkai */
/* Made show the default action :) Shaddai */
/* Trimmed size, added vict info, put lipstick on the pig -- Blod */
CMDF( do_bodybag )
{
   CHAR_DATA *owner;
   OBJ_DATA *obj;
   char buf2[MSL], arg1[MIL], arg2[MIL];
   bool found = false, bag = false;

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "&PUsage: bodybag <character> | bodybag <character> yes\r\n", ch );
      if( is_npc( ch ) )
         progbug( "Bodybag - called w/o enough argument(s)", ch );
      return;
   }

   snprintf( buf2, sizeof( buf2 ), "the corpse of %s", arg1 );

   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "yes" ) )
      bag = true;

   if( is_npc( ch ) )
   {
      if( !( owner = get_char_room( ch, arg1 ) ) )
      {
         send_to_char( "Victim must be in the same room as you.\r\n", ch );
         progbug( "Bodybag: victim not in same room", ch );
         return;
      }
      if( is_npc( owner ) )
      {
         progbug( "Bodybag: bodybagging a npc corpse", ch );
         return;
      }
      bag = true;
   }

   pager_printf( ch, "\r\n&P%s remains of %s ... ", bag ? "Retrieving" : "Searching for", capitalize( arg1 ) );
   for( obj = first_corpse; obj; obj = obj->next_corpse )
   {
      if( obj->pIndexData->vnum != OBJ_VNUM_CORPSE_PC || str_cmp( buf2, obj->short_descr ) )
         continue;
      found = true;
      pager_printf( ch, "\r\n&P%sCorpse: &w%-12.12s %2s ",
         bag ? "Bagging " : "", capitalize( arg1 ),
         is_obj_stat( obj, ITEM_CLANCORPSE ) ? "&RPK" : "" );
      if( obj->in_room )
         pager_printf( ch, "&PIn Room: &w%-22.22s &P[&w%5d&P] ", obj->in_room->area->name, obj->in_room->vnum );
      else if( obj->in_obj )
         pager_printf( ch, "&PIn Obj:  &w%-22.22s &P[&w%5d&P] ", obj->in_obj->short_descr, obj->in_obj->pIndexData->vnum );
      else if( obj->carried_by )
         pager_printf( ch, "&PCarried: &w%-30.30s ", obj->carried_by->name );
      else
         pager_printf( ch, "&P%-39.39s ", "Unknown Location!!!" );
      pager_printf( ch, "&PTimer: &w%d\r\n", obj->timer );

      /* Maybe we should only move the ones in a room */
      if( bag && obj->in_room )
      {
         obj_from_room( obj );
         obj = obj_to_char( obj, ch );
         obj->timer = -1;
         save_char_obj( ch );
      }
   }

   if( !found )
   {
      send_to_pager( "&PNo corpse was found.\r\n", ch );
      return;
   }

   if( is_npc( ch ) )
      return;

   send_to_pager( "\r\n", ch );
   for( owner = first_char; owner; owner = owner->next )
   {
      if( is_npc( owner ) )
         continue;
      if( can_see( ch, owner ) && !str_cmp( arg1, owner->name ) )
         break;
   }
   if( !owner )
   {
      pager_printf( ch, "&P%s is not currently online.\r\n", capitalize( arg1 ) );
      return;
   }

   pager_printf( ch, "&P%s (%d) has ", owner->name, owner->level );
   if( owner->pcdata->deity )
      pager_printf( ch, "%d favor with %s (needed to supplicate: %d)\r\n",
         owner->pcdata->favor, owner->pcdata->deity->name, owner->pcdata->deity->scorpse );
   else
      send_to_pager( "no deity.\r\n", ch );
}

/* New owhere by Altrag, 03/14/96 */
CMDF( do_owhere )
{
   OBJ_DATA *obj;
   int icnt = 0, cnt, x, vnum = 0;

   set_pager_color( AT_PLAIN, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Owhere what?\r\n", ch );
      return;
   }

   if( is_number( argument ) )
      vnum = atoi( argument );

   for( obj = first_object; obj; obj = obj->next )
   {
      if( vnum <= 0 && !nifty_is_name( argument, obj->name ) )
         continue;
      else if( vnum > 0 && obj->pIndexData->vnum != vnum )
         continue;

      pager_printf( ch, "(%3d) [%5d] %-28s ", ++icnt, obj->pIndexData->vnum, obj_short( obj ) );

      if( obj->in_room )
      {
         pager_printf( ch, "&cIn room: &w[%5d] &c(&w%s&c)", obj->in_room->vnum,
            obj->in_room->name ? obj->in_room->name : "Not Set" );
         if( is_obj_stat( obj, ITEM_WILDERNESS ) )
            pager_printf( ch, " &cCords: X[&w%d&c] Y[&w%d&c]", obj->cords[0], obj->cords[1] );
         send_to_pager( "\r\n", ch );
      }

      if( obj->carried_by )
         pager_printf( ch, "&cCarried by: &w[%5d] %s\r\n",
            is_npc( obj->carried_by ) ? obj->carried_by->pIndexData->vnum : 0, PERS( obj->carried_by, ch ) );

      if( obj->in_obj )
      {
         OBJ_DATA *tobj = NULL;

         pager_printf( ch, "&cIn object: &w[%5d] %s\r\n", obj->in_obj->pIndexData->vnum, obj_short( obj->in_obj ) );

         tobj = obj->in_obj;
         cnt = 1;
         while( tobj->in_obj )
         {
            for( x = 0; x < cnt; x++ )
               send_to_pager( "  ", ch );
            pager_printf( ch, "&cIn object: &w[%5d] %s\r\n", tobj->in_obj->pIndexData->vnum, obj_short( tobj->in_obj ) );
            tobj = tobj->in_obj;
            cnt++;
         }
         if( tobj )
         {
            for( x = 0; x < cnt; x++ )
                send_to_pager( "  ", ch );
            if( tobj->in_obj )
               pager_printf( ch, "&cIn object: &w[%5d] %s\r\n",
                  tobj->in_obj->pIndexData->vnum, obj_short( tobj->in_obj ) );
            if( tobj->in_room )
            {
               pager_printf( ch, "&cIn room: &w[%5d] &c(&w%s&c)", tobj->in_room->vnum,
                  tobj->in_room->name ? tobj->in_room->name : "Not Set" );
               if( is_obj_stat( tobj, ITEM_WILDERNESS ) )
                  pager_printf( ch, " &cCords: X[&w%d&c] Y[&w%d&c]", tobj->cords[0], tobj->cords[1] );
               send_to_pager( "\r\n", ch );
            }
            if( tobj->carried_by )
               pager_printf( ch, "&cCarried by: &w[%5d] %s\r\n",
                  is_npc( tobj->carried_by ) ? tobj->carried_by->pIndexData->vnum : 0, PERS( tobj->carried_by, ch ) );
         }
      }
   }

   if( !icnt )
      act( AT_PLAIN, "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
   else
      pager_printf( ch, "%d matches.\r\n", icnt );
}

CMDF( do_reboot )
{
   CHAR_DATA *vch;
   char buf[MSL];

   set_char_color( AT_IMMORT, ch );

   if( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" ) )
   {
      send_to_char( "Usage: reboot [mud now/nosave]\r\n", ch );
      return;
   }

   snprintf( buf, sizeof( buf ), "Reboot by %s.", ch->name );
   do_echo( ch, buf );

   /* Save all characters before booting. */
   if( str_cmp( argument, "nosave" ) )
      for( vch = first_char; vch; vch = vch->next )
         if( !is_npc( vch ) )
            save_char_obj( vch );

   mud_down = true;
}

CMDF( do_shutdown )
{
   CHAR_DATA *vch;
   char buf[MSL];

   set_char_color( AT_IMMORT, ch );

   if( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" ) )
   {
      send_to_char( "Usage: shutdown [mud now/nosave]\r\n", ch );
      return;
   }

   snprintf( buf, sizeof( buf ), "Shutdown by %s.", ch->name );
   append_file( ch, SHUTDOWN_FILE, buf );
   mudstrlcat( buf, "\r\n", sizeof( buf ) );
   do_echo( ch, buf );

   /* Save all characters before booting. */
   if( str_cmp( argument, "nosave" ) )
      for( vch = first_char; vch; vch = vch->next )
         if( !is_npc( vch ) )
            save_char_obj( vch );

   mud_down = true;
}

CMDF( do_snoop )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;
   char arg[MIL];

   set_char_color( AT_IMMORT, ch );

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Snoop whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( !victim->desc )
   {      
      send_to_char( "No descriptor to snoop.\r\n", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "Cancelling all snoops.\r\n", ch );
      for( d = first_descriptor; d; d = d->next )
         if( d->snoop_by == ch->desc )
            d->snoop_by = NULL;
      return;
   }
   if( victim->desc->snoop_by )
   {
      send_to_char( "Busy already.\r\n", ch );
      return;
   }

   /*
    * Minimum snoop level... a secret mset value
    * makes the snooper think that the victim is already being snooped
    */
   if( get_trust( victim ) >= get_trust( ch ) || ( victim->pcdata && victim->pcdata->min_snoop > get_trust( ch ) ) )
   {
      send_to_char( "Busy already.\r\n", ch );
      return;
   }

   if( ch->desc )
   {
      for( d = ch->desc->snoop_by; d; d = d->snoop_by )
         if( d->character == victim )
         {
            send_to_char( "No snoop loops.\r\n", ch );
            return;
         }
   }

   victim->desc->snoop_by = ch->desc;
   send_to_char( "Ok.\r\n", ch );
}

CMDF( do_statshield )
{
   CHAR_DATA *victim;
   char arg[MIL];

   set_char_color( AT_IMMORT, ch );

   one_argument( argument, arg );
   if( is_npc( ch ) || get_trust( ch ) < PERM_HEAD )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Statshield which mobile?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "No such mobile.\r\n", ch );
      return;
   }
   if( !is_npc( victim ) )
   {
      send_to_char( "You can only statshield mobiles.\r\n", ch );
      return;
   }
   xTOGGLE_BIT( victim->act, ACT_STATSHIELD );
   if( !xIS_SET( victim->act, ACT_STATSHIELD ) )
      ch_printf( ch, "You have lifted the statshield on %s.\r\n", victim->short_descr );
   else
      ch_printf( ch, "You have applied a statshield to %s.\r\n", victim->short_descr );
}

CMDF( do_minvoke )
{
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *victim = NULL;
   char arg[MIL], arg3[MIL];
   int vnum, mcount = 1, x;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg3 );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: minvoke <vnum>\r\n", ch );
      return;
   }

   if( !is_number( arg ) )
   {
      char arg2[MIL];
      int hash, cnt;
      int count = number_argument( arg, arg2 );

      vnum = -1;
      for( hash = cnt = 0; hash < MKH; hash++ )
         for( pMobIndex = mob_index_hash[hash]; pMobIndex; pMobIndex = pMobIndex->next )
            if( nifty_is_name( arg2, pMobIndex->name ) && ++cnt == count )
            {
               vnum = pMobIndex->vnum;
               break;
            }
      if( vnum == -1 )
      {
         send_to_char( "No such mobile exists.\r\n", ch );
         return;
      }
   }
   else
      vnum = atoi( arg );

   if( arg3 != NULL && is_number( arg3 ) )
      mcount = URANGE( 1, atoi( arg3 ), 10000 );

   if( get_trust( ch ) < PERM_LEADER )
   {
      AREA_DATA *pArea;

      if( is_npc( ch ) )
      {
         send_to_char( "Huh?\r\n", ch );
         return;
      }
      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to invoke this mobile.\r\n", ch );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\r\n", ch );
         return;
      }
   }
   if( !( pMobIndex = get_mob_index( vnum ) ) )
   {
      send_to_char( "No mobile has that vnum.\r\n", ch );
      return;
   }

   for( x = 0; x < mcount; x++ )
   {
      victim = create_mobile( pMobIndex );
      if( victim )
      {
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
      }
   }
   if( !victim )
      return;
   act( AT_IMMORT, "$n invokes $N!", ch, NULL, victim, TO_ROOM );
   ch_printf( ch, "&YYou invoke %s (&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\r\n", pMobIndex->short_descr, pMobIndex->vnum, pMobIndex->name, victim->level );
}

CMDF( do_oinvoke )
{
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   char arg1[MIL], arg2[MIL];
   int vnum, level;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: oinvoke <vnum> <level>.\r\n", ch );
      return;
   }
   if( arg2 == NULL || arg2[0] == '\0' )
      level = ch->level;
   else
   {
      if( !is_number( arg2 ) )
      {
         send_to_char( "Usage:  oinvoke <vnum> <level>\r\n", ch );
         return;
      }
      level = atoi( arg2 );
      if( level < 0 || level > ch->level )
      {
         send_to_char( "Limited to your trust level.\r\n", ch );
         return;
      }
   }
   if( !is_number( arg1 ) )
   {
      char arg[MIL];
      int hash, cnt;
      int count = number_argument( arg1, arg );

      vnum = -1;
      for( hash = cnt = 0; hash < MKH; hash++ )
         for( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
            if( nifty_is_name( arg, pObjIndex->name ) && ++cnt == count )
            {
               vnum = pObjIndex->vnum;
               break;
            }
      if( vnum == -1 )
      {
         send_to_char( "No such object exists.\r\n", ch );
         return;
      }
   }
   else
      vnum = atoi( arg1 );

   if( get_trust( ch ) < PERM_LEADER )
   {
      AREA_DATA *pArea;

      if( is_npc( ch ) )
      {
         send_to_char( "Huh?\r\n", ch );
         return;
      }
      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to invoke this object.\r\n", ch );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\r\n", ch );
         return;
      }
   }
   if( !( pObjIndex = get_obj_index( vnum ) ) )
   {
      send_to_char( "No object has that vnum.\r\n", ch );
      return;
   }

   if( level == 0 )
   {
      AREA_DATA *temp_area;

      if( !( temp_area = get_area_obj( pObjIndex ) ) )
         level = ch->level;
      else
         level = URANGE( 0, pObjIndex->level, MAX_LEVEL );
   }

   if( !( obj = create_object( pObjIndex, level ) ) )
   {
      send_to_char( "Failed to create object.\r\n", ch );
      bug( "%s: Failed to create_object for %d.\r\n", __FUNCTION__, pObjIndex->vnum );
      return;
   }
   if( !can_wear( obj, ITEM_NO_TAKE ) )
      obj = obj_to_char( obj, ch );
   else
   {
      obj = obj_to_room( obj, ch->in_room );
      if( is_in_wilderness( ch ) )
      {
         xSET_BIT( obj->extra_flags, ITEM_WILDERNESS );
         obj->cords[0] = ch->cords[0];
         obj->cords[1] = ch->cords[1];
      }
      act( AT_IMMORT, "$n fashions $p from ether!", ch, obj, NULL, TO_ROOM );
   }
   ch_printf( ch, "&YYou invoke %s (&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\r\n",
      pObjIndex->short_descr, pObjIndex->vnum, pObjIndex->name, obj->level );
}

CMDF( do_purge )
{
   CHAR_DATA *victim = NULL;
   OBJ_DATA *obj = NULL, *obj_next = NULL;
   char arg[MIL];

   set_char_color( AT_IMMORT, ch );

   one_argument( argument, arg );
   /* 'purge' the room */
   if( arg == NULL || arg[0] == '\0' )
   {
      CHAR_DATA *vnext = NULL;

      if( !ch->in_room )
      {
         bug( "%s: (%s) in NULL room.", __FUNCTION__, ch->name );
         return;
      }

      for( victim = ch->in_room->first_person; victim; victim = vnext )
      {
         vnext = victim->next_in_room;
         if( is_npc( victim ) && victim != ch )
            extract_char( victim, true );
      }

      for( obj = ch->in_room->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         extract_obj( obj );
      }

      /* Storage check */
      if( xIS_SET( ch->in_room->room_flags, ROOM_STORAGEROOM ) )
         save_storage( ch->in_room );
         
      act( AT_IMMORT, "$n purges the room!", ch, NULL, NULL, TO_ROOM );
      act( AT_IMMORT, "You have purged the room!", ch, NULL, NULL, TO_CHAR );
      return;
   }

   /*
    * fixed to get things in room first -- i.e., purge portal (obj),
    * * no more purging mobs with that keyword in another room first
    * * -- Tri 
    */
   if( !( victim = get_char_room( ch, arg ) ) && !( obj = get_obj_here( ch, arg ) ) )
   {
      if( !( victim = get_char_world( ch, arg ) ) && !( obj = get_obj_world( ch, arg ) ) )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }
   }

   /* Single object purge in room for high level purge - Scryn 8/12*/
   if( obj )
   {
      ROOM_INDEX_DATA *room;
      bool storage = false;

      separate_obj( obj );
      if( ( victim = obj->carried_by ) && victim != ch )
         act( AT_IMMORT, "$n purges $p.", ch, obj, victim, TO_VICT );
      else
         act( AT_IMMORT, "$n purges $p.", ch, obj, NULL, TO_ROOM );

      if( ( room = obj->in_room ) )
         storage = true;

      act( AT_IMMORT, "You make $p disappear in a puff of smoke!", ch, obj, NULL, TO_CHAR );
      extract_obj( obj );

      /* Storage check */
      if( room && storage && xIS_SET( room->room_flags, ROOM_STORAGEROOM ) )
         save_storage( room );

      return;
   }

   if( !is_npc( victim ) )
   {
      send_to_char( "You can't purge a PC.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You can't purge yourself!\r\n", ch );
      return;
   }

   act( AT_IMMORT, "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
   act( AT_IMMORT, "You make $N disappear in a puff of smoke!", ch, NULL, victim, TO_CHAR );
   extract_char( victim, true );
}

CMDF( do_low_purge )
{
   CHAR_DATA *victim = NULL;
   OBJ_DATA *obj = NULL;
   char arg[MIL];

   set_char_color( AT_IMMORT, ch );

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Purge what?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) && !( obj = get_obj_here( ch, arg ) ) )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }

   if( obj )
   {
      ROOM_INDEX_DATA *room;
      bool storage = false;

      separate_obj( obj );
      if( ( victim = obj->carried_by ) && victim != ch )
         act( AT_IMMORT, "$n purges $p.", ch, obj, victim, TO_VICT );
      else
         act( AT_IMMORT, "$n purges $p!", ch, obj, NULL, TO_ROOM );

      if( ( room = obj->in_room ) )
         storage = true;

      act( AT_IMMORT, "You make $p disappear in a puff of smoke!", ch, obj, NULL, TO_CHAR );
      extract_obj( obj );

      /* Storage check */
      if( room && storage && xIS_SET( room->room_flags, ROOM_STORAGEROOM ) )
         save_storage( room );

      return;
   }

   if( !is_npc( victim ) )
   {
      send_to_char( "You can't purge a PC.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You can't purge yourself!\r\n", ch );
      return;
   }

   act( AT_IMMORT, "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
   act( AT_IMMORT, "You make $N disappear in a puff of smoke!", ch, NULL, victim, TO_CHAR );
   extract_char( victim, true );
}

CMDF( do_advance )
{
   CHAR_DATA *victim;
   MCLASS_DATA *mclass;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int level, iLevel, sn;
   bool call = false;

   if( !is_immortal( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   set_char_color( AT_IMMORT, ch );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' || arg3 == NULL || arg3[0] == '\0' || !is_number( arg3 ) )
   {
      send_to_char( "Usage: advance <character> <class>/all <level>\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "That character is not in the room.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "You can't advance a mobile.\r\n", ch );
      return;

   }

   if( ch != victim && get_trust( ch ) <= get_trust( victim ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   if( ( level = atoi( arg3 ) ) < 1 || level > MAX_LEVEL )
   {
      ch_printf( ch, "Level range is 1 to %d.\r\n", MAX_LEVEL );
      return;
   }

   if( !str_cmp( arg2, "all" ) )
      call = true;

   for( mclass = victim->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      if( !call && str_cmp( dis_class_name( mclass->wclass ), arg2 ) )
         continue;
      if( level < mclass->level )
      {
         set_char_color( AT_IMMORT, victim );
         ch_printf( ch, "Demoting %s from level %d to level %d for class %s!\r\n", victim->name, mclass->level, level, dis_class_name( mclass->wclass ) );
         ch_printf( victim, "Cursed and forsaken! The gods have lowered your level for class %s...\r\n", dis_class_name( mclass->wclass ) );
         mclass->level = level;
         mclass->exp = 0;
         advance_level( victim );
         for( sn = 0; sn <= MAX_SKILL; sn++ )
         {
            iLevel = get_adept( victim, sn );
            if( victim->pcdata->learned[sn] > iLevel )
            {
               victim->pcdata->learned[sn] = iLevel;
               if( iLevel <= 0 )
                  victim->practice++;
            }
         }
      }
      else if( level == mclass->level )
      {
         ch_printf( ch, "They are already at level %d for class %s.\r\n", mclass->level, dis_class_name( mclass->wclass ) );
         if( !call )
            return;
      }
      else
      {
         set_char_color( AT_IMMORT, victim );
         ch_printf( ch, "Raising %s from level %d to level %d for class %s!\r\n", victim->name, mclass->level, level, dis_class_name( mclass->wclass ) );
         ch_printf( victim, "The gods feel fit to raise your level for class %s!\r\n", dis_class_name( mclass->wclass ) );

         for( ; mclass->level < level; )
         {
            ++mclass->level;
            mclass->exp = 0;
            advance_level( victim );
         }
      }
      if( !call )
         return;
   }
   if( !call )
      ch_printf( ch, "They don't have a %s class to change to level %d.\r\n", arg2, level );
}

/* May be used to increase or decrease someones permission level. */
CMDF( do_elevate )
{
   CHAR_DATA *victim;
   char arg[MIL];
   int oldtrust;

   set_char_color( AT_IMMORT, ch );
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: elevate <char> [imm/builder/leader/head/imp]\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "You can't elevate yourself.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You can't do anything with someone higher or the same trust as you.\r\n", ch );
      return;
   }
   if( get_trust( victim ) < PERM_IMM )
   {
      ch_printf( ch, "%s isn't an immortal yet.\r\n", victim->name );
      return;
   }

   argument = one_argument( argument, arg );
   oldtrust = get_trust( victim );
   if( arg == NULL || arg[0] == '\0' )
   {
      if( ( victim->trust + 1 ) >= PERM_MAX )
      {
         send_to_char( "You can't advance them any higher.\r\n", ch );
         return;
      }
      victim->trust++;
   }
   else
   {
      int temp;

      temp = get_flag( arg, perms_flag, PERM_MAX );
      if( temp <= PERM_ALL || temp == get_trust( victim ) || temp > get_trust( ch ) || temp >= PERM_MAX )
      {
         send_to_char( "Invalid permission.\r\n", ch );
         return;
      }
      victim->trust = temp;
   }
   /* Decreased their permission level */
   if( oldtrust > get_trust( victim ) )
      ch_printf( ch, "Dropping %s from %s to %s.\r\n", victim->name, perms_flag[oldtrust], perms_flag[victim->trust] );
   else
      ch_printf( ch, "Elevating %s from %s to %s.\r\n", victim->name, perms_flag[oldtrust], perms_flag[victim->trust] );
   act( AT_IMMORT, "$n begins to chant softly... then makes some arcane gestures...", ch, NULL, NULL, TO_ROOM );
   set_char_color( AT_IMMORT, victim );
   ch_printf( victim, "%s made you %s.\r\n", ch->name, perms_flag[victim->trust] );
   save_char_obj( victim );
   make_wizlist( );
}

CMDF( do_immortalize )
{
   CHAR_DATA *victim;
   MCLASS_DATA *mclass;
   char arg[MIL];
   int iLevel;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage:  immortalize <char>\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "That player is not here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   if( get_trust( victim ) >= PERM_IMM )
   {
      ch_printf( ch, "Don't be silly, %s is already immortal.\r\n", victim->name );
      return;
   }

   send_to_char( "Immortalizing a player...\r\n", ch );
   set_char_color( AT_IMMORT, victim );
   act( AT_IMMORT, "$n begins to chant softly... then raises $s arms to the sky...", ch, NULL, NULL, TO_ROOM );
   set_char_color( AT_WHITE, victim );
   send_to_char( "You suddenly feel very strange...\r\n\r\n", victim );
   set_char_color( AT_WHITE, victim );

   for( ; victim->level < MAX_LEVEL; )
   {
      for( mclass = victim->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         ++mclass->level;
         mclass->exp = 0;
      }
      advance_level( victim );
   }

   for( iLevel = 0; iLevel < top_sn; iLevel++ )
      victim->pcdata->learned[iLevel] = 100;
   send_to_char( "You know all available spells/skills/tongues/weapons now.\r\n", victim );
   send_to_char( "All available spells/skills/tongues/weapons have been set on them.\r\n", ch );

   for( iLevel = 0; iLevel < STAT_MAX; iLevel++ )
      victim->perm_stats[iLevel] = ( MAX_LEVEL + 25 );
   send_to_char( "You now have the max stats an immortal can have.\r\n", victim );
   send_to_char( "Their stats have been set to the max an immortal can have.\r\n", ch );

   victim->max_hit = 30000;
   victim->hit = victim->max_hit;
   victim->max_move = 30000;
   victim->move = victim->max_move;
   victim->max_mana = 30000;
   victim->mana = victim->max_mana;

   victim->trust = PERM_IMM;
   interpret( ch, (char *)"listen all" );

#ifdef IMC
   imc_initchar( victim );
#endif
   save_char_obj( victim );
   make_wizlist( );
}

CMDF( do_strip )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj_next, *obj_lose;
   int count = 0;

   set_char_color( AT_OBJECT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Strip who?\r\n", ch );
      return;
   }
   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They're not here.\r\n", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "Kinky.\r\n", ch );
      return;
   }
   if( !is_npc( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You haven't the power to succeed against them.\r\n", ch );
      return;
   }
   act( AT_OBJECT, "Searching $N ...", ch, NULL, victim, TO_CHAR );
   for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
   {
      obj_next = obj_lose->next_content;
      obj_from_char( obj_lose );
      obj_to_char( obj_lose, ch );
      pager_printf( ch, "  &G... %s (&g%s) &Gtaken.\r\n", capitalize( obj_lose->short_descr ), obj_lose->name );
      count++;
   }
   if( !count )
      send_to_pager( "&GNothing found to take.\r\n", ch );
}

CMDF( do_restore )
{
   CHAR_DATA *vch, *vch_next;
   char arg[MIL];
   bool boost = false;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Restore whom?\r\n", ch );
      return;
   }
   if( argument && !str_cmp( argument, "boost" ) && get_trust( ch ) >= PERM_HEAD )
   {
      send_to_char( "Boosting!\r\n", ch );
      boost = true;
   }
   if( !str_cmp( arg, "all" ) )
   {
      if( !ch->pcdata )
         return;

      if( get_trust( ch ) < PERM_HEAD )
      {
         if( is_npc( ch ) )
         {
            send_to_char( "You can't do that.\r\n", ch );
            return;
         }
         else
         {
            if( current_time - last_restore_all_time < RESTORE_INTERVAL )
            {
               send_to_char( "Sorry, you can't do a restore all yet.\r\n", ch );
               do_restoretime( ch, (char *)"" );
               return;
            }
         }
      }
      last_restore_all_time = current_time;
      ch->pcdata->restore_time = current_time;
      save_char_obj( ch );
      send_to_char( "Beginning 'restore all' ...\r\n", ch );
      for( vch = first_char; vch; vch = vch_next )
      {
         vch_next = vch->next;

         if( !is_npc( vch ) && !is_immortal( vch ) && !can_pkill( vch ) && !in_arena( vch ) )
         {
            if( boost )
            {
               vch->hit = UMAX( vch->max_hit, ( int )( vch->max_hit * 1.5 ) );
               vch->mana = UMAX( vch->max_mana, ( int )( vch->max_mana * 1.5 ) );
               vch->move = UMAX( vch->max_move, ( int )( vch->max_move * 1.5 ) );
            }
            else
            {
               vch->hit = vch->max_hit;
               vch->mana = vch->max_mana;
               vch->move = vch->max_move;
            }
            update_pos( vch );
            act( AT_IMMORT, "$n has restored you.", ch, NULL, vch, TO_VICT );
         }
      }
      send_to_char( "Restored.\r\n", ch );
      return;
   }

   if( !( vch = get_char_world( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_LEADER && vch != ch && !( is_npc( vch ) && xIS_SET( vch->act, ACT_PROTOTYPE ) ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   if( boost )
   {
      vch->hit = UMAX( vch->max_hit, ( int )( vch->max_hit * 1.5 ) );
      vch->mana = UMAX( vch->max_mana, ( int )( vch->max_mana * 1.5 ) );
      vch->move = UMAX( vch->max_move, ( int )( vch->max_move * 1.5 ) );
   }
   else
   {
      vch->hit = vch->max_hit;
      vch->mana = vch->max_mana;
      vch->move = vch->max_move;
   }
   update_pos( vch );
   if( ch != vch )
     act( AT_IMMORT, "$n has restored you.", ch, NULL, vch, TO_VICT );
   send_to_char( "Restored.\r\n", ch );
}

CMDF( do_restoretime )
{
   long int time_passed;
   int hours = 0, minutes = 0, days = 0;

   set_char_color( AT_IMMORT, ch );

   if( !last_restore_all_time )
      send_to_char( "There has been no restore all since reboot.\r\n", ch );
   else
   {
      time_passed = current_time - last_restore_all_time;
      days = ( int )( time_passed / 43200 );
      time_passed -= ( days * 43200 );
      hours = ( int )( time_passed / 3600 );
      time_passed -= ( hours * 3600 );
      minutes = ( int )( time_passed / 60 );
      time_passed -= ( minutes * 60 );
      send_to_char( "The last restore all was ", ch );
      if( days )
         ch_printf( ch, "%d day%s ", days, days > 1 ? "s" : "" );
      if( hours )
         ch_printf( ch, "%d hour%s ", hours, hours > 1 ? "s" : "" );
      if( minutes )
         ch_printf( ch, "%d minute%s ", minutes, minutes > 1 ? "s" : "" );
      if( time_passed )
         ch_printf( ch, "%ld second%s ", time_passed, time_passed > 1 ? "s" : "" );
      send_to_char( "ago.\r\n", ch );
   }

   if( !ch->pcdata )
      return;

   if( !ch->pcdata->restore_time )
   {
      send_to_char( "You have never done a restore all.\r\n", ch );
      return;
   }

   time_passed = current_time - ch->pcdata->restore_time;
   days = ( int )( time_passed / 43200 );
   time_passed -= ( days * 43200 );
   hours = ( int )( time_passed / 3600 );
   time_passed -= ( hours * 3600 );
   minutes = ( int )( time_passed / 60 );
   time_passed -= ( minutes * 60 );
   send_to_char( "Your last restore all was ", ch );
   if( days )
      ch_printf( ch, "%d day%s ", days, days > 1 ? "s" : "" );
   if( hours )
      ch_printf( ch, "%d hour%s ", hours, hours > 1 ? "s" : "" );
   if( minutes )
      ch_printf( ch, "%d minute%s ", minutes, minutes > 1 ? "s" : "" );
   if( time_passed )
      ch_printf( ch, "%ld second%s ", time_passed, time_passed > 1 ? "s" : "" );
   send_to_char( "ago.\r\n", ch );
}

CMDF( do_freeze )
{
   CHAR_DATA *victim;

   set_char_color( AT_LBLUE, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Freeze whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   set_char_color( AT_LBLUE, victim );
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed, and they saw...\r\n", ch );
      ch_printf( victim, "%s is attempting to freeze you.\r\n", ch->name );
      return;
   }
   xTOGGLE_BIT( victim->act, PLR_FREEZE );
   if( !xIS_SET( victim->act, PLR_FREEZE ) )
   {
      send_to_char( "Your frozen form suddenly thaws.\r\n", victim );
      ch_printf( ch, "%s is now unfrozen.\r\n", victim->name );
   }
   else
   {
      send_to_char( "A godly force turns your body to ice!\r\n", victim );
      ch_printf( ch, "You have frozen %s.\r\n", victim->name );
   }
   save_char_obj( victim );
}

CMDF( do_log )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Log whom?\r\n", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      fLogAll = !fLogAll;
      if( !fLogAll )
         send_to_char( "Log ALL off.\r\n", ch );
      else
         send_to_char( "Log ALL on.\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You can't log/unlog yourself.\r\n", ch );
      return;
   }

   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You don't have the power to log them.\r\n", ch );
      return;
   }

   victim->logged = !victim->logged;
   if( !victim->logged )
      ch_printf( ch, "LOG removed from %s.\r\n", victim->name );
   else
      ch_printf( ch, "LOG applied to %s.\r\n", victim->name );
   save_char_obj( victim );
}

CMDF( do_noemote )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Noemote whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }
   set_char_color( AT_IMMORT, victim );
   xTOGGLE_BIT( victim->act, PLR_NO_EMOTE );
   if( !xIS_SET( victim->act, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can emote again.\r\n", victim );
      ch_printf( ch, "NOEMOTE removed from %s.\r\n", victim->name );
   }
   else
   {
      send_to_char( "You can't emote!\r\n", victim );
      ch_printf( ch, "NOEMOTE applied to %s.\r\n", victim->name );
   }
}

CMDF( do_notell )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Notell whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }
   set_char_color( AT_IMMORT, victim );
   xTOGGLE_BIT( victim->act, PLR_NO_TELL );
   if( !xIS_SET( victim->act, PLR_NO_TELL ) )
   {
      send_to_char( "You can use tells again.\r\n", victim );
      ch_printf( ch, "NOTELL removed from %s.\r\n", victim->name );
   }
   else
   {
      send_to_char( "You can't use tells!\r\n", victim );
      ch_printf( ch, "NOTELL applied to %s.\r\n", victim->name );
   }
}

CMDF( do_notitle )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Notitle whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }
   set_char_color( AT_IMMORT, victim );
   xTOGGLE_BIT( victim->pcdata->flags, PCFLAG_NOTITLE );
   if( !xIS_SET( victim->pcdata->flags, PCFLAG_NOTITLE ) )
   {
      send_to_char( "You can set your own title again.\r\n", victim );
      ch_printf( ch, "NOTITLE removed from %s.\r\n", victim->name );
   }
   else
   {
      set_title( victim, (char *)"can't set their title now." );
      send_to_char( "You can't set your own title!\r\n", victim );
      ch_printf( ch, "NOTITLE set on %s.\r\n", victim->name );
   }
}

CMDF( do_silence )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Silence whom?", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }
   set_char_color( AT_IMMORT, victim );
   if( xIS_SET( victim->act, PLR_SILENCE ) )
      send_to_char( "Player already silenced, use unsilence to remove.\r\n", ch );
   else
   {
      xSET_BIT( victim->act, PLR_SILENCE );
      send_to_char( "You can't use channels!\r\n", victim );
      ch_printf( ch, "You SILENCE %s.\r\n", victim->name );
   }
}

/* Much better than toggling this with do_silence, yech --Blodkai */
CMDF( do_unsilence )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Unsilence whom?\r\n", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }
   if( is_npc( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\r\n", ch );
      return;
   }
   set_char_color( AT_IMMORT, victim );
   if( xIS_SET( victim->act, PLR_SILENCE ) )
   {
      xREMOVE_BIT( victim->act, PLR_SILENCE );
      send_to_char( "You can use channels again.\r\n", victim );
      ch_printf( ch, "SILENCE removed from %s.\r\n", victim->name );
   }
   else
      send_to_char( "That player is not silenced.\r\n", ch );
}

CMDF( do_peace )
{
   CHAR_DATA *rch;

   act( AT_IMMORT, "$n booms, 'PEACE!'", ch, NULL, NULL, TO_ROOM );
   act( AT_IMMORT, "You boom, 'PEACE!'", ch, NULL, NULL, TO_CHAR );
   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( rch->fighting )
      {
         stop_fighting( rch, true );
         do_sit( rch, (char *)"" );
      }

      /* Added by Narn, Nov 28/95 */
      stop_hating( rch, NULL, true );
      stop_hunting( rch, NULL, true );
      stop_fearing( rch, NULL, true );
   }

   send_to_char( "&YOk.\r\n", ch );
}

CMDF( do_users )
{
   DESCRIPTOR_DATA *d;
   int count = 0;

   set_pager_color( AT_PLAIN, ch );
   send_to_pager( "Desc|     Constate      |Idle|    Player    | HostIP          | Port\r\n", ch );
   send_to_pager( "----+-------------------+----+--------------+-----------------+-------\r\n", ch );
   for( d = first_descriptor; d; d = d->next )
   {
      if( d->character && !can_see( ch, d->character ) )
         continue;
      if( argument && argument[0] != '\0' )
      {
         if( str_prefix( argument, d->host ) && ( !d->character || str_prefix( argument, d->character->name ) ) )
            continue;
      }
      count++;
      pager_printf( ch, " %3d| %-17s |%4d| %-12s | %-15s | %d\r\n", d->descriptor, con_state( d->connected ),
         d->idle, d->character ? d->character->name : "(None!)", d->host, d->port );
   }
   pager_printf( ch, "%d user%s.\r\n", count, count == 1 ? "" : "s" );
}

/* Thanks to Grodyn for pointing out bugs in this function. */
CMDF( do_force )
{
   char arg[MIL];
   bool mobsonly;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Force whom to do what?\r\n", ch );
      return;
   }

   mobsonly = get_trust( ch ) < sysdata.perm_forcepc;

   if( !str_cmp( arg, "all" ) )
   {
      CHAR_DATA *vch, *vch_next;

      if( mobsonly )
      {
         send_to_char( "Force whom to do what?\r\n", ch );
         return;
      }

      for( vch = first_char; vch; vch = vch_next )
      {
         vch_next = vch->next;

         if( !is_npc( vch ) && get_trust( vch ) < get_trust( ch ) )
         {
            act( AT_IMMORT, "$n forces you to '$t'.", ch, argument, vch, TO_VICT );
            interpret( vch, argument );
         }
      }
   }
   else
   {
      CHAR_DATA *victim;

      if( !( victim = get_char_world( ch, arg ) ) )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }

      if( victim == ch )
      {
         send_to_char( "Aye aye, right away!\r\n", ch );
         return;
      }

      if( ( get_trust( victim ) >= get_trust( ch ) ) || ( mobsonly && !is_npc( victim ) ) )
      {
         send_to_char( "Do it yourself!\r\n", ch );
         return;
      }

      if( get_trust( ch ) < PERM_LEADER && is_npc( victim ) && !str_prefix( "mp", argument ) )
      {
         send_to_char( "You can't force a mob to do that!\r\n", ch );
         return;
      }
      act( AT_IMMORT, "$n forces you to '$t'.", ch, argument, victim, TO_VICT );
      interpret( victim, argument );
   }

   send_to_char( "Ok.\r\n", ch );
}

CMDF( do_invis )
{
   int level;

   if( !ch || is_npc( ch ) )
      return;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      if( ch->pcdata->wizinvis < 1 )
         ch->pcdata->wizinvis = get_trust( ch );
      xTOGGLE_BIT( ch->act, PLR_WIZINVIS );
      if( xIS_SET( ch->act, PLR_WIZINVIS ) )
      {
         act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly vanish into thin air.\r\n", ch );
      }
      else
      {
         act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly fade back into existence.\r\n", ch );
      }
      return;
   }

   if( !is_number( argument ) )
   {
      send_to_char( "Usage: invis | invis <level>\r\n", ch );
      return;
   }

   if( ( level = atoi( argument ) ) < 1 || level > get_trust( ch ) )
   {
      ch_printf( ch, "Invalid permission level...valid range for you is 1 to %d.\r\n", get_trust( ch ) );
      return;
   }

   ch->pcdata->wizinvis = level;
   ch_printf( ch, "Wizinvis level set to %d.\r\n", ch->pcdata->wizinvis );
}

CMDF( do_holylight )
{
   if( is_npc( ch ) )
      return;
   set_char_color( AT_IMMORT, ch );
   xTOGGLE_BIT( ch->act, PLR_HOLYLIGHT );
   ch_printf( ch, "Holylight mode %s.\r\n", xIS_SET( ch->act, PLR_HOLYLIGHT ) ? "on" : "off" );
}

CMDF( do_cmdtable )
{
   CMDTYPE *cmd;
   int hash, cnt = 0;
   bool lag = false;

   if( !strcmp( argument, "lag" ) )
      lag = true;
   set_pager_color( AT_IMMORT, ch );
   ch_printf( ch, "Commands and Number of %s This Run:\r\n", lag ? "Lags" : "Uses" );
   set_pager_color( AT_PLAIN, ch );
   for( hash = 0; hash < 126; hash++ )
   {
      for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
      {
         if( ( lag && !cmd->lag_count ) || ( !lag && !cmd->userec.num_uses ) )
            continue;
         pager_printf( ch, "%-10.10s %4d ", cmd->name, lag ? cmd->lag_count : cmd->userec.num_uses );
         if( ++cnt == 4 )
         {
            send_to_pager( "\r\n", ch );
            cnt = 0;
         }
      }
   }
   if( cnt != 0 )
      send_to_char( "\r\n", ch );
}

void mortalize( CHAR_DATA *victim )
{
   AREA_DATA *pArea, *next_pArea;
   char buf[MSL], buf2[MSL];

   if( !victim )
      return;
   victim->trust = PERM_ALL;
   STRFREE( victim->pcdata->rank );
   victim->pcdata->wizinvis = PERM_ALL;
   xREMOVE_BIT( victim->act, PLR_WIZINVIS );

   snprintf( buf, sizeof( buf ), "%s%s", GOD_DIR, capitalize( victim->name ) );
   if( !remove( buf ) )
      log_string( "Player's immortal data destroyed. " );
   snprintf( buf2, sizeof( buf2 ), "%s.are", capitalize( victim->name ) );
   for( pArea = first_build; pArea; pArea = next_pArea )
   {
      next_pArea = pArea->next;

      if( !str_cmp( pArea->filename, buf2 ) )
      {
         if( IS_SET( pArea->status, AREA_LOADED ) )
            fold_area( pArea, buf2, false );
         close_area( pArea );
         snprintf( buf, sizeof( buf ), "%s%s", BUILD_DIR, buf2 );
         snprintf( buf2, sizeof( buf2 ), "%s.bak", buf );
         if( !rename( buf, buf2 ) )
            log_string( "Player's area data destroyed.  Area saved as backup." );
      }
   }
   victim->pcdata->area = NULL;
   make_wizlist( );
}

DESCRIPTOR_DATA *temp_descriptor( void )
{
   DESCRIPTOR_DATA *d;

   CREATE( d, DESCRIPTOR_DATA, 1 );
   if( d )
   {
      d->next = NULL;
      d->prev = NULL;
      d->connected = CON_GET_NAME;
      d->outsize = 2000;
      CREATE( d->outbuf, char, d->outsize );
   }
   return d;
}

CMDF( do_mortalize )
{
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   char name[256];
   int old_room_vnum;
   bool loaded;

   set_char_color( AT_IMMORT, ch );
   one_argument( argument, name );
   if( name == NULL || name[0] == '\0' )
   {
      send_to_char( "Usage: mortalize <player>\r\n", ch );
      return;
   }

   for( victim = first_char; victim; victim = victim->next )
   {
      if( is_npc( victim ) )
         continue;
      if( can_see( ch, victim ) && !str_cmp( name, victim->name ) )
         break;
   }

   if( victim )
   {
      if( get_trust( victim ) < PERM_IMM )
      {
         send_to_char( "They are already mortal.\r\n", ch );
         return;
      }
      if( get_trust( victim ) >= get_trust( ch ) )
      {
         send_to_char( "I think you'd better leave that player alone!\r\n", ch );
         ch_printf( ch, "%s just tried to mortalize you!\r\n", ch->name );
         return;
      }
      mortalize( victim );
      return;
   }

   name[0] = UPPER( name[0] );
   if( !valid_pfile( name ) )
   {
      send_to_char( "No such player.\r\n", ch );
      return;
   }

   if( !( d = temp_descriptor( ) ) )
   {
      send_to_char( "Failed to create a temp descriptor.\r\n", ch );
      return;
   }

   loaded = load_char_obj( d, name, false, false );
   add_char( d->character );
   if( get_trust( d->character ) >= get_trust( ch ) )
   {
      send_to_char( "I think you'd better leave that player alone!\r\n", ch );
      d->character->desc = NULL;
      victim = d->character;
      d->character = NULL;
      DISPOSE( d->outbuf );
      DISPOSE( d );
      do_quit( victim, (char *)"" );
      return;
   }
   if( get_trust( d->character ) < PERM_IMM )
   {
      send_to_char( "They are already mortal.\r\n", ch );
      d->character->desc = NULL;
      victim = d->character;
      d->character = NULL;
      DISPOSE( d->outbuf );
      DISPOSE( d );
      do_quit( victim, (char *)"" );
      return;
   }
   old_room_vnum = d->character->in_room->vnum;
   char_to_room( d->character, ch->in_room );
   d->character->desc = NULL;
   victim = d->character;
   d->character = NULL;
   DISPOSE( d->outbuf );
   DISPOSE( d );
   mortalize( victim );
   do_quit( victim, (char *)"" );
}

/* Load up a player file */
CMDF( do_loadup )
{
   CHAR_DATA *temp;
   DESCRIPTOR_DATA *d;
   char name[256], buf[MSL];
   int old_room_vnum;
   bool loaded;

   set_char_color( AT_IMMORT, ch );

   one_argument( argument, name );
   if( name == NULL || name[0] == '\0' )
   {
      send_to_char( "Usage: loadup <playername>\r\n", ch );
      return;
   }
   for( temp = first_char; temp; temp = temp->next )
   {
      if( is_npc( temp ) )
         continue;
      if( can_see( ch, temp ) && !str_cmp( name, temp->name ) )
         break;
   }
   if( temp )
   {
      send_to_char( "They are already playing.\r\n", ch );
      return;
   }

   name[0] = UPPER( name[0] );
   if( !valid_pfile( name ) )
   {
      send_to_char( "&YNo such player exists.\r\n", ch );
      return;
   }

   if( !( d = temp_descriptor( ) ) )
   {
      send_to_char( "Failed to create a temp descriptor.\r\n", ch );
      return;
   }

   loaded = load_char_obj( d, name, false, false );
   if( !d->character )
   {
      DISPOSE( d->outbuf );
      DISPOSE( d );
      log_printf( "%s: Bad player file %s.", __FUNCTION__, name );
      send_to_char( "That player file is corrupt.\r\n", ch );
      return;
   }
   add_char( d->character );
   if( get_trust( d->character ) >= get_trust( ch ) )
   {
      temp = d->character;
      d->character->desc = NULL;
      d->character = NULL;
      DISPOSE( d->outbuf );
      DISPOSE( d );
      send_to_char( "I think you'd better leave that player alone!\r\n", ch );
      do_quit( temp, (char *)"" );
      return;
   }
   old_room_vnum = d->character->in_room->vnum;
   char_to_room( d->character, ch->in_room );
   char_to_char_cords( ch, d->character );
   d->character->desc = NULL;
   d->character->retran = old_room_vnum;
   d->character = NULL;
   DISPOSE( d->outbuf );
   DISPOSE( d );
   ch_printf( ch, "Player %s loaded from room %d.\r\n", capitalize( name ), old_room_vnum );
   snprintf( buf, sizeof( buf ), "%s appears from nowhere, eyes glazed over.\r\n", capitalize( name ) );
   act( AT_IMMORT, buf, ch, NULL, NULL, TO_ROOM );
   act( AT_IMMORT, buf, ch, NULL, NULL, TO_CHAR );
}

/*
 * Extract area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "joe.are susan.are" - Gorog
 */
void extract_area_names( char *inp, char *out )
{
   char buf[MIL], *pbuf = buf;
   int len;

   *out = '\0';
   while( inp && *inp )
   {
      inp = one_argument( inp, buf );
      if( ( len = strlen( buf ) ) >= 5 && !strcmp( ".are", pbuf + len - 4 ) )
      {
         if( *out )
            mudstrlcat( out, " ", MSL );
         mudstrlcat( out, buf, MSL );
      }
   }
}

/*
 * Remove area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "aset sedit cset" - Gorog
 */
void remove_area_names( char *inp, char *out )
{
   char buf[MIL], *pbuf = buf;
   int len;

   *out = '\0';
   while( inp && *inp )
   {
      inp = one_argument( inp, buf );
      if( ( len = strlen( buf ) ) < 5 || strcmp( ".are", pbuf + len - 4 ) )
      {
         if( *out )
            mudstrlcat( out, " ", MSL );
         mudstrlcat( out, buf, MSL );
      }
   }
}

/*
 * Allows members of the Area Council to add Area names to the bestow field.
 * Area names mus end with ".are" so that no commands can be bestowed.
 */
CMDF( do_bestowarea )
{
   CHAR_DATA *victim;
   char arg[MIL], buf[MSL];
   int arg_len;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: bestowarea <victim> <filename>.are\r\n", ch );
      send_to_char( "Usage: bestowarea <victim> [list]\r\n", ch );
      send_to_char( "Usage: bestowarea <victim> none\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "You can't give special abilities to a mob!\r\n", ch );
      return;
   }

   if( get_trust( victim ) < PERM_IMM )
   {
      send_to_char( "They aren't an immortal.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "list" ) )
   {
      if( victim->pcdata->bestowments )
         extract_area_names( victim->pcdata->bestowments, buf );
      else
         buf[0] = '\0';
      ch_printf( ch, "Bestowed areas: %s\r\n", buf );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      if( !victim->pcdata->bestowments )
      {
         send_to_char( "They have no bestowments.\r\n", ch );
         return;
      }
      remove_area_names( victim->pcdata->bestowments, buf );
      STRSET( victim->pcdata->bestowments, buf );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   arg_len = strlen( argument );
   if( arg_len < 5
   || argument[arg_len - 4] != '.' || argument[arg_len - 3] != 'a'
   || argument[arg_len - 2] != 'r' || argument[arg_len - 1] != 'e' )
   {
      send_to_char( "You can only bestow an area name\r\n", ch );
      send_to_char( "E.G. bestow joe sam.are\r\n", ch );
      return;
   }

   if( victim->pcdata->bestowments )
      snprintf( buf, sizeof( buf ), "%s %s", victim->pcdata->bestowments, argument );
   else
      snprintf( buf, sizeof( buf ), "%s", argument );
   STRSET( victim->pcdata->bestowments, buf );
   set_char_color( AT_IMMORT, victim );
   ch_printf( victim, "%s has bestowed you the area: %s\r\n", ch->name, argument );
   send_to_char( "Done.\r\n", ch );
}

CMDF( do_bestow )
{
   CHAR_DATA *victim;
   CMDTYPE *cmd;
   char arg[MIL], buf[MSL], arg_buf[MSL];
   bool fComm = false;

   set_char_color( AT_IMMORT, ch );
   argument = one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Bestow whom with what?\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "You can't give special abilities to a mob!\r\n", ch );
      return;
   }

   if( victim == ch || get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You aren't powerful enough...\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "show list" ) )
   {
      ch_printf( ch, "Current bestowed commands on %s: %s.\r\n", victim->name, victim->pcdata->bestowments );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      STRFREE( victim->pcdata->bestowments );
      ch_printf( ch, "Bestowments removed from %s.\r\n", victim->name );
      ch_printf( victim, "%s has removed your bestowed commands.\r\n", ch->name );
      return;
   }

   arg_buf[0] = '\0';

   argument = one_argument( argument, arg );

   while( arg != NULL && arg[0] != '\0' )
   {
      char *cmd_buf, cmd_tmp[MIL];
      bool cFound = false;

      if( ( get_flag( arg, groups_flag, GROUP_MAX ) ) <= GROUP_ALL )
      {
         if( !( cmd = find_command( arg, false ) ) )
         {
            ch_printf( ch, "No such command as %s!\r\n", arg );
            argument = one_argument( argument, arg );
            continue;
         }
         else if( cmd->perm > get_trust( ch ) )
         {
            ch_printf( ch, "You can't bestow the %s command!\r\n", arg );
            argument = one_argument( argument, arg );
            continue;
         }
      }

      cmd_buf = victim->pcdata->bestowments;
      cmd_buf = one_argument( cmd_buf, cmd_tmp );
      while( cmd_tmp != NULL && cmd_tmp[0] != '\0' )
      {
         if( !str_cmp( cmd_tmp, arg ) )
         {
            cFound = true;
            break;
         }
         cmd_buf = one_argument( cmd_buf, cmd_tmp );
      }

      if( cFound == true )
      {
         argument = one_argument( argument, arg );
         continue;
      }

      snprintf( arg, sizeof( arg ), "%s ", arg );
      mudstrlcat( arg_buf, arg, sizeof( arg_buf ) );
      argument = one_argument( argument, arg );
      fComm = true;
   }
   if( !fComm )
   {
      send_to_char( "Good job, you just bestowed them with 'NOTHING!'\r\n", ch );
      return;
   }

   if( arg_buf[strlen( arg_buf ) - 1] == ' ' )
      arg_buf[strlen( arg_buf ) - 1] = '\0';

   if( victim->pcdata->bestowments )
      snprintf( buf, sizeof( buf ), "%s %s", victim->pcdata->bestowments, arg_buf );
   else
      snprintf( buf, sizeof( buf ), "%s", arg_buf );
   STRSET( victim->pcdata->bestowments, buf );
   set_char_color( AT_IMMORT, victim );
   ch_printf( victim, "%s has bestowed on you the command(s)/group(s): %s\r\n", ch->name, arg_buf );
   send_to_char( "Done.\r\n", ch );
}

CMDF( do_form_password )
{
   char *pwcheck;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: formpass <password>\r\n", ch );
      return;
   }

   /* This is arbitrary to discourage weak passwords */
   if( strlen( argument ) < 5 )
   {
      send_to_char( "Usage: formpass <password>\r\n", ch );
      send_to_char( "New password must be at least 5 characters in length.\r\n", ch );
      return;
   }

   if( argument[0] == '!' )
   {
      send_to_char( "Usage: formpass <password>\r\n", ch );
      send_to_char( "New password can't begin with the '!' character.\r\n", ch );
      return;
   }

   pwcheck = sha256_crypt( argument );
   ch_printf( ch, "%s results in the encrypted string: %s\r\n", argument, pwcheck );
}

/* Purge a player file.  No more player.  -- Altrag */
/* This could have other applications too.. move if needed. -- Altrag */
void close_area( AREA_DATA *pArea )
{
   CHAR_DATA *ech, *ech_next;
   ROOM_INDEX_DATA *rid, *rid_next;
   OBJ_INDEX_DATA *oid, *oid_next;
   MOB_INDEX_DATA *mid, *mid_next;
   NEIGHBOR_DATA *neighbor, *neighbor_next;
   int icnt;

   for( ech = first_char; ech; ech = ech_next )
   {
      ech_next = ech->next;

      stop_fighting( ech, true );
      if( is_npc( ech ) )
      {
         /* if mob is in area, or part of area. */
         if( URANGE( pArea->low_vnum, ech->pIndexData->vnum, pArea->hi_vnum ) == ech->pIndexData->vnum
         || ( ech->in_room && ech->in_room->area == pArea ) )
            extract_char( ech, true );
         continue;
      }
      if( ech->in_room && ech->in_room->area == pArea )
         do_recall( ech, (char *)"" );
   }
   for( icnt = 0; icnt < MKH; icnt++ )
   {
      for( rid = room_index_hash[icnt]; rid; rid = rid_next )
      {
         rid_next = rid->next;

         if( rid->area != pArea )
            continue;

         delete_room( rid );
      }
      pArea->first_room = pArea->last_room = NULL;

      for( mid = mob_index_hash[icnt]; mid; mid = mid_next )
      {
         mid_next = mid->next;

         if( mid->avnum != pArea->vnum )
            continue;

         delete_mob( mid );
      }

      for( oid = obj_index_hash[icnt]; oid; oid = oid_next )
      {
         oid_next = oid->next;

         if( oid->avnum != pArea->vnum )
            continue;

         delete_obj( oid );
      }
   }
   if( pArea->weather )
   {
      for( neighbor = pArea->weather->first_neighbor; neighbor; neighbor = neighbor_next )
      {
         neighbor_next = neighbor->next;
         UNLINK( neighbor, pArea->weather->first_neighbor, pArea->weather->last_neighbor, next, prev );
         STRFREE( neighbor->name );
         DISPOSE( neighbor );
      }
      DISPOSE( pArea->weather );
   }
   STRFREE( pArea->name );
   STRFREE( pArea->filename );
   STRFREE( pArea->resetmsg );
   STRFREE( pArea->author );
   if( xIS_SET( pArea->flags, AFLAG_PROTOTYPE ) )
   {
      UNLINK( pArea, first_build, last_build, next, prev );
      UNLINK( pArea, first_bsort, last_bsort, next_sort, prev_sort );
   }
   else
   {
      UNLINK( pArea, first_area, last_area, next, prev );
      UNLINK( pArea, first_asort, last_asort, next_sort, prev_sort );
      UNLINK( pArea, first_area_name, last_area_name, next_sort_name, prev_sort_name );
   }
   DISPOSE( pArea );
}

void close_all_areas( void )
{
   AREA_DATA *area, *area_next;

   for( area = first_area; area; area = area_next )
   {
      area_next = area->next;
      close_area( area );
   }
   for( area = first_build; area; area = area_next )
   {
      area_next = area->next;
      close_area( area );
   }
}

/* Made to get the trust level of a pfile, used by do_destroy */
int get_pfile_trust( char *name )
{
   FILE *fp;
   EXT_BV flags;
   struct stat fst;
   char buf[MIL], *infoflags, flag[MIL];
   int level = 0, trust = 0, value;
   bool endfound, fMatch;

   snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );

   if( stat( buf, &fst ) == -1 )
      return -1;

   if( !( fp = fopen( buf, "r" ) ) )
   {
      bug( "%s: can't open %s for reading", __FUNCTION__, buf );
      perror( buf );
      return -1;
   }

   level = 0;
   trust = 0;
   xCLEAR_BITS( flags );
   for( ;; )
   {
      const char *word;
      char letter;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      if( letter != '#' )
      {
         bug( "%s: # not found in %s.", __FUNCTION__, buf );
         break;
      }
      word = fread_word( fp );
      endfound = false;
      if( !strcmp( word, "PLAYER" ) )
      {
         for( ;; )
         {
            if( feof( fp ) )
            {
               endfound = true;
               break;
            }
            word = fread_word( fp );
            fMatch = false;
            switch( UPPER( word[0] ) )
            {
               default:
                  fread_to_eol( fp );
                  fMatch = true;
                  break;

               case 'F':
                  WEXTKEY( "Flags", flags, fp, pc_flags, PCFLAG_MAX );
                  break;

               case 'L':
                  KEY( "Level", level, fread_number( fp ) );
                  break;

               case 'S':
                  if( !str_cmp( word, "Sex" ) )
                  {
                     endfound = true;
                     fMatch = true;
                     break;
                  }

               case 'T':
                  KEY( "Trust", trust, fread_number( fp ) );
                  break;
            }
            if( !fMatch )
               fread_to_eol( fp );
            if( endfound )
               break;
         }
      }
      else
         break;
      if( endfound )
         break;
   }

   fclose( fp );
   fp = NULL;

   return trust;
}

CMDF( do_destroy )
{
   CHAR_DATA *victim;
   char buf[MSL], buf2[MSL], *name;

   set_char_color( AT_RED, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Destroy what player file?\r\n", ch );
      return;
   }

   /* Set the file points. */
   name = capitalize( argument );
   if( !valid_pfile( name ) )
   {
      ch_printf( ch, "No player exists by the name %s.\r\n", name );
      return;
   }

   for( victim = first_char; victim; victim = victim->next )
      if( !is_npc( victim ) && !str_cmp( victim->name, name ) )
         break;

   if( !victim )
   {
      DESCRIPTOR_DATA *d;

      /* Make sure they aren't halfway logged in. */
      for( d = first_descriptor; d; d = d->next )
         if( ( victim = d->character ) && !is_npc( victim ) && !str_cmp( victim->name, name ) )
            break;
      if( d )
      {
         if( d->character && get_trust( d->character ) > get_trust( ch ) )
         {
            send_to_char( "You can't destroy someone that is trusted higher then you.\r\n", ch );
            return;
         }
         write_to_descriptor( d, "You have been destroyed.\r\n", 0 );
         close_socket( d, true );
      }
   }
   else
   {
      if( victim == ch )
      {
         send_to_char( "If you wish to destroy yourself use delete.\r\n", ch );
         return;
      }
      if( get_trust( victim ) > get_trust( ch ) )
      {
         send_to_char( "You can't destroy someone that is trusted higher then you.\r\n", ch );
         return;
      }
      send_to_char( "You have been destroyed.\r\n", victim );
      quitting_char = victim;
      save_char_obj( victim );
      saving_char = NULL;
      extract_char( victim, true );
   }

   if( get_pfile_trust( name ) > get_trust( ch ) )
   {
      send_to_char( "You can't destroy someone that is trusted higher then you.\r\n", ch );
      return;
   }

   snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( name[0] ), name );
   if( !remove( buf ) )
   {
      AREA_DATA *pArea;

      set_char_color( AT_RED, ch );
      ch_printf( ch, "Player %s destroyed.\r\n", name );

      remove_from_everything( name );
      check_lockers( );
      if( !is_locker_shared( name ) )
      {
         snprintf( buf, sizeof( buf ), "%s%s", LOCKER_DIR, name );
         if( !remove( buf ) )
            send_to_char( "Player's locker data destroyed.\r\n", ch );
         else if( errno != ENOENT )
         {
            ch_printf( ch, "Unknown error #%d - %s (locker data).\r\n", errno, strerror( errno ) );
            snprintf( buf2, sizeof( buf2 ), "%s destroying %s", ch->name, buf );
            perror( buf2 );
         }
      }

      snprintf( buf, sizeof( buf ), "%s%s", GOD_DIR, name );
      if( !remove( buf ) )
         send_to_char( "Player's immortal data destroyed.\r\n", ch );
      else if( errno != ENOENT )
      {
         ch_printf( ch, "Unknown error #%d - %s (immortal data).\r\n", errno, strerror( errno ) );
         snprintf( buf2, sizeof( buf2 ), "%s destroying %s", ch->name, buf );
         perror( buf2 );
      }

      snprintf( buf2, sizeof( buf2 ), "%s.are", name );
      for( pArea = first_build; pArea; pArea = pArea->next )
      {
         if( !str_cmp( pArea->filename, buf2 ) )
         {
            snprintf( buf, sizeof( buf ), "%s", buf2 );
            if( IS_SET( pArea->status, AREA_LOADED ) )
               fold_area( pArea, buf, false );
            close_area( pArea );
            snprintf( buf2, sizeof( buf2 ), "%s.bak", buf );
            set_char_color( AT_RED, ch ); /* Log message changes colors */
            if( !rename( buf, buf2 ) )
               send_to_char( "Player's area data destroyed. Area saved as backup.\r\n", ch );
            else if( errno != ENOENT )
            {
               ch_printf( ch, "Unknown error #%d - %s (area data).\r\n", errno, strerror( errno ) );
               snprintf( buf2, sizeof( buf2 ), "%s destroying %s", ch->name, buf );
               perror( buf2 );
            }
            break;
         }
      }
   }
   else if( errno == ENOENT )
   {
      set_char_color( AT_PLAIN, ch );
      send_to_char( "Player does not exist.\r\n", ch );
   }
   else
   {
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "Unknown error #%d - %s.\r\n", errno, strerror( errno ) );
      snprintf( buf, sizeof( buf ), "%s destroying %s", ch->name, name );
      perror( buf );
   }
}

CMDF( do_delete )
{
   char buf[MSL], buf2[MSL], *name;

   if( !ch || is_npc( ch ) )
      return;

   set_char_color( AT_RED, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: delete <password>\r\n", ch );
      return;
   }

   name = capitalize( ch->name );
   if( !valid_pfile( name ) )
   {
      send_to_char( "You don't have a player file saved yet.\r\n", ch );
      return;
   }

   if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
   {
      send_to_char( "Wrong password entered.\r\n", ch );
      return;
   }

   quitting_char = ch;
   save_char_obj( ch );
   saving_char = NULL;
   extract_char( ch, true );

   snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( name[0] ), name );
   if( !remove( buf ) )
   {
      AREA_DATA *pArea;

      log_printf( "Player %s destroyed.", name );

      remove_from_everything( name );
      check_lockers( );
      if( !is_locker_shared( name ) )
      {
         snprintf( buf, sizeof( buf ), "%s%s", LOCKER_DIR, name );
         if( !remove( buf ) )
            send_to_char( "Player's locker data destroyed.\r\n", ch );
         else if( errno != ENOENT )
         {
            ch_printf( ch, "Unknown error #%d - %s (locker data).\r\n", errno, strerror( errno ) );
            snprintf( buf2, sizeof( buf2 ), "%s destroying %s", ch->name, buf );
            perror( buf2 );
         }
      }

      snprintf( buf, sizeof( buf ), "%s%s", GOD_DIR, name );
      if( !remove( buf ) )
         log_string( "Player's immortal data destroyed." );
      else if( errno != ENOENT )
      {
         log_printf( "Unknown error #%d - %s (immortal data).", errno, strerror( errno ) );
         snprintf( buf2, sizeof( buf2 ), "%s destroying %s", ch->name, buf );
         perror( buf2 );
      }

      snprintf( buf2, sizeof( buf2 ), "%s.are", name );
      for( pArea = first_build; pArea; pArea = pArea->next )
      {
         if( !str_cmp( pArea->filename, buf2 ) )
         {
            snprintf( buf, sizeof( buf ), "%s%s", BUILD_DIR, buf2 );
            if( IS_SET( pArea->status, AREA_LOADED ) )
               fold_area( pArea, buf, false );
            close_area( pArea );
            snprintf( buf2, sizeof( buf2 ), "%s.bak", buf );
            if( !rename( buf, buf2 ) )
               log_string( "Player's area data destroyed. Area saved as backup." );
            else if( errno != ENOENT )
            {
               log_printf( "Unknown error #%d - %s (area data).", errno, strerror( errno ) );
               snprintf( buf2, sizeof( buf2 ), "%s destroying %s", ch->name, buf );
               perror( buf2 );
            }
            break;
         }
      }
   }
   else if( errno == ENOENT )
      log_string( "Player does not exist." );
   else
   {
      log_printf( "Unknown error for [%s] #%d - %s.", buf, errno, strerror( errno ) );
      snprintf( buf, sizeof( buf ), "%s destroying %s", ch->name, name );
      perror( buf );
   }
}

/*
 * Super-AT command:
 * FOR ALL <action>
 * FOR MORTALS <action>
 * FOR GODS <action>
 * FOR MOBS <action>
 * FOR EVERYWHERE <action>
 *
 * Executes action several times, either on ALL players (not including yourself),
 * MORTALS (including trusted characters), GODS (characters with level higher than
 * L_HERO), MOBS (Not recommended) or every room (not recommended either!)
 *
 * If you insert a # in the action, it will be replaced by the name of the target.
 *
 * If # is a part of the action, the action will be executed for every target
 * in game. If there is no #, the action will be executed for every room containg
 * at least one target, but only once per room. # can't be used with FOR EVERY-
 * WHERE. # can be anywhere in the action.
 *
 * Example: 
 * FOR ALL SMILE -> you will only smile once in a room with 2 players.
 * FOR ALL TWIDDLE # -> In a room with A and B, you will twiddle A then B.
 *
 * Destroying the characters this command acts upon MAY cause it to fail. Try to
 * avoid something like FOR MOBS PURGE (although it actually works at my MUD).
 *
 * FOR MOBS TRANS 3054 (transfer ALL the mobs to Midgaard temple) does NOT work though :)
 *
 * The command works by transporting the character to each of the rooms with target in them.
 * Private rooms aren't violated.
 */

/*
 * Expand the name of a character into a string that identifies THAT
 *  character within a room. E.g. the second 'guard' -> 2. guard
 */
const char *name_expand( CHAR_DATA *ch )
{
   CHAR_DATA *rch;
   static char outbuf[MIL];
   char name[MIL];  /*  HOPEFULLY no mob has a name longer than THAT */
   int count = 1;

   if( !is_npc( ch ) )
      return ch->name;

   one_argument( ch->name, name );  /* copy the first word into name */
   if( name == NULL || name[0] == '\0' ) /* weird mob .. no keywords */
   {
      mudstrlcpy( outbuf, "", sizeof( outbuf ) );  /* Do not return NULL, just an empty buffer */
      return outbuf;
   }

   /* ->people changed to ->first_person -- TRI */
   for( rch = ch->in_room->first_person; rch && ( rch != ch ); rch = rch->next_in_room )
      if( is_name( name, rch->name ) )
         count++;

   snprintf( outbuf, sizeof( outbuf ), "%d.%s", count, name );
   return outbuf;
}

CMDF( do_for )
{
   ROOM_INDEX_DATA *room, *old_room;
   CHAR_DATA *p, *p_prev;  /* p_next to p_prev -- TRI */
   char range[MIL], buf[MSL];
   int i;
   bool fGods = false, fMortals = false, fMobs = false, fEverywhere = false, found;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, range );
   if( range == NULL || range[0] == '\0' || !argument || argument[0] == '\0' )  /* invalid usage? */
   {
      send_to_char( "Usage: for <all/mobs/gods/mortals/everywhere> <command> [<target>]\r\n", ch );
      return;
   }

   if( !str_prefix( "quit", argument ) )
   {
      send_to_char( "Are you trying to crash the MUD or something?\r\n", ch );
      return;
   }

   if( !str_cmp( range, "all" ) )
   {
      fMortals = true;
      fGods = true;
   }
   else if( !str_cmp( range, "gods" ) )
      fGods = true;
   else if( !str_cmp( range, "mortals" ) )
      fMortals = true;
   else if( !str_cmp( range, "mobs" ) )
      fMobs = true;
   else if( !str_cmp( range, "everywhere" ) )
      fEverywhere = true;
   else
   {
      do_for( ch, (char *)"" );   /* show Usage */
      return;
   }

   /* do not allow # to make it easier */
   if( fEverywhere && strchr( argument, '#' ) )
   {
      send_to_char( "Can't use FOR EVERYWHERE with the # thingie.\r\n", ch );
      return;
   }

   set_char_color( AT_PLAIN, ch );
   if( strchr( argument, '#' ) ) /* replace # ? */
   {
      /* char_list - last_char, p_next - gch_prev -- TRI */
      for( p = last_char; p; p = p_prev )
      {
         p_prev = p->prev;
         found = false;

         if( !( p->in_room ) || room_is_private( p->in_room ) || ( p == ch ) )
            continue;

         if( is_npc( p ) && fMobs )
            found = true;
         else if( !is_npc( p ) && get_trust( p ) >= PERM_IMM && fGods )
            found = true;
         else if( !is_npc( p ) && get_trust( p ) < PERM_IMM && fMortals )
            found = true;

         /* It looks ugly to me.. but it works :) */
         if( found ) /* p is 'appropriate' */
         {
            char *pSource = argument;  /* head of buffer to be parsed */
            char *pDest = buf;   /* parse into this */

            while( *pSource )
            {
               if( *pSource == '#' )   /* Replace # with name of target */
               {
                  const char *namebuf = name_expand( p );

                  if( namebuf )  /* in case there is no mob name ?? */
                     while( *namebuf ) /* copy name over */
                        *( pDest++ ) = *( namebuf++ );

                  pSource++;
               }
               else
                  *( pDest++ ) = *( pSource++ );
            }  /* while */
            *pDest = '\0'; /* Terminate */

            /* Execute */
            old_room = ch->in_room;
            char_from_room( ch );
            char_to_room( ch, p->in_room );
            interpret( ch, buf );
            char_from_room( ch );
            char_to_room( ch, old_room );

         }  /* if found */
      }  /* for every char */
   }
   else  /* just for every room with the appropriate people in it */
   {
      for( i = 0; i < MKH; i++ ) /* run through all the buckets */
         for( room = room_index_hash[i]; room; room = room->next )
         {
            found = false;

            /* Anyone in here at all? */
            if( fEverywhere ) /* Everywhere executes always */
               found = true;
            else if( !room->first_person )   /* Skip it if room is empty */
               continue;
            /*
             * ->people changed to first_person -- TRI 
             * Check if there is anyone here of the requried type 
             * Stop as soon as a match is found or there are no more ppl in room 
             * ->people to ->first_person -- TRI 
             */
            for( p = room->first_person; p && !found; p = p->next_in_room )
            {

               if( p == ch )  /* do not execute on oneself */
                  continue;

               if( is_npc( p ) && fMobs )
                  found = true;
               else if( !is_npc( p ) && ( get_trust( p ) >= PERM_IMM ) && fGods )
                  found = true;
               else if( !is_npc( p ) && ( get_trust( p ) < PERM_IMM ) && fMortals )
                  found = true;
            }  /* for everyone inside the room */

            if( found && !room_is_private( room ) )   /* Any of the required type here AND room not private? */
            {
               /*
                * This may be ineffective. Consider moving character out of old_room
                * once at beginning of command then moving back at the end.
                * This however, is more safe?
                */
               old_room = ch->in_room;
               char_from_room( ch );
               char_to_room( ch, room );
               interpret( ch, argument );
               char_from_room( ch );
               char_to_room( ch, old_room );
            }  /* if found */
         }  /* for every room in a bucket */
   }  /* if strchr */
}  /* do_for */

CMDF( do_vsearch )
{
   OBJ_DATA *obj, *in_obj;
   int obj_counter = 0, argi;

   set_pager_color( AT_PLAIN, ch );
   if( !argument || argument[0] == '\0' || !is_number( argument ) )
   {
      send_to_char( "Usage: vsearch <vnum>.\r\n", ch );
      return;
   }

   if( ( argi = atoi( argument ) ) < 1 || argi > MAX_VNUM )
   {
      send_to_char( "Vnum out of range.\r\n", ch );
      return;
   }
   for( obj = first_object; obj; obj = obj->next )
   {
      if( !can_see_obj( ch, obj ) || !( argi == obj->pIndexData->vnum ) )
         continue;

      for( in_obj = obj; in_obj->in_obj; in_obj = in_obj->in_obj );

      pager_printf( ch, "#%2d> ", ++obj_counter );
      if( obj->level != obj->pIndexData->level )
         pager_printf( ch, "Level (%2d) ", obj->level );
      pager_printf( ch, "%s ", obj_short( obj ) );
      if( in_obj->carried_by )
         pager_printf( ch, "%s by %s.", in_obj->wear_loc == -1 ? "carried" : "worn", PERS( in_obj->carried_by, ch ) );
      else if( in_obj->in_room )
         pager_printf( ch, "in room vnum [%4d].", in_obj->in_room->vnum );
      else
         send_to_pager( "not in a room or being carried by anyone?", ch );
      send_to_pager( "\r\n", ch );
   }

   if( !obj_counter )
      send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
}

/* 
 * Simple function to let any imm make any player instantly sober.
 * Saw no need for level restrictions on this.
 * Written by Narn, Apr/96 
 */
CMDF( do_sober )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Who would you like to make sober?\r\n", ch );
      return;
   }
   if( !( victim = get_char_room( ch, argument ) ) || is_npc( victim ) )
   {
      send_to_char( "No player by that name is here.\r\n", ch );
      return;
   }
   victim->pcdata->condition[COND_DRUNK] = 0;
   send_to_char( "They are now sober.\r\n", ch );
   set_char_color( AT_IMMORT, victim );
   send_to_char( "You feel sober again.\r\n", victim );
}

/*
 * quest point set - TRI
 * Usage is: qpset char give/take amount
 */
CMDF( do_qpset )
{
   CHAR_DATA *victim;
   char arg[MIL], arg2[MIL], arg3[MIL];
   int amount;
   bool give = true;

   set_char_color( AT_IMMORT, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "NPCs can't use qpset.\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_IMM )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: qpset <character> [give/take <amount>]\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "There is no such player currently playing.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "Glory can't be given to or taken from a mob.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg2 );
   if( arg2 == NULL || arg2[0] == '\0' )
   {
      ch_printf( ch, "%s has %u glory.\r\n", victim->name, victim->pcdata->quest_curr );
      return;
   }

   argument = one_argument( argument, arg3 );
   amount = atoi( arg3 );
   if( amount <= 0 )
   {
      send_to_char( "Amount must be a positive number greater than 0.\r\n", ch );
      return;
   }

   set_char_color( AT_IMMORT, victim );
   if( !str_cmp( arg2, "give" ) )
   {
      give = true;
      if( ch->pcdata->council && str_cmp( ch->pcdata->council->name, "Quest Council" ) && ( get_trust( ch ) < PERM_LEADER ) )
      {
         send_to_char( "You must be a member of the Quest Council to give qp to a character.\r\n", ch );
         return;
      }
   }
   else if( !str_cmp( arg2, "take" ) )
      give = false;
   else
   {
      do_qpset( ch, (char *)"" );
      return;
   }

   if( give )
   {
      victim->pcdata->quest_curr += amount;
      ch_printf( victim, "Your glory has been increased by %d.\r\n", amount );
      ch_printf( ch, "You have increased the glory of %s by %d.\r\n", victim->name, amount );
   }
   else
   {
      if( ( victim->pcdata->quest_curr - amount ) < 0 )
         ch_printf( ch, "%s does not have %d glory to take.\r\n", victim->name, amount );
      else
      {
         victim->pcdata->quest_curr -= amount;
         ch_printf( victim, "Your glory has been decreased by %d.\r\n", amount );
         ch_printf( ch, "You have decreased the glory of %s by %d.\r\n", victim->name, amount );
      }
   }
}

CMDF( do_fshow )
{
   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: fshow moblog/log/hacked\r\n", ch );
      return;
   }

   set_char_color( AT_LOG, ch );
   if( !str_cmp( argument, "moblog" ) )
   {
      send_to_char( "\r\nCurrent moblog:\r\n", ch );
      show_file( ch, MOBLOG_FILE );
   }
   else if( !str_cmp( argument, "log" ) )
   {
      send_to_char( "\r\nCurrent log:\r\n", ch );
      show_file( ch, LOG_FILE );
   }
   else if( !str_cmp( argument, "hacked" ) )
   {
      send_to_char( "\r\nCurrent hacked log:\r\n", ch );
      show_file( ch, HACKED_FILE );
   }
   else
      send_to_char( "No such file.\r\n", ch );
}

CMDF( do_fclear )
{
   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: fclear moblog/log/hacked\r\n", ch );
      return;
   }

   set_char_color( AT_LOG, ch );
   if( !str_cmp( argument, "moblog" ) )
   {
      remove_file( MOBLOG_FILE );
      send_to_char( "\r\nDeleted moblog file.\r\n", ch );
   }
   else if( !str_cmp( argument, "log" ) )
   {
      remove_file( LOG_FILE );
      send_to_char( "\r\nDeleted log file.\r\n", ch );
   }
   else if( !str_cmp( argument, "hacked" ) )
   {
      remove_file( HACKED_FILE );
      send_to_char( "\r\nDeleted hacked log file.\r\n", ch );
   }
   else
      send_to_char( "No such file.\r\n", ch );
}

CMDF( do_flineclear )
{
   char arg[MSL];
   int dline = -1;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: flineclear moblog/log/hacked <#>\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( is_number( argument ) )
      dline = atoi( argument );
   if( dline < 0 )
   {
      send_to_char( "Usage: flineclear moblog/log/hacked <#>\r\n", ch );
      return;
   }

   set_char_color( AT_LOG, ch );
   if( !str_cmp( arg, "moblog" ) )
   {
      if( remove_line_from_file( MOBLOG_FILE, dline ) )
         ch_printf( ch, "Deleted line %d from moblog file.\r\n", dline );
      else
         ch_printf( ch, "Couldn't delete line %d from moblog file.\r\n", dline );
   }
   else if( !str_cmp( arg, "log" ) )
   {
      if( remove_line_from_file( LOG_FILE, dline ) )
         ch_printf( ch, "Deleted line %d from log file.\r\n", dline );
      else
         ch_printf( ch, "Couldn't delete line %d from log file.\r\n", dline );
   }
   else if( !str_cmp( arg, "hacked" ) )
   {
      if( remove_line_from_file( HACKED_FILE, dline ) )
         ch_printf( ch, "Deleted line %d from hacked log file.\r\n", dline );
      else
         ch_printf( ch, "Couldn't delete line %d from hacked log file.\r\n", dline );
   }
   else
      send_to_char( "No such file.\r\n", ch );
}

CMDF( do_showweather )
{
   AREA_DATA *pArea;
   char arg[MIL];

   if( !ch )
      return;

   argument = one_argument( argument, arg );
   ch_printf( ch, "&[blue]%-40s     %-8s %-8s     %-8s\r\n", "Area Name:", "Temp:", "Precip:", "Wind:" );
   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      if( arg == NULL || arg[0] == '\0' || nifty_is_name_prefix( arg, pArea->name ) )
      {
         ch_printf( ch, "&[green]%-40s &[white]%3d &[blue](&[lblue]%3d&[blue])  &[white]%3d &[blue](&[lblue]%3d&[blue])  &[white]%3d &[blue](&[lblue]%3d&[blue])\r\n",
            pArea->name, pArea->weather->temp, pArea->weather->temp_vector, pArea->weather->precip,
            pArea->weather->precip_vector, pArea->weather->wind, pArea->weather->wind_vector );
         ch_printf( ch, "&[blue]%s\r\n", show_weather( pArea ) );
      }
   }
}

/* Command to control global weather variables and to reset weather */
CMDF( do_setweather )
{
   char arg[MIL];

   set_char_color( AT_BLUE, ch );

   argument = one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      ch_printf( ch, "%15s %14s\r\n", "Parameters:", "Current Value:" );
      ch_printf( ch, "%15s %14d\r\n", "random", rand_factor );
      ch_printf( ch, "%15s %14d\r\n", "climate", climate_factor );
      ch_printf( ch, "%15s %14d\r\n", "neighbor", neigh_factor );
      ch_printf( ch, "%15s %14d\r\n", "unit", weath_unit );
      ch_printf( ch, "%15s %14d\r\n", "maxvector", max_vector );

      send_to_char( "\r\nResulting values:\r\n", ch );
      ch_printf( ch, "Weather variables range from %d to %d.\r\n", -3 * weath_unit, 3 * weath_unit );
      ch_printf( ch, "Weather vectors range from %d to %d.\r\n", -1 * max_vector, max_vector );
      ch_printf( ch, "The maximum a vector can change in one update is %d.\r\n",
         rand_factor + 2 * climate_factor + ( 6 * weath_unit / neigh_factor ) );
   }
   else if( !str_cmp( arg, "random" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set maximum random change in vectors to what?\r\n", ch );
      else
      {
         rand_factor = atoi( argument );
         ch_printf( ch, "Maximum random change in vectors now equals %d.\r\n", rand_factor );
         save_weatherdata( );
      }
   }
   else if( !str_cmp( arg, "climate" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set climate effect coefficient to what?\r\n", ch );
      else
      {
         climate_factor = atoi( argument );
         ch_printf( ch, "Climate effect coefficient now equals %d.\r\n", climate_factor );
         save_weatherdata( );
      }
   }
   else if( !str_cmp( arg, "neighbor" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set neighbor effect divisor to what?\r\n", ch );
      else
      {
         neigh_factor = URANGE( 1, atoi( argument ), 1000 );
         ch_printf( ch, "Neighbor effect coefficient now equals 1/%d.\r\n", neigh_factor );
         save_weatherdata( );
      }
   }
   else if( !str_cmp( arg, "unit" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set weather unit size to what?\r\n", ch );
      else
      {
         weath_unit = URANGE( 1, atoi( argument ), 1000 );
         ch_printf( ch, "Weather unit size now equals %d.\r\n", weath_unit );
         save_weatherdata( );
      }
   }
   else if( !str_cmp( arg, "maxvector" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set maximum vector size to what?\r\n", ch );
      else
      {
         max_vector = atoi( argument );
         ch_printf( ch, "Maximum vector size now equals %d.\r\n", max_vector );
         save_weatherdata( );
      }
   }
   else if( !str_cmp( arg, "reset" ) )
   {
      init_area_weather( );
      send_to_char( "Weather system reinitialized.\r\n", ch );
   }
   else if( !str_cmp( arg, "update" ) )
   {
      int i, number;

      number = URANGE( 1, atoi( argument ), 100 );
      for( i = 0; i < number; i++ )
         weather_update( );

      ch_printf( ch, "Weather system updated %d times.\r\n", number );
   }
   else
   {
      send_to_char( "You may only use one of the following fields:\r\n", ch );
      send_to_char( "  random\r\n  climate\r\n  neighbor\r\n  unit\r\n  maxvector\r\n", ch );
      send_to_char( "You may also reset or update the system using the fields 'reset' and 'update' respectively.\r\n", ch );
   }
}

CMDF( do_khistory )
{
   MOB_INDEX_DATA *tmob;
   CHAR_DATA *vch;
   KILLED_DATA *killed, *killed_next;
   char arg[MIL];
   int track = 0, count;

   if( is_npc( ch ) || !is_immortal( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: khistory <player>\r\n", ch );
      if( get_trust( ch ) >= PERM_HEAD )
         send_to_char( "Usage: khistory <player> clear\r\n", ch );
      return;
   }

   if( !( vch = get_char_world( ch, arg ) ) || is_npc( vch ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   ch_printf( ch, "&[dred]Kill history for %s:&D\r\n", vch->name );

   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( argument, "clear" ) )
   {
      for( killed = vch->pcdata->first_killed; killed; killed = killed_next )
      {
         killed_next = killed->next;

         UNLINK( killed, vch->pcdata->first_killed, vch->pcdata->last_killed, next, prev );
         DISPOSE( killed );
      }
      send_to_char( "&[red]   Has been cleared.&D\r\n", ch );
      return;
   }

   if( !vch->pcdata->first_killed )
   {
      send_to_char( "&[red]   Is empty.&D\r\n", ch );
      return;
   }

   for( killed = vch->pcdata->first_killed; killed; killed = killed_next )
   {
      killed_next = killed->next;

      ++track;
      if( !( tmob = get_mob_index( killed->vnum ) ) )
      {
         bug( "%s: (%s) had an unknown mob vnum [%d]", __FUNCTION__, vch->name, killed->vnum );
         UNLINK( killed, vch->pcdata->first_killed, vch->pcdata->last_killed, next, prev );
         DISPOSE( killed );
         continue;
      }
      count = killed->count;
      ch_printf( ch, "&[red]   %2d> %-30s&[dred](&[red]%-5d&[dred])&[red]   - killed %d time%s.&D\r\n",
         track, capitalize( tmob->short_descr ), tmob->vnum, count, count != 1 ? "s" : "" );
   }
}

CMDF( do_exphistory )
{
   CHAR_DATA *vch;
   EXP_DATA *fexp, *nexp;
   ROOM_INDEX_DATA *room;
   char arg[MIL];
   int track = 0;

   if( is_npc( ch ) || !is_immortal( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: exphistory <player>\r\n", ch );
      if( get_trust( ch ) >= PERM_HEAD )
         send_to_char( "Usage: exphistory <player> clear\r\n", ch );
      return;
   }

   if( !( vch = get_char_world( ch, arg ) ) || is_npc( vch ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   ch_printf( ch, "&[dred]Explorer history for %s:&D\r\n", vch->name );

   if( get_trust( ch ) >= PERM_HEAD && !str_cmp( argument, "clear" ) )
   {
      for( fexp = vch->pcdata->first_explored; fexp; fexp = nexp )
      {
         nexp = fexp->next;
         UNLINK( fexp, vch->pcdata->first_explored, vch->pcdata->last_explored, next, prev );
         DISPOSE( fexp );
      }
      send_to_char( "&[red]   Has been cleared.&D\r\n", ch );
      return;
   }

   for( fexp = vch->pcdata->first_explored; fexp; fexp = nexp )
   {
      nexp = fexp->next;

      if( !( room = get_room_index( fexp->vnum ) ) )
      {
         bug( "%s: (%s) had an unknown room vnum [%d]", __FUNCTION__, vch->name, fexp->vnum );
         UNLINK( fexp, vch->pcdata->first_explored, vch->pcdata->last_explored, next, prev );
         DISPOSE( fexp );
         continue;
      }
      ch_printf( ch, "&[red]   %2d> %-30s&[dred](&[red]%-5d&[dred])&D\r\n", ++track, capitalize( room->name ), room->vnum );
   }
}

/*
 * Command to check for multiple ip addresses in the mud.
 * --Shaddai
 * Added this new struct to do matching
 * If ya think of a better way do it, easiest way I could think of at
 * 2 in the morning :) --Shaddai
 */
typedef struct ipcompare_data IPCOMPARE_DATA;
struct ipcompare_data
{
   struct ipcompare_data *next, *prev;
   char *host;
   int count;
   bool pkill;
};

void free_ipc_data( IPCOMPARE_DATA *ipc )
{
   if( !ipc )
      return;
   STRFREE( ipc->host );
   DISPOSE( ipc );
}

CMDF( do_ipcompare )
{
   DESCRIPTOR_DATA *d;
   IPCOMPARE_DATA *first_ip = NULL, *last_ip = NULL, *hmm, *hmm_next, *temp;
   int count = 0, repeat = 0, total = 0;
   bool fMatch, pkill;

   set_pager_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   for( d = first_descriptor; d; d = d->next )
   {
      fMatch = false;

      if( d->character && can_pkill( d->character ) )
         pkill = true;
      else
         pkill = false;

      for( hmm = first_ip; hmm; hmm = hmm->next )
      {
         if( !str_cmp( hmm->host, d->host ) )
         {
            fMatch = true;
            repeat++;
            total++;
            hmm->count++;
            if( pkill )
               hmm->pkill = true;
            break;
         }
      }
      if( fMatch )
         continue;

      CREATE( temp, IPCOMPARE_DATA, 1 );
      temp->count = 1;
      temp->host = STRALLOC( d->host );
      temp->pkill = pkill;
      LINK( temp, first_ip, last_ip, next, prev );
      count++;
      total++;
   }
   ch_printf( ch, "There is a total of %d descriptor%s.\r\n", total, total != 1 ? "s" : "" );
   ch_printf( ch, "There is %d unique ip address%s.\r\n", count, count != 1 ? "es" : "" );
   if( repeat > 0 )
      ch_printf( ch, "There is %d repeat ip address%s.\r\n", repeat, repeat != 1 ? "es" : "" );
   for( hmm = first_ip; hmm; hmm = hmm_next )
   {
      hmm_next = hmm->next;

      if( hmm->count > 1 )
      {
         send_to_char( "  ", ch );
         if( hmm->pkill )
            send_to_char( "[PKILLER MULTIPLAYING]", ch );
         ch_printf( ch, "%s has %d connections.\r\n", hmm->host, hmm->count );
      }
      UNLINK( hmm, first_ip, last_ip, next, prev );
      free_ipc_data( hmm );
   }
}

bool check_area_conflict( AREA_DATA *area, int low_range, int hi_range )
{
   if( low_range < area->low_vnum && area->low_vnum < hi_range )
      return true;
   if( low_range < area->hi_vnum && area->hi_vnum < hi_range )
      return true;
   if( ( low_range >= area->low_vnum ) && ( low_range <= area->hi_vnum ) )
      return true;
   if( ( hi_range <= area->hi_vnum ) && ( hi_range >= area->low_vnum ) )
      return true;
   return false;
}

/* Runs the entire list, easier to call in places that have to check them all */
bool check_area_conflicts( int lo, int hi )
{
   AREA_DATA *area;

   for( area = first_area; area; area = area->next )
      if( check_area_conflict( area, lo, hi ) )
         return true;

   for( area = first_build; area; area = area->next )
      if( check_area_conflict( area, lo, hi ) )
         return true;

   return false;
}

/*
 * Consolidated *assign function. 
 * Assigns room/obj/mob ranges and initializes new zone - Samson 2-12-99 
 */
/* Bugfix: Vnum range would not be saved properly without placeholders at both ends - Samson 1-6-00 */
CMDF( do_vassign )
{
   CHAR_DATA *victim, *mob;
   ROOM_INDEX_DATA *room;
   MOB_INDEX_DATA *pMobIndex;
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   AREA_DATA *tarea;
   char filename[256], arg1[MIL], arg2[MIL], arg3[MIL];
   int lo = -1, hi = -1;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   lo = atoi( arg2 );
   hi = atoi( arg3 );

   if( arg1[0] == '\0' || lo < 0 || hi < 0 )
   {
      send_to_char( "Usage: vassign <who> <low> <high>\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      send_to_char( "They don't seem to be around.\r\n", ch );
      return;
   }

   if( is_npc( victim ) || get_trust( victim ) < PERM_BUILDER )
   {
      send_to_char( "They wouldn't know what to do with a vnum range.\r\n", ch );
      return;
   }

   if( lo == 0 && hi == 0 )
   {
      if( victim->pcdata->area )
         close_area( victim->pcdata->area );
      victim->pcdata->area = NULL;
      victim->pcdata->range_lo = 0;
      victim->pcdata->range_hi = 0;
      ch_printf( victim, "%s has removed your vnum range.\r\n", ch->name );
      save_char_obj( victim );
      return;
   }

   if( victim->pcdata->area && lo != 0 )
   {
      send_to_char( "You can't assign them a range, they already have one!\r\n", ch );
      return;
   }

   if( lo == 0 && hi != 0 )
   {
      send_to_char( "Unacceptable vnum range, low vnum can't be 0 when hi vnum is not.\r\n", ch );
      return;
   }

   if( lo > hi )
   {
      send_to_char( "Unacceptable vnum range, low vnum must be smaller than high vnum.\r\n", ch );
      return;
   }

   if( check_area_conflicts( lo, hi ) )
   {
      send_to_char( "That vnum range conflicts with another area. Check the zones or vnums command.\r\n", ch );
      return;
   }

   victim->pcdata->range_lo = lo;
   victim->pcdata->range_hi = hi;
   assign_area( victim );
   send_to_char( "Done.\r\n", ch );
   ch_printf( victim, "%s has assigned you the vnum range %d - %d.\r\n", ch->name, lo, hi );
   assign_area( victim );  /* Put back by Thoric on 02/07/96 */

   if( !victim->pcdata->area )
   {
      bug( "%s: assign_area failed", __FUNCTION__ );
      return;
   }

   tarea = victim->pcdata->area;

   /* Initialize first and last rooms in range */
   if( !( room = make_room( lo, tarea ) ) )
   {
      bug( "%s: make_room failed to initialize first room.", __FUNCTION__ );
      return;
   }

   if( !( room = make_room( hi, tarea ) ) )
   {
      bug( "%s: make_room failed to initialize last room.", __FUNCTION__ );
      return;
   }

   /* Initialize first mob in range */
   if( !( pMobIndex = make_mobile( lo, 0, (char *)"first mob" ) ) )
   {
      bug( "%s: make_mobile failed to initialize first mob.", __FUNCTION__ );
      return;
   }
   mob = create_mobile( pMobIndex );
   char_to_room( mob, room );

   /* Initialize last mob in range */
   if( !( pMobIndex = make_mobile( hi, 0, (char *)"last mob" ) ) )
   {
      bug( "%s: make_mobile failed to initialize last mob.", __FUNCTION__ );
      return;
   }
   mob = create_mobile( pMobIndex );
   char_to_room( mob, room );

   /* Initialize first obj in range */
   if( !( pObjIndex = make_object( lo, 0, (char *)"first obj" ) ) )
   {
      bug( "%s: make_object failed to initialize first obj.", __FUNCTION__ );
      return;
   }
   obj = create_object( pObjIndex, 0 );
   obj_to_room( obj, room );

   /* Initialize last obj in range */
   if( !( pObjIndex = make_object( hi, 0, (char *)"last obj" ) ) )
   {
      bug( "%s: make_object failed to initialize last obj.", __FUNCTION__ );
      return;
   }
   obj = create_object( pObjIndex, 0 );
   obj_to_room( obj, room );

   /* Save character and newly created zone */
   save_char_obj( victim );

   if( !IS_SET( tarea->status, AREA_DELETED ) )
   {
      xSET_BIT( tarea->flags, AFLAG_PROTOTYPE );
      SET_BIT( tarea->status, AREA_LOADED );
      snprintf( filename, sizeof( filename ), "%s", tarea->filename );
      fold_area( tarea, filename, false );
   }

   set_char_color( AT_IMMORT, ch );
   ch_printf( ch, "Vnum range set for %s and initialized.\r\n", victim->name );
}

CMDF( do_marry )
{
   CHAR_DATA *vict, *vict2;
   char arg[MIL];

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: marry <player> <player>.\r\n", ch );
      return;
   }
   if( !( vict = get_char_room( ch, arg ) ) )
   {
      ch_printf( ch, "%s doesn't seem to be in the room with you.\r\n", arg );
      return;
   }
   if( is_npc( vict ) )
   {
      ch_printf( ch, "%s is an npc.\r\n", arg );
      return;
   }
   if( vict->pcdata->spouse )
   {
      ch_printf( ch, "%s is already married to %s.\r\n", vict->name, vict->pcdata->spouse );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: marry <player> <player>.\r\n", ch );
      return;
   }
   if( !( vict2 = get_char_room( ch, arg ) ) )
   {
      ch_printf( ch, "%s doesn't seem to be in the room with you.\r\n", arg );
      return;
   }
   if( is_npc( vict2 ) )
   {
      ch_printf( ch, "%s is an npc.\r\n", arg );
      return;
   }
   if( vict2->pcdata->spouse )
   {
      ch_printf( ch, "%s is already married to %s.\r\n", vict2->name, vict2->pcdata->spouse );
      return;
   }

   if( vict == ch || vict2 == ch )
   {
      send_to_char( "You need to have someone else preform the wedding ceremony.\r\n", ch );
      return;
   }

   vict->pcdata->spouse = STRALLOC( vict2->name );
   ch_printf( vict, "You're now married to %s.\r\n", vict->pcdata->spouse );
   save_char_obj( vict );

   vict2->pcdata->spouse = STRALLOC( vict->name );
   ch_printf( vict2, "You're now married to %s.\r\n", vict2->pcdata->spouse );
   save_char_obj( vict2 );

   ch_printf( ch, "%s and %s are now married.\r\n", vict->name, vict2->name );

   echo_to_all_printf( AT_ACTION, ECHOTAR_ALL, "%s and %s are now married.", vict->name, vict2->name );
}

CMDF( do_divorce )
{
   CHAR_DATA *vict, *vict2;
   char arg[MIL];

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: divorce <player> <player>.\r\n", ch );
      return;
   }
   if( !( vict = get_char_room( ch, arg ) ) )
   {
      ch_printf( ch, "%s doesn't seem to be in the room with you.\r\n", arg );
      return;
   }
   if( is_npc( vict ) )
   {
      ch_printf( ch, "%s is an npc.\r\n", arg );
      return;
   }
   if( !vict->pcdata->spouse )
   {
      ch_printf( ch, "%s isn't married to anyone.\r\n", vict->name );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: divorce <player> <player>.\r\n", ch );
      return;
   }
   if( !( vict2 = get_char_room( ch, arg ) ) )
   {
      ch_printf( ch, "%s doesn't seem to be in the room with you.\r\n", arg );
      return;
   }
   if( is_npc( vict2 ) )
   {
      ch_printf( ch, "%s is an npc.\r\n", arg );
      return;
   }
   if( !vict2->pcdata->spouse )
   {
      ch_printf( ch, "%s isn't married to anyone.\r\n", vict2->name );
      return;
   }

   if( str_cmp( vict->pcdata->spouse, vict2->name ) || str_cmp( vict2->pcdata->spouse, vict->name ) )
   {
      ch_printf( ch, "%s and %s aren't married to each other.\r\n", vict->name, vict2->name );
      return;
   }

   if( vict == ch || vict2 == ch )
   {
      send_to_char( "You need to have someone else preform the divorce.\r\n", ch );
      return;
   }

   ch_printf( vict, "You're no longer married to %s.\r\n", vict->pcdata->spouse );
   STRFREE( vict->pcdata->spouse );
   save_char_obj( vict );

   ch_printf( vict2, "You're no longer married to %s.\r\n", vict2->pcdata->spouse );
   STRFREE( vict2->pcdata->spouse );
   save_char_obj( vict2 );

   ch_printf( ch, "%s and %s are no longer married.\r\n", vict->name, vict2->name );

   echo_to_all_printf( AT_ACTION, ECHOTAR_ALL, "%s and %s are now divorced.", vict->name, vict2->name );
}

/* Used so the first immortal can be created by a simple command after creation */
CMDF( do_firstimm )
{
   MCLASS_DATA *mclass;
   struct stat fst;
   int iLevel;

   if( !ch || is_npc( ch ) || ch->level != 1 )
      return;

   if( stat( "system/firstimm", &fst ) == -1 )
      return;

   send_to_char( "Immortalizing you...\r\n", ch );
   for( ; ch->level < MAX_LEVEL; )
   {
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      {
         ++mclass->level;
         mclass->exp = 0;
      }
      advance_level( ch );
   }

   for( iLevel = 0; iLevel < top_sn; iLevel++ )
      ch->pcdata->learned[iLevel] = 100;
   send_to_char( "You know all available spells/skills/tongues/weapons now.\r\n", ch );


   for( iLevel = 0; iLevel < STAT_MAX; iLevel++ )
      ch->perm_stats[iLevel] = ( MAX_LEVEL + 25 );
   send_to_char( "You now have the max stats an immortal can have.\r\n", ch );

   ch->max_hit = 30000;
   ch->hit = ch->max_hit;
   ch->max_move = 30000;
   ch->move = ch->max_move;
   ch->max_mana = 30000;
   ch->mana = ch->max_mana;

   ch->trust = PERM_IMP;
   interpret( ch, (char *)"listen all" );

#ifdef IMC
   imc_initchar( ch );
#endif
   save_char_obj( ch );
   make_wizlist( );

   /* Should only be around for one use. */
   remove_file( "system/firstimm" );
}

bool rename_character( CHAR_DATA *ch, char *newname )
{
   char buf[MSL], buf2[MSL], name[MSL], uname[MSL];

   snprintf( name, sizeof( name ), "%s", capitalize( ch->name ) );
   snprintf( uname, sizeof( uname ), "%s", capitalize( newname ) );
   snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( name[0] ), name );
   if( !remove( buf ) )
      log_printf( "%s: Pfile for %s has been deleted.", __FUNCTION__, name );

   if( is_immortal( ch ) )
   {
      snprintf( buf, sizeof( buf ), "%s%s", GOD_DIR, name );
      if( !remove( buf ) )
         log_printf( "%s: God file %s has been deleted.", __FUNCTION__, name );
   }

   if( ch->pcdata->area )
   {
      snprintf( buf, sizeof( buf ), "%s%s.are", BUILD_DIR, name );
      snprintf( buf2, sizeof( buf2 ), "%s%s.are", BUILD_DIR, uname );
      if( !rename( buf, buf2 ) )
         log_printf( "%s: Area data for %s renamed to %s.\r\n", __FUNCTION__, name, uname );
   }

   /* Take care of renaming the locker while you can */
   rename_locker( ch, uname );
   rename_lockershare( name, uname );

   /* Remove the old name from highscores, it should add the new ones when they save. */
   remove_from_highscores( name );

   snprintf( buf, sizeof( buf ), "%s%s", LOCKER_DIR, name );
   snprintf( buf2, sizeof( buf2), "%s%s", LOCKER_DIR, uname );
   if( !rename( buf, buf2 ) )
      log_printf( "%s: Locker data for %s renamed to %s.\r\n", __FUNCTION__, name, uname );

   if( ch->pcdata->deity )
      rename_deity_worshipper( ch->pcdata->deity, name, uname );

   if( ch->pcdata->nation )
      rename_clan_member( ch->pcdata->nation, name, uname );

   if( ch->pcdata->clan )
      rename_clan_member( ch->pcdata->clan, name, uname );

   if( ch->pcdata->council )
      rename_council_member( ch->pcdata->council, name, uname );

   rename_in_banks( name, uname );

   STRSET( ch->name, uname );
   STRSET( ch->pcdata->filename, uname );

   save_char_obj( ch );

   if( is_immortal( ch ) )
      make_wizlist( );

   return true;
}

CMDF( do_pcrename )
{
   CHAR_DATA *victim;
   char arg1[MIL], arg2[MIL], newname[MSL];

   argument = one_argument( argument, arg1 );
   one_argument( argument, arg2 );

   if( is_npc( ch ) )
      return;

   if( arg1 == NULL || arg1[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
   {
      send_to_char( "Usage: pcrename <victim> <new name>\r\n", ch );
      return;
   }

   if( !check_parse_name( arg2, true ) )
   {
      send_to_char( "Illegal name.\r\n", ch );
      return;
   }

   /* Just a security precaution so you don't rename someone you don't mean too --Shaddai */
   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "That person is not in the room.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "You can't rename a NPC using pcrename.\r\n", ch );
      return;
   }

   if( get_trust( ch ) < get_trust( victim ) )
   {
      send_to_char( "I don't think they would like that!\r\n", ch );
      return;
   }

   snprintf( newname, sizeof( newname ), "%s%c/%s", PLAYER_DIR, tolower( arg2[0] ), capitalize( arg2 ) );
   if( access( newname, F_OK ) == 0 )
   {
      send_to_char( "That name already exists.\r\n", ch );
      return;
   }

   rename_character( victim, arg2 );

   send_to_char( "Character was renamed.\r\n", ch );
}

/* Give immortals a way to look at someone's phistory */
CMDF( do_showphistory )
{
   CHAR_DATA *victim = NULL;
   PER_HISTORY *phistory;
   char arg[MSL];
   int which, swhich = -1;

   if( !ch || is_npc( ch ) || !is_immortal( ch ) )
      return;

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Showphistory for who?\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( is_npc( victim ) )
   {
      send_to_char( "NPCs don't have a history to look at.\r\n", ch );
      return;
   }

   if( get_trust( victim ) > get_trust( ch ) )
   {
      send_to_char( "You can't look at their history.\r\n", ch );
      return;
   }

   if( argument != NULL && argument[0] != '\0' )
   {
      if( !str_cmp( argument, "say" ) )
         swhich = 1;
      else if( !str_cmp( argument, "tell" ) )
         swhich = 0;
      else if( !str_cmp( argument, "yell" ) )
         swhich = 2;
      else if( !str_cmp( argument, "whisper" ) )
         swhich = 3;
      else if( !str_cmp( argument, "fchat" ) )
         swhich = 4;
      else
      {
         send_to_char( "Possible histories to see: [say/tell/yell/whisper/fchat].\r\n", ch );
         return;
      }
   }

   for( which = 0; which < 5; which++ )
   {
      if( swhich != -1 && which != swhich )
         continue;
      if( which == 0 )
         phistory = victim->pcdata->first_tell;
      else if( which == 1 )
         phistory = victim->pcdata->first_say;
      else if( which == 2 )
         phistory = victim->pcdata->first_yell;
      else if( which == 3 )
         phistory = victim->pcdata->first_whisper;
      else if( which == 4 )
         phistory = victim->pcdata->first_fchat;
      else
         break;

      ch_printf( ch, "&D&W%s\r\n",
         ( which == 0 ) ? "Tell History" : ( which == 1 ) ? "Say History" : ( which == 2 ) ? "Yell History" :
         ( which == 3 ) ? "Whisper History" : ( which == 4 ) ? "FChat History" : "Unknown History" );

      if( which == 0 )
         send_to_char( "&[tell]", ch );
      else if( which == 1 )
         send_to_char( "&[say]", ch );
      else if( which == 2 )
         send_to_char( "&[yell]", ch );
      else if( which == 3 )
         send_to_char( "&[whisper]", ch );
      else if( which == 4 )
         send_to_char( "&[fchat]", ch );

      if( !phistory )
      {
         send_to_char( " Has no history to show.\r\n", ch );
         continue;
      }

      for( ; phistory; phistory = phistory->next )
         ch_printf( ch, " [%s] %s", distime( phistory->chtime ), phistory->text );
   }
}
