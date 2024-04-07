/**
 ****************************************************************************************
 *
 * @file nano_bcn_api.h
 *
 * @brief Nano beacon API header file
 *
 * Copyright (C) Inplay Technologies Inc. 2018-2020
 *
 ****************************************************************************************
 */
#ifndef NANO_BCN_API_H
#define NANO_BCN_API_H

#include <stdint.h>

#include "nano_bcn_serial_protocol.h"
#include "nano_bcn_rgn2.h"
#include "nano_bcn_rgn3.h"
#include "nano_bcn_dev.h"
/*
 * APIs
 ****************************************************************************************
 */
#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs
#endif

/**
 * Usage:   Used to register host interface function and init
 * Params:  p_hif     host interface function
 * Return:  0 indicates success, others indicate failure
 */
int nano_bcn_init(host_itf_t *p_hif);

/**
 * Usage:   Used to deinit
 * Params: 
 * Return: 
 */
void nano_bcn_deinit(void);

/**
*Usage:  Used to set initial advertising parameters
* Params : capacitor    on - chip crystal oscillator, 1 ~15
* xo_stable_time  25 ~255, cycles
* xo_strength  0 ~31
* tx_power    transmission power, -5 ~5 dBm
* Return : 0 indicates success, others indicate failure
*/
int nano_bcn_board_setup(uint8_t capacitor, uint8_t xo_stable_time, uint8_t xo_strength, int8_t tx_power);

/**
 * Usage:   Used to set advertising data
 * Params:  bdaddr		bluetooth device address , 6 bytes. cannot be all 0 or all 1
 *		    interval    broadcast interval, millisecond
 *			adv_raw_data  advertising raw data, max length is 31 bytes
 *			len		length of adv_raw_data
 * Return: 0 indicates success, others indicate failure
 */
int nano_bcn_set_advertising(const uint8_t* bdaddr, int interval, uint8_t *adv_raw_data, int len);

/**
 * Usage:   Used to load raw data into SDRAM
 * Params:  
 * Return: 0 indicates success, others indicate failure
 */
int nano_bcn_load_data_to_ram(void);

/**
 * Usage:   Used to start advertising
 * Params:
 * Return:	0 indicates success, others indicate failure
 */
int nano_bcn_start_advertising();

#if defined(__cplusplus)
}     /* Make sure we have C-declarations in C++ programs */
#endif
#endif	// NANO_BCN_API_H
