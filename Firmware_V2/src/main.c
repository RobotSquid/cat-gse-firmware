/*
 * main.c
 *
 * Created: 2021/11/16 15:09:47
 *  Author: tianc
 */ 

#include <asf.h>
#include <string.h>
#include "slip.h"
#include "ui.h"
#include "devices.h"

void devices_init(void);
void process_usb_data(void);

// USB SLIP rx buffer
uint8_t usb_available = 0;
#define RX_BUFFER_LENGTH 1024
uint32_t rx_pos = 0;
uint8_t rx_blob[RX_BUFFER_LENGTH];

int main (void)
{
	system_init();
	devices_init();
	initialize_devices();
	
	while (1) {
		update_sensor_data();
		// todo: process sequence
		push_servo_data();
		process_usb_data();
		
		// finally
		delay_ms(2);
	}
}

void devices_init(void) {
	irq_initialize_vectors();
	cpu_irq_enable();
	udc_start();
	delay_init();
}

void process_usb_data(void) {
	if (usb_available == 1) {
		while (udi_cdc_is_rx_ready()) {
			if (udi_cdc_read_slip_packet(rx_blob, &rx_pos) > 0) {
				process_command(rx_blob, rx_pos);
				rx_pos = 0;
			}
		}
		usb_available = 0;
	}
}

// Data is available on USB, process it
void udi_cdc_rx_callback(uint8_t port) {
	usb_available = 1;
}