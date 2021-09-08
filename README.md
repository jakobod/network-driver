# raw-sockets

# Reference
- https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/
- https://idea.popcount.org/2017-03-20-epoll-is-fundamentally-broken-22/

# TODO
- [ ] set nonblocking on all sockets in the multiplexer
- [ ] io_uring implementation

- [ ] Think about how to benchmark
- [ ] Measure all things individually
  - [ ] Latency
  - [ ] Throughput

- [ ] Include Drivers for different benchmarks
  - Driver types
    - Max-out driver
    - Max-connection driver
    - max throughput driver
    - only reading
    - only writing
    - bursts of connections/data
    - fluctuating load

- Tests
  [x] acceptor
  - epoll_multiplexer
  - pipe_socket
  - pollset_updater
  - raw_socket
  - stream_socket
  [x] tcp_accept_socket
  [x] tcp_stream_socket
