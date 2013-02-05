#include <sys/types.h>
#include <locale.h>
#include <langinfo.h>

#include <stdio.h> //TEMP

struct lconv defLocal=
{
	.decimal_point = ".",
	.mon_decimal_point = ".",
	.thousands_sep = ",",
	.mon_thousands_sep = ",",
	.grouping = "\3",
	.mon_grouping = "\3",
	.int_frac_digits = 2,
	.frac_digits = 2,
	.currency_symbol = "£",
	.int_curr_symbol = "GBP",
	.p_cs_precedes = 1,
	.n_cs_precedes = 1,
	.p_sep_by_space = 0,
	.n_sep_by_space = 0,
	.positive_sign = "+",
	.negative_sign = "-",
	.p_sign_posn = 1,
	.n_sign_posn = 1,
};

char* def="";

char* setlocale(int category, const char* locale)
{
	// TODO
	return "";
}

char* nl_langinfo(nl_item item)
{
	char* ret=NULL;

	switch (item)
	{
		case CODESET:
			ret="ANSI_X3.4-1968";
		default:
//			printf("nl_langinfo: TODO (%d)\n", item);
			break;
	};

	return ret;
}

struct lconv* localeconv(void)
{
	return &defLocal;
}
