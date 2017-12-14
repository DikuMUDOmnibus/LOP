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

#define SYSDATA_FILE SYSTEM_DIR "sysdata.dat"

/* Save system info to data file */
void save_sysdata( bool autosave )
{
   FILE *fp;

   /* If autosave and not set to allow autosave just return */
   if( autosave && !sysdata.autosavecset )
      return;

   if( !( fp = fopen( SYSDATA_FILE, "w" ) ) )
   {
      bug( "%s: couldn't open %s for writing", __FUNCTION__, SYSDATA_FILE );
      perror( SYSDATA_FILE );
      return;
   }

   fprintf( fp, "Version           %d %d\n", sysdata.version_major, sysdata.version_minor );
   if( sysdata.mud_name )
      fprintf( fp, "MudName           %s~\n", sysdata.mud_name );
   if( !xIS_EMPTY( sysdata.save_flags ) )
      fprintf( fp, "Saveflags         %s~\n", ext_flag_string( &sysdata.save_flags, save_flag ) );
   if( sysdata.time_of_max )
      fprintf( fp, "Highplayertime    %ld\n", sysdata.time_of_max );
   if( sysdata.next_pfile_cleanup )
      fprintf( fp, "NPfileCleanup     %ld\n", sysdata.next_pfile_cleanup );
   if( sysdata.alltimemax )
      fprintf( fp, "Highplayers       %d\n", sysdata.alltimemax );
   if( sysdata.ban_site != PERM_HEAD)
      fprintf( fp, "BanSiteLevel      %d\n", sysdata.ban_site );
   if( sysdata.ban_race != PERM_HEAD )
      fprintf( fp, "BanRaceLevel      %d\n", sysdata.ban_race );
   if( sysdata.ban_class != PERM_HEAD )
      fprintf( fp, "BanClassLevel     %d\n", sysdata.ban_class );
   if( sysdata.perm_forcepc != PERM_HEAD )
      fprintf( fp, "Forcepc           %d\n", sysdata.perm_forcepc );
   if( sysdata.maxauction != 5 )
      fprintf( fp, "MaxAuction        %d\n", sysdata.maxauction );
   if( sysdata.NAME_RESOLVING )
      fprintf( fp, "%s", "NameResolving\n" );
   if( sysdata.DENY_NEW_PLAYERS )
      fprintf( fp, "%s", "DenyNewPlayers\n" );
   if( sysdata.WAIT_FOR_AUTH )
      fprintf( fp, "%s", "WaitForAuth\n" );
   if( sysdata.morph_opt )
      fprintf( fp, "%s", "MorphOpt\n" );
   if( sysdata.save_pets )
      fprintf( fp, "%s", "PetSave\n" );
   if( sysdata.pk_loot )
      fprintf( fp, "%s", "PkLoot\n" );
   if( sysdata.wizlock )
      fprintf( fp, "%s", "WizLock\n" );
   if( sysdata.skipclasses )
      fprintf( fp, "%s", "SkipClasses\n" );
   if( sysdata.autosavecset )
      fprintf( fp, "%s", "AutoSaveCset\n" );
   if( sysdata.autosavecommands )
      fprintf( fp, "%s", "AutoSaveCommands\n" );
   if( sysdata.autosavesocials )
      fprintf( fp, "%s", "AutoSaveSocials\n" );
   if( sysdata.autosaveskills )
      fprintf( fp, "%s", "AutoSaveSkills\n" );
   if( sysdata.autosavehelps )
      fprintf( fp, "%s", "AutoSaveHelps\n" );
   if( sysdata.groupall )
      fprintf( fp, "%s", "GroupAll\n" );
   if( sysdata.perm_getobjnotake != PERM_LEADER )
      fprintf( fp, "GetObjNoTake      %d\n", sysdata.perm_getobjnotake );
   if( sysdata.perm_muse != PERM_LEADER )
      fprintf( fp, "Muse              %d\n", sysdata.perm_muse );
   if( sysdata.perm_think != PERM_HEAD )
      fprintf( fp, "Think             %d\n", sysdata.perm_think );
   if( sysdata.perm_build != PERM_BUILDER )
      fprintf( fp, "Build             %d\n", sysdata.perm_build );
   if( sysdata.perm_log != PERM_LOG )
      fprintf( fp, "Log               %d\n", sysdata.perm_log );
   if( sysdata.perm_modify_proto != PERM_LEADER )
      fprintf( fp, "Protoflag         %d\n", sysdata.perm_modify_proto );
   if( sysdata.perm_override_private != PERM_LEADER )
      fprintf( fp, "Overridepriv      %d\n", sysdata.perm_override_private );
   if( sysdata.perm_mset_player != PERM_HEAD )
      fprintf( fp, "Msetplayer        %d\n", sysdata.perm_mset_player );
   if( sysdata.save_frequency != 20 )
      fprintf( fp, "Savefreq          %d\n", sysdata.save_frequency );
   if( sysdata.bestow_dif != 5 )
      fprintf( fp, "Bestowdif         %d\n", sysdata.bestow_dif );
   if( sysdata.mlimit_total != 6 )
      fprintf( fp, "MLimitTotal       %d\n", sysdata.mlimit_total );
   if( sysdata.mlimit_deadly != 1 )
      fprintf( fp, "MLimitDeadly      %d\n", sysdata.mlimit_total );
   if( sysdata.mlimit_peaceful != 3 )
      fprintf( fp, "MLimitPeaceful    %d\n", sysdata.mlimit_peaceful );
   if( sysdata.maxpet > 1 )
      fprintf( fp, "MaxPet            %d\n", sysdata.maxpet );
   if( sysdata.maxkillhistory != 10 )
      fprintf( fp, "MaxKillHistory    %d\n", sysdata.maxkillhistory );
   if( sysdata.expmulti > 1 )
      fprintf( fp, "ExpMulti          %d\n", sysdata.expmulti );
   if( sysdata.mclass > 1 )
      fprintf( fp, "MClass            %d\n", sysdata.mclass );
   if( sysdata.room_limbo )
      fprintf( fp, "RoomLimbo         %d\n", sysdata.room_limbo );
   if( sysdata.room_poly )
      fprintf( fp, "RoomPoly          %d\n", sysdata.room_poly );
   if( sysdata.room_authstart )
      fprintf( fp, "RoomAuthstart     %d\n", sysdata.room_authstart );
   if( sysdata.room_deadly )
      fprintf( fp, "RoomDeadly        %d\n", sysdata.room_deadly );
   if( sysdata.room_school )
      fprintf( fp, "RoomSchool        %d\n", sysdata.room_school );
   if( sysdata.room_temple )
      fprintf( fp, "RoomTemple        %d\n", sysdata.room_temple );
   if( sysdata.room_altar )
      fprintf( fp, "RoomAltar         %d\n", sysdata.room_altar );
   if( sysdata.groupleveldiff != 10 )
      fprintf( fp, "GroupLevelDiff    %d\n", sysdata.groupleveldiff );

   fprintf( fp, "End\n" );
   fclose( fp );
   fp = NULL;
}

void fread_sysdata( FILE *fp )
{
   char *infoflags, flag[MIL];
   const char *word;
   int value;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;

      if( word[0] == EOF )
         word = "End";

      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "AutoSaveCset" ) )
            {
               sysdata.autosavecset = true;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "AutoSaveCommands" ) )
            {
               sysdata.autosavecommands = true;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "AutoSaveSocials" ) )
            {
               sysdata.autosavesocials = true;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "AutoSaveSkills" ) )
            {
               sysdata.autosaveskills = true;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "AutoSaveHelps" ) )
            {
               sysdata.autosavehelps = true;
               fMatch = true;
               break;
            }
            break;

         case 'B':
            KEY( "Bestowdif", sysdata.bestow_dif, fread_number( fp ) );
            KEY( "Build", sysdata.perm_build, fread_number( fp ) );
            KEY( "BanSiteLevel", sysdata.ban_site, fread_number( fp ) );
            KEY( "BanClassLevel", sysdata.ban_class, fread_number( fp ) );
            KEY( "BanRaceLevel", sysdata.ban_race, fread_number( fp ) );
            break;

         case 'D':
            if( !str_cmp( word, "DenyNewPlayers" ) )
            {
               sysdata.DENY_NEW_PLAYERS = true;
               fMatch = true;
               break;
            }
            break;

         case 'E':
            KEY( "ExpMulti", sysdata.expmulti, fread_number( fp ) );
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'F':
            KEY( "Forcepc", sysdata.perm_forcepc, fread_number( fp ) );
            break;

         case 'G':
            KEY( "GroupLevelDiff", sysdata.groupleveldiff, fread_number( fp ) );
            KEY( "GetObjNoTake", sysdata.perm_getobjnotake, fread_number( fp ) );
            if( !str_cmp( word, "GroupAll" ) )
            {
               sysdata.groupall = true;
               fMatch = true;
               break;
            }
            break;

         case 'H':
            KEY( "Highplayers", sysdata.alltimemax, fread_number( fp ) );
            KEY( "Highplayertime", sysdata.time_of_max, fread_time( fp ) );
            break;

         case 'I':
            if( !str_cmp( word, "IdentTries" ) )
            {
               fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'L':
            KEY( "Log", sysdata.perm_log, fread_number( fp ) );
            break;

         case 'M':
            if( !str_cmp( word, "MClass" ) )
            {
               sysdata.mclass = fread_number( fp );
               if( sysdata.mclass > MAX_PC_CLASS )
                  sysdata.mclass = MAX_PC_CLASS;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "MorphOpt" ) )
            {
               sysdata.morph_opt = true;
               fMatch = true;
               break;
            }
            KEY( "MaxAuction", sysdata.maxauction, fread_number( fp ) );
            KEY( "MLimitTotal", sysdata.mlimit_total, fread_number( fp ) );
            KEY( "MLimitDeadly", sysdata.mlimit_deadly, fread_number( fp ) );
            KEY( "MLimitPeaceful", sysdata.mlimit_peaceful, fread_number( fp ) );
            KEY( "MaxPet", sysdata.maxpet, fread_number( fp ) );
            KEY( "MaxKillHistory", sysdata.maxkillhistory, fread_number( fp ) );
            KEY( "Msetplayer", sysdata.perm_mset_player, fread_number( fp ) );
            KEY( "MudName", sysdata.mud_name, fread_string( fp ) );
            KEY( "Muse", sysdata.perm_muse, fread_number( fp ) );
            break;

         case 'N':
            if( !str_cmp( word, "NameResolving" ) )
            {
               sysdata.NAME_RESOLVING = true;
               fMatch = true;
               break;
            }
            KEY( "NPfileCleanup", sysdata.next_pfile_cleanup, fread_time( fp ) );
            break;

         case 'O':
            KEY( "Overridepriv", sysdata.perm_override_private, fread_number( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "PetSave" ) )
            {
               sysdata.save_pets = true;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "PkLoot" ) )
            {
               sysdata.pk_loot = true;
               fMatch = true;
               break;
            }
            KEY( "Protoflag", sysdata.perm_modify_proto, fread_number( fp ) );
            break;

         case 'R':
            KEY( "RoomLimbo", sysdata.room_limbo, fread_number( fp ) );
            KEY( "RoomPoly", sysdata.room_poly, fread_number( fp ) );
            KEY( "RoomAuthstart", sysdata.room_authstart, fread_number( fp ) );
            KEY( "RoomDeadly", sysdata.room_deadly, fread_number( fp ) );
            KEY( "RoomSchool", sysdata.room_school, fread_number( fp ) );
            KEY( "RoomTemple", sysdata.room_temple, fread_number( fp ) );
            KEY( "RoomAltar", sysdata.room_altar, fread_number( fp ) );
            break;

         case 'S':
            WEXTKEY( "Saveflags", sysdata.save_flags, fp, save_flag, SV_MAX );
            KEY( "Savefreq", sysdata.save_frequency, fread_number( fp ) );
            if( !str_cmp( word, "SkipClasses" ) )
            {
               sysdata.skipclasses = true;
               fMatch = true;
               break;
            }
            break;

         case 'T':
            KEY( "Think", sysdata.perm_think, fread_number( fp ) );
            break;

         case 'V':
            if( !str_cmp( word, "Version" ) )
            {
               sysdata.version_major = fread_number( fp );
               sysdata.version_minor = fread_number( fp );
               fMatch = true;
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "WizLock" ) )
            {
               sysdata.wizlock = true;
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "WaitForAuth" ) )
            {
               sysdata.WAIT_FOR_AUTH = true;
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

/* Load the sysdata file */
void load_systemdata( void )
{
   FILE *fp;

   /* Defaults */
   xCLEAR_BITS( sysdata.save_flags );
   sysdata.version_major = 1;
   sysdata.version_minor = 33;
   sysdata.mud_name = NULL;
   sysdata.perm_muse = PERM_LEADER;
   sysdata.perm_think = PERM_HEAD;
   sysdata.perm_build = PERM_BUILDER;
   sysdata.perm_log = PERM_LOG;
   sysdata.perm_modify_proto = PERM_LEADER;
   sysdata.perm_override_private = PERM_LEADER;
   sysdata.perm_mset_player = PERM_HEAD;
   sysdata.perm_getobjnotake = PERM_LEADER;
   sysdata.ban_site = PERM_HEAD;
   sysdata.ban_race = PERM_HEAD;
   sysdata.ban_class = PERM_HEAD;
   sysdata.perm_forcepc = PERM_HEAD;
   sysdata.save_frequency = 20;  /* minutes */
   sysdata.bestow_dif = 5;
   sysdata.maxauction = 5;
   sysdata.time_of_max = 0;
   sysdata.next_pfile_cleanup = 0;
   sysdata.WAIT_FOR_AUTH = false;
   sysdata.DENY_NEW_PLAYERS = false;
   sysdata.NAME_RESOLVING = false;
   sysdata.morph_opt = false;
   sysdata.save_pets = false;
   sysdata.pk_loot = false;
   sysdata.wizlock = false;
   sysdata.autosavecset = false;
   sysdata.autosavecommands = false;
   sysdata.autosavesocials = false;
   sysdata.autosaveskills = false;
   sysdata.autosavehelps = false;
   sysdata.skipclasses = false;
   sysdata.mlimit_total = 6;
   sysdata.mlimit_deadly = 1;
   sysdata.mlimit_peaceful = 4;
   sysdata.maxpet = 1;
   sysdata.maxkillhistory = 10;
   sysdata.expmulti = 1;
   sysdata.mclass = 1;
   sysdata.room_limbo = 2;
   sysdata.room_poly = 3;
   sysdata.room_authstart = 21017;
   sysdata.room_deadly = 21017;
   sysdata.room_school = 21017;
   sysdata.room_temple = 21017;
   sysdata.room_altar = 21017;
   sysdata.groupleveldiff = 10;
   sysdata.groupall = false;

   if( !( fp = fopen( SYSDATA_FILE, "r" ) ) )
   {
      log_printf( "%s: couldn't open %s for reading. Using Defaults.", __FUNCTION__, SYSDATA_FILE );
      perror( SYSDATA_FILE );
      return;
   }
   fread_sysdata( fp );
   fclose( fp );
   fp = NULL;
}

CMDF( do_cset )
{
   char arg[MSL];
   short level;

   set_pager_color( AT_PLAIN, ch );
   if( !argument || argument[0] == '\0' )
   {
      pager_printf( ch, "\r\n&WMudName: &c%s\r\n", sysdata.mud_name ? sysdata.mud_name : "Not Set" );

      pager_printf( ch, "&WVersion: &c%d.%d\r\n", sysdata.version_major, sysdata.version_minor );

      pager_printf( ch, "&WMultiPlayingLimits:\r\n  &wMLTotal:        &c%6d &wMLDeadly:       &c%6d &wMLPeaceful:       &c%6d\r\n",
         sysdata.mlimit_total, sysdata.mlimit_deadly, sysdata.mlimit_peaceful );

      pager_printf( ch, "&WSaveFlag:\r\n  &c%s\r\n", ext_flag_string( &sysdata.save_flags, save_flag ) );

      pager_printf( ch, "&WMaxAuction: &c%d\r\n", sysdata.maxauction );

      send_to_pager( "&WAutoSaves:\r\n", ch );
      pager_printf( ch, "  &wASCset:         &c%6s  &wASCommands:    &c%6s &wASSocials:       &c%6s\r\n",
         sysdata.autosavecset ? "ON" : "off", sysdata.autosavecommands ? "ON" : "off", sysdata.autosavesocials ? "ON" : "off" );
      pager_printf( ch, "  &wASSkills:       &c%6s  &wASHelps:       &c%6s &wMorphOpt:        &c%6s\r\n",
         sysdata.autosaveskills ? "ON" : "off", sysdata.autosavehelps ? "ON" : "off", sysdata.morph_opt ? "ON" : "off" );

      send_to_pager( "&WChannels:\r\n", ch );
      pager_printf( ch, "  &wMuse:           &c%6s &wThink:          &c%6s &wLog:             &c%6s\r\n  &wBuild:         &c%6s\r\n",
         perms_flag[sysdata.perm_muse], perms_flag[sysdata.perm_think],
         perms_flag[sysdata.perm_log], perms_flag[sysdata.perm_build] );

      send_to_pager( "&WBuilding:\r\n", ch );
      pager_printf( ch, "  &wProtoModify:    &c%6s &wMset_Player:    &c%6s\r\n",
         perms_flag[sysdata.perm_modify_proto], perms_flag[sysdata.perm_mset_player] );

      send_to_pager( "&WBan Data:\r\n", ch );
      pager_printf( ch, "  &wBanSite:        &c%6s &wBanClass:       &c%6s &wBanRace:          &c%6s\r\n",
         perms_flag[sysdata.ban_site], perms_flag[sysdata.ban_class], perms_flag[sysdata.ban_race] );

      send_to_pager( "&WOthers:\r\n", ch );
      pager_printf( ch, "  &wGetObjNoTake:   &c%6s &wSaveFrequency:  &c%6d &wBestowDif:        &c%6d\r\n",
         perms_flag[sysdata.perm_getobjnotake], sysdata.save_frequency, sysdata.bestow_dif );
      pager_printf( ch, "  &wGroupLevelDiff: &c%6d &wForcePc:        &c%6s &wOverridePrivate:  &c%6s\r\n",
         sysdata.groupleveldiff, perms_flag[sysdata.perm_forcepc], perms_flag[sysdata.perm_override_private] );
      pager_printf( ch, "  &wMaxPet:         &c%6d &wExpMulti:       &c%6d &wMultiClass:       &c%6d\r\n",
         sysdata.maxpet, sysdata.expmulti, sysdata.mclass );
      pager_printf( ch, "  &wMaxKillHistory: &c%6d\r\n",
         sysdata.maxkillhistory );

      send_to_pager( "&WToggles:\r\n", ch );
      pager_printf( ch, "  &wNameResolving:  &c%6s &wWaitForAuth:    &c%6s &wPkLoot:           &c%6s\r\n",
         sysdata.NAME_RESOLVING ? "ON" : "off", sysdata.WAIT_FOR_AUTH ? "ON" : "off", sysdata.pk_loot ? "ON" : "off" );
      pager_printf( ch, "  &wDenyNewPlayers: &c%6s &wPetSave:        &c%6s\r\n",
         sysdata.DENY_NEW_PLAYERS ? "ON" : "off", sysdata.save_pets ? "ON" : "off" );
      pager_printf( ch, "  &wWizLock:        &c%6s &wSkipClasses:    &c%6s &wGroupAll:         &c%6s\r\n",
         sysdata.wizlock ? "ON" : "off", sysdata.skipclasses ? "ON" : "off", sysdata.groupall ? "ON" : "off" );

      send_to_pager( "&WRooms:\r\n", ch );
      pager_printf( ch, "  &wRoomLimbo:      &c%6d &wRoomPoly:       &c%6d &wRoomAuthStart:    &c%6d\r\n",
         sysdata.room_limbo, sysdata.room_poly, sysdata.room_authstart );
      pager_printf( ch, "  &wRoomDeadly:     &c%6d &wRoomSchool:     &c%6d &wRoomTemple:       &c%6d\r\n",
         sysdata.room_deadly, sysdata.room_school, sysdata.room_temple );
      pager_printf( ch, "  &wRoomAltar:      &c%6d\r\n", sysdata.room_altar );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "save" ) )
   {
      save_sysdata( false );
      send_to_char( "Cset functions saved.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "mudname" ) )
   {
      STRSET( sysdata.mud_name, argument );
      send_to_char( "Name Set.\r\n", ch );
      save_sysdata( true );
      return;
   }

   if( !str_cmp( arg, "version" ) )
   {
      send_to_char( "Usage: cset majorversion/minorversion <#>\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "saveflag" ) )
   {
      int x = 0;

      while( argument && argument[0] != '\0' )
      {
         argument = one_argument( argument, arg );
         x = get_flag( arg, save_flag, SV_MAX );
         if( x < 0 || x >= SV_MAX )
            ch_printf( ch, "%s isn't a valid save flag.\r\n", arg );
         else
            xTOGGLE_BIT( sysdata.save_flags, x );
      }
      send_to_char( "SaveFlags Set.\r\n", ch );
      save_sysdata( true );
      return;
   }

   level = ( short )atoi( argument );

   switch( UPPER( arg[0] ) )
   {
      case 'A':
         if( !str_cmp( arg, "ascset" ) )
         {
            sysdata.autosavecset = !sysdata.autosavecset;
            /* If we are turning off autosave need to do one last auto save, set as false so it will save change since autosave is now off */
            save_sysdata( !sysdata.autosavecset ? false : true );
            ch_printf( ch, "Ascset is now %s.\r\n", sysdata.autosavecset ? "ON" : "off" );
            return;
         }
         if( !str_cmp( arg, "ascommands" ) )
         {
            sysdata.autosavecommands = !sysdata.autosavecommands;
            ch_printf( ch, "Ascommands is now %s.\r\n", sysdata.autosavecommands ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "associals" ) )
         {
            sysdata.autosavesocials = !sysdata.autosavesocials;
            ch_printf( ch, "Associals is now %s.\r\n", sysdata.autosavesocials ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "asskills" ) )
         {
            sysdata.autosaveskills = !sysdata.autosaveskills;
            ch_printf( ch, "Asskills is now %s.\r\n", sysdata.autosaveskills ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "ashelps" ) )
         {
            sysdata.autosavehelps = !sysdata.autosavehelps;
            ch_printf( ch, "Ashelps is now %s.\r\n", sysdata.autosavehelps ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         break;

      case 'B':
         if( !str_cmp( arg, "build" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_build = level;
            ch_printf( ch, "Build is now %d.\r\n", sysdata.perm_build );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "bansite" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.ban_site = level;
            ch_printf( ch, "Bansite is now %d.\r\n", sysdata.ban_site );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "banrace" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.ban_race = level;
            ch_printf( ch, "Banrace is now %d.\r\n", sysdata.ban_race );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "banclass" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.ban_class = level;
            ch_printf( ch, "Banclass is now %d.\r\n", sysdata.ban_class );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "bestowdif" ) )
         {
            sysdata.bestow_dif = URANGE( 1, level, 10 );
            ch_printf( ch, "Bestowdif is now %d.\r\n", sysdata.bestow_dif );
            save_sysdata( true );
            return;
         }
         break;

      case 'D':
         if( !str_cmp( arg, "denynewplayers" ) )
         {
            sysdata.DENY_NEW_PLAYERS = !sysdata.DENY_NEW_PLAYERS;
            ch_printf( ch, "Denynewplayers is now %s.\r\n", sysdata.DENY_NEW_PLAYERS ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         break;

      case 'E':
         if( !str_cmp( arg, "expmulti" ) )
         {
            sysdata.expmulti = URANGE( 1, level, 100 );
            ch_printf( ch, "ExpMulti set to %d.\r\n", sysdata.expmulti );
            save_sysdata( true );
            return;
         }
         break;

      case 'F':
         if( !str_cmp( arg, "forcepc" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_forcepc = level;
            ch_printf( ch, "Forcepc is now %d.\r\n", sysdata.perm_forcepc );
            save_sysdata( true );
            return;
         }
         break;

      case 'G':
         if( !str_cmp( arg, "getobjnotake" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_getobjnotake = level;
            ch_printf( ch, "Getobjnotake is now %d.\r\n", sysdata.perm_getobjnotake );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "groupall" ) )
         {
            sysdata.groupall = !sysdata.groupall;
            ch_printf( ch, "Groupall is now %s.\r\n", sysdata.groupall ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "groupleveldiff" ) )
         {
            sysdata.groupleveldiff = URANGE( -1, level, MAX_LEVEL );
            if( sysdata.groupleveldiff == -1 )
               ch_printf( ch, "Groupleveldiff is %d, this turns off grouping.\r\n", sysdata.groupleveldiff );
            else
               ch_printf( ch, "GroupLevelDiff set to %d.\r\n", sysdata.groupleveldiff );
            save_sysdata( true );
            return;
         }
         break;

      case 'L':
         if( !str_cmp( arg, "log" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_log = level;
            ch_printf( ch, "Log is now %d.\r\n", sysdata.perm_log );
            save_sysdata( true );
            return;
         }
         break;

      case 'M':
         if( !str_cmp( arg, "majorversion" ) )
         {
            sysdata.version_major = UMAX( 1, level );
            ch_printf( ch, "Version is now set to %d.%d.\r\n", sysdata.version_major, sysdata.version_minor );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "maxauction" ) )
         {
            sysdata.maxauction = UMAX( 1, level );
            ch_printf( ch, "MaxAuction is now set to %d.\r\n", sysdata.maxauction );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "minorversion" ) )
         {
            sysdata.version_minor = UMAX( 1, level );
            ch_printf( ch, "Version is now set to %d.%d.\r\n", sysdata.version_major, sysdata.version_minor );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "mltotal" ) )
         {
            sysdata.mlimit_total = URANGE( 1, level, 100 );
            ch_printf( ch, "MLTotal set to %d.\r\n", sysdata.mlimit_total );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "mldeadly" ) )
         {
            sysdata.mlimit_deadly = URANGE( 1, level, 100 );
            ch_printf( ch, "MLDeadly set to %d.\r\n", sysdata.mlimit_deadly );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "mlpeaceful" ) )
         {
            sysdata.mlimit_peaceful = URANGE( 1, level, 100 );
            ch_printf( ch, "MLPeaceful set to %d.\r\n", sysdata.mlimit_peaceful );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "maxpet" ) )
         {
            sysdata.maxpet = URANGE( 1, level, 100 );
            ch_printf( ch, "MaxPet set to %d.\r\n", sysdata.maxpet );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "maxkillhistory" ) )
         {
            sysdata.maxkillhistory = URANGE( 1, level, 100 );
            ch_printf( ch, "MaxKillHistory set to %d.\r\n", sysdata.maxkillhistory );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "multiclass" ) )
         {
            sysdata.mclass = URANGE( 1, level, MAX_PC_CLASS );
            ch_printf( ch, "MultiClass set to %d.\r\n", sysdata.mclass );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "morphopt" ) )
         {
            sysdata.morph_opt = !sysdata.morph_opt;
            ch_printf( ch, "Morphopt is now %s.\r\n", sysdata.morph_opt ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "mset_player" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_mset_player = level;
            ch_printf( ch, "Mset_player is now %d.\r\n", sysdata.perm_mset_player );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "muse" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_muse = level;
            ch_printf( ch, "Muse is now %d.\r\n", sysdata.perm_muse );
            save_sysdata( true );
            return;
         }
         break;

      case 'N':
         if( !str_cmp( arg, "nameresolving" ) )
         {
            sysdata.NAME_RESOLVING = !sysdata.NAME_RESOLVING;
            ch_printf( ch, "Nameresolving is now %s.\r\n", sysdata.NAME_RESOLVING ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         break;

      case 'O':
         if( !str_cmp( arg, "overrideprivate" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_override_private = level;
            ch_printf( ch, "Overrideprivate is now %d.\r\n", sysdata.perm_override_private );
            save_sysdata( true );
            return;
         }
         break;

      case 'P':
         if( !str_cmp( arg, "protomodify" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_modify_proto = level;
            ch_printf( ch, "Protomodify is now %d.\r\n", sysdata.perm_modify_proto );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "petsave" ) )
         {
            sysdata.save_pets = !sysdata.save_pets;
            ch_printf( ch, "Petsave is now %s.\r\n", sysdata.save_pets ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "pkloot" ) )
         {
            sysdata.pk_loot = !sysdata.pk_loot;
            ch_printf( ch, "Pkloot is now %s.\r\n", sysdata.pk_loot ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         break;

      case 'R':
         if( !str_cmp( arg, "roomlimbo" ) )
         {
            sysdata.room_limbo = UMAX( 0, level );
            ch_printf( ch, "Roomlimbo is now %d.\r\n", sysdata.room_limbo );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "roompoly" ) )
         {
            sysdata.room_poly = UMAX( 0, level );
            ch_printf( ch, "Roompoly is now %d.\r\n", sysdata.room_poly );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "roomauthstart" ) )
         {
            sysdata.room_authstart = UMAX( 0, level );
            ch_printf( ch, "Roomauthstart is now %d.\r\n", sysdata.room_authstart );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "roomdeadly" ) )
         {
            sysdata.room_deadly = UMAX( 0, level );
            ch_printf( ch, "Roomdeadly is now %d.\r\n", sysdata.room_deadly );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "roomschool" ) )
         {
            sysdata.room_school = UMAX( 0, level );
            ch_printf( ch, "Roomschool is now %d.\r\n", sysdata.room_school );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "roomtemple" ) )
         {
            sysdata.room_temple = UMAX( 0, level );
            ch_printf( ch, "Roomtemple is now %d.\r\n", sysdata.room_temple );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "roomaltar" ) )
         {
            sysdata.room_altar = UMAX( 0, level );
            ch_printf( ch, "Roomaltar is now %d.\r\n", sysdata.room_altar );
            save_sysdata( true );
            return;
         }
         break;

      case 'S':
         if( !str_prefix( arg, "savefrequency" ) )
         {
            sysdata.save_frequency = level;
            ch_printf( ch, "Savefrequency is now %d.\r\n", sysdata.save_frequency );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "skipclasses" ) )
         {
            sysdata.skipclasses = !sysdata.skipclasses;
            ch_printf( ch, "Skipclasses is now %s.\r\n", sysdata.skipclasses ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         break;

      case 'T':
         if( !str_cmp( arg, "think" ) )
         {
            if( !is_number( argument ) )
               level = get_flag( argument, perms_flag, PERM_MAX );
            if( level < PERM_IMM || level > get_trust( ch ) || level >= PERM_MAX )
            {
               send_to_char( "Permission out of range.\r\n", ch );
               return;
            }
            sysdata.perm_think = level;
            ch_printf( ch, "Think is now %d.\r\n", sysdata.perm_think );
            save_sysdata( true );
            return;
         }
         break;

      case 'W':
         if( !str_cmp( arg, "wizlock" ) )
         {
            sysdata.wizlock = !sysdata.wizlock;
            ch_printf( ch, "Wizlock is now %s.\r\n", sysdata.wizlock ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         if( !str_cmp( arg, "waitforauth" ) )
         {
            sysdata.WAIT_FOR_AUTH = !sysdata.WAIT_FOR_AUTH;
            ch_printf( ch, "Waitforauth is now %s.\r\n", sysdata.WAIT_FOR_AUTH ? "ON" : "off" );
            save_sysdata( true );
            return;
         }
         break;
   }

   ch_printf( ch, "%s is not a valid argument.\r\n", arg );
}

CMDF( do_wizlock )
{
   sysdata.wizlock = !sysdata.wizlock;
   save_sysdata( false );
   ch_printf( ch, "&[danger]Game %swizlocked and sysdata saved.&D\r\n", sysdata.wizlock ? "" : "un-" );
}
