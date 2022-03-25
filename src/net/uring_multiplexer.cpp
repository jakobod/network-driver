/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/uring_multiplexer.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <utility>

#include "net/acceptor.hpp"
#include "net/error.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/socket_sys_includes.hpp"
#include "util/scope_guard.hpp"

namespace net {

uring_multiplexer::uring_multiplexer() {
  // nop
}

uring_multiplexer::~uring_multiplexer() {
  // nop
}

error uring_multiplexer::init(socket_manager_factory_ptr factory,
                              uint16_t port) {
  auto uring_res = io_uring_queue_init(max_uring_depth, &uring, 0);
  if (uring_res != 0)
    return error{net::runtime_error, std::string(strerror(-uring_res))};
  auto pipe_res = make_pipe();
  if (auto err = get_error(pipe_res))
    return *err;
  auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
  pipe_reader_ = pipe_fds.first;
  pipe_writer_ = pipe_fds.second;
  add(std::make_shared<pollset_updater>(pipe_reader_, this));
  // Create Acceptor
  auto socket_res = net::make_tcp_accept_socket(port);
  if (auto err = get_error(socket_res))
    return *err;
  auto accept_socket_pair = std::get<net::acceptor_pair>(socket_res);
  accept_socket_ = accept_socket_pair.first;
  factory_ = std::move(factory);
  return none;
}

void uring_multiplexer::start() {
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&uring_multiplexer::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
  }
}

void uring_multiplexer::shutdown() {
  if (std::this_thread::get_id() == mpx_thread_id_) {
  } else if (!shutting_down_) {
    std::byte code{pollset_updater::shutdown_code};
    auto res = write(pipe_writer_, util::byte_span(&code, 1));
    if (res != 1) {
      std::cerr << "ERROR could not write to pipe: "
                << last_socket_error_as_string() << std::endl;
      abort(); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

void uring_multiplexer::join() {
  if (mpx_thread_.joinable())
    mpx_thread_.join();
}

bool uring_multiplexer::running() {
  return mpx_thread_.joinable();
}

void uring_multiplexer::set_thread_id() {
  mpx_thread_id_ = std::this_thread::get_id();
}

// -- Error handling -----------------------------------------------------------

void uring_multiplexer::handle_error(error err) {
  std::cerr << "ERROR: " << err << std::endl;
  shutdown();
}

// -- Interface functions ------------------------------------------------------

error uring_multiplexer::poll_once(bool) {
  io_uring_cqe* cqe = nullptr;

  while (running_) {
    std::cout << "------- waiting for events -------" << std::endl;
    auto ret = io_uring_wait_cqe(&uring, &cqe);
    std::cout << "got event" << std::endl;
    request_ptr req(reinterpret_cast<request*>(io_uring_cqe_get_data(cqe)));
    if (ret < 0)
      return error{runtime_error,
                   "io_uring_wait_cqe: " + std::string(strerror(-ret))};
    if (cqe->res < 0)
      return error{runtime_error,
                   "Async request failed: " + std::string(strerror(-cqe->res))
                     + " for event: " + to_string(req->ev)};
    auto& mgr = managers_[req->sock.id];
    switch (req->ev) {
      case event::accept: {
        std::cout << "ACCEPT " << cqe->res << std::endl;
        auto new_mgr = mgr->handle_accept(tcp_stream_socket{cqe->res});
        add(std::move(req));
        break;
      }
      case event::read:
        std::cout << "READ " << cqe->res << std::endl;
        if (cqe->res == 0) {
          del(req->sock);
        } else {
          mgr->handle_data(
            {reinterpret_cast<std::byte*>(req->iov.iov_base), cqe->res});
          // Readd the same read request.
          add(std::move(req));
        }
        break;
      case event::write:
        std::cout << "WRITE: " << cqe->res << std::endl;
        break;
      default:
        return error{runtime_error,
                     "Got unknown event type: " + to_string(req->ev)};
    }
    // Mark this request as processed
    io_uring_cqe_seen(&uring, cqe);
    cqe = nullptr;
  }
  return none;
}

void uring_multiplexer::add(socket_manager_ptr mgr) {
  std::cout << "add socket_manager" << std::endl;
  managers_.emplace(mgr->handle().id, mgr);
}

void uring_multiplexer::add(request_ptr req) {
  std::cout << "add_request: " << to_string(req->ev) << std::endl;
  io_uring_sqe* sqe = io_uring_get_sqe(&uring);
  io_uring_prep_writev(sqe, req->sock.id, &req->iov, 1, 0);
  io_uring_sqe_set_data(sqe, req.release());
  io_uring_submit(&uring);
}

error uring_multiplexer::handle_accept(tcp_stream_socket handle) {
  add(factory_->make(handle, this));
  return none;
}

void uring_multiplexer::del(socket handle) {
  std::cout << "del socket_manager" << std::endl;
  managers_.erase(handle.id);
}

uring_multiplexer::manager_map::iterator
uring_multiplexer::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  return new_it;
}

void uring_multiplexer::run() {
  while (running_) {
    if (poll_once(true))
      running_ = false;
  }
}

error_or<std::unique_ptr<uring_multiplexer>>
make_uring_multiplexer(socket_manager_factory_ptr factory, uint16_t port) {
  auto mpx = std::make_unique<uring_multiplexer>();
  if (auto err = mpx->init(std::move(factory), port))
    return err;
  return mpx;
}

} // namespace net
