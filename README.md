# BucketKV

A high-performance, concurrent, in-memory Key-Value store and custom HTTP web server built entirely in C from scratch.

## Features
- **In-Memory Storage**: Lightning-fast read/write operations using a custom HashMap implementation.
- **True Concurrency**: Thread-safe architecture utilizing an array of bucket-level `pthread_mutex_t` locks. Allows multiple threads to insert and read simultaneously without race conditions or segfaults.
- **TTL (Time-To-Live)**: Support for self-destructing keys via background threads. Background workers are completely memory-safe.
- **HTTP Interface**: Interact with the database purely through standard HTTP `GET` and `POST` requests.

## Performance Benchmarks
A million-request stress test against the server yields sub-millisecond latencies across all percentiles. 

```text
Request 999999 done

--- Latency Stats ---
Total requests: 1000000
Average time: 0.00095s
p25 latency:  0.00087s
p50 latency:  0.00091s
p99 latency:  0.00149s
```

## How to Run
Compile the server using `gcc` and the POSIX thread library:
```bash
gcc bucketKV.c -o server -pthread
```

Run the server:
```bash
./server
```
The server will now be listening for connections on `http://localhost:7890`.

## API Endpoints
- **Save Data:** `POST /string` (Body: `key=value`) or `GET /string/<key>/&<value>`
- **Retrieve Data:** `GET /get/string/<key>`
- **Delete Data:** `GET /delete/string/<key>`
- **Set TTL (Auto-Delete):** `GET /time/string/<key>/&<delay_in_seconds>`
- **Export All to JSON:** `GET /jsonString`
- **Backup to Disk:** `GET /memoryString/<folder_name>`

## Testing
To run the latency benchmarks yourself, activate the Python virtual environment and run the provided script:
```bash
python benchmark.py
```
*(Requires the `requests` library: `pip install -r requirements.txt`)*
