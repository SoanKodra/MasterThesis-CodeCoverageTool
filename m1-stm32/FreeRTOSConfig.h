#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// Minimale FreeRTOS-Konfiguration fuer STM32F411RE (Cortex-M4F),
// analog zur AVR-Config aus m5-rtos-avr, aber fuer den offiziellen
// ARM_CM4F-Port zugeschnitten.

#define configUSE_PREEMPTION            1
#define configCPU_CLOCK_HZ              16000000UL
#define configTICK_RATE_HZ              1000
#define configMAX_PRIORITIES            3
#define configMINIMAL_STACK_SIZE        128
#define configTOTAL_HEAP_SIZE           4096
#define configMAX_TASK_NAME_LEN         8
#define configUSE_TRACE_FACILITY        0
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1
#define configUSE_MUTEXES               0
#define configUSE_TIMERS                0
#define configCHECK_FOR_STACK_OVERFLOW  0
#define configUSE_MALLOC_FAILED_HOOK    0
#define configUSE_CO_ROUTINES           0
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0

// Cortex-M-spezifisch: Prioritaeten fuer den NVIC (Interrupt-Controller).
// STM32F4 hat 4 Bit fuer Prioritaeten (16 Stufen).
#define configPRIO_BITS                 4
#define configKERNEL_INTERRUPT_PRIORITY         (15 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (5 << (8 - configPRIO_BITS))

#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskPrioritySet        0
#define INCLUDE_uxTaskPriorityGet       0
#define INCLUDE_vTaskDelete             0
#define INCLUDE_vTaskSuspend            0

// Zwingend erwartet vom ARM_CM4F-Port fuer die SysTick-Konfiguration
#define configUSE_TICKLESS_IDLE         0

#endif
