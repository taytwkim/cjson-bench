#!/usr/bin/env python3
"""Repeated runs for benchmarking."""

import argparse
from pathlib import Path
import subprocess


# __file__ is the file path of this script
DEFAULT_INPUT_JSONL_PATH = Path(__file__).resolve().parent / "jsonl/twitter.jsonl"
DEFAULT_OUTPUT_PATH = Path("out/results.txt")
DEFAULT_NUM_THREADS = 4
DEFAULT_NUM_REPETITIONS = 1000
DEFAULT_RUNS = 5


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--input-jsonl-path", default=DEFAULT_INPUT_JSONL_PATH, type=Path, help="input jsonl path")
    parser.add_argument("--output-path", default=DEFAULT_OUTPUT_PATH, type=Path, help="output path")
    parser.add_argument("--num-threads", default=DEFAULT_NUM_THREADS, type=int, help="# threads")
    parser.add_argument("--num-repetitions", default=DEFAULT_NUM_REPETITIONS, type=int, help="# passes per thread in each run")
    parser.add_argument("--runs", type=int, default=DEFAULT_RUNS, help="# separate runs")

    args = parser.parse_args()

    if args.runs <= 0:
        parser.error("runs must be positive")

    benchmark_path = Path(__file__).resolve().parent / "benchmark"

    command = [
        str(benchmark_path),
        str(args.input_jsonl_path),
        str(args.num_threads),
        str(args.num_repetitions),
    ]

    args.output_path.parent.mkdir(parents=True, exist_ok=True)

    total_elapsed = 0.0

    with args.output_path.open("w") as output:
        for trial in range(1, args.runs + 1):
            print(f"Run {trial}/{args.runs}", flush=True)

            # Inherit exported allocator settings from the shell.
            result = subprocess.run(
                command,
                check=True,
                stdout=subprocess.PIPE,
                text=True,
            )

            for line in result.stdout.splitlines():
                if line.startswith("Elapsed:"):
                    total_elapsed += float(line.split()[1])

            line = " | ".join(result.stdout.splitlines())
            output.write(f"Run {trial}: {line}\n")
            output.flush()

        average_elapsed = total_elapsed / args.runs
        summary = f"Average elapsed: {average_elapsed:.6f} s"

        print(summary)
        output.write(summary + "\n")


if __name__ == "__main__":
    main()
