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
 *                          Shaddai's Polymorph                              *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"

#define ONLY_PKILL      1
#define ONLY_PEACEFULL  2

CMDF( do_morphstat );
void unmorph_all( MORPH_DATA *morph );

#define MKEY( literal, field, value )  \
   if( !str_cmp( word, literal ) )     \
   {                                   \
      STRFREE( field );                \
      field = value;                   \
      fMatch = true;                   \
      break;                           \
   }

MORPH_DATA *morph_start = NULL, *morph_end = NULL;
int morph_vnum = 0;

/* Given the Morph's name, returns the pointer to the morph structure. --Shaddai */
MORPH_DATA *get_morph( char *arg )
{
   MORPH_DATA *morph = NULL;

   if( !arg || arg[0] == '\0' )
      return NULL;
   for( morph = morph_start; morph; morph = morph->next )
      if( !str_cmp( morph->name, arg ) )
         break;
   return morph;
}

/* Given the Morph's vnum, returns the pointer to the morph structure. --Shaddai */
MORPH_DATA *get_morph_vnum( int vnum )
{
   MORPH_DATA *morph = NULL;

   if( vnum < 1 )
      return NULL;
   for( morph = morph_start; morph; morph = morph->next )
      if( morph->vnum == vnum )
         break;
   return morph;
}

/*
 * Checks to see if the char meets all the requirements to morph into
 * the provided morph.  Doesn't Look at NPC's class or race as they
 * are different from pc's, but still checks level and if they can
 * cast it or not.  --Shaddai
 */
bool can_morph( CHAR_DATA *ch, MORPH_DATA *morph, bool is_cast )
{
   MCLASS_DATA *mclass;

   if( !morph )
      return false;
   if( is_immortal( ch ) || is_npc( ch ) )   /* Let immortals morph to anything Also NPC can do any morph  */
      return true;
   if( morph->no_cast && is_cast )
      return false;
   if( ch->level < morph->level )
      return false;
   if( morph->pkill == ONLY_PKILL && !is_pkill( ch ) )
      return false;
   if( morph->pkill == ONLY_PEACEFULL && is_pkill( ch ) )
      return false;
   if( morph->sex != -1 && morph->sex != ch->sex )
      return false;
   if( !xIS_EMPTY( morph->Class ) && !is_npc( ch ) )
   {
      for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
         if( mclass->wclass >= 0 && xIS_SET( morph->Class, mclass->wclass ) )
            return false;
   }
   if( !xIS_EMPTY( morph->race ) && !is_npc( ch ) && xIS_SET( morph->race, ch->race ) )
      return false;
   if( morph->deity && ( !ch->pcdata->deity || !get_deity( morph->deity ) || ch->pcdata->deity != get_deity( morph->deity ) ) )
      return false;
   if( morph->timeto != -1 && morph->timefrom != -1 )
   {
      int tmp, i;
      bool found = false;

      /* i is a sanity check, just in case things go haywire so it doesn't loop forever here. -Shaddai */
      for( i = 0, tmp = morph->timefrom; i < 25 && tmp != morph->timeto; i++ )
      {
         if( tmp == time_info.hour )
         {
            found = true;
            break;
         }
         if( tmp == 23 )
            tmp = 0;
         else
            tmp++;
      }
      if( !found )
         return false;
   }
   if( morph->dayfrom != -1 && morph->dayto != -1
   && ( morph->dayto < ( time_info.day + 1 ) || morph->dayfrom > ( time_info.day + 1 ) ) )
      return false;
   return true;
}

/* Find a morph you can use -- Shaddai */
MORPH_DATA *find_morph( CHAR_DATA *ch, char *target, bool is_cast )
{
   MORPH_DATA *morph = NULL;

   if( !target || target[0] == '\0' )
      return NULL;
   for( morph = morph_start; morph; morph = morph->next )
   {
      if( str_cmp( morph->name, target ) )
         continue;
      if( can_morph( ch, morph, is_cast ) )
         break;
   }
   return morph;
}

void show_morphs( CHAR_DATA *ch )
{
   MORPH_DATA *morph = NULL;
   bool found = false;
   int col = 0;

   if( !ch )
      return;
   for( morph = morph_start; morph; morph = morph->next )
   {
      if( !can_morph( ch, morph, true ) )
         continue;
      found = true;
      ch_printf( ch, "%20s", morph->name );
      if( ++col == 4 )
      {
         col = 0;
         send_to_char( "\r\n", ch );
      }
      else
         send_to_char( " ", ch );
   }
   if( !found )
      send_to_char( "There is nothing you can currently polymorph into.\r\n", ch );
   else if( col != 0 )
      send_to_char( "\r\n", ch );
}

/* 
 * Write one morph structure to a file. It doesn't print the variable to file
 * if it hasn't been set why waste disk-space :)  --Shaddai
 */
void fwrite_morph( FILE *fp, MORPH_DATA *morph )
{
   int stat;

   if( !morph )
      return;
   fprintf( fp, "Morph                  %s\n", morph->name );
   if( morph->obj[0] || morph->obj[1] || morph->obj[2] )
      fprintf( fp, "Objs                %d %d %d\n", morph->obj[0], morph->obj[1], morph->obj[2] );
   if( morph->objuse[0] || morph->objuse[1] || morph->objuse[2] )
      fprintf( fp, "Objuse              %d %d %d\n", morph->objuse[0], morph->objuse[1], morph->objuse[2] );
   if( morph->vnum )
      fprintf( fp, "Vnum                %d\n", morph->vnum );
   if( morph->damroll )
      fprintf( fp, "Damroll             %s~\n", morph->damroll );
   if( morph->defpos != POS_STANDING )
      fprintf( fp, "Defpos              %d\n", morph->defpos );
   if( morph->description )
      fprintf( fp, "Description         %s~\n", morph->description );
   if( morph->help )
      fprintf( fp, "Help                %s~\n", morph->help );
   if( morph->hit )
      fprintf( fp, "Hit                 %s~\n", morph->hit );
   if( morph->hitroll )
      fprintf( fp, "Hitroll             %s~\n", morph->hitroll );
   if( morph->key_words )
      fprintf( fp, "Keywords            %s~\n", morph->key_words );
   if( morph->long_desc )
      fprintf( fp, "Longdesc            %s~\n", morph->long_desc );
   if( morph->mana )
      fprintf( fp, "Mana                %s~\n", morph->mana );
   if( morph->morph_other )
      fprintf( fp, "MorphOther          %s~\n", morph->morph_other );
   if( morph->morph_self )
      fprintf( fp, "MorphSelf           %s~\n", morph->morph_self );
   if( morph->move )
      fprintf( fp, "Move                %s~\n", morph->move );
   if( morph->no_skills )
      fprintf( fp, "NoSkills            %s~\n", morph->no_skills );
   if( morph->short_desc )
      fprintf( fp, "ShortDesc           %s~\n", morph->short_desc );
   if( morph->skills )
      fprintf( fp, "Skills              %s~\n", morph->skills );
   if( morph->unmorph_other )
      fprintf( fp, "UnmorphOther        %s~\n", morph->unmorph_other );
   if( morph->unmorph_self )
      fprintf( fp, "UnmorphSelf         %s~\n", morph->unmorph_self );
   for( stat = 0; stat < RIS_MAX; stat++ )
      if( morph->resistant[stat] != 0 )
         fprintf( fp, "NResistant       %d %s~\n", morph->resistant[stat], ris_flags[stat] );
   if( !xIS_EMPTY( morph->affected_by ) )
      fprintf( fp, "Affected            %s~\n", ext_flag_string( &morph->affected_by, a_flags ) );
   if( !xIS_EMPTY( morph->no_affected_by ) )
      fprintf( fp, "NoAffected          %s~\n", ext_flag_string( &morph->no_affected_by, a_flags ) );
   if( !xIS_EMPTY( morph->Class ) )
      fprintf( fp, "Class               %s~\n", ext_class_string( &morph->Class ) );
   if( !xIS_EMPTY( morph->race ) )
      fprintf( fp, "Race                %s~\n", ext_race_string( &morph->race ) );
   if( morph->timer )
      fprintf( fp, "Timer               %d\n", morph->timer );
   if( morph->used )
      fprintf( fp, "Used                %d\n", morph->used );
   if( morph->sex != -1 )
      fprintf( fp, "Sex                 %d\n", morph->sex );
   if( morph->pkill )
      fprintf( fp, "Pkill               %d\n", morph->pkill );
   if( morph->timefrom != -1 )
      fprintf( fp, "TimeFrom            %d\n", morph->timefrom );
   if( morph->timeto != -1 )
      fprintf( fp, "TimeTo              %d\n", morph->timeto );
   if( morph->dayfrom != -1 )
      fprintf( fp, "DayFrom             %d\n", morph->dayfrom );
   if( morph->dayto != -1 )
      fprintf( fp, "DayTo               %d\n", morph->dayto );
   if( morph->manaused )
      fprintf( fp, "ManaUsed            %d\n", morph->manaused );
   if( morph->moveused )
      fprintf( fp, "MoveUsed            %d\n", morph->moveused );
   if( morph->hpused )
      fprintf( fp, "HpUsed              %d\n", morph->hpused );
   if( morph->favorused )
      fprintf( fp, "FavorUsed          %d\n", morph->favorused );
   if( morph->ac )
      fprintf( fp, "Armor               %d\n", morph->ac );
   for( stat = 0; stat < STAT_MAX; stat++ )
      if( morph->stats[stat] != 0 )
         fprintf( fp, "NStat               %d %s~\n", morph->stats[stat], stattypes[stat] ); 
   if( morph->dodge )
      fprintf( fp, "Dodge               %d\n", morph->dodge );
   if( morph->level )
      fprintf( fp, "Level               %d\n", morph->level );
   if( morph->parry )
      fprintf( fp, "Parry               %d\n", morph->parry );
   if( morph->saving_breath )
      fprintf( fp, "SaveBreath          %d\n", morph->saving_breath );
   if( morph->saving_para_petri )
      fprintf( fp, "SavePara            %d\n", morph->saving_para_petri );
   if( morph->saving_poison_death )
      fprintf( fp, "SavePoison          %d\n", morph->saving_poison_death );
   if( morph->saving_spell_staff )
      fprintf( fp, "SaveSpell           %d\n", morph->saving_spell_staff );
   if( morph->saving_wand )
      fprintf( fp, "SaveWand            %d\n", morph->saving_wand );
   if( morph->tumble )
      fprintf( fp, "Tumble              %d\n", morph->tumble );
   if( morph->no_cast )
      fprintf( fp, "NoCast              %d\n", morph->no_cast );
   fprintf( fp, "%s", "End\n\n" );
}

/*
 * This function saves the morph data.  Should be put in on reboot or shutdown
 * to make use of the sort algorithim. --Shaddai
 */
void save_morphs( bool autosavemorph )
{
   MORPH_DATA *morph;
   FILE *fp;

   /* Might as well make morph_opt more useable */
   if( autosavemorph && !sysdata.morph_opt )
      return;

   if( !( fp = fopen( MORPH_FILE, "w" ) ) )
   {
      bug( "%s: can't open %s for writing.", __FUNCTION__, MORPH_FILE );
      perror( MORPH_FILE );
      return;
   }
   for( morph = morph_start; morph; morph = morph->next )
      fwrite_morph( fp, morph );
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

/*
 *  Command used to set all the morphing information.
 *  Must use the morphset save command, to write the commands to file.
 *  Hp/Mana/Move/Hitroll/Damroll can be set using variables such
 *  as 1d2+10.  No boundry checks are in place yet on those, so care must
 *  be taken when using these.  --Shaddai
 */
CMDF( do_morphset )
{
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MSL];
   char *origarg = argument;
   int value, stat;
   MORPH_DATA *morph = NULL;

   set_char_color( AT_PLAIN, ch );
   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't morphset\r\n", ch );
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

      case SUB_MORPH_DESC:
         if( !ch->dest_buf )
         {
            send_to_char( "Fatal error: report to Someone.\r\n", ch );
            bug( "%s: sub_morph_desc: NULL ch->dest_buf", __FUNCTION__ );
            ch->substate = SUB_NONE;
            return;
         }
         morph = ( MORPH_DATA * ) ch->dest_buf;
         STRFREE( morph->description );
         morph->description = copy_buffer( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         if( ch->substate == SUB_REPEATCMD )
            ch->dest_buf = morph;
         save_morphs( true );
         return;

      case SUB_MORPH_HELP:
         if( !ch->dest_buf )
         {
            send_to_char( "Fatal error: report to Someone.\r\n", ch );
            bug( "%s: sub_morph_help: NULL ch->dest_buf", __FUNCTION__ );
            ch->substate = SUB_NONE;
            return;
         }
         morph = ( MORPH_DATA * ) ch->dest_buf;
         STRFREE( morph->help );
         morph->help = copy_buffer( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         if( ch->substate == SUB_REPEATCMD )
            ch->dest_buf = morph;
         save_morphs( true );
         return;
   }
   morph = NULL;

   if( ch->substate == SUB_REPEATCMD )
   {
      if( !( morph = ( MORPH_DATA * ) ch->dest_buf ) )
      {
         send_to_char( "Someone deleted your morph!\r\n", ch );
         argument = (char *)"done";
      }
      if( !argument || argument[0] == '\0' )
      {
         do_morphstat( ch, morph->name );
         return;
      }
      if( !str_cmp( argument, "stat" ) )
      {
         mudstrlcpy( buf, morph->name, sizeof( buf ) );
         mudstrlcat( buf, " help", sizeof( buf ) );
         do_morphstat( ch, buf );
         return;
      }
      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         send_to_char( "Morphset mode off.\r\n", ch );
         ch->substate = SUB_NONE;
         ch->dest_buf = NULL;
         STRFREE( ch->pcdata->subprompt );
         return;
      }
   }
   if( morph )
   {
      mudstrlcpy( arg1, morph->name, sizeof( arg1 ) );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, sizeof( arg3 ) );
   }
   else
   {
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      mudstrlcpy( arg3, argument, sizeof( arg3 ) );
   }
   if( !str_cmp( arg1, "on" ) )
   {
      send_to_char( "Usage: morphset <morph> on.\r\n", ch );
      return;
   }
   value = is_number( arg3 ) ? atoi( arg3 ) : -1;

   if( atoi( arg3 ) < -1 && value == -1 )
      value = atoi( arg3 );

   if( ch->substate != SUB_REPEATCMD && arg1[0] != '\0' && !str_cmp( arg1, "save" ) )
   {
      save_morphs( false );
      send_to_char( "Morph data saved.\r\n", ch );
      return;
   }
   if( arg1[0] == '\0' || ( arg2[0] == '\0' && ch->substate != SUB_REPEATCMD ) || !str_cmp( arg1, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( morph )
            send_to_char( "Usage: <field> <value>\r\n", ch );
         else
            send_to_char( "Usage: <morph> <field> <value>\r\n", ch );
      }
      else
         send_to_char( "Usage: morphset <morph> <field> <value>\r\n", ch );
      send_to_char( "Usage: morphset save\r\n", ch );
      send_to_char( "&cField being one of:\r\n", ch );
      send_to_char( "--------------------------------------------------------------------\r\n", ch );
      send_to_char( "&w    ac   obj2   deity   skills    objuse3     morphself\r\n", ch );
      send_to_char( "    hp   obj3   dodge   timeto   affected     resistant\r\n", ch );
      send_to_char( "   sex   race   level   tumble   charisma    morphother\r\n", ch );
      send_to_char( "  help   sav1   parry   wisdom   manaused   description\r\n", ch );
      send_to_char( "  long   sav2   pkill  damroll   moveused   unmorphself\r\n", ch );
      send_to_char( "  luck   sav3   short  dayfrom   noskills  constitution\r\n", ch );
      send_to_char( "  mana   sav4   timer  hitroll   strength  intelligence\r\n", ch );
      send_to_char( "  move   sav5  defpos  keyword   timefrom  unmorphother\r\n", ch );
      send_to_char( "  name  class  hpused  objuse1  dexterity\r\n", ch );
      send_to_char( "  obj1  dayto  nocast  objuse2  favorused\r\n", ch );
      send_to_char( "&c--------------------------------------------------------------------&D\r\n", ch );
      return;
   }

   if( !morph )
   {
      if( !is_number( arg1 ) )
         morph = get_morph( arg1 );
      else
         morph = get_morph_vnum( atoi( arg1 ) );

      if( !morph )
      {
         send_to_char( "That morph does not exist.\r\n", ch );
         return;
      }
   }
   if( !str_cmp( arg2, "on" ) )
   {
      if( check_subrestricted( ch ) )
         return;
      ch_printf( ch, "Morphset mode on. (Editing %s).\r\n", morph->name );
      ch->substate = SUB_REPEATCMD;
      ch->dest_buf = morph;
      STRFREE( ch->pcdata->subprompt );
      snprintf( buf, sizeof( buf ), "<&CMorphset &W%s&w> %%i", morph->name );
      ch->pcdata->subprompt = STRALLOC( buf );
      return;
   }

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( !str_cmp( arg2, stattypes[stat] ) )
      {
         if( value < -10 || value > 10 )
         {
            ch_printf( ch, "%s range is -10 to +10.\r\n", capitalize( stattypes[stat] ) );
            return;
         }
         morph->stats[stat] = value;
         save_morphs( true );
         ch_printf( ch, "%s set to %d\r\n", capitalize( stattypes[stat] ), value );
         return;
      }
   }

   if( !str_cmp( arg2, "defpos" ) )
   {
      if( value < 0 || value > POS_STANDING )
      {
         ch_printf( ch, "Position range is 0 to %d.\r\n", POS_STANDING );
         return;
      }
      morph->defpos = value;
   }
   else if( !str_cmp( arg2, "sex" ) )
   {
      value = atoi( argument );

      if( value < 0 || value >= SEX_MAX )
      {
         send_to_char( "Invalid sex.\r\n", ch );
         return;
      }
      morph->sex = value;
   }
   else if( !str_cmp( arg2, "pkill" ) )
   {
      if( !str_cmp( arg3, "pkill" ) )
         morph->pkill = ONLY_PKILL;
      else if( !str_cmp( arg3, "peace" ) )
         morph->pkill = ONLY_PEACEFULL;
      else if( !str_cmp( arg3, "none" ) )
         morph->pkill = 0;
      else
      {
         send_to_char( "Usuage: morphset <morph> pkill [pkill|peace|none]\r\n", ch );
         return;
      }
   }
   else if( !str_cmp( arg2, "manaused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         send_to_char( "Mana used is a value from 0 to 2000.\r\n", ch );
         return;
      }
      morph->manaused = value;
   }
   else if( !str_cmp( arg2, "moveused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         send_to_char( "Move used is a value from 0 to 2000.\r\n", ch );
         return;
      }
      morph->moveused = value;
   }
   else if( !str_cmp( arg2, "hpused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         send_to_char( "Hp used is a value from 0 to 2000.\r\n", ch );
         return;
      }
      morph->hpused = value;
   }
   else if( !str_cmp( arg2, "favorused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         send_to_char( "Favor used is a value from 0 to 2000.\r\n", ch );
         return;
      }
      morph->favorused = value;
   }
   else if( !str_cmp( arg2, "timeto" ) )
   {
      if( value < 0 || value > 23 )
      {
         send_to_char( "Timeto is a value from 0 to 23.\r\n", ch );
         return;
      }
      morph->timeto = value;
   }
   else if( !str_cmp( arg2, "timefrom" ) )
   {
      if( value < 0 || value > 23 )
      {
         send_to_char( "Timefrom is a value from 0 to 23.\r\n", ch );
         return;
      }
      morph->timefrom = value;
   }
   else if( !str_cmp( arg2, "dayto" ) )
   {
      if( value < 0 || value > 29 )
      {
         send_to_char( "Dayto is a value from 0 to 29.\r\n", ch );
         return;
      }
      morph->dayto = value;
   }
   else if( !str_cmp( arg2, "dayfrom" ) )
   {
      if( value < 0 || value > 29 )
      {
         send_to_char( "Dayfrom is a value from 0 to 29.\r\n", ch );
         return;
      }
      morph->dayfrom = value;
   }
   else if( !str_cmp( arg2, "sav1" ) || !str_cmp( arg2, "savepoison" ) )
   {
      if( value < -30 || value > 30 )
      {
         send_to_char( "Saving poison range is -30 to 30.\r\n", ch );
         return;
      }
      morph->saving_poison_death = value;
   }
   else if( !str_cmp( arg2, "sav2" ) || !str_cmp( arg2, "savewand" ) )
   {
      if( value < -30 || value > 30 )
      {
         send_to_char( "Saving wand range is -30 to 30.\r\n", ch );
         return;
      }
      morph->saving_wand = value;
   }
   else if( !str_cmp( arg2, "sav3" ) || !str_cmp( arg2, "savepara" ) )
   {
      if( value < -30 || value > 30 )
      {
         send_to_char( "Saving para range is -30 to 30.\r\n", ch );
         return;
      }
      morph->saving_para_petri = value;
   }
   else if( !str_cmp( arg2, "sav4" ) || !str_cmp( arg2, "savebreath" ) )
   {
      if( value < -30 || value > 30 )
      {
         send_to_char( "Saving breath range is -30 to 30.\r\n", ch );
         return;
      }
      morph->saving_breath = value;
   }
   else if( !str_cmp( arg2, "sav5" ) || !str_cmp( arg2, "savestaff" ) )
   {
      if( value < -30 || value > 30 )
      {
         send_to_char( "Saving staff range is -30 to 30.\r\n", ch );
         return;
      }
      morph->saving_spell_staff = value;
   }
   else if( !str_cmp( arg2, "timer" ) )
   {
      if( value < -1 || value == 0 )
      {
         send_to_char( "Timer must be -1 (None) or greater than 0.\r\n", ch );
         return;
      }
      morph->timer = value;
   }
   else if( !str_cmp( arg2, "hp" ) )
   {
      argument = one_argument( argument, arg3 );
      STRFREE( morph->hit );
      if( str_cmp( arg3, "0" ) )
         morph->hit = STRALLOC( arg3 );
   }
   else if( !str_cmp( arg2, "mana" ) )
   {
      argument = one_argument( argument, arg3 );
      STRFREE( morph->mana );
      if( str_cmp( arg3, "0" ) )
         morph->mana = STRALLOC( arg3 );
   }
   else if( !str_cmp( arg2, "move" ) )
   {
      argument = one_argument( argument, arg3 );
      STRFREE( morph->move );
      if( str_cmp( arg3, "0" ) )
         morph->move = STRALLOC( arg3 );
   }
   else if( !str_cmp( arg2, "ac" ) )
   {
      if( value > 500 || value < -500 )
      {
         send_to_char( "Ac range is -500 to 500.\r\n", ch );
         return;
      }
      morph->ac = value;
   }
   else if( !str_cmp( arg2, "hitroll" ) )
   {
      argument = one_argument( argument, arg3 );
      STRFREE( morph->hitroll );
      if( str_cmp( arg3, "0" ) )
         morph->hitroll = STRALLOC( arg3 );
   }
   else if( !str_cmp( arg2, "damroll" ) )
   {
      argument = one_argument( argument, arg3 );
      STRFREE( morph->damroll );
      if( str_cmp( arg3, "0" ) )
         morph->damroll = STRALLOC( arg3 );
   }
   else if( !str_cmp( arg2, "dodge" ) )
   {
      if( value > 100 || value < -100 )
      {
         send_to_char( "Dodge range is -100 to 100.\r\n", ch );
         return;
      }
      morph->dodge = value;
   }
   else if( !str_prefix( "obj", arg2 ) )
   {
      int oindex;
      char temp[MSL];

      if( arg2[3] == '\0' )
      {
         send_to_char( "Obj 1, 2, or 3.\r\n", ch );
         return;
      }
      temp[0] = arg2[3];
      temp[1] = '\0';
      oindex = atoi( temp );
      if( oindex > 3 || oindex < 1 )
      {
         send_to_char( "Obj 1, 2, or 3.\r\n", ch );
         return;
      }
      if( !( get_obj_index( value ) ) )
      {
         send_to_char( "No such object is currently using that vnum.\r\n", ch );
         return;
      }
      morph->obj[oindex - 1] = value;
   }
   else if( !str_cmp( arg2, "parry" ) )
   {
      if( value > 100 || value < -100 )
      {
         send_to_char( "Parry range is -100 to 100.\r\n", ch );
         return;
      }
      morph->parry = value;
   }
   else if( !str_cmp( arg2, "tumble" ) )
   {
      if( value > 100 || value < -100 )
      {
         send_to_char( "Tumble range is -100 to 100.\r\n", ch );
         return;
      }
      morph->tumble = value;
   }
   else if( !str_cmp( arg2, "level" ) )
   {
      if( value < 0 || value > MAX_LEVEL )
      {
         ch_printf( ch, "Level range is 0 to %d.\r\n", MAX_LEVEL );
         return;
      }
      morph->level = value;
   }
   else if( !str_prefix( arg2, "objuse" ) )
   {
      int oindex;
      char temp[MIL];

      if( arg2[6] == '\0' )
      {
         send_to_char( "Objuse 1, 2 or 3?\r\n", ch );
         return;
      }
      temp[0] = arg2[6];
      temp[1] = '\0';
      oindex = atoi( temp );
      if( oindex > 3 || oindex < 1 )
      {
         send_to_char( "Objuse 1, 2, or 3?\r\n", ch );
         return;
      }
      if( value )
         morph->objuse[oindex - 1] = true;
      else
         morph->objuse[oindex - 1] = false;
   }
   else if( !str_cmp( arg2, "nocast" ) )
   {
      if( value )
         morph->no_cast = true;
      else
         morph->no_cast = false;
   }
   else if( !str_cmp( arg2, "resistant" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: morphset <morph> resistant <resistant> <#>\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      value = get_flag( arg3, ris_flags, RIS_MAX );
      if( value < 0 || value >= RIS_MAX )
         ch_printf( ch, "Unknown ris: %s\r\n", arg3 );
      else
         morph->resistant[value] = atoi( argument );
   }
   else if( !str_cmp( arg2, "affected" ) || !str_cmp( arg2, "aff" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: morphset <morph> affected <flag> [flag]...\r\n", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, a_flags, AFF_MAX );
         if( value < 0 || value >= AFF_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
         else
            xTOGGLE_BIT( morph->affected_by, value );
      }
   }
   else if( !str_cmp( arg2, "noaffected" ) || !str_cmp( arg2, "naff" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: morphset <morph> noaffected <flag> [flag]...\r\n", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, a_flags, AFF_MAX );
         if( value < 0 || value >= AFF_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
         else
            xTOGGLE_BIT( morph->no_affected_by, value );
      }
   }
   else if( !str_cmp( arg2, "short" ) )
   {
      STRSET( morph->short_desc, arg3 );
   }
   else if( !str_cmp( arg2, "morphother" ) )
   {
      STRSET( morph->morph_other, arg3 );
   }
   else if( !str_cmp( arg2, "morphself" ) )
   {
      STRSET( morph->morph_self, arg3 );
   }
   else if( !str_cmp( arg2, "unmorphother" ) )
   {
      STRSET( morph->unmorph_other, arg3 );
   }
   else if( !str_cmp( arg2, "unmorphself" ) )
   {
      STRSET( morph->unmorph_self, arg3 );
   }
   else if( !str_cmp( arg2, "keyword" ) )
   {
      STRSET( morph->key_words, arg3 );
   }
   else if( !str_cmp( arg2, "long" ) )
   {
      STRFREE( morph->long_desc );
      mudstrlcpy( buf, arg3, sizeof( buf ) );
      mudstrlcat( buf, "\r\n", sizeof( buf ) );
      morph->long_desc = STRALLOC( buf );
   }
   else if( !str_cmp( arg2, "description" ) || !str_cmp( arg2, "desc" ) )
   {
      if( check_subrestricted( ch ) )
         return;
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_MORPH_DESC;
      ch->dest_buf = morph;
      start_editing( ch, morph->description );
      return;
   }
   else if( !str_cmp( arg2, "name" ) )
   {
      STRSET( morph->name, arg3 );
   }
   else if( !str_cmp( arg2, "help" ) )
   {
      if( check_subrestricted( ch ) )
         return;
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_MORPH_HELP;
      ch->dest_buf = morph;
      start_editing( ch, morph->help );
      return;
   }
   else if( !str_cmp( arg2, "skills" ) )
   {
      if( arg3[0] == '\0' || !str_cmp( arg3, "none" ) )
      {
         STRFREE( morph->skills );
         send_to_char( "The skills on that morph have been set to none.\r\n", ch );
         return;
      }
      if( !morph->skills )
         snprintf( buf, sizeof( buf ), "%s", arg3 );
      else
         snprintf( buf, sizeof( buf ), "%s %s", morph->skills, arg3 );
      STRSET( morph->skills, buf );
   }
   else if( !str_cmp( arg2, "noskills" ) )
   {
      if( arg3[0] == '\0' || !str_cmp( arg3, "none" ) )
      {
         STRFREE( morph->no_skills );
         send_to_char( "The noskills on that morph have been set to none.\r\n", ch );
         return;
      }
      if( !morph->no_skills )
         snprintf( buf, sizeof( buf ), "%s", arg3 );
      else
         snprintf( buf, sizeof( buf ), "%s %s", morph->no_skills, arg3 );
      STRSET( morph->no_skills, buf );
   }
   else if( !str_cmp( arg2, "class" ) )
   {
      value = get_pc_class( arg3 );

      if( value < 0 || value >= MAX_PC_CLASS )
      {
         ch_printf( ch, "Unknown PC class: %s", arg3 );
         return;
      }
      xTOGGLE_BIT( morph->Class, value );
   }
   else if( !str_cmp( arg2, "race" ) )
   {
      value = get_pc_race( arg3 );

      if( value < 0 || value >= MAX_PC_RACE )
      {
         ch_printf( ch, "Unknown PC race: %s", arg3 );
         return;
      }
      xTOGGLE_BIT( morph->race, value );
   }
   else if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_morphset;
      return;
   }
   else
   {
      do_morphset( ch, (char *)"" );
      return;
   }

   save_morphs( true );
   send_to_char( "Done.\r\n", ch );
}

/*
 *  Shows morph info on a given morph.
 *  To see the description and help file, must use morphstat <morph> help
 *  Shaddai
 */
CMDF( do_morphstat )
{
   MORPH_DATA *morph;
   char arg[MIL];
   int count = 1, stat;

   set_pager_color( AT_CYAN, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_pager( "Morphstat what?\r\n", ch );
      return;
   }
   if( is_npc( ch ) )
   {
      send_to_char( "Mob's can't morphstat\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "list" ) )
   {
      if( !morph_start )
      {
         send_to_pager( "No morph's currently exist.\r\n", ch );
         return;
      }
      for( morph = morph_start; morph; morph = morph->next )
      {
         pager_printf( ch, "&c[&C%2d&c]   Name:  &C%-13s    &cVnum:  &C%4d  &cUsed:  &C%3d\r\n",
                       count, morph->name, morph->vnum, morph->used );
         count++;
      }
      return;
   }
   if( !is_number( arg ) )
      morph = get_morph( arg );
   else
      morph = get_morph_vnum( atoi( arg ) );

   if( !morph )
   {
      send_to_pager( "No such morph exists.\r\n", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      pager_printf( ch, "  &cMorph Name: &C%-20s  Vnum: %4d\r\n", morph->name, morph->vnum );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      send_to_pager( "                           &BMorph Restrictions\r\n", ch );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      pager_printf( ch, "  &cClasses Allowed   : &w%s\r\n", ext_class_string( &morph->Class ) );
      pager_printf( ch, "  &cRaces Not Allowed: &w%s\r\n", ext_race_string( &morph->race ) );
      pager_printf( ch, "  &cSex:  &C%s   &cPkill:   &C%s   &cTime From:   &C%d   &cTime To:    &C%d\r\n",
         morph->sex == SEX_MALE ? "male" :
         morph->sex == SEX_FEMALE ? "female" : "neutral",
         morph->pkill == ONLY_PKILL ? "YES" :
         morph->pkill == ONLY_PEACEFULL ? "NO" : "n/a", morph->timefrom, morph->timeto );
      pager_printf( ch, "  &cDay From:  &C%d  &cDay To:  &C%d\r\n", morph->dayfrom, morph->dayto );
      pager_printf( ch, "  &cLevel:  &C%d       &cMorph Via Spell   : &C%s\r\n",
         morph->level, ( morph->no_cast ) ? "NO" : "yes" );
      pager_printf( ch, "  &cUSAGES:  Mana:  &C%d  &cMove:  &C%d  &cHp:  &C%d  &cFavor:  &C%d\r\n",
         morph->manaused, morph->moveused, morph->hpused, morph->favorused );
      pager_printf( ch, "  &cObj1: &C%d  &cObjuse1: &C%s   &cObj2: &C%d  &cObjuse2: &C%s   &cObj3: &C%d  &cObjuse3: &c%s\r\n",
         morph->obj[0], ( morph->objuse[0] ? "YES" : "no" ), morph->obj[1], ( morph->objuse[1] ? "YES" : "no" ),
         morph->obj[2], ( morph->objuse[2] ? "YES" : "no" ) );
      pager_printf( ch, "  &cTimer: &w%d\r\n", morph->timer );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      send_to_pager( "                       &BEnhancements to the Player\r\n", ch );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      for( stat = 0; stat < STAT_MAX; stat++ )
         pager_printf( ch, " &c(%3.3s: &C%2d&c)", stattypes[stat], morph->stats[stat] );
      send_to_pager( "\r\n", ch );
      for( stat = 0; stat < RIS_MAX; stat++ )
         if( morph->resistant[stat] != 0 )
            pager_printf( ch, " &c( %s: &C%4d&C )\r\n", ris_flags[stat], morph->resistant[stat] );
      pager_printf( ch, "  &cSave versus: &w%d %d %d %d %d       &cDodge: &w%d  &cParry: &w%d  &cTumble: &w%d\r\n",
         morph->saving_poison_death, morph->saving_wand, morph->saving_para_petri, morph->saving_breath,
         morph->saving_spell_staff, morph->dodge, morph->parry, morph->tumble );

      pager_printf( ch, " &cHps    : &w%-5s", morph->hit ? morph->hit : "None" );
      pager_printf( ch, " &cMana   : &w%-5s", morph->mana ? morph->mana : "None" );
      pager_printf( ch, " &cMove   : &w%-5s\r\n", morph->move ? morph->move : "None" );

      pager_printf( ch, " &cDamroll: &w%-5s", morph->damroll ? morph->damroll : "None" );
      pager_printf( ch, " &cHitroll: &w%-5s", morph->hitroll ? morph->hitroll : "None" );
      pager_printf( ch, " &cAC     : &w%d\r\n", morph->ac );

      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      send_to_pager( "                          &BAffects to the Player\r\n", ch );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      pager_printf( ch, "  &cAffected by: &C%s\r\n", ext_flag_string( &morph->affected_by, a_flags ) );
      pager_printf( ch, "  &cSkills     : &w%s\r\n", morph->skills ? morph->skills : "None" );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      send_to_pager( "                     &BPrevented affects to the Player\r\n", ch );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      pager_printf( ch, "  &cNot affected by: &C%s\r\n", ext_flag_string( &morph->no_affected_by, a_flags ) );
      pager_printf( ch, "  &cNot Skills     : &w%s\r\n", morph->no_skills ? morph->no_skills : "None" );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      send_to_char( "\r\n", ch );
   }
   else if( !str_cmp( argument, "help" ) || !str_cmp( argument, "desc" ) )
   {
      pager_printf( ch, "  &cMorph Name  : &C%s\r\n", morph->name );
      pager_printf( ch, "  &cDefault Pos : &w%d\r\n", morph->defpos );
      pager_printf( ch, "  &cKeywords    : &w%s\r\n", morph->key_words );
      pager_printf( ch, "  &cShortdesc   : &w%s\r\n",
                    ( morph->short_desc && morph->short_desc[0] != '\0' ) ? "(None set)" : morph->short_desc );
      pager_printf( ch, "  &cLongdesc    : &w%s",
                    ( morph->long_desc && morph->long_desc[0] != '\0' ) ? "(None set)\r\n" : morph->long_desc );
      pager_printf( ch, "  &cMorphself   : &w%s\r\n", morph->morph_self );
      pager_printf( ch, "  &cMorphother  : &w%s\r\n", morph->morph_other );
      pager_printf( ch, "  &cUnMorphself : &w%s\r\n", morph->unmorph_self );
      pager_printf( ch, "  &cUnMorphother: &w%s\r\n", morph->unmorph_other );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      pager_printf( ch, "                                  &cHelp:\r\n&w%s\r\n", morph->help );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
      pager_printf( ch, "                               &cDescription:\r\n&w%s\r\n", morph->description );
      send_to_pager( "&B[----------------------------------------------------------------------------]\r\n", ch );
   }
   else
   {
      send_to_char( "Usage: morphstat <morph>\r\n", ch );
      send_to_char( "Usage: morphstat <morph> <help/desc>\r\n", ch );
   }
}

/* This function sends the morph/unmorph message to the people in the room. -- Shaddai */
void send_morph_message( CHAR_DATA *ch, MORPH_DATA *morph, bool is_morph )
{
   if( !morph )
      return;

   if( is_morph )
   {
      if( !morph->morph_other )
         act( AT_MORPH, "$n morphs into something else.", ch, NULL, NULL, TO_ROOM );
      else
         act( AT_MORPH, morph->morph_other, ch, NULL, NULL, TO_ROOM );

      if( !morph->morph_self )
         act( AT_MORPH, "You morph into something else.", ch, NULL, NULL, TO_CHAR );
      else
         act( AT_MORPH, morph->morph_self, ch, NULL, NULL, TO_CHAR );
   }
   else
   {
      if( !morph->unmorph_other )
         act( AT_MORPH, "$n changes back to their original form.", ch, NULL, NULL, TO_ROOM );
      else
         act( AT_MORPH, morph->unmorph_other, ch, NULL, NULL, TO_ROOM );

      if( !morph->unmorph_self )
         act( AT_MORPH, "You change back to your original form.", ch, NULL, NULL, TO_CHAR );
      else
         act( AT_MORPH, morph->unmorph_self, ch, NULL, NULL, TO_CHAR );
   }
}

/*
 * Create new player morph, a scailed down version of original morph
 * so if morph gets changed stats don't get messed up.
 */
CHAR_MORPH *make_char_morph( MORPH_DATA *morph )
{
   CHAR_MORPH *ch_morph;
   int stat;

   if( !morph )
      return NULL;

   CREATE( ch_morph, CHAR_MORPH, 1 );
   ch_morph->morph = morph;
   for( stat = 0; stat < STAT_MAX; stat++ )
      ch_morph->stats[stat] = morph->stats[stat];
   for( stat = 0; stat < RIS_MAX; stat++ )
      ch_morph->resistant[stat] = morph->resistant[stat];
   ch_morph->ac = morph->ac;
   ch_morph->saving_breath = morph->saving_breath;
   ch_morph->saving_para_petri = morph->saving_para_petri;
   ch_morph->saving_poison_death = morph->saving_poison_death;
   ch_morph->saving_spell_staff = morph->saving_spell_staff;
   ch_morph->saving_wand = morph->saving_wand;
   ch_morph->timer = morph->timer;
   ch_morph->hitroll = 0;
   ch_morph->damroll = 0;
   ch_morph->hit = 0;
   ch_morph->mana = 0;
   ch_morph->move = 0;
   ch_morph->affected_by = morph->affected_by;
   ch_morph->no_affected_by = morph->no_affected_by;
   return ch_morph;
}

/*
 * Workhorse of the polymorph code, this turns the player into the proper
 * morph.  Doesn't check if you can morph or not so must use can_morph before
 * this is called.  That is so we can let people morph into things without
 * checking. Also, if anything is changed here, make sure it gets changed
 * in do_unmorph otherwise you will be giving your player stats for free.
 * This also does not send the message to people when you morph good to
 * use in save functions.
 * --Shaddai
 */
void do_morph( CHAR_DATA *ch, MORPH_DATA *morph )
{
   CHAR_MORPH *ch_morph;
   int stat;

   if( !morph )
      return;

   if( ch->morph )
      do_unmorph_char( ch );

   ch_morph = make_char_morph( morph );
   for( stat = 0; stat < STAT_MAX; stat++ )
      ch->perm_stats[stat] += morph->stats[stat];
   for( stat = 0; stat < RIS_MAX; stat++ )
      ch->resistant[stat] += morph->resistant[stat];
   ch->armor += morph->ac;
   ch->saving_breath += morph->saving_breath;
   ch->saving_para_petri += morph->saving_para_petri;
   ch->saving_poison_death += morph->saving_poison_death;
   ch->saving_spell_staff += morph->saving_spell_staff;
   ch->saving_wand += morph->saving_wand;
   ch_morph->hitroll = morph->hitroll ? dice_parse( ch, ch, morph->level, morph->hitroll ) : 0;
   ch->hitroll += ch_morph->hitroll;
   ch_morph->damroll = morph->damroll ? dice_parse( ch, ch, morph->level, morph->damroll ) : 0;
   ch->damroll += ch_morph->damroll;

   ch_morph->hit = morph->hit ? dice_parse( ch, ch, morph->level, morph->hit ) : 0;
   if( ( ch->max_hit + ch_morph->hit ) > 2000000000 )
      ch_morph->hit = ( 2000000000 - ch->max_hit );
   ch->max_hit += ch_morph->hit;
   if( ( ch->hit + ch_morph->hit ) > ch->max_hit )
      ch->hit = ch->max_hit;
   else
      ch->hit += ch_morph->hit;

   ch_morph->move = morph->move ? dice_parse( ch, ch, morph->level, morph->move ) : 0;
   if( ( ch->max_move + ch_morph->move ) > 2000000000 )
      ch_morph->move = ( 2000000000 - ch->max_move );
   ch->max_move += ch_morph->move;
   if( ( ch->move + ch_morph->move ) > ch->max_move )
      ch->move = ch->max_move;
   else
      ch->move += ch_morph->move;

   ch_morph->mana = morph->mana ? dice_parse( ch, ch, morph->level, morph->mana ) : 0;
   if( ( ch->max_mana + ch_morph->mana ) > 2000000000 )
      ch_morph->mana = ( 2000000000 - ch->max_mana );
   ch->max_mana += ch_morph->mana;
   if( ( ch->mana + ch_morph->mana ) > ch->max_mana )
      ch->mana = ch->max_mana;
   else
      ch->mana += ch_morph->mana;

   xSET_BITS( ch->affected_by, morph->affected_by );
   xREMOVE_BITS( ch->affected_by, morph->no_affected_by );

   ch->morph = ch_morph;
   morph->used++;
}

/*
 * These functions wrap the sending of morph stuff, with morphing the code.
 * In most cases this is what should be called in the code when using spells,
 * obj morphing, etc... --Shaddai
 */
int do_morph_char( CHAR_DATA *ch, MORPH_DATA *morph )
{
   bool canmorph = true;
   OBJ_DATA *obj;

   if( ch->morph )
      canmorph = false;

   if( canmorph && morph->obj[0] )
   {
      if( !( obj = get_obj_vnum( ch, morph->obj[0] ) ) )
         canmorph = false;
      else if( morph->objuse[0] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, NULL, TO_CHAR );
         separate_obj( obj );
         extract_obj( obj );
      }
   }
   if( canmorph && morph->obj[1] )
   {
      if( !( obj = get_obj_vnum( ch, morph->obj[1] ) ) )
         canmorph = false;
      else if( morph->objuse[1] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, NULL, TO_CHAR );
         separate_obj( obj );
         extract_obj( obj );
      }
   }
   if( canmorph && morph->obj[2] )
   {
      if( !( obj = get_obj_vnum( ch, morph->obj[2] ) ) )
         canmorph = false;
      else if( morph->objuse[2] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, NULL, TO_CHAR );
         separate_obj( obj );
         extract_obj( obj );
      }
   }

   if( canmorph && morph->hpused )
   {
      if( ch->hit < morph->hpused )
         canmorph = false;
      else
         ch->hit -= morph->hpused;
   }

   if( canmorph && morph->moveused )
   {
      if( ch->move < morph->moveused )
         canmorph = false;
      else
         ch->move -= morph->moveused;
   }

   if( canmorph && morph->manaused )
   {
      if( ch->mana < morph->manaused )
         canmorph = false;
      else
         ch->mana -= morph->manaused;
   }

   if( canmorph && morph->favorused )
   {
      if( is_npc( ch ) || !ch->pcdata->deity || ch->pcdata->favor < morph->favorused )
         canmorph = false;
      else
      {
         ch->pcdata->favor -= morph->favorused;
         adjust_favor( ch, -1, 1 );
      }
   }

   if( !canmorph )
   {
      send_to_char( "You begin to transform, but something goes wrong.\r\n", ch );
      return false;
   }
   send_morph_message( ch, morph, true );
   do_morph( ch, morph );
   return true;
}

/*
 * This makes sure to take all the affects given to the player by the morph
 * away.  Several things to keep in mind.  If you add something here make
 * sure you add it to do_morph as well (Unless you want to charge your players
 * for morphing ;) ).  Also make sure that their pfile saves with the morph
 * affects off, as the morph may not exist when they log back in.  This
 * function also does not send the message to people when you morph so it is
 * good to use in save functions and other places you don't want people to
 * see the stuff.
 * --Shaddai
 */
void do_unmorph( CHAR_DATA *ch )
{
   CHAR_MORPH *morph;
   int stat;

   if( !( morph = ch->morph ) )
      return;

   ch->armor -= morph->ac;
   for( stat = 0; stat < STAT_MAX; stat++ )
      ch->perm_stats[stat] -= morph->stats[stat];
   for( stat = 0; stat < RIS_MAX; stat++ )
      ch->resistant[stat] -= morph->resistant[stat];
   ch->saving_breath -= morph->saving_breath;
   ch->saving_para_petri -= morph->saving_para_petri;
   ch->saving_poison_death -= morph->saving_poison_death;
   ch->saving_spell_staff -= morph->saving_spell_staff;
   ch->saving_wand -= morph->saving_wand;
   ch->hitroll -= morph->hitroll;
   ch->damroll -= morph->damroll;

   ch->max_hit -= morph->hit;
   if( ch->hit > morph->hit )
      ch->hit -= morph->hit;
   if( ch->hit > ch->max_hit )
      ch->hit = ch->max_hit;

   ch->max_move -= morph->move;
   if( ch->move > morph->move )
      ch->move -= ch->morph->move;
   if( ch->move > ch->max_move )
      ch->move = ch->max_move;

   ch->max_mana -= morph->mana;
   if( ch->mana > morph->mana )
      ch->mana -= morph->mana;
   if( ch->mana > ch->max_mana )
      ch->mana = ch->max_mana;

   xREMOVE_BITS( ch->affected_by, morph->affected_by );
   DISPOSE( ch->morph );
   update_aris( ch );
}

void do_unmorph_char( CHAR_DATA *ch )
{
   MORPH_DATA *temp;

   if( !ch || !ch->morph )
      return;

   temp = ch->morph->morph;
   do_unmorph( ch );
   send_morph_message( ch, temp, false );
}

/* Morph revert command ( God only knows why the Smaugers left this out ) - Samson 6-14-99 */
CMDF( do_revert )
{
   if( !ch->morph )
   {
      send_to_char( "But you aren't polymorphed?!?\r\n", ch );
      return;
   }
   do_unmorph_char( ch );
}

void setup_morph_vnum( void )
{
   MORPH_DATA *morph;
   int vnum = morph_vnum;

   for( morph = morph_start; morph; morph = morph->next )
      if( morph->vnum > vnum )
         vnum = morph->vnum;
   if( vnum < 1000 )
      vnum = 1000;
   else
      vnum++;

   for( morph = morph_start; morph; morph = morph->next )
   {
      if( morph->vnum == 0 )
      {
         morph->vnum = vnum;
         vnum++;
      }
   }
   morph_vnum = vnum;
}

/* 
 * Function that releases all the memory for a morph struct.  Carefull not
 * to use the memory afterwards as it doesn't exist.
 * --Shaddai 
 */
void free_morph( MORPH_DATA *morph )
{
   if( !morph )
      return;

   STRFREE( morph->damroll );
   STRFREE( morph->description );
   STRFREE( morph->help );
   STRFREE( morph->hit );
   STRFREE( morph->hitroll );
   STRFREE( morph->key_words );
   STRFREE( morph->long_desc );
   STRFREE( morph->mana );
   STRFREE( morph->morph_other );
   STRFREE( morph->morph_self );
   STRFREE( morph->move );
   STRFREE( morph->name );
   STRFREE( morph->short_desc );
   STRFREE( morph->skills );
   STRFREE( morph->no_skills );
   STRFREE( morph->unmorph_other );
   STRFREE( morph->unmorph_self );
   DISPOSE( morph );
}

void free_morphs( void )
{
   MORPH_DATA *morph, *morph_next;

   for( morph = morph_start; morph; morph = morph_next )
   {
      morph_next = morph->next;
      unmorph_all( morph );
      UNLINK( morph, morph_start, morph_end, next, prev );
      free_morph( morph );
   }
}

/* This function set's up all the default values for a morph */
void morph_defaults( MORPH_DATA *morph )
{
   int stat;

   morph->sex = -1;
   morph->timefrom = -1;
   morph->timeto = -1;
   morph->dayfrom = -1;
   morph->dayto = -1;
   morph->pkill = 0;
   morph->manaused = 0;
   morph->moveused = 0;
   morph->hpused = 0;
   morph->favorused = 0;
   xCLEAR_BITS( morph->Class );
   xCLEAR_BITS( morph->race );
   xCLEAR_BITS( morph->affected_by );
   xCLEAR_BITS( morph->no_affected_by );
   morph->obj[0] = 0;
   morph->obj[1] = 0;
   morph->obj[2] = 0;
   morph->objuse[0] = false;
   morph->objuse[1] = false;
   morph->objuse[2] = false;
   morph->used = 0;
   morph->ac = 0;
   morph->defpos = POS_STANDING;
   for( stat = 0; stat < STAT_MAX; stat++ )
       morph->stats[stat] = 0;
   for( stat = 0; stat < RIS_MAX; stat++ )
       morph->resistant[stat] = 0;
   morph->dodge = 0;
   morph->level = 0;
   morph->parry = 0;
   morph->saving_breath = 0;
   morph->saving_para_petri = 0;
   morph->saving_poison_death = 0;
   morph->saving_spell_staff = 0;
   morph->saving_wand = 0;
   morph->tumble = 0;
   morph->no_cast = false;
   morph->timer = -1;
   morph->vnum = 0;
}

/* Read in one morph structure */
MORPH_DATA *fread_morph( FILE *fp )
{
   MORPH_DATA *morph;
   char *arg, *infoflags;
   char temp[MSL], flag[MIL];
   const char *word;
   int i, value;
   bool fMatch;

   word = feof( fp ) ? "End" : fread_word( fp );

   if( !str_cmp( word, "End" ) )
      return NULL;

   CREATE( morph, MORPH_DATA, 1 );
   morph_defaults( morph );
   STRFREE( morph->name );
   morph->name = STRALLOC( ( char * )word );

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;
      switch( UPPER( word[0] ) )
      {
         case 'A':
            KEY( "Armor", morph->ac, fread_number( fp ) );
            KEY( "Affected", morph->affected_by, fread_bitvector( fp ) );
            if( !str_cmp( word, "Absorb" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'C':
            if( !str_cmp( word, "Class" ) )
            {
               arg = fread_flagstring( fp );
               while( arg[0] != '\0' )
               {
                  arg = one_argument( arg, temp );
                  for( i = 0; i < MAX_PC_CLASS; i++ )
                     if( !str_cmp( temp, class_table[i]->name ) )
                     {
                        xSET_BIT( morph->Class, i );
                        break;
                     }
               }
               fMatch = true;
               break;
            }
            break;

         case 'D':
            MKEY( "Damroll", morph->damroll, fread_string( fp ) );
            KEY( "DayFrom", morph->dayfrom, fread_number( fp ) );
            KEY( "DayTo", morph->dayto, fread_number( fp ) );
            KEY( "Defpos", morph->defpos, fread_number( fp ) );
            MKEY( "Description", morph->description, fread_string( fp ) );
            KEY( "Dodge", morph->dodge, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return morph;
            break;

         case 'F':
            KEY( "FavourUsed", morph->favorused, fread_number( fp ) );
            KEY( "FavorUsed", morph->favorused, fread_number( fp ) );
            break;

         case 'H':
            MKEY( "Help", morph->help, fread_string( fp ) );
            MKEY( "Hit", morph->hit, fread_string( fp ) );
            MKEY( "Hitroll", morph->hitroll, fread_string( fp ) );
            KEY( "HpUsed", morph->hpused, fread_number( fp ) );
            break;

         case 'I':
            if( !str_cmp( word, "Immune" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'K':
            MKEY( "Keywords", morph->key_words, fread_string( fp ) );
            break;

         case 'L':
            KEY( "Level", morph->level, fread_number( fp ) );
            MKEY( "Longdesc", morph->long_desc, fread_string( fp ) );
            break;

         case 'M':
            MKEY( "Mana", morph->mana, fread_string( fp ) );
            KEY( "ManaUsed", morph->manaused, fread_number( fp ) );
            MKEY( "MorphOther", morph->morph_other, fread_string( fp ) );
            MKEY( "MorphSelf", morph->morph_self, fread_string( fp ) );
            MKEY( "Move", morph->move, fread_string( fp ) );
            KEY( "MoveUsed", morph->moveused, fread_number( fp ) );
            break;

         case 'N':
            WEXTKEY( "NoAffected", morph->no_affected_by, fp, a_flags, AFF_MAX );
            if( !str_cmp( word, "NoImmune" ) || !str_cmp( word, "NoAbsorb" )
            || !str_cmp( word, "NoResistant" ) || !str_cmp( word, "NoSuscept" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            MKEY( "NoSkills", morph->no_skills, fread_string( fp ) );
            if( !str_cmp( word, "NoCast" ) )
            {
               morph->no_cast = fread_number( fp );
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "NResistant" ) )
            {
               int tmpvalue = fread_number( fp );
               infoflags = fread_flagstring( fp );
               value = get_flag( infoflags, ris_flags, RIS_MAX );
               if( value < 0 || value >= RIS_MAX )
                  bug( "%s: Unknown %s: %s", __FUNCTION__, word, infoflags );
               else
                  morph->resistant[value] = tmpvalue;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "NStat" ) )
            {
               int ustat, stat;

               stat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
                  morph->stats[ustat] = stat;
               fMatch = true;
               break;
            }
            break;

         case 'O':
            if( !str_cmp( word, "Objs" ) )
            {
               morph->obj[0] = fread_number( fp );
               morph->obj[1] = fread_number( fp );
               morph->obj[2] = fread_number( fp );
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Objuse" ) )
            {
               fMatch = true;
               morph->objuse[0] = fread_number( fp );
               morph->objuse[1] = fread_number( fp );
               morph->objuse[2] = fread_number( fp );
            }
            break;

         case 'P':
            KEY( "Parry", morph->parry, fread_number( fp ) );
            KEY( "Pkill", morph->pkill, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Race" ) )
            {
               arg = fread_flagstring( fp );
               while( arg[0] != '\0' )
               {
                  arg = one_argument( arg, temp );
                  for( i = 0; i < MAX_PC_RACE; ++i )
                     if( race_table[i] && !str_cmp( temp, race_table[i]->name ) )
                     {
                        xSET_BIT( morph->race, i );
                        break;
                     }
               }
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Resistant" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'S':
            if( !str_cmp( word, "Stats" ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               morph->stats[STAT_STR] = fread_number( fp );
               morph->stats[STAT_INT] = fread_number( fp );
               morph->stats[STAT_WIS] = fread_number( fp );
               morph->stats[STAT_DEX] = fread_number( fp );
               morph->stats[STAT_CON] = fread_number( fp );
               morph->stats[STAT_CHA] = fread_number( fp );
               morph->stats[STAT_LCK] = fread_number( fp );
               fMatch = true;
               break;
            }
            KEY( "SaveBreath", morph->saving_breath, fread_number( fp ) );
            KEY( "SavePara", morph->saving_para_petri, fread_number( fp ) );
            KEY( "SavePoison", morph->saving_poison_death, fread_number( fp ) );
            KEY( "SaveSpell", morph->saving_spell_staff, fread_number( fp ) );
            KEY( "SaveWand", morph->saving_wand, fread_number( fp ) );
            KEY( "Sex", morph->sex, fread_number( fp ) );
            MKEY( "ShortDesc", morph->short_desc, fread_string( fp ) );
            MKEY( "Skills", morph->skills, fread_string( fp ) );
            if( !str_cmp( word, "Suscept" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'T':
            KEY( "Timer", morph->timer, fread_number( fp ) );
            KEY( "TimeFrom", morph->timefrom, fread_number( fp ) );
            KEY( "TimeTo", morph->timeto, fread_number( fp ) );
            KEY( "Tumble", morph->tumble, fread_number( fp ) );
            break;

         case 'U':
            MKEY( "UnmorphOther", morph->unmorph_other, fread_string( fp ) );
            MKEY( "UnmorphSelf", morph->unmorph_self, fread_string( fp ) );
            KEY( "Used", morph->used, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Vnum", morph->vnum, fread_number( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

/*
 *  This function loads in the morph data.  Note that this function MUST be
 *  used AFTER the race and class tables have been read in and setup.
 *  --Shaddai
 */
void load_morphs( void )
{
   MORPH_DATA *morph;
   const char *word;
   FILE *fp;
   bool my_continue = true, fMatch;

   if( !( fp = fopen( MORPH_FILE, "r" ) ) )
   {
      bug( "%s: can't open %s for reading.", __FUNCTION__, MORPH_FILE );
      perror( MORPH_FILE );
      return;
   }

   while( my_continue )
   {
      morph = NULL;
      word = feof( fp ) ? "#END" : fread_word( fp );
      fMatch = false;

      if( word[0] == EOF )
         word = "#END";

      switch( UPPER( word[0] ) )
      {
         case '#':
            if( !str_cmp( word, "#END" ) )
            {
               fclose( fp );
               fp = NULL;
               fMatch = true;
               my_continue = false;
               break;
            }
            break;

         case 'M':
            if( !str_cmp( word, "Morph" ) )
            {
               morph = fread_morph( fp );
               fMatch = true;
               LINK( morph, morph_start, morph_end, next, prev );
            }
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   setup_morph_vnum( );
   log_string( "Done." );
}

/* This function copys one morph structure to another */
void copy_morph( MORPH_DATA *morph, MORPH_DATA *temp )
{
   int stat;

   STRSET( morph->damroll, temp->damroll );
   STRSET( morph->description, temp->description );
   STRSET( morph->help, temp->help );
   STRSET( morph->hit, temp->hit );
   STRSET( morph->hitroll, temp->hitroll );
   STRSET( morph->key_words, temp->key_words );
   STRSET( morph->long_desc, temp->long_desc );
   STRSET( morph->mana, temp->mana );
   STRSET( morph->morph_other, temp->morph_other );
   STRSET( morph->morph_self, temp->morph_self );
   STRSET( morph->move, temp->move );
   STRSET( morph->name, temp->name );
   STRSET( morph->short_desc, temp->short_desc );
   STRSET( morph->skills, temp->skills );
   STRSET( morph->no_skills, temp->no_skills );
   STRSET( morph->unmorph_other, temp->unmorph_other );
   STRSET( morph->unmorph_self, temp->unmorph_self );
   morph->affected_by = temp->affected_by;
   morph->Class = temp->Class;
   morph->sex = temp->sex;
   morph->timefrom = temp->timefrom;
   morph->timeto = temp->timeto;
   morph->timefrom = temp->timefrom;
   morph->dayfrom = temp->dayfrom;
   morph->dayto = temp->dayto;
   morph->pkill = temp->pkill;
   morph->manaused = temp->manaused;
   morph->moveused = temp->moveused;
   morph->hpused = temp->hpused;
   morph->favorused = temp->favorused;
   morph->affected_by = temp->affected_by;
   morph->no_affected_by = temp->no_affected_by;
   morph->obj[0] = temp->obj[0];
   morph->obj[1] = temp->obj[1];
   morph->obj[2] = temp->obj[2];
   morph->objuse[0] = temp->objuse[0];
   morph->objuse[1] = temp->objuse[1];
   morph->objuse[2] = temp->objuse[2];
   morph->race = temp->race;
   morph->ac = temp->ac;
   morph->defpos = temp->defpos;
   for( stat = 0; stat < STAT_MAX; stat++ )
      morph->stats[stat] = temp->stats[stat];
   for( stat = 0; stat < RIS_MAX; stat++ )
      morph->resistant[stat] = temp->resistant[stat];
   morph->dodge = temp->dodge;
   morph->level = temp->level;
   morph->parry = temp->parry;
   morph->saving_breath = temp->saving_breath;
   morph->saving_para_petri = temp->saving_para_petri;
   morph->saving_poison_death = temp->saving_poison_death;
   morph->saving_spell_staff = temp->saving_spell_staff;
   morph->saving_wand = temp->saving_wand;
   morph->tumble = temp->tumble;
   morph->no_cast = temp->no_cast;
   morph->timer = temp->timer;
}

/* Player command to create a new morph */
CMDF( do_morphcreate )
{
   MORPH_DATA *morph, *temp = NULL;
   char arg1[MIL];
   int value = 0;
   bool copy = false;

   argument = one_argument( argument, arg1 );

   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: morphcreate <name/vnum> [copy]\r\n", ch );
      return;
   }

   if( argument && argument[0] != '\0' && !str_cmp( argument, "copy" ) )
      copy = true;
   if( is_number( arg1 ) )
      value = atoi( arg1 );

   if( copy )
   {
      if( value )
      {
         if( !( temp = get_morph_vnum( value ) ) )
         {
            ch_printf( ch, "No such morph vnum %d exists.\r\n", value );
            return;
         }
      }
      else if( !( temp = get_morph( arg1 ) ) )
      {
         ch_printf( ch, "No such morph %s exists.\r\n", arg1 );
         return;
      }
   }
   CREATE( morph, MORPH_DATA, 1 );
   morph_defaults( morph );
   STRFREE( morph->name );
   if( copy && temp )
      copy_morph( morph, temp );
   else if( value )
      morph->name = STRALLOC( "New Morph" );
   else
      morph->name = STRALLOC( arg1 );
   if( !morph->short_desc || morph->short_desc[0] == '\0' )
   {
      if( value )
         morph->short_desc = STRALLOC( "New Morph" );
      else
         morph->short_desc = STRALLOC( arg1 );
   }
   morph->vnum = morph_vnum;
   morph_vnum++;
   LINK( morph, morph_start, morph_end, next, prev );
   ch_printf( ch, "Morph %s created with vnum %d.\r\n", morph->name, morph->vnum );
}

void unmorph_all( MORPH_DATA *morph )
{
   CHAR_DATA *vch;

   for( vch = first_char; vch; vch = vch->next )
   {
      if( is_npc( vch ) )
         continue;
      if( !vch->morph || !vch->morph->morph || vch->morph->morph != morph )
         continue;
      do_unmorph_char( vch );
   }
}

/*
 * Immortal function to delete a morph. --Shaddai
 * NOTE Need to check all players and force them to unmorph first
 */
CMDF( do_morphdestroy )
{
   MORPH_DATA *morph;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Destroy which morph?\r\n", ch );
      return;
   }

   if( is_number( argument ) )
      morph = get_morph_vnum( atoi( argument ) );
   else
      morph = get_morph( argument );

   if( !morph )
   {
      ch_printf( ch, "Unkown morph %s.\r\n", argument );
      return;
   }
   unmorph_all( morph );
   UNLINK( morph, morph_start, morph_end, next, prev );
   free_morph( morph );
   send_to_char( "Morph deleted.\r\n", ch );
}

void fwrite_morph_data( CHAR_DATA *ch, FILE *fp )
{
   CHAR_MORPH *morph;
   int stat;

   if( !ch->morph )
      return;

   morph = ch->morph;
   fprintf( fp, "%s", "#MorphData\n" );
   /* Only Print Out what is necessary */
   if( morph->morph )
   {
      fprintf( fp, "Vnum            %d\n", morph->morph->vnum );
      fprintf( fp, "Name            %s~\n", morph->morph->name );
   }
   if( !xIS_EMPTY( morph->affected_by ) )
      fprintf( fp, "Affect          %s~\n", ext_flag_string( &morph->affected_by, a_flags ) );
   if( !xIS_EMPTY( morph->no_affected_by ) )
      fprintf( fp, "NoAffect        %s~\n", ext_flag_string( &morph->no_affected_by, a_flags ) );

   for( stat = 0; stat < STAT_MAX; stat++ )
      if( morph->stats[stat] != 0 )
         fprintf( fp, "NStat           %d %s~\n", morph->stats[stat], stattypes[stat] ); 

   for( stat = 0; stat < RIS_MAX; stat++ )
      if( morph->resistant[stat] != 0 )
         fprintf( fp, "NResistant      %d %s~\n", morph->resistant[stat], ris_flags[stat] );
   if( morph->ac )
      fprintf( fp, "Armor           %d\n", morph->ac );
   if( morph->damroll )
      fprintf( fp, "Damroll         %d\n", morph->damroll );
   if( morph->dodge )
      fprintf( fp, "Dodge           %d\n", morph->dodge );
   if( morph->hit )
      fprintf( fp, "Hit             %d\n", morph->hit );
   if( morph->hitroll )
      fprintf( fp, "Hitroll         %d\n", morph->hitroll );
   if( morph->mana )
      fprintf( fp, "Mana            %d\n", morph->mana );
   if( morph->move )
      fprintf( fp, "Move            %d\n", morph->move );
   if( morph->parry )
      fprintf( fp, "Parry           %d\n", morph->parry );
   if( morph->saving_breath )
      fprintf( fp, "Save1           %d\n", morph->saving_breath );
   if( morph->saving_para_petri )
      fprintf( fp, "Save2           %d\n", morph->saving_para_petri );
   if( morph->saving_poison_death )
      fprintf( fp, "Save3           %d\n", morph->saving_poison_death );
   if( morph->saving_spell_staff )
      fprintf( fp, "Save4           %d\n", morph->saving_spell_staff );
   if( morph->saving_wand )
      fprintf( fp, "Save5           %d\n", morph->saving_wand );
   if( morph->timer != -1 )
      fprintf( fp, "Timer           %d\n", morph->timer );
   if( morph->tumble )
      fprintf( fp, "Tumble          %d\n", morph->tumble );
   fprintf( fp, "%s", "End\n" );
}

void clear_char_morph( CHAR_MORPH *morph )
{
   int stat;

   morph->timer = -1;
   xCLEAR_BITS( morph->affected_by );
   xCLEAR_BITS( morph->no_affected_by );
   for( stat = 0; stat < STAT_MAX; stat++ )
      morph->stats[stat] = 0;
   for( stat = 0; stat < RIS_MAX; stat++ )
      morph->resistant[stat] = 0;
   morph->ac = 0;
   morph->damroll = 0;
   morph->dodge = 0;
   morph->hit = 0;
   morph->hitroll = 0;
   morph->mana = 0;
   morph->parry = 0;
   morph->saving_breath = 0;
   morph->saving_para_petri = 0;
   morph->saving_poison_death = 0;
   morph->saving_spell_staff = 0;
   morph->saving_wand = 0;
   morph->tumble = 0;
   morph->morph = NULL;
}

void fread_morph_data( CHAR_DATA *ch, FILE *fp )
{
   CHAR_MORPH *morph;
   bool fMatch;
   const char *word;
   char *infoflags, flag[MIL];
   int value;

   CREATE( morph, CHAR_MORPH, 1 );
   clear_char_morph( morph );
   ch->morph = morph;
   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;

      switch( UPPER( word[0] ) )
      {
         case 'A':
            WEXTKEY( "Affect", morph->affected_by, fp, a_flags, AFF_MAX );
            KEY( "Armor", morph->ac, fread_number( fp ) );
            if( !str_cmp( word, "Absorb" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'D':
            KEY( "Damroll", morph->damroll, fread_number( fp ) );
            KEY( "Dodge", morph->dodge, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( "End", word ) )
               return;
            break;

         case 'H':
            KEY( "Hit", morph->hit, fread_number( fp ) );
            KEY( "Hitroll", morph->hitroll, fread_number( fp ) );
            break;

         case 'I':
            if( !str_cmp( word, "Immune" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'M':
            KEY( "Mana", morph->mana, fread_number( fp ) );
            KEY( "Move", morph->move, fread_number( fp ) );
            break;

         case 'N':
            if( !str_cmp( word, "NResistant" ) )
            {
               int tmpvalue = fread_number( fp );
               infoflags = fread_flagstring( fp );
               value = get_flag( infoflags, ris_flags, RIS_MAX );
               if( value < 0 || value >= RIS_MAX )
                  bug( "%s: Unknown %s: %s", __FUNCTION__, word, infoflags );
               else
                  morph->resistant[value] = tmpvalue;
               fMatch = true;
               break;
            }
            if( !str_cmp( "Name", word ) )
            {
               if( morph->morph )
                  if( str_cmp( morph->morph->name, fread_flagstring( fp ) ) )
                     bug( "Morph Name doesn't match vnum %d.", morph->morph->vnum );
               fMatch = true;
               break;
            }
            WEXTKEY( "NoAffect", morph->no_affected_by, fp, a_flags, AFF_MAX );
            if( !str_cmp( word, "NoImmune" ) || !str_cmp( word, "NoAbsorb" )
            || !str_cmp( word, "NoResistant" ) || !str_cmp( word, "NoSuscept" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "NStat" ) )
            {
               int ustat, stat;

               stat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
                  morph->stats[ustat] = stat;
               fMatch = true;
               break;
            }
            break;

         case 'P':
            KEY( "Parry", morph->parry, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Resistant" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'S':
            if( !str_cmp( "Stats", word ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               morph->stats[STAT_STR] = fread_number( fp );
               morph->stats[STAT_INT] = fread_number( fp );
               morph->stats[STAT_WIS] = fread_number( fp );
               morph->stats[STAT_DEX] = fread_number( fp );
               morph->stats[STAT_CON] = fread_number( fp );
               morph->stats[STAT_CHA] = fread_number( fp );
               morph->stats[STAT_LCK] = fread_number( fp );
               fMatch = true;
               break;
            }
            KEY( "Save1", morph->saving_breath, fread_number( fp ) );
            KEY( "Save2", morph->saving_para_petri, fread_number( fp ) );
            KEY( "Save3", morph->saving_poison_death, fread_number( fp ) );
            KEY( "Save4", morph->saving_spell_staff, fread_number( fp ) );
            KEY( "Save5", morph->saving_wand, fread_number( fp ) );
            if( !str_cmp( word, "Suscept" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'T':
            KEY( "Timer", morph->timer, fread_number( fp ) );
            KEY( "Tumble", morph->tumble, fread_number( fp ) );
            break;

         case 'V':
            if( !str_cmp( "Vnum", word ) )
            {
               morph->morph = get_morph_vnum( fread_number( fp ) );
               fMatch = true;
               break;
            }
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

/* Following functions are for immortal testing purposes. */
CMDF( do_imm_morph )
{
   MORPH_DATA *morph;
   CHAR_DATA *victim = NULL;
   char arg[MIL];

   if( is_npc( ch ) )
   {
      send_to_char( "Only player characters can use this command.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !is_number( arg ) )
   {
      send_to_char( "Usage: morph <vnum> [<target>]\r\n", ch );
      return;
   }
   if( !( morph = get_morph_vnum( atoi( arg ) ) ) )
   {
      ch_printf( ch, "No such morph %d exists.\r\n", atoi( arg ) );
      return;
   }
   if( !argument || argument[0] == '\0' )
      do_morph_char( ch, morph );
   else if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "No one like that in all the realms.\r\n", ch );
      return;
   }
   if( victim && get_trust( ch ) < get_trust( victim ) && !is_npc( victim ) )
   {
      send_to_char( "You can't do that!\r\n", ch );
      return;
   }
   else if( victim )
      do_morph_char( victim, morph );
   send_to_char( "Done.\r\n", ch );
}

/* This is just a wrapper.  --Shaddai */
CMDF( do_imm_unmorph )
{
   CHAR_DATA *victim = NULL;

   if( !argument || argument[0] == '\0' )
      do_unmorph_char( ch );
   else if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "No one like that in all the realms.\r\n", ch );
      return;
   }
   if( victim && get_trust( ch ) < get_trust( victim ) && !is_npc( victim ) )
   {
      send_to_char( "You can't do that!\r\n", ch );
      return;
   }
   else if( victim )
      do_unmorph_char( victim );
   send_to_char( "Done.\r\n", ch );
}

/* Added by Samson 6-13-99 - lists available polymorph forms */
CMDF( do_morphlist )
{
   MORPH_DATA *morph;

   send_to_pager( "&GVnum | &YPolymorph Name\r\n", ch );
   send_to_pager( "&G-----+-----------------------------------\r\n", ch );
   for( morph = morph_start; morph; morph = morph->next )
   {
      if( !morph )
         continue;
      pager_printf( ch, "&G%4d   &Y%s\r\n", morph->vnum, morph->name );
   }
}

#undef ONLY_PKILL
#undef ONLY_PEACEFULL
