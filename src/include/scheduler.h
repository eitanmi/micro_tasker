/**
 ******************************************************************************
 * @file    scheduler.h
 * @brief
 *  
 *  Non preemptive simple scheduler.
 *  By “non preemptive” we mean that it is up to the executing task to release the CPU to other pending tasks,
 *  the scheduler does not and cannot interrupt a task in the middle of its execution. Moreover,
 *  this scheduler is not using any prioritizing method but rather simply jumps from one task to another
 *  in the order of their creation.
 *  Even though it was designed with simplicity in mind, it does offer tasks stack separation, a
 *  nd several standard methods of controlling code execution, and ultimately brings us closer to
 *  making the most of the MCU and managing large code segments in far
 *  smaller and easer to maintain modules.
 *
 */

/******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 SolarEdge.
 * All rights reserved.</center></h2>
 *
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef LV662_HAL_XTSK_
#define LV662_HAL_XTSK_

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

/** @addtogroup XTasks
 * @{
 */

#define HAL_XTASK_ENABLED            (1)          /* Global flag to enable or disablew the module */
#define HAL_XTASK_MAX_STRING_SIZE    (20)         /* Maximum bytes allowed for a task name */
#define HAL_XTASK_DEFAULT_STACK_SIZE (0x800)      /* Default stack size  */
#define HAL_XTASK_INVALID_HANDLE     (0xFFFFFFFF) /* Invalid handle value */
#define HAL_XTASK_COLLECT_STATS      (1)          /* Collect run time statitics */
#define HAL_XTASK_MAX_TIME           (0xFFFFFFFF) /* Max time value */

/* Force stack protection in debug builds */
#ifdef _DEBUG
#define HAL_XTASK_STACK_CHECK_LEN 0 /* Count of bytes to check when validating the stack , set to 0 to disable */
#else
#define HAL_XTASK_STACK_CHECK_LEN 0
#endif

/* Exported types ------------------------------------------------------------*/
/** @defgroup XTSK_Exported_Macros XTasks Exported Macros
 * @{
 */

/* Task prototype, the caller can pass parameter through the void pointer */
typedef void (*TaskFunction_t)(void *);

typedef uint32_t TaskHandle_t; /*!< Task handle handle */

/* 'Printf' style function definition */
typedef int (*PrintfFn)(const char *__format, ...);

/**
 * @}
 */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup XTSK_Exported_Functions XTasker Exported Functions
 * @{
 */

// clang-format off

bool         vTaskStartScheduler(void  );
TaskHandle_t xTaskGetHandle(void);
TaskHandle_t xTaskCreate(char *name, TaskFunction_t cb, uint32_t stackSize, void *ptr);
int          xTaskGetStackUsage(TaskHandle_t handle);
void         xTaskDumpStats(PrintfFn print);

/* Signaling and execution control API */
void         xTaskNotify(TaskHandle_t handle, uint32_t event);
uint32_t     xTaskNotifyWait(uint32_t ticksToWait);
void         taskYIELD(void);
void         vTaskDelay(uint32_t delay);

// clang-format on

/**
 * @}
 */

/**
 * @}
 */

#endif /* LV662_HAL_XTSK_ */

/************************ (C) COPYRIGHT SolarEdge *****END OF FILE****/
