#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>
#include <windows.h>

static int count = 0;
static boolean plugged = FALSE;
static int MY_PRODUCT_ID = -1;
static int MY_VENDOR_ID = -1;

inline void lockscreen() {
	LockWorkStation();
}

int chooseDevices() {
	printf("Choose a USB device to monitor\n");
	libusb_device **list;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	int *pids = malloc(sizeof(int) * cnt);
	int *vids = malloc(sizeof(int) * cnt);
	boolean found = FALSE;

	for (int i = 0; i < cnt; ++i) {
		libusb_device *device = list[i];
		libusb_device_handle *handle = NULL;
		struct libusb_device_descriptor desc = { 0 };
		int r = libusb_get_device_descriptor(device, &desc);
		if (r < 0) {
			printf("failed to get device descriptor\n");
			return -1;
		}
		pids[i] = desc.idProduct;
		vids[i] = desc.idVendor;
		unsigned char product[200] = { 0 };
		unsigned char manufacturer[200] = { 0 };

		if (libusb_open(device, &handle) == 0) {
			libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, manufacturer, 200);
			libusb_get_string_descriptor_ascii(handle, desc.iProduct, product, 200);
			libusb_close(handle);
		}
		
		printf("%d. %d:%d %s %s %s\n", i, desc.idProduct, desc.idVendor, manufacturer, product, serial_number);
	}

	printf("Please select from 0-%lld: ", cnt - 1);
	char c = getchar();
	unsigned int index = c - '0';
	if (index > cnt) {
		return -1;
	}

	MY_PRODUCT_ID = pids[index];
	MY_VENDOR_ID = vids[index];

	free(pids);
	free(vids);
	libusb_free_device_list(list, 1);
	return 0;
}

int poll() {
	libusb_device **list;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	boolean found = FALSE;
	for (int i = 0; i < cnt; ++i) {
		libusb_device *device = list[i];
		struct libusb_device_descriptor desc = { 0 };
		int r = libusb_get_device_descriptor(device, &desc);
		if (r < 0) {
			printf("failed to get device descriptor\n");
			return -1;
		}

		if (desc.idProduct == MY_PRODUCT_ID &&
			desc.idVendor == MY_VENDOR_ID) {
			found = TRUE;
			break;
		}
	}

	if (plugged && !found) {
		lockscreen();
	}

	plugged = found;
	libusb_free_device_list(list, 1);
	return 0;
}

int main(void) {
	libusb_init(NULL);
	if (chooseDevices() < 0) {
		libusb_exit(NULL);
		return 0;
	}

	printf("Listening on your device...");
	while (poll() == 0) {
		Sleep(500);
	}
	libusb_exit(NULL);
	return 0;
}