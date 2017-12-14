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
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "h/mud.h"

CMDF( do_aassign )
{
   AREA_DATA *tarea, *tmp;
   char buf[MSL];

   if( !ch || is_npc( ch ) )
      return;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: aassign <filename>\r\n", ch );
      return;
   }

   if( !str_cmp( "none", argument ) || !str_cmp( "clear", argument ) )
   {
      ch->pcdata->area = NULL;
      assign_area( ch );
      if( !ch->pcdata->area )
         send_to_char( "Area pointer cleared.\r\n", ch );
      else
         send_to_char( "Originally assigned area restored.\r\n", ch );
      return;
   }

   mudstrlcpy( buf, argument, sizeof( buf ) );
   tarea = NULL;

   if( get_trust( ch ) >= PERM_HEAD
   || ( is_name( buf, ch->pcdata->bestowments ) && get_trust( ch ) >= sysdata.perm_modify_proto ) )
   {
      for( tmp = first_area; tmp; tmp = tmp->next )
      {
         if( !str_cmp( buf, tmp->filename ) )
         {
            tarea = tmp;
            break;
         }
      }
   }
   if( !tarea )
   {
      for( tmp = first_build; tmp; tmp = tmp->next )
      {
         if( !str_cmp( buf, tmp->filename ) )
         {
            if( get_trust( ch ) >= PERM_HEAD
            || is_name( tmp->filename, ch->pcdata->bestowments )
            || ( ch->pcdata->council && is_name( "aassign", ch->pcdata->council->powers ) ) )
            {
               tarea = tmp;
               break;
            }
            else
            {
               send_to_char( "You don't have permission to use that area.\r\n", ch );
               return;
            }
         }
      }
   }
   if( !tarea )
   {
      send_to_char( "No such area. Check 'zones' and 'vnums'.\r\n", ch );
      return;
   }
   ch->pcdata->area = tarea;
   ch_printf( ch, "Assigning you: %s\r\n", tarea->name );
}

int save_programs( MPROG_DATA *mprog, FILE *fp )
{
   int count = 0;

   if( ( mprog->arglist && mprog->arglist[0] != '\0' ) )
   {
      if( mprog->type == IN_FILE_PROG )
      {
         fprintf( fp, "> %s %s~\n", mprog_type_to_name( mprog->type ), mprog->arglist );
         count++;
      }
      /* Don't let it save progs which came from files. That would be silly. */
      else if( mprog->comlist && mprog->comlist[0] != '\0' && !mprog->fileprog )
      {
         fprintf( fp, "> %s %s~\n%s~\n", mprog_type_to_name( mprog->type ),
            mprog->arglist, strip_cr( mprog->comlist ) );
         count++;
      }
   }
   return count;
}

bool fold_area( AREA_DATA *tarea, char *filename, bool install )
{
   RESET_DATA *pReset, *tReset, *gReset;
   ROOM_INDEX_DATA *room;
   MOB_INDEX_DATA *pMobIndex;
   OBJ_INDEX_DATA *pObjIndex;
   MPROG_DATA *mprog;
   EXIT_DATA *xit;
   EXTRA_DESCR_DATA *ed;
   NEIGHBOR_DATA *neigh;
   FILE *fp;
   char newfilename[MIL], tempfilename[MIL];
   int val0, val1, val2, val3, val4, val5, vnum, stat;
   bool first;

   if( xIS_SET( tarea->flags, AFLAG_PROTOTYPE ) )
   {
      snprintf( newfilename, sizeof( newfilename ), "%s%s", BUILD_DIR, filename );
      snprintf( tempfilename, sizeof( tempfilename ), "%s%s.temp", BUILD_DIR, filename );
   }
   else
   {
      snprintf( newfilename, sizeof( newfilename ), "%s%s", AREA_DIR, filename );
      snprintf( tempfilename, sizeof( tempfilename ), "%s%s.temp", AREA_DIR, filename );
   }

   log_printf_plus( LOG_NORMAL, PERM_LEADER, "Saving %s...", newfilename );

   if( install )
   {
      xREMOVE_BIT( tarea->flags, AFLAG_PROTOTYPE );
      snprintf( newfilename, sizeof( newfilename ), "%s%s", AREA_DIR, filename );
   }
   if( !( fp = fopen( tempfilename, "w" ) ) )
   {
      bug( "%s: cant open %s for writing.", __FUNCTION__, newfilename );
      perror( newfilename );
      return false;
   }

   /* Save area info */
   if( tarea->name )
      fprintf( fp, "#AREA       %s~\n", tarea->name );
   fprintf( fp, "#AVNUM      %d\n", tarea->vnum );
   if( tarea->author )
      fprintf( fp, "#AUTHOR     %s~\n", tarea->author );
   fprintf( fp, "#RANGES     %d %d %d %d\n", tarea->low_soft_range,
      tarea->hi_soft_range, tarea->low_hard_range, tarea->hi_hard_range );
   if( tarea->resetmsg )
      fprintf( fp, "#RESETMSG   %s~\n", tarea->resetmsg );
   if( tarea->reset_frequency )
      fprintf( fp, "#RESETFREQ  %d\n", tarea->reset_frequency );
   if( !xIS_EMPTY( tarea->flags ) )
      fprintf( fp, "#FLAGS      %s~\n", ext_flag_string( &tarea->flags, area_flags ) );
   fprintf( fp, "#CLIMATE    %d %d %d\n", tarea->weather->climate_temp,
      tarea->weather->climate_precip, tarea->weather->climate_wind );
   for( neigh = tarea->weather->first_neighbor; neigh; neigh = neigh->next )
      fprintf( fp, "#NEIGHBOR   %s~\n", neigh->name );

   /* save mobiles */
   first = true;
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( pMobIndex = get_mob_index( vnum ) ) )
         continue;
      if( first )
         fprintf( fp, "\n#MOBILES\n" );
      first = false;
      if( install )
         xREMOVE_BIT( pMobIndex->act, ACT_PROTOTYPE );
      fprintf( fp, "Vnum        %d\n", vnum );
      if( pMobIndex->level )
         fprintf( fp, "Level       %d\n", pMobIndex->level );
      if( pMobIndex->name )
         fprintf( fp, "Name        %s~\n", strip_cr( pMobIndex->name ) );
      if( pMobIndex->short_descr )
         fprintf( fp, "Short       %s~\n", strip_cr( pMobIndex->short_descr ) );
      if( pMobIndex->long_descr )
         fprintf( fp, "Long        %s~\n", strip_cr( pMobIndex->long_descr ) );
      if( pMobIndex->description )
         fprintf( fp, "Description\n %s~\n", strip_cr( pMobIndex->description ) );
      if( !xIS_EMPTY( pMobIndex->act ) )
         fprintf( fp, "Flags       %s~\n", ext_flag_string( &pMobIndex->act, act_flags ) );
      if( !xIS_EMPTY( pMobIndex->affected_by ) )
         fprintf( fp, "Affected    %s~\n", ext_flag_string( &pMobIndex->affected_by, a_flags ) );
      if( !xIS_EMPTY( pMobIndex->xflags ) )
         fprintf( fp, "Parts       %s~\n", ext_flag_string( &pMobIndex->xflags, part_flags ) );
      for( stat = 0; stat < RIS_MAX; stat++ )
      {
         if( pMobIndex->resistant[stat] == 0 )
            continue;
         fprintf( fp, "NResistant  %d %s~\n", pMobIndex->resistant[stat], ris_flags[stat] );
      }
      if( !xIS_EMPTY( pMobIndex->attacks ) )
         fprintf( fp, "Attacks     %s~\n", ext_flag_string( &pMobIndex->attacks, attack_flags ) );
      if( !xIS_EMPTY( pMobIndex->defenses ) )
         fprintf( fp, "Defenses    %s~\n", ext_flag_string( &pMobIndex->defenses, defense_flags ) );
      if( pMobIndex->defposition )
         fprintf( fp, "DefPosition %s~\n", pos_names[pMobIndex->defposition] );
      if( pMobIndex->sex )
         fprintf( fp, "Sex         %s~\n", sex_names[pMobIndex->sex] );
      if( pMobIndex->alignment )
         fprintf( fp, "Alignment   %d\n", pMobIndex->alignment );
      if( pMobIndex->minhit || pMobIndex->maxhit )
         fprintf( fp, "Hit         %d %d\n", pMobIndex->minhit, pMobIndex->maxhit );
      if( pMobIndex->saving_poison_death || pMobIndex->saving_wand || pMobIndex->saving_para_petri
      || pMobIndex->saving_breath || pMobIndex->saving_spell_staff )
         fprintf( fp, "Saves       %d %d %d %d %d\n", 
            pMobIndex->saving_poison_death, pMobIndex->saving_wand,
            pMobIndex->saving_para_petri, pMobIndex->saving_breath, pMobIndex->saving_spell_staff );
      if( !xIS_EMPTY( pMobIndex->speaks ) )
         fprintf( fp, "Speaks      %s~\n", ext_flag_string( &pMobIndex->speaks, lang_names ) );
      if( !xIS_EMPTY( pMobIndex->speaking ) )
         fprintf( fp, "Speaking    %s~\n", ext_flag_string( &pMobIndex->speaking, lang_names ) );

      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( pMobIndex->perm_stats[stat] == 13 ) /* Lets not bother saving default ones */
            continue;
         fprintf( fp, "NStat       %d %s~\n", pMobIndex->perm_stats[stat], stattypes[stat] );
      }

      if( pMobIndex->ac )
         fprintf( fp, "Armor       %d\n", pMobIndex->ac );
      if( pMobIndex->gold )
         fprintf( fp, "Gold        %d\n", pMobIndex->gold );
      if( pMobIndex->height )
         fprintf( fp, "Height      %d\n", pMobIndex->height );
      if( pMobIndex->weight )
         fprintf( fp, "Weight      %d\n", pMobIndex->weight );
      if( pMobIndex->numattacks )
         fprintf( fp, "NumAttacks  %d\n", pMobIndex->numattacks );
      if( pMobIndex->hitroll )
         fprintf( fp, "HitRoll     %d\n", pMobIndex->hitroll );
      if( pMobIndex->damroll )
         fprintf( fp, "DamRoll     %d\n", pMobIndex->damroll );

      if( pMobIndex->spec_fun && pMobIndex->spec_funname )
         fprintf( fp, "Special     %s\n", pMobIndex->spec_funname );
      if( pMobIndex->pShop )
      {
         int stype;

         fprintf( fp, "NShop       %d %d %d %d ",
            pMobIndex->pShop->profit_buy,
            pMobIndex->pShop->profit_sell, pMobIndex->pShop->open_hour, pMobIndex->pShop->close_hour );
         for( stype = 0; stype < ITEM_TYPE_MAX; stype++ )
            if( pMobIndex->pShop->buy_type[stype] )
               fprintf( fp, " %s", o_types[stype] );
         fprintf( fp, "%s", "~\n" );
      }
      if( pMobIndex->rShop )
      {
         int stype;

         fprintf( fp, "NRepair     %d %d %d ",
            pMobIndex->rShop->profit_fix, pMobIndex->rShop->open_hour,
            pMobIndex->rShop->close_hour );
         for( stype = 0; stype < ITEM_TYPE_MAX; stype++ )
            if( pMobIndex->rShop->fix_type[stype] )
               fprintf( fp, " %s", o_types[stype] );
         fprintf( fp, "%s", "~\n" );
      }

      if( pMobIndex->mudprogs )
      {
         int count = 0;
         for( mprog = pMobIndex->mudprogs; mprog; mprog = mprog->next )
            count = save_programs( mprog, fp );
         if( count > 0 )
            fprintf( fp, "%s", "|\n" );
      }
      fprintf( fp, "End\n\n" );
   }
   if( !first )
      fprintf( fp, "#0\n\n\n" );

   /* save objects */
   first = true;
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( pObjIndex = get_obj_index( vnum ) ) )
         continue;
      if( first )
         fprintf( fp, "#OBJECTS\n" );
      first = false;
      if( install )
         xREMOVE_BIT( pObjIndex->extra_flags, ITEM_PROTOTYPE );
      fprintf( fp, "Vnum     %d\n", vnum );

      if( pObjIndex->level )
         fprintf( fp, "Level    %d\n", pObjIndex->level );

      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( pObjIndex->stat_reqs[stat] == 0 )
            continue;
         fprintf( fp, "NStat    %d %s~\n", pObjIndex->stat_reqs[stat], stattypes[stat] );
      }

      if( pObjIndex->name )
         fprintf( fp, "Name     %s~\n", strip_cr( pObjIndex->name ) );
      if( pObjIndex->short_descr )
         fprintf( fp, "Short    %s~\n", strip_cr( pObjIndex->short_descr ) );
      if( pObjIndex->description )
         fprintf( fp, "Description\n%s~\n", strip_cr( pObjIndex->description ) );
      if( pObjIndex->desc )
         fprintf( fp, "Desc\n%s~\n", strip_cr( pObjIndex->desc ) );
      if( pObjIndex->action_desc )
         fprintf( fp, "Action   %s~\n", strip_cr( pObjIndex->action_desc ) );
      if( pObjIndex->item_type != ITEM_TRASH )
         fprintf( fp, "Type     %s~\n", o_types[pObjIndex->item_type] );
      if( !xIS_EMPTY( pObjIndex->extra_flags ) )
         fprintf( fp, "Flags    %s~\n", ext_flag_string( &pObjIndex->extra_flags, o_flags ) );
      if( !xIS_EMPTY( pObjIndex->wear_flags ) )
         fprintf( fp, "Wear     %s~\n", ext_flag_string( &pObjIndex->wear_flags, w_flags ) );
      if( pObjIndex->layers )
         fprintf( fp, "Layers   %d\n", pObjIndex->layers );
      if( !xIS_EMPTY( pObjIndex->class_restrict ) )
         fprintf( fp, "Classes  %s~\n", ext_class_string( &pObjIndex->class_restrict ) );
      if( !xIS_EMPTY( pObjIndex->race_restrict ) )
         fprintf( fp, "Races    %s~\n", ext_race_string( &pObjIndex->race_restrict ) );

      val0 = pObjIndex->value[0];
      val1 = pObjIndex->value[1];
      val2 = pObjIndex->value[2];
      val3 = pObjIndex->value[3];
      val4 = pObjIndex->value[4];
      val5 = pObjIndex->value[5];
      switch( pObjIndex->item_type )
      {
         case ITEM_PILL:
         case ITEM_POTION:
         case ITEM_SCROLL:
            if( !is_valid_sn( val1 ) )
               val1 = HAS_SPELL_INDEX;
            if( !is_valid_sn( val2 ) )
               val2 = HAS_SPELL_INDEX;
            if( !is_valid_sn( val3 ) )
               val3 = HAS_SPELL_INDEX;
            break;

         case ITEM_STAFF:
         case ITEM_WAND:
            if( !is_valid_sn( val3 ) )
               val3 = HAS_SPELL_INDEX;
            break;

         case ITEM_SALVE:
            if( !is_valid_sn( val3 ) )
               val3 = HAS_SPELL_INDEX;
            if( !is_valid_sn( val4 ) )
               val4 = HAS_SPELL_INDEX;
            if( !is_valid_sn( val5 ) )
               val5 = HAS_SPELL_INDEX;
            break;
      }

      if( val0 != -1 )
         fprintf( fp, "Val0     %d\n", val0 );

      if( val1 != -1 )
      {
         fprintf( fp, "Val1     %d ", val1 );
         if( pObjIndex->item_type == ITEM_PILL || pObjIndex->item_type == ITEM_POTION
         || pObjIndex->item_type == ITEM_SCROLL )
            fprintf( fp, "'%s'", is_valid_sn( val1 ) ? skill_table[val1]->name : "NONE" );
         fprintf( fp, "%s", "\n" );
      }

      if( val2 != -1 )
      {
         fprintf( fp, "Val2     %d ", val2 );
         if( pObjIndex->item_type == ITEM_PILL || pObjIndex->item_type == ITEM_POTION
         || pObjIndex->item_type == ITEM_SCROLL )
            fprintf( fp, "'%s'", is_valid_sn( val2 ) ? skill_table[val2]->name : "NONE" );
         fprintf( fp, "%s", "\n" );
      }

      if( val3 != -1 )
      {
         fprintf( fp, "Val3     %d ", val3 );
         switch( pObjIndex->item_type )
         {
            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
            case ITEM_STAFF:
            case ITEM_WAND:
            case ITEM_SALVE:
               fprintf( fp, "'%s'", is_valid_sn( val3 ) ? skill_table[val3]->name : "NONE" );
               break;
         }
         fprintf( fp, "%s", "\n" );
      }

      if( val4 != -1 )
      {
         fprintf( fp, "Val4     %d ", val4 );
         if( pObjIndex->item_type == ITEM_SALVE )
            fprintf( fp, "'%s'", is_valid_sn( val4 ) ? skill_table[val4]->name : "NONE" );
         fprintf( fp, "%s", "\n" );
      }

      if( val5 != -1 )
      {
         fprintf( fp, "Val5     %d ", val5 );
         if( pObjIndex->item_type == ITEM_SALVE )
            fprintf( fp, "'%s'", is_valid_sn( val5 ) ? skill_table[val5]->name : "NONE" );
         fprintf( fp, "%s", "\n" );
      }

      if( pObjIndex->weight != 1 )
         fprintf( fp, "Weight   %d\n", pObjIndex->weight );
 
      if( pObjIndex->cost )
         fprintf( fp, "Cost     %d\n", pObjIndex->cost );

      for( ed = pObjIndex->first_extradesc; ed; ed = ed->next )
         fprintf( fp, "E\n%s~\n%s~\n", ed->keyword, strip_cr( ed->description ) );

      fwrite_oiaffect( fp, pObjIndex->first_affect );

      if( pObjIndex->mudprogs )
      {
         int count = 0;
         for( mprog = pObjIndex->mudprogs; mprog; mprog = mprog->next )
            count = save_programs( mprog, fp );
         if( count > 0 )
            fprintf( fp, "%s", "|\n" );
      }
      fprintf( fp, "%s", "End\n\n" );
   }
   if( !first )
      fprintf( fp, "%s", "#0\n\n\n" );

   /* save rooms */
   first = true;
   for( vnum = tarea->low_vnum; vnum <= tarea->hi_vnum; vnum++ )
   {
      if( !( room = get_room_index( vnum ) ) )
         continue;
      if( first )
         fprintf( fp, "#ROOMS\n" );
      first = false;
      if( install )
      {
         CHAR_DATA *victim, *vnext;
         OBJ_DATA *obj, *obj_next;

         /* purge room of (prototyped) mobiles */
         for( victim = room->first_person; victim; victim = vnext )
         {
            vnext = victim->next_in_room;
            if( is_npc( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) )
               extract_char( victim, true );
         }
         /* purge room of (prototyped) objects */
         for( obj = room->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            if( xIS_SET( obj->extra_flags, ITEM_PROTOTYPE ) )
               extract_obj( obj );
         }
      }
      fprintf( fp, "Vnum         %d\n", vnum );
      if( room->name )
         fprintf( fp, "Name         %s~\n", strip_cr( room->name ) );
      if( room->description )
         fprintf( fp, "Description  %s~\n", strip_cr( room->description ) );
      if( !xIS_EMPTY( room->room_flags ) )
         fprintf( fp, "Flags        %s~\n", ext_flag_string( &room->room_flags, r_flags ) );
      if( room->sector_type != SECT_INSIDE )
         fprintf( fp, "Sector       %s~\n", sect_flags[room->sector_type] );
      if( room->tele_delay )
         fprintf( fp, "Teledelay    %d\n", room->tele_delay );
      if( room->tele_vnum )
         fprintf( fp, "Televnum     %d\n", room->tele_vnum );
      if( room->tunnel )
         fprintf( fp, "Tunnel       %d\n", room->tunnel );
      for( xit = room->first_exit; xit; xit = xit->next )
      {
         /* Don't save any part of temporary exits */
         if( xIS_SET( xit->exit_info, EX_PORTAL ) ) /* don't save portals */
            continue;
         if( xIS_SET( xit->exit_info, EX_PASSAGE ) ) /* don't save passage exits, mobiles create them */
            continue;

         fprintf( fp, "Exit         %s\n", dir_name[xit->vdir] );
         if( xit->description && xit->description[0] != '\0' )
            fprintf( fp, "Description  %s~\n", strip_cr( xit->description ) );
         if( xit->keyword && xit->keyword[0] != '\0' )
            fprintf( fp, "Keyword      %s~\n", strip_cr( xit->keyword ) );

         if( !xIS_EMPTY( xit->base_info ) )
            fprintf( fp, "Flags        %s~\n", ext_flag_string( &xit->base_info, ex_flags ) );

         if( xit->key >= 0 )
            fprintf( fp, "Key          %d\n", xit->key );
         if( xit->vnum )
            fprintf( fp, "To           %d\n", xit->vnum );
         if( xit->pulltype )
            fprintf( fp, "Pulltype     %s~\n", pull_type_name( xit->pulltype ) );
         if( xit->pull )
            fprintf( fp, "Pull         %d\n", xit->pull );
         fprintf( fp, "%s", "End\n" );
      }

      for( pReset = room->first_reset; pReset; pReset = pReset->next )
      {
         switch( pReset->command ) /* extra arg1 arg2 arg3 */
         {
            default:
            case '*':
               break;
            case 'm':
            case 'M':
            case 'o':
            case 'O':
               if( pReset->wilderness )
                  fprintf( fp, "MR %c %d %d %d %d %d %d %d\n", UPPER( pReset->command ),
                     pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3, pReset->rchance, pReset->cords[0], pReset->cords[1] );
               else
                  fprintf( fp, "NR %c %d %d %d %d %d\n", UPPER( pReset->command ),
                     pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3, pReset->rchance );

               for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
               {
                  switch( tReset->command )
                  {
                     case 'p':
                     case 'P':
                     case 'e':
                     case 'E':
                        fprintf( fp, "  NR %c %d %d %d %d %d\n", UPPER( tReset->command ),
                           tReset->extra, tReset->arg1, tReset->arg2, tReset->arg3, tReset->rchance );
                        if( tReset->first_reset )
                        {
                           for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
                           {
                              if( gReset->command != 'p' && gReset->command != 'P' )
                                 continue;
                              fprintf( fp, "    NR %c %d %d %d %d %d\n", UPPER( gReset->command ),
                                 gReset->extra, gReset->arg1, gReset->arg2, gReset->arg3, gReset->rchance );
                           }
                        }
                        break;

                     case 'g':
                     case 'G':
                        fprintf( fp, "  NR %c %d %d %d %d\n", UPPER( tReset->command ),
                           tReset->extra, tReset->arg1, tReset->arg2, tReset->rchance );
                        if( tReset->first_reset )
                        {
                           for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
                           {
                              if( gReset->command != 'p' && gReset->command != 'P' )
                                 continue;
                              fprintf( fp, "    NR %c %d %d %d %d %d\n", UPPER( gReset->command ),
                                 gReset->extra, gReset->arg1, gReset->arg2, gReset->arg3, gReset->rchance );
                           }
                        }
                        break;

                     case 't':
                     case 'T':
                     case 'h':
                     case 'H':
                        fprintf( fp, "  NR %c %d %d %d %d %d\n", UPPER( tReset->command ),
                           tReset->extra, tReset->arg1, tReset->arg2, tReset->arg3, tReset->rchance );
                        break;
                  }
               }
               break;

            case 'd':
            case 'D':
            case 't':
            case 'T':
            case 'h':
            case 'H':
               fprintf( fp, "NR %c %d %d %d %d %d\n", UPPER( pReset->command ),
                  pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3, pReset->rchance );
               break;

            case 'r':
            case 'R':
               fprintf( fp, "NR %c %d %d %d %d\n", UPPER( pReset->command ), pReset->extra, pReset->arg1,
                  pReset->arg2, pReset->rchance );
               break;
         }
      }

      for( ed = room->first_extradesc; ed; ed = ed->next )
         fprintf( fp, "E\n%s~\n%s~\n", ed->keyword, strip_cr( ed->description ) );

      if( room->mudprogs )
      {
         int count = 0;
         for( mprog = room->mudprogs; mprog; mprog = mprog->next )
            count = save_programs( mprog, fp );
         if( count > 0 )
            fprintf( fp, "%s", "|\n" );
      }
      fprintf( fp, "End\n\n" );
   }
   if( !first )
      fprintf( fp, "#0\n\n\n" );

   /* END */
   fprintf( fp, "#$\n" );
   fclose( fp );
   fp = NULL;
   rename( tempfilename, newfilename );
   return true;
}

CMDF( do_savearea )
{
   AREA_DATA *tarea;
   char filename[256];

   set_char_color( AT_IMMORT, ch );

   if( is_npc( ch ) || !ch->pcdata
   || ( ( !argument || argument[0] == '\0' ) && !ch->pcdata->area )
   || ( get_trust( ch ) < PERM_LEADER && !ch->pcdata->area ) )
   {
      send_to_char( "You don't have an assigned area to save.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' || get_trust( ch ) < PERM_LEADER )
      tarea = ch->pcdata->area;
   else
   {
      bool found = false;

      if( get_trust( ch ) < PERM_LEADER )
      {
         send_to_char( "You can only save your own area.\r\n", ch );
         return;
      }
      for( tarea = first_build; tarea; tarea = tarea->next )
      {
         if( !str_cmp( tarea->filename, argument ) )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Area not found.\r\n", ch );
         return;
      }
   }

   if( !tarea )
   {
      send_to_char( "No area to save.\r\n", ch );
      return;
   }

   /* Ensure not wiping out their area with save before load - Scryn 8/11 */
   if( !IS_SET( tarea->status, AREA_LOADED ) )
   {
      send_to_char( "Your area is not loaded!\r\n", ch );
      return;
   }

   if( !xIS_SET( tarea->flags, AFLAG_PROTOTYPE ) )
   {
      ch_printf( ch, "Can't savearea %s, use foldarea instead.\r\n", tarea->filename );
      return;
   }
   snprintf( filename, sizeof( filename ), "%s", tarea->filename );
   ch_printf( ch, "Saving area %s...\r\n", tarea->filename );
   fold_area( tarea, filename, false );
   set_char_color( AT_IMMORT, ch );
   send_to_char( "Finished saving the area.\r\n", ch );
}

/*
 * Dangerous command.  Can be used to install an area that was either:
 *   (a) already installed but removed from area.lst (Not sure how that one would work lol).
 *   (b) designed offline (Possible reason and would save a hotboot/reboot/shutdown).
 * The mud will likely crash if:
 *   (a) this area is already loaded (Will shut down the mud because of b).
 *   (b) it contains vnums that exist (Will shut down on first already used vnum).
 *   (c) the area has errors (Should just give bug messages).
 *
 * NOTE: Use of this command is not recommended.		-Thoric
 *
 * NOTE: Why in the world was this called unfoldarea if used to load areas....
 *       Since I don't have loadarea it seems way more fitting to use that then unfoldarea.
 *       Also tossed in some safe guards against some things to hopefully keep it from exiting,
 *       have the filename correct instead of $ and keep it from allowing things like loadarea ../player/r/Remcon
 *       granted all it did was exit once it seen PLAYER, but no point in allowing it to start with.
 *       *************Could still crash because of wrong format, errors etc....*************
 *       - Remcon
 */
CMDF( do_loadarea )
{
   AREA_DATA *pArea;
   struct stat fst;
   char buf[MSL];

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Loadarea what?\r\n", ch );
      return;
   }

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      if( !str_cmp( pArea->filename, argument ) )
      {
         send_to_char( "Already an area loaded using that filename.\r\n", ch );
         return;
      }
   }
   for( pArea = first_build; pArea; pArea = pArea->next )
   {
      if( !str_cmp( pArea->filename, argument ) )
      {
         send_to_char( "Already an area being built loaded using that filename.\r\n", ch );
         return;
      }
   }

   snprintf( buf, sizeof( buf ), "%s%s", AREA_DIR, argument );
   if( !is_valid_path( ch, AREA_DIR, argument ) )
      return;
   if( stat( buf, &fst ) == -1 )
   {
      ch_printf( ch, "%s was not found.\r\n", argument );
      return;
   }

   mudstrlcpy( strArea, argument, sizeof( strArea ) );
   fBootDb = true;
   unfoldload = true;
   load_area_file( last_area, argument );
   fBootDb = false;
   unfoldload = false;
   unfoldbadload = false;
}

CMDF( do_foldarea )
{
   AREA_DATA *tarea;

   set_char_color( AT_IMMORT, ch );

   if( is_npc( ch ) || !ch->pcdata
   || ( ( !argument || argument[0] == '\0' ) && !ch->pcdata->area )
   || ( get_trust( ch ) < PERM_LEADER && !ch->pcdata->area ) )
   {
      send_to_char( "You don't have an assigned area to fold.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' || get_trust( ch ) < PERM_LEADER )
      tarea = ch->pcdata->area;
   else
   {
      bool found = false;

      if( get_trust( ch ) < PERM_LEADER )
      {
         send_to_char( "You can only save your own area.\r\n", ch );
         return;
      }
      for( tarea = first_area; tarea; tarea = tarea->next )
      {
         if( !str_cmp( tarea->filename, argument ) )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         send_to_char( "Area not found.\r\n", ch );
         return;
      }
   }

   if( !tarea )
   {
      send_to_char( "No area to fold.\r\n", ch );
      return;
   }

   if( xIS_SET( tarea->flags, AFLAG_PROTOTYPE ) )
   {
      ch_printf( ch, "Can't foldarea %s, use savearea instead.\r\n", tarea->filename );
      return;
   }

   send_to_char( "Folding area...\r\n", ch );
   fold_area( tarea, tarea->filename, false );
   set_char_color( AT_IMMORT, ch );
   send_to_char( "Done.\r\n", ch );
}

void write_area_list( void )
{
   AREA_DATA *tarea;
   FILE *fp;

   if( !( fp = fopen( AREA_LIST, "w" ) ) )
   {
      bug( "FATAL: can't open %s for writing!", AREA_LIST );
      return;
   }
   for( tarea = first_area; tarea; tarea = tarea->next )
   {
      if( xIS_SET( tarea->flags, AFLAG_NOLOAD ) )
         continue;

      fprintf( fp, "%s\n", tarea->filename );
   }
   fprintf( fp, "$\n" );
   fclose( fp );
   fp = NULL;
}

/*
 * A complicated to use command as it currently exists.		-Thoric
 * Once area->author and area->name are cleaned up... it will be easier
 */
CMDF( do_installarea )
{
   AREA_DATA *tarea;
   DESCRIPTOR_DATA *d;
   char arg[MIL], buf[MSL];
   int num;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Usage: installarea <filename> [Area title]\r\n", ch );
      return;
   }

   for( tarea = first_build; tarea; tarea = tarea->next )
   {
      if( !str_cmp( tarea->filename, arg ) )
      {
         if( argument && argument[0] != '\0' )
         {
            STRSET( tarea->name, argument );
         }

         /* Fold area with install flag -- auto-removes prototype flags */
         send_to_char( "Saving and installing file...\r\n", ch );
         if( !( fold_area( tarea, tarea->filename, true ) ) )
         {
           send_to_char( "There was a problem in folding the area...\r\n", ch );
           return;
         }

         /* Remove from prototype area list */
         UNLINK( tarea, first_build, last_build, next, prev );

         /* Add to real area list */
         LINK( tarea, first_area, last_area, next, prev );

         /* Remove it from the prototype sort list. BUGFIX: Samson 4-15-03 */
         UNLINK( tarea, first_bsort, last_bsort, next_sort, prev_sort );

         /* Sort the area into it's proper sort list. BUGFIX: Samson 4-15-03 */
         sort_area( tarea, false );

         /* Don't forget to update the list */
         sort_area_by_name( tarea );

         /* Fix up author if online */
         for( d = first_descriptor; d; d = d->next )
         {
            if( d->character && d->character->pcdata && d->character->pcdata->area == tarea )
            {
               /* remove area from author */
               d->character->pcdata->area = NULL;
               /* clear out author vnums */
               d->character->pcdata->range_lo = 0;
               d->character->pcdata->range_hi = 0;
               save_char_obj( d->character );
            }
         }
         top_area++;
         send_to_char( "Writing area.lst...\r\n", ch );
         write_area_list( );
         send_to_char( "Resetting new area.\r\n", ch );
         num = tarea->nplayer;
         tarea->nplayer = 0;
         reset_area( tarea );
         tarea->nplayer = num;
         send_to_char( "Renaming author's building file.\r\n", ch );
         snprintf( buf, sizeof( buf ), "%s%s.installed", BUILD_DIR, tarea->filename );
         snprintf( arg, sizeof( arg ), "%s%s", BUILD_DIR, tarea->filename );
         rename( arg, buf );
         send_to_char( "Done.\r\n", ch );
         return;
      }
   }
   send_to_char( "No such area exists.\r\n", ch );
}

CMDF( do_astat )
{
   AREA_DATA *tarea;
   bool proto = false, found = false;

   set_char_color( AT_PLAIN, ch );

   tarea = ch->in_room->area;

   if( argument && argument[0] != '\0' )
   {
      for( tarea = first_area; tarea; tarea = tarea->next )
      {
         if( !str_cmp( tarea->filename, argument ) )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         for( tarea = first_build; tarea; tarea = tarea->next )
         {
            if( !str_cmp( tarea->filename, argument ) )
            {
               found = true;
               proto = true;
               break;
            }
         }
      }
      if( !found )
      {
         send_to_char( "Area not found.  Check 'zones'.\r\n", ch );
         return;
      }
   }

   ch_printf( ch, "\r\n&wName:      &W%s\r\n", tarea->name );
   ch_printf( ch, "&wVnum:      &W%d\r\n", tarea->vnum );
   ch_printf( ch, "&wFilename:  &W%-20s\r\n", tarea->filename );
   ch_printf( ch, "&wPrototype: &W%s\r\n", proto ? "yes" : "no" );
   ch_printf( ch, "&wAuthor:    &W%s\r\n", tarea->author );
   ch_printf( ch, "&wAge:       &W%-3d\r\n", tarea->age );
   ch_printf( ch, "&wPlayers:    &W%10d   &wMax: &W%d\r\n", tarea->nplayer, tarea->max_players );

   ch_printf( ch, "&wVnum range: &W%10d &w- &W%d\r\n", tarea->low_vnum, tarea->hi_vnum );
   ch_printf( ch, "&wSoft range: &W%10d &w- &W%d\r\n", tarea->low_soft_range, tarea->hi_soft_range );
   ch_printf( ch, "&wHard range: &W%10d &w- &W%d\r\n", tarea->low_hard_range, tarea->hi_hard_range );

   ch_printf( ch, "&wArea flags: &W%s\r\n", ext_flag_string( &tarea->flags, area_flags ) );
   ch_printf( ch, "&wResetmsg: &W%s\r\n", tarea->resetmsg ? tarea->resetmsg : "(default)" ); /* Rennard */
   ch_printf( ch, "&wReset frequency: &W%d &wminutes.\r\n", tarea->reset_frequency ? tarea->reset_frequency : 15 );
}

/* check other areas for a conflict while ignoring the current area */
bool check_for_area_conflicts( AREA_DATA *carea, int lo, int hi )
{
   AREA_DATA *area;

   for( area = first_area; area; area = area->next )
      if( area != carea && check_area_conflict( area, lo, hi ) )
         return true;
   for( area = first_build; area; area = area->next )
      if( area != carea && check_area_conflict( area, lo, hi ) )
         return true;

   return false;
}

bool check_area_vnum_conflicts( int vnum )
{
   AREA_DATA *area;

   for( area = first_area; area; area = area->next )
      if( area->vnum == vnum )
         return true;

   for( area = first_build; area; area = area->next )
      if( area->vnum == vnum )
         return true;

   return false;
}

CMDF( do_aset )
{
   AREA_DATA *tarea;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int vnum, value;
   bool proto, found;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   vnum = atoi( argument );
   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Usage: aset <area filename> <field> <value>\r\n", ch );
      send_to_char( "Usage: aset <area filename> noload\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "  low_vnum     hi_vnum     low_soft  hi_soft   low_hard   hi_hard\r\n", ch );
      send_to_char( "  name         filename    author    resetmsg  resetfreq  flags\r\n", ch );
      send_to_char( "  vnum\r\n", ch );
      return;
   }

   found = false;
   proto = false;
   for( tarea = first_area; tarea; tarea = tarea->next )
   {
      if( !str_cmp( tarea->filename, arg1 ) )
      {
         found = true;
         break;
      }
   }
   if( !found )
   {
      for( tarea = first_build; tarea; tarea = tarea->next )
      {
         if( !str_cmp( tarea->filename, arg1 ) )
         {
            found = true;
            proto = true;
            break;
         }
      }
   }
   if( !found )
   {
      send_to_char( "Area not found.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "noload" ) )
   {
      if( xIS_SET( tarea->flags, AFLAG_NOLOAD ) )
      {
         xREMOVE_BIT( tarea->flags, AFLAG_NOLOAD );
         send_to_char( "That area will now be added to the area list to be loaded.\r\n", ch );
      }
      else
      {
         xSET_BIT( tarea->flags, AFLAG_NOLOAD );
         send_to_char( "That area will now be removed from the area list so it doesn't load next boot.\r\n", ch );
      }
      send_to_char( "Writing area.lst...\r\n", ch );
      write_area_list( );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      AREA_DATA *uarea;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set an area's name to nothing.\r\n", ch );
         return;
      }
      for( uarea = first_area; uarea; uarea = uarea->next )
      {
         if( !str_cmp( uarea->name, argument ) )
         {
            send_to_char( "There is already an installed area with that name.\r\n", ch );
            return;
         }
      }
      for( uarea = first_build; uarea; uarea = uarea->next )
      {
         if( !str_cmp( uarea->name, argument ) )
         {
            send_to_char( "There is already a prototype area with that name.\r\n", ch );
            return;
         }
      }
      STRSET( tarea->name, argument );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256], newfilename[256];

      if( proto )
      {
         send_to_char( "You should only change the filename of installed areas.\r\n", ch );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set an area's filename to nothing.\r\n", ch );
         return;
      }
      if( strstr( argument, "." ) )
      {
         send_to_char( "You can't use a . in area filenames. (.are) is added onto the end automatically.\r\n", ch );
         return;
      }      
      snprintf( newfilename, sizeof( newfilename ), "%s.are", argument );
      if( !can_use_path( ch, AREA_DIR, newfilename ) )
         return;
      snprintf( filename, sizeof( filename ), "%s%s", AREA_DIR, tarea->filename );
      STRSET( tarea->filename, newfilename );
      snprintf( newfilename, sizeof( newfilename ), "%s%s", AREA_DIR, tarea->filename );
      rename( filename, newfilename );
      write_area_list( );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "vnum" ) )
   {
      if( tarea->vnum == vnum )
      {
         send_to_char( "That area is already set to that vnum.\r\n", ch );
         return;
      }
      if( vnum < 1 )
      {
         send_to_char( "You can only set the vnum for the area to 1 or higher.\r\n", ch );
         return;
      }
      if( check_area_vnum_conflicts( vnum ) )
      {
         send_to_char( "Another area is already using that vnum.\r\n", ch );
         return;
      }
      tarea->vnum = vnum;
      ch_printf( ch, "Area vnum set to %d.\r\n", tarea->vnum );
      return;
   }

   if( !str_cmp( arg2, "low_vnum" ) )
   {
      if( check_for_area_conflicts( tarea, tarea->low_vnum, vnum ) )
      {
         send_to_char( "That would conflict with another area.\r\n", ch );
         return;
      }
      if( tarea->hi_vnum < vnum )
      {
         send_to_char( "Can't set low_vnum higher than the hi_vnum.\r\n", ch );
         return;
      }
      tarea->low_vnum = vnum;
      ch_printf( ch, "Low_vnum set to %d.\r\n", tarea->low_vnum );
      return;
   }

   if( !str_cmp( arg2, "hi_vnum" ) )
   {
      if( check_for_area_conflicts( tarea, tarea->hi_vnum, vnum ) )
      {
         send_to_char( "That would conflict with another area.\r\n", ch );
         return;
      }
      if( tarea->low_vnum > vnum )
      {
         send_to_char( "Can't set hi_vnum lower than the low_vnum.\r\n", ch );
         return;
      }
      tarea->hi_vnum = vnum;
      ch_printf( ch, "Hi_vnum set to %d.\r\n", tarea->hi_vnum );
      return;
   }

   if( !str_cmp( arg2, "low_soft" ) )
   {
      tarea->low_soft_range = URANGE( 0, vnum, MAX_LEVEL );
      ch_printf( ch, "Low_soft set to level %d.\r\n", tarea->low_soft_range );
      return;
   }

   if( !str_cmp( arg2, "hi_soft" ) )
   {
      tarea->hi_soft_range = URANGE( 0, vnum, MAX_LEVEL );
      ch_printf( ch, "Hi_soft set to level %d.\r\n", tarea->hi_soft_range );
      return;
   }

   if( !str_cmp( arg2, "low_hard" ) )
   {
      tarea->low_hard_range = URANGE( 0, vnum, MAX_LEVEL );
      ch_printf( ch, "Low_hard set to level %d.\r\n", tarea->low_hard_range );
      return;
   }

   if( !str_cmp( arg2, "hi_hard" ) )
   {
      tarea->hi_hard_range = URANGE( 0, vnum, MAX_LEVEL );
      ch_printf( ch, "Hi_hard set to level %d.\r\n", tarea->hi_hard_range );
      return;
   }

   if( !str_cmp( arg2, "author" ) )
   {
      STRSET( tarea->author, argument );
      ch_printf( ch, "Author set to %s.\r\n", tarea->author ? tarea->author : "(Nothing)" );
      return;
   }

   if( !str_cmp( arg2, "resetmsg" ) )
   {
      STRSET( tarea->resetmsg, argument );
      ch_printf( ch, "Resetmsg set to %s.\r\n", tarea->resetmsg ? tarea->resetmsg : "(Nothing)" );
      return;
   }

   if( !str_cmp( arg2, "resetfreq" ) )
   {
      tarea->reset_frequency = UMAX( 0, vnum );
      ch_printf( ch, "Resetfreq set to %d.\r\n", tarea->reset_frequency );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: aset <filename> flags <flag> [flag]...\r\n", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, area_flags, AFLAG_MAX );
         if( value < 0 || value >= AFLAG_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
         else
            xTOGGLE_BIT( tarea->flags, value );
      }
      ch_printf( ch, "Flags set to %s.\r\n", ext_flag_string( &tarea->flags, area_flags ) );
      return;
   }

   do_aset( ch, (char *)"" );
}

/* List stuff in the range for which ever */
/*
 * 0 = rooms
 * 1 = objects
 * 2 = mobiles
 * 3 and higher does all three
 */
void do_arealist( CHAR_DATA *ch, short type, char *argument )
{
   ROOM_INDEX_DATA *room;
   OBJ_INDEX_DATA *obj;
   MOB_INDEX_DATA *mob;
   AREA_DATA *tarea;
   char arg1[MIL], arg2[MIL], strcheck[MIL];
   int vnum, lrange, trange, cnt = 0;
   bool free = false;

   set_pager_color( AT_PLAIN, ch );
   if( is_npc( ch ) || !ch->pcdata || ( !ch->pcdata->area && get_trust( ch ) < PERM_LEADER ) )
   {
      send_to_char( "&YYou don't have an assigned area.\r\n", ch );
      return;
   }

   tarea = ch->pcdata->area;
   if( !tarea && ch->in_room && ch->in_room->area )
      tarea = ch->in_room->area;

   strcheck[0] = '\0';
   argument = one_argument( argument, arg1 );
   if( !is_number( arg1 ) )
   {
      if( !str_cmp( arg1, "free" ) )
      {
         free = true;
         argument = one_argument( argument, arg1 );
      }
      else
      {
        snprintf( strcheck, sizeof( strcheck ), "%s", arg1 );
        argument = one_argument( argument, arg1 );
      }
   }
   argument = one_argument( argument, arg2 );

   lrange = ( arg1 != NULL && arg1[0] != '\0' && is_number( arg1 ) ? atoi( arg1 ) : tarea ? tarea->low_vnum : -1 );
   trange = ( arg2 != NULL && arg2[0] != '\0' && is_number( arg2 ) ? atoi( arg2 ) : tarea ? tarea->hi_vnum : -1 );

   if( tarea && ( lrange < tarea->low_vnum || trange > tarea->hi_vnum ) && get_trust( ch ) < PERM_LEADER )
   {
      send_to_char( "&YThat is out of your vnum range.\r\n", ch );
      return;
   }

   if( lrange <= 0 && trange <= 0 )
   {
      send_to_char( "&YYou have to specify a low and high vnum and they have to be above 0.\r\n", ch );
      return;
   }
   if( trange <= lrange )
   {
      send_to_char( "&YThe first vnum should be lower then the second. EXAMPLE ( 400 500 ).\r\n", ch );
      return;
   }
   if( !tarea && ( trange - lrange ) > 1000 )
   {
      send_to_char( "&YLimiting the amount of stuff shown to 1000 possible vnums...\r\n", ch );
      trange = ( lrange + 1000 );
   }
   pager_printf( ch, "Displaying %svnums %d to %d.\r\n", free ? "free " : "", lrange, trange );
   if( strcheck != NULL && strcheck[0] != '\0' )
      pager_printf( ch, "Only displaying things that match: %s.\r\n", strcheck );
   for( vnum = lrange; vnum <= trange; vnum++ )
   {
      if( !free )
      {
         if( ( type == 0 || type > 2 ) && ( room = get_room_index( vnum ) )
         && ( strcheck == NULL || strcheck[0] == '\0' || nifty_is_name_prefix( strcheck, room->name ) ) )
            pager_printf( ch, "%s%5d) %s\r\n", type > 2 ? "ROOM    #" : "", vnum, room->name );
         if( ( type == 1 || type > 2 ) && ( obj = get_obj_index( vnum ) )
         && ( strcheck == NULL || strcheck[0] == '\0' || nifty_is_name_prefix( strcheck, obj->name ) ) )
            pager_printf( ch, "%s%5d) %-20s (%s)\r\n", type > 2 ? "OBJECT  #" : "", vnum, obj->name, obj->short_descr );
         if( ( type == 2 || type > 2 ) && ( mob = get_mob_index( vnum ) )
         && ( strcheck == NULL || strcheck[0] == '\0' || nifty_is_name_prefix( strcheck, mob->name ) ) )
            pager_printf( ch, "%s%5d) %-20s (%s)\r\n", type > 2 ? "MOBILE  #" : "", vnum, mob->name, mob->short_descr );
      }
      else /* Redone to do a slightly better display */
      {
         if( ( type == 0 || type > 2 ) && !( room = get_room_index( vnum ) ) )
         {
            pager_printf( ch, "%s%5d", type > 2 ? "ROOM    #" : "", vnum );
            if( type > 2 || cnt++ >= 6 )
            {
               cnt = 0;
               send_to_pager( "\r\n", ch );
            }
            else
               send_to_pager( " ", ch );
         }
         if( ( type == 1 || type > 2 ) && !( obj = get_obj_index( vnum ) ) )
         {
            pager_printf( ch, "%s%5d", type > 2 ? "OBJECT  #" : "", vnum );
            if( type > 2 || cnt++ >= 6 )
            {
               cnt = 0;
               send_to_pager( "\r\n", ch );
            }
            else
               send_to_pager( " ", ch );
         }
         if( ( type == 2 || type > 2 ) && !( mob = get_mob_index( vnum ) ) )
         {
            pager_printf( ch, "%s%5d", type > 2 ? "MOBILE  #" : "", vnum );
            if( type > 2 || cnt++ >= 6 )
            {
               cnt = 0;
               send_to_pager( "\r\n", ch );
            }
            else
               send_to_pager( " ", ch );
         }
      }
   }
   if( cnt != 0 )
      send_to_pager( "\r\n", ch );
}

CMDF( do_rlist )
{
   do_arealist( ch, 0, argument );
}

CMDF( do_olist )
{
   do_arealist( ch, 1, argument );
}

CMDF( do_mlist )
{
   do_arealist( ch, 2, argument );
}

CMDF( do_vlist )
{
   do_arealist( ch, 3, argument );
}
