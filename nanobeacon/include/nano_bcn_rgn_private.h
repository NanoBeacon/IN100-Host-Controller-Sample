/**
 ****************************************************************************************
 *
 * @file nano_bcn_rgn_private.h
 *
 * @brief Nano beacon region header file
 *
 * Copyright (C) Inplay Technologies Inc. 2018-2020
 *
 ****************************************************************************************
 */
#ifndef NANO_BCN_RGN_PRIVATE_H
#define NANO_BCN_RGN_PRIVATE_H

#include <stdint.h>

 // Region 2 command type
typedef enum cmd_type {
	CMD_INVALID = 0,
	CMD_DELAY,
	CMD_REG_WRITE,
	CMD_REG_READ_MOD_WRITE,
}cmd_type_t;

// Region 3 
typedef struct {
	uint8_t addr_not_present;
	uint8_t header_len;
	uint8_t header[4];
} pkt_f0_t;

typedef union {
	uint8_t timer_sel;
	uint8_t rand_num_sel;
	uint8_t sensor_idx;
	uint8_t mts_offset;
	uint8_t gpio_sel;
	uint8_t plsdtct_sel;
	uint16_t reg_addr;
} pkt_fx_private_t;

typedef struct {
	uint8_t data_src : 5;
	uint8_t endian : 1;
	uint8_t encryption : 1;
	uint8_t encryption_last : 1;

	uint8_t length : 6;
	uint8_t msb_lsb_sel : 1;
	uint8_t encryption_output_order : 1;

	pkt_fx_private_t u;

	uint8_t data_offset;
} pkt_f1to7_t;

typedef union {
	struct {
		uint8_t preamble : 1;
		uint8_t num_fields : 4;
		uint8_t	sync_pattern_en : 1;
		uint8_t rsv : 2;
	} u;
	uint8_t data;
} pkt_ctl_byte0_t;

typedef union {
	struct {
		uint8_t cte_en : 1;
		uint8_t cte_len : 5;
		uint8_t	phy_rate : 2;
	} u;
	uint8_t data;
} pkt_ctl_byte1_t;

typedef union {
	struct {
		uint8_t	eax_en : 1;
		uint8_t	key_sel : 2;
		uint8_t	rot_exp : 5;
	} u;
	uint8_t data;
} pkt_ctl_byte2_t;

typedef struct {
	pkt_ctl_byte0_t b0;
	pkt_ctl_byte1_t b1;
	pkt_ctl_byte2_t b2;
	uint32_t sync_pattern;
	pkt_f0_t f0;
	uint8_t f1to7_cnt;
	uint8_t cur_pos;
	uint8_t* data_buffer;
	pkt_f1to7_t	f[7];
} pkt_ctl_t;

typedef union {
	struct {
		uint8_t	adv_type : 1;
		uint8_t	trig_source : 7;
	} u;
	uint8_t data;
} adv_ctl_byte0_t;

typedef union {
	struct {
		uint8_t	adv_trig_mode : 2;
		uint8_t	adv_ext_en : 1;
		uint8_t secondary_adv_exist : 2;
		uint8_t	adv_channel_control : 3;
	} u;
	uint8_t data;
} adv_ctl_byte8_t;

typedef union {
	struct {
		uint8_t	adv_addr_type : 3;
		uint8_t	addr_key_sel : 5;
	} u;
	uint8_t data;
} addr_privte_data_t;

typedef struct {
	adv_ctl_byte0_t b0;
	uint8_t gpio_source;
	uint8_t b2;
	uint8_t adv_interval[3];
	uint16_t packet_table_location;
	adv_ctl_byte8_t b8;
	uint8_t evnt_wakeup_period[3];
	uint8_t num_event_control[2];
	addr_privte_data_t addr_privte_data;
	uint8_t adv_addr[6];
	uint8_t addr_gen_interval[2];
	pkt_ctl_t pkt_ctl;
} adv_ctl_t;

typedef enum {
	ADV_DATA_TYPE_PREDEF = 0,
	ADV_DATA_TYPE_TIMER = 1,
	ADV_DATA_TYPE_RAND = 2,
	ADV_DATA_TYPE_TEMP = 3,
	ADV_DATA_TYPE_VBAT = 4,
	ADV_DATA_TYPE_SENSOR = 5,
	ADV_DATA_TYPE_TAG = 7,
	ADV_DATA_TYPE_MTS_DATA = 8,
	ADV_DATA_TYPE_TX_PWR_0M = 9,
	ADV_DATA_TYPE_INP_UUID = 10,
	ADV_DATA_TYPE_CUS_UUID = 11,
	ADV_DATA_TYPE_EID = 12,
	ADV_DATA_TYPE_ADV_EVNT_CNT = 13,
	ADV_DATA_TYPE_AUXPTR = 14,
	ADV_DATA_TYPE_SLEEP_CNT = 15,
	ADV_DATA_TYPE_GPIO_VAL = 27,
	ADV_DATA_TYPE_PLSDTCT = 28,
	ADV_DATA_TYPE_GPIO_CNT = 29,
	ADV_DATA_TYPE_REG_DATA = 31,
}adv_data_type;

#endif	// NANO_BCN_RGN_PRIVATE_H
