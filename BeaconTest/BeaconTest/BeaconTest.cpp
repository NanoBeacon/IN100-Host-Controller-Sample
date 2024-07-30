#include <iostream>
#include <cstdio>
#include <Windows.h>
#include "serial_if.h"
#include "nano_bcn_api.h"

extern "C" void host_sleep(uint32_t ms)
{
    Sleep(ms);
}

extern "C" int host_write(uint8_t * buf, uint16_t buf_len, uint32_t tmo)
{
    int res;
    res = serial_write(buf, buf_len);
    if (res == SERIAL_ERR_NO_ERROR)
        res = UART_ERR_NO_ERROR;
    else if (res == SERIAL_ERR_WRITE_TMO)
        res = UART_ERR_TMO;

    return res;
}


extern "C" int host_read(uint8_t * buf, uint16_t buf_len, uint32_t tmo)
{
    int res;
    res = serial_read(buf, buf_len, tmo);
    if (res == SERIAL_ERR_NO_ERROR)
        res = UART_ERR_NO_ERROR;
    else if (res == SERIAL_ERR_WRITE_TMO)
        res = UART_ERR_TMO;

    return res;
}

extern "C" void host_uart_break(int on)
{
    serial_break(on);
}

extern "C" void print_data(uint16_t data)
{
    printf("%04x \n", data);
}

static void mcu_wakeup_nanobeacon()
{
    std::cout << "MCU's GPIO output high level to enable IN100's CHIP_EN" << std::endl;
    std::cout << "MCU control it's GPIO to wakeup IN100" << std::endl;
    Sleep(30);
}

static void example_gpio_trigger_advertising(void)
{
    char buff[32];
    uint8_t bdaddr[6] = { 0x10, 0x02, 0x03, 0x04, 0x05, 0x06 };
    uint8_t adv_raw[] = { 0x0D, 0xff, 0x05, 0x05, 0x01, 0x02 };
    gpios_setting_t gpio_settings;
    memset(&gpio_settings, 0, sizeof(gpio_settings));

    /* set GPIO2 high level to trigger advertising */
    gpio_settings.gpio2_cfg.dir = BCN_GPIO_INPUT;
    gpio_settings.gpio2_cfg.pull_up_down = BCN_GPIO_PULL_DISABLE;
    gpio_settings.gpio2_cfg.trigger = GPIO_TRIGGER_HIGH;
    gpio_settings.gpio2_cfg.wakeup = GPIO_WAKEUP_DISABLE;

    /* set GPIO3 high level to wakeup chip */
    gpio_settings.gpio3_cfg.dir = BCN_GPIO_INPUT;
    gpio_settings.gpio3_cfg.pull_up_down = BCN_GPIO_PULL_DISABLE;
    gpio_settings.gpio3_cfg.trigger = GPIO_TRIGGER_DISABLE;
    gpio_settings.gpio3_cfg.wakeup = GPIO_WAKEUP_HIGH;
    bcn_gpio_settings(&gpio_settings);
 
    /* enable sleep after advertising */
    bcn_sleep_after_tx_en();

    /* disable on - chip measurement VCC and temperature */
    bcn_on_chip_measurement_en(0, 0);

    /* advertising set #1 , advertising on channels 37,38,39, period is 100ms */
    bcn_adv_tx_set(ADV_SET_1, 100, ADV_CH_37_38_39, ADV_MODE_EVENT);
    bcn_adv_address_set(ADV_SET_1, ADDR_PUBLIC, bdaddr, 60, 0);

    /* add predefined data to advertising payload */
    bcn_adv_data_add_predefined_data(ADV_SET_1, adv_raw, sizeof(adv_raw), 0);

    /* add register(memory) data to advertising payload */
    bcn_adv_data_add_register_read_data(ADV_SET_1, 0x2f00, 8, 0, 0);

    /* set GPIO2 as trigger source */
    bcn_adv_trigger_setting_set(ADV_SET_1, 1<<2, 0, 5, 3, 1000);

    nano_bcn_load_data_to_ram();
    nano_bcn_start_advertising();
    std::cout << "run ..." << std::endl;
    while (1)
    {
        std::cout << "Let GPIO2 input a high level to trigger the advertising" << std::endl;
        std::cout << "Let GPIO3 input a low level to put chip to sleep" << std::endl;
        std::cout << "input any key + Enter to continue" << std::endl;
        std::cin >> buff;

        std::cout << "Let GPIO2 input a low level to stop advertising" << std::endl;
        std::cout << "Let GPIO3 input a high level to wakeup chip" << std::endl;
        std::cout << "input any key + Enter to continue" << std::endl;
        std::cin >> buff;

        std::cout << "write register (memory) to update advertising data" << std::endl;
        nano_bcn_write_reg(0x2f00, 0x010203a4, NULL);
        nano_bcn_write_reg(0x2f04, 0x050607a8, NULL);
    }
}

static void example_continuous_advertising(void)
{
    uint8_t bdaddr[6] = { 0x10, 0x02, 0x03, 0x04, 0x05, 0x06 };
    uint8_t adv_raw[] = { 0x09, 0xff, 0x05, 0x05, 0x01, 0x02 };
    /*disable on-chip measurement VCC and temperature*/
    bcn_on_chip_measurement_en(0, 0);

    /*advertising set #1 , advertising on channels 37,38,39, period is 1000ms*/
    bcn_adv_tx_set(ADV_SET_1, 1000, ADV_CH_37_38_39, ADV_MODE_CONTINUOUS);
    bcn_adv_address_set(ADV_SET_1, ADDR_PUBLIC, bdaddr, 60, 0);

    /*add predefined data to advertising payload */
    bcn_adv_data_add_predefined_data(ADV_SET_1, adv_raw, sizeof(adv_raw), 0);

    /* add advertising count data to advertising payload */
    bcn_adv_data_add_adv_count(ADV_SET_1, 4, 0, 0);

    nano_bcn_load_data_to_ram();
    nano_bcn_start_advertising();
    std::cout << "advertising ..." << std::endl;


    Sleep(35000);
    /*stop or change advertising payload*/
    bcn_adv_data_reset();
    nano_bcn_chip_reset();
}

static void example_continuous_advertising_with_vcc_temperature(void)
{
    uint16_t efuse_val;
    uint16_t efuse_val2;
    uint16_t efuse_val3;
    uint8_t bdaddr[6] = { 0x10, 0x02, 0x03, 0x04, 0x05, 0x06 };
    uint8_t adv_raw[] = { 0x05, 0xff, 0x05, 0x05, 0x01, 0x02 };
    uint8_t adv_raw2[] = { 0x07, 0xff, 0x05, 0x05, 0x01, 0x02, 0x03, 04 };
    //*enable on-chip measurement VCC and temperature*/
    bcn_on_chip_measurement_en(1, 1);
    efuse_val = 0;
    int res = nano_bcn_read_efuse(0x09, &efuse_val);
    if (0 != res) {
        std::cout << "efuse read failed" << std::endl;
        return;
    }

    /*Calibration adn unit mapping processing*/
    bcn_on_chip_measurement_temp_unit_mapping(efuse_val, 0.01);
    efuse_val = 0;
    nano_bcn_read_efuse(0x06, &efuse_val);
    efuse_val2 = 0;
    nano_bcn_read_efuse(0x07, &efuse_val2);
    efuse_val3 = 0;
    nano_bcn_read_efuse(0x08, &efuse_val3);
    bcn_on_chip_measurement_vcc_unit_mapping(efuse_val, efuse_val2, efuse_val3, 0.03125);

    adv_raw[0] = 8;
    /*advertising set #1 , advertising on channels 37,38,39, period is 1000ms*/
    bcn_adv_tx_set(ADV_SET_1, 1000, ADV_CH_37_38_39, ADV_MODE_CONTINUOUS);
    bcn_adv_address_set(ADV_SET_1, ADDR_PUBLIC, bdaddr, 60, 0);

    /*add predefined data to advertising payload */
    bcn_adv_data_add_predefined_data(ADV_SET_1, adv_raw, sizeof(adv_raw), 0);

    /*add VCC data to advertising payload */
    bcn_adv_data_add_vcc(ADV_SET_1, 1, 0, 0);

    /*add temperature data to advertising payload */
    bcn_adv_data_add_temperature(ADV_SET_1, 2, 0, 0);

    Sleep(35000);
    /*stop or change advertising payload*/
    bcn_adv_data_reset();
    nano_bcn_chip_reset();
}

static void example_multi_advertising_sets(void)
{
    uint8_t bdaddr[6] = { 0x10, 0x02, 0x03, 0x04, 0x05, 0x06 };
    uint8_t adv_raw[] = { 0x05, 0xff, 0x05, 0x05, 0x01, 0x02 };
    uint8_t adv_raw2[] = { 0x07, 0xff, 0x05, 0x05, 0x01, 0x02, 0x03, 04 };
    bcn_on_chip_measurement_en(0, 0);

    /* advertising set #1 */
    bcn_adv_tx_set(ADV_SET_1, 1000, ADV_CH_37_38_39, ADV_MODE_CONTINUOUS);
    bcn_adv_address_set(ADV_SET_1, ADDR_PUBLIC, bdaddr, 60, 0);
    bcn_adv_data_add_predefined_data(ADV_SET_1, adv_raw, sizeof(adv_raw), 0);

    /* advertising set #2 */
    bcn_adv_tx_set(ADV_SET_2, 1000, ADV_CH_37_38_39, ADV_MODE_CONTINUOUS);
    bdaddr[0] = 0x11;
    bcn_adv_address_set(ADV_SET_2, ADDR_PUBLIC, bdaddr, 60, 0);
    bcn_adv_data_add_predefined_data(ADV_SET_2, adv_raw2, sizeof(adv_raw2), 0);

    Sleep(35000);
    /*stop or change advertising payload*/
    bcn_adv_data_reset();
    nano_bcn_chip_reset();
}



int main()
{
    host_itf_t hif;
    
    int j = 0;
    const char* port = "\\\\.\\COM34";
    int res = serial_open(port, 115200);
    if (res != SERIAL_ERR_NO_ERROR) {
        std::cout << "uart open failed ! " << port << std::endl;
        return 0;
    }
    hif.delay = host_sleep;
    hif.serial_rx = host_read;
    hif.serial_tx = host_write;
    hif.serial_break = host_uart_break;

    mcu_wakeup_nanobeacon();

    res = nano_bcn_init(&hif);
    std::cout << "nano_bcn_init:" << res << std::endl;

    res = nano_bcn_board_setup(7, 36, 16, 0);
    std::cout << "nano_bcn_board_setup:" << res << std::endl;
    
    //example_continuous_advertising();
    //example_continuous_advertising_with_vcc_temperature();
    //example_multi_advertising_sets();
    example_gpio_trigger_advertising();
   
    std::cout << "exit" << std::endl;
    return 0;
}