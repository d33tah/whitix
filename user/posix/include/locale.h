#ifndef _POSIX_LOCALE_H
#define _POSIX_LOCALE_H

/* TODO: Fix soon. */
#define LC_TIME		0
#define LC_CTYPE	1
#define LC_ALL		2
#define LC_COLLATE	3
#define LC_MONETARY	4
#define LC_NUMERIC	5
#define LC_ADDRESS	6
#define LC_MESSAGES	7

struct lconv
{
	char* decimal_point,*mon_decimal_point;
	char* thousands_sep,*mon_thousands_sep;
	char* grouping, *mon_grouping;
	char int_frac_digits,frac_digits;

	char* currency_symbol,*int_curr_symbol;
	char p_cs_precedes,n_cs_precedes;
	char p_sep_by_space,n_sep_by_space;

	char* positive_sign,*negative_sign;
	char p_sign_posn,n_sign_posn;
};

struct lconv* localeconv();

#endif
