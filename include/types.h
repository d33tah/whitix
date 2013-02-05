/* types.h - common kernel types.
 * ==============================
 *
 *  This file is part of Whitix.
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

#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

typedef long unsigned int size_t;
typedef long unsigned int off_t;
typedef long unsigned int pos_t;
typedef long unsigned int addr_t;
typedef long unsigned int vid_t;
typedef long unsigned int sector_t; /* Index of a sector on disk. */
typedef long unsigned int time_t;

/* Limits */
#define INT_MAX	0x7fffffffL

#endif
