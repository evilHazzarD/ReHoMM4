// MD5 (GOG's chunk checks) and SHA-256 (the manifest), incremental.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace h4::install {

class Md5 {
 public:
  Md5();
  void Update(const void* data, size_t n);
  std::string HexDigest();  // finishes; lower-case hex

 private:
  void Block(const uint8_t* p);
  uint32_t h_[4];
  uint64_t len_ = 0;
  uint8_t buf_[64];
  size_t used_ = 0;
};

class Sha256 {
 public:
  Sha256();
  void Update(const void* data, size_t n);
  std::string HexDigest();

 private:
  void Block(const uint8_t* p);
  uint32_t h_[8];
  uint64_t len_ = 0;
  uint8_t buf_[64];
  size_t used_ = 0;
};

std::string Md5Hex(const void* data, size_t n);

}  // namespace h4::install
