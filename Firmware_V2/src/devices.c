/*
 * sensors.c
 *
 * Created: 2021/11/18 12:04:47
 *  Author: tianc
 */ 

#include "devices.h"

sensor_data pressure_data;
sensor_types pressure_types;
sensor_data temperature_data;
sensor_types temperature_types;
servo_data servos_data;
servo_data prev_servos_data;
double load_cell_data;
uint32_t last_servo_update[16];

uint8_t servo_i2c_addr = 0b1000001;
uint8_t servo_enable_pin = PIN_PA00;
uint8_t spark_enable_pin = PIN_PA07;
uint8_t load_cell_clk_pin = PIN_PA15;
uint8_t load_cell_data_pin = PIN_PA24;
uint8_t pressure_i2c_addr[3] = {0b1001000, 0b1001001, 0b1001010};
uint8_t temperature_spi_ss[3] = {PIN_PA04, PIN_PA05, PIN_PA11};

struct i2c_master_module i2c_master;
struct i2c_master_packet i2c_packet;
uint8_t i2c_buffer[80];

struct spi_module spi_master;
struct spi_slave_inst spi_slaves[3];
uint8_t spi_tx_buffer[2];
uint8_t spi_rx_buffer[2];

uint32_t msec = 0;

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
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(servo_enable_pin, &config_port_pin);
	port_pin_set_config(spark_enable_pin, &config_port_pin);
	set_spark_enabled(0);
	
	// configure HX711
	port_pin_set_config(load_cell_clk_pin, &config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(load_cell_data_pin, &config_port_pin);
}

// TODO: implement conversions
double convert_reading(double sensor_reading, uint8_t sensor_type) {
	switch (sensor_type) {
	case SENSOR_TEMP_KTYPE:
		if (sensor_reading > 2) return 295;
		return 295+sensor_reading*24390.24;
	case SENSOR_PRESS_100MV_1KPSI:
		return sensor_reading*68947.57;
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
	load_cell_data = hx711_read_cell();
}

void push_servo_data(void) {
	pca9685_write_servos(0, 16);
}

void ads1015_start_conversion(uint8_t chip, uint8_t mux) {
	i2c_packet.address = pressure_i2c_addr[chip];
	i2c_buffer[0] = 0b00000001; // point to config register
	i2c_buffer[1] = 0b10001111; // single shot, FSR=0.256V, set mux
	if (mux == 1) i2c_buffer[1] += 0b00110000;
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
	return (0.256*(double)result)/0x8000;
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
	uint32_t current_time = get_msec();
	i2c_packet.address = servo_i2c_addr;
	i2c_packet.data_length = count*4+1;
	i2c_buffer[0] = 0x6 + 4*start; //LEDx_ON_L register for start, auto increments
	uint8_t enable_flag = 0;
	for (uint8_t i = 0; i < count; i++) {
		i2c_buffer[i*4+1] = 0x0; //LEDx_ON_L
		i2c_buffer[i*4+2] = 0x0; //LEDx_ON_H
		uint16_t corrected_data = servos_data.servos[i];
		if (current_time - last_servo_update[i] > 1200) {
			if ((corrected_data > prev_servos_data.servos[i]) && (corrected_data-prev_servos_data.servos[i] > 99)) {
				corrected_data -= 50;
			} else if ((corrected_data < prev_servos_data.servos[i]) && (prev_servos_data.servos[i]-corrected_data > 99)) {
				corrected_data += 50;
			}
		}
		uint32_t dat = ((uint32_t)corrected_data*4096)/20000;
		i2c_buffer[i*4+3] = (uint8_t)(dat & 0xFF); //LEDx_OFF_L
		i2c_buffer[i*4+4] = (uint8_t)((dat & 0xF00) >> 8); //LEDx_OFF_H
		if (servos_data.servos[i] == 0 || (current_time - last_servo_update[i]) > 1400) {
			i2c_buffer[i*4+4] += 0b00010000; 
		} else {
			enable_flag = 1;
		}
	}
	port_pin_set_output_level(servo_enable_pin, enable_flag);
	i2c_master_write_packet_wait(&i2c_master, &i2c_packet);
}

double hx711_read_cell(void) {
	int32_t raw_data = 0;
	for (uint8_t i = 0; i < 24; i++) {
		port_pin_set_output_level(load_cell_clk_pin, 1);
		port_pin_set_output_level(load_cell_clk_pin, 0);
		uint8_t bit = port_pin_get_input_level(load_cell_data_pin);
		raw_data |= ((uint32_t)(bit) << (31-i));
	}
	port_pin_set_output_level(load_cell_clk_pin, 1); // input A, 128x
	port_pin_set_output_level(load_cell_clk_pin, 0);
	raw_data >>= 8;
	return (double)raw_data;
} 

void set_spark_enabled(uint8_t enabled) {
	port_pin_set_output_level(spark_enable_pin, enabled);
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

uint8_t* get_cell_data(void) {
	return (uint8_t*)&load_cell_data;
}

void set_servo_data(uint8_t* data) {
	for (int i = 0; i < 16; i++) {
		prev_servos_data.servos[i] = servos_data.servos[i];
		last_servo_update[i] = get_msec();
	}
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
	prev_servos_data.servos[servo] = servos_data.servos[servo];
	last_servo_update[servo] = get_msec();
	servos_data.servos[servo] = position;
}

void SysTick_Handler(void) {
	msec++;
}

uint32_t get_msec(void) {
	if (SysTick_Config(system_cpu_clock_get_hz()/1000) == 0) return msec;
	return 0;
}