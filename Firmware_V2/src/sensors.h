/*
 * sensors.h
 *
 * Created: 2021/11/18 12:05:02
 *  Author: tianc
 */ 


#ifndef SENSORS_H_
#define SENSORS_H_

#include <asf.h>

void initialize_sensors(void);
void update_sensor_data(void);

double get_pressure(uint8_t sensor);
double get_temperature(uint8_t sensor);

uint8_t* get_pressure_data(void);
uint8_t* get_temperature_data(void);

void ads1015_start_conversion(uint8_t chip, uint8_t mux);
double ads1015_read_conversion(uint8_t chip, uint8_t mux);

#endif /* SENSORS_H_ */