/*
 * ui.h
 *
 * Created: 2021/11/17 23:43:28
 *  Author: tianc
 */ 


#ifndef UI_H_
#define UI_H_

#include "slip.h"
#include <string.h>

void process_command(uint8_t* data, uint32_t len);

void set_valve_states(uint8_t* data, uint32_t len);
void send_command_status(uint8_t data);
void set_sensor_types(uint8_t type, uint8_t* data, uint32_t len);

void set_sequence_data(uint8_t* data, uint32_t len);
void set_sequence_position(uint8_t* data, uint32_t len);

#endif /* UI_H_ */