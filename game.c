/*****************************************************************************
 *
 *   Copyright(C) 2011, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * Embedded Artists AB assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. Embedded Artists AB
 * reserves the right to make changes in the software without
 * notification. Embedded Artists AB also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "string.h"
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_timer.h"
#include "lpc177x_8x_uart.h"
#include "joystick.h"
#include "board.h"

const char begin_flag;
static volatile bool fDebouncing;
static volatile bool blink;

/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/

#define UART_PORT (LPC_UART0)


/******************************************************************************
 * Local Functions
 *****************************************************************************/


/******************************************************************************
 * Main method
 *****************************************************************************/
void TIMER0_IRQHandler(void) {
	fDebouncing = false;
  	Chip_TIMER_Disable(LPC_TIMER0);
  	Chip_TIMER_Reset(LPC_TIMER0);
  	Chip_TIMER_ClearMatch(LPC_TIMER0,0);
 }


void TIMER1_IRQHandler(void) {
  	Board_LED_Toggle(0);
  	Chip_TIMER_Reset(LPC_TIMER1);
  	Chip_TIMER_ClearMatch(LPC_TIMER1,1);
 }


void GPIO_IRQHandler(void)
  {
  	if(fDebouncing) {}
  	else {
  		blink = !blink;
  		fDebouncing = true;
  		Chip_TIMER_Enable(LPC_TIMER0);
  		if(blink) {
  			Board_LED_Toggle(0);
  			Chip_TIMER_Reset(LPC_TIMER1);
  			Chip_TIMER_Enable(LPC_TIMER1);
  			Chip_UART_Send(LPC_UART0, &begin_flag, 1);
  		}
  		else {
  			Board_LED_Set(0, false);
  			Chip_TIMER_Disable(LPC_TIMER1);
  			Chip_UART_SendBlocking(LPC_UART0, &stop_flag, 3);
  		}
  	}
  	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT2, 1 << 10);
  }

int main (void)
{
	TIM_TIMERCFG_Type timerCfg;

  uint8_t joyState = 0;

  /* Initialize devices */

  // initialize timer
  TIM_ConfigStructInit(TIM_TIMER_MODE, &timerCfg);
  TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);


  console_init();
  joystick_init();

  Chip_GPIO_SetPinDIRInput(LPC_GPIO, GPIOINT_PORT2, 10);
  Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT2, 1 << 10);
  NVIC_ClearPendingIRQ(GPIO_IRQn);
  NVIC_EnableIRQ(GPIO_IRQn);

  Chip_TIMER_Init(LPC_TIMER0);
  Chip_TIMER_PrescaleSet(LPC_TIMER0, PrescaleValue);
  Chip_TIMER_SetMatch(LPC_TIMER0, 0, 100);
  Chip_TIMER_MatchEnableInt(LPC_TIMER0, 0);

  Chip_TIMER_Init(LPC_TIMER1);
  Chip_TIMER_PrescaleSet(LPC_TIMER1, PrescaleValue);
  Chip_TIMER_SetMatch(LPC_TIMER1, 1, 250);
  Chip_TIMER_MatchEnableInt(LPC_TIMER1, 1);

  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  NVIC_EnableIRQ(TIMER0_IRQn);
  NVIC_ClearPendingIRQ(TIMER1_IRQn);
  NVIC_EnableIRQ(TIMER1_IRQn);

  Board_UART_Init(LPC_UART0);
  Chip_UART_Init(LPC_UART0);
  Chip_UART_SetBaud(LPC_UART0, 115200);
  Chip_UART_ConfigData(LPC_UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
  Chip_UART_IntEnable(LPC_UART0, (UART_IER_RBRINT | UART_IER_RLSINT));
  Chip_UART_TXEnable(LPC_UART0);
  Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
  							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

  while(1) {
    joyState = joystick_read();

    char byte;

    if (joyState & JOYSTICK_UP) {
      //console_sendString((uint8_t*)"Up ");
      byte = "u";
      Chip_UART_Send(LPC_UART0, &byte, 1);
    }

    if (joyState & JOYSTICK_DOWN) {
      //console_sendString((uint8_t*)"Down ");
      byte = "d";
      Chip_UART_Send(LPC_UART0, &byte, 1);
    }

    if (joyState & JOYSTICK_LEFT) {
      //console_sendString((uint8_t*)"Left ");
      byte = "l";
      Chip_UART_Send(LPC_UART0, &byte, 1);
    }

    if (joyState & JOYSTICK_RIGHT) {
      //console_sendString((uint8_t*)"Right ");
      byte = "r";
      Chip_UART_Send(LPC_UART0, &byte, 1);
    }

    if (joyState & JOYSTICK_CENTER) {
      //console_sendString((uint8_t*)"Center ");
      byte1 = "c";
      Chip_UART_Send(LPC_UART0, &byte, 1);
    }

    if (joyState != 0) {
      console_sendString((uint8_t*)"\r\n");
    }

    TIM_Waitms(200);

  }
}
