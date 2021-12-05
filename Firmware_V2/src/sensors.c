/*
 * sensors.c
 *
 * Created: 2021/11/18 12:04:47
 *  Author: tianc
 */ 

#include "sensors.h"

typedef struct sensor_data {
	double sensors[6];
} sensor_data;

sensor_data pressure_data;
sensor_data temperature_data;

uint8_t pressure_i2c_addr[3] = {0b1001000, 0b1001001, 0b1001010};

struct i2c_master_module i2c_master;
struct i2c_master_packet i2c_packet;
uint8_t i2c_buffer[4];
struct spi_module spi_master;


void initialize_sensors(void) {
	// configure i2c
	struct i2c_master_config i2c_config1;
	i2c_master_get_config_defaults(&i2c_config1);
	i2c_config1.pinmux_pad0 = PINMUX_PA08C_SERCOM0_PAD0;
	i2c_config1.pinmux_pad1 = PINMUX_PA09C_SERCOM0_PAD1;
	i2c_config1.baud_rate = 400;
	i2c_master_init(&i2c_master, SERCOM0, &i2c_config1);
	i2c_master_enable(&i2c_master);
	i2c_packet.data = i2c_buffer;
	// configure spi
	struct spi_config spi_config1;
	spi_get_config_defaults(&spi_config1);
	spi_config1.mux_setting = SPI_SIGNAL_MUX_SETTING_I; // MISO PAD0, SCK PAD1, MOSI PAD3
	spi_config1.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	spi_config1.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	spi_config1.pinmux_pad2 = PINMUX_UNUSED;
	spi_config1.pinmux_pad3 = PINMUX_PA19C_SERCOM1_PAD3;
	spi_init(&spi_master, SERCOM1, &spi_config1);
	spi_enable(&spi_master);
}

void update_sensor_data(void) {
	// read pressures at 0 mux and start conversions for 1 mux
	for (uint8_t i = 0; i < 3; i++) {
		pressure_data.sensors[i*2] = ads1015_read_conversion(i, 0);
		ads1015_start_conversion(i, 1);
	}
	// takes roughly 1ms to do the above, so the conversion should be ready
	// read pressures at 1 mux and start conversions for 0 mux
	for (uint8_t i = 0; i < 3; i++) {
		pressure_data.sensors[i*2+1] = ads1015_read_conversion(i, 1);
		ads1015_start_conversion(i, 0);
	}
}

void ads1015_start_conversion(uint8_t chip, uint8_t mux) {
	i2c_packet.address = pressure_i2c_addr[chip];
	i2c_buffer[0] = 0b00000001; // point to config register
	i2c_buffer[1] = 0b10000101; // single shot, FSR=2.048V, set mux
	if (mux == 1) i2c_buffer[1] += 0b011 << 4;
	i2c_buffer[2] = 0b11100000; // 3300 SPS, default comparator settings
	i2c_packet.data_length = 3;
	i2c_master_write_packet_wait(&i2c_master, &i2c_packet);
}

double ads1015_read_conversion(uint8_t chip, uint8_t mux) {
	i2c_packet.address = pressure_i2c_addr[chip];
	i2c_buffer[0] = 0b00000000; // point to conversion register
	i2c_packet.data_length = 1;
	i2c_master_write_packet_wait(&i2c_master, &i2c_packet);
	i2c_packet.data_length = 2;
	i2c_master_read_packet_wait(&i2c_master, &i2c_packet);
	int16_t result = ((int16_t)i2c_buffer[0] << 8) + i2c_buffer[1];
	return (2.048*(double)result)/0x8000;
}

double get_pressure(uint8_t sensor) {
	return pressure_data.sensors[sensor];
}

double get_temperature(uint8_t sensor) {
	return temperature_data.sensors[sensor];
}

uint8_t* get_pressure_data(void) {
	return (uint8_t*)&pressure_data;
}

uint8_t* get_temperature_data(void) {
	return (uint8_t*)&temperature_data;
}