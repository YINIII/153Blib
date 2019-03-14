#include "board.h"

#define LINE_IN 0
#define MIC 1
#define BUFFER_FULL 0
#define BUFFER_EMPTY 1
#define BUFFER_AVAILABLE 2

typedef struct ring_buff {
	uint32_t buffer[256];
	uint8_t read_index;
	uint8_t write_index;
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
	Board_LED_Toggle(0);
	Chip_TIMER_Reset(LPC_TIMER0);
	Chip_TIMER_ClearMatch(LPC_TIMER0, 0);
}

void I2S_IRQHandler(void) {
	while ((ring_buff_get_status(&ring_buffer) != BUFFER_FULL) && (Chip_I2S_GetRxLevel(LPC_I2S) > 0)) {
		ring_buffer.buffer[ring_buffer.write_index++] = Chip_I2S_Receive(LPC_I2S);
	}
//	while ((ring_buff_get_status(&ring_buffer) != BUFFER_EMPTY) && (Chip_I2S_GetTxLevel(LPC_I2S) < 8)) {
//		Chip_I2S_Send(LPC_I2S, ring_buffer.buffer[ring_buffer.read_index++]);
//	}
}

int main(void) {

	I2S_AUDIO_FORMAT_T audio_Confg;
	audio_Confg.SampleRate = 48000;
	/* Select audio data is 2 channels (1 is mono, 2 is stereo) */
	audio_Confg.ChannelNumber = 2;
	/* Select audio data is 16 bits */
	audio_Confg.WordWidth = 16;
	int PrescaleValue = 120000;

	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, true);

	Board_Audio_Init(LPC_I2S, MIC);
	Chip_I2S_Init(LPC_I2S);
//	Chip_I2S_RxConfig(LPC_I2S, &audio_Confg);
	Chip_I2S_TxConfig(LPC_I2S, &audio_Confg);
	Chip_I2S_DisableMute(LPC_I2S);
//	Chip_I2S_Int_RxCmd(LPC_I2S, ENABLE, 4);
	Chip_I2S_Int_TxCmd(LPC_I2S, ENABLE, 4);
	NVIC_ClearPendingIRQ(I2S_IRQn);
	NVIC_EnableIRQ(I2S_IRQn);

	Chip_TIMER_Init(LPC_TIMER0);
	Chip_TIMER_PrescaleSet(LPC_TIMER0, PrescaleValue);
	Chip_TIMER_SetMatch(LPC_TIMER0, 0, 200);
	Chip_TIMER_MatchEnableInt(LPC_TIMER0, 0);
	Chip_TIMER_Disable(LPC_TIMER0);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);

//	Chip_I2S_RxStart(LPC_I2S);
	Chip_I2S_TxStart(LPC_I2S);

	while(1) {
		__WFI();
	}

	return 0;
}
