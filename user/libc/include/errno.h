#ifndef LIBC_ERRNO_H
#define LIBC_ERRNO_H

/* FIXME: CLEANUP */

#define ESUCCESS 0	/* Successful */
#define EPERM   1	/* No permission, operation not permitted */
#define ENOENT  2	/* No such file or directory */
#define EIO     3	/* Low-level I/O error */
#define EBADF   4	/* Bad fd */
#define ESRCH	EBADF /* For now */
#define ENOMEM  5	/* Out of memory */
#define EACCES  6	/* Not allowed access */
#define EBUSY   7	/* Device busy */
#define EFAULT  8	/* Bad address */
#define EEXIST  9	/* Already exists */
#define EMFILE  10	/*Out of file handles */
#define ENOSPC  11	/* No space on device */
#define EROFS   12	/* Read only filesystem */
#define ENOSYS  13	/* No system call */
#define ENOTIMPL 14
#define ENOTDIR 15
#define SIOPENDING 16
#define EISDIR 17
#define EINVAL 18
#define EINTR  19
#define E2BIG	20
#define EILSEQ	21
#define EAGAIN	22
#define ECHILD	23
#define ENOEXEC	24
#define ETIMEDOUT	25

#define EADDRINUSE	26
#define EINPROGRESS	27
#define EAFNOSUPPORT	28
#define EISCONN		29
#define ENETUNREACH	30
#define ENAMETOOLONG	31
#define EPIPE		32
#define ENOTEMPTY	33
#define ENOTCONN	34
#define ENOTSOCK	35
#define ENFILE		36
#define ENOTTY		37
#define EALREADY	38
#define ECONNABORTED	39
#define ECONNREFUSED	40
#define EOPNOTSUPP	41
#define ESPIPE	42
#define ECONNRESET	43
#define EHOSTUNREACH	44
#define ENOBUFS	45
#define EMSGSIZE	46
#define ENOPROTOOPT	47
#define EWOULDBLOCK	48
#define EADDRNOTAVAIL	49
#define ESOCKTNOSUPPORT	50
#define EPROTONOSUPPORT	51
#define EXDEV	52
#define ENOLCK	53
#define EIDRM	54

#define EDOM    55 /* Math domain error */
#define ERANGE  56 /* Math range error */

#define ENXIO	57

/* Extras */
#define EDESTADDRREQ	58
#define ENODEV	59

int* errnoLocation();
#define errno *errnoLocation()

#endif
