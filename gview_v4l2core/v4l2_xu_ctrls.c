/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <libusb.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "gview.h"
#include "v4l2_devices.h"
#include "v4l2_xu_ctrls.h"

extern int verbosity;

/*
 * extension unit defined in Leopard Imaging USB3.0
 */
#define LENGTH_OF_XU_MAP_LEOPARD (15)
#define LI_XU_MODE_SWITCH 0x01
#define LI_XU_WINDOW_REPOSITION 0x02
#define LI_XU_LED_MODES 0x03
#define LI_XU_GAIN_CONTROL_RGB 0x04
#define LI_XU_GAIN_CONTROL_A 0x05
#define LI_XU_EXPOSURE_TIME 0x06
#define LI_XU_UUID_HWFW_REV 0x07
#define LI_XU_DEFECT_PIXEL_TABLE 0x08
#define LI_XU_SOFT_TRIGGER 0x09
#define LI_XU_TRIGGER_MODE 0x0b
#define LI_XU_TRIGGER_DELAY_TIME 0x0a
#define LI_XU_SENSOR_REGISTER_CONFIGURATION 0x0c
#define LI_XU_EXTENSION_INFO 0x0d
#define LI_XU_SENSOR_REG_RW 0x0e
#define LI_XU_GENERIC_I2C_RW 0x10

/*
 * define the value array for storing
 * the query configuration values
 */ 
unsigned char value[256+6] = {0};

/*
 * XU controls
 */

#define V4L2_CID_BASE_EXTCTR 0x0A046D01
#define V4L2_CID_BASE_LOGITECH V4L2_CID_BASE_EXTCTR
#define V4L2_CID_PANTILT_RESET_LOGITECH V4L2_CID_BASE_LOGITECH + 2

/*this should realy be replaced by V4L2_CID_FOCUS_ABSOLUTE in libwebcam*/
#define V4L2_CID_LED1_MODE_LOGITECH V4L2_CID_BASE_LOGITECH + 4
#define V4L2_CID_LED1_FREQUENCY_LOGITECH V4L2_CID_BASE_LOGITECH + 5
#define V4L2_CID_DISABLE_PROCESSING_LOGITECH V4L2_CID_BASE_LOGITECH + 0x70
#define V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH V4L2_CID_BASE_LOGITECH + 0x71
#define V4L2_CID_LAST_EXTCTR V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH

#define UVC_GUID_LOGITECH_VIDEO_PIPE                                                                   \
	{                                                                                                  \
		0x82, 0x06, 0x61, 0x63, 0x70, 0x50, 0xab, 0x49, 0xb8, 0xcc, 0xb3, 0x85, 0x5e, 0x8d, 0x22, 0x50 \
	}
#define UVC_GUID_LOGITECH_MOTOR_CONTROL                                                                \
	{                                                                                                  \
		0x82, 0x06, 0x61, 0x63, 0x70, 0x50, 0xab, 0x49, 0xb8, 0xcc, 0xb3, 0x85, 0x5e, 0x8d, 0x22, 0x56 \
	}
#define UVC_GUID_LOGITECH_USER_HW_CONTROL                                                              \
	{                                                                                                  \
		0x82, 0x06, 0x61, 0x63, 0x70, 0x50, 0xab, 0x49, 0xb8, 0xcc, 0xb3, 0x85, 0x5e, 0x8d, 0x22, 0x1f \
	}

#define XU_HW_CONTROL_LED1 1
#define XU_MOTORCONTROL_PANTILT_RELATIVE 1
#define XU_MOTORCONTROL_PANTILT_RESET 2
#define XU_MOTORCONTROL_FOCUS 3
#define XU_COLOR_PROCESSING_DISABLE 5
#define XU_RAW_DATA_BITS_PER_PIXEL 8

/* some Logitech webcams have pan/tilt/focus controls */
#define LENGTH_OF_XU_MAP (9)

static struct uvc_menu_info led_menu_entry[4] = {{0, N_("Off")},
												 {1, N_("On")},
												 {2, N_("Blinking")},
												 {3, N_("Auto")}};
/* Leopard xu query struct*/
struct uvc_xu_control_query xu_query = {
	.unit		= 3, 			//extension unit id, has to be unit 3
	.selector	= 1, 			//control selector, TD
	.query		= UVC_GET_CUR,	//request code to send to the device
	.size		= 4, 			//TD, control data size (in bytes)
	.data		= value,		//control value
};

typedef struct reg_seq{
	unsigned char regdata_width;
	unsigned short regaddr;
	unsigned short regval;
}reg_seq;

/* known xu control mappings */
static struct uvc_xu_control_mapping xu_mappings[] =
	{
		{.id = V4L2_CID_PAN_RELATIVE,
		 .name = N_("Pan (relative)"),
		 .entity = UVC_GUID_LOGITECH_MOTOR_CONTROL,
		 .selector = XU_MOTORCONTROL_PANTILT_RELATIVE,
		 .size = 16,
		 .offset = 0,
		 .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		 .data_type = UVC_CTRL_DATA_TYPE_SIGNED,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_TILT_RELATIVE,
		 .name = N_("Tilt (relative)"),
		 .entity = UVC_GUID_LOGITECH_MOTOR_CONTROL,
		 .selector = XU_MOTORCONTROL_PANTILT_RELATIVE,
		 .size = 16,
		 .offset = 16,
		 .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		 .data_type = UVC_CTRL_DATA_TYPE_SIGNED,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_PAN_RESET,
		 .name = N_("Pan Reset"),
		 .entity = UVC_GUID_LOGITECH_MOTOR_CONTROL,
		 .selector = XU_MOTORCONTROL_PANTILT_RESET,
		 .size = 1,
		 .offset = 0,
		 .v4l2_type = V4L2_CTRL_TYPE_BUTTON,
		 .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_TILT_RESET,
		 .name = N_("Tilt Reset"),
		 .entity = UVC_GUID_LOGITECH_MOTOR_CONTROL,
		 .selector = XU_MOTORCONTROL_PANTILT_RESET,
		 .size = 1,
		 .offset = 1,
		 .v4l2_type = V4L2_CTRL_TYPE_BUTTON,
		 .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_FOCUS_ABSOLUTE,
		 .name = N_("Focus"),
		 .entity = UVC_GUID_LOGITECH_MOTOR_CONTROL,
		 .selector = XU_MOTORCONTROL_FOCUS,
		 .size = 8,
		 .offset = 0,
		 .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		 .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_LED1_MODE_LOGITECH,
		 .name = N_("LED1 Mode"),
		 .entity = UVC_GUID_LOGITECH_USER_HW_CONTROL,
		 .selector = XU_HW_CONTROL_LED1,
		 .size = 8,
		 .offset = 0,
		 .v4l2_type = V4L2_CTRL_TYPE_MENU,
		 .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED,
		 .menu_info = led_menu_entry,
		 .menu_count = 4,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_LED1_FREQUENCY_LOGITECH,
		 .name = N_("LED1 Frequency"),
		 .entity = UVC_GUID_LOGITECH_USER_HW_CONTROL,
		 .selector = XU_HW_CONTROL_LED1,
		 .size = 8,
		 .offset = 16,
		 .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		 .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_DISABLE_PROCESSING_LOGITECH,
		 .name = N_("Disable video processing"),
		 .entity = UVC_GUID_LOGITECH_VIDEO_PIPE,
		 .selector = XU_COLOR_PROCESSING_DISABLE,
		 .size = 8,
		 .offset = 0,
		 .v4l2_type = V4L2_CTRL_TYPE_BOOLEAN,
		 .data_type = UVC_CTRL_DATA_TYPE_BOOLEAN,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},
		{.id = V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH,
		 .name = N_("Raw bits per pixel"),
		 .entity = UVC_GUID_LOGITECH_VIDEO_PIPE,
		 .selector = XU_RAW_DATA_BITS_PER_PIXEL,
		 .size = 8,
		 .offset = 0,
		 .v4l2_type = V4L2_CTRL_TYPE_INTEGER,
		 .data_type = UVC_CTRL_DATA_TYPE_UNSIGNED,
		 .menu_info = NULL,
		 .menu_count = 0,
		 .reserved = {0, 0, 0, 0}},

};

/*
 * get GUID unit id, if any
 * args:
 *   vd - pointer to video device data
 *   guid - 16 byte xu GUID
 *
 * asserts:
 *   vd is not null
 *   device_list->list_devices is not null
 *
 * returns: unit id for the matching GUID or 0 if none
 */
uint8_t get_guid_unit_id(v4l2_dev_t *vd, uint8_t *guid)
{
	v4l2_device_list_t *my_device_list = get_device_list();

	/*asserts*/
	assert(vd != NULL);
	assert(my_device_list->list_devices != NULL);
	if (my_device_list->list_devices[vd->this_device].vendor != 0x046D)
	{
		if (verbosity > 2)
			printf("V4L2_CORE: not a logitech device (vendor_id=0x%4x): skiping peripheral V3 unit id check\n",
				   my_device_list->list_devices[vd->this_device].vendor);
		return 0;
	}

	uint64_t busnum = my_device_list->list_devices[vd->this_device].busnum;
	uint64_t devnum = my_device_list->list_devices[vd->this_device].devnum;

	if (verbosity > 2)
		printf("V4L2_CORE: checking pan/tilt unit id for device %i (bus:%" PRId64 " dev:%" PRId64 ")\n", vd->this_device, busnum, devnum);
	/* use libusb */
	libusb_context *usb_ctx = NULL;
	libusb_device **device_list = NULL;
	libusb_device *device = NULL;
	ssize_t cnt;
	int i;

	uint8_t unit_id = 0; /*reset it*/

	if (usb_ctx == NULL)
		libusb_init(&usb_ctx);

	cnt = libusb_get_device_list(usb_ctx, &device_list);
	for (i = 0; i < cnt; i++)
	{
		uint64_t dev_busnum = libusb_get_bus_number(device_list[i]);
		uint64_t dev_devnum = libusb_get_device_address(device_list[i]);

		if (verbosity > 2)
			printf("V4L2_CORE: (libusb) checking bus(%" PRId64 ") dev(%" PRId64 ") for device\n", dev_busnum, dev_devnum);

		if (busnum == dev_busnum && devnum == dev_devnum)
		{
			device = libusb_ref_device(device_list[i]);
			break;
		}
	}

	libusb_free_device_list(device_list, 1);

	if (device)
	{
		if (verbosity > 1)
			printf("V4L2_CORE: (libusb) checking for GUID unit id\n");
		struct libusb_device_descriptor desc;

		if (libusb_get_device_descriptor(device, &desc) == 0)
		{
			for (i = 0; i < desc.bNumConfigurations; ++i)
			{
				struct libusb_config_descriptor *config = NULL;

				if (libusb_get_config_descriptor(device, i, &config) == 0)
				{
					int j = 0;
					for (j = 0; j < config->bNumInterfaces; j++)
					{
						int k = 0;
						for (k = 0; k < config->interface[j].num_altsetting; k++)
						{
							const struct libusb_interface_descriptor *interface;
							const uint8_t *ptr = NULL;

							interface = &config->interface[j].altsetting[k];
							if (interface->bInterfaceClass != LIBUSB_CLASS_VIDEO ||
								interface->bInterfaceSubClass != USB_VIDEO_CONTROL)
								continue;
							ptr = interface->extra;
							while (ptr - interface->extra +
									   sizeof(xu_descriptor) <
								   interface->extra_length)
							{
								xu_descriptor *desc = (xu_descriptor *)ptr;

								if (desc->bDescriptorType == USB_VIDEO_CONTROL_INTERFACE &&
									desc->bDescriptorSubType == USB_VIDEO_CONTROL_XU_TYPE &&
									memcmp(desc->guidExtensionCode, guid, 16) == 0)
								{
									unit_id = desc->bUnitID;

									libusb_unref_device(device);
									/*it's a match*/
									if (verbosity > 1)
										printf("V4L2_CORE: (libusb) found GUID unit id %i\n", unit_id);
									return unit_id;
								}
								ptr += desc->bLength;
							}
						}
					}
				}
				else
					fprintf(stderr, "V4L2_CORE: (libusb) couldn't get config descriptor for configuration %i\n", i);
			}
		}
		else
			fprintf(stderr, "V4L2_CORE: (libusb) couldn't get device descriptor\n");
		libusb_unref_device(device);
	}
	else
		fprintf(stderr, "V4L2_CORE: (libusb) couldn't get device\n");
	/*no match found*/
	return unit_id;
}

/*
 * tries to map available xu controls for supported devices
 * args:
 *   vd - pointer to video device data
 *
 * asserts:
 *   vd is not null
 *   vd->fd is valid ( > 0 )
 *
 * returns: 0 if enumeration succeded or errno otherwise
 */
int init_xu_ctrls(v4l2_dev_t *vd)
{
	v4l2_device_list_t *my_device_list = get_device_list();

	/*assertions*/
	assert(vd != NULL);
	assert(vd->fd > 0);
	assert(my_device_list->list_devices != NULL);

	int i = 0;
	int err = 0;

	/*if it is Logitech camera, print the error message*/
	if (my_device_list->list_devices[vd->this_device].vendor == 0x046d)
	{
		/* after adding the controls, add the mapping now */
		for (i = 0; i < LENGTH_OF_XU_MAP; i++)
		{
			if (verbosity > 0)
				printf("V4L2_CORE: mapping control for %s\n", xu_mappings[i].name);
			if ((err = xioctl(vd->fd, UVCIOC_CTRL_MAP, &xu_mappings[i])) < 0)
			{
				if ((errno != EEXIST) || (errno != EACCES))
				{
					fprintf(stderr, "V4L2_CORE: (UVCIOC_CTRL_MAP) Error: %s\n", strerror(errno));
				}
				else if (errno == EACCES)
				{
					fprintf(stderr, "V4L2_CORE: need admin previledges for adding extension controls\n");
					fprintf(stderr, "V4L2_CORE: please run 'guvcview --add_ctrls' as root (or with sudo)\n");
					return (-1);
				}
				else
					fprintf(stderr, "V4L2_CORE: Mapping exists: %s\n", strerror(errno));
			}
		}
		return err;
	}

	/*if it is Leopard camera, print the error messages accordingly*/
	else if (my_device_list->list_devices[vd->this_device].vendor == 0x2a0b)
	{
		char choice;
		char choice2;
		__u16 regAddr;
		__u16 regVal;

		//SensorRegRead(vd, 0x007e);//0x20
		//SensorRegRead(vd, 0x00b6);//0xc7
		//SensorRegRead(vd, 0x0617);//0x10

		return err;
	}

	else
	{ /* after adding the controls, add the mapping now */
		for (i = 0; i < LENGTH_OF_XU_MAP; i++)
		{
			if (verbosity > 0)
				printf("V4L2_CORE: mapping control for %s\n", xu_mappings[i].name);
			if ((err = xioctl(vd->fd, UVCIOC_CTRL_MAP, &xu_mappings[i])) < 0)
			{
				if ((errno != EEXIST) || (errno != EACCES))
				{
					fprintf(stderr, "V4L2_CORE: (UVCIOC_CTRL_MAP) Error: %s\n", strerror(errno));
				}
				else if (errno == EACCES)
				{
					fprintf(stderr, "V4L2_CORE: need admin previledges for adding extension controls\n");
					fprintf(stderr, "V4L2_CORE: please run 'guvcview --add_ctrls' as root (or with sudo)\n");
					return (-1);
				}
				else
					fprintf(stderr, "V4L2_CORE: Mapping exists: %s\n", strerror(errno));
			}
		}
		return err;
	}
}

/*
 * get lenght of xu control defined by unit id and selector
 * args:
 *   vd - pointer to video device data
 *   unit - unit id of xu control
 *   selector - selector for control
 *
 * asserts:
 *   vd is not null
 *   vd->fd is valid ( > 0 )
 *
 * returns: length of xu control
 */
uint16_t get_length_xu_control(v4l2_dev_t *vd, uint8_t unit, uint8_t selector)
{
	/*assertions*/
	assert(vd != NULL);
	assert(vd->fd > 0);

	uint16_t length = 0;

	struct uvc_xu_control_query xu_ctrl_query =
		{
			.unit = unit,
			.selector = selector,
			.query = UVC_GET_LEN,
			.size = sizeof(length),
			.data = (uint8_t *)&length};

	if (xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_ctrl_query) < 0)
	{
		fprintf(stderr, "V4L2_CORE: UVCIOC_CTRL_QUERY (GET_LEN) - Error: %s\n", strerror(errno));
		return 0;
	}

	return length;
}

/*
 * get uvc info for xu control defined by unit id and selector
 * args:
 *   vd - pointer to video device data
 *   unit - unit id of xu control
 *   selector - selector for control
 *
 * asserts:
 *   vd is not null
 *   vd->fd is valid ( > 0 )
 *
 * returns: info of xu control
 */
uint8_t get_info_xu_control(v4l2_dev_t *vd, uint8_t unit, uint8_t selector)
{
	/*assertions*/
	assert(vd != NULL);
	assert(vd->fd > 0);

	uint8_t info = 0;

	struct uvc_xu_control_query xu_ctrl_query =
		{
			.unit = unit,
			.selector = selector,
			.query = UVC_GET_INFO,
			.size = sizeof(info),
			.data = &info};

	if (xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_ctrl_query) < 0)
	{
		fprintf(stderr, "V4L2_CORE: UVCIOC_CTRL_QUERY (GET_INFO) - Error: %s\n", strerror(errno));
		return 0;
	}

	return info;
}

/*
 * runs a query on xu control defined by unit id and selector
 * args:
 *   vd - pointer to video device data
 *   unit - unit id of xu control
 *   selector - selector for control
 *   query - query type
 *   data - pointer to query data
 *
 * asserts:
 *   vd is not null
 *   vd->fd is valid ( > 0 )
 *
 * returns: 0 if query succeded or errno otherwise
 */
int query_xu_control(v4l2_dev_t *vd, uint8_t unit, uint8_t selector, uint8_t query, void *data)
{
	int err = 0;
	uint16_t len = v4l2core_get_length_xu_control(vd, unit, selector);

	struct uvc_xu_control_query xu_ctrl_query =
		{
			.unit = unit,
			.selector = selector,
			.query = query,
			.size = len,
			.data = (uint8_t *)data};

	/*get query data*/
	if ((err = xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_ctrl_query)) < 0)
	{
		fprintf(stderr, "V4L2_CORE: UVCIOC_CTRL_QUERY (%i) - Error: %s\n", query, strerror(errno));
	}

	return err;
}

/* 
`* handle the error for opening the device
 * On success 0 is returned. ON error -1 is returned and errno is set appropriately
 */
void li_error_handle()
{
	int res = errno;

	const char *err;
	switch(res)
	{
		case ENOENT:	err = "Extension unit or control not found"; break;
		case ENOBUFS:	err = "Buffer size does not match control size"; break;
		case EINVAL:	err = "Invalid request code"; break;
		case EBADRQC:	err = "Request not supported by control"; break;
		case EFAULT:	err = "The data pointer references an incessible memory area"; break;
		default:		err = strerror(res); break;
	}

	printf("failed %s. (System code: %d) \n", err, res);
}

/*
 *	Byte0: bit7 0:read;1:write. bit[6:0] regAddr width, 1:8-bit register address ; 2:16-bit register address
 *		   0x81: write, regAddr is 8-bit; 0x82: write, regAddr is 16-bit
 *		   0x01: read,  regAddr is 8-bit; 0x02: read,  regAddr is 16-bit
 *	Byte1: Length of register data,1~256
 *	Byte2: i2c salve address 8bit
 *	Byte3: register address
 *	Byte4: register address(16bit) or register data
 *
 *  Register data starts from Byte4(8bit address) or Byte5(16bit address)
 */
void li_fx3_set_i2c_cmd(v4l2_dev_t *vd, int rw_flag, int bufcnt, int slaveAddr, int regAddr,unsigned char *i2c_data)
{
	int regVal = 0;
	xu_query.selector = LI_XU_GENERIC_I2C_RW;
	xu_query.query = UVC_SET_CUR;
	xu_query.size = 256+6;

	memset (&value, 0x00, sizeof(value));
	value[0] = rw_flag;
	value[1] = bufcnt-1;
	value[2] = slaveAddr>>8;
	value[3] = slaveAddr&0xff;
	value[4] = regAddr>>8;
	value[5] = regAddr&0xff;
	if(bufcnt==1) {
		value[6] = *i2c_data;
		regVal = value[6];
	}
	else
	{
		value[6] = *(i2c_data+1);
		value[7] = *i2c_data;
		regVal = (value[6] << 8) + value[7];
	}

	if(ioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query) != 0)
		li_error_handle();
	else
		 //printf("set ioctl for fd=%d success\n", fd);
		 printf("Write REG[0x%x]: 0x%x\r\n",regAddr, regVal);

}

void li_fx3_get_i2c_cmd(v4l2_dev_t *vd, int rw_flag, int bufcnt, int slaveAddr, int regAddr,unsigned char *i2c_data)
{
	int regVal = 0;
	xu_query.selector = LI_XU_GENERIC_I2C_RW;
	xu_query.query = UVC_SET_CUR;
	xu_query.size = 256+6;

	//setting the read configuration
	memset (&value, 0x00, sizeof(value));
	value[0] = rw_flag;
	value[1] = bufcnt-1;
	value[2] = slaveAddr>>8;
	value[3] = slaveAddr&0xff;
	value[4] = regAddr>>8;
	value[5] = regAddr&0xff;
	value[6] = 0;
	value[7] = 0;

	if(ioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query) != 0)
		li_error_handle();

	//getting the value
	xu_query.query = UVC_GET_CUR;
	value[6] = 0;
	value[7] = 0;
	if(ioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query) != 0)
		li_error_handle();
	
	if(bufcnt==1) {
		regVal = value[6];
	}
	else
	{
		regVal = (value[6] << 8) + value[7];
	}
	//printf("get ioctl for fd=%d success\n", fd);
	printf("Read REG[0x%x]: 0x%x\r\n",regAddr, regVal);

}

/* inside Leopard XU firmware to get the sensor register address
 * need to apply UVC_SET_CUR before UVC_GET_CUR 
 * for read the sensor register value
 */
int li_sensor_reg_read(v4l2_dev_t *vd, int regAddr)
{
	int regVal = 0;
	int err = 0;
	xu_query.selector = LI_XU_SENSOR_REG_RW;
	xu_query.query = UVC_SET_CUR;
	xu_query.size = 5;

	// setting the read configuration
	value[0] = 0; // indicate for read
	value[1] = (regAddr >> 8) & 0xff;
	value[2] = regAddr & 0xff;
	if ((err = xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query)) < 0)
		li_error_handle();

	// getting the value
	xu_query.query = UVC_GET_CUR;
	value[0] = 0;
	value[3] = 0;//regVal MSB
	value[4] = 0;//regVal LSB
	if ((err = xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query)) < 0)
		li_error_handle();

	regVal = (value[3] << 8) + value[4];
	printf("REG[0x%x] = 0x%x\n", regAddr, regVal);

	return regVal;
}

// write the sensor register value
void li_sensor_reg_write(v4l2_dev_t *vd, int regAddr, int regVal)
{
	int err = 0;

	xu_query.selector = LI_XU_SENSOR_REG_RW;
	xu_query.query = UVC_SET_CUR;
	xu_query.size = 5;
	
	// setting the read configuration
	value[0] = 1; // indicate for write
	value[1] = (regAddr >> 8) & 0xff;
	value[2] = regAddr & 0xff;
	value[3] = (regVal >> 8) & 0xff;
	value[4] = regVal & 0xff;

	if ((err = xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query)) < 0)
		li_error_handle();
}

// get Leopard sensor RGB gain, support AR0330, M034_AP0100, MT9031, OV10640 so far
int li_sensor_set_gain_control_rgb(v4l2_dev_t *vd, int g_rGain, int g_grGain, int g_gbGain, int g_bGain)
{
	int regVal = 0;
	int err = 0;
	xu_query.selector = LI_XU_GAIN_CONTROL_RGB;
	xu_query.query = UVC_SET_CUR;
	xu_query.size = 8;

	// getting configuration
	value[0] = g_rGain & 0xff; 	//rGain LSB
	value[1] = g_rGain >> 8; 	//rGain MSB
	value[2] = g_grGain & 0xff;	//grGain LSB
	value[3] = g_grGain >> 8;	//grGain MSB
	value[4] = g_gbGain & 0xff;	//gbGain LSB
	value[5] = g_gbGain >> 8;   //gbGain MSB
	value[6] = g_bGain & 0xff;  //bGain LSB
	value[7] = g_bGain >> 8;	//bGain MSB
	if ((err = xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query)) < 0)
		li_error_handle();

	printf("set RGB gain succesfully\r\n");

	return err;
}

// write the sensor register value
void li_sensor_get_gain_control_rgb(v4l2_dev_t *vd)
{
	int err = 0;
	int i = 0;
	xu_query.selector = LI_XU_GAIN_CONTROL_RGB;
	xu_query.query = UVC_GET_CUR;
	xu_query.size = 8;

	for (i = 0; i < 8; i++) {
		value[i] = 0;
	}
	if ((err = xioctl(vd->fd, UVCIOC_CTRL_QUERY, &xu_query)) < 0)
		li_error_handle();
	uint16_t g_rGain = 0, g_grGain = 0, g_gbGain = 0, g_bgGain = 0;
	g_rGain = (value[1] << 8) + value[0]; 	//rGain
	g_grGain = (value[3] << 8) + value[2]; 	//grGain
	g_gbGain = (value[5] << 8) + value[4]; 	//gbGain
	g_bgGain = (value[7] << 8) + value[6]; 	//bGain
	printf("get RGB gain:\r\nrGain = 0x%x,\r\n grGain = 0x%x,\r\n gbGain = 0x%x,\r\n grGain = 0x%x\r\n ", g_rGain, g_grGain, g_gbGain, g_bgGain);
	
}