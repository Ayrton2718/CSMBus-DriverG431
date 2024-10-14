/*
 * cc_io.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#ifndef SRC_CAN_CSMBUS_CC_IO_H
#define SRC_CAN_CSMBUS_CC_IO_H

#include "cc_type.h"
#include "fdcan.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCIO_HCAN ((&hfdcan1))

typedef CCType_bool_t (*CCIo_canCallback_t)(CCReg_t reg, const uint8_t* data, size_t len);
typedef void (*CCIo_resetCallback_t)(void);


void CCIo_init(void);

void CCIo_bind(CCType_appid_t appid, CCIo_canCallback_t callback, CCIo_resetCallback_t reset_callback);

void CCIo_sendUser(CCReg_t reg, const uint8_t* data, uint8_t len);

CCType_bool_t CCIo_isSafetyOn(void);

void CCIo_process(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_CAN_CSMBUS_CC_IO_H */
