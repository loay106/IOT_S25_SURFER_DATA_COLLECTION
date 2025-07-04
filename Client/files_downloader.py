import os
import time
import hashlib
import requests
import urllib.parse

__all__ = [
    'SAMPLINGS_DOWNLOAD_DIRECTORY',
    'list_available_sampling_files',
    'download_file',
    "remove_all_samplings"
]

PORT = 80
LIST_SAMPLINGS_ENDPOINT = '/samplings/list'
DOWNLOAD_ENDPOINT = '/download?file='
VALIDATE_ENDPOINT = "/validate"
REMOVE_ALL_SAMPLINGS = "/samplings/delete"

SAMPLINGS_DOWNLOAD_DIRECTORY = './tmp/samplings'


def list_available_sampling_files(hostname, ip) -> list:
    url = f"http://{ip}:{PORT}{LIST_SAMPLINGS_ENDPOINT}"
    response = requests.get(url)
    # print(f"[DEBUG] Raw response from {ip}:", response.text)
    return response.json().get("files")


def download_file(hostname, ip, filename, validate_download: bool):
    filename_only = os.path.basename(filename)
    device_dir = os.path.join(SAMPLINGS_DOWNLOAD_DIRECTORY, hostname)
    os.makedirs(device_dir, exist_ok=True)

    tmp_path = os.path.join(device_dir, filename_only + ".tmp")
    final_path = os.path.join(device_dir, filename_only)

    if os.path.exists(final_path):
        print(f"[+] {filename} was downloaded before, skipping file...")
        return

    if os.path.exists(tmp_path):
        os.remove(tmp_path)

    while True:
        try:
            encoded_filename = urllib.parse.quote(filename)
            url = f"http://{ip}:{PORT}{DOWNLOAD_ENDPOINT}{encoded_filename}"
            response = requests.get(url)

            if response.ok:
                with open(tmp_path, 'wb') as f:
                    f.write(response.content)
                print(f"[+] Downloaded {filename} from {hostname} saved to {tmp_path}")

                if validate_download:
                    # Calculate MD5 locally
                    local_md5 = _calculate_md5(tmp_path)
                    if local_md5:
                        _validate_file_on_device(hostname, ip, filename, local_md5)
                    else:
                        print(f"[!] Failed to calculate MD5 for {filename_only}")
                    os.rename(tmp_path, final_path)
                    break  # Exit loop on success
                else:
                    os.rename(tmp_path, final_path)
                    break  # Exit loop on success
            else:
                print(f"[!] Download failed with status {response.status_code}. Retrying...")

        except Exception as e:
            print(f"[!] Error downloading {filename} from {hostname}: {e}. Retrying...")

        time.sleep(2)  # Avoid spamming the device/network


def remove_all_samplings(hostname, ip):
    try:
        url = f"http://{ip}:{PORT}{REMOVE_ALL_SAMPLINGS}"
        response = requests.post(url)
        if response.ok:
            print(f"All samplings removed from {hostname}")
            return True
        else:
            print(f"[!] Failed to remove all samplings from {hostname} (status {response.status_code})")
    except Exception as e:
        print(f"[!] Error contacting {hostname} to remove samplings: {e}")
    return False


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


def _validate_file_on_device(hostname, ip, filename, expected_md5):
    url = f"http://{ip}:{PORT}{VALIDATE_ENDPOINT}"
    params = {
        "file": filename,
        "md5": expected_md5
    }

    while True:
        try:
            response = requests.get(url, params=params)

            if response.status_code == 200:
                print(f"MD5 validated on device for {filename}")
                return True

            elif response.status_code == 409:
                print(f"MD5 mismatch reported by device for {filename}")
                return False  # No point retrying if hashes don’t match

            else:
                print(f"[!] Unexpected status {response.status_code} for {filename}. Retrying...")

        except Exception as e:
            print(f"[!] Error validating {filename} on {hostname}: {e}. Retrying...")

        time.sleep(2)
