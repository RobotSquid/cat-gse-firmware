/*
 * sequencer.c
 *
 * Created: 2021/12/05 01:12:59
 *  Author: tianc
 */ 

#include "sequencer.h"

typedef struct servo_keyframe {
	uint32_t delay_ms_before;
	uint8_t servo_idx;
	uint32_t servo_state;
} servo_keyframe;

#define TRIGGER_NONE         0x00
#define TRIGGER_BUTTON       0x01 // param1: button idx
#define TRIGGER_TIMER        0x02 // param1: time delay
#define TRIGGER_PRESSURE_GT  0x11 // param1: sensor idx, param2: value
#define TRIGGER_PRESSURE_LT  0x12 // param1: sensor idx, param2: value
#define TRIGGER_TEMP_GT      0x21 // param1: sensor idx, param2: value
#define TRIGGER_TEMP_LT      0x22 // param1: sensor idx, param2: value

typedef struct state_transition {
	uint8_t trigger_type;
	uint32_t trigger_param1;
	uint32_t trigger_param2;
	uint8_t target_state;
	servo_keyframe transition[16];
} state_transition;

typedef struct state_node {
	uint8_t state_flags; // bit0: state active, bit1: lockout 
	state_transition transitions[4];
	state_transition abort;
} state_node;

typedef struct state_machine {
	uint8_t state_idx;
	state_node states[8];
} state_machine;

state_machine oxidizer_side;
state_machine fuel_side;
uint8_t button_states[16];
uint8_t keyframe_idx;
servo_keyframe *current_keyframes;
uint32_t last_keyframe_millis = 0;
uint32_t timer_start_millis = 0;

void process_sequence(void) {
	check_state_machine(&oxidizer_side);
	check_state_machine(&fuel_side);
}

void check_state_machine(state_machine* machine) {
	state_node *node = &machine->states[machine->state_idx];
	for (int i = 0; i < 4; i++) {
		state_transition *transition = &node->transitions[i];
		if (get_criteria_satisfied(transition)) {
			execute_transition(machine, node, transition);
			break;
		}
	}
}

uint8_t get_criteria_satisfied(state_transition *transition) {
	switch (transition->trigger_type) {
	case TRIGGER_BUTTON:
		return transition->trigger_param1 < 16 && button_states[transition->trigger_param1];
	case TRIGGER_TIMER:
		return get_msec() - timer_start_millis > transition->trigger_param1;
	case TRIGGER_PRESSURE_GT:
		return transition->trigger_param1 < 6 && get_pressure(transition->trigger_param1) > transition->trigger_param2;
	case TRIGGER_PRESSURE_LT:
		return transition->trigger_param1 < 6 && get_pressure(transition->trigger_param1) < transition->trigger_param2;
	case TRIGGER_TEMP_GT:
		return transition->trigger_param1 < 6 && get_temperature(transition->trigger_param1) > transition->trigger_param2;
	case TRIGGER_TEMP_LT:
		return transition->trigger_param1 < 6 && get_temperature(transition->trigger_param1) < transition->trigger_param2;
	case TRIGGER_NONE:
	default:
		return 0;
	}
}

uint8_t execute_transition(state_machine* machine, state_node *node, state_transition *transition) {
	machine->state_idx = transition->target_state;
	node->state_flags &= ~(0b01);
	machine->states[machine->state_idx].state_flags &= ~(0b01);
	current_keyframes = &transition->transition;
	keyframe_idx = 0;
	last_keyframe_millis = 0;
	return 1;
}