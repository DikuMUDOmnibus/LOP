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

typedef struct trivia_data TRIVIA_DATA;
typedef struct answer_data ANSWER_DATA;

#define TRIVIA_FILE SYSTEM_DIR "trivia.dat"
#define PULSE_TRIVIA (5 * PULSE_PER_SECOND)
#define MAX_QUESTIONS 20

typedef struct use_trivia_data UTRIVIA_DATA;
struct use_trivia_data
{
   bool running;              /* Is it currently running or not */
   TRIVIA_DATA *trivia;       /* What trivia is it on */
   int qasked[MAX_QUESTIONS]; /* Keep a list of asked questions this run */
   short timer;
};
extern UTRIVIA_DATA *utrivia;

struct answer_data
{
   ANSWER_DATA *next, *prev;
   char *answer;
};

struct trivia_data
{
   TRIVIA_DATA *next, *prev;
   ANSWER_DATA *first_answer, *last_answer;
   char *question;
   int reward;
};
