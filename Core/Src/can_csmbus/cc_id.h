/*
 * cc_id.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#ifndef SRC_CAN_CSMBUS_CC_ID_H
#define SRC_CAN_CSMBUS_CC_ID_H

#include "cc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CS_ID_FIXED_ADDR (CCId_8)

void CCId_init(void);

CCId_t CCId_getId(void);

void CCId_process(CCType_bool_t is_safety_on);

#ifdef __cplusplus
}
#endif


#endif /* SRC_CAN_CSMBUS_CC_ID_H */
