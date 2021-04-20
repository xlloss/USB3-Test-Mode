/*
©  [2018] Microchip Technology Inc. and its subsidiaries.

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


int main(int argc, char **argv)
{
	int r,i,z;
	char yesno[] = "";
	int timeout_time;
	char port_select[] = "";
	char test_select[] = "";
	ssize_t cnt;
	int port;
	int err = 0;
	int VENDOR_ID = 0;
	int PRODUCT_ID = 0;
	char PRODUCT_ID1[4];
	char VENDOR_ID1[4];

INPUT1:
	printf("Please enter the vendor ID of the USB3.1 hub under test\n0x");
	err = scanf("%s", VENDOR_ID1);
	if(!err)
	{
		printf("Please enter 4 digit hex only (len=%d, %s)\n", err, VENDOR_ID1);
		goto INPUT1;
	}

	int count1;
	char hexset1[] = "0123456789ABCDEFabcdef";
	count1 = strspn(VENDOR_ID1, hexset1);
	if (count1 != 4)
	{
		printf("Please enter 4 digit hex only (count1=%d)\n", count1);
		goto INPUT1;
	}
	VENDOR_ID = (int)strtol(VENDOR_ID1, NULL, 16);

INPUT2:
	printf("Please enter the product ID of the USB3.1 hub under test\n0x");
	err = scanf("%s", PRODUCT_ID1);
	if(!err)
	{
		printf("Please enter 4 digit hex only\n");
		goto INPUT2;
	}

	int count2;
	char hexset2[] = "0123456789ABCDEFabcdef";
	count2 = strspn(PRODUCT_ID1, hexset2);
	if (count2 != 4)
	{
		printf("Please enter 4 digit hex only\n");
		goto INPUT2;
	}
	PRODUCT_ID = (int)strtol(PRODUCT_ID1, NULL, 16);

	printf("This demo will iniate USB3.1 TX_COMPLIANCE test mode on a port.\n");

TEST_SEL:
		printf("Press '1' for TX_COMPLIANCE\n");
		printf("Press 'q' or 'Q' or CTL+C to quit\n");
		err = scanf("%s", test_select);
		if (!err) {
			printf("Enter to for TX_COMPLIANCE\n");
			goto PORT_SEL1;
		}

		if (strcmp(test_select, "1")==0)
		{
PORT_SEL1:
			printf("Which port will TX_COMPLIANCE be sent to?\n");
			err = scanf("%s", port_select);
			if (!err)
				goto PORT1;

			if (strcmp(port_select, "1") == 0)
			{
PORT1:
				printf("Port 1 selected\n");
				session.port_num = 1;
				session.wIndex = 0x0A01;
			}
			else if(strcmp(port_select, "2") == 0)
			{
				printf("Port 2 selected\n");
				session.port_num = 2;
				session.wIndex = 0x0A02;
			}
			else if(strcmp(port_select, "3") == 0)
			{
				printf("Port 3 selected\n");
				session.port_num = 3;
				session.wIndex = 0x0A03;
			}
			else if(strcmp(port_select, "4") == 0)
			{
				printf("Port 4 selected\n");
				session.port_num = 4;
				session.wIndex = 0x0A04;
			}
			else if(strcmp(port_select, "5") == 0)
			{
				printf("Port 5 selected\n");
				session.port_num = 5;
				session.wIndex = 0x0A05;
			}
			else if(strcmp(port_select, "6") == 0)
			{
				printf("Port 6 selected\n");
				session.port_num = 6;
				session.wIndex = 0x0A06;
			}
			else if(strcmp(port_select, "7") == 0)
			{
				printf("Port 7 selected\n");
				session.port_num = 7;
				session.wIndex = 0x0A07;
			}
			else
			{
				printf("Invalid Port Selected. Please select a valid port # or Ctrl+C to quit\n");
				goto PORT_SEL1;
			}
		}
		else if (strcmp(test_select, "q")==0 || strcmp(test_select, "Q")==0)
		{
			return 0;
		}
		else
		{
			printf("Invalid Test Selected or invalid character.\n");
			goto TEST_SEL;
		}


		r = libusb_init(&session.ctx);
		if (r < 0) {
			printf("Init Error %i occourred.\n", r);
			return -EIO;
		}


		cnt = libusb_get_device_list(session.ctx, &session.dev);
		if (cnt < 0) {
			printf("no device found\n");
			libusb_exit(session.ctx);
			return -ENODEV;
		}

		/* open device w/ vendorID and productID */
		printf("Opening device ID %04x:%04x...", VENDOR_ID, PRODUCT_ID);
		session.dev_handle = libusb_open_device_with_vid_pid(session.ctx, VENDOR_ID, PRODUCT_ID);
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
			printf("Device has kernel driver attached.\n");
			/* detach it */
			if (!libusb_detach_kernel_driver(session.dev_handle, 0))
				printf("Kernel Driver Detached!\n");
		}

		int len;
		int transferred;
		uint8_t bmRequestType = 0x23;
		uint8_t bRequest = 0x03;
		uint16_t wValue = 0x0005;
		//uint16_t wIndex = 0x0000;
		unsigned char *data = 0;
		uint16_t wLength = 0x0000;
		unsigned int timeout_ = 50000000;

		/* Send Endpoint Reflector control transfer */
		r = libusb_control_transfer(session.dev_handle,
							bmRequestType,				/* bmRequestType is set to 0x23  */
							bRequest,					/* bRequest is set to SET_FEATURE = 0x03 */
							wValue,						/* wValue is the Feature Selector , which is PORT_LINK_STATE = 0x05*/
							session.wIndex, 			/* wIndex selects the PORT_LINK_STATE (MSBs) which is set to Compliance Mode (0x0A) and the Port Number (LSBs) which is selected in above user input prompt*/
							data,
							wLength,
							timeout_);
		if (!r){
			printf("Port now in TX_COMPLIANCE test mode!\n");
		}
		else{
			printf("Control transfer failed. Error: %d\n", r);
		}

		/* close the device we opened */
		libusb_close(session.dev_handle);
		libusb_exit(session.ctx);
		return 0;

}
