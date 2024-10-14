/*
 * cc_led.c
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#include "cc_led.h"

#include <main.h>

static uint8_t      g_isEnableBlink;
static uint32_t     g_tenMsTimer;

static uint8_t      g_txStopCount;
static uint8_t      g_rxStopCount;

static uint8_t      g_errStopCount;
static uint8_t      g_errFlgCount;
static uint8_t      g_errBlinkInterval;


void CCLed_init(void)
{
    g_isEnableBlink = 1;
    g_tenMsTimer = 0;

	g_txStopCount = 0;
	g_rxStopCount = 0;

    g_errStopCount = 0;
	g_errFlgCount = 0;
    g_errBlinkInterval = 50; // First bus err interval will be 100ms.

    HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_RESET);
}

void CCLed_tx(void)
{
    g_txStopCount = 1;
}


void CCLed_rx(void)
{
    g_rxStopCount = 1;
}



void CCLed_hungUp(void)
{
    g_isEnableBlink = 0;
    
    HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_RESET);
}


void CCLed_err(void)
{
    g_errStopCount = 20;
    g_errBlinkInterval = 5;
    HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
}

void CCLed_busErr(void)
{
    g_errStopCount = 10;
}


void CCLed_process(CCType_bool_t is_safety_on)
{
    register uint32_t now_tick = HAL_GetTick();

    if(g_isEnableBlink && g_tenMsTimer < now_tick)
    {
        g_tenMsTimer = now_tick + 10;

        if(g_txStopCount == 0)
        {
            HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_RESET);
        }else{
            HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
            g_txStopCount--;
        }

        if(g_rxStopCount == 0)
        {
            HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_RESET);
        }else{
            HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
            g_rxStopCount--;
        }

        if(g_errStopCount == 0)
        {
            HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_RESET);
            g_errFlgCount = g_errBlinkInterval;
        }else{
            if(g_errFlgCount == 0)
            {
                g_errFlgCount = g_errBlinkInterval;
                HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_ERR_Pin);
            }else{
                g_errFlgCount--;
            }
            g_errStopCount--;
        }
    }
}