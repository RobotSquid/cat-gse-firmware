/*
 * ui.h
 *
 * Created: 2021/11/17 23:43:28
 *  Author: tianc
 */ 


#ifndef UI_H_
#define UI_H_

#include "slip.h"
#include "devices.h"
#include "sequencer.h"
#include <string.h>

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
#define CMD_GET_STATES        0x15
#define CMD_ABORT_ABORT       0xFF

#define ACK                   0xFF
#define NACK			      0x00

void process_command(uint8_t* data, uint32_t len);

void set_valve_states(uint8_t* data, uint32_t len);
void send_command_status(uint8_t data);
void set_sensor_types(uint8_t type, uint8_t* data, uint32_t len);

void set_states_data(uint8_t type, uint8_t* data, uint32_t len);

#endif /* UI_H_ */