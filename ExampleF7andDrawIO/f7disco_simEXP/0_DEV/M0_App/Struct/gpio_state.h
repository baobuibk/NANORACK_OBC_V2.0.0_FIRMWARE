/*
 * gpio_state.h
 *
 *  Created on: Apr 18, 2025
 *      Author: CAO HIEU
 */

#ifndef M0_APP_STRUCT_GPIO_STATE_H_
#define M0_APP_STRUCT_GPIO_STATE_H_

typedef enum {
    toOBC_ERROR = 0,      // BUSY = 0, READYSEND = 0
    toOBC_READYSEND = 1,  // BUSY = 0, READYSEND = 1
    toOBC_BUSY = 2,       // BUSY = 1, READYSEND = 0
    toOBC_IDLE = 3        // BUSY = 1, READYSEND = 1
} toOBC_State_t;

void toOBC_Init(void);
void toOBC_SetState(toOBC_State_t state);
toOBC_State_t toOBC_GetState(void);

typedef enum {
    CPROCESS_FREE,  	// CPROCESS pin = 1 (high)
    CPROCESS_BUSY   	// CPROCESS pin = 0 (low)
} CPROCESS_State_t;

void CPROCESS_Init(void);
void CPROCESS_SetState(CPROCESS_State_t state);
CPROCESS_State_t CPROCESS_GetState(void);

#endif /* M0_APP_STRUCT_GPIO_STATE_H_ */
