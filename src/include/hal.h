/**
 ******************************************************************************
 * @file    win_wrapper.h
 * @author  SolarEdge Infrastructure Team.
 * @brief   WIN32 Access Layer Header File.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 SolarEdge.
 * All rights reserved.</center></h2>
 *
 *
 ******************************************************************************
 */

#ifndef WIN_WRAPPER_H
#define WIN_WRAPPER_H

/* Common C std includes */
#include <Windows.h>
#include <assert.h>
#include <conio.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/** @addtogroup HAL
 * @{
 */

#define HAL_VAL_IN_RANGE(val, low, high) ((val - low) <= (high - low)) /*!< Checks that x falls within low and high */
#define HAL_MIN(a, b)                    (((a) < (b)) ? (a) : (b))
#define HAL_MAX(a, b)                    (((a) > (b)) ? (a) : (b))
#define HAL_SET_BIT(REG, BIT)            (REG |= BIT) /*!< Bits manipulations */

typedef struct __HAL_TimeTypeDef
{
    uint8_t  days;
    uint8_t  hours;
    uint8_t  minutes;
    uint8_t  seconds;
    uint32_t msecs;

} HAL_TimeTypeDef;

typedef enum
{
    Color_White,
    Color_Red,
    Color_Green,
    Color_Blue,
    Color_Yellow,

} HAL_TermColor;

/* Win32 support  API*/

void     HAL_InitTicks(void);
uint32_t HAL_GetTick(void);
void     HAL_TicksToTime(HAL_TimeTypeDef *time, uint32_t ms);
void     HAL_Delay(uint16_t ticks);
void     HAL_Pause(char *str, char expected);
void *   HAL_GetStackPointer(void);
int      HAL_getch(void);
void     HAL_Enable_Colors(void);

int printf_c(HAL_TermColor color, const char *format, ...);

/**
 * @}
 */

#endif /* WIN_WRAPPER_H */

/************************ (C) COPYRIGHT SolarEdge *****END OF FILE****/
