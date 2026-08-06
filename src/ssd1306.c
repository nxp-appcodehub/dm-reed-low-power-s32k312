/*==================================================================================================
 * Project : RTD AUTOSAR 4.9
 * Platform : CORTEXM
 * Peripheral : S32K3XX
 * Dependencies : none
 *
 * Autosar Version : 4.9.0
 * Autosar Revision : ASR_REL_4_9_REV_0000
 * Autosar Conf.Variant :
 * SW Version : 7.0.0
 * Build Version : S32K3_RTD_7_0_0_QLP03_D2512_ASR_REL_4_9_REV_0000_20251210
 *
 * Copyright 2020 - 2026 NXP
 *
 *   NXP Proprietary. This software is owned or controlled by NXP and may only be
 *   used strictly in accordance with the applicable license terms. By expressly
 *   accepting such terms or by downloading, installing, activating and/or otherwise
 *   using the software, you are agreeing that you have read, and that you agree to
 *   comply with and are bound by, such license terms. If you do not agree to be
 *   bound by the applicable license terms, then you may not retain, install,
 *   activate or otherwise use the software.
 ==================================================================================================*/

/*==================================================================================================
 *   @file    ssd1306.c
 *   @brief   SSD1306 OLED Display Driver Implementation
 ==================================================================================================*/

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "ssd1306.h"
#include "ssd1306_font.h"
#include "Siul2_Dio_Ip.h"
#include "Siul2_Port_Ip.h"
#include "OsIf.h"
#include <stdlib.h>

/*==================================================================================================
 *                              MODULE-LEVEL VARIABLES
 *==================================================================================================*/

/** @brief Last I2C transaction status — retained for optional error inspection */
Flexio_I2c_Ip_StatusType status;

/**
 * @brief   In-RAM shadow frame buffer.
 * @details Organised as OLED_PAGES rows × SCREEN_WIDTH columns.
 */
static uint8_t SSD1306_Buffer[OLED_PAGES][SCREEN_WIDTH] = { 0 };

/*==================================================================================================
 *                              PRIVATE HELPER FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Blocking delay using OsIf system counter.
 * @param ms  Number of milliseconds to wait.
 *            Must not exceed 4,294,967 ms to avoid uint32 overflow in ms * 1000U.
 */
static void SSD1306_DelayMs(uint32_t ms) {
	uint32 cur = OsIf_GetCounter(OSIF_COUNTER_SYSTEM);
	uint32 elapsed = 0U;
	uint32 timeout = OsIf_MicrosToTicks(ms * 1000U, OSIF_COUNTER_SYSTEM);

	while (elapsed < timeout) {
		elapsed += OsIf_GetElapsed(&cur, OSIF_COUNTER_SYSTEM);
	}
}

/**
 * @brief   Send a single command byte to the SSD1306 over FlexIO I2C.
 * @param[in] cmd  SSD1306 command byte to transmit.
 */
static void SSD1306_WriteCmd(uint8_t cmd) {
	uint8_t txBuffer[2] = { 0x00, cmd };

	Flexio_I2c_Ip_MasterSetSlaveAddr(OLED_FLEXIO_INSTANCE, OLED_FLEXIO_CHANNEL,
			OLED_ADDR);
	Flexio_I2c_Ip_MasterSendDataBlocking(OLED_FLEXIO_INSTANCE,
			OLED_FLEXIO_CHANNEL, txBuffer, 2, TRUE, OLED_I2C_TIMEOUT);
}

/**
 * @brief   Send a block of data bytes to the SSD1306 GDDRAM over FlexIO I2C.
 * @param[in] data    Pointer to the pixel data array.
 * @param[in] length  Number of data bytes to send (clamped to 128).
 */
static void SSD1306_WriteData(const uint8_t *data, uint32_t length) {
	uint8_t txBuffer[129];

	if (length > 128U)
		length = 128U;

	txBuffer[0] = 0x40;
	for (uint32_t i = 0; i < length; i++) {
		txBuffer[i + 1] = data[i];
	}

	Flexio_I2c_Ip_MasterSetSlaveAddr(OLED_FLEXIO_INSTANCE, OLED_FLEXIO_CHANNEL,
			OLED_ADDR);
	Flexio_I2c_Ip_MasterSendDataBlocking(OLED_FLEXIO_INSTANCE,
			OLED_FLEXIO_CHANNEL, txBuffer, length + 1U, TRUE, OLED_I2C_TIMEOUT);
}

/*==================================================================================================
 *                              PUBLIC FUNCTION DEFINITIONS
 *==================================================================================================*/

void SSD1306_ClearBuffer(void) {
	for (int p = 0; p < OLED_PAGES; p++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			SSD1306_Buffer[p][x] = 0x00;
		}
	}
}

void SSD1306_UpdateScreen(void) {
	for (uint8_t p = 0; p < OLED_PAGES; p++) {
		uint8_t physical_col = OLED_X_OFFSET;

		uint8_t txBuffer[4] = { 0x00,
		(uint8_t) (0xB0 + p),
		(uint8_t) (0x00 + (physical_col & 0x0FU)),
		(uint8_t) (0x10 + ((physical_col >> 4U) & 0x0FU)) };

		Flexio_I2c_Ip_MasterSetSlaveAddr(OLED_FLEXIO_INSTANCE,
				OLED_FLEXIO_CHANNEL, OLED_ADDR);
		Flexio_I2c_Ip_MasterSendDataBlocking(OLED_FLEXIO_INSTANCE,
				OLED_FLEXIO_CHANNEL, txBuffer, 4, TRUE, OLED_I2C_TIMEOUT);

		SSD1306_WriteData(SSD1306_Buffer[p], SCREEN_WIDTH);
	}
}

void SSD1306_ScrollRegionLeft(uint8_t start_x, uint8_t end_x,
		uint8_t start_page, uint8_t end_page) {
	if (start_page >= OLED_PAGES)
		return;
	if (end_page >= OLED_PAGES)
		end_page = OLED_PAGES - 1U;
	if (end_x >= SCREEN_WIDTH)
		end_x = SCREEN_WIDTH - 1U;
	if (start_x >= end_x)
		return;

	for (int p = start_page; p <= end_page; p++) {
		for (int x = start_x; x < end_x; x++) {
			SSD1306_Buffer[p][x] = SSD1306_Buffer[p][x + 1];
		}
		SSD1306_Buffer[p][end_x] = 0x00;
	}
}

void SSD1306_DrawPixel(int x, int y, bool color) {
	if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
		return;

	if (color) {
		SSD1306_Buffer[y / 8][x] |= (uint8_t) (1U << (y % 8));
	} else {
		SSD1306_Buffer[y / 8][x] &= ~(uint8_t) (1U << (y % 8));
	}
}

void SSD1306_DrawLine(int x0, int y0, int x1, int y1, bool color) {
	int dx = abs(x1 - x0);
	int sx = (x0 < x1) ? 1 : -1;
	int dy = -abs(y1 - y0);
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy;
	int e2;

	while (1) {
		SSD1306_DrawPixel(x0, y0, color);

		if (x0 == x1 && y0 == y1)
			break;

		e2 = 2 * err;

		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}

		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void SSD1306_FillRect(int x, int y, int w, int h, bool color) {
	for (int i = x; i < x + w; i++) {
		for (int j = y; j < y + h; j++) {
			SSD1306_DrawPixel(i, j, color);
		}
	}
}

void SSD1306_DrawChar(int x, int y, char c, uint8_t size, bool color) {
	if (c < 32 || c > 126)
		c = 32;

	for (int i = 0; i < 5; i++) {
		uint8_t line = SSD1306_Font5x8[c - 32][i];

		for (int j = 0; j < 8; j++) {
			if (line & 0x01U) {
				if (size == 1) {
					SSD1306_DrawPixel(x + i, y + j, color);
				} else {
					SSD1306_FillRect(x + (i * size), y + (j * size), size,
							size, color);
				}
			}
			line >>= 1;
		}
	}
}

void SSD1306_DrawString(int x, int y, const char *str, uint8_t size,
		bool color) {
	int cursor_x = x;
	int cursor_y = y;

	while (*str) {
		if (*str == '\n') {
			cursor_y += 8 * size;
			cursor_x = x;
		} else {
			SSD1306_DrawChar(cursor_x, cursor_y, *str, size, color);
			cursor_x += 6 * size;
		}
		str++;
	}
}

void SSD1306_Init(void) {
	/* Hardware reset sequence */
	Siul2_Dio_Ip_WritePin(RST_PORT, RST_PIN, 1);
	SSD1306_DelayMs(10U);
	Siul2_Dio_Ip_WritePin(RST_PORT, RST_PIN, 0);
	SSD1306_DelayMs(20U);
	Siul2_Dio_Ip_WritePin(RST_PORT, RST_PIN, 1);
	SSD1306_DelayMs(50U);

	SSD1306_WriteCmd(0xAE); /* Display OFF (sleep mode)                                      */
	SSD1306_WriteCmd(0x20); /* Set Memory Addressing Mode                                    */
	SSD1306_WriteCmd(0x02); /*   → Page Addressing Mode                                      */
	SSD1306_WriteCmd(0xA8); /* Set Multiplex Ratio (next byte sets the ratio)                */
	SSD1306_WriteCmd(0x26); /*   → MUX ratio = 39 (0x26+1), matches 39-row display height   */
	SSD1306_WriteCmd(0xD3); /* Set Display Offset (next byte sets vertical shift)            */
	SSD1306_WriteCmd(0x00); /*   → No vertical shift (0 rows)                               */
	SSD1306_WriteCmd(0x40); /* Set Display Start Line → Line 0 (bits [5:0] = 0)             */
	SSD1306_WriteCmd(0xA1); /* Set Segment Re-map → Column 127 mapped to SEG0 (mirror X)    */
	SSD1306_WriteCmd(0xC8); /* Set COM Output Scan Direction → Remapped, scan from COM[N-1] to COM0 (mirror Y) */
	SSD1306_WriteCmd(0xDA); /* Set COM Pins Hardware Configuration (next byte configures)    */
	SSD1306_WriteCmd(0x12); /*   → Alternative COM pin config, disable COM left/right remap  */
	SSD1306_WriteCmd(0x81); /* Set Contrast Control (next byte sets the contrast level)      */
	SSD1306_WriteCmd(0x7F); /*   → Contrast = 127 (mid-range, 0x00 min … 0xFF max)          */
	SSD1306_WriteCmd(0xA4); /* Entire Display ON → Output follows RAM content (normal mode)  */
	SSD1306_WriteCmd(0xA6); /* Set Normal Display → 1 = lit pixel, 0 = dark pixel           */
	SSD1306_WriteCmd(0xD5); /* Set Display Clock Divide Ratio / Oscillator Frequency         */
	SSD1306_WriteCmd(0x80); /*   → Divide ratio = 1, oscillator frequency = mid (0x8)       */
	SSD1306_WriteCmd(0x8D); /* Charge Pump Setting (next byte enables/disables)              */
	SSD1306_WriteCmd(0x14); /*   → Enable charge pump during display on                      */
	SSD1306_WriteCmd(0xAF); /* Display ON — exit sleep mode and turn on the panel            */

	SSD1306_ClearBuffer();
	SSD1306_UpdateScreen();
}


/* =========================================================
 * New public functions
 * ========================================================= */

void SSD1306_SetContrast(uint8_t contrast) {
	SSD1306_WriteCmd(0x81U);
	SSD1306_WriteCmd(contrast);
}

void SSD1306_SetInverse(bool enable) {
	SSD1306_WriteCmd(enable ? 0xA7U : 0xA6U);
}

void SSD1306_DrawProgressBar(int x, int y, int w, int h, uint8_t progress) {
	int fill;

	if (progress > 100U)
		progress = 100U;

	/* Outline rectangle */
	SSD1306_DrawLine(x,         y,         x + w - 1, y,         true);
	SSD1306_DrawLine(x,         y + h - 1, x + w - 1, y + h - 1, true);
	SSD1306_DrawLine(x,         y,         x,          y + h - 1, true);
	SSD1306_DrawLine(x + w - 1, y,         x + w - 1,  y + h - 1, true);

	/* Proportional fill — 2-pixel inner margin on every side */
	fill = ((w - 4) * (int)progress) / 100;
	if (fill > 0) {
		SSD1306_FillRect(x + 2, y + 2, fill, h - 4, true);
	}
}

/**
 * @brief   Exact port of oledBclick_startupAnimation() adapted for the
 *          SSD1306 driver API (96 × 39 px, same physical dimensions).
 */
void SSD1306_StartupAnimation(void) {
	uint8_t i;
	uint8_t progress;

	/* ── 1. Three white/black blink cycles ── */
	for (i = 0U; i < 3U; i++) {
		SSD1306_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
		SSD1306_UpdateScreen();
		SSD1306_DelayMs(100U);

		SSD1306_ClearBuffer();
		SSD1306_UpdateScreen();
		SSD1306_DelayMs(100U);
	}

	/* ── 2. White fill + contrast fade ── */
	SSD1306_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
	SSD1306_UpdateScreen();

	for (i = 0U; i < 3U; i++) {
		SSD1306_SetContrast(i);
		SSD1306_DelayMs(30U);
	}
	SSD1306_SetContrast(0xCFU);
	SSD1306_DelayMs(200U);

	/* ── 3. "BOOTING..." with inverse flashes ── */
	SSD1306_ClearBuffer();
	SSD1306_DrawString(20, 15, "BOOTING...", 1, true);
	SSD1306_UpdateScreen();

	for (i = 0U; i < 4U; i++) {
		SSD1306_SetInverse(true);
		SSD1306_DelayMs(150U);
		SSD1306_SetInverse(false);
		SSD1306_DelayMs(150U);
	}

	/* ── 4. "INITIALIZING" + progress bar ── */
	SSD1306_ClearBuffer();
	SSD1306_DrawString(15, 5, "INITIALIZING", 1, true);
	SSD1306_UpdateScreen();
	SSD1306_DelayMs(300U);

	for (progress = 0U; progress <= 100U; progress = (uint8_t)(progress + 5U)) {
		/* Erase and redraw the progress bar area */
		SSD1306_FillRect(10, 15, 76, 12, false);
		SSD1306_DrawProgressBar(10, 15, 76, 12, progress);

		/* Erase and redraw the percentage text */
		SSD1306_FillRect(35, 32, 30, 8, false);

		if (progress >= 100U) {
			SSD1306_DrawChar(38, 32, '1', 1, true);
			SSD1306_DrawChar(44, 32, '0', 1, true);
			SSD1306_DrawChar(50, 32, '0', 1, true);
		} else if (progress >= 10U) {
			SSD1306_DrawChar(38, 32, (char)('0' + (progress / 10U)), 1, true);
			SSD1306_DrawChar(44, 32, (char)('0' + (progress % 10U)), 1, true);
		} else {
			SSD1306_DrawChar(38, 32, ' ',                            1, true);
			SSD1306_DrawChar(44, 32, (char)('0' + progress),         1, true);
		}
		SSD1306_DrawChar(56, 32, '%', 1, true);

		SSD1306_UpdateScreen();
		SSD1306_DelayMs(50U);
	}
	SSD1306_DelayMs(300U);

	/* ── 5. Column-wipe transition (4-pixel strips left → right) ── */
	for (i = 0U; i < SCREEN_WIDTH; i = (uint8_t)(i + 4U)) {
		SSD1306_FillRect((int)i, 0, 4, SCREEN_HEIGHT, false);
		SSD1306_UpdateScreen();
		SSD1306_DelayMs(10U);
	}

	/* ── 6. "READY!" for 500 ms then clear ── */
	SSD1306_ClearBuffer();
	SSD1306_DrawString(25, 15, "READY!", 1, true);
	SSD1306_UpdateScreen();
	SSD1306_DelayMs(500U);

	SSD1306_ClearBuffer();
	SSD1306_UpdateScreen();
}
