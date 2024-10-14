/*
 * cc_timer.h
 *
 *  Created on: Oct 12, 2023
 *      Author: sen
 */

#ifndef SRC_CAN_CSMBUS_CC_TIMER_H
#define SRC_CAN_CSMBUS_CC_TIMER_H

#include "cc_type.h"
#include "tim.h"

#define CS_TIMER_USE_HTIM (&htim7)

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CCTimer_callback_t)(void);


typedef struct
{
    volatile uint32_t ms;
    volatile uint16_t us;
} CCTimer_t;

void CCTimer_init(void);

void CCTimer_bind(CCTimer_callback_t callback);

void CCTimer_start(CCTimer_t* tim);

uint32_t CCTimer_getMs(const CCTimer_t tim);
uint32_t CCTimer_getUs(const CCTimer_t tim);

void CCTimer_delayUs(uint32_t us);

void __CCTimer_interrupt(TIM_HandleTypeDef* htim);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CAN_CSMBUS_CC_TIMER_H */
