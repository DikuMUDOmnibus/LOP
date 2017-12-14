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
#include "h/mud.h"

/* 0 = White, 1 = Red, 2 = Blue, 3 = Green, 4 = Orange, 5 = Yellow */
char *show_cube_color( short rubnum )
{
   if( rubnum == 0 ) return (char *)"&WW&D";
   if( rubnum == 1 ) return (char *)"&RR&D";
   if( rubnum == 2 ) return (char *)"&BB&D";
   if( rubnum == 3 ) return (char *)"&GG&D";
   if( rubnum == 4 ) return (char *)"&OO&D";
   if( rubnum == 5 ) return (char *)"&YY&D";

   return (char *)"&pU&D";
}

void show_cube( CHAR_DATA *ch )
{
   if( !ch || is_npc( ch ) )
      return;

   set_char_color( AT_WHITE, ch );
   send_to_char( "          Back\r\n", ch );
   send_to_char( "         - - -\r\n", ch );
   ch_printf( ch, "   Down |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[6] ), show_cube_color( ch->pcdata->rubik[7] ), show_cube_color( ch->pcdata->rubik[8] ) );
   ch_printf( ch, "        |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[3] ), show_cube_color( ch->pcdata->rubik[4] ), show_cube_color( ch->pcdata->rubik[5] ) );
   ch_printf( ch, "     Up |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[0] ), show_cube_color( ch->pcdata->rubik[1] ), show_cube_color( ch->pcdata->rubik[2] ) );
   send_to_char( "         - - -\r\n", ch );

   send_to_char( "            Up\r\n", ch );
   send_to_char( "         - - -\r\n", ch );
   ch_printf( ch, "   Back |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[9] ), show_cube_color( ch->pcdata->rubik[10] ), show_cube_color( ch->pcdata->rubik[11] ) );
   ch_printf( ch, "        |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[12] ), show_cube_color( ch->pcdata->rubik[13] ), show_cube_color( ch->pcdata->rubik[14] ) );
   ch_printf( ch, "  Front |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[15] ), show_cube_color( ch->pcdata->rubik[16] ), show_cube_color( ch->pcdata->rubik[17] ) );
   send_to_char( "         - - -\r\n", ch );

   send_to_char( "  Left   Front   Right\r\n", ch );
   send_to_char( " - - -   - - -   - - -\r\n", ch );
   ch_printf( ch, "|%s|%s|%s| |%s|%s|%s| |%s|%s|%s|\r\n",
      show_cube_color( ch->pcdata->rubik[18] ), show_cube_color( ch->pcdata->rubik[19] ), show_cube_color( ch->pcdata->rubik[20] ),
      show_cube_color( ch->pcdata->rubik[27] ), show_cube_color( ch->pcdata->rubik[28] ), show_cube_color( ch->pcdata->rubik[29] ),
      show_cube_color( ch->pcdata->rubik[36] ), show_cube_color( ch->pcdata->rubik[37] ), show_cube_color( ch->pcdata->rubik[38] ) );
   ch_printf( ch, "|%s|%s|%s| |%s|%s|%s| |%s|%s|%s|\r\n",
      show_cube_color( ch->pcdata->rubik[21] ), show_cube_color( ch->pcdata->rubik[22] ), show_cube_color( ch->pcdata->rubik[23] ),
      show_cube_color( ch->pcdata->rubik[30] ), show_cube_color( ch->pcdata->rubik[31] ), show_cube_color( ch->pcdata->rubik[32] ),
      show_cube_color( ch->pcdata->rubik[39] ), show_cube_color( ch->pcdata->rubik[40] ), show_cube_color( ch->pcdata->rubik[41] ) );
   ch_printf( ch, "|%s|%s|%s| |%s|%s|%s| |%s|%s|%s|\r\n",
      show_cube_color( ch->pcdata->rubik[24] ), show_cube_color( ch->pcdata->rubik[25] ), show_cube_color( ch->pcdata->rubik[26] ),
      show_cube_color( ch->pcdata->rubik[33] ), show_cube_color( ch->pcdata->rubik[34] ), show_cube_color( ch->pcdata->rubik[35] ),
      show_cube_color( ch->pcdata->rubik[42] ), show_cube_color( ch->pcdata->rubik[43] ), show_cube_color( ch->pcdata->rubik[44] ) );
   send_to_char( " - - -   - - -   - - -\r\n", ch );

   send_to_char( "          Down\r\n", ch );
   send_to_char( "         - - -\r\n", ch );
   ch_printf( ch, "  Front |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[51] ), show_cube_color( ch->pcdata->rubik[52] ), show_cube_color( ch->pcdata->rubik[53] ) );
   ch_printf( ch, "        |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[48] ), show_cube_color( ch->pcdata->rubik[49] ), show_cube_color( ch->pcdata->rubik[50] ) );
   ch_printf( ch, "   Back |%s|%s|%s|\r\n", show_cube_color( ch->pcdata->rubik[45] ), show_cube_color( ch->pcdata->rubik[46] ), show_cube_color( ch->pcdata->rubik[47] ) );
   send_to_char( "         - - -\r\n", ch );
}

void move_cube( CHAR_DATA *ch, short mway )
{
   short tmp_rub[54];
   short rcount;

   /* Based on what way its moved have to change stuff around first copy the current setup */
   for( rcount = 0; rcount < 54; rcount++ )
      tmp_rub[rcount] = ch->pcdata->rubik[rcount];

   /* Ok, copy over the changes */
   /*  0 = F */
   if( mway == 0 )
   {
      tmp_rub[15] = ch->pcdata->rubik[26];   tmp_rub[16] = ch->pcdata->rubik[23];
      tmp_rub[17] = ch->pcdata->rubik[20];   tmp_rub[20] = ch->pcdata->rubik[51];
      tmp_rub[23] = ch->pcdata->rubik[52];   tmp_rub[26] = ch->pcdata->rubik[53];
      tmp_rub[27] = ch->pcdata->rubik[33];   tmp_rub[28] = ch->pcdata->rubik[30];
      tmp_rub[29] = ch->pcdata->rubik[27];   tmp_rub[30] = ch->pcdata->rubik[34];
      tmp_rub[32] = ch->pcdata->rubik[28];   tmp_rub[33] = ch->pcdata->rubik[35];
      tmp_rub[34] = ch->pcdata->rubik[32];   tmp_rub[35] = ch->pcdata->rubik[29];
      tmp_rub[36] = ch->pcdata->rubik[15];   tmp_rub[39] = ch->pcdata->rubik[16];
      tmp_rub[42] = ch->pcdata->rubik[17];   tmp_rub[51] = ch->pcdata->rubik[42];
      tmp_rub[52] = ch->pcdata->rubik[39];   tmp_rub[53] = ch->pcdata->rubik[36];
   }
   /*  1 = Fi */
   else if( mway == 1 )
   {
      tmp_rub[15] = ch->pcdata->rubik[36];   tmp_rub[16] = ch->pcdata->rubik[39];
      tmp_rub[17] = ch->pcdata->rubik[42];   tmp_rub[20] = ch->pcdata->rubik[17];
      tmp_rub[23] = ch->pcdata->rubik[16];   tmp_rub[26] = ch->pcdata->rubik[15];
      tmp_rub[27] = ch->pcdata->rubik[29];   tmp_rub[28] = ch->pcdata->rubik[32];
      tmp_rub[29] = ch->pcdata->rubik[35];   tmp_rub[30] = ch->pcdata->rubik[28];
      tmp_rub[32] = ch->pcdata->rubik[34];   tmp_rub[33] = ch->pcdata->rubik[27];
      tmp_rub[34] = ch->pcdata->rubik[30];   tmp_rub[35] = ch->pcdata->rubik[33];
      tmp_rub[36] = ch->pcdata->rubik[53];   tmp_rub[39] = ch->pcdata->rubik[52];
      tmp_rub[42] = ch->pcdata->rubik[51];   tmp_rub[51] = ch->pcdata->rubik[20];
      tmp_rub[52] = ch->pcdata->rubik[23];   tmp_rub[53] = ch->pcdata->rubik[26];
   }
   /*  2 = R */
   else if( mway == 2 )
   {
      tmp_rub[2] = ch->pcdata->rubik[17];    tmp_rub[5] = ch->pcdata->rubik[14];
      tmp_rub[8] = ch->pcdata->rubik[11];    tmp_rub[11] = ch->pcdata->rubik[29];
      tmp_rub[14] = ch->pcdata->rubik[32];   tmp_rub[17] = ch->pcdata->rubik[35];
      tmp_rub[29] = ch->pcdata->rubik[53];   tmp_rub[32] = ch->pcdata->rubik[50];
      tmp_rub[35] = ch->pcdata->rubik[47];   tmp_rub[36] = ch->pcdata->rubik[42];
      tmp_rub[37] = ch->pcdata->rubik[39];   tmp_rub[38] = ch->pcdata->rubik[36];
      tmp_rub[39] = ch->pcdata->rubik[43];   tmp_rub[41] = ch->pcdata->rubik[37];
      tmp_rub[42] = ch->pcdata->rubik[44];   tmp_rub[43] = ch->pcdata->rubik[41];
      tmp_rub[44] = ch->pcdata->rubik[38];   tmp_rub[47] = ch->pcdata->rubik[2];
      tmp_rub[50] = ch->pcdata->rubik[5];    tmp_rub[53] = ch->pcdata->rubik[8];
   }
   /*  3 = Ri */
   else if( mway == 3 )
   {
      tmp_rub[2] = ch->pcdata->rubik[47];    tmp_rub[5] = ch->pcdata->rubik[50];
      tmp_rub[8] = ch->pcdata->rubik[53];    tmp_rub[11] = ch->pcdata->rubik[8];
      tmp_rub[14] = ch->pcdata->rubik[5];    tmp_rub[17] = ch->pcdata->rubik[2];
      tmp_rub[29] = ch->pcdata->rubik[11];   tmp_rub[32] = ch->pcdata->rubik[14];
      tmp_rub[35] = ch->pcdata->rubik[17];   tmp_rub[36] = ch->pcdata->rubik[38];
      tmp_rub[37] = ch->pcdata->rubik[41];   tmp_rub[38] = ch->pcdata->rubik[44];
      tmp_rub[39] = ch->pcdata->rubik[37];   tmp_rub[41] = ch->pcdata->rubik[43];
      tmp_rub[42] = ch->pcdata->rubik[36];   tmp_rub[43] = ch->pcdata->rubik[39];
      tmp_rub[44] = ch->pcdata->rubik[42];   tmp_rub[47] = ch->pcdata->rubik[35];
      tmp_rub[50] = ch->pcdata->rubik[32];   tmp_rub[53] = ch->pcdata->rubik[29];
   }
   /*  4 = L */
   else if( mway == 4 )
   {
      tmp_rub[0] = ch->pcdata->rubik[45];    tmp_rub[3] = ch->pcdata->rubik[48];
      tmp_rub[6] = ch->pcdata->rubik[51];    tmp_rub[9] = ch->pcdata->rubik[6];
      tmp_rub[12] = ch->pcdata->rubik[3];    tmp_rub[15] = ch->pcdata->rubik[0];
      tmp_rub[18] = ch->pcdata->rubik[24];   tmp_rub[19] = ch->pcdata->rubik[21];
      tmp_rub[20] = ch->pcdata->rubik[18];   tmp_rub[21] = ch->pcdata->rubik[25];
      tmp_rub[23] = ch->pcdata->rubik[19];   tmp_rub[24] = ch->pcdata->rubik[26];
      tmp_rub[25] = ch->pcdata->rubik[23];   tmp_rub[26] = ch->pcdata->rubik[20];
      tmp_rub[27] = ch->pcdata->rubik[9];    tmp_rub[30] = ch->pcdata->rubik[12];
      tmp_rub[33] = ch->pcdata->rubik[15];   tmp_rub[45] = ch->pcdata->rubik[33];
      tmp_rub[48] = ch->pcdata->rubik[30];   tmp_rub[51] = ch->pcdata->rubik[27];
   }
   /*  5 = Li */
   else if( mway == 5 )
   {
      tmp_rub[0] = ch->pcdata->rubik[15];    tmp_rub[3] = ch->pcdata->rubik[12];
      tmp_rub[6] = ch->pcdata->rubik[9];     tmp_rub[9] = ch->pcdata->rubik[27];
      tmp_rub[12] = ch->pcdata->rubik[30];   tmp_rub[15] = ch->pcdata->rubik[33];
      tmp_rub[18] = ch->pcdata->rubik[20];   tmp_rub[19] = ch->pcdata->rubik[23];
      tmp_rub[20] = ch->pcdata->rubik[26];   tmp_rub[21] = ch->pcdata->rubik[19];
      tmp_rub[23] = ch->pcdata->rubik[25];   tmp_rub[24] = ch->pcdata->rubik[18];
      tmp_rub[25] = ch->pcdata->rubik[21];   tmp_rub[26] = ch->pcdata->rubik[24];
      tmp_rub[27] = ch->pcdata->rubik[51];   tmp_rub[30] = ch->pcdata->rubik[48];
      tmp_rub[33] = ch->pcdata->rubik[45];   tmp_rub[45] = ch->pcdata->rubik[0];
      tmp_rub[48] = ch->pcdata->rubik[3];    tmp_rub[51] = ch->pcdata->rubik[6];
   }
   /*  6 = D */
   else if( mway == 6 )
   {
      tmp_rub[6] = ch->pcdata->rubik[44];    tmp_rub[7] = ch->pcdata->rubik[43];
      tmp_rub[8] = ch->pcdata->rubik[42];    tmp_rub[24] = ch->pcdata->rubik[8];
      tmp_rub[25] = ch->pcdata->rubik[7];    tmp_rub[26] = ch->pcdata->rubik[6];
      tmp_rub[33] = ch->pcdata->rubik[24];   tmp_rub[34] = ch->pcdata->rubik[25];
      tmp_rub[35] = ch->pcdata->rubik[26];   tmp_rub[42] = ch->pcdata->rubik[33];
      tmp_rub[43] = ch->pcdata->rubik[34];   tmp_rub[44] = ch->pcdata->rubik[35];
      tmp_rub[45] = ch->pcdata->rubik[47];   tmp_rub[46] = ch->pcdata->rubik[50];
      tmp_rub[47] = ch->pcdata->rubik[53];   tmp_rub[48] = ch->pcdata->rubik[46];
      tmp_rub[50] = ch->pcdata->rubik[52];   tmp_rub[51] = ch->pcdata->rubik[45];
      tmp_rub[52] = ch->pcdata->rubik[48];   tmp_rub[53] = ch->pcdata->rubik[51];
   }
   /*  7 = Di */
   else if( mway == 7 )
   {
      tmp_rub[6] = ch->pcdata->rubik[26];    tmp_rub[7] = ch->pcdata->rubik[25];
      tmp_rub[8] = ch->pcdata->rubik[24];    tmp_rub[24] = ch->pcdata->rubik[33];
      tmp_rub[25] = ch->pcdata->rubik[34];   tmp_rub[26] = ch->pcdata->rubik[35];
      tmp_rub[33] = ch->pcdata->rubik[42];   tmp_rub[34] = ch->pcdata->rubik[43];
      tmp_rub[35] = ch->pcdata->rubik[44];   tmp_rub[42] = ch->pcdata->rubik[8];
      tmp_rub[43] = ch->pcdata->rubik[7];    tmp_rub[44] = ch->pcdata->rubik[6];
      tmp_rub[45] = ch->pcdata->rubik[51];   tmp_rub[46] = ch->pcdata->rubik[48];
      tmp_rub[47] = ch->pcdata->rubik[45];   tmp_rub[48] = ch->pcdata->rubik[52];
      tmp_rub[50] = ch->pcdata->rubik[46];   tmp_rub[51] = ch->pcdata->rubik[53];
      tmp_rub[52] = ch->pcdata->rubik[50];   tmp_rub[53] = ch->pcdata->rubik[47];
   }
   /*  8 = B */
   else if( mway == 8 )
   {
      tmp_rub[0] = ch->pcdata->rubik[2];     tmp_rub[1] = ch->pcdata->rubik[5];
      tmp_rub[2] = ch->pcdata->rubik[8];     tmp_rub[3] = ch->pcdata->rubik[1];
      tmp_rub[5] = ch->pcdata->rubik[7];     tmp_rub[6] = ch->pcdata->rubik[0];
      tmp_rub[7] = ch->pcdata->rubik[3];     tmp_rub[8] = ch->pcdata->rubik[6];
      tmp_rub[9] = ch->pcdata->rubik[38];    tmp_rub[10] = ch->pcdata->rubik[41];
      tmp_rub[11] = ch->pcdata->rubik[44];   tmp_rub[18] = ch->pcdata->rubik[11];
      tmp_rub[21] = ch->pcdata->rubik[10];   tmp_rub[24] = ch->pcdata->rubik[9];
      tmp_rub[38] = ch->pcdata->rubik[47];   tmp_rub[41] = ch->pcdata->rubik[46];
      tmp_rub[44] = ch->pcdata->rubik[45];   tmp_rub[45] = ch->pcdata->rubik[18];
      tmp_rub[46] = ch->pcdata->rubik[21];   tmp_rub[47] = ch->pcdata->rubik[24];
   }
   /*  9 = Bi */
   else if( mway == 9 )
   {
      tmp_rub[0] = ch->pcdata->rubik[6];     tmp_rub[1] = ch->pcdata->rubik[3];
      tmp_rub[2] = ch->pcdata->rubik[0];     tmp_rub[3] = ch->pcdata->rubik[7];
      tmp_rub[5] = ch->pcdata->rubik[1];     tmp_rub[6] = ch->pcdata->rubik[8];
      tmp_rub[7] = ch->pcdata->rubik[5];     tmp_rub[8] = ch->pcdata->rubik[2];
      tmp_rub[9] = ch->pcdata->rubik[24];    tmp_rub[10] = ch->pcdata->rubik[21];
      tmp_rub[11] = ch->pcdata->rubik[18];   tmp_rub[18] = ch->pcdata->rubik[45];
      tmp_rub[21] = ch->pcdata->rubik[46];   tmp_rub[24] = ch->pcdata->rubik[47];
      tmp_rub[38] = ch->pcdata->rubik[9];    tmp_rub[41] = ch->pcdata->rubik[10];
      tmp_rub[44] = ch->pcdata->rubik[11];   tmp_rub[45] = ch->pcdata->rubik[44];
      tmp_rub[46] = ch->pcdata->rubik[41];   tmp_rub[47] = ch->pcdata->rubik[38];
   }
   /* 10 = U */
   else if( mway == 10 )
   {
      tmp_rub[0] = ch->pcdata->rubik[20];    tmp_rub[1] = ch->pcdata->rubik[19];
      tmp_rub[2] = ch->pcdata->rubik[18];    tmp_rub[9] = ch->pcdata->rubik[15];
      tmp_rub[10] = ch->pcdata->rubik[12];   tmp_rub[11] = ch->pcdata->rubik[9];
      tmp_rub[12] = ch->pcdata->rubik[16];   tmp_rub[14] = ch->pcdata->rubik[10];
      tmp_rub[15] = ch->pcdata->rubik[17];   tmp_rub[16] = ch->pcdata->rubik[14];
      tmp_rub[17] = ch->pcdata->rubik[11];   tmp_rub[18] = ch->pcdata->rubik[27];
      tmp_rub[19] = ch->pcdata->rubik[28];   tmp_rub[20] = ch->pcdata->rubik[29];
      tmp_rub[27] = ch->pcdata->rubik[36];   tmp_rub[28] = ch->pcdata->rubik[37];
      tmp_rub[29] = ch->pcdata->rubik[38];   tmp_rub[36] = ch->pcdata->rubik[2];
      tmp_rub[37] = ch->pcdata->rubik[1];    tmp_rub[38] = ch->pcdata->rubik[0];
   }
   /* 11 = Ui */
   else if( mway == 11 )
   {
      tmp_rub[0] = ch->pcdata->rubik[38];    tmp_rub[1] = ch->pcdata->rubik[37];
      tmp_rub[2] = ch->pcdata->rubik[36];    tmp_rub[9] = ch->pcdata->rubik[11];
      tmp_rub[10] = ch->pcdata->rubik[14];   tmp_rub[11] = ch->pcdata->rubik[17];
      tmp_rub[12] = ch->pcdata->rubik[10];   tmp_rub[14] = ch->pcdata->rubik[16];
      tmp_rub[15] = ch->pcdata->rubik[9];    tmp_rub[16] = ch->pcdata->rubik[12];
      tmp_rub[17] = ch->pcdata->rubik[15];   tmp_rub[18] = ch->pcdata->rubik[2];
      tmp_rub[19] = ch->pcdata->rubik[1];    tmp_rub[20] = ch->pcdata->rubik[0];
      tmp_rub[27] = ch->pcdata->rubik[18];   tmp_rub[28] = ch->pcdata->rubik[19];
      tmp_rub[29] = ch->pcdata->rubik[20];   tmp_rub[36] = ch->pcdata->rubik[27];
      tmp_rub[37] = ch->pcdata->rubik[28];   tmp_rub[38] = ch->pcdata->rubik[29];
   }

   /* Copy the changes */
   for( rcount = 0; rcount < 54; rcount++ )
      ch->pcdata->rubik[rcount] = tmp_rub[rcount];
}

void turn_cube( CHAR_DATA *ch, short mway )
{
   short tmp_rub[54];
   short rcount;

   /*  0 = Turn it so Left is now Front */
   if( mway == 0 )
   {
      move_cube( ch, 11 ); /* Moves the Up part */
      move_cube( ch, 6 ); /* Moves the down part */

      /* Based on what way its moved have to change stuff around first copy the current setup */
      for( rcount = 0; rcount < 54; rcount++ )
         tmp_rub[rcount] = ch->pcdata->rubik[rcount];

      /* Now we just move the center part the hard way */
      tmp_rub[3] = ch->pcdata->rubik[41];    tmp_rub[4] = ch->pcdata->rubik[40];
      tmp_rub[5] = ch->pcdata->rubik[39];    tmp_rub[21] = ch->pcdata->rubik[5];
      tmp_rub[22] = ch->pcdata->rubik[4];    tmp_rub[23] = ch->pcdata->rubik[3];
      tmp_rub[30] = ch->pcdata->rubik[21];   tmp_rub[31] = ch->pcdata->rubik[22];
      tmp_rub[32] = ch->pcdata->rubik[23];   tmp_rub[39] = ch->pcdata->rubik[30];
      tmp_rub[40] = ch->pcdata->rubik[31];   tmp_rub[41] = ch->pcdata->rubik[32];
   }
   /*  1 = Turn it so Right is now Front */
   else if( mway == 1 )
   {
      move_cube( ch, 10 ); /* Moves the Up part */
      move_cube( ch, 7 ); /* Moves the Down part backwards */

      /* Based on what way its moved have to change stuff around first copy the current setup */
      for( rcount = 0; rcount < 54; rcount++ )
         tmp_rub[rcount] = ch->pcdata->rubik[rcount];

      /* Now we just move the center part the hard way */
      tmp_rub[3] = ch->pcdata->rubik[23];    tmp_rub[4] = ch->pcdata->rubik[22];
      tmp_rub[5] = ch->pcdata->rubik[21];    tmp_rub[21] = ch->pcdata->rubik[30];
      tmp_rub[22] = ch->pcdata->rubik[31];   tmp_rub[23] = ch->pcdata->rubik[32];
      tmp_rub[30] = ch->pcdata->rubik[39];   tmp_rub[31] = ch->pcdata->rubik[40];
      tmp_rub[32] = ch->pcdata->rubik[41];   tmp_rub[39] = ch->pcdata->rubik[5];
      tmp_rub[40] = ch->pcdata->rubik[4];    tmp_rub[41] = ch->pcdata->rubik[3];
   }
   /*  2 = Turn it so Up is now Front */
   else if( mway == 2 )
   {
      move_cube( ch, 3 ); /* Moves the Right part backwards */
      move_cube( ch, 4 ); /* Moves the Left part */

      /* Based on what way its moved have to change stuff around first copy the current setup */
      for( rcount = 0; rcount < 54; rcount++ )
         tmp_rub[rcount] = ch->pcdata->rubik[rcount];

      /* Now we just move the center part the hard way */
      tmp_rub[1] = ch->pcdata->rubik[46];    tmp_rub[4] = ch->pcdata->rubik[49];
      tmp_rub[7] = ch->pcdata->rubik[52];    tmp_rub[10] = ch->pcdata->rubik[7];
      tmp_rub[13] = ch->pcdata->rubik[4];    tmp_rub[16] = ch->pcdata->rubik[1];
      tmp_rub[28] = ch->pcdata->rubik[10];   tmp_rub[31] = ch->pcdata->rubik[13];
      tmp_rub[34] = ch->pcdata->rubik[16];   tmp_rub[46] = ch->pcdata->rubik[34];
      tmp_rub[49] = ch->pcdata->rubik[31];   tmp_rub[52] = ch->pcdata->rubik[28];
   }
   /*  3 = Turn it so Back is now Front */
   else if( mway == 3 )
   {
      move_cube( ch, 3 ); /* Moves the Right part backwards */
      move_cube( ch, 3 ); /* Moves the Right part backwards, again */
      move_cube( ch, 4 ); /* Moves the Left part */
      move_cube( ch, 4 ); /* Moves the Left part, again */

      /* Based on what way its moved have to change stuff around first copy the current setup */
      for( rcount = 0; rcount < 54; rcount++ )
         tmp_rub[rcount] = ch->pcdata->rubik[rcount];

      /* Now we just move the center part the hard way */
      tmp_rub[1] = ch->pcdata->rubik[34];    tmp_rub[4] = ch->pcdata->rubik[31];
      tmp_rub[7] = ch->pcdata->rubik[28];    tmp_rub[10] = ch->pcdata->rubik[52];
      tmp_rub[13] = ch->pcdata->rubik[49];   tmp_rub[16] = ch->pcdata->rubik[46];
      tmp_rub[28] = ch->pcdata->rubik[7];    tmp_rub[31] = ch->pcdata->rubik[4];
      tmp_rub[34] = ch->pcdata->rubik[1];    tmp_rub[46] = ch->pcdata->rubik[16];
      tmp_rub[49] = ch->pcdata->rubik[13];   tmp_rub[52] = ch->pcdata->rubik[10];
   }
   /*  4 = Turn it so Down is now Front */
   else if( mway == 4 )
   {
      move_cube( ch, 2 ); /* Moves the Right part */
      move_cube( ch, 5 ); /* Moves the Left part backwards */

      /* Based on what way its moved have to change stuff around first copy the current setup */
      for( rcount = 0; rcount < 54; rcount++ )
         tmp_rub[rcount] = ch->pcdata->rubik[rcount];

      /* Now we just move the center part the hard way */
      tmp_rub[1] = ch->pcdata->rubik[16];    tmp_rub[4] = ch->pcdata->rubik[13];
      tmp_rub[7] = ch->pcdata->rubik[10];    tmp_rub[10] = ch->pcdata->rubik[28];
      tmp_rub[13] = ch->pcdata->rubik[31];   tmp_rub[16] = ch->pcdata->rubik[34];
      tmp_rub[28] = ch->pcdata->rubik[52];   tmp_rub[31] = ch->pcdata->rubik[49];
      tmp_rub[34] = ch->pcdata->rubik[46];   tmp_rub[46] = ch->pcdata->rubik[1];
      tmp_rub[49] = ch->pcdata->rubik[4];    tmp_rub[52] = ch->pcdata->rubik[7];
   }

   /* Copy the changes */
   for( rcount = 0; rcount < 54; rcount++ )
      ch->pcdata->rubik[rcount] = tmp_rub[rcount];
}

/* More or less just make it do random moves to mix it all up */
void mix_cube( CHAR_DATA *ch )
{
   short mmix, rchance;

   if( !ch || is_npc( ch ) )
      return;

   /* Limit how much mixing it does */
   for( mmix = 0; mmix < 25; mmix++ )
   {
      rchance = number_range( 0, 11 );
      move_cube( ch, rchance );
   }
}

/* Is the cube completed? */
bool is_completed( CHAR_DATA *ch )
{
   short rcount, rcheck = 0;

   if( !ch || is_npc( ch ) )
      return false;

   /* Check each side of the cube and see if it is complete */
   for( rcount = 0; rcount < 54; rcount++ )
   {
      if( rcount == 0 || rcount == 9 || rcount == 18
      || rcount == 27 || rcount == 36 || rcount == 45 )
         rcheck = ch->pcdata->rubik[rcount];
      if( ch->pcdata->rubik[rcount] != rcheck )
         return false;
   }

   /* Well if we made it through them all and they all match it's completed */
   return true;
}

bool is_playing_rubik( CHAR_DATA *ch )
{
   short rcount;

   if( !ch || is_npc( ch ) )
      return false;

   for( rcount = 0; rcount < 54; rcount++ )
      if( ch->pcdata->rubik[rcount] != 0 )
         return true;

   return false;
}

void check_rubik( CHAR_DATA *ch )
{
   short rcount;
   short color[6];

   if( !ch || is_npc( ch ) )
      return;

   for( rcount = 0; rcount < 6; rcount++ )
      color[rcount] = 0;

   for( rcount = 0; rcount < 54; rcount++ )
      color[ch->pcdata->rubik[rcount]]++;

   for( rcount = 0; rcount < 6; rcount++ )
   {
      if( color[rcount] != 9 )
         ch_printf( ch, "There are %d %s!\r\n", color[rcount], show_cube_color( rcount ) );
   }
}

/* 0 = White, 1 = Red, 2 = Blue, 3 = Green, 4 = Orange, 5 = Yellow */
void start_rubiks( CHAR_DATA *ch )
{
   short rcount, cucolor;

   if( !ch || is_npc( ch ) )
      return;

   /* Setup all the defaults */
   for( rcount = 0; rcount < 54; rcount++ )
   {
      if( rcount < 9 )       cucolor = 4;
      else if( rcount < 18 ) cucolor = 0;
      else if( rcount < 27 ) cucolor = 3;
      else if( rcount < 36 ) cucolor = 1;
      else if( rcount < 45 ) cucolor = 2;
      else                   cucolor = 5;

      ch->pcdata->rubik[rcount] = cucolor;
   }

   /* Mix up the cube */
   mix_cube( ch );

   check_rubik( ch );
}

/* Handle the command so the player can play the rubik's cube */
CMDF( do_rubiks )
{
   char arg[MIL];

   if( !ch || is_npc( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      if( is_playing_rubik( ch ) )
         show_cube( ch );
      else
         send_to_char( "You aren't currently playing the rubik's cube. Type 'rubiks start' to start playing.\r\n", ch );
      return;
   }

   if( !str_cmp( argument, "start" ) )
   {
      start_rubiks( ch );
      show_cube( ch );
      return;
   }

   if( !is_playing_rubik( ch ) )
   {
      send_to_char( "You aren't currently playing the rubik's cube. Type 'rubiks start' to start playing.\r\n", ch );
      return;
   }

   if( !str_cmp( argument, "quit" ) )
   {
      short rcount;

      ch->pcdata->rquits++;
      for( rcount = 0; rcount < 54; rcount++ )
         ch->pcdata->rubik[rcount] = 0;
      send_to_char( "You have quit playing the rubik's cube. Type 'rubiks start' if you want to play again.\r\n", ch );
      return;
   }

   if( !str_cmp( argument, "mix" ) )
   {
      mix_cube( ch );
      show_cube( ch );
      return;
   }

   one_argument( argument, arg ); /* Lets just see what one arg is */
   if( !str_cmp( arg, "turn" ) )
   {
      argument = one_argument( argument, arg ); /* Ok so it was turn lets take it off */

      if( !str_cmp( argument, "left" ) ) /* If we turn the cube left the right side becomes the front */
         turn_cube( ch, 1 );
      else if( !str_cmp( argument, "right" ) ) /* If we turn the cube right the left side becomes the front */
         turn_cube( ch, 0 );
      else if( !str_cmp( argument, "up" ) ) /* If we turn the cube up the down side becomes the front */
         turn_cube( ch, 4 );
      else if( !str_cmp( argument, "down" ) ) /* If we turn the cube down the up side becomes the front */
         turn_cube( ch, 2 );
      else if( !str_cmp( argument, "back" ) ) /* This is just a way to make the back side come to the front side */
         turn_cube( ch, 3 );
      
      show_cube( ch );
      return;
   }

   /* Will allow for the full way you want it to do to go at once */
   while( argument && argument[0] != '\0' )
   {
      argument = one_argument( argument, arg );
      if( !str_cmp( arg, "f" ) )
         move_cube( ch, 0 );
      else if( !str_cmp( arg, "fi" ) )
         move_cube( ch, 1 );
      else if( !str_cmp( arg, "r" ) )
         move_cube( ch, 2 );
      else if( !str_cmp( arg, "ri" ) )
         move_cube( ch, 3 );
      else if( !str_cmp( arg, "l" ) )
         move_cube( ch, 4 );
      else if( !str_cmp( arg, "li" ) )
         move_cube( ch, 5 );
      else if( !str_cmp( arg, "d" ) )
         move_cube( ch, 6 );
      else if( !str_cmp( arg, "di" ) )
         move_cube( ch, 7 );
      else if( !str_cmp( arg, "b" ) )
         move_cube( ch, 8 );
      else if( !str_cmp( arg, "bi" ) )
         move_cube( ch, 9 );
      else if( !str_cmp( arg, "u" ) )
         move_cube( ch, 10 );
      else if( !str_cmp( arg, "ui" ) )
         move_cube( ch, 11 );
   }

   show_cube( ch );
   check_rubik( ch );

   if( is_completed( ch ) )
   {
      short rcount;

      ch->pcdata->rwins++;
      for( rcount = 0; rcount < 54; rcount++ )
         ch->pcdata->rubik[rcount] = 0;
      send_to_char( "You have completed the rubik's cube, CONGRATULATIONS!!! Type 'rubiks start' if you want to play again.\r\n", ch );
   }
}
