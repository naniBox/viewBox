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
#include <ch.h>
#include <string.h>
#include "oled.h"
#include "oled_char_data.c"
#include "naniBox_256x64_v2.c"
#include "kbb_types.h"

STATIC_ASSERT(sizeof(OD1.frame_buffer)==sizeof(logo),FRAME_BUFFER_LOGO_MISMATCH_1);
STATIC_ASSERT(sizeof(OD1.frame_buffer)==LOGO_SIZE,   FRAME_BUFFER_LOGO_MISMATCH_2);

//-----------------------------------------------------------------------------
static void oled_finish_xfer(SPIDriver * spip);

//-----------------------------------------------------------------------------
OLEDDriver OD1;

//-----------------------------------------------------------------------------
static const OLEDConfig oled_default_cfg =
{
	{
		oled_finish_xfer,
		GPIOB,
		GPIOB_OLED_CS,
		SPI_CR1_BR_0 // SPI_CR1_BR_1 | SPI_CR1_BR_0 //| SPI_CR1_CPOL | SPI_CR1_CPHA
	},
	{	GPIOB, GPIOB_OLED_DC 	},
	{	GPIOB, GPIOB_OLED_RES 	}
};

//-----------------------------------------------------------------------------
static void
oled_finish_xfer(SPIDriver * spip)
{
	(void)spip;
	spiUnselectI(spip);
}

//-----------------------------------------------------------------------------
void OLEDStart(OLEDDriver * od, const OLEDConfig * ocfgp, SPIDriver * spip)
{
	if ( ocfgp )
		od->cfgp = ocfgp;
	else
		od->cfgp = &oled_default_cfg;
	od->spip = spip;

	palSetPad(od->cfgp->spicfg.ssport, od->cfgp->spicfg.sspad);
	palSetPad(od->cfgp->RES.port, od->cfgp->RES.pad);
	chThdSleepMilliseconds(50);
	palClearPad(od->cfgp->RES.port, od->cfgp->RES.pad);
	chThdSleepMilliseconds(50);
	palSetPad(od->cfgp->RES.port, od->cfgp->RES.pad);
	chThdSleepMilliseconds(50);

	spiStart(od->spip, &od->cfgp->spicfg);

    spiSelect(od->spip);

	OLED_set_command_lock(od, 0x12);			// Unlock Basic Commands (0x12/0x16)
	OLED_set_display_on_off(od, 0x00);		// Display Off (0x00/0x01)
	OLED_set_column_address(od,  0x1C, 0x5B );
	OLED_set_row_address(od,  0x00, 0x3F );
	OLED_set_display_clock(od, 0x91);		// Set Clock as 80 Frames/Sec
	OLED_set_multiplex_ratio(od, 0x3F);		// 1/64 Duty (0x0F~0x3F)
	OLED_set_display_offset(od, 0x00);		// Shift Mapping RAM Counter (0x00~0x3F)
	OLED_set_start_line(od, 0x00);			// Set Mapping RAM Display Start Line (0x00~0x7F)
	OLED_set_remap_format(od, 0x14);			// Set Horizontal Address Increment
					//     Column Address 0 Mapped to SEG0
					//     Disable Nibble Remap
					//     Scan from COM[N-1] to COM0
					//     Disable COM Split Odd Even
					//     Enable Dual COM Line Mode
	OLED_set_gpio(od, 0x00);				// Disable GPIO Pins Input
	OLED_set_function_selection(od, 0x01);		// Enable Internal VDD Regulator
	OLED_set_display_enhancement_a(od, 0xA0,0xFD);

	OLED_set_contrast_current(od, 0xFF);		// Set Segment Output Current
	OLED_set_master_current(od, 0x0F);		// Set Scale Factor of Segment Output Current Control
	//  OLED_set_gray_scale_table(od, );			// Set Pulse Width for Gray Scale Table
	OLED_set_linear_gray_scale_table(od);
	OLED_set_phase_length(od, 0x32);			// Set Phase 1 as 5 Clocks & Phase 2 as 14 Clocks
	//OLED_set_display_enhancement_b(od, 0x20);	// Enhance Driving Scheme Capability (0x00/0x20)
	OLED_set_display_enhancement_b(od, 0x00);	// Enhance Driving Scheme Capability (0x00/0x20)
	OLED_set_precharge_voltage(od, 0x1F);		// Set Pre-Charge Voltage Level as 0.60*VCC
	OLED_set_precharge_period(od, 0x08);		// Set Second Pre-Charge Period as 8 Clocks
	OLED_set_vcomh(od, 0x07);			// Set Common Pins Deselect Voltage Level as 0.86*VCC
	//OLED_set_display_mode(od, 0x02);			// Normal Display Mode (0x00/0x01/0x02/0x03)
	OLED_set_display_mode(od, 0x02);			// Normal Display Mode (0x00/0x01/0x02/0x03)
	//OLED_set_partial_display(od, 0x01,0x00,0x00);	// Disable Partial Display
	//Exit_Partial_Display();
	OLED_set_display_on_off(od, 0x01);		// Display On (0x00/0x01)

	spiUnselect(od->spip);

	OLED_clear_frame_buffer(od);
	memcpy(od->frame_buffer, logo, LOGO_SIZE);
	OLED_put_frame_buffer(od);
	// the put frame is async, takes ~3ms on the standard SPI config, so just
	// wait around to make sure it's finished during startup.
	chThdSleepMilliseconds(10);
}

//-----------------------------------------------------------------------------
void OLED_put_pixel(OLEDDriver * od, uint8_t colour, int16_t x, int16_t y)
{
	if (y >= OLED_HEIGHT)
		return;
	if (x >= OLED_WIDTH)
		return;
	uint16_t offset = (y << 7) + (x >> 1);
	uint8_t * buf = od->frame_buffer + offset;

	if (x & 0x01)
		*buf = (*buf & 0xf0) | colour;
	else
		*buf = (*buf & 0x0f) | (colour << 4);
}

//-----------------------------------------------------------------------------
void OLED_draw_char(OLEDDriver * od, uint8_t ch, uint8_t colour, int16_t x, int16_t y)
{
	for (uint8_t i = 0; i < 5; i++)
	{
		uint8_t line = OLED_font[(ch * 5) + i];
		int16_t sxi = x + i;
		int16_t syi = y;
		OLED_put_pixel(od, 0x01 & line ? colour : 0, sxi, syi++);
		OLED_put_pixel(od, 0x02 & line ? colour : 0, sxi, syi++);
		OLED_put_pixel(od, 0x04 & line ? colour : 0, sxi, syi++);
		OLED_put_pixel(od, 0x08 & line ? colour : 0, sxi, syi++);
		OLED_put_pixel(od, 0x10 & line ? colour : 0, sxi, syi++);
		OLED_put_pixel(od, 0x20 & line ? colour : 0, sxi, syi++);
		OLED_put_pixel(od, 0x40 & line ? colour : 0, sxi, syi++);
	}
}

//-----------------------------------------------------------------------------
void OLED_draw_string(OLEDDriver * od, char * str, uint8_t colour, int16_t x, int16_t y)
{
	while (*str)
	{
		OLED_draw_char(od, *str, colour, x, y);
		x += OLED_CHAR_WIDTH + 1;
		str++;
	}
}

//-----------------------------------------------------------------------------
void OLED_put_frame_buffer(OLEDDriver * od)
{
    spiSelect(od->spip);

    palClearPad(od->cfgp->DC.port, od->cfgp->DC.pad);
	OLED_set_column_address(od, 0x1C, 0x5B);
	OLED_set_row_address(od, 0x00, 0x40);
	OLED_set_write_ram(od);

	palSetPad(od->cfgp->DC.port, od->cfgp->DC.pad);
	spiStartSend(od->spip, OLED_BUFSIZE, od->frame_buffer);
}

//---------------------------------------------------------------------------
void OLED_clear_frame_buffer(OLEDDriver * od)
{
	memset(od->frame_buffer, 0, sizeof(od->frame_buffer));
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void OLED_write_command(OLEDDriver * od, unsigned char d)
{
	palClearPad(od->cfgp->DC.port, od->cfgp->DC.pad);
	spiPolledExchange(od->spip, d);
}

//-----------------------------------------------------------------------------
void OLED_write_data(OLEDDriver * od, unsigned char d)
{
	palSetPad(od->cfgp->DC.port, od->cfgp->DC.pad);
	spiPolledExchange(od->spip, d);
}

//-----------------------------------------------------------------------------
void OLED_exit_partial_display(OLEDDriver * od)
{
	OLED_write_command(od, 0xA9);
}

//-----------------------------------------------------------------------------
void OLED_set_column_address(OLEDDriver * od, unsigned char a, unsigned char b)
{
	OLED_write_command(od, 0x15);			// Set Column Address
	OLED_write_data(od, a);				//   Default => 0x00
	OLED_write_data(od, b);				//   Default => 0x77
}

//-----------------------------------------------------------------------------
void OLED_set_row_address(OLEDDriver * od, unsigned char a, unsigned char b)
{
	OLED_write_command(od, 0x75);			// Set Row Address
	OLED_write_data(od, a);				//   Default => 0x00
	OLED_write_data(od, b);				//   Default => 0x7F
}

//-----------------------------------------------------------------------------
void OLED_set_write_ram(OLEDDriver * od)
{
	OLED_write_command(od, 0x5C);			// Enable MCU to Write into RAM
}

//-----------------------------------------------------------------------------
void OLED_set_read_ram(OLEDDriver * od)
{
	OLED_write_command(od, 0x5D);			// Enable MCU to Read from RAM
}

//-----------------------------------------------------------------------------
void OLED_set_remap_format(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xA0);			// Set Re-Map / Dual COM Line Mode
	OLED_write_data(od, d);				//   Default => 0x40
	//     Horizontal Address Increment
	//     Column Address 0 Mapped to SEG0
	//     Disable Nibble Remap
	//     Scan from COM0 to COM[N-1]
	//     Disable COM Split Odd Even
	OLED_write_data(od, 0x11);		//   Default => 0x01 (Disable Dual COM Mode)
}

//-----------------------------------------------------------------------------
void OLED_set_start_line(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xA1);			// Set Vertical Scroll by RAM
	OLED_write_data(od, d);				//   Default => 0x00
}

//-----------------------------------------------------------------------------
void OLED_set_display_offset(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xA2);			// Set Vertical Scroll by Row
	OLED_write_data(od, d);				//   Default => 0x00
}

//-----------------------------------------------------------------------------
void OLED_set_display_mode(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xA4 | d);			// Set Display Mode
	//   Default => 0xA4
	//     0xA4 (0x00) => Entire Display Off, All Pixels Turn Off
	//     0xA5 (0x01) => Entire Display On, All Pixels Turn On at GS Level 15
	//     0xA6 (0x02) => Normal Display
	//     0xA7 (0x03) => Inverse Display
}

//-----------------------------------------------------------------------------
void OLED_set_partial_display(OLEDDriver * od, unsigned char a, unsigned char b, unsigned char c)
{
	OLED_write_command(od, 0xA8 | a);
	// Default => 0x8F
	//   Select Internal Booster at Display On
	if (a == 0x00)
	{
		OLED_write_data(od, b);
		OLED_write_data(od, c);
	}
}

//-----------------------------------------------------------------------------
void OLED_set_function_selection(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xAB);			// Function Selection
	OLED_write_data(od, d);				//   Default => 0x01
	//     Enable Internal VDD Regulator
}

//-----------------------------------------------------------------------------
void OLED_set_display_on_off(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xAE | d);			// Set Display On/Off
	//   Default => 0xAE
	//     0xAE (0x00) => Display Off (Sleep Mode On)
	//     0xAF (0x01) => Display On (Sleep Mode Off)
}

//-----------------------------------------------------------------------------
void OLED_set_phase_length(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xB1);// Phase 1 (Reset) & Phase 2 (Pre-Charge) Period Adjustment
	OLED_write_data(od, d);	//   Default => 0x74 (7 Display Clocks [Phase 2] / 9 Display Clocks [Phase 1])
	//     D[3:0] => Phase 1 Period in 5~31 Display Clocks
	//     D[7:4] => Phase 2 Period in 3~15 Display Clocks
}

//-----------------------------------------------------------------------------
void OLED_set_display_clock(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xB3);// Set Display Clock Divider / Oscillator Frequency
	OLED_write_data(od, d);				//   Default => 0xD0
	//     A[3:0] => Display Clock Divider
	//     A[7:4] => Oscillator Frequency
}

//-----------------------------------------------------------------------------
void OLED_set_display_enhancement_a(OLEDDriver * od, unsigned char a, unsigned char b)
{
	OLED_write_command(od, 0xB4);			// Display Enhancement
	OLED_write_data(od, 0xA0 | a);			//   Default => 0xA2
	//     0xA0 (0x00) => Enable External VSL
	//     0xA2 (0x02) => Enable Internal VSL (Kept VSL Pin N.C.)
	OLED_write_data(od, 0x05 | b);			//   Default => 0xB5
	//     0xB5 (0xB0) => Normal
	//     0xFD (0xF8) => Enhance Low Gray Scale Display Quality
}

//-----------------------------------------------------------------------------
void OLED_set_gpio(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xB5);			// General Purpose IO
	OLED_write_data(od, d);	//   Default => 0x0A (GPIO Pins output Low Level.)
}

//-----------------------------------------------------------------------------
void OLED_set_precharge_period(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xB6);			// Set Second Pre-Charge Period
	OLED_write_data(od, d);				//   Default => 0x08 (8 Display Clocks)
}

//-----------------------------------------------------------------------------
void OLED_set_precharge_voltage(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xBB);			// Set Pre-Charge Voltage Level
	OLED_write_data(od, d);				//   Default => 0x17 (0.50*VCC)
}

//-----------------------------------------------------------------------------
void OLED_set_vcomh(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xBE);			// Set COM Deselect Voltage Level
	OLED_write_data(od, d);				//   Default => 0x04 (0.80*VCC)
}

//-----------------------------------------------------------------------------
void OLED_set_contrast_current(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xC1);			// Set Contrast Current
	OLED_write_data(od, d);				//   Default => 0x7F
}

//-----------------------------------------------------------------------------
void OLED_set_master_current(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xC7);			// Master Contrast Current Control
	OLED_write_data(od, d);				//   Default => 0x0f (Maximum)
}

//-----------------------------------------------------------------------------
void OLED_set_multiplex_ratio(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xCA);			// Set Multiplex Ratio
	OLED_write_data(od, d);				//   Default => 0x7F (1/128 Duty)
}

//-----------------------------------------------------------------------------
void OLED_set_display_enhancement_b(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xD1);			// Display Enhancement
	OLED_write_data(od, 0x82 | d);			//   Default => 0xA2
	//     0x82 (0x00) => Reserved
	//     0xA2 (0x20) => Normal
	OLED_write_data(od, 0x20);
}

//-----------------------------------------------------------------------------
void OLED_set_command_lock(OLEDDriver * od, unsigned char d)
{
	OLED_write_command(od, 0xFD);			// Set Command Lock
	OLED_write_data(od, 0x12 | d);			//   Default => 0x12
	//     0x12 => Driver IC interface is unlocked from entering command.
	//     0x16 => All Commands are locked except 0xFD.
}

//-----------------------------------------------------------------------------
void OLED_set_linear_gray_scale_table(OLEDDriver * od)
{
	OLED_write_command(od, 0xB9);		// Set Default Linear Gray Scale Table
}

//-----------------------------------------------------------------------------
void OLED_fill_ram(OLEDDriver * od, unsigned char Data)
{
	unsigned char i, j;

	//Set_Column_Address(0x00,0x77);
	//Set_Row_Address(0x00,0x7F);
	OLED_set_column_address(od, 0x00, 0x77);
	OLED_set_row_address(od, 0x00, 0x40);

	OLED_set_write_ram(od);

	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < 120; j++)
		{
			OLED_write_data(od, Data);
			OLED_write_data(od, Data);
		}
	}
}

//-----------------------------------------------------------------------------
void OLED_fill_block(OLEDDriver * od, unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
	unsigned char i, j;

	OLED_set_column_address(od, OLED_PIXEL_SHIFT + a, OLED_PIXEL_SHIFT + b);
	OLED_set_row_address(od, c, d);
	OLED_set_write_ram(od);

	for (i = 0; i < (d - c + 1); i++)
	{
		for (j = 0; j < (b - a + 1); j++)
		{
			OLED_write_data(od, Data);
			OLED_write_data(od, Data);
		}
	}
}

//-----------------------------------------------------------------------------
void OLED_clear_screen(OLEDDriver * od)
{
	OLED_fill_ram(od, 0x00);
}

//-----------------------------------------------------------------------------
void OLED_grayscale(OLEDDriver * od)
{
	// Level 16 => Column 1~16
	OLED_fill_block(od, 0xFF, 0x00, 0x03, 0x00, OLED_MAX_ROW);

	// Level 15 => Column 17~32
	OLED_fill_block(od, 0xEE, 0x04, 0x07, 0x00, OLED_MAX_ROW);

	// Level 14 => Column 33~48
	OLED_fill_block(od, 0xDD, 0x08, 0x0B, 0x00, OLED_MAX_ROW);

	// Level 13 => Column 49~64
	OLED_fill_block(od, 0xCC, 0x0C, 0x0F, 0x00, OLED_MAX_ROW);

	// Level 12 => Column 65~80
	OLED_fill_block(od, 0xBB, 0x10, 0x13, 0x00, OLED_MAX_ROW);

	// Level 11 => Column 81~96
	OLED_fill_block(od, 0xAA, 0x14, 0x17, 0x00, OLED_MAX_ROW);

	// Level 10 => Column 97~112
	OLED_fill_block(od, 0x99, 0x18, 0x1B, 0x00, OLED_MAX_ROW);

	// Level 9 => Column 113~128
	OLED_fill_block(od, 0x88, 0x1C, 0x1F, 0x00, OLED_MAX_ROW);

	// Level 8 => Column 129~144
	OLED_fill_block(od, 0x77, 0x20, 0x23, 0x00, OLED_MAX_ROW);

	// Level 7 => Column 145~160
	OLED_fill_block(od, 0x66, 0x24, 0x27, 0x00, OLED_MAX_ROW);

	// Level 6 => Column 161~176
	OLED_fill_block(od, 0x55, 0x28, 0x2B, 0x00, OLED_MAX_ROW);

	// Level 5 => Column 177~192
	OLED_fill_block(od, 0x44, 0x2C, 0x2F, 0x00, OLED_MAX_ROW);

	// Level 4 => Column 193~208
	OLED_fill_block(od, 0x33, 0x30, 0x33, 0x00, OLED_MAX_ROW);

	// Level 3 => Column 209~224
	OLED_fill_block(od, 0x22, 0x34, 0x37, 0x00, OLED_MAX_ROW);

	// Level 2 => Column 225~240
	OLED_fill_block(od, 0x11, 0x38, 0x3B, 0x00, OLED_MAX_ROW);

	// Level 1 => Column 241~256
	OLED_fill_block(od, 0x00, 0x3C, OLED_MAX_COLUMN, 0x00, OLED_MAX_ROW);
}
