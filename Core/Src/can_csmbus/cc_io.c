/*
 * cc_io.c
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */


#include "cc_io.h"

#include "cc_id.h"
#include "cc_led.h"

#define CS_IO_SEND_BUFFER        (4)

typedef struct
{
    uint8_t tx_data[8];
    FDCAN_TxHeaderTypeDef header;
} CCIo_packet_t;

static uint8_t         g_send_wp;
static volatile uint8_t         g_send_rp;
static volatile uint8_t         g_send_count;
static CCIo_packet_t   g_send_buffer[CS_IO_SEND_BUFFER];

static uint16_t   g_my_id;
static CCType_appid_t g_appid;
static CCIo_canCallback_t g_can_callback;
static CCIo_resetCallback_t g_reset_callback;

static volatile uint32_t g_safety_time;


static void CCIo_setFilterMask(uint32_t index, uint32_t fifo_id, uint16_t id1, uint16_t mask1);
static CCType_bool_t CCIo_dummyCallback(CCReg_t reg, const uint8_t* data, size_t len){return CCTYPE_FALSE;}
static void CCIo_dummyResetCallback(void){}
static void CCIo_send(uint16_t reg, const uint8_t* data, uint8_t len);


void CCIo_init(void)
{
    g_my_id = (CCId_getId() & 0b01111);

    CCIo_setFilterMask(0, FDCAN_FILTER_TO_RXFIFO0, g_my_id << 6,   0b01111 << 6); // m2s registers
    CCIo_setFilterMask(1, FDCAN_FILTER_TO_RXFIFO1, 0b00000 << 6,   0b11111 << 6); // broad cast

    if(HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_FDCAN_Start(CCIO_HCAN) != HAL_OK){ 
        Error_Handler();
    }
    
    if (HAL_FDCAN_ActivateNotification(CCIO_HCAN, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_FDCAN_ActivateNotification(CCIO_HCAN, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_FDCAN_ActivateNotification(CCIO_HCAN, FDCAN_IT_BUS_OFF, 0) != HAL_OK)
    {
        Error_Handler();
    }

    g_appid = CCType_appid_UNKNOWN;
    g_can_callback = CCIo_dummyCallback;
    g_reset_callback = CCIo_dummyResetCallback;

    g_safety_time = 0;

    g_send_wp = 0;
    g_send_rp = 0;
    g_send_count = 0;
    memset(g_send_buffer, 0x00, sizeof(g_send_buffer));
}


void CCIo_bind(CCType_appid_t appid, CCIo_canCallback_t callback, CCIo_resetCallback_t reset_callback)
{
    g_appid = appid;
    g_can_callback = callback;
    g_reset_callback = reset_callback;
}

void CCIo_sendUser(CCReg_t reg, const uint8_t* data, uint8_t len)
{
    CCIo_send(CCTYPE_MAKE_WRITE_REG(CCTYPE_MAKE_USER_REG(reg)), data, len);
}


static void CCIo_send(uint16_t reg, const uint8_t* data, uint8_t len)
{
    CCIo_packet_t packet;

    packet.header.TxFrameType = FDCAN_DATA_FRAME;
    packet.header.IdType = FDCAN_STANDARD_ID;
    packet.header.Identifier = CCTYPE_MAKE_S2M_CAN_ID(g_my_id, reg);
    packet.header.DataLength = len << 16;
    packet.header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    packet.header.BitRateSwitch = FDCAN_BRS_OFF;
    packet.header.FDFormat = FDCAN_CLASSIC_CAN;
    packet.header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    packet.header.MessageMarker = 0;
    memcpy(packet.tx_data, data, sizeof(uint8_t) * len);

    if(HAL_FDCAN_GetTxFifoFreeLevel(CCIO_HCAN) != 0)
    {
        if (HAL_FDCAN_AddMessageToTxFifoQ(CCIO_HCAN, &packet.header, packet.tx_data) == HAL_OK)
        {
            CCLed_tx();
        }else{
            CCLed_busErr();
        }
    }else{
		if(g_send_count == CS_IO_SEND_BUFFER)
		{
			g_send_rp++;
			g_send_count--;
            CCLed_busErr();
		}
		g_send_buffer[g_send_wp % CS_IO_SEND_BUFFER] = packet;
		g_send_wp++;
		g_send_count++;
    }
}


CCType_bool_t CCIo_isSafetyOn(void)
{
    return (g_safety_time < HAL_GetTick());
}

void CCIo_process(void)
{
    size_t send_count = g_send_count;
    for(size_t i = 0; (i < send_count) && (HAL_FDCAN_GetTxFifoFreeLevel(CCIO_HCAN) != 0); i++)
    {
        CCIo_packet_t packet = g_send_buffer[g_send_rp & CS_IO_SEND_BUFFER];
        if(HAL_FDCAN_AddMessageToTxFifoQ(CCIO_HCAN, &packet.header, packet.tx_data) == HAL_OK)
        {
            CCLed_tx();
        }else{
            CCLed_busErr();
        }
        g_send_rp++;
        g_send_count--;
    }
}

// Register
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
    {
        FDCAN_RxHeaderTypeDef   RxHeader;
        uint8_t data[8];

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, data) == HAL_OK)
        {
            uint32_t can_id = RxHeader.Identifier;
            uint8_t len = RxHeader.DataLength >> 16;

            if(CCTYPE_IS_M2S_PACKET(can_id))
            {
                if(CCTYPE_IS_SYS_REG(CCTYPE_GET_PACKET_REG(can_id)))
				{
					CCReg_t reg = CCTYPE_GET_SYS_REG(CCTYPE_GET_PACKET_REG(can_id));
					if(CCTYPE_IS_ACK_REG(CCTYPE_GET_PACKET_REG(can_id)))
					{
						CCType_ack_t ack;
						ack.checksum = 0;
						for(size_t i = 0; i < len; i++)
						{
							ack.checksum += data[i];
						}
						CCIo_send(CCTYPE_MAKE_ACK_REG(CCTYPE_MAKE_SYS_REG(reg)), (const uint8_t*)&ack, sizeof(CCType_ack_t));
					}

					switch(reg)
					{
					case CCReg_m2sSystem_REQ_APPID:{
						uint8_t appid = (uint8_t)g_appid;
						CCIo_send(CCTYPE_MAKE_WRITE_REG(CCTYPE_MAKE_SYS_REG(CCReg_s2mSystem_APPID)), &appid, sizeof(uint8_t));
						CCLed_rx();
						}break;

					default:
						break;
					}
				}
				else
				{
					CCReg_t reg = CCTYPE_GET_USER_REG(CCTYPE_GET_PACKET_REG(can_id));
					if(CCTYPE_IS_ACK_REG(CCTYPE_GET_PACKET_REG(can_id)))
					{
						CCType_ack_t ack;
						ack.checksum = 0;
						for(size_t i = 0; i < len; i++)
						{
							ack.checksum += data[i];
						}
						CCIo_send(CCTYPE_MAKE_ACK_REG(CCTYPE_MAKE_USER_REG(reg)), (const uint8_t*)&ack, sizeof(CCType_ack_t));
					}

					if(g_can_callback(reg, data, len))
					{
						CCLed_rx();
					}else{
                        CCLed_err();
					}
				}
            }
            else
            {
                CCLed_err();
            }
        }else{
            CCLed_busErr();
        }
    }
}


// BRC
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
    if((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != 0)
    {
        FDCAN_RxHeaderTypeDef   RxHeader;
        uint8_t data[8];

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &RxHeader, data) == HAL_OK)
        {
            uint32_t can_id = RxHeader.Identifier;
            uint8_t len = RxHeader.DataLength >> 16;

            CCType_brcReg_t reg = CCTYPE_GET_BRC_REG(CCTYPE_GET_PACKET_REG(can_id));
			switch (reg)
			{
			case CCType_brcReg_Safety:
				g_safety_time = HAL_GetTick() + CCTYPE_SAFETY_TIMEOUT;
				CCLed_rx();
				break;

			case CCType_brcReg_Unsafe:
				if(len == 4)
				{
					if((data[3] == 'U') && (data[2] == 'N') && (data[1] == 'S') && (data[0] == 'F'))
					{
						g_safety_time = 1;
						CCLed_rx();
					}
				}
				break;

			case CCType_brcReg_Reset:
                if(len == 4)
				{
					if((data[3] == 'R') && (data[2] == 'E') && (data[1] == 'S') && (data[0] == 'T'))
					{
                        g_reset_callback();
						CCLed_rx();
					}
				}
				break;

			default:
				break;
			}
        }
        else
        {
            CCLed_busErr();
        }
    }
}


static void CCIo_setFilterMask(uint32_t index, uint32_t fifo_id, uint16_t id1, uint16_t mask1)
{
	FDCAN_FilterTypeDef filter;
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterIndex = index;
    filter.FilterConfig = fifo_id;
    filter.FilterID1 = id1;
    filter.FilterID2 = mask1;

    if (HAL_FDCAN_ConfigFilter(CCIO_HCAN, &filter) != HAL_OK){
        Error_Handler();
    }
}

// BUS-OFF割り込みハンドラ
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs) {
	if (hfdcan->Instance == CCIO_HCAN->Instance) {
		if ((ErrorStatusITs & FDCAN_IT_BUS_OFF) != RESET) {
            FDCAN_ProtocolStatusTypeDef protocolStatus = {};
            HAL_FDCAN_GetProtocolStatus(hfdcan, &protocolStatus);
            if (protocolStatus.BusOff) {
                CLEAR_BIT(hfdcan->Instance->CCCR, FDCAN_CCCR_INIT);
            }
		}
	}
}
