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

#ifndef SECTIONS_H
#define SECTIONS_H

/* Saves declaring all these variables multiple times */

extern char code[];
extern char endCode[];
extern char data[];
extern char endData[];
extern char bss[];
extern char endbss[];
extern char symstrings_start[];
extern char symstrings_end[];
extern char symtable_start[];
extern char symtable_end[];
extern char initcode_start[];
extern char initcode_end[];
extern char initdata_start[];
extern char initdata_end[];
extern char end[];

#define initcode __attribute__((section(".initcode")))
#define initdata __attribute__((section(".initdata")))

#endif
