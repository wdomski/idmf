/*
 * Author Wojciech Domski 2015
 * www.domski.pl
 *
 * Port of non real-time api for IDMF board to
 * real-time API for RTDM Xenomai.
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

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "idmf_api.h"
#include <rtdm/rtdm.h>

/*****************************************************************************/
/* open/close board */

/**
 * idmf_board_open - open an IntelliDAQ Multi-Function board
 * @devname:	name of the char device file representing the board
 *
 * This function opens the device file associated with the IntelliDAQ Multi-
 * Function board. 
 *
 * The function either returns the board board or a negative error code.
 */
idmf_board * idmf_open(const char * nDeviceName) {
	int i;
	int err = 0;

	idmf_board * board;

	board = malloc(sizeof(idmf_board));

	board->DeviceName = malloc(strlen(nDeviceName) + 1);
	strcpy(board->DeviceName, nDeviceName);

	board->handle = rt_dev_open(board->DeviceName, 0);
	if (board->handle < 0) {
		err = board->handle;
	}

	if (err < 0)
		return 0;

	for (i = 0; i < NUM_ADCS; i++)
		board->adc_values[i] = 0;

	for (i = 0; i < NUM_PORTS; i++)
		board->port_values[i] = 0;

	board->gpio_values = 0;

	reg_write(board, BCT_PWR, 0);

	reg_write(board, ENC_PWRCTRL, 0xFF);

	return board;
}

/**
 * idmf_board_close - closes an IntelliDAQ Multi-Function board
 * @board:	the board
 *
 * This function closes the device file associated with the IntelliDAQ Multi-
 * Function board. The board is no longer useable after this call.
 */
int idmf_close(idmf_board *board) {

	int err = 0;

	err = rt_dev_close(board->handle);

	free(board->DeviceName);

	free(board);

	return err;
}

static inline void reg_write(idmf_board *board, __u32 address, __u32 value) {
	rt_dev_ioctl(board->handle, REG_WRITE | address, &value);
}

static inline __u32 reg_read(idmf_board *board, __u32 address) {
	__u32 value = 0;

	rt_dev_ioctl(board->handle, REG_READ | address, &value);

	return value;
}

static inline void serial_write(idmf_board *board, __u32 value) {
	int i;

	reg_write(board, ADC_REF, 0x00);

	for (i = 23; i >= 0; i--) {
		if (value & (1 << i)) {
			reg_write(board, ADC_REF, 0x05);
			reg_write(board, ADC_REF, 0x04);
		} else {
			reg_write(board, ADC_REF, 0x01);
			reg_write(board, ADC_REF, 0x00);
		}
	}

	reg_write(board, ADC_REF, 0x02);
}

/*****************************************************************************/
/* DAC functions */

/**
 * idmf_dac_update - updates the outputs of all DACs
 * @board:	the board
 *
 * This function causes the hardware to update all analog outputs according to
 * the digital values stored in the DAC registers.
 */
void idmf_dac_update(idmf_board *board) {
	reg_write(board, DAC_CONF, 0x0000C000);
}

/**
 * idmf_dac_read - reads the current value of a DAC register
 * @board:	the board
 * @channel:	the channel of the DAC on the board
 *
 * This function reads the contents of the specified DAC register. This value
 * does not forcibly equal the current output value of the DAC. The value may
 * have been changed without a subsequent call to idmf_dac_update.
 *
 * This function returns the read register value.
 */
__s16 idmf_dac_read(idmf_board *board, int channel) {
	if ((channel < 0) || (channel >= NUM_DACS))
		return 0;

	return (__s16 ) reg_read(board, DAC_VALUE + channel * 0x04);
}

/**
 * idmf_dac_write - write a new value to a DAC register
 * @board:	the board
 * @channel:	the channel of the DAC on the board
 * @value:	the new value to be written
 *
 * This function writes the specified value to the specified DAC register.
 * This value does not change the output of the DAC until idmf_dac_update is 
 * called.
 */
void idmf_dac_write(idmf_board *board, int channel, __s16 value) {
	if ((channel < 0) || (channel >= NUM_DACS))
		return;

	reg_write(board, DAC_VALUE + channel * 0x04, (__u32 ) value);
}

/*****************************************************************************/
/* ADC functions */

/**
 * idmf_adc_config - configure the on-board analog reference
 * @board:	the board
 * @refadc:	reference voltage of ADC (5.0 V / 65535)
 * @refina:	reference voltage of INA (5.0 V / 65535)
 */
void idmf_adc_config(idmf_board *board, __u16 refadc, __u16 refina) {
	reg_write(board, ADC_DATA, 0x0C);
	serial_write(board, 0x00100000 | refina);
	serial_write(board, 0x00240000 | refadc);
}

/**
 * idmf_adc_request - request ADC
 * @board:	the board
 */
void idmf_adc_request(idmf_board *board) {
	reg_write(board, BCT_ADC, 0x01);
}

/**
 * idmf_adc_update - convert ADC samples
 * @board:	the board
 */
void idmf_adc_run(idmf_board *board) {
	reg_write(board, BCT_ADC, 0x00);
}

/**
 * idmf_adc_update - acquire ADC measurements
 * @board:	the board
 */
void idmf_adc_acquire(idmf_board *board) {
	board->adc_values[5] = reg_read(board, ADC_DATA);
	board->adc_values[4] = reg_read(board, ADC_DATA);
	board->adc_values[1] = reg_read(board, ADC_DATA);
	board->adc_values[0] = reg_read(board, ADC_DATA);
	board->adc_values[3] = reg_read(board, ADC_DATA);
	board->adc_values[2] = reg_read(board, ADC_DATA);
	board->adc_values[7] = reg_read(board, ADC_DATA);
	board->adc_values[6] = reg_read(board, ADC_DATA);
}

/**
 * idmf_adc_update - acquire and convert sample
 *
 * It is strongly not advised to use this function
 * inside RTDM task context!
 *
 * Instead use:
 *
 * idmf_adc_request()
 * rtdm_task_sleep() <- sleep in nanoseconds
 * idmf_adc_run()
 * rtdm_task_sleep() <- sleep in nanoseconds
 * idmf_adc_update()
 *
 * @board:	the board
 */
void idmf_adc_update(idmf_board *board) {
	reg_write(board, BCT_ADC, 0x01);
	usleep(1);
	reg_write(board, BCT_ADC, 0x00);
	usleep(3);

	board->adc_values[5] = reg_read(board, ADC_DATA);
	board->adc_values[4] = reg_read(board, ADC_DATA);
	board->adc_values[1] = reg_read(board, ADC_DATA);
	board->adc_values[0] = reg_read(board, ADC_DATA);
	board->adc_values[3] = reg_read(board, ADC_DATA);
	board->adc_values[2] = reg_read(board, ADC_DATA);
	board->adc_values[7] = reg_read(board, ADC_DATA);
	board->adc_values[6] = reg_read(board, ADC_DATA);
}

/**
 * idmf_adc_read - read value
 * @board:	the board
 * @channel:	the channel of the ADC on the board
 */
__s16 idmf_adc_read(idmf_board *board, int channel) {
	if ((channel < 0) || (channel >= NUM_ADCS))
		return 0;

	return board->adc_values[channel];
}

/*****************************************************************************/
/* port functions */

/**
 * idmf_port_config - configure the direction of the data ports
 * @board:	the board
 * @dirx:	direction of port x, where 0 denotes an input
 * @diry:	direction of port y, where 0 denotes an input
 * @dirz:	direction of port z, where 0 denotes an input
 *
 * This function sets the direction of the data ports. Each port can be
 * configured either as an input or as an output. A direction value of 0 makes
 * the port an input, any other value an output.
 */
void idmf_port_config(idmf_board *board, int dirx, int diry, int dirz) {
	__u8 prt_ctrl = 0x9B;

	if (dirx)
		prt_ctrl &= 0xF6;

	if (diry)
		prt_ctrl &= 0xEF;

	if (dirz)
		prt_ctrl &= 0xFD;

	reg_write(board, PRT_CTRL, (__u32 ) prt_ctrl);
}

/**
 * idmf_port_read - read the current value of a data port
 * @board:	the board
 * @port:	the port on the board
 * @channel:	the channel within the port
 *
 * This function reads the specified port value. The value always reflects the
 * signal on the physical port, independently of the configured direction. When
 * the drive strength of an output port is exceeded the read value may differ
 * from the written value.
 *
 * This function returns the read value.
 */
__u8 idmf_port_read(idmf_board *board, int port, int channel) {
	if (!board)
		return 0;

	if ((port < 0) || (port >= NUM_PORTS))
		return 0;

	if ((channel < -1) || (channel >= NUM_PORT_CHANNELS))
		return 0;
	else if (channel == -1)
		return (__u8 ) reg_read(board, PRT_VALUE + port * 0x04);
	else
		return ((__u8 ) reg_read(board, PRT_VALUE + port * 0x04) >> channel)
				& 0x01;
}

/**
 * idmf_port_write - write a new value to a data port
 * @board:	the board
 * @port:	the port of the DAC on the board
 * @channel:	the channel within the port
 * @value:	the new value to be written
 *
 * This function writes the specified value to the specified DAC register. If
 * the port is configured as an input, the value on the port does not change
 * until the port is configured as an output by calling idmf_port_config.
 */
void idmf_port_write(idmf_board *board, int port, int channel, __u8 value) {
	if (!board)
		return;

	if ((port < 0) || (port >= NUM_PORTS))
		return;

	if ((channel < -1) || (channel >= NUM_PORT_CHANNELS))
		return;
	else if (channel == -1)
		board->port_values[port] = value;
	else
		board->port_values[port] =
				(board->port_values[port] & (~(1 << channel)))
						| ((value ? 1 : 0) << channel);

	reg_write(board, PRT_VALUE + port * 0x04, board->port_values[port]);
}

/*****************************************************************************/
/* gpio functions */

/**
 * idmf_gpio_config - configure the direction of the general-purpose I/O pins
 * @board:	the board
 * @dirs:	the direction bit mask, where 0 denotes an input, 1 an output
 *
 * This function sets the directions of all general-purpose I/O pins. Each pin
 * can be individually configured as an input or as an output. A zero bit makes
 * the corresponding pin an input; a one bit makes it an output.
 */
void idmf_gpio_config(idmf_board *board, __u32 dirs) {
	reg_write(board, GPIO_DIR0, dirs);
	reg_write(board, GPIO_DIR1, dirs);
}

/**
 * idmf_gpio_read - read the current values of the general-purpose I/O pins
 * @board:	the board
 * @channel:	the channel of the gpio on the board
 *
 * This function reads the signals of all general-purpose I/O pins. A zero bit
 * denotes a low signal; a one bit denotes a high signal.
 *
 * This function returns the read values.
 */
__u32 idmf_gpio_read(idmf_board *board, int channel) {
	if (!board)
		return 0;

	if ((channel < -1) || (channel >= NUM_GPIOS))
		return 0;
	else if (channel == -1)
		return reg_read(board, GPIO_IN);
	else
		return (reg_read(board, GPIO_IN) >> channel) & 0x01;
}

/**
 * idmf_gpio_write - write new values to the general-purpose I/O pins
 * @board:	the board
 * @channel:	the channel of the gpio on the board
 *
 * This function writes the specified values to the general-purpose I/O pins.
 * A zero bit denotes a low signal; a one bit denotes a high signal. If a pin
 * is configured as an input, the value on the pin does not change until the
 * pin is configured as an output by calling idmf_gpio_config.
 */
void idmf_gpio_write(idmf_board *board, int channel, __u32 values) {
	if (!board)
		return;

	if ((channel < -1) || (channel >= NUM_GPIOS))
		return;
	else if (channel == -1)
		board->gpio_values = values;
	else
		board->gpio_values = (board->gpio_values & (~(1 << channel)))
				| ((values ? 1 : 0) << channel);

	reg_write(board, GPIO_OUT, board->gpio_values);
}

/*****************************************************************************/
/* enc functions */

/**
 * idmf_enc_config
 * @board:	the board
 * @channel:	the channel of the encoder on the board
 * @mode:	the counter mode (1x, 2x, 4x)
 *
 * This function sets the counter mode of the specified encoder. In mode 1, the
 * counter increments by 1 per 360 degrees, in mode 2, the counter increments
 * by 2 per 360 degrees, and in mode 4, the counter increments by 4 per 360 
 * degrees. Any other value has no effect.
 */
void idmf_enc_config(idmf_board *board, int channel, int mode) {
	__u32 mask;

	if ((channel < 0) || (channel >= NUM_ENCS))
		return;

	switch (mode) {
	case 1:
		mask = 0x00020080;
		break;
	case 2:
		mask = 0x00060090;
		break;
	case 4:
		mask = 0x00069096;
		break;
	default:
		mask = 0;
		break;
	}

	if (mask)
		reg_write(board, MFC_DCR + channel * 0x40, mask);
}

/**
 * idmf_enc_read - read count of encoder
 * @board:	the board
 * @channel:	the channel of the encoder on the board
 *
 * This function reads the current count of the specified encoder as a signed
 * 32-bit number.
 *
 * This function returns the read value.
 */
__s32 idmf_enc_read(idmf_board *board, int channel) {
	if ((channel < 0) || (channel >= NUM_ENCS))
		return 0;

	return (__s32 ) reg_read(board, MFC_CNT + channel * 0x40);
}

/**
 * idmf_enc_write - write count of encoder
 * @board:	the board
 * @channel:	the channel of the encoder on the board
 * @value:	the new value to be written
 *
 * This function writes a new value to the count register of the specified
 * encoder.
 */
void idmf_enc_write(idmf_board *board, int channel, __s32 value) {
	if ((channel < 0) || (channel >= NUM_ENCS))
		return;

	reg_write(board, MFC_CNT + channel * 0x40, (__u32 ) value);
}

/*****************************************************************************/
/* led function */

/**
 * idmf_led_write
 */
void idmf_led_write(idmf_board *board, __u32 value) {
	value = value ? 0x0001 : 0;
	reg_write(board, BCT_LED, value);
}

void idmf_led_read(idmf_board *board, __u32 * value) {
	*value = reg_read(board, BCT_LED);
}
