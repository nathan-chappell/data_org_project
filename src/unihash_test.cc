//unihash_test.cc

#include "test_unit.h"
//

#include "universal_hash.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
//

static std::FILE* errFile = fopen("error_file", "w");
static std::FILE* tableFile = fopen("table_file", "w");

using namespace data_org_project_names;

/*
 * Get a "sample"
 * param: buckets
 * count to buckets
 * convert to pmf
 */

using Histogram = std::vector<double>;
using Observations = std::vector<size_t>;

size_t sampleSize = 100;

template<typename Hash>
struct Sample {

  Hash hash;
  void TakeSample(size_t n)
  {
    observations.clear();
    for (size_t i = 0; i < n; ++i) observations.push_back(hash(i));
    for (size_t i = 0; i < n; ++i) printf("sampled: %zu\n", observations[i]);
    hash.Refresh();
  }

  size_t size() const { return observations.size(); }

  Observations observations;
};

struct Stats {
  size_t n;
  Histogram pmf, mass;

  Stats(size_t n = 10) : n(n), pmf(n), mass(n) {}

  void GetStats(const Observations& sample, size_t range = SIZE_MAX)
  {
    std::fill(pmf.begin(), pmf.end(), 0);
    std::fill(mass.begin(), mass.end(), 0);

    for (auto&& datum : sample) {
      int index = rint(floor(
            ((datum % range) / (double)range) * n)
      );
      printf("datum: %zu, index: %d, range: %zu\n", 
              datum, index, range);
      ++mass[index];
    }

    for (size_t i = 0; i < n; ++i) pmf[i] = mass[i]/(double)sample.size();
  }

  void PrintStats()
  {
    printf("mass:|");
    for (auto&& m : mass) printf(" %.2f |", m);
    printf("|\npmf: |");
    for (auto&& p : pmf) printf(" %.2f |",  p);
    printf("|\n");
  }
};

template<typename Hash>
void TestDist(size_t range)
{
  Stats stats;
  Sample<Hash> sample;

  for (size_t i = 0; i < 3; ++i) 
  {
    sample.TakeSample(sampleSize);
    stats.GetStats(sample.observations, range);
    stats.PrintStats();
    printf("\n\n");
  } 
}

auto RandChar = std::bind(
    std::geometric_distribution<char>(.8), 
    std::default_random_engine());

struct MyKey {
  MyKey() {for (auto& c : key) c = (RandChar() % 26 + 'a'); key[15] = 0; }
  MyKey(size_t) {
    for (auto& c : key) {
      c = (RandChar() % 26 + 'a'); 
      key[15] = 0; 
    }
    k = RandChar();
    j = k*k*k;
    f = '7';
  }
  int k;
  size_t j;
  char f;
  char key[16];
};

int main(int argc, char** argv) {
  if (argc > 1) sampleSize = std::stoull(argv[1]);
 
  printf("samples: %zu\n", sampleSize);

  TestDist<UniHash<MyKey>>(0x1000);

  //UniHash<size_t> uni;
  //printf("params:\n%s\n", uni.ToString().c_str());
  //for (size_t i = 0; i < 5; ++i) printf("i: %zu, final hash: %zu\n", i, uni(i));
}
