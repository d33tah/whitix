/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef ERROR_H
#define ERROR_H

/* 
 * Follows roughly standard Unix error codes
 * If this is updated, update errno.h in libc
 */

#define ESUCCESS	0 /* Success */
#define EPERM		1 /* No permission, operation not permitted */
#define ENOENT		2 /* No such file or directory */
#define EIO			3 /* Low-level I/O error */
#define EBADF		4 /* Bad file descriptor */
#define ENOMEM		5 /* Out of memory */
#define EACCES		6 /* Not allowed access */
#define EBUSY		7 /* Device busy */
#define EFAULT		8 /* Bad address */
#define EEXIST		9 /* Already exists */
#define EMFILE		10 /* Out of file handles */
#define ENOSPC		11 /* No space on device */
#define EROFS		12 /* Read only filesystem/device */
#define ENOSYS		13 /* No such system call */
#define ENOTIMPL	14 /* Not implemented yet */
#define ENOTDIR		15 /* The file descriptor is not a directory */
#define SIOPENDING	16 /* An I/O operation is pending completion */
#define EISDIR		17 /* Is a directory - cannot read or write to it */
#define EINVAL		18 /* Invalid value */
#define EINTR		19 /* Operation interrupted by signal */
#define EPIPE		20 /* Pipe error */

/* Network errors */
#define EWOULDBLOCK	21 /* No new connections to accept etc. */
#define EPROTO		22 /* Protocol error */
#define EDESTADDRREQ 23 /* Destination address required */
#define EFAMILY		24 /* Socket family not supported. */
#define ESOCKTYPE	25 /* Socket type not supported. */
#define EOPNOTSUPP	26 /* Operation not supported on socket. */
#define ECONNRESET	27 /* Connection reset by peer. */
#define ETIMEDOUT	28 /* Operation timed out. */

#endif
