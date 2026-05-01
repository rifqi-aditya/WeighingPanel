#ifndef USB_HOST_PRINTER_H
#define USB_HOST_PRINTER_H

#include <Arduino.h>
#include "usb/usb_host.h"

class USBHostPrinter {
public:
    USBHostPrinter();
    
    // Inisialisasi stack USB Host
    bool begin();
    
    // Kirim data mentah (ESC/POS) ke printer
    bool write(const uint8_t* data, size_t len);
    bool write(const char* str);
    
    // Status koneksi
    bool isConnected() const { return _device_connected && _interface_claimed; }
    
    // Loop internal (harus dipanggil jika tidak menggunakan task terpisah)
    void handleEvents();

private:
    // USB Host State
    bool _device_connected = false;
    bool _interface_claimed = false;
    uint8_t _dev_addr = 0;
    usb_host_client_handle_t _client_hdl = NULL;
    usb_device_handle_t _dev_hdl = NULL;
    uint8_t _bulk_out_ep = 0;
    
    // Fungsi internal untuk deteksi printer
    void _onDeviceConnected(usb_host_client_event_msg_t *event_msg);
    void _onDeviceDisconnected(usb_host_client_event_msg_t *event_msg);
    bool _claimPrinterInterface();
    
    // Callback statis untuk library USB Host
    static void _clientEventCallback(const usb_host_client_event_msg_t *event_msg, void *arg);
};

#endif
