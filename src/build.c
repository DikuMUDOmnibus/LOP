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
 *                       Online Building and Editing Module                  *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "h/mud.h"
#include "h/sha256.h"

extern int top_explorer;
void remove_from_banks( const char *player );
void remove_from_highscores( const char *name );

REL_DATA *first_relation = NULL, *last_relation = NULL;

/*
 * Exit Pull/push types
 * (water, air, earth, fire)
 */
const char *ex_pmisc[] = { "undefined", "vortex", "vacuum", "slip", "ice", "mysterious" };

const char *ex_pwater[] = { "current", "wave", "whirlpool", "geyser" };

const char *ex_pair[] = { "wind", "storm", "coldwind", "breeze" };

const char *ex_pearth[] = { "landslide", "sinkhole", "quicksand", "earthquake" };

const char *ex_pfire[] = { "lava", "hotair" };

const char *trig_flags[] =
{
   "up",             "unlock",        "lock",          "d_north",        "d_south",
   "d_east",         "d_west",        "d_up",          "d_down",         "d_northeast",
   "d_northwest",    "d_southeast",   "d_southwest",   "d_somewhere",    "door",
   "container",      "open",          "close",         "passage",        "oload",
   "mload",          "teleport",      "teleportall",   "teleportplus",   "death",
   "cast",           "rand4",         "rand6",         "rand10",         "rand11",
   "showroomdesc",   "autoreturn"
};

int get_trigflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( trig_flags ) / sizeof( trig_flags[0] ) ); x++ )
      if( !str_cmp( flag, trig_flags[x] ) )
         return x;
   return -1;
}

const char *cont_flags[] =
{
   "closeable",   "pickproof",   "closed",   "locked",   "eatkey",
   "r6",          "r7",          "r8",       "r9",       "r10",
   "r11",         "r12",         "r13",      "r14",      "r15",
   "r16",         "r17",         "r18",      "r19",      "r20",
   "r21",         "r22",         "r23",      "r24",      "r25",
   "r26",         "r27",         "r28",      "r29",      "r30",
   "r31",         "r32"
};

int get_contflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( cont_flags ) / sizeof( cont_flags[0] ) ); x++ )
      if( !str_cmp( flag, cont_flags[x] ) )
         return x;
   return -1;
}

/*
 * Note: I put them all in one big set of flags since almost all of these
 * can be shared between mobs, objs and rooms for the exception of
 * bribe and hitprcnt, which will probably only be used on mobs.
 * ie: drop -- for an object, it would be triggered when that object is
 * dropped; -- for a room, it would be triggered when anything is dropped
 *          -- for a mob, it would be triggered when anything is dropped
 *
 * Something to consider: some of these triggers can be grouped together,
 * and differentiated by different arguments... for example:
 *  hour and time, rand and randiw, speech and speechiw
 * 
 */
const char *mprog_flags[] =
{
   "act",        "speech",   "rand",     "fight",      "death",
   "hitprcnt",   "entry",    "greet",    "allgreet",   "give",
   "bribe",      "hour",     "time",     "wear",       "remove",
   "sac",        "look",     "exa",      "zap",        "get",
   "drop",       "damage",   "repair",   "pull",       "push",
   "sleep",      "rest",     "leave",    "script",     "use",
   "scrap",      "open",     "close",    "put"
};

char *flag_string( int bitvector, const char *flagarray[] )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < 32; x++ )
      if( IS_SET( bitvector, 1 << x ) && flagarray[x] )
      {
         mudstrlcat( buf, flagarray[x], sizeof( buf ) );
         /* don't catenate a blank if the last char is blank  --Gorog */
         if( buf[0] != '\0' && ' ' != buf[strlen( buf ) - 1] )
            mudstrlcat( buf, " ", sizeof( buf ) );
      }
   if( ( x = strlen( buf ) ) > 0 )
      buf[--x] = '\0';

   return buf;
}

char *ext_class_string( EXT_BV *bitvector )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < MAX_BITS; x++ )
   {
      if( xIS_SET( *bitvector, x ) && x < MAX_PC_CLASS && class_table[x] && class_table[x]->name )
      {
         mudstrlcat( buf, class_table[x]->name, sizeof( buf ) );
         if( buf[0] != '\0' && buf[strlen( buf ) - 1] != ' ' )
            mudstrlcat( buf, " ", sizeof( buf ) );
      }
   }
   if( ( x = strlen( buf ) ) > 0 )
      buf[--x] = '\0';

   return buf;
}

char *ext_race_string( EXT_BV *bitvector )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < MAX_BITS; x++ )
   {
      if( xIS_SET( *bitvector, x ) && x < MAX_PC_RACE && race_table[x] && race_table[x]->name )
      {
         mudstrlcat( buf, race_table[x]->name, sizeof( buf ) );
         if( buf[0] != '\0' && buf[strlen( buf ) - 1] != ' ' )
            mudstrlcat( buf, " ", sizeof( buf ) );
      }
   }
   if( ( x = strlen( buf ) ) > 0 )
      buf[--x] = '\0';

   return buf;
}

char *ext_flag_string( EXT_BV *bitvector, const char *flagarray[] )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < MAX_BITS; x++ )
   {
      if( xIS_SET( *bitvector, x ) )
      {
         mudstrlcat( buf, flagarray[x], sizeof( buf ) );
         if( buf[0] != '\0' && buf[strlen( buf ) - 1] != ' ' )
            mudstrlcat( buf, " ", sizeof( buf ) );
      }
      /* Lets break out once max is found...can crash otherwise */
      if( !str_cmp( flagarray[x], "max" ) )
         break;
   }
   if( ( x = strlen( buf ) ) > 0 )
      buf[--x] = '\0';

   return buf;
}

bool is_npc( CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return false;
   }
   if( xIS_SET( ch->act, ACT_IS_NPC ) )
      return true;
   return false;
}

bool is_immortal( CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return false;
   }
   if( get_trust( ( ch ) ) >= PERM_IMM )
      return true;
   return false;
}

bool is_avatar( CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return false;
   }
   if( ch->level >= MAX_LEVEL )
      return true;
   return false;
}

bool valid_destbuf( CHAR_DATA *ch, const char *function )
{
   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return false;
   }
   if( !ch->dest_buf )
   {
      send_to_char( "Fatal error: report to an Immortal.\r\n", ch );
      bug( "%s: [%s] is checking on a NULL ch->dest_buf", __FUNCTION__, function );
      ch->substate = SUB_NONE;
      return false;
   }
   return true;
}

bool can_rmodify( CHAR_DATA *ch, ROOM_INDEX_DATA *room )
{
   AREA_DATA *pArea;
   int vnum = room->vnum;

   if( is_npc( ch ) )
      return false;
   if( get_trust( ch ) >= sysdata.perm_modify_proto )
      return true;
   if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
   {
      send_to_char( "You must have an assigned area to modify this room.\r\n", ch );
      return false;
   }
   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return true;

   send_to_char( "That room is not in your allocated range.\r\n", ch );
   return false;
}

bool can_omodify( CHAR_DATA *ch, OBJ_DATA *obj )
{
   AREA_DATA *pArea;
   int vnum = obj->pIndexData->vnum;

   if( is_npc( ch ) )
      return false;
   if( get_trust( ch ) >= sysdata.perm_modify_proto )
      return true;
   if( !is_obj_stat( obj, ITEM_PROTOTYPE ) )
   {
      send_to_char( "You can't modify this object.\r\n", ch );
      return false;
   }
   if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
   {
      send_to_char( "You must have an assigned area to modify this object.\r\n", ch );
      return false;
   }
   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return true;

   send_to_char( "That object is not in your allocated range.\r\n", ch );
   return false;
}

bool can_mmodify( CHAR_DATA *ch, CHAR_DATA *mob )
{
   AREA_DATA *pArea;
   int vnum;

   if( mob == ch )
      return true;

   if( !is_npc( mob ) )
   {
      if( get_trust( ch ) >= sysdata.perm_modify_proto && get_trust( ch ) > get_trust( mob ) )
         return true;
      else
         send_to_char( "You can't do that.\r\n", ch );
      return false;
   }

   vnum = mob->pIndexData->vnum;

   if( is_npc( ch ) )
      return false;
   if( get_trust( ch ) >= sysdata.perm_modify_proto )
      return true;
   if( !xIS_SET( mob->act, ACT_PROTOTYPE ) )
   {
      send_to_char( "You can't modify this mobile.\r\n", ch );
      return false;
   }
   if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
   {
      send_to_char( "You must have an assigned area to modify this mobile.\r\n", ch );
      return false;
   }
   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return true;

   send_to_char( "That mobile is not in your allocated range.\r\n", ch );
   return false;
}

bool can_medit( CHAR_DATA *ch, MOB_INDEX_DATA *mob )
{
   AREA_DATA *pArea;
   int vnum = mob->vnum;

   if( is_npc( ch ) )
      return false;
   if( get_trust( ch ) >= PERM_BUILDER )
      return true;
   if( !xIS_SET( mob->act, ACT_PROTOTYPE ) )
   {
      send_to_char( "You can't modify this mobile.\r\n", ch );
      return false;
   }
   if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
   {
      send_to_char( "You must have an assigned area to modify this mobile.\r\n", ch );
      return false;
   }
   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return true;

   send_to_char( "That mobile is not in your allocated range.\r\n", ch );
   return false;
}

int get_pc_class( char *Class )
{
   int x;

   for( x = 0; x < MAX_PC_CLASS; x++ )
      if( class_table[x] && !str_cmp( class_table[x]->name, Class ) )
         return x;
   return -1;
}

int get_pc_race( char *type )
{
   int i;

   for( i = 0; i < MAX_PC_RACE; i++ )
      if( race_table[i] && !str_cmp( type, race_table[i]->name ) )
         return i;
   return -1;
}

int get_pulltype( char *type )
{
   unsigned int x;

   if( !str_cmp( type, "none" ) || !str_cmp( type, "clear" ) )
      return 0;

   for( x = 0; x < ( sizeof( ex_pmisc ) / sizeof( ex_pmisc[0] ) ); x++ )
      if( !str_cmp( type, ex_pmisc[x] ) )
         return x;

   for( x = 0; x < ( sizeof( ex_pwater ) / sizeof( ex_pwater[0] ) ); x++ )
      if( !str_cmp( type, ex_pwater[x] ) )
         return x + PT_WATER;
   for( x = 0; x < ( sizeof( ex_pair ) / sizeof( ex_pair[0] ) ); x++ )
      if( !str_cmp( type, ex_pair[x] ) )
         return x + PT_AIR;
   for( x = 0; x < ( sizeof( ex_pearth ) / sizeof( ex_pearth[0] ) ); x++ )
      if( !str_cmp( type, ex_pearth[x] ) )
         return x + PT_EARTH;
   for( x = 0; x < ( sizeof( ex_pfire ) / sizeof( ex_pfire[0] ) ); x++ )
      if( !str_cmp( type, ex_pfire[x] ) )
         return x + PT_FIRE;
   return -1;
}

int get_mpflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( mprog_flags ) / sizeof( mprog_flags[0] ) ); x++ )
      if( !str_cmp( flag, mprog_flags[x] ) )
         return x;
   return -1;
}

/* Remove carriage returns from a line */
char *strip_cr( char *str )
{
   static char newstr[MSL];
   int i, j;

   if( !str || str[0] == '\0' )
      return (char *)"";

   for( i = j = 0; str[i] != '\0'; i++ )
   {
      if( str[i] != '\r' )
         newstr[j++] = str[i];
   }
   newstr[j] = '\0';
   return newstr;
}

void start_editing( CHAR_DATA *ch, char *data )
{
   EDITOR_DATA *edit;
   char c;
   int lines, size, lpos;
   bool longline = false, longstring = false;

   if( !ch->desc )
   {
      bug( "%s", "Fatal: start_editing: no desc" );
      return;
   }

   if( ch->substate == SUB_RESTRICTED )
      bug( "%s", "NOT GOOD: start_editing: ch->substate == SUB_RESTRICTED" );

   if( ch->editor )
      stop_editing( ch );

   CREATE( edit, EDITOR_DATA, 1 );
   edit->numlines = 0;
   edit->on_line = 0;
   edit->size = 0;
   size = 0;
   lpos = 0;
   lines = 0;
   if( !data )
      data = (char *)"";

   if( !data )
      bug( "%s: data is NULL!", __FUNCTION__ );
   else
   {
      for( ;; )
      {
         c = data[size++];
         if( c == '\0' )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
         else if( c == '\r' );
         else if( c == '\n' || lpos > ( MLS - 1 ) )
         {
            if( lpos > ( MLS - 1 ) )
               longline = true;
            edit->line[lines][lpos] = '\0';
            ++lines;
            lpos = 0;
            if( c != '\n' && lines < MEL && size < ( MSL - 1 ) ) /* Was loosing what c was here */
               edit->line[lines][lpos++] = c;
         }
         else
            edit->line[lines][lpos++] = c;

         if( lines >= MEL || size >= ( MSL - 1 ) )
         {
            longstring = true;
            edit->line[lines][lpos] = '\0';
            break;
         }
      }
      if( longline )
         send_to_char( "&RThere was at least one line to long for the editor to handle properly and if saved will result in things being rearranged.&D\r\n", ch );
      if( longstring )
         send_to_char( "&RThat string is to big for the editor to handle properly and if saved will result in the loss of data.&D\r\n", ch );
   }
   if( lpos > 0 && lpos < ( MLS - 1 ) && lines < MEL )
   {
      edit->line[lines][lpos] = '~';
      edit->line[lines][lpos + 1] = '\0';
      ++lines;
      lpos = 0;
   }
   edit->numlines = lines;
   edit->size = size;
   edit->on_line = lines;

   set_char_color( AT_GREEN, ch );
   send_to_char( "Begin entering your text now (/? = help /s = save /c = clear /l = list)\r\n", ch );
   send_to_char( "-----------------------------------------------------------------------\r\n> ", ch );

   ch->editor = edit;
   ch->desc->connected = CON_EDITING;
}

char *copy_buffer( CHAR_DATA *ch )
{
   char buf[MSL];
   char tmp[MLS + 2];
   short x, len;

   if( !ch )
   {
      bug( "%s: null ch", __FUNCTION__ );
      return NULL;
   }

   if( !ch->editor )
   {
      bug( "%s: null editor", __FUNCTION__ );
      return NULL;
   }

   buf[0] = '\0';
   for( x = 0; x < ch->editor->numlines; x++ )
   {
      mudstrlcpy( tmp, ch->editor->line[x], sizeof( tmp ) );
      len = strlen( tmp );
      if( tmp != NULL && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         mudstrlcat( tmp, "\r\n", sizeof( tmp ) );
      smash_tilde( tmp );
      mudstrlcat( buf, tmp, sizeof( buf ) );
   }
   return STRALLOC( buf );
}

int get_buffer_size( CHAR_DATA *ch )
{
   char buf[MSL];
   char tmp[MLS + 2];
   short x, len;

   if( !ch )
   {
      bug( "%s: null ch", __FUNCTION__ );
      return 0;
   }

   if( !ch->editor )
   {
      bug( "%s: null editor", __FUNCTION__ );
      return 0;
   }

   buf[0] = '\0';
   for( x = 0; x < ch->editor->numlines; x++ )
   {
      mudstrlcpy( tmp, ch->editor->line[x], sizeof( tmp ) );
      len = strlen( tmp );
      if( tmp != NULL && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         mudstrlcat( tmp, "\r\n", sizeof( tmp ) );
      smash_tilde( tmp );
      mudstrlcat( buf, tmp, sizeof( buf ) );
   }
   return strlen( buf );
}

void stop_editing( CHAR_DATA *ch )
{
   set_char_color( AT_PLAIN, ch );
   if( ch->editor )
   {
      DISPOSE( ch->editor );
      ch->editor = NULL;
   }
   send_to_char( "Editing has stopped.\r\n", ch );
   ch->dest_buf = NULL;
   ch->spare_ptr = NULL;
   ch->substate = SUB_NONE;
   if( ch->desc )
      ch->desc->connected = CON_PLAYING;
}

CMDF( do_goto )
{
   ROOM_INDEX_DATA *location, *in_room;
   CHAR_DATA *fch, *fch_next, *victim;
   AREA_DATA *pArea;
   char arg[MIL];
   int vnum;

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Goto where?\r\n", ch );
      return;
   }

   loc_in_wilderness = false;

   if( !is_number( arg ) && ( fch = get_char_world( ch, arg ) ) )
   {
      if( !is_npc( fch ) && get_trust( ch ) < get_trust( fch )
      && xIS_SET( fch->pcdata->flags, PCFLAG_DND ) )
      {
         pager_printf( ch, "Sorry. %s does not wish to be disturbed.\r\n", fch->name );
         pager_printf( fch, "Your DND flag just foiled %s's goto command.\r\n", ch->name );
         return;
      }
   }

   if( !( location = find_location( ch, arg ) ) )
   {
      vnum = atoi( arg );
      if( vnum < 0 || get_room_index( vnum ) )
      {
         send_to_char( "You can't find that...\r\n", ch );
         return;
      }
      if( get_trust( ch ) < PERM_BUILDER || vnum < 1 || is_npc( ch ) || !ch->pcdata->area )
      {
         send_to_char( "No such location.\r\n", ch );
         return;
      }
      if( get_trust( ch ) < sysdata.perm_modify_proto )
      {
         if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
         {
            send_to_char( "You must have an assigned area to create rooms.\r\n", ch );
            return;
         }
         if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
         {
            send_to_char( "That room is not within your assigned range.\r\n", ch );
            return;
         }
      }
      if( !( location = make_room( vnum, ch->pcdata->area ) ) )
      {
         bug( "%s: make_room failed", __FUNCTION__ );
         return;
      }
      set_char_color( AT_WHITE, ch );
      send_to_char( "Waving your hand, you form order from swirling chaos,\r\nand step into a new reality...\r\n", ch );
   }

   if( ( victim = room_is_dnd( ch, location ) ) )
   {
      send_to_pager( "That room is \"do not disturb\" right now.\r\n", ch );
      pager_printf( victim, "Your DND flag just foiled %s's goto command.\r\n", ch->name );
      return;
   }

   if( room_is_private( location ) )
   {
      if( get_trust( ch ) < sysdata.perm_override_private )
      {
         send_to_char( "That room is private right now.\r\n", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\r\n", ch );
   }

   in_room = ch->in_room;
   stop_fighting( ch, true );

   if( !xIS_SET( ch->act, PLR_WIZINVIS ) )
   {
      if( ch->pcdata && ch->pcdata->bamfout )
         act( AT_IMMORT, "$T", ch, NULL, ch->pcdata->bamfout, TO_ROOM );
      else
         act( AT_IMMORT, "$n leaves in a swirling mist.", ch, NULL, NULL, TO_ROOM );
   }
   ch->regoto = ch->in_room->vnum;
   char_from_room( ch );
   if( ch->mount )
   {
      char_from_room( ch->mount );
      char_to_room( ch->mount, location );
      set_loc_cords( ch->mount );
   }
   char_to_room( ch, location );
   set_loc_cords( ch );

   if( !xIS_SET( ch->act, PLR_WIZINVIS ) )
   {
      if( ch->pcdata && ch->pcdata->bamfin )
         act( AT_IMMORT, "$T", ch, NULL, ch->pcdata->bamfin, TO_ROOM );
      else
         act( AT_IMMORT, "$n appears in a swirling mist.", ch, NULL, NULL, TO_ROOM );
   }
   do_look( ch, (char *)"auto" );

   if( ch->in_room == in_room )
      return;
   for( fch = in_room->first_person; fch; fch = fch_next )
   {
      fch_next = fch->next_in_room;
      if( fch->master == ch && is_immortal( fch ) )
      {
         act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
         do_goto( fch, argument );
      }
      else if( is_npc( fch ) && fch->master == ch )
      {
         if( !xIS_SET( ch->act, PLR_WIZINVIS ) )
            act( AT_IMMORT, "$n leaves in a swirling mist.", fch, NULL, NULL, TO_ROOM );
         char_from_room( fch );
         char_to_room( fch, location );
         set_loc_cords( fch );
         if( !xIS_SET( ch->act, PLR_WIZINVIS ) )
            act( AT_IMMORT, "$n appears in a swirling mist.", fch, NULL, NULL, TO_ROOM );
      }
   }
}

CMDF( do_mset )
{
   CHAR_DATA *victim;
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MSL], *origarg = argument;
   int stat, v2, value, minattr, maxattr;
   bool lockvictim;

   set_char_color( AT_PLAIN, ch );
   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't mset\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_MOB_DESC:
         if( !valid_destbuf( ch, __FUNCTION__ ) )
            return;
         if( !( victim = ( CHAR_DATA *) ch->dest_buf ) )
         {
            send_to_char( "Looks like the victim you were editing got lost along the way.\r\n", ch );
            stop_editing( ch );
            return;
         }
         if( char_died( victim ) )
         {
            send_to_char( "Your victim died!\r\n", ch );
            stop_editing( ch );
            return;
         }
         victim->editing = NULL;
         STRFREE( victim->description );
         victim->description = copy_buffer( ch );
         if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            STRSET( victim->pIndexData->description, victim->description );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         return;
   }

   victim = NULL;
   lockvictim = false;

   if( ch->substate == SUB_REPEATCMD )
   {
      victim = ( CHAR_DATA *) ch->dest_buf;

      if( !victim )
      {
         send_to_char( "Your victim died!\r\n", ch );
         argument = (char *)"done";
      }

      if( !argument || argument[0] == '\0' || !str_cmp( argument, "stat" ) )
      {
         if( victim )
         {
            if( is_npc( victim ) )
               snprintf( buf, sizeof( buf ), "%d", victim->pIndexData->vnum );
            else
               snprintf( buf, sizeof( buf ), "%s", victim->name );
            do_mstat( ch, buf );
         }
         else
            send_to_char( "No victim selected.  Type '?' for help.\r\n", ch );
         return;
      }
      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         if( ch->dest_buf )
            RelDestroy( relMSET_ON, ch, ch->dest_buf );
         send_to_char( "Mset mode off.\r\n", ch );
         ch->substate = SUB_NONE;
         ch->dest_buf = NULL;
         if( ch->pcdata && ch->pcdata->subprompt )
         {
            STRFREE( ch->pcdata->subprompt );
            ch->pcdata->subprompt = NULL;
         }
         return;
      }
   }
   if( victim )
   {
      lockvictim = true;
      mudstrlcpy( arg1, victim->name, sizeof( arg1 ) );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, sizeof( arg3 ) );
   }
   else
   {
      lockvictim = false;
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, sizeof( arg3 ) );
   }

   if( !str_cmp( arg1, "on" ) )
   {
      send_to_char( "Usage: mset <victim|vnum> on.\r\n", ch );
      return;
   }

   if( arg1 == NULL || arg1[0] == '\0' || ( ( arg2 == NULL || arg2[0] == '\0' ) && ch->substate != SUB_REPEATCMD ) || !str_cmp( arg1, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( victim )
            send_to_char( "Usage: <field>  <value>\r\n", ch );
         else
            send_to_char( "Usage: <victim> <field>  <value>\r\n", ch );
      }
      else
         send_to_char( "Usage: mset <victim> <field>  <value>\r\n", ch );
      send_to_char( "\r\n", ch );
      send_to_char( "Field being one of:\r\n", ch );
      send_to_char( "    hp   move   align     title    council      dexterity\r\n", ch );
      send_to_char( "    qp   name   armor    attack    damroll      resistant\r\n", ch );
      send_to_char( "   age   part   blood    height    defense     numattacks\r\n", ch );
      send_to_char( "   pos   race   class    maxhit    hitroll    description\r\n", ch );
      send_to_char( "   sex   rank   deity    minhit   affected    mentalstate\r\n", ch );
      send_to_char( "  clan   sav1   drunk    nation   charisma   constitution\r\n", ch );
      send_to_char( "  full   sav2   favor    speaks   minsnoop   intelligence\r\n", ch );
      send_to_char( "  gold   sav3   flags    thirst   password\r\n", ch );
      send_to_char( "  long   sav4   level    wisdom   practice\r\n", ch );
      send_to_char( "  luck   sav5   pkill    weight   speaking\r\n", ch );
      send_to_char( "  mana   spec   short   aloaded   strength\r\n", ch );
      return;
   }

   if( !victim && get_trust( ch ) < PERM_LEADER )
   {
      if( !( victim = get_char_room( ch, arg1 ) ) )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }
   }
   else if( !victim )
   {
      if( !( victim = get_char_world( ch, arg1 ) ) )
      {
         send_to_char( "No one like that in all the realms.\r\n", ch );
         return;
      }
   }

   if( !can_mmodify( ch, victim ) )
      return;

   if( get_trust( ch ) < get_trust( victim ) && !is_npc( victim ) )
   {
      send_to_char( "You can't do that!\r\n", ch );
      ch->dest_buf = NULL;
      return;
   }
   if( get_trust( ch ) < PERM_HEAD && is_npc( victim ) && xIS_SET( victim->act, ACT_STATSHIELD ) )
   {
      send_to_char( "You can't do that!\r\n", ch );
      ch->dest_buf = NULL;
      return;
   }
   if( lockvictim )
      ch->dest_buf = victim;

   if( is_npc( victim ) || is_immortal( victim ) )
   {
      minattr = 1;
      maxattr = ( MAX_LEVEL + 25 );
   }
   else
   {
      minattr = 3;
      maxattr = ( MAX_LEVEL );
   }

   if( !str_cmp( arg2, "on" ) )
   {
      if( check_subrestricted( ch ) )
         return;
      ch_printf( ch, "Mset mode on. (Editing %s).\r\n", victim->name );
      ch->substate = SUB_REPEATCMD;
      ch->dest_buf = victim;
      if( ch->pcdata )
      {
         if( is_npc( victim ) )
            snprintf( buf, sizeof( buf ), "<&CMset &W#%d&w> %%i", victim->pIndexData->vnum );
         else
            snprintf( buf, sizeof( buf ), "<&CMset &W%s&w> %%i", victim->name );
         STRSET( ch->pcdata->subprompt, buf );
      }
      RelCreate( relMSET_ON, ch, victim );
      return;
   }
   value = is_number( arg3 ) ? atoi( arg3 ) : -1;

   if( atoi( arg3 ) < -1 && value == -1 )
      value = atoi( arg3 );

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( !str_cmp( arg2, stattypes[stat] ) )
      {
         if( value < minattr || value > maxattr )
         {
            ch_printf( ch, "%s range is %d to %d.\r\n", capitalize( stattypes[stat] ), minattr, maxattr );
            return;
         }
         victim->perm_stats[stat] = value;
         ch_printf( ch, "%s set to %d\r\n", capitalize( stattypes[stat] ), value );
         if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
         {
            victim->pIndexData->perm_stats[stat] = value;
            send_to_char( "Index set too.\r\n", ch );
         }
         return;
      }
   }

   /* Bound to be a bit faster if you do this */
   switch( UPPER( arg2[0] ) )
   {
      case 'A':
         if( !str_cmp( arg2, "age" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "You can only set a players age.\r\n", ch );
               return;
            }
            if( value < 18 )
            {
               send_to_char( "You can't set someone's age to less then 18.\r\n", ch );
               return;
            }
            victim->pcdata->birth_year = get_birth_year( ch, value );
            save_char_obj( victim );
            ch_printf( ch, "%s's age has been set to %d.\r\n", victim->name, get_age( victim ) );
            return;
         }

         if( !str_cmp( arg2, "align" ) )
         {
            victim->alignment = URANGE( -1000, value, 1000 );
            ch_printf( ch, "Align set to %d.\r\n", victim->alignment );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->alignment = victim->alignment;
               send_to_char( "Prototype Align set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "armor" ) )
         {
            victim->armor = URANGE( 0, value, 1000 );
            ch_printf( ch, "Armor set to %d.\r\n", victim->armor );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->ac = victim->armor;
               send_to_char( "Prototype Armor set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "affected" ) )
         {
            if( !is_npc( victim ) && get_trust( ch ) < PERM_LEADER )
            {
               send_to_char( "You can only modify a mobile's flags.\r\n", ch );
               return;
            }

            if( !can_mmodify( ch, victim ) )
               return;
            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <victim> affected <flag> [flag]...\r\n", ch );
               return;
            }
            while( argument && argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_flag( arg3, a_flags, AFF_MAX );
               if( value < 0 || value >= AFF_MAX )
                  ch_printf( ch, "Unknown affected: %s\r\n", arg3 );
               else
               {
                  xTOGGLE_BIT( victim->affected_by, value );
                  ch_printf( ch, "%s affected %s.\r\n", a_flags[value], xIS_SET( victim->affected_by, value ) ? "set" : "removed" );
                  if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                  {
                     xTOGGLE_BIT( victim->pIndexData->affected_by, value );
                     ch_printf( ch, "%s index affected %s.\r\n", a_flags[value], xIS_SET( victim->pIndexData->affected_by, value ) ? "set" : "removed" );
                  }
               }
            }
            return;
         }

         if( !str_cmp( arg2, "attack" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "You can only modify a mobile's attacks.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <victim> attack <flag> [flag]...\r\n", ch );
               return;
            }
            while( argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_flag( arg3, attack_flags, ATCK_MAX );
               if( value < 0 || value >= ATCK_MAX )
                  ch_printf( ch, "Unknown attack: %s\r\n", arg3 );
               else
               {
                  xTOGGLE_BIT( victim->attacks, value );
                  ch_printf( ch, "%s attack %s.\r\n", attack_flags[value], xIS_SET( victim->attacks, value ) ? "set" : "removed" );
                  if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                  {
                     xTOGGLE_BIT( victim->pIndexData->attacks, value );
                     ch_printf( ch, "%s index attack %s.\r\n", attack_flags[value], xIS_SET( victim->pIndexData->attacks, value ) ? "set" : "removed" );
                  }
               }
            }
            return;
         }

         if( !str_cmp( arg2, "aloaded" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Player Characters only.\r\n", ch );
               return;
            }

            if( !victim->pcdata->area )
            {
               send_to_char( "Player does not have an area assigned to them.\r\n", ch );
               return;
            }

            if( !can_mmodify( ch, victim ) )
               return;

            if( !IS_SET( victim->pcdata->area->status, AREA_LOADED ) )
            {
               SET_BIT( victim->pcdata->area->status, AREA_LOADED );
               send_to_char( "Your area set to LOADED!\r\n", victim );
               if( ch != victim )
                  send_to_char( "Area set to LOADED!\r\n", ch );
               return;
            }
            else
            {
               REMOVE_BIT( victim->pcdata->area->status, AREA_LOADED );
               send_to_char( "Your area set to NOT-LOADED!\r\n", victim );
               if( ch != victim )
                  send_to_char( "Area set to NON-LOADED!\r\n", ch );
               return;
            }
         }
         break;

      case 'B':
         if( !str_cmp( arg2, "blood" ) )
         {
            victim->max_mana = UMAX( 0, value );
            ch_printf( ch, "%s set to %d.\r\n", arg2, victim->max_mana );
            return;
         }
         break;

      case 'C':
         if( !str_cmp( arg2, "class" ) )
         {
            MCLASS_DATA *mclass;
            int uclass, mcount = 0;
            bool add = false;

            if( is_npc( victim ) )
            {
               send_to_char( "Mobiles have no reason to have a class.\r\n", ch );
               return;
            }

            value = -1;

            argument = one_argument( argument, arg2 );
            if( !str_cmp( arg2, "add" ) )
               add = true;
            else if( is_number( arg2 ) )
            {
               if( ( value = atoi( arg2 ) ) < 0 )
               {
                  ch_printf( ch, "Invalid class number use a number equal to or above %d.\r\n", 0 );
                  return;
               }
            }
            else
            {
               if( ( value = get_char_cnum( ch, arg2 ) ) < 0 )
               {
                  send_to_char( "They don't have that class on them to change.\r\n", ch );
                  return;
               }
            }

            uclass = get_pc_class( argument );
            if( uclass < 0 && is_number( argument ) )
               uclass = atoi( argument );
            if( uclass < -1 || uclass >= MAX_PC_CLASS )
            {
               ch_printf( ch, "Class range is -1 to %d.\r\n", ( MAX_PC_CLASS - 1 ) );
               return;
            }
            if( uclass >= 0 && char_is_class( victim, uclass ) )
            {
               ch_printf( ch, "They already have class [%d]%s.\r\n", uclass, dis_class_name( uclass ) );
               return;
            }

            if( add )
            {
               if( uclass >= 0 )
               {
                  if( uclass >= 0 && char_is_class( victim, uclass ) )
                  {
                     ch_printf( ch, "They already have class [%d]%s.\r\n", uclass, dis_class_name( uclass ) );
                     return;
                  }
                  if( !( mclass = add_mclass( victim, uclass, 2, 0, 0.0, false ) ) )
                     ch_printf( ch, "Failed to add [%d]%s to them.\r\n", uclass, dis_class_name( uclass ) );
                  else
                     ch_printf( ch, "Class [%d]%s has been added to them.\r\n", mclass->wclass, dis_class_name( mclass->wclass ) );
               }
               else
                  send_to_char( "Need to specify a valid class to be added.\r\n", ch );
               return;
            }

            for( mclass = victim->pcdata->first_mclass; mclass; mclass = mclass->next )
            {
               if( mcount++ == value )
               {
                  mclass->wclass = uclass;
                  ch_printf( ch, "Class %d has been set to [%d]%s.\r\n", value, mclass->wclass, dis_class_name( mclass->wclass ) );
                  break;
               }
            }

            if( !mclass )
            {
               send_to_char( "There is no class in that spot on them to set.\r\n", ch );
               return;
            }

            if( mclass->wclass == -1 )
            {
               if( mcount > 1 ) /* They have more then one class set */
               {
                  UNLINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
                  DISPOSE( mclass );
                  update_level( ch );
                  send_to_char( "The class has been removed from them completely.\r\n", ch );
               }
               else
                  send_to_char( "They only have 1 class and it has been set to none.\r\n", ch );
               return;
            }
            return;
         }

         if( !str_cmp( arg2, "clan" ) )
         {
            CLAN_DATA *clan;

            if( get_trust( ch ) < PERM_LEADER )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }

            if( arg3 == NULL || arg3[0] == '\0' )
            {
               if( victim->pcdata->clan )
               {
                  remove_clan_member( victim->pcdata->clan, victim->name );
                  save_clan( victim->pcdata->clan );
               }
               victim->pcdata->clan = NULL;
               send_to_char( "Clan set to nothing.\r\n", ch );
               return;
            }
            if( !( clan = get_clan( arg3 ) ) || clan->clan_type == CLAN_NATION )
            {
               send_to_char( "No such clan.\r\n", ch );
               return;
            }
            if( victim->pcdata->clan )
            {
               remove_clan_member( victim->pcdata->clan, victim->name );
               save_clan( victim->pcdata->clan );
            }
            if( clan->clan_type == CLAN_PLAIN )
               xSET_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
            victim->pcdata->clan = clan;
            add_clan_member( clan, victim->name );
            save_clan( victim->pcdata->clan );
            send_to_char( "Clan set.\r\n", ch );
            return;
         }

         if( !str_cmp( arg2, "council" ) )
         {
            COUNCIL_DATA *council;

            if( get_trust( ch ) < PERM_HEAD )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }

            if( arg3 == NULL || arg3[0] == '\0' )
            {
               if( victim->pcdata->council )
               {
                  remove_council_member( victim->pcdata->council, victim->name );
                  save_council( victim->pcdata->council );
               }
               victim->pcdata->council = NULL;
               send_to_char( "Removed from council.\r\n", ch );
               return;
            }

            if( !( council = get_council( arg3 ) ) )
            {
               send_to_char( "No such council.\r\n", ch );
               return;
            }

            if( victim->pcdata->council )
            {
               remove_council_member( victim->pcdata->council, victim->name );
               save_council( victim->pcdata->council );
            }
            victim->pcdata->council = council;
            add_council_member( council, victim->name );
            save_council( council );
            send_to_char( "Done.\r\n", ch );
            return;
         }
         break;

      case 'D':
         if( !str_cmp( arg2, "damroll" ) )
         {
            victim->damroll = URANGE( 0, value, 65 );
            ch_printf( ch, "Damroll set to %d.\r\n", victim->damroll );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->damroll = victim->damroll;
               send_to_char( "Prototype Damroll set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "drunk" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            victim->pcdata->condition[COND_DRUNK] = URANGE( 0, value, 100 );
            ch_printf( ch, "Drunk set to %d.\r\n", victim->pcdata->condition[COND_DRUNK] );
            return;
         }

         if( !str_cmp( arg2, "deity" ) )
         {
            DEITY_DATA *deity;

            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }

            if( arg3 == NULL || arg3[0] == '\0' )
            {
               if( victim->pcdata->deity )
               {
                  --victim->pcdata->deity->worshippers;
                  if( victim->pcdata->deity->worshippers < 0 )
                     victim->pcdata->deity->worshippers = 0;
                  save_deity( victim->pcdata->deity );
               }
               victim->pcdata->deity = NULL;
               send_to_char( "Deity removed.\r\n", ch );
               return;
            }

            if( !( deity = get_deity( arg3 ) ) )
            {
               send_to_char( "No such deity.\r\n", ch );
               return;
            }
            if( victim->pcdata->deity )
            {
               --victim->pcdata->deity->worshippers;
               if( victim->pcdata->deity->worshippers < 0 )
                  victim->pcdata->deity->worshippers = 0;
               save_deity( victim->pcdata->deity );
            }
            victim->pcdata->deity = deity;
            ++deity->worshippers;
            save_deity( deity );
            send_to_char( "Done.\r\n", ch );
            return;
         }

         if( !str_cmp( arg2, "description" ) )
         {
            if( victim->editing )
            {
               send_to_char( "Someone is currently editing the victim's description.\r\n", ch );
               return;
            }
            if( arg3 != NULL && arg3[0] != '\0' )
            {
               STRSET( victim->description, arg3 );
               send_to_char( "Description Set.\r\n", ch );
               if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
               {
                  STRSET( victim->pIndexData->description, victim->description );
                  send_to_char( "Prototype Description Set.\r\n", ch );
               }
               return;
            }

            if( check_subrestricted( ch ) )
               return;
            if( ch->substate == SUB_REPEATCMD )
               ch->tempnum = SUB_REPEATCMD;
            else
               ch->tempnum = SUB_NONE;
            ch->substate = SUB_MOB_DESC;
            ch->dest_buf = victim;
            victim->editing = ch;
            start_editing( ch, victim->description );
            return;
         }

         if( !str_cmp( arg2, "defense" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "You can only modify a mobile's defenses.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <victim> defense <flag> [flag]...\r\n", ch );
               return;
            }
            while( argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_flag( arg3, defense_flags, DFND_MAX );
               if( value < 0 || value >= DFND_MAX )
                  ch_printf( ch, "Unknown defense: %s\r\n", arg3 );
               else
               {
                  xTOGGLE_BIT( victim->defenses, value );
                  ch_printf( ch, "%s defense %s.\r\n", defense_flags[value], xIS_SET( victim->defenses, value ) ? "set" : "removed" );
                  if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                  {
                     xTOGGLE_BIT( victim->pIndexData->defenses, value );
                     ch_printf( ch, "%s index defense %s.\r\n", defense_flags[value], xIS_SET( victim->pIndexData->defenses, value ) ? "set" : "removed" );
                  }
               }
            }
            return;
         }
         break;

      case 'F':
         if( !str_cmp( arg2, "favor" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            victim->pcdata->favor = URANGE( -2500, value, 2500 );
            ch_printf( ch, "Favor set to %d.\r\n", victim->pcdata->favor );
            return;
         }

         if( !str_cmp( arg2, "full" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            victim->pcdata->condition[COND_FULL] = URANGE( 0, value, 100 );
            ch_printf( ch, "Full set to %d.\r\n", victim->pcdata->condition[COND_FULL] );
            return;
         }

         if( !str_cmp( arg2, "flags" ) )
         {
            bool pcflag;

            if( !is_npc( victim ) && get_trust( ch ) < PERM_LEADER )
            {
               send_to_char( "You can only modify a mobile's flags.\r\n", ch );
               return;
            }

            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <vic> flags <flag> [flag]...\r\n", ch );
               return;
            }

            while( argument && argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );

               pcflag = false;

               if( is_npc( victim ) )
               {
                  value = get_flag( arg3, act_flags, ACT_MAX );

                  if( value < 0 || value >= ACT_MAX )
                     ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
                  else if( value == ACT_PROTOTYPE && ch->level < sysdata.perm_modify_proto )
                     send_to_char( "You can't change the prototype flag.\r\n", ch );
                  else if( value == ACT_IS_NPC )
                     send_to_char( "If the NPC flag could be changed, it would cause many problems.\r\n", ch );
                  else
                  {
                     xTOGGLE_BIT( victim->act, value );
                     ch_printf( ch, "%s flag %s.\r\n", act_flags[value], xIS_SET( victim->act, value ) ? "set" : "removed" );
                     if( xIS_SET( victim->act, ACT_PROTOTYPE ) || value == ACT_PROTOTYPE )
                     {
                        xTOGGLE_BIT( victim->pIndexData->act, value );
                        ch_printf( ch, "%s index flag %s.\r\n", act_flags[value], xIS_SET( victim->pIndexData->act, value ) ? "set" : "removed" );
                     }
                  }
               }
               else
               {
                  value = get_flag( arg3, plr_flags, PLR_MAX );

                  if( value < 0 || value >= PLR_MAX )
                  {
                     value = get_flag( arg3, pc_flags, PCFLAG_MAX );
                     if( value < 0 || value >= PCFLAG_MAX )
                        ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
                     else
                     {
                        xTOGGLE_BIT( victim->pcdata->flags, value );
                        ch_printf( ch, "%s pcflag %s.\r\n", pc_flags[value], xIS_SET( victim->pcdata->flags, value ) ? "set" : "removed" );
                     }
                  }
                  else if( value == PLR_IS_NPC )
                     send_to_char( "If the NPC flag could be changed, it would cause many problems.\r\n", ch );
                  else
                  {
                     xTOGGLE_BIT( victim->act, value );
                     ch_printf( ch, "%s flag %s.\r\n", plr_flags[value], xIS_SET( victim->act, value ) ? "set" : "removed" );
                  }
               }
            }
            return;
         }
         break;

      case 'G':
         if( !str_cmp( arg2, "gold" ) )
         {
            int unval = 0;

            if( !is_number( arg3 ) )
            {
               send_to_char( "You have to specify an amount.\r\n", ch );
               return;
            }

            unval = atoi( arg3 );

            set_gold( victim, unval );
            ch_printf( ch, "Gold set to %s.\r\n", show_char_gold( victim ) );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->gold = victim->gold;
               send_to_char( "Prototype Gold set to match.\r\n", ch );
            }
            return;
         }
         break;

      case 'H':
         if( !str_cmp( arg2, "hitroll" ) )
         {
            victim->hitroll = URANGE( 0, value, 85 );
            ch_printf( ch, "Hitroll set to %d.\r\n", victim->hitroll );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->hitroll = victim->hitroll;
               send_to_char( "Prototype Hitroll set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "height" ) )
         {
            victim->height = UMAX( 0, value );
            ch_printf( ch, "Height set to %d.\r\n", victim->height );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->height = victim->height;
               send_to_char( "Prototype Height set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "hp" ) )
         {
            victim->max_hit = UMAX( 1, value );
            ch_printf( ch, "Hp set to %d.\r\n", victim->max_hit );
            return;
         }
         break;

      case 'L':
         if( !str_cmp( arg2, "level" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "Not on PC's.\r\n", ch );
               return;
            }
            victim->level = URANGE( 1, value, MAX_LEVEL );
            ch_printf( ch, "Level set to %d.\r\n", victim->level );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->level = victim->level;
               send_to_char( "Prototype Level set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "long" ) )
         {
            if( arg3 == NULL || arg3[0] == '\0' )
            {
               send_to_char( "You can't set the long field to nothing.\r\n", ch );
               return;
            }
            mudstrlcpy( buf, arg3, sizeof( buf ) );
            mudstrlcat( buf, "\r\n", sizeof( buf ) );
            STRSET( victim->long_descr, buf );
            send_to_char( "Long set.\r\n", ch );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               STRSET( victim->pIndexData->long_descr, victim->long_descr );
               send_to_char( "Prototype Long set to match.\r\n", ch );
            }
            return;
         }
         break;

      case 'M':
         if( !str_cmp( arg2, "mentalstate" ) )
         {
            victim->mental_state = URANGE( -100, value, 100 );
            ch_printf( ch, "Mentalstate set to %d.\r\n", victim->mental_state );
            return;
         }

         /* Although shouldn't do it much there might be times where you need to modify the mod stats on someone */
         if( !str_cmp( arg2, "modstat" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "You can't set the modstats on npcs.\r\n", ch );
               return;
            }
            argument = one_argument( argument, arg2 );
            if( !( stat = get_flag( arg2, stattypes, STAT_MAX ) ) )
            {
               ch_printf( ch, "%s isn't a valid stat to modify.\r\n", arg2 );
               return;
            }
            argument = one_argument( argument, arg2 );
            value = is_number( arg2 ) ? atoi( arg2 ) : -1;
            victim->mod_stats[stat] = value;
            ch_printf( ch, "Mod_%s set to %d\r\n", capitalize( stattypes[stat] ), value );
            send_to_char( "Make sure you have it set to what it should be based on equipment and spells/skills.\r\n", ch );
            return;
         }

         if( !str_cmp( arg2, "mana" ) )
         {
            victim->max_mana = UMAX( 0, value );
            ch_printf( ch, "%s set to %d.\r\n", arg2, victim->max_mana );
            return;
         }

         if( !str_cmp( arg2, "move" ) )
         {
            victim->max_move = UMAX( 0, value );
            ch_printf( ch, "Move set to %d.\r\n", victim->max_move );
            return;
         }

         if( !str_cmp( arg2, "minsnoop" ) )
         {
            if( get_trust( ch ) < PERM_HEAD )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            if( get_trust( ch ) <= get_trust( victim ) )
            {
               send_to_char( "You aren't able to modify their minsnoop.\r\n", ch );
               return;
            }
            if( value > get_trust( ch ) )
            {
               send_to_char( "You wouldn't be able to snoop them then.\r\n", ch );
               return;
            }
            victim->pcdata->min_snoop = URANGE( 0, value, ( PERM_MAX - 1 ) );
            ch_printf( ch, "Minsnoop set to %d[%s].\r\n", victim->pcdata->min_snoop, perms_flag[victim->pcdata->min_snoop] );
            return;
         }

         if( !str_cmp( arg2, "minhit" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "Mobiles only.\r\n", ch );
               return;
            }
            if( !xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               send_to_char( "Can only set minhit on prototype mobiles.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            if( value < 1 || value > 2000000000 )
            {
               send_to_char( "Minhit range is 1 to 2,000,000,000.\r\n", ch );
               return;
            }
            if( value > victim->pIndexData->maxhit )
            {
               ch_printf( ch, "Increasing maxhit to %d so its higher then minhit.\r\n", value + 100 );
               victim->pIndexData->maxhit = ( value + 100 );
            }
            victim->pIndexData->minhit = value;
            victim->max_hit = number_range( victim->pIndexData->minhit, victim->pIndexData->maxhit );
            victim->hit = victim->max_hit;
            send_to_char( "Done.\r\n", ch );
            return;
         }

         if( !str_cmp( arg2, "maxhit" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "Mobiles only.\r\n", ch );
               return;
            }
            if( !xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               send_to_char( "Can only set maxhit on prototype mobiles.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            if( value < 101 || value > 2000000100 )
            {
               send_to_char( "Maxhit range is 101 to 2,000,000,100.\r\n", ch );
               return;
            }
            if( value <= victim->pIndexData->minhit )
            {
               ch_printf( ch, "Decreasing minhit to %d so its lower then maxhit.\r\n", value - 100 );
               victim->pIndexData->minhit = ( value - 100 );
            }
            victim->pIndexData->maxhit = value;
            victim->max_hit = number_range( victim->pIndexData->minhit, victim->pIndexData->maxhit );
            victim->hit = victim->max_hit;
            send_to_char( "Done.\r\n", ch );
            return;
         }
         break;

      case 'N':
         if( !str_cmp( arg2, "numattacks" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "Not on PC's.\r\n", ch );
               return;
            }

            if( value < 0 || value > 20 )
            {
               send_to_char( "Attacks range is 0 to 20.\r\n", ch );
               return;
            }
            victim->numattacks = URANGE( 0, value, 20 );
            ch_printf( ch, "Numattacks set to %d.\r\n", victim->numattacks );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->numattacks = victim->numattacks;
               send_to_char( "Prototype Numattacks set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "name" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "Not on PC's.\r\n", ch );
               return;
            }
            if( arg3 == NULL || arg3[0] == '\0' )
            {
               send_to_char( "Names can't be set to an empty string.\r\n", ch );
               return;
            }
            STRSET( victim->name, arg3 );
            send_to_char( "Name set.\r\n", ch );
            if( xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               STRSET( victim->pIndexData->name, victim->name );
               send_to_char( "Prototype Name set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "nation" ) )
         {
            CLAN_DATA *nation;

            if( get_trust( ch ) < PERM_LEADER )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }

            if( arg3 == NULL || arg3[0] == '\0' )
            {
               if( victim->pcdata->nation )
               {
                  remove_clan_member( victim->pcdata->nation, victim->name );
                  save_clan( victim->pcdata->nation );
               }
               victim->pcdata->nation = NULL;
               send_to_char( "Nation set to nothing.\r\n", ch );
               return;
            }
            if( !( nation = get_clan( arg3 ) ) || nation->clan_type != CLAN_NATION )
            {
               send_to_char( "No such nation.\r\n", ch );
               return;
            }
            if( victim->pcdata->nation )
            {
               remove_clan_member( victim->pcdata->nation, victim->name );
               save_clan( victim->pcdata->nation );
            }
            victim->pcdata->nation = nation;
            add_clan_member( nation, victim->name );
            save_clan( nation );
            send_to_char( "Nation set.\r\n", ch );
            return;
         }
         break;

      case 'P':
         if( !str_cmp( arg2, "practice" ) )
         {
            victim->practice = URANGE( 0, value, 100 );
            ch_printf( ch, "Practice set to %d.\r\n", victim->practice );
            return;
         }

         if( !str_cmp( arg2, "password" ) )
         {
            char *pwdnew;

            if( get_trust( ch ) < PERM_HEAD )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            if( is_npc( victim ) )
            {
               send_to_char( "Mobs don't have passwords.\r\n", ch );
               return;
            }

            if( strlen( arg3 ) < 5 )
            {
               send_to_char( "New password must be at least five characters long.\r\n", ch );
               return;
            }

            if( arg3[0] == '!' )
            {
               send_to_char( "New password can't begin with the '!' character.", ch );
               return;
            }

            pwdnew = sha256_crypt( arg3 );   /* SHA-256 Encryption */
            STRSET( victim->pcdata->pwd, pwdnew );
            if( xIS_SET( sysdata.save_flags, SV_PASSCHG ) )
               save_char_obj( victim );
            send_to_char( "Password set.\r\n", ch );
            ch_printf( victim, "Your password has been changed by %s.\r\n", ch->name );
            return;
         }

         if( !str_cmp( arg2, "part" ) )
         {
            if( !is_npc( victim ) && get_trust( ch ) < PERM_LEADER )
            {
               send_to_char( "You can only modify a mobile's parts.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <victim> part <flag> [flag]...\r\n", ch );
               return;
            }
            while( argument && argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_flag( arg3, part_flags, PART_MAX );
               if( value < 0 || value >= PART_MAX )
                  ch_printf( ch, "Unknown part: %s\r\n", arg3 );
               else
               {
                  xTOGGLE_BIT( victim->xflags, value );
                  ch_printf( ch, "%s part %s.\r\n", part_flags[value], xIS_SET( victim->xflags, value ) ? "set" : "removed" );
                  if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
                  {
                     xTOGGLE_BIT( victim->pIndexData->xflags, value );
                     ch_printf( ch, "%s index part %s.\r\n", part_flags[value], xIS_SET( victim->pIndexData->xflags, value ) ? "set" : "removed" );
                  }
               }
            }
            return;
         }

         if( !str_cmp( arg2, "pos" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "Mobiles only.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            value = get_flag( arg3, pos_names, POS_STANDING );
            if( value < 0 && is_number( arg3 ) )
               value = atoi( arg3 );
            if( value < 0 || value > POS_STANDING )
            {
               ch_printf( ch, "Position range is 0 to %d.\r\n", POS_STANDING );
               return;
            }
            victim->position = value;
            ch_printf( ch, "Position set to [%d]%s.\r\n", victim->position, pos_names[victim->position] );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->defposition = victim->position;
               victim->pIndexData->defposition = victim->position;
               send_to_char( "DefPositions set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "pkill" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Player Characters only.\r\n", ch );
               return;
            }

            if( !can_mmodify( ch, victim ) )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }

            xTOGGLE_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
            if( !xIS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
            {
               xSET_BIT( victim->act, PLR_NICE );
               send_to_char( "You're now a NON-PKILL player.\r\n", victim );
               if( ch != victim )
                  send_to_char( "That player is now non-pkill.\r\n", ch );
               if( victim->pcdata->clan && victim->pcdata->clan->clan_type == CLAN_PLAIN && !is_immortal( victim ) )
               {
                  remove_clan_member( victim->pcdata->clan, victim->name );
                  save_clan( victim->pcdata->clan );
                  victim->pcdata->clan = NULL;
               }
            }
            else
            {
               xREMOVE_BIT( victim->act, PLR_NICE );
               send_to_char( "You're now a PKILL player.\r\n", victim );
               if( ch != victim )
                  send_to_char( "That player is now pkill.\r\n", ch );
               if( victim->pcdata->clan && victim->pcdata->clan->clan_type != CLAN_PLAIN && !is_immortal( victim ) )
               {
                  remove_clan_member( victim->pcdata->clan, victim->name );
                  save_clan( victim->pcdata->clan );
                  victim->pcdata->clan = NULL;
               }
            }
            save_char_obj( victim );
            return;
         }
         break;

      case 'Q':
         if( !str_cmp( arg2, "qp" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            victim->pcdata->quest_curr = URANGE( 0, value, 5000 );
            ch_printf( ch, "Qp set to %u.\r\n", victim->pcdata->quest_curr );
            return;
         }
         break;

      case 'R':
         if( !str_cmp( arg2, "race" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Mobiles have no reason to have a race.\r\n", ch );
               return;
            }
            value = get_pc_race( arg3 );
            if( value < 0 && is_number( arg3 ) )
               value = atoi( arg3 );
            if( value < 0 || value >= MAX_PC_RACE )
            {
               ch_printf( ch, "Race range is 0 to %d.\r\n", MAX_PC_RACE - 1 );
               return;
            }
            victim->race = value;
            ch_printf( ch, "Victim's race set to %s.\r\n", dis_race_name( victim->race ) );
            return;
         }

         if( !str_cmp( arg2, "rank" ) )
         {
            if( get_trust( ch ) < PERM_LEADER )
            {
               send_to_char( "You can't do that.\r\n", ch );
               return;
            }
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            STRSET( victim->pcdata->rank, argument );
            send_to_char( "Rank set.\r\n", ch );
            return;
         }

         if( !str_cmp( arg2, "resistant" ) )
         {
            if( !is_npc( victim ) && get_trust( ch ) < PERM_LEADER )
            {
               send_to_char( "You can only modify a mobile's resistancies.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <victim> resistant <flag> <#>\r\n", ch );
               return;
            }
            argument = one_argument( argument, arg3 );
            value = get_flag( arg3, ris_flags, RIS_MAX );
            if( value < 0 || value >= RIS_MAX )
               ch_printf( ch, "Unknown resistant: %s\r\n", arg3 );
            else
            {
               int tempris;

               if( !argument || argument[0] == '\0' || !is_number( argument ) )
               {
                  send_to_char( "Usage: mset <victim> resistant <flag> <#>\r\n", ch );
                  return;
               }

               tempris = atoi( argument );
               victim->resistant[value] = tempris;
               ch_printf( ch, "%s resistant set to %d.\r\n", ris_flags[value], victim->resistant[value] );
               if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
               {
                  victim->pIndexData->resistant[value] = victim->resistant[value];
                  ch_printf( ch, "%s index resistant set to %d.\r\n", ris_flags[value], victim->pIndexData->resistant[value] );
               }
            }
            return;
         }
         break;

      case 'S':
         if( !str_cmp( arg2, "sav1" ) )
         {
            victim->saving_poison_death = URANGE( -30, value, 30 );
            ch_printf( ch, "Sav1 (saving_poison_death) set to %d.\r\n", victim->saving_poison_death );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->saving_poison_death = victim->saving_poison_death;
               send_to_char( "Prototype Sav1 set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "sav2" ) )
         {
            victim->saving_wand = URANGE( -30, value, 30 );
            ch_printf( ch, "Sav2 (saving_wand) set to %d.\r\n", victim->saving_wand );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->saving_wand = victim->saving_wand;
               send_to_char( "Prototype Sav2 set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "sav3" ) )
         {
            victim->saving_para_petri = URANGE( -30, value, 30 );
            ch_printf( ch, "Sav3 (saving_para_petri) set to %d.\r\n", victim->saving_para_petri );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->saving_para_petri = victim->saving_para_petri;
               send_to_char( "Prototype Sav3 set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "sav4" ) )
         {
            victim->saving_breath = URANGE( -30, value, 30 );
            ch_printf( ch, "Sav4 (saving_breath) set to %d.\r\n", victim->saving_breath );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->saving_breath = victim->saving_breath;
               send_to_char( "Prototype Sav4 set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "sav5" ) )
         {
            victim->saving_spell_staff = URANGE( -30, value, 30 );
            ch_printf( ch, "Sav5 (saving_spell_staff) set to %d.\r\n", victim->saving_spell_staff );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->saving_spell_staff = victim->saving_spell_staff;
               send_to_char( "Prototype Sav5 set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "sex" ) )
         {
            value = get_flag( arg3, sex_names, SEX_MAX );
            if( value < 0 && is_number( arg3 ) )
               value = atoi( arg3 );
            if( value < 0 || value >= SEX_MAX )
            {
               send_to_char( "Sex range is 0[neutral], 1[male], or 2[female].\r\n", ch );
               return;
            }
            victim->sex = value;
            ch_printf( ch, "Sex set to %d[%s].\r\n", victim->sex, sex_names[victim->sex] );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->sex = value;
               send_to_char( "Prototype Sex set.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "short" ) )
         {
            if( arg3 == NULL || arg3[0] == '\0' )
            {
               send_to_char( "You can't set the short field to nothing.\r\n", ch );
               return;
            }
            STRSET( victim->short_descr, arg3 );
            send_to_char( "Short set.\r\n", ch );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               STRSET( victim->pIndexData->short_descr, victim->short_descr );
               send_to_char( "Prototype Short set to match.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "spec" ) || !str_cmp( arg2, "spec_fun" ) )
         {
            SPEC_FUN *specfun;

            if( !is_npc( victim ) )
            {
               send_to_char( "Not on PC's.\r\n", ch );
               return;
            }
            if( !str_cmp( arg3, "none" ) )
            {
               victim->spec_fun = NULL;
               STRFREE( victim->spec_funname );
               send_to_char( "Special function removed.\r\n", ch );
               if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
               {
                  victim->pIndexData->spec_fun = NULL;
                  STRFREE( victim->pIndexData->spec_funname );
               }
               return;
            }
            if( !( specfun = spec_lookup( arg3 ) ) )
            {
               send_to_char( "No such function.\r\n", ch );
               return;
            }
            if( !validate_spec_fun( arg3 ) )
            {
               ch_printf( ch, "%s is not a valid spec_fun for mobiles.\r\n", arg3 );
               return;
            }
            victim->spec_fun = specfun;
            STRSET( victim->spec_funname, arg3 );
            send_to_char( "Victim special function set.\r\n", ch );
            if( xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->spec_fun = victim->spec_fun;
               STRSET( victim->pIndexData->spec_funname, arg3 );
               send_to_char( "Prototype Victim special function set.\r\n", ch );
            }
            return;
         }

         if( !str_cmp( arg2, "speaks" ) )
         {
            if( !can_mmodify( ch, victim ) )
               return;
            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <victim> speaks <language> [language] ...\r\n", ch );
               return;
            }
            while( argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );

               value = get_langflag( arg3 );
               if( value == LANG_UNKNOWN )
               {
                  ch_printf( ch, "Unknown language: %s\r\n", arg3 );
                  continue;
               }
               else if( !is_npc( victim ) )
               {
                  if( !( value &= VALID_LANGS ) )
                  {
                     ch_printf( ch, "Players may not know %s.\r\n", arg3 );
                     continue;
                  }
               }

               v2 = get_langnum( arg3 );
               if( v2 == -1 )
                  ch_printf( ch, "Unknown language: %s\r\n", arg3 );
               else
                  xTOGGLE_BIT( victim->speaks, v2 );
            }
            if( xIS_SET( victim->act, ACT_PROTOTYPE ) )
               victim->pIndexData->speaks = victim->speaks;
            send_to_char( "Done.\r\n", ch );
            return;
         }

         if( !str_cmp( arg2, "speaking" ) )
         {
            if( !is_npc( victim ) )
            {
               send_to_char( "Players must choose the language they speak themselves.\r\n", ch );
               return;
            }
            if( !can_mmodify( ch, victim ) )
               return;
            if( !argument || argument[0] == '\0' )
            {
               send_to_char( "Usage: mset <victim> speaking <language> [language]...\r\n", ch );
               return;
            }
            while( argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_langflag( arg3 );
               if( value == LANG_UNKNOWN )
                  ch_printf( ch, "Unknown language: %s\r\n", arg3 );
               else
               {
                  v2 = get_langnum( arg3 );
                  if( v2 == -1 )
                     ch_printf( ch, "Unknown language: %s\r\n", arg3 );
                  else
                     xTOGGLE_BIT( victim->speaking, 1 << v2 );
               }
            }
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
               victim->pIndexData->speaking = victim->speaking;
            send_to_char( "Done.\r\n", ch );
            return;
         }
         break;

      case 'T':
         if( !str_cmp( arg2, "thirst" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            victim->pcdata->condition[COND_THIRST] = URANGE( 0, value, 100 );
            ch_printf( ch, "Thirst set to %d.\r\n", victim->pcdata->condition[COND_THIRST] );
            return;
         }

         if( !str_cmp( arg2, "title" ) )
         {
            if( is_npc( victim ) )
            {
               send_to_char( "Not on NPC's.\r\n", ch );
               return;
            }
            set_title( victim, arg3 );
            return;
         }
         break;

      case 'W':
         if( !str_cmp( arg2, "weight" ) )
         {
            victim->weight = UMAX( 0, value );
            ch_printf( ch, "Weight set to %d.\r\n", victim->weight );
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
            {
               victim->pIndexData->weight = victim->weight;
               send_to_char( "Prototype Weight set to match.\r\n", ch );
            }
            return;
         }
         break;
   }

   /* Generate usage message. */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_mset;
   }
   else
      do_mset( ch, (char *)"" );
}

CMDF( do_oset )
{
   OBJ_DATA *obj, *tmpobj;
   EXTRA_DESCR_DATA *ed;
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MSL], *origarg = argument;
   int value, tmp, stat;
   bool lockobj;

   set_char_color( AT_PLAIN, ch );
   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't oset\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_OBJ_EXTRA:
         if( !valid_destbuf( ch, __FUNCTION__ ) )
            return;
         ed = ( EXTRA_DESCR_DATA *) ch->dest_buf;
         STRFREE( ed->description );
         ed->description = copy_buffer( ch );
         tmpobj = ( OBJ_DATA *) ch->spare_ptr;
         stop_editing( ch );
         ch->dest_buf = tmpobj;
         ch->substate = ch->tempnum;
         return;

      case SUB_OBJ_LONG:
         if( !valid_destbuf( ch, __FUNCTION__ ) )
            return;
         obj = ( OBJ_DATA *) ch->dest_buf;
         if( !obj )
         {
            send_to_char( "Your object was extracted!\r\n", ch );
            stop_editing( ch );
            return;
         }
         STRFREE( obj->description );
         obj->description = copy_buffer( ch );
         if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         {
            if( can_omodify( ch, obj ) )
               STRSET( obj->pIndexData->description, obj->description );
         }
         tmpobj = ( OBJ_DATA *) ch->spare_ptr;
         stop_editing( ch );
         ch->substate = ch->tempnum;
         ch->dest_buf = tmpobj;
         return;

      case SUB_OBJ_DESC:
         if( !valid_destbuf( ch, __FUNCTION__ ) )
            return;
         obj = ( OBJ_DATA *) ch->dest_buf;
         if( !obj )
         {
            send_to_char( "Your object was extracted!\r\n", ch );
            stop_editing( ch );
            return;
         }
         STRFREE( obj->desc );
         obj->desc = copy_buffer( ch );
         if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         {
            if( can_omodify( ch, obj ) )
               STRSET( obj->pIndexData->desc, obj->desc );
         }
         tmpobj = ( OBJ_DATA *) ch->spare_ptr;
         stop_editing( ch );
         ch->substate = ch->tempnum;
         ch->dest_buf = tmpobj;
         return;
   }

   obj = NULL;

   if( ch->substate == SUB_REPEATCMD )
   {
      if( !( obj = ( OBJ_DATA * ) ch->dest_buf ) )
      {
         send_to_char( "Your object was extracted!\r\n", ch );
         argument = (char *)"done";
      }

      if( !argument || argument[0] == '\0' || !str_cmp( argument, "stat" ) )
      {
         if( obj )
            do_ostat( ch, obj->name );
         else
            send_to_char( "No object selected.  Type '?' for help.\r\n", ch );
         return;
      }
      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {

         if( ch->dest_buf )
            RelDestroy( relOSET_ON, ch, ch->dest_buf );
         send_to_char( "Oset mode off.\r\n", ch );
         ch->substate = SUB_NONE;
         ch->dest_buf = NULL;
         if( ch->pcdata && ch->pcdata->subprompt )
         {
            STRFREE( ch->pcdata->subprompt );
            ch->pcdata->subprompt = NULL;
         }
         return;
      }
   }
   if( obj )
   {
      lockobj = true;
      mudstrlcpy( arg1, obj->name, sizeof( arg1 ) );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, sizeof( arg3 ) );
   }
   else
   {
      lockobj = false;
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, sizeof( arg3 ) );
   }

   if( !str_cmp( arg1, "on" ) )
   {
      send_to_char( "Usage: oset <object|vnum> on.\r\n", ch );
      return;
   }

   if( arg1[0] == '\0' || arg2[0] == '\0' || !str_cmp( arg1, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( obj )
            send_to_char( "Usage: <field>  <value>\r\n", ch );
         else
            send_to_char( "Usage: <object> <field>  <value>\r\n", ch );
      }
      else
         send_to_char( "Usage: oset <object> <field>  <value>\r\n", ch );
      send_to_char( "\r\n", ch );
      send_to_char( "Field being one of:\r\n", ch );
      send_to_char( "  ed  v2    v5  long   type  level  affect    rmaffect\r\n", ch );
      send_to_char( "  v0  v3  cost  name   wear  short  layers  actiondesc\r\n", ch );
      send_to_char( "  v1  v4  desc  rmed  flags  timer  weight\r\n", ch );
      send_to_char( "For salves:\r\n", ch );
      send_to_char( "  slevel,  doses,  delay,  spell1,  spell2,  spell3\r\n", ch );
      send_to_char( "For armor:\r\n", ch );
      send_to_char( "  ac,  condition\r\n", ch );
      send_to_char( "For scroll/potion/pill:\r\n", ch );
      send_to_char( "  slevel,  spell1,  spell2,  spell3\r\n", ch );
      send_to_char( "For missileweapons and weapons:\r\n", ch );
      send_to_char( "  weapontype,  condition\r\n", ch );
      send_to_char( "For wands and staves:\r\n", ch );
      send_to_char( "  slevel,  spell,  maxcharges,  charges\r\n", ch );
      send_to_char( "For containers:\r\n", ch );
      send_to_char( "  cflags,  key,  capacity\r\n", ch );
      send_to_char( "For levers and switches:\r\n", ch );
      send_to_char( "  tflags\r\n", ch );
      return;
   }

   if( !obj && get_trust( ch ) < PERM_LEADER )
   {
      if( !( obj = get_obj_here( ch, arg1 ) ) )
      {
         send_to_char( "You can't find that here.\r\n", ch );
         return;
      }
   }
   else if( !obj )
   {
      if( !( obj = get_obj_world( ch, arg1 ) ) )
      {
         send_to_char( "There is nothing like that in all the realms.\r\n", ch );
         return;
      }
   }
   if( lockobj )
      ch->dest_buf = obj;

   separate_obj( obj );
   value = atoi( arg3 );

   if( obj->wear_loc != WEAR_NONE )
   {
      send_to_char( "You can't set an object that is being worn by someone.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "on" ) )
   {
      if( check_subrestricted( ch ) )
         return;
      ch_printf( ch, "Oset mode on. (Editing '%s' vnum %d).\r\n", obj->name, obj->pIndexData->vnum );
      ch->substate = SUB_REPEATCMD;
      ch->dest_buf = obj;
      if( ch->pcdata )
      {
         snprintf( buf, sizeof( buf ), "<&COset &W#%d&w> %%i", obj->pIndexData->vnum );
         STRSET( ch->pcdata->subprompt, buf );
      }
      RelCreate( relOSET_ON, ch, obj );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( arg3 == NULL || arg3[0] == '\0' )
      {
         send_to_char( "You can't set the name of an object to nothing.\r\n", ch );
         return;
      }
      STRSET( obj->name, arg3 );
      send_to_char( "The name has been set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         STRSET( obj->pIndexData->name, obj->name );
         send_to_char( "The prototype name has been set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( arg3 == NULL || arg3[0] == '\0' )
      {
         send_to_char( "You can't set the short field to nothing.\r\n", ch );
         return;
      }
      STRSET( obj->short_descr, arg3 );
      send_to_char( "The short has been set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         STRSET( obj->pIndexData->short_descr, obj->short_descr );
         send_to_char( "The prototype short has been set.\r\n", ch );
      }
      else
      {
         if( str_infix( "rename", obj->name ) )
         {
            snprintf( buf, sizeof( buf ), "%s %s", obj->name, "rename" );
            STRSET( obj->name, buf );
            send_to_char( "Rename has been added to the short.\r\n", ch );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      if( arg3 != NULL && arg3[0] != '\0' )
      {
         if( !can_omodify( ch, obj ) )
            return;
         STRSET( obj->description, arg3 );
         send_to_char( "Long has been set.\r\n", ch );
         if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         {
            STRSET( obj->pIndexData->description, obj->description );
            send_to_char( "Prototype Long has been set.\r\n", ch );
         }
         return;
      }

      if( check_subrestricted( ch ) )
         return;
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      if( lockobj )
         ch->spare_ptr = obj;
      else
         ch->spare_ptr = NULL;
      ch->substate = SUB_OBJ_LONG;
      ch->dest_buf = obj;
      start_editing( ch, obj->description );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      if( arg3 != NULL && arg3[0] != '\0' )
      {
         if( !can_omodify( ch, obj ) )
            return;
         STRSET( obj->desc, arg3 );
         send_to_char( "Desc has been set.\r\n", ch );
         if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         {
            STRSET( obj->pIndexData->desc, obj->desc );
            send_to_char( "Prototype Desc has been set.\r\n", ch );
         }
         return;
      }

      if( check_subrestricted( ch ) )
         return;
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      if( lockobj )
         ch->spare_ptr = obj;
      else
         ch->spare_ptr = NULL;
      ch->substate = SUB_OBJ_DESC;
      ch->dest_buf = obj;
      start_editing( ch, obj->desc );
      return;
   }

   if( !str_cmp( arg2, "ed" ) )
   {
      if( arg3 == NULL || arg3[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> ed <keywords>\r\n", ch );
         return;
      }
      if( check_subrestricted( ch ) )
         return;
      if( obj->timer )
      {
         send_to_char( "It's not safe to edit an extra description on an object with a timer.\r\nTurn it off first.\r\n", ch );
         return;
      }
      if( obj->item_type == ITEM_PAPER && get_trust( ch ) < PERM_IMP )
      {
         send_to_char( "You can't add an extra description to a note paper at the moment.\r\n", ch );
         return;
      }
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         ed = SetOExtraProto( obj->pIndexData, arg3 );
      else
         ed = SetOExtra( obj, arg3 );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      if( lockobj )
         ch->spare_ptr = obj;
      else
         ch->spare_ptr = NULL;
      ch->substate = SUB_OBJ_EXTRA;
      ch->dest_buf = ed;
      start_editing( ch, ed->description );
      return;
   }

   if( !str_cmp( arg2, "rmed" ) )
   {
      if( arg3 == NULL || arg3[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> rmed <keywords>\r\n", ch );
         return;
      }
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         if( DelOExtraProto( obj->pIndexData, arg3 ) )
            send_to_char( "Deleted.\r\n", ch );
         else
            send_to_char( "Not found.\r\n", ch );
         return;
      }
      if( DelOExtra( obj, arg3 ) )
         send_to_char( "Deleted.\r\n", ch );
      else
         send_to_char( "Not found.\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_BUILDER )
   {
      send_to_char( "You can only oset the name, short and long right now.\r\n", ch );
      return;
   }

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( !str_cmp( arg2, stattypes[stat] ) )
      {
         if( !can_omodify( ch, obj ) )
            return;
         obj->stat_reqs[stat] = URANGE( 0, value, 1000 );
         ch_printf( ch, "%s set to %d\r\n", capitalize( stattypes[stat] ), obj->stat_reqs[stat] );
         if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         {
            obj->pIndexData->stat_reqs[stat] = obj->stat_reqs[stat];
            send_to_char( "Index set too.\r\n", ch );
         }
         return;
      }
   }

   if( !str_cmp( arg2, "classes" ) )
   {
      bool modified = false;

      if( !can_omodify( ch, obj ) )
         return;

      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         stat = get_pc_class( arg1 );
         if( stat < 0 || stat >= MAX_PC_CLASS )
            ch_printf( ch, "Invalid class [%s].\r\n", arg1 );
         else
         {
            xTOGGLE_BIT( obj->class_restrict, stat );
            modified = true;
         }
      }
      if( modified )
      {
         send_to_char( "Classes has been set.\r\n", ch );
         if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         {
            obj->pIndexData->class_restrict = obj->class_restrict;
            send_to_char( "Index Set too.\r\n", ch );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "races" ) )
   {
      bool modified = false;

      if( !can_omodify( ch, obj ) )
         return;

      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         stat = get_pc_race( arg1 );
         if( stat < 0 || stat >= MAX_PC_RACE )
            ch_printf( ch, "Invalid race [%s].\r\n", arg1 );
         else
         {
            xTOGGLE_BIT( obj->race_restrict, stat );
            modified = true;
         }
      }
      if( modified )
      {
         send_to_char( "Races has been set.\r\n", ch );
         if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         {
            obj->pIndexData->race_restrict = obj->race_restrict;
            send_to_char( "Index Set too.\r\n", ch );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->value[0] = value;
      send_to_char( "Value0 set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[0] = value;
         send_to_char( "Prototype Value0 set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->value[1] = value;
      send_to_char( "Value1 set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[1] = value;
         send_to_char( "Prototype Value1 set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->value[2] = value;
      send_to_char( "Value2 set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[2] = value;
         send_to_char( "Prototype Value2 set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->value[3] = value;
      send_to_char( "Value3 set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[3] = value;
         send_to_char( "Prototype Value3 set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->value[4] = value;
      send_to_char( "Value4 set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[4] = value;
         send_to_char( "Prototype Value4 set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->value[5] = value;
      send_to_char( "Value5 set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[5] = value;
         send_to_char( "Prototype Value5 set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> type <type>\r\n", ch );
         return;
      }
      if( is_number( argument ) )
         value = atoi( argument );
      else
         value = get_flag( argument, o_types, ITEM_TYPE_MAX );
      if( value < 1 || value >= ITEM_TYPE_MAX )
      {
         ch_printf( ch, "Unknown type: %s\r\n", argument );
         return;
      }
      obj->item_type = ( short )value;
      ch_printf( ch, "Type set to %s.\r\n", o_types[obj->item_type] );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->item_type = obj->item_type;
         ch_printf( ch, "Prototype Type set to %s.\r\n", o_types[obj->item_type] );
      }
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> flags <flag> [flag]...\r\n", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, o_flags, ITEM_MAX );
         if( value < 0 || value >= ITEM_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
         else
         {
            if( value == ITEM_PROTOTYPE
            && get_trust( ch ) < PERM_HEAD && !is_name( "protoflag", ch->pcdata->bestowments ) )
               send_to_char( "You can't change the prototype flag.\r\n", ch );
            else
            {
               xTOGGLE_BIT( obj->extra_flags, value );
               ch_printf( ch, "%s flag %s.\r\n", o_flags[value], xIS_SET( obj->extra_flags, value ) ? "set" : "removed" );
               if( xIS_SET( obj->extra_flags, ITEM_PROTOTYPE ) || value == ITEM_PROTOTYPE )
               {
                  xTOGGLE_BIT( obj->pIndexData->extra_flags, value );
                  ch_printf( ch, "%s index flag %s.\r\n", o_flags[value], xIS_SET( obj->pIndexData->extra_flags, value ) ? "set" : "removed" );
               }
            }
         }
      }
      return;
   }

   if( !str_cmp( arg2, "wear" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> wear <flag> [flag]...\r\n", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, w_flags, ITEM_WEAR_MAX );
         if( value < 0 || value >= ITEM_WEAR_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
         else
         {
            xTOGGLE_BIT( obj->wear_flags, value );
            ch_printf( ch, "%s wear flag %s.\r\n", w_flags[value], xIS_SET( obj->wear_flags, value ) ? "set" : "removed" );
            if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
            {
               xTOGGLE_BIT( obj->pIndexData->wear_flags, value );
               ch_printf( ch, "%s index wear flag %s.\r\n", w_flags[value], xIS_SET( obj->pIndexData->wear_flags, value ) ? "set" : "removed" );
            }
         }
      }
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->level = URANGE( 0, value, MAX_LEVEL );
      send_to_char( "Level set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->level = obj->level;
         send_to_char( "Prototype Level set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( obj->carried_by )
         obj->carried_by->carry_weight -= get_obj_weight( obj );
      obj->weight = UMAX( 0, value );
      if( obj->carried_by )
         obj->carried_by->carry_weight += get_obj_weight( obj );
      send_to_char( "Weight set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->weight = obj->weight;
         send_to_char( "Prototype Weight set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "cost" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->cost = UMAX( 0, value );
      send_to_char( "Cost set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->cost = obj->cost;
         send_to_char( "Prototoype Cost set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "layers" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->layers = UMAX( 0, value );
         send_to_char( "Prototype layers set.\r\n", ch );
      }
      else
         send_to_char( "Item must have prototype flag to set this value.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "timer" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->timer = value;
      send_to_char( "Timer set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
         send_to_char( "Just FYI prototype doesn't have a timer to set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "actiondesc" ) )
   {
      if( !can_omodify( ch, obj ) )
         return;
      if( strstr( arg3, "%n" ) || strstr( arg3, "%d" ) || strstr( arg3, "%l" ) )
      {
         send_to_char( "Illegal characters!\r\n", ch );
         return;
      }
      STRSET( obj->action_desc, arg3 );
      send_to_char( "Actiondesc set.\r\n", ch );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         STRSET( obj->pIndexData->action_desc, obj->action_desc );
         send_to_char( "Prototype Actiondesc set.\r\n", ch );
      }
      return;
   }

   /* Crash fix and name support by Shaddai */
   if( !str_cmp( arg2, "affect" ) )
   {
      AFFECT_DATA *paf;
      short loc;
      int bitv = -1;

      if( !can_omodify( ch, obj ) )
         return;
      argument = one_argument( argument, arg2 );
      if( arg2 == NULL || arg2[0] == '\0' || !argument || argument[0] == 0 )
      {
         send_to_char( "Usage: oset <object> affect <field> <value>\r\n", ch );
         return;
      }
      loc = get_flag( arg2, a_types, APPLY_MAX );
      if( loc < 1 || loc >= APPLY_MAX )
      {
         ch_printf( ch, "Unknown field: %s\r\n", arg2 );
         return;
      }

      argument = one_argument( argument, arg3 );
      if( loc == APPLY_EXT_AFFECT )
      {
         value = get_flag( arg3, a_flags, AFF_MAX );
         if( value < 0 || value >= AFF_MAX )
         {
            ch_printf( ch, "Unknown affected: %s\r\n", arg3 );
            return;
         }
      }
      else if( loc == APPLY_STAT )
      {
         bitv = get_flag( arg3, stattypes, STAT_MAX );
         if( bitv < 0 || bitv >= STAT_MAX )
         {
            ch_printf( ch, "Unknown stat: %s\r\n", arg3 );
            return;
         }
         argument = one_argument( argument, arg3 );
         value = atoi( arg3 );
      }
      else if( loc == APPLY_RESISTANT )
      {
         bitv = get_flag( arg3, ris_flags, RIS_MAX );
         if( bitv < 0 || bitv >= RIS_MAX )
         {
            ch_printf( ch, "Unknown ris: %s\r\n", arg3 );
            return;
         }
         argument = one_argument( argument, arg3 );
         value = atoi( arg3 );
      }
      else
      {
         if( ( loc == APPLY_WEARSPELL || loc == APPLY_REMOVESPELL || loc == APPLY_STRIPSN || loc == APPLY_WEAPONSPELL )
         && !is_number( arg3 ) )
         {
            value = bsearch_skill_exact( arg3, gsn_first_spell, gsn_first_skill - 1 );
            if( value == -1 )
            {
               ch_printf( ch, "Unknown spell name: %s\r\n", arg3 );
               return;
            }
         }
         else
            value = atoi( arg3 );
      }
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = loc;
      paf->modifier = value;
      xCLEAR_BITS( paf->bitvector );
      if( loc == APPLY_STAT || loc == APPLY_RESISTANT )
         xSET_BIT( paf->bitvector, bitv );
      paf->next = NULL;
      paf->enchantment = false;

      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         if( loc != APPLY_WEARSPELL && loc != APPLY_REMOVESPELL && loc != APPLY_STRIPSN && loc != APPLY_WEAPONSPELL )
         {
            CHAR_DATA *vch;
            OBJ_DATA *eq;

            for( vch = first_char; vch; vch = vch->next )
               for( eq = vch->first_carrying; eq; eq = eq->next_content )
                  if( eq->pIndexData == obj->pIndexData && eq->wear_loc != WEAR_NONE )
                     affect_modify( vch, paf, true );
         }
         LINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
         send_to_char( "Prototype Affect added.\r\n", ch );
      }
      else
      {
         LINK( paf, obj->first_affect, obj->last_affect, next, prev );
         send_to_char( "Extra Affect added.\r\n", ch );
      }
      ++top_affect;
      return;
   }

   if( !str_cmp( arg2, "rmaffect" ) )
   {
      AFFECT_DATA *paf;
      OBJ_INDEX_DATA *pObjIndex;
      short loc, count;

      if( !can_omodify( ch, obj ) )
         return;
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: oset <object> rmaffect <affect#>\r\n", ch );
         return;
      }
      loc = atoi( argument );
      if( loc < 1 )
      {
         send_to_char( "Invalid number.\r\n", ch );
         return;
      }

      count = 0;

      pObjIndex = obj->pIndexData;
      for( paf = pObjIndex->first_affect; paf; paf = paf->next )
      {
         if( ++count == loc )
         {
            if( !is_obj_stat( obj, ITEM_PROTOTYPE ) )
            {
               send_to_char( "That is an affect on the object index and requires the object to be prototype to remove the affect.\r\n", ch );
               return;
            }
            loc = paf->location;
            if( loc != APPLY_WEARSPELL && loc != APPLY_REMOVESPELL && loc != APPLY_STRIPSN && loc != APPLY_WEAPONSPELL )
            {
               CHAR_DATA *vch;
               OBJ_DATA *eq;

               for( vch = first_char; vch; vch = vch->next )
                  for( eq = vch->first_carrying; eq; eq = eq->next_content )
                     if( eq->pIndexData == pObjIndex && eq->wear_loc != WEAR_NONE )
                        affect_modify( vch, paf, false );
            }
            UNLINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
            DISPOSE( paf );
            send_to_char( "Removed from the prototype affects.\r\n", ch );
            --top_affect;
            return;
         }
      }

      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ++count == loc )
         {
            UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
            DISPOSE( paf );
            send_to_char( "Removed from the extra affects.\r\n", ch );
            --top_affect;
            return;
         }
      }

      send_to_char( "No such affect found in the prototype or extra affects to remove.\r\n", ch );
      return;
   }

   /* Make it easier to set special object values by name than number  -Thoric */
   tmp = -1;
   switch( obj->item_type )
   {
      case ITEM_MISSILE_WEAPON:
      case ITEM_WEAPON:
         if( !str_cmp( arg2, "weapontype" ) )
         {
            unsigned int x;

            value = -1;
            for( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               send_to_char( "Unknown weapon type.\r\n", ch );
               return;
            }
            tmp = 3;
            break;
         }
         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         break;

      case ITEM_ARMOR:
         if( !str_cmp( arg2, "ac" ) )
            tmp = 0;
         if( !str_cmp( arg2, "condition" ) )
            tmp = 1;
         break;

      case ITEM_SALVE:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "doses" ) )
            tmp = 1;
         if( !str_cmp( arg2, "delay" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 3;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 4;
         if( !str_cmp( arg2, "spell3" ) )
            tmp = 5;
         if( tmp >= 3 && tmp <= 5 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 1;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell3" ) )
            tmp = 3;
         if( tmp >= 1 && tmp <= 3 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "maxcharges" ) )
            tmp = 1;
         if( !str_cmp( arg2, "charges" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell" ) )
         {
            tmp = 3;
            value = skill_lookup( arg3 );
         }
         break;

      case ITEM_CONTAINER:
         if( !str_cmp( arg2, "capacity" ) )
            tmp = 0;
         if( !str_cmp( arg2, "cflags" ) )
         {
            int tmpval = 0;

            tmp = 1;
            argument = arg3;
            while( argument && argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_contflag( arg3 );
               if( value < 0 || value > 31 )
                  ch_printf( ch, "Invalid container flag %s\r\n", arg3 );
               else
                  tmpval += ( 1 << value );
            }
            value = tmpval;
         }
         if( !str_cmp( arg2, "key" ) )
            tmp = 2;
         break;

      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         if( !str_cmp( arg2, "tflags" ) )
         {
            int tmpval = 0;

            tmp = 0;
            argument = arg3;
            while( argument && argument[0] != '\0' )
            {
               argument = one_argument( argument, arg3 );
               value = get_trigflag( arg3 );
               if( value < 0 || value > 31 )
                  ch_printf( ch, "Invalid tflag %s\r\n", arg3 );
               else
                  tmpval += ( 1 << value );
            }
            value = tmpval;
         }
         break;
   }
   if( tmp >= 0 && tmp <= 5 )
   {
      if( !can_omodify( ch, obj ) )
         return;
      obj->value[tmp] = value;
      ch_printf( ch, "%s set.\r\n", arg2 );
      if( is_obj_stat( obj, ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[tmp] = value;
         ch_printf( ch, "Prototype %s set.\r\n", arg2 );
      }
      return;
   }

   /* Generate usage message. */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_oset;
   }
   else
      do_oset( ch, (char *)"" );
}

/* Returns value 0 - 9 based on directional text. -1 if Invalid */
int get_dir( char *txt )
{
   char c1, c2;

   c1 = txt[0];
   if( c1 == '\0' )
      return 0;
   c2 = txt[1];

   if( !str_cmp( txt, "northeast" ) || ( c1 == 'n' && c2 == 'e' ) || c1 == '6' )
      return DIR_NORTHEAST;
   if( !str_cmp( txt, "northwest" ) || ( c1 == 'n' && c2 == 'w' ) || c1 == '7' )
      return DIR_NORTHWEST;
   if( !str_cmp( txt, "southeast" ) || ( c1 == 's' && c2 == 'e' ) || c1 == '8' )
      return DIR_SOUTHEAST;
   if( !str_cmp( txt, "southwest" ) || ( c1 == 's' && c2 == 'w' ) || c1 == '9' )
      return DIR_SOUTHWEST;
   if( !str_cmp( txt, "somewhere" ) || c1 == '?' || ( c1 == '1' && c2 == '0' ) )
      return DIR_SOMEWHERE;

   if( c1 == 'n' || c1 == 'N' || c1 == '0' )
      return DIR_NORTH;
   if( c1 == 'e' || c1 == 'E' || c1 == '1' )
      return DIR_EAST;
   if( c1 == 's' || c1 == 'S' || c1 == '2' )
      return DIR_SOUTH;
   if( c1 == 'w' || c1 == 'W' || c1 == '3' )
      return DIR_WEST;
   if( c1 == 'u' || c1 == 'U' || c1 == '4' )
      return DIR_UP;
   if( c1 == 'd' || c1 == 'D' || c1 == '5' )
      return DIR_DOWN;

   return -1; /* If we get here return -1 */
}

CMDF( do_redit )
{
   ROOM_INDEX_DATA *location, *tmp;
   EXTRA_DESCR_DATA *ed;
   EXIT_DATA *xit, *texit;
   static const char *dir_text[] = { "North", "East", "South", "West", "Up", "Down", "NorthEast", "NorthWest", "SouthEast", "SouthWest", "Somewhere" };
   char arg[MIL], arg2[MIL], arg3[MIL], buf[MSL], *origarg = argument;
   int value, edir = 0, ekey, evnum;

   set_char_color( AT_PLAIN, ch );
   if( !ch->desc )
   {
      send_to_char( "You have no descriptor.\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;
      case SUB_ROOM_DESC:
         location = ( ROOM_INDEX_DATA *) ch->dest_buf;
         if( !location )
         {
            bug( "%s: sub_room_desc: NULL ch->dest_buf", __FUNCTION__ );
            location = ch->in_room;
         }
         STRFREE( location->description );
         location->description = copy_buffer( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         return;

      case SUB_ROOM_EXTRA:
         ed = ( EXTRA_DESCR_DATA *) ch->dest_buf;
         if( !ed )
         {
            bug( "%s: sub_room_extra: NULL ch->dest_buf", __FUNCTION__ );
            stop_editing( ch );
            return;
         }
         STRFREE( ed->description );
         ed->description = copy_buffer( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         return;
   }

   location = ch->in_room;

   argument = one_argument( argument, arg );
   if( ch->substate == SUB_REPEATCMD )
   {
      if( arg == NULL || arg[0] == '\0' )
      {
         do_rstat( ch, (char *)"" );
         return;
      }
      if( !str_cmp( arg, "done" ) || !str_cmp( arg, "off" ) )
      {
         send_to_char( "Redit mode off.\r\n", ch );
         if( ch->pcdata && ch->pcdata->subprompt )
         {
            STRFREE( ch->pcdata->subprompt );
            ch->pcdata->subprompt = NULL;
         }
         ch->substate = SUB_NONE;
         return;
      }
   }
   if( arg[0] == '\0' || !str_cmp( arg, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         send_to_char( "Usage: <field> value\r\n", ch );
      else
         send_to_char( "Usage: redit <field> value\r\n", ch );
      send_to_char( "\r\n", ch );
      send_to_char( "Field being one of:\r\n", ch );
      send_to_char( "  ed    rmed   exkey   exflags  televnum\r\n", ch );
      send_to_char( "  exit  pull   bexit   exname   pulltype\r\n", ch );
      send_to_char( "  name  push   flags   sector   teledelay\r\n", ch );
      send_to_char( "  desc  rlist  exdesc  tunnel\r\n", ch );
      return;
   }

   if( !can_rmodify( ch, location ) )
      return;

   if( !str_cmp( arg, "on" ) )
   {
      if( check_subrestricted( ch ) )
         return;
      send_to_char( "Redit mode on.\r\n", ch );
      ch->substate = SUB_REPEATCMD;
      if( ch->pcdata )
         STRSET( ch->pcdata->subprompt, "<&CRedit &W#%r&w> %i" );
      return;
   }

   if( !str_cmp( arg, "name" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Set the room name.  A very brief single line room description.\r\n", ch );
         send_to_char( "Usage: redit name <Room summary>\r\n", ch );
         return;
      }
      STRSET( location->name, argument );
      send_to_char( "Name set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "desc" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_DESC;
      ch->dest_buf = location;
      start_editing( ch, location->description );
      return;
   }

   if( !str_cmp( arg, "tunnel" ) )
   {
      location->tunnel = URANGE( 0, atoi( argument ), 1000 );
      ch_printf( ch, "Tunnel set to %d.\r\n", location->tunnel );
      return;
   }

   if( !str_cmp( arg, "ed" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Create an extra description.\r\n", ch );
         send_to_char( "You must supply keyword(s).\r\n", ch );
         return;
      }
      if( check_subrestricted( ch ) )
         return;
      ed = SetRExtra( location, argument );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_EXTRA;
      ch->dest_buf = ed;
      start_editing( ch, ed->description );
      return;
   }

   if( !str_cmp( arg, "rmed" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Remove an extra description.\r\n", ch );
         send_to_char( "You must supply keyword(s).\r\n", ch );
         return;
      }
      if( DelRExtra( location, argument ) )
         send_to_char( "Deleted.\r\n", ch );
      else
         send_to_char( "Not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "rlist" ) )
   {
      RESET_DATA *pReset;
      char *rbuf;
      short num;

      if( !location->first_reset )
      {
         send_to_char( "This room has no resets to list.\r\n", ch );
         return;
      }
      num = 0;
      for( pReset = location->first_reset; pReset; pReset = pReset->next )
      {
         num++;
         if( !( rbuf = sprint_reset( pReset, &num ) ) )
            continue;
         send_to_char( rbuf, ch );
      }
      return;
   }

   if( !str_cmp( arg, "flags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Toggle the room flags.\r\n", ch );
         send_to_char( "Usage: redit flags <flag> [flag]...\r\n", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg2 );
         value = get_flag( arg2, r_flags, ROOM_MAX );
         if( value < 0 || value >= ROOM_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg2 );
         else
         {
            xTOGGLE_BIT( location->room_flags, value );
            /* Stopping it from being a storage room */
            if( value == ROOM_STORAGEROOM && !xIS_SET( location->room_flags, value ) )
            {
               char storage[MSL];

               snprintf( storage, sizeof( storage ), "%s%d", STORAGE_DIR, location->vnum );
               if( !remove( storage ) )
                  send_to_char( "Removed the storage file for the room.\r\n", ch );
            }
            if( value == ROOM_EXPLORER )
            {
               if( xIS_SET( location->room_flags, ROOM_EXPLORER ) )
                  top_explorer++;
               else
                  top_explorer--;
            }
            ch_printf( ch, "Flag [%d]%s has been %s.\r\n", value, arg2, xIS_SET( location->room_flags, value ) ? "set" : "removed" );
         }
      }
      return;
   }

   if( !str_cmp( arg, "teledelay" ) )
   {
      location->tele_delay = UMAX( 0, atoi( argument ) );
      ch_printf( ch, "Teledelay set to %d.\r\n", location->tele_delay );
      return;
   }

   if( !str_cmp( arg, "televnum" ) )
   {
      value = URANGE( 0, atoi( argument ), MAX_VNUM );
      if( value != 0 && !get_room_index( value ) )
      {
         send_to_char( "Not a valid room vnum.\r\n", ch );
         return;
      }
      location->tele_vnum = value;
      ch_printf( ch, "Televnum set to %d.\r\n", location->tele_vnum );
      return;
   }

   if( !str_cmp( arg, "sector" ) )
   {
      value = get_flag( argument, sect_flags, SECT_MAX );
      if( value < 0 && is_number( argument ) )
         value = atoi( argument );
      if( value < 0 || value >= SECT_MAX )
         ch_printf( ch, "No such sector (%s).\r\n", argument );
      else
      {
         location->sector_type = value;
         ch_printf( ch, "Sector set to %d[%s].\r\n", location->sector_type, sect_flags[ch->in_room->sector_type] );
      }
      return;
   }

   if( !str_cmp( arg, "exkey" ) )
   {
      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2[0] == '\0' || arg3[0] == '\0' )
      {
         send_to_char( "Usage: redit exkey <dir> <key vnum>\r\n", ch );
         return;
      }
      edir = get_dir( arg2 );
      value = atoi( arg3 );
      if( !get_obj_index( value ) )
      {
         send_to_char( "No such key to use exist.\r\n", ch );
         return;
      }
      if( !( xit = get_exit( location, edir ) ) )
      {
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
         return;
      }
      xit->key = value;
      send_to_char( "Exkey Set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "exname" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Change or clear exit keywords.\r\n", ch );
         send_to_char( "Usage: redit exname <dir> [keywords]\r\n", ch );
         return;
      }
      edir = get_dir( arg2 );
      if( !( xit = get_exit( location, edir ) ) )
      {
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
         return;
      }
      STRSET( xit->keyword, argument );
      send_to_char( "Exname set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "exflags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Toggle or display exit flags.\r\n", ch );
         send_to_char( "Usage: redit exflags <dir> <flag> [flag]...\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg2 );
      edir = get_dir( arg2 );
      if( !( xit = get_exit( location, edir ) ) )
      {
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         snprintf( buf, sizeof( buf ), "Flags for exit direction: [ %s ] Keywords: [ %s ] Key: %d\r\n[%s]\r\n",
            dir_text[xit->vdir], xit->keyword ? xit->keyword : "(Not Set)", xit->key, ext_flag_string( &xit->exit_info, ex_flags ) );
         if( !xSAME_BITS( xit->exit_info, xit->base_info ) )
         {
            mudstrlcat( buf, "[", sizeof( buf ) );
            mudstrlcat( buf, ext_flag_string( &xit->base_info, ex_flags ), sizeof( buf ) );
            mudstrlcat( buf, "]\r\n", sizeof( buf ) );
         }
         send_to_char( buf, ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg2 );
         value = get_flag( arg2, ex_flags, EX_MAX );
         if( value < 0 || value >= EX_MAX )
            ch_printf( ch, "Unknown ex_flag: %s\r\n", arg2 );
         else
         {
            xTOGGLE_BIT( xit->exit_info, value );
            xit->base_info = xit->exit_info;
            ch_printf( ch, "%s exflag %s.\r\n", ex_flags[value], xIS_SET( xit->exit_info, value ) ? "set" : "removed" );
         }
      }
      return;
   }

   if( !str_cmp( arg, "exit" ) )
   {
      bool changed = false, showmessg = false;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Create, change or remove an exit.\r\n", ch );
         send_to_char( "Usage: redit exit <dir> [room] [key] [keywords] [flags]\r\n", ch );
         return;
      }

      edir = get_dir( arg2 );

      if( arg3 == NULL || arg3[0] == '\0' )
         evnum = 0;
      else
         evnum = atoi( arg3 );

      xit = get_exit( location, edir );

      if( !evnum )
      {
         if( xit )
         {
            ch_printf( ch, "Removed exit %s in room %d that went to room %d.\r\n", dir_text[xit->vdir], xit->rvnum, xit->vnum );
            extract_exit( location, xit );
            return;
         }
         send_to_char( "No exit in that direction.\r\n", ch );
         return;
      }
      if( evnum < 1 || evnum > MAX_VNUM )
      {
         send_to_char( "Invalid room number.\r\n", ch );
         return;
      }
      if( !( tmp = get_room_index( evnum ) ) )
      {
         send_to_char( "Non-existant room.\r\n", ch );
         return;
      }

      if( !xit )
      {
         if( xit && get_exit_to( location, edir, tmp->vnum ) )
         {
            ch_printf( ch, "There is already an exit %s in room %d to %d.\r\n", dir_text[edir], xit->rvnum, xit->vnum );
            return;
         }

         if( !( xit = make_exit( location, tmp, edir ) ) )
         {
            send_to_char( "Failed to make_exit.\r\n", ch );
            return;
         }
         xit->keyword = NULL;
         xit->description = NULL;
         xit->key = -1;
         xCLEAR_BITS( xit->exit_info );
         xCLEAR_BITS( xit->base_info );
         act( AT_IMMORT, "$n reveals a hidden passage!", ch, NULL, NULL, TO_ROOM );
      }
      else
      {
         act( AT_IMMORT, "Something is different...", ch, NULL, NULL, TO_ROOM );
         changed = true;
      }

      if( xit->to_room != tmp )
      {
         ch_printf( ch, "Changed exit %s in room %d from room %d to %d.\r\n", dir_text[edir], xit->rvnum, xit->vnum, evnum );
         changed = true;
         xit->to_room = tmp;
         xit->vnum = evnum;
         if( ( texit = get_exit_to( xit->to_room, rev_dir[edir], location->vnum ) ) )
         {
            texit->rexit = xit;
            xit->rexit = texit;
         }
      }

      /* Set the key */
      argument = one_argument( argument, arg3 );
      if( arg3 != NULL && arg3[0] != '\0' )
      {
         if( is_number( arg3 ) )
         {
            ekey = atoi( arg3 );
            if( ekey != 0 )
            {
               if( !get_obj_index( ekey ) )
                  ch_printf( ch, "%d isn't a valid object so can't be set as a key.\r\n", ekey );
               else
               {
                  if( xit->key != ekey )
                     showmessg = true;
                  xit->key = ekey;
               }
            }
         }
      }

      /* Set the keywords */
      argument = one_argument( argument, arg3 );
      if( arg3 != NULL && arg3[0] != '\0' )
      {
         if( xit->keyword && xit->keyword[0] != '\0' && str_cmp( xit->keyword, arg3 ) )
            showmessg = true;
         STRSET( xit->keyword, arg3 );
      }

      /* Set flags */
      if( argument && argument[0] != '\0' )
      {
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg3 );
            value = get_flag( arg3, ex_flags, EX_MAX );
            if( value < 0 || value >= EX_MAX )
               ch_printf( ch, "Unknown ex_flag (%s).\r\n", arg3 );
            else
            {
               if( !xIS_SET( xit->exit_info, value ) )
                  showmessg = true;
               xSET_BIT( xit->exit_info, value );
               xSET_BIT( xit->base_info, value );
            }
         }
      }

      if( !changed )
         ch_printf( ch, "Added exit %s from room %d to %d.\r\n", dir_text[edir], xit->rvnum, xit->vnum );
      else if( showmessg )
         ch_printf( ch, "Modified exit %s from room %d to %d.\r\n", dir_text[edir], xit->rvnum, xit->vnum );
      else if( xit )
      {
         ch_printf( ch, "Removed exit %s in room %d that went to room %d.\r\n", dir_text[xit->vdir], xit->rvnum, xit->vnum );
         extract_exit( location, xit );
      }
      return;
   }

   if( !str_cmp( arg, "bexit" ) )
   {
      EXIT_DATA *nxit = NULL, *rxit = NULL;
      char tmpcmd[MIL];
      int vnum = 0, ovnum = 0;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Create, change or remove a two-way exit.\r\n", ch );
         send_to_char( "Usage: redit bexit <dir> [room] [key] [keywords] [flags]\r\n", ch );
         return;
      }

      edir = get_dir( arg2 );
      if( ( nxit = get_exit( location, edir ) ) )
      {
         ovnum = nxit->vnum;
         vnum = nxit->vnum;
         if( nxit->to_room )
            rxit = get_exit( nxit->to_room, rev_dir[edir] );
         else
            rxit = NULL;
      }

      snprintf( tmpcmd, sizeof( tmpcmd ), "exit %s %s %s", arg2, arg3, argument );
      do_redit( ch, tmpcmd );

      if( ( nxit = get_exit( location, edir ) ) )
      {
         vnum = nxit->vnum;
         if( !rxit || vnum != ovnum )
         {
            if( nxit->to_room )
               rxit = get_exit( nxit->to_room, rev_dir[edir] );
            else
               rxit = NULL;
         }
      }

      if( vnum )
      {
         snprintf( tmpcmd, sizeof( tmpcmd ), "%d redit exit %d %d %s", vnum, rev_dir[edir], location->vnum, argument );
         do_at( ch, tmpcmd );
      }

      if( ovnum != 0 && ovnum != vnum )
      {
         snprintf( tmpcmd, sizeof( tmpcmd ), "%d redit exit %d", ovnum, rev_dir[edir] );
         do_at( ch, tmpcmd );
      }
      return;
   }

   if( !str_cmp( arg, "pulltype" ) || !str_cmp( arg, "pushtype" ) )
   {
      int pt;

      argument = one_argument( argument, arg2 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         ch_printf( ch, "Set the %s between this room, and the destination room.\r\n", arg );
         ch_printf( ch, "Usage: redit %s <dir> <type>\r\n", arg );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         if( ( pt = get_pulltype( argument ) ) == -1 )
            ch_printf( ch, "Unknown pulltype: %s.  (See help PULLTYPES)\r\n", argument );
         else
         {
            xit->pulltype = pt;
            send_to_char( "Done.\r\n", ch );
         }
      }
      else
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "pull" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Set the 'pull' between this room, and the destination room.\r\n", ch );
         send_to_char( "Usage: redit pull <dir> <force (0 to 100)>\r\n", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         xit->pull = URANGE( -100, atoi( argument ), 100 );
         send_to_char( "Done.\r\n", ch );
      }
      else
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "push" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Set the 'push' away from the destination room in the opposite direction.\r\n", ch );
         send_to_char( "Usage: redit push <dir> <force (0 to 100)>\r\n", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         xit->pull = URANGE( -100, -( atoi( argument ) ), 100 );
         send_to_char( "Done.\r\n", ch );
      }
      else
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "exdesc" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Create or clear a description for an exit.\r\n", ch );
         send_to_char( "Usage: redit exdesc <dir> [description]\r\n", ch );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2 + 1 );
         xit = get_exit_num( location, edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = get_exit( location, edir );
      }
      if( xit )
      {
         STRFREE( xit->description );
         if( argument && argument[0] != '\0' )
         {
            snprintf( buf, sizeof( buf ), "%s\r\n", argument );
            xit->description = STRALLOC( buf );
         }
         send_to_char( "Done.\r\n", ch );
      }
      else
         send_to_char( "No exit in that direction.  Use 'redit exit ...' first.\r\n", ch );
      return;
   }

   /* Generate usage message. */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_redit;
   }
   else
      do_redit( ch, (char *)"" );
}

CMDF( do_ocreate )
{
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   char arg[MIL], arg2[MIL];
   int vnum, cvnum;

   if( is_npc( ch ) )
   {
      send_to_char( "Mobiles can't create.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   vnum = is_number( arg ) ? atoi( arg ) : -1;

   if( vnum == -1 || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: ocreate <vnum> [copy vnum] <item name>\r\n", ch );
      return;
   }

   if( vnum < 1 || vnum > MAX_VNUM )
   {
      send_to_char( "Vnum out of range.\r\n", ch );
      return;
   }

   one_argument( argument, arg2 );
   cvnum = atoi( arg2 );
   if( cvnum != 0 )
      argument = one_argument( argument, arg2 );
   if( cvnum < 1 )
      cvnum = 0;

   if( get_obj_index( vnum ) )
   {
      send_to_char( "An object with that number already exists.\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_LEADER )
   {
      AREA_DATA *pArea;

      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to create objects.\r\n", ch );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\r\n", ch );
         return;
      }
   }

   if( !( pObjIndex = make_object( vnum, cvnum, argument ) ) )
   {
      send_to_char( "Couldn't make_object.\r\n", ch );
      log_printf( "%s: make_object failed.", __FUNCTION__ );
      return;
   }
   if( !( obj = create_object( pObjIndex, pObjIndex->level ) ) )
   {
      send_to_char( "Couldn't create_object.\r\n", ch );
      log_printf( "%s: create_object failed.", __FUNCTION__ );
      return;
   }

   obj_to_char( obj, ch );
   act( AT_IMMORT, "$n makes arcane gestures, and opens $s hands to reveal $p!", ch, obj, NULL, TO_ROOM );
   ch_printf( ch, "&YYou make arcane gestures, and open your hands to reveal %s!\r\nObjVnum:  &W%d   &YKeywords:  &W%s\r\n",
      pObjIndex->short_descr, pObjIndex->vnum, pObjIndex->name );
}

CMDF( do_mcreate )
{
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *mob;
   char arg[MIL], arg2[MIL];
   int vnum, cvnum;

   if( is_npc( ch ) )
   {
      send_to_char( "Mobiles can't create.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   vnum = is_number( arg ) ? atoi( arg ) : -1;

   if( vnum == -1 || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: mcreate <vnum> [cvnum] <mobile name>\r\n", ch );
      return;
   }

   if( vnum < 1 || vnum > MAX_VNUM )
   {
      send_to_char( "Vnum out of range.\r\n", ch );
      return;
   }

   one_argument( argument, arg2 );
   cvnum = atoi( arg2 );
   if( cvnum != 0 )
      argument = one_argument( argument, arg2 );
   if( cvnum < 1 )
      cvnum = 0;

   if( get_mob_index( vnum ) )
   {
      send_to_char( "A mobile with that number already exists.\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_LEADER )
   {
      AREA_DATA *pArea;

      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to create mobiles.\r\n", ch );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\r\n", ch );
         return;
      }
   }

   if( !( pMobIndex = make_mobile( vnum, cvnum, argument ) ) )
   {
      send_to_char( "Error.\r\n", ch );
      log_printf( "%s: make_mobile failed to make a mob using vnum %d.", __FUNCTION__, vnum );
      return;
   }
   mob = create_mobile( pMobIndex );
   char_to_room( mob, ch->in_room );
   if( is_in_wilderness( ch ) )
      put_in_wilderness( mob, ch->cords[0], ch->cords[1] );
   act( AT_IMMORT, "$n waves $s arms about, and $N appears at $s command!", ch, NULL, mob, TO_ROOM );
   ch_printf( ch, "&YYou wave your arms about, and %s appears at your command!\r\nMobVnum:  &W%d   &YKeywords:  &W%s\r\n",
      pMobIndex->short_descr, pMobIndex->vnum, pMobIndex->name );
}

/* Simple but nice and handy line editor. - Thoric */
void edit_buffer( CHAR_DATA *ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   EDITOR_DATA *edit;
   char cmd[MIL], buf[( MLS + 2 )];
   int cbufsize = 0;
   short x, line, max_buf_lines;
   bool save;

   if( !ch )
      return;

   set_char_color( AT_GREEN, ch );

   if( !( d = ch->desc ) )
   {
      send_to_char( "You have no descriptor.\r\n", ch );
      return;
   }

   if( d->connected != CON_EDITING )
   {
      send_to_char( "You can't do that!\r\n", ch );
      bug( "%s: d->connected != CON_EDITING", __FUNCTION__ );
      stop_editing( ch );
      d->connected = CON_PLAYING;
      return;
   }

   if( ch->substate <= SUB_PAUSE )
   {
      send_to_char( "You can't do that!\r\n", ch );
      bug( "%s: illegal ch->substate (%d)", __FUNCTION__, ch->substate );
      stop_editing( ch );
      d->connected = CON_PLAYING;
      return;
   }

   if( !ch->editor )
   {
      send_to_char( "You can't do that!\r\n", ch );
      bug( "%s: null editor", __FUNCTION__ );
      stop_editing( ch );
      d->connected = CON_PLAYING;
      return;
   }

   edit = ch->editor;
   save = false;
   max_buf_lines = ( ( MEL - 1 ) / 2 ); /* Defaults to half max */

   /* For these use the max possible */
   if( ch->substate == SUB_MPROG_EDIT || ch->substate == SUB_HELP_EDIT || ch->substate == SUB_MAP_EDIT )
      max_buf_lines = ( MEL - 1 );

   if( argument[0] == '/' || argument[0] == '\\' )
   {
      one_argument( argument + 1, cmd );
      if( !str_cmp( cmd, "?" ) )
      {
         send_to_char( "Editing commands\r\n---------------------------------\r\n", ch );
         send_to_char( "/l              list buffer\r\n", ch );
         send_to_char( "/c              clear buffer\r\n", ch );
         send_to_char( "/d [line]       delete line\r\n", ch );
         send_to_char( "/g <line>       goto line\r\n", ch );
         send_to_char( "/i <line>       insert line\r\n", ch );
         send_to_char( "/a              abort editing\r\n", ch );
         if( get_trust( ch ) >= PERM_IMM )
            send_to_char( "/! <command>    execute command (don't use another editing command)\r\n", ch );
         send_to_char( "/s              save buffer\r\n\r\n> ", ch );
         return;
      }
      if( !str_cmp( cmd, "c" ) )
      {
         memset( edit, '\0', sizeof( EDITOR_DATA ) );
         edit->numlines = 0;
         edit->on_line = 0;
         send_to_char( "Buffer cleared.\r\n> ", ch );
         return;
      }
      if( !str_cmp( cmd, "i" ) )
      {
         if( edit->numlines >= max_buf_lines )
            send_to_char( "Buffer is full.\r\n> ", ch );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
            else
               line = edit->on_line;
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               send_to_char( "Out of range.\r\n> ", ch );
            else
            {
               for( x = ++edit->numlines; x > line; x-- )
                  mudstrlcpy( edit->line[x], edit->line[x - 1], MLS );
               mudstrlcpy( edit->line[line], "", MLS );
               send_to_char( "Line inserted.\r\n", ch );
               cbufsize = get_buffer_size( ch );
               ch_printf( ch, "Your buffer currently has %d of %d characters.\r\n", cbufsize, MSL );
               ch_printf( ch, "Your buffer currently has %d of %d lines.\r\n> ", edit->numlines, max_buf_lines );
            }
         }
         return;
      }
      if( !str_cmp( cmd, "d" ) )
      {
         if( edit->numlines == 0 )
            send_to_char( "Buffer is empty.\r\n> ", ch );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
            else
               line = edit->on_line;
            /* While this might not look right should keep the last line blank */
            if( line < 0 || line >= edit->numlines )
               ch_printf( ch, "Out of range. Valid range is 1 - %d\r\n> ", edit->numlines );
            else
            {
               if( line == 0 && edit->numlines == 1 )
               {
                  memset( edit, '\0', sizeof( EDITOR_DATA ) );
                  edit->numlines = 0;
                  edit->on_line = 0;
                  ch_printf( ch, "Line %d deleted.\r\n> ", ( line + 1 ) );
                  return;
               }
               for( x = line; x < ( edit->numlines - 1 ); x++ )
                  mudstrlcpy( edit->line[x], edit->line[x + 1], MLS );
               /* Clear last line, decrease numlines and clear new last line */
               mudstrlcpy( edit->line[edit->numlines--], "", MLS );
               mudstrlcpy( edit->line[edit->numlines], "", MLS );
               if( edit->on_line > edit->numlines )
                  edit->on_line = edit->numlines;
               ch_printf( ch, "Line %d deleted.\r\n", ( line + 1 ) );
               cbufsize = get_buffer_size( ch );
               ch_printf( ch, "Your buffer currently has %d of %d characters.\r\n", cbufsize, MSL );
               ch_printf( ch, "Your buffer currently has %d of %d lines.\r\n> ", edit->numlines, max_buf_lines );
            }
         }
         return;
      }
      if( !str_cmp( cmd, "g" ) )
      {
         if( edit->numlines == 0 )
            send_to_char( "Buffer is empty.\r\n> ", ch );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
            else
            {
               send_to_char( "Goto what line?\r\n> ", ch );
               return;
            }
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               send_to_char( "Out of range.\r\n> ", ch );
            else
            {
               edit->on_line = line;
               ch_printf( ch, "(On line %d)\r\n> ", line + 1 );
            }
         }
         return;
      }
      if( !str_cmp( cmd, "l" ) )
      {
         if( edit->numlines == 0 )
            send_to_char( "Buffer is empty.\r\n> ", ch );
         else
         {
            send_to_char( "------------------\r\n", ch );
            for( x = 0; x <= edit->numlines; x++ )
            {
               ch_printf( ch, "%s%2d> %s\r\n", ( ( edit->on_line + 1 ) == ( x + 1 ) ) ? "*" : " ",
                  ( x + 1 ), edit->line[x] );
            }
            send_to_char( "------------------\r\n", ch );
            cbufsize = get_buffer_size( ch );
            ch_printf( ch, "Your buffer currently has %d of %d characters.\r\n", cbufsize, MSL );
            ch_printf( ch, "Your buffer currently has %d of %d lines.\r\n> ", edit->numlines, max_buf_lines );
         }
         return;
      }
      if( !str_cmp( cmd, "a" ) )
      {
         send_to_char( "\r\nAborting... ", ch );
         stop_editing( ch );
         return;
      }
      if( !str_cmp( cmd, "!" ) )
      {
         DO_FUN *last_cmd;
         int substate = ch->substate;

         if( get_trust( ch ) < PERM_IMM )
         {
            send_to_char( "\r\nYou don't have access to this.\r\n", ch );
            return;
         }
         last_cmd = ch->last_cmd;
         ch->substate = SUB_RESTRICTED;
         interpret( ch, argument + 3 );
         ch->substate = substate;
         ch->last_cmd = last_cmd;
         set_char_color( AT_GREEN, ch );
         send_to_char( "\r\n> ", ch );
         return;
      }
      if( !str_cmp( cmd, "s" ) )
      {
         d->connected = CON_PLAYING;
         if( !ch->last_cmd )
            return;
         ( *ch->last_cmd ) ( ch, (char *)"" );
         return;
      }
   }

   if( ( edit->size + strlen( argument ) + 1 ) >= ( MSL - 1 ) )
   {
      send_to_char( "Your buffer is full.\r\n", ch );
      save = true;
   }
   else
   {
      while( argument && argument[0] != '\0' )
      {
         int normal = strlen( argument ); /* Normal string length */
         int nocolor = color_strlen( argument ); /* String length without color */
         int max = ( 79 + ( normal - nocolor ) );
         int countback = 0;

         if( nocolor > 79 ) /* Limit of normal characters */
         {
            if( max > MLS ) /* Limit it all */
               max = MLS;

            /* Lets see if we can go backwards and find a space */
            for( countback = max; countback > 0; countback-- )
            {
               if( argument[countback] == ' ' )
                  break;
            }
            if( countback > 0 )
               max = countback;

            strncpy( buf, argument, max );
            buf[max] = 0;
            argument += max;

            /* Remove the extra space on start of next line */
            if( countback > 0 )
               argument++;

            /* Remove starting spaces */
            while( *argument == ' ' )
               argument++;

            send_to_char( "(Long line (excludling color) wrapped)\r\n> ", ch );
         }
         else if( normal > ( MLS - 2 ) ) /* Limit of size ( counting colors ) */
         {
            max = ( MLS - 2 );
            /* Lets see if we can go backwards and find a space */
            for( countback = ( MLS - 2 ); countback > 0; countback-- )
               if( argument[countback] == ' ' )
                  break;
            if( countback > 0 )
               max = countback;

            strncpy( buf, argument, max );
            buf[max] = 0;
            argument += max;

            /* Remove the extra space on start of next line */
            if( countback > 0 )
               argument++;

            /* Remove starting spaces */
            while( *argument == ' ' )
               argument++;

            send_to_char( "(Long line (includling color) wrapped)\r\n> ", ch );
         }
         else
         {
            mudstrlcpy( buf, argument, sizeof( buf ) );
            argument += strlen( argument );
         }

         mudstrlcpy( edit->line[edit->on_line++], buf, MLS );
         if( edit->on_line > edit->numlines )
            edit->numlines++;

         if( edit->numlines > max_buf_lines )
         {
            edit->numlines = max_buf_lines;
            send_to_char( "Your buffer is full.\r\n", ch );
            save = true;
         }
      }
   }

   if( ( cbufsize = get_buffer_size( ch ) ) >= MSL )
   {
      send_to_char( "Your buffer is full.\r\n", ch );
      save = true;
   }
   else
   {
      ch_printf( ch, "Your buffer currently has %d of %d characters.\r\n", cbufsize, MSL );
      ch_printf( ch, "Your buffer currently has %d of %d lines.\r\n", edit->numlines, max_buf_lines );
   }

   if( save )
   {
      d->connected = CON_PLAYING;
      if( !ch->last_cmd )
         return;
      ( *ch->last_cmd ) ( ch, (char *)"" );
      return;
   }
   send_to_char( "> ", ch );
}

void assign_area( CHAR_DATA *ch )
{
   AREA_DATA *tarea, *tmp;
   char buf[MSL], buf2[MSL], taf[MFN];
   bool created = false;

   if( is_npc( ch ) )
      return;
   if( get_trust( ch ) >= PERM_IMM && ch->pcdata->range_lo && ch->pcdata->range_hi )
   {
      tarea = ch->pcdata->area;
      snprintf( taf, sizeof( taf ), "%s.are", capitalize( ch->name ) );
      if( !tarea )
      {
         for( tmp = first_build; tmp; tmp = tmp->next )
            if( !str_cmp( taf, tmp->filename ) )
            {
               tarea = tmp;
               break;
            }
      }
      if( !tarea )
      {
         log_printf_plus( LOG_NORMAL, get_trust( ch ), "Creating area entry for %s", ch->name );
         CREATE( tarea, AREA_DATA, 1 );
         LINK( tarea, first_build, last_build, next, prev );
         tarea->first_room = tarea->last_room = NULL;
         snprintf( buf, sizeof( buf ), "{PROTO} %s's area in progress", ch->name );
         tarea->name = STRALLOC( buf );
         tarea->filename = STRALLOC( taf );
         snprintf( buf2, sizeof( buf2 ), "%s", ch->name );
         tarea->author = STRALLOC( buf2 );
         tarea->age = 0;
         tarea->nplayer = 0;
         CREATE( tarea->weather, WEATHER_DATA, 1 );   /* FB */
         tarea->weather->temp = 0;
         tarea->weather->precip = 0;
         tarea->weather->wind = 0;
         tarea->weather->temp_vector = 0;
         tarea->weather->precip_vector = 0;
         tarea->weather->wind_vector = 0;
         tarea->weather->climate_temp = 2;
         tarea->weather->climate_precip = 2;
         tarea->weather->climate_wind = 2;
         tarea->weather->first_neighbor = NULL;
         tarea->weather->last_neighbor = NULL;
         tarea->weather->echo = NULL;
         tarea->weather->echo_color = AT_GRAY;
         created = true;
      }
      else
         log_printf_plus( LOG_NORMAL, get_trust( ch ), "Updating area entry for %s", ch->name );
      tarea->low_vnum = ch->pcdata->range_lo;
      tarea->hi_vnum = ch->pcdata->range_hi;
      ch->pcdata->area = tarea;
      if( created )
         sort_area( tarea, true );
   }
}

EXTRA_DESCR_DATA *SetRExtra( ROOM_INDEX_DATA *room, char *keywords )
{
   EXTRA_DESCR_DATA *ed;

   for( ed = room->first_extradesc; ed; ed = ed->next )
   {
      if( is_name( keywords, ed->keyword ) )
         break;
   }
   if( !ed )
   {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      LINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
      ed->keyword = STRALLOC( keywords );
      ed->description = NULL;
      top_ed++;
   }
   return ed;
}

bool DelRExtra( ROOM_INDEX_DATA *room, char *keywords )
{
   EXTRA_DESCR_DATA *rmed;

   for( rmed = room->first_extradesc; rmed; rmed = rmed->next )
   {
      if( is_name( keywords, rmed->keyword ) )
         break;
   }
   if( !rmed )
      return false;
   UNLINK( rmed, room->first_extradesc, room->last_extradesc, next, prev );
   STRFREE( rmed->keyword );
   STRFREE( rmed->description );
   DISPOSE( rmed );
   top_ed--;
   return true;
}

EXTRA_DESCR_DATA *SetOExtra( OBJ_DATA *obj, char *keywords )
{
   EXTRA_DESCR_DATA *ed;

   for( ed = obj->first_extradesc; ed; ed = ed->next )
   {
      if( is_name( keywords, ed->keyword ) )
         break;
   }
   if( !ed )
   {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
      ed->keyword = STRALLOC( keywords );
      ed->description = NULL;
      top_ed++;
   }
   return ed;
}

bool DelOExtra( OBJ_DATA *obj, char *keywords )
{
   EXTRA_DESCR_DATA *rmed;

   for( rmed = obj->first_extradesc; rmed; rmed = rmed->next )
   {
      if( is_name( keywords, rmed->keyword ) )
         break;
   }
   if( !rmed )
      return false;
   UNLINK( rmed, obj->first_extradesc, obj->last_extradesc, next, prev );
   STRFREE( rmed->keyword );
   STRFREE( rmed->description );
   DISPOSE( rmed );
   top_ed--;
   return true;
}

EXTRA_DESCR_DATA *SetOExtraProto( OBJ_INDEX_DATA *obj, char *keywords )
{
   EXTRA_DESCR_DATA *ed;

   for( ed = obj->first_extradesc; ed; ed = ed->next )
   {
      if( is_name( keywords, ed->keyword ) )
         break;
   }
   if( !ed )
   {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
      ed->keyword = STRALLOC( keywords );
      ed->description = NULL;
      top_ed++;
   }
   return ed;
}

bool DelOExtraProto( OBJ_INDEX_DATA *obj, char *keywords )
{
   EXTRA_DESCR_DATA *rmed;

   for( rmed = obj->first_extradesc; rmed; rmed = rmed->next )
   {
      if( is_name( keywords, rmed->keyword ) )
         break;
   }
   if( !rmed )
      return false;
   UNLINK( rmed, obj->first_extradesc, obj->last_extradesc, next, prev );
   STRFREE( rmed->keyword );
   STRFREE( rmed->description );
   DISPOSE( rmed );
   top_ed--;
   return true;
}

void mpedit( CHAR_DATA *ch, MPROG_DATA *mprg, int mptype, char *argument )
{
   if( mptype != -1 )
   {
      mprg->type = mptype;
      if( mprg->arglist )
         STRFREE( mprg->arglist );
      mprg->arglist = STRALLOC( argument );
   }
   ch->substate = SUB_MPROG_EDIT;
   ch->dest_buf = mprg;
   start_editing( ch, mprg->comlist );
}

/* Mobprogram editing - cumbersome - Thoric */
CMDF( do_mpedit )
{
   CHAR_DATA *victim;
   MPROG_DATA *mprog, *mprg, *mprg_next = NULL;
   char arg1[MIL], arg2[MIL], arg3[MIL], arg4[MIL];
   int value, mptype = -1, cnt;

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't mpedit\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;

      case SUB_MPROG_EDIT:
         if( !valid_destbuf( ch, __FUNCTION__ ) )
            return;
         mprog = ( MPROG_DATA *) ch->dest_buf;
         if( mprog->comlist )
            STRFREE( mprog->comlist );
         mprog->comlist = copy_buffer( ch );
         stop_editing( ch );
         return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   value = atoi( arg3 );
   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Usage: mpedit <victim> <command> [number] <program> <value>\r\n", ch );
      send_to_char( "\r\n", ch );
      send_to_char( "Command being one of:\r\n", ch );
      send_to_char( "  add    delete  insert  edit\r\n", ch );
      send_to_char( "Program being one of:\r\n", ch );
      send_to_char( "  act    speech  rand    fight  hitprcnt greet  allgreet\r\n", ch );
      send_to_char( "  entry  give    bribe   death  time     hour   script\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_LEADER )
   {
      if( !( victim = get_char_room( ch, arg1 ) ) )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }
   }
   else
   {
      if( !( victim = get_char_world( ch, arg1 ) ) )
      {
         send_to_char( "No one like that in all the realms.\r\n", ch );
         return;
      }
   }

   if( !is_npc( victim ) )
   {
      send_to_char( "You can't do that!\r\n", ch );
      return;
   }
   if( get_trust( ch ) < PERM_HEAD && is_npc( victim ) && xIS_SET( victim->act, ACT_STATSHIELD ) )
   {
      set_pager_color( AT_IMMORT, ch );
      send_to_pager( "Their godly glow prevents you from getting close enough.\r\n", ch );
      return;
   }
   if( !can_mmodify( ch, victim ) )
      return;

   if( !xIS_SET( victim->act, ACT_PROTOTYPE ) )
   {
      send_to_char( "A mobile must have a prototype flag to be mpset.\r\n", ch );
      return;
   }

   mprog = victim->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !str_cmp( arg2, "edit" ) )
   {
      if( !mprog )
      {
         send_to_char( "That mobile has no mob programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( arg4[0] != '\0' )
      {
         mptype = get_mpflag( arg4 );
         if( mptype == -1 )
         {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mpedit( ch, mprg, mptype, argument );
            xCLEAR_BITS( victim->pIndexData->progtypes );
            for( mprg = mprog; mprg; mprg = mprg->next )
               xSET_BIT( victim->pIndexData->progtypes, mprg->type );
            return;
         }
      }
      send_to_char( "Program not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      int num;
      bool found;

      if( !mprog )
      {
         send_to_char( "That mobile has no mob programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = 0;
      found = false;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mptype = mprg->type;
            found = true;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = num = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
         if( mprg->type == mptype )
            num++;
      if( value == 1 )
      {
         mprg_next = victim->pIndexData->mudprogs;
         victim->pIndexData->mudprogs = mprg_next->next;
      }
      else
         for( mprg = mprog; mprg; mprg = mprg_next )
         {
            mprg_next = mprg->next;
            if( ++cnt == ( value - 1 ) )
            {
               mprg->next = mprg_next->next;
               break;
            }
         }
      if( mprg_next )
      {
         STRFREE( mprg_next->arglist );
         STRFREE( mprg_next->comlist );
         DISPOSE( mprg_next );
         if( num <= 1 )
            xREMOVE_BIT( victim->pIndexData->progtypes, mptype );
         send_to_char( "Program removed.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      if( !mprog )
      {
         send_to_char( "That mobile has no mob programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      mptype = get_mpflag( arg4 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\r\n", ch );
         return;
      }
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      if( value == 1 )
      {
         CREATE( mprg, MPROG_DATA, 1 );
         xSET_BIT( victim->pIndexData->progtypes, mptype );
         mpedit( ch, mprg, mptype, argument );
         mprg->next = mprog;
         victim->pIndexData->mudprogs = mprg;
         return;
      }
      cnt = 1;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value && mprg->next )
         {
            CREATE( mprg_next, MPROG_DATA, 1 );
            xSET_BIT( victim->pIndexData->progtypes, mptype );
            mpedit( ch, mprg_next, mptype, argument );
            mprg_next->next = mprg->next;
            mprg->next = mprg_next;
            return;
         }
      }
      send_to_char( "Program not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "add" ) )
   {
      if( ( mptype = get_mpflag( arg3 ) ) == -1 )
      {
         send_to_char( "Unknown program type.\r\n", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't add a program without specifying some other information.", ch );
         return;
      }
      if( mprog )
         for( ; mprog->next; mprog = mprog->next );
      CREATE( mprg, MPROG_DATA, 1 );
      if( mprog )
         mprog->next = mprg;
      else
         victim->pIndexData->mudprogs = mprg;
      xSET_BIT( victim->pIndexData->progtypes, mptype );
      mpedit( ch, mprg, mptype, argument );
      mprg->next = NULL;
      return;
   }

   do_mpedit( ch, (char *)"" );
}

CMDF( do_opedit )
{
   OBJ_DATA *obj;
   MPROG_DATA *mprog, *mprg, *mprg_next = NULL;
   char arg1[MIL], arg2[MIL], arg3[MIL], arg4[MIL];
   int value, mptype = -1, cnt;

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't opedit\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;

      case SUB_MPROG_EDIT:
         if( !valid_destbuf( ch, __FUNCTION__ ) )
            return;
         mprog = ( MPROG_DATA *) ch->dest_buf;
         if( mprog->comlist )
            STRFREE( mprog->comlist );
         mprog->comlist = copy_buffer( ch );
         stop_editing( ch );
         return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   value = atoi( arg3 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Usage: opedit <object> <command> [number] <program> <value>\r\n\r\n", ch );
      send_to_char( "Command being one of:\r\n", ch );
      send_to_char( "  add   delete  insert  edit\r\n", ch );
      send_to_char( "Program being one of:\r\n", ch );
      send_to_char( "  act   speech  rand    wear    remove  sac  zap\r\n", ch );
      send_to_char( "  get   drop    damage  repair  greet   exa  use\r\n", ch );
      send_to_char( "  pull  push (for levers, pullchains, buttons)\r\n\r\n", ch );
      send_to_char( "Object should be in your inventory to edit.\r\n", ch );
      return;
   }

   if( get_trust( ch ) < PERM_LEADER )
   {
      if( !( obj = get_obj_carry( ch, arg1 ) ) )
      {
         send_to_char( "You aren't carrying that.\r\n", ch );
         return;
      }
   }
   else
   {
      if( !( obj = get_obj_world( ch, arg1 ) ) )
      {
         send_to_char( "Nothing like that in all the realms.\r\n", ch );
         return;
      }
   }

   if( !can_omodify( ch, obj ) )
      return;

   if( !is_obj_stat( obj, ITEM_PROTOTYPE ) )
   {
      send_to_char( "An object must have a prototype flag to be opset.\r\n", ch );
      return;
   }

   mprog = obj->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !str_cmp( arg2, "edit" ) )
   {
      if( !mprog )
      {
         send_to_char( "That object has no obj programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( arg4[0] != '\0' )
      {
         mptype = get_mpflag( arg4 );
         if( mptype == -1 )
         {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mpedit( ch, mprg, mptype, argument );
            xCLEAR_BITS( obj->pIndexData->progtypes );
            for( mprg = mprog; mprg; mprg = mprg->next )
               xSET_BIT( obj->pIndexData->progtypes, mprg->type );
            return;
         }
      }
      send_to_char( "Program not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      int num;
      bool found;

      if( !mprog )
      {
         send_to_char( "That object has no obj programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = 0;
      found = false;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mptype = mprg->type;
            found = true;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = num = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
         if( mprg->type == mptype )
            num++;
      if( value == 1 )
      {
         mprg_next = obj->pIndexData->mudprogs;
         obj->pIndexData->mudprogs = mprg_next->next;
      }
      else
         for( mprg = mprog; mprg; mprg = mprg_next )
         {
            mprg_next = mprg->next;
            if( ++cnt == ( value - 1 ) )
            {
               mprg->next = mprg_next->next;
               break;
            }
         }
      if( mprg_next )
      {
         STRFREE( mprg_next->arglist );
         STRFREE( mprg_next->comlist );
         DISPOSE( mprg_next );
         if( num <= 1 )
            xREMOVE_BIT( obj->pIndexData->progtypes, mptype );
         send_to_char( "Program removed.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      if( !mprog )
      {
         send_to_char( "That object has no obj programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg4 );
      mptype = get_mpflag( arg4 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\r\n", ch );
         return;
      }
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      if( value == 1 )
      {
         CREATE( mprg, MPROG_DATA, 1 );
         xSET_BIT( obj->pIndexData->progtypes, mptype );
         mpedit( ch, mprg, mptype, argument );
         mprg->next = mprog;
         obj->pIndexData->mudprogs = mprg;
         return;
      }
      cnt = 1;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value && mprg->next )
         {
            CREATE( mprg_next, MPROG_DATA, 1 );
            xSET_BIT( obj->pIndexData->progtypes, mptype );
            mpedit( ch, mprg_next, mptype, argument );
            mprg_next->next = mprg->next;
            mprg->next = mprg_next;
            return;
         }
      }
      send_to_char( "Program not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "add" ) )
   {
      mptype = get_mpflag( arg3 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\r\n", ch );
         return;
      }
      if( mprog )
         for( ; mprog->next; mprog = mprog->next );
      CREATE( mprg, MPROG_DATA, 1 );
      if( mprog )
         mprog->next = mprg;
      else
         obj->pIndexData->mudprogs = mprg;
      xSET_BIT( obj->pIndexData->progtypes, mptype );
      mpedit( ch, mprg, mptype, argument );
      mprg->next = NULL;
      return;
   }

   do_opedit( ch, (char *)"" );
}

/* RoomProg Support */
void rpedit( CHAR_DATA *ch, MPROG_DATA *mprg, int mptype, char *argument )
{
   if( mptype != -1 )
   {
      mprg->type = mptype;
      STRFREE( mprg->arglist );
      mprg->arglist = STRALLOC( argument );
   }
   ch->substate = SUB_MPROG_EDIT;
   ch->dest_buf = mprg;
   start_editing( ch, mprg->comlist );
}

CMDF( do_rpedit )
{
   MPROG_DATA *mprog, *mprg, *mprg_next = NULL;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int value, mptype = -1, cnt;

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't rpedit\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;

      case SUB_MPROG_EDIT:
         if( !valid_destbuf( ch, __FUNCTION__ ) )
            return;
         mprog = ( MPROG_DATA *) ch->dest_buf;
         if( mprog->comlist )
            STRFREE( mprog->comlist );
         mprog->comlist = copy_buffer( ch );
         stop_editing( ch );
         return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   value = atoi( arg2 );

   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: rpedit <command> [number] <program> <value>\r\n", ch );
      send_to_char( "Command being one of:\r\n", ch );
      send_to_char( "  add  delete  insert  edit\r\n", ch );
      send_to_char( "Program being one of:\r\n", ch );
      send_to_char( "  act  speech  rand    sleep  rest  fight  entry  leave  death\r\n\r\n", ch );
      send_to_char( "You should be standing in the room you wish to edit.\r\n", ch );
      return;
   }

   if( !can_rmodify( ch, ch->in_room ) )
      return;

   mprog = ch->in_room->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !str_cmp( arg1, "edit" ) )
   {
      if( !mprog )
      {
         send_to_char( "This room has no room programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      if( arg3[0] != '\0' )
      {
         mptype = get_mpflag( arg3 );
         if( mptype == -1 )
         {
            send_to_char( "Unknown program type.\r\n", ch );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mpedit( ch, mprg, mptype, argument );
            xCLEAR_BITS( ch->in_room->progtypes );
            for( mprg = mprog; mprg; mprg = mprg->next )
               xSET_BIT( ch->in_room->progtypes, mprg->type );
            return;
         }
      }
      send_to_char( "Program not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg1, "delete" ) )
   {
      int num;
      bool found;

      if( !mprog )
      {
         send_to_char( "That room has no room programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = 0;
      found = false;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value )
         {
            mptype = mprg->type;
            found = true;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      cnt = num = 0;
      for( mprg = mprog; mprg; mprg = mprg->next )
         if( mprg->type == mptype )
            num++;
      if( value == 1 )
      {
         mprg_next = ch->in_room->mudprogs;
         ch->in_room->mudprogs = mprg_next->next;
      }
      else
      {
         for( mprg = mprog; mprg; mprg = mprg_next )
         {
            mprg_next = mprg->next;
            if( ++cnt == ( value - 1 ) )
            {
               mprg->next = mprg_next->next;
               break;
            }
         }
      }
      if( mprg_next )
      {
         STRFREE( mprg_next->arglist );
         STRFREE( mprg_next->comlist );
         DISPOSE( mprg_next );
         if( num <= 1 )
            xREMOVE_BIT( ch->in_room->progtypes, mptype );
         send_to_char( "Program removed.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      if( !mprog )
      {
         send_to_char( "That room has no room programs.\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      mptype = get_mpflag( arg2 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\r\n", ch );
         return;
      }
      if( value < 1 )
      {
         send_to_char( "Program not found.\r\n", ch );
         return;
      }
      if( value == 1 )
      {
         CREATE( mprg, MPROG_DATA, 1 );
         xSET_BIT( ch->in_room->progtypes, mptype );
         mpedit( ch, mprg, mptype, argument );
         mprg->next = mprog;
         ch->in_room->mudprogs = mprg;
         return;
      }
      cnt = 1;
      for( mprg = mprog; mprg; mprg = mprg->next )
      {
         if( ++cnt == value && mprg->next )
         {
            CREATE( mprg_next, MPROG_DATA, 1 );
            xSET_BIT( ch->in_room->progtypes, mptype );
            mpedit( ch, mprg_next, mptype, argument );
            mprg_next->next = mprg->next;
            mprg->next = mprg_next;
            return;
         }
      }
      send_to_char( "Program not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg1, "add" ) )
   {
      mptype = get_mpflag( arg2 );
      if( mptype == -1 )
      {
         send_to_char( "Unknown program type.\r\n", ch );
         return;
      }
      if( mprog )
         for( ; mprog->next; mprog = mprog->next );
      CREATE( mprg, MPROG_DATA, 1 );
      if( mprog )
         mprog->next = mprg;
      else
         ch->in_room->mudprogs = mprg;
      xSET_BIT( ch->in_room->progtypes, mptype );
      mpedit( ch, mprg, mptype, argument );
      mprg->next = NULL;
      return;
   }

   do_rpedit( ch, (char *)"" );
}

CMDF( do_rdelete )
{
   ROOM_INDEX_DATA *location;

   if( ch->substate == SUB_RESTRICTED )
   {
      send_to_char( "You can't do that while in a subprompt.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Delete which room?\r\n", ch );
      return;
   }

   /* Find the room. */
   if( !( location = find_location( ch, argument ) ) )
   {
      send_to_char( "No such location.\r\n", ch );
      return;
   }

   /* Does the player have the right to delete this room? */
   if( get_trust( ch ) < sysdata.perm_modify_proto
   && ( location->vnum < ch->pcdata->area->low_vnum || location->vnum > ch->pcdata->area->hi_vnum ) )
   {
      send_to_char( "That room is not in your assigned range.\r\n", ch );
      return;
   }

   delete_room( location );
   fix_exits( ); /* Need to call this to solve a crash */
   ch_printf( ch, "Room %s has been deleted.\r\n", argument );
}

CMDF( do_odelete )
{
   OBJ_INDEX_DATA *obj;
   int vnum;

   if( ch->substate == SUB_RESTRICTED )
   {
      send_to_char( "You can't do that while in a subprompt.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Delete which object?\r\n", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      send_to_char( "You must specify the object's vnum to delete it.\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   /* Find the obj. */
   if( !( obj = get_obj_index( vnum ) ) )
   {
      send_to_char( "No such object.\r\n", ch );
      return;
   }

   /* Does the player have the right to delete this object? */
   if( get_trust( ch ) < sysdata.perm_modify_proto
   && ( obj->vnum < ch->pcdata->area->low_vnum || obj->vnum > ch->pcdata->area->hi_vnum ) )
   {
      send_to_char( "That object is not in your assigned range.\r\n", ch );
      return;
   }
   delete_obj( obj );
   ch_printf( ch, "Object %d has been deleted.\r\n", vnum );
}

CMDF( do_mdelete )
{
   MOB_INDEX_DATA *mob;
   int vnum;

   if( ch->substate == SUB_RESTRICTED )
   {
      send_to_char( "You can't do that while in a subprompt.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Delete which mob?\r\n", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      send_to_char( "You must specify the mob's vnum to delete it.\r\n", ch );
      return;
   }

   vnum = atoi( argument );

   /* Find the mob. */
   if( !( mob = get_mob_index( vnum ) ) )
   {
      send_to_char( "No such mob.\r\n", ch );
      return;
   }

   /* Does the player have the right to delete this mob? */
   if( get_trust( ch ) < sysdata.perm_modify_proto
   && ( mob->vnum < ch->pcdata->area->low_vnum || mob->vnum > ch->pcdata->area->hi_vnum ) )
   {
      send_to_char( "That mob is not in your assigned range.\r\n", ch );
      return;
   }

   delete_mob( mob );
   ch_printf( ch, "Mob %d has been deleted.\r\n", vnum );
}

/*  
 *  Mobile and Object Program Copying 
 *  Last modified Feb. 24 1999
 *  Mystaric
 */
void mpcopy( MPROG_DATA *source, MPROG_DATA *destination )
{
   destination->type = source->type;
   destination->triggered = source->triggered;
   destination->resetdelay = source->resetdelay;
   destination->arglist = STRALLOC( source->arglist );
   destination->comlist = STRALLOC( source->comlist );
   destination->next = NULL;
}

CMDF( do_opcopy )
{
   OBJ_DATA *source = NULL, *destination = NULL;
   MPROG_DATA *source_oprog = NULL, *dest_oprog = NULL, *source_oprg = NULL, *dest_oprg = NULL;
   char sobj[MIL], prog[MIL], num[MIL], dobj[MIL];
   int value = -1, optype = -1, cnt = 0;
   bool COPY = false;

   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't opcopy\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   argument = one_argument( argument, sobj );
   argument = one_argument( argument, prog );

   if( sobj == NULL || sobj[0] == '\0' || prog == NULL || prog[0] == '\0' )
   {
      send_to_char( "Usage: opcopy <object1> <program> [number] <object2>\r\n", ch );
      send_to_char( "Usage: opcopy <object1> all <object2>\r\n", ch );
      send_to_char( "Usage: opcopy <object1> all <object2> <program>\r\n", ch );
      send_to_char( "\r\n", ch );
      send_to_char( "Program being one of:\r\n", ch );
      send_to_char( "  act speech rand wear remove sac zap get\r\n", ch );
      send_to_char( "  drop damage repair greet exa use\r\n", ch );
      send_to_char( "  pull push (for levers,pullchains,buttons)\r\n\r\n", ch );
      send_to_char( "Object should be in your inventory to edit.\r\n", ch );
      return;
   }

   if( !strcmp( prog, "all" ) )
   {
      argument = one_argument( argument, dobj );
      argument = one_argument( argument, prog );
      optype = get_mpflag( prog );
      COPY = true;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, dobj );
      value = atoi( num );
   }

   if( get_trust( ch ) < PERM_LEADER )
   {
      if( !( source = get_obj_carry( ch, sobj ) ) )
      {
         send_to_char( "You aren't carrying source object.\r\n", ch );
         return;
      }

      if( !( destination = get_obj_carry( ch, dobj ) ) )
      {
         send_to_char( "You aren't carrying destination object.\r\n", ch );
         return;
      }
   }
   else
   {
      if( !( source = get_obj_world( ch, sobj ) ) )
      {
         send_to_char( "Can't find source object in all the realms.\r\n", ch );
         return;
      }

      if( !( destination = get_obj_world( ch, dobj ) ) )
      {
         send_to_char( "Can't find destination object in all the realms.\r\n", ch );
         return;
      }
   }

   if( source == destination )
   {
      send_to_char( "Source and destination objects can't be the same\r\n", ch );
      return;
   }


   if( !can_omodify( ch, destination ) )
   {
      send_to_char( "You can't modify destination object.\r\n", ch );
      return;
   }

   if( !is_obj_stat( destination, ITEM_PROTOTYPE ) )
   {
      send_to_char( "Destination object must have prototype flag.\r\n", ch );
      return;
   }

   set_char_color( AT_PLAIN, ch );

   source_oprog = source->pIndexData->mudprogs;
   dest_oprog = destination->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !source_oprog )
   {
      send_to_char( "Source object has no mob programs.\r\n", ch );
      return;
   }

   if( COPY )
   {
      for( source_oprg = source_oprog; source_oprg; source_oprg = source_oprg->next )
      {
         if( optype == source_oprg->type || optype == -1 )
         {
            if( dest_oprog )
               for( ; dest_oprog->next; dest_oprog = dest_oprog->next );
            CREATE( dest_oprg, MPROG_DATA, 1 );
            if( dest_oprog )
               dest_oprog->next = dest_oprg;
            else
            {
               destination->pIndexData->mudprogs = dest_oprg;
               dest_oprog = dest_oprg;
            }
            mpcopy( source_oprg, dest_oprg );
            xSET_BIT( destination->pIndexData->progtypes, dest_oprg->type );
            cnt++;
         }
      }

      if( cnt == 0 )
      {
         send_to_char( "No such program in source object\r\n", ch );
         return;
      }
      ch_printf( ch, "%d programs successfully copied from %s to %s.\r\n", cnt, sobj, dobj );
      return;
   }

   if( value < 1 )
   {
      send_to_char( "No such program in source object.\r\n", ch );
      return;
   }

   optype = get_mpflag( prog );

   for( source_oprg = source_oprog; source_oprg; source_oprg = source_oprg->next )
   {
      if( ++cnt == value && source_oprg->type == optype )
      {
         if( dest_oprog )
            for( ; dest_oprog->next; dest_oprog = dest_oprog->next );
         CREATE( dest_oprg, MPROG_DATA, 1 );
         if( dest_oprog )
            dest_oprog->next = dest_oprg;
         else
            destination->pIndexData->mudprogs = dest_oprg;
         mpcopy( source_oprg, dest_oprg );
         xSET_BIT( destination->pIndexData->progtypes, dest_oprg->type );
         ch_printf( ch, "%s program %d from %s successfully copied to %s.\r\n", prog, value, sobj, dobj );
         return;
      }
   }
   if( !source_oprg )
   {
      send_to_char( "No such program in source object.\r\n", ch );
      return;
   }
   do_opcopy( ch, (char *)"" );
}

CMDF( do_mpcopy )
{
   CHAR_DATA *source = NULL, *destination = NULL;
   MPROG_DATA *source_mprog = NULL, *dest_mprog = NULL, *source_mprg = NULL, *dest_mprg = NULL;
   char smob[MIL], prog[MIL], num[MIL], dmob[MIL];
   int value = -1, mptype = -1, cnt = 0;
   bool COPY = false;

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't opcop\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   argument = one_argument( argument, smob );
   argument = one_argument( argument, prog );

   if( smob[0] == '\0' || prog[0] == '\0' )
   {
      send_to_char( "Usage: mpcopy <mobile1> <program> [number] <mobile2>\r\n", ch );
      send_to_char( "Usage: mpcopy <mobile1> all <mobile2>\r\n", ch );
      send_to_char( "Usage: mpcopy <mobile1> all <mobile2> <program>\r\n\r\n", ch );
      send_to_char( "Program being one of:\r\n", ch );
      send_to_char( "  act speech rand fight hitprcnt greet allgreet\r\n", ch );
      send_to_char( "  entry give bribe death time hour script\r\n", ch );
      return;
   }

   if( !strcmp( prog, "all" ) )
   {
      argument = one_argument( argument, dmob );
      argument = one_argument( argument, prog );
      mptype = get_mpflag( prog );
      COPY = true;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, dmob );
      value = atoi( num );
   }

   if( get_trust( ch ) < PERM_LEADER )
   {
      if( !( source = get_char_room( ch, smob ) ) )
      {
         send_to_char( "Source mobile is not present.\r\n", ch );
         return;
      }

      if( !( destination = get_char_room( ch, dmob ) ) )
      {
         send_to_char( "Destination mobile is not present.\r\n", ch );
         return;
      }
   }
   else
   {
      if( !( source = get_char_world( ch, smob ) ) )
      {
         send_to_char( "Can't find source mobile\r\n", ch );
         return;
      }

      if( !( destination = get_char_world( ch, dmob ) ) )
      {
         send_to_char( "Can't find destination mobile\r\n", ch );
         return;
      }
   }
   if( source == destination )
   {
      send_to_char( "Source and destination mobiles can't be the same\r\n", ch );
      return;
   }

   if( get_trust( ch ) < source->level || !is_npc( source ) ||
       get_trust( ch ) < destination->level || !is_npc( destination ) )
   {
      send_to_char( "You can't do that!\r\n", ch );
      return;
   }

   if( !can_mmodify( ch, destination ) )
   {
      send_to_char( "You can't modify destination mobile.\r\n", ch );
      return;
   }

   if( !xIS_SET( destination->act, ACT_PROTOTYPE ) )
   {
      send_to_char( "Destination mobile must have a prototype flag to mpcopy.\r\n", ch );
      return;
   }

   source_mprog = source->pIndexData->mudprogs;
   dest_mprog = destination->pIndexData->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !source_mprog )
   {
      send_to_char( "Source mobile has no mob programs.\r\n", ch );
      return;
   }


   if( COPY )
   {
      for( source_mprg = source_mprog; source_mprg; source_mprg = source_mprg->next )
      {
         if( mptype == source_mprg->type || mptype == -1 )
         {
            if( dest_mprog )
               for( ; dest_mprog->next; dest_mprog = dest_mprog->next );
            CREATE( dest_mprg, MPROG_DATA, 1 );

            if( dest_mprog )
               dest_mprog->next = dest_mprg;
            else
            {
               destination->pIndexData->mudprogs = dest_mprg;
               dest_mprog = dest_mprg;
            }
            mpcopy( source_mprg, dest_mprg );
            xSET_BIT( destination->pIndexData->progtypes, dest_mprg->type );
            cnt++;
         }
      }

      if( cnt == 0 )
      {
         send_to_char( "No such program in source mobile\r\n", ch );
         return;
      }
      ch_printf( ch, "%d programs successfully copied from %s to %s.\r\n", cnt, smob, dmob );
      return;
   }

   if( value < 1 )
   {
      send_to_char( "No such program in source mobile.\r\n", ch );
      return;
   }

   mptype = get_mpflag( prog );

   for( source_mprg = source_mprog; source_mprg; source_mprg = source_mprg->next )
   {
      if( ++cnt == value && source_mprg->type == mptype )
      {
         if( dest_mprog )
            for( ; dest_mprog->next; dest_mprog = dest_mprog->next );
         CREATE( dest_mprg, MPROG_DATA, 1 );
         if( dest_mprog )
            dest_mprog->next = dest_mprg;
         else
            destination->pIndexData->mudprogs = dest_mprg;
         mpcopy( source_mprg, dest_mprg );
         xSET_BIT( destination->pIndexData->progtypes, dest_mprg->type );
         ch_printf( ch, "%s program %d from %s successfully copied to %s.\r\n", prog, value, smob, dmob );
         return;
      }
   }

   if( !source_mprg )
   {
      send_to_char( "No such program in source mobile.\r\n", ch );
      return;
   }
   do_mpcopy( ch, (char *)"" );
}

CMDF( do_rpcopy )
{
   ROOM_INDEX_DATA *source = NULL, *destination = NULL;
   MPROG_DATA *source_rprog = NULL, *dest_rprog = NULL, *source_rprg = NULL, *dest_rprg = NULL;
   char sroom[MIL], prog[MIL], num[MIL], droom[MIL];
   int value = -1, rptype = -1, cnt = 0;
   bool COPY = false;

   set_char_color( AT_PLAIN, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't rpcop\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "You have no descriptor\r\n", ch );
      return;
   }

   argument = one_argument( argument, sroom );
   argument = one_argument( argument, prog );

   if( sroom[0] == '\0' || prog[0] == '\0' )
   {
      send_to_char( "Usage: rpcopy <room1> <program> [number] <room2>\r\n", ch );
      send_to_char( "Usage: rpcopy <room1> all <room2>\r\n", ch );
      send_to_char( "Usage: rpcopy <room1> all <room2> <program>\r\n\r\n", ch );
      send_to_char( "Program being one of:\r\n", ch );
      send_to_char( "  act speech rand fight sleep rest leave\r\n", ch );
      send_to_char( "  entry death time hour script\r\n", ch );
      return;
   }

   if( !strcmp( prog, "all" ) )
   {
      argument = one_argument( argument, droom );
      argument = one_argument( argument, prog );
      rptype = get_mpflag( prog );
      COPY = true;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, droom );
      value = atoi( num );
   }

   if( !( source = get_room_index( atoi( sroom ) ) ) )
   {
      send_to_char( "Source room doesn't exist.\r\n", ch );
      return;
   }
   if( !( destination = get_room_index( atoi( droom ) ) ) )
   {
      send_to_char( "Destination room doesn't exist.\r\n", ch );
      return;
   }

   if( source == destination )
   {
      send_to_char( "Source and destination rooms can't be the same\r\n", ch );
      return;
   }

   if( !can_rmodify( ch, destination ) )
   {
      send_to_char( "You can't modify destination room.\r\n", ch );
      return;
   }

   source_rprog = source->mudprogs;
   dest_rprog = destination->mudprogs;

   set_char_color( AT_GREEN, ch );

   if( !source_rprog )
   {
      send_to_char( "Source room has no mob programs.\r\n", ch );
      return;
   }

   if( COPY )
   {
      for( source_rprg = source_rprog; source_rprg; source_rprg = source_rprg->next )
      {
         if( rptype == source_rprg->type || rptype == -1 )
         {
            if( dest_rprog )
               for( ; dest_rprog->next; dest_rprog = dest_rprog->next );

            CREATE( dest_rprg, MPROG_DATA, 1 );

            if( dest_rprog )
               dest_rprog->next = dest_rprg;
            else
            {
               destination->mudprogs = dest_rprg;
               dest_rprog = dest_rprg;
            }
            mpcopy( source_rprg, dest_rprg );
            xSET_BIT( destination->progtypes, dest_rprg->type );
            cnt++;
         }
      }

      if( cnt == 0 )
         send_to_char( "No such program in source room.\r\n", ch );
      else
         ch_printf( ch, "%d programs successfully copied from %s to %s.\r\n", cnt, sroom, droom );
      return;
   }

   if( value < 1 )
   {
      send_to_char( "No such program in source room.\r\n", ch );
      return;
   }

   rptype = get_mpflag( prog );

   for( source_rprg = source_rprog; source_rprg; source_rprg = source_rprg->next )
   {
      if( ++cnt == value && source_rprg->type == rptype )
      {
         if( dest_rprog )
            for( ; dest_rprog->next; dest_rprog = dest_rprog->next );
         CREATE( dest_rprg, MPROG_DATA, 1 );
         if( dest_rprog )
            dest_rprog->next = dest_rprg;
         else
            destination->mudprogs = dest_rprg;
         mpcopy( source_rprg, dest_rprg );
         xSET_BIT( destination->progtypes, dest_rprg->type );
         ch_printf( ch, "%s program %d from %s successfully copied to %s.\r\n", prog, value, sroom, droom );
         return;
      }
   }

   if( !source_rprg )
   {
      send_to_char( "No such program in source room.\r\n", ch );
      return;
   }
   do_rpcopy( ch, (char *)"" );
}

/*
 * Modify an area's climate
 * Last modified: July 15, 1997 - Fireblade
 */
CMDF( do_climate )
{
   AREA_DATA *area;
   char arg[MIL];

   /* Little error checking */
   if( !ch )
   {
      bug( "%s: NULL character.", __FUNCTION__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: character not in a room.", __FUNCTION__ );
      return;
   }

   if( !ch->in_room->area )
   {
      bug( "%s: character not in an area.", __FUNCTION__ );
      return;
   }

   if( !ch->in_room->area->weather )
   {
      bug( "%s: area with NULL weather data.", __FUNCTION__ );
      return;
   }

   set_char_color( AT_BLUE, ch );

   area = ch->in_room->area;

   argument = strlower( argument );
   argument = one_argument( argument, arg );

   if( arg == NULL || arg[0] == '\0' )
   {
      NEIGHBOR_DATA *neigh;

      ch_printf( ch, "%s:\r\n", area->name );
      ch_printf( ch, "  Temperature:   %s\r\n", temp_settings[area->weather->climate_temp] );
      ch_printf( ch, "  Precipitation: %s\r\n", precip_settings[area->weather->climate_precip] );
      ch_printf( ch, "  Wind:          %s\r\n", wind_settings[area->weather->climate_wind] );

      if( area->weather->first_neighbor )
         send_to_char( "Neighboring weather systems:\r\n", ch );

      for( neigh = area->weather->first_neighbor; neigh; neigh = neigh->next )
         ch_printf( ch, "  %s\r\n", neigh->name );

      return;
   }
   else if( !str_cmp( arg, "temp" ) )
   {
      int i;

      argument = one_argument( argument, arg );
      for( i = 0; i < MAX_CLIMATE; i++ )
      {
         if( str_cmp( arg, temp_settings[i] ) )
            continue;

         area->weather->climate_temp = i;
         ch_printf( ch, "The climate temperature for %s is now %s.\r\n", area->name, temp_settings[i] );
         break;
      }

      if( i == MAX_CLIMATE )
      {
         send_to_char( "Possible temperature settings:\r\n", ch );
         for( i = 0; i < MAX_CLIMATE; i++ )
            ch_printf( ch, "  %s\r\n", temp_settings[i] );
      }

      return;
   }
   else if( !str_cmp( arg, "precip" ) )
   {
      int i;
      argument = one_argument( argument, arg );

      for( i = 0; i < MAX_CLIMATE; i++ )
      {
         if( str_cmp( arg, precip_settings[i] ) )
            continue;

         area->weather->climate_precip = i;
         ch_printf( ch, "The climate precipitation for %s is now %s.\r\n", area->name, precip_settings[i] );
         break;
      }

      if( i == MAX_CLIMATE )
      {
         send_to_char( "Possible precipitation settings:\r\n", ch );
         for( i = 0; i < MAX_CLIMATE; i++ )
            ch_printf( ch, "  %s\r\n", precip_settings[i] );
      }

      return;
   }
   else if( !str_cmp( arg, "wind" ) )
   {
      int i;
      argument = one_argument( argument, arg );

      for( i = 0; i < MAX_CLIMATE; i++ )
      {
         if( str_cmp( arg, wind_settings[i] ) )
            continue;

         area->weather->climate_wind = i;
         ch_printf( ch, "The climate wind for %s is now %s.\r\n", area->name, wind_settings[i] );
         break;
      }

      if( i == MAX_CLIMATE )
      {
         send_to_char( "Possible wind settings:\r\n", ch );
         for( i = 0; i < MAX_CLIMATE; i++ )
            ch_printf( ch, "  %s\r\n", wind_settings[i] );
      }
      return;
   }
   else if( !str_cmp( arg, "neighbor" ) )
   {
      NEIGHBOR_DATA *neigh;
      AREA_DATA *tarea;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Add or remove which area?\r\n", ch );
         return;
      }

      /* look for a matching list item */
      for( neigh = area->weather->first_neighbor; neigh; neigh = neigh->next )
      {
         if( nifty_is_name( argument, neigh->name ) )
            break;
      }

      /* if the a matching list entry is found, remove it */
      if( neigh )
      {
         /* look for the neighbor area in question */
         if( !( tarea = neigh->address ) )
            tarea = get_area( neigh->name );

         /* if there is an actual neighbor area remove its entry to this area */
         if( tarea )
         {
            NEIGHBOR_DATA *tneigh;

            tarea = neigh->address;
            for( tneigh = tarea->weather->first_neighbor; tneigh; tneigh = tneigh->next )
            {
               if( !strcmp( area->name, tneigh->name ) )
                  break;
            }

            UNLINK( tneigh, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
            STRFREE( tneigh->name );
            DISPOSE( tneigh );
         }

         UNLINK( neigh, area->weather->first_neighbor, area->weather->last_neighbor, next, prev );
         ch_printf( ch, "The weather in %s and %s no longer affect each other.\r\n", neigh->name, area->name );
         STRFREE( neigh->name );
         DISPOSE( neigh );
      }
      else /* add an entry */
      {
         if( !( tarea = get_area( argument ) ) )
         {
            send_to_char( "No such area exists.\r\n", ch );
            return;
         }
         else if( tarea == area )
         {
            ch_printf( ch, "%s already affects its own weather.\r\n", area->name );
            return;
         }

         /* add the entry */
         CREATE( neigh, NEIGHBOR_DATA, 1 );
         neigh->name = STRALLOC( tarea->name );
         neigh->address = tarea;
         LINK( neigh, area->weather->first_neighbor, area->weather->last_neighbor, next, prev );

         /* add an entry to the neighbor's list */
         CREATE( neigh, NEIGHBOR_DATA, 1 );
         neigh->name = STRALLOC( area->name );
         neigh->address = area;
         LINK( neigh, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );

         ch_printf( ch, "The weather in %s and %s now affect one another.\r\n", tarea->name, area->name );
      }
      return;
   }
   else
   {
      send_to_char( "Climate may only be followed by one of the following fields:\r\n", ch );
      send_to_char( "  temp\r\n", ch );
      send_to_char( "  precip\r\n", ch );
      send_to_char( "  wind\r\n", ch );
      send_to_char( "  neighbor\r\n", ch );
      return;
   }
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelCreate( relation_type tp, void *actor, void *subject )
{
   REL_DATA *tmp;

   if( tp < relMSET_ON || tp > relOSET_ON )
   {
      bug( "%s: invalid type (%d)", __FUNCTION__, tp );
      return;
   }
   if( !actor )
   {
      bug( "%s: NULL actor", __FUNCTION__ );
      return;
   }
   if( !subject )
   {
      bug( "%s: NULL subject", __FUNCTION__ );
      return;
   }
   for( tmp = first_relation; tmp; tmp = tmp->next )
   {
      if( tmp->Type == tp && tmp->Actor == actor && tmp->Subject == subject )
      {
         bug( "%s: duplicated relation", __FUNCTION__ );
         return;
      }
   }
   CREATE( tmp, REL_DATA, 1 );
   tmp->Type = tp;
   tmp->Actor = actor;
   tmp->Subject = subject;
   LINK( tmp, first_relation, last_relation, next, prev );
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelDestroy( relation_type tp, void *actor, void *subject )
{
   REL_DATA *rq;

   if( tp < relMSET_ON || tp > relOSET_ON )
   {
      bug( "%s: invalid type (%d)", __FUNCTION__, tp );
      return;
   }
   if( !actor )
   {
      bug( "%s: NULL actor", __FUNCTION__ );
      return;
   }
   if( !subject )
   {
      bug( "%s: NULL subject", __FUNCTION__ );
      return;
   }
   for( rq = first_relation; rq; rq = rq->next )
   {
      if( rq->Type == tp && rq->Actor == actor && rq->Subject == subject )
      {
         UNLINK( rq, first_relation, last_relation, next, prev );
         DISPOSE( rq );
         break;
      }
   }
}

CMDF( do_rdig )
{
   ROOM_INDEX_DATA *location = NULL, *in_room;
   EXIT_DATA *pexit;
   int edir, start, end, i;

   if( !ch || is_npc( ch ) || !ch->in_room )
      return;

   if( get_trust( ch ) < PERM_BUILDER || !ch->pcdata->area )
   {
      send_to_char( "You aren't able to use rdig.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: rdig <direction>\r\n", ch );
      return;
   }

   if( ch->pcdata->area )
   {
      start = ch->pcdata->area->low_vnum;
      end = ch->pcdata->area->hi_vnum;
   }
   else
   {
      start = ch->pcdata->range_lo;
      end = ch->pcdata->range_hi;
   }

   /* Get what way this will be going */
   edir = get_dir( argument );
   in_room = ch->in_room;
   if( ( pexit = get_exit( in_room, edir ) ) )
   {
      send_to_char( "You can't dig that way because an exit already exist there.\r\n", ch );
      return;
   }

   /* See if we can make a new room */
   for( i = start; i <= end; i++ )
   {
      if( !get_room_index( i ) )
      {
         if( !( location = make_room( i, ch->pcdata->area ) ) )
         {
            bug( "%s: make_room failed", __FUNCTION__ );
            return;
         }
         break; /* Made a new room so break out */
      }
   }

   if( !location )
   {
      send_to_char( "You don't have any vnums free for use.\r\n", ch );
      return;
   }

   location->area = ch->pcdata->area;
   STRSET( location->name, in_room->name );
   STRSET( location->description, in_room->description );
   xSET_BITS( location->room_flags, in_room->room_flags );
   location->light = in_room->light;
   location->sector_type = in_room->sector_type;

   pexit = make_exit( in_room, location, edir );
   pexit->keyword = NULL;
   pexit->description = NULL;
   pexit->key = -1;
   xCLEAR_BITS( pexit->exit_info );
   xCLEAR_BITS( pexit->base_info );

   pexit = make_exit( location, in_room, rev_dir[ edir ] );
   pexit->keyword = NULL;
   pexit->description = NULL;
   pexit->key = -1;
   xCLEAR_BITS( pexit->exit_info );
   xCLEAR_BITS( pexit->base_info );

   send_to_char( "Waving your hand, you form order from swirling chaos,\r\nand step into a new reality...\r\n", ch );
   char_from_room( ch );
   char_to_room( ch, location );
   do_look( ch, (char *)"auto" );
}

CMDF( do_aexit )
{
   AREA_DATA *tarea, *otherarea;
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *room;
   int i, vnum, lrange, trange;
   bool found = false;

   if( !ch )
      return;

   if( !ch->in_room )
   {
      send_to_char( "You're in a NULL room!\r\n", ch );
      return;
   }

   if( !( tarea = ch->in_room->area ) )
   {
      send_to_char( "You're in a NULL area!\r\n", ch );
      return;
   }

   pager_printf( ch, "\r\nExits leading to %s.\r\n", tarea->filename );
   for( otherarea = first_area; otherarea; otherarea = otherarea->next )
   {
      if( tarea == otherarea )
         continue;

      trange = otherarea->hi_vnum;
      lrange = otherarea->low_vnum;

      for( vnum = lrange; vnum <= trange; vnum++ )
      {
         if( !( room = get_room_index( vnum ) ) )
            continue;
         if( xIS_SET( room->room_flags, ROOM_TELEPORT ) )
         {
            if( room->tele_vnum >= tarea->low_vnum && room->tele_vnum <= tarea->hi_vnum )
            {
               pager_printf( ch, "   From: %5d To  : %5d (Teleport)\r\n", vnum, room->tele_vnum );
               found = true;
            }
         }
         for( i = 0; i < DIR_MAX; i++ )
         {
            if( !( pexit = get_exit( room, i ) ) )
               continue;
            if( pexit->to_room->area == tarea )
            {
               pager_printf( ch, "   From: %5d To  : %5d (%s)\r\n", vnum, pexit->vnum, dir_name[i] );
               found = true;
            }
         }
      }
   }

   if( !found )
      pager_printf( ch, "   There are no exits leading to %s.\r\n", tarea->filename );

   found = false;

   trange = tarea->hi_vnum;
   lrange = tarea->low_vnum;

   pager_printf(ch, "\r\nExits leading from %s.\r\n", tarea->filename);
   for( vnum = lrange; vnum <= trange; vnum++ )
   {
      if( !( room = get_room_index( vnum ) ) )
         continue;

      if( xIS_SET( room->room_flags, ROOM_TELEPORT ) && ( room->tele_vnum < lrange || room->tele_vnum > trange ) )
      {
         pager_printf( ch, "   To  : %5d From: %5d (Teleport)\r\n", room->tele_vnum, vnum );
         found = true;
      }

      for( i = 0; i < DIR_MAX; i++ )
      {
         if( !( pexit = get_exit( room, i ) ) )
            continue;
         if( pexit->to_room->area != tarea )
         {
            pager_printf(ch, "   To  : %5d From: %5d (%s)\r\n", pexit->vnum, vnum, dir_name[i] );
            found = true;
         }
      }
   }

   if( !found )
      pager_printf( ch, "   There are no exits leading from %s.\r\n", tarea->filename );
}

/* This function is set to remove a character from deities, clans and councils by name alone */
void remove_from_everything( const char *name )
{
   DEITY_DATA *deity, *deity_next;
   CLAN_DATA *clan, *clan_next;
   COUNCIL_DATA *council, *council_next;

   /* Check all deities and all members and remove where you can */
   for( deity = first_deity; deity; deity = deity_next )
   {
      deity_next = deity->next;

      if( remove_deity_worshipper( deity, name ) )
         save_deity( deity );
   }

   for( clan = first_clan; clan; clan = clan_next )
   {
      clan_next = clan->next;

      if( remove_clan_member( clan, name ) )
         save_clan( clan );
   }

   for( council = first_council; council; council = council_next )
   {
      council_next = council->next;

      if( remove_council_member( council, name ) )
         save_council( council );
   }

   remove_from_highscores( name );
   remove_from_banks( name );
}
