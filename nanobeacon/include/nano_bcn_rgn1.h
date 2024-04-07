/**
 ****************************************************************************************
 *
 * @file nano_bcn_rgn1.h
 *
 * @brief Nano beacon region1 header file
 *
 * Copyright (C) Inplay Technologies Inc. 2018-2020
 *
 ****************************************************************************************
 */
#ifndef NANO_BCN_RGN1_H
#define NANO_BCN_RGN1_H

#include <stdint.h>

#define RGN1_XO_CAP_OFFSET  0
#define RGN1_XO_CAP_MASK   0xf
#define RGN1_XO_CAP_SHIFT  5

#define RGN1_RGN2_PRESENT_OFFSET  0
#define RGN1_RGN2_PRESENT_MASK   0x1
#define RGN1_RGN2_PRESENT_SHIFT  13

#define RGN1_RGN3_PRESENT_OFFSET  0
#define RGN1_RGN3_PRESENT_MASK   0x1
#define RGN1_RGN3_PRESENT_SHIFT  14

#define RGN1_PULSE_COUNT_EN_OFFSET  1
#define RGN1_PULSE_COUNT_EN_MASK   0x1
#define RGN1_PULSE_COUNT_EN_SHIFT  0

#define RGN1_UART_PIN_SEL_OFFSET  1
#define RGN1_UART_PIN_SEL_MASK   0x1
#define RGN1_UART_PIN_SEL_SHIFT  1

#define RGN1_UART1WIRE_EN_OFFSET  1
#define RGN1_UART1WIRE_EN_MASK   0x1
#define RGN1_UART1WIRE_EN_SHIFT  8

#define RGN1_SLEEP_AFT_TX_OFFSET  1
#define RGN1_SLEEP_AFT_TX_MASK   0x1
#define RGN1_SLEEP_AFT_TX_SHIFT  9

#define RGN1_SLEEP_AFT_UART_BI_OFFSET  2
#define RGN1_SLEEP_AFT_UART_BI_MASK   0x1
#define RGN1_SLEEP_AFT_UART_BI_SHIFT  14

#define RGN1_MTS_PRESENT_OFFSET  2
#define RGN1_MTS_PRESENT_MASK   0x1
#define RGN1_MTS_PRESENT_SHIFT  15

#define RGN1_CAL_FREQ_OFFSET  3
#define RGN1_CAL_FREQ_MASK   0x1f
#define RGN1_CAL_FREQ_SHIFT  8

#define RGN1_TX_PKT_GAP_OFFSET  4
#define RGN1_TX_PKT_GAP_MASK   0xfff
#define RGN1_TX_PKT_GAP_SHIFT  0

#define RGN1_REG_VAL_TRIG_OFFSET  8
#define RGN1_REG_VAL_TRIG_MASK   1
#define RGN1_REG_VAL_TRIG_SHIFT  5

#define RGN1_TRIGGER1_LOW_TH_OFFSET   14
#define RGN1_TRIGGER2_LOW_TH_OFFSET   15
#define RGN1_TRIGGER2_HIGH_TH_OFFSET  16
#define RGN1_TRIGGER3_LOW_TH_OFFSET   17
#define RGN1_TRIGGER3_HIGH_TH_OFFSET  18
#define RGN1_TRIGGER4_LOW_TH_OFFSET   19
#define RGN1_TRIGGER4_HIGH_TH_OFFSET  20

#define RGN1_SENSOR_SW0_EN_OFFSET  46
#define RGN1_SENSOR_SW0_EN_MASK   0xff
#define RGN1_SENSOR_SW0_EN_SHIFT  0

#define RGN1_SENSOR_SW1_EN_OFFSET  46
#define RGN1_SENSOR_SW1_EN_MASK   0xff
#define RGN1_SENSOR_SW1_EN_SHIFT  8

#define RGN1_SENSOR_EN_OFFSET  47
#define RGN1_SENSOR_EN_MASK   0xfff
#define RGN1_SENSOR_EN_SHIFT  0

#define RGN1_PULSE_SW0_EN_OFFSET  47
#define RGN1_PULSE_SW0_EN_MASK   0x1
#define RGN1_PULSE_SW0_EN_SHIFT  12

#define RGN1_PULSE_SW1_EN_OFFSET  47
#define RGN1_PULSE_SW1_EN_MASK   0x1
#define RGN1_PULSE_SW1_EN_SHIFT  13

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

#if defined(__cplusplus)
}     /* Make sure we have C-declarations in C++ programs */
#endif
#endif	// NANO_BCN_RGN1_H
