import os
from datetime import datetime, timezone
from concurrent.futures import ThreadPoolExecutor, as_completed

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
    except Exception as e:
        print("An error occurred! Try again...")
        print(e)


def download_by_timestamp(timestamp, validate_download):
    def download_for_device(dev):
        files = dev.available_samplings.get(timestamp, set())
        unit_mac = dev.hostname.split("-")[1]

        for file in files:
            download_file(dev.ip, unit_mac, file, validate_download)

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

    merge_files(timestamp)


def download_all_samplings_flow(validate_download):
    for timestamp in AVAILABLE_SAMPLINGS:
        download_by_timestamp(timestamp, validate_download)


def list_by_timestamp_flow():
    try:
        choices = [
            Choice(title="All samplings", value="all")
        ]

        for ts in sorted(AVAILABLE_SAMPLINGS):
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

        selected_validate = questionary.select(
            "Validate files after download?",
            choices=["Yes (Slower, recommended)", "No (Faster)"]
        ).ask()
        validate_download = True if selected_validate == "Yes (Slower, recommended)" else False

        if selected == "all":
            download_all_samplings_flow(validate_download)
        else:
            download_by_timestamp(selected, validate_download)

    except Exception as e:
        print("An error occurred! Try again...")
        print(e)


def main():
    while True:
        print("Scanning for available devices, this can take a few minutes...")
        scan_devices_flow()
        choice = questionary.select(
            f"Discovered {len(FOUND_DEVICES)} device(s)."
            f" Does this match the number of the sampling units in your system?",
            choices=[
                "Yes",
                "No (Rescan)"
            ]
        ).ask()
        if choice == "Yes":
            break

    while True:
        choice = questionary.select(
            "Choose an operation:",
            choices=[
                "List available samplings and download",
                "Exit"
            ]
        ).ask()

        if choice == "List available samplings and download":
            list_by_timestamp_flow()
        elif choice == "Exit":
            break

        print("\n" + "-" * 40 + "\n")


if __name__ == "__main__":
    main()
