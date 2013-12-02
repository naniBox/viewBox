/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // naniBox.com

    This file is part of kuroBox / naniBox.

    kuroBox / naniBox is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    kuroBox / naniBox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

//----------------------------------------------------------------------------
#ifndef _naniBox_kuroBox_nb_util
#define _naniBox_kuroBox_nb_util

//----------------------------------------------------------------------------
#include "ch.h"
#include "hal.h"

//----------------------------------------------------------------------------
// handy for my driver structs
typedef struct PortPad PortPad;
struct PortPad
{
	ioportid_t port;
	uint16_t pad;
};

//----------------------------------------------------------------------------
// used for the panic function
typedef enum
{
	no_panic	       = 0,
	unknown_panic      = 1
} panic_msg_t;

//----------------------------------------------------------------------------
#define swap_u8(a,b) {uint8_t x=a;a=b;b=x;}
#define ASSERT(expr,where,msg) chDbgAssert(expr, where, msg)

//----------------------------------------------------------------------------
#define KB_OK				0
#define KB_NOT_OK			1
#define KB_ERR_VALUE		2
#define KB_ERR_UNKNOWN		255

//-----------------------------------------------------------------------------
uint8_t  calc_checksum_8( uint8_t * buf, uint16_t buf_size );
uint16_t calc_checksum_16( uint8_t * buf, uint16_t buf_size );

#endif // _naniBox_kuroBox_nb_util
