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

#include <bitmap.h>
#include <module.h>
#include <typedefs.h>

const BYTE bMasks[2][8]={
	{0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F},
	{0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80}
};

/***********************************************************************
 *
 * FUNCTION:	BmapSetBit
 *
 * DESCRIPTION: Set a bit in a bitmap.
 *
 * PARAMETERS:	array - bitmap in question.
 *				index - index (in bits) of the value to be set.
 *				value - value to set bit to - 0 or 1.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void BmapSetBit(void* array,DWORD index,int value)
{	
	DWORD offset=index/8;

	if (value)
		((BYTE*)array)[offset]|=bMasks[value][index & 7];
	else
		((BYTE*)array)[offset]&=bMasks[value][index & 7];
}

SYMBOL_EXPORT(BmapSetBit);

/***********************************************************************
 *
 * FUNCTION:	BmapGetBit
 *
 * DESCRIPTION: Get the value of a bit in a bitmap.
 *
 * PARAMETERS:	array - bitmap in question.
 *				index - index (in bits) of the bit.
 *
 * RETURNS:		Value of the bit at index.
 *
 ***********************************************************************/

int BmapGetBit(void* array,DWORD index)
{
	return ((BYTE*)array)[index/8] & (1 << (index & 7));
}

SYMBOL_EXPORT(BmapGetBit);
