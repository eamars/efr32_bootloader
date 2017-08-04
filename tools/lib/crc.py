def dscrc8_byte(crc, value):
	crc = crc ^ value
	for i in range(8):
		if crc & 0x01:
			crc = (crc >> 1) ^ 0x8c
		else:
			crc >>= 1
	return crc
