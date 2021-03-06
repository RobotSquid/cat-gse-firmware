/*
 * sensors.h
 *
 * Created: 2021/11/18 12:05:02
 *  Author: tianc
 */ 


#ifndef SENSORS_H_
#define SENSORS_H_

#include <asf.h>

#define SENSOR_TEMP_KTYPE  0x10
#define SENSOR_PRESS_100MV_1KPSI  0x20

typedef struct sensor_data {
	double sensors[6];
} sensor_data;

typedef struct sensor_types {
	uint8_t type[6];
} sensor_types;

typedef struct servo_data {
	uint16_t servos[16];
} servo_data;

void initialize_devices(void);
void update_sensor_data(void);
void push_servo_data(void);

double get_pressure(uint8_t sensor);
double get_temperature(uint8_t sensor);

uint8_t* get_pressure_data(void);
uint8_t* get_temperature_data(void);
uint8_t* get_valve_data(void);
uint8_t* get_cell_data(void);

double convert_reading(double sensor_reading, uint8_t sensor_type);

void set_pressure_types(uint8_t* data);
void set_temperature_types(uint8_t* data);

void ads1015_start_conversion(uint8_t chip, uint8_t mux);
double ads1015_read_conversion(uint8_t chip, uint8_t mux);

double ads1118_read_and_start_conversion(uint8_t chip, uint8_t mux);

double hx711_read_cell(void);

void pca9685_init(void);
void pca9685_write_servos(uint8_t start, uint8_t count);

void set_servo_data(uint8_t* data);
void set_servo(uint8_t servo, uint16_t position);
void set_spark_enabled(uint8_t enabled);

uint32_t get_msec(void);

#endif /* SENSORS_H_ */