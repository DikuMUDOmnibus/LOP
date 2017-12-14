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
#include "h/mud.h"

int MAX_PC_RACE;

RACE_TYPE *race_table[MAX_RACE];

void write_race_list( void )
{
   FILE *fp;
   int i;

   if( !( fp = fopen( RACE_LIST, "w" ) ) )
   {
      bug( "%s: Error opening %s.", __FUNCTION__, RACE_LIST );
      perror( RACE_LIST );
      return;
   }
   for( i = 0; i < MAX_PC_RACE && race_table[i]; i++ )
      if( race_table[i] && race_table[i]->name )
         fprintf( fp, "%s.race\n", race_table[i]->name );
   fprintf( fp, "%s", "$\n" );
   fclose( fp );
   fp = NULL;
}

bool create_new_race( int rcindex, char *argument )
{
   int stat, i;

   if( !argument || argument[0] == '\0' )
      return false;
   if( rcindex >= MAX_RACE || race_table[rcindex] )
      return false;
   CREATE( race_table[rcindex], RACE_TYPE, 1 );
   if( !race_table[rcindex] )
      return false;
   argument[0] = UPPER( argument[0] );
   race_table[rcindex]->used = 0;
   race_table[rcindex]->rlistplace = rcindex;
   race_table[rcindex]->name = STRALLOC( argument );
   race_table[rcindex]->hit = 100;
   race_table[rcindex]->mana = 100;
   race_table[rcindex]->move = 100;
   race_table[rcindex]->uses = 1; /* Set it to use Mana */
   race_table[rcindex]->alignment = 0;
   race_table[rcindex]->minalign = -1000;
   race_table[rcindex]->maxalign = 1000;
   race_table[rcindex]->ac_plus = 0;
   race_table[rcindex]->minheight = number_range( 50, 75 );
   race_table[rcindex]->maxheight = number_range( 75, 100 );
   race_table[rcindex]->minweight = number_range( 150, 200 );
   race_table[rcindex]->maxweight = number_range( 200, 250 );
   race_table[rcindex]->hunger_mod = 0;
   race_table[rcindex]->thirst_mod = 0;
   race_table[rcindex]->race_recall = 0;
   for( stat = 0; stat < STAT_MAX; stat++ )
      race_table[rcindex]->base_stats[stat] = 13;
   for( stat = 0; stat < STAT_MAX; stat++ )
      race_table[rcindex]->max_stats[stat] = MAX_LEVEL;
   for( i = 0; i < MAX_WHERE_NAME; ++i )
   {
      race_table[rcindex]->where_name[i] = NULL;
      race_table[rcindex]->lodge_name[i] = NULL;
   }
   xCLEAR_BITS( race_table[rcindex]->class_restriction );
   xCLEAR_BITS( race_table[rcindex]->where_restrict );
   xCLEAR_BITS( race_table[rcindex]->language );
   xCLEAR_BITS( race_table[rcindex]->affected );

   for( stat = 0; stat < RIS_MAX; stat++ )
      race_table[rcindex]->resistant[stat] = 0;

   return true;
}

void write_race_file( int ra )
{
   FILE *fp;
   char filename[MIL];
   RACE_TYPE *race = race_table[ra];
   int i, x, y, stat;

   if( !race )
   {
      bug( "%s: NULL race in race_table[%d].", __FUNCTION__, ra );
      return;
   }

   if( !race->name )
   {
      bug( "%s: Race has NULL name in race_table[%d].", __FUNCTION__, ra );
      return;
   }

   snprintf( filename, sizeof( filename ), "%s%s.race", RACE_DIR, race->name );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: Can't open: %s for writing", __FUNCTION__, filename );
      perror( filename );
      return;
   }

   fprintf( fp, "Name        %s~\n", race->name );
   fprintf( fp, "Used        %d\n", race->used );
   if( !xIS_EMPTY( race->class_restriction ) )
      fprintf( fp, "Classes     %s~\n", ext_class_string( &race->class_restriction ) );
   if( !xIS_EMPTY( race->where_restrict ) )
      fprintf( fp, "WhRestrict  %s~\n", ext_flag_string( &race->where_restrict, wear_locs ) );
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( race->base_stats[stat] == 13 && race->max_stats[stat] == MAX_LEVEL )
         continue;
      fprintf( fp, "NStat       %d %d %s~\n", race->base_stats[stat], race->max_stats[stat], stattypes[stat] );
   }
   fprintf( fp, "Hit         %d\n", race->hit );
   fprintf( fp, "Mana        %d\n", race->mana );
   fprintf( fp, "Move        %d\n", race->move );
   fprintf( fp, "Uses        %d\n", race->uses );
   if( !xIS_EMPTY( race->affected ) )
      fprintf( fp, "Affected    %s~\n", ext_flag_string( &race->affected, a_flags ) );

   for( stat = 0; stat < RIS_MAX; stat++ )
      if( race->resistant[stat] != 0 )
         fprintf( fp, "Resistant   %d %s~\n", race->resistant[stat], ris_flags[stat] );

   if( !xIS_EMPTY( race->language ) )
      fprintf( fp, "SLanguage   %s~\n", ext_flag_string( &race->language, lang_names ) );
   fprintf( fp, "Align       %d\n", race->alignment );
   fprintf( fp, "Min_Align   %d\n", race->minalign );
   fprintf( fp, "Max_Align   %d\n", race->maxalign );
   fprintf( fp, "AC_Plus     %d\n", race->ac_plus );
   fprintf( fp, "MinHeight   %d\n", race->minheight );
   fprintf( fp, "MaxHeight   %d\n", race->maxheight );
   fprintf( fp, "MinWeight   %d\n", race->minweight );
   fprintf( fp, "MaxWeight   %d\n", race->maxweight );
   fprintf( fp, "Hunger_Mod  %d\n", race->hunger_mod );
   fprintf( fp, "Thirst_mod  %d\n", race->thirst_mod );
   fprintf( fp, "Race_Recall %d\n", race->race_recall );
   for( i = 0; i < MAX_WHERE_NAME; i++ )
   {
      if( race->where_name[i] && str_cmp( race->where_name[i], where_name[i] ) )
         fprintf( fp, "WhereName  %d %s~\n", i, race->where_name[i] );
      if( race->lodge_name[i] && str_cmp( race->lodge_name[i], lodge_name[i] ) )
         fprintf( fp, "LodgeName  %d %s~\n", i, race->lodge_name[i] );
   }
   for( x = 0; x < top_sn; x++ )
   {
      if( !skill_table[x] || !skill_table[x]->name )
         continue;
      if( ( y = skill_table[x]->race_level[race->rlistplace] ) > 0 )
         fprintf( fp, "Skill '%s' %d %d\n", skill_table[x]->name, y, skill_table[x]->race_adept[race->rlistplace] );
   }
   fprintf( fp, "End\n" );
   fclose( fp );
   fp = NULL;
}

RACE_TYPE *valid_race( char *name )
{
   RACE_TYPE *race = NULL;
   int rcheck;

   if( name && name[0] != '\0' )
   {
      if( is_number( name ) )
      {
         if( ( rcheck = atoi( name ) ) >= 0 && rcheck < MAX_PC_RACE )
            race = race_table[rcheck];
      }
      else
      {
         for( rcheck = 0; rcheck < MAX_PC_RACE; rcheck++ )
         {
            if( !race_table[rcheck] || !race_table[rcheck]->name )
               continue;

            if( !str_cmp( race_table[rcheck]->name, name ) )
            {
               race = race_table[rcheck];
               break;
            }
         }
      }
   }
   return race;
}

void show_race( CHAR_DATA *ch, RACE_TYPE *race )
{
   int i, ct, stat, ra, cnt;
   bool first = true;

   set_pager_color( AT_PLAIN, ch );

   if( !race )
   {
      send_to_char( "No such race.\r\n", ch );
      return;
   }

   ra = race->rlistplace;
   ch_printf( ch, "&wRACE: &W%s\r\n", race->name ? race->name : "Not Set" );

   ch_printf( ch, "&wUsed: &W%d\r\n", race->used );

   ch_printf( ch, "&wDisallowed classes:\r\n&W%s\r\n", ext_class_string( &race->class_restriction ) );

   ch_printf( ch, "&wUnwearable Locations:\r\n&W%s\r\n", ext_flag_string( &race->where_restrict, wear_locs ) );
   ct = 0;
   send_to_char( "&wAllowed classes:&W\r\n", ch );
   for( i = 0; i < MAX_PC_CLASS; i++ )
   {
      if( !xIS_SET( race->class_restriction, i ) )
      {
         ch_printf( ch, "%s ", class_table[i]->name );
         if( ++ct == 6 )
         {
            send_to_char( "\r\n", ch );
            ct = 0;
         }
      }
   }
   if( ct != 0 )
      send_to_char( "\r\n", ch );

   ch_printf( ch, "&wUses: &W%s\r\n",
      race->uses == 0 ? "Nothing" : race->uses == 1 ? "Mana" : race->uses == 2 ? "Blood" : "Unknown" );
   send_to_char( "&wBase Stats:\r\n", ch );
   ct = 0;
   ch_printf( ch, "&c%14s: &w%4d &c%14s: &w%4d &c%14s: &w%4d\r\n&c%14s: &w%4d &c%14s: &w%4d ",
      "Hit Points", race->hit, race->uses == 2 ? "Blood" : "Mana", race->mana, "Move", race->move,
      "Alignment", race->alignment, "AC", race->ac_plus );
   ct = 2;
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
       ch_printf( ch, "&c%14s: &w%4d", capitalize( stattypes[stat] ), race->base_stats[stat] );
       if( ++ct == 3 )
       {
          send_to_char( "\r\n", ch );
          ct = 0;
       }
       else
          send_to_char( " ", ch );
   }
   if( ct != 0 )
      send_to_char( "\r\n", ch );

   send_to_char( "&wMax Stats: (To set use 'max <stat>')\r\n", ch );
   ct = 0;
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
       ch_printf( ch, "&c%14s: &w%4d", capitalize( stattypes[stat] ), race->max_stats[stat] );
       if( ++ct == 3 )
       {
          send_to_char( "\r\n", ch );
          ct = 0;
       }
       else
          send_to_char( " ", ch );
   }
   if( ct != 0 )
      send_to_char( "\r\n", ch );

   ch_printf( ch, "&wMin Align: &W%4d   &wMax Align: &W%4d\r\n",
      race->minalign, race->maxalign );
   ch_printf( ch, "&wHeight: &W%4d-%4d &win.   Weight: &W%4d-%4d &wlbs. HungerMod: &W%4d  &wThirstMod: &W%d\r\n",
      race->minheight, race->maxheight, race->minweight, race->maxweight, race->hunger_mod, race->thirst_mod );

   ch_printf( ch, "&wSpoken Languages: &W%s\r\n", ext_flag_string( &race->language, lang_names ) );
   ch_printf( ch, "&wAffected by:      &W%s\r\n", ext_flag_string( &race->affected, a_flags ) );

   for( stat = 0; stat < RIS_MAX; stat++ )
      if( race->resistant[stat] != 0 )
         ch_printf( ch, "&wResistant to:     &W%s %d\r\n", ris_flags[stat], race->resistant[stat] );

   {
      int x, y, low, hi, last = -1;

      low = 0;
      hi = MAX_LEVEL;
      set_pager_color( AT_BLUE, ch );
      for( x = low; x <= hi; x++ )
      {
         cnt = 0;
         for( y = gsn_first_spell; y < gsn_top_sn; y++ )
         {
            if( skill_table[y]->race_level[ra] == x )
            {
               if( first )
                  send_to_pager( "&wSpells/Skills/Weapons\r\n", ch );
               first = false;
               if( x != last )
               {
                  if( cnt != 0 )
                  {
                     send_to_pager( "\r\n", ch );
                     cnt = 0;
                  }
                  pager_printf( ch, "&w[Level &W%d&w]\r\n", x );
               }
               last = x;
               pager_printf( ch, "  &w(&W%3d&w)&D%-22s", skill_table[y]->race_adept[ra], skill_table[y]->name );
               if( ++cnt == 3 )
               {
                  send_to_pager( "\r\n", ch );
                  cnt = 0;
               }
            }
         }
         if( cnt != 0 )
            send_to_pager( "\r\n", ch );
      }
   }

   cnt = 0;
   first = true;
   for( i = 0; i < MAX_WHERE_NAME; ++i )
   {
      if( !race->where_name[i] )
         continue;
      if( first )
         send_to_pager( "Where names:\r\n", ch );
      first = false;
      pager_printf( ch, "%10s> %s\r\n", wear_locs[i], race->where_name[i] );
   }
   first = true;
   for( i = 0; i < MAX_WHERE_NAME; ++i )
   {
      if( !race->lodge_name[i] )
         continue;
      if( first )
         send_to_pager( "Lodge names:\r\n", ch );
      first = false;
      pager_printf( ch, "%10s> %s\r\n", wear_locs[i], race->lodge_name[i] );
   }
}

/* Modified by Samson to allow setting language by name - 8-6-98 */
CMDF( do_setrace )
{
   RACE_TYPE *race;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   int value, v2, i, stat, ra;

   set_char_color( AT_PLAIN, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: setrace <race> [create/save]\r\n", ch );
      send_to_char( "Usage: setrace <race> <field> <value>\r\n", ch );
      send_to_char( "Usage: setrace <race> wherename <location> <string>\r\n", ch );
      send_to_char( "Usage: setrace <race> wearrestrict <location>\r\n", ch );
      send_to_char( "Usage: setrace <race> skill <skill> <level> <adept>\r\n", ch );
      send_to_char( "\r\nField being one of:\r\n", ch );
      send_to_char( "    ac      uses     wisdom   maxalign  maxweight\r\n", ch );
      send_to_char( "   hit    hunger    classes   minalign  minheight\r\n", ch );
      send_to_char( "  luck    recall              strength  minweight\r\n", ch );
      send_to_char( "  mana    resist   affected  alignment  constitution\r\n", ch );
      send_to_char( "  move    thirst   charisma  dexterity  intelligence\r\n", ch );
      send_to_char( "  name  language  maxheight\r\n", ch );
      return;
   }

   race = valid_race( arg1 );

   if( !str_cmp( arg2, "create" ) && race )
   {
      send_to_char( "That race already exists!\r\n", ch );
      return;
   }
   else if( !race && str_cmp( arg2, "create" ) )
   {
      send_to_char( "No such race.\r\n", ch );
      return;
   }

   if( arg2 == NULL || arg2[0] == '\0' )
   {
      show_race( ch, race );
      return;
   }

   if( !str_cmp( arg2, "create" ) )
   {
      char filename[1024];

      if( MAX_PC_RACE >= MAX_RACE )
      {
         send_to_char( "You need to up MAX_RACE in mud.h and make clean.\r\n", ch );
         return;
      }
      snprintf( filename, sizeof( filename ), "%s.race", arg1 );
      if( !can_use_path( ch, RACE_DIR, filename ) )
         return;
      if( !create_new_race( MAX_PC_RACE, arg1 ) )
      {
         send_to_char( "Couldn't create a new race.\r\n", ch );
         return;
      }
      write_race_file( MAX_PC_RACE );
      MAX_PC_RACE++;
      write_race_list( );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   ra = race->rlistplace;

   if( !str_cmp( arg2, "save" ) )
   {
      write_race_file( ra );
      send_to_char( "Race saved.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "You must specify an argument.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "skill" ) )
   {
      SKILLTYPE *skill;
      int sn;

      argument = one_argument( argument, arg2 );
      if( ( sn = skill_lookup( arg2 ) ) >= 0 )
      {
         if( !( skill = get_skilltype( sn ) ) )
         {
            send_to_char( "Invalid skilltype???\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         skill->race_level[ra] = URANGE( -1, atoi( arg2 ), MAX_LEVEL );
         argument = one_argument( argument, arg2 );
         skill->race_adept[ra] = URANGE( 0, atoi( arg2 ), 100 );
         write_race_file( ra );
         ch_printf( ch, "Skill \"%s\" added at level %d and %d%%.\r\n",
            skill->name, skill->race_level[ra], skill->race_adept[ra] );
      }
      else
         ch_printf( ch, "No skill: %s.\r\n", arg2 );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      RACE_TYPE *rcheck;
      char buf[256];

      one_argument( argument, arg1 );
      if( arg1 == NULL || arg1[0] == '\0' )
      {
         send_to_char( "You can't set a race name to nothing.\r\n", ch );
         return;
      }
      snprintf( buf, sizeof( buf ), "%s.race", arg1 );
      if( !can_use_path( ch, RACE_DIR, buf ) )
         return;
      if( ( rcheck = valid_race( arg1 ) ) )
      {
         ch_printf( ch, "Already a race called %s.\r\n", arg1 );
         return;
      }
      snprintf( buf, sizeof( buf ), "%s%s.race", RACE_DIR, race->name );
      remove_file( buf );
      arg1[0] = UPPER( arg1[0] );
      STRSET( race->name, arg1 );
      write_race_file( ra );
      write_race_list( );
      ch_printf( ch, "Race name set to %s.\r\n", race->name );
      return;
   }

   if( !str_cmp( arg2, "wearrestrict" ) )
   {
      bool fset = false;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> wearrestrict <location>\r\n", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, wear_locs, WEAR_MAX );
         if( value < 0 || value >= WEAR_MAX )
            ch_printf( ch, "Unknown wearlocation (%s), see Help WEARLOCS.\r\n", arg3 );
         else
         {
            xTOGGLE_BIT( race->where_restrict, value );
            fset = true;
         }
      }
      if( !fset )
         return;
      write_race_file( ra );
      send_to_char( "Race wearrestrict set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "wherename" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> wherename <location> <string>\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      value = get_flag( arg3, wear_locs, WEAR_MAX );
      if( value < 0 || value >= WEAR_MAX )
      {
         ch_printf( ch, "Unknown wearlocation (%s), see Help WEARLOCS.\r\n", arg3 );
         return;
      }
      STRSET( race->where_name[value], argument );
      write_race_file( ra );
      send_to_char( "Race wherename set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "lodgename" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> lodgename <location> <string>\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg3 );
      value = get_flag( arg3, wear_locs, WEAR_MAX );
      if( value < 0 || value >= WEAR_MAX )
      {
         ch_printf( ch, "Unknown wearlocation (%s), see Help WEARLOCS.\r\n", arg3 );
         return;
      }
      STRSET( race->lodge_name[value], argument );
      write_race_file( ra );
      send_to_char( "Race lodgename set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      bool fset = false;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> affected <flag> [flag]...\r\n", ch );
         return;
      }
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_flag( arg3, a_flags, AFF_MAX );
         if( value < 0 || value >= AFF_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg3 );
         else
         {
            xTOGGLE_BIT( race->affected, value );
            fset = true;
         }
      }
      if( !fset )
         return;
      write_race_file( ra );
      send_to_char( "Racial affects set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "resist" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch, "Usage: setrace <race> %s <flag> [flag]...\r\n", arg2 );
         return;
      }

      argument = one_argument( argument, arg3 );
      value = get_flag( arg3, ris_flags, RIS_MAX );
      if( value < 0 || value >= RIS_MAX )
      {
         ch_printf( ch, "Unknown %s: %s\r\n", arg2, arg3 );
         return;
      }
      else
         race->resistant[value] = atoi( argument );
      write_race_file( ra );
      ch_printf( ch, "Racial resistant %s set to %d.\r\n", ris_flags[value], race->resistant[value] );
      return;
   }

   if( !str_cmp( arg2, "language" ) )
   {
      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_langflag( arg3 );
         v2 = get_langnum( arg3 );

         if( v2 == -1 || value == LANG_UNKNOWN )
         {
            ch_printf( ch, "Unknown language: %s\r\n", arg3 );
            continue;
         }
         else
         {
            if( !( value &= VALID_LANGS ) )
            {
               ch_printf( ch, "Player races may not speak %s.\r\n", arg3 );
               continue;
            }
         }
         xTOGGLE_BIT( race->language, v2 );
      }
      write_race_file( ra );
      send_to_char( "Racial language set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "classes" ) )
   {
      bool modified = false;

      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg1 );
         for( i = 0; i < MAX_PC_CLASS && class_table[i]; i++ )
         {
            if( !class_table[i] || !class_table[i]->name )
               continue;
            if( !str_cmp( arg1, class_table[i]->name ) )
            {
               xTOGGLE_BIT( race->class_restriction, i );
               modified = true;
            }
         }
      }
      if( !modified )
         send_to_char( "No such class.\r\n", ch );
      else
      {
         write_race_file( ra );
         send_to_char( "Race Classes set.\r\n", ch );
      }
      return;
   }

   /* Check stats for matches */
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      if( !str_cmp( arg2, stattypes[stat] ) )
      {
         race->base_stats[stat] = ( short )atoi( argument );
         write_race_file( ra );
         ch_printf( ch, "%s set to %d\r\n", capitalize( stattypes[stat] ), atoi( argument ) );
         return;
      }
   }

   if( !str_cmp( arg2, "max" ) )
   {
      argument = one_argument( argument, arg2 );

      /* Check stats for matches */
      for( stat = 0; stat < STAT_MAX; stat++ )
      {
         if( !str_cmp( arg2, stattypes[stat] ) )
         {
            race->max_stats[stat] = ( short )URANGE( 0, atoi( argument ), MAX_LEVEL );
            write_race_file( ra );
            ch_printf( ch, "Max %s set to %d\r\n", capitalize( stattypes[stat] ), atoi( argument ) );
            return;
         }
      }
      send_to_char( "No such max stat to set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "hit" ) )
      race->hit = ( short )atoi( argument );
   else if( !str_cmp( arg2, "mana" ) )
      race->mana = ( short )atoi( argument );
   else if( !str_cmp( arg2, "move" ) )
      race->move = ( short )atoi( argument );
   else if( !str_cmp( arg2, "uses" ) )
   {
      if( is_number( argument ) )
      {
         int tval = atoi( argument );
         if( tval < 0 || tval > 2 )
         {
            send_to_char( "You may only set uses to 0 = Nothing, 1 = Mana, 2 = Blood.\r\n", ch );
            return;
         }
         race->uses = tval;
      }
      else if( !str_cmp( argument, "Mana" ) )
         race->uses = 1;
      else if( !str_cmp( argument, "Blood" ) )
         race->uses = 2;
      else if( !str_cmp( argument, "Nothing" ) )
         race->uses = 0;
      else
      {
         send_to_char( "You may only set uses to 0 = Nothing, 1 = Mana, 2 = Blood.\r\n", ch );
         return;
      }
      write_race_file( ra );
      ch_printf( ch, "Uses set to %s\r\n",
         race->uses == 0 ? "Nothing" : race->uses == 1 ? "Mana" : race->uses == 2 ? "Blood" : "Unknown" );
      return;
   }
   else if( !str_cmp( arg2, "ac" ) )
      race->ac_plus = atoi( argument );
   else if( !str_cmp( arg2, "alignment" ) )
      race->alignment = atoi( argument );
   else if( !str_cmp( arg2, "minalign" ) )
      race->minalign = atoi( argument );
   else if( !str_cmp( arg2, "maxalign" ) )
      race->maxalign = atoi( argument );
   else if( !str_cmp( arg2, "minheight" ) )
      race->minheight = atoi( argument );
   else if( !str_cmp( arg2, "maxheight" ) )
      race->maxheight = atoi( argument );
   else if( !str_cmp( arg2, "minweight" ) )
      race->minweight = atoi( argument );
   else if( !str_cmp( arg2, "maxweight" ) )
      race->maxweight = atoi( argument );
   else if( !str_cmp( arg2, "thirst" ) )
      race->thirst_mod = atoi( argument );
   else if( !str_cmp( arg2, "hunger" ) )
      race->hunger_mod = atoi( argument );
   else if( !str_cmp( arg2, "recall" ) )
      race->race_recall = atoi( argument );
   else
   {
      do_setrace( ch, (char *)"" );
      return;
   }
   write_race_file( ra );
   ch_printf( ch, "%s changed and race saved.\r\n", arg2 );
}

CMDF( do_races )
{
   RACE_TYPE *Race;
   int ra, count;

   for( ra = 0, count = ( is_immortal( ch ) ? 0 : 1 ); ra < MAX_PC_RACE; ra++, count++ )
   {
      if( !( Race = race_table[ra] ) || !Race->name )
         continue;
      pager_printf( ch, "&w[&W%2d&w] &w(&W%3d&w) &W%s&D\r\n", count, Race->used, Race->name );
   }
}

bool load_race_file( const char *fname )
{
   RACE_TYPE *race;
   FILE *fp;
   const char *word;
   char *infoflags, flag[MIL], filename[MIL];
   int i, wear = 0, value, ra = -1, stat;
   bool fMatch;

   snprintf( filename, sizeof( filename ), "%s%s", RACE_DIR, fname );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      perror( filename );
      return false;
   }

   CREATE( race, RACE_TYPE, 1 );
   race->uses = 0;
   for( i = 0; i < MAX_WHERE_NAME; ++i )
   {
      race->where_name[i] = NULL;
      race->lodge_name[i] = NULL;
   }
   for( stat = 0; stat < STAT_MAX; stat++ )
   {
      race->base_stats[stat] = 13;
      race->max_stats[stat] = MAX_LEVEL;
   }

   for( ra = 0; ra < MAX_RACE; ra++ )
   {
      if( !race_table[ra] )
         break;
   }
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
            KEY( "Align", race->alignment, fread_number( fp ) );
            KEY( "AC_Plus", race->ac_plus, fread_number( fp ) );
            WEXTKEY( "Affected", race->affected, fp, a_flags, AFF_MAX );
            break;

         case 'C':
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
                        xSET_BIT( race->class_restriction, iclass );
                        break;
                     }
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               fclose( fp );
               fp = NULL;
               if( ra < 0 || ra >= MAX_RACE )
               {
                  bug( "%s: Race (%s) bad/not found (%d)", __FUNCTION__, race->name ? race->name : "name not found", ra );
                  STRFREE( race->name );
                  for( i = 0; i < MAX_WHERE_NAME; ++i )
                  {
                     STRFREE( race->where_name[i] );
                     STRFREE( race->lodge_name[i] );
                  }
                  DISPOSE( race );
                  return false;
               }
               race_table[ra] = race;
               race_table[ra]->rlistplace = ra;
               return true;
            }
            break;

         case 'F':
            if( !str_cmp( word, "FMana" ) )
            {
               int iclass = fread_number( fp );
               if( iclass )
                  race->uses = 1;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "FBlood" ) )
            {
               int iclass = fread_number( fp );
               if( iclass )
                  race->uses = 2;
               fMatch = true;
               break;
            }
            break;

         case 'H':
            KEY( "Hit", race->hit, fread_number( fp ) );
            KEY( "Hunger_Mod", race->hunger_mod, fread_number( fp ) );
            break;

         case 'L':
            if( !str_cmp( word, "Language" ) ) /* Handle loading old ones and just don't bother with them */
            {
               fread_number( fp );
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "LodgeName" ) )
            {
               wear = fread_number( fp );

               if( wear < MAX_WHERE_NAME )
               {
                  STRFREE( race->lodge_name[wear] );
                  race->lodge_name[wear] = fread_string( fp );
               }
               else
               {
                  bug( "%s: Too many lodge_names", __FUNCTION__ );
                  fread_flagstring( fp );
               }
               fMatch = true;
               break;
            }
            break;

         case 'M':
            KEY( "MinHeight", race->minheight, fread_number( fp ) );
            KEY( "MaxHeight", race->maxheight, fread_number( fp ) );
            KEY( "MinWeight", race->minweight, fread_number( fp ) );
            KEY( "MaxWeight", race->maxweight, fread_number( fp ) );
            KEY( "Mana", race->mana, fread_number( fp ) );
            KEY( "Move", race->move, fread_number( fp ) );
            KEY( "Min_Align", race->minalign, fread_number( fp ) );
            KEY( "Max_Align", race->maxalign, fread_number( fp ) );
            if( !str_cmp( word, "MaxStats" ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               race->max_stats[STAT_STR] = fread_number( fp );
               race->max_stats[STAT_INT] = fread_number( fp );
               race->max_stats[STAT_WIS] = fread_number( fp );
               race->max_stats[STAT_DEX] = fread_number( fp );
               race->max_stats[STAT_CON] = fread_number( fp );
               race->max_stats[STAT_CHA] = fread_number( fp );
               race->max_stats[STAT_LCK] = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'N':
            if( !str_cmp( word, "NStat" ) )
            {
               int ustat, umstat;

               stat = fread_number( fp );
               umstat = fread_number( fp );
               infoflags = fread_flagstring( fp );
               ustat = get_flag( infoflags, stattypes, STAT_MAX );
               if( ustat < 0 || ustat >= STAT_MAX )
                  bug( "%s: unknown stat [%s].", __FUNCTION__, infoflags );
               else
               {
                  race->base_stats[ustat] = stat;
                  race->max_stats[ustat] = umstat;
               }
               fMatch = true;
               break;
            }
            KEY( "Name", race->name, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Race_Recall", race->race_recall, fread_number( fp ) );
            if( !str_cmp( word, "Resistant" ) )
            {
               int tmpvalue = fread_number( fp );
               infoflags = fread_flagstring( fp );
               value = get_flag( infoflags, ris_flags, RIS_MAX );
               if( value < 0 || value >= RIS_MAX )
                  bug( "%s: Unknown %s: %s", __FUNCTION__, word, infoflags );
               else
                  race->resistant[value] = tmpvalue;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Resist" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            break;

         case 'S':
            WEXTKEY( "SLanguage", race->language, fp, lang_names, LANG_UNKNOWN );
            if( !str_cmp( word, "Stats" ) )
            {
               /* These are the default ones so go ahead and convert them incase someone changes one day */
               race->base_stats[STAT_STR] = fread_number( fp );
               race->base_stats[STAT_INT] = fread_number( fp );
               race->base_stats[STAT_WIS] = fread_number( fp );
               race->base_stats[STAT_DEX] = fread_number( fp );
               race->base_stats[STAT_CON] = fread_number( fp );
               race->base_stats[STAT_CHA] = fread_number( fp );
               race->base_stats[STAT_LCK] = fread_number( fp );
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Suscept" ) )
            {
               fread_flagstring( fp );
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Skill" ) )
            {
               int sn, lev, adp;

               word = fread_word( fp );
               lev = fread_number( fp );
               adp = fread_number( fp );
               sn = skill_lookup( word );
               if( !is_valid_sn( sn ) )
                  bug( "%s: Skill %s unknown", __FUNCTION__, word );
               else
               {
                  if( ra >= 0 && ra < MAX_RACE )
                  {
                     skill_table[sn]->race_level[ra] = lev;
                     skill_table[sn]->race_adept[ra] = adp;
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'T':
            KEY( "Thirst_Mod", race->thirst_mod, fread_number( fp ) );
            break;

         case 'U':
            KEY( "Uses", race->uses, fread_number( fp ) );
            KEY( "Used", race->used, fread_number( fp ) );
            break;

         case 'W':
            WEXTKEY( "WhRestrict", race->where_restrict, fp, wear_locs, WEAR_MAX );
            if( !str_cmp( word, "WhereName" ) )
            {
               wear = fread_number( fp );

               if( wear < MAX_WHERE_NAME )
               {
                  STRFREE( race->where_name[wear] );
                  race->where_name[wear] = fread_string( fp );
               }
               else
               {
                  bug( "%s: Too many where_names", __FUNCTION__ );
                  fread_flagstring( fp );
               }
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

/* Load in all the race files. */
void load_races( void )
{
   FILE *fp;
   const char *filename;
   int i;

   MAX_PC_RACE = 0;

   for( i = 0; i < MAX_RACE; i++ )
      race_table[i] = NULL;

   if( !( fp = fopen( RACE_LIST, "r" ) ) )
   {
      perror( RACE_LIST );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fp ) ? "$" : fread_word( fp );
      if( filename[0] == '$' || filename[0] == EOF )
         break;

      if( !load_race_file( filename ) )
         bug( "%s: Can't load race file: %s", __FUNCTION__, filename );
      else
         MAX_PC_RACE++;
   }
   fclose( fp );
   fp = NULL;
}

void save_races( void )
{
   int x;

   for( x = 0; x < MAX_PC_RACE; x++ )
      if( race_table[x] && race_table[x]->name )
         write_race_file( x );
}
