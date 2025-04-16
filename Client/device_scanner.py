from zeroconf import Zeroconf, ServiceBrowser
import socket
import time

FOUND_DEVICES = {}

SERVICE_TYPE = "_http._tcp.local."

__all__ = [
    'FOUND_DEVICES',
    'scan_for_mdns_services',
    'close_scanner'
]


class ESP32Listener:
    def remove_service(self, zf, service_type, name):
        pass

    def update_service(self, zf, service_type, name):
        pass

    def add_service(self, zf, service_type, name):
        global FOUND_DEVICES
        info = zf.get_service_info(service_type, name)
        if info:
            ip_address = socket.inet_ntoa(info.addresses[0])
            hostname = name.split('.')[0]
            print(f"[+] Found ESP32 unit: {hostname} at {ip_address}")
            FOUND_DEVICES[hostname] = ip_address


zeroconf = Zeroconf()
listener = ESP32Listener()


def scan_for_mdns_services():
    global zeroconf, listener
    print("[*] Scanning for devices via mDNS...")
    browser = ServiceBrowser(zeroconf, SERVICE_TYPE, listener)
    time.sleep(10)


def close_scanner():
    zeroconf.close()
