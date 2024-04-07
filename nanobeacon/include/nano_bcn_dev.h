/**
 ****************************************************************************************
 *
 * @file nano_bcn_dev.h
 *
 * @brief Nano beacon dev header file
 *
 * Copyright (C) Inplay Technologies Inc. 2018-2020
 *
 ****************************************************************************************
 */
#ifndef NANO_BCN_DEV_H
#define NANO_BCN_DEV_H

#include <stdint.h>
#include "nano_bcn_rgn2.h"

#define ON_CHIP_MEASUREMENT_ENABLE 1

typedef enum {
	BCN_GPIO0 = 0,
	BCN_GPIO1,
	BCN_GPIO2,
	BCN_GPIO3,
	BCN_GPIO4,
	BCN_GPIO5,
	BCN_GPIO6,
	BCN_GPIO7,
}bcn_gpio_t;

typedef enum {
	BCN_GPIO_DEFAULT = 0,
	BCN_GPIO_INPUT,
	BCN_GPIO_OUTPUT_LOW,
	BCN_GPIO_OUTPUT_HIGH,
	BCN_GPIO_ANALOG,
}bcn_gpio_dir_t;

typedef enum {
	BCN_GPIO_PULL_DISABLE = 0,
	BCN_GPIO_PULL_UP,
	BCN_GPIO_PULL_DOWN,
}bcn_gpio_pull_t;

typedef enum {
	GPIO_TRIGGER_DISABLE = 0,
	GPIO_TRIGGER_LOW,
	GPIO_TRIGGER_HIGH,
	GPIO_TRIGGER_RISING_EDGE,
	GPIO_TRIGGER_FALLING_EDGE,
}gpio_trigger_t;

typedef enum {
	GPIO_WAKEUP_DISABLE = 0,
	GPIO_WAKEUP_HIGH,
	GPIO_WAKEUP_LOW,
}gpio_wakeup_t;

typedef struct gpio_config_s
{
	bcn_gpio_dir_t dir;
	bcn_gpio_pull_t pull_up_down;
	gpio_trigger_t trigger;
	gpio_wakeup_t wakeup;
	uint8_t latch_en;
	uint8_t dimask;
	uint8_t pad[2];
}gpio_config_t;

typedef struct gpios_setting_s
{
	gpio_config_t gpio0_cfg;
	gpio_config_t gpio1_cfg;
	gpio_config_t gpio2_cfg;
	gpio_config_t gpio3_cfg;
	gpio_config_t gpio4_cfg;
	gpio_config_t gpio5_cfg;
	gpio_config_t gpio6_cfg;
	gpio_config_t gpio7_cfg;
}gpios_setting_t;

typedef enum {
	NONCE_SALT_STATIC_RANDOM = 0, /*unchange after cold boot*/
	NONCE_SALT_RANDOM,
	NONCE_SALT_PREDEFINED,
}bcn_nonce_salt_t;

typedef enum {
	NONCE_CNT_SECOND_CNT = 0, /*1s count */
	NONCE_CNT_100MS_CNT, /*0.1s count */
	NONCE_CNT_ADV_CNT,
	NONCE_CNT_PREDEFINED,
	NONCE_CNT_STATIC_RANDOM,
	NONCE_CNT_RANDOM = 6
}bcn_nonce_cnt_t;

/*
 * APIs
 ****************************************************************************************
 */
#if defined(__cplusplus)
extern "C" {                // Make sure we have C-declarations in C++ programs
#endif

void bcn_rgn1_set_region23_from_ram(void);
void bcn_xo_settings(uint8_t capacitor, uint8_t stable_time, uint8_t strength);
void bcn_tx_power_set(int8_t tx_power, uint16_t efuse05);
void bcn_gpio_settings(gpios_setting_t* gpio_settings);
void bcn_timer1_enable(void);
void bcn_reg_val_trig_en(void);
void bcn_sleep_after_tx_en(void);
void bcn_crypto_config(bcn_nonce_salt_t salt, bcn_nonce_cnt_t cnt);

void bcn_on_chip_measurement_en(uint8_t vcc_en, uint8_t temp_en);
#if ON_CHIP_MEASUREMENT_ENABLE
void bcn_on_chip_measurement_temp_unit_mapping(uint16_t efuse09_val, float unit_lsb);
void bcn_on_chip_measurement_vcc_unit_mapping(uint16_t efuse06_val, uint16_t efuse07_val, uint16_t efuse08_val, float unit_lsb);
#endif

#if defined(__cplusplus)
}     /* Make sure we have C-declarations in C++ programs */
#endif
#endif	// NANO_BCN_DEV_H
