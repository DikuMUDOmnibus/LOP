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

void update_calendar( void );

TIME_INFO_DATA time_info;

typedef struct day_data DAY_DATA;
struct day_data
{
   DAY_DATA *next, *prev;
   char *name;
};

typedef struct month_data MONTH_DATA;
struct month_data
{
   MONTH_DATA *next, *prev;
   char *name;
   short days; /* Number of days this month */
};

typedef struct holiday_data HOLIDAY_DATA;
struct holiday_data
{
   HOLIDAY_DATA *next, *prev;
   char *name;    /* Name of holiday */
   char *message; /* Holiday message */
   short month;   /* Month the holiday is in */
   short date;    /* Date the holiday is on */
};

DAY_DATA *first_day, *last_day;
MONTH_DATA *first_month, *last_month;
HOLIDAY_DATA *first_holiday, *last_holiday;

const char *get_day( int find )
{
   DAY_DATA *day;
   int count = 0;

   for( day = first_day; day; day = day->next )
   {
      if( ++count == find )
         return (day && day->name && day->name[0] != '\0') ? day->name : "Unknown";
   }
   return "Unknown";
}

const char *get_month( int find )
{
   MONTH_DATA *month;
   int count = 0;

   for( month = first_month; month; month = month->next )
   {
      if( ++count == find )
         return (month && month->name && month->name[0] != '\0') ? month->name : "Unknown";
   }
   return "Unknown";
}

DAY_DATA *get_ddata( int find )
{
   DAY_DATA *day;
   int count = 0;

   for( day = first_day; day; day = day->next )
   {
      if( ++count == find )
         return day;
   }
   return NULL;
}

MONTH_DATA *get_mdata( int find )
{
   MONTH_DATA *month;
   int count = 0;

   for( month = first_month; month; month = month->next )
   {
      if( ++count == find )
         return month;
   }
   return NULL;
}

HOLIDAY_DATA *is_a_holiday( int month, int date )
{
   HOLIDAY_DATA *holiday;

   for( holiday = first_holiday; holiday; holiday = holiday->next )
   {
      if( holiday->month == month && holiday->date == date )
         return holiday;
   }
   return NULL;
}

void save_timeinfo( void )
{
   FILE *fp;

   if( !( fp = fopen( TIMEINFO_FILE, "w" ) ) )
   {
      bug( "%s: couldn't open %s for writing", __FUNCTION__, TIMEINFO_FILE );
      perror( TIMEINFO_FILE );
      return;
   }
   fprintf( fp, "Hour  %d\n", time_info.hour );
   fprintf( fp, "WDay  %d\n", time_info.wday );
   fprintf( fp, "Day   %d\n", time_info.day );
   fprintf( fp, "Month %d\n", time_info.month );
   fprintf( fp, "Year  %d\n", time_info.year );
   fprintf( fp, "End\n" );
   fclose( fp );
   fp = NULL;
}

void fread_timeinfo( FILE *fp )
{
   const char *word;
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

         case 'D':
            KEY( "Day", time_info.day, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( time_info.year < 18 )
                  time_info.year = 18;
               return;
            }
            break;

         case 'H':
            KEY( "Hour", time_info.hour, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Month", time_info.month, fread_number( fp ) );
            break;

         case 'W':
            KEY( "WDay", time_info.wday, fread_number( fp ) );
            break;

         case 'Y':
            KEY( "Year", time_info.year, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void update_sunlight( void )
{
   if( time_info.hour < 5 )
      time_info.sunlight = SUN_DARK;
   else if( time_info.hour < 6 )
      time_info.sunlight = SUN_RISE;
   else if( time_info.hour < 19 )
      time_info.sunlight = SUN_LIGHT;
   else if( time_info.hour < 20 )
      time_info.sunlight = SUN_SET;
   else
      time_info.sunlight = SUN_DARK;
}

void load_timeinfo( void )
{
   FILE *fp;

   /* Defaults */
   time_info.hour = 0;
   /* Under the current setup: the 1st day of the 1st month of the 18th year should be 3 */
   time_info.wday = 3;
   time_info.day = 1;
   time_info.month = 1;
   /* Year should probably always be at least 18 since people get a birthday of current day, month, (year - 17) */
   time_info.year = 18;
   update_sunlight( );

   if( !( fp = fopen( TIMEINFO_FILE, "r" ) ) )
   {
      log_printf( "%s: couldn't open %s for reading. Using Defaults.", __FUNCTION__, TIMEINFO_FILE );
      perror( TIMEINFO_FILE );
      return;
   }
   fread_timeinfo( fp );
   update_sunlight( );
   fclose( fp );
   fp = NULL;

   update_calendar( ); /* Go ahead and toss it through to make sure the info is correct on startup */
}

/*
 * get echo messages according to time changes...
 * some echoes depend upon the weather so an echo must be
 * found for each area
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_time_echo( WEATHER_DATA *weath )
{
   int n, pindex;

   n = number_bits( 2 );
   pindex = ( weath->precip + 3 * weath_unit - 1 ) / weath_unit;
   weath->echo = NULL;
   weath->echo_color = AT_GRAY;

   switch( time_info.hour )
   {
      case 5:
      {
         const char *echo_strings[4] =
         {
            "The day has begun.\r\n",
            "The day has begun.\r\n",
            "The sky slowly begins to glow.\r\n",
            "The sun slowly embarks upon a new day.\r\n"
         };
         weath->echo = (char *)echo_strings[n];
         weath->echo_color = AT_YELLOW;
         break;
      }

      case 6:
      {
         const char *echo_strings[4] =
         {
            "The sun rises in the east.\r\n",
            "The sun rises in the east.\r\n",
            "The hazy sun rises over the horizon.\r\n",
            "Day breaks as the sun lifts into the sky.\r\n"
         };
         weath->echo = (char *)echo_strings[n];
         weath->echo_color = AT_ORANGE;
         break;
      }

      case 12:
      {
         if( pindex > 0 )
            weath->echo = (char *)"It's noon.\r\n";
         else
         {
            const char *echo_strings[2] =
            {
               "The intensity of the sun heralds the noon hour.\r\n",
               "The sun's bright rays beat down upon your shoulders.\r\n"
            };
            weath->echo = (char *)echo_strings[n % 2];
         }
         weath->echo_color = AT_WHITE;
         break;
      }

      case 19:
      {
         const char *echo_strings[4] =
         {
            "The sun slowly disappears in the west.\r\n",
            "The reddish sun sets past the horizon.\r\n",
            "The sky turns a reddish orange as the sun ends its journey.\r\n",
            "The sun's radiance dims as it sinks in the sky.\r\n"
         };
         weath->echo = (char *)echo_strings[n];
         weath->echo_color = AT_RED;
         break;
      }

      case 20:
      {
         if( pindex > 0 )
         {
            const char *echo_strings[2] =
            {
               "The night begins.\r\n",
               "Twilight descends around you.\r\n"
            };
            weath->echo = (char *)echo_strings[n % 2];
         }
         else
         {
            const char *echo_strings[2] =
            {
               "The moon's gentle glow diffuses through the night sky.\r\n",
               "The night sky gleams with glittering starlight.\r\n"
            };
            weath->echo = (char *)echo_strings[n % 2];
         }
         weath->echo_color = AT_DBLUE;
         break;
      }
   }
}

char *show_holiday( void )
{
   HOLIDAY_DATA *holiday;

   if( ( holiday = is_a_holiday( time_info.month, time_info.day ) ) && holiday->message )
      return holiday->message;
   return NULL;
}

/* update the time */
void time_update( void )
{
   AREA_DATA *pArea;
   DESCRIPTOR_DATA *d;
   WEATHER_DATA *weath;
   MONTH_DATA *month;
   DAY_DATA *day;
   char *hmessage;
   int mday = 0, mmonth = 0;

   switch( ++time_info.hour )
   {
      case 5:
      case 6:
      case 12:
      case 19:
      case 20:
         for( pArea = first_area; pArea; pArea = ( pArea == last_area ) ? first_build : pArea->next )
            get_time_echo( pArea->weather );

         for( d = first_descriptor; d; d = d->next )
         {
            if( d->connected == CON_PLAYING && is_outside( d->character ) && is_awake( d->character ) )
            {
               weath = d->character->in_room->area->weather;
               if( !weath->echo )
                  continue;
               set_char_color( weath->echo_color, d->character );
               send_to_char( weath->echo, d->character );
            }
         }
         break;

      case 24:
         time_info.hour = 0;
         time_info.day++;
         time_info.wday++;
         if( ( hmessage = show_holiday( ) ) )
         {
            for( d = first_descriptor; d; d = d->next )
            {
               if( d->connected == CON_PLAYING )
                  ch_printf( d->character, "%s\r\n", hmessage );
            }
         }
         break;
   }

   update_sunlight( );

   if( !( month = get_mdata( time_info.month ) ) )
   {
      bug( "%s: invalid month!!!!", __FUNCTION__ );
      return;
   }

   for( day = first_day; day; day = day->next )
      mday++;

   if( time_info.wday > mday )
      time_info.wday = 1;

   if( time_info.day > month->days )
   {
      time_info.day = 1;
      time_info.month++;
      give_interest( );
   }

   for( month = first_month; month; month = month->next )
      mmonth++;
 
   if( time_info.month > mmonth )
   {
      time_info.month = 1;
      time_info.year++;
   }

   /* Shouldn't be to bad to save it each update */
   save_timeinfo( );
}

const char *number_suffix( int num )
{
   if( num > 4 && num < 20 )
      return "th";
   if( num % 10 == 1 )
      return "st";
   if( num % 10 == 2 )
      return "nd";
   if( num % 10 == 3 )
      return "rd";
   return "th";
}

CMDF( do_time )
{
   int day, hour;
   bool pm = false;

   hour = time_info.hour;
   if( hour >= 12 )
   {
      hour -= 12;
      pm = true;
   }
   if( hour == 0 )
      hour = 12;

   day = time_info.day;

   set_char_color( AT_YELLOW, ch );
   ch_printf( ch, "It is %d%s, Day of %s, the %d%s of %s, in the %d%s year.\r\n",
      hour, pm ? "PM" : "AM", get_day(time_info.wday), day, number_suffix( day ),
      get_month(time_info.month), time_info.year, number_suffix( time_info.year ) );
   ch_printf( ch, "The mud started up at: %s (EST)\r\n", str_boot_time );
   ch_printf( ch, "The system time:       %s (EST)\r\n", distime( current_time ) );
   ch_printf( ch, "Next pfile cleanup:    %s (EST)\r\n", distime( sysdata.next_pfile_cleanup ) );
}

char *show_birthday( CHAR_DATA *ch )
{
   static char buf[MSL];

   if( ch->pcdata->birth_month == time_info.month && ch->pcdata->birth_day == time_info.day )
      snprintf( buf, sizeof( buf ), "%s", "Your birthday is today." );
   else
      snprintf( buf, sizeof( buf ), "Your birthday is the %d%s of %s, in the %d%s year.",
         ch->pcdata->birth_day, number_suffix( ch->pcdata->birth_day ),
         get_month(ch->pcdata->birth_month), ch->pcdata->birth_year, number_suffix( ch->pcdata->birth_year ) );
   return buf;
}

void save_calendarinfo( void )
{
   FILE *fp;
   MONTH_DATA *month;
   DAY_DATA *day;
   HOLIDAY_DATA *holiday;

   if( !( fp = fopen( CALENDARINFO_FILE, "w" ) ) )
   {
      bug( "%s: couldn't open %s for writing", __FUNCTION__, CALENDARINFO_FILE );
      perror( CALENDARINFO_FILE );
      return;
   }

   for( day = first_day; day; day = day->next )
   {
      fprintf( fp, "%s", "#DAY\n" );
      fprintf( fp, "Name    %s~\n", day->name );
      fprintf( fp, "%s", "End\n" );
   }

   for( month = first_month; month; month = month->next )
   {
      fprintf( fp, "%s", "#MONTH\n" );
      fprintf( fp, "Name    %s~\n", month->name );
      fprintf( fp, "Days    %d\n", month->days );
      fprintf( fp, "%s", "End\n" );
   }

   for( holiday = first_holiday; holiday; holiday = holiday->next )
   {
      fprintf( fp, "%s", "#HOLIDAY\n" );
      fprintf( fp, "Name    %s~\n", holiday->name );
      if( holiday->message )
         fprintf( fp, "Message %s~\n", holiday->message );
      fprintf( fp, "Month   %d\n", holiday->month );
      fprintf( fp, "Date    %d\n", holiday->date );
      fprintf( fp, "%s", "End\n" );
   }

   fprintf( fp, "#END\n" );
   fclose( fp );
   fp = NULL;
}

HOLIDAY_DATA *find_holiday( char *name )
{
   HOLIDAY_DATA *holiday;

   if( !name || name[0] == '\0' )
      return NULL;
   for( holiday = first_holiday; holiday; holiday = holiday->next )
   {
      if( !str_cmp( holiday->name, name ) )
         return holiday;
   }
   return NULL;
}

DAY_DATA *find_day( char *name )
{
   DAY_DATA *day;

   if( !name || name[0] == '\0' )
      return NULL;
   for( day = first_day; day; day = day->next )
   {
      if( !str_cmp( day->name, name ) )
         return day;
   }
   return NULL;
}

MONTH_DATA *find_month( char *name )
{
   MONTH_DATA *month;

   if( !name || name[0] == '\0' )
      return NULL;
   for( month = first_month; month; month = month->next )
   {
      if( !str_cmp( month->name, name ) )
         return month;
   }
   return NULL;
}

void free_holiday( HOLIDAY_DATA *holiday )
{
   if( !holiday )
      return;
   STRFREE( holiday->name );
   STRFREE( holiday->message );
   DISPOSE( holiday );
}

void remove_holiday( HOLIDAY_DATA *holiday )
{
   if( !holiday )
      return;
   UNLINK( holiday, first_holiday, last_holiday, next, prev );
}

void free_day( DAY_DATA *day )
{
   if( !day )
      return;
   STRFREE( day->name );
   DISPOSE( day );
}

void remove_day( DAY_DATA *day )
{
   if( !day )
      return;
   UNLINK( day, first_day, last_day, next, prev );
}

void free_month( MONTH_DATA *month )
{
   if( !month )
      return;
   STRFREE( month->name );
   DISPOSE( month );
}

void remove_month( MONTH_DATA *month )
{
   if( !month )
      return;
   UNLINK( month, first_month, last_month, next, prev );
}

void free_all_calendarinfo( void )
{
   DAY_DATA *day, *day_next;
   MONTH_DATA *month, *month_next;
   HOLIDAY_DATA *holiday, *holiday_next;

   for( day = first_day; day; day = day_next )
   {
      day_next = day->next;
      remove_day( day );
      free_day( day );
   }

   for( month = first_month; month; month = month_next )
   {
      month_next = month->next;
      remove_month( month );
      free_month( month );
   }

   for( holiday = first_holiday; holiday; holiday = holiday_next )
   {
      holiday_next = holiday->next;
      remove_holiday( holiday );
      free_holiday( holiday );
   }
}

void add_holiday( HOLIDAY_DATA *holiday )
{
   if( !holiday )
      return;
   LINK( holiday, first_holiday, last_holiday, next, prev );
}

void add_day( DAY_DATA *day )
{
   if( !day )
      return;
   LINK( day, first_day, last_day, next, prev );
}

void add_month( MONTH_DATA *month )
{
   if( !month )
      return;
   LINK( month, first_month, last_month, next, prev );
}

void fread_holiday( FILE *fp )
{
   HOLIDAY_DATA *holiday;
   const char *word;
   bool fMatch;

   /* Setup defaults */
   CREATE( holiday, HOLIDAY_DATA, 1 );
   holiday->name = NULL;
   holiday->message = NULL;
   holiday->date = 0;
   holiday->month = 0;

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

         case 'D':
            KEY( "Date", holiday->date, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               add_holiday( holiday );
               return;
            }
            break;

         case 'M':
            KEY( "Message", holiday->message, fread_string( fp ) );
            KEY( "Month", holiday->month, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", holiday->name, fread_string( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_holiday( holiday );
}

void fread_monthinfo( FILE *fp )
{
   MONTH_DATA *month;
   const char *word;
   bool fMatch;

   /* Setup defaults */
   CREATE( month, MONTH_DATA, 1 );
   month->name = NULL;
   month->days = 31;

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

         case 'D':
            KEY( "Days", month->days, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               add_month( month );
               return;
            }
            break;

         case 'N':
            KEY( "Name", month->name, fread_string( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_month( month );
}

void fread_dayinfo( FILE *fp )
{
   DAY_DATA *day;
   const char *word;
   bool fMatch;

   /* Setup defaults */
   CREATE( day, DAY_DATA, 1 );
   day->name = NULL;

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

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               add_day( day );
               return;
            }
            break;

         case 'N':
            KEY( "Name", day->name, fread_string( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_day( day );
}

void load_calendarinfo( void )
{
   FILE *fp;

   if( !( fp = fopen( CALENDARINFO_FILE, "r" ) ) )
   {
      log_printf( "%s: couldn't open %s for reading. Using Defaults.", __FUNCTION__, CALENDARINFO_FILE );
      perror( CALENDARINFO_FILE );
      return;
   }

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
         bug( "%s: # not found.", __FUNCTION__ );
         break;
      }
      word = fread_word( fp );
      if( !str_cmp( word, "MONTH" ) )
      {
         fread_monthinfo( fp );
         continue;
      }
      else if( !str_cmp( word, "DAY" ) )
      {
         fread_dayinfo( fp );
         continue;
      }
      else if( !str_cmp( word, "HOLIDAY" ) )
      {
         fread_holiday( fp );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section (%s).", __FUNCTION__, word );
         fread_to_eol( fp );
         continue;
      }
   }

   fclose( fp );
   fp = NULL;
}

/* Auto fix the calendar when changes are made to things */
void update_calendar( void )
{
   DAY_DATA *day;
   MONTH_DATA *month;
   char buf[MSL], tmpbuf[MSL];
   int year = 1, wday = 0, omonth = 1, maxdays = 0, date = 0, cday, hour, swday = 0, pwday;
   bool pm = false, start = true;

   hour = time_info.hour;
   if( hour >= 12 )
   {
      hour -= 12;
      pm = true;
   }
   if( hour == 0 )
      hour = 12;

   cday = time_info.day;

   buf[0] = '\0';
   for( day = first_day; day; day = day->next )
   {
      snprintf( tmpbuf, sizeof( tmpbuf ), "%10.10s |", day->name );
      mudstrlcat( buf, tmpbuf, sizeof( buf ) );
      ++maxdays;
   }

   /* Should keep the year 18 and higher */
   if( time_info.year < 18 )
      time_info.year = 18;

   for( year = 1; year <= time_info.year; year++ )
   {
      omonth = 0;
      for( month = first_month; month; month = month->next )
      {
         omonth++;
         start = true;
         pwday = 0;

         for( date = 1; date <= month->days; date++ )
         {
            ++wday;

            if( year == time_info.year && omonth == time_info.month && date == time_info.day )
            {
               if( wday == time_info.wday && date == cday
               && omonth == time_info.month && year == time_info.year )
               {
                  return;
               }

               time_info.wday = wday;
               time_info.day = date;
               time_info.month = omonth;
               time_info.year = year;
               save_timeinfo( );
               log_printf( "%s: calendar has been updated and saved.", __FUNCTION__ );
               return;
            }
            if( wday >= maxdays )
               wday = 0;
         }
         swday = wday;
      }
   }
}

CMDF( do_timeset )
{
   MONTH_DATA *month, *tmpmonth;
   DAY_DATA *day, *tmpday;
   HOLIDAY_DATA *holiday, *tmpholiday;
   char arg[MIL], arg2[MIL];
   int value = 0, count = 0;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: timeset [day/month/holiday/stats]\r\n", ch );
      send_to_char( "Usage: timeset [day/month/holiday] [create/delete] <name>\r\n", ch );
      send_to_char( "Usage: timeset [day/month/holiday] place <#> <name of day/month/holiday to move>\r\n", ch );
      send_to_char( "Usage: timeset [day/month/holiday] name <name of day/month/holiday to change> <name>\r\n", ch );
      send_to_char( "Usage: timeset holiday [month/date] <#> <name of holiday to change>\r\n", ch );
      send_to_char( "Usage: timeset holiday messsage <name of holiday to change> <new holiday message>\r\n", ch );
      send_to_char( "Usage: timeset month days <#> <name of month to change>\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !str_cmp( arg, "stats" ) )
   {
      int days = 0, months = 0, tdays = 0, holidays = 0;

      for( day = first_day; day; day = day->next )
         days++;
      for( month = first_month; month; month = month->next )
      {
         months++;
         tdays += month->days;
      }
      for( holiday = first_holiday; holiday; holiday = holiday->next )
         holidays++;

      ch_printf( ch, "There are %d mud days a week.\r\n", days );
      ch_printf( ch, "There are %d mud months a year.\r\n", months );
      ch_printf( ch, "There are %d holidays a year.\r\n", holidays );
      ch_printf( ch, "There is a total of %d days a year.\r\n", tdays );
      return;
   }

   if( !str_cmp( arg, "day" ) )
   {
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         bool found = false;

         send_to_char( "Current mud days:\r\n", ch );
         for( day = first_day; day; day = day->next )
         {
            ch_printf( ch, "%20s", day->name );
            found = true;
            if( ++count == 4 )
            {
               count = 0;
               send_to_char( "\r\n", ch );
            }
         }
         if( count != 0 )
            send_to_char( "\r\n", ch );
         if( !found )
            send_to_char( "There are currently no days.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "name" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What day would you like to change the name on?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         if( !( day = find_day( arg2 ) ) )
         {
            send_to_char( "There is no such day using that name.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What name would you like to use for that day?\r\n", ch );
            return;
         }
         if( ( tmpday = find_day( argument ) ) )
         {
            send_to_char( "There is already a day using that name.\r\n", ch );
            return;
         }
         STRSET( day->name, argument );
         save_calendarinfo( );
         send_to_char( "The day name has been changed.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "place" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What place would you like the day moved to?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         value = atoi( arg2 );
         if( value <= 0 )
         {
            send_to_char( "You can't set a days place to 0 or below.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What day would you like to move?\r\n", ch );
            return;
         }
         if( !( day = find_day( argument ) ) )
         {
            send_to_char( "There is no such day using that name.\r\n", ch );
            return;
         }
         for( tmpday = first_day; tmpday; tmpday = tmpday->next )
         {
            if( !str_cmp( tmpday->name, day->name ) )
               continue;
            if( ++count == value )
               break;
         }
         if( !tmpday && count == ( value - 1 ) )
            count = value;
         if( tmpday || count == value )
         {
            remove_day( day );
            if( tmpday )
               INSERT( day, tmpday, first_day, next, prev );
            else
               add_day( day );
         }
         else 
         {
            send_to_char( "Can't insert it there.\r\n", ch );
            return;
         }
         save_calendarinfo( );
         send_to_char( "The day has been moved to the specified place.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "create" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What name would you like the new day to be?\r\n", ch );
            return;
         }
         if( ( day = find_day( argument ) ) )
         {
            send_to_char( "There is already a day using that name.\r\n", ch );
            return;
         }
         CREATE( day, DAY_DATA, 1 );
         if( !day )
         {
            bug( "%s: Failed to create a day in memory.", __FUNCTION__ );
            return;
         }
         STRSET( day->name, argument );
         add_day( day );
         save_calendarinfo( );
         update_calendar( );
         ch_printf( ch, "%s day created.\r\n", day->name );
         return;
      }
      if( !str_cmp( arg2, "delete" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What day would you like to delete?\r\n", ch );
            return;
         }
         if( !( day = find_day( argument ) ) )
         {
            send_to_char( "There is no day using that name.\r\n", ch );
            return;
         }
         remove_day( day );
         free_day( day );
         save_calendarinfo( );
         update_calendar( );
         send_to_char( "The day has been deleted.\r\n", ch );
         return;
      }
   }

   if( !str_cmp( arg, "month" ) )
   {
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         bool found = false;

         send_to_char( "Current mud months:\r\n", ch );
         for( month = first_month; month; month = month->next )
         {
            ch_printf( ch, "%22s[%2d]", month->name, month->days );
            found = true;
            if( ++count == 3 )
            {
               count = 0;
               send_to_char( "\r\n", ch );
            }
         }
         if( count != 0 )
            send_to_char( "\r\n", ch );
         if( !found )
            send_to_char( "There are currently no months.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "name" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What month would you like to change the name on?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         if( !( month = find_month( arg2 ) ) )
         {
            send_to_char( "There is no such month using that name.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What name would you like to use for that month?\r\n", ch );
            return;
         }
         if( ( tmpmonth = find_month( argument ) ) )
         {
            send_to_char( "There is already a month using that name.\r\n", ch );
            return;
         }
         STRSET( month->name, argument );
         save_calendarinfo( );
         send_to_char( "The month name has been changed.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "days" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "How many days would you like the month to have?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         value = atoi( arg2 );
         if( value <= 0 || value > 100 )
         {
            send_to_char( "Days range per month is 1 to 100.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What month would you like to change?\r\n", ch );
            return;
         }
         if( !( month = find_month( argument ) ) )
         {
            send_to_char( "There is no such month using that name.\r\n", ch );
            return;
         }
         month->days = value;
         save_calendarinfo( );
         update_calendar( );
         send_to_char( "The month's days have been changed.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "place" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What place would you like the month moved to?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         value = atoi( arg2 );
         if( value <= 0 )
         {
            send_to_char( "You can't set a months place to 0 or below.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What month would you like to move?\r\n", ch );
            return;
         }
         if( !( month = find_month( argument ) ) )
         {
            send_to_char( "There is no such month using that name.\r\n", ch );
            return;
         }
         for( tmpmonth = first_month; tmpmonth; tmpmonth = tmpmonth->next )
         {
            if( !str_cmp( tmpmonth->name, month->name ) )
               continue;
            if( ++count == value )
               break;
         }
         if( !tmpmonth && count == ( value - 1 ) )
            count = value;
         if( tmpmonth || count == value )
         {
            remove_month( month );
            if( tmpmonth )
               INSERT( month, tmpmonth, first_month, next, prev );
            else
               add_month( month );
         }
         else
         {
            send_to_char( "Can't insert it there.\r\n", ch );
            return;
         }
         save_calendarinfo( );
         send_to_char( "The month has been moved to the specified place.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "create" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What name would you like the new month to be?\r\n", ch );
            return;
         }
         if( ( month = find_month( argument ) ) )
         {
            send_to_char( "There is already a month using that name.\r\n", ch );
            return;
         }
         CREATE( month, MONTH_DATA, 1 );
         if( !month )
         {
            bug( "%s: Failed to create a month in memory.", __FUNCTION__ );
            return;
         }
         STRSET( month->name, argument );
         month->days = 31;
         add_month( month );
         save_calendarinfo( );
         update_calendar( );
         ch_printf( ch, "%s month created and set to 31 days.\r\n", month->name );
         return;
      }

      if( !str_cmp( arg2, "delete" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What month would you like to delete?\r\n", ch );
            return;
         }
         if( !( month = find_month( argument ) ) )
         {
            send_to_char( "There is no month using that name.\r\n", ch );
            return;
         }
         remove_month( month );
         free_month( month );
         save_calendarinfo( );
         update_calendar( );
         send_to_char( "The month has been deleted.\r\n", ch );
         return;
      }
   }

   if( !str_cmp( arg, "holiday" ) )
   {
      if( arg2 == NULL || arg2[0] == '\0' )
      {
         send_to_char( "Current holidays:\r\n", ch );
         for( holiday = first_holiday; holiday; holiday = holiday->next )
         {
            count++;
            ch_printf( ch, "%22s[%2d %2d] %s\r\n", holiday->name, holiday->month, holiday->date,
               holiday->message ? holiday->message : "(Not Set)" );
         }
         if( !count )
            send_to_char( "There are currently no holidays.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "name" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What holiday would you like to change the name on?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         if( !( holiday = find_holiday( arg2 ) ) )
         {
            send_to_char( "There is no such holiday using that name.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What name would you like to use for that holiday?\r\n", ch );
            return;
         }
         if( ( tmpholiday = find_holiday( argument ) ) )
         {
            send_to_char( "There is already a holiday using that name.\r\n", ch );
            return;
         }
         STRSET( holiday->name, argument );
         save_calendarinfo( );
         send_to_char( "The holiday name has been changed.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "message" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What holiday would you like to change the message on?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         if( !( holiday = find_holiday( arg2 ) ) )
         {
            send_to_char( "There is no such holiday using that name.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What message would you like to use for that holiday?\r\n", ch );
            return;
         }
         STRSET( holiday->message, argument );
         save_calendarinfo( );
         send_to_char( "The holiday message has been changed.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "date" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What date would you like the holiday to be on?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         value = atoi( arg2 );
         if( value <= 0 || value > 100 )
         {
            send_to_char( "Date range is 1 to 100.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What holiday would you like to change?\r\n", ch );
            return;
         }
         if( !( holiday = find_holiday( argument ) ) )
         {
            send_to_char( "There is no such holiday using that name.\r\n", ch );
            return;
         }
         holiday->date = value;
         save_calendarinfo( );
         send_to_char( "The holiday's date has been changed.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "month" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What month would you like the holiday to be on?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         value = atoi( arg2 );
         if( value <= 0 || value > 100 )
         {
            send_to_char( "Month range is 1 to 100.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What holiday would you like to change?\r\n", ch );
            return;
         }
         if( !( holiday = find_holiday( argument ) ) )
         {
            send_to_char( "There is no such holiday using that name.\r\n", ch );
            return;
         }
         holiday->month = value;
         save_calendarinfo( );
         send_to_char( "The holiday's month has been changed.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "place" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What place would you like the holiday  moved to?\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg2 );
         value = atoi( arg2 );
         if( value <= 0 )
         {
            send_to_char( "You can't set a holidays place to 0 or below.\r\n", ch );
            return;
         }
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What holiday would you like to move?\r\n", ch );
            return;
         }
         if( !( holiday = find_holiday( argument ) ) )
         {
            send_to_char( "There is no such holiday using that name.\r\n", ch );
            return;
         }
         for( tmpholiday = first_holiday; tmpholiday; tmpholiday = tmpholiday->next )
         {
            if( !str_cmp( tmpholiday->name, holiday->name ) )
               continue;
            if( ++count == value )
               break;
         }
         if( !tmpholiday && count == ( value - 1 ) )
            count = value;
         if( tmpholiday || count == value )
         {
            remove_holiday( holiday );
            if( tmpholiday )
               INSERT( holiday, tmpholiday, first_holiday, next, prev );
            else
               add_holiday( holiday );
         }
         else
         {
            send_to_char( "Can't insert it there.\r\n", ch );
            return;
         }
         save_calendarinfo( );
         send_to_char( "The holiday has been moved to the specified place.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "create" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What name would you like the new holiday to be?\r\n", ch );
            return;
         }
         if( ( holiday = find_holiday( argument ) ) )
         {
            send_to_char( "There is already a holiday using that name.\r\n", ch );
            return;
         }
         CREATE( holiday, HOLIDAY_DATA, 1 );
         if( !holiday )
         {
            bug( "%s: Failed to create a holiday in memory.", __FUNCTION__ );
            return;
         }
         STRSET( holiday->name, argument );
         holiday->date = 1;
         holiday->month = 1;
         holiday->message = NULL;
         add_holiday( holiday );
         save_calendarinfo( );
         ch_printf( ch, "%s holiday created.\r\n", holiday->name );
         return;
      }

      if( !str_cmp( arg2, "delete" ) )
      {
         if( !argument || argument[0] == '\0' )
         {
            send_to_char( "What holiday would you like to delete?\r\n", ch );
            return;
         }
         if( !( holiday = find_holiday( argument ) ) )
         {
            send_to_char( "There is no holiday using that name.\r\n", ch );
            return;
         }
         remove_holiday( holiday );
         free_holiday( holiday );
         save_calendarinfo( );
         send_to_char( "The holiday has been deleted.\r\n", ch );
         return;
      }
   }

   do_timeset( ch, (char *)"" );
}

CMDF( do_calendarcheck )
{
   DAY_DATA *day;
   MONTH_DATA *month;
   char buf[MSL], tmpbuf[MSL];
   int year = 1, wday = 0, omonth = 1, maxdays = 0, date = 0, cday, hour, swday = 0, pwday;
   bool pm = false, fix = false, start = true, log = false;

   if( argument && argument[0] != '\0' )
   {
      if( !str_cmp( argument, "fix" ) )
         fix = true;
      else if( !str_cmp( argument, "log" ) )
         log = true;
   }

   hour = time_info.hour;
   if( hour >= 12 )
   {
      hour -= 12;
      pm = true;
   }
   if( hour == 0 )
      hour = 12;

   cday = time_info.day;

   buf[0] = '\0';
   for( day = first_day; day; day = day->next )
   {
      snprintf( tmpbuf, sizeof( tmpbuf ), "%10.10s |", day->name );
      mudstrlcat( buf, tmpbuf, sizeof( buf ) );
      ++maxdays;
   }

   /* Should keep the year 18 and higher */
   if( time_info.year < 18 )
      time_info.year = 18;

   for( year = 1; year <= time_info.year; year++ )
   {
      omonth = 0;
      for( month = first_month; month; month = month->next )
      {
         omonth++;
         start = true;
         pwday = 0;

         if( log )
         {
            fprintf( stderr, "\nMonth of %s in the %d%s year.\n", get_month( omonth ), year, number_suffix( year ) );
            fprintf( stderr, "%s\n", buf );
         }
         for( date = 1; date <= month->days; date++ )
         {
            ++wday;

            if( log )
            {
               if( start )
               {
                  while( swday > 0 )
                  {
                      fprintf( stderr, "%12s", "" );
                      swday--;
                      pwday++;
                  }
                  fprintf( stderr, "%10d  ", date );
                  pwday++;
                  if( pwday >= maxdays )
                  {
                     fprintf( stderr, "%s", "\n" );
                     pwday = 0;
                  }
                  start = false;
               }
               else
               {
                  fprintf( stderr, "%10d  %s", date, ++pwday >= maxdays ? "\n" : "");
                  if( pwday >= maxdays )
                     pwday = 0;
               }
            }
            if( year == time_info.year && omonth == time_info.month && date == time_info.day )
            {
               if( wday == time_info.wday && date == cday
               && omonth == time_info.month && year == time_info.year )
               {
                  send_to_char( "Current mud time is setup and looks like it should.\r\n", ch );
                  return;
               }

               send_to_char( "Current mud time shows:\r\n", ch );
               ch_printf( ch, "It is %d%s, Day of %s, the %d%s of %s, in the %d%s year.\r\n",
                  hour, pm ? "PM" : "AM", get_day(time_info.wday), cday, number_suffix( cday ),
                  get_month(time_info.month), time_info.year, number_suffix( time_info.year ) );

               send_to_char( "\r\nShould show:\r\n", ch );
               ch_printf( ch, "It is %d%s, Day of %s, the %d%s of %s, in the %d%s year.\r\n",
                  hour, pm ? "PM" : "AM", get_day( wday ), date, number_suffix( date ),
                  get_month( omonth ), year, number_suffix( year ) );

               if( fix )
               {
                  time_info.wday = wday;
                  time_info.day = date;
                  time_info.month = omonth;
                  time_info.year = year;
                  save_timeinfo( );
                  send_to_char( "Current mud time updated to look like it should now.\r\n", ch );
               }
               return;
            }
            if( wday >= maxdays )
               wday = 0;
         }
         swday = wday;
      }
   }
}

/*
 * This function does nothing but take the current mud time and
 * show a calendar for the current mud month and year
 * - Remcon
 */
CMDF( do_calendar )
{
   DAY_DATA *ddata;
   MONTH_DATA *mdata;
   HOLIDAY_DATA *hdata = NULL;
   char buf[MSL], tmpbuf[MSL];
   int year = time_info.year;
   int month = time_info.month;
   int day = time_info.day;
   int weekday = time_info.wday;
   int startweekday = weekday;
   int onweekday = -1;
   int onday = 0;
   int countday = 0;
   int maxweekday = 0;
   bool usedstart = false;

   if( !first_day )
   {
      send_to_char( "No day found to try and make a calendar with.\r\n", ch );
      return;
   }

   /* Get maxweekdays and all the names for display */
   buf[0] = '\0';
   for( ddata = first_day; ddata; ddata = ddata->next )
   {
      snprintf( tmpbuf, sizeof( tmpbuf ), "&w%10.10s &z|&D", ddata->name );
      mudstrlcat( buf, tmpbuf, sizeof( buf ) );
      maxweekday++;
   }

   /* First lets fine out what the first weekday should be this month
    * by counting backwards from the current day */
   for( onday = time_info.day; onday > 0; onday-- )
   {
      /* Subtract 1 and if below 0 loop back around to the maxweekday */
      if( --startweekday <= 0 )
      {
         if( onday != 1 )
            startweekday = maxweekday;
      }
   }

   /* Show day month and year */
   ch_printf( ch, "\r\n&zThe Day of &w%s&z, the &w%d&z%s of &w%s&z, in the &w%d&z%s year.&D\r\n",
      get_day( weekday ), day, number_suffix( day ), get_month( month ), year, number_suffix( year ) );

   /* Show a list of the days of the week */
   send_to_char( buf, ch );

   /* Finish off the days of the week with new line */
   send_to_char( "\r\n", ch );

   /* Get the month data */
   if( !( mdata = get_mdata( month ) ) )
   {
      send_to_char( "Month data couldn't be found.\r\n", ch );
      return;
   }

   /* Now lets loop through the days frontwards and display what we need to */
   for( onday = 1; onday < ( mdata->days + maxweekday ); onday++ )
   {
      /* Increase the current weekday */
      onweekday++;

      /* This is to show the correct days of that month and no more */
      if( countday >= mdata->days )
         break;

      /* If current weekday is above maxweekday send a new line and set to 0 */
      if( onweekday >= maxweekday )
      {
         onweekday = 0;
         send_to_char( "\r\n", ch );
      }

      /* See if we have used the starting weekday yet and if not send blank
       * spots to line up correctly for when we do */
      if( !usedstart && onweekday < startweekday )
      {
         ch_printf( ch, "%10.10s&D  ", "" );
         continue;
      }

      /* Finaly start counting the dates we are showing */
      countday++;

      /* Put current day in red, char birthday in yellow, holidays in light blue
       * others in gray. Done so that the current day overrides others */
      if( countday == day )
         send_to_char( "&R", ch );
      else if( countday == ch->pcdata->birth_day && month == ch->pcdata->birth_month )
         send_to_char( "&Y", ch );
      else if( ( hdata = is_a_holiday( month, countday ) ) )
         send_to_char( "&C", ch );
      else
         send_to_char( "&w", ch );

      /* If we got this far we are bound to be using or have used starting weekday */
      usedstart = true;

      /* Show what count we are on, using buf so we can correcly position it */
      sprintf( buf, "%d", countday );
      ch_printf( ch, "%10.10s&D  ", buf );
   }

   /* Haven't sent a new line from other spot? send one now */
   if( onweekday != 0 )
      send_to_char( "\r\n", ch );

   /* List character birthday incase they don't have color on */
   if( month == ch->pcdata->birth_month )
      ch_printf( ch, "&zYour birthday is the &w%d&z%s.&D\r\n", ch->pcdata->birth_day,
         number_suffix( ch->pcdata->birth_day ) );

   for( onday = 1; onday < mdata->days; onday++ )
      if( ( hdata = is_a_holiday( month, onday ) ) )
         ch_printf( ch, "&z%s is the &w%d&z%s.&D\r\n", hdata->name, onday, number_suffix( onday ) );
}
