/*
 * sensors.c
 *
 * Created: 2021/11/18 12:04:47
 *  Author: tianc
 */ 

#include "devices.h"

#define SENSOR_TEMP_KTYPE  0x10
#define SENSOR_PRESS_TEST  0x20

typedef struct sensor_data {
	double sensors[6];
} sensor_data;

typedef struct sensor_types {
	uint8_t type[6];
} sensor_types;

typedef struct servo_data {
	uint16_t servos[16];
} servo_data;

sensor_data pressure_data;
sensor_types pressure_types;
sensor_data temperature_data;
sensor_types temperature_types;
servo_data servos_data;

uint8_t servo_i2c_addr = 0b1000001;
uint8_t pressure_i2c_addr[3] = {0b1001000, 0b1001001, 0b1001010};
uint8_t temperature_spi_ss[3] = {PIN_PA04, PIN_PA05, PIN_PA11};

struct i2c_master_module i2c_master;
struct i2c_master_packet i2c_packet;
uint8_t i2c_buffer[80];

struct spi_module spi_master;
struct spi_slave_inst spi_slaves[3];
uint8_t spi_tx_buffer[2];
uint8_t spi_rx_buffer[2];

void initialize_devices(void) {
	// configure I2C
	struct i2c_master_config i2c_config1;
	i2c_master_get_config_defaults(&i2c_config1);
	i2c_config1.pinmux_pad0 = PINMUX_PA08C_SERCOM0_PAD0;
	i2c_config1.pinmux_pad1 = PINMUX_PA09C_SERCOM0_PAD1;
	i2c_config1.baud_rate = 400;
	i2c_master_init(&i2c_master, SERCOM0, &i2c_config1);
	i2c_master_enable(&i2c_master);
	i2c_packet.data = i2c_buffer;
	
	// configure SPI
	struct spi_config spi_config1;
	struct spi_slave_inst_config spi_slave_config1;
	spi_slave_inst_get_config_defaults(&spi_slave_config1);
	for (int i = 0; i < 3; i++) {
		spi_slave_config1.ss_pin = temperature_spi_ss[i];
		spi_attach_slave(&spi_slaves[i], &spi_slave_config1);
	}
	spi_get_config_defaults(&spi_config1);
	spi_config1.mux_setting = SPI_SIGNAL_MUX_SETTING_I; // MISO PAD0, SCK PAD1, MOSI PAD3
	spi_config1.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	spi_config1.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	spi_config1.pinmux_pad2 = PINMUX_UNUSED;
	spi_config1.pinmux_pad3 = PINMUX_PA19C_SERCOM1_PAD3;
	spi_config1.transfer_mode = SPI_TRANSFER_MODE_1; // CPHA 1, CPOL 0
	spi_init(&spi_master, SERCOM1, &spi_config1);
	spi_enable(&spi_master);
	
	// configure PCA9685
	pca9685_init();
}

// TODO: implement conversions
double convert_reading(double sensor_reading, uint8_t sensor_type) {
	switch (sensor_type) {
	case SENSOR_TEMP_KTYPE:
		return 150;
	case SENSOR_PRESS_TEST:
		return 25;
	}
	return 0;
}

void update_sensor_data(void) {
	// read pressures, temperatures at 0 mux and start conversions for 1 mux
	for (uint8_t i = 0; i < 3; i++) {
		temperature_data.sensors[i*2] = convert_reading(ads1118_read_and_start_conversion(i, 1), temperature_types.type[i*2]);
		pressure_data.sensors[i*2] = convert_reading(ads1015_read_conversion(i, 0), pressure_types.type[i*2]);
		ads1015_start_conversion(i, 1);
	}
	// ensure conversion is ready
	delay_ms(1);
	// read pressures, temperatures at 1 mux and start conversions for 0 mux
	for (uint8_t i = 0; i < 3; i++) {
		temperature_data.sensors[i*2+1] = convert_reading(ads1118_read_and_start_conversion(i, 0), temperature_types.type[i*2+1]);
		pressure_data.sensors[i*2+1] = convert_reading(ads1015_read_conversion(i, 1), pressure_types.type[i*2+1]);
		ads1015_start_conversion(i, 0);
	}
}

void push_servo_data(void) {
	pca9685_write_servos(0, 16);
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

double ads1118_read_and_start_conversion(uint8_t chip, uint8_t mux) {
	spi_select_slave(&spi_master, &spi_slaves[chip], true);
	spi_tx_buffer[0] = 0b10000101; // single shot, FSR=2.048V, set mux
	if (mux == 1) spi_tx_buffer[0] += 0b011 << 4;
	spi_tx_buffer[1] = 0b11101010; // 860 SPS, default other settings
	spi_transceive_buffer_wait(&spi_master, spi_tx_buffer, spi_rx_buffer, 2);
	int16_t result = ((int16_t)spi_rx_buffer[0] << 8) + spi_rx_buffer[1];
	spi_select_slave(&spi_master, &spi_slaves[chip], false);
	return (2.048*(double)result)/0x8000;
}

void pca9685_init(void) {
	i2c_packet.address = servo_i2c_addr;
	i2c_packet.data_length = 2;
	i2c_buffer[0] = 0xFE; // PRE_SCALE register
	i2c_buffer[1] = 121; // 50 Hz
	i2c_master_write_packet_wait(&i2c_master, &i2c_packet);
	i2c_buffer[0] = 0x00; // MODE1 register
	i2c_buffer[1] = 0b00100000; // chip on, auto increment on
	i2c_master_write_packet_wait(&i2c_master, &i2c_packet);
}

void pca9685_write_servos(uint8_t start, uint8_t count) {
	i2c_packet.address = servo_i2c_addr;
	i2c_packet.data_length = count*4+1;
	i2c_buffer[0] = 0x6 + 4*start; //LEDx_ON_L register for start, auto increments
	for (uint8_t i = 0; i < count; i++) {
		i2c_buffer[i*4+1] = 0x0; //LEDx_ON_L
		i2c_buffer[i*4+2] = 0x0; //LEDx_ON_H
		uint32_t dat = ((uint32_t)servos_data.servos[i]*4096)/20000;
		i2c_buffer[i*4+3] = (uint8_t)(dat & 0xFF); //LEDx_OFF_L
		i2c_buffer[i*4+4] = (uint8_t)((dat & 0xF00) >> 8); //LEDx_OFF_H
		if (servos_data.servos[i] == 0) i2c_buffer[i*4+4] += 0b00010000; 
	}
	i2c_master_write_packet_wait(&i2c_master, &i2c_packet);
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

uint8_t* get_valve_data(void) {
	return (uint8_t*)&servos_data;
}

void set_servo_data(uint8_t* data) {
	for (int i = 0; i < 32; i++) {
		((uint8_t*)&servos_data)[i] = data[i];
	}
}

void set_pressure_types(uint8_t* data) {
	for (int i = 0; i < 6; i++) {
		((uint8_t*)&pressure_types)[i] = data[i];
	}
}

void set_temperature_types(uint8_t* data) {
	for (int i = 0; i < 6; i++) {
		((uint8_t*)&temperature_types)[i] = data[i];
	}
}

void set_servo(uint8_t servo, uint16_t position) {
	servos_data.servos[servo] = position;
}