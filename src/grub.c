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
 *                                Grub structure                             *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "h/mud.h"

/* Check the rooms and inform if something was found */
bool grub_room( CHAR_DATA *ch, int icheck, int minamount, int maxamount, EXT_BV iflags )
{
   ROOM_INDEX_DATA *room;
   int icnt;
   bool found = false;

   for( icnt = 0; icnt < MKH; icnt++ )
   {
      for( room = room_index_hash[icnt]; room; room = room->next )
      {
         /* 1 is a tunnel check */
         if( icheck == 1 )
         {
            if( maxamount != -1 )
            {
               if( room->tunnel < minamount || room->tunnel > maxamount )
                  continue;
            }
            else if( room->tunnel != minamount )
               continue;
            found = true;
            ch_printf( ch, "Room %5d has tunnel set to %d.\r\n", room->vnum, room->tunnel );
         }
         /* 2 is a sector check */
         else if( icheck == 2 )
         {
            if( room->sector_type != minamount )
               continue;
            found = true;
            ch_printf( ch, "Room %5d has sector set to %s.\r\n", room->vnum, sect_flags[room->sector_type] );
         }
         /* 3 is a televnum check */
         else if( icheck == 3 )
         {
            if( minamount != -1 && room->tele_vnum != minamount )
            {
               if( room->tele_vnum != minamount )
                  continue;
            }
            found = true;
            ch_printf( ch, "Room %5d has televnum set to %d.\r\n", room->vnum, room->tele_vnum );
         }
         /* 4 is checking flags */
         else if( icheck == 4 )
         {
            if( xHAS_ALL_BITS( room->room_flags, iflags ) )
            {
               found = true;
               ch_printf( ch, "Room %5d has the [%s] flag(s) set.\r\n", room->vnum, ext_flag_string( &iflags, r_flags )  );
            }
         }
         /* 5 is checking light */
         else if( icheck == 5 )
         {
            if( maxamount != -1 )
            {
               if( room->light < minamount || room->light > maxamount )
                  continue;
            }
            else if( room->light != minamount )
               continue;
            found = true;
            ch_printf( ch, "Room %5d has light set to %d.\r\n", room->vnum, room->light );
         }
      }
   }
   return found;
}

/* Check the objects and inform if something was found */
bool grub_obj( CHAR_DATA *ch, int icheck, int minamount, int maxamount, EXT_BV iflags )
{
   OBJ_INDEX_DATA *obj;
   int icnt;
   bool found = false;

   for( icnt = 0; icnt < MKH; icnt++ )
   {
      for( obj = obj_index_hash[icnt]; obj; obj = obj->next )
      {
         /* 1 is checking flags */
         if( icheck == 1 )
         {
            if( xHAS_ALL_BITS( obj->extra_flags, iflags ) )
            {
               found = true;
               ch_printf( ch, "Object %5d has the [%s] flag(s) set.\r\n", obj->vnum, ext_flag_string( &iflags, o_flags )  );
            }
         }
         /* 2 is a weight check */
         else if( icheck == 2 )
         {
            if( maxamount != -1 )
            {
               if( obj->weight < minamount || obj->weight > maxamount )
                  continue;
            }
            else if( obj->weight != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has weight set to %d.\r\n", obj->vnum, obj->weight );
         }
         /* 3 is checking the type */
         else if( icheck == 3 )
         {
            if( obj->item_type != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has type set to %s.\r\n", obj->vnum, o_types[obj->item_type] );
         }
         /* 4 is checking the level */
         else if( icheck == 4 )
         {
            if( maxamount != -1 )
            {
               if( obj->level < minamount || obj->level > maxamount )
                  continue;
            }
            else if( obj->level != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has level set to %d.\r\n", obj->vnum, obj->level );
         }
         /* 5 is checking the cost */
         else if( icheck == 5 )
         {
            if( maxamount != -1 )
            {
               if( obj->cost < minamount || obj->cost > maxamount )
                  continue;
            }
            else if( obj->cost != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has cost set to %d.\r\n", obj->vnum, obj->cost );
         }
         /* 6 - 11 is checking value[icheck-6] */
         else if( icheck == 6 || icheck == 7 || icheck == 8 || icheck == 9 || icheck == 10 || icheck == 11 )
         {
            short value = ( icheck - 6 );
            if( maxamount != -1 )
            {
               if( obj->value[value] < minamount || obj->value[value] > maxamount )
                  continue;
            }
            else if( obj->value[value] != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has value[%d] set to %d.\r\n", obj->vnum, value, obj->value[value] );
         }
         /* 12 is checking count */
         else if( icheck == 12 )
         {
            if( maxamount != -1 )
            {
               if( obj->count < minamount || obj->count > maxamount )
                  continue;
            }
            else if( obj->count != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has a count of %d.\r\n", obj->vnum, obj->count );
         }
         /* 13 is checking classes */
         else if( icheck == 13 )
         {
            if( xHAS_ALL_BITS( obj->class_restrict, iflags ) )
            {
               found = true;
               ch_printf( ch, "Object %5d has the [%s] class(es) restricted.\r\n", obj->vnum, ext_class_string( &iflags )  );
            }
         }
         /* 14 is checking races */
         else if( icheck == 14 )
         {
            if( xHAS_ALL_BITS( obj->race_restrict, iflags ) )
            {
               found = true;
               ch_printf( ch, "Object %5d has the [%s] race(s) restricted.\r\n", obj->vnum, ext_race_string( &iflags )  );
            }
         }
         /* 15 is checking layers */
         else if( icheck == 15 )
         {
            if( maxamount != -1 )
            {
               if( obj->layers < minamount || obj->layers > maxamount )
                  continue;
            }
            else if( obj->layers != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has layers set to %d.\r\n", obj->vnum, obj->layers );
         }
         /* 16 is checking wearflags */
         else if( icheck == 16 )
         {
            if( xHAS_ALL_BITS( obj->wear_flags, iflags ) )
            {
               found = true;
               ch_printf( ch, "Object %5d has the [%s] wear flag(s) set.\r\n", obj->vnum, ext_flag_string( &iflags, w_flags )  );
            }
         }
         /* 17 - 23 is checking stats[icheck-17] */
         else if( icheck == 17 || icheck == 18 || icheck == 19 || icheck == 20 || icheck == 21 || icheck == 22 || icheck == 23 )
         {
            short value = ( icheck - 17 );
            if( maxamount != -1 )
            {
               if( obj->stat_reqs[value] < minamount || obj->stat_reqs[value] > maxamount )
                  continue;
            }
            else if( obj->stat_reqs[value] != minamount )
               continue;
            found = true;
            ch_printf( ch, "Object %5d has %s set to %d.\r\n", obj->vnum, stattypes[value], obj->stat_reqs[value] );
         }
      }
   }
   return found;
}

/* Check the mobiles and inform if something was found */
bool grub_mob( CHAR_DATA *ch, int icheck, int minamount, int maxamount, EXT_BV iflags )
{
   MOB_INDEX_DATA *mob;
   int icnt;
   bool found = false;

   for( icnt = 0; icnt < MKH; icnt++ )
   {
      for( mob = mob_index_hash[icnt]; mob; mob = mob->next )
      {
         /* 1 is checking the level */
         if( icheck == 1 )
         {
            if( maxamount != -1 )
            {
               if( mob->level < minamount || mob->level > maxamount )
                  continue;
            }
            else if( mob->level != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has level set to %d.\r\n", mob->vnum, mob->level );
         }
         /* 2 is checking the killed */
         else if( icheck == 2 )
         {
            if( maxamount != -1 )
            {
               if( mob->killed < minamount || mob->killed > maxamount )
                  continue;
            }
            else if( mob->killed != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has been killed %d times.\r\n", mob->vnum, mob->killed );
         }
         /* 3 is checking the count */
         else if( icheck == 3 )
         {
            if( maxamount != -1 )
            {
               if( mob->count < minamount || mob->count > maxamount )
                  continue;
            }
            else if( mob->count != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has a count of %d.\r\n", mob->vnum, mob->count );
         }
         /* 4 is checking the damroll */
         else if( icheck == 4 )
         {
            if( maxamount != -1 )
            {
               if( mob->damroll < minamount || mob->damroll > maxamount )
                  continue;
            }
            else if( mob->damroll != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has damroll set to %d.\r\n", mob->vnum, mob->damroll );
         }
         /* 5 is checking the hitroll */
         else if( icheck == 5 )
         {
            if( maxamount != -1 )
            {
               if( mob->hitroll < minamount || mob->hitroll > maxamount )
                  continue;
            }
            else if( mob->hitroll != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has hitroll set to %d.\r\n", mob->vnum, mob->hitroll );
         }
         /* 6 is checking the height */
         else if( icheck == 6 )
         {
            if( maxamount != -1 )
            {
               if( mob->height < minamount || mob->height > maxamount )
                  continue;
            }
            else if( mob->height != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has height set to %d.\r\n", mob->vnum, mob->height );
         }
         /* 7 is checking the weight */
         else if( icheck == 7 )
         {
            if( maxamount != -1 )
            {
               if( mob->weight < minamount || mob->weight > maxamount )
                  continue;
            }
            else if( mob->weight != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has weight set to %d.\r\n", mob->vnum, mob->weight );
         }
         /* 8 is checking the minhit */
         else if( icheck == 8 )
         {
            if( maxamount != -1 )
            {
               if( mob->minhit < minamount || mob->minhit > maxamount )
                  continue;
            }
            else if( mob->minhit != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has minhit set to %d.\r\n", mob->vnum, mob->minhit );
         }
         /* 9 is checking the maxhit */
         else if( icheck == 9 )
         {
            if( maxamount != -1 )
            {
               if( mob->maxhit < minamount || mob->maxhit > maxamount )
                  continue;
            }
            else if( mob->maxhit != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has maxhit set to %d.\r\n", mob->vnum, mob->maxhit );
         }
         /* 10 is checking the saving_spell_staff */
         else if( icheck == 10 )
         {
            if( maxamount != -1 )
            {
               if( mob->saving_spell_staff < minamount || mob->saving_spell_staff > maxamount )
                  continue;
            }
            else if( mob->saving_spell_staff != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has saving_spell_staff set to %d.\r\n", mob->vnum, mob->saving_spell_staff );
         }
         /* 11 is checking the saving_breath */
         else if( icheck == 11 )
         {
            if( maxamount != -1 )
            {
               if( mob->saving_breath < minamount || mob->saving_breath > maxamount )
                  continue;
            }
            else if( mob->saving_breath != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has saving_breath set to %d.\r\n", mob->vnum, mob->saving_breath );
         }
         /* 12 is checking the saving_para_petri */
         else if( icheck == 12 )
         {
            if( maxamount != -1 )
            {
               if( mob->saving_para_petri < minamount || mob->saving_para_petri > maxamount )
                  continue;
            }
            else if( mob->saving_para_petri != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has saving_para_petri set to %d.\r\n", mob->vnum, mob->saving_para_petri );
         }
         /* 13 is checking the saving_wand */
         else if( icheck == 13 )
         {
            if( maxamount != -1 )
            {
               if( mob->saving_wand < minamount || mob->saving_wand > maxamount )
                  continue;
            }
            else if( mob->saving_wand != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has saving_wand set to %d.\r\n", mob->vnum, mob->saving_wand );
         }
         /* 14 is checking the saving_poison_death */
         else if( icheck == 14 )
         {
            if( maxamount != -1 )
            {
               if( mob->saving_poison_death < minamount || mob->saving_poison_death > maxamount )
                  continue;
            }
            else if( mob->saving_poison_death != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has saving_poison_death set to %d.\r\n", mob->vnum, mob->saving_poison_death );
         }
         /* 15 is checking the numattacks */
         else if( icheck == 15 )
         {
            if( maxamount != -1 )
            {
               if( mob->numattacks < minamount || mob->numattacks > maxamount )
                  continue;
            }
            else if( mob->numattacks != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has numattacks set to %d.\r\n", mob->vnum, mob->numattacks );
         }
         /* 16 is checking the ac */
         else if( icheck == 16 )
         {
            if( maxamount != -1 )
            {
               if( mob->ac < minamount || mob->ac > maxamount )
                  continue;
            }
            else if( mob->ac != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has ac set to %d.\r\n", mob->vnum, mob->ac );
         }
         /* 17 is checking the alignment */
         else if( icheck == 17 )
         {
            if( maxamount != -1 )
            {
               if( mob->alignment < minamount || mob->alignment > maxamount )
                  continue;
            }
            else if( mob->alignment != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has alignment set to %d.\r\n", mob->vnum, mob->alignment );
         }
         /* 18 and 19 are checking the gold */
         else if( icheck == 18 || icheck == 19 )
         {
            if( maxamount != -1 )
            {
               if( mob->gold < minamount || mob->gold > maxamount )
                  continue;
            }
            else if( mob->gold != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has gold set to %d.\r\n", mob->vnum, mob->gold );
         }
         /* 20 is checking act flags */
         else if( icheck == 20 )
         {
            if( xHAS_ALL_BITS( mob->act, iflags ) )
            {
               found = true;
               ch_printf( ch, "Mobile %5d has the [%s] flag(s) set.\r\n", mob->vnum, ext_flag_string( &iflags, act_flags )  );
            }
         }
         /* 21 is checking the sex */
         else if( icheck == 21 )
         {
            if( mob->sex != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has sex set to %s.\r\n", mob->vnum, sex_names[mob->sex] );
         }
         /* 22 - 28 is checking perm_stats[icheck-22] */
         else if( icheck == 22 || icheck == 23 || icheck == 24 || icheck == 25 || icheck == 26 || icheck == 27 || icheck == 28 )
         {
            short value = ( icheck - 22 );
            if( maxamount != -1 )
            {
               if( mob->perm_stats[value] < minamount || mob->perm_stats[value] > maxamount )
                  continue;
            }
            else if( mob->perm_stats[value] != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has %s set to %d.\r\n", mob->vnum, stattypes[value], mob->perm_stats[value] );
         }
         /* 29 is checking affected_by */
         else if( icheck == 29 )
         {
            if( xHAS_ALL_BITS( mob->affected_by, iflags ) )
            {
               found = true;
               ch_printf( ch, "Mobile %5d has the [%s] affected_by(s) set.\r\n", mob->vnum, ext_flag_string( &iflags, a_flags )  );
            }
         }
         /* 30 is checking resistant */
         else if( icheck == 30 )
         {
            if( mob->resistant[minamount] != maxamount )
               continue;
            ch_printf( ch, "Mobile %5d has the [%s] resistant set to %d.\r\n", mob->vnum, ris_flags[minamount], mob->resistant[minamount] );
         }
         /* 31 and 32 Unused */
         /* 33 is checking xflags */
         else if( icheck == 33 )
         {
            if( xHAS_ALL_BITS( mob->xflags, iflags ) )
            {
               found = true;
               ch_printf( ch, "Mobile %5d has the [%s] xflag(s) set.\r\n", mob->vnum, ext_flag_string( &iflags, part_flags )  );
            }
         }
         /* 34 is checking attacks */
         else if( icheck == 34 )
         {
            if( xHAS_ALL_BITS( mob->attacks, iflags ) )
            {
               found = true;
               ch_printf( ch, "Mobile %5d has the [%s] attack(s) set.\r\n", mob->vnum, ext_flag_string( &iflags, attack_flags )  );
            }
         }
         /* 35 is checking defenses */
         else if( icheck == 35 )
         {
            if( xHAS_ALL_BITS( mob->defenses, iflags ) )
            {
               found = true;
               ch_printf( ch, "Mobile %5d has the [%s] defense(s) set.\r\n", mob->vnum, ext_flag_string( &iflags, defense_flags )  );
            }
         }
         /* 36 is Unused */
         /* 37 && 38 is checking the defposition */
         else if( icheck == 37 || icheck == 38 )
         {
            if( mob->defposition != minamount )
               continue;
            found = true;
            ch_printf( ch, "Mobile %5d has defposition set to %s.\r\n", mob->vnum, pos_names[mob->defposition] );
         }
      }
   }
   return found;
}

/* Check the resets and inform if something was found */
bool grub_reset( CHAR_DATA *ch, int icheck, int vnum )
{
   ROOM_INDEX_DATA *room;
   RESET_DATA *pReset, *tReset, *pReset_next, *tReset_next, *gReset, *gReset_next;
   int icnt;
   bool found = false;

   for( icnt = 0; icnt < MKH; icnt++ )
   {
      for( room = room_index_hash[icnt]; room; room = room->next )
      {
         for( pReset = room->first_reset; pReset; pReset = pReset_next )
         {
            pReset_next = pReset->next;

            switch( pReset->command )
            {
               default:
                  break;

               case 'M':
                  if( icheck == 1 ) /* Check for mobile vnum */
                  {
                     if( vnum == pReset->arg1 )
                     {
                        found = true;
                        ch_printf( ch, "Room %5d has a reset for that mobile vnum.\r\n", room->vnum );
                        break;
                     }
                  }
                  else if( icheck == 2 ) /* Check the other resets for object vnum */
                  {
                     for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
                     {
                        tReset_next = tReset->next_reset;

                        if( vnum == tReset->arg1 )
                        {
                           found = true;
                           ch_printf( ch, "Room %5d has a reset for that object vnum.\r\n", room->vnum );
                           break;
                        }

                        for( gReset = tReset->first_reset; gReset; gReset = gReset_next )
                        {
                           gReset_next = gReset->next_reset;

                           switch( gReset->command )
                           {
                              default:
                                 break;

                              case 'P':
                                 if( vnum == gReset->arg1 )
                                 {
                                    found = true;
                                    ch_printf( ch, "Room %5d has a reset for that object vnum.\r\n", room->vnum );
                                    break;
                                 }
                                 if( vnum == gReset->arg3 )
                                 {
                                    found = true;
                                    ch_printf( ch, "Room %5d has a reset for that object vnum.\r\n", room->vnum );
                                    break;
                                 }
                                 break;
                           }
                        }
                     }
                  }
                  break;

               case 'O':
                  if( icheck == 2 ) /* Check the other resets for object vnum */
                  {
                     if( vnum == pReset->arg1 )
                     {
                        found = true;
                        ch_printf( ch, "Room %5d has a reset for that object vnum.\r\n", room->vnum );
                        break;
                     }

                     for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
                     {
                        tReset_next = tReset->next_reset;

                        switch( tReset->command )
                        {
                           default:
                              break;

                           case 'P':
                              if( vnum == tReset->arg1 )
                              {
                                 found = true;
                                 ch_printf( ch, "Room %5d has a reset for that object vnum.\r\n", room->vnum );
                                 break;
                              }
                              if( vnum == tReset->arg3 )
                              {
                                 found = true;
                                 ch_printf( ch, "Room %5d has a reset for that object vnum.\r\n", room->vnum );
                                 break;
                              }
                              break;
                        }
                     }
                  }
                  break;
            }
         }
      }
   }
   return found;
}

CMDF( do_grub )
{
   EXT_BV iflags;
   char arg[MSL];
   int minamount = -1, maxamount = -1, stat;
   bool objchck = false, mobchck = false, roomchck = false;

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "obj" ) || !str_cmp( arg, "mob" ) || !str_cmp( arg, "room" ) || !str_cmp( arg, "reset" ) )
   {
      if( !str_cmp( arg, "reset" ) )
      {
         argument = one_argument( argument, arg );
         if( !str_cmp( arg, "obj" ) )
         {
            argument = one_argument( argument, arg );
            if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
            {
               send_to_char( "Usage: grub reset obj <#>\r\n", ch );
               return;
            }
            minamount = atoi( arg );
            if( !grub_reset( ch, 2, minamount ) )
               ch_printf( ch, "No resets found for object vnum %d.\r\n", minamount );
            return;
         }
         else if( !str_cmp( arg, "mob" ) )
         {
            argument = one_argument( argument, arg );
            if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
            {
               send_to_char( "Usage: grub reset mob <#>\r\n", ch );
               return;
            }
            minamount = atoi( arg );
            if( !grub_reset( ch, 1, minamount ) )
               ch_printf( ch, "No resets found for mobile vnum %d.\r\n", minamount );
            return;
         }
         else
         {
            send_to_char( "Usage: grub reset mob/obj <#>\r\n", ch );
            return;
         }
      }
      if( !str_cmp( arg, "obj" ) )
         objchck = true;
      else if( !str_cmp( arg, "mob" ) )
         mobchck = true;
      else if( !str_cmp( arg, "room" ) )
         roomchck = true;
   }
   else
   {
      send_to_char( "For usage information see 'help grub'.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   xCLEAR_BITS( iflags );

   if( objchck  )
   {
      if( !str_cmp( arg, "flags" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub obj flags <flag> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, o_flags, ITEM_MAX );
            if( minamount < 0 || minamount >= ITEM_MAX )
               ch_printf( ch, "Unknown object flag: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No flags would be checked.\r\n", ch );
            return;
         }
         if( !grub_obj( ch, 1, -1, -1, iflags ) )
            send_to_char( "No objects found with those flags set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "weight" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub obj weight <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_obj( ch, 2, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No objects found with weight between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No objects found with weight set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "type" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' )
         {
            send_to_char( "Usage: grub obj type <type>/<#>\r\n", ch );
            return;
         }
         minamount = get_flag( arg, o_types, ITEM_TYPE_MAX );
         if( minamount < 0 && is_number( arg ) )
            minamount = atoi( arg );
         if( minamount < 0 || minamount >= ITEM_TYPE_MAX )
            ch_printf( ch, "No such type to look for (%s).\r\n", arg );
         else if( !grub_obj( ch, 3, minamount, -1, iflags ) )
            ch_printf( ch, "No objects found with type set to %s.\r\n", o_types[minamount] );
         return;
      }
      if( !str_cmp( arg, "level" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub obj level <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount < 0 )
         {
            send_to_char( "Use a value equal to or higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_obj( ch, 4, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No objects found with level between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No objects found with level set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "cost" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub obj cost <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount < 0 )
         {
            send_to_char( "Use a value equal to or higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_obj( ch, 5, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No objects found with cost between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No objects found with cost set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "value0" ) || !str_cmp( arg, "value1" ) || !str_cmp( arg, "value2" )
      || !str_cmp( arg, "value3" ) || !str_cmp( arg, "value4" ) || !str_cmp( arg, "value5" ) )
      {
         int value;

         if( arg[5] == '0' )
            value = 0;
         else if( arg[5] == '1' )
            value = 1;
         else if( arg[5] == '2' )
            value = 2;
         else if( arg[5] == '3' )
            value = 3;
         else if( arg[5] == '4' )
            value = 4;
         else if( arg[5] == '5' )
            value = 5;
         else
         {
            send_to_char( "No clue what went wrong here but better safe than sorry.\r\n", ch );
            return;
         }

         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            ch_printf( ch, "Usage: grub obj value%d <#> [<#>]\r\n", value );
            return;
         }
         minamount = atoi( arg );
         if( minamount < 0 )
         {
            send_to_char( "Use a value equal to or higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_obj( ch, ( 6 + value ), minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No objects found with value[%d] between %d and %d.\r\n", value, minamount, maxamount );
            else
               ch_printf( ch, "No objects found with value[%d] set to %d.\r\n", minamount, value );
         }
         return;
      }
      if( !str_cmp( arg, "count" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub obj count <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount < 0 )
         {
            send_to_char( "Use a value equal to or higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_obj( ch, 12, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No objects found with a count between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No objects found with a count of %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "class" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub obj class <class> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_pc_class( arg );
            if( minamount < 0 || minamount >= MAX_PC_CLASS )
               ch_printf( ch, "Unknown class: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No classes would be checked.\r\n", ch );
            return;
         }
         if( !grub_obj( ch, 13, -1, -1, iflags ) )
            send_to_char( "No objects found with those classes restricted.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "race" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub obj race <race> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_pc_race( arg );
            if( minamount < 0 || minamount >= MAX_PC_RACE )
               ch_printf( ch, "Unknown race: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No races would be checked.\r\n", ch );
            return;
         }
         if( !grub_obj( ch, 14, -1, -1, iflags ) )
            send_to_char( "No objects found with those races restricted.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "layers" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub obj layers <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount < 0 )
         {
            send_to_char( "Use a value equal to or higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_obj( ch, 15, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No objects found with layers between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No objects found with layers set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "wear" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub obj wear <flag> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, w_flags, ITEM_WEAR_MAX );
            if( minamount < 0 || minamount >= ITEM_WEAR_MAX )
               ch_printf( ch, "Unknown object wear flag: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No wear flags would be checked.\r\n", ch );
            return;
         }
         if( !grub_obj( ch, 16, -1, -1, iflags ) )
            send_to_char( "No objects found with those wear flags set.\r\n", ch );
         return;
      }
      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( !str_cmp( arg, stattypes[stat] ) )
         {
            argument = one_argument( argument, arg );
            if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
            {
               ch_printf( ch, "Usage: grub obj %s <#> [<#>]\r\n", stattypes[stat] );
               return;
            }
            minamount = atoi( arg );
            if( minamount < 0 )
            {
               send_to_char( "Use a value equal to or higher than 0.\r\n", ch );
               return;
            }
            if( is_number( argument ) )
            {
               maxamount = atoi( argument );
               if( maxamount < minamount )
               {
                  send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
                  return;
               }
            }
            if( !grub_obj( ch, ( 17 + stat ), minamount, maxamount, iflags ) )
            {
               if( maxamount != -1 )
                  ch_printf( ch, "No objects found with %s between %d and %d.\r\n", stattypes[stat], minamount, maxamount );
               else
                  ch_printf( ch, "No objects found with %s set to %d.\r\n", stattypes[stat], minamount );
            }
            return;
         }
      }
      send_to_char( "Usage: grub obj flags <flag> [<...>]\r\n", ch );
      send_to_char( "Usage: grub obj weight <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub obj type <type>/<#>\r\n", ch );
      send_to_char( "Usage: grub obj level <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub obj cost <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub obj value<0-5> <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub obj count <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub obj class <class> [<...>]\r\n", ch );
      send_to_char( "Usage: grub obj race <race> [<...>]\r\n", ch );
      send_to_char( "Usage: grub obj layers <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub obj wear <flag> [<...>]\r\n", ch );
      send_to_char( "Usage: grub obj <stat> <#> [<#>]\r\n", ch );
   }
   else if( mobchck )
   {
      if( !str_cmp( arg, "level" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob level <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 1, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with level between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with level set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "level" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob killed <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 2, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with killed between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with killed set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "count" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob count <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 3, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with count between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with count set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "damroll" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob damroll <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 4, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with damroll between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with damroll set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "hitroll" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob hitroll <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 5, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with hitroll between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with hitroll set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "height" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob height <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 6, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with height between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with height set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "weight" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob weight <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 7, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with weight between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with weight set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "minhit" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob minhit <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 8, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with minhit between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with minhit set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "maxhit" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob maxhit <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 9, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with maxhit between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with maxhit set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "savingspellstaff" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob savingspellstaff <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 10, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with saving_spell_staff between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with saving_spell_staff set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "savingbreath" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob savingbreath <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 11, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with savingbreath between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with savingbreath set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "savingparapetri" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob savingparapetri <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 12, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with saving_para_petri between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with saving_para_petri set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "savingwand" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob savingwand <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 13, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with saving_wand between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with saving_wand set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "savingpoisondeath" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob savingpoisondeath <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 14, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with saving_poison_death between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with saving_poison_death set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "numattacks" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob numattacks <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 15, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with numattacks between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with numattacks set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "ac" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob ac <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 16, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with ac between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with ac set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "alignment" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob alignment <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount < -1000 || minamount > 1000 )
         {
            send_to_char( "Use a value between -1000 and 1000.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
            if( maxamount > 1000 )
            {
               send_to_char( "Proper range is between -1000 and 1000.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 17, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with alignment between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with alignment set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "gold" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob gold <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 18, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with gold between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with gold set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "bgold" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub mob bgold <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_mob( ch, 19, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No mobiles found with bgold between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No mobiles found with bgold set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "flags" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob flags <flag> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, act_flags, ACT_MAX );
            if( minamount < 0 || minamount >= ACT_MAX )
               ch_printf( ch, "Unknown mobile flag: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No flags would be checked.\r\n", ch );
            return;
         }
         if( !grub_mob( ch, 20, -1, -1, iflags ) )
            send_to_char( "No mobiles found with those flags set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "sex" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob sex <sex>\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         minamount = get_flag( arg, sex_names, SEX_MAX );
         if( minamount < 0 || minamount >= SEX_MAX )
            ch_printf( ch, "Unknown mobile sex: %s\r\n", arg );
         else if( !grub_mob( ch, 21, minamount, -1, iflags ) )
            send_to_char( "No mobiles found with that sex.\r\n", ch );
         return;
      }
      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( !str_cmp( arg, stattypes[stat] ) )
         {
            argument = one_argument( argument, arg );
            if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
            {
               ch_printf( ch, "Usage: grub mob %s <#> [<#>]\r\n", stattypes[stat] );
               return;
            }
            minamount = atoi( arg );
            if( minamount < 0 )
            {
               send_to_char( "Use a value equal to or higher than 0.\r\n", ch );
               return;
            }
            if( is_number( argument ) )
            {
               maxamount = atoi( argument );
               if( maxamount < minamount )
               {
                  send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
                  return;
               }
            }
            if( !grub_mob( ch, ( 22 + stat ), minamount, maxamount, iflags ) )
            {
               if( maxamount != -1 )
                  ch_printf( ch, "No mobiles found with %s between %d and %d.\r\n", stattypes[stat], minamount, maxamount );
               else
                  ch_printf( ch, "No mobiles found with %s set to %d.\r\n", stattypes[stat], minamount );
            }
            return;
         }
      }
      if( !str_cmp( arg, "affected" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob affected <aff> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, a_flags, AFF_MAX );
            if( minamount < 0 || minamount >= AFF_MAX )
               ch_printf( ch, "Unknown mobile affect: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No flags would be checked.\r\n", ch );
            return;
         }
         if( !grub_mob( ch, 29, -1, -1, iflags ) )
            send_to_char( "No mobiles found with those affects set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "resistant" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob resistant <ris> <#>\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         minamount = get_flag( arg, ris_flags, RIS_MAX );
         if( minamount < 0 || minamount >= RIS_MAX )
         {
            ch_printf( ch, "Unknown mobile resistant: %s\r\n", arg );
            return;
         }

         maxamount = atoi( argument );
         if( !grub_mob( ch, 30, minamount, maxamount, iflags ) )
            send_to_char( "No mobiles found with those resistants set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "parts" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob parts <part> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, part_flags, PART_MAX );
            if( minamount < 0 || minamount >= PART_MAX )
               ch_printf( ch, "Unknown mobile part: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No parts would be checked.\r\n", ch );
            return;
         }
         if( !grub_mob( ch, 33, -1, -1, iflags ) )
            send_to_char( "No mobiles found with those parts set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "attacks" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob attacks <attack> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, attack_flags, ATCK_MAX );
            if( minamount < 0 || minamount >= ATCK_MAX )
               ch_printf( ch, "Unknown mobile attack: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No attackss would be checked.\r\n", ch );
            return;
         }
         if( !grub_mob( ch, 34, -1, -1, iflags ) )
            send_to_char( "No mobiles found with those attacks set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "defenses" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob defenses <defense> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, defense_flags, DFND_MAX );
            if( minamount < 0 || minamount >= DFND_MAX )
               ch_printf( ch, "Unknown mobile defense: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No defenses would be checked.\r\n", ch );
            return;
         }
         if( !grub_mob( ch, 35, -1, -1, iflags ) )
            send_to_char( "No mobiles found with those defenses set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "position" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob position <pos>\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         minamount = get_flag( arg, pos_names, POS_MAX );
         if( minamount < 0 || minamount >= POS_MAX )
            ch_printf( ch, "Unknown mobile position: %s\r\n", arg );
         else if( !grub_mob( ch, 37, minamount, -1, iflags ) )
            send_to_char( "No mobiles found with that position.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "defposition" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub mob defposition <pos>\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         minamount = get_flag( arg, pos_names, POS_MAX );
         if( minamount < 0 || minamount >= POS_MAX )
            ch_printf( ch, "Unknown mobile defposition: %s\r\n", arg );
         else if( !grub_mob( ch, 38, minamount, -1, iflags ) )
            send_to_char( "No mobiles found with that defposition.\r\n", ch );
         return;
      }
      send_to_char( "Usage: grub mob flags <flag> [<...>]\r\n", ch );
      send_to_char( "Usage: grub mob sex <sex>\r\n", ch );
      send_to_char( "Usage: grub mob <stat> <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub mob affected <aff> [<...>]\r\n", ch );
      send_to_char( "Usage: grub mob resistant/immune/susceptible/absorb <ris> [<...>]\r\n", ch );
      send_to_char( "Usage: grub mob parts <part> [<...>]\r\n", ch );
      send_to_char( "Usage: grub mob attacks <attack> [<...>]\r\n", ch );
      send_to_char( "Usage: grub mob defenses <defense> [<...>]\r\n", ch );
      send_to_char( "Usage: grub mob position/defposition <pos>\r\n", ch );
      send_to_char( "Usage: grub mob <check> <#> [<#>]\r\n", ch );
      send_to_char( "Checks:\r\n", ch );
      send_to_char( "     ac   bgold   minhit     alignment    savingparapetri\r\n", ch );
      send_to_char( "   gold  height   weight    numattacks   savingspellstaff\r\n", ch );
      send_to_char( "  count  killed  damroll    savingwand  savingpoisondeath\r\n", ch );
      send_to_char( "  level  maxhit  hitroll  savingbreath\r\n", ch );
   }
   else if( roomchck )
   {
      if( !str_cmp( arg, "tunnel" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub room tunnel <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_room( ch, 1, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No rooms found with tunnel between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No rooms found with tunnel set to %d.\r\n", minamount );
         }
         return;
      }
      if( !str_cmp( arg, "sector" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' )
         {
            send_to_char( "Usage: grub room sector <sector>/<#>\r\n", ch );
            return;
         }
         minamount = get_flag( arg, sect_flags, SECT_MAX );
         if( minamount < 0 && is_number( arg ) )
            minamount = atoi( arg );
         if( minamount < 0 || minamount >= SECT_MAX )
            ch_printf( ch, "No such sector to look for (%s).\r\n", arg );
         else if( !grub_room( ch, 2, minamount, -1, iflags ) )
            ch_printf( ch, "No rooms found with sector set to %s.\r\n", sect_flags[minamount] );
         return;
      }
      if( !str_cmp( arg, "televnum" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' )
         {
            if( !grub_room( ch, 3, -1, -1, iflags ) )
               send_to_char( "No rooms found with a set televnum.\r\n", ch );
            return;
         }
         if( !is_number( arg ) )
         {
            send_to_char( "Usage: grub room televnum [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a number above 0.\r\n", ch );
            return;
         }
         if( !grub_room( ch, 3, minamount, -1, iflags ) )
            ch_printf( ch, "No rooms found with televnum set to %d.\r\n", minamount );
         return;
      }
      if( !str_cmp( arg, "flags" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "Usage: grub room flags <flag> [<...>]\r\n", ch );
            return;
         }
         while( argument && argument[0] != '\0' )
         {
            argument = one_argument( argument, arg );

            minamount = get_flag( arg, r_flags, ROOM_MAX );
            if( minamount < 0 || minamount >= ROOM_MAX )
               ch_printf( ch, "Unknown room flag: %s\r\n", arg );
            else
               xTOGGLE_BIT( iflags, minamount );
         }
         if( xIS_EMPTY( iflags ) )
         {
            send_to_char( "No flags would be checked.\r\n", ch );
            return;
         }
         if( !grub_room( ch, 4, -1, -1, iflags ) )
            send_to_char( "No rooms found with those flags set.\r\n", ch );
         return;
      }
      if( !str_cmp( arg, "light" ) )
      {
         argument = one_argument( argument, arg );
         if( arg == NULL || arg[0] == '\0' || !is_number( arg ) )
         {
            send_to_char( "Usage: grub room light <#> [<#>]\r\n", ch );
            return;
         }
         minamount = atoi( arg );
         if( minamount <= 0 )
         {
            send_to_char( "Use a value higher than 0.\r\n", ch );
            return;
         }
         if( is_number( argument ) )
         {
            maxamount = atoi( argument );
            if( maxamount < minamount )
            {
               send_to_char( "Use a max amount higher than the minamount.\r\n", ch );
               return;
            }
         }
         if( !grub_room( ch, 5, minamount, maxamount, iflags ) )
         {
            if( maxamount != -1 )
               ch_printf( ch, "No rooms found with light between %d and %d.\r\n", minamount, maxamount );
            else
               ch_printf( ch, "No rooms found with light set to %d.\r\n", minamount );
         }
         return;
      }
      send_to_char( "Usage: grub room tunnel <#> [<#>]\r\n", ch );
      send_to_char( "Usage: grub room sector <sector>/<#>\r\n", ch );
      send_to_char( "Usage: grub room televnum [<#>]\r\n", ch );
      send_to_char( "Usage: grub room flags <flag> [<...>]\r\n", ch );
      send_to_char( "Usage: grub room light <#> [<#>]\r\n", ch );
   }
}
