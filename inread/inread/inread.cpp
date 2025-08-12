#include <iostream>
#include <cstdio>
#include <Windows.h>
#include <tchar.h>

#include "serial_if.h"
#include "nano_bcn_api.h"
#include "nano_bcn_rf_test.h"

extern "C" int read_customer_product_id(int argc, char* argv[]);
extern "C" int read_mac(int argc, char* argv[]);

enum {
    CMD_ERR_OK = 0,
    CMD_ERR_EMPTY_CHIP = 100,
    CMD_ERR_ILLEGAL_CMD = 101,
    CMD_ERR_NO_MAC = 102

};
typedef struct read_cmd_s {
    const char* cmd;
    int param_cnt;
    int (*operation)(int argc, char* argv[]);
}read_cmd_t;

read_cmd_t my_cmds[] =
{
    {"mac", 1, read_mac},
    {"custid", 1, read_customer_product_id}
};

static int print_help(int argc, char* argv[])
{
    std::cout << "read MAC: inread.exe mac COMxxx" << std::endl;
    std::cout << "read custom product ID: inread.exe custid COMxxx" << std::endl;

    std::cout << std::endl;
    std::cout << "inread ? " << CMD_ERR_ILLEGAL_CMD  << " []" << std::endl;
    return CMD_ERR_ILLEGAL_CMD;
}



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

extern "C" static int intf_setup(char* com_port)
{
    host_itf_t hif;

    int j = 0;
    int res = serial_open(com_port, 115200);
    if (res != SERIAL_ERR_NO_ERROR) {
        //std::cout << "uart open failed ! " << com_port << std::endl;
        return res;
    }
    hif.delay = host_sleep;
    hif.serial_rx = host_read;
    hif.serial_tx = host_write;
    hif.serial_break = host_uart_break;

    res = nano_bcn_init(&hif, 0);
    return res;

}

extern "C" static void intf_deinit(void)
{
    nano_bcn_deinit();
    serial_close();
}

extern "C" int read_customer_product_id(int argc, char* argv[])
{
    int ret;
    char data_buff[256];
    uint16_t id[3];
    if (argc < 2) {
        ret = CMD_ERR_ILLEGAL_CMD;
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    snprintf(data_buff, sizeof(data_buff), "\\\\.\\%s", argv[2]);
    ret = intf_setup(data_buff);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    
    ret = nano_bcn_read_efuse(27, &id[0]);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    ret = nano_bcn_read_efuse(28, &id[1]);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    ret = nano_bcn_read_efuse(29, &id[2]);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    intf_deinit();
    snprintf(data_buff, sizeof(data_buff), "inread %s 0 [%04X%04X%04X]", argv[1], id[2], id[1], id[0]);
    std::cout << std::endl;
    std::cout << data_buff << std::endl;
    return 0;
}

static int get_adv_set_mac(int adv_offset, uint8_t *mac)
{
    int b_offset;
    int adv_type;
    int adv_addr_type;
    int num_fields;
    int sync_pattern_type;
    uint16_t val;
    uint16_t tmp;
    
    b_offset = adv_offset * 2;
    int ret = nano_bcn_read_efuse(b_offset >> 1, &val);
    if (ret != SERIAL_ERR_NO_ERROR) {
        return ret;
    }
    adv_type = val & 0x1;
    b_offset += 9;
    if (0x1 == adv_type) {
        b_offset += 5;
    }
    ret = nano_bcn_read_efuse(b_offset >> 1, &val);
    if (ret != SERIAL_ERR_NO_ERROR) {
        return ret;
    }

    int mac_idx = 5;
    if (0 == (b_offset & 1)) {
        adv_addr_type = val & 0x7;
        mac[mac_idx--] = (val >> 8) & 0xff;
        b_offset += 2;
    }
    else {
        adv_addr_type = (val >> 8) & 0x7;
        b_offset += 1;
    }
    if (0 != adv_addr_type) {
        return CMD_ERR_NO_MAC;
    }
    while (1) {
        ret = nano_bcn_read_efuse(b_offset >> 1, &val);
        if (ret != SERIAL_ERR_NO_ERROR) {
            return ret;
        }
        b_offset += 2;
        mac[mac_idx] = val & 0xff;
        if (0 == mac_idx)
            break;
        mac_idx--;

        mac[mac_idx] = (val >> 8) & 0xff;
        if (0 == mac_idx)
            break;
        mac_idx--;
    }
    return 0;
}

extern "C" int read_mac(int argc, char* argv[])
{
    int ret;
    char data_buff[256];
    uint16_t val;
    int r4_en;
    int offset;
    if (argc < 2) {
        ret = CMD_ERR_ILLEGAL_CMD;
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    snprintf(data_buff, sizeof(data_buff), "\\\\.\\%s", argv[2]);
    ret = intf_setup(data_buff);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }

    ret = nano_bcn_read_efuse(1, &val);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }

    // r2 offset
    if (0 == val) {
        offset = 74;
    }
    else {
        offset = val & 0xff;
        if (0 == offset)
            offset = (val >> 8) & 0xff;
    }

    ret = nano_bcn_read_efuse(37, &val);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    r4_en = val & (3 << 11);

    // r2 word count
    ret = nano_bcn_read_efuse(offset, &val);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    if (0 == val) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << CMD_ERR_EMPTY_CHIP << " []" << std::endl;
        return ret;
    }
    offset = offset + val + 1;

    if (r4_en) {
        ret = nano_bcn_read_efuse(offset, &val);
        if (ret != SERIAL_ERR_NO_ERROR) {
            intf_deinit();
            std::cout << std::endl;
            std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
            return ret;
        }
        offset = offset + val + 1;
    }

    int adv_set_offset[3];
    adv_set_offset[0] = offset;
    adv_set_offset[1] = offset;
    adv_set_offset[2] = offset;
    //std::cout << "r3_start_addr:" << offset << std::endl;


    offset += 1;
    ret = nano_bcn_read_efuse(offset, &val);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    int adv_sets_cnt = val & 0xff;
    if (0 == adv_sets_cnt) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << CMD_ERR_NO_MAC << " []" << std::endl;
        return ret;
    }
  
    adv_set_offset[0] = adv_set_offset[0] + ((val >> 8) & 0xff) / 2;
    //std::cout << "adv_sets_cnt:" << adv_sets_cnt << std::endl;
    //std::cout << "adv_set1_offset:" << adv_set_offset[0] << std::endl;

    offset += 1;
    ret = nano_bcn_read_efuse(offset, &val);
    if (ret != SERIAL_ERR_NO_ERROR) {
        intf_deinit();
        std::cout << std::endl;
        std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
        return ret;
    }
    adv_set_offset[1] = adv_set_offset[1] +(val & 0xff) / 2;
    adv_set_offset[2] = adv_set_offset[2] + ((val >> 8) & 0xff)/2;
    //std::cout << "adv_set2_offset:" << adv_set_offset[1] << std::endl;
    //std::cout << "adv_set3_offset:" << adv_set_offset[2] << std::endl;
    uint8_t adv_addr[3][6];

    for (int i = 0; i < adv_sets_cnt; i++) {
        ret = get_adv_set_mac(adv_set_offset[i], adv_addr[i]);
        if (0 != ret)
        {
            intf_deinit();
            std::cout << std::endl;
            std::cout << "inread " << argv[1] << " " << ret << " []" << std::endl;
            return ret;
        }
    }
    intf_deinit();
    int str_len = snprintf(data_buff, sizeof(data_buff), "inread %s 0 [", argv[1]);
    for (int i = 0; i < adv_sets_cnt; i++) {
        if (i == (adv_sets_cnt - 1)) {
            r4_en = snprintf(data_buff + str_len, sizeof(data_buff) - str_len, "%02X%02X%02X%02X%02X%02X]",
                adv_addr[i][0], adv_addr[i][1], adv_addr[i][2], adv_addr[i][3], adv_addr[i][4], adv_addr[i][5]);
        }
        else {
            r4_en = snprintf(data_buff + str_len, sizeof(data_buff) - str_len, "%02X%02X%02X%02X%02X%02X,",
                adv_addr[i][0], adv_addr[i][1], adv_addr[i][2], adv_addr[i][3], adv_addr[i][4], adv_addr[i][5]);
        }
        str_len += r4_en;
    }
    std::cout << std::endl;
    std::cout << data_buff << std::endl;
    return 0;
}

int main(int argc, char* argv[])
{
    int ret;
    read_cmd_t* p_cmd;
    if (argc > 1) {
        for (int i = 0; i < sizeof(my_cmds) / sizeof(struct read_cmd_s); i++) {
            p_cmd = &my_cmds[i];
            if (0 == strcmp(p_cmd->cmd, argv[1])) {
                ret = p_cmd->operation(argc, argv);
                return ret;
            }
        }
    }
    return print_help(argc, argv);
}