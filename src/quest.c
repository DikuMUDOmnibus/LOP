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
 *                                  Quest                                    *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "h/mud.h"

/* See if the character has killed the specified mobile vnum already */
int has_killed( CHAR_DATA *ch, int vnum )
{
   KILLED_DATA *killed;

   if( !ch || !ch->pcdata )
      return 0;
   for( killed = ch->pcdata->first_killed; killed; killed = killed->next )
   {
      if( killed->vnum != vnum )
         continue;
      return killed->count;
   }
   return 0;
}

bool can_quest_room( CHAR_DATA *ch, ROOM_INDEX_DATA *room )
{
   if( !room->first_exit || !room->area )
      return false;
   if( xIS_SET( room->room_flags, ROOM_DEATH )
   || xIS_SET( room->room_flags, ROOM_SAFE )     || xIS_SET( room->room_flags, ROOM_SOLITARY )
   || xIS_SET( room->room_flags, ROOM_PET_SHOP ) || xIS_SET( room->room_flags, ROOM_DONATION )
   || xIS_SET( room->room_flags, ROOM_PRIVATE )  || xIS_SET( room->area->flags, AFLAG_NOQUEST )
   || ch->level < room->area->low_hard_range     || ch->level > room->area->hi_hard_range )
      return false;
   return true;
}

/* Can this mobile used for a quest? */
bool can_quest_mobile( CHAR_DATA *ch, CHAR_DATA *questman, CHAR_DATA *victim )
{
   int leveldiff;

   if( !ch || !victim )
      return false;
   if( !is_npc( victim ) || !can_see( ch, victim ) )
      return false;
   if( victim == ch || victim == questman )
      return false;
   if( !victim->in_room || !victim->in_room->first_exit )
      return false;
   leveldiff = ( victim->level - ch->level );
   if( leveldiff < -5 || leveldiff > 10 )
      return false;
   if( xIS_SET( victim->act, ACT_PACIFIST ) || xIS_SET( victim->act, ACT_PROTOTYPE )
   || is_safe( ch, victim, false ) || xIS_SET( victim->in_room->area->flags, AFLAG_NOQUEST ) )
      return false;
   if( victim->pIndexData->pShop || victim->pIndexData->rShop )
      return false;
   if( !can_quest_room( ch, victim->in_room ) )
      return false;
   return true;
}

/* Should the player quest for the object? */
bool can_quest_object( CHAR_DATA *ch, OBJ_DATA *olist, OBJ_DATA *obj )
{
   OBJ_DATA *cobj;

   for( cobj = olist; cobj; cobj = cobj->next_content )
   {
      /* If they already have a copy of the object no point in bothering with it */
      if( cobj->pIndexData == obj->pIndexData )
         return false;

      if( cobj->first_content )
         if( !can_quest_object( ch, cobj->first_content, obj ) )
            return false;
   }
   return true;
}

void reward_quest( CHAR_DATA *ch, CHAR_DATA *victim )
{
   char buf[MSL], nbuf[MSL];
   int qpoints = 0, gold = 0, practices = 0;

   if( !ch || !victim )
      return;
   qpoints = number_range( 3, 10 );
   ch->pcdata->quest_curr += qpoints;
   if( number_percent( ) < 25 )
   {
      gold = number_range( 100, 500 );
      increase_gold( ch, gold );
   }
   if( number_percent( ) < 10 )
   {
      practices = number_range( 1, 3 );
      ch->practice += practices;
   }
   snprintf( buf, sizeof( buf ), "tell 0.%s As a reward I'm going to give you %d quest points", ch->name, qpoints );
   if( gold > 0 && practices > 0 )
      snprintf( nbuf, sizeof( nbuf ), "%s, %d gold, and %d practices.", buf, gold, practices );
   else if( gold > 0 )
      snprintf( nbuf, sizeof( nbuf ), "%s and %d gold.", buf, gold );
   else if( practices > 0 )
      snprintf( nbuf, sizeof( nbuf ), "%s and %d practices.", buf, practices );
   else
      snprintf( nbuf, sizeof( nbuf ), "%s.", buf );
   interpret( victim, nbuf );
}

void clear_chquest( CHAR_DATA *ch )
{
   if( !ch || is_npc( ch ) )
      return;
   ch->questgiver = 0;
   ch->questvnum = 0;
   ch->questcountdown = 0;
   ch->questtype = 0;
}

void complete_quest( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
   if( !ch )
      return;
   clear_chquest( ch );
   ch->questcountdown = number_range( 10, 20 );
   ch->pcdata->questcompleted++;
   xREMOVE_BIT( ch->act, PLR_QUESTOR );
   if( victim )
   {
      if( obj )
      {
         separate_obj( obj );
         obj_from_char( obj );
         obj_to_char( obj, victim );
         interpret_printf( victim, "tell 0.%s Thank you for bringing me %s.", ch->name, obj->short_descr );
         extract_obj( obj );
      }
      else
         interpret_printf( victim, "tell 0.%s Thank you for taking care of that problem for me.", ch->name );
      reward_quest( ch, victim );
      interpret_printf( victim, "tell 0.%s If you come back in %d minutes I might have more for you to do.", ch->name, ch->questcountdown );
   }
   save_char_obj( ch );
}

void assign_quest( CHAR_DATA *ch, CHAR_DATA *questman )
{
   CHAR_DATA *victim = NULL;
   ROOM_INDEX_DATA *qroom = NULL;
   OBJ_DATA *obj, *iobj = NULL;

   if( !ch || is_npc( ch ) )
      return;

   if( ch->questcountdown > 0 )
   {
      if( xIS_SET( ch->act, PLR_QUESTOR ) )
         interpret_printf( questman, "tell 0.%s I'm sorry, but you're already on a quest.", ch->name );
      else
         interpret_printf( questman, "tell 0.%s I'm sorry, but you still have %d minutes till you can quest again.", ch->name, ch->questcountdown );
      return;
   }

   /* Lets toss through the list of objects and see if we can find one to have them quest for */
   for( obj = first_object; obj; obj = obj->next )
   {
      iobj = NULL;
      victim = NULL;
      qroom = NULL;

      if( number_percent( ) > 10 )
         continue;

      if( !can_see_obj( ch, obj ) || can_wear( obj, ITEM_NO_TAKE ) )
         continue;

      if( !can_quest_object( ch, ch->first_carrying, obj ) )
         continue;

      /* Should we kill someone for an object they have? */
      if( ( victim = obj->carried_by ) )
      {
         if( !can_quest_mobile( ch, questman, victim ) )
            continue;
         else
            break;
      }
      /* Should we look for the object in a room? */
      else if( ( qroom = obj->in_room ) )
      {
         if( !can_quest_room( ch, qroom ) )
            continue;
         else
            break;
      }
      else if( ( iobj = obj->in_obj ) )
      {
         bool scontinue = false;

         while( iobj->in_obj )
         {
            if( !can_see_obj( ch, iobj ) )
            {
               scontinue = true;
               break;
            }
            iobj = iobj->in_obj;
         }
         if( scontinue )
            continue;
         if( ( victim = iobj->carried_by ) )
         {
            if( !can_quest_mobile( ch, questman, victim ) )
               continue;
            else
               break;
         }
         else if( ( qroom = iobj->in_room ) )
         {
            if( !can_quest_room( ch, qroom ) )
               continue;
            else
               break;
         }
      }
      else /* object not in a room, carried by someone or in an object */
         continue;
   }

   /* Maybe we can find a mobile to let them kill */
   if( !obj )
   {
      for( victim = first_char; victim; victim = victim->next )
      {
         if( number_percent( ) > 20 )
            continue;
         if( !is_npc( victim ) )
            continue;
         if( has_killed( ch, victim->pIndexData->vnum ) > 0 )
            continue;
         if( !can_quest_mobile( ch, questman, victim ) )
            continue;
         else
            break;
      }
   }

   if( !obj && !victim )
   {
      ch->questcountdown = number_range( 5, 10 );
      interpret_printf( questman, "tell 0.%s I'm sorry, but I don't need anything at this time.", ch->name );
      interpret_printf( questman, "tell 0.%s Check back in %d minutes to see if I need anything.", ch->name, ch->questcountdown );
      return;
   }

   if( obj )
   {
      interpret_printf( questman, "tell 0.%s I'm looking for %s.", ch->name, obj->short_descr );
      if( qroom )
      {
         if( !qroom->area )
            interpret_printf( questman, "tell 0.%s Look for it at %s.", ch->name, qroom->name );
         else
            interpret_printf( questman, "tell 0.%s Look for it at %s in %s.", ch->name, qroom->name, qroom->area->name );
      }
      else if( victim )
      {
         if( !victim->in_room || !victim->in_room->area )
            interpret_printf( questman, "tell 0.%s Look for it on %s.", ch->name, PERS( victim, ch ) );
         else
            interpret_printf( questman, "tell 0.%s Look for it on %s in %s.", ch->name, PERS( victim, ch ),
               victim->in_room->area->name );
      }
      else if( iobj )
      {
         if( ( victim = iobj->carried_by ) )
         {
            if( victim->in_room && victim->in_room->area )
               interpret_printf( questman, "tell 0.%s Look for it in %s on %s in %s.", ch->name, iobj->short_descr,
                  PERS( victim, ch ), victim->in_room->area->name );
            else
               interpret_printf( questman, "tell 0.%s Look for it in %s on %s.", ch->name, iobj->short_descr,
                  PERS( victim, ch ) );
         }
         else if( ( qroom = iobj->in_room ) )
         {
            if( qroom->area )
               interpret_printf( questman, "tell 0.%s Look for it in %s at %s in %s.", ch->name, iobj->short_descr, qroom->name, qroom->area->name );
            else
               interpret_printf( questman, "tell 0.%s Look for it in %s at %s.", ch->name, iobj->short_descr, qroom->name );
         }
         else
            interpret_printf( questman, "tell 0.%s Look for it in %s.", ch->name, iobj->short_descr );
      }
      ch->questcountdown = number_range( 10, 20 );
      ch->questvnum = obj->pIndexData->vnum;
      ch->questtype = 1; /* Object */
      interpret_printf( questman, "tell 0.%s You have %d minutes to bring me it.", ch->name, ch->questcountdown );
   }
   else /* Mobile quest here */
   {
      ch->questcountdown = number_range( 10, 20 );
      ch->questvnum = victim->pIndexData->vnum;
      ch->questtype = 2; /* Mobile */
      interpret_printf( questman, "tell 0.%s You have %d minutes to kill %s. Look for %s in %s.", ch->name,
         ch->questcountdown, PERS( victim, ch ), him_her[victim->sex], victim->in_room->area->name );
   }
   xSET_BIT( ch->act, PLR_QUESTOR );
   ch->questgiver = questman->pIndexData->vnum;
   save_char_obj( ch );
}

CHAR_DATA *find_questgiver( CHAR_DATA *ch )
{
   CHAR_DATA *questgiver = NULL;

   for( questgiver = ch->in_room->first_person; questgiver; questgiver = questgiver->next_in_room )
   {
      if( is_npc( questgiver ) && xIS_SET( questgiver->act, ACT_QUESTGIVER ) )
         break;
   }
   return questgiver;
}

CMDF( do_aquest )
{
   CHAR_DATA *questgiver;
   MOB_INDEX_DATA *questmob;
   OBJ_DATA *obj;

   if( !ch || is_npc( ch ) )
      return;
   if( !argument || argument[0] == '\0' )
   {
      if( ch->questcountdown > 0 )
      {
         if( xIS_SET( ch->act, PLR_QUESTOR ) )
         {
            OBJ_INDEX_DATA *oindex;

            if( ch->questtype == 1 )
            {
               if( !( oindex = get_obj_index( ch->questvnum ) ) )
               {
                  clear_chquest( ch );
                  xREMOVE_BIT( ch->act, PLR_QUESTOR );
                  send_to_char( "The object you were questing for no longer exist.\r\n", ch );
                  send_to_char( "Quest has been removed and you may quest again.\r\n", ch );
                  save_char_obj( ch );
                  return;
               }
               ch_printf( ch, "You're currently on a quest to bring back %s.\r\n", oindex->short_descr );
            }
            else if( ch->questtype == 2 )
            {
               if( !( questmob = get_mob_index( ch->questvnum ) ) )
               {
                  clear_chquest( ch );
                  xREMOVE_BIT( ch->act, PLR_QUESTOR );
                  send_to_char( "The mobile you were questing for no longer exist.\r\n", ch );
                  send_to_char( "Quest has been removed and you may quest again.\r\n", ch );
                  save_char_obj( ch );
                  return;
               }
               ch_printf( ch, "You're currently on a quest to kill %s.\r\n", questmob->short_descr );
            }
            ch_printf( ch, "You have %d minute%s left to complete the quest.\r\n", ch->questcountdown,
               ch->questcountdown == 1 ? "" : "s" );
         }
         else
            ch_printf( ch, "You have %d minute%s till you can quest again.\r\n", ch->questcountdown,
               ch->questcountdown == 1 ? "" : "s" );
      }
      else
         send_to_char( "You aren't currently on a quest.\r\n", ch );
      return;
   }
   if( !( questgiver = find_questgiver( ch ) ) )
   {
      send_to_char( "You aren't at a questgiver.\r\n", ch );
      return;
   }
   if( !str_cmp( argument, "request" ) )
   {
      assign_quest( ch, questgiver );
      return;
   }
   if( !str_cmp( argument, "complete" ) )
   {
      if( !xIS_SET( ch->act, PLR_QUESTOR ) )
      {
         interpret_printf( questgiver, "tell 0.%s I'm sorry, but you aren't on a quest.", ch->name );
         return;
      }
      else if( questgiver->pIndexData->vnum != ch->questgiver )
      {
         interpret_printf( questgiver, "tell 0.%s I'm sorry, but I never sent you on a quest.", ch->name );
         return;
      }

      if( ch->questtype == 1 )
      {
         for( obj = ch->first_carrying; obj; obj = obj->next_content )
         {
            separate_obj( obj );
            if( obj->wear_loc != WEAR_NONE )
               continue;
            if( obj->pIndexData->vnum == ch->questvnum )
            {
               complete_quest( ch, questgiver, obj );
               return;
            }
         }
         interpret_printf( questgiver, "tell 0.%s I'm sorry, but you don't seem to have the item im wanting.", ch->name );
      }
      else if( ch->questtype == 2 )
      {
         if( has_killed( ch, ch->questvnum ) > 0 )
         {
            complete_quest( ch, questgiver, NULL );
            return;
         }
         interpret_printf( questgiver, "tell 0.%s I'm sorry, but you haven't killed who I asked you to yet.", ch->name );
      }
      interpret_printf( questgiver, "tell 0.%s You have %d minute%s left to complete the quest.",
         ch->name, ch->questcountdown, ch->questcountdown == 1 ? "" : "s" );
      return;
   }
   if( !str_cmp( argument, "cancel" ) )
   {
      if( !xIS_SET( ch->act, PLR_QUESTOR ) )
      {
         interpret_printf( questgiver, "tell 0.%s I'm sorry, but you aren't on a quest.", ch->name );
         return;
      }
      else if( questgiver->pIndexData->vnum != ch->questgiver )
      {
         interpret_printf( questgiver, "tell 0.%s I'm sorry, but I never sent you on a quest.", ch->name );
         return;
      }
      clear_chquest( ch );
      ch->questcountdown = number_range( 15, 25 );
      xREMOVE_BIT( ch->act, PLR_QUESTOR );
      interpret_printf( questgiver, "tell 0.%s I'm sorry you aren't going to finish the quest.", ch->name );
      interpret_printf( questgiver, "tell 0.%s Check back in %d minutes to see if I need anything.", ch->name, ch->questcountdown );
      save_char_obj( ch );
      return;
   }
   send_to_char( "Usage: aquest [request/cancel/complete]\r\n", ch );
}

void quest_update( void )
{
   CHAR_DATA *ch;
   MOB_INDEX_DATA *victim;
   OBJ_INDEX_DATA *oindex;
   static int quest_update_time;

   if( --quest_update_time > 0 )
      return;

   /* Should be around 1 minute per update */
   quest_update_time = ( 60 * PULSE_PER_SECOND );

   for( ch = first_char; ch; ch = ch->next )
   {
      if( is_npc( ch ) || ch->questcountdown == 0 )
         continue;

      if( ch->questtype == 1 && ch->questvnum != 0 && !( oindex = get_obj_index( ch->questvnum ) ) )
      {
         clear_chquest( ch );
         xREMOVE_BIT( ch->act, PLR_QUESTOR );
         send_to_char( "The object you were questing for no longer exist.\r\n", ch );
         send_to_char( "Quest has been removed and you may quest again.\r\n", ch );
         save_char_obj( ch );
         return;
      }
      else if( ch->questtype == 2 && ch->questvnum != 0 && !( victim = get_mob_index( ch->questvnum ) ) )
      {
         clear_chquest( ch );
         xREMOVE_BIT( ch->act, PLR_QUESTOR );
         send_to_char( "The mobile you were questing for no longer exist.\r\n", ch );
         send_to_char( "Quest has been removed and you may quest again.\r\n", ch );
         save_char_obj( ch );
         return;
      }
      if( --ch->questcountdown == 0 )
      {
         if( xIS_SET( ch->act, PLR_QUESTOR ) )
         {
            clear_chquest( ch );
            ch->questcountdown = number_range( 15, 25 );
            ch_printf( ch, "You have ran out of time for the quest!\r\nYou can quest again in %d minutes.\r\n", ch->questcountdown );
            xREMOVE_BIT( ch->act, PLR_QUESTOR );
            save_char_obj( ch );
         }
         else
            send_to_char( "You may now quest again.\r\n", ch );
      }
      else if( ch->questcountdown < 6 && xIS_SET( ch->act, PLR_QUESTOR ) )
         ch_printf( ch, "Better hurry! You only have %d minute%s left to complete the quest.\r\n", ch->questcountdown,
            ch->questcountdown == 1 ? "" : "s" );
   }
}

#define REWARD_FILE SYSTEM_DIR "rewards.dat"
typedef struct reward_data REWARD_DATA;
struct reward_data
{
   REWARD_DATA *next, *prev;
   int vnum;
   int qpcost;
};
REWARD_DATA *first_reward, *last_reward;

void save_rewards( void )
{
   REWARD_DATA *reward;
   FILE *fp;

   if( !first_reward )
   {
      remove_file( REWARD_FILE );
      return;
   }
   if( !( fp = fopen( REWARD_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, REWARD_FILE );
      perror( REWARD_FILE );
      return;
   }
   for( reward = first_reward; reward; reward = reward->next )
      fprintf( fp, "Reward  %5d %5d\n", reward->vnum, reward->qpcost );
   fprintf( fp, "%s", "End\n" );
   fclose( fp );
   fp = NULL;
}

void free_reward( REWARD_DATA *reward )
{
   if( !reward )
      return;
   UNLINK( reward, first_reward, last_reward, next, prev );
   DISPOSE( reward );
}

void free_rewards( void )
{
   while( last_reward )
      free_reward( last_reward );
}

void add_reward( int vnum, int qpcost )
{
   REWARD_DATA *reward;

   CREATE( reward, REWARD_DATA, 1 );
   reward->vnum = vnum;
   reward->qpcost = qpcost;
   LINK( reward, first_reward, last_reward, next, prev );
}

void load_rewards( void )
{
   FILE *fp;

   first_reward = last_reward = NULL;
   if( !( fp = fopen( REWARD_FILE, "r" ) ) )
      return;
   for( ;; )
   {
      char *word;

      if( feof( fp ) )
         break;
      word = fread_word( fp );
      if( word[0] == EOF )
         break;
      if( !str_cmp( word, "Reward" ) )
      {
         int vnum = fread_number( fp );
         int qpcost = fread_number( fp );

         if( get_obj_index( vnum ) )
            add_reward( vnum, qpcost );
         continue;
      }
      else if( !str_cmp( word, "End" ) )
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

REWARD_DATA *find_reward( int vnum )
{
   REWARD_DATA *reward;

   for( reward = first_reward; reward; reward = reward->next )
      if( reward->vnum == vnum )
         return reward;
   return NULL;
}

CMDF( do_showrewards )
{
   REWARD_DATA *reward;
   int cnt = 0, col = 0;

   for( reward = first_reward; reward; reward = reward->next )
   {
      if( ++cnt == 1 )
      {
         send_to_char( "-----------------------------------------------------------------\r\n", ch );
         ch_printf( ch, "| %6s %6s | %6s %6s | %6s %6s | %6s %6s |\r\n", "Vnum", "QPCost", "Vnum", "QPCost",
            "Vnum", "QPCost", "Vnum", "QPCost" );
         send_to_char( "-----------------------------------------------------------------\r\n", ch );
      }
      ch_printf( ch, "  %6d %6d ", reward->vnum, reward->qpcost );
      if( ++col == 4 )
      {
         col = 0;
         send_to_char( "\r\n", ch );
      }
   }
   if( col != 0 )
      send_to_char( "\r\n", ch );
   ch_printf( ch, "There %s currently %d reward%s.\r\n", cnt == 1 ? "is" : "are", cnt, cnt == 1 ? "" : "s" );
}

CMDF( do_reward )
{
   REWARD_DATA *reward;
   OBJ_INDEX_DATA *oindex;
   OBJ_DATA *obj;
   CHAR_DATA *questgiver = NULL;
   char arg[MSL];
   int cnt = 0, value = 0;

   if( !( questgiver = find_questgiver( ch ) ) )
   {
      send_to_char( "You aren't at a questgiver.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      for( reward = first_reward; reward; reward = reward->next )
      {
         if( !( oindex = get_obj_index( reward->vnum ) ) || !oindex->short_descr )
            continue;
         ch_printf( ch, "%3d> %6d - %s\r\n", ++cnt, reward->qpcost, oindex->short_descr );
      }
      if( !cnt )
         send_to_char( "There are currently no rewards.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( ( value = atoi( arg ) ) <= 0 )
   {
      send_to_char( "Usage: reward <#>.\r\n", ch );
      return;
   }
   for( reward = first_reward; reward; reward = reward->next )
   {
       if( !( oindex = get_obj_index( reward->vnum ) ) || !oindex->short_descr )
          continue;
       if( ++cnt == value )
          break;
   }
   if( !reward )
   {
      send_to_char( "No such reward.\r\n", ch );
      return;
   }
   if( str_cmp( argument, "buy" ) )
   {
      if( !( obj = create_object( get_obj_index( reward->vnum ), 0 ) ) )
      {
         send_to_char( "There was a problem creating the object.\r\n", ch );
         return;
      }
      ch_printf( ch, "%3d> %6d - %s", value, reward->qpcost, obj->short_descr );
      show_obj( ch, obj );
      send_to_char( "Use 'reward <#> buy' if you wish to buy this reward.\r\n", ch );
      extract_obj( obj );
      return;
   }
   if( reward->qpcost > ch->pcdata->quest_curr )
   {
      send_to_char( "You don't have enough quest points to get that reward yet.\r\n", ch );
      return;
   }
   if( !( obj = create_object( get_obj_index( reward->vnum ), 0 ) ) )
   {
      send_to_char( "There was a problem creating the object.\r\n", ch );
      return;
   }
   xSET_BIT( obj->extra_flags, ITEM_QUEST );
   if( !( obj = obj_to_char( obj, ch ) ) )
   {
      send_to_char( "There was a problem in giving you the object.\r\n", ch );
      return;
   }
   ch->pcdata->quest_curr -= reward->qpcost;
   ch_printf( ch, "You have traded %d quest points for reward #%d.\r\n", reward->qpcost, value );
   save_char_obj( ch );
}

CMDF( do_setreward )
{
   REWARD_DATA *reward;
   char arg[MIL], arg2[MIL];
   int vnum = 0, value = 0;

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: setreward create <vnum> <qpcost>\r\n", ch );
      send_to_char( "Usage: setreward delete <vnum>\r\n", ch );
      send_to_char( "Usage: setreward <vnum> vnum/qpcost <value>\r\n", ch );
      return;
   }
   argument = one_argument( argument, arg2 );
   if( arg2 == NULL || arg2[0] == '\0' )
   {
      do_setreward( ch, (char *)"" );
      return;
   }
   if( !str_cmp( arg, "delete" ) )
   {
      vnum = atoi( arg2 );
      if( !( reward = find_reward( vnum ) ) )
      {
         send_to_char( "There is no such reward using that vnum.\r\n", ch );
         return;
      }
      free_reward( reward );
      save_rewards( );
      send_to_char( "The reward has been deleted.\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "create" ) )
   {
      vnum = atoi( arg2 );
      if( !get_obj_index( vnum ) )
      {
         send_to_char( "No object is using that vnum.\r\n", ch );
         return;
      }
      if( ( reward = find_reward( vnum ) ) )
      {
         send_to_char( "There is already a reward using that vnum.\r\n", ch );
         return;
      }
      if( ( value = atoi( argument ) ) <= 0 )
      {
         send_to_char( "Usage: setreward create <vnum> <qpcost>\r\n", ch );
         return;
      }
      add_reward( vnum, value );
      save_rewards( );
      send_to_char( "The reward has been added.\r\n", ch );
      return;
   }
   if( !( reward = find_reward( atoi( arg ) ) ) )
   {
      send_to_char( "There is no reward using that vnum.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "vnum" ) )
   {
      if( ( vnum = atoi( argument ) ) <= 0 )
      {
         send_to_char( "Usage: setreward <vnum> vnum <new vnum>.\r\n", ch );
         return;
      }
      if( !get_obj_index( vnum ) )
      {
         send_to_char( "No object is using that vnum.\r\n", ch );
         return;
      }
      reward->vnum = vnum;
      save_rewards( );
      send_to_char( "That reward's vnum has been changed.\r\n", ch );
      return;
   }
   if( !str_cmp( arg2, "qpcost" ) )
   {
      if( ( value = atoi( argument ) ) <= 0 )
      {
         send_to_char( "Usage: setreward <vnum> qpcost <qpcost>.\r\n", ch );
         return;
      }
      reward->qpcost = value;
      save_rewards( );
      send_to_char( "That reward's qpcost has been changed.\r\n", ch );
      return;
   }
   do_setreward( ch, (char *)"" );
}

CMDF( do_useglory )
{
   OBJ_DATA *obj;
   AFFECT_DATA *paf;
   char arg1[MSL];
   int value;

   argument = one_argument( argument, arg1 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "What would you like to use glory on and for?\r\n", ch );
      return;
   }
   if( !( obj = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You don't have that item.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   separate_obj( obj );

   if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
   {
      send_to_char( "You can't use glory on this object.\r\n", ch );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
   {
      send_to_char( "You must remove it first.\r\n", ch );
      return;
   }

   if( !str_cmp( arg1, "weight" ) )
   {
      if( obj->weight <= 1 )
      {
         send_to_char( "No point in reduceing the weight of that object.\r\n", ch );
         return;
      }
      if( ch->pcdata->quest_curr < 100 )
      {
         send_to_char( "You don't have the 100 glory required.\r\n", ch );
         return;
      }
      obj->weight = 1;
      ch->pcdata->quest_curr -= 100;
      save_char_obj( ch );
      send_to_char( "The object's weight has been reduced to 1.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "glow" ) )
   {
      if( ch->pcdata->quest_curr < 5 )
      {
         send_to_char( "You don't have the 5 glory required.\r\n", ch );
         return;
      }
      xTOGGLE_BIT( obj->extra_flags, ITEM_GLOW );
      ch->pcdata->quest_curr -= 5;
      save_char_obj( ch );
      if( is_obj_stat( obj, ITEM_GLOW ) )
         send_to_char( "The item now glows.\r\n", ch );
      else
         send_to_char( "The item stops glowing.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "noscrap" ) )
   {
      if( ch->pcdata->quest_curr < 500 )
      {
         send_to_char( "You don't have the 500 glory required.\r\n", ch );
         return;
      }
      if( is_obj_stat( obj, ITEM_NOSCRAP ) )
      {
         send_to_char( "This object already resist scrapping.\r\n", ch );
         return;
      }
      xSET_BIT( obj->extra_flags, ITEM_NOSCRAP );
      ch->pcdata->quest_curr -= 500;
      save_char_obj( ch );
      send_to_char( "The object will now resist scrapping.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "resist" ) )
   {
      if( ch->pcdata->quest_curr < 500 )
      {
         send_to_char( "You don't have the 500 glory required.\r\n", ch );
         return;
      }
      value = get_flag( argument, ris_flags, RIS_MAX );
      if( value < 0 || value >= RIS_MAX )
      {
         ch_printf( ch, "Unknown resistant: %s\r\n", argument );
         return;
      }
      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ( paf->location % REVERSE_APPLY ) == APPLY_RESISTANT && paf->modifier == value )
         {
            send_to_char( "The object already has an affect to help you resist that.\r\n", ch );
            return;
         }
      }
      paf = NULL;
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = APPLY_RESISTANT;
      paf->modifier = value;
      xCLEAR_BITS( paf->bitvector );
      paf->next = NULL;
      paf->enchantment = false;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      top_affect++;
      ch->pcdata->quest_curr -= 500;
      save_char_obj( ch );
      send_to_char( "The object will now help you resist that.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "affect" ) )
   {
      if( ch->pcdata->quest_curr < 400 )
      {
         send_to_char( "You don't have the 400 glory required.\r\n", ch );
         return;
      }
      value = get_flag( argument, a_flags, AFF_MAX );
      if( value <= 0 || value >= AFF_MAX )
      {
         ch_printf( ch, "Unknown affect: %s\r\n", argument );
         return;
      }
      /* Only allow these here */
      if( value != AFF_INVISIBLE && value != AFF_DETECT_EVIL && value != AFF_DETECT_INVIS
      && value != AFF_DETECT_MAGIC && value != AFF_DETECT_HIDDEN && value != AFF_INFRARED
      && value != AFF_SNEAK && value != AFF_HIDE && value != AFF_FLYING && value != AFF_PASS_DOOR
      && value != AFF_FLOATING && value != AFF_DETECTTRAPS && value != AFF_SCRYING
      && value != AFF_AQUA_BREATH && value != AFF_DETECT_SNEAK )
      {
         ch_printf( ch, "Unknown affect: %s\r\n", argument );
         return;
      }
      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT && paf->modifier == value )
         {
            send_to_char( "The object already has that affect.\r\n", ch );
            return;
         }
      }
      paf = NULL;
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = APPLY_EXT_AFFECT;
      paf->modifier = value;
      xCLEAR_BITS( paf->bitvector );
      paf->next = NULL;
      paf->enchantment = false;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      top_affect++;
      ch->pcdata->quest_curr -= 400;
      save_char_obj( ch );
      send_to_char( "The object now has that affect.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "armor" ) )
   {
      if( ch->pcdata->quest_curr < 50 )
      {
         send_to_char( "You don't have the 50 glory required.\r\n", ch );
         return;
      }
      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ( paf->location % REVERSE_APPLY ) == APPLY_ARMOR && !paf->enchantment ) /* Don't want it increasing the enchantment */
            break;
      }
      if( !paf )
      {
         paf = NULL;
         CREATE( paf, AFFECT_DATA, 1 );
         paf->type = -1;
         paf->duration = -1;
         paf->location = APPLY_ARMOR;
         paf->modifier = 1;
         xCLEAR_BITS( paf->bitvector );
         paf->next = NULL;
         paf->enchantment = false;
         LINK( paf, obj->first_affect, obj->last_affect, next, prev );
         top_affect++;
      }
      else
         paf->modifier += 1;
      ch->pcdata->quest_curr -= 50;
      save_char_obj( ch );
      send_to_char( "The object now affects your armor class more.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "hitroll" ) )
   {
      if( ch->pcdata->quest_curr < 50 )
      {
         send_to_char( "You don't have the 50 glory required.\r\n", ch );
         return;
      }
      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ( paf->location % REVERSE_APPLY ) == APPLY_HITROLL && !paf->enchantment ) /* Don't want it increasing the enchantment */
            break;
      }
      if( !paf )
      {
         paf = NULL;
         CREATE( paf, AFFECT_DATA, 1 );
         paf->type = -1;
         paf->duration = -1;
         paf->location = APPLY_HITROLL;
         paf->modifier = 1;
         xCLEAR_BITS( paf->bitvector );
         paf->next = NULL;
         paf->enchantment = false;
         LINK( paf, obj->first_affect, obj->last_affect, next, prev );
         top_affect++;
      }
      else
         paf->modifier += 1;
      ch->pcdata->quest_curr -= 50;
      save_char_obj( ch );
      send_to_char( "The object now affects your hitroll more.\r\n", ch );
      return;
   }
   if( !str_cmp( arg1, "damroll" ) )
   {
      if( ch->pcdata->quest_curr < 50 )
      {
         send_to_char( "You don't have the 50 glory required.\r\n", ch );
         return;
      }
      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ( paf->location % REVERSE_APPLY ) == APPLY_DAMROLL && !paf->enchantment ) /* Don't want it increasing the enchantment */
            break;
      }
      if( !paf )
      {
         paf = NULL;
         CREATE( paf, AFFECT_DATA, 1 );
         paf->type = -1;
         paf->duration = -1;
         paf->location = APPLY_DAMROLL;
         paf->modifier = 1;
         xCLEAR_BITS( paf->bitvector );
         paf->next = NULL;
         paf->enchantment = false;
         LINK( paf, obj->first_affect, obj->last_affect, next, prev );
         top_affect++;
      }
      else
         paf->modifier += 1;
      ch->pcdata->quest_curr -= 50;
      save_char_obj( ch );
      send_to_char( "The object now affects your damroll more.\r\n", ch );
      return;
   }
}
