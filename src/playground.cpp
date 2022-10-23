#include "util/byte_span.hpp"
#include "util/error_or.hpp"
#include "util/logger.hpp"

#include "net/pipe_socket.hpp"

#include <array>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

struct type {};

int main(int, char**) {
  auto pipe_res = net::make_pipe();
  if (auto err = util::get_error(pipe_res)) {
    LOG_ERROR("Could not create pipe");
    std::exit(-1);
  }
  auto [pipe_reader, pipe_writer] = std::get<net::pipe_socket_pair>(pipe_res);

  constexpr const std::uint8_t code = 42;
  type t;
  type* t_ptr = &t;
  std::array<std::uint8_t, sizeof(std::uint8_t) + sizeof(type*)> write_buf;
  write_buf[0] = code;
  std::copy(reinterpret_cast<std::uint8_t*>(&t_ptr),
            reinterpret_cast<std::uint8_t*>(&t_ptr) + sizeof(type*),
            write_buf.begin() + 1);
  if (auto res = write(pipe_writer, util::as_const_bytes(write_buf));
      res <= 0) {
    LOG_ERROR("Could not write to pipe (", res,
              "): ", net::last_socket_error_as_string());
    std::exit(-1);
  }

  std::array<std::uint8_t, sizeof(std::uint8_t) + sizeof(type*)> read_buf;
  if (auto res = read(pipe_reader, util::as_bytes(read_buf)); res <= 0) {
    LOG_ERROR("Could not read from pipe(", res,
              "): ", net::last_socket_error_as_string());
    std::exit(-1);
  }

  auto received_code = read_buf[0];
  type* received_t_ptr = nullptr;
  std::copy(read_buf.begin() + 1, read_buf.end(),
            reinterpret_cast<std::uint8_t*>(&received_t_ptr));

  LOG_DEBUG(NET_ARG(code), " == ", NET_ARG(received_code));
  LOG_DEBUG(NET_ARG(t_ptr), " == ", NET_ARG(received_t_ptr));
  return 0;
}
