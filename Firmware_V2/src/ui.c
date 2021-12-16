/*
 * ui.c
 *
 * Created: 2021/11/17 23:43:16
 *  Author: tianc
 */ 

#include "ui.h"
#include "devices.h"

#define CMD_IDENT             0x00
#define CMD_VERSION           0x01
#define CMD_SET_VALVES        0x02
#define CMD_GET_VALVES        0x03
#define CMD_TEMP_STATES       0x04
#define CMD_PRESS_STATES      0x05
#define CMD_TEMP_TYPES        0x06
#define CMD_PRESS_TYPES       0x07
#define CMD_SINGLE_VALVE      0x08
#define CMD_SET_OX_STATES     0x11
#define CMD_SET_FUEL_STATES   0x12
#define CMD_RESET_STATE       0x13
#define CMD_BUTTON_PRESSED    0x14

#define ACK               0xFF
#define NACK			  0x00

const char* IDENT = "CAT GSE CONTROLLER";
const char* VERSION = "0.0.1";

void process_command(uint8_t* data, uint32_t len) {
	switch (data[0]) {
	case CMD_IDENT:
		udi_cdc_write_slip_packet((uint8_t*)IDENT, strlen(IDENT));
		return;
	case CMD_VERSION:
		udi_cdc_write_slip_packet((uint8_t*)VERSION, strlen(VERSION));
		return;
	case CMD_SET_VALVES:
		return set_valve_states(data+1, len-1);
	case CMD_GET_VALVES:
		udi_cdc_write_slip_packet(get_valve_data(), 32);
		return;
	case CMD_TEMP_STATES:
		udi_cdc_write_slip_packet(get_temperature_data(), 6*8);
		return;
	case CMD_PRESS_STATES:
		udi_cdc_write_slip_packet(get_pressure_data(), 6*8);
		return;
	case CMD_TEMP_TYPES:
		return set_sensor_types(0, data+1, len-1);
	case CMD_PRESS_TYPES:
		return set_sensor_types(1, data+1, len-1);
	case CMD_SINGLE_VALVE:
		if (len < 6) return send_command_status(NACK);
		send_command_status(ACK);
		set_servo(data[1], *((uint32_t*)(data+2)));
	case CMD_SET_OX_STATES:
	case CMD_SET_FUEL_STATES:
		// NOT IMPLEMENTED
		return set_sequence_data(data+1, len-1);
	case CMD_RESET_STATE:
		// NOT IMPLEMENTED
		return set_sequence_position(data+1, len-1);
	case CMD_BUTTON_PRESSED:
		// NOT IMPLEMENTED
		return;
	default:
		send_command_status(NACK);
	}
}

void set_valve_states(uint8_t* data, uint32_t len) {
	if (len != 32) return send_command_status(NACK);
	send_command_status(ACK);
	set_servo_data(data);
}

void set_sensor_types(uint8_t type, uint8_t* data, uint32_t len) {
	if (len != 96) return send_command_status(NACK);
	send_command_status(ACK);
	if (type == 0) set_temperature_types(data);
	else if (type == 1) set_pressure_types(data);
}

void send_command_status(uint8_t data) {
	udi_cdc_write_slip_packet(&data, 1);
}

void set_states_data(state_machine *machine, uint8_t* data, uint32_t len) {
	// NOT IMPLEMENTED
	send_command_status(ACK);
}

void set_sequence_position(uint8_t* data, uint32_t len) {
	// NOT IMPLEMENTED
	send_command_status(ACK);
}