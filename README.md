# network-driver
A network abstraction framework aimed at implementing different (multithreaded) multiplexing solutions, encapsulated by a common API.
I use this library to write benchmarks aiming to find a good approach to multithreaded networking.
The repo can be found [here](https://github.com/jakobod/network-driver-benchmark).

# Requirements

- Linux >= 2.6.2 (Epoll introduced EPOLLONESHOT with this version)
- CMake
- Threads

# How to
building the project is accomplished by doing
```
./configure
make -C build -j$(nproc)
```

If one wants to build tests, they have to be enabled explicitly
```
./configure --enable-testing
make -C build -j$(nproc)
make -C build test
```


# TODO
- [x] epoll implementation
- [x] multithreaded epoll implementation
- [ ] io_uring implementation
- [ ] EBPF?! - https://www.nginx.com/blog/our-roadmap-quic-http-3-support-nginx/

- [x] Implement receive-semantics at_most, at_least, exactly

# Interesting Links
- https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/
- https://idea.popcount.org/2017-03-20-epoll-is-fundamentally-broken-22/
