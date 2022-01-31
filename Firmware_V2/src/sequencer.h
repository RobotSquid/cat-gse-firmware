/*
 * sequencer.h
 *
 * Created: 2021/12/05 15:21:52
 *  Author: tianc
 */ 


#ifndef SEQUENCER_H_
#define SEQUENCER_H_

#include <asf.h>
#include "devices.h"

typedef struct servo_keyframe {
	uint32_t delay_ms_before;
	uint32_t servo_state;
	uint8_t servo_idx;
	uint8_t temp1, temp2, temp3;
} servo_keyframe;

#define TRIGGER_NONE         0x00
#define TRIGGER_BUTTON       0x01 // param1: button idx
#define TRIGGER_TIMER        0x02 // param1: time delay
#define TRIGGER_PRESSURE_GT  0x11 // param1: sensor idx, param2: value
#define TRIGGER_PRESSURE_LT  0x12 // param1: sensor idx, param2: value
#define TRIGGER_TEMP_GT      0x21 // param1: sensor idx, param2: value
#define TRIGGER_TEMP_LT      0x22 // param1: sensor idx, param2: value
#define TRIGGER_FORCE_GT     0x31 // param1: value
#define TRIGGER_FORCE_LT     0x32 // param1: value

typedef struct state_transition {
	uint8_t transition_flags; // bit0: abort transition
	uint8_t trigger_type;
	uint8_t target_state;
	uint8_t temp1;
	uint32_t trigger_param1;
	uint32_t trigger_param2;
	servo_keyframe transition[4];
} state_transition;

// size of node: 32+4+8*(12+4*(12))=516
typedef struct state_node {
	uint32_t state_flags; // bit0: state active
	uint16_t state_servos[16];
	state_transition transitions[8];
} state_node;

typedef struct state_machine {
	uint8_t state_idx;
	state_node states[8];
	uint8_t keyframe_idx;
	servo_keyframe *current_keyframes;
	uint32_t last_keyframe_millis;
	uint32_t timer_start_millis;
} state_machine;

void process_sequence(void);
uint8_t check_state_machine(state_machine *machine);
uint8_t get_criteria_triggered(state_machine *machine, state_transition *transition);
uint8_t execute_transition(state_machine *machine, state_node *node, state_transition *transition);
void reset_states(void);
void set_fuel_states_data(uint8_t *data);
void set_ox_states_data(uint8_t *data);
uint8_t* get_machine_states(void);
void set_button_pressed(uint8_t button);
void execute_abort(void);

#endif /* SEQUENCER_H_ */