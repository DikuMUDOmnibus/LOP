/*
 * Codebase macros - Change as you might need.
 * Yes, Rogel, you can gloat all you want. You win, this is cleaner, though not by a whole lot.
 */
#ifndef __IMC2CFG_H__
#define __IMC2CFG_H__

#define CH_IMCDATA( ch )  ( (ch)->pcdata->imcchardata )
#define CH_IMCLEVEL( ch ) ( (ch)->trust )
#define CH_IMCNAME( ch )  ( (ch)->name )
#define CH_IMCSEX( ch )   ( (ch)->sex )
#define CH_IMCTITLE( ch ) ( (ch)->pcdata->title )
#define CH_IMCRANK( ch )  ( (ch)->pcdata->rank )

#endif
