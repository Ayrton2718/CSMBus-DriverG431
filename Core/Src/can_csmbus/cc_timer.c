#include "cs_timer.h"

static volatile uint32_t g_ms_count;

static CSTimer_callback_t   g_callback;

void CSTimer_dummyCallback(void){}


void CSTimer_init(void)
{
    g_ms_count = 0;

    g_callback = CSTimer_dummyCallback;

    HAL_TIM_Base_Start_IT(CS_TIMER_USE_HTIM);
}

void CSTimer_bind(CSTimer_callback_t callback)
{
    g_callback = callback;
}

void CSTimer_start(CSTimer_t* tim)
{
    uint16_t now_us = __HAL_TIM_GET_COUNTER(CS_TIMER_USE_HTIM);
	uint32_t now_ms = g_ms_count;

    tim->ms = now_ms;
    tim->us = now_us;
}

uint32_t CSTimer_getMs(const CSTimer_t tim)
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

uint32_t CSTimer_getUs(const CSTimer_t tim)
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


void CSTimer_delayUs(uint32_t us)
{
    CSTimer_t start;
    CSTimer_start(&start);
    while(CSTimer_getUs(start) < us) {}
}

void __CSTimer_interrupt(TIM_HandleTypeDef* htim)
{
    if(htim->Instance == CS_TIMER_USE_HTIM->Instance)
    {
        g_ms_count++;
        g_callback();
    }
}
