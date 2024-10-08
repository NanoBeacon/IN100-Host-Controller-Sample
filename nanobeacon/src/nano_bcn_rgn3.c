#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nano_bcn_rgn3.h"
#include "nano_bcn_rgn_private.h"

typedef struct {
	uint16_t size_in_word;
	uint8_t cs_offset[3];
	uint8_t adv_cnt;
	adv_ctl_t adv_ctl_struct[ADV_SET_CNT_MAX];
} adv_ctl_table_t;

static uint8_t adv_data_buffer[96];
static adv_ctl_table_t adv_ctl_table;

void bcn_adv_region_data_reset(void)
{
	uint8_t* pbuffer;
	memset(&adv_ctl_table, 0, sizeof(adv_ctl_table_t));
	adv_ctl_table.cs_offset[0] = 6;
	for (int i = 0; i < ADV_SET_CNT_MAX; i++) {
		pbuffer = &adv_data_buffer[i * 32];
		adv_ctl_table.adv_ctl_struct[i].pkt_ctl.data_buffer = pbuffer;
		adv_ctl_table.adv_ctl_struct[i].pkt_ctl.f0.header[0] = 0x02;
	}
}

int bcn_adv_tx_set(advset_index_t idx, int tx_interval, adv_channel_ctl_t ch_ctl, phy_rate_t phy, adv_mode_t mode)
{
	adv_ctl_t* adv_ctl;
	if (idx > ADV_SET_NUM) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	tx_interval = tx_interval * 1000 / 625;
	if (tx_interval > 0xffffff) {
		tx_interval = 0xffffff;
	}
	adv_ctl->adv_interval[0] = tx_interval & 0xff;
	adv_ctl->adv_interval[1] = (tx_interval >> 8) & 0xff;
	adv_ctl->adv_interval[2] = (tx_interval >> 16) & 0xff;
	adv_ctl->b0.u.adv_type = mode;
	adv_ctl->pkt_ctl.b1.u.phy_rate = phy;
	adv_ctl->b8.u.adv_channel_control = ch_ctl;
	return RET_OK;
}

int bcn_adv_address_set(advset_index_t idx, address_type_t addr_type, const uint8_t *bdaddr, uint16_t addr_gen_interval, uint8_t key_idx)
{
	adv_ctl_t* adv_ctl;
	if (idx > ADV_SET_NUM ) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	if (ADDR_NOT_PRESENT == addr_type) {
		adv_ctl->pkt_ctl.f0.addr_not_present = 1;
	}
	else {
		adv_ctl->addr_privte_data.u.adv_addr_type = addr_type;
		adv_ctl->addr_privte_data.u.addr_key_sel = key_idx;
		adv_ctl->addr_gen_interval[0] = addr_gen_interval & 0xff;
		adv_ctl->addr_gen_interval[1] = (addr_gen_interval >> 8) & 0xff;
		if (NULL != bdaddr) {
			memcpy(adv_ctl->adv_addr, bdaddr, 6);
		}
		if (ADDR_PUBLIC == addr_type) {
			adv_ctl->pkt_ctl.f0.header[0] = 0x02;
		}
		else {
			adv_ctl->pkt_ctl.f0.header[0] = 0x42;
		}
	}
	return RET_OK;
}

void bcn_adv_data_reset(void)
{
	adv_ctl_t* adv_ctl;
	pkt_ctl_t* pkt_ctl;
	pkt_f1to7_t* f1to7;
	for (int i = 0; i < ADV_SET_NUM; i++) {
		adv_ctl = &adv_ctl_table.adv_ctl_struct[i];
		pkt_ctl = &adv_ctl->pkt_ctl;
		pkt_ctl->cur_pos = 0;
		pkt_ctl->f1to7_cnt = 0;
		memset(&pkt_ctl->f[0], 0, sizeof(pkt_f1to7_t)*7);
	}
}

int bcn_adv_data_add_predefined_data(advset_index_t idx, uint8_t* data, uint8_t length, char encrypt_en)
{
	adv_ctl_t* adv_ctl;
	pkt_f1to7_t *f1to7;
	int tmp;
	if (idx > ADV_SET_NUM || NULL == data || length > 31) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	if ((adv_ctl->pkt_ctl.cur_pos + length) >= 31) {
		return RET_ERROR;
	}
	tmp = 0;
	for (int i = 0; i < adv_ctl->pkt_ctl.f1to7_cnt; i++) {
		f1to7 = &adv_ctl->pkt_ctl.f[i];
		tmp += f1to7->length;
	}
	if ((tmp + length) > 31) {
		return RET_ERROR;
	}
	if (adv_ctl->pkt_ctl.f1to7_cnt < 7) {
		f1to7 = &adv_ctl->pkt_ctl.f[adv_ctl->pkt_ctl.f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_PREDEF;
		f1to7->endian = 0;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (adv_ctl->pkt_ctl.f1to7_cnt > 0)) {
			adv_ctl->pkt_ctl.f[adv_ctl->pkt_ctl.f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->length = length;
		memcpy(adv_ctl->pkt_ctl.data_buffer + adv_ctl->pkt_ctl.cur_pos, data, length);
		f1to7->data_offset = adv_ctl->pkt_ctl.cur_pos;
		adv_ctl->pkt_ctl.cur_pos += length;
		
		adv_ctl->pkt_ctl.f1to7_cnt++;

		return RET_OK;
	}
	return RET_ERROR;
}

int bcn_adv_data_add_adv_count(advset_index_t idx,  uint8_t length, uint8_t bigendian,char encrypt_en)
{
	int tmp;
	adv_ctl_t* adv_ctl;
	pkt_ctl_t *pkt_ctl;
	pkt_f1to7_t* f1to7;
	if (idx > ADV_SET_NUM || length > 4) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	pkt_ctl = &adv_ctl->pkt_ctl;
	tmp = 0;
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if ((tmp + length) > 31) {
		return RET_ERROR;
	}
	if (pkt_ctl->f1to7_cnt < 7) {
		f1to7 = &pkt_ctl->f[pkt_ctl->f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_ADV_EVNT_CNT;
		f1to7->endian = bigendian;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (pkt_ctl->f1to7_cnt > 0)) {
			pkt_ctl->f[pkt_ctl->f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->length = length;
		adv_ctl->pkt_ctl.f1to7_cnt++;
		return RET_OK;
	}
	return RET_ERROR;
}

int bcn_adv_data_add_second_count(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en)
{
	int tmp;
	adv_ctl_t* adv_ctl;
	pkt_ctl_t* pkt_ctl;
	pkt_f1to7_t* f1to7;
	if (idx > ADV_SET_NUM || length > 4) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	pkt_ctl = &adv_ctl->pkt_ctl;
	tmp = 0;
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if ((tmp + length) > 31) {
		return RET_ERROR;
	}
	if (pkt_ctl->f1to7_cnt < 7) {
		f1to7 = &pkt_ctl->f[pkt_ctl->f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_TIMER;
		f1to7->u.timer_sel = 1;
		f1to7->endian = bigendian;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (pkt_ctl->f1to7_cnt > 0)) {
			pkt_ctl->f[pkt_ctl->f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->length = length;
		adv_ctl->pkt_ctl.f1to7_cnt++;
		return RET_OK;
	}
	return RET_ERROR;
}

int bcn_adv_data_add_100ms_count(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en)
{
	int tmp;
	adv_ctl_t* adv_ctl;
	pkt_ctl_t* pkt_ctl;
	pkt_f1to7_t* f1to7;
	if (idx > ADV_SET_NUM || length > 4) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	pkt_ctl = &adv_ctl->pkt_ctl;
	tmp = 0;
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if ((tmp + length) > 31) {
		return RET_ERROR;
	}
	if (pkt_ctl->f1to7_cnt < 7) {
		f1to7 = &pkt_ctl->f[pkt_ctl->f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_TIMER;
		f1to7->u.timer_sel = 0;
		f1to7->endian = bigendian;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (pkt_ctl->f1to7_cnt > 0)) {
			pkt_ctl->f[pkt_ctl->f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->length = length;
		adv_ctl->pkt_ctl.f1to7_cnt++;
		return RET_OK;
	}
	return RET_ERROR;
}

int bcn_adv_data_add_gpio_status(advset_index_t idx, char encrypt_en)
{
	int tmp;
	adv_ctl_t* adv_ctl;
	pkt_ctl_t* pkt_ctl;
	pkt_f1to7_t* f1to7;
	if (idx > ADV_SET_NUM) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	pkt_ctl = &adv_ctl->pkt_ctl;
	tmp = 0;
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if ((tmp + 1) > 31) {
		return RET_ERROR;
	}
	if (pkt_ctl->f1to7_cnt < 7) {
		f1to7 = &pkt_ctl->f[pkt_ctl->f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_GPIO_VAL;
		f1to7->endian = 0;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (pkt_ctl->f1to7_cnt > 0)) {
			pkt_ctl->f[pkt_ctl->f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->length = 1;
		adv_ctl->pkt_ctl.f1to7_cnt++;
		return RET_OK;
	}
	return RET_ERROR;
}

int bcn_adv_data_add_vcc(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en)
{
	int tmp;
	adv_ctl_t* adv_ctl;
	pkt_ctl_t* pkt_ctl;
	pkt_f1to7_t* f1to7;
	if (idx > ADV_SET_NUM || length > 4) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	pkt_ctl = &adv_ctl->pkt_ctl;
	tmp = 0;
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if ((tmp + length) > 31) {
		return RET_ERROR;
	}
	if (pkt_ctl->f1to7_cnt < 7) {
		f1to7 = &pkt_ctl->f[pkt_ctl->f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_VBAT;
		f1to7->endian = bigendian;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (pkt_ctl->f1to7_cnt > 0)) {
			pkt_ctl->f[pkt_ctl->f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->length = length;
		adv_ctl->pkt_ctl.f1to7_cnt++;
		return RET_OK;
	}
	return RET_ERROR;
}

int bcn_adv_data_add_temperature(advset_index_t idx, uint8_t length, uint8_t bigendian, char encrypt_en)
{
	int tmp;
	adv_ctl_t* adv_ctl;
	pkt_ctl_t* pkt_ctl;
	pkt_f1to7_t* f1to7;
	if (idx > ADV_SET_NUM || length > 4) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	pkt_ctl = &adv_ctl->pkt_ctl;
	tmp = 0;
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if ((tmp + length) > 31) {
		return RET_ERROR;
	}
	if (pkt_ctl->f1to7_cnt < 7) {
		f1to7 = &pkt_ctl->f[pkt_ctl->f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_TEMP;
		f1to7->endian = bigendian;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (pkt_ctl->f1to7_cnt > 0)) {
			pkt_ctl->f[pkt_ctl->f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->length = length;
		adv_ctl->pkt_ctl.f1to7_cnt++;
		return RET_OK;
	}
	return RET_ERROR;
}

int bcn_adv_data_add_register_read_data(advset_index_t idx, uint16_t address, uint8_t length, uint8_t bigendian, char encrypt_en)
{
	int tmp;
	adv_ctl_t* adv_ctl;
	pkt_ctl_t* pkt_ctl;
	pkt_f1to7_t* f1to7;
	if (idx > ADV_SET_NUM || length > 31) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	pkt_ctl = &adv_ctl->pkt_ctl;
	tmp = 0;
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if ((tmp + length) > 31) {
		return RET_ERROR;
	}
	if (pkt_ctl->f1to7_cnt < 7) {
		f1to7 = &pkt_ctl->f[pkt_ctl->f1to7_cnt];
		f1to7->data_src = ADV_DATA_TYPE_REG_DATA;
		f1to7->endian = bigendian;
		f1to7->encryption = encrypt_en;
		f1to7->encryption_last = encrypt_en;
		if (encrypt_en && (pkt_ctl->f1to7_cnt > 0)) {
			pkt_ctl->f[pkt_ctl->f1to7_cnt - 1].encryption_last = 0;
		}
		f1to7->u.reg_addr = address;
		f1to7->length = length;
		adv_ctl->pkt_ctl.f1to7_cnt++;
		return RET_OK;
	}
	return RET_ERROR;
}
/**
 ****************************************************************************************
 * @brief Set parameters for triggering type advertising
 *
 * @param[in] idx                  index of advertising sets
 * @param[in] gpio_bitmap          bit0~bit7 : GPIO0~GPIO7. 1 enable, 0 disable
 * @param[in] sensor_bitmap        bit0~bit7 :trigger1 low ,
 *                                           trigger2 high,trigger2 low ,
 *                                           trigger3 high,trigger3 low,
 *  										 trigger4 high,trigger4 low.
 *											 1 enable, 0 disable
 * @param[in] event_num            numnber of advertising events for trigger (event-driven) advertising
 * @param[in] event_mode           mode 0,1,2,3
 * @param[in] sensor_check_period  unit is slot. Chip wakeups every specified period and check whether the event is triggered  for sensor  trigger
 *
 * @return    0 if sucessful,  non-0 else
 ****************************************************************************************
 */
int bcn_adv_trigger_setting_set(advset_index_t idx, uint8_t gpio_bitmap, uint8_t sensor_bitmap, uint16_t event_num, uint8_t event_mode, uint32_t sensor_check_period)
{
	adv_ctl_t* adv_ctl;
	if (idx > ADV_SET_NUM) {
		return RET_INVALID_PARAMETER;
	}
	adv_ctl = &adv_ctl_table.adv_ctl_struct[idx];
	adv_ctl->b0.u.trig_source = sensor_bitmap & 0x7f;
	adv_ctl->gpio_source = gpio_bitmap;
	adv_ctl->num_event_control[0] = event_num & 0xff;
	adv_ctl->num_event_control[1] = (event_num >> 8) & 0xff;
	adv_ctl->b8.u.adv_trig_mode = event_mode & 3;
	adv_ctl->evnt_wakeup_period[0] = sensor_check_period & 0xff;
	adv_ctl->evnt_wakeup_period[1] = (sensor_check_period >> 8) & 0xff;
	adv_ctl->evnt_wakeup_period[2] = (sensor_check_period >> 16) & 0xff;
	return RET_OK;
}

static inline int _get_f0_len(pkt_f0_t *f0)
{
	return (1 + f0->header_len);
}

static inline int _get_f1to7_len(pkt_f1to7_t* f)
{
	int f7len = 2;
	if (ADV_DATA_TYPE_PREDEF == f->data_src) {
		f7len += f->length;
	}
	else if (ADV_DATA_TYPE_REG_DATA == f->data_src) {
		f7len += 2;
	}
	return f7len;
}

static int _get_packet_len(pkt_ctl_t* pkt_ctl)
{
	int length = 3;
	if (pkt_ctl->b0.u.sync_pattern_en) {
		length += 4;
	}
	length += _get_f0_len(&pkt_ctl->f0);
	for (int i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		length += _get_f1to7_len(&pkt_ctl->f[i]);
	}
	return length;
}

static int _get_cs_only_len(adv_ctl_t* adv_ctl)
{
	int length = 9;
	if (1 == adv_ctl->b0.u.adv_type) {
		length += 5;
	}
	length++;
	if (ADDR_PUBLIC == adv_ctl->addr_privte_data.u.adv_addr_type) {
		length += 6;
	}
	else if(ADDR_RANDOM_NON_RESOLVABLE == adv_ctl->addr_privte_data.u.adv_addr_type ||
            ADDR_RANDOM_RESOLVABLE == adv_ctl->addr_privte_data.u.adv_addr_type) {
		length += 2;
	}
	return length;
}

static int _get_ctl_struct_total_len(adv_ctl_t* adv_ctl)
{
	int length;
	if (0 == adv_ctl->pkt_ctl.f1to7_cnt) {
		return 0;
	}
	length = _get_cs_only_len(adv_ctl);
	length += _get_packet_len(&adv_ctl->pkt_ctl);
	return length;
}

static int _f1to7_to_raw(pkt_f1to7_t *f, pkt_ctl_t* pkt_ctl, uint8_t* buffer)
{
	int idx = 0;
	uint16_t tmp;
	uint8_t* pdata;
	buffer[idx++] = (f->data_src & 0x1f) | ((f->endian & 0x01) << 5) | ((f->encryption & 0x01) << 6) | ((f->encryption_last & 0x01) << 7);
	switch (f->data_src)
	{
	case ADV_DATA_TYPE_PREDEF:
		buffer[idx++] = (f->length & 0x3f) | ((f->encryption_output_order & 1) << 6) | ((f->msb_lsb_sel & 1) << 7);
		pdata = pkt_ctl->data_buffer + f->data_offset;
		for (int i = 0; i < f->length; i++) {
			buffer[idx++] = pdata[i];
		}
		break;
	case ADV_DATA_TYPE_TIMER:
		buffer[idx++] = (f->u.timer_sel & 0x07) | ((f->length & 0x07) << 3)| ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_RAND:
		buffer[idx++] = (f->u.rand_num_sel & 0x03) | ((f->length & 0x0f) << 2) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_TEMP:
	case ADV_DATA_TYPE_VBAT:
		buffer[idx++] =  ((f->length - 1) & 0x01) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_SENSOR:
		buffer[idx++] = (f->u.sensor_idx & 0x0f) | ((f->length & 0x03) << 4) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_TAG:
		buffer[idx++] = (f->length & 0x0f) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_MTS_DATA:
		buffer[idx++] = (f->u.sensor_idx & 0x0f) | ((f->length & 0x03) << 4) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_CUS_UUID:
	case ADV_DATA_TYPE_INP_UUID:
		buffer[idx++] = (f->length & 0x07) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_EID:
		buffer[idx++] = 8 | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_ADV_EVNT_CNT:
		buffer[idx++] = (f->length & 0x3f) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_SLEEP_CNT:
		buffer[idx++] = ((f->length - 1) & 0x1) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_GPIO_VAL:
		buffer[idx++] = (f->u.gpio_sel & 0x03) | ((f->length & 0x01) << 2) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_PLSDTCT:
		buffer[idx++] = (f->u.plsdtct_sel & 0x01) | (((f->length - 1) & 0x01) << 1) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_GPIO_CNT:
		buffer[idx++] = (f->length & 0x3) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	case ADV_DATA_TYPE_REG_DATA:
		buffer[idx++] = (f->length & 0x3f) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		if (f->endian) {
			tmp = f->u.reg_addr + f->length - 1;
		}
		else {
			tmp = f->u.reg_addr;
		}
		buffer[idx++] = tmp & 0xff;
		buffer[idx++] = (tmp >> 8) & 0xff;
		break;
	default:
		buffer[idx++] = (f->length & 0x3f) | ((f->encryption_output_order & 0x01) << 6) | ((f->msb_lsb_sel & 1) << 7);
		break;
	}
	return idx;
}

static int _pkt_ctl_to_raw(pkt_ctl_t *pkt_ctl, uint8_t* buffer)
{
	int idx = 0;
	int i;
	int tmp;
	pkt_ctl->b0.u.num_fields = 1 + pkt_ctl->f1to7_cnt;
	buffer[idx++] = pkt_ctl->b0.data;
	buffer[idx++] = pkt_ctl->b1.data;
	buffer[idx++] = pkt_ctl->b2.data;
	if (pkt_ctl->b0.u.sync_pattern_en) {
		buffer[idx++] = pkt_ctl->sync_pattern & 0xff;
		buffer[idx++] = (pkt_ctl->sync_pattern >> 8) & 0xff;
		buffer[idx++] = (pkt_ctl->sync_pattern >> 16) & 0xff;
		buffer[idx++] = (pkt_ctl->sync_pattern >> 24) & 0xff;
	}
	/*field 0*/
	tmp = 0;
	for (i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		tmp += pkt_ctl->f[i].length;
	}
	if (pkt_ctl->f0.addr_not_present) {
		pkt_ctl->f0.header[1] = tmp;
	}
	else
	{
		pkt_ctl->f0.header[1] = tmp + 6;
	}
	pkt_ctl->f0.header_len = 2;
	buffer[idx++] = ((pkt_ctl->f0.header_len - 2) & 0x03) | ((pkt_ctl->f0.addr_not_present & 0x01) << 2);
	for (i = 0; i < pkt_ctl->f0.header_len; i++) {
		buffer[idx++] = pkt_ctl->f0.header[i];
		//printf("header %d %d\n", i, pkt_ctl->f0.header[i]);
	}

	/*field 1 ~ 7*/
	for (i = 0; i < pkt_ctl->f1to7_cnt; i++) {
		idx += _f1to7_to_raw(&pkt_ctl->f[i], pkt_ctl, buffer + idx);
	}
	return idx;
}
static int _adv_ctl_struct_to_raw(adv_ctl_t* adv_ctl, uint8_t cs_offset,  uint8_t* buffer)
{
	int idx = 0;
	int i;
	uint8_t* pbuff = buffer + cs_offset;
	if (0 == adv_ctl->pkt_ctl.f1to7_cnt) {
		return 0;
	}
	adv_ctl->packet_table_location = cs_offset + _get_cs_only_len(adv_ctl);
	pbuff[idx++] = adv_ctl->b0.data;
	pbuff[idx++] = adv_ctl->gpio_source;
	pbuff[idx++] = adv_ctl->b2;
	pbuff[idx++] = adv_ctl->adv_interval[0];
	pbuff[idx++] = adv_ctl->adv_interval[1];
	pbuff[idx++] = adv_ctl->adv_interval[2];
	pbuff[idx++] = adv_ctl->packet_table_location & 0xff;
	pbuff[idx++] = (adv_ctl->packet_table_location >> 8) & 0xff;
	pbuff[idx++] = adv_ctl->b8.data;
	if (1 == adv_ctl->b0.u.adv_type) {
		pbuff[idx++] = adv_ctl->evnt_wakeup_period[0];
		pbuff[idx++] = adv_ctl->evnt_wakeup_period[1];
		pbuff[idx++] = adv_ctl->evnt_wakeup_period[2];
		pbuff[idx++] = adv_ctl->num_event_control[0];
		pbuff[idx++] = adv_ctl->num_event_control[1];
	}
	pbuff[idx++] = adv_ctl->addr_privte_data.data;
	if (ADDR_PUBLIC == adv_ctl->addr_privte_data.u.adv_addr_type) {
		for (i = 0; i < 6; i++) {
			pbuff[idx++] = adv_ctl->adv_addr[i];
		}
	}
	else if (ADDR_RANDOM_NON_RESOLVABLE == adv_ctl->addr_privte_data.u.adv_addr_type ||
             ADDR_RANDOM_RESOLVABLE == adv_ctl->addr_privte_data.u.adv_addr_type) {
		pbuff[idx++] = adv_ctl->addr_gen_interval[0];
		pbuff[idx++] = adv_ctl->addr_gen_interval[1];
	}
	idx += _pkt_ctl_to_raw(&adv_ctl->pkt_ctl, pbuff+idx);
	return idx;
}

int bcn_adv_data_to_binary(uint8_t *buffer, uint16_t size)
{
	adv_ctl_t* adv_ctl;
	uint16_t idx = 1;
	uint8_t adv_set_idx = 0;
	uint16_t total_size = 0;
	adv_ctl_table.size_in_word = 0;
    adv_ctl_table.cs_offset[0] = 6;
    adv_ctl_table.cs_offset[1] =0;
    adv_ctl_table.cs_offset[2] =0;
	for (int i = 0; i < ADV_SET_CNT_MAX; i++) {
		adv_ctl = &adv_ctl_table.adv_ctl_struct[i];
		if (0 == adv_ctl->pkt_ctl.f1to7_cnt) {
			continue;
		}
		adv_ctl_table.cs_offset[adv_set_idx] = total_size + 6;
		total_size += _adv_ctl_struct_to_raw(adv_ctl, adv_ctl_table.cs_offset[adv_set_idx], buffer);
		adv_set_idx++;
	}
	adv_ctl_table.adv_cnt = adv_set_idx;
	if (1 == total_size % 2) {
		buffer[total_size + 6] = 0;
		total_size += 1;
	}

	if (total_size > 0) {
		adv_ctl_table.size_in_word = total_size / 2 + 2;
		buffer[0] = adv_ctl_table.size_in_word & 0xff;
		buffer[1] = (adv_ctl_table.size_in_word >> 8) & 0xff;
		buffer[2] = adv_set_idx;
		buffer[3] = adv_ctl_table.cs_offset[0];
		buffer[4] = adv_ctl_table.cs_offset[1];
		buffer[5] = adv_ctl_table.cs_offset[2];
	}
	return (total_size + 6);
}
