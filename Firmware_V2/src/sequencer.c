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
state_machine fuel_size;

