import csv
import glob
import os

__all__ = [
    'merge_files'
]


def merge_files(timestamp, base_dir="./tmp/samplings", output_dir="./samplings"):
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, f"{timestamp}.csv")

    with open(output_path, "w", newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["device_id", "sensor_id", "sensor_model", "sampling_read_value"])

        # Search all subdirectories for matching files
        pattern = os.path.join(base_dir, "*", f"{timestamp}_*_*")  # match per sensor
        filepaths = glob.glob(pattern)

        for filepath in filepaths:
            try:
                device_id = os.path.basename(os.path.dirname(filepath))
                filename = os.path.basename(filepath)
                parts = filename.split("_")

                if len(parts) < 3:
                    print(f"[!] Skipping malformed file: {filename}")
                    continue

                sensor_id = parts[1]
                sensor_model = parts[2].split(".")[0]  # remove .txt if present

                # Read sampling values
                with open(filepath, "r") as f:
                    line = f.read().strip()
                    print(f"[DEBUG] Reading from {filepath}: {line}")
                    values = [v for v in line.split("|") if v.strip() != ""]

                if not values or (len(values) == 1 and values[0] == ''):
                    print(f"[!] No sampling values found in file: {filename}")
                    continue

                for value in values:
                    # print(f"[DEBUG] Writing row: {device_id}, {sensor_id}, {sensor_model}, {value}")
                    writer.writerow([device_id, sensor_id, sensor_model, value])

            except Exception as e:
                print(f"[!] Failed to merge file {filepath}: {e}")

    print(f"[✓] Merged CSV created: {output_path}")
