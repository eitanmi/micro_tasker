/**
  ******************************************************************************
  * @file    main.c
  * @brief   Sample file that demonstrates the scheduler.
  *
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 SolarEdge.
  * All rights reserved.</center></h2>
  *
  *
  ******************************************************************************
  */

#include "hal.h"
#include "scheduler.h"

/* Global task handles */
TaskHandle_t htsk_moshe, htsk_aviv, htsk_eli;

/**************************************************************************/ /**
 *                                                                           
 * @brief
 *  Task Moshe: Pause for 2 seconds and notifies task C.
 * @return
 *   nothing.
 *
 *****************************************************************************/

void tsk_moshe(void *args)
{
    int val = 0;

    while ( 1 )
    {
        printf_c(Color_Green, "Moshe Loop started..");

        vTaskDelay(2000);

        printf_c(Color_Green, "Moshe Loop ended");

        xTaskNotify(htsk_eli, val++);
    }
}

/**************************************************************************/ /**
 * @brief
 *  Task Aviv: count from 0 to ... x
 * @return
 *   nothing.
 *
 *****************************************************************************/

void tsk_aviv(void *args)
{

    volatile uint32_t y      = 0;
    const uint32_t    target = 0xffffff;

    while ( 1 )
    {
        y = 0;
        printf_c(Color_Red, "Aviv Counting from 0 to %d", target);

        while ( y++ != target )
        {
            taskYIELD();
        }

        printf_c(Color_Red, "Aviv done counting, taking 5 seconds break..");
        vTaskDelay(5000);
    }
}

/**************************************************************************/ /**
 * @brief
 *  Task Eli: waits for any event.
 * @return
 *   nothing.
 *
 *****************************************************************************/

void tsk_eli(void *args)
{
    uint32_t event;

    while ( 1 )
    {
        printf_c(Color_Blue, "Eli Waiting for event");

        event = xTaskNotifyWait(HAL_XTASK_MAX_TIME);

        printf_c(Color_Blue, "Eli Got event %d, thinking about that for a while..", event);
        vTaskDelay(2000);
    }
}

/**************************************************************************/ /**
 * @brief
 *  Everything must have a beginning
 * @return
 *   exit code.
 *
 *****************************************************************************/

int main(int argc, char *argv[])
{
    HAL_Enable_Colors();
    SetConsoleTitle("Scheduler");

    HAL_InitTicks();

    /* Create few tasks */
    htsk_moshe = xTaskCreate("TSK_MOSHE", tsk_moshe, 0x3000, NULL);
    htsk_aviv  = xTaskCreate("TSK_AVIV", tsk_aviv, 0x3000, NULL);
    htsk_eli   = xTaskCreate("TSK_ELI", tsk_eli, 0x3000, NULL);

    /* Start the scheduler infinite loop */
    vTaskStartScheduler();

    return 0;
}
