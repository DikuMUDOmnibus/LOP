/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2005 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine, and Adjani.    *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office. TX 5-877-286         *
 *                                                                          *
 * External contributions from Xorith, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *               Color Module -- Allow user customizable Colors.            *
 *                                   --Matthew                              *
 *                      Enhanced ANSI parser by Samson                      *
 ****************************************************************************/

/*
* The following instructions assume you know at least a little bit about
* coding.  I firmly believe that if you can't code (at least a little bit),
* you don't belong running a mud.  So, with that in mind, I don't hold your
* hand through these instructions.
*
* You may use this code provided that:
*
*     1)  You understand that the authors _DO NOT_ support this code
*         Any help you need must be obtained from other sources.  The
*         authors will ignore any and all requests for help.
*     2)  You will mention the authors if someone asks about the code.
*         You won't take credit for the code, but you can take credit
*         for any enhancements you make.
*     3)  This message remains intact.
*
* If you would like to find out how to send the authors large sums of money,
* you may e-mail the following address:
*
* Matthew Bafford & Christopher Wigginton
* wiggy@mudservices.com
*/

/*
 * To add new color types:
 *
 * 1.  Edit color.h, and:
 *     1.  Add a new AT_ define.
 *     2.  Increment MAX_COLORS by however many AT_'s you added.
 * 2.  Edit color.c and:
 *     1.  Add the name(s) for your new color(s) to the end of the pc_displays array.
 *     2.  Add the default color(s) to the end of the default_set array.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include "h/mud.h"

const char *pc_displays[MAX_COLORS] =
{
   "black",      "dred",       "dgreen",      "orange",
   "dblue",      "purple",     "cyan",        "gray",
   "dgray",      "red",        "green",       "yellow",
   "blue",       "pink",       "lblue",       "white",
   "blink",      "bdred",      "bdgreen",     "borange",
   "bdblue",     "bpurple",    "bcyan",       "bgray",
   "bdgray",     "bred",       "bgreen",      "byellow",
   "bblue",      "bpink",      "blblue",      "bwhite",
   /* Above are the basic 0 - 31 that should always be the same */
   "plain",      "action",     "hurt",        "falling",
   "danger",     "magic",      "skill",       "social",
   "consider",   "report",     "poison",      "dying",
   "dead",       "carnage",    "damage",      "flee",
   "hit",        "hitme",      "say",         "rmname",
   "rmdesc",     "objects",    "people",      "bye",
   "gold",       "gtells",     "hungry",      "thirsty",
   "fire",       "sober",      "wearoff",     "exits",
   "reset",      "log",        "diemsg",      "chat",
   "yell",       "tell",       "immort",      "wartalk",
   "muse",       "think",      "racetalk",    "clantalk",
   "nation",     "ignore",     "whisper",     "shout",
   "auction",    "intermud",   "divider",     "morph",
   "help",       "help2",      "board",       "board2",
   "note",       "note2",      "score",       "score2",
   "who",        "who2",       "male",        "female",
   "hint",       "hint2",      "prac",        "prac2",
   "prac3",      "group",      "group2",      "sudoku",
   "sudoku2",    "hscore",     "hscore2",     "music",
   "council",    "quest",      "classtalk",   "traffic",
   "fchat"
};

/* All defaults are set to Alsherok default scheme, if you don't 
like it, change it around to suite your own needs - Samson */
const short default_set[MAX_COLORS] =
{
   AT_BLACK,        AT_BLOOD,         AT_DGREEN,        AT_ORANGE,
   AT_DBLUE,        AT_PURPLE,        AT_CYAN,          AT_GRAY,
   AT_DGRAY,        AT_RED,           AT_GREEN,         AT_YELLOW,
   AT_BLUE,         AT_PINK,          AT_LBLUE,         AT_WHITE,
   AT_BLACK_BLINK,  AT_BLOOD_BLINK,   AT_DGREEN_BLINK,  AT_ORANGE_BLINK,
   AT_DBLUE_BLINK,  AT_PURPLE_BLINK,  AT_CYAN_BLINK,    AT_GRAY_BLINK,
   AT_DGRAY_BLINK,  AT_RED_BLINK,     AT_GREEN_BLINK,   AT_YELLOW_BLINK,
   AT_BLUE_BLINK,   AT_PINK_BLINK,    AT_LBLUE_BLINK,   AT_WHITE_BLINK,
   /* Above are the basic 0 - 31 that should always be the same */
   AT_GRAY,         AT_PURPLE,        AT_RED,           AT_RED,
   AT_YELLOW,       AT_LBLUE,         AT_WHITE,         AT_GREEN,
   AT_LBLUE,        AT_LBLUE,         AT_RED,           AT_RED,
   AT_RED,          AT_RED,           AT_RED,           AT_GREEN,
   AT_LBLUE,        AT_BLOOD,         AT_LBLUE,         AT_GRAY,
   AT_BLUE,         AT_GRAY,          AT_DBLUE,         AT_GRAY,
   AT_RED,          AT_GRAY,          AT_BLUE,          AT_PINK,
   AT_GRAY,         AT_GRAY,          AT_YELLOW,        AT_GREEN,
   AT_GRAY,         AT_ORANGE,        AT_BLUE,          AT_RED,
   AT_GRAY,         AT_GRAY,          AT_GREEN,         AT_DGREEN,
   AT_DGREEN,       AT_ORANGE,        AT_GRAY,          AT_RED,
   AT_GRAY,         AT_DGREEN,        AT_RED,           AT_BLUE,
   AT_RED,          AT_CYAN,          AT_YELLOW,        AT_PINK,
   AT_DGREEN,       AT_WHITE,         AT_PINK,          AT_WHITE,
   AT_BLUE,         AT_BLUE,          AT_BLUE,          AT_GREEN,
   AT_GRAY,         AT_GREEN,         AT_LBLUE,         AT_PURPLE,
   AT_GRAY,         AT_CYAN,          AT_BLUE,          AT_CYAN,
   AT_RED,          AT_DGREEN,        AT_GREEN,         AT_YELLOW,
   AT_GREEN,        AT_YELLOW,        AT_LBLUE,         AT_DGRAY,
   AT_GREEN,        AT_RED,           AT_LBLUE,         AT_GREEN,
   AT_LBLUE
};

const char *valid_color[] =
{
   "black",    "dred",      "dgreen",    "orange",
   "dblue",    "purple",    "cyan",      "gray",
   "dgray",    "red",       "green",     "yellow",
   "blue",     "pink",      "lblue",     "white",
   "blink",    "bdred",     "bdgreen",   "bdorange",
   "bdblue",   "bpurple",   "bcyan",     "bgray",
   "bdgray",   "bred",      "bgreen",    "byellow",
   "bblue",    "bpink",     "blblue",    "bwhite",
   "\0"
};

/*  0 thru 15 are for normal colors */
/* 16 thru 31 are for blinking colors */
const char *color_return[] =
{
   ANSI_BLACK,    ANSI_DRED,      ANSI_DGREEN,    ANSI_ORANGE,    ANSI_DBLUE,
   ANSI_PURPLE,   ANSI_CYAN,      ANSI_GRAY,      ANSI_DGRAY,     ANSI_RED,
   ANSI_GREEN,    ANSI_YELLOW,    ANSI_BLUE,      ANSI_PINK,      ANSI_LBLUE,
   ANSI_WHITE,    BLINK_BLACK,    BLINK_DRED,     BLINK_DGREEN,   BLINK_ORANGE,
   BLINK_DBLUE,   BLINK_PURPLE,   BLINK_CYAN,     BLINK_GRAY,     BLINK_DGRAY,
   BLINK_RED,     BLINK_GREEN,    BLINK_YELLOW,   BLINK_BLUE,     BLINK_PINK,
   BLINK_LBLUE,   BLINK_WHITE
};

void show_colorthemes( CHAR_DATA *ch )
{
   DIR *dp;
   struct dirent *dentry;
   int count = 0, col = 0;

   send_to_pager( "&W*********************************[ &wTHEMES &W]**********************************\r\n", ch );

   dp = opendir( COLOR_DIR );
   dentry = readdir( dp );
   while( dentry )
   {
      /* Added by Tarl 3 Dec 02 because we are now using CVS */
      if( !str_cmp( dentry->d_name, "CVS" ) )
      {
         dentry = readdir( dp );
         continue;
      }
      if( dentry->d_name[0] != '.' )
      {
         ++count;
         pager_printf( ch, "%s%-15.15s", color_str( AT_PLAIN, ch ), dentry->d_name );
         if( ++col % 4 == 0 )
            send_to_pager( "\r\n", ch );
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   if( count == 0 )
      send_to_pager( "No themes defined yet.\r\n", ch );

   if( col % 4 != 0 )
      send_to_pager( "\r\n", ch );
}

void show_colors( CHAR_DATA *ch )
{
   short count;

   send_to_pager( "&BUsage: color ansitest\r\n", ch );
   send_to_pager( "&BUsage: color [color type] [color] | default\r\n", ch );
   send_to_pager( "&BUsage: color _reset_ (Resets all colors to default set)\r\n", ch );
   send_to_pager( "&BUsage: color _all_ [color] (Sets all color types to [color])\r\n", ch );
   send_to_pager( "&BUsage: color theme [name] (Sets all color types to a defined theme)\r\n", ch );

   send_to_pager( "&W*********************************[ &wCOLORS &W]**********************************\r\n", ch );
   for( count = 0; count < 32; ++count )
   {
      if( ( count % 8 ) == 0 && count != 0 )
      {
         send_to_pager( "\r\n", ch );
      }
      pager_printf( ch, "%s%-10s%s", color_str( count, ch ), pc_displays[count], ANSI_RESET );
   }

   send_to_pager( "\r\n&W*******************************[ &wCOLOR TYPES &W]*******************************\r\n", ch );
   for( count = 32; count < MAX_COLORS; ++count )
   {
      if( ( count % 8 ) == 0 && count != 32 )
      {
         send_to_pager( "\r\n", ch );
      }
      pager_printf( ch, "%s%-10s%s", color_str( count, ch ), pc_displays[count], ANSI_RESET );
   }
   send_to_pager( "\r\n", ch );

   show_colorthemes( ch );
}

void reset_colors( CHAR_DATA *ch )
{
   FILE *fp;
   char filename[MIL];
   int max_colors = 0, x;

   if( is_npc( ch ) )
   {
      log_printf( "%s: Attempting to reset NPC colors: %s", __FUNCTION__, ch->short_descr );
      return;
   }

   /* Set up the defaults for everyone incase the files dont have all that they should */
   memcpy( &ch->colors, &default_set, sizeof( default_set ) );

   snprintf( filename, sizeof( filename ), "%s%s", COLOR_DIR, "default" );
   if( !( fp = fopen( filename, "r" ) ) )
      return;

   while( !feof( fp ) )
   {
      char *word = fread_word( fp );
      if( !str_cmp( word, "MaxColors" ) )
      {
         max_colors = fread_number( fp );
         continue;
      }
      if( !str_cmp( word, "Colors" ) )
      {
         for( x = 0; x < max_colors; ++x )
         {
            if( x < MAX_COLORS )
               ch->colors[x] = fread_number( fp );
            else
               fread_number( fp );
         }
         continue;
      }
      if( !str_cmp( word, "End" ) )
      {
         fclose( fp );
         fp = NULL;
         return;
      }
   }
   fclose( fp );
   fp = NULL;
}

CMDF( do_color )
{
   char arg[MIL], arg2[MIL], log_buf[MSL];
   int x;
   short count = 0, y = 0;
   bool dMatch = false, cMatch = false;

   if( is_npc( ch ) )
   {
      send_to_pager( "Only PC's can change colors.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      show_colors( ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "savetheme" ) && is_immortal( ch ) )
   {
      FILE *fp;
      char filename[MIL];

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You must specify a name for this theme to save it.\r\n", ch );
         return;
      }
      if( strstr( argument, ".." ) || strstr( argument, "/" ) || strstr( argument, "\\" ) )
      {
         send_to_char( "Invalid theme name.\r\n", ch );
         return;
      }
      snprintf( filename, sizeof( filename ), "%s%s", COLOR_DIR, argument );
      if( !( fp = fopen( filename, "w" ) ) )
      {
         ch_printf( ch, "Unable to write to color file %s\r\n", filename );
         return;
      }
      fprintf( fp, "%s", "#COLORTHEME\n" );
      fprintf( fp, "Name         %s~\n", argument );
      fprintf( fp, "MaxColors    %d\n", MAX_COLORS );
      fprintf( fp, "%s", "Colors      " );
      for( x = 0; x < MAX_COLORS; ++x )
         fprintf( fp, " %d", ch->colors[x] );
      fprintf( fp, "%s", "\nEnd\n" );
      fclose( fp );
      fp = NULL;
      ch_printf( ch, "Color theme %s saved.\r\n", argument );
      return;
   }

   if( !str_cmp( arg, "theme" ) )
   {
      FILE *fp;
      char filename[MIL];
      int max_colors = 0;

      if( !argument || argument[0] == '\0' )
      {
         show_colorthemes( ch );
         return;
      }

      if( strstr( argument, ".." ) || strstr( argument, "/" ) || strstr( argument, "\\" ) )
      {
         send_to_char( "Invalid theme.\r\n", ch );
         return;
      }

      snprintf( filename, sizeof( filename ), "%s%s", COLOR_DIR, argument );
      if( !( fp = fopen( filename, "r" ) ) )
      {
         ch_printf( ch, "There is no theme called %s.\r\n", argument );
         return;
      }

      while( !feof( fp ) )
      {
         char *word = fread_word( fp );
         if( !str_cmp( word, "MaxColors" ) )
         {
            max_colors = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "Colors" ) )
         {
            for( x = 0; x < max_colors; ++x )
            {
               if( x < MAX_COLORS )
                  ch->colors[x] = fread_number( fp );
               else
                  fread_number( fp );
            }
            if( max_colors < MAX_COLORS )
            {
               for( x = max_colors; x < MAX_COLORS; ++x )
                  ch->colors[x] = default_set[x];
            }
            continue;
         }
         if( !str_cmp( word, "End" ) )
         {
            fclose( fp );
            fp = NULL;
            ch_printf( ch, "Color theme has been changed to %s.\r\n", argument );
            save_char_obj( ch );
            return;
         }
      }
      fclose( fp );
      fp = NULL;
      ch_printf( ch, "An error occured while trying to set color theme %s.\r\n", argument );
      return;
   }

   if( !str_cmp( arg, "ansitest" ) )
   {
      write_to_buffer( ch->desc, ANSI_RESET, 0 );
      write_to_buffer( ch->desc, "Normal Colors:\r\n", 0 );

      snprintf( log_buf, sizeof( log_buf ), "%sBlack%s  %sDRed%s  %sDGreen%s  %sOrange%s  %sDBlue%s  %sPurple%s  %sCyan%s   %sGray%s\r\n",
         ANSI_BLACK, ANSI_RESET, ANSI_DRED, ANSI_RESET, ANSI_DGREEN, ANSI_RESET, ANSI_ORANGE, ANSI_RESET, ANSI_DBLUE,
         ANSI_RESET, ANSI_PURPLE, ANSI_RESET, ANSI_CYAN, ANSI_RESET, ANSI_GRAY, ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );

      snprintf( log_buf, sizeof( log_buf ), "%sDGray%s  %sRed%s   %sGreen%s   %sYellow%s  %sBlue%s   %sPink%s    %sLBlue%s  %sWhite%s\r\n",
         ANSI_DGRAY, ANSI_RESET, ANSI_RED, ANSI_RESET, ANSI_GREEN, ANSI_RESET, ANSI_YELLOW, ANSI_RESET, ANSI_BLUE,
         ANSI_RESET, ANSI_PINK, ANSI_RESET, ANSI_LBLUE, ANSI_RESET, ANSI_WHITE, ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );

      write_to_buffer( ch->desc, ANSI_RESET, 0 );
      write_to_buffer( ch->desc, "Blinking Colors:\r\n", 0 );

      snprintf( log_buf, sizeof( log_buf ), "%sBlack%s  %sDRed%s  %sDGreen%s  %sOrange%s  %sDBlue%s  %sPurple%s  %sCyan%s   %sGray%s\r\n",
         BLINK_BLACK, ANSI_RESET, BLINK_DRED, ANSI_RESET, BLINK_DGREEN, ANSI_RESET, BLINK_ORANGE, ANSI_RESET,
         BLINK_DBLUE, ANSI_RESET, BLINK_PURPLE, ANSI_RESET, BLINK_CYAN, ANSI_RESET, BLINK_GRAY, ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );

      snprintf( log_buf, sizeof( log_buf ), "%sDGray%s  %sRed%s   %sGreen%s   %sYellow%s  %sBlue%s   %sPink%s    %sLBlue%s  %sWhite%s\r\n",
         BLINK_DGRAY, ANSI_RESET, BLINK_RED, ANSI_RESET, BLINK_GREEN, ANSI_RESET, BLINK_YELLOW, ANSI_RESET,
         BLINK_BLUE, ANSI_RESET, BLINK_PINK, ANSI_RESET, BLINK_LBLUE, ANSI_RESET, BLINK_WHITE, ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );

      write_to_buffer( ch->desc, ANSI_RESET, 0 );
      write_to_buffer( ch->desc, "Background Colors:\r\n", 0 );

      snprintf( log_buf, sizeof( log_buf ), "%s%sBlack%s  %s%sDRed%s  %s%sDGreen%s  %s%sOrange%s  %s%sDBlue%s  %s%sPurple%s  %s%sCyan%s   %s%sGray%s\r\n",
         ANSI_BLACK,  BACK_BLACK,  ANSI_RESET, ANSI_DRED,   BACK_DRED,   ANSI_RESET,
         ANSI_DGREEN, BACK_DGREEN, ANSI_RESET, ANSI_ORANGE, BACK_ORANGE, ANSI_RESET,
         ANSI_DBLUE,  BACK_DBLUE,  ANSI_RESET, ANSI_PURPLE, BACK_PURPLE, ANSI_RESET,
         ANSI_CYAN,   BACK_CYAN,   ANSI_RESET, ANSI_GRAY,   BACK_GRAY,   ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );

      snprintf( log_buf, sizeof( log_buf ), "%s%sDGray%s  %s%sRed%s   %s%sGreen%s   %s%sYellow%s  %s%sBlue%s   %s%sPink%s    %s%sLBlue%s  %s%sWhite%s\r\n",
         ANSI_DGRAY, BACK_DGRAY, ANSI_RESET, ANSI_RED,    BACK_RED,    ANSI_RESET,
         ANSI_GREEN, BACK_GREEN, ANSI_RESET, ANSI_YELLOW, BACK_YELLOW, ANSI_RESET,
         ANSI_BLUE,  BACK_BLUE,  ANSI_RESET, ANSI_PINK,   BACK_PINK,   ANSI_RESET,
         ANSI_LBLUE, BACK_LBLUE, ANSI_RESET, ANSI_WHITE,  BACK_WHITE,  ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );

      write_to_buffer( ch->desc, ANSI_RESET, 0 );
      write_to_buffer( ch->desc, "Other Things:\r\n", 0 );

      snprintf( log_buf, sizeof( log_buf ), "%s%sItalics%s\r\n", ANSI_GRAY, ANSI_ITALIC, ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );
      snprintf( log_buf, sizeof( log_buf ), "%sStrikeout%s\r\n", ANSI_STRIKEOUT, ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );
      snprintf( log_buf, sizeof( log_buf ), "%sUnderline%s\r\n", ANSI_UNDERLINE, ANSI_RESET );
      write_to_buffer( ch->desc, log_buf, 0 );
      return;
   }

   if( !str_prefix( arg, "_reset_" ) )
   {
      reset_colors( ch );
      send_to_pager( "All color types reset to default colors.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg2 );

   if( arg == NULL || arg[0] == '\0' )
   {
      send_to_char( "Change which color type?\r\n", ch );
      return;
   }

   if( arg == NULL || arg[0] == '\0' || arg2 == NULL || arg2[0] == '\0' )
      cMatch = false;
   else if( !str_prefix( arg, "_all_" ) )
   {
      dMatch = true;
      count = -1;

      /* search for a valid color setting */
      for( y = 0; y < 32; y++ )
      {
         if( !str_cmp( arg2, valid_color[y] ) )
         {
            cMatch = true;
            break;
         }
      }
   }
   else
   {
      /* search for the display type and str_cmp */
      for( count = 32; count < MAX_COLORS; count++ )
      {
         if( !str_prefix( arg, pc_displays[count] ) )
         {
            dMatch = true;
            break;
         }
      }

      if( !dMatch )
      {
         ch_printf( ch, "%s is an invalid color type.\r\n", arg );
         send_to_char( "Type color with no arguments to see available options.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "default" ) )
      {
         ch->colors[count] = default_set[count];
         ch_printf( ch, "Display %s set back to default.\r\n", pc_displays[count] );
         return;
      }

      /* search for a valid color setting */
      for( y = 0; y < 32; y++ )
      {
         if( !str_cmp( arg2, valid_color[y] ) )
         {
            cMatch = true;
            break;
         }
      }
   }

   if( !cMatch )
   {
      if( arg[0] )
         ch_printf( ch, "Invalid color for type %s.\r\n", arg );
      else
         send_to_pager( "Invalid color.\r\n", ch );

      send_to_pager( "Choices are:\r\n", ch );

      for( count = 0; count < 32; count++ )
      {
         if( count % 8 == 0 && count != 0 )
            send_to_pager( "\r\n", ch );

         pager_printf( ch, "%-10s", valid_color[count] );
      }
      pager_printf( ch, "\r\n%-10s\r\n", "default" );
      return;
   }
   else
      pager_printf( ch, "Color type %s set to color %s.\r\n", count == -1 ? "_all_" : pc_displays[count], valid_color[y] );

   /* Only toss in blink of we need to */
   if( !str_cmp( argument, "blink" ) && y < 16 )
      y += AT_BLINK;

   if( count == -1 )
   {
      int ccount;

      for( ccount = 0; ccount < MAX_COLORS; ++ccount )
         ch->colors[ccount] = y;

      set_pager_color( y, ch );

      pager_printf( ch, "All color types set to color %s%s.%s\r\n",
                    valid_color[y > AT_BLINK ? y - AT_BLINK : y], y > AT_BLINK ? " [BLINKING]" : "", ANSI_RESET );
   }
   else
   {
      ch->colors[count] = y;

      set_pager_color( count, ch );

      if( !str_cmp( argument, "blink" ) )
         ch_printf( ch, "Display %s set to color %s [BLINKING]%s\r\n",
                    pc_displays[count], valid_color[y - AT_BLINK], ANSI_RESET );
      else
         ch_printf( ch, "Display %s set to color %s.\r\n", pc_displays[count], valid_color[y] );
   }
}

const char *color_str( short AType, CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s", "color_str: NULL ch!" );
      return ( "" );
   }

   if( is_npc( ch ) || !xIS_SET( ch->act, PLR_ANSI ) )
      return ( "" );

   if( ch->colors[AType] >= 0 && ch->colors[AType] <= 31 )
      return ( color_return[ch->colors[AType]] );
   else
      return ( ANSI_RESET );
}

/* Random Ansi Color Code -- Xorith */
const char *random_ansi( short type )
{
   switch( type )
   {
      default:
      case 1: /* Default ANSI Fore-ground */
         switch( number_range( 1, 15 ) )
         {
            case 1:  return ( ANSI_DRED );
            case 2:  return ( ANSI_DGREEN );
            case 3:  return ( ANSI_ORANGE );
            case 4:  return ( ANSI_DBLUE );
            case 5:  return ( ANSI_PURPLE );
            case 6:  return ( ANSI_CYAN );
            case 7:  return ( ANSI_GRAY );
            case 8:  return ( ANSI_DGRAY );
            case 9:  return ( ANSI_RED );
            case 10: return ( ANSI_GREEN );
            case 11: return ( ANSI_YELLOW );
            case 12: return ( ANSI_BLUE );
            case 13: return ( ANSI_PINK );
            case 14: return ( ANSI_LBLUE );
            case 15: return ( ANSI_WHITE );
            default: return ( ANSI_RESET );
         }

      case 2: /* ANSI Blinking */
         switch( number_range( 1, 14 ) )
         {
            case 1:  return ( BLINK_DGREEN );
            case 2:  return ( BLINK_ORANGE );
            case 3:  return ( BLINK_DBLUE );
            case 4:  return ( BLINK_PURPLE );
            case 5:  return ( BLINK_CYAN );
            case 6:  return ( BLINK_GRAY );
            case 7:  return ( BLINK_DGRAY );
            case 8:  return ( BLINK_RED );
            case 9:  return ( BLINK_GREEN );
            case 10: return ( BLINK_YELLOW );
            case 11: return ( BLINK_BLUE );
            case 12: return ( BLINK_PINK );
            case 13: return ( BLINK_LBLUE );
            default:
            case 14: return ( BLINK_WHITE );
         }

      case 3: /* ANSI Background */
         switch( number_range( 1, 15 ) )
         {
            case 1:  return ( BACK_DRED );
            case 2:  return ( BACK_DGREEN );
            case 3:  return ( BACK_ORANGE );
            case 4:  return ( BACK_DBLUE );
            case 5:  return ( BACK_PURPLE );
            case 6:  return ( BACK_CYAN );
            case 7:  return ( BACK_GRAY );
            case 8:  return ( BACK_DGRAY );
            case 9:  return ( BACK_RED );
            case 10: return ( BACK_GREEN );
            case 11: return ( BACK_YELLOW );
            case 12: return ( BACK_BLUE );
            case 13: return ( BACK_PINK );
            case 14: return ( BACK_LBLUE );
            default:
            case 15: return ( BACK_WHITE );
         }
   }
}

/*
 * Quixadhal - I rewrote this from scratch.  It now returns the number of
 * characters in the SOURCE string that should be skipped, it always fills
 * the DESTINATION string with a valid translation (even if that is itself,
 * or an empty string), and the default for ANSI is false, since mobs and
 * logfiles shouldn't need colour.
 *
 * NOTE:  dstlen is the length of your pre-allocated buffer that you passed
 * in.  It must be at least 3 bytes, but should be long enough to hold the
 * longest translation sequence (probably around 16-32).
 *
 * NOTE:  vislen is the "visible" length of the translation token.  This is
 * used by color_strlen to properly figure the visual length of a string.
 * If you need the number of bytes (such as for output buffering), use the
 * normal strlen function.
 */
int colorcode( const char *src, char *dst, DESCRIPTOR_DATA *d, int dstlen, int *vislen )
{
   CHAR_DATA *ch = NULL;
   bool ansi = false;
   const char *sympos = NULL;

   /* No descriptor, assume ANSI conversion can't be done. */
   if( !d )
      ansi = false;
   /* But, if we have one, check for a PC and set accordingly. If no PC, assume ANSI can be done. For color logins. */
   else
   {
      ch = d->character;

      if( ch )
         ansi = ( !is_npc( ch ) && xIS_SET( ch->act, PLR_ANSI ) );
      else
         ansi = true;
   }

   if( !dst )
      return 0;   /* HEY, I said at least 3 BYTES! */

   dst[0] = '\0'; /* Initialize the the default NOTHING */

   /* Move along, nothing to see here */
   if( !src || !*src )
      return 0;

   switch( *src )
   {
      case '&':  /* NORMAL, Foreground colour */
         switch( src[1] )
         {
            case '&':  /* Escaped self, return one of us */
               dst[0] = src[0];
               dst[1] = '\0';
               if( vislen )
                  *vislen = 1;
               return 2;

            case 'Z':  /* Random Ansi Foreground */
               if( ansi )
                  mudstrlcpy( dst, random_ansi( 1 ), dstlen );
               break;

            case '[':  /* Symbolic color name */
               if( ( sympos = strchr( src + 2, ']' ) ) )
               {
                  register int subcnt = 0;
                  unsigned int sublen = 0;

                  sublen = sympos - src - 2;
                  for( subcnt = 0; subcnt < MAX_COLORS; subcnt++ )
                  {
                     if( !strncmp( src + 2, pc_displays[subcnt], sublen ) )
                     {
                        if( strlen( pc_displays[subcnt] ) == sublen )
                        {
                           /*
                            * These can only be used with a logged in char 
                            */
                           if( ansi && ch )
                              mudstrlcpy( dst, color_str( subcnt, ch ), dstlen );
                           if( vislen )
                              *vislen = 0;
                           return sublen + 3;
                        }
                     }
                  }
               }  /* found matching ] */

               /*
                * Unknown symbolic name, return just the sequence  
                */
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = '\0';
               if( vislen )
                  *vislen = 2;
               return 2;

            case 'i':  /* Italic text */
            case 'I':
               if( ansi )
                  mudstrlcpy( dst, ANSI_ITALIC, dstlen );
               break;

            case 'v':  /* Reverse colors */
            case 'V':
               if( ansi )
                  mudstrlcpy( dst, ANSI_REVERSE, dstlen );
               break;

            case 'u':  /* Underline */
            case 'U':
               if( ansi )
                  mudstrlcpy( dst, ANSI_UNDERLINE, dstlen );
               break;

            case 's':  /* Strikeover */
            case 'S':
               if( ansi )
                  mudstrlcpy( dst, ANSI_STRIKEOUT, dstlen );
               break;

            case 'd':  /* Player's client default color */
               if( ansi )
                  mudstrlcpy( dst, ANSI_RESET, dstlen );
               break;

            case 'D':  /* Reset to custom color for whatever is being displayed */
               if( ansi )
               {
                  /* Yes, this reset here is quite necessary to cancel out other things */
                  mudstrlcpy( dst, ANSI_RESET, dstlen );
                  if( ch && ch->desc )
                     mudstrlcat( dst, color_str( ch->desc->pagecolor, ch ), dstlen );
               }
               break;

            case 'x':  /* Black */
               if( ansi )
                  mudstrlcpy( dst, ANSI_BLACK, dstlen );
               break;

            case 'O':  /* Orange/Brown */
               if( ansi )
                  mudstrlcpy( dst, ANSI_ORANGE, dstlen );
               break;

            case 'c':  /* Cyan */
               if( ansi )
                  mudstrlcpy( dst, ANSI_CYAN, dstlen );
               break;

            case 'z':  /* Dark Gray */
               if( ansi )
                  mudstrlcpy( dst, ANSI_DGRAY, dstlen );
               break;

            case 'g':  /* Dark Green */
               if( ansi )
                  mudstrlcpy( dst, ANSI_DGREEN, dstlen );
               break;

            case 'G':  /* Light Green */
               if( ansi )
                  mudstrlcpy( dst, ANSI_GREEN, dstlen );
               break;

            case 'P':  /* Pink/Light Purple */
               if( ansi )
                  mudstrlcpy( dst, ANSI_PINK, dstlen );
               break;

            case 'r':  /* Dark Red */
               if( ansi )
                  mudstrlcpy( dst, ANSI_DRED, dstlen );
               break;

            case 'b':  /* Dark Blue */
               if( ansi )
                  mudstrlcpy( dst, ANSI_DBLUE, dstlen );
               break;

            case 'w':  /* Gray */
               if( ansi )
                  mudstrlcpy( dst, ANSI_GRAY, dstlen );
               break;

            case 'Y':  /* Yellow */
               if( ansi )
                  mudstrlcpy( dst, ANSI_YELLOW, dstlen );
               break;

            case 'C':  /* Light Blue */
               if( ansi )
                  mudstrlcpy( dst, ANSI_LBLUE, dstlen );
               break;

            case 'p':  /* Purple */
               if( ansi )
                  mudstrlcpy( dst, ANSI_PURPLE, dstlen );
               break;

            case 'R':  /* Red */
               if( ansi )
                  mudstrlcpy( dst, ANSI_RED, dstlen );
               break;

            case 'B':  /* Blue */
               if( ansi )
                  mudstrlcpy( dst, ANSI_BLUE, dstlen );
               break;

            case 'W':  /* White */
               if( ansi )
                  mudstrlcpy( dst, ANSI_WHITE, dstlen );
               break;

            default:   /* Unknown sequence, return all the chars */
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = '\0';
               if( vislen )
                  *vislen = 2;
               return 2;
         }
         break;

      case '^':  /* BACKGROUND colour */
         switch( src[1] )
         {
            case '^':  /* Escaped self, return one of us */
               dst[0] = src[0];
               dst[1] = '\0';
               if( vislen )
                  *vislen = 1;
               return 2;

            case 'Z':  /* Random Ansi Background */
               if( ansi )
                  mudstrlcpy( dst, random_ansi( 3 ), dstlen );
               break;

            case 'x':  /* Black */
               if( ansi )
                  mudstrlcpy( dst, BACK_BLACK, dstlen );
               break;

            case 'r':  /* Dark Red */
               if( ansi )
                  mudstrlcpy( dst, BACK_DRED, dstlen );
               break;

            case 'g':  /* Dark Green */
               if( ansi )
                  mudstrlcpy( dst, BACK_DGREEN, dstlen );
               break;

            case 'O':  /* Orange/Brown */
               if( ansi )
                  mudstrlcpy( dst, BACK_ORANGE, dstlen );
               break;

            case 'b':  /* Dark Blue */
               if( ansi )
                  mudstrlcpy( dst, BACK_DBLUE, dstlen );
               break;

            case 'p':  /* Purple */
               if( ansi )
                  mudstrlcpy( dst, BACK_PURPLE, dstlen );
               break;

            case 'c':  /* Cyan */
               if( ansi )
                  mudstrlcpy( dst, BACK_CYAN, dstlen );
               break;

            case 'w':  /* Gray */
               if( ansi )
                  mudstrlcpy( dst, BACK_GRAY, dstlen );
               break;

            case 'z':  /* Dark Gray */
               if( ansi )
                  mudstrlcpy( dst, BACK_DGRAY, dstlen );
               break;

            case 'R':  /* Red */
               if( ansi )
                  mudstrlcpy( dst, BACK_RED, dstlen );
               break;

            case 'G':  /* Green */
               if( ansi )
                  mudstrlcpy( dst, BACK_GREEN, dstlen );
               break;

            case 'Y':  /* Yellow */
               if( ansi )
                  mudstrlcpy( dst, BACK_YELLOW, dstlen );
               break;

            case 'B':  /* Blue */
               if( ansi )
                  mudstrlcpy( dst, BACK_BLUE, dstlen );
               break;

            case 'P':  /* Pink */
               if( ansi )
                  mudstrlcpy( dst, BACK_PINK, dstlen );
               break;

            case 'C':  /* Light Blue */
               if( ansi )
                  mudstrlcpy( dst, BACK_LBLUE, dstlen );
               break;

            case 'W':  /* White */
               if( ansi )
                  mudstrlcpy( dst, BACK_WHITE, dstlen );
               break;

            default:   /* Unknown sequence, return all the chars */
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = '\0';
               if( vislen )
                  *vislen = 2;
               return 2;
         }
         break;

      case '}':  /* BLINK Foreground colour */
         switch( src[1] )
         {
            case '}':  /* Escaped self, return one of us */
               dst[0] = src[0];
               dst[1] = '\0';
               if( vislen )
                  *vislen = 1;
               return 2;

            case 'Z':  /* Random Ansi Blink */
               if( ansi )
                  mudstrlcpy( dst, random_ansi( 2 ), dstlen );
               break;

            case 'x':  /* Black */
               if( ansi )
                  mudstrlcpy( dst, BLINK_BLACK, dstlen );
               break;

            case 'O':  /* Orange/Brown */
               if( ansi )
                  mudstrlcpy( dst, BLINK_ORANGE, dstlen );
               break;

            case 'c':  /* Cyan */
               if( ansi )
                  mudstrlcpy( dst, BLINK_CYAN, dstlen );
               break;

            case 'z':  /* Dark Gray */
               if( ansi )
                  mudstrlcpy( dst, BLINK_DGRAY, dstlen );
               break;

            case 'g':  /* Dark Green */
               if( ansi )
                  mudstrlcpy( dst, BLINK_DGREEN, dstlen );
               break;

            case 'G':  /* Light Green */
               if( ansi )
                  mudstrlcpy( dst, BLINK_GREEN, dstlen );
               break;

            case 'P':  /* Pink/Light Purple */
               if( ansi )
                  mudstrlcpy( dst, BLINK_PINK, dstlen );
               break;

            case 'r':  /* Dark Red */
               if( ansi )
                  mudstrlcpy( dst, BLINK_DRED, dstlen );
               break;

            case 'b':  /* Dark Blue */
               if( ansi )
                  mudstrlcpy( dst, BLINK_DBLUE, dstlen );
               break;

            case 'w':  /* Gray */
               if( ansi )
                  mudstrlcpy( dst, BLINK_GRAY, dstlen );
               break;

            case 'Y':  /* Yellow */
               if( ansi )
                  mudstrlcpy( dst, BLINK_YELLOW, dstlen );
               break;

            case 'C':  /* Light Blue */
               if( ansi )
                  mudstrlcpy( dst, BLINK_LBLUE, dstlen );
               break;

            case 'p':  /* Purple */
               if( ansi )
                  mudstrlcpy( dst, BLINK_PURPLE, dstlen );
               break;

            case 'R':  /* Red */
               if( ansi )
                  mudstrlcpy( dst, BLINK_RED, dstlen );
               break;

            case 'B':  /* Blue */
               if( ansi )
                  mudstrlcpy( dst, BLINK_BLUE, dstlen );
               break;

            case 'W':  /* White */
               if( ansi )
                  mudstrlcpy( dst, BLINK_WHITE, dstlen );
               break;

            default:   /* Unknown sequence, return all the chars */
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = '\0';
               if( vislen )
                  *vislen = 2;
               return 2;
         }
         break;

      default:   /* Just a normal character */
         dst[0] = *src;
         dst[1] = '\0';
         if( vislen )
            *vislen = 1;
         return 1;
   }
   if( vislen )
      *vislen = 0;
   return 2;
}

/*
 * Quixadhal - I rewrote this too, so that it uses colorcode.  It may not
 * be as efficient as just walking over the string and counting, but it
 * keeps us from duplicating the code several times.
 *
 * This function returns the intended screen length of a string which has
 * color codes embedded in it.  It does this by stripping the codes out
 * entirely (A NULL descriptor means ANSI will be false).
 */
int color_strlen( const char *src )
{
   register unsigned int i = 0;
   int len = 0;

   if( !src || !*src )  /* Move along, nothing to see here */
      return 0;

   for( i = 0; i < strlen( src ); )
   {
      char dst[20];
      int vislen;

      switch( src[i] )
      {
         case '&':  /* NORMAL, Foreground colour */
         case '^':  /* BACKGROUND colour */
         case '}':  /* BLINK Foreground colour */
            *dst = '\0';
            vislen = 0;
            i += colorcode( &src[i], dst, NULL, 20, &vislen ); /* Skip input token */
            len += vislen; /* Count output token length */
            break;   /* this was missing - if you have issues, remove it */

         default:   /* No conversion, just count */
            ++len;
            ++i;
            break;
      }
   }
   return len;
}

/* Quixadhal - And this one needs to use the new version too. */
char *color_align( const char *argument, int size, int align )
{
   int space = 0;
   int len = 0;
   static char buf[MSL];

   len = color_strlen( argument );
   space = ( size - len );
   if( align == ALIGN_RIGHT || len >= size )
      snprintf( buf, sizeof( buf ), "%*.*s", len, len, argument );
   else if( align == ALIGN_CENTER )
      snprintf( buf, sizeof( buf ), "%*s%s%*s", ( space / 2 ), "", argument,
                ( ( space / 2 ) * 2 ) == space ? ( space / 2 ) : ( ( space / 2 ) + 1 ), "" );
   else if( align == ALIGN_LEFT )
      snprintf( buf, sizeof( buf ), "%s%*s", argument, space, "" );

   return buf;
}

/*
 * Quixadhal - This takes a string and converts any and all color tokens
 * in it to the desired output tokens, using the provided character's
 * preferences.
 */
char *colorize( const char *txt, DESCRIPTOR_DATA *d )
{
   static char result[MSL*2];

   *result = '\0';
   if( txt && *txt && d )
   {
      const char *colstr;
      const char *prevstr = txt;
      char colbuf[20];
      int ln;

      while( ( colstr = strpbrk( prevstr, "&^}h" ) ) )
      {
         register int reslen = 0;

         if( colstr > prevstr )
         {
            if( ( (MSL*2) - ( reslen = strlen( result ) ) ) <= ( colstr - prevstr ) )
            {
               bug( "%s: OVERFLOW in internal (MSL * 2) buffer!", __PRETTY_FUNCTION__ );
               break;
            }
            strncat( result, prevstr, ( colstr - prevstr ) );  /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
            result[reslen + ( colstr - prevstr )] = '\0';   /* strncat WON'T NULL terminate this! */
         }
         if( colstr[0] == 'h' && colstr[1] == 't' && colstr[2] == 't' && colstr[3] == 'p' )
         {
            char http[MIL];

            one_argument( ( char * )colstr, http );
            mudstrlcat( result, http, sizeof( result ) );
            ln = strlen( http );
            prevstr = colstr + ln;
            continue;
         }
         ln = colorcode( colstr, colbuf, d, 20, NULL );
         if( ln > 0 )
         {
            mudstrlcat( result, colbuf, sizeof( result ) );
            prevstr = colstr + ln;
         }
         else
            prevstr = colstr + 1;
      }
      if( *prevstr )
         mudstrlcat( result, prevstr, sizeof( result ) );
   }
   return result;
}

/* Moved from comm.c */
void set_char_color( short AType, CHAR_DATA *ch )
{
   if( !ch || !ch->desc )
      return;

   if( is_npc( ch ) )
      return;

   send_to_char( color_str( AType, ch ), ch );
   if( !ch->desc )
   {
      bug( "%s: NULL descriptor after send_to_char! CH: %s", __FUNCTION__, ch->name ? ch->name : "Unknown?!?" );
      return;
   }
   ch->desc->pagecolor = ch->colors[AType];
}

void write_to_pager( DESCRIPTOR_DATA *d, const char *txt, unsigned int length )
{
   int pageroffset;  /* Pager fix by thoric */

   if( length <= 0 )
      length = strlen( txt );

   if( length == 0 )
      return;

   if( !d->pagebuf )
   {
      d->pagesize = MSL;
      CREATE( d->pagebuf, char, d->pagesize );
   }
   if( !d->pagepoint )
   {
      d->pagepoint = d->pagebuf;
      d->pagetop = 0;
      d->pagecmd = '\0';
   }
   if( d->pagetop == 0 && !d->fcommand )
   {
      d->pagebuf[0] = '\r';
      d->pagebuf[1] = '\n';
      d->pagetop = 2;
   }
   pageroffset = d->pagepoint - d->pagebuf;  /* pager fix (goofup fixed 08/21/97) */
   while( d->pagetop + length >= d->pagesize )
   {
      if( d->pagesize > MSL * 16 )
      {
         bug( "%s: Pager overflow. Ignoring.", __FUNCTION__ );
         d->pagetop = 0;
         d->pagepoint = NULL;
         DISPOSE( d->pagebuf );
         d->pagesize = MSL;
         return;
      }
      d->pagesize *= 2;
      RECREATE( d->pagebuf, char, d->pagesize );
   }
   d->pagepoint = d->pagebuf + pageroffset;  /* pager fix (goofup fixed 08/21/97) */
   strncpy( d->pagebuf + d->pagetop, txt, length );   /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
   d->pagetop += length;
   d->pagebuf[d->pagetop] = '\0';
}

void set_pager_color( short AType, CHAR_DATA *ch )
{
   if( !ch || !ch->desc )
      return;

   if( is_npc( ch ) )
      return;

   send_to_pager( color_str( AType, ch ), ch );
   if( !ch->desc )
   {
      bug( "%s: NULL descriptor after send_to_pager! CH: %s", __FUNCTION__, ch->name ? ch->name : "Unknown?!?" );
      return;
   }
   ch->desc->pagecolor = ch->colors[AType];
}

/* Writes to a descriptor, usually best used when there's no character to send to ( like logins ) */
void send_to_desc( const char *txt, DESCRIPTOR_DATA *d )
{
   if( !d )
   {
      bug( "%s: NULL *d", __FUNCTION__ );
      return;
   }

   if( !txt || !d->descriptor )
      return;

   write_to_buffer( d, colorize( txt, d ), 0 );
}

/*
 * Write to one char. Convert color into ANSI sequences.
 */
void send_to_char( const char *txt, CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s", "send_to_char: NULL ch!" );
      return;
   }

   if( txt && ch->desc )
      send_to_desc( txt, ch->desc );
}

void send_to_pager( const char *txt, CHAR_DATA *ch )
{
   if( !ch )
   {
      bug( "%s", "send_to_pager: NULL ch!" );
      return;
   }

   if( txt && ch->desc )
   {
      DESCRIPTOR_DATA *d = ch->desc;

      ch = d->character;
      if( is_npc( ch ) || !xIS_SET( ch->pcdata->flags, PCFLAG_PAGERON ) )
      {
         if( ch->desc )
            send_to_desc( txt, ch->desc );
      }
      else
      {
         if( ch->desc )
            write_to_pager( ch->desc, colorize( txt, ch->desc ), 0 );
      }
   }
}

void ch_printf( CHAR_DATA *ch, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   send_to_char( buf, ch );
}

/* Made to handle the act and to set the reply trigger */
void act_tell( CHAR_DATA *to, CHAR_DATA *teller, const char *format, CHAR_DATA *ch, void *arg1, void *arg2, int type )
{
   act( AT_TELL, format, ch, arg1, arg2, type );
   to->reply = teller;
}

void act_printf( short AType, CHAR_DATA *ch, void *arg1, void *arg2, int type, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   act( AType, buf, ch, arg1, arg2, type );
}

/* Sends to all playing characters except ch */
void playing_printf( CHAR_DATA *ch, const char *fmt, ... )
{
   DESCRIPTOR_DATA *d;
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   for( d = first_descriptor; d; d = d->next )
      if( d->connected == CON_PLAYING && d->character != ch )
         send_to_char( buf, d->character );
}

void pager_printf( CHAR_DATA *ch, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   send_to_pager( buf, ch );
}

/*
 * The primary output interface for formatted output.
 */
/* Major overhaul. -- Alty */
void ch_printf_color( CHAR_DATA *ch, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   send_to_char( buf, ch );
}

void pager_printf_color( CHAR_DATA *ch, char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   send_to_pager( buf, ch );
}

void paint( short AType, CHAR_DATA *ch, const char *fmt, ... )
{
   char buf[MSL * 2];   /* better safe than sorry */
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   set_char_color( AType, ch );
   send_to_char( buf, ch );
   set_char_color( AType, ch );
}

/* Wrapper function for any "legacy" code that may be installed later */
void send_to_char_color( const char *txt, CHAR_DATA *ch )
{
   send_to_char( txt, ch );
}

/* Wrapper function for any "legacy" code that may be installed later */
void send_to_pager_color( const char *txt, CHAR_DATA *ch )
{
   send_to_pager( txt, ch );
}
