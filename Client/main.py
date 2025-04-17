import os
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
import traceback

import questionary
from questionary import Choice

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
    try:
        found = scan_for_mdns_services()
        for hostname, ip in found.items():
            device = ESP32Device(hostname, ip)
            while True:
                try:
                    files = list_available_sampling_files(hostname)
                    for file in files:
                        timestamp = extract_timestamp(file)
                        AVAILABLE_SAMPLINGS.add(timestamp)
                        device.available_samplings[timestamp] = set(files)
                        FOUND_DEVICES[hostname] = device
                    break
                except Exception as e:
                    print(f"Failed to get sampling list from device {hostname}. Retrying...")
                    print(e)
    except Exception as e:
        print("An error occurred! Try again...")
        print(e)


def download_by_timestamp(timestamp, validate_download):
    def download_for_device(dev):
        files = dev.available_samplings.get(timestamp, set())
        for file in files:
            download_file(dev.hostname, file, validate_download)

    tasks = []

    with ThreadPoolExecutor(max_workers=len(FOUND_DEVICES)) as executor:
        for hostname, device in FOUND_DEVICES.items():
            future = executor.submit(download_for_device, device)
            tasks.append(future)

        for future in as_completed(tasks):
            try:
                future.result()
            except Exception as e:
                print(f"[!] Device download task failed: {e}")
                print(traceback.format_exc())


    merge_files(timestamp)


def download_all_samplings_flow(validate_download):
    for timestamp in AVAILABLE_SAMPLINGS:
        download_by_timestamp(timestamp, validate_download)


def delete_samplings_flow():
    try:
        for _, device in FOUND_DEVICES.items():
            while True:
                is_deleted = remove_all_samplings(device.hostname)
                if is_deleted:
                    device.available_samplings.clear()
                    break
                else:
                    time.sleep(3)
            AVAILABLE_SAMPLINGS.clear()
    except Exception as e:
        print("An error occurred! Try again...")
        print(e)


def list_by_timestamp_flow():
    if len(AVAILABLE_SAMPLINGS) == 0:
        print("No samplings found, try using the device first on sampling mode")
        return
    try:
        choices = [
            Choice(title="All samplings", value="all")
        ]
        for ts in reversed(sorted(AVAILABLE_SAMPLINGS)):
            try:
                dt = datetime.fromtimestamp(int(ts))
                label = dt.strftime("%Y-%m-%d %H:%M:%S") + f" ({ts})"
                choices.append(Choice(title=label, value=str(ts)))
            except Exception as e:
                print(f"[!] Skipped invalid timestamp {ts}: {e}")

        selected = questionary.select(
            "Select a sampling to download:",
            choices=choices
        ).ask()

        if not selected:
            print("[!] No sampling selected.")
            return

        if selected == "all":
            download_all_samplings_flow(True)
        else:
            download_by_timestamp(selected, True)

    except Exception as e:
        print("An error occurred! Try again...")
        print(e)


def main():
    print("Scanning for available devices, this can take a few minutes...")
    scan_devices_flow()
    if len(FOUND_DEVICES) == 0:
        print("No devices were discovered")
        print("Make sure the devices are running on the same network (Wifi) "
              f"and/or the client has access to the local networks then run the script again")
        print("Exiting...")
        time.sleep(10)
        return

    print(f"Discovered {len(FOUND_DEVICES)} device(s).")
    while True:
        choice = questionary.select(
            "Choose an operation:",
            choices=[
                "List available samplings and download",
                "Remove samplings from ESP32 devices",
                "Exit"
            ]
        ).ask()

        if choice == "List available samplings and download":
            list_by_timestamp_flow()
        elif choice == "Remove samplings from ESP32 devices":
            delete_samplings_flow()
        elif choice == "Exit":
            break

        print("\n" + "-" * 40 + "\n")


if __name__ == "__main__":
    main()
