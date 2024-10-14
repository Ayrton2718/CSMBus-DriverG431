/*
 * cc_timer.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */
#include "cc_timer.h"

static volatile uint32_t g_ms_count;

static CCTimer_callback_t   g_callback;

void CCTimer_dummyCallback(void){}


void CCTimer_init(void)
{
    g_ms_count = 0;

    g_callback = CCTimer_dummyCallback;

    HAL_TIM_Base_Start_IT(CS_TIMER_USE_HTIM);
}

void CCTimer_bind(CCTimer_callback_t callback)
{
    g_callback = callback;
}

void CCTimer_start(CCTimer_t* tim)
{
    uint16_t now_us = __HAL_TIM_GET_COUNTER(CS_TIMER_USE_HTIM);
	uint32_t now_ms = g_ms_count;

    tim->ms = now_ms;
    tim->us = now_us;
}

uint32_t CCTimer_getMs(const CCTimer_t tim)
{
    uint16_t now_us = __HAL_TIM_GET_COUNTER(CS_TIMER_USE_HTIM);
	uint32_t now_ms = g_ms_count;

    uint32_t ms;

    if(tim.us < now_us)
    {
    	ms = (now_ms - tim.ms);
    }else if(now_ms == tim.ms){
    	ms = 0;
    }else{
    	ms = (now_ms - tim.ms - 1);
    }
    return ms;
}

uint32_t CCTimer_getUs(const CCTimer_t tim)
{
    uint16_t now_us = __HAL_TIM_GET_COUNTER(CS_TIMER_USE_HTIM);
    uint32_t now_ms = g_ms_count;

    uint32_t us;

    if(tim.us < now_us)
    {
    	us = (now_ms - tim.ms) * 1000 + (now_us - tim.us);
    }else if(now_ms == tim.ms){
    	us = ((1000 + now_us) - tim.us);
    }else{
    	us = (now_ms - tim.ms - 1) * 1000 + ((1000 + now_us) - tim.us);
    }

    return us;
}


void CCTimer_delayUs(uint32_t us)
{
    CCTimer_t start;
    CCTimer_start(&start);
    while(CCTimer_getUs(start) < us) {}
}

void __CCTimer_interrupt(TIM_HandleTypeDef* htim)
{
    if(htim->Instance == CS_TIMER_USE_HTIM->Instance)
    {
        g_ms_count++;
        g_callback();
    }
}
