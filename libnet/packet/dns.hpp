/**
 *  @author    Jakob Otto
 *  @file      dns.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

struct dns_header {
  uin16_t id; // identification number

  uint8_t rd : 1;     // recursion desired
  uint8_t tc : 1;     // truncated message
  uint8_t aa : 1;     // authoritive answer
  uint8_t opcode : 4; // purpose of message
  uint8_t qr : 1;     // query/response flag

  uint8_t rcode : 4; // response code
  uint8_t cd : 1;    // checking disabled
  uint8_t ad : 1;    // authenticated data
  uint8_t z : 1;     // its z! reserved
  uint8_t ra : 1;    // recursion available

  uin16_t q_count;    // number of question entries
  uin16_t ans_count;  // number of answer entries
  uin16_t auth_count; // number of authority entries
  uin16_t add_count;  // number of resource entries
};

namespace packet {

class dns {};

} // namespace packet
