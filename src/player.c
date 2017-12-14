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
 * 		Commands for personal player settings/statictics	     *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "h/mud.h"

extern int top_explorer;

CMDF( do_speed )
{
   int size;

   if( !is_npc( ch ) )
   {
      send_to_char( "You can't change your speed.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What would you like to set your speed to? Valid Range is 1 - 32.\r\n", ch );
      return;
   }
   if( ( size = atoi( argument ) ) < 1 || size > 32 )
   {
      send_to_char( "Invalid speed. Valid Range is 1 - 32.\r\n", ch );
      return;
   }
   ch->pcdata->speed = size;
   if( ch->desc )
      ch->desc->speed = ch->pcdata->speed;
   ch_printf( ch, "Your speed is now set to %d which is %dBytes.\r\n", ch->pcdata->speed, ( 128 * ch->pcdata->speed ) );
}

/* This part will take the stat and see if it should get an increase or not */
void handle_stat( CHAR_DATA *ch, short stat, bool silence, short chance )
{
   int difck;
   short max;

   if( stat < 0 || stat >= STAT_MAX )
   {
      bug( "%s: trying to increase stat [%d] that isn't there?", __FUNCTION__, stat );
      return;
   }

   max = UMIN( ch->level, ( ( is_npc( ch ) || is_immortal( ch ) ) ? ( MAX_LEVEL + 25 ) : MAX_LEVEL ) );

   /* Race max */
   if( !is_npc( ch ) && !is_immortal( ch ) && race_table[ch->race] )
      max = UMIN( max, race_table[ch->race]->max_stats[stat] );

   if( ch->perm_stats[stat] >= max )
   {
      if( !silence )
         ch_printf( ch, "You're already at your max %s for now.\r\n", stattypes[stat] );
      return;
   }

   difck = ( max - ch->perm_stats[stat] ); /* How much are they lacking from possible max */
   if( number_percent( ) >= difck ) /* If above max continue, if max is 100 and they have 15, 85% chance of getting past this part */
      return;
   if( number_percent( ) > chance ) /* Always a good chance of it failing to make it hard on them */
      return;

   ch->perm_stats[stat] = URANGE( 1, ( ch->perm_stats[stat] + 1 ), max );
   /* Increase other things when stats are increased */
   if( stat == STAT_CON )
   {
      ch->max_hit = UMAX( ch->max_hit, ( ch->max_hit + 5 ) );
      ch->max_move = UMAX( ch->max_move, ( ch->max_move + 1 ) );
      if( is_vampire( ch ) )
         ch->max_mana = UMAX( ch->max_mana, ( ch->max_mana + 2 ) );
   }
   if( stat == STAT_DEX )
   {
      ch->max_move = UMAX( ch->max_move, ( ch->max_move + 5 ) );
      if( is_vampire( ch ) )
         ch->max_mana = UMAX( ch->max_mana, ( ch->max_mana + 1 ) );
   }
   if( stat == STAT_INT && !is_vampire( ch ) )
      ch->max_mana = UMAX( ch->max_mana, ( ch->max_mana + 2 ) );
   if( stat == STAT_WIS && !is_vampire( ch ) )
      ch->max_mana = UMAX( ch->max_mana, ( ch->max_mana + 1 ) );
   if( stat == STAT_STR )
      ch->max_hit = UMAX( ch->max_hit, ( ch->max_hit + 2 ) );
   if( stat == STAT_LCK )
   {
      if( number_percent( ) > 80 )
         ch->max_hit = UMAX( ch->max_hit, ( ch->max_hit + 2 ) );
      if( number_percent( ) > 60 )
         ch->max_move = UMAX( ch->max_move, ( ch->max_move + 1 ) );
      if( number_percent( ) > 50 )
         ch->max_mana = UMAX( ch->max_mana, ( ch->max_mana + 1 ) );
   }
   ch_printf( ch, "You're now up to %d %s.\r\n", ch->perm_stats[stat], stattypes[stat] );
}

CMDF( do_pushup )
{
   int moveloss = 10;

   switch( ch->substate )
   {
      default:
         if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         if( ch->in_room->sector_type == SECT_AIR )
         {
            send_to_char( "What?  In the air?!\r\n", ch );
            return;
         }
         if( ch->perm_stats[STAT_STR] >= ch->level )
         {
            send_to_char( "You're already as strong as you can currently get.\r\n", ch );
            return;
         }
         if( ch->move < moveloss )
         {
            send_to_char( "You're to exhausted to do pushups.\r\n", ch );
            return;
         }
         add_timer( ch, TIMER_DO_FUN, number_range( 2, 4 ), do_pushup, 1 );
         act( AT_ACTION, "You begin doing pushups...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n begins doing pushups...", ch, NULL, NULL, TO_ROOM );
         ch->move -= moveloss;
         return;

      case 1:
         ch->substate = SUB_NONE;
         act( AT_ACTION, "You finish doing pushups.", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n finishes doing pushups.", ch, NULL, NULL, TO_ROOM );
         handle_stat( ch, STAT_STR, false, 3 );
         return;

      case SUB_TIMER_DO_ABORT:
         ch->substate = SUB_NONE;
         act( AT_ACTION, "You stop doing pushups...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n stops doing pushups...", ch, NULL, NULL, TO_ROOM );
         return;
   }
}

CMDF( do_situp )
{
   int moveloss = 10;

   switch( ch->substate )
   {
      default:
         if( is_npc( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         if( ch->in_room->sector_type == SECT_AIR )
         {
            send_to_char( "What?  In the air?!\r\n", ch );
            return;
         }
         if( ch->perm_stats[STAT_CON] >= ch->level )
         {
            send_to_char( "You're already at your max constitution for now.\r\n", ch );
            return;
         }
         if( ch->move < moveloss )
         {
            send_to_char( "You're to exhausted to do situps.\r\n", ch );
            return;
         }
         add_timer( ch, TIMER_DO_FUN, number_range( 2, 4 ), do_situp, 1 );
         act( AT_ACTION, "You begin doing situps...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n begins doing situps...", ch, NULL, NULL, TO_ROOM );
         ch->move -= moveloss;
         return;

      case 1:
         ch->substate = SUB_NONE;
         act( AT_ACTION, "You finish doing situps.", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n finishes doing situps.", ch, NULL, NULL, TO_ROOM );
         handle_stat( ch, STAT_CON, false, 3 );
         return;

      case SUB_TIMER_DO_ABORT:
         ch->substate = SUB_NONE;
         act( AT_ACTION, "You stop doing situps...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n stops doing situps...", ch, NULL, NULL, TO_ROOM );
         return;
   }
}

CMDF( do_gold )
{
   ch_printf( ch, "&[gold]You have %s gold pieces.&D\r\n", show_char_gold( ch ) );
}

CMDF( do_worth )
{
   AFFECT_DATA *paf;
   SKILLTYPE *skill;
   const char *s1, *s2, *person;
   int count = 0;

   if( is_npc( ch ) )
      return;

   s1 = color_str( AT_SCORE, ch );
   s2 = color_str( AT_SCORE2, ch );
   person = color_str( AT_PERSON, ch );
   if( ch->sex == SEX_MALE )
      person = color_str( AT_MALE, ch );
   if( ch->sex == SEX_FEMALE )
      person = color_str( AT_FEMALE, ch );

   for( paf = ch->first_affect; paf; paf = paf->next )
   {
      if( ( skill = get_skilltype( paf->type ) ) )
         count++;
   }

   pager_printf( ch, "\r\n%sWorth for %s%s%s.\r\n", s1, person, ch->name, ch->pcdata->title );
   pager_printf( ch, "%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", s1 );
   pager_printf( ch, "%sLevel:   %s%-5d %sFavor:   %s%-10d %sAlignment: %s%-10d %sGlory: %s%-13u\r\n",
      s1, s2, ch->level, s1, s2, !ch->pcdata->deity ? 0 : ch->pcdata->favor, s1, s2, ch->alignment, s1, s2, ch->pcdata->quest_curr );
   pager_printf( ch, "%sItems:   %s%-5d %sWeight:  %s%-10d %sStyle:     %s%-10s %sGold:  %s%-13s\r\n",
      s1, s2, ch->carry_number, s1, s2, ch->carry_weight, s1, s2, style_names[ch->style], s1, s2, show_char_gold( ch ) );
   pager_printf( ch, "%sHitroll: %s%-5d %sDamroll: %s%-10d %sAffects:   %s%-10d\r\n",
      s1, s2, get_hitroll( ch ), s1, s2, get_damroll( ch ), s1, s2, count );
   pager_printf( ch, "%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.\r\n", s1 );
}

/* Affects-at-a-glance, Blodkai */
CMDF( do_affected )
{
   AFFECT_DATA *paf;
   SKILLTYPE *skill;
   int count = 0, col = 0;
   bool fby = false, found = false;

   set_char_color( AT_SCORE, ch );
   if( !xIS_EMPTY( ch->affected_by ) )
      send_to_char("&[dgray]-=&[gray][ &[white] Summary &[white]]&[gray]=-&D\r\n", ch);
   if( !xIS_EMPTY( ch->affected_by ) )
      ch_printf( ch, "   &[blue]Affects:     &[lblue]%s&D\r\n", ext_flag_string( &ch->affected_by, a_flags ) );

   for( count = 0; count < RIS_MAX; count++ )
   {
      if( ch->resistant[count] != 0 )
      {
         if( !found )
            send_to_char( "   &[blue]Resistant: ", ch );
         else if( col == 0 )
            send_to_char( "              ", ch );
         found = true;
         ch_printf( ch, "&[lblue]%12s %4d&D", ris_flags[count], ch->resistant[count] );
         if( ++col == 3 )
         {
            col = 0;
            send_to_char( "\r\n", ch );
         }
      }
   }
   if( col != 0 )
      send_to_char( "\r\n", ch );
   count = 0;
   col = 0;

   if( !str_cmp( argument, "by" ) )
      fby = true;
   if( ch->first_affect )
   {
      send_to_char( "\r\n", ch );
      for( paf = ch->first_affect; paf; paf = paf->next )
      {
         if( ( skill = get_skilltype( paf->type ) ) )
         {
            count++;
            if( fby )
            {
               ch_printf( ch, "&[blue]%2d&[dgray]> &[blue]%-18.18s ",
                  count, skill->name );
               if( ( paf->location % REVERSE_APPLY ) == APPLY_STAT )
                  ch_printf( ch, "&[white]- &[pink]Modifies &[gray]%-14.14s &[blue]", ext_flag_string( &paf->bitvector, stattypes ) );
               else if( ( paf->location % REVERSE_APPLY ) == APPLY_RESISTANT )
                  ch_printf( ch, "&[white]- &[pink]Modifies &[gray]%-14.14s &[blue]", ext_flag_string( &paf->bitvector, ris_flags ) );
               else
                  ch_printf( ch, "&[white]- &[pink]Modifies &[gray]%-14.14s &[blue]", a_types[paf->location % REVERSE_APPLY] );

               if( ( paf->location % REVERSE_APPLY ) == APPLY_EXT_AFFECT )
               {
                  if( paf->modifier == 0 )
                     ch_printf( ch, " by &[yellow]%-14s", ext_flag_string( &paf->bitvector, a_flags ) );
                  else if( paf->modifier > 0 && paf->modifier < AFF_MAX )
                     ch_printf( ch, " by &[yellow]%-14s", a_flags[paf->modifier] );
                  else
                     ch_printf( ch, " by &[yellow]%-14d", paf->modifier );
               }
               else
                  ch_printf( ch, " by &[yellow]%-14d", paf->modifier );
               ch_printf( ch, " &[blue]for &[red]%5d &[blue]Seconds &D\r\n", paf->duration );
            }
            else
            {
               if( col != 0 )
                  send_to_char( " ", ch );
               ch_printf( ch, "&[blue]%18.18s&[gray][&[white]%4d&[gray]]",
                  skill->name, paf->duration );
               if( ++col == 3 )
               {
                  col = 0;
                  send_to_char( "\r\n", ch );
               }
            }
         }
      }
   }

   if( col != 0 )
      send_to_char( "\r\n", ch );
   if( count == 0 )
      send_to_char( "&[white]No cantrip or skill affects you.&D\r\n\r\n", ch );
   else
      ch_printf( ch, "&[white]Total of &[red]%d&[white] affects.&D\r\n", count );

   if( ch->saving_spell_staff != 0 || ch->saving_para_petri != 0 || ch->saving_wand != 0
   || ch->saving_poison_death != 0 || ch->saving_breath != 0 )
      send_to_char( "\r\n&[dgray]-=&[gray][ &[white]Saves vs Spells &[gray]]&[dgray]=-&D\r\n\r\n", ch );
   if( ch->saving_spell_staff != 0 )
      ch_printf(ch, "&[red]* &[gray]Spell Staff   &[white]- &[green]%d&D\r\n", ch->saving_spell_staff );
   if( ch->saving_para_petri != 0 )
      ch_printf(ch, "&[red]* &[gray]Paralysis     &[white]- &[green]%d&D\r\n", ch->saving_para_petri );
   if( ch->saving_wand != 0 )
      ch_printf(ch, "&[red]* &[gray]Wands         &[white]- &[green]%d&D\r\n", ch->saving_wand );
   if( ch->saving_poison_death != 0 )
      ch_printf(ch, "&[red]* &[gray]Petrification &[white]- &[green]%d&D\r\n", ch->saving_poison_death );
   if( ch->saving_breath != 0 )
      ch_printf(ch, "&[red]* &[gray]Breath        &[white]- &[green]%d&D\r\n&D", ch->saving_breath );
}

CMDF( do_inventory )
{
   set_char_color( AT_RED, ch );
   ch_printf( ch, "You're carrying: Items: %d/%d Weight: %d/%d\r\n",
      ch->carry_number, can_carry_n( ch ), ch->carry_weight, can_carry_w( ch ) );
   show_list_to_char( ch->first_carrying, ch, true, true );
}

CMDF( do_equipment )
{
   OBJ_DATA *obj, *topobj;
   int iWear, cond, start;
   bool found = false;

   set_char_color( AT_RED, ch );
   send_to_char( "You're using:\r\n", ch );
   set_char_color( AT_OBJECT, ch );
   for( iWear = 0; iWear < WEAR_MAX; iWear++ )
   {
      topobj = get_eq_char( ch, iWear );

      for( obj = ch->first_carrying; obj; obj = obj->next_content )
      {
         if( obj->wear_loc == iWear )
         {
            if( !is_obj_stat( obj, ITEM_LODGED ) )
            {
               if( ( !is_npc( ch ) ) && ( ch->race > 0 ) && ( ch->race < MAX_PC_RACE )
               && race_table[ch->race] && race_table[ch->race]->where_name[iWear] )
                  send_to_char( race_table[ch->race]->where_name[iWear], ch );
               else
                  send_to_char( where_name[iWear], ch );
            }
            else
            {
               if( ( !is_npc( ch ) ) && ( ch->race > 0 ) && ( ch->race < MAX_PC_RACE )
               && race_table[ch->race] && race_table[ch->race]->lodge_name[iWear] )
                  send_to_char( race_table[ch->race]->lodge_name[iWear], ch );
               else
                  send_to_char( lodge_name[iWear], ch );
            }

            if( can_see_obj( ch, obj ) )
            {
               switch( obj->item_type )
               {
                  case ITEM_LIGHT:
                  case ITEM_AXE:
                  case ITEM_LOCKPICK:
                  case ITEM_SHOVEL:
                  case ITEM_ARMOR:
                     if( obj->value[0] > 0 && obj->value[1] > 0 )
                        cond = (int) ( ( 10 * obj->value[0] / obj->value[1] ) );
                     else
                        cond = -1;
                     break;

                  case ITEM_MISSILE_WEAPON:
                  case ITEM_WEAPON:
                     if( obj->value[0] > 0 )
                        cond = (int) ( ( 10 * obj->value[0] / INIT_WEAPON_CONDITION ) );
                     else
                        cond = -1;
                     break;

                  case ITEM_WAND:
                  case ITEM_STAFF:
                     if( obj->value[1] > 0 && obj->value[2] > 0 )
                        cond = (int) ( ( 10 * obj->value[2] / obj->value[1] ) );
                     else
                        cond = -1;
                     break;

                  case ITEM_CONTAINER:
                  case ITEM_KEYRING:
                  case ITEM_QUIVER:
                     if( obj->value[3] > 0 && obj->value[4] > 0 )
                        cond = (int) ( ( 10 * obj->value[3] / obj->value[4] ) );
                     else
                        cond = -1;
                     break;

                  default:
                     cond = -1;
                     break;
               }
               send_to_char( "&C<&R", ch );
               if( obj == topobj )
               {
                  if( cond >= 0 )
                  {
                     for( start = 1; start <= 10; start++ )
                     {
                        if( start <= cond )
                           send_to_char( "+", ch );
                        else
                           send_to_char( " ",ch );
                     }
                  }
                  else if( is_obj_stat( obj, ITEM_LODGED ) )
                     send_to_char( "--LODGED--", ch );
                  else
                     send_to_char( "----N/A---", ch );
               }
               else if( is_obj_stat( obj, ITEM_LODGED ) )
                  send_to_char( "--LODGED--", ch );
               else /* Not the top object so its layered */
                  send_to_char( "---LAYER--", ch );
               ch_printf(ch, "&C>&D%s  ", "&[objects]" );

               send_to_char( format_obj_to_char( obj, ch, true ), ch );
               if( obj->bsplatter )
                  send_to_char( " &RBloody&D", ch );
               if( obj->bstain )
                  send_to_char( " &RStained&D", ch );
               send_to_char( "\r\n", ch );
            }
            else
               send_to_char( "something.\r\n", ch );
            found = true;
         }
      }
   }

   if( !found )
      send_to_char( "Nothing.\r\n", ch );
}

void set_title( CHAR_DATA *ch, char *title )
{
   char buf[MSL];

   if( is_npc( ch ) )
      return;

   if( isalpha( title[0] ) || isdigit( title[0] ) )
   {
      buf[0] = ' ';
      mudstrlcpy( buf + 1, title, sizeof( buf ) - 1 );
   }
   else
      mudstrlcpy( buf, title, sizeof( buf ) );

   STRSET( ch->pcdata->title, buf );
}

CMDF( do_title )
{
   if( is_npc( ch ) )
      return;

   set_char_color( AT_SCORE, ch );
   if( ch->level < 5 )
   {
      send_to_char( "Sorry... you must be at least level 5 to set your title...\r\n", ch );
      return;
   }
   if( xIS_SET( ch->pcdata->flags, PCFLAG_NOTITLE ) )
   {
      set_char_color( AT_IMMORT, ch );
      send_to_char( "The Gods prohibit you from changing your title.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Change your title to what?\r\n", ch );
      return;
   }

   if( strlen( argument ) > 50 )
      argument[50] = '\0';

   set_title( ch, argument );
   send_to_char( "Title has been set.\r\n", ch );
}

CMDF( do_homepage )
{
   char buf[MSL];

   if( is_npc( ch ) )
      return;

   if( ch->level < 5 )
   {
      send_to_char( "Sorry... you must be at least level 5 to do that.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      char *homepage = ch->pcdata->homepage ? ch->pcdata->homepage : ( char * )"";
      ch_printf( ch, "Your homepage is: %s\r\n", show_tilde( homepage ) );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      STRFREE( ch->pcdata->homepage );
      send_to_char( "Homepage cleared.\r\n", ch );
      return;
   }

   if( strstr( argument, "://" ) )
      mudstrlcpy( buf, argument, sizeof( buf ) );
   else
      snprintf( buf, sizeof( buf ), "http://%s", argument );
   if( strlen( buf ) > 70 )
      buf[70] = '\0';

   hide_tilde( buf );
   STRSET( ch->pcdata->homepage, buf );
   send_to_char( "Homepage set.\r\n", ch );
}

CMDF( do_email )
{
   char buf[MSL];

   if( is_npc( ch ) )
      return;

   if( ch->level < 5 )
   {
      send_to_char( "Sorry... you must be at least level 5 to do that.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      char *email = ch->pcdata->email ? ch->pcdata->email : ( char * )"";
      ch_printf( ch, "Your email is: %s\r\n", show_tilde( email ) );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      STRFREE( ch->pcdata->email );
      send_to_char( "Email cleared.\r\n", ch );
      return;
   }

   mudstrlcpy( buf, argument, sizeof( buf ) );
   if( strlen( buf ) > 70 )
      buf[70] = '\0';

   hide_tilde( buf );
   STRSET( ch->pcdata->email, buf );
   send_to_char( "Email set.\r\n", ch );
}

CMDF( do_msn )
{
   char buf[MSL];

   if( is_npc( ch ) )
      return;

   if( ch->level < 5 )
   {
      send_to_char( "Sorry... you must be at least level 5 to do that.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      char *msn = ch->pcdata->msn ? ch->pcdata->msn : ( char * )"";
      ch_printf( ch, "Your msn is: %s\r\n", show_tilde( msn ) );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      STRFREE( ch->pcdata->msn );
      send_to_char( "Msn cleared.\r\n", ch );
      return;
   }

   mudstrlcpy( buf, argument, sizeof( buf ) );
   if( strlen( buf ) > 70 )
      buf[70] = '\0';

   hide_tilde( buf );
   STRSET( ch->pcdata->msn, buf );
   send_to_char( "Msn set.\r\n", ch );
}

CMDF( do_yahoo )
{
   char buf[MSL];

   if( is_npc( ch ) )
      return;

   if( ch->level < 5 )
   {
      send_to_char( "Sorry... you must be at least level 5 to do that.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      char *yahoo = ch->pcdata->yahoo ? ch->pcdata->yahoo : ( char * )"";
      ch_printf( ch, "Your yahoo is: %s\r\n", show_tilde( yahoo ) );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      STRFREE( ch->pcdata->yahoo );
      send_to_char( "Yahoo cleared.\r\n", ch );
      return;
   }

   mudstrlcpy( buf, argument, sizeof( buf ) );
   if( strlen( buf ) > 70 )
      buf[70] = '\0';

   hide_tilde( buf );
   STRSET( ch->pcdata->yahoo, buf );
   send_to_char( "Yahoo set.\r\n", ch );
}

CMDF( do_gtalk )
{
   char buf[MSL];

   if( is_npc( ch ) )
      return;

   if( ch->level < 5 )
   {
      send_to_char( "Sorry... you must be at least level 5 to do that.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      char *gtalk = ch->pcdata->gtalk ? ch->pcdata->gtalk : ( char * )"";
      ch_printf( ch, "Your gtalk is: %s\r\n", show_tilde( gtalk ) );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      STRFREE( ch->pcdata->gtalk );
      send_to_char( "GTalk cleared.\r\n", ch );
      return;
   }

   mudstrlcpy( buf, argument, sizeof( buf ) );
   if( strlen( buf ) > 70 )
      buf[70] = '\0';

   hide_tilde( buf );
   STRSET( ch->pcdata->gtalk, buf );
   send_to_char( "GTalk set.\r\n", ch );
}

/* Set your personal description - Thoric */
CMDF( do_description )
{
   if( is_npc( ch ) )
   {
      send_to_char( "Monsters are too dumb to do that!\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   switch( ch->substate )
   {
      default:
         bug( "%s: illegal substate", __FUNCTION__ );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_DESC;
         ch->dest_buf = ch;
         start_editing( ch, ch->description );
         return;

      case SUB_PERSONAL_DESC:
         STRFREE( ch->description );
         ch->description = copy_buffer( ch );
         stop_editing( ch );
         return;
   }
}

/* Ripped off do_description for whois bio's -- Scryn*/
CMDF( do_bio )
{
   if( is_npc( ch ) )
   {
      send_to_char( "Mobs can't set a bio.\r\n", ch );
      return;
   }
   if( ch->level < 5 )
   {
      set_char_color( AT_SCORE, ch );
      send_to_char( "You must be at least level five to write your bio...\r\n", ch );
      return;
   }
   if( !ch->desc )
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   switch( ch->substate )
   {
      default:
         bug( "%s: illegal substate", __FUNCTION__ );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You can't use this command from within another command.\r\n", ch );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_BIO;
         ch->dest_buf = ch;
         start_editing( ch, ch->pcdata->bio );
         return;

      case SUB_PERSONAL_BIO:
         STRFREE( ch->pcdata->bio );
         ch->pcdata->bio = copy_buffer( ch );
         stop_editing( ch );
         return;
   }
}

CMDF( do_score )
{
   MCLASS_DATA *mclass;
   const char *s1, *s2;
   int stat;

   if( is_npc( ch ) )
      return;

   s1 = color_str( AT_SCORE, ch );
   s2 = color_str( AT_SCORE2, ch );

   set_pager_color( AT_SCORE, ch );
   pager_printf( ch, "\r\n%sScore for %s%s%s.\r\n", s1,
      ch->sex == SEX_MALE ? color_str( AT_MALE, ch ) : ch->sex == SEX_FEMALE ? color_str( AT_FEMALE, ch ) : "",
      ch->name, ( !is_npc( ch ) && ch->pcdata->title ) ? ch->pcdata->title : "" );

   if( get_trust( ch ) >= PERM_IMM )
      pager_printf( ch, "%sYou're a trusted %s%s.\r\n", s1, s2, perms_flag[get_trust( ch )] );
   if( ch->pcdata->spouse )
      pager_printf( ch, "%sYou're married to %s%s.\r\n", s1, s2, ch->pcdata->spouse );

   set_pager_color( AT_SCORE, ch );

   if( !is_npc( ch ) && ch->pcdata->first_explored && top_explorer > 0 )
   {
      EXP_DATA *fexp;
      int cnt = 0;

      for( fexp = ch->pcdata->first_explored; fexp; fexp = fexp->next )
         cnt++; /* The more of them they find the more exp they gain */

      if( cnt > 0 )
         pager_printf( ch, "%sYou have found %s%d%s of %s%d%s rooms.\r\n", s1, s2, cnt, s1, s2, top_explorer, s1 );
   }

   if( !is_npc( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
      send_to_pager( "You're drunk.\r\n", ch );
   if( !is_npc( ch ) && ch->pcdata->condition[COND_THIRST] == 0 )
      send_to_pager( "You're in danger of dehydrating.\r\n", ch );
   if( !is_npc( ch ) && ch->pcdata->condition[COND_FULL] == 0 )
      send_to_pager( "You're starving to death.\r\n", ch );
   if( ch->position != POS_SLEEPING )
   {
      switch( ch->mental_state / 10 )
      {
         default:
            send_to_pager( "You're completely messed up!\r\n", ch );
            break;
         case -10:
            send_to_pager( "You're barely conscious.\r\n", ch );
            break;
         case -9:
            send_to_pager( "You can barely keep your eyes open.\r\n", ch );
            break;
         case -8:
            send_to_pager( "You're extremely drowsy.\r\n", ch );
            break;
         case -7:
            send_to_pager( "You feel very unmotivated.\r\n", ch );
            break;
         case -6:
            send_to_pager( "You feel sedated.\r\n", ch );
            break;
         case -5:
            send_to_pager( "You feel sleepy.\r\n", ch );
            break;
         case -4:
            send_to_pager( "You feel tired.\r\n", ch );
            break;
         case -3:
            send_to_pager( "You could use a rest.\r\n", ch );
            break;
         case -2:
            send_to_pager( "You feel a little under the weather.\r\n", ch );
            break;
         case -1:
            send_to_pager( "You feel fine.\r\n", ch );
            break;
         case 0:
            send_to_pager( "You feel great.\r\n", ch );
            break;
         case 1:
            send_to_pager( "You feel energetic.\r\n", ch );
            break;
         case 2:
            send_to_pager( "Your mind is racing.\r\n", ch );
            break;
         case 3:
            send_to_pager( "You can't think straight.\r\n", ch );
            break;
         case 4:
            send_to_pager( "Your mind is going 100 miles an hour.\r\n", ch );
            break;
         case 5:
            send_to_pager( "You're high as a kite.\r\n", ch );
            break;
         case 6:
            send_to_pager( "Your mind and body are slipping apart.\r\n", ch );
            break;
         case 7:
            send_to_pager( "Reality is slipping away.\r\n", ch );
            break;
         case 8:
            send_to_pager( "You have no idea what is real, and what is not.\r\n", ch );
            break;
         case 9:
            send_to_pager( "You feel immortal.\r\n", ch );
            break;
         case 10:
            send_to_pager( "You're a Supreme Entity.\r\n", ch );
            break;
      }
   }
   else if( ch->mental_state > 45 )
      send_to_pager( "Your sleep is filled with strange and vivid dreams.\r\n", ch );
   else if( ch->mental_state > 25 )
      send_to_pager( "Your sleep is uneasy.\r\n", ch );
   else if( ch->mental_state < -35 )
      send_to_pager( "You're deep in a much needed sleep.\r\n", ch );
   else if( ch->mental_state < -25 )
      send_to_pager( "You're in deep slumber.\r\n", ch );

   if( ch->pcdata->bestowments )
      pager_printf( ch, "%sYou're bestowed with the command(s): %s%s.\r\n", s1, s2, ch->pcdata->bestowments );

   if( ch->pcdata->birth_year )
      pager_printf( ch, "%s%s\r\n", s2, show_birthday( ch ) );

   pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`%sCharacter Data%s`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );

   pager_printf( ch, "%sLevel: %s%-13d %sRace:  %s%-13.13s %sPlayed: ",
      s1, s2, ch->level, s1, s2, capitalize( dis_race_name( ch->race ) ), s1 );
   {
      int tmptime, time = ch->played;

      tmptime = ( time / 3600 );
      time -= ( tmptime * 3600 );
      pager_printf( ch, "%s%d%s:", s2, tmptime, s1 ); /* Hours */
      tmptime = ( time / 60 );
      time -= ( tmptime * 60 );
      pager_printf( ch, "%s%2.2d%s:", s2, tmptime, s1 ); /* Minutes */
      tmptime = time;
      pager_printf( ch, "%s%2.2d\r\n", s2, tmptime ); /* Seconds */
   }

   pager_printf( ch, "%sAge:   %s%-13d                      %sLog In: %s%s\r\n",
      s1, s2, get_age( ch ), s1, s2, distime( ch->logon ) );

   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      pager_printf( ch, "%sLevel: %s%-13d %sClass: %s%-13.13s", s1, s2, mclass->level, s1, s2, capitalize( dis_class_name( mclass->wclass ) ) );
      if( mclass->level < MAX_LEVEL )
         pager_printf( ch, " %sExp:    %s%.f", s1, s2, mclass->exp );
      send_to_pager( "\r\n", ch );
   }

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      pager_printf( ch, "%s%3.3s:   %s%-13d", s1, capitalize( stattypes[stat] ), s2, get_curr_stat( stat, ch ) );
      if( stat == 0 )
         pager_printf( ch, " %sHR:    %s%-13d %sSaved:  %s%s\r\n",
            s1, s2, get_hitroll( ch ), s1, s2, ch->save_time ? distime( ch->save_time ) : "no save this session" );
      else if( stat == 1 )
         pager_printf( ch, " %sDR:    %s%-13d %sTime:   %s%s\r\n",
            s1, s2, get_damroll( ch ), s1, s2, distime( current_time ) );
      else if( stat == 2 )
         pager_printf( ch, " %sArmor: %s%-13d\r\n", s1, s2, get_ac( ch ) );
      else if( stat == 3 )
         pager_printf( ch, " %sAlign: %s%+-13d %sItems:  %s%d %s(max %s%d%s)\r\n",
            s1, s2, ch->alignment, s1, s2, ch->carry_number, s1, s2, can_carry_n( ch ), s1 );
      else if( stat == 4 )
         pager_printf( ch, " %sWimpy: %s%-13d %sWeight: %s%d %s(max %s%d%s)\r\n",
            s1, s2, ch->wimpy, s1, s2, ch->carry_weight, s1, s2, can_carry_w( ch ), s1 );
      else if( stat == 5 )
         pager_printf( ch, " %sGlory: %s%u\r\n", s1, s2, ch->pcdata->quest_curr );
      else if( stat == 6 )
         pager_printf( ch, " %sPract: %s%d\r\n", s1, s2, ch->practice );
   }

   pager_printf( ch, "%sPos'n: %s%-13s ", s1, s2, pos_names[ch->position] );

   pager_printf( ch, "%sHP:    %s%d %sof %s%d\r\n", s1, s2, ch->hit, s1, s2, ch->max_hit );

   pager_printf( ch, "%sStyle: %s%-13s %s", s1, s2, style_names[ch->style], s1 );

   if( is_vampire( ch ) )
      send_to_pager( "Blood: ", ch );
   else
      send_to_pager( "Mana:  ", ch );
   pager_printf( ch, "%s%d %sof %s%d\r\n", s2, ch->mana, s1, s2, ch->max_mana );

   pager_printf( ch, "%sGold:  %s%-13s %sMove:  %s%d %sof %s%d\r\n",
      s1, s2, show_char_gold( ch ), s1, s2, ch->move, s1, s2, ch->max_move );

   if( xIS_SET( ch->act, PLR_QUESTOR ) || ch->pcdata->questcompleted != 0 )
   {
      MOB_INDEX_DATA *victim, *vindex;
      OBJ_INDEX_DATA *oindex;
      char *name;

      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.%sQuest Data%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );
      pager_printf( ch, "%sYou have completed %s%d%s.\r\n", s1, s2, ch->pcdata->questcompleted, s1 );
      if( xIS_SET( ch->act, PLR_QUESTOR ) )
      {
         if( ( vindex = get_mob_index( ch->questgiver ) ) )
         {
            name = vindex->short_descr;
            name[0] = UPPER( name[0] );
            pager_printf( ch, "%s%s %swants you to ", s2, name, s1 );
            if( ch->questtype == 1 && ( oindex = get_obj_index( ch->questvnum ) ) )
               pager_printf( ch, "bring %s %s%s%s.\r\n", him_her[vindex->sex], s2, oindex->short_descr, s1 );
            if( ch->questtype == 2 && ( victim = get_mob_index( ch->questvnum ) ) )
            {
               name = victim->short_descr;
               pager_printf( ch, "kill %s%s %sfor %s.\r\n", s2, name, s1, him_her[vindex->sex] );
            }
         }
         pager_printf( ch, "%sYou have %s%d %sminute%s left to complete the quest.\r\n", s1, s2, ch->questcountdown, s1, ch->questcountdown != 1 ? "s" : "" );
      }
   }

   if( ch->pcdata && ch->pcdata->first_pet )
   {
      CHAR_DATA *pet;

      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.%sPet   Data%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );
      for( pet = ch->pcdata->first_pet; pet; pet = pet->next_pet )
      {
         pager_printf( ch, "%s%s%s: %s%s\r\n", s2, capitalize( PERS( pet, ch ) ), s1, s2,
            !pet->in_room ? "Unknown" : ( pet->in_room == ch->in_room ) ? "In the room with you" : pet->in_room->name );
      }
   }

   if( ch->morph && ch->morph->morph )
   {
      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.%sMorph Data%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );

      if( is_immortal( ch ) )
         pager_printf( ch, "%sMorphed as (%s%d%s) %s%s %swith a timer of %s%d.\r\n",
            s1, s2, ch->morph->morph->vnum, s1, s2, ch->morph->morph->short_desc, s1, s2, ch->morph->timer );
      else
         pager_printf( ch, "%sYou're morphed into a %s%s.\r\n", s1, s2, ch->morph->morph->short_desc );
   }

   if( ch->pcdata->mkills || ch->pcdata->mdeaths || ch->pcdata->pkills || ch->pcdata->pdeaths )
   {
      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.%sKill  Data%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );
      if( ch->pcdata->mkills )
         pager_printf( ch, "%sMKills: %s%u", s1, s2, ch->pcdata->mkills );
      if( ch->pcdata->mdeaths )
         pager_printf( ch, "%s%sMDeaths: %s%u", s1, ch->pcdata->mkills ? "  " : "", s2, ch->pcdata->mdeaths );
      if( ch->pcdata->pkills )
         pager_printf( ch, "%s%sPKills: %s%u", s1, ( ch->pcdata->mkills || ch->pcdata->mdeaths ) ? "  " : "",
            s2, ch->pcdata->pkills );
      if( ch->pcdata->pdeaths )
         pager_printf( ch, "%s%sPDeaths: %s%u",
            s1, ( ch->pcdata->mkills || ch->pcdata->mdeaths || ch->pcdata->pkills ) ? "  " : "",
            s2, ch->pcdata->pdeaths );
      send_to_pager( "\r\n", ch );
   }
   if( ch->pcdata->deity )
   {
      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.%sDeity Data%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );
      pager_printf( ch, "%s%-15.15s     %sFavor: %s%d\r\n", s2, ch->pcdata->deity->name, s1, s2, ch->pcdata->favor );
   }
   if( ch->pcdata->clan )
   {
      CLAN_DATA *clan = ch->pcdata->clan;

      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.%sClan  Data%s.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );
      pager_printf( ch, "%s%-15.15s     %sMembers: %s%d\r\n", s2, clan->name, s1, s2, clan->members );
   }
   if( ch->pcdata->nation )
   {
      CLAN_DATA *nation = ch->pcdata->nation;

      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~%sNation  Data%s~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );
      pager_printf( ch, "%s%-15.15s     %sMembers: %s%d\r\n", s2, nation->name, s1, s2, nation->members );
   }
   if( is_immortal( ch ) )
   {
      pager_printf( ch, "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`%sImmortal  Data%s`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n", s1, s2, s1 );

      pager_printf( ch, "%sWizinvis [%s%s%s]  Wizlevel (%s%d%s)\r\n",
         s1, s2, xIS_SET( ch->act, PLR_WIZINVIS ) ? "X" : " ", s1, s2, ch->pcdata->wizinvis, s1 );

      pager_printf( ch, "%sBamfin:  %s%s %s\r\n", s1, s2, ch->name,
         ch->pcdata->bamfin ? ch->pcdata->bamfin : "appears in a swirling mist." );
      pager_printf( ch, "%sBamfout: %s%s %s\r\n", s1, s2, ch->name,
         ch->pcdata->bamfout ? ch->pcdata->bamfout : "leaves in a swirling mist." );

      if( ch->pcdata->area )
      {
         pager_printf( ch, "%sVnums: (%s%-5.5d %s- %s%-5.5d%s)\r\n",
            s1, s2, ch->pcdata->area->low_vnum, s1, s2, ch->pcdata->area->hi_vnum, s1 );
         pager_printf( ch, "%sArea Loaded [%s%s%s]\r\n",
            s1, s2, ( IS_SET( ch->pcdata->area->status, AREA_LOADED ) ) ? "yes" : "no", s1 );
      }
   }
   send_to_pager( "\r\n", ch );
}

CMDF( do_stat )
{
   int stat, col = 0;

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   ch_printf( ch, "You report: %d/%d hp %d/%d %s %d/%d mv.\r\n",
      ch->hit, ch->max_hit, ch->mana, ch->max_mana, is_vampire( ch ) ? "blood" : "mana", ch->move, ch->max_move );

   send_to_char( "Stats: ", ch );
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      ch_printf( ch, "%3.3s(%d)", capitalize( stattypes[stat] ), get_curr_stat( stat, ch ) );
      if( ++col == 7 )
      {
         col = 0;
         send_to_char( "\r\n", ch );
      }
      else
         send_to_char( " ", ch );
   }
   if( col != 0 )
      send_to_char( "\r\n", ch );
}

CMDF( do_statreport )
{
   char buf[MSL];
   int stat, col = 0;

   if( is_npc( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   do_stat( ch, (char *)"" );

   snprintf( buf, sizeof( buf ), "$n reports: %d/%d hp %d/%d %s %d/%d mv.",
      ch->hit, ch->max_hit, ch->mana, ch->max_mana, is_vampire( ch ) ? "blood" : "mana", ch->move, ch->max_move );
   act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );

   snprintf( buf, sizeof( buf ), "%s", "$n's Stats: " );
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), "%3.3s(%d)", capitalize( stattypes[stat] ), get_curr_stat( stat, ch ) );
      if( ++col == 7 )
      {
         col = 0;
         mudstrlcat( buf, "\r\n", sizeof( buf ) );
      }
      else
         mudstrlcat( buf, " ", sizeof( buf ) );
   }
   if( col != 0 )
      mudstrlcat( buf, "\r\n", sizeof( buf ) );
   act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
}

CMDF( do_report )
{
   char buf[MIL];

   if( is_npc( ch ) && ch->fighting )
      return;

   ch_printf( ch, "You report: %d/%d hp %d/%d %s %d/%d mv.\r\n",
      ch->hit, ch->max_hit, ch->mana, ch->max_mana, is_vampire( ch ) ? "blood" : "mana",
      ch->move, ch->max_move );

   snprintf( buf, sizeof( buf ), "$n reports: %d/%d hp %d/%d %s %d/%d mv.\r\n",
      ch->hit, ch->max_hit, ch->mana, ch->max_mana, is_vampire( ch ) ? "blood" : "mana",
      ch->move, ch->max_move );
   act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
}

CMDF( do_fprompt )
{
   char arg[MIL];

   set_char_color( AT_GRAY, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "NPC's can't change their prompt..\r\n", ch );
      return;
   }
   one_argument( argument, arg );
   if( !*arg || !str_cmp( arg, "display" ) )
   {
      send_to_char( "Your current fighting prompt string:\r\n", ch );
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "%s\r\n", !ch->pcdata->fprompt ? default_fprompt( ch ) : ch->pcdata->fprompt );
      set_char_color( AT_GRAY, ch );
      send_to_char( "Type 'help prompt' for information on changing your prompt.\r\n", ch );
      return;
   }
   send_to_char( "Replacing old prompt of:\r\n", ch );
   set_char_color( AT_WHITE, ch );
   ch_printf( ch, "%s\r\n", !ch->pcdata->fprompt ? default_fprompt( ch ) : ch->pcdata->fprompt );

   STRFREE( ch->pcdata->fprompt );
   if( strlen( argument ) > 128 )
      argument[128] = '\0';

   if( str_cmp( arg, "default" ) )
      ch->pcdata->fprompt = STRALLOC( argument );
}

CMDF( do_prompt )
{
   char arg[MIL];

   set_char_color( AT_GRAY, ch );

   if( is_npc( ch ) )
   {
      send_to_char( "NPC's can't change their prompt..\r\n", ch );
      return;
   }
   one_argument( argument, arg );
   if( !*arg || !str_cmp( arg, "display" ) )
   {
      send_to_char( "Your current prompt string:\r\n", ch );
      set_char_color( AT_WHITE, ch );
      ch_printf( ch, "%s\r\n", !ch->pcdata->prompt ? default_prompt( ch ) : ch->pcdata->prompt );
      set_char_color( AT_GRAY, ch );
      send_to_char( "Type 'help prompt' for information on changing your prompt.\r\n", ch );
      return;
   }
   send_to_char( "Replacing old prompt of:\r\n", ch );
   set_char_color( AT_WHITE, ch );
   ch_printf( ch, "%s\r\n", !ch->pcdata->prompt ? default_prompt( ch ) : ch->pcdata->prompt );

   STRFREE( ch->pcdata->prompt );
   if( strlen( argument ) > 128 )
      argument[128] = '\0';

   if( str_cmp( arg, "default" ) )
      ch->pcdata->prompt = STRALLOC( argument );
}
