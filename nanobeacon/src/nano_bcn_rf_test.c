#include <stdlib.h>
#include <string.h>

#include "nano_bcn_rf_test.h"
#include "nano_bcn_serial_protocol.h"

static const uint8_t TX_POWER_TAB[] = { 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -7, -9, -11, -13, -16, -20, -30, -54 };
static const uint8_t PA_GAVIN_TAB[] = { 0x78, 0x78, 0x54, 0x3a, 0x2e, 0x24, 0x1e, 0x19, 0x16, 0x13, 0x10, 0x0e, 0x0b, 8, 7, 5, 3, 1, 0 };

static uint8_t _get_pa_gain(int8_t tx_power)
{
	for (int i = 0; i < sizeof(TX_POWER_TAB); i++)
	{
		if ((tx_power == TX_POWER_TAB[i]) || (tx_power > TX_POWER_TAB[i])) {
			return PA_GAVIN_TAB[i];
		}
	}
	return PA_GAVIN_TAB[5];
}

static int _ldo_calibration(int8_t tx_power)
{
	uint16_t efuse05;
	uint32_t val, dig_ldo_val;
	uint32_t val_tmp;
	uint8_t rf_ldo_code;
	uint8_t rf_ldo_cal;
	uint8_t rf_ldo_cal_present;
	uint8_t dig_ldo_cal;
	uint8_t dig_ldo_cal_present;
	int ret = nano_bcn_read_efuse(05, &efuse05);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	rf_ldo_cal = (efuse05 >> 6) & 0x0f;
	rf_ldo_cal_present = (efuse05 >> 10) & 0x01;
	dig_ldo_cal = (efuse05 >> 11) & 0x0f;
	dig_ldo_cal_present = (efuse05 >> 15) & 0x01;

	if (0 == rf_ldo_cal) {
		rf_ldo_cal = 9;
	}
	if (0 == dig_ldo_cal) {
		dig_ldo_cal = 9;
	}
	if (tx_power > 4) {
		rf_ldo_code = 15;
	}
	else if (tx_power > 2) {
		rf_ldo_code = rf_ldo_cal + 1;
	}
	else {
		rf_ldo_code = rf_ldo_cal;
	}
	val = rf_ldo_code;
	if (val > 0x0f) {
		val = 0x0f;
	}
	val = 0x300 | (val << 4) | val;
	if (dig_ldo_cal > 2) {
		dig_ldo_val = dig_ldo_cal - 2;
	}
	else {
		dig_ldo_val = 0;
	}
	dig_ldo_val = (dig_ldo_val << 16) | (1 << 20);
	val = val | dig_ldo_val;
	ret = nano_bcn_write_reg(0x3254, val, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	if (rf_ldo_cal > 1) {
		val = rf_ldo_cal - 1;
	}
	else {
		val = 0;
	}
	val = val << 24;
	ret = nano_bcn_read_reg(0x3244, &val_tmp);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	val_tmp &= (~0xf000000);
	val = val | val_tmp;
	ret = nano_bcn_write_reg(0x3244, val, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	
	if (dig_ldo_cal > 3) {
		dig_ldo_val = dig_ldo_cal - 3;
	}
	else {
		dig_ldo_val = 0;
	}
	val = dig_ldo_val << 16;
	ret = nano_bcn_read_reg(0x324C, &val_tmp);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	val_tmp &= (~0xf0000);
	val = val | val_tmp;
	ret = nano_bcn_write_reg(0x324C, val, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
}

/*
*  tx_pattern[PRBS9, 00001111, 01010101, PRBS15, 11111111, 00000000, 11110000, 10101010]
*/


int dtm_start(uint8_t ch, bool is_125k, uint8_t tx_len, uint8_t tx_pattern, uint8_t cap_code, int8_t tx_power)
{
	uint16_t efuse05;
	uint32_t value;
	uint32_t val_tmp;
	uint8_t pa_gain = _get_pa_gain(tx_power);
	int ret = _ldo_calibration(tx_power);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x3320, 0x1000, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x331c, cap_code, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1d10, 0x1699c, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1c44, 0x800e00c6, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1ad0, 0x258, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1d04, 0x941, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_read_reg(0x1c90, &val_tmp);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	value = (val_tmp & 0xffffff00) | pa_gain;
	ret = nano_bcn_write_reg(0x1c90, value, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x3240, 0x8f66114a, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x2c0, 0x3100, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}

	value = (tx_len << 8) | (tx_pattern & 0x7);
	ret = nano_bcn_write_reg(0x2f8, value, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
		
	value = 1;
	value |= ((tx_pattern & 0x7) << 4);
	value |= (tx_len << 8);
	value |= ((ch & 0x3F) << 20);
	if (0 == tx_len)
	{
		value |= (1 << 16); //infinite TX
	}
	if (is_125k)
		value |= (3 << 28);
	ret = nano_bcn_write_reg(0x2e8, value, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x244, 0, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x248, 0, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	return ret;
}

int dtm_stop(void)
{
	int ret = nano_bcn_write_reg(0x2e8, 0, NULL);
	return ret;
}

int carrier_test_start(uint8_t ch, uint8_t cap_code, int8_t tx_power)
{
	uint16_t efuse05;
	uint32_t value;
	uint32_t val_tmp;
	uint8_t pa_gain = _get_pa_gain(tx_power);
	int ret = _ldo_calibration(tx_power);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x3320, 0x1000, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x331c, cap_code, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1d10, 0, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1c44, 0x800e00c6, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1ad0, 0x258, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x1d04, 0x731, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_read_reg(0x1c90, &val_tmp);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	value = (val_tmp & 0xffffff00) | pa_gain;
	ret = nano_bcn_write_reg(0x1c90, value, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x3240, 0x8f66114a, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x2c0, 0x3100, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	
	ret = nano_bcn_write_reg(0x2f8, 0, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}

	value = 1;
	value |= ((ch & 0x3F) << 20);
	value |= (1 << 16);
	ret = nano_bcn_write_reg(0x2e8, value, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x244, 1, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x248, 1, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	return ret;
}

int carrier_test_stop(void)
{
	int ret = nano_bcn_write_reg(0x244, 0, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}

	ret = nano_bcn_write_reg(0x248, 0, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	ret = nano_bcn_write_reg(0x2e8, 0, NULL);
	if (NANO_BCN_ERR_NO_ERROR != ret)
	{
		return ret;
	}
	return ret;
}