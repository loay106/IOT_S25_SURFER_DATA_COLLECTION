from zeroconf import Zeroconf, ServiceBrowser
import socket
import time

SERVICE_TYPE = "_http._tcp.local."

__all__ = [
    'scan_for_mdns_services',
]


class ESP32Listener:
    def __init__(self):
        self.discovered_devices = {}

    def remove_service(self, zf, service_type, name):
        pass

    def update_service(self, zf, service_type, name):
        pass

    def add_service(self, zf, service_type, name):
        info = zf.get_service_info(service_type, name)
        if info:
            ip_address = socket.inet_ntoa(info.addresses[0])
            hostname = name.split('.')[0]
            print(f"[+] Found ESP32 unit: {hostname} at {ip_address}")
            self.discovered_devices[hostname] = ip_address


def scan_for_mdns_services() -> dict:
    # hostname to ip mapping
    zeroconf = Zeroconf()
    listener = ESP32Listener()
    print("[*] Scanning for devices via mDNS...")
    browser = ServiceBrowser(zeroconf, SERVICE_TYPE, listener)
    time.sleep(5)
    zeroconf.close()
    discovered_devices = listener.discovered_devices.copy()
    return discovered_devices
