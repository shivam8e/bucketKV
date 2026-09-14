import requests
import time

url = "http://localhost:7890/string"
latencies = []

try:
    for i in range(0, 1000000):    
        data = f'{i}=vagfgf{i}'

        headers = {
            'Content-Type': 'application/x-www-form-urlencoded'
        }

        start_time = time.time()
        response = requests.post(url, data=str(data), headers=headers)
        end_time = time.time()
        
        latencies.append(end_time - start_time)
        print(f"Request {i} done", end='\r')
        
except KeyboardInterrupt:
    print("\nStopping early...")

if latencies:
    latencies.sort()
    avg_time = sum(latencies) / len(latencies)
    p25 = latencies[int(len(latencies) * 0.25)]
    p50 = latencies[int(len(latencies) * 0.50)]
    p99 = latencies[int(len(latencies) * 0.99)]
    
    print("\n\n--- Latency Stats ---")
    print(f"Total requests: {len(latencies)}")
    print(f"Average time: {avg_time:.5f}s")
    print(f"p25 latency:  {p25:.5f}s")
    print(f"p50 latency:  {p50:.5f}s")
    print(f"p99 latency:  {p99:.5f}s")
