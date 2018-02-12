//test_unit.cc

#include "test_unit.h"
//#include "test_units.h"
//

#include "larson_kalja.h"

#include <cstdio>
#include <vector>
//



const size_t kEntriesPerPage = 10;
const size_t kBlockSize = sizeof(LkPageHeader) + kEntriesPerPage*sizeof(DirEntry);
const size_t kMaxRand = 300;


auto RandSize = std::bind(std::uniform_int_distribution<size_t>(0,kMaxRand), 
                          std::default_random_engine());
struct RandomPair {
  size_t key, blockId;
  RandomPair() : key(RandSize()), blockId(RandSize()) {}
};

//returns the final "inserted" pair, given a key
BlockId GetCorrectBlockId(const std::vector<RandomPair>& rpVec, size_t key) {
  auto rp = rpVec.end();
  do {
    --rp;
    if (rp->key == key) return rp->blockId;
  } while (rp != rpVec.begin());
  return -1;
}

class LkTableTest : public TestBase {
 public:
  LkTableTest() : TestBase("LkTableTest"), model_(200) {}
  /**Gameplan:
   * make a small table, insert a bunch of things, see if the stuff
   * that was inserted is still there.
  */
  void InsertTest() {
    //lkTable has 8 pages * 10 entries per page, try inserting 60
    //pairs
    const size_t kNumInsertions = 60;
    LkTable lkTable(&model_, 8, 100);
    printf("Inserting %zu entries", kNumInsertions);
    std::vector<RandomPair> randPairVec(kNumInsertions);
    for (auto rp : randPairVec) {
      lkTable.Insert(rp.key, rp.blockId);
    }
    for (auto rp : randPairVec) {
      TEST(lkTable.Find(rp.key).second.blockId == 
          GetCorrectBlockId(randPairVec, rp.key));
    }
  }

  void Run() override {
    InsertTest();
  }

 private:
  unsafe_inmemory_storage model_;
  size_t successes_;
  size_t failures_;
};


int main() {
  TestSuite testSuite;

  testSuite.RegisterTest<LkTableTest>();
  testSuite.Run();
}
