/**
 ****************************************************************************************
 *
 * @file nano_bcn_rgn3.h
 *
 * @brief Nano beacon region3 header file
 *
 * Copyright (C) Inplay Technologies Inc. 2018-2020
 *
 ****************************************************************************************
 */
#ifndef NANO_BCN_RGN3_H
#define NANO_BCN_RGN3_H

#include <stdint.h>

#define ADV_SET_CNT_MAX  2

#define RET_OK    0
#define RET_ERROR 1
#define RET_INVALID_PARAMETER 2

typedef enum {
	ADV_SET_1 = 0,
#if ADV_SET_CNT_MAX > 1
	ADV_SET_2,
#endif
#if ADV_SET_CNT_MAX > 2
	ADV_SET_3,
#endif
	ADV_SET_NUM
}advset_index_t;

typedef enum {
	ADV_MODE_CONTINUOUS = 0,
	ADV_MODE_EVENT
}adv_mode_t;

typedef enum {
	ADDR_PUBLIC = 0,
	ADDR_RANDOM_NON_RESOLVABLE,
	ADDR_RANDOM_RESOLVABLE,
	ADDR_RANDOM_STATIC,
	ADDR_NOT_PRESENT
}address_type_t;

typedef enum {
	ADV_CH_37_38_39 = 0,
	ADV_CH_38_39,
	ADV_CH_37_39,
	ADV_CH_39,
	ADC_CH_37_38,
	ADV_CH_38,
	ADV_CH_37,
}adv_channel_ctl_t;

/*
 * APIs
 ****************************************************************************************
 */
#if defined(__cplusplus)
extern "C" {                // Make sure we have C-declarations in C++ programs
#endif

void bcn_adv_region_data_reset(void);
int bcn_adv_tx_set(advset_index_t idx, int tx_interval, adv_channel_ctl_t ch_ctl, adv_mode_t mode);
int bcn_adv_address_set(advset_index_t idx, address_type_t addr_type, const uint8_t* bdaddr,
	                    uint16_t addr_gen_interval, uint8_t key_idx);

int bcn_adv_data_add_predefined_data(advset_index_t idx, uint8_t* data, uint8_t length, char encrypt_en);
int bcn_adv_data_add_adv_count(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en);
int bcn_adv_data_add_second_count(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en);
int bcn_adv_data_add_100ms_count(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en);
int bcn_adv_data_add_gpio_status(advset_index_t idx, char encrypt_en);
int bcn_adv_data_add_vcc(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en);
int bcn_adv_data_add_temperature(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en);
int bcn_adv_data_add_register_read_data(advset_index_t idx, uint16_t address, uint8_t length, uint8_t bigendian, char encrypt_en);

int bcn_adv_trigger_setting_set(advset_index_t idx, uint8_t gpio_bitmap, uint8_t sensor_bitmap, uint16_t event_num, uint8_t event_mode, uint32_t sensor_check_period);

void bcn_adv_data_reset(void);
int bcn_adv_data_to_binary(uint8_t* buffer, uint16_t size);

#if defined(__cplusplus)
}     /* Make sure we have C-declarations in C++ programs */
#endif
#endif	// NANO_BCN_RGN3_H
