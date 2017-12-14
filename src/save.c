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
 *		     Character saving and loading module		     *
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "h/mud.h"

CHAR_DATA *fread_mobile( FILE *fp );
EXT_BV multimeb( int bit, ... );
void free_obj( OBJ_DATA *obj, bool unlink );
void fwrite_char( CHAR_DATA *ch, FILE *fp );
void fread_char( CHAR_DATA *ch, FILE *fp, bool preload, bool copyover );
void fread_sudoku( CHAR_DATA *ch, bool display, FILE *fp );
void fwrite_sudoku( CHAR_DATA *ch, FILE *fp );
void fwrite_morph_data( CHAR_DATA *ch, FILE *fp );
void fread_morph_data( CHAR_DATA *ch, FILE *fp );
void update_highscore( CHAR_DATA *ch );
void fwrite_mobile( FILE *fp, CHAR_DATA *mob );
void set_ch_personal( PC_DATA *pc );
bool is_playing_rubik( CHAR_DATA *ch );

/* Increment with every major format change. */
#define SAVEVERSION 1

CHAR_DATA *quitting_char, *loading_char, *saving_char;
int file_ver;

/* Array of containers read for proper re-nesting of objects. */
static OBJ_DATA *rgObjNest[MAX_NEST];

#ifdef WIN32   /* NJG */
UINT timer_code = 0; /* needed to kill the timer */

/* Note: need to include: WINMM.LIB to link to timer functions */
void caught_alarm( void );
void CALLBACK alarm_handler( UINT IDEvent, UINT uReserved, DWORD dwUser, DWORD dwReserved1, DWORD dwReserved2 )
{
   caught_alarm( );
}

void kill_timer( void )
{
   if( timer_code )
      timeKillEvent( timer_code );
   timer_code = 0;
}
#endif

void de_equipchar( CHAR_DATA *ch )
{
   OBJ_DATA *obj;
   int wear_loc;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc > -1 && obj->wear_loc < WEAR_MAX )
      {
         wear_loc = obj->wear_loc;
         unequip_char( ch, obj );
         obj->t_wear_loc = wear_loc;
      }
   }
}

void re_equipchar( CHAR_DATA *ch )
{
   OBJ_DATA *obj;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->t_wear_loc > -1 && obj->t_wear_loc < WEAR_MAX )
      {
         equip_char( ch, obj, obj->t_wear_loc );
         obj->t_wear_loc = -1;
      }
   }
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch )
{
   FILE *fp;
   char strsave[MIL], strtemp[MIL];

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return;
   }

   if( is_npc( ch ) || ch->level < 2 )
      return;

   update_highscore( ch );

   saving_char = ch;

   de_equipchar( ch );
   ch->save_time = current_time;

   if( get_trust( ch ) >= PERM_IMM )
   {
      snprintf( strsave, sizeof( strsave ), "%s%s", GOD_DIR, capitalize( ch->pcdata->filename ) );
      snprintf( strtemp, sizeof( strtemp ), "%s.temp", strsave );
      if( !( fp = fopen( strtemp, "w" ) ) )
      {
         perror( strtemp );
         bug( "%s: cant open %s", __FUNCTION__, strtemp );
      }
      else
      {
         fprintf( fp, "NTrust       %s~\n", perms_flag[get_trust( ch )] );
         fprintf( fp, "Sex          %s~\n", sex_names[ch->sex] );
         fprintf( fp, "Pcflags      %s~\n", ext_flag_string( &ch->pcdata->flags, pc_flags ) );
         if( ch->pcdata->range_lo && ch->pcdata->range_hi )
            fprintf( fp, "VnumRange    %d %d\n", ch->pcdata->range_lo, ch->pcdata->range_hi );
         fclose( fp );
         fp = NULL;
         if( rename( strtemp, strsave ) )
         {
            perror( strsave );
            bug( "%s: Couldn't rename (%s) to (%s).", __FUNCTION__, strtemp, strsave );
         }
      }
   }

   snprintf( strsave, sizeof( strsave ), "%s%c/%s", PLAYER_DIR, tolower( ch->pcdata->filename[0] ),
      capitalize( ch->pcdata->filename ) );
   snprintf( strtemp, sizeof( strtemp ), "%s.temp", strsave );

   if( !( fp = fopen( strtemp, "w" ) ) )
   {
      perror( strtemp );
      bug( "%s: cant open %s", __FUNCTION__, strtemp );
   }
   else
   {
      fwrite_char( ch, fp );
      if( ch->morph )
         fwrite_morph_data( ch, fp );
      if( ch->first_carrying )
         fwrite_obj( ch, ch->last_carrying, fp, 0, OS_CARRY, ch->pcdata->hotboot );
      if( sysdata.save_pets && ch->pcdata->first_pet )
      {
         CHAR_DATA *pet;

         for( pet = ch->pcdata->first_pet; pet; pet = pet->next_pet )
            fwrite_mobile( fp, pet );
      }
      fprintf( fp, "#END\n" );
      fclose( fp );
      fp = NULL;
      if( rename( strtemp, strsave ) )
      {
         perror( strsave );
         bug( "%s: Couldn't rename (%s) to (%s).", __FUNCTION__, strtemp, strsave );
      }
   }

   re_equipchar( ch );
   quitting_char = NULL;
   saving_char = NULL;
}

/* Write the char. */
void fwrite_char( CHAR_DATA *ch, FILE *fp )
{
   FRIEND_DATA *ofriend;
   EXP_DATA *fexp;
   MCLASS_DATA *mclass;
   KILLED_DATA *killed;
   int sn, stat;
   short pos;

   fprintf( fp, "#PLAYER\n" );
   fprintf( fp, "Version        %d\n", SAVEVERSION );
   fprintf( fp, "Name           %s~\n", ch->name );
   fprintf( fp, "Level          %d\n", ch->level );

   if( !xIS_EMPTY( ch->pcdata->flags ) )
      fprintf( fp, "Flags          %s~\n", ext_flag_string( &ch->pcdata->flags, pc_flags ) );
   if( !xIS_EMPTY( ch->act ) )
      fprintf( fp, "Act            %s~\n", ext_flag_string( &ch->act, plr_flags ) );
   if( !xIS_EMPTY( ch->affected_by ) )
      fprintf( fp, "AffectedBy     %s~\n", ext_flag_string( &ch->affected_by, a_flags ) );
   if( !xIS_EMPTY( ch->no_affected_by ) )
      fprintf( fp, "NoAffectedBy   %s~\n", ext_flag_string( &ch->no_affected_by, a_flags ) );

   for( stat = 0; stat < RIS_MAX; stat++ )
      if( ch->resistant[stat] != 0 )
         fprintf( fp, "NResistant     %d %s~\n", ch->resistant[stat], ris_flags[stat] );

   if( is_immortal( ch ) && ch->trust )
      fprintf( fp, "NTrust         %s~\n", perms_flag[ch->trust] );
   fprintf( fp, "Sex            %s~\n", sex_names[ch->sex] );
   if( ch->description )
      fprintf( fp, "Description    %s~\n", ch->description );
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      fprintf( fp, "MClass         %s~ %d %d %.f\n",
         ( mclass->wclass < 0 ) ? "-1" : class_table[mclass->wclass]->name, mclass->level, mclass->cpercent, mclass->exp );
   }

   if( race_table[ch->race] && race_table[ch->race]->name )
      fprintf( fp, "Race           %s~\n", race_table[ch->race]->name );
   if( !xIS_EMPTY( ch->speaks ) )
      fprintf( fp, "Speaks         %s~\n", ext_flag_string( &ch->speaks, lang_names ) );
   if( !xIS_EMPTY( ch->speaking ) )
      fprintf( fp, "Speaking       %s~\n", ext_flag_string( &ch->speaking, lang_names ) );
   if( ch->pcdata->spouse )
      fprintf( fp, "Spouse         %s~\n", ch->pcdata->spouse );
   if( ch->bsplatter )
      fprintf( fp, "BSplatter      %d\n", ch->bsplatter );

   for( ofriend = ch->pcdata->first_friend; ofriend; ofriend = ofriend->next )
   {
      /* Should we save it? */
      if( ofriend->name && ofriend->approved )
         fprintf( fp, "Friend         %d %s~\n", ofriend->sex, ofriend->name );
   }

   if( ch->pcdata->speed != 1 )
      fprintf( fp, "Speed          %d\n", ch->pcdata->speed );

   if( ch->pcdata->channels )
      fprintf( fp, "Channels       %s~\n", ch->pcdata->channels );

   /* Save channel history */
   fwrite_phistory( ch, fp );

   fprintf( fp, "Played         %d\n", ch->played );
   fprintf( fp, "Birthday       %d %d %d\n", ch->pcdata->birth_month, ch->pcdata->birth_day, ch->pcdata->birth_year );
   fprintf( fp, "Room           %d\n",
      ( ch->in_room == get_room_index( sysdata.room_limbo )
      && ch->was_in_room ) ? ch->was_in_room->vnum : ch->in_room->vnum );

   if( xIS_SET( ch->act, PLR_WILDERNESS ) )
      fprintf( fp, "Cords          %d %d\n", ch->cords[0], ch->cords[1] );

   fprintf( fp, "HpManaMove     %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
   if( ch->gold )
      fprintf( fp, "Gold           %d\n", ch->gold );
   fprintf( fp, "Height         %d\n", ch->height );
   fprintf( fp, "Weight         %d\n", ch->weight );

   if( ch->logged )
      fprintf( fp, "%s", "Logged\n" );

   fwrite_sudoku( ch, fp );

   if( is_playing_rubik( ch ) )
   {
      short rnum;

      fprintf( fp, "%s", "Rubik         " );
      for( rnum = 0; rnum < 54; rnum++ )
         fprintf( fp, " %d", ch->pcdata->rubik[rnum] );
      fprintf( fp, "%s", "\n" );
   }
   fprintf( fp, "RubikWins      %u\n", ch->pcdata->rwins );
   fprintf( fp, "RubikQuits     %u\n", ch->pcdata->rquits );

   pos = ch->position;
   if( pos == POS_BERSERK || pos == POS_AGGRESSIVE || pos == POS_FIGHTING || pos == POS_DEFENSIVE || pos == POS_EVASIVE )
      pos = POS_STANDING;
   fprintf( fp, "Position       %s~\n", pos_names[pos] );
   fprintf( fp, "Style          %s~\n", style_names[ch->style] );
   if( ch->practice )
      fprintf( fp, "Practice       %d\n", ch->practice );
   if( ch->saving_poison_death || ch->saving_wand || ch->saving_para_petri || ch->saving_breath || ch->saving_spell_staff )
      fprintf( fp, "SavingThrows   %d %d %d %d %d\n",
         ch->saving_poison_death, ch->saving_wand, ch->saving_para_petri, ch->saving_breath, ch->saving_spell_staff );
   if( ch->alignment )
      fprintf( fp, "Alignment      %d\n", ch->alignment );
   if( ch->pcdata->favor )
      fprintf( fp, "Favor          %d\n", ch->pcdata->favor );
   if( ch->pcdata->quest_curr )
      fprintf( fp, "Glory          %u\n", ch->pcdata->quest_curr );
   if( ch->pcdata->questcompleted )
      fprintf( fp, "QCompleted     %u\n", ch->pcdata->questcompleted );
   if( ch->questcountdown )
      fprintf( fp, "QCountdown     %d\n", ch->questcountdown );
   if( ch->questgiver )
      fprintf( fp, "QGiver         %d\n", ch->questgiver );
   if( ch->questvnum )
      fprintf( fp, "QVnum          %d\n", ch->questvnum );
   if( ch->questtype )
      fprintf( fp, "QType          %d\n", ch->questtype );
   if( ch->hitroll )
      fprintf( fp, "Hitroll        %d\n", ch->hitroll );
   if( ch->damroll )
      fprintf( fp, "Damroll        %d\n", ch->damroll );
   if( ch->armor )
      fprintf( fp, "Armor          %d\n", ch->armor );
   if( ch->wimpy )
      fprintf( fp, "Wimpy          %d\n", ch->wimpy );
   if( ch->mental_state != -10 )
      fprintf( fp, "Mentalstate    %d\n", ch->mental_state );
   fprintf( fp, "Password       %s~\n", ch->pcdata->pwd );
   if( ch->pcdata->rank )
      fprintf( fp, "Rank           %s~\n", ch->pcdata->rank );
   if( ch->pcdata->bestowments )
      fprintf( fp, "Bestowments    %s~\n", ch->pcdata->bestowments );
   fprintf( fp, "Title          %s~\n", ch->pcdata->title );
   if( ch->pcdata->homepage )
      fprintf( fp, "Homepage       %s~\n", ch->pcdata->homepage );
   if( ch->pcdata->email )
      fprintf( fp, "Email          %s~\n", ch->pcdata->email );
   if( ch->pcdata->msn )
      fprintf( fp, "MSN            %s~\n", ch->pcdata->msn );
   if( ch->pcdata->yahoo )
      fprintf( fp, "Yahoo          %s~\n", ch->pcdata->yahoo );
   if( ch->pcdata->gtalk )
      fprintf( fp, "GTalk          %s~\n", ch->pcdata->gtalk );
   if( ch->pcdata->bio )
      fprintf( fp, "Bio            %s~\n", ch->pcdata->bio );
   if( ch->pcdata->min_snoop )
      fprintf( fp, "Minsnoop       %d\n", ch->pcdata->min_snoop );
   if( ch->pcdata->prompt )
      fprintf( fp, "Prompt         %s~\n", ch->pcdata->prompt );
   if( ch->pcdata->fprompt )
      fprintf( fp, "FPrompt        %s~\n", ch->pcdata->fprompt );
   if( ch->pcdata->pagerlen != 24 )
      fprintf( fp, "Pagerlen       %d\n", ch->pcdata->pagerlen );
   if( ch->pcdata->kltime )
      fprintf( fp, "KeepAlive      %d\n", ch->pcdata->kltime );

   {
      HOST_DATA *host;

      for( host = ch->first_host; host; host = host->next )
         fprintf( fp, "Host           %d %s~ %d\n", host->prefix, host->host, host->suffix );
   }

   {
      IGNORE_DATA *temp;

      for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
         fprintf( fp, "Ignored        %s~\n", temp->name );
   }

   if( is_immortal( ch ) )
   {
      if( ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
         fprintf( fp, "Bamfin         %s~\n", ch->pcdata->bamfin );
      if( ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
         fprintf( fp, "Bamfout        %s~\n", ch->pcdata->bamfout );
      if( ch->pcdata && ch->pcdata->restore_time )
         fprintf( fp, "Restore_time   %ld\n", ch->pcdata->restore_time );
      fprintf( fp, "WizInvis       %d\n", ch->pcdata->wizinvis );
      if( ch->pcdata->range_lo && ch->pcdata->range_hi )
         fprintf( fp, "VnumRange      %d %d\n", ch->pcdata->range_lo, ch->pcdata->range_hi );
   }
   if( ch->pcdata->council && ch->pcdata->council->name )
      fprintf( fp, "Council        %s~\n", ch->pcdata->council->name );
   if( ch->pcdata->deity && ch->pcdata->deity->name )
      fprintf( fp, "Deity          %s~\n", ch->pcdata->deity->name );
   if( ch->pcdata->clan && ch->pcdata->clan->name )
      fprintf( fp, "Clan           %s~\n", ch->pcdata->clan->name );
   if( ch->pcdata->nation && ch->pcdata->nation->name )
      fprintf( fp, "Nation         %s~\n", ch->pcdata->nation->name );
   if( ch->pcdata->news_read )
      fprintf( fp, "NewsRead       %ld\n", ch->pcdata->news_read );
   if( ch->pcdata->pkills )
      fprintf( fp, "PKills         %u\n", ch->pcdata->pkills );
   if( ch->pcdata->pdeaths )
      fprintf( fp, "PDeaths        %u\n", ch->pcdata->pdeaths );
   if( get_timer( ch, TIMER_PKILLED ) && ( get_timer( ch, TIMER_PKILLED ) > 0 ) )
      fprintf( fp, "PTimer         %d\n", get_timer( ch, TIMER_PKILLED ) );
   if( ch->pcdata->mkills )
      fprintf( fp, "MKills         %u\n", ch->pcdata->mkills );
   if( ch->pcdata->mdeaths )
      fprintf( fp, "MDeaths        %u\n", ch->pcdata->mdeaths );

   for( stat = 0; stat < STAT_MAX; stat++ )
      fprintf( fp, "NStat          %d %d %s~\n", ch->perm_stats[stat], ch->mod_stats[stat], stattypes[stat] );

   fprintf( fp, "Condition      %d %d %d\n", ch->pcdata->condition[0], ch->pcdata->condition[1], ch->pcdata->condition[2] );

   fprintf( fp, "Site           %s\n", ( ch->desc && ch->desc->host ) ? ch->desc->host : "(Link-Dead)" );

   for( sn = 0; sn < top_sn; sn++ )
   {
      if( skill_table[sn]->name && ch->pcdata->learned[sn] > 0 )
      {
         switch( skill_table[sn]->type )
         {
            default:
               fprintf( fp, "%s", "Skill " );
               break;

            case SKILL_DELETED: /* Don't save one thats deleted it will bug on login */
               continue;

            case SKILL_SPELL:
               fprintf( fp, "%s", "Spell " );
               break;

            case SKILL_WEAPON:
               fprintf( fp, "%s", "Weapon" );
               break;

            case SKILL_TONGUE:
               fprintf( fp, "%s", "Tongue" );
               break;
         }
         fprintf( fp, "         %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
      }
   }

   fwrite_chaffect( fp, ch->first_affect );

   for( killed = ch->pcdata->first_killed; killed; killed = killed->next )
   {
      /* Make sure its a valid vnum currently */
      if( !get_mob_index( killed->vnum ) )
         continue;
      fprintf( fp, "Killed         %d %d\n", killed->vnum, killed->count );
   }

   for( fexp = ch->pcdata->first_explored; fexp; fexp = fexp->next )
      fprintf( fp, "Explored       %d\n", fexp->vnum );

   /* Save color values - Samson 9-29-98 */
   {
      int x;

      fprintf( fp, "MaxColors      %d\n", MAX_COLORS );
      fprintf( fp, "Colors         " );
      for( x = 0; x < MAX_COLORS; x++ )
         fprintf( fp, "%d ", ch->colors[x] );
      fprintf( fp, "\n" );
   }

#ifdef IMC
   imc_savechar( ch, fp );
#endif

   fprintf( fp, "End\n\n" );
}

/* Write an object and its contents. */
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest, short os_type, bool hotboot )
{
   EXTRA_DESCR_DATA *ed;
   int stat;

   if( !obj )
      return;

   if( iNest >= MAX_NEST )
   {
      bug( "%s: iNest hit MAX_NEST %d", __FUNCTION__, iNest );
      return;
   }

   /* Slick recursion to write lists backwards, so loading them will load in forwards order. */
   if( obj->prev_content && os_type == OS_CARRY )
      fwrite_obj( ch, obj->prev_content, fp, iNest, OS_CARRY, hotboot );

   /* Do NOT save prototype items! - Thoric */
   if( !hotboot )
   {
      if( ( obj->item_type == ITEM_KEY && !is_obj_stat( obj, ITEM_CLANOBJECT ) )
      || is_obj_stat( obj, ITEM_PROTOTYPE ) )
         return;
   }

   /* DO NOT save corpses lying on the ground as a hotboot item, they are already saved elsewhere! - Samson */
   if( hotboot && obj->item_type == ITEM_CORPSE_PC )
      return;

   fprintf( fp, "%s\n", ( os_type == OS_CORPSE ? "#CORPSE" : os_type == OS_LOCKER ? "#LOCKER" : "#OBJECT" ) );

   if( iNest )
      fprintf( fp, "Nest         %d\n", iNest );
   if( obj->count > 1 )
      fprintf( fp, "Count        %d\n", obj->count );
   if( obj->name && str_cmp( obj->name, obj->pIndexData->name ) )
      fprintf( fp, "Name         %s~\n", strip_cr( obj->name ) );
   if( obj->short_descr && str_cmp( obj->short_descr, obj->pIndexData->short_descr ) )
      fprintf( fp, "ShortDescr   %s~\n", strip_cr( obj->short_descr ) );
   if( obj->description && str_cmp( obj->description, obj->pIndexData->description ) )
      fprintf( fp, "Description  %s~\n", strip_cr( obj->description ) );
   if( obj->desc && str_cmp( obj->desc, obj->pIndexData->desc ) )
      fprintf( fp, "Desc         %s~\n", strip_cr( obj->desc ) );
   if( obj->action_desc && str_cmp( obj->action_desc, obj->pIndexData->action_desc ) )
      fprintf( fp, "ActionDesc   %s~\n", strip_cr( obj->action_desc ) );
   fprintf( fp, "Vnum         %d\n", obj->pIndexData->vnum );
   if( obj->bsplatter )
      fprintf( fp, "BSplatter    %d\n", obj->bsplatter );
   if( obj->bstain )
      fprintf( fp, "BStain       %d\n", obj->bstain );
   if( ( os_type == OS_CORPSE || hotboot ) && obj->in_room )
   {
      fprintf( fp, "Room         %d\n", obj->in_room->vnum );
      fprintf( fp, "Rvnum        %d\n", obj->room_vnum );
   }
   if( obj->owner && obj->owner[0] != '\0' )
      fprintf( fp, "Owner        %s~\n", obj->owner );
   if( !xSAME_BITS( obj->extra_flags, obj->pIndexData->extra_flags ) )
      fprintf( fp, "ExtraFlags   %s~\n", ext_flag_string( &obj->extra_flags, o_flags ) );
   if( !xSAME_BITS( obj->wear_flags, obj->pIndexData->wear_flags ) )
      fprintf( fp, "WearFlags    %s~\n", ext_flag_string( &obj->wear_flags, w_flags ) );

   /* Need to save where it was at */
   if( obj->t_wear_loc != -1 )
      fprintf( fp, "WearLoc      %s~\n", wear_locs[obj->t_wear_loc] );

   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( obj->stat_reqs[stat] == 0 )
         continue;
      fprintf( fp, "NStat        %d %s~\n", obj->stat_reqs[stat], stattypes[stat] );
   }

   if( !xIS_EMPTY( obj->class_restrict ) && !xSAME_BITS( obj->class_restrict, obj->pIndexData->class_restrict ) )
   {
      int iclass;

      fprintf( fp, "%s", "Classes " );
      for( iclass = 0; iclass < MAX_PC_CLASS; iclass++ )
      {
         if( class_table[iclass] && xIS_SET( obj->class_restrict, iclass ) && class_table[iclass]->name )
            fprintf( fp, " %s", class_table[iclass]->name );
      }
      fprintf( fp, "%s", "~\n" );
   }

   if( !xIS_EMPTY( obj->race_restrict ) && !xSAME_BITS( obj->race_restrict, obj->pIndexData->race_restrict ) )
   {
      int irace;

      fprintf( fp, "%s", "Races   " );
      for( irace = 0; irace < MAX_PC_RACE; irace++ )
      {
         if( race_table[irace] && xIS_SET( obj->race_restrict, irace ) && race_table[irace]->name )
            fprintf( fp, " %s", race_table[irace]->name );
      }
      fprintf( fp, "%s", "~\n" );
   }

   if( obj->item_type != obj->pIndexData->item_type )
      fprintf( fp, "ItemType     %s~\n", o_types[obj->item_type] );
   if( obj->weight != obj->pIndexData->weight )
      fprintf( fp, "Weight       %d\n", obj->weight );
   if( obj->level != obj->pIndexData->level )
      fprintf( fp, "Level        %d\n", obj->level );
   if( obj->timer )
      fprintf( fp, "Timer        %d\n", obj->timer );
   if( obj->cost != obj->pIndexData->cost )
      fprintf( fp, "Cost         %d\n", obj->cost );
   if( obj->value[0] || obj->value[1] || obj->value[2] || obj->value[3] || obj->value[4] || obj->value[5] )
      fprintf( fp, "Values       %d %d %d %d %d %d\n",
         obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4], obj->value[5] );

   switch( obj->item_type )
   {
      case ITEM_PILL:
      case ITEM_POTION:
      case ITEM_SCROLL:
         if( is_valid_sn( obj->value[1] ) )
            fprintf( fp, "Spell 1      '%s'\n", skill_table[obj->value[1]]->name );
         if( is_valid_sn( obj->value[2] ) )
            fprintf( fp, "Spell 2      '%s'\n", skill_table[obj->value[2]]->name );
         if( is_valid_sn( obj->value[3] ) )
            fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         if( is_valid_sn( obj->value[3] ) )
            fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );
         break;

      case ITEM_SALVE:
         if( is_valid_sn( obj->value[4] ) )
            fprintf( fp, "Spell 4      '%s'\n", skill_table[obj->value[4]]->name );
         if( is_valid_sn( obj->value[5] ) )
            fprintf( fp, "Spell 5      '%s'\n", skill_table[obj->value[5]]->name );
         break;
   }

   fwrite_objaffects( fp, obj->first_affect );

   for( ed = obj->first_extradesc; ed; ed = ed->next )
      fprintf( fp, "ExtraDescr   %s~ %s~\n", ed->keyword, ed->description ? strip_cr( ed->description ) : "" );

   fprintf( fp, "End\n\n" );

   if( obj->first_content )
      fwrite_obj( ch, obj->last_content, fp, iNest + 1, OS_CARRY, hotboot );
}

/* Load a char and inventory into a new ch structure. */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name, bool preload, bool copyover )
{
   CHAR_DATA *ch;
   FILE *fp;
   char strsave[MIL];
   int i;
   bool found;

   CREATE( ch, CHAR_DATA, 1 );
   if( !ch )
   {
      bug( "%s: couldn't create ch.", __FUNCTION__ );
      return false;
   }
   clear_char( ch );
   loading_char = ch;

   d->character = ch;
   ch->desc = d;
   ch->act = multimeb( PLR_BLANK, PLR_COMBINE, PLR_PROMPT, -1 );

   CREATE( ch->pcdata, PC_DATA, 1 );
   if( !ch->pcdata )
   {
      bug( "%s: couldn't create ch->pcdata.", __FUNCTION__ );
      return false;
   }
   LINK( ch->pcdata, first_pc, last_pc, next, prev ); /* Keep a link of the pcdata */
   ch->pcdata->nextregen = 4;
   ch->pcdata->character = ch;
   ch->pcdata->onboard = 1;
   ch->pcdata->filename = STRALLOC( name );
   ch->pcdata->condition[COND_THIRST] = 48;
   ch->pcdata->condition[COND_FULL] = 48;
   ch->pcdata->condition[COND_DRUNK] = 0;
   ch->pcdata->wizinvis = 0;
   ch->pcdata->charmies = 0;
   for( i = 0; i < MAX_SKILL; i++ )
      ch->pcdata->learned[i] = 0;
   ch->pcdata->news_read = 0;
   ch->pcdata->pagerlen = 24;
   ch->pcdata->kltime = 5;
   ch->pcdata->first_ignored = ch->pcdata->last_ignored = NULL;
   ch->pcdata->first_tell = ch->pcdata->last_tell = NULL;
   ch->pcdata->first_say = ch->pcdata->last_say = NULL;
   ch->pcdata->first_yell = ch->pcdata->last_yell = NULL;
   ch->pcdata->first_fchat = ch->pcdata->last_fchat = NULL;
   ch->pcdata->first_whisper = ch->pcdata->last_whisper = NULL;
   ch->pcdata->hotboot = false;  /* Never changed except when PC is saved during hotboot save */
   ch->pcdata->clan = NULL;
   ch->pcdata->nation = NULL;
   ch->pcdata->council = NULL;
   ch->pcdata->deity = NULL;
   ch->pcdata->first_pet = ch->pcdata->last_pet = NULL;
   ch->pcdata->first_mclass = ch->pcdata->last_mclass = NULL;
   ch->pcdata->first_explored = ch->pcdata->last_explored = NULL;
   ch->pcdata->first_friend = ch->pcdata->last_friend = NULL;
   ch->pcdata->pwd = NULL;
   ch->pcdata->bamfin = ch->pcdata->bamfout = NULL;
   ch->pcdata->rank = NULL;
   ch->pcdata->bestowments = NULL;
   ch->pcdata->title = NULL;
   ch->pcdata->homepage = NULL;
   ch->pcdata->email = NULL;
   ch->pcdata->msn = NULL;
   ch->pcdata->yahoo = NULL;
   ch->pcdata->gtalk = NULL;
   ch->pcdata->bio = NULL;
   ch->pcdata->prompt = ch->pcdata->fprompt = NULL;
   ch->pcdata->range_lo = ch->pcdata->range_hi = 0;
   ch->pcdata->quest_curr = 0;
   ch->pcdata->questcompleted = 0;

#ifdef IMC
   imc_initchar( ch );
#endif

   found = false;
   if( valid_pfile( name ) )
   {
      log_printf_plus( LOG_COMM, PERM_HEAD, "%s player data for: %s",
         preload ? "Preloading" : "Loading", ch->pcdata->filename );
   }
   else /* No player found */
   {
      if( !ch->name )
         ch->name = STRALLOC( name );
      return false;
   }

   snprintf( strsave, sizeof( strsave ), "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );
   if( ( fp = fopen( strsave, "r" ) ) )
   {
      int iNest;

      for( iNest = 0; iNest < MAX_NEST; iNest++ )
         rgObjNest[iNest] = NULL;

      found = true;

      /* Cheat so that bug will show line #'s -- Altrag */
      fpArea = fp;
      mudstrlcpy( strArea, strsave, sizeof( strArea ) );
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
            bug( "%s: # not found for %s.", __FUNCTION__, name );
            break;
         }

         word = fread_word( fp );
         if( !strcmp( word, "PLAYER" ) )
         {
            fread_char( ch, fp, preload, copyover );
            if( preload )
               break;
         }
         else if( !strcmp( word, "OBJECT" ) )   /* Objects  */
            fread_obj( ch, NULL, fp, OS_CARRY );
         else if( !strcmp( word, "MorphData" ) )   /* Morphs */
            fread_morph_data( ch, fp );
         else if( !strcmp( word, "MOBILE" ) )
         {
            CHAR_DATA *mob;
            CHAR_DATA *pet;
            int count = 0;

            if( ( mob = fread_mobile( fp ) ) )
            {
               for( pet = ch->pcdata->first_pet; pet; pet = pet->next_pet )
                  count++;
               if( count < sysdata.maxpet )
               {
                  LINK( mob, ch->pcdata->first_pet, ch->pcdata->last_pet, next_pet, prev_pet );
                  mob->master = ch;
                  xSET_BIT( mob->affected_by, AFF_CHARM );
                  if( ch->position == POS_MOUNTED && xIS_SET( mob->act, ACT_MOUNTED ) && ch->in_room == mob->in_room )
                  {
                     ch->mount = mob;
                     mob->mounter = ch;
                     ch->position = POS_MOUNTED;
                  }
                  else
                     xREMOVE_BIT( mob->act, ACT_MOUNTED );
               }
               else
               {
                  bug( "%s: %s already has the max number of pets - skipping", __FUNCTION__, ch->name );
                  extract_char( mob, true );
               }
            }
            else
               bug( "%s: Deleted mob saved on %s - skipping", __FUNCTION__, ch->name );
         }
         else if( !strcmp( word, "END" ) )   /* Done     */
            break;
         else
         {
            bug( "%s: bad section in %s.", __FUNCTION__, name );
            break;
         }
      }
      fclose( fp );
      fp = NULL;
      fpArea = NULL;
      mudstrlcpy( strArea, "$", sizeof( strArea ) );
   }

   if( !ch->name )
      ch->name = STRALLOC( name );
   if( found )
   {
      if( is_immortal( ch ) )
      {
         if( ch->pcdata->wizinvis < 1 || ch->pcdata->wizinvis > get_trust( ch ) )
            ch->pcdata->wizinvis = get_trust( ch );
         assign_area( ch );
      }
      else
         ch->pcdata->wizinvis = 0;

      re_equipchar( ch );
   }
   update_aris( ch );
   if( ch->position == POS_MOUNTED && !ch->mount )
      ch->position = POS_STANDING;
   loading_char = NULL;
   if( !preload )
      set_ch_personal( ch->pcdata );
   return found;
}

/* Read in a char. */
void fread_char( CHAR_DATA *ch, FILE *fp, bool preload, bool copyover )
{
   AFFECT_DATA *paf;
   const char *word;
   char *line, *infoflags, flag[MIL];
   double tmpexp = 0.0;
   int x1, x2, x3, x4, max_colors = 0, value, tmpclass = -1;
   bool fMatch;

   file_ver = 0;

   /* Setup color values in case player has none set - Samson */
   memcpy( &ch->colors, &default_set, sizeof( default_set ) );

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
            if( !str_cmp( word, "Absorb" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            WEXTKEY( "Act", ch->act, fp, plr_flags, PLR_MAX );
            WEXTKEY( "AffectedBy", ch->affected_by, fp, a_flags, AFF_MAX );
            KEY( "Alignment", ch->alignment, fread_number( fp ) );
            KEY( "Armor", ch->armor, fread_number( fp ) );
            if( !strcmp( word, "Affect" ) || !strcmp( word, "AffectData" ) )
            {
               fMatch = true;
               if( preload )
               {
                  fread_to_eol( fp );
                  break;
               }
               if( ( paf = fread_chaffect( fp, 0, __FILE__, __LINE__ ) ) )
                  LINK( paf, ch->first_affect, ch->last_affect, next, prev );
               break;
            }

            if( !strcmp( word, "AttrPerm" ) )
            {
               int max = is_immortal( ch ) ? ( MAX_LEVEL + 25 ) : MAX_LEVEL;

               /* These are the default ones so go ahead and convert them incase someone changes one day */
               ch->perm_stats[STAT_STR] = URANGE( 1, fread_number( fp ), max );
               ch->perm_stats[STAT_INT] = URANGE( 1, fread_number( fp ), max );
               ch->perm_stats[STAT_WIS] = URANGE( 1, fread_number( fp ), max );
               ch->perm_stats[STAT_DEX] = URANGE( 1, fread_number( fp ), max );
               ch->perm_stats[STAT_CON] = URANGE( 1, fread_number( fp ), max );
               ch->perm_stats[STAT_CHA] = URANGE( 1, fread_number( fp ), max );
               ch->perm_stats[STAT_LCK] = URANGE( 1, fread_number( fp ), max );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "AttrMod" ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               ch->mod_stats[STAT_STR] = fread_number( fp );
               ch->mod_stats[STAT_INT] = fread_number( fp );
               ch->mod_stats[STAT_WIS] = fread_number( fp );
               ch->mod_stats[STAT_DEX] = fread_number( fp );
               ch->mod_stats[STAT_CON] = fread_number( fp );
               ch->mod_stats[STAT_CHA] = fread_number( fp );
               ch->mod_stats[STAT_LCK] = fread_number( fp );

               fMatch = true;
               break;
            }
            break;

         case 'B':
            if( !str_cmp( word, "BGold" ) )
            {
               fread_number( fp );
               fMatch = true;
               break;
            }
            KEY( "BSplatter", ch->bsplatter, fread_number( fp ) );
            KEY( "Bamfin", ch->pcdata->bamfin, fread_string( fp ) );
            KEY( "Bamfout", ch->pcdata->bamfout, fread_string( fp ) );
            KEY( "Bestowments", ch->pcdata->bestowments, fread_string( fp ) );
            KEY( "Bio", ch->pcdata->bio, fread_string( fp ) );
            if( !strcmp( word, "Birthday" ) )
            {
               ch->pcdata->birth_month = fread_number( fp );
               ch->pcdata->birth_day = fread_number( fp );
               ch->pcdata->birth_year = fread_number( fp );
               if( ch->pcdata->birth_year > time_info.year )
                  ch->pcdata->birth_year = ( time_info.year - 18 );
               fMatch = true;
               break;
            }
            break;

         case 'C':
            if( !strcmp( word, "Cords" ) )
            {
               ch->cords[0] = fread_number( fp );
               ch->cords[1] = fread_number( fp );
               fMatch = true;
               break;
            }
            KEY( "Channels", ch->pcdata->channels, fread_string( fp ) );
            if( !strcmp( word, "Clan" ) )
            {
               CLAN_DATA *clan;

               infoflags = fread_flagstring( fp );
               if( infoflags && infoflags[0] != '\0' )
               {
                  if( !( clan = get_clan( infoflags ) ) )
                  {
                     if( !preload )
                        ch_printf( ch, "The organization %s no longer exists.\r\n", infoflags );
                  }
                  else if( !is_clan_member( clan, ch->name ) )
                  {
                     if( !preload )
                        ch_printf( ch, "You're no longer a member of the organization %s.\r\n", infoflags );
                  }
                  else
                     ch->pcdata->clan = clan;
               }
               fMatch = true;
               break;
            }

            if( !str_cmp( word, "Class" ) )
            {
               infoflags = fread_flagstring( fp );
               value = get_pc_class( infoflags );
               if( value < 0 || value >= MAX_PC_CLASS )
                  bug( "%s: invalid class (%s)", __FUNCTION__, infoflags );
               else
                  tmpclass = value;
               fMatch = true;
               break;
            }

            /* Load color values - Samson 9-29-98 */
            if( !str_cmp( word, "Colors" ) )
            {
               int x;

               for( x = 0; x < max_colors; x++ )
               {
                  if( x < MAX_COLORS )
                     ch->colors[x] = fread_number( fp );
                  else
                     fread_number( fp );
               }
               fMatch = true;
               break;
            }

            if( !str_cmp( word, "Condition" ) )
            {
               line = fread_line( fp );
               sscanf( line, "%d %d %d %d", &x1, &x2, &x3, &x4 );
               ch->pcdata->condition[0] = x1;
               ch->pcdata->condition[1] = x2;
               ch->pcdata->condition[2] = x3;
               fMatch = true;
               break;
            }

            if( !strcmp( word, "Council" ) )
            {
               infoflags = fread_flagstring( fp );
               if( infoflags && infoflags[0] != '\0' && !( ch->pcdata->council = get_council( infoflags ) ) )
               {
                  if( !preload )
                     ch_printf( ch, "The council %s no longer exists.\r\n", infoflags );
               }
               fMatch = true;
               break;
            }
            break;

         case 'D':
            if( !strcmp( word, "DisSudoku" ) )
            {
               fread_sudoku( ch, true, fp );
               fMatch = true;
               break;
            }
            WEXTKEY( "Deaf", ch->deaf, fp, channelflags, CHANNEL_MAX );
            KEY( "Damroll", ch->damroll, fread_number( fp ) );
            if( !strcmp( word, "Deity" ) )
            {
               DEITY_DATA *deity;

               infoflags = fread_flagstring( fp );
               if( infoflags && infoflags[0] != '\0' )
               {
                  if( !( deity = get_deity( infoflags ) ) )
                  {
                     if( !preload )
                     {
                        ch_printf( ch, "The deity %s no longer exists.\r\n", infoflags );
                        ch->pcdata->favor = 0;
                     }
                  }
                  else if( !is_deity_worshipper( deity, ch->name ) )
                  {
                     if( !preload )
                     {
                        ch_printf( ch, "You're no longer a worshipper of the deity %s.\r\n", infoflags );
                        ch->pcdata->favor = 0;
                     }
                  }
                  else
                     ch->pcdata->deity = deity;
               }
               fMatch = true;
               break;
            }
            KEY( "Description", ch->description, fread_string( fp ) );
            break;

         /* 'E' was moved to after 'S' */

         case 'F':
            if( !strcmp( word, "FChatHist" ) )
            {
               fread_phistory( ch, fp, 4 );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Friend" ) )
            {
               FRIEND_DATA *afriend;
               int sex = fread_number( fp );

               /* If in pfile lets assume it is approved */
               infoflags = fread_flagstring( fp );
               if( infoflags && infoflags[0] != '\0' )
               {
                  if( !( afriend = add_friend( ch, infoflags ) ) )
                  {
                     if( !preload )
                        ch_printf( ch, "Your friend %s no longer exist and has been removed from your list.\r\n", infoflags );
                  }
                  else
                  {
                     afriend->sex = sex;
                     afriend->approved = true;
                  }
               }
               fMatch = true;
               break;
            }

            KEY( "Favor", ch->pcdata->favor, fread_number( fp ) );
            if( !strcmp( word, "Filename" ) )
            {
               /* File Name already set externally. */
               fread_to_eol( fp );
               fMatch = true;
               break;
            }
            WEXTKEY( "Flags", ch->pcdata->flags, fp, pc_flags, PCFLAG_MAX );
            KEY( "FPrompt", ch->pcdata->fprompt, fread_string( fp ) );
            break;

         case 'G':
            KEY( "Glory", ch->pcdata->quest_curr, fread_un_number( fp ) );
            KEY( "Gold", ch->gold, fread_number( fp ) );
            KEY( "GTalk", ch->pcdata->gtalk, fread_string( fp ) );
            break;

         case 'H':
            if( !strcmp( word, "Host" ) )
            {
               HOST_DATA *host;

               CREATE( host, HOST_DATA, 1 );
               host->prefix = fread_number( fp );
               host->host = fread_string( fp );
               host->suffix = fread_number( fp );
               host->next = host->prev = NULL;
               LINK( host, ch->first_host, ch->last_host, next, prev );
               fMatch = true;
               break;
            }
            KEY( "Height", ch->height, fread_number( fp ) );
            KEY( "Hitroll", ch->hitroll, fread_number( fp ) );
            KEY( "Homepage", ch->pcdata->homepage, fread_string( fp ) );

            if( !strcmp( word, "HpManaMove" ) )
            {
               ch->hit = fread_number( fp );
               ch->max_hit = fread_number( fp );
               ch->mana = fread_number( fp );
               ch->max_mana = fread_number( fp );
               ch->move = fread_number( fp );
               ch->max_move = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'I':
            if( !strcmp( word, "Ignored" ) )
            {
               IGNORE_DATA *inode;
               char *temp;

               /* Get the name */
               temp = fread_flagstring( fp );

               /* Add the name to the list if it's a valid pfile */
               if( valid_pfile( temp ) )
               {
                  CREATE( inode, IGNORE_DATA, 1 );
                  inode->name = STRALLOC( temp );
                  inode->next = NULL;
                  inode->prev = NULL;
                  LINK( inode, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
               }
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Immune" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
#ifdef IMC
            if( ( fMatch = imc_loadchar( ch, fp, word ) ) )
               break;
#endif
            break;

         case 'K':
            KEY( "KeepAlive", ch->pcdata->kltime, fread_number( fp ) );
            if( !strcmp( word, "Killed" ) )
            {
               KILLED_DATA *killed;
               int vnum, count;

               vnum = fread_number( fp );
               count = fread_number( fp );
               /* Can just toss it all in next time they kill something it will remove any it doesn't need */
               CREATE( killed, KILLED_DATA, 1 );
               killed->vnum = vnum;
               killed->count = count;
               LINK( killed, ch->pcdata->first_killed, ch->pcdata->last_killed, next, prev );
               fMatch = true;
               break;
            }
            break;

         case 'L':
            KEY( "Level", ch->level, fread_number( fp ) );
            KEY( "LongDescr", ch->long_descr, fread_string( fp ) );
            if( !strcmp( word, "Logged" ) )
            {
               ch->logged = true;
               fMatch = true;
               break;
            }
            break;

         case 'M':
            if( !str_cmp( word, "MClass" ) )
            {
               MCLASS_DATA *mclass, *mcnext;
               double exp;
               int mcount = 0, wclass, level, cpercent;

               for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
                  mcount++;

               infoflags = fread_flagstring( fp );
               if( is_number( infoflags ) )
                  value = atoi( infoflags );
               else
                  value = get_pc_class( infoflags );

               wclass = value;
               level = fread_number( fp );
               cpercent = fread_number( fp );
               exp = fread_double( fp );

               if( value < 0 || value >= MAX_PC_CLASS )
               {
                  if( mcount >= 1 )
                     bug( "%s: invalid class (%s)", __FUNCTION__, infoflags );
                  else /* Need to set up something for default */
                  {
                     add_mclass( ch, -1, level, cpercent, exp, false );
                     mcount++;
                  }
               }
               else
               {
                  add_mclass( ch, wclass, level, cpercent, exp, false );
                  mcount++;
               }

               /* Remove any unneeded ones */
               for( mclass = ch->pcdata->first_mclass; mclass; mclass = mcnext )
               {
                  mcnext = mclass->next;
                  if( mclass->wclass < 0 || mclass->wclass >= MAX_PC_CLASS )
                  {
                     if( mcount > 1 )
                     {
                        UNLINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
                        DISPOSE( mclass );
                        mcount--;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            KEY( "MSN", ch->pcdata->msn, fread_string( fp ) );
            KEY( "MaxColors", max_colors, fread_number( fp ) );
            KEY( "MDeaths", ch->pcdata->mdeaths, fread_un_number( fp ) );
            KEY( "Mentalstate", ch->mental_state, fread_number( fp ) );
            KEY( "Minsnoop", ch->pcdata->min_snoop, fread_number( fp ) );
            KEY( "MKills", ch->pcdata->mkills, fread_un_number( fp ) );
            KEY( "Mobinvis", ch->mobinvis, fread_number( fp ) );
            if( !str_cmp( word, "MGold" ) )
            {
               fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'N':
            SKEY( "NTrust", ch->trust, fp, perms_flag, PERM_MAX );
            if( !str_cmp( word, "NStat" ) )
            {
               int ustat, umstat, stat;

               stat = fread_number( fp );
               umstat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
               {
                  ch->perm_stats[ustat] = stat;
                  ch->mod_stats[ustat] = umstat;
               }
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Nation" ) )
            {
               CLAN_DATA *nation;

               infoflags = fread_flagstring( fp );
               if( infoflags && infoflags[0] != '\0' )
               {
                  if( !( nation = get_clan( infoflags ) ) )
                  {
                     if( !preload )
                        ch_printf( ch, "The nation %s no longer exists.\r\n", infoflags );
                  }
                  else if( !is_clan_member( nation, ch->name ) )
                  {
                     if( !preload )
                        ch_printf( ch, "You're no longer a member of the nation %s.\r\n", infoflags );
                  }
                  else
                     ch->pcdata->nation = nation;
               }
               fMatch = true;
               break;
            }
            KEY( "NewsRead", ch->pcdata->news_read, fread_number( fp ) );
            KEY( "Name", ch->name, fread_string( fp ) );
            if( !str_cmp( word, "NResistant" ) )
            {
               int tmpvalue = fread_number( fp );
               infoflags = fread_flagstring( fp );
               value = get_flag( infoflags, ris_flags, RIS_MAX );
               if( value < 0 || value >= RIS_MAX )
                  bug( "%s: Unknown %s: %s", __FUNCTION__, word, infoflags );
               else
                  ch->resistant[value] = tmpvalue;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "NoResistant" ) || !str_cmp( word, "NoSusceptible" )
            || !str_cmp( word, "NoImmune" ) || !str_cmp( word, "NoAbsorb" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            WEXTKEY( "NoAffectedBy", ch->no_affected_by, fp, a_flags, AFF_MAX );
            break;

         case 'P':
            KEY( "Pagerlen", ch->pcdata->pagerlen, fread_number( fp ) );
            KEY( "Password", ch->pcdata->pwd, fread_string( fp ) );
            KEY( "PDeaths", ch->pcdata->pdeaths, fread_un_number( fp ) );
            KEY( "PKills", ch->pcdata->pkills, fread_un_number( fp ) );
            KEY( "Played", ch->played, fread_number( fp ) );
            SKEY( "Position", ch->position, fp, pos_names, POS_MAX );
            KEY( "Practice", ch->practice, fread_number( fp ) );
            KEY( "Prompt", ch->pcdata->prompt, fread_string( fp ) );
            if( !strcmp( word, "PTimer" ) )
            {
               add_timer( ch, TIMER_PKILLED, fread_number( fp ), NULL, 0 );
               fMatch = true;
               break;
            }
            break;

         case 'Q':
            KEY( "QCountdown", ch->questcountdown, fread_number( fp ) );
            KEY( "QGiver", ch->questgiver, fread_number( fp ) );
            KEY( "QVnum", ch->questvnum, fread_number( fp ) );
            KEY( "QType", ch->questtype, fread_number( fp ) );
            KEY( "QCompleted", ch->pcdata->questcompleted, fread_un_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Rubik" ) )
            {
               short rnum;

               for( rnum = 0; rnum < 54; rnum++ )
                  ch->pcdata->rubik[rnum] = fread_number( fp );

               fMatch = true;
               break;
            }
            KEY( "RubikWins", ch->pcdata->rwins, fread_un_number( fp ) );
            KEY( "RubikQuits", ch->pcdata->rquits, fread_un_number( fp ) );

            if( !str_cmp( word, "Race" ) )
            {
               infoflags = fread_flagstring( fp );
               value = get_pc_race( infoflags );
               if( value < 0 || value >= MAX_PC_RACE )
               {
                  bug( "%s: invalid race (%s)", __FUNCTION__, infoflags );
                  ch->race = 0;
               }
               else
                  ch->race = value;
               fMatch = true;
               break;
            }
            KEY( "Rank", ch->pcdata->rank, fread_string( fp ) );
            if( !str_cmp( word, "Resistant" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            KEY( "Restore_time", ch->pcdata->restore_time, fread_number( fp ) );
            if( !strcmp( word, "Room" ) )
            {
               ch->in_room = get_room_index( fread_number( fp ) );
               if( !ch->in_room )
                  ch->in_room = get_room_index( sysdata.room_limbo );
               fMatch = true;
               break;
            }
            break;

         case 'S':
            if( !strcmp( word, "SayHist" ) )
            {
               fread_phistory( ch, fp, 1 );
               fMatch = true;
               break;
            }
            KEY( "Spouse", ch->pcdata->spouse, fread_string( fp ) );
            if( !strcmp( word, "Speed" ) )
            {
               ch->pcdata->speed = fread_number( fp );
               if( ch->desc )
                  ch->desc->speed = ch->pcdata->speed;
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Sudoku" ) )
            {
               fread_sudoku( ch, false, fp );
               fMatch = true;
               break;
            }
            KEY( "SudokuWins", ch->pcdata->swins, fread_un_number( fp ) );
            KEY( "SudokuQuits", ch->pcdata->squits, fread_un_number( fp ) );
            KEY( "SudokuStart", ch->pcdata->sstarttime, fread_time( fp ) );
            KEY( "SudokuFast", ch->pcdata->sfastesttime, fread_time( fp ) );
            KEY( "SudokuSlow", ch->pcdata->sslowesttime, fread_time( fp ) );
            KEY( "SudokuLast", ch->pcdata->slasttime, fread_time( fp ) );
            SKEY( "Sex", ch->sex, fp, sex_names, SEX_MAX );
            SKEY( "Style", ch->style, fp, style_names, STYLE_MAX );
            KEY( "ShortDescr", ch->short_descr, fread_string( fp ) );
            if( !str_cmp( word, "Susceptible" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            WEXTKEY( "Speaks", ch->speaks, fp, lang_names, LANG_UNKNOWN );
            WEXTKEY( "Speaking", ch->speaking, fp, lang_names, LANG_UNKNOWN );
            if( !strcmp( word, "SavingThrows" ) )
            {
               ch->saving_poison_death = fread_number( fp );
               ch->saving_wand = fread_number( fp );
               ch->saving_para_petri = fread_number( fp );
               ch->saving_breath = fread_number( fp );
               ch->saving_spell_staff = fread_number( fp );
               fMatch = true;
               break;
            }

            if( !strcmp( word, "Site" ) )
            {
               if( !preload && !copyover )
                  ch_printf( ch, "Last connected from: %s\r\n", fread_word( fp ) );
               else
                  fread_to_eol( fp );
               fMatch = true;
               if( preload )
                  word = "End";
               else
                  break;
            }

            if( !strcmp( word, "Skill" ) )
            {
               int sn;

               if( preload )
                  word = "End";
               else
               {
                  value = fread_number( fp );
                  infoflags = fread_word( fp );
                  sn = bsearch_skill_exact( infoflags, gsn_first_skill, gsn_first_weapon - 1 );
                  if( sn < 0 )
                     bug( "%s: unknown skill (%d)(%s).", __FUNCTION__, value, infoflags );
                  else
                  {
                     ch->pcdata->learned[sn] = value;

                     if( get_trust( ch ) < PERM_IMM )
                     {
                        if( ch->pcdata->learned[sn] > get_adept( ch, sn ) )
                        {
                           ch->pcdata->learned[sn] = get_adept( ch, sn );
                           if( ch->pcdata->learned[sn] <= 0 )
                              ch->practice++;
                        }
                     }
                  }
                  fMatch = true;
                  break;
               }
            }

            if( !strcmp( word, "Spell" ) )
            {
               int sn;

               if( preload )
                  word = "End";
               else
               {
                  value = fread_number( fp );
                  infoflags = fread_word( fp );
                  sn = bsearch_skill_exact( infoflags, gsn_first_spell, gsn_first_skill - 1 );
                  if( sn < 0 )
                     bug( "%s: unknown spell (%d)(%s).", __FUNCTION__, value, infoflags );
                  else
                  {
                     ch->pcdata->learned[sn] = value;

                     if( get_trust( ch ) < PERM_IMM )
                     {
                        if( ch->pcdata->learned[sn] > get_adept( ch, sn ) )
                        {
                           ch->pcdata->learned[sn] = get_adept( ch, sn );
                           if( ch->pcdata->learned[sn] <= 0 )
                              ch->practice++;
                        }
                     }
                  }
                  fMatch = true;
                  break;
               }
            }
            if( strcmp( word, "End" ) )
               break;

         case 'E':
            KEY( "Email", ch->pcdata->email, fread_string( fp ) );
            if( !strcmp( word, "Explored" ) )
            {
               EXP_DATA *fexp;
               int vnum;

               vnum = fread_number( fp );
               if( get_room_index( vnum ) ) /* Valid vnum? */
               {
                  CREATE( fexp, EXP_DATA, 1 );
                  fexp->vnum = vnum;
                  LINK( fexp, ch->pcdata->first_explored, ch->pcdata->last_explored, next, prev );
               }
               fMatch = true;
               break;
            }
            if( !strcmp( word, "End" ) )
            {
               MCLASS_DATA *mclass;
               int cpercent = 0;

               if( !ch->pcdata->birth_year )
               {
                  ch->pcdata->birth_month = time_info.month;
                  ch->pcdata->birth_day = time_info.day;
                  ch->pcdata->birth_year = UMAX( 1, ( time_info.year - 17 ) );
               }
               if( tmpclass >= 0 || !ch->pcdata->first_mclass )
               {
                  add_mclass( ch, tmpclass, ch->level, 100, tmpexp, false );
                  tmpclass = -1;
                  tmpexp = 0;
               }
               for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
               {
                  if( ( cpercent + mclass->cpercent ) > 100 )
                  {
                     mclass->cpercent = ( 100 - cpercent );
                     cpercent = 100;
                  }
                  else
                     cpercent += mclass->cpercent;
               }
               ch->editor = NULL;

               if( !is_immortal( ch ) && xIS_EMPTY( ch->speaking ) )
                  xSET_BIT( ch->speaking, LANG_COMMON );
               if( xIS_SET( ch->act, PLR_QUESTOR ) )
               {
                  ch->questgiver = 0;
                  ch->questvnum = 0;
                  ch->questcountdown = 5;
                  ch->questtype = 0;
                  xREMOVE_BIT( ch->act, PLR_QUESTOR );
                  if( !preload )
                     send_to_char( "Your quest has been removed and you must wait 5 minutes to quest again.\r\n", ch );
               }
               return;
            }
            KEY( "Exp", tmpexp, fread_double( fp ) );
            break;

         case 'T':
            if( !strcmp( word, "TellHist" ) )
            {
               fread_phistory( ch, fp, 0 );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Tongue" ) )
            {
               int sn;

               if( preload )
                  word = "End";
               else
               {
                  value = fread_number( fp );
                  infoflags = fread_word( fp );
                  sn = bsearch_skill_exact( infoflags, gsn_first_tongue, gsn_top_sn - 1 );
                  if( sn < 0 )
                     bug( "%s: unknown tongue (%d)(%s).", __FUNCTION__, value, infoflags );
                  else
                  {
                     ch->pcdata->learned[sn] = value;

                     if( get_trust( ch ) < PERM_IMM )
                     {
                        if( ch->pcdata->learned[sn] > get_adept( ch, sn ) )
                        {
                           ch->pcdata->learned[sn] = get_adept( ch, sn );
                           if( ch->pcdata->learned[sn] <= 0 )
                              ch->practice++;
                        }
                     }
                  }
                  fMatch = true;
               }
               break;
            }
            KEY( "Trust", ch->trust, fread_number( fp ) );
            if( !strcmp( word, "Title" ) )
            {
               infoflags = fread_flagstring( fp );
               set_title( ch, infoflags );
               fMatch = true;
               break;
            }
            break;

         case 'U':
            if( !strcmp( word, "UnkHist" ) )
            {
               fread_phistory( ch, fp, -1 );
               fMatch = true;
               break;
            }
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            if( !strcmp( word, "VnumRange" ) )
            {
               ch->pcdata->range_lo = fread_number( fp );
               ch->pcdata->range_hi = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'W':
            if( !strcmp( word, "WhispHist" ) )
            {
               fread_phistory( ch, fp, 3 );
               fMatch = true;
               break;
            }
            KEY( "Weight", ch->weight, fread_number( fp ) );
            if( !strcmp( word, "Weapon" ) )
            {
               int sn;

               if( preload )
                  word = "End";
               else
               {
                  value = fread_number( fp );
                  infoflags = fread_word( fp );
                  sn = bsearch_skill_exact( infoflags, gsn_first_weapon, gsn_first_tongue - 1 );
                  if( sn < 0 )
                     bug( "%s: unknown weapon (%d)(%s).", __FUNCTION__, value, infoflags );
                  else
                  {
                     ch->pcdata->learned[sn] = value;

                     if( get_trust( ch ) < PERM_IMM )
                     {
                        if( ch->pcdata->learned[sn] > get_adept( ch, sn ) )
                        {
                           ch->pcdata->learned[sn] = get_adept( ch, sn );
                           if( ch->pcdata->learned[sn] <= 0 )
                              ch->practice++;
                        }
                     }
                  }
                  fMatch = true;
               }
               break;
            }
            KEY( "Wimpy", ch->wimpy, fread_number( fp ) );
            KEY( "WizInvis", ch->pcdata->wizinvis, fread_number( fp ) );
            break;

         case 'Y':
            if( !strcmp( word, "YellHist" ) )
            {
               fread_phistory( ch, fp, 2 );
               fMatch = true;
               break;
            }
            KEY( "Yahoo", ch->pcdata->yahoo, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void fread_obj( CHAR_DATA *ch, NOTE_DATA *pnote, FILE *fp, short os_type )
{
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *room = NULL;
   const char *word;
   char *infoflags, flag[MSL];
   int iNest, value;
   bool fMatch, fNest, fVnum;

   if( ch )
   {
      room = ch->in_room;
      if( ch->tempnum == -9999 )
         file_ver = 0;
   }
   CREATE( obj, OBJ_DATA, 1 );
   if( !obj )
      bug( "%s: failed to CREATE obj.", __FUNCTION__ );
   obj->count = 1;
   obj->wear_loc = -1;
   obj->t_wear_loc = -1;
   obj->weight = 1;
   obj->bsplatter = 0;
   obj->bstain = 0;

   fNest = true;  /* Requiring a Nest 0 is a waste */
   fVnum = false;
   iNest = 0;

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
            KEY( "ActionDesc", obj->action_desc, fread_string( fp ) );
            break;

         case 'B':
            KEY( "BSplatter", obj->bsplatter, fread_number( fp ) );
            KEY( "BStain", obj->bstain, fread_number( fp ) );
            break;

         case 'C':
            KEY( "Cost", obj->cost, fread_number( fp ) );
            KEY( "Count", obj->count, fread_number( fp ) );
            if( !str_cmp( word, "Classes" ) )
            {
               int iclass;

               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  for( iclass = 0; iclass < MAX_PC_CLASS; iclass++ )
                  {
                     if( !class_table[iclass] || !class_table[iclass]->name )
                        continue;
                     if( !str_cmp( class_table[iclass]->name, flag ) )
                     {
                        xSET_BIT( obj->class_restrict, iclass );
                        break;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'D':
            KEY( "Description", obj->description, fread_string( fp ) );
            KEY( "Desc", obj->desc, fread_string( fp ) );
            break;

         case 'E':
            WEXTKEY( "ExtraFlags", obj->extra_flags, fp, o_flags, ITEM_MAX );

            if( !strcmp( word, "ExtraDescr" ) )
            {
               EXTRA_DESCR_DATA *ed;

               CREATE( ed, EXTRA_DESCR_DATA, 1 );
               ed->keyword = fread_string( fp );
               ed->description = fread_string( fp );
               LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
               fMatch = true;
            }

            if( !strcmp( word, "End" ) )
            {
               if( !fNest || !fVnum )
               {
                  if( obj->name )
                     bug( "%s: %s incomplete object.", __FUNCTION__, obj->name );
                  else
                     bug( "%s: incomplete object.", __FUNCTION__ );
                  free_obj( obj, false ); /* Not linked yet, but does need fully freed */
                  return;
               }
               if( !obj->name )
                  obj->name = QUICKLINK( obj->pIndexData->name );
               if( !obj->description )
                  obj->description = QUICKLINK( obj->pIndexData->description );
               if( !obj->short_descr )
                  obj->short_descr = QUICKLINK( obj->pIndexData->short_descr );
               if( !obj->action_desc )
                  obj->action_desc = QUICKLINK( obj->pIndexData->action_desc );
               if( !obj->desc )
                  obj->desc = QUICKLINK( obj->pIndexData->desc );
               obj->pIndexData->count += obj->count;
               LINK( obj, first_object, last_object, next, prev );
               if( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC )
               {
                  LINK( obj, first_corpse, last_corpse, next_corpse, prev_corpse );
                  ++num_corpses;
               }
               LINK( obj, obj->pIndexData->first_copy, obj->pIndexData->last_copy, next_index, prev_index );
               if( fNest )
                  rgObjNest[iNest] = obj;
               numobjsloaded += obj->count;
               ++physicalobjects;
               if( obj->wear_loc < -1 || obj->wear_loc >= WEAR_MAX )
                  obj->wear_loc = -1;
               if( obj->t_wear_loc < -1 || obj->t_wear_loc >= WEAR_MAX )
                  obj->t_wear_loc = -1;
               /* Corpse saving. -- Altrag */
               if( os_type == OS_CORPSE )
               {
                  if( !room )
                  {
                     bug( "%s: Corpse without room", __FUNCTION__ );
                     room = get_room_index( sysdata.room_limbo );
                  }
                  if( obj->timer < 1 )
                     obj->timer = 40;
                  obj = obj_to_room( obj, room );
               }
               else if( os_type == OS_LOCKER )
               {
                  /* Want to make sure it uses the room the ch is in */
                  if( ch && ch->in_room )
                     room = ch->in_room;

                  if( !room )
                  {
                     bug( "%s: Locker (%s) without room", __FUNCTION__, ( obj && obj->name ) ? obj->name : "Unknown" );
                     room = get_room_index( sysdata.room_limbo );
                  }
                  if( room )
                     obj = obj_to_room( obj, room );
               }
               else if( os_type == OS_AUCTION )
               {
                  if( !pnote->obj )
                     pnote->obj = obj;
                  if( iNest == 0 || !rgObjNest[iNest] )
                     ; /* Do nothing */
                  else
                  {
                     if( rgObjNest[iNest - 1] )
                     {
                        separate_obj( rgObjNest[iNest - 1] );
                        obj = obj_to_obj( obj, rgObjNest[iNest - 1] );
                     }
                     else
                        bug( "%s: nest layer missing %d", __FUNCTION__, iNest - 1 );
                  }
               }
               else if( iNest == 0 || !rgObjNest[iNest] )
                  obj = obj_to_char( obj, ch );
               else
               {
                  if( rgObjNest[iNest - 1] )
                  {
                     separate_obj( rgObjNest[iNest - 1] );
                     obj = obj_to_obj( obj, rgObjNest[iNest - 1] );
                  }
                  else
                     bug( "%s: nest layer missing %d", __FUNCTION__, iNest - 1 );
               }
               if( fNest )
                  rgObjNest[iNest] = obj;
               return;
            }
            break;

         case 'I':
            SKEY( "ItemType", obj->item_type, fp, o_types, ITEM_TYPE_MAX );
            break;

         case 'L':
            KEY( "Level", obj->level, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", obj->name, fread_string( fp ) );
            if( !strcmp( word, "NAffect" ) || !strcmp( word, "NAffectData" ) || !strcmp( word, "NAffectN" ) || !strcmp( word, "NAffectDataN" ) )
            {
               AFFECT_DATA *paf = NULL;

               if( !strcmp( word, "NAffect" ) || !strcmp( word, "NaffectData" ) )
                  paf = fread_chaffect( fp, 0, __FILE__, __LINE__ );
               else if( !strcmp( word, "NAffectN" ) || !strcmp( word, "NAffectDataN" ) ) /* Needs a little extra */
                  paf = fread_chaffect( fp, 1, __FILE__, __LINE__ );

               if( paf )
               {
                  LINK( paf, obj->first_affect, obj->last_affect, next, prev );
                  top_affect++;
               }

               fMatch = true;
               break;
            }
            if( !strcmp( word, "Nest" ) )
            {
               iNest = fread_number( fp );
               if( iNest < 0 || iNest >= MAX_NEST )
               {
                  bug( "%s: bad nest %d.", __FUNCTION__, iNest );
                  iNest = 0;
                  fNest = false;
               }
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
                  obj->stat_reqs[ustat] = stat;
               fMatch = true;
               break;
            }
            break;

        case 'O':
            KEY( "Owner", obj->owner, fread_string( fp ) );
	    break;

         case 'R':
            KEY( "Room", room, get_room_index( fread_number( fp ) ) );
            KEY( "Rvnum", obj->room_vnum, fread_number( fp ) );
            if( !str_cmp( word, "Races" ) )
            {
               int irace;

               infoflags = fread_flagstring( fp );
               while( infoflags && infoflags[0] != '\0' )
               {
                  infoflags = one_argument( infoflags, flag );
                  for( irace = 0; irace < MAX_PC_RACE; irace++ )
                  {
                     if( !race_table[irace] || !race_table[irace]->name )
                        continue;
                     if( !str_cmp( race_table[irace]->name, flag ) )
                     {
                        xSET_BIT( obj->race_restrict, irace );
                        break;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'S':
            KEY( "ShortDescr", obj->short_descr, fread_string( fp ) );
            if( !strcmp( word, "Stats" ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               obj->stat_reqs[STAT_STR] = fread_number( fp );
               obj->stat_reqs[STAT_INT] = fread_number( fp );
               obj->stat_reqs[STAT_WIS] = fread_number( fp );
               obj->stat_reqs[STAT_DEX] = fread_number( fp );
               obj->stat_reqs[STAT_CON] = fread_number( fp );
               obj->stat_reqs[STAT_CHA] = fread_number( fp );
               obj->stat_reqs[STAT_LCK] = fread_number( fp );
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Spell" ) )
            {
               int iValue;
               int sn;

               iValue = fread_number( fp );
               infoflags = fread_word( fp );
               sn = skill_lookup( infoflags );
               if( iValue < 0 || iValue > 5 )
                  bug( "%s: bad iValue %d.", __FUNCTION__, iValue );
               else if( sn < 0 )
                  bug( "%s: unknown spell (%s) on object.", __FUNCTION__, infoflags );
               else
                  obj->value[iValue] = sn;
               fMatch = true;
               break;
            }
            break;

         case 'T':
            KEY( "Timer", obj->timer, fread_number( fp ) );
            break;

         case 'V':
            if( !strcmp( word, "Values" ) )
            {
               int x1, x2, x3, x4, x5, x6;
               char *ln = fread_line( fp );

               x1 = x2 = x3 = x4 = x5 = x6 = 0;
               sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

               obj->value[0] = x1;
               obj->value[1] = x2;
               obj->value[2] = x3;
               obj->value[3] = x4;
               obj->value[4] = x5;
               obj->value[5] = x6;
               fMatch = true;
               break;
            }

            if( !strcmp( word, "Vnum" ) )
            {
               int vnum;

               vnum = fread_number( fp );
               if( !( obj->pIndexData = get_obj_index( vnum ) ) )
                  fVnum = false;
               else
               {
                  fVnum = true;
                  obj->cost = obj->pIndexData->cost;
                  obj->weight = obj->pIndexData->weight;
                  obj->item_type = obj->pIndexData->item_type;
                  obj->wear_flags = obj->pIndexData->wear_flags;
                  obj->extra_flags = obj->pIndexData->extra_flags;
               }
               fMatch = true;
               break;
            }
            break;

         case 'W':
            WEXTKEY( "WearFlags", obj->wear_flags, fp, w_flags, ITEM_WEAR_MAX );
            /* Here we set t_wear_loc instead of wear_loc so it will equip it correctly */
            SKEY( "WearLoc", obj->t_wear_loc, fp, wear_locs, WEAR_MAX );
            KEY( "Weight", obj->weight, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match (%s).", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void set_alarm( long seconds )
{
#ifdef WIN32
   kill_timer( );   /* kill old timer */
   timer_code = timeSetEvent( seconds * 1000L, 1000, alarm_handler, 0, TIME_PERIODIC );
#else
   alarm( seconds );
#endif
}

/* Based on last time modified, show when a player was last on	-Thoric */
CMDF( do_last )
{
   DIR *dp, *edp;
   struct dirent *de, *ep;
   struct stat fst;
   char buf[MSL], arg[MIL], name[MIL];
   int max = -1, cnt = 0, pcount = 0, msize = 0, ksize = 0, psize = 0, days = 0;

   one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' || is_number( arg ) )
   {
      if( get_trust( ch ) < PERM_HEAD )
      {
         send_to_char( "Usage: last <player name>\r\n", ch );
         return;
      }
      if( arg != NULL && is_number( arg ) )
         max = atoi( arg );
      if( !( dp = opendir( PLAYER_DIR ) ) )
      {
         bug( "%s: can't open %s", __FUNCTION__, PLAYER_DIR );
         perror( PLAYER_DIR );
         return;
      }
      while( ( de = readdir( dp ) ) )
      {
         if( de->d_name[0] == '.' )
            continue;
         snprintf( buf, sizeof( buf ), "%s%s", PLAYER_DIR, de->d_name );
         if( !( edp = opendir( buf ) ) )
         {
            bug( "%s: can't open %s", __FUNCTION__, buf );
            perror( buf );
            closedir( dp );
            dp = NULL;
            return;
         }
         while( ( ep = readdir( edp ) ) )
         {
            if( ep->d_name[0] == '.' )
               continue;
            snprintf( buf, sizeof( buf ), "%s%s/%s", PLAYER_DIR, de->d_name, ep->d_name );
            if( stat( buf, &fst ) == -1 )
               continue;
            ++pcount;
            psize += ( int )fst.st_size;
            while( psize >= 1024 )
            {
               psize -= 1024;
               ksize++;
            }
            while( ksize >= 1024 )
            {
               ksize -= 1024;
               msize++;
            }
            days = ( ( current_time - fst.st_mtime ) / 86400 );
            snprintf( buf, sizeof( buf ), "%12s(%dK)(%dD)", ep->d_name, ( int )fst.st_size / 1024, days );
            ch_printf( ch, "%24s", buf );
            if( ++cnt == 3 || pcount == max )
            {
               cnt = 0;
               send_to_char( "\r\n", ch );
            }
            if( pcount == max )
            {
               closedir( edp );
               edp = NULL;
               closedir( dp );
               dp = NULL;
               return;
            }
         }
         closedir( edp );
         edp = NULL;
      }
      if( cnt != 0 && pcount != max )
         send_to_char( "\r\n", ch );
      closedir( dp );
      dp = NULL;

      ch_printf( ch, "Total pfiles counted: %d.\r\n", pcount );
      ch_printf( ch, "Total size of pfiles: %dMB %dKB %dBytes.\r\n", msize, ksize, psize );
      return;
   }
   mudstrlcpy( name, capitalize( arg ), sizeof( name ) );
   snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( arg[0] ), name );
   if( stat( buf, &fst ) != -1 && check_parse_name( capitalize( name ), false ) )
      ch_printf( ch, "%s was last on: %s\r\n", name, distime( fst.st_mtime ) );
   else
      ch_printf( ch, "%s was not found.\r\n", name );
}

CMDF( do_corpses )
{
   OBJ_DATA *corpse;
   int count = 0;

   send_to_char( "Player corpses:\r\n", ch );
   for( corpse = first_corpse; corpse; corpse = corpse->next_corpse )
   {
      count++;
      pager_printf( ch, "   %-12.12s", ( corpse->short_descr + 14 ) );
      if( corpse->in_room )
         pager_printf( ch, " &PIn Room: &w%-22.22s &P[&w%5d&P] ", corpse->in_room->area->name, corpse->in_room->vnum );
      else if( corpse->in_obj )
         pager_printf( ch, " &PIn Obj:  &w%-22.22s &P[&w%5d&P] ", corpse->in_obj->short_descr, corpse->in_obj->pIndexData->vnum );
      else if( corpse->carried_by )
         pager_printf( ch, " &PCarried: &w%-30.30s ", corpse->carried_by->name );
      else
         pager_printf( ch, " &P%-39.39s ", "Unknown Location!!!" );
      send_to_pager( "\r\n", ch );
   }
   if( !count )
      send_to_char( "   (None)\r\n", ch );
}

/* Added support for removeing so we could take the write_corpses out of the save_char_obj function. --Shaddai */
void write_corpses( OBJ_DATA *obj, bool dontsave )
{
   OBJ_DATA *corpse;
   FILE *fp = NULL;
   char *name, buf[MIL];

   name = ( obj->short_descr + 14 );
   snprintf( buf, sizeof( buf ), "%s%s", CORPSE_DIR, capitalize( name ) );

   for( corpse = first_corpse; corpse; corpse = corpse->next_corpse )
   {
      if( corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_PC
      && corpse->in_room
      && !str_cmp( corpse->short_descr + 14, name )
      && ( !dontsave || obj != corpse )
      && corpse->first_content )
      {
         if( !fp )
         {
            if( !( fp = fopen( buf, "w" ) ) )
            {
               bug( "%s: Can't open file %s.", __FUNCTION__, buf );
               perror( buf );
               return;
            }
         }
         fwrite_obj( NULL, corpse, fp, 0, OS_CORPSE, false );
      }
   }
   if( fp )
   {
      fprintf( fp, "#END\n\n" );
      fclose( fp );
      fp = NULL;
   }
   else
      remove( buf );
}

void load_corpses( void )
{
   DIR *dp;
   FILE *fp;
   struct dirent *de;
   char corpsefile[MIL];

   if( !( dp = opendir( CORPSE_DIR ) ) )
   {
      bug( "%s: can't open %s", __FUNCTION__, CORPSE_DIR );
      perror( CORPSE_DIR );
      return;
   }

   falling = 1;   /* Arbitrary, must be > 0 though. */
   while( ( de = readdir( dp ) ) )
   {
      if( de->d_name[0] == '.' )
         continue;

      snprintf( corpsefile, sizeof( corpsefile ), "%s%s", CORPSE_DIR, de->d_name );
      fprintf( stderr, "Corpse -> %s\n", corpsefile );
      if( !( fp = fopen( corpsefile, "r" ) ) )
      {
         perror( corpsefile );
         continue;
      }
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == EOF )
            break;
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
         if( !strcmp( word, "CORPSE" ) )
            fread_obj( NULL, NULL, fp, OS_CORPSE );
         else if( !strcmp( word, "OBJECT" ) )
            fread_obj( NULL, NULL, fp, OS_CARRY );
         else if( !strcmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section.", __FUNCTION__ );
            break;
         }
      }
      fclose( fp );
      fp = NULL;
   }
   closedir( dp );
   falling = 0;
}

/* This will write one mobile structure pointed to --Shaddai */
void fwrite_mobile( FILE *fp, CHAR_DATA *mob )
{
   int stat;

   if( !is_npc( mob ) || !fp )
      return;

   de_equipchar( mob );

   fprintf( fp, "#MOBILE\n" );
   fprintf( fp, "Vnum        %d\n", mob->pIndexData->vnum );
   if( mob->in_room )
      fprintf( fp, "Room        %d\n",
         ( mob->in_room == get_room_index( sysdata.room_limbo )
         && mob->was_in_room ) ? mob->was_in_room->vnum : mob->in_room->vnum );
   if( mob->name && ( !mob->pIndexData->name || str_cmp( mob->name, mob->pIndexData->name ) ) )
      fprintf( fp, "Name        %s~\n", mob->name );
   if( mob->short_descr && ( !mob->pIndexData->short_descr || str_cmp( mob->short_descr, mob->pIndexData->short_descr ) ) )
      fprintf( fp, "Short       %s~\n", strip_cr( mob->short_descr ) );
   if( mob->long_descr && ( !mob->pIndexData->long_descr || str_cmp( mob->long_descr, mob->pIndexData->long_descr ) ) )
      fprintf( fp, "Long        %s~\n", strip_cr( mob->long_descr ) );
   if( mob->description && ( !mob->pIndexData->description || str_cmp( mob->description, mob->pIndexData->description ) ) )
      fprintf( fp, "Description %s~\n", strip_cr( mob->description ) );
   fprintf( fp, "Position    %d\n", mob->position );
   if( mob->level != mob->pIndexData->level )
      fprintf( fp, "Level       %d\n", mob->level );
   fprintf( fp, "HpManaMove  %d %d %d %d %d %d\n", mob->hit, mob->max_hit, mob->mana, mob->max_mana, mob->move, mob->max_move );
   if( !xIS_EMPTY( mob->act ) && !xSAME_BITS( mob->act, mob->pIndexData->act ) )
      fprintf( fp, "Flags       %s~\n", ext_flag_string( &mob->act, act_flags ) );
   if( !xIS_EMPTY( mob->affected_by ) )
      fprintf( fp, "AffectedBy  %s~\n", ext_flag_string( &mob->affected_by, a_flags ) );
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( mob->perm_stats[stat] == mob->pIndexData->perm_stats[stat] && mob->mod_stats[stat] == 0 )
         continue;
      /* If there stats are different should save it...yes? */
      fprintf( fp, "NStat       %d %d %s~\n", mob->perm_stats[stat], mob->mod_stats[stat], stattypes[stat] );
   }
   fwrite_chaffect( fp, mob->first_affect );

   if( mob->first_carrying )
      fwrite_obj( mob, mob->last_carrying, fp, 0, OS_CARRY, false );

   fprintf( fp, "EndMobile\n" );

   re_equipchar( mob );
}

/* This will read one mobile structure pointer to by fp --Shaddai */
CHAR_DATA *fread_mobile( FILE *fp )
{
   MOB_INDEX_DATA *mindex = NULL;
   CHAR_DATA *mob = NULL;
   AFFECT_DATA *paf;
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   const char *word;
   char *infoflags, flag[MSL];
   int value, inroom = 0;
   bool fMatch;

   word = feof( fp ) ? "EndMobile" : fread_word( fp );
   if( !strcmp( word, "Vnum" ) )
   {
      int vnum;

      vnum = fread_number( fp );
      if( !( mindex = get_mob_index( vnum ) ) || !( mob = create_mobile( mindex ) ) )
      {
         for( ;; )
         {
            word = feof( fp ) ? "EndMobile" : fread_word( fp );
            if( !strcmp( word, "EndMobile" ) )
               break;
         }
         if( !mindex )
            bug( "%s: No index data for vnum %d", __FUNCTION__, vnum );
         else
            bug( "%s: couldn't create_mobile for vnum %d", __FUNCTION__, vnum );
         return NULL;
      }
   }
   else
   {
      bug( "%s: (%s) found instead of Vnum", __FUNCTION__, word );
      for( ;; )
      {
         word = feof( fp ) ? "EndMobile" : fread_word( fp );
         if( !strcmp( word, "EndMobile" ) )
            break;
      }
      return NULL;
   }
   for( ;; )
   {
      word = feof( fp ) ? "EndMobile" : fread_word( fp );
      fMatch = false;
      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case '#':
            if( !strcmp( word, "#OBJECT" ) )
            {
               fread_obj( mob, NULL, fp, OS_CARRY );
               fMatch = true;
               break;
            }
            break;

         case 'A':
            WEXTKEY( "AffectedBy", mob->affected_by, fp, a_flags, AFF_MAX );
            if( !strcmp( word, "Affect" ) || !strcmp( word, "AffectData" ) )
            {
               fMatch = true;
               if( ( paf = fread_chaffect( fp, 0, __FILE__, __LINE__ ) ) )
                  LINK( paf, mob->first_affect, mob->last_affect, next, prev );
               break;
            }
            break;

         case 'D':
            if( !strcmp( word, "Description" ) )
            {
               STRFREE( mob->description );
               mob->description = fread_string( fp );
               fMatch = true;
               break;
            }
            break;

         case 'E':
            if( !strcmp( word, "EndMobile" ) )
            {
               if( inroom == 0 )
                  inroom = sysdata.room_temple;
               if( !( pRoomIndex = get_room_index( inroom ) ) )
                  pRoomIndex = get_room_index( sysdata.room_temple );
               char_to_room( mob, pRoomIndex );
               re_equipchar( mob );
               return mob;
            }
            break;

         case 'F':
            WEXTKEY( "Flags", mob->act, fp, act_flags, ACT_MAX );
            break;

         case 'H':
            if( !strcmp( word, "HpManaMove" ) )
            {
               mob->hit = fread_number( fp );
               mob->max_hit = fread_number( fp );
               mob->mana = fread_number( fp );
               mob->max_mana = fread_number( fp );
               mob->move = fread_number( fp );
               mob->max_move = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'L':
            KEY( "Level", mob->level, fread_number( fp ) );
            if( !strcmp( word, "Long" ) )
            {
               STRFREE( mob->long_descr );
               mob->long_descr = fread_string( fp );
               fMatch = true;
               break;
            }
            break;

         case 'N':
            if( !str_cmp( word, "NStat" ) )
            {
               int ustat, umstat, stat;

               stat = fread_number( fp );
               umstat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
               {
                  mob->perm_stats[ustat] = stat;
                  mob->mod_stats[ustat] = umstat;
               }
               fMatch = true;
               break;
            }
            if( !strcmp( word, "Name" ) )
            {
               STRFREE( mob->name );
               mob->name = fread_string( fp );
               fMatch = true;
               break;
            }
            break;

         case 'P':
            KEY( "Position", mob->position, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Room", inroom, fread_number( fp ) );
            break;

         case 'S':
            if( !strcmp( word, "Short" ) )
            {
               STRFREE( mob->short_descr );
               mob->short_descr = fread_string( fp );
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

void remove_oldest_pfile_backup( void )
{
   DIR *dp;
   char buf[MSL];
   static char oldestname[MSL];
   struct dirent *de;
   struct stat fst;
   time_t oldesttime = current_time;

   if ( !( dp = opendir( BACKUP_DIR ) ) )
   {
      bug( "%s: can't open %s", __FUNCTION__, BACKUP_DIR );
      perror( BACKUP_DIR );
      return;
   }

   oldestname[0] = '\0';

   /* Ok have the directory open so lets check the files and the time and remove the oldest one */
   while( ( de = readdir ( dp ) ) )
   {
      if ( de->d_name[0] == '.' )
         continue;

      if( !str_suffix( de->d_name, "pfiles.tgz" ) ) /* We only want to count and mess with pfiles.tgz */
         continue;

      snprintf ( buf, sizeof ( buf ), "%s%s", BACKUP_DIR, de->d_name );
      if( stat( buf, &fst ) == -1 )
         continue;

      if( oldesttime > fst.st_mtime ) /* The oldest has the lowest time */
      {
         oldesttime = fst.st_mtime;
         snprintf( oldestname, sizeof( oldestname ), "%s%s", BACKUP_DIR, de->d_name );
      }
   }
   closedir ( dp );
   dp = NULL;

   if( oldestname != NULL && oldestname[0] != '\0' && !remove( oldestname ) )
      log_printf( "%s: %s has been deleted to keep the pfile backups down.", __FUNCTION__, oldestname );
}

void save_pfile_backup( void )
{
   char buf[MSL];
   int logindex, tcount = 0, echeck;

   for( logindex = 1; ; logindex++ )
   {
      snprintf( buf, sizeof( buf ), "%s%dpfiles.tgz", BACKUP_DIR, logindex );
      if( exists_file( buf ) )
         continue;
      if( logindex >= 11 )
      {
         remove_oldest_pfile_backup( );
         if( ++tcount > 5 ) /* Don't allow a constant loop if for some reason it can't remove one to use */
         {
            bug( "%s: failed to save pfile backup.", __FUNCTION__ );
            return;
         }
         logindex = 0;
         continue;
      }
      break;
   }

   /* Ok we have an empty log to use so use it */
   snprintf( buf, sizeof( buf ), "tar -czf %s%dpfiles.tgz %s", BACKUP_DIR, logindex, PLAYER_DIR );
   echeck = system( buf );
}

/* Used to cleanup the pfiles */
void pfile_cleanup( void )
{
   DIR *dp, *edp;
   FILE *fp;
   EXT_BV flags;
   struct dirent *de, *ep;
   struct stat fst;
   char buf[MIL], *infoflags, flag[MIL];
   int level = 0, trust = 0, days = 0, checked = 0, deleted = 0, value;
   bool endfound, fMatch;

   if( sysdata.next_pfile_cleanup > current_time )
      return;

   log_string( "Saving pfile backup" );
   save_pfile_backup( );

   log_string( "Starting pfile cleanup" );
   sysdata.next_pfile_cleanup = ( current_time + 86400 );
   save_sysdata( false );

   if( !( dp = opendir( PLAYER_DIR ) ) )
   {
      bug( "%s: can't open %s", __FUNCTION__, PLAYER_DIR );
      perror( PLAYER_DIR );
      return;
   }
   while( ( de = readdir( dp ) ) )
   {
      if( de->d_name[0] == '.' )
         continue;
      snprintf( buf, sizeof( buf ), "%s%s", PLAYER_DIR, de->d_name );
      if( !( edp = opendir( buf ) ) )
      {
         bug( "%s: can't open %s", __FUNCTION__, buf );
         perror( buf );
         closedir( dp );
         dp = NULL;
         return;
      }
      while( ( ep = readdir( edp ) ) )
      {
         if( ep->d_name[0] == '.' )
            continue;
         snprintf( buf, sizeof( buf ), "%s%s/%s", PLAYER_DIR, de->d_name, ep->d_name );
         if( stat( buf, &fst ) == -1 )
            continue;
         if( !( fp = fopen( buf, "r" ) ) )
         {
            bug( "%s: can't open %s for reading", __FUNCTION__, buf );
            perror( buf );
            continue;
         }
         level = 0;
         trust = 0;
         xCLEAR_BITS( flags );
         ++checked;
         for( ;; )
         {
            char letter;
            const char *word;

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

                     case 'N':
                        SKEY( "NTrust", trust, fp, perms_flag, PERM_MAX );
                        break;

                     case 'S':
                        if( !str_cmp( word, "Sex" ) )
                        {
                           endfound = true;
                           fMatch = true;
                           break;
                        }
                        break;

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

         /* Don't delete any of the immortals for being absent to long */
         if( trust > PERM_ALL || xIS_SET( flags, PCFLAG_NOPDELETE ) )
            continue;
         days = ( ( current_time - fst.st_mtime ) / 86400 );
         /* You have as many days as your level */
         if( days <= level )
            continue;
         if( !remove( buf ) )
         {
            log_printf( "%s: Deleted %s [Level %d] for not logging on for %d day%s.", __FUNCTION__, ep->d_name, level, days, days != 1 ? "s" : "" );

            check_lockers( );
            remove_from_everything( ep->d_name ); /* Oops looks like we were leaving them around in places */
            if( !is_locker_shared( ep->d_name ) )
            {
               snprintf( buf, sizeof( buf ), "%s%s", LOCKER_DIR, ep->d_name );
               if( !remove( buf ) )
                  log_string( "Player's locker data destroyed.\r\n" );
            }
            ++deleted;
         }
         else
            log_printf( "%s: Couldn't delete %s [Level %d] for not logging on for %d day%s.", __FUNCTION__, ep->d_name, level, days, days != 1 ? "s" : "" );
      }
      closedir( edp );
      edp = NULL;
   }
   closedir( dp );
   dp = NULL;
   log_string( "Finished pfile cleanup." );
   if( checked )
      log_printf( "Checked %d player file%s.", checked, checked != 1 ? "s" : "" );
   if( deleted )
      log_printf( "Deleted %d player file%s.", deleted, deleted != 1 ? "s" : "" );
}
