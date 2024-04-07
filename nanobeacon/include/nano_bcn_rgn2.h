/**
 ****************************************************************************************
 *
 * @file nano_bcn_rgn2.h
 *
 * @brief Nano beacon region2 header file
 *
 * Copyright (C) Inplay Technologies Inc. 2018-2020
 *
 ****************************************************************************************
 */
#ifndef NANO_BCN_RGN2_H
#define NANO_BCN_RGN2_H

#include <stdint.h>

// Region 2 boot type
typedef enum {
	BOOT_COLD = 0,
	BOOT_WARM,
	BOOT_COLD_WARM = 3,
}boot_type_t;

// Region 2 trigger type
typedef enum {
	TRIG_INVALID = 0,
	TRIG_WAKEUP,
	TRIG_CALIB_START,
	TRIG_CALIB_DONE,
	TRIG_BB_TX_EN,
	TRIG_IIC_OP_DONE,
	TRIG_DLY_TX_EN_DONE,
	TRIG_BB_TX_DONE,
	TRIG_TX_DONE,
	TRIG_PRE_SLEEP,
	TRIG_ADC_START,
	TRIG_ADC_DONE,
	TRIG_DELAY_TX_EN_START,
	TRIG_PULSE_RISE,
	TRIG_PULSE_DONE
}trig_type_t;

// Region 2 size type
typedef enum {
	SIZE_BYTE = 0,
	SIZE_HWORD,
	SIZE_WORD = 3,
}size_type_t;

/*
 * APIs
 ****************************************************************************************
 */
#if defined(__cplusplus)
extern "C" {                // Make sure we have C-declarations in C++ programs
#endif

void bcn_rgn1_data_reset(void);
void bcn_rgn1_set_value(uint16_t value, uint16_t mask, uint8_t shift, uint8_t offset);
int bcn_rgn1_load_to_ram(void);
int bcn_rgn1_brun(void);

void bcn_rgn2_data_reset(void);
uint16_t bcn_rgn2_get_size(void);
int bcn_rgn2_load_to_ram(void);
int bcn_rgn2_brun(void);

void bcn_rgn2_delay(uint32_t delay, boot_type_t coldwarm, trig_type_t trigger);
void bcn_rgn2_reg_write(uint16_t address, uint32_t value, size_type_t size, boot_type_t coldwarm, 
                        trig_type_t trigger);
void bcn_rgn2_reg_read_write(uint16_t address, uint32_t value, uint32_t mask, size_type_t size, 
                             boot_type_t coldwarm, trig_type_t trigger);

#if defined(__cplusplus)
}     /* Make sure we have C-declarations in C++ programs */
#endif
#endif	// NANO_BCN_RGN2_H
