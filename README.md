# network-driver
A network abstraction framework aimed at implementing different (multithreaded) multiplexing solutions, encapsulated by a common API.
This is a project I work on in my free time. It may be useful to some, but I just have fun designing this framework :)

# Requirements

- C++20
- Linux >= 2.6.2 (Epoll introduced EPOLLONESHOT with this version)
- CMake
- Threads
- OpenSSL (I may relax this with a config option for people that don't want encrypted transport)

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

There are many tests bundled with this project, but they may not provide a good base to start. Have a look at my [benchmark](https://github.com/jakobod/network-driver-benchmark) repo where I actually put the whole stack to some use. 


# TODO
- [ ] datagram_transport

- [x] multithreaded epoll implementation
  - [ ] Check for race-conditions?!
- [ ] io_uring implementation
- [ ] EBPF?! - https://www.nginx.com/blog/our-roadmap-quic-http-3-support-nginx/

# Interesting Links
- https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/
- https://idea.popcount.org/2017-03-20-epoll-is-fundamentally-broken-22/
