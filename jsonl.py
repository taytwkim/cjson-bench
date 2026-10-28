#!/usr/bin/env python3
"""Convert JSON to JSON Lines, extracting tweets or events from known files."""

import argparse
import json
from pathlib import Path
import sys


def main():
    # __doc__ is docstring at the top of the file
    parser = argparse.ArgumentParser(description=__doc__)
    
    # Usage: python3 jsonl.py json/twitter.json jsonl/twitter.jsonl
    
    # add first argument
    parser.add_argument("input", type=Path, help="input JSON file")

    # add second argument
    parser.add_argument(
        "output", type=Path, nargs="?", help="output JSONL file (default: stdout)"
    )

    args = parser.parse_args()

    try:
        if args.output and args.input.resolve() == args.output.resolve():
            parser.error("input and output must be different files")

        with args.input.open(encoding="utf-8") as source:
            data = json.load(source)

        if args.input.stem == "twitter":
            if not isinstance(data, dict) or not isinstance(data.get("statuses"), list):
                parser.error("twitter input must contain a 'statuses' array")

            records = data["statuses"]

        elif args.input.stem == "citm_catalog":
            if not isinstance(data, dict) or not isinstance(data.get("events"), dict):
                parser.error("citm_catalog input must contain an 'events' object")

            records = list(data["events"].values())

        if not all(isinstance(record, dict) for record in records):
            parser.error("each extracted record must be a JSON object")

        def write_records(destination):
            for record in records:
                destination.write(json.dumps(record, separators=(",", ":")) + "\n")

        if args.output:
            with args.output.open("w", encoding="utf-8") as destination:
                write_records(destination)
        else:
            write_records(sys.stdout)

    except (OSError, ValueError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")


if __name__ == "__main__":
    main()
