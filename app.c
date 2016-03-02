/***************************************************************************
 *   Copyright (C) 2015 by Wojciech Domski                                 *
 *   Wojciech.Domski@gmail.com                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "idmf_api.h"

idmf_board * idmf;

/*
the names of devices' are idmfX where X starts at 0 and is incremented by
one
idmf0 idmf1 ....
*/
char device[64] = "idmf0";

int main(int argc, char * argv[]) {
	int err = 0;
	int i = 0, c = 0;
	int per = 0;
	int mode = 0;
	int channel = 0;
	int value = 0;

	if (argc == 2) {
		strcpy(device, argv[1]);
	}

	printf("Init %s\n", device);

	idmf = idmf_open(device);

	if (!idmf) {
		printf("Error while opening device\n");

		return -1;
	}

	do {
		printf(
				"Select:\n0 - LED test\n1 - GPIO\n2 - ADC\n5 - exit\nYour choice: ");
		scanf("%d", &per);

		switch (per) {
		case 0:
			idmf_led_read(idmf, &c);
			printf("initial value: %d\n\n", c);

			//blink
			for (i = 0; i < 10; ++i) {
				usleep(200000);
				idmf_led_write(idmf, 1);
				idmf_led_read(idmf, &c);
				printf("LED 1 -> %d\n", c);

				usleep(200000);
				idmf_led_write(idmf, 0);
				idmf_led_read(idmf, &c);
				printf("LED 0 -> %d\n", c);
			}

			printf("final value: ");
			scanf("%d", &c);
			idmf_led_write(idmf, c);
			break;
		case 1:
			printf("Select:\n0 - read\n1 - write\n\nYour choice: ");
			scanf("%d", &mode);
			printf("Channel: ");
			scanf("%d", &channel);

			if (mode == 1) {
				printf("Value (0 or 1): ");
				scanf("%d", &value);

				//set as output
				idmf_gpio_config(idmf, 0x01 << channel);
				idmf_gpio_write(idmf, channel, value);
			} else {
				//set all as input
				idmf_gpio_config(idmf, 0x00);
				value = idmf_gpio_read(idmf, channel);
				printf("Value : %d\n", value);
			}

			break;
		case 2:
			printf("Channel: ");
			scanf("%d", &channel);
			idmf_adc_config(idmf, (__u16 ) (65536.0 * 3.2768 / 5.0),
					(__u16 ) (65536.0 * 4.0100 / 5.0));
			idmf_adc_request(idmf);
			usleep(1);	//in RTDM task context use rtdm_task_sleep() <- sleep in nanoseconds
			idmf_adc_run(idmf);
			usleep(3);  //in RTDM task context use rtdm_task_sleep() <- sleep in nanoseconds
			idmf_adc_acquire(idmf);
			value = idmf_adc_read(idmf, channel);
			printf("Value : %d\n", value);

			break;

		case 5:
			break;
		default:
			printf("Unknown option\n");
			break;
		}

		if (per == 5) {
			break;
		}
	} while (1);

	err = idmf_close(idmf);

	if (err < 0) {
		printf("Error while closing device %d\n", err);
	}

	return 0;
}

