/**
 * @file communication.h
 * @brief Communication protocol implementation
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 *
 * Support bi-directional communication between host and hatch
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include <stdint.h>
#include <stdint.h>

#include "sip.h"
#include "io_device.h"

#define PACKET_MIN_PAYLOAD_LEN

struct communication_t;

typedef void (*comm_cb_data_f) (uint16_t, uint8_t *, uint8_t);
typedef void (*comm_cb_inst_f) (const void *, uint8_t *, uint8_t);

typedef struct
{
	io_device * serial_device;	/// io device handler
	sip_t sip;					/// serial interface protocol handler
	comm_cb_data_f callback_data;	/// callback function when receiving data
	comm_cb_inst_f callback_inst;	/// callback function when reciving instructions
} communication_t;

typedef enum
{
	COMM_DATA = 0x0,
	COMM_ACK = 0x1,
	COMM_CMD = 0x2
} communication_command_t;

// initialize communication module
void communication_init(communication_t * handler, io_device * device, comm_cb_data_f data, comm_cb_inst_f inst);

// module cleanup (not implemented)
void communication_cleanup(communication_t * handler);

// wait unitl serial port is ready
bool communication_ready(communication_t * handler);

// receive byte from serial port
void communication_receive(communication_t * handler);

#endif
