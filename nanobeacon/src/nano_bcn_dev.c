#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "nano_bcn_dev.h"
#include "nano_bcn_rgn1.h"

#define LOCAL_TRACE  printf

static const uint8_t TX_POWER_TAB[] = {5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -7, -9, -11, -13, -16, -20, -30, -54};
static const uint8_t PA_GAVIN_TAB[] = { 0x78, 0x78, 0x54, 0x3a, 0x2e, 0x24, 0x1e, 0x19, 0x16, 0x13, 0x10, 0x0e, 0x0b, 8, 7, 5, 3, 1, 0 };

#if ON_CHIP_MEASUREMENT_ENABLE
static void _vcc_measurement_calibration(uint16_t efuse06_val, uint16_t efuse07_val, uint16_t efuse08_val, float unitLSB)
{
	uint16_t value3_2 = efuse06_val;
	uint16_t value1_2 = efuse08_val;
	uint16_t value2_2 = efuse07_val;
	uint8_t efuseNsamples = 32;
	uint8_t	nsamples = 16;
	uint32_t negate = 0;
	uint32_t shift_fx;
	uint32_t slope_fx;
	uint32_t nlog2_fx;
	int offset_fx;
	float rangeHigh, rangeLow, CalCodeHigh, CalCodeLow;

	if (0 == value3_2 && 0 == value1_2) {
		LOCAL_TRACE("no vcc calibration data\n");
		return;
	}
	if (0 == value3_2) {
		value3_2 = value2_2;
		rangeHigh = 2.2f;
	}
	else {
		rangeHigh = 3.2f;
	}
	if (0 == value1_2) {
		value1_2 = value2_2;
		rangeLow = 2.2f;
	}
	else {
		rangeLow = 1.2f;
	}
	if (0 == value3_2 || 0 == value1_2) {
		LOCAL_TRACE("no vcc calibration data 2\n");
		return;
	}
	
	CalCodeHigh = value3_2;
	CalCodeHigh  = CalCodeHigh / efuseNsamples;
	CalCodeLow = value1_2;
	CalCodeLow  = CalCodeLow / efuseNsamples;
	float slope = (rangeHigh - rangeLow) / (CalCodeHigh - CalCodeLow);
	float offset = (rangeHigh + rangeLow - slope * (CalCodeHigh + CalCodeLow)) / 2;
	slope = slope / unitLSB;
	offset = offset / unitLSB;
	if (slope < 0)
	{
		negate = 1;
		slope = slope * (-1);
	}
	float slope_normalized = slope / nsamples;
	float step2;
	step2 = 11 - log2(slope_normalized);
	shift_fx = floor(step2);
	slope_fx = floor(slope_normalized * pow(2, shift_fx) + 0.5);
	nlog2_fx = floor(log2(nsamples));
	offset_fx = round(offset + 0.5);
	if (offset_fx < 0) {
		offset_fx = 65536 + offset_fx;
	}
	uint32_t val = 0x0D040442;
	val = val & (~(0xf << 4));
	val = val | (nlog2_fx << 4);
	if (0x0D040442 != val) {
		bcn_rgn2_reg_write(0x750, val, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
	
	val = 0x00A1A;
	val = val & (~(0x1f << 8));
	val = val & (~(0x1 << 16));
	val = val | ((shift_fx << 8) | (negate << 16));
	if (0x00A1A != val) {
		bcn_rgn2_reg_write(0x754, val, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
	
	val = slope_fx | (offset_fx << 12) ;
	if (0x400 != val) {
		bcn_rgn2_reg_write(0x758, val, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
}

static void _onchip_temperature_calibration(uint16_t efuse09_val, float unitLSB)
{
	float scaledCalOffset;
	int iCalOffset;
	int offset_fx0;
	int shift_fx, slope_fx;
	char negate;
	double vtemp_slope = (-2.472727273);
	double vtemp_offset = 921.3181818;
	uint8_t nsamples = 16;
	uint8_t efuseNsamples = 32;
	uint32_t value;
	int expectedScaledVtempZero = (int)(vtemp_offset * nsamples + 0.5);
	iCalOffset = efuse09_val;
	if (iCalOffset > 32768) {
		iCalOffset = iCalOffset - 65536;
	}
	
	if (iCalOffset >= 0)
		scaledCalOffset = (iCalOffset * nsamples + (efuseNsamples >> 1)) / efuseNsamples;
	else
		scaledCalOffset = -1 * (-1 * iCalOffset * nsamples + (efuseNsamples >> 1)) / efuseNsamples;

	offset_fx0 = expectedScaledVtempZero + scaledCalOffset;

			
	vtemp_slope = (1 / vtemp_slope) / unitLSB;
	if (vtemp_slope < 0) {
		negate = 1;
		vtemp_slope = vtemp_slope * (-1);
	}	
	else {
		negate = 0;
	}
	vtemp_slope = vtemp_slope / nsamples;
	shift_fx = (int)(floor(11 - log2(vtemp_slope)));
	slope_fx = (int)(floor(vtemp_slope * pow(2, shift_fx) + 0.5));
	int nlog2_fx = (int)floor(log2(nsamples));
	if (offset_fx0 < 0) {
		offset_fx0 = 65536 + offset_fx0;
	}

	value = 0x0D040442;
	value = value & (~(0xf << 4));
	value = value | (nlog2_fx << 4);
	if (0x0D040442 != value) {
		bcn_rgn2_reg_write(0x730, value, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}

	value = 0x00A34;
	value = value & (~(0x1f << 8));
	value = value & (~(0x1 << 16));
	value = value | ((shift_fx << 8) | (negate << 16));
	bcn_rgn2_reg_write(0x734, value, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);

	value = slope_fx | (offset_fx0 << 12) | (1 << 28);
	if (0x400 != value) {
		bcn_rgn2_reg_write(0x738, value, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
}
#endif

static _ldo_calibration(int8_t tx_power, uint16_t efuse05)
{
	uint32_t val, dig_ldo_val;
	uint8_t rf_ldo_code;
	uint8_t rf_ldo_cal = (efuse05 >> 6) & 0x0f;
	uint8_t rf_ldo_cal_present = (efuse05 >> 10) & 0x01;
	uint8_t dig_ldo_cal = (efuse05 >> 11) & 0x0f;
	uint8_t dig_ldo_cal_present = (efuse05 >> 15) & 0x01;
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
	bcn_rgn2_reg_write(0x3254, val, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	if (rf_ldo_cal > 1) {
		val = rf_ldo_cal - 1;
	}
	else {
		val = 0;
	}
	val = val << 24;
	bcn_rgn2_reg_read_write(0x3244, val, 0xf000000, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	if (dig_ldo_cal > 3) {
		dig_ldo_val = dig_ldo_cal - 3;
	}
	else {
		dig_ldo_val = 0;
	}
	val = dig_ldo_val << 16;
	bcn_rgn2_reg_read_write(0x324C, val, 0xf0000, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
}

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

void bcn_rgn1_set_region23_from_ram(void)
{
	bcn_rgn1_set_value(0, RGN1_RGN2_PRESENT_MASK, RGN1_RGN2_PRESENT_SHIFT, RGN1_RGN2_PRESENT_OFFSET);
	bcn_rgn1_set_value(0, RGN1_RGN3_PRESENT_MASK, RGN1_RGN3_PRESENT_SHIFT, RGN1_RGN3_PRESENT_OFFSET);
}

#if ON_CHIP_MEASUREMENT_ENABLE
void bcn_on_chip_measurement_en(uint8_t vcc_en, uint8_t temp_en)
{
	if (vcc_en) {
		bcn_rgn1_set_value(1, 1, (RGN1_SENSOR_EN_SHIFT + 10), RGN1_SENSOR_EN_OFFSET);
	}
	else {
		bcn_rgn1_set_value(0, 1, (RGN1_SENSOR_EN_SHIFT + 10), RGN1_SENSOR_EN_OFFSET);
		bcn_rgn2_reg_write(0x750, 0x10, SIZE_BYTE, BOOT_WARM, TRIG_WAKEUP);
	}
	if (temp_en) {
		bcn_rgn1_set_value(1, 1, (RGN1_SENSOR_EN_SHIFT + 9), RGN1_SENSOR_EN_OFFSET);
	}
	else {
		bcn_rgn1_set_value(0, 1, (RGN1_SENSOR_EN_SHIFT + 9), RGN1_SENSOR_EN_OFFSET);
		bcn_rgn2_reg_write(0x730, 0x10, SIZE_BYTE, BOOT_WARM, TRIG_WAKEUP);
	}
}

void bcn_on_chip_measurement_temp_unit_mapping(uint16_t efuse09_val, float unit_lsb)
{
	_onchip_temperature_calibration(efuse09_val, unit_lsb);
}

void bcn_on_chip_measurement_vcc_unit_mapping(uint16_t efuse06_val, uint16_t efuse07_val, uint16_t efuse08_val, float unit_lsb)
{
	_vcc_measurement_calibration(efuse06_val, efuse07_val, efuse08_val, unit_lsb);
}
#else
void bcn_on_chip_measurement_en(uint8_t vcc_en, uint8_t temp_en)
{
	if (0 == vcc_en) {
		bcn_rgn1_set_value(0, 1, (RGN1_SENSOR_EN_SHIFT + 10), RGN1_SENSOR_EN_OFFSET);
		bcn_rgn2_reg_write(0x750, 0x10, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
	if (0 == temp_en) {
		bcn_rgn1_set_value(0, 1, (RGN1_SENSOR_EN_SHIFT + 9), RGN1_SENSOR_EN_OFFSET);
		bcn_rgn2_reg_write(0x730, 0x10, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
}
#endif

void bcn_xo_settings(uint8_t capacitor, uint8_t stable_time, uint8_t strength)
{
	uint32_t tmp;
	if (capacitor > 15) {
		capacitor = 15;
	}
	if (stable_time < 25) {
		stable_time = 25;
	}
	if (strength > 31)
	{
		strength = 31;
	}
	bcn_rgn1_set_value(capacitor, RGN1_XO_CAP_MASK, RGN1_XO_CAP_SHIFT, RGN1_XO_CAP_OFFSET);

	if (3 != capacitor) {
		bcn_rgn2_reg_write(0x331C, capacitor, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	}
	bcn_rgn2_reg_write(0x3490, stable_time, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);

	tmp = ((stable_time + 6) << 16) | (stable_time + 6);
	bcn_rgn2_reg_write(0x1020, tmp, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);

	if (0x1F != strength) {
		tmp = strength << 8;
		bcn_rgn2_reg_write(0x3320, tmp, SIZE_HWORD, BOOT_COLD, TRIG_WAKEUP);
	}
}

void bcn_tx_power_set(int8_t tx_power, uint16_t efuse05)
{
	uint8_t gain = _get_pa_gain(tx_power);
	bcn_rgn2_reg_write(0x1C90, gain, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
	_ldo_calibration(tx_power, efuse05);
}

static _gpio_set(bcn_gpio_t gpio, bcn_gpio_dir_t dir, bcn_gpio_pull_t pull)
{
	uint32_t value = 0x720;
	uint16_t addr = 0xA08 + gpio * 4;
	if (BCN_GPIO_ANALOG == dir) {
		value &= (~(3 << 8));
	}
	else if (BCN_GPIO_OUTPUT_LOW == dir) {
		value &= (~(1 << 9));
		value &= (~(1 << 5));
	}
	else if (BCN_GPIO_OUTPUT_HIGH == dir) {
		value &= (~(1 << 9));
		value |= (1 << 4);
		value &= (~(1 << 5));
	}
	if (BCN_GPIO_PULL_DISABLE == pull) {
		value &= (~(3 << 10));
	}
	else if (BCN_GPIO_PULL_DOWN == pull) {
		value &= (~(3 << 10));
		value |= (1 << 11);
	}
	if (0x720 != value) {
		bcn_rgn2_reg_write(addr, value, SIZE_HWORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
}

void bcn_gpio_settings(gpios_setting_t *gpio_settings)
{
	gpio_config_t* cfg;
	uint32_t maskb_latch = 0;
	uint32_t wakeup_mask = 0;
	uint32_t wakeup_polarity = 0;
	uint32_t trig_polarity_val = 0;
	uint32_t reg32e4 = 0x80000000;
	uint32_t reg32dc = 0;
	uint32_t reg32cc = 0;
	uint32_t reg32e0 = 0;
	uint32_t reg3328 = 0x204;
	cfg = &gpio_settings->gpio0_cfg;
	for (int i = 0; i < 8; i++)
	{
		if (BCN_GPIO_DEFAULT != cfg->dir)
		{
			_gpio_set(i, cfg->dir, cfg->pull_up_down);
			if (GPIO_TRIGGER_DISABLE != cfg->trigger)
			{
				if (GPIO_TRIGGER_LOW == cfg->trigger)
				{
					cfg->wakeup = GPIO_WAKEUP_LOW;
					trig_polarity_val = trig_polarity_val | (1 << i) | (1 << (i + 8)) | (1 << (i + 16));
				}
				else if (GPIO_TRIGGER_HIGH == cfg->trigger)
				{
					cfg->wakeup = GPIO_WAKEUP_HIGH;
				}
				else if (GPIO_TRIGGER_RISING_EDGE == cfg->trigger)
				{
					cfg->wakeup = GPIO_WAKEUP_HIGH;
					reg32e4 |= (1 << i);
					reg32e0 |= (1 << i);
					reg32dc |= (1 << i);
				}
				else if (GPIO_TRIGGER_FALLING_EDGE == cfg->trigger)
				{
					cfg->wakeup = GPIO_WAKEUP_HIGH;
					reg32e4 |= (1 << (i + 16));
					reg32e0 |= (1 << i);
					reg32dc |= (1 << i);
				}
				reg32cc = reg32cc | (1 << i) | (1 << (i + 8)) | (1 << (i + 16));
				cfg->latch_en = 1;
				cfg->dimask = 1;
			}
			if (GPIO_WAKEUP_DISABLE == cfg->wakeup)
			{
				wakeup_mask |= (1 << i);
			}
			else if (GPIO_WAKEUP_LOW == cfg->wakeup)
			{
				wakeup_polarity |= (1 << i);
				if (i < 4)
					reg3328 |= 0x30;
				else
					reg3328 |= 0x50;
				cfg->latch_en = 1;
				cfg->dimask = 1;
			}
			else if (GPIO_WAKEUP_HIGH == cfg->wakeup)
			{
				if (i < 4)
					reg3328 |= 0x30;
				else
					reg3328 |= 0x50;
				cfg->latch_en = 1;
				cfg->dimask = 1;
			}
			if (cfg->latch_en)
				maskb_latch |= (1 << i);
			if (cfg->dimask)
				maskb_latch |= (1 << (i+16));
		}
		cfg++;
	}
	if (0 != trig_polarity_val)
		bcn_rgn2_reg_write(0x1310, trig_polarity_val, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	if (0 != wakeup_mask)
		bcn_rgn2_reg_write(0x32C4, wakeup_mask, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	if (0 != wakeup_polarity)
		bcn_rgn2_reg_write(0x32C8, wakeup_polarity, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	if (0 != reg32dc)
		bcn_rgn2_reg_write(0x32DC, reg32dc, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	if (0 != reg32e0)
		bcn_rgn2_reg_write(0x32e0, reg32e0, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	if (0x80000000 != reg32e4)
		bcn_rgn2_reg_write(0x32e4, reg32e4, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	if (0x204 != reg3328)
	{
		reg3328 = reg3328 & 0xfff0;
		reg3328 |= 0x08;
		bcn_rgn2_reg_write(0x3328, reg3328, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	}
	if (0 != reg32cc)
		bcn_rgn2_reg_write(0x32cc, reg32cc, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	if (0 != maskb_latch)
		bcn_rgn2_reg_write(0x3300, maskb_latch, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
}


void bcn_gpio_config(bcn_gpio_t gpio, bcn_gpio_dir_t dir, bcn_gpio_pull_t pull)
{
	uint16_t addr = 0xA08 + gpio * 4;
	uint32_t value = 0x720;
	if (BCN_GPIO_ANALOG == dir) {
		value &= (~(3 << 8));
	}
	else if (BCN_GPIO_OUTPUT_LOW == dir) {
		value &= (~(1 << 9));
		value &= (~(1 << 5));
	}
	else if (BCN_GPIO_OUTPUT_HIGH == dir) {
		value &= (~(1 << 9));
		value |= (1 << 4);
		value &= (~(1 << 5));
	}
	if (BCN_GPIO_PULL_DISABLE == pull) {
		value &= (~(3 << 10));
	}
	else if(BCN_GPIO_PULL_DOWN == pull) {
		value &= (~(3 << 10));
		value |= (1 << 11);
	}
	if (0x720 != value) {
		bcn_rgn2_reg_write(addr, value, SIZE_HWORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
}

void bcn_gpio_latch_dimask_config(uint8_t latch_gpio_bitmap, uint8_t maskb_gpio_bitmap)
{
	uint32_t value = maskb_gpio_bitmap;
	value = value << 16;
	value |= latch_gpio_bitmap;
	if (0 != value) {
		bcn_rgn2_reg_write(3300, value, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	}
}

void bcn_gpio_wakeup_config(uint8_t wakeup_bitmap, uint8_t polarity_bitmap)
{
	bcn_rgn2_reg_write(0x32C4, wakeup_bitmap, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0x32C8, polarity_bitmap, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
}

void bcn_gpio_level_trigger_config(uint8_t polarity_bitmap)
{
	uint32_t value;
	uint32_t tmp = polarity_bitmap;
	value = (tmp << 16) | (tmp << 8) | tmp;
	if (0 != value) {
		bcn_rgn2_reg_write(0x1310, value, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
}

void bcn_gpio_edge_trigger_config(uint8_t gpio_en_bitmap, uint8_t falling_edge_bitmap, uint8_t rising_edge_bitmap)
{
	uint32_t value;

	value = gpio_en_bitmap;
	if (0 != value)
	{
		bcn_rgn2_reg_write(0x32E0, value, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
		bcn_rgn2_reg_write(0x32dc, value, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	}
	falling_edge_bitmap = falling_edge_bitmap & gpio_en_bitmap;
	rising_edge_bitmap = rising_edge_bitmap & gpio_en_bitmap;
	value = falling_edge_bitmap;
	value = value << 16;
	value |= rising_edge_bitmap;
	value |= 0x80000000;
	if (0x80000000 != value) {
		bcn_rgn2_reg_write(0x32E4, value, SIZE_WORD, BOOT_COLD, TRIG_WAKEUP);
	}
}

void bcn_gpio_edge_count_enable(bcn_gpio_t src_gpio)
{
	uint32_t value = (src_gpio << 4) | 3;
	bcn_rgn2_reg_write(0x32EC, value, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
	bcn_rgn2_reg_write(0xB40, 0x0f, SIZE_BYTE, BOOT_COLD_WARM, TRIG_WAKEUP);
}

void bcn_timer1_enable(void)
{
	bcn_rgn2_reg_write(0x3290, 0x23, SIZE_BYTE, BOOT_COLD, TRIG_WAKEUP);
}

void bcn_crypto_config(bcn_nonce_salt_t salt, bcn_nonce_cnt_t cnt)
{
	uint32_t value = 0xDFF;
	value |= (salt << 26);
	value |= (salt << 28);
	value |= (salt << 30);
	if (0xDFF != value) {
		bcn_rgn2_reg_write(0x1280, value, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}

	value = 0x24;
	value |= (cnt << 6);
	value |= (cnt << 9);
	value |= (cnt << 12);
	if (0x24 != value) {
		bcn_rgn2_reg_write(0x1284, value, SIZE_WORD, BOOT_COLD_WARM, TRIG_WAKEUP);
	}
	
}

void bcn_reg_val_trig_en(void)
{
	bcn_rgn1_set_value(1, RGN1_REG_VAL_TRIG_MASK, RGN1_REG_VAL_TRIG_SHIFT, RGN1_REG_VAL_TRIG_OFFSET);
}

void bcn_sleep_after_tx_en(void)
{
	bcn_rgn1_set_value(1, RGN1_SLEEP_AFT_TX_MASK, RGN1_SLEEP_AFT_TX_SHIFT, RGN1_SLEEP_AFT_TX_OFFSET);
}
