/*
=== Frame details ===
ID:     0x513 (for the first 4 sensors, then 0x514 etc.)
DLC:    0x8 (bytes)
Byte 0: Sensor 0, least significant byte
Byte 1: Sensor 0, most significant byte
Byte 2: Sensor 1, least significant byte
Byte 3: Sensor 1, most significant byte
Byte 4: Sensor 2, least significant byte
Byte 5: Sensor 2, most significant byte
Byte 6: Sensor 3, least significant byte
Byte 7: Sensor 3, most significant byte

All readings are in Pascals, with an offset of 70000.

e.g. if you get the following data for a sensor:
Least significant byte: 0x30
Most significant byte:  0x75
1) Reassemble them into a 16-bit value: 0x7530
2) Convert to decimal: 0x7530 = 30000
3) Add the offset to get the pressure: 30000 + 70000 = 100000
*/


// This is subtracted from the pressure so we can pack it into two bytes.
const uint32_t CAN_FRAME_PRESSURE_OFFSET = 70000; 


struct can_frame pressure_frame;


void init_pressure_frame(uint32_t can_id){
	pressure_frame.can_id = can_id;
	pressure_frame.can_dlc = 8;
	pressure_frame.data[0] = 0x0;
	pressure_frame.data[1] = 0x0;
	pressure_frame.data[2] = 0x0;
	pressure_frame.data[3] = 0x0;
	pressure_frame.data[4] = 0x0;
	pressure_frame.data[5] = 0x0;
	pressure_frame.data[6] = 0x0;
	pressure_frame.data[7] = 0x0;
}


void add_pressure(uint32_t pressure, uint8_t pos){
	// Max 4 pressures per frame (two bytes each).
	// Pressures are stored with the least significant byte first, and the more significant byte second.
	// Note that 'pos' is the pressure index (i.e. 0-3), not the start byte!
	if (pos > 3) return;
	
	// Offset the pressure so it fits into two bytes
	uint16_t under_pressure = 0; // Values below CAN_FRAME_PRESSURE_OFFSET are capped at 0
	if (pressure > CAN_FRAME_PRESSURE_OFFSET){
		pressure -= CAN_FRAME_PRESSURE_OFFSET;
		if (pressure <= 65535){
			under_pressure = (uint16_t)pressure;
		}
		else{
			under_pressure = 65535;  // Values above CAN_FRAME_PRESSURE_OFFSET + 65535 are capped at 65535
		}
	}

	// Load it into the CAN frame
	pressure_frame.data[pos*2] = under_pressure & 0xFF;
	pressure_frame.data[(pos*2)+1] = (under_pressure >> 8) & 0xFF;
}


void send_pressure_frame(MCP2515 *can_module){
	can_module->sendMessage(&pressure_frame);
}
