import os

from device_scanner import *
from files_downloader import *
from files_merger import *


def extract_timestamp(filename):
    # Expects filename format: /samplings/1481765933_0_Mock-HX711
    try:
        base = os.path.basename(filename)
        parts = base.split("_")
        return parts[0] if len(parts) >= 3 else None
    except Exception:
        return None


def main():
    scan_for_mdns_services()
    all_files_by_timestamp = {}

    for hostname, ip in FOUND_DEVICES.items():
        files = list_available_sampling_files(ip)
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

    merge_files('1481765933')


if __name__ == "__main__":
    main()
