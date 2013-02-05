/* TODO: Document */

#ifndef _AR_H
#define _AR_H

#define	OARMAG1	0177555
#define	OARMAG2	0177545

#define	ARMAG		"!<arch>\n"
#define	SARMAG		8

#define	AR_EFMT1	"#1/"

struct ar_hdr {
	char ar_name[16];		/* name */
	char ar_date[12];		/* modification time */
	char ar_uid[6];			/* user id */
	char ar_gid[6];			/* group id */
	char ar_mode[8];		/* octal file permissions */
	char ar_size[10];		/* size in bytes */
#define	ARFMAG	"`\n"
	char ar_fmag[2];		/* consistency check */
};

#endif
