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

typedef struct bank_data BANK_DATA;
typedef struct share_data SHARE_DATA;
typedef struct transaction_data TRANSACTION_DATA;
#define BANK_FILE SYSTEM_DIR "bank.dat" /* Bank file */

/*
 * To make it more secure then just a password.
 * allows ones with access to the account to specify who
 * can access the account.
 */
struct share_data
{
   SHARE_DATA *next, *prev;
   char *name;
};

struct transaction_data
{
   TRANSACTION_DATA *next, *prev;
   char *transaction;   
};

/* Bank data */
struct bank_data
{
   BANK_DATA *next, *prev;
   SHARE_DATA *first_share, *last_share;
   TRANSACTION_DATA *first_transaction, *last_transaction;
   char *created;
   char *account;
   double balance;
};
