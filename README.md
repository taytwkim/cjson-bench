# JSON Parsing Benchmark

JSON parsing workload using [cJSON](https://github.com/DaveGamble/cJSON).

I implemented a memory allocator and wanted to test it on a realistic workload with interesting allocation patterns. A JSON parser seemed like a good fit, as JSON can be nested and contain different types, leading to varied allocation sizes and patterns.

`main.c` creates multiple threads. Each thread reads JSON objects from a `jsonl` file, parsing each object and immediately deleting the resulting tree.

A more realistic workload could use a shared work queue to distribute objects from the same file among threads. But since the `jsonl` files I have aren't that big, I have each thread parse the same, entire file.

The files under `jsonl/` were extracted from datasets in [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark/tree/master/data).

Allocation interposition relies on Linux `LD_PRELOAD`.

## Usage

### Data

`jsonl.py` is a Python script that extracts tweets and events from `twitter.json` and `citm_catalog.json`. The output contains one JSON object per line, making it easier to process individual entries than in the original JSON files.

```bash
# python3 jsonl.py input_json_filename output_jsonl_filename 
python3 jsonl.py json/twitter.json jsonl/twitter.jsonl
```

### JSON Parsing

`main.c` is the main program that creates threads and parses the JSON objects.

```bash
# Build the program
make 

# Usage: ./benchmark FILENAME NUM_THREADS NUM_REPETITIONS"
./benchmark jsonl/twitter.jsonl 2 1
```

`NUM_REPETITIONS` can be set to test the program at scale by having each thread make multiple passes over the input file.

The program prints the measured wall-clock time and throughput.

```
> ./benchmark jsonl/twitter.jsonl 2 1
Threads: 2
Records per pass: 100
Repetitions per thread: 1
Total records: 200
Elapsed: 0.003192 s
Throughput: 62650 records/s
```

Timing starts after the threads have been created. There is no synchronization between threads during the parsing phase, though `malloc` uses locks internally for thread safety.

### Benchmarking

`run_bench.py` runs the `benchmark` binary and writes the results to an output file.

```shell
# Load and configure malloc through env var.
export LD_PRELOAD=/path/to/my/malloc.so

# Run benchmark.
python3 run_bench.py

# (Optional) Unset env vars.
unset LD_PRELOAD  
```

Runs can be configured:

```shell
python3 run_bench.py \
  --input-jsonl-path jsonl/twitter.jsonl \
  --num-threads 8 \
  --num-repetitions 1000 \
  --runs 10 \
  --output-path out/threads8.txt
```