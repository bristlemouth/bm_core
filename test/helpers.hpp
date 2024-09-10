#ifndef __TEST_HELPERS_HPP__
#define __TEST_HELPERS_HPP__

#include <ostream>

// Generate a sequence of random unique numbers
// see: https://preshing.com/20121224/how-to-generate-a-sequence-of-unique-random-integers/
class rand_sequence_unique {
private:
  uint32_t i;
  uint32_t intermediate;

  static uint32_t qpr(uint32_t x) {
    static const uint32_t prime = 4294967291;
    if (x >= prime) {
      return x;
    }
    uint32_t residue = ((uint64_t)x * x) % prime;
    return (x <= prime / 2) ? residue : prime - residue;
  }

public:
  rand_sequence_unique(uint32_t base, uint32_t offset) {
    i = qpr(qpr(base) + 0x682f0161);
    intermediate = qpr(qpr(offset) + 0x46790905);
  }

  unsigned int next() { return qpr((qpr(i++) + intermediate) ^ 0x5bf03635); }
};

class rnd_gen {
public:
  rnd_gen() { srand(time(NULL)); }

  size_t rnd_int(size_t mod, size_t min) {
    size_t ret = 0;
    ret = rand() % mod;
    ret = ret < min ? min : ret;
    return ret;
  }

  void rnd_array(uint8_t *buf, size_t size) {
    for (size_t i = 0U; i < size; i++) {
      buf[i] = rand() % UINT8_MAX;
    }
  }
};

#endif
