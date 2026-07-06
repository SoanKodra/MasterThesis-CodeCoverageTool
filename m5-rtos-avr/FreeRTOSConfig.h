#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// Minimale FreeRTOS-Konfiguration fuer den Mega2560, angepasst fuer
// dieses Projekt (M5, Proposal Abschnitt 11.2). Nur die Optionen,
// die wir tatsaechlich brauchen, aktiviert - Rest bewusst aus.

#define configUSE_PREEMPTION            1
#define configCPU_CLOCK_HZ              16000000UL
#define configMAX_PRIORITIES            3
#define configMINIMAL_STACK_SIZE        128
#define configMAX_TASK_NAME_LEN         8
#define configUSE_TRACE_FACILITY        0
#define configUSE_16_BIT_TICKS          1
#define configIDLE_SHOULD_YIELD         1
#define configUSE_MUTEXES               0
#define configUSE_TIMERS                0
#define configCHECK_FOR_STACK_OVERFLOW  0
#define configUSE_MALLOC_FAILED_HOOK    0
#define configUSE_CO_ROUTINES           0
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0

#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskPrioritySet        0
#define INCLUDE_uxTaskPriorityGet       0
#define INCLUDE_vTaskDelete             0
#define INCLUDE_vTaskSuspend            0

#endif
