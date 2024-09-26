#ifndef NANO_BCN_RF_TEST_H
#define NANO_BCN_RF_TEST_H
#include <stdint.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {                // Make sure we have C-declarations in C++ programs
#endif

int dtm_start(uint8_t ch, bool is_125k, uint8_t tx_len, uint8_t tx_pattern, uint8_t cap_code, int8_t tx_power);
int dtm_stop(void);

int carrier_test_start(uint8_t ch, uint8_t cap_code, int8_t tx_power);
int carrier_test_stop(void);

#if defined(__cplusplus)
}     /* Make sure we have C-declarations in C++ programs */
#endif
#endif /*NANO_BCN_RF_TEST_H*/
