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
 *                             Friend Code                                   *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "h/mud.h"

void free_friend( FRIEND_DATA *ofriend )
{
   if( !ofriend )
      return;
   STRFREE( ofriend->name );
   DISPOSE( ofriend );
}

void free_all_friends( CHAR_DATA *ch )
{
   FRIEND_DATA *ofriend, *ofriend_next;

   if( !ch || !ch->pcdata || !ch->pcdata->first_friend )
      return;

   for( ofriend = ch->pcdata->first_friend; ofriend; ofriend = ofriend_next )
   {
      ofriend_next = ofriend->next;
      UNLINK( ofriend, ch->pcdata->first_friend, ch->pcdata->last_friend, next, prev );
      free_friend( ofriend );
   }
}

FRIEND_DATA *find_friend( CHAR_DATA *ch, char *fname )
{
   FRIEND_DATA *ofriend;

   if( !ch || !fname || fname[0] == '\0' )
      return NULL;
   for( ofriend = ch->pcdata->first_friend; ofriend; ofriend = ofriend->next )
   {
      if( !str_cmp( ofriend->name, fname ) )
         return ofriend;
   }
   return NULL;
}

FRIEND_DATA *add_friend( CHAR_DATA *ch, char *fname )
{
   FRIEND_DATA *ofriend;

   if( !ch || !fname || fname[0] == '\0' )
      return NULL;
   fname = capitalize( fname );
   if( ( ofriend = find_friend( ch, fname ) ) )
      return NULL;
   if( !valid_pfile( fname ) )
      return NULL;
   CREATE( ofriend, FRIEND_DATA, 1 );
   STRSET( ofriend->name, fname );
   LINK( ofriend, ch->pcdata->first_friend, ch->pcdata->last_friend, next, prev );
   return ofriend;
}

CMDF( do_friend )
{
   FRIEND_DATA *ofriend;
   DESCRIPTOR_DATA *d;
   char arg[MSL], uname[MSL];
   const char *person;
   short cnt = -1;
   bool found = false, pending = false;

   if( !ch || !ch->pcdata )
      return;
   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      for( ofriend = ch->pcdata->first_friend; ofriend; ofriend = ofriend->next )
      {
         if( !ofriend->name )
            continue;
         if( cnt == -1 )
         {
            send_to_char( "&[white]         Friend          Friend          Friend          Friend\r\n", ch );
            send_to_char( "--------------- --------------- --------------- ---------------\r\n", ch );
            cnt = 0;
         }
         found = true;
         person = color_str( AT_PERSON, ch );
         if( ofriend->sex == SEX_MALE )
            person = color_str( AT_MALE, ch );
         if( ofriend->sex == SEX_FEMALE )
            person = color_str( AT_FEMALE, ch );
         if( !ofriend->approved )
         {
            snprintf( uname, sizeof( uname ), "[%13.13s]", ofriend->name );
            pending = true;
         }
         else
            snprintf( uname, sizeof( uname ), "%15.15s", ofriend->name );
         ch_printf( ch, "%s%15.15s", person, uname );
         if( ++cnt == 4 )
         {
            cnt = 0;
            send_to_char( "\r\n", ch );
         }
         else
            send_to_char( " ", ch );
      }
      if( !found )
         send_to_char( "&[white]You currently have no one as a friend.\r\n", ch );
      else if( cnt != 0 )
         send_to_char( "\r\n", ch );
      if( pending )
         send_to_char( "&[white]Ones in [ ] are pending and not saved until you approve to have them on your friend list.\r\n", ch );
      return;
   }
   argument = capitalize( argument );
   if( !str_cmp( arg, "add" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Who would you like to add to your friend list?\r\n", ch );
         return;
      }
      for( d = first_descriptor; d; d = d->next )
      {
         if( !d->character || !d->character->pcdata )
            continue;
         if( !str_cmp( d->character->name, argument ) )
            break;
      }
      if( !d || !d->character || !d->character->pcdata || str_cmp( d->character->name, argument ) || !can_see( ch, d->character ) )
      {
         send_to_char( "No one online by that name to add to your friend list.\r\n", ch );
         return;
      }
      if( !( ofriend = add_friend( d->character, ch->name ) ) )
      {
         ch_printf( ch, "Sorry %s couldn't be sent a friend request currently.\r\n", argument );
         return;
      }
      ofriend->sex = ch->sex;
      ofriend->approved = false;
      ch_printf( ch, "%s has been sent a friend request.\r\n", argument );
      ch_printf( d->character, "%s has sent you a friend request.\r\n", ch->name );
      return;
   }
   if( !str_cmp( arg, "remove" ) )
   {
      if( !( ofriend = find_friend( ch, argument ) ) )
      {
         ch_printf( ch, "Sorry %s isn't one of your friends.\r\n", argument );
         return;
      }
      UNLINK( ofriend, ch->pcdata->first_friend, ch->pcdata->last_friend, next, prev );
      free_friend( ofriend );
      ch_printf( ch, "%s is no longer one of your friends.\r\n", argument );
      return;
   }
   if( !str_cmp( arg, "approve" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Who would you like to approve to have you on their friend's list.\r\n", ch );
         return;
      }
      for( d = first_descriptor; d; d = d->next )
      {
         if( !d->character || !d->character->pcdata )
            continue;
         if( str_cmp( d->character->name, argument ) )
            continue;

         if( !( ofriend = find_friend( ch, d->character->name ) ) )
            continue;
         if( ofriend->approved )
         {
            send_to_char( "You have already approved them to have you on their friend list.\r\n", ch );
            return;
         }
         ofriend->approved = true;
         ch_printf( ch, "You have allowed %s to have you on %s friends list.\r\n", d->character->name, his_her[d->character->sex] );
         ch_printf( d->character, "%s has allowed you to have %s on your friends list.\r\n", ch->name, him_her[ch->sex] );

         if( !( ofriend = add_friend( d->character, ch->name ) ) )
         {
            ofriend->sex = ch->sex;
            ofriend->approved = true;
         }
         return;
      }
      send_to_char( "No one by that name was found online wanting you on their friend list.\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "deny" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Who would you like to deny to have you on their friend's list.\r\n", ch );
         return;
      }
      for( d = first_descriptor; d; d = d->next )
      {
         if( !d->character || !d->character->pcdata )
            continue;
         if( str_cmp( d->character->name, argument ) )
            continue;
         if( !( ofriend = find_friend( ch, d->character->name ) ) )
            continue;
         UNLINK( ofriend, ch->pcdata->first_friend, ch->pcdata->last_friend, next, prev );
         free_friend( ofriend );
         ch_printf( ch, "You have denied %s having you on %s friends list.\r\n", d->character->name, his_her[d->character->sex] );
         ch_printf( d->character, "%s has denied you having %s on your friends list.\r\n", ch->name, him_her[ch->sex] );
         if( ( ofriend = find_friend( d->character, ch->name ) ) )
         {
            UNLINK( ofriend, d->character->pcdata->first_friend, d->character->pcdata->last_friend, next, prev );
            free_friend( ofriend );
         }
         return;
      }
      send_to_char( "No one by that name was found online wanting you on their friend list.\r\n", ch );
      return;
   }

   send_to_char( "&[white]Usage: friend add/remove/approve/deny <name>\r\n", ch );
}

void send_friend_info( CHAR_DATA *ch, char *message )
{
   DESCRIPTOR_DATA *d;
   FRIEND_DATA *ofriend, *tfriend;
   const char *person;

   if( !ch || !message || !ch->pcdata )
      return;
   if( xIS_SET( ch->act, PLR_NOFINFO ) )
      return;
   for( d = first_descriptor; d; d = d->next )
   {
      if( !d->character || !d->character->pcdata || !( ofriend = find_friend( d->character, ch->name ) ) || !ofriend->approved )
         continue;

      /* Might as well go ahead and update the sex if it is needed */
      if( ofriend->sex != ch->sex )
         ofriend->sex = ch->sex;

      /* Ch is on the friend list of d->character, if d->character isn't on the friend list of ch lets remove now */
      if( !( tfriend = find_friend( ch, d->character->name ) ) )
      {
         ch_printf( d->character, "%s has decided you're no longer %s friend.\r\n", ch->name, his_her[ch->sex] );
         UNLINK( ofriend, d->character->pcdata->first_friend, d->character->pcdata->last_friend, next, prev );
         free_friend( ofriend );
         continue;
      }

      if( xIS_SET( d->character->act, PLR_NOFINFO ) )
         continue;
      if( !can_see( d->character, ch ) )
         continue;      
      person = color_str( AT_PERSON, d->character );
      if( ofriend->sex == SEX_MALE )
         person = color_str( AT_MALE, d->character );
      if( ofriend->sex == SEX_FEMALE )
         person = color_str( AT_FEMALE, d->character );
      ch_printf( d->character, "&[white][Friend] %s%s\r\n", person, message );
   }
}

void friend_update( CHAR_DATA *ch )
{
   FRIEND_DATA *ofriend;
   const char *person;
   int count = -1;

   if( !ch || !ch->pcdata )
      return;

   for( ofriend = ch->pcdata->first_friend; ofriend; ofriend = ofriend->next )
   {
      if( !ofriend->name || ofriend->approved )
         continue;

      if( count == -1 )
      {
         send_to_char( "&[white]Pending friend requests.\r\n", ch );
         send_to_char( "&[white]         Friend          Friend          Friend          Friend\r\n", ch );
         send_to_char( "--------------- --------------- --------------- ---------------\r\n", ch );
         count = 0;
      }

      person = color_str( AT_PERSON, ch );
      if( ofriend->sex == SEX_MALE )
         person = color_str( AT_MALE, ch );
      if( ofriend->sex == SEX_FEMALE )
         person = color_str( AT_FEMALE, ch );
      ch_printf( ch, "%s%15.15s", person, ofriend->name );
      if( ++count == 4 )
      {
         count = 0;
         send_to_char( "\r\n", ch );
      }
      else
         send_to_char( " ", ch );
   }

   if( count != -1 && count != 0 )
      send_to_char( "\r\n", ch );
}
