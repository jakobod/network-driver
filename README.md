# raw-sockets

# Reference
- https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/
- https://idea.popcount.org/2017-03-20-epoll-is-fundamentally-broken-22/

# TODO
- io_uring implementation

- Think about how to benchmark
- Measure all things individually
  - Latency
  - Throughput

- Include Drivers for different benchmarks
  - Driver types
    - Max-out driver
    - Max-connection driver
    - max throughput driver
    - only reading
    - only writing
    - bursts of connections/data
    - fluctuating load
