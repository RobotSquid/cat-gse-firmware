/*
 * slip.h
 *
 * Created: 2021/11/16 15:09:47
 *  Author: tianc
 */ 


#ifndef SLIP_H_
#define SLIP_H_

#include <asf.h>

uint32_t udi_cdc_read_slip_packet(uint8_t* buf, uint32_t* pos);
void udi_cdc_write_slip_packet(uint8_t* buf, uint32_t len);

#endif /* SLIP_H_ */