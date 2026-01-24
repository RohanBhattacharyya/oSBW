#include "StarSecureRandom.hpp"

#include <random>

namespace Star {

ByteArray secureRandomBytes(size_t size) {
  std::random_device rd;
  ByteArray bytes;
  bytes.resize(size);
  for (size_t i = 0; i < size; ++i)
    bytes[i] = static_cast<uint8_t>(rd());
  return bytes;
}

}
