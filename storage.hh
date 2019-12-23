/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the the 3-Clause BSD License.
 * See the accompanying file LICENSE or the copy at 
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef _DL_STORAGE_
#define _DL_STORAGE_

#include <time.h>
#include <iostream>

#include <google/protobuf/message.h>

namespace dl {

  // copy input stream to output stream
  void copy(std::istream& in, std::ostream& out, long length = -1);

  // load file to output stream
  void read_file(const std::string& path, std::ostream& data);

  // save input stream to file
  void write_file(std::istream& data, const std::string& path);

  // variable integer encoding
  void write_varint32(uint32_t val, std::ostream& stream);

  // variable integer encoding
  bool read_varint32(uint32_t &val, std::istream& stream);

  // write delimited protobuf to stream
  void write_pb(const ::google::protobuf::Message& pb, std::ostream& stream);

  // read delimited protobuf from stream
  bool read_pb(::google::protobuf::Message& pb, std::istream& stream);

  // base64_encode
  void encode_base64(const std::string& data, std::string& base64);

  // base64_decode
  void decode_base64(const std::string& base64, std::string& data);

  // suseconds_t (microseconds) to string of format 2009-06-15 20:20:00.123456
  std::string usec_to_string(const suseconds_t& time);

  // suseconds_t (seconds) to string of format 2009-06-15 20:20:00
  std::string time_to_string(const time_t& time);

  // suseconds_t (microseconds) to string of format 20:20:00
  std::string time_string(const time_t& time);

  // suseconds_t (seconds) to string of format 2009-06-15
  std::string date_string(const time_t& time);

  // get local time
  time_t time_now();

  // get time zone relative to UCT
  time_t time_zone(const time_t& now);

  // get calandar date to time
  tm time_to_date(const time_t& time);

  // get calandar date to time
  time_t date_to_time(const tm& date);

} // namespace dl

#endif /* _DL_STORAGE_ */
