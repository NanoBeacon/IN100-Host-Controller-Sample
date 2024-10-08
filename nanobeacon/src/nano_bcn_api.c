#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nano_bcn_api.h"

/**
 * Usage:   Used to register host interface function and init
 * Params:  p_hif     host interface function
 * Return:  0 indicates success, others indicate failure
 */
int nano_bcn_init(host_itf_t *p_hif)
{
	return nano_bcn_uart_init(p_hif);
}

void nano_bcn_deinit(void)
{
	nano_bcn_uart_deinit();

}

/**
 * Usage:  Used to set initial advertising parameters
 * Params: capacitor    on-chip crystal oscillator , 1 ~ 15	   
 *         xo_stable_time  25 ~ 255 ,cycles
 *         xo_strength  0 ~ 31
 *         tx_power    transmission power , -5 ~ 5 dBm
 * Return: 0 indicates success, others indicate failure
 */
int nano_bcn_board_setup(uint8_t capacitor, uint8_t xo_stable_time, uint8_t xo_strength, int8_t tx_power)
{
	uint16_t efuse_val;
	bcn_rgn1_data_reset();
	bcn_rgn2_data_reset();
	bcn_adv_region_data_reset();
	bcn_xo_settings(capacitor, xo_stable_time, xo_strength);
	efuse_val = 0;
	nano_bcn_read_efuse(05, &efuse_val);
	bcn_tx_power_set(tx_power, efuse_val);
	return 0;
}

/**
 * Usage:   Used to set advertising data
 * Params:  bdaddr		bluetooth device address , 6 bytes. cannot be all 0 or all 1
 *		    interval    broadcast interval, millisecond
 *			adv_raw_data  advertising raw data, max length is 31 bytes
 *			len		length of adv_raw_data
 * Return: 0 indicates success, others indicate failure
 */
int nano_bcn_set_advertising(const uint8_t* bdaddr, int interval, uint8_t* adv_raw_data, int len)
{
	advset_index_t idx = ADV_SET_1;
	bcn_adv_tx_set(idx, interval, ADV_CH_37_38_39, PHY_RATE_1M, ADV_MODE_CONTINUOUS);
	bcn_adv_address_set(idx, ADDR_PUBLIC, bdaddr, 60, 0);
	bcn_adv_data_add_predefined_data(idx, adv_raw_data, len, 0);
	return 0;
}

static int _load_adv_data_to_ram(void)
{
	int i, ret, size;
	uint32_t value;
	uint16_t mem_address = 0x100;
	uint8_t* buffer = malloc(512);
	/*setup region 3*/
	if (buffer)
	{
		size = bcn_adv_data_to_binary(buffer, 512);
		value = buffer[0] | (buffer[1] << 8);
		ret = nano_bcn_write_mem(mem_address++, value, NULL);
		for (i = 2; i < size;) {
			value = buffer[i] | (buffer[i + 1] << 8) | (buffer[i + 2] << 16) | (buffer[i + 3] << 24);
			ret = nano_bcn_write_mem(mem_address++, value, NULL);
			i += 4;
		}
		free(buffer);
	}
	else
	{
		ret = -1;
	}
	return ret;
}

/**
 * Usage:   Used to load raw data into SDRAM
 * Params:
 * Return: 0 indicates success, others indicate failure
 */
int nano_bcn_load_data_to_ram(void)
{
	int ret;
	bcn_rgn1_set_region23_from_ram();
	ret = bcn_rgn1_load_to_ram();
	if (0 != ret) {
		return ret;
	}
	ret = bcn_rgn2_load_to_ram();
	ret = _load_adv_data_to_ram();
	return ret;
}

/**
 * Usage:   Used to start advertising
 * Params:
 * Return:	0 indicates success, others indicate failure
 */
int nano_bcn_start_advertising()
{
	int ret;
	uint32_t value;
	ret = nano_bcn_write_reg(0x3394, 0x100, NULL);
	ret += nano_bcn_write_reg(0x3390, 0x777, NULL);
	if (0 == nano_bcn_read_reg(0x10b0, &value)){
		value = value | 0x10;
		value = value & 0xfffffffc;
		ret += nano_bcn_write_reg(0x10b0, value, NULL);
	}
	else {
		ret += 1;
	}
    if (0 == nano_bcn_read_reg(0x10e0, &value)){
        if (bcn_rgn1_is_sleep_aft_tx())
        {
            value = value | (1 << 25);
            nano_bcn_write_mem(0x20, value, NULL);
            nano_bcn_write_reg(0x10e0, value, NULL);
        }        
    }
    return ret;
}