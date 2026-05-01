#include "USBHostPrinter.h"
#include "esp_log.h"

static const char *TAG = "USBH_PRINTER";

// Konfigurasi internal
#define CLIENT_EVENT_QUEUE_SIZE 10
#define TRANSFER_BUFFER_SIZE 2048

USBHostPrinter::USBHostPrinter() {}

void USBHostPrinter::_clientEventCallback(const usb_host_client_event_msg_t *event_msg, void *arg) {
    USBHostPrinter *obj = (USBHostPrinter *)arg;
    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        obj->_onDeviceConnected((usb_host_client_event_msg_t *)event_msg);
    } else if (event_msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        obj->_onDeviceDisconnected((usb_host_client_event_msg_t *)event_msg);
    }
}

bool USBHostPrinter::begin() {
    // 1. Install USB Host Library
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err_t err = usb_host_install(&host_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) { 
        Serial.println("USBH_PRINTER: Failed to install USB Host");
        return false;
    }
    delay(100); // Berikan waktu PHY untuk stabil

    // 2. Register Client
    const usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = CLIENT_EVENT_QUEUE_SIZE,
        .async = {
            .client_event_callback = _clientEventCallback,
            .callback_arg = this,
        }
    };
    err = usb_host_client_register(&client_config, &_client_hdl);
    if (err != ESP_OK) {
        Serial.println("USBH_PRINTER: Failed to register USB client");
        return false;
    }

    Serial.println("USBH_PRINTER: Driver Started");
    return true;
}

void USBHostPrinter::handleEvents() {
    if (_client_hdl) {
        // Berikan waktu lebih lama (5ms) agar stack USB lebih stabil
        usb_host_client_handle_events(_client_hdl, pdMS_TO_TICKS(5));
    }
    // Handle background library events
    uint32_t event_flags;
    usb_host_lib_handle_events(pdMS_TO_TICKS(5), &event_flags);
}

void USBHostPrinter::_onDeviceConnected(usb_host_client_event_msg_t *event_msg) {
    if (_device_connected) return;

    _dev_addr = event_msg->new_dev.address;
    if (usb_host_device_open(_client_hdl, _dev_addr, &_dev_hdl) == ESP_OK) {
        Serial.printf("USBH_PRINTER: Device opened, address: %d\n", _dev_addr);
        if (_claimPrinterInterface()) {
            _device_connected = true;
            Serial.println("USBH_PRINTER: Printer detected and claimed!");
        } else {
            Serial.println("USBH_PRINTER: Device is NOT a printer or failed to claim.");
            usb_host_device_close(_client_hdl, _dev_hdl);
            _dev_hdl = NULL;
        }
    }
}

void USBHostPrinter::_onDeviceDisconnected(usb_host_client_event_msg_t *event_msg) {
    Serial.println("USBH_PRINTER: Device disconnected");
    _device_connected = false;
    _interface_claimed = false;
    if (_dev_hdl) {
        usb_host_device_close(_client_hdl, _dev_hdl);
        _dev_hdl = NULL;
    }
}

bool USBHostPrinter::_claimPrinterInterface() {
    const usb_device_desc_t *dev_desc;
    usb_host_get_device_descriptor(_dev_hdl, &dev_desc);

    const usb_config_desc_t *config_desc;
    usb_host_get_active_config_descriptor(_dev_hdl, &config_desc);

    int offset = 0;
    const usb_standard_desc_t *next_desc = (const usb_standard_desc_t *)config_desc;
    uint8_t current_intf = 0xFF;

    while (next_desc != NULL) {
        if (next_desc->bDescriptorType == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
            const usb_intf_desc_t *intf = (const usb_intf_desc_t *)next_desc;
            if (intf->bInterfaceClass == 0x07) { // Printer Class
                current_intf = intf->bInterfaceNumber;
                Serial.printf("USBH_PRINTER: Found Printer Interface: %d\n", current_intf);
            } else {
                current_intf = 0xFF;
            }
        } 
        else if (next_desc->bDescriptorType == USB_B_DESCRIPTOR_TYPE_ENDPOINT && current_intf != 0xFF) {
            const usb_ep_desc_t *ep = (const usb_ep_desc_t *)next_desc;
            if ((ep->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) == USB_BM_ATTRIBUTES_XFER_BULK) {
                if ((ep->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) == 0) { // OUT
                    _bulk_out_ep = ep->bEndpointAddress;
                    Serial.printf("USBH_PRINTER: Found Bulk Out Endpoint: 0x%02X\n", _bulk_out_ep);
                    
                    if (usb_host_interface_claim(_client_hdl, _dev_hdl, current_intf, 0) == ESP_OK) {
                        _interface_claimed = true;
                        return true;
                    }
                }
            }
        }
        next_desc = usb_parse_next_descriptor(next_desc, config_desc->wTotalLength, &offset);
    }
    return false;
}

bool USBHostPrinter::write(const uint8_t* data, size_t len) {
    if (!isConnected()) return false;

    // Alokasikan transfer
    usb_transfer_t *transfer;
    usb_host_transfer_alloc(TRANSFER_BUFFER_SIZE, 0, &transfer);
    
    if (transfer) {
        size_t to_send = (len > TRANSFER_BUFFER_SIZE) ? TRANSFER_BUFFER_SIZE : len;
        memcpy(transfer->data_buffer, data, to_send);
        transfer->num_bytes = to_send;
        transfer->device_handle = _dev_hdl;
        transfer->bEndpointAddress = _bulk_out_ep;
        transfer->callback = [](usb_transfer_t *t) {
            usb_host_transfer_free(t); // Bebaskan setelah selesai
        };
        transfer->context = this;

        esp_err_t err = usb_host_transfer_submit(transfer);
        return (err == ESP_OK);
    }
    return false;
}

bool USBHostPrinter::write(const char* str) {
    return write((const uint8_t*)str, strlen(str));
}
