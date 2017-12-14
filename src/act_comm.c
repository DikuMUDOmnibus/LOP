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
 *			   Player communication module                       *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "h/mud.h"

void free_phistory( PER_HISTORY *phistory )
{
   if( !phistory )
      return;
   STRFREE( phistory->text );
   DISPOSE( phistory );
}

/* Types: 0 = tell, 1 = say, 2 = yell, 3 = whisper, 4 = fchat */
void add_phistory( int type, CHAR_DATA *ch, char *argument )
{
   PER_HISTORY *phistory, *phistory_next, *phistory_remove = NULL, *history_start = NULL;
   int y = 0;

   if( !ch || is_npc( ch ) || !ch->pcdata || !argument || argument[0] == '\0' )
      return;
   if( type < 0 || type > 4 )
      return;

   if( type == 0 )
      history_start = ch->pcdata->first_tell;
   else if( type == 1 )
      history_start = ch->pcdata->first_say;
   else if( type == 2 )
      history_start = ch->pcdata->first_yell;
   else if( type == 3 )
      history_start = ch->pcdata->first_whisper;
   else if( type == 4 )
      history_start = ch->pcdata->first_fchat;

   for( phistory = history_start; phistory; phistory = phistory_next )
   {
      phistory_next = phistory->next;
      if( !phistory_remove )
         phistory_remove = phistory;
      if( ++y >= 20 )
      {
         if( type == 0 )
            UNLINK( phistory_remove, ch->pcdata->first_tell, ch->pcdata->last_tell, next, prev );
         else if( type == 1 )
            UNLINK( phistory_remove, ch->pcdata->first_say, ch->pcdata->last_say, next, prev );
         else if( type == 2 )
            UNLINK( phistory_remove, ch->pcdata->first_yell, ch->pcdata->last_yell, next, prev );
         else if( type == 3 )
            UNLINK( phistory_remove, ch->pcdata->first_whisper, ch->pcdata->last_whisper, next, prev );
         else if( type == 4 )
            UNLINK( phistory_remove, ch->pcdata->first_fchat, ch->pcdata->last_fchat, next, prev );
         free_phistory( phistory_remove );
         phistory_remove = NULL;
         y--;
      }
   }
   phistory = NULL;
   CREATE( phistory, PER_HISTORY, 1 );
   if( !phistory )
   {
      bug( "%s: couldn't create a phistory.\r\n", __FUNCTION__ );
      return;
   }
   smash_tilde( argument );
   phistory->text = STRALLOC( argument );
   phistory->chtime = current_time;
   if( type == 0 )
      LINK( phistory, ch->pcdata->first_tell, ch->pcdata->last_tell, next, prev );
   else if( type == 1 )
      LINK( phistory, ch->pcdata->first_say, ch->pcdata->last_say, next, prev );
   else if( type == 2 )
      LINK( phistory, ch->pcdata->first_yell, ch->pcdata->last_yell, next, prev );
   else if( type == 3 )
      LINK( phistory, ch->pcdata->first_whisper, ch->pcdata->last_whisper, next, prev );
   else if( type == 4 )
      LINK( phistory, ch->pcdata->first_fchat, ch->pcdata->last_fchat, next, prev );
   else
      free_phistory( phistory );
}

void fwrite_phistory( CHAR_DATA *ch, FILE *fp )
{
   PER_HISTORY *phistory;
   int which;

   if( !ch || !fp || is_npc( ch ) )
      return;
   for( which = 0; which < 5; which++ )
   {
      if( which == 0 )
         phistory = ch->pcdata->first_tell;
      else if( which == 1 )
         phistory = ch->pcdata->first_say;
      else if( which == 2 )
         phistory = ch->pcdata->first_yell;
      else if( which == 3 )
         phistory = ch->pcdata->first_whisper;
      else if( which == 4 )
         phistory = ch->pcdata->first_fchat;
      else
         break;
      if( !phistory )
         continue;
      for( ; phistory; phistory = phistory->next )
      {
         fprintf( fp, "%-10s",
            ( which == 0 ) ? "TellHist" : ( which == 1 ) ? "SayHist" : ( which == 2 ) ? "YellHist" :
            ( which == 3 ) ? "WhispHist" : ( which == 4 ) ? "FChatHist" : "UnkHist" );
         fprintf( fp, "     %ld %s~\n", phistory->chtime, strip_cr( phistory->text ) );
      }
   }
}

void fread_phistory( CHAR_DATA *ch, FILE *fp, int type )
{
   PER_HISTORY *phistory;

   if( !ch || is_npc( ch ) || !fp )
      return;
   CREATE( phistory, PER_HISTORY, 1 );
   if( !phistory )
   {
      bug( "%s: couldn't create a phistory.\r\n", __FUNCTION__ );
      fread_time( fp );
      fread_flagstring( fp );
      return;
   }
   phistory->chtime = fread_time( fp );
   phistory->text = fread_string( fp );
   if( type == 0 )
      LINK( phistory, ch->pcdata->first_tell, ch->pcdata->last_tell, next, prev );
   else if( type == 1 )
      LINK( phistory, ch->pcdata->first_say, ch->pcdata->last_say, next, prev );
   else if( type == 2 )
      LINK( phistory, ch->pcdata->first_yell, ch->pcdata->last_yell, next, prev );
   else if( type == 3 )
      LINK( phistory, ch->pcdata->first_whisper, ch->pcdata->last_whisper, next, prev );
   else if( type == 4 )
      LINK( phistory, ch->pcdata->first_fchat, ch->pcdata->last_fchat, next, prev );
   else
      free_phistory( phistory );
}

LANG_DATA *get_lang( const char *name )
{
   LANG_DATA *lng;

   for( lng = first_lang; lng; lng = lng->next )
      if( !str_cmp( lng->name, name ) )
         return lng;
   return NULL;
}

/* percent = percent knowing the language. */
char *translate( int percent, const char *in, const char *name )
{
   LCNV_DATA *cnv;
   LANG_DATA *lng;
   static char buf[256];
   char buf2[256];
   const char *pbuf;
   char *pbuf2 = buf2;
   static char log_buf[MSL];

   if( percent > 99 || !str_cmp( name, "common" ) )
   {
      mudstrlcpy( log_buf, in, sizeof( log_buf ) );
      return log_buf;
   }

   /* If we don't know this language... use "default" */
   if( !( lng = get_lang( name ) ) )
      if( !( lng = get_lang( "default" ) ) )
      {
         mudstrlcpy( log_buf, in, sizeof( log_buf ) );
         return log_buf;
      }

   for( pbuf = in; *pbuf; )
   {
      for( cnv = lng->first_precnv; cnv; cnv = cnv->next )
      {
         if( !str_prefix( cnv->old, pbuf ) )
         {
            if( percent && ( rand( ) % 100 ) < percent )
            {
               strncpy( pbuf2, pbuf, cnv->olen );
               pbuf2[cnv->olen] = '\0';
               pbuf2 += cnv->olen;
            }
            else
            {
               mudstrlcpy( pbuf2, cnv->lnew, 256 );
               pbuf2 += cnv->nlen;
            }
            pbuf += cnv->olen;
            break;
         }
      }
      if( !cnv )
      {
         if( isalpha( *pbuf ) && ( !percent || ( rand( ) % 100 ) > percent ) )
         {
            *pbuf2 = lng->alphabet[LOWER( *pbuf ) - 'a'];
            if( isupper( *pbuf ) )
               *pbuf2 = UPPER( *pbuf2 );
         }
         else
            *pbuf2 = *pbuf;
         pbuf++;
         pbuf2++;
      }
   }
   *pbuf2 = '\0';
   for( pbuf = buf2, pbuf2 = buf; *pbuf; )
   {
      for( cnv = lng->first_cnv; cnv; cnv = cnv->next )
         if( !str_prefix( cnv->old, pbuf ) )
         {
            mudstrlcpy( pbuf2, cnv->lnew, 256 );
            pbuf += cnv->olen;
            pbuf2 += cnv->nlen;
            break;
         }
      if( !cnv )
         *( pbuf2++ ) = *( pbuf++ );
   }
   *pbuf2 = '\0';
   return buf;
}

char *drunk_speech( const char *argument, CHAR_DATA *ch )
{
   const char *arg = argument;
   static char buf[MIL * 2];
   char buf1[MIL * 2], *txt, *txt1;
   short drunk;

   if( is_npc( ch ) || !ch->pcdata )
   {
      mudstrlcpy( buf, argument, sizeof( buf ) );
      return buf;
   }

   drunk = ch->pcdata->condition[COND_DRUNK];

   if( drunk <= 0 )
   {
      mudstrlcpy( buf, argument, sizeof( buf ) );
      return buf;
   }

   buf[0] = '\0';
   buf1[0] = '\0';

   if( !argument )
   {
      bug( "%s: NULL argument", __FUNCTION__ );
      return (char *)"";
   }

   txt = buf;
   txt1 = buf1;

   while( *arg != '\0' )
   {
      if( toupper( *arg ) == 'T' )
      {
         if( number_percent( ) < ( drunk * 2 ) )  /* add 'h' after an 'T' */
         {
            *txt++ = *arg;
            *txt++ = 'h';
         }
         else
            *txt++ = *arg;
      }
      else if( toupper( *arg ) == 'X' )
      {
         if( number_percent( ) < ( drunk * 2 / 2 ) )
         {
            *txt++ = 'c', *txt++ = 's', *txt++ = 'h';
         }
         else
            *txt++ = *arg;
      }
      else if( number_percent( ) < ( drunk * 2 / 5 ) )  /* slurred letters */
      {
         short slurn = number_range( 1, 2 );
         short currslur = 0;

         while( currslur < slurn )
            *txt++ = *arg, currslur++;
      }
      else
         *txt++ = *arg;

      arg++;
   };

   *txt = '\0';

   txt = buf;

   while( *txt != '\0' )   /* Let's mess with the string's caps */
   {
      if( number_percent( ) < ( 2 * drunk / 2.5 ) )
      {
         if( isupper( *txt ) )
            *txt1 = tolower( *txt );
         else if( islower( *txt ) )
            *txt1 = toupper( *txt );
         else
            *txt1 = *txt;
      }
      else
         *txt1 = *txt;

      txt1++, txt++;
   };

   *txt1 = '\0';
   txt1 = buf1;
   txt = buf;

   while( *txt1 != '\0' )  /* Let's make them stutter */
   {
      if( *txt1 == ' ' )   /* If there's a space, then there's gotta be a */
      {  /* along there somewhere soon */

         while( *txt1 == ' ' )   /* Don't stutter on spaces */
            *txt++ = *txt1++;

         if( ( number_percent( ) < ( 2 * drunk / 4 ) ) && *txt1 != '\0' )
         {
            short offset = number_range( 0, 2 );
            short pos = 0;

            while( *txt1 != '\0' && pos < offset )
               *txt++ = *txt1++, pos++;

            if( *txt1 == ' ' )   /* Make sure not to stutter a space after */
            {  /* the initial offset into the word */
               *txt++ = *txt1++;
               continue;
            }

            pos = 0;
            offset = number_range( 2, 4 );
            while( *txt1 != '\0' && pos < offset )
            {
               *txt++ = *txt1;
               pos++;
               if( *txt1 == ' ' || pos == offset ) /* Make sure we don't stick */
               {  /* A hyphen right before a space */
                  txt1--;
                  break;
               }
               *txt++ = '-';
            }
            if( *txt1 != '\0' )
               txt1++;
         }
      }
      else
         *txt++ = *txt1++;
   }

   *txt = '\0';

   return buf;
}

bool silent_room( CHAR_DATA *ch )
{
   if( !ch || !ch->in_room )
      return true;

   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return true;
   }

   return false;
}

bool silent_character( CHAR_DATA *ch )
{
   if( !ch )
      return true;

   if( is_npc( ch ) )
      return false;

   if( xIS_SET( ch->act, PLR_SILENCE ) )
   {
      send_to_char( "You are silenced.\r\n", ch );
      return true;
   }

   return false;
}

CMDF( do_say )
{
   CHAR_DATA *vch;
   EXT_BV actflags;
   PER_HISTORY *phistory;
   char buf[MSL];
   int speaking = -1, lang;

   if( !argument || argument[0] == '\0' )
   {
      if( is_npc( ch ) )
         send_to_char( "Say what?\r\n", ch );
      else
      {
         if( ch->pcdata->first_say )
            for( phistory = ch->pcdata->first_say; phistory; phistory = phistory->next )
               ch_printf( ch, "%s\r\n", phistory->text );
         else
            send_to_char( "You haven't seen anyone say anything.\r\n", ch );
      }
      return;
   }

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( xIS_SET( ch->speaking, lang_array[lang] ) )
      {
         speaking = lang;
         break;
      }
   }

   if( silent_room( ch ) )
      return;

   actflags = ch->act;
   if( is_npc( ch ) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   MOBtrigger = false;
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      char *sbuf = argument;

      if( vch == ch )
         continue;

      if( !can_see_character( ch, vch ) )
         continue;

      /* Check to see if character is ignoring speaker */
      if( is_ignoring( vch, ch ) )
      {
         /* continue unless speaker is an immortal */
         if( !is_immortal( ch ) || get_trust( vch ) > get_trust( ch ) )
            continue;
         else
         {
            set_char_color( AT_IGNORE, vch );
            ch_printf( vch, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
         }
      }

      if( speaking != -1 )
      {
         int speakswell = UMIN( knows_language( vch, speaking ), knows_language( ch, speaking ) );

         if( speakswell < 75 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
      sbuf = drunk_speech( sbuf, ch );

      act( AT_SAY, "$n says ' $t &D'", ch, sbuf, vch, TO_VICT );
      if( !is_npc( vch ) )
      {
         snprintf( buf, sizeof( buf ), "&[say]%s said ' %s &D&[say]'", capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), sbuf );
         add_phistory( 1, vch, buf );
      }
   }
   ch->act = actflags;
   act( AT_SAY, "You say ' $T &D'", ch, NULL, drunk_speech( argument, ch ), TO_CHAR );
   MOBtrigger = true;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, sizeof( buf ), "[%s] %s: (says) %s &D", distime( current_time ), is_npc( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }
   mprog_speech_trigger( argument, ch );
   if( char_died( ch ) )
      return;
   oprog_speech_trigger( argument, ch );
   if( char_died( ch ) )
      return;
   rprog_speech_trigger( argument, ch );
}

bool afk_check( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !ch || !victim )
      return true;

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_AFK ) )
   {
      send_to_char( "That player is afk.\r\n", ch );
      return true;
   }

   return false;
}

bool desc_check( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !ch || !victim )
      return true;

   if( !is_npc( victim ) && ( !victim->desc ) )
   {
      send_to_char( "That player is link-dead.\r\n", ch );
      return true;
   }

   return false;
}

bool tell_check( CHAR_DATA *ch, CHAR_DATA *victim )
{
   if( !ch || !victim )
      return true;

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_TELLOFF )
   && ( !is_immortal( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
   {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim, TO_CHAR );
      return true;
   }

   return false;
}

CMDF( do_whisper )
{
   CHAR_DATA *victim;
   PER_HISTORY *phistory;
   char arg[MIL], buf[MIL], *sbuf;
   int position, speaking = -1, lang;

   argument = one_argument( argument, arg );
   if( arg != NULL && arg[0] != '\0' && !is_npc( ch ) && !str_cmp( arg, "off" ) )
   {
      xSET_BIT( ch->act, PLR_WHISPEROFF );
      send_to_char( "Whispsers are now turned off.\r\n", ch );
      return;
   }

   if( arg == NULL || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      if( is_npc( ch ) )
         send_to_char( "Whisper to whom what?\r\n", ch );
      else
      {
         if( !ch->pcdata->first_whisper )
            send_to_char( "No one has whispered anything to you.\r\n", ch );
         else
            for( phistory = ch->pcdata->first_whisper; phistory; phistory = phistory->next )
               ch_printf( ch, "%s\r\n", phistory->text );
      }
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "You have a nice little chat with yourself.\r\n", ch );
      return;
   }

   if( desc_check( ch, victim ) || afk_check( ch, victim ) )
      return;

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_WHISPEROFF )
   && ( !is_immortal( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
   {
      act( AT_PLAIN, "$E has $S whispers turned off.", ch, NULL, victim, TO_CHAR );
      return;
   }

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( xIS_SET( ch->speaking, lang_array[lang] ) )
      {
         speaking = lang;
         break;
      }
   }

   if( !is_npc( ch ) )
      xREMOVE_BIT( ch->act, PLR_WHISPEROFF );

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
      send_to_char( "That player is silenced.  They will receive your message but can't respond.\r\n", ch );

   if( victim->desc && victim->desc->connected == CON_EDITING && get_trust( ch ) < PERM_LEADER )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /* Check to see if target of tell is ignoring the sender */
   if( is_ignoring( victim, ch ) )
   {
      /* If the sender is an imm then they can't be ignored */
      if( !is_immortal( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
      }
   }

   act( AT_WHISPER, "You whisper to $N ' $t &D'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;
   sbuf = argument;
   if( speaking != -1 )
   {
      int speakswell = UMIN( knows_language( victim, speaking ), knows_language( ch, speaking ) );

      if( speakswell < 85 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
   sbuf = drunk_speech( sbuf, ch );

   act( AT_WHISPER, "$n whispers to you ' $t &D'", ch, sbuf, victim, TO_VICT );

   if( !is_npc( victim ) )
   {
      snprintf( buf, sizeof( buf ), "&[whisper]%s whispered to you ' %s &D&[whisper]'",
         capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), sbuf );
      add_phistory( 3, victim, buf );
   }

   if( !silent_room( ch ) )
      act( AT_WHISPER, "$n whispers something to $N.", ch, NULL, victim, TO_NOTVICT );

   victim->position = position;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, sizeof( buf ), "[%s] %s: %s &D(whisper to) %s.", distime( current_time ),
         is_npc( ch ) ? ch->short_descr : ch->name, argument, is_npc( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   mprog_speech_trigger( argument, ch );
}

CMDF( do_tell )
{
   CHAR_DATA *victim;
   char arg[MIL], buf[MIL], *sbuf;
   int position, speaking = -1, lang;

   if( silent_room( ch ) )
      return;

   if( silent_character( ch ) )
      return;

   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_NO_TELL ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg != NULL && arg[0] != '\0' && !is_npc( ch ) && !str_cmp( arg, "off" ) )
   {
      xSET_BIT( ch->act, PLR_TELLOFF );
      send_to_char( "Tells are now turned off.\r\n", ch );
      return;
   }

   if( arg == NULL || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Tell whom what?\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg ) )
   || ( is_npc( victim ) && victim->in_room != ch->in_room ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "You have a nice little chat with yourself.\r\n", ch );
      return;
   }

   if( desc_check( ch, victim ) || afk_check( ch, victim ) || tell_check( ch, victim ) )
      return;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( xIS_SET( ch->speaking, lang_array[lang] ) )
      {
         speaking = lang;
         break;
      }
   }

   if( !is_npc( ch ) )
      xREMOVE_BIT( ch->act, PLR_TELLOFF );

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
      send_to_char( "That player is silenced.  They will receive your message but can't respond.\r\n", ch );

   if( ( !is_immortal( ch ) && !is_awake( victim ) ) )
   {
      act( AT_PLAIN, "$E is too tired to discuss such matters with you now.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( !is_npc( victim ) && xIS_SET( victim->in_room->room_flags, ROOM_SILENCE ) )
   {
      act( AT_PLAIN, "A magic force prevents your message from being heard.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( victim->desc  /* make sure desc exists first  -Thoric */
   && victim->desc->connected == CON_EDITING && get_trust( ch ) < PERM_LEADER )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /* Check to see if target of tell is ignoring the sender */
   if( is_ignoring( victim, ch ) )
   {
      /* If the sender is an imm then they can't be ignored */
      if( !is_immortal( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
      }
   }

   ch->retell = victim;

   /* Bug fix by guppy@wavecomputers.net */
   MOBtrigger = false;
   act( AT_TELL, "You tell $N ' $t &D'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;

   sbuf = argument;
   if( speaking != -1 )
   {
      int speakswell = UMIN( knows_language( victim, speaking ), knows_language( ch, speaking ) );

      if( speakswell < 85 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
   sbuf = drunk_speech( sbuf, ch );

   act_tell( victim, ch, "$n tells you ' $t &D'", ch, sbuf, victim, TO_VICT );

   if( !is_npc( victim ) )
   {
      snprintf( buf, sizeof( buf ), "&[tell]%s told you ' %s &D&[tell]'", capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), sbuf );
      add_phistory( 0, victim, buf );
   }

   MOBtrigger = true;

   victim->position = position;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, sizeof( buf ), "[%s] %s: %s &D(tell to) %s.", distime( current_time ),
         is_npc( ch ) ? ch->short_descr : ch->name, argument, is_npc( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   mprog_speech_trigger( argument, ch );
}

CMDF( do_reply )
{
   CHAR_DATA *victim;
   char buf[MSL], *sbuf;
   int position, speaking = -1, lang;

   if( silent_room( ch ) )
      return;

   if( silent_character( ch ) )
      return;

   if( !( victim = ch->reply ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( desc_check( ch, victim ) || afk_check( ch, victim ) || tell_check( ch, victim ) )
      return;

   if( ( !is_immortal( ch ) && !is_awake( victim ) )
   || ( !is_npc( victim ) && xIS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) )
   {
      act( AT_PLAIN, "$E can't hear you.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->desc  /* make sure desc exists first  -Thoric */
   && victim->desc->connected == CON_EDITING && get_trust( ch ) < PERM_LEADER )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      act( AT_PLAIN, "What would you like to tell $M?", ch, NULL, victim, TO_CHAR );
      return;
   }

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( xIS_SET( ch->speaking, lang_array[lang] ) )
      {
         speaking = lang;
         break;
      }
   }

   if( !is_npc( ch ) )
      xREMOVE_BIT( ch->act, PLR_TELLOFF );

   /* Check to see if the receiver is ignoring the sender */
   if( is_ignoring( victim, ch ) )
   {
      /* If the sender is an imm they can't be ignored */
      if( !is_immortal( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
      }
   }

   act( AT_TELL, "You tell $N ' $t &D'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;

   sbuf = argument;
   if( speaking != -1 )
   {
      int speakswell = UMIN( knows_language( victim, speaking ), knows_language( ch, speaking ) );

      if( speakswell < 85 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
   sbuf = drunk_speech( sbuf, ch );

   act_tell( victim, ch, "$n tells you ' $t &D'", ch, sbuf, victim, TO_VICT );

   if( !is_npc( victim ) )
   {
      snprintf( buf, sizeof( buf ), "&[tell]%s told you ' %s &D&[tell]'", capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), sbuf );
      add_phistory( 0, victim, buf );
   }

   victim->position = position;
   ch->retell = victim;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, sizeof( buf ), "[%s] %s: %s &D(reply to) %s.", distime( current_time ),
         is_npc( ch ) ? ch->short_descr : ch->name, argument, is_npc( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   mprog_speech_trigger( argument, ch );
}

CMDF( do_retell )
{
   CHAR_DATA *victim;
   char buf[MIL], *sbuf;
   int position, speaking = -1, lang;

   if( silent_room( ch ) )
      return;

   if( silent_character( ch ) )
      return;

   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_NO_TELL ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What message do you wish to send?\r\n", ch );
      return;
   }

   if( !( victim = ch->retell ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( desc_check( ch, victim ) || afk_check( ch, victim ) || tell_check( ch, victim ) )
      return;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( xIS_SET( ch->speaking, lang_array[lang] ) )
      {
         speaking = lang;
         break;
      }
   }

   if( !is_npc( ch ) )
      xREMOVE_BIT( ch->act, PLR_TELLOFF );

   if( !is_npc( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
      send_to_char( "That player is silenced. They will receive your message, but can't respond.\r\n", ch );

   if( ( !is_immortal( ch ) && !is_awake( victim ) )
   || ( !is_npc( victim ) && xIS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) )
   {
      act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( victim->desc && victim->desc->connected == CON_EDITING && get_trust( ch ) < PERM_LEADER )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer. Please " "try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /* check to see if the target is ignoring the sender */
   if( is_ignoring( victim, ch ) )
   {
      /* if the sender is an imm then they can't be ignored */
      if( !is_immortal( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempy to ignore %s, but " "are unable to do so.\r\n", ch->name );
      }
   }

   act( AT_TELL, "You tell $N ' $t &D'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;

   sbuf = argument;
   if( speaking != -1 )
   {
      int speakswell = UMIN( knows_language( victim, speaking ), knows_language( ch, speaking ) );

      if( speakswell < 85 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
   sbuf = drunk_speech( sbuf, ch );

   act_tell( victim, ch, "$n tells you ' $t &D'", ch, sbuf, victim, TO_VICT );

   if( !is_npc( victim ) )
   {
      snprintf( buf, sizeof( buf ), "&[tell]%s told you ' %s &D&[tell]'", capitalize( is_npc( ch ) ? ch->short_descr : ch->name ), sbuf );
      add_phistory( 0, victim, buf );
   }

   victim->position = position;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, sizeof( buf ), "[%s] %s: %s &D(retell to) %s.", distime( current_time ),
         is_npc( ch ) ? ch->short_descr : ch->name, argument, is_npc( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   mprog_speech_trigger( argument, ch );
}

CMDF( do_repeat )
{
   PER_HISTORY *phistory;

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   if( !ch->pcdata->first_tell )
   {
      send_to_char( "You haven't been told anything.\r\n", ch );
      return;
   }
   set_char_color( AT_TELL, ch );
   for( phistory = ch->pcdata->first_tell; phistory; phistory = phistory->next )
      ch_printf( ch, "%s\r\n", phistory->text );
}

/* 0 - Room, 1 - Global */
void handle_emote( short type, CHAR_DATA *ch, const char *argument )
{
   CHAR_DATA *vch, *vch_next, *victim;
   static char results[ MSL * 2 ];
   char emote_vict[MSL];
   int x;
   bool foundch = false, doingat;

   results[0] = '\0';

   for( x = 0; argument[x] != '\0'; x++ )
   {
      if( argument[x] == '$' && argument[x + 1] == 'n' )
      {
         foundch = true;
         break;
      }
   }

   if( !foundch )
   {
      send_to_char( "You must include $n in the emote somewhere.\r\n", ch );
      return;
   }

   vch = NULL;
   vch_next = NULL;

   if( type == 1 )
      vch = first_char;
   else if( type == 0 )
      vch = ch->in_room->first_person;

   for( ; vch; vch = vch_next )
   {
      if( type == 1 )
         vch_next = vch->next;
      else if( type == 0 )
         vch_next = vch->next_in_room;

      /* If room then skip anyone not in room */
      if( type == 0 && vch->in_room != ch->in_room )
         continue;
      /* If global skip npcs not in the room */
      else if( type == 1 && !is_npc( vch ) && vch->in_room != ch->in_room )
         continue;

      results[0] = '\0';
      for( x = 0; argument[x] != '\0'; x++ )
      {
         if( argument[x] == '\0' )
            break;
         if( argument[x] != '@' && argument[x] != '#' )
         {
            if( vch == ch && argument[x] == '$' && argument[x + 1] == 'n' )
            {
               mudstrlcat( results, "You", sizeof( results ) );
               x++;
               continue;
            }
            add_letter( results, argument[x] );
         }
         else
         {
            doingat = false;
            if( argument[x] == '@' )
               doingat = true;
            emote_vict[0] = '\0';
            victim = NULL;
            x++;
            for( ; ; )
            {
               if( argument[x] == ' ' )
               {
                 x--;
                 break;
               }
               if( argument[x] == '\0' )
                  break;
               add_letter( emote_vict, argument[x] );
               x++;
            }
            if( emote_vict[0] == '\0' )
            {
               send_to_char( "No victim specified???\r\n", ch );
               return;
            }
            if( ( type == 1 && !( victim = get_char_world( ch, emote_vict ) ) )
            || ( type == 0 && !( victim = get_char_room( ch, emote_vict ) ) ) )
            {
               ch_printf( ch, "Failed to find %s in the %s.\r\n", emote_vict, type == 1 ? "mud" : "room" );
               return;
            }
            if( !victim )
            {
               send_to_char( "No victim???\r\n", ch );
               return;
            }
            if( victim == vch )
            {
               if( doingat )
                  mudstrlcat( results, "You", sizeof( results ) );
               else
                  mudstrlcat( results, "your", sizeof( results ) );
            }
            else
            {
               if( doingat )
                  mudstrlcat( results, PERS( victim, vch ), sizeof( results ) );
               else
                  mudstrlcat( results, !can_see( vch, victim ) ? "its" : his_her[URANGE( 0, victim->sex, 2 )], sizeof( results ) );
            }
         }
      }

      if( results[0] != '\0' )
         act( AT_ACTION, results, ch, NULL, vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
   }
}

CMDF( do_gemote )
{
   CHAR_DATA *vch;
   EXT_BV actflags;
   char buf[MSL], *plast;

   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't show your emotions.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "GEmote what?\r\n", ch );
      return;
   }

   actflags = ch->act;
   if( is_npc( ch ) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   for( plast = argument; *plast != '\0'; plast++ )
      ;

   mudstrlcpy( buf, argument, sizeof( buf ) );
   if( isalpha( plast[-1] ) )
      mudstrlcat( buf, ".", sizeof( buf ) );
   MOBtrigger = false;
   for( vch = first_char; vch; vch = vch->next )
   {
      char *sbuf = buf;

      if( is_ignoring( vch, ch ) )
      {
         if( !is_immortal( ch ) || get_trust( vch ) > get_trust( ch ) )
            continue;
         else
         {
            set_char_color( AT_IGNORE, vch );
            ch_printf( vch, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
         }
      }

      act( AT_ACTION, "[GEmote] $n $t", ch, sbuf, vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
   }
   MOBtrigger = true;
   ch->act = actflags;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, sizeof( buf ), "[%s] %s %s &D(gemote)", distime( current_time ), is_npc( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }
}

CMDF( do_emote )
{
   CHAR_DATA *vch;
   EXT_BV actflags;
   char buf[MSL], *plast;

   if( !is_npc( ch ) && xIS_SET( ch->act, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't show your emotions.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Emote what?\r\n", ch );
      return;
   }

   actflags = ch->act;
   if( is_npc( ch ) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   for( plast = argument; *plast != '\0'; plast++ )
      ;

   mudstrlcpy( buf, argument, sizeof( buf ) );
   if( isalpha( plast[-1] ) )
      mudstrlcat( buf, ".", sizeof( buf ) );
   MOBtrigger = false;
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      char *sbuf = buf;

      if( is_ignoring( vch, ch ) )
      {
         if( !is_immortal( ch ) || get_trust( vch ) > get_trust( ch ) )
            continue;
         else
         {
            set_char_color( AT_IGNORE, vch );
            ch_printf( vch, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
         }
      }

      act( AT_ACTION, "$n $t", ch, sbuf, vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
   }
   MOBtrigger = true;
   ch->act = actflags;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, sizeof( buf ), "[%s] %s %s &D(emote)", distime( current_time ), is_npc( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }

   handle_emote( 0, ch, argument );
}

CMDF( do_quit )
{
   char log_buf[MSL];
   int level;

   if( !ch )
      return;

   if( is_npc( ch ) )
   {
      send_to_char( "Mobiles can't just quit when they want to!\r\n", ch );
      return;
   }

   if( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
   || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
   || ch->position == POS_BERSERK )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "No way! You're fighting.\r\n", ch );
      return;
   }

   if( ch->position < POS_STUNNED )
   {
      set_char_color( AT_BLOOD, ch );
      send_to_char( "You're not DEAD yet.\r\n", ch );
      return;
   }

   if( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !is_immortal( ch ) )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "Your adrenaline is pumping too hard to quit now!\r\n", ch );
      return;
   }

   if( ch->wimpy > ( int )( ch->max_hit / 2.25 ) )
      do_wimpy( ch, (char *)"max" );

   set_char_color( AT_WHITE, ch );
   send_to_char( "Your surroundings begin to fade as a mystical swirling vortex of colors\r\nenvelops your body... When you come to, things aren't as they were.\r\n\r\n", ch );
   act( AT_SAY, "A strange voice says, 'We await your return, $n...'", ch, NULL, NULL, TO_CHAR );
   act( AT_BYE, "$n has left the game.", ch, NULL, NULL, TO_CANSEE );
   set_char_color( AT_GRAY, ch );

   level = get_trust( ch );
   snprintf( log_buf, sizeof( log_buf ), "%s has quit (Room %d).", ch->name, ( ch->in_room ? ch->in_room->vnum : 0 ) );

   {
      char message[MSL];

      snprintf( message, sizeof( message ), "%s has quit.", ch->name );
      send_friend_info( ch, message );
   }

   quitting_char = ch;
   save_char_obj( ch );

   quitting_char = ch;
   if( sysdata.save_pets && ch->pcdata->first_pet )
   {
      CHAR_DATA *pet, *pet_next;

      for( pet = ch->pcdata->first_pet; pet; pet = pet_next )
      {
         pet_next = pet->next_pet;
         act( AT_BYE, "$N follows $S master into the Void.", ch, NULL, pet, TO_ROOM );
         extract_char( pet, true );
      }
   }

   saving_char = NULL;
   quitting_char = NULL;

   extract_char( ch, true );
   log_string_plus( log_buf, LOG_COMM, level );
}

void send_ansi_title( CHAR_DATA *ch )
{
   FILE *rpfile;
   char BUFF[MSL * 2];
   int num = 0;

   if( ( rpfile = fopen( ANSITITLE_FILE, "r" ) ) )
   {
      while( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
         num++;
      fclose( rpfile );
      BUFF[num] = 0;
      write_to_buffer( ch->desc, BUFF, num );
   }
}

void send_ascii_title( CHAR_DATA *ch )
{
   FILE *rpfile;
   char BUFF[MSL];
   int num = 0;

   if( ( rpfile = fopen( ASCTITLE_FILE, "r" ) ) )
   {
      while( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
         num++;
      fclose( rpfile );
      BUFF[num] = 0;
      write_to_buffer( ch->desc, BUFF, num );
   }
}

CMDF( do_ansi )
{
   if( !ch || is_npc( ch ) )
      return;

   xTOGGLE_BIT( ch->act, PLR_ANSI );
   if( xIS_SET( ch->act, PLR_ANSI ) )
   {
      set_char_color( AT_WHITE + AT_BLINK, ch );
      send_to_char( "ANSI ON!!!\r\n", ch );
   }
   else
      send_to_char( "ANSI OFF!!!\r\n", ch );
}

CMDF( do_save )
{
   if( !ch || is_npc( ch ) )
      return;
   if( ch->level < 2 )
   {
      send_to_char( "&BYou must be at least second level to save.\r\n", ch );
      return;
   }
   wait_state( ch, 2 ); /* For big muds with save-happy players, like RoD */
   update_aris( ch );   /* update char affects and RIS */
   save_char_obj( ch );
   saving_char = NULL;
   send_to_char( "Saved...\r\n", ch );
}

/*
 * Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas)			-Thoric
 */
bool circle_follow( CHAR_DATA *ch, CHAR_DATA *victim )
{
   CHAR_DATA *tmp;

   for( tmp = victim; tmp; tmp = tmp->master )
      if( tmp == ch )
         return true;
   return false;
}

CMDF( do_dismiss )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Dismiss whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ( IS_AFFECTED( victim, AFF_CHARM ) ) && ( is_npc( victim ) ) && ( victim->master == ch ) )
   {
      stop_hating( victim, NULL, true );
      stop_hunting( victim, NULL, true );
      stop_fearing( victim, NULL, true );
      stop_follower( victim );
      act( AT_ACTION, "$n dismisses $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You dismiss $N.", ch, NULL, victim, TO_CHAR );
   }
   else
      send_to_char( "You can't dismiss them.\r\n", ch );
}

CMDF( do_follow )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Follow whom?\r\n", ch );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master )
   {
      act( AT_PLAIN, "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
      return;
   }

   if( victim == ch )
   {
      if( !ch->master )
      {
         send_to_char( "You already follow yourself.\r\n", ch );
         return;
      }
      if( ch->group )
         remove_char_from_group( ch );
      stop_follower( ch );
      return;
   }

   if( ( ch->level - victim->level < -10 || ch->level - victim->level > 10 )
   && !is_avatar( ch ) && !( ch->level < 15 && !is_npc( victim )
   && victim->pcdata->council && !str_cmp( victim->pcdata->council->name, "Newbie Council" ) ) )
   {
      send_to_char( "You aren't of the right caliber to follow.\r\n", ch );
      return;
   }

   if( circle_follow( ch, victim ) )
   {
      send_to_char( "Following in loops is not allowed... sorry.\r\n", ch );
      return;
   }

   if( ch->master )
      stop_follower( ch );

   if( ch->group )
      remove_char_from_group( ch );

   add_follower( ch, victim );
}

void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
   CHAR_DATA *pet;
   int count = 0;

   if( ch->master )
   {
      bug( "%s: non-null master.", __FUNCTION__ );
      return;
   }

   /* Support for saving pets --Shaddai */
   if( is_npc( ch ) && xIS_SET( ch->act, ACT_PET ) && !is_npc( master ) )
   {
      for( pet = master->pcdata->first_pet; pet; pet = pet->next_pet )
         count++;
      if( count < sysdata.maxpet )
         LINK( ch, master->pcdata->first_pet, master->pcdata->last_pet, next_pet, prev_pet );
      else
      {
         if( can_see( master, ch ) )
            act( AT_ACTION, "$n can't follow you because you already have to many pets.", ch, NULL, master, TO_VICT );
         act( AT_ACTION, "You can't follow $N because $E already has to many pets.", ch, NULL, master, TO_CHAR );
         return;
      }
   }

   ch->master = master;
   ch->leader = NULL;

   if( can_see( master, ch ) )
      act( AT_ACTION, "$n now follows you.", ch, NULL, master, TO_VICT );
   act( AT_ACTION, "You now follow $N.", ch, NULL, master, TO_CHAR );
}

void stop_follower( CHAR_DATA *ch )
{
   if( !ch->master )
   {
      bug( "%s: null master.", __FUNCTION__ );
      return;
   }

   if( is_npc( ch ) && !is_npc( ch->master ) )
   {
      CHAR_DATA *pet;

      for( pet = ch->master->pcdata->first_pet; pet; pet = pet->next_pet )
      {
         if( pet != ch )
            continue;
         UNLINK( ch, ch->master->pcdata->first_pet, ch->master->pcdata->last_pet, next_pet, prev_pet );
      }
   }
   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      xREMOVE_BIT( ch->affected_by, AFF_CHARM );
      affect_strip( ch, gsn_charm_person );
      if( !is_npc( ch->master ) )
         ch->master->pcdata->charmies--;
   }

   if( can_see( ch->master, ch ) )
      if( is_npc( ch->master ) && !is_immortal( ch ) && is_immortal( ch->master ) )
         act( AT_ACTION, "$n stops following you.", ch, NULL, ch->master, TO_VICT );
   act( AT_ACTION, "You stop following $N.", ch, NULL, ch->master, TO_CHAR );

   ch->master = NULL;
   ch->leader = NULL;
}

void die_follower( CHAR_DATA *ch )
{
   CHAR_DATA *fch;

   if( ch->master )
      stop_follower( ch );

   ch->leader = NULL;

   for( fch = first_char; fch; fch = fch->next )
   {
      if( fch->master == ch )
         stop_follower( fch );
      if( fch->leader == ch )
         fch->leader = fch;
   }
}

CMDF( do_order )
{
   CHAR_DATA *victim, *och, *och_next;
   char arg[MIL], argbuf[MIL], log_buf[MSL], arg2[MIL];
   bool found, fAll;

   mudstrlcpy( argbuf, argument, sizeof( argbuf ) );
   argument = one_argument( argument, arg );

   one_argument( argument, arg2 ); /* Get the second argument for testing for an mpcommand */

   if( arg == NULL || !argument || arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Order whom to do what?\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You feel like taking, not giving, orders.\r\n", ch );
      return;
   }

   if( !str_prefix( "mp", arg2 ) ) /* Check the next word to make sure it doesn't start with mp */
   {
      send_to_char( "No.. I don't think so.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "all" ) )
   {
      fAll = true;
      victim = NULL;
   }
   else
   {
      fAll = false;
      if( !( victim = get_char_room( ch, arg ) ) )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }

      if( victim == ch )
      {
         send_to_char( "Aye aye, right away!\r\n", ch );
         return;
      }

      if( !IS_AFFECTED( victim, AFF_CHARM ) || victim->master != ch )
      {
         send_to_char( "Do it yourself!\r\n", ch );
         return;
      }
   }

   found = false;
   for( och = ch->in_room->first_person; och; och = och_next )
   {
      och_next = och->next_in_room;

      if( IS_AFFECTED( och, AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) && !is_immortal( och ) )
      {
         found = true;
         act( AT_ACTION, "$n orders you to '$t'.", ch, argument, och, TO_VICT );
         interpret( och, argument );
      }
   }

   if( found )
   {
      snprintf( log_buf, sizeof( log_buf ), "%s: order %s.", ch->name, argbuf );
      log_string_plus( log_buf, LOG_NORMAL, get_trust( ch ) );
      send_to_char( "Ok.\r\n", ch );
      wait_state( ch, 12 );
   }
   else
      send_to_char( "You have no followers here.\r\n", ch );
}

GROUP_DATA *create_group( CHAR_DATA *ch )
{
   GROUP_DATA *group;

   if( !ch || ch->group )
      return NULL;

   CREATE( group, GROUP_DATA, 1 );
   if( !group )
   {
      bug( "%s: group is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   group->leader = NULL;
   group->first_char = NULL;
   group->last_char = NULL;
   return group;
}

void free_group( GROUP_DATA *group )
{
   CHAR_DATA *ch, *ch_next;

   if( !group )
      return;
   for( ch = group->first_char; ch; ch = ch_next )
   {
      ch_next = ch->next_in_group;
      if( ch == group->leader )
         group->leader = NULL;
      UNLINK( ch, group->first_char, group->last_char, next_in_group, prev_in_group );
      ch->group = NULL;
      send_to_char( "Your group has been disbanded.\r\n", ch );
   }
}

void add_char_to_group( GROUP_DATA *group, CHAR_DATA *ch )
{
   if( !ch )
      return;
   if( ch->group )
   {
      send_to_char( "You are already in a group.\r\n", ch );
      return;
   }
   if( !group )
      group = create_group( ch );
   if( !group )
   {
      bug( "%s: no group to add %s to.", __FUNCTION__, ch->name );
      return;
   }
   LINK( ch, group->first_char, group->last_char, next_in_group, prev_in_group );
   ch->group = group;
   if( !group->leader )
      group->leader = ch;
   if( group->leader != ch )
   {
      act( AT_ACTION, "$N joins $n's group.", group->leader, NULL, ch, TO_NOTVICT );
      act( AT_ACTION, "You join $n's group.", group->leader, NULL, ch, TO_VICT );
      act( AT_ACTION, "$N joins your group.", group->leader, NULL, ch, TO_CHAR );
   }
   else
   {
      act( AT_ACTION, "$n starts a group.", group->leader, NULL, NULL, TO_ROOM );
      act( AT_ACTION, "You start a group.", group->leader, NULL, NULL, TO_CHAR );
   }
}

void remove_char_from_group( CHAR_DATA *ch )
{
   GROUP_DATA *group;

   if( !( group = ch->group ) )
      return;
   UNLINK( ch, group->first_char, group->last_char, next_in_group, prev_in_group );
   ch->group = NULL;
   if( group->leader == ch )
      group->leader = group->first_char;

   act( AT_ACTION, "$N has been removed from $n's group.", group->leader, NULL, ch, TO_NOTVICT );
   act( AT_ACTION, "You've been removed from $n's group.", group->leader, NULL, ch, TO_VICT );
   act( AT_ACTION, "$N has been removed from your group.", group->leader, NULL, ch, TO_CHAR );

   if( !group->first_char || group->first_char == group->last_char )
      free_group( group );
}

bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{
   if( ach && bch && ach->group && bch->group && ach->group == bch->group )
      return true;
   if( ach == bch )
      return true;
   return false;
}

void free_all_groups( void )
{
   GROUP_DATA *group, *group_next;

   for( group = first_group; group; group = group_next )
   {
      group_next = group->next;
      free_group( group );
   }
}

CMDF( do_group )
{
   CHAR_DATA *victim, *gch;
   GROUP_DATA *group;
   char buf[MSL];
   double percent;

   if( sysdata.groupleveldiff == -1 )
   {
      send_to_char( "Grouping has been turned off.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      set_char_color( AT_GROUP, ch );
      if( !( group = ch->group ) )
      {
         send_to_char( "You are not currently in a group.\r\n", ch );
         return;
      }
      ch_printf( ch, "\r\nFollowing &[group2]%-13.13s        &[group][  HP  ] [ MP/BP ] [ Moves ] [ MST ]\r\n",
         PERS( group->leader, ch ) );
      for( gch = group->first_char; gch; gch = gch->next_in_group )
      {
         if( is_same_group( gch, ch ) )
         {
            set_char_color( AT_DGREEN, ch );
            if( gch->alignment > 750 )
               snprintf( buf, sizeof( buf ), "%s", " G" );
            else if( gch->alignment > 350 )
               snprintf( buf, sizeof( buf ), "%s", "-G" );
            else if( gch->alignment > 150 )
               snprintf( buf, sizeof( buf ), "%s", "+N" );
            else if( gch->alignment > -150 )
               snprintf( buf, sizeof( buf ), "%s", " N" );
            else if( gch->alignment > -350 )
               snprintf( buf, sizeof( buf ), "%s", "-N" );
            else if( gch->alignment > -750 )
               snprintf( buf, sizeof( buf ), "%s", "+E" );
            else
               snprintf( buf, sizeof( buf ), "%s", " E" );

            ch_printf( ch, "&[group][&[group2]%3d %2.2s %3.3s %3.3s&[group]]&[group2]  ",
               gch->level, is_npc( gch ) ? "" : buf,
               is_npc( gch ) ? "" : dis_race_name( gch->race ),
               is_npc( gch ) ? "" : dis_main_class_name( gch ) );
            ch_printf( ch, "%-12.12s ", capitalize( PERS( gch, ch ) ) );


            if( !is_npc( gch ) )
            {
               percent = 0.0;
               if( gch->hit > 0 && gch->max_hit > 0 )
                  percent = get_percent( gch->hit, gch->max_hit );

               if( percent < 25 )
                  set_char_color( AT_DANGER, ch );
               else if( percent < 75 )
                  set_char_color( AT_HURT, ch );
               else
                  set_char_color( AT_GROUP2, ch );

               if( percent < 1 )
                  ch_printf( ch, "  %.3f%% ", percent );
               else
                  ch_printf( ch, "  %.f%% ", percent );
            }
            else
               ch_printf( ch, "  %4s ", "" );

            if( !is_npc( gch ) )
            {
               percent = 0.0;
               if( gch->mana > 0 && gch->max_mana > 0 )
                  percent = get_percent( gch->mana, gch->max_mana );

               if( is_vampire( gch ) )
                  set_char_color( AT_BLOOD, ch );
               else
                  set_char_color( AT_GROUP2, ch );

               if( percent < 1 )
                  ch_printf( ch, "    %.3f%% ", percent );
               else
                  ch_printf( ch, "    %.f%% ", percent );
            }
            else
               ch_printf( ch, "    %4s ", "" );

            if( !is_npc( gch ) )
            {
               percent = 0.0;
               if( gch->move > 0 && gch->max_move > 0 )
                  percent = get_percent( gch->move, gch->max_move );

               if( percent < 75 )
                  set_char_color( AT_YELLOW, ch );
               else
                  set_char_color( AT_GROUP2, ch );

               if( percent < 1 )
                  ch_printf( ch, "     %.3f%%", percent );
               else
                  ch_printf( ch, "     %.f%%", percent );
            }
            else
               ch_printf( ch, "     %4s", "" );

            if( !is_npc( gch ) )
            {
               if( gch->mental_state < -25 || gch->mental_state > 25 )
                  set_char_color( AT_YELLOW, ch );
               else
                  set_char_color( AT_GROUP2, ch );
            }
            ch_printf( ch, "      %3.3s",
               is_npc( gch ) ? ""
               : gch->mental_state > 75 ? "+++"
               : gch->mental_state > 50 ? "=++"
               : gch->mental_state > 25 ? "==+"
               : gch->mental_state > -25 ? "==="
               : gch->mental_state > -50 ? "-=="
               : gch->mental_state > -75 ? "--="
               : "---" );
            send_to_char( "\r\n", ch );
         }
      }
      return;
   }

   if( !strcmp( argument, "disband" ) )
   {
      if( ch->leader || ch->master )
      {
         send_to_char( "You can't disband a group if you're following someone.\r\n", ch );
         return;
      }

      if( !ch->group )
      {
         send_to_char( "You can't disband a group if you aren't in one.\r\n", ch );
         return;
      }

      free_group( ch->group );
      return;
   }

   if( !strcmp( argument, "all" ) )
   {
      int count = 0;

      if( !ch->group )
         add_char_to_group( ch->group, ch );
      for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
      {
         if( ch != gch && can_see( ch, gch )
         && gch->master == ch && !ch->master && !ch->leader
         && abs( ch->level - gch->level ) <= sysdata.groupleveldiff
         && !is_same_group( gch, ch )
         && ( sysdata.groupall || is_pkill( ch ) == is_pkill( gch ) ) )
         {
            gch->leader = ch;
            count++;
            add_char_to_group( ch->group, gch );
         }
      }

      if( count == 0 )
      {
         send_to_char( "You have no eligible group members.\r\n", ch );
         if( ch->group && ch->group->first_char == ch->group->last_char )
            free_group( ch->group );
      }
      else
      {
         act( AT_ACTION, "$n groups $s followers.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You group your followers.\r\n", ch );
      }
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ch->master || ( ch->leader && ch->leader != ch ) )
   {
      send_to_char( "But you're following someone else!\r\n", ch );
      return;
   }

   if( victim->master != ch && ch != victim )
   {
      act( AT_PLAIN, "$N isn't following you.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim == ch )
   {
      act( AT_PLAIN, "You can't group yourself.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( is_same_group( victim, ch ) && ch != victim )
   {
      victim->leader = NULL;
      remove_char_from_group( victim );
      return;
   }

   if( abs( ch->level - victim->level ) > sysdata.groupleveldiff
   || ( !sysdata.groupall && is_pkill( ch ) != is_pkill( victim ) ) )
   {
      act( AT_PLAIN, "$N can't join $n's group.", ch, NULL, victim, TO_NOTVICT );
      act( AT_PLAIN, "You can't join $n's group.", ch, NULL, victim, TO_VICT );
      act( AT_PLAIN, "$N can't join your group.", ch, NULL, victim, TO_CHAR );
      return;
   }

   victim->leader = ch;
   if( !ch->group )
      add_char_to_group( ch->group, ch );
   add_char_to_group( ch->group, victim );
}

/* 'Split' originally by Gnort, God of Chaos. */
CMDF( do_split )
{
   CHAR_DATA *gch;
   char buf[MSL], arg[MSL];
   int members, share, extra, amount = 0, uamount;
   bool autosplit = false;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Split how much?\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( !str_cmp( arg, "auto" ) )
   {
      autosplit = true;
      argument = one_argument( argument, arg );
   }

   if( !is_number( argument ) )
   {
      send_to_char( "You have to specify the amount to split.\r\n", ch );
      return;
   }

   amount = atoi( argument );

   if( amount < 0 )
   {
      send_to_char( "Your group wouldn't like that.\r\n", ch );
      return;
   }

   if( amount == 0 )
   {
      send_to_char( "You hand out zero coins, but no one notices.\r\n", ch );
      return;
   }

   if( !has_gold( ch, amount ) )
   {
      send_to_char( "You don't have that much gold.\r\n", ch );
      return;
   }

   members = 0;
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( autosplit && xIS_SET( ch->act, PLR_AUTOSPLIT ) && !xIS_SET( gch->act, PLR_AUTOSPLIT ) )
         continue;
      if( is_same_group( gch, ch ) )
         members++;
   }

   if( autosplit && xIS_SET( ch->act, PLR_AUTOSPLIT ) && members < 2 )
      return;

   if( members < 2 )
   {
      send_to_char( "Just keep it all.\r\n", ch );
      return;
   }

   share = amount / members;
   extra = amount % members;

   if( share == 0 )
   {
      send_to_char( "Don't even bother, cheapskate.\r\n", ch );
      return;
   }

   decrease_gold( ch, amount );
   uamount = ( share + extra );
   increase_gold( ch, uamount );

   set_char_color( AT_GOLD, ch );
   ch_printf( ch, "You split %s gold coins.", num_punct( amount ) );
   ch_printf( ch, "  Your share is %s gold coins.\r\n", num_punct( ( share + extra ) ) );

   snprintf( buf, sizeof( buf ), "$n splits %s gold coins.", num_punct( amount ) );
   snprintf( buf + strlen( buf ),  sizeof( buf ) - strlen( buf ), "  Your share is %s gold coins.", num_punct( share ) );

   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( autosplit && xIS_SET( ch->act, PLR_AUTOSPLIT ) && !xIS_SET( gch->act, PLR_AUTOSPLIT ) )
         continue;
      if( gch != ch && is_same_group( gch, ch ) )
      {
         if( !can_hold_gold( gch, share ) )
         {
            /* If the gch can't hold it might as well give it back to the character instead of letting it go to waste */
            act( AT_GOLD, buf, ch, NULL, ch, TO_VICT );
            increase_gold( ch, share );
            continue;
         }
         act( AT_GOLD, buf, ch, NULL, gch, TO_VICT );
         increase_gold( gch, share );
      }
   }
}

CMDF( do_gtell )
{
   CHAR_DATA *gch;
   GROUP_DATA *group;
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( xIS_SET( ch->speaking, lang_array[lang] ) )
      {
         speaking = lang;
         break;
      }
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Tell your group what?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->act, PLR_NO_TELL ) )
   {
      send_to_char( "Your message didn't get through!\r\n", ch );
      return;
   }

   if( !( group = ch->group ) )
   {
      send_to_char( "No group to tell anything to.\r\n", ch );
      return;
   }

   /* Note use of send_to_char, so gtell works on sleepers. */
   for( gch = group->first_char; gch; gch = gch->next_in_group )
   {
      set_char_color( AT_GTELL, gch );
      /* Groups unscrambled regardless of clan language.  Other languages still garble though. -- Altrag */
      if( speaking != -1 )
      {
         int speakswell = UMIN( knows_language( gch, speaking ), knows_language( ch, speaking ) );

         if( speakswell < 85 )
            ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name, translate( speakswell, argument, lang_names[speaking] ) );
         else
            ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name, argument );
      }
      else
         ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name, argument );
   }
}

/*
 * Language support functions. -- Altrag
 * 07/01/96
 *
 * Modified to return how well the language is known 04/04/98 - Thoric
 * Currently returns 100% for known languages... but should really return
 * a number based on player's wisdom (maybe 50+((25-wisdom)*2) ?)
 */
int knows_language( CHAR_DATA *ch, int language )
{
   short sn;

   if( !is_npc( ch ) && is_immortal( ch ) )
      return 100;
   if( is_npc( ch ) && xIS_EMPTY( ch->speaks ) ) /* No langs = knows all for npcs */
      return 100;
   if( is_npc( ch ) && xIS_SET( ch->speaks, language ) )
      return 100;
   /* everyone KNOWS common tongue */
   if( language == LANG_COMMON )
      return 100;
   if( !is_npc( ch ) )
   {
      int lang;

      if( xIS_SET( race_table[ch->race]->language, language ) )
         return 100;

      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      {
         if( language == lang_array[lang] && xIS_SET( ch->speaks, lang_array[lang] ) )
         {
            if( ( sn = skill_lookup( lang_names[lang] ) ) != -1 )
               return ch->pcdata->learned[sn];
         }
      }
   }
   return 0;
}

int countlangs( EXT_BV languages )
{
   int numlangs = 0, looper;

   for( looper = 0; lang_array[looper] != LANG_UNKNOWN; looper++ )
   {
      if( xIS_SET( languages, lang_array[looper] ) )
         numlangs++;
   }
   return numlangs;
}

CMDF( do_speak )
{
   char arg[MIL];
   int langs;

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "all" ) && is_immortal( ch ) )
   {
      for( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
         xSET_BIT( ch->speaking, lang_array[langs] );
      send_to_char( "&[say]Now speaking all languages.\r\n", ch );
      return;
   }
   for( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
   {
      if( !str_prefix( arg, lang_names[langs] ) && knows_language( ch, lang_array[langs] ) )
      {
         xCLEAR_BITS( ch->speaking );
         xSET_BIT( ch->speaking, lang_array[langs] );
         set_char_color( AT_SAY, ch );
         ch_printf( ch, "You now speak %s.\r\n", lang_names[langs] );
         return;
      }
   }
   set_char_color( AT_SAY, ch );
   send_to_char( "You don't know that language.\r\n", ch );
}

CMDF( do_languages )
{
   CHAR_DATA *sch;
   char arg[MIL];
   int lang, col = 0, sn, prct, prac, speaking = -1, langs, adept;

   if( is_npc( ch ) )
   {
      send_to_char( "You have no reason to use languages.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      {
         if( knows_language( ch, lang_array[lang] ) )
         {
            if( xIS_SET( ch->speaking, lang_array[lang] ) )
               set_char_color( AT_SAY, ch );
            else
               set_char_color( AT_PLAIN, ch );
            send_to_char( lang_names[lang], ch );
            if( ++col == 4 )
            {
               send_to_char( "\r\n", ch );
               col = 0;
            }
            else
               send_to_char( " ", ch );
         }
      }
      if( col != 0 )
         send_to_char( "\r\n", ch );
      return;
   }

   if( str_cmp( arg, "learn" ) )
   {
      send_to_char( "Usage: languages [learn <language>]\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Learn which language?\r\n", ch );
      return;
   }

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
   {
      if( !str_prefix( argument, lang_names[lang] ) )
         break;
   }

   if( lang_array[lang] == LANG_UNKNOWN )
   {
      send_to_char( "That is not a language.\r\n", ch );
      return;
   }

   if( !( VALID_LANGS & lang_array[lang] ) )
   {
      send_to_char( "You may not learn that language.\r\n", ch );
      return;
   }

   if( ( sn = skill_lookup( lang_names[lang] ) ) < 0 )
   {
      send_to_char( "That is not a language.\r\n", ch );
      return;
   }

   adept = get_adept( ch, sn );
   if( ch->pcdata->learned[sn] >= adept )
   {
      act( AT_PLAIN, "You're already fluent in $t.", ch, (char *)lang_names[lang], NULL, TO_CHAR );
      return;
   }

   for( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
   {
      if( xIS_SET( ch->speaking, lang_array[langs] ) )
      {
         speaking = langs;
         break;
      }
   }

   for( sch = ch->in_room->first_person; sch; sch = sch->next_in_room )
   {
      if( is_npc( sch ) && xIS_SET( sch->act, ACT_SCHOLAR )
      && ( speaking == -1 || knows_language( sch, speaking ) )
      && knows_language( sch, lang_array[lang] ) )
         break;
   }

   if( !sch )
   {
      send_to_char( "There is no one who can teach that language here.\r\n", ch );
      return;
   }

   if( countlangs( ch->speaks ) >= ( ch->level / 10 ) && ch->pcdata->learned[sn] <= 0 )
   {
      act_tell( ch, sch, "$n tells you 'You may not learn a new language yet.'", sch, NULL, ch, TO_VICT );
      return;
   }

   /* 0..16 cha = 2 pracs, 17..25 = 1 prac. -- Altrag */
   prac = urange( 1, ( ch->level - get_curr_cha( ch ) ), MAX_LEVEL );
   if( ch->practice < prac )
   {
      act_tell( ch, sch, "$n tells you 'You don't have enough practices.'", sch, NULL, ch, TO_VICT );
      return;
   }

   ch->practice -= prac;

   prct = 5 + ( get_curr_int( ch ) / 6 ) + ( get_curr_wis( ch ) / 7 );
   ch->pcdata->learned[sn] += prct;
   ch->pcdata->learned[sn] = UMIN( ch->pcdata->learned[sn], adept );
   xSET_BIT( ch->speaks, lang_array[lang] );
   if( ch->pcdata->learned[sn] == prct )
      act( AT_PLAIN, "You begin lessons in $t.", ch, (char *)lang_names[lang], NULL, TO_CHAR );
   else if( ch->pcdata->learned[sn] >= adept )
      act( AT_PLAIN, "You are now as fluent in $t as you can get.", ch, (char *)lang_names[lang], NULL, TO_CHAR );
   else
      act( AT_PLAIN, "You continue lessons in $t.", ch, (char *)lang_names[lang], NULL, TO_CHAR );
}
