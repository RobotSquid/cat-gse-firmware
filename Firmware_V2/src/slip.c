/*
 * slip.c
 *
 * Created: 2021/11/16 15:10:10
 *  Author: tianc
 */ 

#include "slip.h"

#define END 0xC0
#define ESC 0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD

/* Receive data from USB CDC RX buffer until buffer empty or SLIP END reached and place in buffer
 * If the data contains a SLIP END packet, process packet in buffer and return length
 * Else, return 0
 */
uint32_t udi_cdc_read_slip_packet(uint8_t* buf, uint32_t* pos) {
	uint8_t c = 0;
	while (udi_cdc_is_rx_ready() && (c = udi_cdc_getc()) != END) buf[(*pos)++] = c;
	//while (udi_cdc_is_rx_ready()) udi_cdc_getc();
	if (c != END) return 0;
	uint32_t src_i = 0;
	uint32_t dest_i = 0;
	for (; src_i < (*pos); src_i++) {
		if (buf[src_i] == ESC) {
			src_i += 1;
			if (buf[src_i] == ESC_END) buf[dest_i++] = END;
			else if (buf[src_i] == ESC_ESC) buf[dest_i++] = ESC;
		} else {
			buf[dest_i++] = buf[src_i];
		}
	}
	return ((*pos) = dest_i);
}

/* Transmit buffer with in SLIP protocol
 */
void udi_cdc_write_slip_packet(uint8_t* buf, uint32_t len) {
	for (uint32_t src_i = 0; src_i < len; src_i++) {
		if (buf[src_i] == ESC) {
			udi_cdc_putc(ESC);
			udi_cdc_putc(ESC_ESC);
		}
		else if (buf[src_i] == END) {
			udi_cdc_putc(ESC);
			udi_cdc_putc(ESC_END);
		}
		else udi_cdc_putc(buf[src_i]);
	}
	udi_cdc_putc(END);
}