#pragma once

/*
 * This header gives the universal hash family.  Note that as of right
 * now it is "pseudo random," in the sense that it's not properly
 * seeded (this is to make it easier to debug/ work with, and is easy
 * to change in the case it is desired to be more random).  Here is
 * where the algorithm is stolen from: Introduction To Algorithms, MIT
 * Press
 */

#include <cassert>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <type_traits>


namespace data_org_project_names {


static auto Rand32 = bind(std::uniform_int_distribution<uint32_t>(), 
                        std::default_random_engine());

class UniHash16 {
 public:

  /*
   * Refresh
   */
  void
  Refresh() 
  {
    randomMask_ = Rand32();
    multiplier_ = Rand32();
    adder_      = Rand32();
  }

  /*
   * ToString
   */
  std::string 
  ToString() const
  {
    return "{randomMask_: " + std::to_string(randomMask_) + ", "
           "multiplier_: "  + std::to_string(multiplier_) + ", "
           "adder_: "       + std::to_string(adder_) + "}";
  }

  friend uint16_t Hash16(uint32_t,const UniHash16&);

 private:
  uint32_t randomMask_;
  uint32_t multiplier_;
  uint32_t adder_;
};

/*
 * Hash16
 */
uint16_t Hash16(uint32_t key, const UniHash16& parameters)
{
  uint64_t hash = key;

  //this prime number was retrieved from: 
  //https://primes.utm.edu/lists/small/small.html
  static const uint64_t big_prime = 5915587277; // > 2^32

  //int-modulated twice to make sure we don't overflow...
  hash ^= parameters.randomMask_;
  hash *= parameters.multiplier_;
  hash %= big_prime;
  hash += parameters.adder_;
  hash %= big_prime;

  return hash;
}

/*
 * Key -> uint64_t
 */
template<typename Key, 
         typename = std::enable_if_t<
           std::is_trivially_copyable<Key>::value>
         >
struct UniHash {
  static_assert(sizeof(Key) >= 4 && sizeof(Key) % 4 == 0);

  //Need at least four UniHash16 to generate a full hash
  static const size_t kNumParams = std::max(sizeof(Key)/2, (size_t)4);

  UniHash16 parameters[kNumParams]; //TODO make private

  UniHash() { 
    Refresh();
  }
  /*
   * Refresh
   */
  void Refresh() { for (auto& p : parameters) p.Refresh(); }
  /*
   * "BUG OF DEATH"
   * The key must be translated to zero-initialized memory before reading
   * its object representation for the hash function.  This is because not
   * fully-aligned structures may have noise that can affect the hash
   * function
   */
  /*
   * operator()
   */
  uint64_t 
  operator()(const Key& key) const
  {
    Key* fresh = (Key*)calloc(sizeof(key), 1);
    *fresh = key;

    uint64_t hash = 0;
    auto reg = (uint32_t*)fresh; //register
    auto acc = (uint16_t*)&hash;  //accumulator

    /*
     * If the Key type is large enough, simply run through its object,
     * collecting bits
     */
    if (sizeof(Key)/2 >= kNumParams)
    {
      for (size_t i = 0; i < sizeof(Key)/4; ++i)
      {
        auto acc_ = acc + i % 4;
        *acc_    ^= Hash16(*reg++, parameters[i % 4]);

        printf("\tHash: %zu\n", hash);
      }

    } else {

      for (size_t i = 0; i < kNumParams; ++i)
      {
        auto reg_ = reg + i % (sizeof(Key)/4);
        *acc++   ^= Hash16(*reg_, parameters[i]);

        printf("\thash: %zu\n", hash);
      }
    }

    free(fresh);

    return hash;
  }
  /*
   * ToString
   */
  std::string
  ToString() const 
  {
    std::string str = "{UniHas\t";
    for (auto&& p : parameters) str += p.ToString() + "\n\t";
    str.pop_back(); str.pop_back();
    return str + "}";
  }

};


}; //data_org_project_names 
