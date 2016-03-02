/*
 * Modifications:
 *
 * 	Wojciech Domski 2015
 * 	www.domski.pl
 *
 * Work based on:
 *
 * Mecovis IntelliDAQ Multi-Function Series API
 * Copyright (C) 2011, Mecovis GmbH.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef __IDMF_API_H
#define __IDMF_API_H

#include <linux/types.h>

#define NUM_DACS		 8
#define NUM_ADCS		 8
#define NUM_PORTS		 3
#define NUM_PORT_CHANNELS	 8
#define NUM_ENCS		 8
#define NUM_GPIOS		24

#define DAC_CONF	0x0000
#define DAC_VALUE	0x0020
#define PRT_VALUE	0x0080
#define PRT_CTRL	0x008C
#define ENC_PWRCTRL	0x0090
#define GPIO_DIR0	0x0094
#define GPIO_DIR1	0x0098
#define ENC_ALARM0	0x009C
#define ENC_ALARM1	0x00A0
#define ENC_PWRSTAT	0x00A4
#define ADC_DATA	0x00A8
#define ADC_REF		0x00AC
#define BCT_LED		0x0200
#define BCT_PWR		0x0204
#define BCT_ADC		0x0208
#define GPIO_IN		0x0210
#define GPIO_OUT	0x0214
#define MFC_CCR		0x0300
#define MFC_CSR		0x0304
#define MFC_CNT		0x0308
#define MFC_PLV		0x030C
#define MFC_DCR		0x0318

#define REG_WRITE	0x10000000
#define REG_READ	0x20000000

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char * DeviceName;

	int handle;

	__s16 adc_values[NUM_ADCS];
	__u8 port_values[NUM_PORTS];
	__u32 gpio_values;
} idmf_board;

idmf_board * idmf_open(const char * nDeviceName);
int idmf_close(idmf_board *board);

static inline void reg_write(idmf_board *board, __u32 address, __u32 value);

static inline __u32 reg_read(idmf_board *board, __u32 address);

__s16 idmf_dac_read(idmf_board *board, int channel);
void idmf_dac_write(idmf_board *board, int channel, __s16 value);
void idmf_dac_update(idmf_board *board);

void idmf_adc_config(idmf_board *board, __u16 refadc, __u16 refina);
void idmf_adc_update(idmf_board *board);
__s16 idmf_adc_read(idmf_board *board, int channel);

void idmf_port_config(idmf_board *board, int dirx, int diry, int dirz);
__u8 idmf_port_read(idmf_board *board, int port, int channel);
void idmf_port_write(idmf_board *board, int port, int channel, __u8 value);

void idmf_gpio_config(idmf_board *board, __u32 dirs);
__u32 idmf_gpio_read(idmf_board *board, int channel);
void idmf_gpio_write(idmf_board *board, int channel, __u32 value);

void idmf_enc_config(idmf_board *board, int channel, int mode);
__s32 idmf_enc_read(idmf_board *board, int channel);
void idmf_enc_write(idmf_board *board, int channel, __s32 value);

void idmf_led_write(idmf_board *board, __u32 value);

#ifdef __cplusplus
}
#endif

#endif
