/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the MIT License.
 * See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
 */

#include <sstream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <memory>
#include <ctime>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

#include "storage.hh"
#include "varint.hh"
#include "base64.h"

namespace dl {

  // copy limited number of bytes between streams
  void copy(std::istream& in, std::ostream& out, long length) {
    // copy entire buffer
    if (length < 0) {
      out << in.rdbuf();
      return;
    }

    // create copy buffer
    int count = 0;
    std::vector<char> buffer(1024 * 1024);
    char *data = buffer.data();

    // copy 'length' bytes
    while (count < length) {
      int size = (length - count < buffer.size()) ?
        (length - count) : buffer.size();

      in.read(data, size);
      out.write(data, size);
      count += size;
    }
  }

  // read file to stream
  void read_file(const std::string& path, std::ostream& data) {
    // open the file
    std::ifstream file(path, std::ios::in | std::ifstream::binary);
    if (!file) {
      std::ostringstream error;
      error << "Failed to read file '" << path << "'. Error code ";
      error << errno << " ." << std::endl;
      throw std::runtime_error(error.str());
    }

    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // read the data
    data << file.rdbuf();
    file.close();
  }

  // write stream to file
  void write_file(std::istream& data, const std::string& path) {
    // open the file
    std::ofstream file(path, std::ios::out | std::ofstream::binary);
    if (!file) {
      std::ostringstream error;
      error << "Failed to write file '" << path << "'. Error code ";
      error << errno << " ." << std::endl;
      throw std::runtime_error(error.str());
    }

    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // write the data
    file << data.rdbuf();
    file.close();
  }

  // variable integer encoding
  void write_varint32(uint32_t val, std::ostream& stream) {
    uint8_t buffer[10];
    auto size = encodeVarint(val, buffer);
    stream.put(size);
    stream.write((const char*)buffer, size);
  }

  // variable integer encoding
  bool read_varint32(uint32_t &val, std::istream& stream) {
    uint8_t buffer[10];
    auto size = stream.get();
    if (stream.eof()) return false;
    if (size > sizeof(buffer)) return false;
    stream.read((char*)buffer, size);
    val = decodeVarint(buffer, size);
    return true;
  }

  // write protobuf message in envelope
  void write_pb(const ::google::protobuf::Message& pb, std::ostream& output) {
    dl::write_varint32(pb.ByteSize(), output);
    google::protobuf::io::OstreamOutputStream ostream(&output);
    google::protobuf::io::CodedOutputStream ocoded(&ostream);
    pb.SerializeWithCachedSizes(&ocoded);
  }

  // read protobuf message in envelope
  bool read_pb(::google::protobuf::Message& pb, std::istream& input) {
    // check EOF
    if (input.peek() == EOF) {
      return false;
    }

    uint32_t size;
    if (!dl::read_varint32(size, input)) {
      throw std::runtime_error("Failed to read protobuf message size");
    }

    // copy message to buffer
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
    input.read((char*)buffer.get(), size);
    google::protobuf::io::CodedInputStream icoded(buffer.get(), size);

    // parse message from buffer
    auto limit = icoded.PushLimit(size);
    if (!pb.ParseFromCodedStream(&icoded)) {
      std::ostringstream error;
      error << "Failed to read protobuf message of type '";
      error << pb.GetDescriptor()->name() << "'";
      throw std::runtime_error(error.str());
    }
    if (icoded.BytesUntilLimit() > 0) {
      std::ostringstream error;
      error << "Incomplete protobuf message of type '";
      error << pb.GetDescriptor()->name() << "'";
      throw std::runtime_error(error.str());
    }
    icoded.PopLimit(limit);

    return true;
  }

  // base64_encode
  void encode_base64(const std::string& data, std::string& base64) {
    std::vector<char> buffer(Base64encode_len(data.size()));
    Base64encode(buffer.data(), data.data(), data.size());
    base64 = base64.data();
  }

  // base64_decode
  void decode_base64(const std::string& base64, std::string& data) {
    std::vector<char> buffer(Base64decode_len(base64.data()));
    Base64decode(buffer.data(), base64.data());
    data = buffer.data();
  }

  // suseconds_t (microseconds) to string of format 2009-06-15 20:20:00.1234567
  std::string usec_to_string(const suseconds_t& time) {
    time_t secs = time / 1000000;
    uint32_t usec = time % 1000000;
    std::tm * ptm = std::gmtime(&secs);
    char buffer[32];
    auto len = std::strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", ptm);
    std::sprintf(buffer + len, ".%06d", usec);
    return std::string(buffer);
  }

  // time_t (seconds) to string of format 2009-06-15 20:20:00
  std::string time_to_string(const time_t& time) {
    std::tm * ptm = std::gmtime(&time);
    char buffer[32];
    auto len = std::strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", ptm);
    return std::string(buffer);
  }

  // time_t (seconds) to date string of format 2009-06-15
  std::string date_string(const time_t& time) {
    std::tm * ptm = std::gmtime(&time);
    char buffer[32];
    auto len = std::strftime(buffer, 20, "%Y-%m-%d", ptm);
    return std::string(buffer);
  }

  // time_t (seconds) to time string of format 20:20:00
  std::string time_string(const time_t& time) {
    std::tm * ptm = std::gmtime(&time);
    char buffer[32];
    auto len = std::strftime(buffer, 20, "%H:%M:%S", ptm);
    return std::string(buffer);
  }

  // get local time
  time_t time_now() {
    return std::time(NULL);
  }

  // get time zone relative to UCT
  time_t time_zone(const time_t& now) {
    tm* _local = std::localtime(&now);
    time_t local = std::mktime(std::localtime(&now));
    time_t gmt = std::mktime(std::gmtime(&now));
    time_t dst = ((_local->tm_isdst>0) ? 3600 : 0);
    return difftime(local, gmt) + dst;
  }

  // get calandar date to time
  tm time_to_date(const time_t& time) {
    return *std::localtime(&time);
  }

  // get calandar date to time
  time_t date_to_time(const tm& date) {
    time_t now = std::time(NULL);
    tm* _local = std::localtime(&now);

    time_t local = std::mktime(std::localtime(&now));
    time_t gmt = std::mktime(std::gmtime(&now));

    *_local = date;
    return mktime(_local); // seconds
  }
  
} // namespace dl

