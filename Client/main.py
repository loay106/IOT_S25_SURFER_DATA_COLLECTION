import os
import time

import questionary

from device_scanner import *
from files_downloader import *
from files_merger import *


class ESP32Device:
    def __init__(self, hostname, ip):
        self.hostname = hostname
        self.ip = ip
        self.available_samplings: dict[str, set[str]] = {}


FOUND_DEVICES: dict[str, ESP32Device] = {}
AVAILABLE_SAMPLINGS: set[str] = set()


def extract_timestamp(filename):
    # Expects filename format: /samplings/1481765933_0_Mock-HX711
    try:
        base = os.path.basename(filename)
        parts = base.split("_")
        return parts[0] if len(parts) >= 3 else None
    except Exception:
        return None


def scan_devices_flow():
    found = scan_for_mdns_services()
    for hostname, ip in found.items():
        if hostname not in FOUND_DEVICES:
            FOUND_DEVICES[hostname] = ESP32Device(hostname, ip)
        else:
            FOUND_DEVICES[hostname].ip = ip

        if len(FOUND_DEVICES[hostname].available_samplings) == 0:
            files = list_available_sampling_files(ip)
            for file in files:
                timestamp = extract_timestamp(file)
                AVAILABLE_SAMPLINGS.add(timestamp)
                FOUND_DEVICES[hostname].available_samplings.setdefault(timestamp, set()).add(file)

    print(f"[✓] Discovered {len(FOUND_DEVICES)} device(s).")


def download_by_timestamp_flow():
    selected = questionary.select(
        "Select a timestamp to download and merge files for:",
        choices=list(AVAILABLE_SAMPLINGS)
    ).ask()

    if not selected:
        print("[!] No timestamp selected.")
        return

    for hostname, device in FOUND_DEVICES.items():
        files = device.available_samplings.get(selected, set())
        for file in files:
            unit_mac = device.hostname.split("-")[1]
            download_file(device.ip, unit_mac, file)

    merge_files(selected)


def main():
    while True:
        choice = questionary.select(
            "Choose an operation:",
            choices=[
                "Scan for devices",
                "List samplings and download",
                "Exit"
            ]
        ).ask()

        if choice == "Scan for devices":
            try:
                scan_devices_flow()
            except Exception as e:
                print(e)
                while True:
                    time.sleep(500)

        elif choice == "List samplings and download":
            download_by_timestamp_flow()

        elif choice == "Exit":
            print("Goodbye!")
            break

        print("\n" + "-" * 40 + "\n")


if __name__ == "__main__":
    main()
