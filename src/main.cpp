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
#include <sys/stat.h>
#include <unistd.h>

#include "net/acceptor.hpp"
#include "net/error.hpp"
#include "net/tcp_stream_socket.hpp"
#include "util/scope_guard.hpp"

static constexpr size_t READ_SZ = 8192;

enum class event {
  accept = 0,
  read = 1,
  write = 2,
};

struct request {
  event ev;
  net::tcp_stream_socket sock;
  std::array<iovec, 1> iov;
};

io_uring ring;

static constexpr const char* http_response
  = "HTTP/1.0 404 Not Found\r\n"
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
int add_accept_request(net::tcp_accept_socket acc, sockaddr_in* client_addr,
                       socklen_t* client_addr_len) {
  auto sqe = io_uring_get_sqe(&ring);
  io_uring_prep_accept(sqe, acc.id, reinterpret_cast<sockaddr*>(client_addr),
                       client_addr_len, 0);
  auto req = new request;
  req->ev = event::accept;
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring);
  return 0;
}

int add_read_request(net::tcp_stream_socket sock) {
  auto sqe = io_uring_get_sqe(&ring);
  auto req = new request;
  req->iov[0].iov_base = new uint8_t[READ_SZ];
  req->iov[0].iov_len = READ_SZ;
  req->ev = event::read;
  req->sock = sock;
  /* Linux kernel 5.5 has support for readv, but not for recv() or read() */
  io_uring_prep_readv(sqe, sock.id, req->iov.data(), 1, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring);
  return 0;
}

void add_write_request(request* req) {
  std::cout << "add_write_request" << std::endl;
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
  req->ev = event::write;
  io_uring_prep_writev(sqe, req->sock.id, req->iov.data(), req->iov.size(), 0);
  io_uring_sqe_set_data(sqe, &req);
  io_uring_submit(&ring);
}

void reply(const std::string& line, net::tcp_stream_socket sock) {
  std::cout << "reply " << line << std::endl;
  auto req = new request;
  req->sock = sock;
  req->iov[0].iov_base = new uint8_t[line.size()];
  req->iov[0].iov_len = line.size();
  memcpy(req->iov[0].iov_base, line.c_str(), line.size());
  add_write_request(req);
}

int handle_client_request(request* req) {
  std::istringstream iss(
    std::string(reinterpret_cast<const char*>(req->iov.begin()->iov_base),
                req->iov.begin()->iov_len));
  std::string line;
  std::getline(iss, line);
  std::cout << "handle_client_request with line: " << line << std::endl;
  reply(line, {req->sock});
  return 0;
}

void server_loop(net::tcp_accept_socket server_socket) {
  struct io_uring_cqe* cqe;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  add_accept_request(server_socket, &client_addr, &client_addr_len);

  while (1) {
    int ret = io_uring_wait_cqe(&ring, &cqe);
    auto req = reinterpret_cast<request*>(cqe->user_data);
    if (ret < 0)
      fail("io_uring_wait_cqe");
    if (cqe->res < 0)
      fail("Async request failed: " + std::string(strerror(-cqe->res))
           + " for event: " + std::to_string(static_cast<int>(req->ev)));

    switch (req->ev) {
      case event::accept:
        std::cout << "ACCEPT " << cqe->res << std::endl;
        add_accept_request(server_socket, &client_addr, &client_addr_len);
        add_read_request(net::tcp_stream_socket{cqe->res});
        delete (req);
        break;
      case event::read:
        std::cout << "READ " << cqe->res << std::endl;
        if (!cqe->res) {
          std::cerr << "Empty request!" << std::endl;
          break;
        }
        handle_client_request(req);
        delete[] reinterpret_cast<char*>(req->iov[0].iov_base);
        delete req;
        break;
      case event::write:
        std::cout << "WRITE: " << cqe->res << std::endl;
        for (auto& iov : req->iov)
          delete[] reinterpret_cast<char*>(iov.iov_base);
        close(req->sock);
        free(req);
        break;
    }
    /* Mark this request as processed */
    io_uring_cqe_seen(&ring, cqe);
  }
}

int main() {
  util::scope_guard guard([&]() { io_uring_queue_exit(&ring); });
  auto acceptor_res = net::make_tcp_accept_socket(8080);
  if (auto err = net::get_error(acceptor_res))
    fail("Failed to create an acceptor: " + net::to_string(*err));
  auto [server_socket, port] = std::get<net::acceptor_pair>(acceptor_res);
  if (!net::reuseaddr(server_socket, true))
    fail("reuseaddr");
  io_uring_queue_init(10, &ring, 0);
  server_loop(server_socket);
  return 0;
}
