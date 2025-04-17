import os
import re

__all__ = ['extract_role_and_mac', 'extract_timestamp']


def extract_role_and_mac(hostname: str):
    pattern = r"^esp32-data-collector-([a-zA-Z0-9]+)-([a-zA-Z0-9_]+)$"
    match = re.match(pattern, hostname)
    if match:
        role = match.group(1)
        mac = match.group(2)
        is_main = role == "main"
        return is_main, mac
    else:
        return None, None


def extract_timestamp(filename):
    # Expects filename format: /samplings/1481765933_0_Mock-HX711
    try:
        base = os.path.basename(filename)
        parts = base.split("_")
        return parts[0] if len(parts) >= 3 else None
    except Exception:
        return None
