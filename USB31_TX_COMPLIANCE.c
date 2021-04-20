/* ©  [2018] Microchip Technology Inc. and its subsidiaries.

Subject to your compliance with these terms, you may use Microchip software and
any derivatives exclusively with Microchip products. It is your responsibility
to comply with third party license terms applicable to your use of third party
software (including open source software) that may accompany Microchip software.

THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER EXPRESS,
IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES
OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN
NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN
ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST
EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU
HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

Author: Connor Chilton <connor.chilton@microchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libusb.h>
#include <semaphore.h>
#include <errno.h>

/* the buffer sizes can exceed the USB MTU */
#define MAX_CTL_XFER	64
#define MAX_BULK_XFER	512


/**
 * struct my_usb_device - struct that ties USB related stuff
 * @dev: pointer libusb devic
 * @dev_handle: device handle for USB devices
 * @ctx: context of the current session
 * @device_desc: structure that holds the device descriptor fields
 * @inbuf: buffer for USB IN communication
 * @outbut: buffer for USB OUT communication
 */

struct libusb_session {
	libusb_device **dev;
	libusb_device_handle *dev_handle;
	libusb_context *ctx;
	struct libusb_device_descriptor device_desc;
	unsigned char inbuf[MAX_CTL_XFER];
	unsigned char outbuf[MAX_BULK_XFER];
	uint16_t wIndex;
	int port_num;
};

static struct libusb_session session;

#define LINK_COMPLIANCE_MODE 0x0A

int main(int argc, char **argv)
{
	int ret, port, err, len;
	unsigned int port_select;
	unsigned int vendor_id, product_id;
	unsigned int timeout_set = 50000000;
	unsigned char *data = 0;
	ssize_t cnt;
	uint8_t bmRequestType = 0x23, bRequest = 0x03;
	uint16_t wLength = 0x0000, wValue = 0x0005;


VID_INPUT:
	printf("Please enter the Vendor ID of the USB3.1 HUB under test\n0x");
	err = scanf("%x", &vendor_id);
	if (!err) {
		printf("Please enter 4 digit hex only (len=%d, 0x%x)\n", err, vendor_id);
		goto VID_INPUT;
	}

PID_INPUT:
	printf("Please enter the product ID of the USB3.1 hub under test\n0x");
	err = scanf("%x", &product_id);
	if (!err) {
		printf("Please enter 4 digit hex only\n");
		goto PID_INPUT;
	}

PORT_SEL:
		printf("Which port will TX_COMPLIANCE be sent to ?\n");
		printf("Port Selected:\n");
		err = scanf("%d", &port_select);
		if (!err || port_select < 1 || port_select > 7) {
			printf("PORT %d fail\n", port_select);
			goto PORT_SEL;
		}

		session.port_num = port_select;
		session.wIndex = LINK_COMPLIANCE_MODE << 8 | port_select << 0;

		ret = libusb_init(&session.ctx);
		if (ret < 0) {
			printf("Init Error %i occourred\n", ret);
			return -EIO;
		}

		ret = libusb_get_device_list(session.ctx, &session.dev);
		if (ret < 0) {
			printf("no device found\n");
			libusb_exit(session.ctx);
			return -ENODEV;
		}

		/* open device w/ vendorID and productID */
		printf("Opening device ID %04x:%04x...", vendor_id, product_id);
		session.dev_handle = libusb_open_device_with_vid_pid(session.ctx, vendor_id, product_id);
		if (!session.dev_handle) {
			printf("failed/not in list\n");
			libusb_exit(session.ctx);
			return -ENODEV;
		}
		printf("ok\n");

		/* free the list, unref the devices in it */
		libusb_free_device_list(session.dev, 1);

		/* find out if a kernel driver is attached */
		if (libusb_kernel_driver_active(session.dev_handle, 0) == 1) {
			printf("Device has kernel driver attached\n");
			/* detach it */
			if (!libusb_detach_kernel_driver(session.dev_handle, 0))
				printf("Kernel Driver Detached\n");
		}

		/*
		 * bmRequestType: is set to 0x23
		 * 
		 * bRequest: is set to SET_FEATURE = 0x03
		 * 
		 * wValue: is the Feature Selector, which is PORT_LINK_STATE = 0x05
		 * 
		 * wIndex: selects the PORT_LINK_STATE (MSBs) which is set to
		 * Compliance Mode (0x0A) and the Port Number (LSBs) which is
		 * selected in above   user input prompt
		 * 
		 */

		/* Send Endpoint Reflector control transfer */
		ret = libusb_control_transfer(session.dev_handle,
							bmRequestType,
							bRequest,
							wValue,
							session.wIndex,
							data,
							wLength,
							timeout_set);
		if (!ret)
			printf("Port now in TX_COMPLIANCE test mode!\n");
		else
			printf("Control transfer failed. Error: %d\n", ret);

		/* close the device we opened */
		libusb_close(session.dev_handle);
		libusb_exit(session.ctx);
		return 0;
}
