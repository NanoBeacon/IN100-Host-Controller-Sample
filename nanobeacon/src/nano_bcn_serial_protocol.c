#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nano_bcn_serial_protocol.h"
/// Uart protocol defines

#define HEAD 	0xAE
#define TAIL 	0xEA

enum op_code {
	OP_READY = 0x01,
	OP_RATE = 0x02,
	OP_RD_REG = 0x03,
	OP_WR_REG = 0x04,
	OP_RD_MEM = 0x05,
	OP_WR_MEM = 0x06,
	OP_RD_EFUSE = 0x07,
	OP_WR_EFUSE = 0x08,
	OP_MBIST = 0x0A,
};

enum {
	ACK = 0x80,
	NAK = 0x81,
};

#define PAYLOAD_READY 0xAA

typedef struct {
	int ready;
	host_itf_t hif;

} nano_bcn_dev_t;

static nano_bcn_dev_t g_dev;

static int nak_error(uint8_t err_code)
{
	int res;

	switch (err_code) {
		case 0x00:
			res = NANO_BCN_ERR_NO_ERROR;
			break;
		case 0x01:
			res = NANO_BCN_ERR_RX_TMO;
			break;
		case 0x02:
			res = NANO_BCN_ERR_ILLEGAL_TAIL_BYTE;
			break;
		case 0x03:
			res = NANO_BCN_ERR_ILLEGAL_CMD;
			break;
		case 0x10:
			res = NANO_BCN_ERR_UART_BI;
			break;
		case 0x11:
			res = NANO_BCN_ERR_UART_FE;
			break;
		case 0x20:
		case 0x21:
		case 0x28:
			res = NANO_BCN_ERR_ILLEGAL_ADDR;
			break;
		case 0x30:
			res = NANO_BCN_ERR_HCI_PARSE;
			break;
		default:
			res = 	NANO_BCN_ERR_UNKNOWN;
			break;
	}		

	return res;

}

static int clear_uart_rx_buffer()
{
	host_itf_t *p_hif = &g_dev.hif;
	int res = NANO_BCN_ERR_NO_ERROR;
	uint8_t buf[4];
	do{
	    res = p_hif->serial_rx(buf, 1, 10);
	}
	while(UART_ERR_NO_ERROR == res);
	return res;
}


static int uart_read(uint8_t opcode, uint16_t addr, uint8_t *data)
{
	host_itf_t *p_hif = &g_dev.hif;
	int res = NANO_BCN_ERR_NO_ERROR;
	uint8_t buf[8];
	uint8_t outbuff[16];
	uint16_t cmd_size;
	uint16_t reply_size;
	uint16_t offset = 0;
	buf[0] = HEAD;
	buf[1] = opcode;
	if (opcode == OP_RD_EFUSE) {
		buf[2] = 1;
		buf[3] = (uint8_t)(addr & 0xff);
		buf[4] = TAIL;
		cmd_size = 5;
	} else {
		buf[2] = 2;
		buf[3] = (uint8_t)(addr >> 8);
		buf[4] = (uint8_t)(addr);
		buf[5] = TAIL;
		cmd_size = 6;
	}
	
	if ((res = p_hif->serial_tx(buf, cmd_size, 100)) != UART_ERR_NO_ERROR) {
		return NANO_BCN_ERR_UART;
	}
	p_hif->delay(1);

	if (opcode == OP_RD_EFUSE) {
		reply_size = 6;
	} else {
		reply_size = 8;
	}
	memset(outbuff, 0, sizeof(outbuff));
	res = p_hif->serial_rx(outbuff, reply_size, 100);
	if (res == UART_ERR_NO_ERROR)
	{
		uint16_t size = cmd_size > reply_size ? reply_size : cmd_size;
		if (0 == memcmp(buf, outbuff, size))
		{
			/* for UART 1-wire mode, TX and RX connected together */
			res = p_hif->serial_rx(outbuff + reply_size, cmd_size, 100);
			offset = cmd_size;
		}
	}
	if (res == UART_ERR_NO_ERROR) {
		res = NANO_BCN_ERR_BAD_RESP;
		if ((outbuff[offset] == HEAD) && (outbuff[offset + reply_size -1] == TAIL)) {
			if (outbuff[offset + 1] == (0x80 |opcode)) {
				if (opcode == OP_RD_EFUSE) {
					if (outbuff[offset + 2] == 2) {
						res = NANO_BCN_ERR_NO_ERROR;
						data[0] = outbuff[offset + 3];
						data[1] = outbuff[offset + 4];
					}
				} else {
					if (outbuff[offset + 2] == 4) {
						res = NANO_BCN_ERR_NO_ERROR;
						data[0] = outbuff[offset + 3];
						data[1] = outbuff[offset + 4];
						data[2] = outbuff[offset + 5];
						data[3] = outbuff[offset + 6];
					}
				}
			} 
		} 
		else
		{
			clear_uart_rx_buffer();
		}
	} else {
		if (res == UART_ERR_TMO) {
			res = NANO_BCN_ERR_RX_TMO;
		} else {
			res = NANO_BCN_ERR_UART;
		}
	}

	return res;
}


static int uart_write(uint8_t opcode, uint16_t addr, uint32_t wd, uint8_t *p_rd)
{
	host_itf_t *p_hif = &g_dev.hif;
	int res;
	uint8_t buf[12];
	uint8_t outbuff[20];
	uint16_t cmd_size;
	uint16_t reply_size;
	uint16_t offset = 0;

	buf[0] = HEAD;
	buf[1] = opcode;
	if (opcode == OP_WR_EFUSE) {
		buf[2] = 2;
		buf[3] = (uint8_t)(addr >> 4);
		buf[4] = (uint8_t)(addr & 0xF);
		buf[5] = TAIL;
		cmd_size = 6;
	} else {
		buf[2] = 6;
		buf[3] = (uint8_t)(addr >> 8);
		buf[4] = (uint8_t)(addr);
		buf[5] = (uint8_t)(wd >> 24);
		buf[6] = (uint8_t)(wd >> 16);
		buf[7] = (uint8_t)(wd >> 8);
		buf[8] = (uint8_t)(wd >> 0);
		buf[9] = TAIL;
		cmd_size = 10;
	}
	
	if ((res = p_hif->serial_tx(buf, cmd_size, 100)) != UART_ERR_NO_ERROR) {
		return NANO_BCN_ERR_UART;
	}
	p_hif->delay(1);

	if ((opcode == OP_RD_EFUSE)||(OP_WR_EFUSE == opcode)) {
		reply_size = 6;
	} else {
		reply_size = 8;
	}
	memset(outbuff, 0, sizeof(outbuff));
	res = p_hif->serial_rx(outbuff, reply_size, 100);
	if (res == UART_ERR_NO_ERROR)
	{
		uint16_t size = cmd_size > reply_size ? reply_size : cmd_size;
		if (0 == memcmp(buf, outbuff, size))
		{
			/* for UART 1-wire mode, TX and RX connected together */
			res = p_hif->serial_rx(outbuff + reply_size, cmd_size, 100);
			offset = cmd_size;
		}
	}
	if (res == UART_ERR_NO_ERROR) {
		res = NANO_BCN_ERR_BAD_RESP;
		if ((outbuff[offset] == HEAD) && (outbuff[offset + reply_size -1] == TAIL)) {
			if (outbuff[offset + 1] == (0x80 |opcode)) {
				if ((opcode == OP_RD_EFUSE) || (OP_WR_EFUSE == opcode)) {
					if (outbuff[offset + 2] == 2) {
						res = NANO_BCN_ERR_NO_ERROR;
						if (p_rd) {
							p_rd[0] = outbuff[offset + 3];
							p_rd[1] = outbuff[offset + 4];
						}
					}
				} else {
					if (outbuff[offset + 2] == 4) {
						res = NANO_BCN_ERR_NO_ERROR;
						if (p_rd) {
							p_rd[0] = outbuff[offset + 3];
							p_rd[1] = outbuff[offset + 4];
							p_rd[2] = outbuff[offset + 5];
							p_rd[3] = outbuff[offset + 6];
						}
					}
				}
			} 
		} 
		else
		{
			clear_uart_rx_buffer();
		}
	} else {
		if (res == UART_ERR_TMO) {
			res = NANO_BCN_ERR_RX_TMO;
		} else {
			res = NANO_BCN_ERR_UART;
		}
	}

	return res;
}

static int send_ready(int * baud_rate)
{
	host_itf_t *p_hif = &g_dev.hif;
	int res;
	uint8_t buf[8];
	uint8_t outbuff[16];
	uint16_t offset = 0;
	buf[0] = HEAD;
	buf[1] = OP_READY;
	buf[2] = 0x01;	// length
	buf[3] = PAYLOAD_READY;
	buf[4] = TAIL;

	if ((res = p_hif->serial_tx(buf, 5, 100)) != UART_ERR_NO_ERROR) {
		return NANO_BCN_ERR_UART;
	}

	int retry = 0;
	memset(outbuff, 0, sizeof(outbuff));
	while (1) {
		res = p_hif->serial_rx(outbuff, 7, 100);
		if (res == UART_ERR_NO_ERROR)
		{
			if (0 == memcmp(buf, outbuff, 5))
			{
				res = p_hif->serial_rx(outbuff + 7, 5, 100);
				offset = 5;
			}
		}
		if (res == UART_ERR_NO_ERROR) {
			res = NANO_BCN_ERR_BAD_RESP;
			if ((outbuff[offset + 0] == HEAD) && (outbuff[offset + 6] == TAIL)) {
				if (outbuff[1] == ACK) {
					if (outbuff[offset + 2] == 3) {
						res = NANO_BCN_ERR_NO_ERROR;
						uint32_t baud = (outbuff[offset + 5]<<16)|(outbuff[offset + 4]<<8)| outbuff[offset + 3];

						float frac = outbuff[offset + 5];
						frac = frac / 16;
						uint32_t int_div = (outbuff[offset + 3] << 8) + outbuff[offset + 4];
						frac = (frac + int_div) * 16;
						if (baud_rate)
						{
							*baud_rate = (int)(26000000 / frac);
						}
					}
				} else if (outbuff[offset + 1] == NAK) {
					if (outbuff[offset + 2] == 1) {
						res = nak_error(outbuff[offset + 3]);
					}
				} else {
					res = NANO_BCN_ERR_UNKNOWN;
				}
			}
			else
			{
				clear_uart_rx_buffer();
			}
			break;
		} else {
			if (res == UART_ERR_TMO) {
				retry += 1;
				if (retry >= 3) {
					res = 	NANO_BCN_ERR_RX_TMO;
					break;
				}
			} else {
				res = NANO_BCN_ERR_UART;
				break;
			}
		}
	}

	return res;
} 

int nano_bcn_uart_init(host_itf_t *p_hif)
{
	if (!p_hif->delay)
		return NANO_BCN_ERR_INVALID_PARAM;

	if (!p_hif->serial_break)
	{
		//Some devices' serial ports do not support break signals
		//return NANO_BCN_ERR_INVALID_PARAM;
	}
	if (!p_hif->serial_rx) 
		return NANO_BCN_ERR_INVALID_PARAM;

	if (!p_hif->serial_tx) 
		return NANO_BCN_ERR_INVALID_PARAM;

	memset(&g_dev, 0, sizeof(nano_bcn_dev_t));

	g_dev.hif.serial_tx = p_hif->serial_tx;
	g_dev.hif.serial_rx = p_hif->serial_rx;
	g_dev.hif.delay = p_hif->delay;
	g_dev.hif.serial_break = p_hif->serial_break;

	int res = NANO_BCN_ERR_NO_ERROR;
	clear_uart_rx_buffer();
	res = send_ready(NULL);
	if (res != NANO_BCN_ERR_NO_ERROR)
	{
		clear_uart_rx_buffer();
		res = send_ready(NULL);
	}
	if (res == NANO_BCN_ERR_NO_ERROR) {
		g_dev.ready = 1;
	}	
	return res;
}

int nano_bcn_uart_baud_rate_auto_detect(int *baud_rate)
{
	nano_bcn_dev_t* pd = &g_dev;
	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;
	int res = NANO_BCN_ERR_NO_ERROR;
	clear_uart_rx_buffer();
	res = send_ready(baud_rate);
	if (res != NANO_BCN_ERR_NO_ERROR)
	{
		clear_uart_rx_buffer();
		res = send_ready(baud_rate);
	}
	return res;
}

void nano_bcn_uart_deinit(void)
{
	nano_bcn_dev_t *pd = &g_dev;
	
	if (pd->ready)
		pd->ready = 0;

}

int nano_bcn_read_reg(uint16_t adr, uint32_t *p_rd)
{
	nano_bcn_dev_t *pd = &g_dev;
	int res;
	uint8_t buf[4];

	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;

	if (!p_rd)
		return NANO_BCN_ERR_INVALID_PARAM;
   
	res = uart_read(OP_RD_REG, adr, buf);
	if (res == NANO_BCN_ERR_NO_ERROR) {
		*p_rd = buf[3]|(buf[2]<<8)|(buf[1]<<16)|(buf[0]<<24);
	}

	return res;
}

int nano_bcn_write_reg(uint16_t adr, uint32_t wd, uint32_t *p_rd)
{
	nano_bcn_dev_t *pd = &g_dev;
	int res;
	uint8_t buf[4];

	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;

	res = uart_write(OP_WR_REG, adr, wd, buf);
	if (p_rd) {
		if (res == NANO_BCN_ERR_NO_ERROR) {
			*p_rd = buf[3]|(buf[2]<<8)|(buf[1]<<16)|(buf[0]<<24);
		}
	}

	return res;
}

int nano_bcn_read_mem(uint16_t adr, uint32_t *p_rd)
{
	nano_bcn_dev_t *pd = &g_dev;
	int res;
	uint8_t buf[4];

	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;

	if (!p_rd)
		return NANO_BCN_ERR_INVALID_PARAM;

	res = uart_read(OP_RD_MEM, adr, buf);
	if (res == NANO_BCN_ERR_NO_ERROR) {
		*p_rd = buf[3]|(buf[2]<<8)|(buf[1]<<16)|(buf[0]<<24);
	}

	return res;
}

int nano_bcn_write_mem(uint16_t adr, uint32_t wd, uint32_t *p_rd)
{
	nano_bcn_dev_t *pd = &g_dev;
	int res;
	uint8_t buf[4];

	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;

	res = uart_write(OP_WR_MEM, adr, wd, buf);
	if (p_rd) {
		if (res == NANO_BCN_ERR_NO_ERROR) {
			*p_rd = buf[0]|(buf[1]<<8)|(buf[2]<<16)|(buf[3]<<24);
		}
	}

	return res;
}

int nano_bcn_read_efuse(uint8_t aid, uint16_t *p_rd)
{
	nano_bcn_dev_t *pd = &g_dev;
	int res;
	uint8_t buf[2];

	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;

	if (!p_rd)
		return NANO_BCN_ERR_INVALID_PARAM;

	res = uart_read(OP_RD_EFUSE, aid, buf);
	if (res == NANO_BCN_ERR_NO_ERROR) {
		*p_rd = buf[0]|(buf[1]<<8);
	}

	return res;
}

int nano_bcn_write_efuse(uint8_t aid, uint16_t wd, uint16_t *p_rd)
{
	nano_bcn_dev_t *pd = &g_dev;
	int res;
	uint8_t buf[4];
	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;

	// Write
	for (uint32_t i = 0; i < 16; i++) {
		if (wd & (1<<i)) {
			uint16_t bit_adr = (aid << 4)|(i);
			res = uart_write(OP_WR_EFUSE, bit_adr, 0, NULL);
			if (res != NANO_BCN_ERR_NO_ERROR)
				break;
		}
	}

	// Read back new value
	if (p_rd) {
		res = uart_read(OP_RD_EFUSE, aid, buf);
		if (res == NANO_BCN_ERR_NO_ERROR) {
			*p_rd = buf[0]|(buf[1]<<8);
		}
	}

	return res;
}

int nano_bcn_chip_reset(void)
{
	int ret;
	uint8_t buf[12];
	nano_bcn_dev_t* pd = &g_dev;
	host_itf_t* p_hif = &g_dev.hif;
	if (!pd->ready)
		return NANO_BCN_ERR_NOT_READY;

	buf[0] = HEAD;
	buf[1] = OP_WR_REG;
	buf[2] = 6;
	buf[3] = 0x33;
	buf[4] = 0x30;
	buf[5] = 0;
	buf[6] = 0;
	buf[7] = 0;
	buf[8] = 0;
	buf[9] = TAIL;
	if ((ret = p_hif->serial_tx(buf, 10, 100)) != UART_ERR_NO_ERROR) {
		return NANO_BCN_ERR_UART;
	}
	p_hif->delay(30);
	clear_uart_rx_buffer();
	send_ready(NULL);
	return ret;
}

void nano_bcn_sleep_by_uart_break_on()
{
	nano_bcn_dev_t* pd = &g_dev;
	host_itf_t* p_hif = &g_dev.hif;
	if (!pd->ready || NULL == p_hif->serial_break)
		return ;
	p_hif->serial_break(1);
}

void nano_bcn_wakeup_by_uart_break_off()
{
	nano_bcn_dev_t* pd = &g_dev;
	host_itf_t* p_hif = &g_dev.hif;
	if (!pd->ready || NULL == p_hif->serial_break)
		return ;
	p_hif->serial_break(0);
}