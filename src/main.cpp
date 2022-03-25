#include <cstddef>
#include <ctype.h>
#include <fcntl.h>
#include <iostream>
#include <liburing.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

#include "net/acceptor.hpp"
#include "net/error.hpp"
#include "net/socket.hpp"
#include "net/tcp_stream_socket.hpp"
#include "util/scope_guard.hpp"

static constexpr size_t READ_SZ = 8192;

enum class event : uint8_t {
  accept = 0,
  read = 1,
  write = 2,
  none = 255,
};

std::string to_string(event e) {
  switch (e) {
    case event::none:
      return "event::none";
    case event::accept:
      return "event::accept";
    case event::read:
      return "event::read";
    case event::write:
      return "event::write";
    default:
      return std::to_string(static_cast<uint8_t>(e));
  }
}

struct request {
  request() : ev{event::none}, sock{net::invalid_socket}, iov{nullptr, 0} {
    std::cout << "default init of request" << std::endl;
  }

  request(event e, net::socket sock, size_t size)
    : ev{e}, sock{sock}, iov{new std::byte[size], size} {
    std::cout << "qualified init of request" << std::endl;
  }

  ~request() {
    delete[] reinterpret_cast<std::byte*>(iov.iov_base);
  }

  const event ev;
  const net::socket sock;
  const iovec iov;
};

io_uring ring;

static constexpr std::string_view http_response
  = "HTTP/2 200 OK\r\n"
    "Content-type: text/html\r\n"
    "\r\n"
    "<html>"
    "<head>"
    "<title>My own little server</title>"
    "</head>"
    "<body>"
    "<h1>Hello World</h1>"
    "</body>"
    "</html>";

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "ERROR: " << msg << std::endl;
  exit(EXIT_FAILURE);
}

/// This function is responsible for setting up the main listening socket used
/// by the web server.
void add_accept_request(net::tcp_accept_socket acc, sockaddr_in* client_addr,
                        socklen_t* client_addr_len) {
  std::cout << "adding accept request" << std::endl;
  auto sqe = io_uring_get_sqe(&ring);
  io_uring_prep_accept(sqe, acc.id, reinterpret_cast<sockaddr*>(client_addr),
                       client_addr_len, 0);
  auto req = new request(event::accept, acc, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring);
}

void add_read_request(net::tcp_stream_socket sock) {
  std::cout << "adding read request" << std::endl;
  auto sqe = io_uring_get_sqe(&ring);
  auto req = new request(event::read, sock, READ_SZ);
  /* Linux kernel 5.5 has support for readv, but not for recv() or read() */
  io_uring_prep_readv(sqe, sock.id, &req->iov, 1, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring);
}

void add_write_request(util::const_byte_span bytes,
                       net::tcp_stream_socket sock) {
  std::cout << "add_write_request" << std::endl;
  io_uring_sqe* sqe = io_uring_get_sqe(&ring);
  auto req = new request(event::write, sock, bytes.size());
  std::cout << "new request @ " << std::hex << req << std::endl;
  mempcpy(req->iov.iov_base, bytes.data(), bytes.size());
  io_uring_prep_writev(sqe, req->sock.id, &req->iov, 1, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring);
}

void server_loop(net::tcp_accept_socket server_socket) {
  io_uring_cqe* cqe = nullptr;
  sockaddr_in client_addr = {};
  socklen_t client_addr_len = sizeof(client_addr);

  add_accept_request(server_socket, &client_addr, &client_addr_len);

  while (true) {
    std::cout << "------- waiting for events -------" << std::endl;
    auto ret = io_uring_wait_cqe(&ring, &cqe);
    std::cout << "got event" << std::endl;

    auto req = reinterpret_cast<request*>(io_uring_cqe_get_data(cqe));
    util::scope_guard g([req] {
      std::cout << "scope_guard" << std::endl;
      delete req;
    });
    std::cout << "request from cqe @ " << std::hex << req << std::endl;

    if (ret < 0)
      fail("io_uring_wait_cqe");
    if (!cqe) {
      std::cerr << "Got empty cqe" << std::endl;
      continue;
    }
    if (cqe->res < 0)
      fail("Async request failed: " + std::string(strerror(-cqe->res))
           + " for event: " + to_string(req->ev));
    auto ev = req->ev;
    std::cout << "req->ev = " << to_string(ev) << "cqe->res = " << cqe->res
              << std::endl;
    switch (ev) {
      case event::accept:
        std::cout << "ACCEPT " << cqe->res << std::endl;
        add_accept_request(server_socket, &client_addr, &client_addr_len);
        add_read_request(net::tcp_stream_socket{cqe->res});
        break;
      case event::read:
        std::cout << "READ " << cqe->res << std::endl;
        if (!cqe->res) {
          std::cerr << "Empty request!" << std::endl;
          break;
        }
        add_write_request(
          util::const_byte_span{reinterpret_cast<const std::byte*>(
                                  http_response.data()),
                                http_response.size()},
          net::socket_cast<net::tcp_stream_socket>(req->sock));
        break;
      case event::write:
        std::cout << "WRITE: " << cqe->res << std::endl;
        close(req->sock);
        break;
      default:
        fail("Got unknown event type: " + to_string(ev));
        break;
    }
    /* Mark this request as processed */
    io_uring_cqe_seen(&ring, cqe);
    cqe = nullptr;
  }
}

int main() {
  util::scope_guard guard([&]() { io_uring_queue_exit(&ring); });
  auto acceptor_res = net::make_tcp_accept_socket(0);
  if (auto err = net::get_error(acceptor_res))
    fail("Failed to create an acceptor: " + net::to_string(*err));
  auto [server_socket, port] = std::get<net::acceptor_pair>(acceptor_res);
  if (!net::reuseaddr(server_socket, true))
    fail("reuseaddr");
  std::cout << "http://127.0.0.1:" << port << std::endl;
  std::cout << "telnet 127.0.0.1 " << port << std::endl;
  io_uring_queue_init(10, &ring, 0);
  server_loop(server_socket);
  return 0;
}
