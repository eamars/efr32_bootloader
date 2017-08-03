/**
 * @file communication.c
 * @brief Communication protocol implementation
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 *
 * Support bi-directional communication between host and hatch
 */
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "communication.h"
#include "io_device.h"
#include "sip.h"
#include "convert.h"
#include "dscrc8.h"
#include "io_device.h"
#include "debug.h"


/**
 * Private: generate response for data packet
 * @param handler communication handler for accessing internal properties
 * @param ack_no  packet number to acknowledge
 * @param crc     crc of previous packet
 */
static void communication_response(communication_t *handler, uint16_t ack_no, crc8_t crc)
{
	io_device *serial_device = handler->serial_device;
	sip_t *sip = &(handler->sip);

	// set command
	sip_set(sip, SIP_CMD_S, COMM_ACK);

	// set payload_length
	sip_set(sip, SIP_PAYLOAD_LENGTH_S, 0x05);	///< 3 bytes payload according to specification

	// include 2 byte acknowledgement number
	uint8_t buffer[2];
	uint16_to_uint8(&ack_no, buffer);	///< caution: depending on the architecture, the byte order may change

	// set two byte of the payload (unfoldded)
	sip_set(sip, SIP_PAYLOAD_0_S, buffer[0]);
	sip_set(sip, SIP_PAYLOAD_1_S, buffer[1]);

	// include 2 byte rx availability
	int rx_window = handler->serial_device->rx_window(handler->serial_device->device);
	uint16_t rx_fifo_available = rx_window > 0 ? (uint16_t) rx_window : (uint16_t) 1;
	uint16_to_uint8(&rx_fifo_available, buffer);
	sip_set(sip, SIP_PAYLOAD_2_S, buffer[0]);
	sip_set(sip, SIP_PAYLOAD_3_S, buffer[1]);

	// set crc
	sip_set(sip, SIP_PAYLOAD_4_S, crc);

	// get raw pointer
	uint8_t *raw_ptr = sip_get_raw_ptr(sip);

	// send over io device
	for (uint8_t i = 0; i < SIP_FIXED_OVERHEAD + 10; i++)	///< 4 prefix characters + 5*2 payload characters
	{
		serial_device->put(serial_device->device, *(raw_ptr + i));
	}

	// send last terminator
	serial_device->put(serial_device->device, '>');

	// send message back immediately
	serial_device->flush(serial_device->device);
}

/**
 * Private: callback function for handling incoming data packet
 * @param args communication handler (need to cast)
 */
static void sip_data_cb(void *args)
{
	communication_t *handler = (communication_t *) args;
	sip_t *sip = &(handler->sip);
	crc8_t crc8 = 0;
	uint16_t packet_no = 0;
	uint16_t block_offset = 0;
	uint8_t payload[SIP_PAYLOAD_LENGTH];
	uint8_t payload_length;

	// if the payload is too short, currently less than 3 bytes, the packet
	// will be discarded without notification (no error message generated)
	payload_length = sip_get(sip, SIP_PAYLOAD_LENGTH_S);
	if (payload_length < 3)
	{
		return;
	}

	// decode payload
	for (uint8_t i = SIP_PAYLOAD_0_S; i < payload_length; i++)
	{
		// extract bytes
		payload[i] = sip_get(sip, (sip_symbol_t) i);

		// calculate crc
		crc8 = dscrc8_byte(crc8, payload[i]);
	}

	// read current packet index from first four bytes
	// currently we are using little endian
	// TODO: depending on the architecture, the byte order may change
	uint8_to_uint16(payload, &packet_no);
	uint8_to_uint16(payload + 2, &block_offset);

	// execute callback function (trim current packet index and block offset from payload)
	handler->callback_data(block_offset, payload + 4, (uint8_t)(payload_length - 4));

	// construct response
	communication_response(handler, packet_no, crc8);
}

/**
 * Private: callback function for handling acknowledgement packet
 * @param args communication handler (need to cast)
 */
static void sip_ack_cb(void *args)
{
	(void) args;
}

/**
 * Private: callback function for handling instruction packet
 * @param args communication handler (need to cast)
 */
static void sip_inst_cb(void *args)
{
	communication_t *handler = (communication_t *) args;
	sip_t *sip = &(handler->sip);
	uint16_t packet_no = 0;
	uint8_t payload_length;
	uint8_t payload[SIP_PAYLOAD_LENGTH];
	crc8_t crc8 = 0;

	// if the payload is too short, currently less than 3 bytes, the packet
	// will be discarded without notification (no error message generated)
	payload_length = sip_get(sip, SIP_PAYLOAD_LENGTH_S);
	if (payload_length < 3)
	{
		return;
	}

	// decode payload
	for (uint8_t i = SIP_PAYLOAD_0_S; i < payload_length; i++)
	{
		// extract bytes
		payload[i] = sip_get(sip, (sip_symbol_t) i);

		// calculate crc
		crc8 = dscrc8_byte(crc8, payload[i]);
	}

	// read current packet index from first four bytes
	// currently we are using little endian
	// TODO: depending on the architecture, the byte order may change
	uint8_to_uint16(payload, &packet_no);

	// execute callback function (trim current packet index from payload)
	handler->callback_inst(handler, payload + 2, (uint8_t)(payload_length - 2));

	// construct response
	communication_response(handler, packet_no, crc8);
}

/**
 * Initialize communication module
 * @param handler communication handler for accessing internal properties
 * @param device  abstraction for serial device
 * @param data    callback function for handling data packet
 * @param inst    callback function for handling instruction packet
 */
void communication_init(communication_t * handler, io_device * device, comm_cb_data_f data, comm_cb_inst_f inst)
{
	// assign device
	handler->serial_device = device;

	// initialize sip module
	sip_init(&(handler->sip));

	// register callback for comm module
	handler->callback_data = data;
	handler->callback_inst = inst;

	// register few callback functions for implementing protocols
	sip_register_cb(&(handler->sip), (uint8_t) COMM_DATA, sip_data_cb, handler);
	sip_register_cb(&(handler->sip), (uint8_t) COMM_ACK, sip_ack_cb, handler);
	sip_register_cb(&(handler->sip), (uint8_t) COMM_CMD, sip_inst_cb, handler);
}

/**
 * Cleanup the communication module (unused)
 * @param handler communication handler for accessing internal properties
 */
void communication_cleanup(communication_t * handler)
{

}

/**
 * Attempt to read from sequential device
 * @param  handler communication handler for accessing internal properties
 * @return         true if the device is ready to read, false if the device is still busy
 */
bool communication_ready(communication_t * handler)
{
	return handler->serial_device->read_ready(handler->serial_device->device);
}

/**
 * Read one character from sequential device and push to sip stack
 * @param handler communication handler for accessing internal properties
 */
void communication_receive(communication_t * handler)
{
	uint8_t byte;

	// read one byte from serial
	// the serial device must at least implement getchar function
	byte = (uint8_t) handler->serial_device->get(handler->serial_device->device);

	// push received char to sip stack
	sip_poll(&(handler->sip), byte);
}
