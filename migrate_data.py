#!/usr/bin/env python3
"""
Data Migration Script: Split existing data.csv into per-day files

This script reads an existing data.csv file from the Weather Station SD card
and splits it into per-day CSV files (YYYY-MM-DD.csv) for faster loading.

Usage:
    python3 migrate_data.py /path/to/data.csv /output/directory

The output directory will contain files like:
    2024-01-15.csv
    2024-01-16.csv
    etc.
"""

import sys
from datetime import datetime, timezone
from pathlib import Path
from collections import defaultdict
import os


def migrate_data(input_csv, output_dir):
    """
    Split data.csv by date into per-day files.

    Args:
        input_csv: Path to the input data.csv file
        output_dir: Directory to write per-day CSV files
    """

    input_path = Path(input_csv)
    if not input_path.exists():
        print(f"Error: Input file not found: {input_csv}")
        return False

    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    # Group data by date
    data_by_date = defaultdict(list)
    skipped = 0
    total_lines = 0

    print(f"Reading {input_csv}...")

    with open(input_path, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue

            # Skip header (any line that starts with non-digit)
            if not line[0].isdigit():
                print(f"  Skipping header line: {line[:50]}...")
                continue

            try:
                # Parse CSV: timestamp_ms is first field
                fields = line.split(',')
                if len(fields) < 4:
                    print(f"  Warning: Line {line_num} has too few fields, skipping")
                    skipped += 1
                    continue

                timestamp_ms = int(fields[0])

                # Convert milliseconds to seconds, then to datetime
                timestamp_s = timestamp_ms / 1000.0
                dt = datetime.fromtimestamp(timestamp_s, tz=timezone.utc)

                # Format date as YYYY-MM-DD
                date_str = dt.strftime('%Y-%m-%d')

                # Store line with date
                data_by_date[date_str].append(line)
                total_lines += 1

            except (ValueError, IndexError) as e:
                print(f"  Warning: Error parsing line {line_num}: {e}")
                skipped += 1
                continue

    print(f"\nProcessed {total_lines} data lines, skipped {skipped}")
    print(f"Found data for {len(data_by_date)} unique dates")

    # Write per-day files
    print(f"\nWriting per-day CSV files to {output_dir}...")

    for date_str in sorted(data_by_date.keys()):
        output_file = output_path / f"{date_str}.csv"
        lines = data_by_date[date_str]

        with open(output_file, 'w') as f:
            # Write all lines for this date
            for line in lines:
                f.write(line + '\n')

        print(f"  {date_str}.csv: {len(lines)} samples")

    print(f"\nMigration complete! Created {len(data_by_date)} per-day CSV files.")
    return True


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 migrate_data.py <input_csv> <output_directory>")
        print()
        print("Example:")
        print("  python3 migrate_data.py /Volumes/USBDRIVE/data.csv ./daily_data/")
        sys.exit(1)

    input_csv = sys.argv[1]
    output_dir = sys.argv[2]

    success = migrate_data(input_csv, output_dir)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
