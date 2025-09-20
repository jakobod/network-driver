#include "net/uring_multiplexer.hpp"

#include "net/application.hpp"
#include "net/manager_factory.hpp"
#include "net/receive_policy.hpp"
#include "net/uring_manager_impl.hpp"

#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/intrusive_ptr.hpp"
#include "util/logger.hpp"

#include <iostream>

using namespace net;

namespace {

struct mirror_application : public application {
  mirror_application(manager* parent) : parent_{parent} {
    // nop
  }

  ~mirror_application() override = default;

  util::error init(const util::config&) override {
    parent_->configure_next_read(receive_policy::up_to(2048));
    parent_->start_reading();
    return util::none;
  }

  util::error consume(util::const_byte_span data) override {
    auto& buf = parent_->write_buffer();
    buf.insert(buf.end(), data.begin(), data.end());
    parent_->start_writing();
    return util::none;
  }

private:
  manager* parent_{nullptr};
};

struct manager_factory_impl : public manager_factory {
  uring_manager_ptr make_uring_manager(sockets::socket handle,
                                       uring_multiplexer* mpx) override {
    using manager_type = uring_manager_impl<mirror_application>;
    auto mgr           = util::make_intrusive<manager_type>(handle, mpx);
    return mgr;
  }
};

} // namespace

int main(int, char**) {
  util::config cfg;
  cfg.add_config_entry("logger.terminal-output", true);
  LOG_INIT(cfg);
  LOG_DEBUG("Something");

  uring_multiplexer mpx{std::make_unique<manager_factory_impl>()};
  if (const auto err = mpx.init(cfg)) {
    std::cout << err << std::endl;
    return EXIT_FAILURE;
  }

  mpx.start();
  mpx.join();

  return EXIT_SUCCESS;
}

// constexpr int QUEUE_DEPTH = 256;

// enum class op_type { ACCEPT, READ, WRITE, POLL };

// struct conn_info {
//   int fd                        = {};
//   op_type type                  = {};
//   std::array<char, 2048> buffer = {};
// };

// enum class manager_type {
//   acceptor,
// };

// void register_read(io_uring* ring, int fd) {
//   auto* read_info   = new conn_info{fd, op_type::READ};
//   io_uring_sqe* sqe = io_uring_get_sqe(ring);
//   io_uring_prep_recv(sqe, fd, read_info->buffer.data(),
//                      read_info->buffer.size(), 0);
//   io_uring_sqe_set_data(sqe, read_info);
//   io_uring_submit(ring);
// }

// int main() {
//   // 1. Setup listening socket
//   const auto res = sockets::make_tcp_accept_socket(
//     net::ip::v4_endpoint{net::ip::v4_address::any, 0});
//   if (auto err = util::get_error(res)) {
//     perror("socket");
//     return 1;
//   }

//   const auto [accept_socket, port] =
//   std::get<net::sockets::acceptor_pair>(res); sockets::socket_guard
//   guard{accept_socket};

//   // 2. Init io_uring
//   io_uring ring{};
//   if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
//     perror("io_uring_queue_init");
//     return 1;
//   }

//   // 3. Submit initial accept
//   auto* sqe  = io_uring_get_sqe(&ring);
//   auto* info = new conn_info{accept_socket.id, op_type::ACCEPT};
//   io_uring_prep_accept(sqe, accept_socket.id, nullptr, nullptr, 0);
//   io_uring_sqe_set_data(sqe, info);
//   io_uring_submit(&ring);

//   std::cout << "Server listening on port " << port
//             << ", with fd=" << accept_socket.id << "...\n";

//   // 4. Event loop
//   while (true) {
//     io_uring_cqe* cqe = nullptr;

//     std::cout << "Waiting for incoming events\n";

//     int ret = io_uring_wait_cqe(&ring, &cqe);
//     if (ret < 0) {
//       perror("io_uring_wait_cqe");
//       break;
//     }
//     std::cout << "Got cqe!\n";

//     auto* info = reinterpret_cast<conn_info*>(io_uring_cqe_get_data(cqe));
//     int res    = cqe->res;
//     io_uring_cqe_seen(&ring, cqe);

//     if (!info) {
//       continue;
//     }

//     std::cout << "Got some info!\n";

//     switch (info->type) {
//       case op_type::ACCEPT: {
//         if (res > 0) {
//           int client_fd = res;
//           std::cout << "New client fd=" << client_fd << "\n";
//           register_read(&ring, client_fd);
//         }

//         // Re-arm accept
//         io_uring_sqe* sqe = io_uring_get_sqe(&ring);
//         io_uring_prep_accept(sqe, accept_socket.id, nullptr, nullptr, 0);
//         io_uring_sqe_set_data(sqe, info);
//         io_uring_submit(&ring);
//         break;
//       }

//       case op_type::READ: {
//         if (res > 0) {
//           int n            = res;
//           auto* write_info = new conn_info{info->fd, op_type::WRITE};
//           memcpy(write_info->buffer.data(), info->buffer.data(), n);
//           sqe = io_uring_get_sqe(&ring);
//           io_uring_prep_send(sqe, info->fd, write_info->buffer.data(),
//                              write_info->buffer.size(), 0);
//           io_uring_sqe_set_data(sqe, write_info);

//           // Rearm read
//           sqe = io_uring_get_sqe(&ring);
//           io_uring_prep_recv(sqe, info->fd, info->buffer.data(),
//                              info->buffer.size(), 0);
//           io_uring_sqe_set_data(sqe, info);
//           io_uring_submit(&ring);
//         } else {
//           std::cout << "Client disconnected fd=" << info->fd << "\n";
//           close(info->fd);
//         }
//         break;
//       }

//       case op_type::WRITE: {
//         if (res > 0) {
//           // Re-arm poll for next read
//           auto* poll_info   = new conn_info{info->fd, op_type::POLL};
//           io_uring_sqe* sqe = io_uring_get_sqe(&ring);
//           io_uring_prep_poll_add(sqe, info->fd, POLL_IN);
//           io_uring_sqe_set_data(sqe, poll_info);
//           io_uring_submit(&ring);
//         }
//         delete info;
//         break;
//       }
//     }
//   }

//   io_uring_queue_exit(&ring);
//   return 0;
// }
