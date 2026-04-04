/**
 *  @author    Jakob Otto
 *  @file      dns.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

/// @brief DNS message header structure.
/// Represents the fixed 12-byte header format for DNS queries and responses,
/// containing flags for query type, response code, and resource record counts.
struct dns_header {
  uin16_t id; ///<< Transaction identifier for matching requests to responses

  uint8_t rd : 1;     ///<< Recursion desired (client request)
  uint8_t tc : 1;     ///<< Truncated message (response flag)
  uint8_t aa : 1;     ///<< Authoritative answer (response flag)
  uint8_t opcode : 4; ///<< Operation code (0=standard query, 1=inverse, etc.)
  uint8_t qr : 1;     ///<< Query/Response flag (0=query, 1=response)

  uint8_t rcode : 4; ///<< Response code (0=no error, 1=format error, etc.)
  uint8_t cd : 1;    ///<< Checking disabled
  uint8_t ad : 1;    ///<< Authenticated data
  uint8_t z : 1;     ///<< Reserved for future use
  uint8_t ra : 1;    ///<< Recursion available

  uin16_t q_count;    ///<< Number of question entries
  uin16_t ans_count;  ///<< Number of answer entries
  uin16_t auth_count; ///<< Number of authority entries
  uin16_t add_count;  ///<< Number of additional resource entries
};

namespace packet {

/// @brief DNS protocol layer class (placeholder).
/// Future DNS packet handling will extend this class for query/response
/// processing.
class dns {};

} // namespace packet
