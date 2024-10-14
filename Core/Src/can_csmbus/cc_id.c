/*
 * cc_id.c
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#include "cc_id.h"

#include <string.h>
#include "main.h"
#include "stm32g4xx_hal.h"

#define CS_ID_CHECK_PATTERN (0x94DACB7D)

#define CS_ID_FLASH_START_ADDR (0x800F800)
#define CS_ID_FLASH_END_ADDR   (0x800FFFF)

#define CS_ID_BLINK_TIM (20)
#define CS_ID_SLEEP_TIM (100)

typedef struct
{
	uint32_t  	check_pattern;
    uint16_t     id;
    uint16_t     checksum;
}__attribute__((__packed__)) CCId_flash1_t;


// The type must be 16 bit or larger.
typedef struct
{
	uint64_t     flash1;
}__attribute__((__packed__)) CCId_flashData_t;


static CCId_t g_my_addr;
static uint32_t g_tenMsTimer;
static uint16_t g_push_counter;

static uint8_t      g_id;
static uint16_t     g_idLoopCount;

static void CCId_flashWriteId(CCId_t id);
static CCId_t CCId_flashReadId(void);
static CCType_bool_t CCId_isPushingBtn(void);


void CCId_init(void)
{
    g_my_addr = CCId_flashReadId();
    if(g_my_addr == CCId_UNKNOWN)
    {
        for(uint8_t i = 0; i < 4; i++)
        {
            HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
            HAL_Delay(200);
            HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_RESET);
            HAL_Delay(200);
        }

        CCId_flashWriteId(CS_ID_FIXED_ADDR);
        NVIC_SystemReset();
    }

    g_id = CCId_convertId2Num(g_my_addr);
}

CCId_t CCId_getId(void)
{
    return g_my_addr;
}

void CCId_process(CCType_bool_t is_safety_on)
{
    register uint32_t now_tick = HAL_GetTick();
    if(g_tenMsTimer < now_tick)
    {
        g_tenMsTimer = now_tick + 10;

        if(is_safety_on && CCId_isPushingBtn())
        {
            g_push_counter++;
            if(300 <= g_push_counter)
            {
                HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_RESET);
                if(CCId_isPushingBtn() == CCTYPE_FALSE)
                {
                    uint16_t id_num = 0;
                    uint8_t flg = 0;
                    uint32_t end_tick = HAL_GetTick() + 3000;

                    uint16_t blinkFlgCount = 10;
                    HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_RESET);
                    while(HAL_GetTick() < end_tick)
                    {
                        if(CCId_isPushingBtn() && flg == 0)
                        {
                            id_num++;
                            HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_SET);
                            flg = 1;
                            end_tick = HAL_GetTick() + 10000;
                        }

                        if((CCId_isPushingBtn() == CCTYPE_FALSE) && flg == 1)
                        {
                            HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_RESET);
                            flg = 0;
                            end_tick = HAL_GetTick() + 3000;
                        }

                        if(blinkFlgCount == 0)
                        {
                            HAL_GPIO_TogglePin(LED_TX_GPIO_Port, LED_TX_Pin);
                            HAL_GPIO_TogglePin(LED_RX_GPIO_Port, LED_RX_Pin);
                            blinkFlgCount = 10;
                        }else{
                            blinkFlgCount--;
                        }

                        HAL_Delay(10);
                    }
                    
                    HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_RESET);

                    if((id_num != 0) && (id_num < CCId_convertId2Num(CCId_UNKNOWN)))
                    {
                        CCId_flashWriteId(CCId_convertNum2Id(id_num - 1));
                        NVIC_SystemReset();
                    }
                }
            }
        }else{
            g_push_counter = 0;
        }

        if(g_my_addr != CCId_UNKNOWN)
        {
            if(g_idLoopCount <= (CS_ID_SLEEP_TIM + CS_ID_BLINK_TIM))
            {
                HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_RESET);
            }else{
                if((g_idLoopCount % CS_ID_BLINK_TIM) == 0)
                {
                    HAL_GPIO_TogglePin(LED_ID_GPIO_Port, LED_ID_Pin);
                }
            }

            if(g_idLoopCount == 0)
            {
                g_idLoopCount = (((uint16_t)g_id + 1) * CS_ID_BLINK_TIM * 2) + (CS_ID_SLEEP_TIM + CS_ID_BLINK_TIM);
            }else{
                g_idLoopCount--;
            }
        }else{
            HAL_GPIO_WritePin(LED_ID_GPIO_Port, LED_ID_Pin, GPIO_PIN_SET);
        }
    }
}


static CCId_t CCId_flashReadId(void)
{
    const CCId_flashData_t* flash_data_p = (const CCId_flashData_t*)CS_ID_FLASH_START_ADDR;
    CCId_flashData_t flash_data;

    memcpy(&flash_data, flash_data_p, sizeof(CCId_flashData_t));

    CCId_flash1_t* flash1 = (CCId_flash1_t*)&flash_data.flash1;

    if(flash1->check_pattern == CS_ID_CHECK_PATTERN)
    {
        if(flash1->checksum == flash1->id)
        {
            if(flash1->id < CCId_convertId2Num(CCId_UNKNOWN))
            {
                return CCId_convertNum2Id(flash1->id);
            }
            else
            {
                HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
                HAL_Delay(1000);
                return CCId_UNKNOWN;
            }
        }
        else
        {
            HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
            HAL_Delay(1000);
            return CCId_UNKNOWN;
        }
    }
    else
    {
        HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        return CCId_UNKNOWN;
    }
}

static void CCId_flashWriteId(CCId_t id)
{
    if(HAL_FLASH_Unlock() != HAL_OK)
    {
        HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        return;
    }

    FLASH_EraseInitTypeDef erase;
    uint32_t pageError = 0;
	erase.TypeErase = FLASH_TYPEERASE_PAGES;	// select sector
    erase.Banks = FLASH_BANK_1;
    erase.NbPages = 1;
    erase.Page = 31;
	if(HAL_FLASHEx_Erase(&erase, &pageError) != HAL_OK)
    {
        HAL_FLASH_Lock();

        HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        return;
    }

    CCId_flashData_t flash_data;
    CCId_flash1_t* flash1 = (CCId_flash1_t*)&flash_data.flash1;
    flash1->check_pattern = CS_ID_CHECK_PATTERN;
    flash1->id = CCId_convertId2Num(id);
    flash1->checksum = flash1->id;

    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, CS_ID_FLASH_START_ADDR, (uint64_t)flash_data.flash1) != HAL_OK)
    {
        HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
    }

    if(HAL_FLASH_Lock() != HAL_OK)
    {
        HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_RX_GPIO_Port, LED_RX_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        return;
    }
}


static CCType_bool_t CCId_isPushingBtn(void)
{
    static uint8_t befor1_state = 0;
    static uint8_t befor2_state = 0;
    if(!HAL_GPIO_ReadPin(BTN_ID_GPIO_Port, BTN_ID_Pin))
    {
        // on
        if(befor1_state)
        {
            befor2_state = 1;
            return CCTYPE_TRUE;
        }else if(!befor1_state && befor2_state){
            befor2_state = 1;
            return CCTYPE_TRUE;
        }else if(!befor1_state && !befor2_state){
            befor1_state = 1;
            return CCTYPE_FALSE;
        }
    }else{
        // off
        if(!befor1_state)
        {
            befor2_state = 0;
            return CCTYPE_FALSE;
        }else if(befor1_state && !befor2_state){
            befor1_state = 0;
            return CCTYPE_FALSE;
        }else if(befor1_state && befor2_state){
            befor1_state = 0;
            return CCTYPE_TRUE;
        }
    }
    return CCTYPE_FALSE;
}
