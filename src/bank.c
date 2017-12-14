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
 *                            Bank Support                                   *
 *****************************************************************************/
/*
 * Can create/delete an account.
 * Can withdraw/deposit into an account.
 * Can transfer gold from your account to another.
 * Accounts can have other characters share it.
 * Memory Cleaned up on exit.
 * Bank data is saved and loaded.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "h/mud.h"
#include "h/sha256.h"
#include "h/bank.h"

BANK_DATA *first_bank, *last_bank;

#define MGOLD   2000000000

/* Have to handle the max double gold a bit different */
/* Somewhere between 10 sextillion and 100 sextillion the double quits being as accurate as you would want */
double maxdoublegold = 0.0;
void setmaxdoublegold( void )
{
   maxdoublegold += 10000;
   maxdoublegold *= 1000000000;
   maxdoublegold *= 1000000000;
}

/* This is set up to show how much gold a character has */
char *show_char_gold( CHAR_DATA *ch )
{
   if( !ch )
      return (char *)"";

   return num_punct( ch->gold );
}

char *show_bank_balance( BANK_DATA *bank )
{
   if( !bank )
      return (char *)"";

   return double_punct( bank->balance );
}

bool has_gold( CHAR_DATA *ch, int amount )
{
   if( !ch )
      return false;

   if( ch->gold >= amount )
      return true;

   return false;
}

bool has_balance( BANK_DATA *bank, int amount )
{
   if( !bank )
      return false;

   if( bank->balance >= amount )
      return true;

   return false;
}

/* Used to see if the character can hold that much gold */
bool can_hold_gold( CHAR_DATA *ch, int amount )
{
   if( !ch )
      return false;

   if( ( ch->gold + amount ) > MGOLD || ( ch->gold + amount ) < ch->gold )
      return false;

   return true;
}

bool can_hold_balance( BANK_DATA *bank, int amount )
{
   if( !bank )
      return false;

   if( ( bank->balance + amount ) > maxdoublegold || ( bank->balance + amount ) < bank->balance )
      return false;

   return true;
}

void set_gold( CHAR_DATA *ch, int amount )
{
   if( !ch )
      return;

   amount = URANGE( 0, amount, MGOLD );

   ch->gold = amount;
}

/*
 * Only return true if it fully handles the increase.
 */
bool increase_gold( CHAR_DATA *ch, int amount )
{
   if( !ch )
      return false;

   /* Protect to limit it to the max and we are increasing so if it goes - it was way to high so put to max */
   if( ( ch->gold + amount ) >= MGOLD || ( ch->gold + amount ) < ch->gold )
   {
      send_to_char( "You're now carrying as much gold as you can handle.\r\n", ch );
      ch->gold = MGOLD;
      return false;
   }

   ch->gold += amount;

   return true;
}

/*
 * Always take into consideration that bamount and amount will change as they are transfered in increase_big_num
 * so if you wish to show them do so before sending to increase_balance.
 * Only return true if it fully handles the increase.
 */
bool increase_balance( CHAR_DATA *ch, BANK_DATA *bank, int amount )
{
   if( !bank )
      return false;

   if( bank->balance >= maxdoublegold )
   {
      if( ch )
         send_to_char( "The account already has as much gold as it can handle.\r\n", ch );
      return false;
   }

   /* Protect to limit it to the max and we are increasing so if it goes - it was way to high so put to max */
   if( ( bank->balance + amount ) >= maxdoublegold || ( bank->balance + amount ) < bank->balance )
   {
      if( ch )
         send_to_char( "The account now has as much gold as it can handle.\r\n", ch );
      bank->balance = maxdoublegold;
      return false;
   }

   bank->balance += amount;
   return true;
}

void decrease_gold( CHAR_DATA *ch, int amount )
{
   if( !ch )
      return;

   ch->gold -= amount;

   if( ch->gold < 0 )
      ch->gold = 0;
}

void decrease_balance( BANK_DATA *bank, int amount )
{
   if( !bank )
      return;

   bank->balance -= amount;

   if( bank->balance < 0 )
      bank->balance = 0.0;
}

void free_shared( SHARE_DATA *shared )
{
   STRFREE( shared->name );
   DISPOSE( shared );
}

void free_transaction( TRANSACTION_DATA *trans )
{
   STRFREE( trans->transaction );
   DISPOSE( trans );
}

void remove_shared( BANK_DATA *bank, SHARE_DATA *shared )
{
   UNLINK( shared, bank->first_share, bank->last_share, next, prev );
}

void remove_transaction( BANK_DATA *bank, TRANSACTION_DATA *trans )
{
   UNLINK( trans, bank->first_transaction, bank->last_transaction, next, prev );
}

void free_all_shared( BANK_DATA *bank )
{
   SHARE_DATA *shared, *shared_next;

   for( shared = bank->first_share; shared; shared = shared_next )
   {
      shared_next = shared->next;
      remove_shared( bank, shared );
      free_shared( shared );
   }
}

void free_all_transactions( BANK_DATA *bank )
{
   TRANSACTION_DATA *trans, *trans_next;

   for( trans = bank->first_transaction; trans; trans = trans_next )
   {
      trans_next = trans->next;
      remove_transaction( bank, trans );
      free_transaction( trans );
   }
}

void add_shared( BANK_DATA *bank, SHARE_DATA *shared )
{
   LINK( shared, bank->first_share, bank->last_share, next, prev );
}

void add_transaction( BANK_DATA *bank, TRANSACTION_DATA *trans )
{
   LINK( trans, bank->first_transaction, bank->last_transaction, next, prev );
}

void free_bank( BANK_DATA *bank )
{
   STRFREE( bank->account );
   STRFREE( bank->created );
   free_all_shared( bank );
   free_all_transactions( bank );
   DISPOSE( bank );
}

void remove_bank( BANK_DATA *bank )
{
   UNLINK( bank, first_bank, last_bank, next, prev );
}

void free_all_banks( void )
{
   BANK_DATA *bank, *bank_next;

   for( bank = first_bank; bank; bank = bank_next )
   {
      bank_next = bank->next;
      remove_bank( bank );
      free_bank( bank );
   }
}

void add_bank( BANK_DATA *bank )
{
   LINK( bank, first_bank, last_bank, next, prev );
}

void save_banks( void )
{
   BANK_DATA *bank;
   SHARE_DATA *shared;
   TRANSACTION_DATA *trans;
   FILE *fp;

   if( !first_bank )
   {
      remove_file( BANK_FILE );
      return;
   }
   if( !( fp = fopen( BANK_FILE, "w" ) ) )
   {
      bug( "%s: Can't open %s for writing.", __FUNCTION__, BANK_FILE );
      perror( BANK_FILE );
      return;
   }
   for( bank = first_bank; bank; bank = bank->next )
   {
      fprintf( fp, "%s", "#BANK\n" );
      fprintf( fp, "Createdby %s~\n", bank->created );
      fprintf( fp, "Account   %s~\n", bank->account );
      fprintf( fp, "NBalance  %.f\n", bank->balance );
      for( shared = bank->first_share; shared; shared = shared->next )
         if( shared->name )
            fprintf( fp, "Shared    %s~\n", shared->name );
      for( trans = bank->first_transaction; trans; trans = trans->next )
         if( trans->transaction )
            fprintf( fp, "Trans     %s~\n", trans->transaction );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

BANK_DATA *new_bank( void )
{
   BANK_DATA *bank = NULL;

   CREATE( bank, BANK_DATA, 1 );
   if( !bank )
   {
      bug( "%s: bank is NULL after CREATE.", __FUNCTION__ );
      return NULL;
   }
   bank->created = NULL;
   bank->account = NULL;
   bank->first_share = bank->last_share = NULL;
   bank->first_transaction = bank->last_transaction = NULL;
   bank->balance = 0.0;
   return bank;
}

void fread_bank( FILE *fp )
{
   BANK_DATA *bank;
   const char *word;
   bool fMatch;

   bank = new_bank( );

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
               add_bank( bank );
               return;
	    }
	    break;

         case 'A':
            KEY( "Account", bank->account, fread_string( fp ) );
            break;

         case 'B':
            if( !str_cmp( word, "BBalance" ) )
            {
               double check;
               int bbalance = fread_number( fp );

               check = 0.0;
               check += bbalance;
               check *= 1000000000;
               bank->balance += check; /* Billions */
               fMatch = true;
               break;
            }
            if( !str_cmp( word, "Balance" ) )
            {
               int balance = fread_number( fp );

               bank->balance += balance; /* Under a billion */
               fMatch = true;
               break;
            }
            break;

         case 'C':
            KEY( "Createdby", bank->created, fread_string( fp ) );
            break;

         case 'M':
            if( !str_cmp( word, "MBalance" ) )
            {
               double check;
               int mbalance = fread_number( fp );

               check = 0.0;
               check += mbalance;
               check *= 1000000;
               bank->balance += check; /* Millions */
               fMatch = true;
               break;
            }
            break;

         case 'N':
            KEY( "NBalance", bank->balance, fread_double( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Shared" ) )
            {
               SHARE_DATA *shared;

               CREATE( shared, SHARE_DATA, 1 );
               shared->name = fread_string( fp );
               add_shared( bank, shared );
               fMatch = true;
               break;
            }
            break;

         case 'T':
            if( !str_cmp( word, "Trans" ) )
            {
               TRANSACTION_DATA *trans;

               CREATE( trans, TRANSACTION_DATA, 1 );
               trans->transaction = fread_string( fp );
               add_transaction( bank, trans );
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
   free_bank( bank );
}

void load_banks( void )
{
   FILE *fp;

   first_bank = last_bank = NULL;
   if( !( fp = fopen( BANK_FILE, "r" ) ) )
      return;
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
      if( !str_cmp( word, "BANK" ) )
      {
         fread_bank( fp );
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

typedef enum
{
   BANK_DELETE, BANK_SHARE, BANK_TRANSFER, BANK_BALANCE, BANK_WITHDRAW, BANK_DEPOSIT, BANK_CREATE
} bank_types;

BANK_DATA *find_bank( char *account )
{
   BANK_DATA *bank;

   for( bank = first_bank; bank; bank = bank->next )
   {
      if( !str_cmp( bank->account, account ) )
         return bank;
   }
   return NULL;
}

CHAR_DATA *find_banker( CHAR_DATA *ch )
{
   CHAR_DATA *banker = NULL;

   for( banker = ch->in_room->first_person; banker; banker = banker->next_in_room )
   {
      if( is_npc( banker ) && xIS_SET( banker->act, ACT_BANKER ) )
         break;
   }
   return banker;
}

bool can_access_bank( BANK_DATA *bank, CHAR_DATA *ch )
{
   SHARE_DATA *shared;

   /* Creater of the account naturally has access */
   if( !str_cmp( bank->created, ch->name ) )
      return true;
   for( shared = bank->first_share; shared; shared = shared->next )
   {
      /* If they are in the shared then they have access */
      if( !str_cmp( shared->name, ch->name ) )
         return true;
   }
   /* Well made it this far so naturally they can't access it */
   return false;
}

void modify_shared( CHAR_DATA *ch, BANK_DATA *bank, char *argument )
{
   SHARE_DATA *shared;
   char arg[MIL];
   bool adding, save = false;

   /* Only allow the creator to change who has access to the account */
   if( str_cmp( bank->created, ch->name ) )
   {
      send_to_char( "Sorry, but only the creator of the account can change who has access to it.\r\n", ch );
      return;
   }

   /* See if they have access to it already and they aren't the leader */
   while( argument && argument[0] != '\0' )
   {
      adding = true; /* Lets assume we will be adding at first */
      argument = one_argument( argument, arg );

      /* If the arg is the creator just ignore it so it doesn't get put in as shared */
      if( !str_cmp( bank->created, arg ) )
         continue;

      for( shared = bank->first_share; shared; shared = shared->next )
      {
         /* Ok it was in the list so lets remove it */
         if( !str_cmp( shared->name, arg ) )
         {
            remove_shared( bank, shared );
            free_shared( shared );
            adding = false;
            save = true;
            ch_printf( ch, "%s is no longer able to access the account.\r\n", arg );
            break; /* Break out the for loop */
         }
      }
      /* Didn't find one so lets add it */
      if( adding )
      {
         /* Check for a valid pfile */
         if( !valid_pfile( arg ) )
         {
            ch_printf( ch, "%s is not a valid player and can't be added.\r\n", arg );
            continue;
         }
         CREATE( shared, SHARE_DATA, 1 );
         shared->name = STRALLOC( arg );
         add_shared( bank, shared );
         save = true;
         ch_printf( ch, "%s is now able to access the account.\r\n", arg );
      }
   }
   if( save )
      save_banks( );
   else
      send_to_char( "Nothing changed in the account.\r\n", ch );
}

void new_transaction( BANK_DATA *bank, char *string )
{
   TRANSACTION_DATA *trans;
   struct timeval now_time;
   char buf[MSL];
   int count = 0;

   /* Update time. */
   gettimeofday( &now_time, NULL );
   current_time = ( time_t ) now_time.tv_sec;
   current_time += ( time_t ) TIME_MODIFY;

   snprintf( buf, sizeof( buf ), "%s %s", distime( current_time ), string );
   CREATE( trans, TRANSACTION_DATA, 1 );
   STRSET( trans->transaction, buf );
   add_transaction( bank, trans );

   for( trans = bank->first_transaction; trans; trans = trans->next )
      ++count;
   while( count > 20 )
   {
      trans = bank->first_transaction;
      remove_transaction( bank, trans );
      free_transaction( trans );
      --count;
   }
}

void give_interest( void )
{
   BANK_DATA *bank = NULL;
   char buf[MSL];
   double interest = 0.0;

   for( bank = first_bank; bank; bank = bank->next )
   {
      if( bank->balance <= 0 )
         continue;

      interest += ( bank->balance * .0003 );
      if( interest > 0 )
      {
         if( ( bank->balance + interest ) > maxdoublegold )
         {
            interest = maxdoublegold;
            interest -= bank->balance;
            if( interest > 0 )
            {
               snprintf( buf, sizeof( buf ), "Gained %s interest.", double_punct( interest ) );
               new_transaction( bank, buf );
               bank->balance = maxdoublegold;
            }
         }
         else
         {
            snprintf( buf, sizeof( buf ), "Gained %s interest.", double_punct( interest ) );
            new_transaction( bank, buf );
            bank->balance += interest;
         }
      }
   }
   save_banks( );
}

void handle_bank( CHAR_DATA *ch, char *account, short type, int amount, char *taccount, bool uall )
{
   CHAR_DATA *banker = NULL;
   BANK_DATA *bank = NULL, *tbank = NULL;
   TRANSACTION_DATA *trans;
   char buf[MSL];

   if( !ch )
      return;

   if( !( banker = find_banker( ch ) ) )
   {
      send_to_char( "You aren't at a banker.\r\n", ch );
      return;
   }

   /* Other wise we should be doing something to balance */
   if( type != BANK_SHARE && type != BANK_DELETE && type != BANK_BALANCE && amount <= 0 && !uall )
   {
      send_to_char( "You need to use a number above 0 or all.\r\n", ch );
      return;
   }

   if( ( bank = find_bank( account ) ) && type == BANK_CREATE )
   {
      send_to_char( "An account by that name already exist.\r\n", ch );
      return;
   }

   if( !bank && type != BANK_CREATE )
   {
      send_to_char( "Invalid account.\r\n", ch );
      return;
   }

   /* Create a new account with amount deposit */
   if( type == BANK_CREATE )
   {
      if( uall )
         amount = ch->gold;
      else
      {
         if( amount <= 0 )
         {
            send_to_char( "You need to at least deposit something to open an account.\r\n", ch );
            return;
         }
         if( !has_gold( ch, amount ) )
         {
            send_to_char( "You don't have that much gold.\r\n", ch );
            return;
         }
      }
      if( !( bank = new_bank( ) ) )
         return;

      bank->created = STRALLOC( ch->name );
      bank->account = STRALLOC( account );

      decrease_gold( ch, amount );
      increase_balance( ch, bank, amount );
      snprintf( buf, sizeof( buf ), "%s opened the account with %s gold.", ch->name, num_punct( amount ) );
      new_transaction( bank, buf );
      add_bank( bank );
      save_banks( );
      save_char_obj( ch );
      ch_printf( ch, "Account %s has been created and has a balance of %s gold.\r\n", bank->account, show_bank_balance( bank ) );
      return;
   }

   /* Deposit amount */
   if( type == BANK_DEPOSIT )
   {
      if( uall )
         amount = ch->gold;
      else
      {
         if( amount <= 0 )
         {
            send_to_char( "You can't deposit 0 or less gold.\r\n", ch );
            return;
         }
         if( !has_gold( ch, amount ) )
         {
            send_to_char( "You can't deposit more gold then you have.\r\n", ch );
            return;
         }
      }

      if( !can_hold_balance( bank, amount ) )
      {
         ch_printf( ch, "Account %s can't hold that much gold.\r\n", bank->account );
         return;
      }

      decrease_gold( ch, amount );
      increase_balance( ch, bank, amount );
      snprintf( buf, sizeof( buf ), "%s deposited %s gold.", ch->name, num_punct( amount ) );
      new_transaction( bank, buf );

      save_banks( );
      save_char_obj( ch );

      ch_printf( ch, "You have deposited %s gold", num_punct( amount ) );
      if( can_access_bank( bank, ch ) )
         ch_printf( ch, " and now have %s gold on account %s", show_bank_balance( bank ), bank->account );
      send_to_char( ".\r\n", ch );
      return;
   }

   /* Do they have access to this account? */
   if( !can_access_bank( bank, ch ) )
   {
      ch_printf( ch, "You don't have permission to do anything with account %s.\r\n", bank->account );
      return;
   }

   /* Modify who all else can access this account */
   if( type == BANK_SHARE )
   {
      modify_shared( ch, bank, taccount );
      return;
   }

   /* Check balance */
   if( type == BANK_BALANCE )
   {
      ch_printf( ch, "Account %s has %s gold.\r\n", bank->account, show_bank_balance( bank ) );

      for( trans = bank->first_transaction; trans; trans = trans->next )
      {
         if( trans == bank->first_transaction )
            send_to_char( "Last 20 Transactions.\r\n", ch );
         ch_printf( ch, "%s\r\n", trans->transaction );
      }
      return;
   }

   /* Delete account */
   if( type == BANK_DELETE )
   {
      /* Only allow the creator to delete the account, since all money in it is lost */
      if( str_cmp( bank->created, ch->name ) )
      {
         send_to_char( "Sorry, but only the creator of the account can delete it.\r\n", ch );
         return;
      }

      ch_printf( ch, "Account %s has been deleted.\r\n", bank->account );
      remove_bank( bank );
      free_bank( bank );
      save_banks( );
      return;
   }

   /* Withdraw amount */
   if( type == BANK_WITHDRAW )
   {
      if( uall )
      {
         if( bank->balance > MGOLD )
         {
            send_to_char( "You can't hold that much gold.\r\n", ch );
            return;
         }
         amount = (int)bank->balance;
      }
      else
      {
         if( !has_balance( bank, amount ) )
         {
            send_to_char( "You don't have that much to withdraw.\r\n", ch );
            return;
         }
         if( amount <= 0 )
         {
            send_to_char( "You can't withdraw 0 or less coins.\r\n", ch );
            return;
         }
      }

      if( !can_hold_gold( ch, amount ) )
      {
         send_to_char( "You can't hold that much gold.\r\n", ch );
         return;
      }

      increase_gold( ch, amount );
      decrease_balance( bank, amount );
      snprintf( buf, sizeof( buf ), "%s withdrawed %s gold.", ch->name, num_punct( amount ) );
      new_transaction( bank, buf );
      ch_printf( ch, "You have withdrawn %s gold ", num_punct( amount ) );
      ch_printf( ch, "and now have %s gold on account %s.\r\n", show_bank_balance( bank ), bank->account );
      if( bank->balance <= 0 )
      {
         ch_printf( ch, "As you get the last of the gold from account %s, it is closed.\r\n", bank->account );
         remove_bank( bank );
         free_bank( bank );
      }
      save_banks( );
      save_char_obj( ch );
      return;
   }

   /* Transfer amount */
   if( type == BANK_TRANSFER )
   {
      if( uall )
      {
         if( bank->balance > MGOLD )
         {
            send_to_char( "You can't transfer that much at once.\r\n", ch );
            return;
         }
         amount = (int)bank->balance;
      }
      else
      {
         if( amount <= 0 )
         {
            send_to_char( "You can't transfer 0 or less gold.\r\n", ch );
            return;
         }
         if( !has_balance( bank, amount ) )
         {
            send_to_char( "You don't have that much in that account.\r\n", ch );
            return;
         }
      }

      if( !( tbank = find_bank( taccount ) ) )
      {
         send_to_char( "Invalid account to transfer funds to.\r\n", ch );
         return;
      }

      if( !can_hold_balance( tbank, amount ) )
      {
         send_to_char( "That account can't hold that much gold.\r\n", ch );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s transfered %s gold from %s.", ch->name, num_punct( amount ), bank->account );
      new_transaction( tbank, buf );
      snprintf( buf, sizeof( buf ), "%s transfered %s gold to %s.", ch->name, num_punct( amount ), tbank->account );
      new_transaction( bank, buf );

      decrease_balance( bank, amount );

      ch_printf( ch, "You have transfered %s gold from %s to %s.\r\n", num_punct( amount ), bank->account, tbank->account );

      increase_balance( ch, tbank, amount );

      if( bank->balance <= 0 )
      {
         remove_bank( bank );
         free_bank( bank );
         send_to_char( "As you transfer the last of the gold in the account, it is closed.\r\n", ch );
      }

      save_banks( );
      return;
   }

   send_to_char( "Ok, now exactly how did you get here???\r\n", ch );
}

void bank_Usage_display( CHAR_DATA *ch )
{
   send_to_char( "Usage: bank [<account>] delete/balance\r\n", ch );
   send_to_char( "Usage: bank [<account>] share <name>\r\n", ch );
   send_to_char( "Usage: bank [<account>] create/deposit/withdraw <amount>/all.\r\n", ch );
   send_to_char( "Usage: bank [<account>] transfer <amount> <account>\r\n", ch );
}

/* Find the first bank someone has access to */
char *get_first_bank( CHAR_DATA *ch )
{
   BANK_DATA *bank;

   for( bank = first_bank; bank; bank = bank->next )
   {
      if( !can_access_bank( bank, ch ) )
         continue;
      return bank->account;
   }

   return NULL;
}

CMDF( do_bank )
{
   CHAR_DATA *banker = NULL;
   BANK_DATA *bank;
   char arg[MIL], account[MIL], *nbank;
   int amount = 0, bamount = 0, count = 0;
   short type;
   bool uall = false;

   /* Show a character all the bank accounts they can access and the amount */
   if( ( !argument || argument[0] == '\0' ) )
   {
      for( bank = first_bank; bank; bank = bank->next )
      {
         if( !can_access_bank( bank, ch ) )
            continue;

         amount++;
         ch_printf( ch, "&[white]%15.15s &[yellow]%19s&D", bank->account, show_bank_balance( bank ) );
         if( ++count == 3 )
         {
            count = 0;
            send_to_char( "\r\n", ch );
         }
         else
            send_to_char( " ", ch );
      }
      if( count != 0 )
         send_to_char( "\r\n", ch );
      if( amount == 0 )
         send_to_char( "&[white]No accounts found to display.&D\r\n", ch );
      else
         ch_printf( ch, "&[lblue]%d &[white]accounts.&D\r\n", amount );     
      return;
   }

   if( !( banker = find_banker( ch ) ) )
   {
      send_to_char( "You aren't at a banker.\r\n", ch );
      return;
   }

   argument = one_argument( argument, account );
   if( account == NULL || account[0] == '\0' )
   {
      bank_Usage_display( ch );
      return;
   }

   if( !str_cmp( account, "delete" ) && str_cmp( argument, "delete" ) )
   {
      send_to_char( "Please specify the account you want to delete.\r\n", ch );
      return;
   }

   /* Just put arg as account */
   if( !str_cmp( account, "create" ) || !str_cmp( account, "balance" )
   || !str_cmp( account, "deposit" ) || !str_cmp( account, "withdraw" )
   || !str_cmp( account, "transfer" ) || !str_cmp( account, "share" ) )
   {
      snprintf( arg, sizeof( arg ), "%s", account );
      if( !( nbank = get_first_bank( ch ) ) )
         snprintf( account, sizeof( account ), "%s", ch->name );
      else
         snprintf( account, sizeof( account ), "%s", nbank );
   }
   else
      argument = one_argument( argument, arg );

   if( !str_cmp( arg, "create" ) )        type = BANK_CREATE;
   else if( !str_cmp( arg, "balance" ) )  type = BANK_BALANCE;
   else if( !str_cmp( arg, "deposit" ) )  type = BANK_DEPOSIT;
   else if( !str_cmp( arg, "withdraw" ) ) type = BANK_WITHDRAW;
   else if( !str_cmp( arg, "transfer" ) ) type = BANK_TRANSFER;
   else if( !str_cmp( arg, "delete" ) )   type = BANK_DELETE;
   else if( !str_cmp( arg, "share" ) )    type = BANK_SHARE;
   else
   {
      bank_Usage_display( ch );
      return;
   }

   if( type != BANK_SHARE && type != BANK_DELETE && type != BANK_BALANCE )
   {
      argument = one_argument( argument, arg );
      if( !str_cmp( arg, "all" ) )
         uall = true;
      else
      {
         if( !is_number( arg ) || !( amount = atoi( arg ) ) || amount <= 0 || amount > MGOLD )
         {
            ch_printf( ch, "You must specify a valid number from 1 - %s.\r\n", num_punct( MGOLD ) );
            return;
         }
      }
   }

   if( type != BANK_SHARE && type != BANK_DELETE && type != BANK_BALANCE && amount <= 0 && bamount <= 0 && !uall )
   {
      bank_Usage_display( ch );
      return;
   }

   handle_bank( ch, account, type, amount, argument, uall );
}

void remove_from_banks( const char *player )
{
   BANK_DATA *bank, *bank_next;
   SHARE_DATA *shared, *shared_next;
   bool resave = false;

   for( bank = first_bank; bank; bank = bank_next )
   {
      bank_next = bank->next;

      /* Remove all banks created by the player */
      if( !str_cmp( bank->created, player ) )
      {
         remove_bank( bank );
         free_bank( bank );
         resave = true;
         continue;
      }

      /* Remove player from any banks it has access to */
      for( shared = bank->first_share; shared; shared = shared_next )
      {
         shared_next = shared->next;

         if( !str_cmp( shared->name, player ) )
         {
            remove_shared( bank, shared );
            free_shared( shared );
            resave = true;
         }
      }
   }

   if( resave )
      save_banks( );
}

void rename_in_banks( const char *oplayer, const char *nplayer )
{
   BANK_DATA *bank, *bank_next;
   SHARE_DATA *shared, *shared_next;
   bool resave = false;

   for( bank = first_bank; bank; bank = bank_next )
   {
      bank_next = bank->next;

      if( !str_cmp( bank->created, oplayer ) )
      {
         STRSET( bank->created, nplayer );
         resave = true;
      }

      /* Rename player for any banks it has access to */
      for( shared = bank->first_share; shared; shared = shared_next )
      {
         shared_next = shared->next;

         if( !str_cmp( shared->name, oplayer ) )
         {
            STRSET( shared->name, nplayer );
            resave = true;
         }
      }
   }

   if( resave )
      save_banks( );
}
