/*
 * ui.c
 *
 * Created: 2021/11/17 23:43:16
 *  Author: tianc
 */ 

#include "ui.h"

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
		return set_servo(data[1], *((uint32_t*)(data+2)));
	case CMD_SET_OX_STATES:
		return set_states_data(1, data+1, len-1);
	case CMD_SET_FUEL_STATES:
		return set_states_data(0, data+1, len-1);
	case CMD_RESET_STATE:
		send_command_status(ACK);
		return reset_states();
	case CMD_BUTTON_PRESSED:
		if (len < 2) return send_command_status(NACK);
		send_command_status(ACK);
		return set_button_pressed(data[1]);
	case CMD_GET_STATES:
		udi_cdc_write_slip_packet(get_machine_states(), 2);
	case CMD_ABORT_ABORT:
		execute_abort();
		send_command_status(ACK);
	default:
		return send_command_status(NACK);
	}
}

void set_valve_states(uint8_t* data, uint32_t len) {
	if (len != 32) return send_command_status(NACK);
	send_command_status(ACK);
	set_servo_data(data);
}

void set_sensor_types(uint8_t type, uint8_t* data, uint32_t len) {
	if (len != 6) return send_command_status(NACK);
	send_command_status(ACK);
	if (type == 0) set_temperature_types(data);
	else if (type == 1) set_pressure_types(data);
}

void send_command_status(uint8_t data) {
	udi_cdc_write_slip_packet(&data, 1);
}

void set_states_data(uint8_t type, uint8_t* data, uint32_t len) {
	if (len != 409*8) return send_command_status(NACK);
	send_command_status(ACK);
	if (type == 0) set_fuel_states_data(data);
	else if (type == 1) set_ox_states_data(data);
}