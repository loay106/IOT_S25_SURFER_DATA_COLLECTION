import os
import socket
import time
import hashlib

import requests
from zeroconf import Zeroconf, ServiceBrowser

PORT = 80
TEST_ENDPOINT = '/status'
LIST_SAMPLINGS_ENDPOINT = '/list/samplings'
DOWNLOAD_ENDPOINT = '/download?file='
VALIDATE_ENDPOINT = "/validate"

found_devices = {}
samplings_download_dir = './tmp/samplings'


class ESP32Listener:
    def remove_service(self, zeroconf, service_type, name):
        pass

    def update_service(self, zeroconf, service_type, name):
        pass

    def add_service(self, zeroconf, service_type, name):
        info = zeroconf.get_service_info(service_type, name)
        if info:
            ip_address = socket.inet_ntoa(info.addresses[0])
            hostname = name.split('.')[0]
            print(f"[+] Found ESP32 unit: {hostname} at {ip_address}")
            found_devices[hostname] = ip_address


def discover_mdns_services(service_type="_http._tcp.local.", wait_time=3):
    print("[*] Discovering devices via mDNS...")
    zeroconf = Zeroconf()
    listener = ESP32Listener()
    browser = ServiceBrowser(zeroconf, service_type, listener)
    time.sleep(wait_time)
    zeroconf.close()


def list_files(ip):
    try:
        url = f"http://{ip}:{PORT}{LIST_SAMPLINGS_ENDPOINT}"
        response = requests.get(url, timeout=3)
        print(f"[DEBUG] Raw response from {ip}:", response.text)
        return response.json().get("files")
    except Exception as e:
        print(f"[!] Failed to list files from {ip}: {e}")
        return []


def download_file(ip, device_id, filename):
    try:
        url = f"http://{ip}:{PORT}{DOWNLOAD_ENDPOINT}{filename}"
        response = requests.get(url, timeout=5)
        if response.ok:
            filename_only = os.path.basename(filename)
            device_dir = os.path.join(samplings_download_dir, device_id)
            os.makedirs(device_dir, exist_ok=True)
            path = os.path.join(device_dir, filename_only)

            with open(path, 'wb') as f:
                f.write(response.content)

            print(f"[+] Downloaded {filename} from {ip} → saved to {path}")

            # Calculate MD5 locally
            local_md5 = calculate_md5(path)
            if local_md5:
                # Send it to device for comparison
                validate_file_on_device(ip, filename, local_md5)
            else:
                print(f"[!] Failed to calculate MD5 for {filename_only}")
        else:
            print(f"[!] Failed to download {filename} from {ip}")
    except Exception as e:
        print(f"[!] Error downloading {filename} from {ip}: {e}")


def calculate_md5(file_path):
    """
    Calculates the MD5 hash of a file's contents.
    """
    hash_md5 = hashlib.md5()
    try:
        with open(file_path, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
        return hash_md5.hexdigest()
    except FileNotFoundError:
        return None


def get_remote_md5(ip, filename):
    try:
        url = f"http://{ip}:{PORT}{VALIDATE_ENDPOINT}{filename}"
        response = requests.get(url, timeout=3)
        if response.ok:
            return response.text.strip()
        else:
            print(f"[!] Failed to validate {filename} from {ip} - status code {response.status_code}")
    except Exception as e:
        print(f"[!] Error calling validate for {filename} on {ip}: {e}")
    return None


def validate_file_on_device(ip, filename, expected_md5):
    print("FILE PATH TO VALIDATE:")
    print(filename)
    try:
        url = f"http://{ip}:{PORT}{VALIDATE_ENDPOINT}"
        params = {
            "file": filename,
            "md5": expected_md5
        }
        response = requests.get(url, params=params, timeout=50)
        if response.status_code == 200:
            print(f"[✔] MD5 validated on device for {filename}")
            return True
        elif response.status_code == 409:
            print(f"[❌] MD5 mismatch reported by device for {filename}")
        else:
            print(f"[!] Device returned {response.status_code} for {filename}: {response.text}")
    except Exception as e:
        print(f"[!] Error validating {filename} on {ip}: {e}")
    return False


def extract_timestamp(filename):
    # Expects filename format: /samplings/1481765933_0_Mock-HX711
    try:
        base = os.path.basename(filename)
        parts = base.split("_")
        return parts[0] if len(parts) >= 3 else None
    except Exception:
        return None


def main():
    discover_mdns_services()
    all_files_by_timestamp = {}

    for hostname, ip in found_devices.items():
        files = list_files(ip)
        print(f"[=] Files on {hostname} ({ip}): {files}")
        for file in files:
            timestamp = extract_timestamp(file)
            print(f"[DEBUG] Extracted timestamp from {file}: {timestamp}")
            if timestamp:
                all_files_by_timestamp.setdefault(timestamp, []).append((ip, file))
                unit_mac = hostname.split("-")[1]
                download_file(ip, unit_mac, file)
            else:
                print(f"[!] Skipped malformed filename: {file}")


if __name__ == "__main__":
    main()
