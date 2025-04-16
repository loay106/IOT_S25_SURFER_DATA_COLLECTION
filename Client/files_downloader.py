import os
import time
import hashlib
import requests


__all__ = [
    'SAMPLINGS_DOWNLOAD_DIRECTORY',
    'list_available_sampling_files',
    'download_file'
]

PORT = 80
LIST_SAMPLINGS_ENDPOINT = '/list/samplings'
DOWNLOAD_ENDPOINT = '/download?file='
VALIDATE_ENDPOINT = "/validate"

SAMPLINGS_DOWNLOAD_DIRECTORY = './tmp/samplings'


def list_available_sampling_files(ip) -> list:
    try:
        url = f"http://{ip}:{PORT}{LIST_SAMPLINGS_ENDPOINT}"
        response = requests.get(url)
        print(f"[DEBUG] Raw response from {ip}:", response.text)
        return response.json().get("files")
    except Exception as e:
        print(f"[!] Failed to list files from {ip}: {e}")
        return []


def download_file(ip, device_id, filename):
    filename_only = os.path.basename(filename)
    device_dir = os.path.join(SAMPLINGS_DOWNLOAD_DIRECTORY, device_id)
    os.makedirs(device_dir, exist_ok=True)
    path = os.path.join(device_dir, filename_only)

    while True:
        try:
            url = f"http://{ip}:{PORT}{DOWNLOAD_ENDPOINT}{filename}"
            response = requests.get(url)

            if response.ok:
                with open(path, 'wb') as f:
                    f.write(response.content)
                print(f"[+] Downloaded {filename} from {ip} → saved to {path}")

                # Calculate MD5 locally
                local_md5 = _calculate_md5(path)
                if local_md5:
                    _validate_file_on_device(ip, filename, local_md5)
                else:
                    print(f"[!] Failed to calculate MD5 for {filename_only}")
                break  # Exit loop on success

            else:
                print(f"[!] Download failed with status {response.status_code}. Retrying...")

        except Exception as e:
            print(f"[!] Error downloading {filename} from {ip}: {e}. Retrying...")

        time.sleep(2)  # Avoid spamming the device/network


def _calculate_md5(file_path):
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


def _validate_file_on_device(ip, filename, expected_md5):
    url = f"http://{ip}:{PORT}{VALIDATE_ENDPOINT}"
    params = {
        "file": filename,
        "md5": expected_md5
    }

    while True:
        try:
            response = requests.get(url, params=params, timeout=5)

            if response.status_code == 200:
                print(f"[✔] MD5 validated on device for {filename}")
                return True

            elif response.status_code == 409:
                print(f"[❌] MD5 mismatch reported by device for {filename}")
                return False  # No point retrying if hashes don’t match

            else:
                print(f"[!] Unexpected status {response.status_code} for {filename}. Retrying...")

        except Exception as e:
            print(f"[!] Error validating {filename} on {ip}: {e}. Retrying...")

        time.sleep(2)






