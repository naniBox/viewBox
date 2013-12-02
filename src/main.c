/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // dmo@nanibox.com // naniBox.com

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

#include <string.h>
#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <memstreams.h>
#include <time.h>
#include <math.h>
#include <chrtclib.h>
#include "oled.h"

//-----------------------------------------------------------------------------
#include "kbb_types.h"
KBB_TYPES_VERSION_CHECK(0x0001)

//-----------------------------------------------------------------------------
static WORKING_AREA(waBlinker, 128);
static msg_t 
thBlinker(void *arg) 
{
	(void)arg;
	chRegSetThreadName("Blinker");
	while( !chThdShouldTerminate() )
	{
		palTogglePad(GPIOA, GPIOA_LED4);
		chThdSleepMilliseconds(250);
	}
	return 0;
}

//-----------------------------------------------------------------------------
#define INIT_CBUF() \
	memset(charbuf,0,sizeof(charbuf));\
	msObjectInit(&msb, (uint8_t*)charbuf, sizeof(charbuf), 0);
#define bss ((BaseSequentialStream *)&msb)
static char charbuf[128];
static MemoryStream msb;

//-----------------------------------------------------------------------------
static SerialConfig serial1_default_cfg = {
	57600,
	0,
	USART_CR2_STOP1_BITS | USART_CR2_LINEN,
	0
};

//-----------------------------------------------------------------------------
static void frame_to_time(smpte_timecode_t * smpte_timecode, ltc_frame_t * ltc_frame)
{
	smpte_timecode->hours = ltc_frame->hours_units + ltc_frame->hours_tens*10;
	smpte_timecode->minutes  = ltc_frame->minutes_units  + ltc_frame->minutes_tens*10;
	smpte_timecode->seconds  = ltc_frame->seconds_units  + ltc_frame->seconds_tens*10;
	smpte_timecode->frames = ltc_frame->frame_units + ltc_frame->frame_tens*10;
}

//-----------------------------------------------------------------------------
// Implemented from
// http://www.nalresearch.com/files/Standard%20Modems/A3LA-XG/A3LA-XG%20SW%20Version%201.0.0/GPS%20Technical%20Documents/GPS.G1-X-00006%20(Datum%20Transformations).pdf
// input is in SI (meters)
#define A		6378137.000f
#define B		6356752.31424518f
#define EE		0.0066943799901411239f
#define E1E1	0.0067394967422762389f
static void ecef2lla(int32_t xx, int32_t yy, int32_t zz, float * olat, float * olon, float * oalt)
{
	float x = xx/100.0f;
	float y = yy/100.0f;
	float z = zz/100.0f;

	// const float F = 1.0/298.257223563f;
	// const float A = 6378137.000f;
	// const float B = 6356752.31424518f;			// A*(1.0-F)

	// const float E = 0.081819190842620321f;	// sqrt((A*A-B*B)/(A*A));
	// const float EE = 0.0066943799901411239f;	// E*E
	// const float E1 = 0.082094437949694496f;	// sqrt((A*A-B*B)/(B*B));
	// const float E1E1 = 0.0067394967422762389f;	// E1*E1

	float lambda = atan2f(y, x);
	float p = sqrtf(x*x + y*y);
	float theta = atan2f((z*A), (p*B));
	float phi = atan2f(z + E1E1*B*powf(sinf(theta), 3), p - EE*A*powf(cosf(theta), 3));
	float N = A / sqrtf(1.0f - EE*powf(sinf(phi), 2));
	float h = p / cosf(phi) - N;

	*olat = phi*180.0f/(float)M_PI;
	*olon = lambda*180.0f/(float)M_PI;
	*oalt = h;
}

//-----------------------------------------------------------------------------
kbb_display_t kbb_display;
uint8_t kbb_display_buffer[KBB_DISPLAY_SIZE];
uint8_t kbb_display_idx;
volatile uint8_t data_updated;

//-----------------------------------------------------------------------------
static WORKING_AREA(read_wa, 1024);
static msg_t
read_thread(void *arg)
{
	(void)arg;
	chRegSetThreadName("read");
	memset(&kbb_display, 0, sizeof(kbb_display));

	while( !chThdShouldTerminate() )
	{
		palSetPad(GPIOA, GPIOA_LED3);
		uint8_t c = 0;
		// read one byte at a time, this function blocks until there's
		// something to read
		if ( sdRead(&SD1, &c, 1) != 1 )
			// but just in case, if nothing was read, try again
			continue;

		palClearPad(GPIOA, GPIOA_LED3);
		if ( ( kbb_display_idx == 0 && c == ((KBB_PREAMBLE>> 0)&0xff) ) ||
			 ( kbb_display_idx == 1 && c == ((KBB_PREAMBLE>> 8)&0xff) ) ||
			 ( kbb_display_idx == 2 && c == ((KBB_PREAMBLE>>16)&0xff) ) ||
			 ( kbb_display_idx == 3 && c == ((KBB_PREAMBLE>>24)&0xff) ) ||
			 ( ( kbb_display_idx > 3 ) &&
			   ( kbb_display_idx < KBB_DISPLAY_SIZE ) ) )
		{
			kbb_display_buffer[kbb_display_idx++] = c;
		}
		else
		{
			palTogglePad(GPIOA, GPIOA_LED4);
			kbb_display_idx = 0;
			memset(&kbb_display, 0, sizeof(kbb_display));
		}

		if ( kbb_display_idx == KBB_DISPLAY_SIZE )
		{
			kbb_display_t * buf = (kbb_display_t *)kbb_display_buffer;
			if ( buf->header.preamble == KBB_PREAMBLE )
			{
				uint16_t cs = calc_checksum_16(kbb_display_buffer+KBB_CHECKSUM_START, KBB_DISPLAY_SIZE-KBB_CHECKSUM_START);
				if ( cs == buf->header.checksum )
				{
					if ( buf->header.msg_class == KBB_CLASS_EXTERNAL &&
						 buf->header.msg_subclass == KBB_SUBCLASS_EXTERNAL_01 )
					{
						memcpy(&kbb_display, kbb_display_buffer, sizeof(kbb_display));
						data_updated++;
					}
				}
			}
	    	kbb_display_idx = 0;
	    	memset(&kbb_display_buffer, 0, sizeof(kbb_display_buffer));
		}


		/*







		uint8_t c = 0;
		if ( sdRead(&SD1, &c, 1) != 1 )
			// but just in case, if nothing was read, try again
			continue;
		char_buf[char_buf_idx++] = c;
		if ( char_buf_idx >= sizeof(char_buf) || c == 13 || c == 10)
		{
			chSysLock();
			memset(out_buf, 0, 33);
			memcpy(out_buf, char_buf, char_buf_idx);
			out_buf[32] = 0;
			char_buf_idx = 0;
			chSysUnlock();
		}

		*/
	}
	return 0;
}

//-----------------------------------------------------------------------------
int main(void) 
{
	halInit();
	chSysInit();

	sdStart(&SD1, &serial1_default_cfg);
	chprintf(((BaseSequentialStream *)&SD1), "%s\n\r\n\r", BOARD_NAME);

	// enable the button!!
	palSetPad(GPIOC, GPIOC_KILL);

	// just blink to indicate we haven't crashed
	chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, thBlinker, NULL);

	palSetPadMode(GPIOA, GPIOA_SPI1_SCK,  PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);       	// New SCK.
	palSetPadMode(GPIOA, GPIOA_SPI1_MISO, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);       	// New MISO.
	palSetPadMode(GPIOA, GPIOA_SPI1_MOSI, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);       	// New MOSI.
	palSetPadMode(GPIOA, GPIOA_LED3, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_LOWEST);      // LED
	palSetPadMode(GPIOA, GPIOA_LED2, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_LOWEST);      // LED

	// use the default config.
	OLEDStart(&OD1, NULL, &SPID1);
	//OLED_draw_string(&OD1, "KuroBox remote display - viewBox", 0x0f, 0, 0*OLED_LINE_HEIGHT);
	OLED_put_frame_buffer(&OD1);
	chThdSleepMilliseconds(20);

	chThdCreateStatic(read_wa, sizeof(read_wa), NORMALPRIO, read_thread, NULL);

	int frame = 0;
	int last_frame_count = 0;
	int curr_sec = 0;
	int fps = 0;
	struct tm timp;

	while( 1 )
	{
		systime_t sleep_until = chTimeNow() + MS2ST(5);
		if ( data_updated )
		{
			palSetPad(GPIOA, GPIOA_LED2);
			rtcGetTimeTm(&RTCD1, &timp);
			if ( curr_sec != timp.tm_sec )
			{
				curr_sec = timp.tm_sec;
				fps = frame - last_frame_count;
				last_frame_count = frame;
			}

			OLED_clear_frame_buffer(&OD1);

			//------------------------------------------------------------------
			OLED_draw_string(&OD1, "KuroBox remote display - viewBox", 0x0f, 0, 0*OLED_LINE_HEIGHT);

			//------------------------------------------------------------------
			smpte_timecode_t smpte_timecode;
			frame_to_time(&smpte_timecode, &kbb_display.ltc_frame);
			INIT_CBUF();
			chprintf(bss,"LTC: %.2d:%.2d:%.2d.%.2d",
					smpte_timecode.hours,
					smpte_timecode.minutes,
					smpte_timecode.seconds,
					smpte_timecode.frames);
			OLED_draw_string(&OD1, charbuf, 0x0f, 0, 1*OLED_LINE_HEIGHT);

			//------------------------------------------------------------------
			INIT_CBUF();
			chprintf(bss,"YPR: %4i / %4i / %4i",
					(int)kbb_display.vnav.ypr[0],(int)kbb_display.vnav.ypr[1],(int)kbb_display.vnav.ypr[2]);
			OLED_draw_string(&OD1, charbuf, 0x0f, 0, 2*OLED_LINE_HEIGHT);

			//------------------------------------------------------------------
			float lat,lon,alt;
			lat=lon=alt=0.0f;
			ecef2lla(kbb_display.ecef[0], kbb_display.ecef[1], kbb_display.ecef[2], &lat, &lon, &alt);
			INIT_CBUF();
			chprintf(bss,"GPS: %3f / %3f / %3f", lat, lon, alt);
			OLED_draw_string(&OD1, charbuf, 0x0f, 0, 3*OLED_LINE_HEIGHT);

			//------------------------------------------------------------------
			INIT_CBUF();
			chprintf(bss,"TMP: %3.4f", kbb_display.temperature);
			OLED_draw_string(&OD1, charbuf, 0x0f, 0, 4*OLED_LINE_HEIGHT);

			//------------------------------------------------------------------
			INIT_CBUF();
			chprintf(bss,"                   Frame: %6d; FPS: %3d", frame++, fps+1);
			OLED_draw_string(&OD1, charbuf, 0x0f, 0, 7*OLED_LINE_HEIGHT);
			OLED_put_frame_buffer(&OD1);

			data_updated = 0;

			palClearPad(GPIOA, GPIOA_LED2);
		}

		while ( sleep_until < chTimeNow() )
			sleep_until += MS2ST(5);
		chThdSleepUntil(sleep_until);
		sleep_until += MS2ST(5);            // Next deadline
	}
}

