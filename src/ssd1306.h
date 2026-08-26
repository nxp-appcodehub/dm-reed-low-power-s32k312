/*==================================================================================================
 * Project : RTD AUTOSAR 4.9
 * Platform : CORTEXM
 * Peripheral : S32K3XX
 * Dependencies : none
 *
 * Autosar Version : 4.9.0
 * Autosar Revision : ASR_REL_4_9_REV_0000
 * Autosar Conf.Variant :
 * SW Version : 7.0.1
 * Build Version : S32K3_RTD_7_0_1_D2602_ASR_REL_4_9_REV_0000_20260206
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

#ifndef SSD1306_H
#define SSD1306_H

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "Flexio_I2c_Ip.h"

/*==================================================================================================
 *                                         DEFINES
 *==================================================================================================*/

/** @brief FlexIO peripheral instance used for OLED communication  */
#define OLED_FLEXIO_INSTANCE    0U

/** @brief FlexIO I2C master channel assigned to the OLED           */
#define OLED_FLEXIO_CHANNEL     0U

/** @brief 7-bit I2C address of the SSD1306 controller             */
#define OLED_ADDR               0x3C

/** @brief Blocking I2C transfer timeout (arbitrary loop count)     */
#define OLED_I2C_TIMEOUT        100000U

/** @brief Visible display width in pixels                          */
#define SCREEN_WIDTH            96

/** @brief Visible display height in pixels                         */
#define SCREEN_HEIGHT           39

/** @brief Physical column offset — shifts viewport on 128-col panel */
#define OLED_X_OFFSET           32

/** @brief Number of 8-pixel pages covering SCREEN_HEIGHT rows      */
#define OLED_PAGES              5

/*==================================================================================================
 *                                  FUNCTION PROTOTYPES
 *==================================================================================================*/

/**
 * @brief   Initialise the SSD1306 display controller.
 */
void SSD1306_Init(void);

/**
 * @brief   Flush the local frame buffer to the SSD1306 over FlexIO I2C.
 */
void SSD1306_UpdateScreen(void);

/**
 * @brief   Clear the entire local frame buffer (all pixels off).
 */
void SSD1306_ClearBuffer(void);

/**
 * @brief   Set or clear a single pixel in the frame buffer.
 */
void SSD1306_DrawPixel(int x, int y, bool color);

/**
 * @brief   Draw a straight line between two points using Bresenham's algorithm.
 */
void SSD1306_DrawLine(int x0, int y0, int x1, int y1, bool color);

/**
 * @brief   Fill a rectangular region in the frame buffer with a solid colour.
 */
void SSD1306_FillRect(int x, int y, int w, int h, bool color);

/**
 * @brief   Shift a rectangular region of the frame buffer one pixel left.
 */
void SSD1306_ScrollRegionLeft(uint8_t start_x, uint8_t end_x,
		uint8_t start_page, uint8_t end_page);

/**
 * @brief   Render a single ASCII character using the built-in 5×8 bitmap font.
 */
void SSD1306_DrawChar(int x, int y, char c, uint8_t size, bool color);

/**
 * @brief   Render a null-terminated ASCII string using the built-in 5×8 font.
 */
void SSD1306_DrawString(int x, int y, const char *str, uint8_t size, bool color);

/**
 * @brief   Set the SSD1306 display contrast level.
 * @details Sends the 0x81 contrast command followed by the value byte.
 *          Valid range is 0x00 (dimmest) to 0xFF (brightest).
 *
 * @param[in] contrast  Contrast value (0x00–0xFF).
 */
void SSD1306_SetContrast(uint8_t contrast);

/**
 * @brief   Enable or disable display-inversion mode.
 * @details Sends 0xA7 (inverse: lit pixel = 0 in GDDRAM) or
 *          0xA6 (normal: lit pixel = 1 in GDDRAM) to the controller.
 *          No frame-buffer modification is needed — the SSD1306 handles
 *          the inversion in hardware.
 *
 * @param[in] enable  true = inverted display, false = normal display.
 */
void SSD1306_SetInverse(bool enable);

/**
 * @brief   Draw a progress bar outline and filled region.
 * @details Draws a 1-pixel border rectangle at (x, y, w, h), then fills
 *          the interior proportionally according to @p progress.
 *          The inner fill starts at (x+2, y+2) leaving a 2-pixel border
 *          on every side, matching the original oledBclick_drawProgressBar style.
 *
 * @param[in] x        Left edge of the bar.
 * @param[in] y        Top edge of the bar.
 * @param[in] w        Total width of the bar in pixels.
 * @param[in] h        Total height of the bar in pixels.
 * @param[in] progress Fill percentage (0–100).
 */
void SSD1306_DrawProgressBar(int x, int y, int w, int h, uint8_t progress);

/**
 * @brief   Play the startup animation sequence on the SSD1306.
 * @details Exact port of oledBclick_startupAnimation() from the
 *          FRDM_A_S32K344_OLED_DISPLAY_FLEXIO_I2C reference project.
 *
 *          Sequence:
 *            1. Three full-screen white/black blink cycles.
 *            2. White fill with three contrast fade steps, then restore.
 *            3. "BOOTING..." text with four inverse-video flashes.
 *            4. "INITIALIZING" text with a progress bar from 0 → 100 %.
 *            5. Column-wipe transition (4-pixel strips cleared left to right).
 *            6. "READY!" message for 500 ms, then clear.
 *
 *          Must be called after SSD1306_Init() and before the main loop.
 */
void SSD1306_StartupAnimation(void);

#endif /* SSD1306_H */
