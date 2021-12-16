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
	uint8_t transition_flags; // bit0: abort transition
	uint8_t trigger_type;
	uint32_t trigger_param1;
	uint32_t trigger_param2;
	uint8_t target_state;
	servo_keyframe transition[4];
} state_transition;

typedef struct state_node {
	uint8_t state_flags; // bit0: state active
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

state_machine oxidizer_side;
state_machine fuel_side;
uint8_t button_states[16];

void process_sequence(void) {
	while (check_state_machine(&oxidizer_side)) {};
	while (check_state_machine(&fuel_side)) {};
	for (int i = 0; i < 16; i++) {
		button_states[i] = 0;
	}
}

uint8_t check_state_machine(state_machine* machine) {
	state_node *node = &machine->states[machine->state_idx];
	for (int i = 0; i < 8; i++) {
		state_transition *transition = &node->transitions[i];
		// (state_active AND not lockout) OR (abort_transition)
		uint8_t transition_allowed = ((node->state_flags & 0b1) || (transition->transition_flags & 0b1));
		if (transition_allowed && get_criteria_triggered(transition)) {
			execute_transition(machine, node, transition);
			return 1;
		}
	}
	if (machine->keyframe_idx < 16) {
		servo_keyframe *next = machine->current_keyframes[machine->keyframe_idx];
		if (get_msec() - machine->last_keyframe_millis >= next->delay_ms_before) {
			if (next->servo_state != 0) set_servo(next->servo_idx, next->servo_idx);
			machine->last_keyframe_millis = get_msec();
			machine->keyframe_idx++;
		}
	}
	return 0;
}

uint8_t get_criteria_triggered(state_machine *machine, state_transition *transition) {
	switch (transition->trigger_type) {
	case TRIGGER_BUTTON:
		return transition->trigger_param1 < 16 && button_states[transition->trigger_param1];
	case TRIGGER_TIMER:
		return get_msec() - machine->timer_start_millis >= transition->trigger_param1;
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
	machine->states[machine->state_idx].state_flags |= 0b01;
	machine->current_keyframes = &transition->transition;
	machine->keyframe_idx = 0;
	machine->last_keyframe_millis = get_msec();
	machine->timer_start_millis = get_msec();
	return 1;
}

void reset_states(void) {
	oxidizer_side.state_idx = 0;
	oxidizer_side.keyframe_idx = 16;
	fuel_side.state_idx = 0;
	fuel_side.keyframe_idx = 16;
}

void set_fuel_states_data(void) {
	
}