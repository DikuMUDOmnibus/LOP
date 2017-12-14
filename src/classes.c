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

int MAX_PC_CLASS;

CLASS_TYPE *class_table[MAX_CLASS];

int get_char_cnum( CHAR_DATA *ch, char *argument )
{
   MCLASS_DATA *mclass;
   int cnt = 0;

   if( !ch || is_npc( ch ) )
      return -1;
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      if( mclass->wclass >= 0 && mclass->wclass < MAX_PC_CLASS
      && class_table[mclass->wclass] && !str_cmp( class_table[mclass->wclass]->name, argument ) )
         return cnt;
      ++cnt;
   }
   return -1;
}

bool char_is_class( CHAR_DATA *ch, int cnum )
{
   MCLASS_DATA *mclass;

   if( !ch || is_npc( ch ) )
      return false;
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
   {
      if( cnum == mclass->wclass )
         return true;
   }
   return false;
}

void write_class_restriction_file( void )
{
   FILE *fp;
   char filename[MIL];
   int x, count = 0;

   snprintf( filename, sizeof( filename ), "%screstrictions.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "Can't open: %s for writing", filename );
      perror( filename );
      return;
   }
   for( x = 0; x < MAX_PC_CLASS; x++ )
   {
      if( class_table[x] && class_table[x]->name )
      {
         if( xIS_EMPTY( class_table[x]->class_restriction ) )
            continue;
         count++;
         fprintf( fp, "Restrict %s~ %s~\n", class_table[x]->name, ext_class_string( &class_table[x]->class_restriction ) );
      }
   }
   fprintf( fp, "$\n" );
   fclose( fp );
   fp = NULL;

   if( !count)
      remove_file( filename );
}

void write_class_file( int cl )
{
   CLASS_TYPE *Class = class_table[cl];
   FILE *fp;
   char filename[MIL];
   int x, y;

   if( !Class )
   {
      bug( "%s: NULL class for %d", __FUNCTION__, cl );
      return;
   }

   if( !Class->name )
   {
      bug( "%s: NULL class->name for %d", __FUNCTION__, cl );
      return;
   }

   snprintf( filename, sizeof( filename ), "%s%s.class", CLASS_DIR, Class->name );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "Can't open: %s for writing", filename );
      perror( filename );
      return;
   }
   fprintf( fp, "Name  %s~\n", Class->name );
   fprintf( fp, "Used  %d\n", Class->used );
   for( x = 0; x < top_sn; x++ )
   {
      if( !skill_table[x] || !skill_table[x]->name )
         continue;
      if( ( y = skill_table[x]->skill_level[cl] ) > 0 )
         fprintf( fp, "Skill '%s' %d %d\n", skill_table[x]->name, y, skill_table[x]->skill_adept[cl] );
   }
   fprintf( fp, "End\n" );
   fclose( fp );
   fp = NULL;
}

void load_class_restrictions( void )
{
   FILE *fp;
   const char *word;
   char filename[MIL], *infoflags, flag[MSL];
   int uclass, iclass;

   snprintf( filename, sizeof( filename ), "%screstrictions.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "r" ) ) )
      return;

   for( ;; )
   {
      word = feof( fp ) ? "$" : fread_word( fp );
      if( word[0] == '$' || word[0] == EOF )
         break;
      if( !str_cmp( word, "Restrict" ) )
      {
         uclass = get_pc_class( fread_flagstring( fp ) );
         if( !class_table[uclass] || !class_table[uclass]->name )
            fread_flagstring( fp );
         else
         {
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
                     xSET_BIT( class_table[uclass]->class_restriction, iclass );
                     break;
                  }
               }
            }
         }
      }
   }
   fclose( fp );
   fp = NULL;
}

void write_class_list( void )
{
   FILE *fp;
   int i;

   if( !( fp = fopen( CLASS_LIST, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, CLASS_LIST );
      perror( CLASS_LIST );
      return;
   }
   for( i = 0; i < MAX_PC_CLASS; i++ )
      if( class_table[i] && class_table[i]->name )
         fprintf( fp, "%s.class\n", class_table[i]->name );
   fprintf( fp, "%s", "$\n" );
   fclose( fp );
   fp = NULL;
}

CLASS_TYPE *valid_class( char *name )
{
   CLASS_TYPE *Class = NULL;
   int ccheck;

   if( name && name[0] != '\0' )
   {
      if( is_number( name ) )
      {
         if( ( ccheck = atoi( name ) ) >= 0 && ccheck < MAX_PC_CLASS )
            Class = class_table[ccheck];
      }
      else
      {
         for( ccheck = 0; ccheck < MAX_PC_CLASS && class_table[ccheck]; ccheck++ )
         {
            if( !class_table[ccheck] || !class_table[ccheck]->name )
               continue;

            if( !str_cmp( class_table[ccheck]->name, name ) )
            {
               Class = class_table[ccheck];
               break;
            }
         }
      }
   }
   return Class;
}

CMDF( do_classes )
{
   CLASS_TYPE *Class;
   int cl, count;

   if( !ch )
      return;
   for( cl = 0, count = ( is_immortal( ch ) ? 0 : 1 ); cl < MAX_PC_CLASS; cl++, count++ )
   {
      if( !( Class = class_table[cl] ) || !Class->name )
         continue;
      pager_printf( ch, "&w[&W%2d&w] &w(&W%3d&w) &W%s&D\r\n", count, Class->used, Class->name );
   }
}

void show_class( CHAR_DATA *ch, CLASS_TYPE *Class )
{
   int cl, low, hi, ct, i;

   set_pager_color( AT_PLAIN, ch );

   if( !Class )
   {
      send_to_char( "No such class.\r\n", ch );
      return;
   }

   cl = Class->clistplace;
   pager_printf( ch, "&wCLASS: &W%s\r\n&w\r\n", Class->name );
   pager_printf( ch, "&wUsed:  &W%d\r\n", Class->used );
   pager_printf( ch, "&wDisallowed classes:\r\n&W%s\r\n", ext_class_string( &Class->class_restriction ) );

   ct = 0;
   send_to_pager( "&wAllowed classes:&W\r\n", ch );
   for( i = 0; i < MAX_PC_CLASS; i++ )
   {
      if( !xIS_SET( Class->class_restriction, i ) )
      {
         pager_printf( ch, "%s ", class_table[i]->name );
         if( ++ct == 6 )
         {
            send_to_pager( "\r\n", ch );
            ct = 0;
         }
      }
   }
   if( ct != 0 )
      send_to_pager( "\r\n", ch );

   {
      int x, y, cnt, last = -1;
      bool first = true;

      low = 0;
      hi = MAX_LEVEL;
      set_pager_color( AT_BLUE, ch );
      for( x = low; x <= hi; x++ )
      {
         cnt = 0;
         for( y = gsn_first_spell; y < gsn_top_sn; y++ )
         {
            if( skill_table[y]->skill_level[cl] == x )
            {
               if( first )
                  send_to_pager( "\r\n&wSpells/Skills/Weapons\r\n", ch );
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

               pager_printf( ch, " &w(&W%3d&w)&D%-22s", skill_table[y]->skill_adept[cl], skill_table[y]->name );
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
}

/* Create a new class online. - Shaddai */
bool create_new_class( int rcindex, char *argument )
{
   if( !argument || argument[0] == '\0' )
      return false;
   if( rcindex >= MAX_CLASS || class_table[rcindex] )
      return false;

   CREATE( class_table[rcindex], CLASS_TYPE, 1 );
   if( !class_table[rcindex] )
      return false;

   class_table[rcindex]->clistplace = rcindex;
   argument[0] = UPPER( argument[0] );
   class_table[rcindex]->name = STRALLOC( argument );
   class_table[rcindex]->used = 0;
   xCLEAR_BITS( class_table[rcindex]->class_restriction );
   return true;
}

/* Edit class information - Thoric */
CMDF( do_setclass )
{
   CLASS_TYPE *Class;
   char arg1[MIL], arg2[MIL];
   int cl;

   set_char_color( AT_PLAIN, ch );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1 == NULL || arg1[0] == '\0' )
   {
      send_to_char( "Usage: setclass <class> [create/save]\r\n", ch );
      send_to_char( "Usage: setclass <class> skill <skill> <level> <adept>\r\n", ch );
      send_to_char( "Usage: setclass <class> name <new name>\r\n", ch );
      send_to_char( "Usage: setclass <class> classes [<class>]\r\n", ch );
      return;
   }

   Class = valid_class( arg1 );
   if( !str_cmp( arg2, "create" ) && Class )
   {
      send_to_char( "That class already exists!\r\n", ch );
      return;
   }
   else if( !Class && str_cmp( arg2, "create" ) )
   {
      send_to_char( "No such class.\r\n", ch );
      return;
   }

   if(  arg2 == NULL || arg2[0] == '\0' )
   {
      show_class( ch, Class );
      return;
   }
 
   if( !str_cmp( arg2, "create" ) )
   {
      char filename[1024];

      if( MAX_PC_CLASS >= MAX_CLASS )
      {
         send_to_char( "You need to up MAX_CLASS in src/h/mud.h and make clean.\r\n", ch );
         return;
      }
      snprintf( filename, sizeof( filename ), "%s.class", arg1 );
      if( !can_use_path( ch, CLASS_DIR, filename ) )
         return;
      if( !( create_new_class( MAX_PC_CLASS, arg1 ) ) )
      {
         send_to_char( "Couldn't create a new class.\r\n", ch );
         return;
      }
      write_class_file( MAX_PC_CLASS );
      MAX_PC_CLASS++;
      write_class_list( );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   cl = Class->clistplace;

   if( !str_cmp( arg2, "save" ) )
   {
      write_class_file( cl );
      send_to_char( "Class saved.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "You must specify an argument.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "classes" ) )
   {
      int i;
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
               xTOGGLE_BIT( Class->class_restriction, i );
               modified = true;
            }
         }
      }
      if( !modified )
         send_to_char( "No such class.\r\n", ch );
      else
      {
         write_class_restriction_file( );
         send_to_char( "Class Classes set.\r\n", ch );
      }
      return;
   }

   if( !str_cmp( arg2, "skill" ) )
   {
      SKILLTYPE *skill;
      int sn;

      argument = one_argument( argument, arg2 );
      if( ( sn = skill_lookup( arg2 ) ) < 0 )
      {
         ch_printf( ch, "No such skill as %s.\r\n", arg2 );
         return;
      }
      if( !( skill = get_skilltype( sn ) ) )
      {
         send_to_char( "Invalid skilltype???\r\n", ch );
         return;
      }
      argument = one_argument( argument, arg2 );
      skill->skill_level[cl] = URANGE( -1, atoi( arg2 ), MAX_LEVEL );
      argument = one_argument( argument, arg2 );
      skill->skill_adept[cl] = URANGE( 0, atoi( arg2 ), 100 );
      write_class_file( cl );
      if( skill->skill_level[cl] == -1 )
         ch_printf( ch, "Skill \"%s\" now has a level of -1 and means the class no longer gets it.\r\n", skill->name );
      else
         ch_printf( ch, "Skill \"%s\" added at level %d and %d%%.\r\n", skill->name, skill->skill_level[cl],
            skill->skill_adept[cl] );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      CLASS_TYPE *ccheck;
      char buf[256];

      one_argument( argument, arg1 );
      if( arg1 == NULL || arg1[0] == '\0' )
      {
         send_to_char( "You can't set a class name to nothing.\r\n", ch );
         return;
      }
      snprintf( buf, sizeof( buf ), "%s.class", arg1 );
      if( !can_use_path( ch, CLASS_DIR, buf ) )
         return;
      if( ( ccheck = valid_class( arg1 ) ) )
      {
         ch_printf( ch, "Already a class called %s.\r\n", arg1 );
         return;
      }
      snprintf( buf, sizeof( buf ), "%s%s.class", CLASS_DIR, Class->name );
      remove_file( buf );
      STRSET( Class->name, capitalize( argument ) );
      ch_printf( ch, "Class renamed to %s.\r\n", arg1 );
      write_class_file( cl );
      write_class_list( );
      return;
   }
   do_setclass( ch, (char *)"" );
}

bool load_class_file( const char *fname )
{
   FILE *fp;
   CLASS_TYPE *Class;
   const char *word;
   char buf[MIL];
   int cl = -1;
   bool fMatch;

   snprintf( buf, sizeof( buf ), "%s%s", CLASS_DIR, fname );
   if( !( fp = fopen( buf, "r" ) ) )
   {
      perror( buf );
      return false;
   }

   CREATE( Class, CLASS_TYPE, 1 );

   for( cl = 0; cl < MAX_CLASS; cl++ )
   {
      if( !class_table[cl] )
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

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               fclose( fp );
               fp = NULL;
               /* Keeps them all in order instead of keeping them at a set number */
               if( cl < 0 || cl >= MAX_CLASS )
               {
                  bug( "%s: Class (%s) bad/not found (%d)", __FUNCTION__, Class->name ? Class->name : buf, cl );
                  STRFREE( Class->name );
                  DISPOSE( Class );
                  return false;
               }
               class_table[cl] = Class;
               class_table[cl]->clistplace = cl;
               return true;
            }
            break;

         case 'N':
            KEY( "Name", Class->name, fread_string( fp ) );
            break;

         case 'S':
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
                  if( cl >= 0 && cl < MAX_CLASS )
                  {
                     skill_table[sn]->skill_level[cl] = lev;
                     skill_table[sn]->skill_adept[cl] = adp;
                  }
               }
               fMatch = true;
               break;
            }
            break;

         case 'U':
            KEY( "Used", Class->used, fread_number( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void load_classes( void )
{
   FILE *fp;
   const char *filename;
   int i;

   MAX_PC_CLASS = 0;

   for( i = 0; i < MAX_CLASS; i++ )
      class_table[i] = NULL;

   if( !( fp = fopen( CLASS_LIST, "r" ) ) )
   {
      bug( "%s: Can't open %s for reading", __FUNCTION__, CLASS_LIST );
      perror( CLASS_LIST );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fp ) ? "$" : fread_word( fp );
      if( filename[0] == '$' || filename[0] == EOF )
         break;

      if( !load_class_file( filename ) )
         bug( "%s: Can't load class file: %s", __FUNCTION__, filename );
      else
         MAX_PC_CLASS++;
   }
   fclose( fp );
   fp = NULL;
}

void save_classes( void )
{
   int x;

   for( x = 0; x < MAX_PC_CLASS; x++ )
      if( class_table[x] && class_table[x]->name )
         write_class_file( x );
}

MCLASS_DATA *add_mclass( CHAR_DATA *ch, int wclass, int level, int cpercent, double exp, bool limit )
{
   MCLASS_DATA *mclass;
   int mcount = 0;

   if( !ch || !ch->pcdata )
      return NULL;
   for( mclass = ch->pcdata->first_mclass; mclass; mclass = mclass->next )
      mcount++;
   if( limit && mcount >= sysdata.mclass )
   {
      send_to_char( "You already have all the classes you can.\r\n", ch );
      update_level( ch );
      return NULL;
   }
   CREATE( mclass, MCLASS_DATA, 1 );
   if( !mclass )
   {
      bug( "%s: failed to create mclass.", __FUNCTION__ );
      update_level( ch );
      return NULL;
   }
   mclass->wclass = wclass;
   mclass->level = level;
   mclass->cpercent = cpercent;
   mclass->exp = exp;
   LINK( mclass, ch->pcdata->first_mclass, ch->pcdata->last_mclass, next, prev );
   update_level( ch );
   return mclass;
}
