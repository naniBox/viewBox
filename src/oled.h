/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // naniBox.com

    This file is part of viewBox / naniBox.

    viewBox / naniBox is free software; you can redistribute it and/or modify
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

//-----------------------------------------------------------------------------
#ifndef _naniBox_viewBox_oled_H_
#define _naniBox_viewBox_oled_H_

//-----------------------------------------------------------------------------
#include <ch.h>
#include <hal.h>
#include "kb_util.h"

//-----------------------------------------------------------------------------
#define	OLED_PIXEL_SHIFT		0x1C
#define OLED_MAX_COLUMN			0x3F			// 256/4-1
#define OLED_MAX_ROW			0x3F			// 64-1
#define OLED_WIDTH 				256
#define OLED_HEIGHT 			64
#define OLED_CHAR_WIDTH 		5
#define OLED_CHAR_HEIGHT 		7
#define OLED_LINE_HEIGHT 		(OLED_CHAR_HEIGHT+1)
#define OLED_BUFSIZE 			(OLED_WIDTH*OLED_HEIGHT/2) // div2 because each pixel is a nibble


//-----------------------------------------------------------------------------
typedef struct OLEDConfig OLEDConfig;
struct OLEDConfig
{
	SPIConfig spicfg;
	PortPad DC;
	PortPad RES;
};

//-----------------------------------------------------------------------------
typedef struct OLEDDriver OLEDDriver;
struct OLEDDriver
{
	const OLEDConfig * cfgp;
	SPIDriver * spip;
	uint8_t frame_buffer[OLED_BUFSIZE];
};

extern OLEDDriver OD1;

//-----------------------------------------------------------------------------
void OLEDStart(OLEDDriver * odp, const OLEDConfig * ocfgp, SPIDriver * spip);
void OLED_put_pixel(OLEDDriver * od, uint8_t colour, int16_t x, int16_t y);
void OLED_draw_char(OLEDDriver * od, uint8_t ch, uint8_t colour, int16_t x, int16_t y);
void OLED_draw_string(OLEDDriver * od, char * str, uint8_t colour, int16_t x, int16_t y);
void OLED_put_frame_buffer(OLEDDriver * od);
void OLED_clear_frame_buffer(OLEDDriver * od);
void OLED_write_command(OLEDDriver * od, unsigned char d);
void OLED_OLED_write_data(OLEDDriver * od, unsigned char d);
void OLED_exit_partial_display(OLEDDriver * od);
void OLED_set_column_address(OLEDDriver * od, unsigned char a, unsigned char b);
void OLED_set_row_address(OLEDDriver * od, unsigned char a, unsigned char b);
void OLED_set_write_ram(OLEDDriver * od);
void OLED_set_read_ram(OLEDDriver * od);
void OLED_set_remap_format(OLEDDriver * od, unsigned char d);
void OLED_set_start_line(OLEDDriver * od, unsigned char d);
void OLED_set_display_offset(OLEDDriver * od, unsigned char d);
void OLED_set_display_mode(OLEDDriver * od, unsigned char d);
void OLED_set_partial_display(OLEDDriver * od, unsigned char a, unsigned char b, unsigned char c);
void OLED_set_function_selection(OLEDDriver * od, unsigned char d);
void OLED_set_display_on_off(OLEDDriver * od, unsigned char d);
void OLED_set_phase_length(OLEDDriver * od, unsigned char d);
void OLED_set_display_clock(OLEDDriver * od, unsigned char d);
void OLED_set_display_enhancement_a(OLEDDriver * od, unsigned char a, unsigned char b);
void OLED_set_gpio(OLEDDriver * od, unsigned char d);
void OLED_set_precharge_period(OLEDDriver * od, unsigned char d);
void OLED_set_precharge_voltage(OLEDDriver * od, unsigned char d);
void OLED_set_vcomh(OLEDDriver * od, unsigned char d);
void OLED_set_contrast_current(OLEDDriver * od, unsigned char d);
void OLED_set_master_current(OLEDDriver * od, unsigned char d);
void OLED_set_multiplex_ratio(OLEDDriver * od, unsigned char d);
void OLED_set_display_enhancement_b(OLEDDriver * od, unsigned char d);
void OLED_set_command_lock(OLEDDriver * od, unsigned char d);
void OLED_set_linear_gray_scale_table(OLEDDriver * od);
void OLED_fill_ram(OLEDDriver * od, unsigned char Data);
void OLED_fill_block(OLEDDriver * od, unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d);
void OLED_clear_screen(OLEDDriver * od);
void OLED_grayscale(OLEDDriver * od);

#endif // _naniBox_viewBox_oled_H_
