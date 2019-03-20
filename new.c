#include "board.h"
#include "uda1380.h"

#define LINE_IN 0
#define MIC 1
#define BUFFER_FULL 0
#define BUFFER_EMPTY 1
#define BUFFER_AVAILABLE 2

static volatile bool fDebouncing;
static volatile bool blink;
const char begin_flag[] = "bgn";
const char stop_flag[] = "stp";

typedef struct ring_buff {
	uint32_t buffer[16000];
	uint16_t read_index;
	uint16_t write_index;
} Ring_Buffer_t;

static Ring_Buffer_t ring_buffer;

static uint8_t ring_buff_get_status(Ring_Buffer_t *ring_buff) {
	if (ring_buff->read_index == ring_buff->write_index) {
		return BUFFER_EMPTY;
	}
	else if (ring_buff->read_index == (ring_buff->write_index) + 1) {
		return BUFFER_FULL;
	}
	else {return BUFFER_AVAILABLE; }
}

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

void I2S_IRQHandler(void) {
//	if(Chip_I2S_GetRxLevel(LPC_I2S) > 0) {
//		uint32_t data = Chip_I2S_Receive(LPC_I2S);
//		uint8_t byte1 = data >> 24;
//		uint8_t byte2 = data >> 16;
//		uint8_t byte3 = data >> 8;
//		uint8_t byte4 = data;
//		Chip_UART_Send(LPC_UART0, &byte1, 1);
//		Chip_UART_Send(LPC_UART0, &byte2, 1);
//		Chip_UART_Send(LPC_UART0, &byte3, 1);
//		Chip_UART_Send(LPC_UART0, &byte4, 1);
//	}
	while ((ring_buff_get_status(&ring_buffer) != BUFFER_FULL) && (Chip_I2S_GetRxLevel(LPC_I2S) > 0)) {
		ring_buffer.buffer[ring_buffer.write_index++] = Chip_I2S_Receive(LPC_I2S);
		ring_buffer.write_index = ring_buffer.write_index % 16000;
		//printf("%d\n", ring_buffer.buffer[ring_buffer.write_index - 1]);
	}
//	while (ring_buff_get_status(&ring_buffer) != BUFFER_EMPTY) {
//		uint8_t byte1 = ring_buffer.buffer[ring_buffer.read_index] >> 24;
//		uint8_t byte2 = ring_buffer.buffer[ring_buffer.read_index] >> 16;
//		uint8_t byte3 = ring_buffer.buffer[ring_buffer.read_index] >> 8;
//		uint8_t byte4 = ring_buffer.buffer[ring_buffer.read_index];
//		Chip_UART_Send(LPC_UART0, &byte1, 1);
//		Chip_UART_Send(LPC_UART0, &byte2, 1);
//		Chip_UART_Send(LPC_UART0, &byte3, 1);
//		Chip_UART_Send(LPC_UART0, &byte4, 1);
//		ring_buffer.read_index++;
//		ring_buffer.read_index = ring_buffer.read_index % 16000;
//	}
	while (ring_buff_get_status(&ring_buffer) != BUFFER_EMPTY) {
		Chip_UART_Send(LPC_UART0, &ring_buffer.buffer[ring_buffer.read_index++], 4);
		ring_buffer.read_index = ring_buffer.read_index % 16000;
	}
//	while ((ring_buff_get_status(&ring_buffer) != BUFFER_EMPTY) && (Chip_I2S_GetTxLevel(LPC_I2S) < 8)) {
//		Chip_UART_Send(LPC_UART0, &ring_buffer.buffer[ring_buffer.read_index], 4);
//		Chip_I2S_Send(LPC_I2S, ring_buffer.buffer[ring_buffer.read_index++]);
//	}
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
			Chip_UART_SendBlocking(LPC_UART0, &begin_flag, 3);
			Chip_I2S_RxStart(LPC_I2S);
		}
		else {
			Board_LED_Set(0, false);
			Chip_TIMER_Disable(LPC_TIMER1);
			Chip_I2S_RxPause(LPC_I2S);
			Chip_UART_SendBlocking(LPC_UART0, &stop_flag, 3);
		}
	}
	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT2, 1 << 10);
}

int main(void) {

	I2S_AUDIO_FORMAT_T audio_Confg;
	audio_Confg.SampleRate = 44100;
	/* Select audio data is 2 channels (1 is mono, 2 is stereo) */
	audio_Confg.ChannelNumber = 2;
	/* Select audio data is 16 bits */
	audio_Confg.WordWidth = 16;
	int PrescaleValue = 120000;
	blink = false;

	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);

	Board_Audio_Init(LPC_I2S, MIC);
	Chip_I2S_Init(LPC_I2S);
	Chip_I2S_RxConfig(LPC_I2S, &audio_Confg);
	Chip_I2S_TxConfig(LPC_I2S, &audio_Confg);
	Chip_I2S_DisableMute(LPC_I2S);
	Chip_I2S_Int_RxCmd(LPC_I2S, ENABLE, 4);
	Chip_I2S_Int_TxCmd(LPC_I2S, DISABLE, 4);
	NVIC_ClearPendingIRQ(I2S_IRQn);
	NVIC_EnableIRQ(I2S_IRQn);

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
//	NVIC_EnableIRQ(UART0_IRQn);
//	NVIC_SetPriority(UART0_IRQn, 1);
	Chip_UART_TXEnable(LPC_UART0);
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	Chip_I2S_RxStop(LPC_I2S);
//	Chip_I2S_TxStart(LPC_I2S);

	while(1) {
		__WFI();
	}

	return 0;
}
