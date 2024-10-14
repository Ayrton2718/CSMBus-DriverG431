/*
 * cc_led.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#ifndef SRC_CAN_CSMBUS_CC_LED_H
#define SRC_CAN_CSMBUS_CC_LED_H

#include "cc_type.h"


#ifdef __cplusplus
extern "C" {
#endif

void CCLed_init(void);

// tx : toggle
void CCLed_tx(void);

// rx : toggle
void CCLed_rx(void);

// Hang up
// tx : on, rx : on, er : on
void CCLed_hungUp(void);

// Other Error
// tx : blink, rx : blink, er : blink
void CCLed_err(void);

// Other bus_err
// tx : blink, rx : blink, er : on
void CCLed_busErr(void);

// Control function
void CCLed_process(CCType_bool_t is_safety_on);

#ifdef __cplusplus
}
#endif



#endif /* SRC_CAN_CSMBUS_CC_LED_H */
