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

#ifndef STDARG_H
#define STDARG_H

#define STACK_ALIGN		sizeof(int)

typedef char* VaList;

#define VaStart(list, lastArg) \
	__builtin_va_start(list, lastArg)
	
/* Must be the other way round because of the comma operator */
#define VaArg(list,type) \
    __builtin_va_arg(list, type)

#define VaCopy(dest, src) \
	__builtin_va_copy(dest, src)

/* Just for a certain cleanliness */
#define VaEnd(list) \
	__builtin_va_end(list)

#endif
