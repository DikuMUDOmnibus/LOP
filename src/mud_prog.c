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
 *****************************************************************************
 *  The MUDprograms are heavily based on the original MOBprogram code that   *
 *  was written by N'Atas-ha.						     *
 *  Much has been added, including the capability to put a "program" on      *
 *  rooms and objects, not to mention many more triggers and ifchecks, as    *
 *  well as "script" support.						     *
 *                                                                           *
 *  Error reporting has been changed to specify whether the offending        *
 *  program is on a mob, a room or and object, along with the vnum.          *
 *                                                                           *
 *  Mudprog parsing has been rewritten (in mprog_driver). Mprog_process_if   *
 *  and mprog_process_cmnd have been removed, mprog_do_command is new.       *
 *  Full support for nested ifs is in.                                       *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include "h/mud.h"

/*
 * Recursive function used by the carryingvnum ifcheck.
 * It loops thru all objects belonging to a char (in nested containers)
 * and returns true if it finds a matching vnum.
 * I declared it static to limit its scope to this file.  --Gorog
 *
 * This recursive function works by using the following method for
 * traversing the nodes in a binary tree:
 *
 *    Start at the root node
 *    if there is a child then visit the child
 *       if there is a sibling then visit the sibling
 *    else
 *    if there is a sibling then visit the sibling
 */
static bool carryingvnum_visit( CHAR_DATA *ch, OBJ_DATA *obj, int vnum )
{
   if( obj->wear_loc == -1 && obj->pIndexData->vnum == vnum )
      return true;

   if( obj->first_content ) /* node has a child? */
   {
      if( carryingvnum_visit( ch, obj->first_content, vnum ) )
         return true;

      if( obj->next_content ) /* node has a sibling? */
         if( carryingvnum_visit( ch, obj->next_content, vnum ) )
            return true;
   }
   else if( obj->next_content )  /* node has a sibling? */
      if( carryingvnum_visit( ch, obj->next_content, vnum ) )
         return true;
   return false;
}

/*
 *  Defines by Narn for new mudprog parsing, used as 
 *  return values from mprog_do_command.
 */
#define COMMANDOK    1
#define IFTRUE       2
#define IFFALSE      3
#define ORTRUE       4
#define ORFALSE      5
#define FOUNDELSE    6
#define FOUNDENDIF   7
#define IFIGNORED    8
#define ORIGNORED    9

/* Ifstate defines, used to create and access ifstate array
   in mprog_driver. */
#define MAX_IFS     20  /* should always be generous */
#define IN_IF        0
#define IN_ELSE      1
#define DO_IF        2
#define DO_ELSE      3

#define MAX_PROG_NEST   20

/* Mudprogram additions */
CHAR_DATA *supermob;
OBJ_DATA *supermob_obj;
struct act_prog_data *room_act_list;
struct act_prog_data *obj_act_list;
struct act_prog_data *mob_act_list;

void free_prog_actlists( void )
{
   struct act_prog_data *apd, *apd_next;

   for( apd = room_act_list; apd; apd = apd_next )
   {
      apd_next = apd->next;
      DISPOSE( apd );
   }

   for( apd = obj_act_list; apd; apd = apd_next )
   {
      apd_next = apd->next;
      DISPOSE( apd );
   }

   for( apd = mob_act_list; apd; apd = apd_next )
   {
      apd_next = apd->next;
      DISPOSE( apd );
   }
}

/* Local function code and brief comments. */
void init_supermob( void )
{
   ROOM_INDEX_DATA *office;

   if( !( supermob = create_mobile( get_mob_index( MOB_VNUM_SUPERMOB ) ) ) )
   {
      bug( "%s: Couldn't create supermob using vnum %d.", __FUNCTION__, MOB_VNUM_SUPERMOB );
      return;
   }

   if( !( office = get_room_index( sysdata.room_poly ) ) )
   {
      bug( "%s: Couldn't find room vnum %d.", __FUNCTION__, sysdata.room_poly );
      return;
   }
   char_to_room( supermob, office );
}

/*
 * Used to get sequential lines of a multi line string (separated by "\r\n")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */
char *mprog_next_command( char *clist )
{
   char *pointer = clist;

   while( *pointer != '\r' && *pointer != '\n' && *pointer != '\0' )
      pointer++;

   while( *pointer == '\r' || *pointer == '\n' )
      *pointer++ = '\0';

   return ( pointer );
}

char *mprog_one_command( char *argument, char *arg_first )
{
   char lch = ' ';
   short count = 0;
   bool newappend = false;

   if( !argument || argument[0] == '\0' )
   {
      arg_first[0] = '\0';
      return (char *)"\0";
   }

   while( isspace( *argument ) )
      argument++;

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == '\n' || *argument == '\r' )
      {
         argument++; /* Continue on with argument */
         if( lch == '+' )  /* If the last char was a + we want to continue */
         {
            if( newappend )
            {
               arg_first--; /* Need to replace the + with a space */
               *arg_first = ' ';
               arg_first++;
               newappend = false;
            }
            continue;
         }
         break; /* Other wise break out here */
      }
      if( *argument == '+' )
         newappend = true;
      else
         newappend = false;
      *arg_first = *argument;
      lch = *argument;
      arg_first++;
      argument++;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      argument++;
   return argument;
}

/*
 * These two functions do the basic evaluation of ifcheck operators.
 * It is important to note that the string operations aren't what
 * you probably expect.  Equality is exact and division is substring.
 * remember that lhs has been stripped of leading space, but can
 * still have trailing spaces so be careful when editing since:
 * "guard" and "guard " aren't equal.
 */
bool mprog_seval( char *lhs, char *opr, char *rhs, CHAR_DATA *mob )
{
   if( !str_cmp( opr, "==" ) )
      return ( bool ) ( !str_cmp( lhs, rhs ) );
   if( !str_cmp( opr, "!=" ) )
      return ( bool ) ( str_cmp( lhs, rhs ) );
   if( !str_cmp( opr, "/" ) )
      return ( bool ) ( !str_infix( rhs, lhs ) );
   if( !str_cmp( opr, "!/" ) )
      return ( bool ) ( str_infix( rhs, lhs ) );

   progbug_printf( mob, "%s: Improper MOBprog operator '%s'", __FUNCTION__, opr );
   return 0;
}

bool mprog_cbveval( bool lhs, char *opr, CHAR_DATA *mob )
{
   if( !str_cmp( opr, "==" ) )
      return lhs;
   if( !str_cmp( opr, "!=" ) )
      return !lhs;
   progbug_printf( mob, "%s: Improper MOBprog operator '%s'", __FUNCTION__, opr );
   return 0;
}

bool mprog_veval( int lhs, char *opr, int rhs, CHAR_DATA *mob )
{
   if( !str_cmp( opr, "==" ) )
      return ( lhs == rhs );
   if( !str_cmp( opr, "!=" ) )
      return ( lhs != rhs );
   if( !str_cmp( opr, ">" ) )
      return ( lhs > rhs );
   if( !str_cmp( opr, "<" ) )
      return ( lhs < rhs );
   if( !str_cmp( opr, "<=" ) )
      return ( lhs <= rhs );
   if( !str_cmp( opr, ">=" ) )
      return ( lhs >= rhs );
   if( !str_cmp( opr, "&" ) )
      return ( lhs & rhs );
   if( !str_cmp( opr, "|" ) )
      return ( lhs | rhs );
   if( !str_cmp( opr, "%" ) )
      return ( ( lhs % rhs ) == 0 );
   progbug_printf( mob, "%s: Improper MOBprog operator '%s'", __FUNCTION__, opr );
   return 0;
}

/* Added for Kayle's MProg Variable Expansion, 8/25/2010 */
/* Modified to compile in this codebase */
char *parse_var( const char *cmnd, char_data *mob, char_data *actor, OBJ_DATA *obj, const void *vo, char_data *rndm )
{
   static char results[ MSL * 2 ];
   int x;
   char_data *chkchar = NULL;
   OBJ_DATA *chkobj = NULL;
   ROOM_INDEX_DATA *chkroom = NULL;

   results[0] = '\0';
   for( x = 0; cmnd[x] != '\0'; x++ )
   {
      chkchar = NULL;
      chkobj = NULL;
      if( cmnd[x] == '\0' )
         return results;
      if( cmnd[x] != '$' )
         add_letter( results, cmnd[x] );
      else
      {
         switch( cmnd[x + 1] )
         {
            default:
               add_letter( results, cmnd[x] ); 
               break;
            case 'i':
               chkchar = mob;
               break;
            case 'n':
               chkchar = actor;
               break;
            case 't':
               chkchar = ( char_data * )vo;
               break;
            case 'r':
               chkchar = rndm;
               break;
            case 'o':
               chkobj = obj;
               break;
            case 'p':
               chkobj = ( OBJ_DATA * )vo;
               break;
         }
      }
      if( chkchar || chkobj || chkroom )
      {
         x += 2;
         if( cmnd[x] != '.' || cmnd[x] == '\0' )
         {
            add_letter( results, cmnd[x-2]); 
            add_letter( results, cmnd[x-1]); 
            add_letter( results, cmnd[x]); 
            if( cmnd[x] == '\0' )
               return results;
            continue;
         }
         else  
         {
            char word[MIL];;
            word[0] = '\0';
            x++;
            while( ( isalpha( cmnd[x] ) || isdigit( cmnd[x] ) ) && cmnd[x] != '\0' )  
            {
               add_letter( word, cmnd[x] );
               x++;
            }
            if( chkchar != NULL )  
            {
               /*
                * Because of how many there are shortning how many checks it has to do.
                * Well thought I would have more then this, but oh well it is already done now :)
                */
               switch( UPPER( word[0] ) )
               {
                  default:
                     break;

                  case 'C':
                     if( !str_cmp( word, "constitution" ) )
                     {
                        mudstrlcat( results, format( "%d", get_curr_stat( STAT_CON, chkchar ) ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "charisma" ) )
                     {
                        mudstrlcat( results, format( "%d", get_curr_stat( STAT_CHA, chkchar ) ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "class" ) ) /* Npcs don't have a class, but will return Unknown */
                     {
                        mudstrlcat( results, dis_main_class_name( chkchar ), MSL );
                        break;
                     }
                     break;

                  case 'D':
                     if( !str_cmp( word, "dexterity" ) )
                     {
                        mudstrlcat( results, format( "%d", get_curr_stat( STAT_DEX, chkchar ) ), MSL );
                        break;
                     }
                     break;

                  case 'G':
                     if( !str_cmp( word, "gold" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->gold ), MSL );
                        break;
                     }
                     break;

                  case 'H':
                     if( !str_cmp( word, "hp" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->hit ), MSL );
                        break;
                     }
                     break;

                  case 'I':
                     if( !str_cmp( word, "intelligence" ) )
                     {
                        mudstrlcat( results, format( "%d", get_curr_stat( STAT_INT, chkchar ) ), MSL );
                        break;
                     }
                     break;

                  case 'L':
                     if( !str_cmp( word, "level" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->level ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "luck" ) )
                     {
                        mudstrlcat( results, format( "%d", get_curr_stat( STAT_LCK, chkchar ) ), MSL );
                        break;
                     }
                     break;

                  case 'M':
                     if( !str_cmp( word, "maxhp" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->max_hit ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "mana" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->mana ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "maxmana" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->max_mana ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "move" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->move ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "maxmove" ) )
                     {
                        mudstrlcat( results, format( "%d", chkchar->max_move ), MSL );
                        break;
                     }
                     break;

                  case 'N':
                     if( !str_cmp( word, "name" ) )
                     {
                        if( is_npc( chkchar ) )
                           mudstrlcat( results, format( "%s", chkchar->short_descr ), MSL );
                        else
                           mudstrlcat( results, format( "%s", chkchar->name ), MSL );
                        break;
                     }
                     break;

                  case 'R':
                     if( !str_cmp( word, "race" ) )
                     {
                        /* Npcs don't have a race */
                        if( is_npc( chkchar ) )
                           mudstrlcat( results, "Unknown", MSL );
                        else
                           mudstrlcat( results, dis_race_name( chkchar->race ), MSL );
                     }
                     break;

                  case 'S':
                     if( !str_cmp( word, "sex" ) )
                     {
                        mudstrlcat( results, format( "%s", sex_names[chkchar->sex] ), MSL );
                        break;
                     }
                     if( !str_cmp( word, "strength" ) )
                     {
                        mudstrlcat( results, format( "%d", get_curr_stat( STAT_STR, chkchar ) ), MSL );
                        break;
                     }
                     break;

                  case 'W':
                     if( !str_cmp( word, "wisdom" ) )
                     {
                        mudstrlcat( results, format( "%d", get_curr_stat( STAT_WIS, chkchar ) ), MSL );
                        break;
                     }
                     break;
               }
            }
            if( chkobj != NULL )
            {
               if( !str_cmp( word, "level" ) )
                   mudstrlcat( results, format( "%d", chkobj->level ), MSL );
            }
            if( cmnd[x] == '\0')
               return results;
            else
               add_letter( results, cmnd[x]);
         }
      }
   }
   return results;
}

#define isoperator( c ) ( (c) == '=' || (c) == '<' || (c) == '>' || (c) == '!' || (c) == '&' || (c) == '|' || (c) == '%' )
#define MAX_IF_ARGS 6
/*
 * This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The Usage for an if check is: ifcheck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return BERR otherwise return boolean 1,0
 * Redone by Altrag.. kill all that big copy-code that performs the
 * same action on each variable..
 */
int mprog_do_ifcheck( char *ifcheck, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, CHAR_DATA *rndm )
{
   CHAR_DATA *chkchar = NULL;
   OBJ_DATA *chkobj = NULL, *tobj, *pObj;
   char buf[MSL], opr[MIL];
   char *chck, *cvar;
   char *argv[MAX_IF_ARGS];
   char *rval = (char *)"";
   char *q, *p = buf;
   int lhsvl = 0, rhsvl = 0, vnum = 0, argc = 0;

   if( !*ifcheck )
   {
      progbug( "Null ifcheck", mob );
      return BERR;
   }

   /*
    * New parsing by Thoric to allow for multiple arguments inside the
    * brackets, ie: if leveldiff($n, $i) > 10
    * It's also smaller, cleaner and probably faster
    */
   strcpy( buf, ifcheck );
   opr[0] = '\0';
   while( isspace( *p ) )
      ++p;
   argv[argc++] = p;
   while( isalnum( *p ) )
      ++p;
   while( isspace( *p ) )
      *p++ = '\0';
   if( *p != '(' )
   {
      progbug( "Ifcheck Usage error (missing left bracket)", mob );
      return BERR;
   }

   *p++ = '\0';
   /* Need to check for spaces or if name( $n ) isn't legal --Shaddai */
   while( isspace( *p ) )
      *p++ = '\0';
   for( ;; )
   {
      argv[argc++] = p;
      while( *p == '$' || isalnum( *p ) )
         ++p;
      while( isspace( *p ) )
         *p++ = '\0';
      switch( *p )
      {
         case ',':
            *p++ = '\0';
            while( isspace( *p ) )
               *p++ = '\0';
            if( argc >= MAX_IF_ARGS )
            {
               while( *p && *p != ')' )
                  ++p;
               if( *p )
                  *p++ = '\0';
               while( isspace( *p ) )
                  *p++ = '\0';
               goto doneargs;
            }
            break;

         case ')':
            *p++ = '\0';
            while( isspace( *p ) )
               *p++ = '\0';
            goto doneargs;

         default:
            progbug( "Ifcheck Usage warning (missing right bracket)", mob );
            goto doneargs;
      }
   }

 doneargs:
   q = p;
   while( isoperator( *p ) )
      ++p;
   strncpy( opr, q, p - q );
   opr[p - q] = '\0';
   while( isspace( *p ) )
      *p++ = '\0';
   rval = parse_var( p, mob, actor, obj, vo, rndm ); /* Added for Kayle's MProg Variable Expansion, 8/25/2010 */
   rval = p;
   while( *p )
      ++p;
   *p = '\0';

   if( !*opr )
      strcpy( opr, "==" );

   chck = argv[0] ? argv[0] : ( char * )"";
   cvar = argv[1] ? argv[1] : ( char * )"";

   /*
    * chck contains check, cvar is the variable in the (), opr is the
    * operator if there is one, and rval is the value if there was an
    * operator.
    */
   if( cvar[0] == '$' )
   {
      switch( cvar[1] )
      {
         case 'i':
            if( !( chkchar = mob ) )
            {
               progbug_printf( mob, "%s: chkchar is NULL for '$i' to '%s'", __FUNCTION__, chck );
               return BERR;
            }
            break;

         case 'n':
            if( !( chkchar = actor ) )
            {
               progbug_printf( mob, "%s: chkchar is NULL for '$n' to '%s'", __FUNCTION__, chck );
               return BERR;
            }
            break;

         case 't':
            if( !( chkchar = ( CHAR_DATA * ) vo ) )
            {
               progbug_printf( mob, "%s: chkchar is NULL for '$t' to '%s'", __FUNCTION__, chck );
               return BERR;
            }
            break;

         case 'r':
            if( !( chkchar = rndm ) )
               return BERR;
            break;

         case 'o':
            if( !( chkobj = obj ) )
            {
               progbug_printf( mob, "%s: chkobj is NULL for '$o' to '%s'", __FUNCTION__, chck );
               return BERR;
            }
            break;

         case 'p':
            if( !( chkobj = ( OBJ_DATA * ) vo ) )
            {
               progbug_printf( mob, "%s: chkobj is NULL for '$p' to '%s'", __FUNCTION__, chck );
               return BERR;
            }
            break;

         default:
            progbug_printf( mob, "%s: Bad argument '%c%c' to '%s'", __FUNCTION__, cvar[0], cvar[1], chck );
            return BERR;
      }
      if( !chkchar && !chkobj )
         return BERR;
   }
   else if( cvar )
      vnum = atoi( cvar );

   switch( UPPER( chck[0] ) )
   {
      default:
         break;

      case 'M':
         if( !str_cmp( chck, "mkills" ) )
         {
            if( !chkchar || is_npc( chkchar ) )
            {
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            return mprog_veval( chkchar->pcdata->mkills, opr, atoi( rval ), mob );
         }
         if( !str_cmp( chck, "mdeaths" ) )
         {
            if( !chkchar || is_npc( chkchar ) )
            {
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            return mprog_veval( chkchar->pcdata->mdeaths, opr, atoi( rval ), mob );
         }
         if( !str_cmp( chck, "mobinarea" ) )
         {
            CHAR_DATA *tmob;
            MOB_INDEX_DATA *m_index;
            int world_count = 0, found_count = 0;

            if( vnum < 1 || vnum > MAX_VNUM || !( m_index = get_mob_index( vnum ) ) )
            {
               progbug( "Bad vnum to 'mobinarea'", mob );
               return BERR;
            }

            world_count = m_index->count;
            for( tmob = first_char; tmob && found_count != world_count; tmob = tmob->next )
            {
               if( is_npc( tmob ) && tmob->pIndexData->vnum == vnum )
               {
                  found_count++;

                  if( tmob->in_room->area == mob->in_room->area )
                     lhsvl++;
               }
            }
            rhsvl = UMAX( 0, atoi( rval ) );

            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }
         if( !str_cmp( chck, "mobinroom" ) )
         {
            CHAR_DATA *oMob;

            if( vnum < 1 || vnum > MAX_VNUM )
            {
               progbug( "Bad vnum to 'mobinroom'", mob );
               return BERR;
            }

            for( oMob = mob->in_room->first_person; oMob; oMob = oMob->next_in_room )
               if( is_npc( oMob ) && oMob->pIndexData->vnum == vnum )
                  lhsvl++;

            rhsvl = UMAX( 0, atoi( rval ) );

            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "mobinworld" ) )
         {
            MOB_INDEX_DATA *m_index;

            if( vnum < 1 || vnum > MAX_VNUM || !( m_index = get_mob_index( vnum ) ) )
            {
               progbug( "Bad vnum to 'mobinworld'", mob );
               return BERR;
            }
            lhsvl = m_index->count;
            rhsvl = UMAX( 0, atoi( rval ) );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }
         break;

      case 'O':
         if( !str_cmp( chck, "ovnumhere" ) )
         {
            if( vnum < 1 || vnum > MAX_VNUM )
            {
               progbug( "OvnumHere: bad vnum", mob );
               return BERR;
            }

            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->pIndexData->vnum == vnum )
                  lhsvl += pObj->count;
            for( pObj = mob->in_room->first_content; pObj; pObj = pObj->next_content )
               if( pObj->pIndexData->vnum == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, atoi( rval ) );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "otypeinv" ) )
         {
            if( !is_number( cvar ) )
               vnum = get_flag( cvar, o_types, ITEM_TYPE_MAX );
            if( vnum < 0 || vnum >= ITEM_TYPE_MAX )
            {
               progbug( "OtypeInv: bad type", mob );
               return BERR;
            }
            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->wear_loc == WEAR_NONE && pObj->item_type == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, atoi( rval ) );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "otypehere" ) )
         {
            if( !is_number( cvar ) )
               vnum = get_flag( cvar, o_types, ITEM_TYPE_MAX );
            if( vnum < 0 || vnum >= ITEM_TYPE_MAX )
            {
               progbug( "OtypeHere: bad type", mob );
               return BERR;
            }
            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->item_type == vnum )
                  lhsvl += pObj->count;
            for( pObj = mob->in_room->first_content; pObj; pObj = pObj->next_content )
               if( pObj->item_type == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, atoi( rval ) );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "otyperoom" ) )
         {
            if( !is_number( cvar ) )
               vnum = get_flag( cvar, o_types, ITEM_TYPE_MAX );
            if( vnum < 0 || vnum >= ITEM_TYPE_MAX )
            {
               progbug( "OtypeRoom: bad type", mob );
               return BERR;
            }
            for( pObj = mob->in_room->first_content; pObj; pObj = pObj->next_content )
               if( pObj->item_type == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, atoi( rval ) );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "otypewear" ) )
         {
            if( !is_number( cvar ) )
               vnum = get_flag( cvar, o_types, ITEM_TYPE_MAX );
            if( vnum < 0 || vnum >= ITEM_TYPE_MAX )
            {
               progbug( "OtypeWear: bad type", mob );
               return BERR;
            }
            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->wear_loc != WEAR_NONE && pObj->item_type == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, atoi( rval ) );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "otypecarry" ) )
         {
            if( !is_number( cvar ) )
               vnum = get_flag( cvar, o_types, ITEM_TYPE_MAX );
            if( vnum < 0 || vnum >= ITEM_TYPE_MAX )
            {
               progbug( "OtypeCarry: bad type", mob );
               return BERR;
            }
            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->item_type == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, atoi( rval ) );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "ovnumroom" ) )
         {
            if( vnum < 1 || vnum > MAX_VNUM )
            {
               progbug( "OvnumRoom: bad vnum", mob );
               return BERR;
            }
            for( pObj = mob->in_room->first_content; pObj; pObj = pObj->next_content )
               if( pObj->pIndexData->vnum == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, is_number( rval ) ? atoi( rval ) : -1 );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "ovnumcarry" ) )
         {
            if( vnum < 1 || vnum > MAX_VNUM )
            {
               progbug( "OvnumCarry: bad vnum", mob );
               return BERR;
            }
            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->pIndexData->vnum == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, is_number( rval ) ? atoi( rval ) : -1 );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "ovnumwear" ) )
         {
            if( vnum < 1 || vnum > MAX_VNUM )
            {
               progbug( "OvnumWear: bad vnum", mob );
               return BERR;
            }
            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->wear_loc != WEAR_NONE && pObj->pIndexData->vnum == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, is_number( rval ) ? atoi( rval ) : -1 );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }

         if( !str_cmp( chck, "ovnuminv" ) )
         {
            if( vnum < 1 || vnum > MAX_VNUM )
            {
               progbug( "OvnumInv: bad vnum", mob );
               return BERR;
            }
            for( pObj = mob->first_carrying; pObj; pObj = pObj->next_content )
               if( pObj->wear_loc == WEAR_NONE && pObj->pIndexData->vnum == vnum )
                  lhsvl += pObj->count;
            rhsvl = UMAX( 0, is_number( rval ) ? atoi( rval ) : -1 );
            return mprog_veval( lhsvl, opr, rhsvl, mob );
         }
         break;

      case 'P':
         if( !str_cmp( chck, "pdeaths" ) )
         {
            if( !chkchar || is_npc( chkchar ) )
            {
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            return mprog_veval( chkchar->pcdata->pdeaths, opr, atoi( rval ), mob );
         }
         if( !str_cmp( chck, "pkills" ) )
         {
            if( !chkchar || is_npc( chkchar ) )
            {
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            return mprog_veval( chkchar->pcdata->pkills, opr, atoi( rval ), mob );
         }
         break;

      case 'R':
         if( !str_cmp( chck, "rand" ) )
            return ( number_percent( ) <= vnum );
         break;

      case 'T':
         if( !str_cmp( chck, "timeskilled" ) )
         {
            MOB_INDEX_DATA *pMob;

            if( chkchar )
            {
               if( is_npc( chkchar ) )
                  pMob = chkchar->pIndexData;
               else
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
            }
            else if( !( pMob = get_mob_index( vnum ) ) )
            {
               progbug_printf( mob, "TimesKilled ifcheck: bad vnum [%d]", vnum );
               return BERR;
            }
            return mprog_veval( pMob->killed, opr, atoi( rval ), mob );
         }
         break;
   }

   if( chkchar )
   {
      switch( UPPER( chck[0] ) )
      {
         default:
            break;

         case 'A':
            if( !str_cmp( chck, "asuppressed" ) )
               return mprog_veval( get_timer( chkchar, TIMER_ASUPPRESSED ), opr, atoi( rval ), mob );
            break;

         case 'C':
            if( !str_cmp( chck, "con" ) )
               return mprog_veval( get_curr_con( chkchar ), opr, atoi( rval ), mob );
            if( !str_cmp( chck, "cha" ) )
               return mprog_veval( get_curr_cha( chkchar ), opr, atoi( rval ), mob );
            if( !str_cmp( chck, "clan" ) )
            {
               if( is_npc( chkchar ) || !chkchar->pcdata->clan )
                  return false;
               return mprog_seval( chkchar->pcdata->clan->name, opr, rval, mob );
            }
            if( !str_cmp( chck, "cansee" ) )
               return mprog_cbveval( can_see( mob, chkchar ), opr, mob );
            if( !str_cmp( chck, "canpkill" ) )
               return mprog_cbveval( can_pkill( chkchar ), opr, mob );
            /* Is char carrying a specific piece of eq?  -- Gorog */
            if( !str_cmp( chck, "carryingvnum" ) )
            {
               if( !is_number( rval ) )
               {
                  progbug_printf( mob, "CarryingVnum is checking for [%s] instead of a vnum.", rval );
                  return BERR;
               }
               vnum = atoi( rval );
               if( !chkchar->first_carrying )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_cbveval( carryingvnum_visit( chkchar, chkchar->first_carrying, vnum ), opr, mob );
            }

            if( !str_cmp( chck, "council" ) )
            {
               if( is_npc( chkchar ) || !chkchar->pcdata->council )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_seval( chkchar->pcdata->council->name, opr, rval, mob );
            }
            if( !str_cmp( chck, "class" ) )
            {
               MCLASS_DATA *mclass;

               if( is_npc( chkchar ) ) /* Npcs have no class */
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }

               for( mclass = chkchar->pcdata->first_mclass; mclass; mclass = mclass->next )
               {
                  if( mclass->wclass >= 0 && !str_cmp( rval, dis_class_name( mclass->wclass ) ) )
                  {
                     if( !str_cmp( opr, "!=" ) )
                        return false;
                     return true;
                  }
               }

               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            if( !str_cmp( chck, "carryweight" ) )
               return mprog_veval( chkchar->carry_weight, opr, atoi( rval ), mob );
            break;

         case 'D':
            if( !str_cmp( chck, "dex" ) )
               return mprog_veval( get_curr_dex( chkchar ), opr, atoi( rval ), mob );
            if( !str_cmp( chck, "deity" ) )
            {
               if( is_npc( chkchar ) || !chkchar->pcdata->deity )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_seval( chkchar->pcdata->deity->name, opr, rval, mob );
            }
            break;

         case 'F':
            if( !str_cmp( chck, "favor" ) )
            {
               if( is_npc( chkchar ) || !chkchar->pcdata->deity )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_veval( chkchar->pcdata->favor, opr, atoi( rval ), mob );
            }
            break;

         case 'G':
            if( !str_cmp( chck, "goldamt" ) )
               return mprog_veval( chkchar->gold, opr, atoi( rval ), mob );
            break;

         case 'H':
            if( !str_cmp( chck, "hitprcnt" ) )
               return ( !chkchar->hit || !chkchar->max_hit ) ? false : mprog_veval( ( chkchar->hit * 100 ) / chkchar->max_hit, opr, atoi( rval ), mob );
            if( !str_cmp( chck, "hit" ) )
               return mprog_veval( chkchar->hit, opr, atoi( rval ), mob );
            break;

         case 'I':
            if( !str_cmp( chck, "ispassage" ) )
               return mprog_cbveval( find_door( chkchar, rval, true ), opr, mob );
            if( !str_cmp( chck, "isopen" ) )
            {
               EXIT_DATA *pexit;

               if( !( pexit = find_door( chkchar, rval, true ) ) || xIS_SET( pexit->exit_info, EX_CLOSED ) )
               {
                  if( !str_cmp( opr, "!=" ) ) /* If checking not isopen then return true */
                     return true;
                  return false;
               }
               if( !str_cmp( opr, "!=" ) ) /* If checking not isopen then return false */
                  return false;
               return true;
            }
            if( !str_cmp( chck, "islocked" ) )
            {
               EXIT_DATA *pexit;

               if( !( pexit = find_door( chkchar, rval, true ) ) || !xIS_SET( pexit->exit_info, EX_LOCKED ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               if( !str_cmp( opr, "!=" ) )
                  return false;
               return true;
            }
            if( !str_cmp( chck, "ispkill" ) )
               return mprog_cbveval( is_pkill( chkchar ), opr, mob );
            if( !str_cmp( chck, "isdevoted" ) )
               return mprog_cbveval( is_devoted( chkchar ), opr, mob );
            if( !str_cmp( chck, "ispc" ) )
               return mprog_cbveval( !is_npc( chkchar ), opr, mob );
            if( !str_cmp( chck, "isnpc" ) )
               return mprog_cbveval( is_npc( chkchar ), opr, mob );
            if( !str_cmp( chck, "ismounted" ) )
               return mprog_cbveval( ( chkchar->position == POS_MOUNTED ), opr, mob );
            if( !str_cmp( chck, "ismorphed" ) )
               return mprog_cbveval( ( chkchar->morph ), opr, mob );
            if( !str_cmp( chck, "isfight" ) )
               return mprog_cbveval( who_fighting( chkchar ), opr, mob );
            if( !str_cmp( chck, "isimmort" ) )
               return mprog_cbveval( ( get_trust( chkchar ) >= PERM_IMM ), opr, mob );
            if( !str_cmp( chck, "isfollow" ) )
               return mprog_cbveval( ( chkchar->master && chkchar->master->in_room == chkchar->in_room ), opr, mob );

            if( !str_cmp( chck, "isalign" ) )
               return mprog_veval( chkchar->alignment, opr, atoi( rval ), mob );
               
            if( !str_cmp( chck, "ispcact" ) )
            {
               int value = get_flag( rval, plr_flags, PLR_MAX );

               if( value < 0 || value >= PLR_MAX )
               {
                  progbug_printf( mob, "Unknown plr flag [%s] being checked", rval );
                  return BERR;
               }
               if( is_npc( chkchar ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_cbveval( xIS_SET( chkchar->act, value ), opr, mob );
            }

            if( !str_cmp( chck, "isnpcact" ) )
            {
               int value = get_flag( rval, act_flags, ACT_MAX );

               if( value < 0 || value >= ACT_MAX )
               {
                  progbug_printf( mob, "Unknown act flag [%s] being checked", rval );
                  return BERR;
               }
               if( !is_npc( chkchar ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_cbveval( xIS_SET( chkchar->act, value ), opr, mob );
            }

            if( !str_cmp( chck, "isaffected" ) )
            {
               int value = get_flag( rval, a_flags, AFF_MAX );

               if( value < 0 || value >= AFF_MAX )
               {
                  progbug_printf( mob, "Unknown affect [%s] being checked", rval );
                  return BERR;
               }
               return mprog_cbveval( IS_AFFECTED( chkchar, value ), opr, mob );
            }
            /* Check added to see if the person isleader of == clan Shaddai */
            if( !str_cmp( chck, "isleader" ) )
            {
               CLAN_DATA *temp;

               if( is_npc( chkchar ) || !( temp = get_clan( rval ) ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               if( mprog_seval( chkchar->name, opr, temp->leader, mob )
               || mprog_seval( chkchar->name, opr, temp->number1, mob )
               || mprog_seval( chkchar->name, opr, temp->number2, mob ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return false;
                  return true;
               }

               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            /* Check added to see if the person isleader of == clan Gorog */
            if( !str_cmp( chck, "isclanleader" ) )
            {
               CLAN_DATA *temp;

               if( is_npc( chkchar ) || !( temp = get_clan( rval ) ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               if( mprog_seval( chkchar->name, opr, temp->leader, mob ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return false;
                  return true;
               }
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            if( !str_cmp( chck, "isclan1" ) )
            {
               CLAN_DATA *temp;

               if( is_npc( chkchar ) || !( temp = get_clan( rval ) ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               if( mprog_seval( chkchar->name, opr, temp->number1, mob ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return false;
                  return true;
               }
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            if( !str_cmp( chck, "isclan2" ) )
            {
               CLAN_DATA *temp;

               if( is_npc( chkchar ) || !( temp = get_clan( rval ) ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               if( mprog_seval( chkchar->name, opr, temp->number2, mob ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return false;
                  return true;
               }
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            if( !str_cmp( chck, "int" ) )
               return mprog_veval( get_curr_int( chkchar ), opr, atoi( rval ), mob );
            break;

         case 'L':
            if( !str_cmp( chck, "lck" ) )
               return mprog_veval( get_curr_lck( chkchar ), opr, atoi( rval ), mob );
            break;

         case 'M':
            if( !str_cmp( chck, "mobinvislevel" ) )
               return ( is_npc( chkchar ) ? mprog_veval( chkchar->mobinvis, opr, atoi( rval ), mob ) : false );
            if( !str_cmp( chck, "manaprcnt" ) )
               return ( !chkchar->mana || !chkchar->max_mana ) ? false : mprog_veval( ( chkchar->mana * 100 ) / chkchar->max_mana, opr, atoi( rval ), mob );
            if( !str_cmp( chck, "moveprcnt" ) )
               return ( !chkchar->move || !chkchar->max_move ) ? false : mprog_veval( ( chkchar->move * 100 ) / chkchar->max_move, opr, atoi( rval ), mob );
            if( !str_cmp( chck, "multi" ) )
            {
               CHAR_DATA *ch;

               if( !chkchar->desc )
                  return mprog_veval( 0, opr, atoi( rval ), mob );

               for( ch = first_char; ch; ch = ch->next )
                  if( !is_npc( chkchar ) && !is_npc( ch ) && ch->desc && chkchar->desc && !str_cmp( ch->desc->host, chkchar->desc->host ) )
                     lhsvl++;
               rhsvl = UMAX( 0, atoi( rval ) );
               return mprog_veval( lhsvl, opr, rhsvl, mob );
            }
            if( !str_cmp( chck, "morph" ) )
            {
               if( !chkchar->morph || !chkchar->morph->morph )
                  return false;
               return mprog_veval( chkchar->morph->morph->vnum, opr, rhsvl, mob );
            }
            if( !str_cmp( chck, "mana" ) )
               return mprog_veval( chkchar->mana, opr, atoi( rval ), mob );
            if( !str_cmp( chck, "move" ) )
               return mprog_veval( chkchar->move, opr, atoi( rval ), mob );
            break;

         case 'N':
            if( !str_cmp( chck, "nation" ) )
            {
               if( is_npc( chkchar ) || !chkchar->pcdata->nation )
                  return false;
               return mprog_seval( chkchar->pcdata->nation->name, opr, rval, mob );
            }
            if( !str_cmp( chck, "numfighting" ) )
               return mprog_veval( chkchar->num_fighting - 1, opr, atoi( rval ), mob );
            if( !str_cmp( chck, "norecall" ) )
               return mprog_cbveval( xIS_SET( chkchar->in_room->room_flags, ROOM_NO_RECALL ), opr, mob );
            break;

         case 'P':
            if( !str_cmp( chck, "position" ) )
            {
               int value;

               if( !is_number( rval ) )
                  value = get_flag( rval, pos_names, POS_MAX );
               else
                  value = atoi( rval );
               if( value < 0 || value >= POS_MAX )
               {
                  progbug_printf( mob, "Bad position [%s] being checked", rval );
                  return BERR;
               }
               return mprog_veval( chkchar->position, opr, value, mob );
            }
            if( !str_cmp( chck, "perm" ) )
               return mprog_veval( get_trust( chkchar ), opr, atoi( rval ), mob );
            break;

         case 'R':
            if( !str_cmp( chck, "race" ) )
            {
               if( is_npc( chkchar ) || !race_table[chkchar->race] )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_seval( ( char * )race_table[chkchar->race]->name, opr, rval, mob );
            }
            break;

         case 'S':
            if( !str_cmp( chck, "str" ) )
               return mprog_veval( get_curr_str( chkchar ), opr, atoi( rval ), mob );
            if( !str_cmp( chck, "sex" ) )
               return mprog_veval( chkchar->sex, opr, atoi( rval ), mob );
            break;

         case 'W':
            if( !str_cmp( chck, "wis" ) )
               return mprog_veval( get_curr_wis( chkchar ), opr, atoi( rval ), mob );
            if( !str_cmp( chck, "wasinroom" ) )
               return !chkchar->was_in_room ? false : mprog_veval( chkchar->was_in_room->vnum, opr, atoi( rval ), mob );
            /* Is char wearing some eq on a specific wear loc?  -- Gorog */
            if( !str_cmp( chck, "wearing" ) )
            {
               int i = 0;

               for( tobj = chkchar->first_carrying; tobj; tobj = tobj->next_content )
               {
                  i++;
                  if( chkchar == tobj->carried_by && tobj->wear_loc > -1 && !str_cmp( rval, item_w_flags[tobj->wear_loc] ) )
                  {
                     if( !str_cmp( opr, "!=" ) )
                        return false;
                     return true;
                  }
               }
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            /* Is char wearing some specific vnum?  -- Gorog */
            if( !str_cmp( chck, "wearingvnum" ) )
            {
               if( !is_number( rval ) )
               {
                  progbug_printf( mob, "WearingVnum is checking for [%s] instead of a vnum.", rval );
                  return BERR;
               }
               vnum = atoi( rval );
               for( tobj = chkchar->first_carrying; tobj; tobj = tobj->next_content )
               {
                  if( tobj->pIndexData->vnum == vnum && chkchar == tobj->carried_by && tobj->wear_loc > -1 )
                  {
                     if( !str_cmp( opr, "!=" ) )
                        return false;
                     return true;
                  }
               }
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            if( !str_cmp( chck, "waitstate" ) )
            {
               if( is_npc( chkchar ) || !chkchar->wait )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               return mprog_veval( chkchar->wait, opr, atoi( rval ), mob );
            }
            break;
      }
   }

   if( chkobj )
   {
      switch( UPPER( chck[0] ) )
      {
         default:
            break;

         case 'C':
            /* Check to see if the container contains the specified vnum */
            if( !str_cmp( chck, "contobjvnum" ) )
            {
               OBJ_DATA *ovnum;

               vnum = atoi( rval );
               for( ovnum = chkobj->first_content; ovnum; ovnum = ovnum->next_content )
               {
                  if( ovnum->pIndexData->vnum == vnum )
                  {
                     if( !str_cmp( opr, "!=" ) )
                        return false;
                     return true;
                  }
               }
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            /* Check to see if the container contains the specified amount */
            if( !str_cmp( chck, "contobjcount" ) )
            {
               OBJ_DATA *ovnum;
               int count = 0;

               for( ovnum = chkobj->first_content; ovnum; ovnum = ovnum->next_content )
                  count += ovnum->count;

               return mprog_veval( count, opr, atoi( rval ), mob );
            }
            break;

         case 'L':
            if( !str_cmp( chck, "leverpos" ) )
            {
               int isup = false, wantsup = false;

               if( chkobj->item_type != ITEM_SWITCH && chkobj->item_type != ITEM_LEVER
               && chkobj->item_type != ITEM_PULLCHAIN && chkobj->item_type != ITEM_BUTTON )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               if( IS_SET( obj->value[0], TRIG_UP ) )
                  isup = true;
               if( !str_cmp( rval, "up" ) )
                  wantsup = true;
               return mprog_veval( wantsup, opr, isup, mob );
            }
            break;

         case 'O':
            if( !str_cmp( chck, "objtype" ) )
            {
               if( !is_number( cvar ) )
                  vnum = get_flag( cvar, o_types, ITEM_TYPE_MAX );
               else
                  vnum = atoi( rval );
               if( vnum < 0 || vnum >= ITEM_TYPE_MAX )
               {
                  progbug( "ObjType: invalid type", mob );
                  return BERR;
               }
               return mprog_veval( chkobj->item_type, opr, vnum, mob );
            }
            if( !str_cmp( chck, "objval0" ) )
               return mprog_veval( chkobj->value[0], opr, atoi( rval ), mob );
            if( !str_cmp( chck, "objval1" ) )
               return mprog_veval( chkobj->value[1], opr, atoi( rval ), mob );
            if( !str_cmp( chck, "objval2" ) )
               return mprog_veval( chkobj->value[2], opr, atoi( rval ), mob );
            if( !str_cmp( chck, "objval3" ) )
               return mprog_veval( chkobj->value[3], opr, atoi( rval ), mob );
            if( !str_cmp( chck, "objval4" ) )
               return mprog_veval( chkobj->value[4], opr, atoi( rval ), mob );
            if( !str_cmp( chck, "objval5" ) )
               return mprog_veval( chkobj->value[5], opr, atoi( rval ), mob );
            break;
      }
   }

   /*
    * The following checks depend on the fact that cval[1] can only contain
    * one character, and that NULL checks were made previously.
    */
   switch( UPPER( chck[0] ) )
   {
      default:
         break;

      case 'C':
         if( !str_cmp( chck, "charcount" ) ) /* -- Gorog */
         {
            CHAR_DATA *tch;
            ROOM_INDEX_DATA *room;
            int count = -1;

            room = get_room_index( vnum ? vnum : mob->in_room->vnum );
            for( tch = room ? room->first_person : NULL; tch; tch = tch->next_in_room )
               if( is_npc( tch ) || get_trust( tch ) < PERM_IMM )  /* mortal or mob */
                  count++;
            return mprog_veval( count, opr, atoi( rval ), mob );
         }
         break;

      case 'D':
         if( !str_cmp( chck, "day" ) )
         {
            struct tm *time;

            time = localtime( &current_time );
            return mprog_veval( time->tm_mday, opr, atoi( rval ), mob );
         }
         break;

      case 'H':
         if( !str_cmp( chck, "hour" ) )
         {
            struct tm *time;

            time = localtime( &current_time );
            return mprog_veval( time->tm_hour, opr, atoi( rval ), mob );
         }
         break;

      case 'I':
         if( !str_cmp( chck, "inroom" ) )
         {
            if( ( !chkchar || !chkchar->in_room ) && ( !chkobj || !chkobj->in_room ) )
            {
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            else if( chkchar )
               return mprog_veval( chkchar->in_room->vnum, opr, atoi( rval ), mob );
            else if( chkobj )
               return mprog_veval( chkobj->in_room->vnum, opr, atoi( rval ), mob );
         }
         break;

      case 'L':
         if( !str_cmp( chck, "level" ) )
         {
            if( !chkchar && !chkobj )
            {
               if( !str_cmp( opr, "!=" ) )
                  return true;
               return false;
            }
            else if( chkchar )
               return mprog_veval( chkchar->level, opr, atoi( rval ), mob );
            else if( chkobj )
               return mprog_veval( chkobj->level, opr, atoi( rval ), mob );
         }
         break;

      case 'M':
         if( !str_cmp( chck, "mhour" ) )
            return mprog_veval( time_info.hour, opr, atoi( rval ), mob );
         if( !str_cmp( chck, "mday" ) )
            return mprog_veval( time_info.day, opr, atoi( rval ), mob );
         if( !str_cmp( chck, "mmonth" ) )
            return mprog_veval( time_info.month, opr, atoi( rval ), mob );
         if( !str_cmp( chck, "myear" ) )
            return mprog_veval( time_info.year, opr, atoi( rval ), mob );
         if( !str_cmp( chck, "month" ) )
         {
            struct tm *time;

            time = localtime( &current_time );
            return mprog_veval( time->tm_mon, opr, atoi( rval ), mob );
         }
         if( !str_cmp( chck, "mortcount" ) ) /* -- Gorog */
         {
            CHAR_DATA *tch;
            ROOM_INDEX_DATA *room;
            int count = 0;

            room = get_room_index( vnum ? vnum : mob->in_room->vnum );
            for( tch = room ? room->first_person : NULL; tch; tch = tch->next_in_room )
               if( ( !is_npc( tch ) ) && get_trust( tch ) < PERM_IMM )
                  count++;
            return mprog_veval( count, opr, atoi( rval ), mob );
         }

         if( !str_cmp( chck, "mobcount" ) )  /* -- Gorog */
         {
            CHAR_DATA *tch;
            ROOM_INDEX_DATA *room;
            int count = -1;

            room = get_room_index( vnum ? vnum : mob->in_room->vnum );
            for( tch = room ? room->first_person : NULL; tch; tch = tch->next_in_room )
               if( ( is_npc( tch ) ) )
                  count++;
            return mprog_veval( count, opr, atoi( rval ), mob );
         }
         break;

      case 'N':
         if( !str_cmp( chck, "name" ) )
            return mprog_seval( chkchar ? chkchar->name : chkobj->name, opr, rval, mob );
         break;

      case 'R':
         if( !str_cmp( chck, "rank" ) ) /* Shaddai */
         {
            if( chkchar && !is_npc( chkchar ) )
               return mprog_seval( chkchar->pcdata->rank, opr, rval, mob );
            if( !str_cmp( opr, "!=" ) )
               return true;
            return false;
         }
         break;

      case 'V':
         if( !str_cmp( chck, "vnum" ) )
         {
            if( chkchar )
            {
               if( !is_npc( chkchar ) )
               {
                  if( !str_cmp( opr, "!=" ) )
                     return true;
                  return false;
               }
               lhsvl = chkchar->pIndexData->vnum;
               return mprog_veval( lhsvl, opr, atoi( rval ), mob );
            }
            else if( chkobj )
               return mprog_veval( chkobj->pIndexData->vnum, opr, atoi( rval ), mob );
            else
            {
               progbug( "Vnum ifcheck with NULL chkchar and NULL chkobj.", mob );
               return BERR;
            }
         }
         break;

      case 'Y':
         if( !str_cmp( chck, "year" ) )
         {
            struct tm *time;

            time = localtime( &current_time );
            return mprog_veval( ( time->tm_year + 1900 ), opr, atoi( rval ), mob );
         }
         break;
   }

   /*
    * Ok... all the ifchecks are done, so if we didnt find ours then something
    * odd happened.  So report the bug and abort the MUDprogram (return error)
    */
   progbug_printf( mob, "Unknown ifcheck [%s]", chck );
   return BERR;
}

#undef isoperator
#undef MAX_IF_ARGS

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object 
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
/*
 *  There's no reason to make the mud crash when a variable's
 *  fubared.  I added some ifs.  I'm willing to trade some 
 *  performance for stability. -Haus
 *
 *  Added char_died checks	-Thoric
 */
void mprog_translate( char ch, char *t, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, CHAR_DATA *rndm )
{
   CHAR_DATA *vict = ( CHAR_DATA * ) vo;
   OBJ_DATA *v_obj = ( OBJ_DATA * ) vo;

   if( v_obj && v_obj->pIndexData )
      vict = NULL;
   else
      v_obj = NULL;

   *t = '\0';
   switch( ch )
   {
      case 'i':
         if( mob && !char_died( mob ) && mob->name )
            one_argument( mob->name, t );
         else
            strcpy( t, "someone" );
         break;

      case 'I':
         if( mob && !char_died( mob ) && mob->short_descr )
            strcpy( t, mob->short_descr );
         else
            strcpy( t, "someone" );
         break;

      case 'n':
         if( actor && !char_died( actor ) && can_see( mob, actor ) )
         {
            one_argument( actor->name, t );
            if( !is_npc( actor ) )
               *t = UPPER( *t );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'N':
         if( actor && !char_died( actor ) && can_see( mob, actor ) )
         {
            if( is_npc( actor ) )
               strcpy( t, actor->short_descr );
            else
            {
               strcpy( t, actor->name );
               mudstrlcat( t, actor->pcdata->title, MSL );
            }
         }
         else
            strcpy( t, "someone" );
         break;

      case 'f':
         if( actor && !char_died( actor ) && actor->fighting && actor->fighting->who
         && !char_died( actor->fighting->who ) && can_see( mob, actor->fighting->who ) )
         {
            strcpy( t, actor->fighting->who->name );
            one_argument( actor->fighting->who->name, t );
            if( !is_npc( actor ) )
               *t = UPPER( *t );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'F': /* Used to display the name of who you are fighting */
         if( actor && !char_died( actor ) && actor->fighting && actor->fighting->who
         && !char_died( actor->fighting->who ) && can_see( mob, actor->fighting->who ) )
         {
            if( is_npc( actor->fighting->who ) )
               strcpy( t, actor->fighting->who->short_descr );
            else
            {
               strcpy( t, actor->fighting->who->name );
               *t = UPPER( *t );
            }
         }
         else
            strcpy( t, "someone" );
         break;

      case 't':
         if( vict && !char_died( vict ) && can_see( mob, vict ) )
         {
            one_argument( vict->name, t );
            if( !is_npc( vict ) )
               *t = UPPER( *t );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'T':
         if( vict && !char_died( vict ) && can_see( mob, vict ) )
         {
            if( is_npc( vict ) )
               strcpy( t, vict->short_descr );
            else
            {
               strcpy( t, vict->name );
               mudstrlcat( t, " ", MSL );
               mudstrlcat( t, vict->pcdata->title, MSL );
            }
         }
         else
            strcpy( t, "someone" );
         break;

      case 'r':
         if( rndm && !char_died( rndm ) && can_see( mob, rndm ) )
         {
            one_argument( rndm->name, t );
            if( !is_npc( rndm ) )
               *t = UPPER( *t );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'R':
         if( rndm && !char_died( rndm ) && can_see( mob, rndm ) )
         {
            if( is_npc( rndm ) )
               strcpy( t, rndm->short_descr );
            else
            {
               strcpy( t, rndm->name );
               mudstrlcat( t, " ", MSL );
               mudstrlcat( t, rndm->pcdata->title, MSL );
            }
         }
         else
            strcpy( t, "someone" );
         break;

      case 'e':
         if( actor && !char_died( actor ) && can_see( mob, actor ) )
            strcpy( t, he_she[actor->sex] );
         else
            strcpy( t, "it" );
         break;

      case 'm':
         if( actor && !char_died( actor ) && can_see( mob, actor ) )
            strcpy( t, him_her[actor->sex] );
         else
            strcpy( t, "it" );
         break;

      case 's':
         if( actor && !char_died( actor ) && can_see( mob, actor ) )
            strcpy( t, his_her[actor->sex] );
         else
            strcpy( t, "its'" );
         break;

      case 'E':
         if( vict && !char_died( vict ) && can_see( mob, vict ) )
            strcpy( t, he_she[vict->sex] );
         else
            strcpy( t, "it" );
         break;

      case 'M':
         if( vict && !char_died( vict ) && can_see( mob, vict ) )
            strcpy( t, him_her[vict->sex] );
         else
            strcpy( t, "it" );
         break;

      case 'S':
         if( vict && !char_died( vict ) && can_see( mob, vict ) )
            strcpy( t, his_her[vict->sex] );
         else
            strcpy( t, "its'" );
         break;

      case 'j':
         if( mob && !char_died( mob ) )
            strcpy( t, he_she[mob->sex] );
         else
            strcpy( t, "it" );
         break;

      case 'k':
         if( mob && !char_died( mob ) )
            strcpy( t, him_her[mob->sex] );
         else
            strcpy( t, "it" );
         break;

      case 'l':
         if( mob && !char_died( mob ) )
            strcpy( t, his_her[mob->sex] );
         else
            strcpy( t, "it" );
         break;

      case 'J':
         if( rndm && !char_died( rndm ) && can_see( mob, rndm ) )
            strcpy( t, he_she[rndm->sex] );
         else
            strcpy( t, "it" );
         break;

      case 'K':
         if( rndm && !char_died( rndm ) && can_see( mob, rndm ) )
            strcpy( t, him_her[rndm->sex] );
         else
            strcpy( t, "its'" );
         break;

      case 'L':
         if( rndm && !char_died( rndm ) && can_see( mob, rndm ) )
            strcpy( t, his_her[rndm->sex] );
         else
            strcpy( t, "its" );
         break;

      case 'o':
         if( obj && can_see_obj( mob, obj ) )
            one_argument( obj->name, t );
         else
            strcpy( t, "something" );
         break;

      case 'O':
         if( obj && can_see_obj( mob, obj ) )
            strcpy( t, obj->short_descr );
         else
            strcpy( t, "something" );
         break;

      case 'w':
         if( obj && obj->carried_by && obj->wear_loc != -1 && can_see_obj( mob, obj ) && can_see( mob, obj->carried_by ) )
            one_argument( obj->carried_by->name, t );
         else
            strcpy( t, "noone" );
         break;

      case 'W':
         if( obj && obj->carried_by && obj->wear_loc != -1 && can_see_obj( mob, obj ) && can_see( mob, obj->carried_by ) )
         {
            if( is_npc( obj->carried_by ) )
              strcpy( t, obj->carried_by->short_descr );
            else
            {
               strcpy( t, obj->carried_by->name );
               *t = UPPER( *t );
            }
         }
         else
            strcpy( t, "noone" );
         break;

      case 'z':
         if( v_obj && v_obj->carried_by && v_obj->wear_loc != -1 && can_see_obj( mob, v_obj ) && can_see( mob, v_obj->carried_by ) )
            one_argument( v_obj->carried_by->name, t );
         else
            strcpy( t, "noone" );
         break;

      case 'Z':
         if( v_obj && v_obj->carried_by && v_obj->wear_loc != -1 && can_see_obj( mob, v_obj ) && can_see( mob, v_obj->carried_by ) )
         {
            if( is_npc( v_obj->carried_by ) )
              strcpy( t, v_obj->carried_by->short_descr );
            else
            {
               strcpy( t, v_obj->carried_by->name );
               *t = UPPER( *t );
            }
         }
         else
            strcpy( t, "noone" );
         break;

      case 'p':
         if( v_obj && can_see_obj( mob, v_obj ) )
            one_argument( v_obj->name, t );
         else
            strcpy( t, "something" );
         break;

      case 'P':
         if( v_obj && can_see_obj( mob, v_obj ) )
            strcpy( t, v_obj->short_descr );
         else
            strcpy( t, "something" );
         break;

      case 'a':
         if( obj )
            strcpy( t, aoran( obj->name ) );
         else
            strcpy( t, "a" );
         break;

      case 'A':
         if( v_obj )
            strcpy( t, aoran( v_obj->name ) );
         else
            strcpy( t, "a" );
         break;

      case '$':
         strcpy( t, "$" );
         break;

      default:
         progbug_printf( mob, "Bad $var %c", ch );
         break;
   }
}

/*
 * The main focus of the MOBprograms.  This routine is called 
 * whenever a trigger is successful.  It is responsible for parsing
 * the command list and figuring out what to do. However, like all
 * complex procedures, everything is farmed out to the other guys.
 *
 * This function rewritten by Narn for Realms of Despair, Dec/95.
 */
void mprog_driver( char *com_list, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, bool single_step )
{
//   char tmpcmndlst[MSL], *command_list, *cmnd;
   char tmpcmndlst[MSL], *command_list, cmnd[MSL];
   CHAR_DATA *rndm = NULL, *vch = NULL;
   int count = 0, iflevel = 0, result, ignorelevel = 0;
   bool ifstate[MAX_IFS][DO_ELSE + 1];
   static int prog_nest;

   if( IS_AFFECTED( mob, AFF_CHARM ) )
      return;

   /* Next couple of checks stop program looping. -- Altrag */
   if( mob == actor )
   {
      progbug( "triggering oneself.", mob );
      return;
   }

   if( ++prog_nest > MAX_PROG_NEST )
   {
      progbug( "max_prog_nest exceeded.", mob );
      --prog_nest;
      return;
   }

   /* Make sure all ifstate bools are set to false */
   for( iflevel = 0; iflevel < MAX_IFS; iflevel++ )
   {
      for( count = 0; count < DO_ELSE; count++ )
      {
         ifstate[iflevel][count] = false;
      }
   }

   iflevel = 0;

   /*
    * get a random visible player who is in the room with the mob.
    *
    * If there isn't a random player in the room, rndm stays NULL.
    * If you do a $r, $R, $j, or $k with rndm = NULL, you'll crash
    * in mprog_translate.
    *
    * Adding appropriate error checking in mprog_translate. - Haus
    *
    * This used to ignore players MAX_LEVEL - 3 and higher (standard
    * Merc has 4 immlevels).  Thought about changing it to ignore all
    * imms, but decided to just take it out.  If the mob can see you, 
    * you may be chosen as the random player. -Narn
    */

   count = 0;
   for( vch = mob->in_room->first_person; vch; vch = vch->next_in_room )
      if( !is_npc( vch ) && can_see( mob, vch ) )
      {
         if( number_range( 0, count ) == 0 )
            rndm = vch;
         count++;
      }

   /* If we have a NULL com_list just return */
   if( !com_list )
      return;

   strcpy( tmpcmndlst, com_list );
   command_list = tmpcmndlst;
   if( single_step )
   {
      if( mob->mpscriptpos > strlen( tmpcmndlst ) )
         mob->mpscriptpos = 0;
      else
         command_list += mob->mpscriptpos;
      if( *command_list == '\0' )
      {
         command_list = tmpcmndlst;
         mob->mpscriptpos = 0;
      }
   }

   /*
    * From here on down, the function is all mine.  The original code
    * did not support nested ifs, so it had to be redone.  The max 
    * logiclevel (MAX_IFS) is defined at the beginning of this file, 
    * use it to increase/decrease max allowed nesting.  -Narn 
    */
   while( true )
   {
      /*
       * With these two lines, cmnd becomes the current line from the prog,
       * and command_list becomes everything after that line. 
       */
//      cmnd = command_list;
//      command_list = mprog_next_command( command_list );

      command_list = mprog_one_command( command_list, cmnd );

      /* Are we at the end? */
      if( cmnd[0] == '\0' )
      {
         if( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] )
         {
            progbug( "Missing endif", mob );
         }
         --prog_nest;
         return;
      }

      /* Evaluate/execute the command, check what happened. */
      result = mprog_do_command( cmnd, mob, actor, obj, vo, rndm,
         ( ifstate[iflevel][IN_IF] && !ifstate[iflevel][DO_IF] )
         || ( ifstate[iflevel][IN_ELSE] && !ifstate[iflevel][DO_ELSE] ), ( ignorelevel > 0 ) );

      /* Script prog support  -Thoric */
      if( single_step )
      {
         bug( "%s: single_step", __FUNCTION__ );
         mob->mpscriptpos = command_list - tmpcmndlst;
         --prog_nest;
         return;
      }

      /*
       * This is the complicated part.  Act on the returned value from
       * mprog_do_command according to the current logic state. 
       */
      switch( result )
      {
         case COMMANDOK:
            /* Ok, this one's a no-brainer. */
            continue;

         case IFTRUE:
            /*
             * An if was evaluated and found true.  Note that we are in an
             * if section and that we want to execute it. 
             */
            iflevel++;
            if( iflevel == MAX_IFS )
            {
               progbug( "Maximum nested ifs exceeded", mob );
               --prog_nest;
               return;
            }

            ifstate[iflevel][IN_IF] = true;
            ifstate[iflevel][DO_IF] = true;
            break;

         case IFFALSE:
            /*
             * An if was evaluated and found false.  Note that we are in an
             * if section and that we don't want to execute it unless we find
             * an or that evaluates to true. 
             */
            iflevel++;
            if( iflevel == MAX_IFS )
            {
               progbug( "Maximum nested ifs exceeded", mob );
               --prog_nest;
               return;
            }
            ifstate[iflevel][IN_IF] = true;
            ifstate[iflevel][DO_IF] = false;
            break;

         case ORTRUE:
            /* An or was evaluated and found true.  We should already be in an if section, so note that we want to execute it. */
            if( !ifstate[iflevel][IN_IF] )
            {
               progbug( "Unmatched or", mob );
               --prog_nest;
               return;
            }
            ifstate[iflevel][DO_IF] = true;
            break;

         case ORFALSE:
            /*
             * An or was evaluated and found false.  We should already be in an
             * if section, and we don't need to do much.  If the if was true or
             * there were/will be other ors that evaluate(d) to true, they'll set
             * do_if to true. 
             */
            if( !ifstate[iflevel][IN_IF] )
            {
               progbug( "Unmatched or", mob );
               --prog_nest;
               return;
            }
            continue;

         case FOUNDELSE:
            /*
             * Found an else.  Make sure we're in an if section, bug out if not.
             * If this else is not one that we wish to ignore, note that we're now 
             * in an else section, and look at whether or not we executed the if 
             * section to decide whether to execute the else section.  Ca marche 
             * bien. 
             */
            if( ignorelevel > 0 )
               continue;

            if( ifstate[iflevel][IN_ELSE] )
            {
               progbug( "Found else in an else section", mob );
               --prog_nest;
               return;
            }
            if( !ifstate[iflevel][IN_IF] )
            {
               progbug( "Unmatched else", mob );
               --prog_nest;
               return;
            }

            ifstate[iflevel][IN_ELSE] = true;
            ifstate[iflevel][DO_ELSE] = !ifstate[iflevel][DO_IF];
            ifstate[iflevel][IN_IF] = false;
            ifstate[iflevel][DO_IF] = false;

            break;

         case FOUNDENDIF:
            /*
             * Hmm, let's see... FOUNDENDIF must mean that we found an endif.
             * So let's make sure we were expecting one, return if not.  If this
             * endif matches the if or else that we're executing, note that we are 
             * now no longer executing an if.  If not, keep track of what we're 
             * ignoring. 
             */
            if( !( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] ) )
            {
               progbug( "Unmatched endif", mob );
               --prog_nest;
               return;
            }

            if( ignorelevel > 0 )
            {
               ignorelevel--;
               continue;
            }

            ifstate[iflevel][IN_IF] = false;
            ifstate[iflevel][DO_IF] = false;
            ifstate[iflevel][IN_ELSE] = false;
            ifstate[iflevel][DO_ELSE] = false;

            iflevel--;
            break;

         case IFIGNORED:
            if( !( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] ) )
            {
               progbug( "Parse error, ignoring if while not in if or else", mob );
               --prog_nest;
               return;
            }
            ignorelevel++;
            break;

         case ORIGNORED:
            if( !( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] ) )
            {
               progbug( "Unmatched or", mob );
               --prog_nest;
               return;
            }
            if( ignorelevel == 0 )
            {
               progbug( "Parse error, mistakenly ignoring or", mob );
               --prog_nest;
               return;
            }
            break;

         case BERR:
            --prog_nest;
            return;
      }
   }
}

/*
 * This function replaces mprog_process_cmnd.  It is called from 
 * mprog_driver, once for each line in a mud prog.  This function
 * checks what the line is, executes if/or checks and calls interpret
 * to perform the the commands.  Written by Narn, Dec 95.
 */
int mprog_do_command( char *cmnd, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, CHAR_DATA *rndm, bool ignore, bool ignore_ors )
{
   char firstword[MIL], *ifcheck, buf[MIL], tmp[MIL], *point, *str, *i;
   int validif, vnum;

   /* Isolate the first word of the line, it gives us a clue what we want to do. */
   ifcheck = one_argument( cmnd, firstword );

   if( !str_cmp( firstword, "if" ) )
   {
      /*
       * Ok, we found an if.  According to the boolean 'ignore', either
       * ignore the ifcheck and report that back to mprog_driver or do
       * the ifcheck and report whether it was successful. 
       */
      if( ignore )
         return IFIGNORED;
      else
         validif = mprog_do_ifcheck( ifcheck, mob, actor, obj, vo, rndm );

      if( validif == 1 )
         return IFTRUE;

      if( validif == 0 )
         return IFFALSE;

      return BERR;
   }

   if( !str_cmp( firstword, "or" ) )
   {
      /* Same behavior as with ifs, but use the boolean 'ignore_ors' to decide which way to go. */
      if( ignore_ors )
         return ORIGNORED;
      else
         validif = mprog_do_ifcheck( ifcheck, mob, actor, obj, vo, rndm );

      if( validif == 1 )
         return ORTRUE;

      if( validif == 0 )
         return ORFALSE;

      return BERR;
   }

   /* For else and endif, just report back what we found.  Mprog_driver keeps track of logiclevels. */
   if( !str_cmp( firstword, "else" ) )
   {
      return FOUNDELSE;
   }

   if( !str_cmp( firstword, "endif" ) )
   {
      return FOUNDENDIF;
   }

   /*
    * Ok, didn't find an if, an or, an else or an endif.  
    * If the command is in an if or else section that is not to be 
    * performed, the boolean 'ignore' is set to true and we just 
    * return.  If not, we try to execute the command. 
    */
   if( ignore )
      return COMMANDOK;

   /* If the command is 'break', that's all folks. */
   if( !str_cmp( firstword, "break" ) )
      return BERR;

   cmnd = parse_var( cmnd, mob, actor, obj, vo, rndm );

   vnum = mob->pIndexData->vnum;
   point = buf;
   str = cmnd;

   /* This chunk of code taken from mprog_process_cmnd. */
   while( *str != '\0' )
   {
      if( *str != '$' )
      {
         *point++ = *str++;
         continue;
      }
      str++;
      mprog_translate( *str, tmp, mob, actor, obj, vo, rndm );
      i = tmp;
      ++str;
      while( ( *point = *i ) != '\0' )
         ++point, ++i;
   }
   *point = '\0';

   interpret( mob, buf );

   /*
    * If the mob is mentally unstable and does things like fireball
    * itself, let's make sure it's still alive. 
    */
   if( char_died( mob ) )
   {
      bug( "Mob died while executing program, vnum %d.", vnum );
      return BERR;
   }

   return COMMANDOK;
}

/* Global function code and brief comments. */
bool mprog_keyword_check( const char *argu, const char *argl )
{
   char word[MIL], arg1[MIL], arg2[MIL], *arg, *arglist, *start, *end;
   unsigned int i;

   strcpy( arg1, strlower( argu ) );
   arg = arg1;
   strcpy( arg2, strlower( argl ) );
   arglist = arg2;

   for( i = 0; i < strlen( arglist ); i++ )
      arglist[i] = LOWER( arglist[i] );
   for( i = 0; i < strlen( arg ); i++ )
      arg[i] = LOWER( arg[i] );
   if( ( arglist[0] == 'p' ) && ( arglist[1] == ' ' ) )
   {
      arglist += 2;
      while( ( start = strstr( arg, arglist ) ) )
      {
         if( ( start == arg || *( start - 1 ) == ' ' )
         && ( *( end = start + strlen( arglist ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
            return true;
         else
            arg = start + 1;
      }
   }
   else
   {
      arglist = one_argument( arglist, word );
      for( ; word[0] != '\0'; arglist = one_argument( arglist, word ) )
      {
         while( ( start = strstr( arg, word ) ) )
         {
            if( ( start == arg || *( start - 1 ) == ' ' )
            && ( *( end = start + strlen( word ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
               return true;
            else
               arg = start + 1;
         }
      }
   }
   return false;
}

/*
 * The next two routines are the basic trigger types. Either trigger
 * on a certain percent, or trigger on a keyword or word phrase.
 * To see how this works, look at the various trigger routines..
 */
void mprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type )
{
   char temp1[MSL], temp2[MIL], word[MIL], *list, *start, *dupl, *end;
   MPROG_DATA *mprg;
   unsigned int i;

   for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
   {
      if( mprg->type == type )
      {
         if( !mprg->arglist )
            continue;
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); i++ )
            list[i] = LOWER( list[i] );
         strcpy( temp2, arg );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); i++ )
            dupl[i] = LOWER( dupl[i] );
         if( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
         {
            list += 2;
            while( ( start = strstr( dupl, list ) ) )
            {
               if( ( start == dupl || *( start - 1 ) == ' ' )
               && ( *( end = start + strlen( list ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
               {
                  mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
                  break;
               }
               else
                  dupl = start + 1;
            }
         }
         else
         {
            list = one_argument( list, word );
            for( ; word[0] != '\0'; list = one_argument( list, word ) )
            {
               while( ( start = strstr( dupl, word ) ) )
               {
                  if( ( start == dupl || *( start - 1 ) == ' ' )
                  && ( *( end = start + strlen( word ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
                  {
                     mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
                     break;
                  }
                  else
                     dupl = start + 1;
               }
            }
         }
      }
   }
}

void mprog_percent_check( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type )
{
   MPROG_DATA *mprg;

   for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
   {
      if( !mprg || !mprg->comlist || !mprg->arglist || !is_number( mprg->arglist ) )
         continue;

      if( ( mprg->type == type ) && ( number_percent( ) <= atoi( mprg->arglist ) ) )
      {
         mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
         if( type != GREET_PROG && type != ALL_GREET_PROG )
            break;
      }
   }
}

void mprog_time_check( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type )
{
   MPROG_DATA *mprg;
   bool trigger_time;

   for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
   {
      if( !mprg || !mprg->comlist || !mprg->arglist || !is_number( mprg->arglist ) )
         continue;

      if( !( trigger_time = ( time_info.hour == atoi( mprg->arglist ) ) ) )
      {
         if( mprg->triggered )
            mprg->triggered = false;
         continue;
      }

      if( ( mprg->type == type ) && ( ( !mprg->triggered ) || ( mprg->type == HOUR_PROG ) ) )
      {
         mprg->triggered = true;
         mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
      }
   }
}

void mob_act_add( CHAR_DATA *mob )
{
   struct act_prog_data *runner, *tmp_mal;

   for( runner = mob_act_list; runner; runner = runner->next )
      if( runner->vo == mob )
         return;

   CREATE( runner, struct act_prog_data, 1 );
   runner->vo = mob;
   runner->next = NULL;
   /*
    * The head of the list is being changed in
    * aggr_update, So append to the end of the list instead. - Druid
    */
   if( mob_act_list )
   {
      tmp_mal = mob_act_list;

      while( tmp_mal->next )
         tmp_mal = tmp_mal->next;

      /* put at the end */
      tmp_mal->next = runner;
   }
   else
      mob_act_list = runner;
}

/*
 * The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void mprog_act_trigger( char *buf, CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj, void *vo )
{
   MPROG_ACT_LIST *tmp_act, *tmp_mal;
   MPROG_DATA *mprg;
   bool found = false;

   if( is_npc( mob ) && HAS_PROG( mob->pIndexData, ACT_PROG ) )
   {
      /* Don't let a mob trigger itself, nor one instance of a mob trigger another instance. */
      if( is_npc( ch ) && ch->pIndexData == mob->pIndexData )
         return;

      /* make sure this is a matching trigger */
      for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
      {
         if( mprg->type == ACT_PROG && mprog_keyword_check( buf, mprg->arglist ) )
         {
            found = true;
            break;
         }
      }
      if( !found )
         return;

      CREATE( tmp_act, MPROG_ACT_LIST, 1 );
      /* Losing the head of the list -Druid */
      if( mob->mpactnum > 0 )
      {
         tmp_mal = mob->mpact;

         while( tmp_mal->next )
            tmp_mal = tmp_mal->next;

         /* Put at the end */
         tmp_mal->next = tmp_act;
      }
      else
         mob->mpact = tmp_act;

      tmp_act->next = NULL;
      tmp_act->buf = STRALLOC( buf );
      tmp_act->ch = ch;
      tmp_act->obj = obj;
      tmp_act->vo = vo;
      mob->mpactnum++;
      mob_act_add( mob );
   }
}

void mprog_bribe_trigger( CHAR_DATA *mob, CHAR_DATA *ch, int amount )
{
   char buf[MSL];
   MPROG_DATA *mprg;
   OBJ_DATA *obj;
   int useprog = 0, amdiff = 0, uamount = 0, onprog;

   if( is_npc( mob ) && can_see( mob, ch ) && HAS_PROG( mob->pIndexData, BRIBE_PROG ) )
   {
      /* Don't let a mob trigger itself, nor one instance of a mob trigger another instance. */
      if( is_npc( ch ) && ch->pIndexData == mob->pIndexData )
         return;

      obj = create_object( get_obj_index( OBJ_VNUM_MONEY_SOME ), 0 );
      if( !obj )
      {
         bug( "%s: failed to create object using vnum %d.", __FUNCTION__, OBJ_VNUM_MONEY_SOME );
         return;
      }

      snprintf( buf, sizeof( buf ), obj->short_descr, num_punct( amount ) );
      STRSET( obj->short_descr, buf );
      obj->value[0] = amount;
      obj = obj_to_char( obj, mob );
      decrease_gold( mob, amount );

      uamount = ( obj->value[0] * obj->count );

      /* Go through once to see which we should use */
      onprog = 0;
      for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
      {
         ++onprog;
         if( ( mprg->type == BRIBE_PROG ) && ( uamount >= atoi( mprg->arglist ) ) && ( amdiff == 0 || ( uamount - atoi( mprg->arglist ) ) < amdiff ) )
         {
            amdiff = ( uamount - atoi( mprg->arglist ) );
            useprog = onprog;
         }
      }

      /* Go through it again and use the closest bribe program to the amount */
      onprog = 0;
      for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
      {
         if( ++onprog != useprog )
            continue;

         if( ( mprg->type == BRIBE_PROG ) && ( uamount >= atoi( mprg->arglist ) ) )
         {
            increase_gold( mob, uamount );
            mprog_driver( mprg->comlist, mob, ch, obj, NULL, false );
            if( obj )
               extract_obj( obj );
            break;
         }
      }
   }
}

void mprog_death_trigger( CHAR_DATA *killer, CHAR_DATA *mob )
{
   if( is_npc( mob ) && killer != mob && HAS_PROG( mob->pIndexData, DEATH_PROG ) )
   {
      mob->position = POS_STANDING;
      mprog_percent_check( mob, killer, NULL, NULL, DEATH_PROG );
      mob->position = POS_DEAD;
   }
   death_cry( mob );
}

void mprog_entry_trigger( CHAR_DATA *mob )
{
   if( is_npc( mob ) && HAS_PROG( mob->pIndexData, ENTRY_PROG ) )
      mprog_percent_check( mob, NULL, NULL, NULL, ENTRY_PROG );
}

void mprog_fight_trigger( CHAR_DATA *mob, CHAR_DATA *ch )
{
   if( is_npc( mob ) && HAS_PROG( mob->pIndexData, FIGHT_PROG ) )
      mprog_percent_check( mob, ch, NULL, NULL, FIGHT_PROG );
}

void mprog_give_trigger( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj )
{
   MPROG_DATA *mprg;
   char buf[MIL];
   int vnum;

   if( is_npc( mob ) && can_see( mob, ch ) && HAS_PROG( mob->pIndexData, GIVE_PROG ) )
   {
      /* Don't let a mob trigger itself, nor one instance of a mob trigger another instance. */
      if( is_npc( ch ) && ch->pIndexData == mob->pIndexData )
         return;

      for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
      {
         one_argument( mprg->arglist, buf );

         vnum = -1;
         if( is_number( buf ) )
            vnum = atoi( buf );

         if( mprg->type == GIVE_PROG && ( !str_cmp( obj->name, buf ) || !str_cmp( "all", buf ) || ( vnum != -1 && obj->pIndexData->vnum == vnum ) ) )
         {
            mprog_driver( mprg->comlist, mob, ch, obj, NULL, false );
            break;
         }
      }
   }
}

void mprog_greet_trigger( CHAR_DATA *ch )
{
   /* We need a temporary list of the chars to keep it only firing ones it needs to, probably needed in other locations too */
   typedef struct tmp_char TMP_CHAR;
   struct tmp_char
   {
      TMP_CHAR *next, *prev;
      CHAR_DATA *who; /* Who are we checking now? */
   };
   TMP_CHAR *tmp_first = NULL, *tmp_last = NULL, *tmpmob, *tmpmob_next;
   CHAR_DATA *vmob, *vmob_next;
   int rvnum;

   if( !ch || !ch->in_room )
      return;

   rvnum = ch->in_room->vnum;

   /* Ok link them in the tmp list */
   for( vmob = ch->in_room->first_person; vmob; vmob = vmob_next )
   {
      vmob_next = vmob->next_in_room;

      if( !vmob || !vmob->in_room || vmob->in_room->vnum != rvnum || !is_npc( vmob ) || !can_see( vmob, ch )
      || vmob->fighting || !is_awake( vmob ) || char_died( vmob ) || char_died( ch ) || ( is_npc( ch ) && ch->pIndexData == vmob->pIndexData ) )
         continue;

      CREATE( tmpmob, TMP_CHAR, 1 );
      tmpmob->who = vmob;
      LINK( tmpmob, tmp_first, tmp_last, next, prev );
   }

   /* Ok now toss through the tmp list and remove them as we go */
   for( tmpmob = tmp_first; tmpmob; tmpmob = tmpmob_next )
   {
      tmpmob_next = tmpmob->next;

      vmob = tmpmob->who;

      if( !vmob || !vmob->in_room || vmob->in_room->vnum != rvnum || !is_npc( vmob ) || !can_see( vmob, ch )
      || vmob->fighting || !is_awake( vmob ) || char_died( vmob ) || char_died( ch ) || ( is_npc( ch ) && ch->pIndexData == vmob->pIndexData ) )
      {
         UNLINK( tmpmob, tmp_first, tmp_last, next, prev );
         tmpmob->who = NULL;
         DISPOSE( tmpmob );
         continue;
      }

      if( HAS_PROG( vmob->pIndexData, GREET_PROG ) )
         mprog_percent_check( vmob, ch, NULL, NULL, GREET_PROG );
      else if( HAS_PROG( vmob->pIndexData, ALL_GREET_PROG ) )
         mprog_percent_check( vmob, ch, NULL, NULL, ALL_GREET_PROG );

      UNLINK( tmpmob, tmp_first, tmp_last, next, prev );
      tmpmob->who = NULL;
      DISPOSE( tmpmob );
   }

   /* Just incase for some reason something is still in need to remove it now */
   for( tmpmob = tmp_first; tmpmob; tmpmob = tmpmob_next )
   {
      tmpmob_next = tmpmob->next;
      UNLINK( tmpmob, tmp_first, tmp_last, next, prev );
      tmpmob->who = NULL;
      DISPOSE( tmpmob );
   }
}

void mprog_hitprcnt_trigger( CHAR_DATA *mob, CHAR_DATA *ch )
{
   MPROG_DATA *mprg;

   if( is_npc( mob ) && HAS_PROG( mob->pIndexData, HITPRCNT_PROG ) )
   {
      for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
      {
         if( mprg->type == HITPRCNT_PROG && ( 100 * mob->hit / mob->max_hit ) < atoi( mprg->arglist ) )
         {
            mprog_driver( mprg->comlist, mob, ch, NULL, NULL, false );
            break;
         }
      }
   }
}

void mprog_random_trigger( CHAR_DATA *mob )
{
   if( HAS_PROG( mob->pIndexData, RAND_PROG ) )
      mprog_percent_check( mob, NULL, NULL, NULL, RAND_PROG );
}

void mprog_time_trigger( CHAR_DATA *mob )
{
   if( HAS_PROG( mob->pIndexData, TIME_PROG ) )
      mprog_time_check( mob, NULL, NULL, NULL, TIME_PROG );
}

void mprog_hour_trigger( CHAR_DATA *mob )
{
   if( HAS_PROG( mob->pIndexData, HOUR_PROG ) )
      mprog_time_check( mob, NULL, NULL, NULL, HOUR_PROG );
}

void mprog_speech_trigger( char *txt, CHAR_DATA *actor )
{
   CHAR_DATA *vmob;

   for( vmob = actor->in_room->first_person; vmob; vmob = vmob->next_in_room )
   {
      if( is_npc( vmob ) && HAS_PROG( vmob->pIndexData, SPEECH_PROG ) )
      {
         if( is_npc( actor ) && actor->pIndexData == vmob->pIndexData )
            continue;
         mprog_wordlist_check( txt, vmob, actor, NULL, NULL, SPEECH_PROG );
      }
   }
}

void mprog_script_trigger( CHAR_DATA *mob )
{
   MPROG_DATA *mprg;

   if( HAS_PROG( mob->pIndexData, SCRIPT_PROG ) )
      for( mprg = mob->pIndexData->mudprogs; mprg; mprg = mprg->next )
         if( mprg->type == SCRIPT_PROG
         && ( mprg->arglist[0] == '\0' || mob->mpscriptpos != 0 || atoi( mprg->arglist ) == time_info.hour ) )
            mprog_driver( mprg->comlist, mob, NULL, NULL, NULL, true );
}

void oprog_script_trigger( OBJ_DATA *obj )
{
   MPROG_DATA *mprg;

   if( HAS_PROG( obj->pIndexData, SCRIPT_PROG ) )
   {
      for( mprg = obj->pIndexData->mudprogs; mprg; mprg = mprg->next )
      {
         if( mprg->type == SCRIPT_PROG )
         {
            if( mprg->arglist[0] == '\0' || obj->mpscriptpos != 0 || atoi( mprg->arglist ) == time_info.hour )
            {
               set_supermob( obj );
               mprog_driver( mprg->comlist, supermob, NULL, NULL, NULL, true );
               obj->mpscriptpos = supermob->mpscriptpos;
               release_supermob( );
            }
         }
      }
   }
}

void rprog_script_trigger( ROOM_INDEX_DATA *room )
{
   MPROG_DATA *mprg;

   if( HAS_PROG( room, SCRIPT_PROG ) )
   {
      for( mprg = room->mudprogs; mprg; mprg = mprg->next )
      {
         if( mprg->type == SCRIPT_PROG )
         {
            if( mprg->arglist[0] == '\0' || room->mpscriptpos != 0 || atoi( mprg->arglist ) == time_info.hour )
            {
               rset_supermob( room );
               mprog_driver( mprg->comlist, supermob, NULL, NULL, NULL, true );
               room->mpscriptpos = supermob->mpscriptpos;
               release_supermob( );
            }
         }
      }
   }
}

/*  Mudprogram additions begin here */
void set_supermob( OBJ_DATA *obj )
{
   ROOM_INDEX_DATA *room;
   OBJ_DATA *in_obj;
   CHAR_DATA *mob;
   char buf[200];

   if( !supermob )
      supermob = create_mobile( get_mob_index( MOB_VNUM_SUPERMOB ) );

   if( !( mob = supermob ) ) /* debugging */
   {
      bug( "%s: mob is NULL after being set to supermob???", __FUNCTION__ );
      return;
   }
   if( !obj )
      return;

   supermob_obj = obj;
   for( in_obj = obj; in_obj->in_obj; in_obj = in_obj->in_obj )
      ;

   if( in_obj->carried_by )
      room = in_obj->carried_by->in_room;
   else
      room = obj->in_room;

   if( !room )
      return;

   STRSET( supermob->short_descr, obj->short_descr );
   supermob->mpscriptpos = obj->mpscriptpos;

   /* Added by Jenny to allow bug messages to show the vnum of the object, and not just supermob's vnum */
   snprintf( buf, sizeof( buf ), "Object #%d", obj->pIndexData->vnum );
   STRSET( supermob->description, buf );

   char_from_room( supermob );
   char_to_room( supermob, room );
}

void release_supermob( void )
{
   supermob_obj = NULL;
   /* Reset it back to normal since we did change it when setting the supermob */
   STRSET( supermob->short_descr, supermob->pIndexData->short_descr );
   STRSET( supermob->description, supermob->pIndexData->description );
   char_from_room( supermob );
   char_to_room( supermob, get_room_index( sysdata.room_poly ) );
}

bool oprog_percent_check( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type )
{
   MPROG_DATA *mprg;
   bool executed = false;

   for( mprg = obj->pIndexData->mudprogs; mprg; mprg = mprg->next )
   {
      if( mprg->type == type && ( number_percent( ) <= atoi( mprg->arglist ) ) )
      {
         executed = true;
         mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
         if( type != GREET_PROG )
            break;
      }
   }
   return executed;
}

/* Triggers follow */
void oprog_greet_trigger( CHAR_DATA *ch )
{
   OBJ_DATA *vobj;

   for( vobj = ch->in_room->first_content; vobj; vobj = vobj->next_content )
   {
      if( HAS_PROG( vobj->pIndexData, GREET_PROG ) )
      {
         set_supermob( vobj );   /* not very efficient to do here */
         oprog_percent_check( supermob, ch, vobj, NULL, GREET_PROG );
         release_supermob( );
      }
   }
}

void oprog_speech_trigger( char *txt, CHAR_DATA *ch )
{
   OBJ_DATA *vobj;

   /* supermob is set and released in oprog_wordlist_check */
   for( vobj = ch->in_room->first_content; vobj; vobj = vobj->next_content )
   {
      if( HAS_PROG( vobj->pIndexData, SPEECH_PROG ) )
         oprog_wordlist_check( txt, supermob, ch, vobj, NULL, SPEECH_PROG, vobj );
   }
}

/* Called at top of obj_update make sure to put an if(!obj) continue after it */
void oprog_random_trigger( OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, RAND_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, NULL, obj, NULL, RAND_PROG );
      release_supermob( );
   }
}

/* in wear_obj, between each successful equip_char the subsequent return */
void oprog_wear_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, WEAR_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, WEAR_PROG );
      release_supermob( );
   }
}

bool oprog_use_trigger( CHAR_DATA *ch, OBJ_DATA *obj, CHAR_DATA *vict, OBJ_DATA *targ )
{
   bool executed = false;

   if( HAS_PROG( obj->pIndexData, USE_PROG ) )
   {
      set_supermob( obj );
      if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND || obj->item_type == ITEM_SCROLL )
      {
         if( vict )
            executed = oprog_percent_check( supermob, ch, obj, vict, USE_PROG );
         else
            executed = oprog_percent_check( supermob, ch, obj, targ, USE_PROG );
      }
      else
         executed = oprog_percent_check( supermob, ch, obj, NULL, USE_PROG );
      release_supermob( );
   }
   return executed;
}

/* call in remove_obj, right after unequip_char do a if(!ch) return right after, and return true (?) if !ch */
void oprog_remove_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, REMOVE_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, REMOVE_PROG );
      release_supermob( );
   }
}

/* call in do_sac, right before extract_obj */
void oprog_sac_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, SAC_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, SAC_PROG );
      release_supermob( );
   }
}

void oprog_open_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, OPEN_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, OPEN_PROG );
      release_supermob( );
   }
}

void oprog_close_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, CLOSE_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, CLOSE_PROG );
      release_supermob( );
   }
}

/* call in do_get, right before check_for_trap do a if(!ch) return right after */
void oprog_get_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, GET_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, GET_PROG );
      release_supermob( );
   }
}

/* called in damage_obj in act_obj.c */
void oprog_damage_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, DAMAGE_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, DAMAGE_PROG );
      release_supermob( );
   }
}

/* called in make_scraps in makeobjs.c */
void oprog_scrap_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, SCRAP_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, SCRAP_PROG );
      release_supermob( );
   }
}

/* called in do_repair in shops.c */
void oprog_repair_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, REPAIR_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, REPAIR_PROG );
      release_supermob( );
   }
}

/* call twice in do_drop, right after the act( AT_ACTION,...) do a if(!ch) return right after */
void oprog_drop_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, DROP_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, DROP_PROG );
      release_supermob( );
   }
}

void oprog_put_trigger( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
   if( HAS_PROG( obj->pIndexData, PUT_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, PUT_PROG );
      release_supermob( );
   }
   if( HAS_PROG( container->pIndexData, PUT_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, container, NULL, PUT_PROG );
      release_supermob( );
   }
}

/* call towards end of do_examine, right before check_for_trap */
void oprog_examine_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, EXA_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, EXA_PROG );
      release_supermob( );
   }
}

/* call in fight.c, group_gain, after (?) the obj_to_room */
void oprog_zap_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, ZAP_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, ZAP_PROG );
      release_supermob( );
   }
}

/* call in levers.c, towards top of do_push_or_pull see note there */
void oprog_pull_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, PULL_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, PULL_PROG );
      release_supermob( );
   }
}

/* call in levers.c, towards top of do_push_or_pull see note there */
void oprog_push_trigger( CHAR_DATA *ch, OBJ_DATA *obj )
{
   if( HAS_PROG( obj->pIndexData, PUSH_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, PUSH_PROG );
      release_supermob( );
   }
}

void oprog_act_trigger( char *buf, OBJ_DATA *mobj, CHAR_DATA *ch, OBJ_DATA *obj, void *vo )
{
   if( HAS_PROG( mobj->pIndexData, ACT_PROG ) )
   {
      MPROG_ACT_LIST *tmp_act, *tmp_mal;

      CREATE( tmp_act, MPROG_ACT_LIST, 1 );
      /* Losing the head of the list -Druid */
      if( mobj->mpactnum > 0 )
      {
         tmp_mal = mobj->mpact;

         while( tmp_mal->next )
            tmp_mal = tmp_mal->next;

         /* Put at the end */
         tmp_mal->next = tmp_act;
      }
      else
         mobj->mpact = tmp_act;

      tmp_act->next = NULL;
      tmp_act->buf = STRALLOC( buf );
      tmp_act->ch = ch;
      tmp_act->obj = obj;
      tmp_act->vo = vo;
      mobj->mpactnum++;
      obj_act_add( mobj );
   }
}

void oprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type, OBJ_DATA *iobj )
{
   char temp1[MSL], temp2[MIL], word[MIL], *list, *start, *dupl, *end;
   MPROG_DATA *mprg;
   unsigned int i;

   for( mprg = iobj->pIndexData->mudprogs; mprg; mprg = mprg->next )
   {
      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); i++ )
            list[i] = LOWER( list[i] );
         strcpy( temp2, arg );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); i++ )
            dupl[i] = LOWER( dupl[i] );
         if( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
         {
            list += 2;
            while( ( start = strstr( dupl, list ) ) )
            {
               if( ( start == dupl || *( start - 1 ) == ' ' )
               && ( *( end = start + strlen( list ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
               {
                  set_supermob( iobj );
                  mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
                  release_supermob( );
                  break;
               }
               else
                  dupl = start + 1;
            }
         }
         else
         {
            list = one_argument( list, word );
            for( ; word[0] != '\0'; list = one_argument( list, word ) )
            {
               while( ( start = strstr( dupl, word ) ) )
               {
                  if( ( start == dupl || *( start - 1 ) == ' ' )
                  && ( *( end = start + strlen( word ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
                  {
                     set_supermob( iobj );
                     mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
                     release_supermob( );
                     break;
                  }
                  else
                     dupl = start + 1;
               }
            }
         }
      }
   }
}

/* room_prog support starts here */
void rset_supermob( ROOM_INDEX_DATA *room )
{
   char buf[200];

   if( !room )
      return;

   STRSET( supermob->short_descr, room->name );
   STRSET( supermob->name, room->name );
   supermob->mpscriptpos = room->mpscriptpos;

   /* Added by Jenny to allow bug messages to show the vnum of the room, and not just supermob's vnum */
   snprintf( buf, sizeof( buf ), "Room #%d", room->vnum );
   STRSET( supermob->description, buf );
   char_from_room( supermob );
   char_to_room( supermob, room );
}

void rprog_percent_check( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type )
{
   MPROG_DATA *mprg;

   if( !mob->in_room )
      return;

   for( mprg = mob->in_room->mudprogs; mprg; mprg = mprg->next )
   {
      if( mprg->type == type && number_percent( ) <= atoi( mprg->arglist ) )
      {
         mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
         if( type != ENTRY_PROG )
            break;
      }
   }
}

/* Triggers follow */
void rprog_act_trigger( char *buf, ROOM_INDEX_DATA *room, CHAR_DATA *ch, OBJ_DATA *obj, void *vo )
{
   if( HAS_PROG( room, ACT_PROG ) )
   {
      MPROG_ACT_LIST *tmp_act, *tmp_mal;

      CREATE( tmp_act, MPROG_ACT_LIST, 1 );
      /* Losing the head of the list -Druid */
      if( room->mpactnum > 0 )
      {
         tmp_mal = room->mpact;

         while( tmp_mal->next )
            tmp_mal = tmp_mal->next;

         /* Put at the end */
         tmp_mal->next = tmp_act;
      }
      else
         room->mpact = tmp_act;

      tmp_act->next = NULL;
      tmp_act->buf = STRALLOC( buf );
      tmp_act->ch = ch;
      tmp_act->obj = obj;
      tmp_act->vo = vo;
      room->mpactnum++;
      room_act_add( room );
   }
}

void rprog_leave_trigger( CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, LEAVE_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, LEAVE_PROG );
      release_supermob( );
   }
}

void rprog_enter_trigger( CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, ENTRY_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, ENTRY_PROG );
      release_supermob( );
   }
}

void rprog_sleep_trigger( CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, SLEEP_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, SLEEP_PROG );
      release_supermob( );
   }
}

void rprog_rest_trigger( CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, REST_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, REST_PROG );
      release_supermob( );
   }
}

void oprog_fight_trigger( CHAR_DATA *ch )
{
   OBJ_DATA *obj;

   if( !ch->fighting )
      return;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc == WEAR_NONE )
         continue;
      if( !HAS_PROG( obj->pIndexData, FIGHT_PROG ) )
         continue;
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, FIGHT_PROG );
      release_supermob( );
   }
}

void rprog_rfight_trigger( CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, FIGHT_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, FIGHT_PROG );
      release_supermob( );
   }
}

void rprog_death_trigger( CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, DEATH_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, DEATH_PROG );
      release_supermob( );
   }
}

void rprog_speech_trigger( char *txt, CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, SPEECH_PROG ) )
      rprog_wordlist_check( txt, supermob, ch, NULL, NULL, SPEECH_PROG, ch->in_room );
}

void rprog_random_trigger( CHAR_DATA *ch )
{
   if( HAS_PROG( ch->in_room, RAND_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, RAND_PROG );
      release_supermob( );
   }
}

void rprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type, ROOM_INDEX_DATA *room )
{
   char temp1[MSL], temp2[MIL], word[MIL];
   MPROG_DATA *mprg;
   char *list, *start, *dupl, *end;
   unsigned int i;

   if( actor && !char_died( actor ) && actor->in_room )
      room = actor->in_room;

   for( mprg = room->mudprogs; mprg; mprg = mprg->next )
   {
      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); i++ )
            list[i] = LOWER( list[i] );
         strcpy( temp2, arg );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); i++ )
            dupl[i] = LOWER( dupl[i] );
         if( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
         {
            list += 2;
            while( ( start = strstr( dupl, list ) ) )
            {
               if( ( start == dupl || *( start - 1 ) == ' ' )
               && ( *( end = start + strlen( list ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
               {
                  rset_supermob( room );
                  mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
                  release_supermob( );
                  break;
               }
               else
                  dupl = start + 1;
            }
         }
         else
         {
            list = one_argument( list, word );
            for( ; word[0] != '\0'; list = one_argument( list, word ) )
            {
               while( ( start = strstr( dupl, word ) ) )
               {
                  if( ( start == dupl || *( start - 1 ) == ' ' )
                  && ( *( end = start + strlen( word ) ) == ' ' || *end == '\r' || *end == '\n' || *end == '\0' ) )
                  {
                     rset_supermob( room );
                     mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
                     release_supermob( );
                     break;
                  }
                  else
                     dupl = start + 1;
               }
            }
         }
      }
   }
}

void rprog_time_check( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj, void *vo, int type )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) vo;
   MPROG_DATA *mprg;
   bool trigger_time;

   for( mprg = room->mudprogs; mprg; mprg = mprg->next )
   {
      trigger_time = ( time_info.hour == atoi( mprg->arglist ) );

      if( !trigger_time )
      {
         if( mprg->triggered )
            mprg->triggered = false;
         continue;
      }

      if( mprg->type == type && ( ( !mprg->triggered ) || ( mprg->type == HOUR_PROG ) ) )
      {
         mprg->triggered = true;
         mprog_driver( mprg->comlist, mob, actor, obj, vo, false );
      }
   }
}

void rprog_time_trigger( ROOM_INDEX_DATA *room )
{
   if( HAS_PROG( room, TIME_PROG ) )
   {
      rset_supermob( room );
      rprog_time_check( supermob, NULL, NULL, room, TIME_PROG );
      release_supermob( );
   }
}

void rprog_hour_trigger( ROOM_INDEX_DATA *room )
{
   if( HAS_PROG( room, HOUR_PROG ) )
   {
      rset_supermob( room );
      rprog_time_check( supermob, NULL, NULL, room, HOUR_PROG );
      release_supermob( );
   }
}

/* Written by Jenny, Nov 29/95 */
void progbug( const char *str, CHAR_DATA *mob )
{
   int vnum = mob->pIndexData ? mob->pIndexData->vnum : 0;

   if( vnum == MOB_VNUM_SUPERMOB )
      bug( "%s: %s.", !mob->description ? "(Unknown)" : strip_cr( mob->description ), str );
   else
      bug( "Mob #%d: %s.", vnum, str );
}

void progbug_printf( CHAR_DATA *mob, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, sizeof( buf ), fmt, args );
   va_end( args );

   progbug( buf, mob );
}

/*
 * Room act prog updates.  Use a separate list cuz we dont really wanna go
 * thru 5-10000 rooms every pulse.. can we say lag? -- Alty
 */
void room_act_add( ROOM_INDEX_DATA *room )
{
   struct act_prog_data *runner, *tmp_ral;

   for( runner = room_act_list; runner; runner = runner->next )
      if( runner->vo == room )
         return;
   CREATE( runner, struct act_prog_data, 1 );
   runner->vo = room;
   runner->next = NULL;
   /*
    * The head of the list is being changed in
    * room_act_update, So append to the end of the list 
    * instead. -Druid
    */
   if( room_act_list )
   {
      tmp_ral = room_act_list;

      while( tmp_ral->next )
         tmp_ral = tmp_ral->next;

      /* put at the end */
      tmp_ral->next = runner;
   }
   else
      room_act_list = runner;
}

void room_act_update( void )
{
   struct act_prog_data *runner;
   MPROG_ACT_LIST *mpact;

   while( ( runner = room_act_list ) )
   {
      ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) runner->vo;

      while( ( mpact = room->mpact ) )
      {
         if( mpact->ch->in_room == room )
            rprog_wordlist_check( mpact->buf, supermob, mpact->ch, mpact->obj, mpact->vo, ACT_PROG, room );
         room->mpact = mpact->next;
         STRFREE( mpact->buf );
         DISPOSE( mpact );
      }
      room->mpact = NULL;
      room->mpactnum = 0;
      room_act_list = runner->next;
      DISPOSE( runner );
   }
}

void obj_act_add( OBJ_DATA *obj )
{
   struct act_prog_data *runner, *tmp_oal;

   for( runner = obj_act_list; runner; runner = runner->next )
      if( runner->vo == obj )
         return;
   CREATE( runner, struct act_prog_data, 1 );
   runner->vo = obj;
   runner->next = NULL;
   /* The head of the list is being changed in obj_act_update, So append to the end of the list instead. -Druid */
   if( obj_act_list )
   {
      tmp_oal = obj_act_list;

      while( tmp_oal->next )
         tmp_oal = tmp_oal->next;

      /* put at the end */
      tmp_oal->next = runner;
   }
   else
      obj_act_list = runner;
}

void obj_act_update( void )
{
   struct act_prog_data *runner;
   MPROG_ACT_LIST *mpact;

   while( ( runner = obj_act_list ) )
   {
      OBJ_DATA *obj = ( OBJ_DATA * ) runner->vo;

      while( ( mpact = obj->mpact ) )
      {
         oprog_wordlist_check( mpact->buf, supermob, mpact->ch, mpact->obj, mpact->vo, ACT_PROG, obj );
         obj->mpact = mpact->next;
         STRFREE( mpact->buf );
         DISPOSE( mpact );
      }
      obj->mpact = NULL;
      obj->mpactnum = 0;
      obj_act_list = runner->next;
      DISPOSE( runner );
   }
}
