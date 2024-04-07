#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nano_bcn_rgn2.h"
#include "nano_bcn_rgn_private.h"

#define REGION2_CMD_BYTES 256

typedef struct region_two_data_s {
	int pos;
	uint16_t size;
	uint16_t trig_bitmap;
	uint8_t data[REGION2_CMD_BYTES];
}region_two_data_t;

static region_two_data_t region_two_data;

void bcn_rgn2_data_reset(void)
{
	memset(&region_two_data, 0, sizeof(region_two_data_t));
	region_two_data.size = (REGION2_CMD_BYTES >> 1) + 1;
	bcn_rgn2_reg_write(0x3480, 0x2010000, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x3484, 0x3030002, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x34c8, 0x102, SIZE_HWORD, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x3494, 0x10020018, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x1084, 0x00, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x1320, 0x10101, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x11C8, 0x00, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x1c44, 0xC6, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x1ad0, 0x58, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x1d04, 0x731, SIZE_HWORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x3240, 0x42, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x3244, 0x43, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x3500, 0x03, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x32e8, 0xffffffff, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
}

uint16_t bcn_rgn2_get_size(void)
{
	return region_two_data.size;
}

int bcn_rgn2_load_to_ram(void)
{
	int i, ret = 0;
	uint32_t value;
	uint16_t mem_address = 0x40;
	//value = region_two_data.size;
	value = ((region_two_data.pos + 2) >> 1) + 1;
	ret += nano_bcn_write_mem(mem_address++, value, NULL);

	value = region_two_data.trig_bitmap | (region_two_data.data[0] << 16) | (region_two_data.data[1] << 24);
	ret += nano_bcn_write_mem(mem_address++, value, NULL);
	int bytes = ((REGION2_CMD_BYTES - 2) / 4) * 4 + 2;
	for (i = 2; i < bytes;) {
		value = region_two_data.data[i] | (region_two_data.data[i + 1] << 8) | (region_two_data.data[i + 2] << 16) | (region_two_data.data[i + 3] << 24);
		ret += nano_bcn_write_mem(mem_address++, value, NULL);
		i += 4;
	}
	value = 0;
	for (int j = 0; j < (REGION2_CMD_BYTES - i); j++)
	{
		value |= (region_two_data.data[i + j] << (j * 8));
	}
	ret += nano_bcn_write_mem(mem_address++, value, NULL);
	return ret;
}

int bcn_rgn2_brun(void)
{
	int ret = 0;
	uint8_t addr = 74;
	uint16_t value;
    value = ((region_two_data.pos + 2) >> 1) + 1;
	ret = nano_bcn_write_efuse(addr++, value, NULL);
	ret += nano_bcn_write_efuse(addr++, region_two_data.trig_bitmap, NULL);
	for (int i = 0; i < REGION2_CMD_BYTES;) {
		value = region_two_data.data[i] | (region_two_data.data[i + 1] << 8);
		ret += nano_bcn_write_efuse(addr++, value, NULL);
		i += 2;
	}
	return ret;
}

void bcn_rgn2_delay(uint32_t delay, boot_type_t coldwarm, trig_type_t trigger)
{
	if ((region_two_data.pos + 4) < REGION2_CMD_BYTES) {
		region_two_data.trig_bitmap |= (1 << trigger);
		region_two_data.data[region_two_data.pos++] = 1 | (coldwarm << 2) | (trigger << 4);
		region_two_data.data[region_two_data.pos++] = delay & 0xff;
		region_two_data.data[region_two_data.pos++] = (delay >> 8) & 0xff;
		region_two_data.data[region_two_data.pos++] = (delay >> 16) & 0xff;
	}
}

void bcn_rgn2_reg_write(uint16_t address, uint32_t value, size_type_t size, boot_type_t coldwarm, trig_type_t trigger)
{
	if ((region_two_data.pos + 7) < REGION2_CMD_BYTES) {
		region_two_data.trig_bitmap |= (1 << trigger);
		region_two_data.data[region_two_data.pos++] = 2 | (coldwarm << 2) | (trigger << 4);
		region_two_data.data[region_two_data.pos++] = size | ((address & 0x3f) << 2);
		region_two_data.data[region_two_data.pos++] = (address >> 6) & 0xff;
		region_two_data.data[region_two_data.pos++] = value & 0xff;
		if (size > SIZE_BYTE) {
			region_two_data.data[region_two_data.pos++] = (value >> 8) & 0xff;
		}
		if (size > SIZE_HWORD) {
			region_two_data.data[region_two_data.pos++] = (value >> 16) & 0xff;
			region_two_data.data[region_two_data.pos++] = (value >> 24) & 0xff;
		}
	}
	
}

void bcn_rgn2_reg_read_write(uint16_t address, uint32_t value, uint32_t mask, size_type_t size, boot_type_t coldwarm, trig_type_t trigger)
{
	if ((region_two_data.pos + 11) < REGION2_CMD_BYTES) {
		region_two_data.trig_bitmap |= (1 << trigger);
		region_two_data.data[region_two_data.pos++] = 3 | (coldwarm << 2) | (trigger << 4);
		region_two_data.data[region_two_data.pos++] = size | ((address & 0x3f) << 2);
		region_two_data.data[region_two_data.pos++] = (address >> 6) & 0xff;
		region_two_data.data[region_two_data.pos++] = mask & 0xff;
		if (size > SIZE_BYTE) {
			region_two_data.data[region_two_data.pos++] = (mask >> 8) & 0xff;
		}
		if (size > SIZE_HWORD) {
			region_two_data.data[region_two_data.pos++] = (mask >> 16) & 0xff;
			region_two_data.data[region_two_data.pos++] = (mask >> 24) & 0xff;
		}
		region_two_data.data[region_two_data.pos++] = value & 0xff;
		if (size > SIZE_BYTE) {
			region_two_data.data[region_two_data.pos++] = (value >> 8) & 0xff;
		}
		if (size > SIZE_HWORD) {
			region_two_data.data[region_two_data.pos++] = (value >> 16) & 0xff;
			region_two_data.data[region_two_data.pos++] = (value >> 24) & 0xff;
		}
	}
}

