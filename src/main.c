/*==================================================================================================
* Project : RTD AUTOSAR 4.9
* Platform : CORTEXM
* Peripheral : S32K3XX
* Dependencies : none
*
* Autosar Version : 4.9.0
* Autosar Revision : ASR_REL_4_7_REV_0000
* Autosar Conf.Variant :
* SW Version : 7.0.1
* Build Version : S32K3_RTD_7_0_1_D2602_ASR_REL_4_9_REV_0000_20260206
*
* Copyright 2020 - 2026 NXP
*
*   NXP Proprietary. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms.  By expressly
*   accepting such terms or by downloading, installing, activating and/or otherwise
*   using the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms.  If you do not agree to be
*   bound by the applicable license terms, then you may not retain, install,
*   activate or otherwise use the software.
==================================================================================================*/

/*==================================================================================================
*   @file    main.c
*   @brief   Reed Switch Low-Power Wake-Up Demo with OLED Display
*   @details Initialises all peripherals (clocks, GPIO, WKPU, FlexIO I2C, SSD1306 OLED)
*            and runs the main loop that monitors a reed switch (WKPU18 / PTB17),
*            displays the current state on an SSD1306 OLED (via FlexIO I2C),
*            and allows the MCU to enter/exit standby mode via SW2.
*
*   @board   FRDM-A-S32K312
*   @mcu     S32K312 (Cortex-M7)
==================================================================================================*/

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "Wkpu_Ip.h"           /*!< WKPU    Driver                          */
#include "Clock_Ip.h"          /*!< CLOCK   Driver                          */
#include "Power_Ip.h"          /*!< POWER   Driver                          */
#include "Siul2_Port_Ip.h"     /*!< PORT    Driver                          */
#include "Siul2_Dio_Ip.h"      /*!< DIO     Driver                          */
#include "Flexio_Mcl_Ip.h"     /*!< FlexIO  MCL Driver                      */
#include "Flexio_I2c_Ip.h"     /*!< FlexIO  I2C Driver                      */
#include "OsIf.h"              /*!< OsIf    Abstraction Layer               */
#include <stdbool.h>

#include "ssd1306.h"           /*!< SSD1306 OLED Driver                     */

/*==================================================================================================
*                                     DEFINES
==================================================================================================*/

/* Wake up include normal and fast
 * 1: enable fast wake up
 * 0: enable normal wake up */
#define WKPU_USE_FAST           1U

/*!< WKPU instance used - 0  */
#define WKPU_INST               0U

/*!< Simple software delay count for reed status polling interval */
#define REED_REPORT_DELAY       500000U

/*!< Index of the WKPU18 channel entry in Wkpu_Ip_ChannelConfig_PB[].
 *   IcuWkpuChannels_1 in the .mex maps hw channel 18 (PTB17 / REED_IN)
 *   and is stored at index 0 of the generated channel config array.    */
#define WKPU18_CHANNEL_IDX      0U

/*!< Hardware WKPU channel number for the reed switch (PTB17) */
#define WKPU18_HW_CHANNEL       18U

/*!< FlexIO instance used for the SSD1306 OLED I2C bus */
#define OLED_FLEXIO_INSTANCE    0U

/*!< FlexIO I2C channel (master channel index) for the OLED */
#define OLED_FLEXIO_CHANNEL     0U

/*==================================================================================================
*                                     EXTERN DECLARATIONS
==================================================================================================*/
extern void undefined_handler(void);
static void Wkpu_FastWkpuBootAddress(void);

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/* Standby ram end address */
static const uint32 Standby_Stack_StartAddr = 0x20408000;

/* FastStandby Boot VectorTable should be placed in flash to avoid miss after entering standby */
/* Power FastStandby Boot Base Address should be input the address of FastWkpuBootVectorTable */
__attribute__ ((section(".fast_wkpu_boot_vector"), aligned(1024))) uintptr_t const FastWkpuBootVectorTable[] =
{
    (const uintptr_t)Standby_Stack_StartAddr,       /* Set SP after fast wake up  */
    (const uintptr_t)&Wkpu_FastWkpuBootAddress,     /* Reset_Handler after fast wake up */
    (const uintptr_t)&undefined_handler,            /* undefined_handler after fast wake up */
};

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Blocking millisecond delay using OsIf.
 * @param[in] ms  Number of milliseconds to wait.
 */
static void App_DelayMs(uint32_t ms)
{
    uint32_t cur     = OsIf_GetCounter(OSIF_COUNTER_SYSTEM);
    uint32_t elapsed = 0U;
    uint32_t timeout = OsIf_MicrosToTicks(ms * 1000U, OSIF_COUNTER_SYSTEM);

    while (elapsed < timeout)
    {
        elapsed += OsIf_GetElapsed(&cur, OSIF_COUNTER_SYSTEM);
    }
}

/**
* @brief     This function to show MCU what need to do after fast wake up.
* @details   First, you can modify FIRC clock (FIRC default is 24 MHz).
*            However, you can add your code at here. MCU need to jump to Reset_Handler
*            to initialize the MCU.
* @param[in] None
* @return    void
**/
static void Wkpu_FastWkpuBootAddress(void)
{
    /* FIRC is 24M after wakeup. Change FIRC to 48M by bare metal code. */
    if (((IP_CONFIGURATION_GPR->CONFIG_REG_GPR & CONFIGURATION_GPR_CONFIG_REG_GPR_APP_CORE_ACC_MASK)
         >> CONFIGURATION_GPR_CONFIG_REG_GPR_APP_CORE_ACC_SHIFT) == 5u)
    {
        /* FIRC Divider to 1 */
        IP_CONFIGURATION_GPR->CONFIG_REG_GPR = CONFIGURATION_GPR_CONFIG_REG_GPR_APP_CORE_ACC(5)
                                             | CONFIGURATION_GPR_CONFIG_REG_GPR_FIRC_DIV_SEL(3);
    }

    /* Write your code here */

    /* Jump to Reset_Handler to initialize the MCU. */
    __asm("bl Reset_Handler");
}

/**
* @brief     Configure the WKPU module to wake up from WKPU18 (PTB17 / REED_IN).
* @details   Initialises the WKPU IP and enables the interrupt/wakeup for
*            hw channel 18, which is stored at index WKPU18_CHANNEL_IDX in
*            the generated Wkpu_Ip_ChannelConfig_PB[] array.
* @param[in] None
* @return    void
**/
static void Wkpu_Config(void)
{
    /* Initialise the WKPU IP block (clears all previous channel settings) */
    Wkpu_Ip_Init(WKPU_INST, &Wkpu_Ip_Config_PB);

    /* Enable wakeup/interrupt on WKPU18 (PTB17 / REED_IN, channel index 0) */
    Wkpu_Ip_EnableInterrupt(WKPU_INST, Wkpu_Ip_ChannelConfig_PB[WKPU18_CHANNEL_IDX].hwChannel);
}

/**
* @brief     Set clock and other modules configuration before entering standby.
* @details   This function sets system to target wakeup configuration; It sets the
*            clock, wakeup, and other modules registers for wakeup requirement change.
* @param[in] None
* @return    void
**/
static void Wkpu_EnterStandby(void)
{
    /* Switch to FIRC clock */
    Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);

    /* Configure Wkpu module to wake up on WKPU18 */
    Wkpu_Config();

#if (1 == WKPU_USE_FAST)
    /* Enable STANDBY IO configurations and Enter Standby Mode.
     * Fast exit from standby after generating wake up event */
    Power_Ip_SetMode(&Power_Ip_aModeConfigPB[2]);
#else
    /* Enable STANDBY IO configurations and Enter Standby Mode.
     * Normal exit from standby after generating wake up event */
    Power_Ip_SetMode(&Power_Ip_aModeConfigPB[1]);
#endif
}

/*==================================================================================================
*                                   OLED DISPLAY HELPERS
==================================================================================================*/

/**
 * @brief   Initialise the FlexIO I2C peripheral and the SSD1306 OLED display.
 * @details Must be called after Clock_Ip_Init() and Siul2_Port_Ip_Init() so that
 *          the FlexIO clock and the SDA/SCL pins are already configured.
 * @return  void
 */
static void OLED_PeripheralInit(void)
{
    Flexio_Mcl_Ip_Init(Flexio_Ip_paxBase[0]);
    /* Initialise the FlexIO device (timer/shifter resource allocation) */
    Flexio_Mcl_Ip_InitDevice(&Flexio_Ip_Sa_xFlexioInit);

    /* Initialise the FlexIO I2C master channel that drives the OLED */
    Flexio_I2c_Ip_MasterInit(OLED_FLEXIO_INSTANCE, OLED_FLEXIO_CHANNEL,
                             &Flexio_I2cMasterChannel0);

    /* Initialise the SSD1306 controller */
    SSD1306_Init();
}

/**
 * @brief   Render the reed switch status on the OLED display.
 * @details Clears the display buffer and draws a two-line status screen:
 *            Line 0: "Reed Monitor"   (header)
 *            Line 1: "CLOSED" or "OPEN" depending on @p closed
 *            Line 2: "Standby: ARMED" or "Standby: READY"
 *
 * @param[in] closed   true  → magnet is near (door closed / secure)
 *                     false → magnet removed  (door open  / alarm)
 * @param[in] standby  true  → MCU is about to enter / has just left standby
 *                     false → normal run mode
 */
static void OLED_UpdateStatus(bool closed, bool standby)
{
    SSD1306_ClearBuffer();

    /* ── Header ─────────────────────────────────────────────────────── */
    SSD1306_DrawString(4, 0, "Reed Monitor", 1, true);
    SSD1306_DrawLine(0, 9, 95, 9, true);

    /* ── Reed state ──────────────────────────────────────────────────── */
    SSD1306_DrawString(1, 12, "State:", 1, true);
    if (closed)
    {
        SSD1306_DrawString(40, 12, "CLOSED", 1, true);
    }
    else
    {
        SSD1306_DrawString(40, 12, "OPEN  ", 1, true);
    }

    /* ── Standby status ──────────────────────────────────────────────── */
    if (standby)
    {
        SSD1306_DrawString(1, 24, "Standby: ARMED ", 1, true);
    }
    else
    {
        SSD1306_DrawString(1, 24, "Standby: READY ", 1, true);
    }

    SSD1306_UpdateScreen();
}

/**
 * @brief   Show a full-screen standby notification on the OLED then blank it.
 * @details Displays a "Entering Standby" message for ~1 s so the user can
 *          read it before the display is cleared to save power.
 */
static void OLED_ShowStandbyScreen(void)
{
    SSD1306_ClearBuffer();
    SSD1306_DrawString(4,  0,  "Reed Monitor",    1, true);
    SSD1306_DrawLine(0, 9, 95, 9, true);
    SSD1306_DrawString(1,  14, "Entering",        1, true);
    SSD1306_DrawString(1,  24, "Standby...",      1, true);
    SSD1306_UpdateScreen();

    App_DelayMs(1000U);

    /* Blank the display to minimise power during standby */
    SSD1306_ClearBuffer();
    SSD1306_UpdateScreen();
}

/**
 * @brief   Show a wakeup notification on the OLED.
 * @details Displayed immediately after the MCU wakes from standby so the
 *          user gets visual confirmation before the normal status screen
 *          is restored.
 */
static void OLED_ShowWakeupScreen(void)
{
    SSD1306_ClearBuffer();
    SSD1306_DrawString(4,  0,  "Reed Monitor",    1, true);
    SSD1306_DrawLine(0, 9, 95, 9, true);
    SSD1306_DrawString(1,  14, "Woke up!",        1, true);
    SSD1306_DrawString(1,  24, "WKPU18 event",    1, true);
    SSD1306_UpdateScreen();

    App_DelayMs(3000U);
}

/*==================================================================================================
*                                   REED STATUS REPORTING
==================================================================================================*/

/**
* @brief     Report the reed switch status by polling the WKPU18 input state.
* @details   PTB17 is a WKPU-only pin (not GPIO), so its level cannot be read
*            via Siul2_Dio_Ip_ReadPin(). Instead Wkpu_Ip_GetInputState() is
*            used to directly sample the current logic level on the pin:
*              - TRUE  (1) → pin is HIGH → magnet IS close  (CLOSED)
*              - FALSE (0) → pin is LOW  → magnet NOT close (OPEN)
*            A software delay counter throttles the polling rate and a
*            last-state variable ensures the OLED is only refreshed on change.
* @param[in] None
* @return    void
**/
static void ReportReedStatus(void)
{
    static uint8_t  lastReedState  = 0xFFU; /* 0xFF = invalid sentinel (first run) */
    static uint32_t delayCounter   = 0U;

    /* Only sample at the configured interval */
    delayCounter++;
    if (delayCounter < REED_REPORT_DELAY)
    {
        return;
    }
    delayCounter = 0U;

    /* Read the current logic level on PTB17 via the WKPU input state register.
     * Wkpu_Ip_GetInputState() returns TRUE when the pin is HIGH (magnet close). */
    uint8_t currentState = (Wkpu_Ip_GetInputState(WKPU_INST, WKPU18_HW_CHANNEL) == TRUE) ? 1U : 0U;

    if (currentState != lastReedState)
    {
        lastReedState = currentState;

        if (currentState == 1U)
        {
            OLED_UpdateStatus(true,  false);
        }
        else
        {
            OLED_UpdateStatus(false, false);
        }
    }
}

/*==================================================================================================
*                                          MAIN
==================================================================================================*/

int main(void)
{
    Clock_Ip_StatusType clockStatus = CLOCK_IP_ERROR;

    /* ── Clock & Power init ─────────────────────────────────────────── */
    while ((clockStatus = Clock_Ip_Init(&Clock_Ip_aClockConfig[0])) != CLOCK_IP_SUCCESS);

    /* Init MCU MC_RGM part of the registers, Init Power Management Controller
     * and Disable the padkeeping */
    Power_Ip_Init(&Power_Ip_HwIPsConfigPB);

    /* Read the reset reason to detect a standby wakeup.
     * On S32K312, fast wakeup from standby causes a functional reset so
     * execution always restarts from the top of main(). This call also
     * clears RDSS to prevent automatic re-entry into standby. */
    Power_Ip_ResetType resetReason = Power_Ip_GetResetReason();

    /* Set power to run mode (enable last mile regulator and other PMC config) */
    Power_Ip_SetMode(&Power_Ip_aModeConfigPB[0]);

    /* ── OsIf (needed by App_DelayMs) ──────────────────────────────── */
    OsIf_Init(NULL_PTR);

    /* ── Pin configuration ──────────────────────────────────────────── */
    /* Configure all pins: WKPU18/PTB17, LED, SW2, FlexIO SDA/SCL */
    Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS_PortContainer_0_BOARD_InitPeripherals,
                       g_pin_mux_InitConfigArr_PortContainer_0_BOARD_InitPeripherals);

    /* ── WKPU init (needed for reed polling before standby) ─────────── */
    Wkpu_Ip_Init(WKPU_INST, &Wkpu_Ip_Config_PB);
    Wkpu_Ip_EnableInterrupt(WKPU_INST, Wkpu_Ip_ChannelConfig_PB[WKPU18_CHANNEL_IDX].hwChannel);

    /* ── OLED init (FlexIO I2C + SSD1306) ──────────────────────────── */
    OLED_PeripheralInit();

    /* ── LED on ─────────────────────────────────────────────────────── */
    Siul2_Dio_Ip_WritePin(LED_GRN_PORT, LED_GRN_PIN, 1U);

    /* ── Startup / wakeup banner ────────────────────────────────────── */
    if (MCU_WAKEUP_REASON == resetReason)
    {
        /* This boot is a wakeup from standby - show wakeup screen on OLED */
        OLED_ShowWakeupScreen();
    }

    /* Show initial reed state on OLED */
    {
        bool initClosed = (Wkpu_Ip_GetInputState(WKPU_INST, WKPU18_HW_CHANNEL) == TRUE);
        OLED_UpdateStatus(initClosed, false);
    }

    /* ── Main loop ──────────────────────────────────────────────────── */
    for (;;)
    {
        /* Continuously poll and report reed switch status via WKPU18 input state */
        ReportReedStatus();

        /* Wait for the button SW2 to be pressed */
        if (Siul2_Dio_Ip_ReadPin(USR_SW0_PORT, USR_SW0_PIN) == 1)
        {
            /* Wait for SW2 to be released */
            while (Siul2_Dio_Ip_ReadPin(USR_SW0_PORT, USR_SW0_PIN) == 1) {}

            /* Update OLED to show standby armed, then blank it */
            OLED_UpdateStatus(
                (Wkpu_Ip_GetInputState(WKPU_INST, WKPU18_HW_CHANNEL) == TRUE),
                true);
            OLED_ShowStandbyScreen();

            /* Turn off LED */
            Siul2_Dio_Ip_WritePin(LED_GRN_PORT, LED_GRN_PIN, 0U);

            /* Enter Standby Mode - wakeup armed on WKPU18 */
            Wkpu_EnterStandby();

            /* Turn LED back ON to indicate wake-up */
            Siul2_Dio_Ip_WritePin(LED_GRN_PORT, LED_GRN_PIN, 1U);

            /* Re-initialise FlexIO I2C and OLED after wakeup (normal mode only) */
            OLED_PeripheralInit();
            OLED_ShowWakeupScreen();

            /* Restore current reed state on OLED */
            {
                bool nowClosed = (Wkpu_Ip_GetInputState(WKPU_INST, WKPU18_HW_CHANNEL) == TRUE);
                OLED_UpdateStatus(nowClosed, false);
            }
        }
    }

    return 0;
}

/** @} */
