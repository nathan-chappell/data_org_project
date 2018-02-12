//larson_kalja_test.cc

#include "test_unit.h"
//

#include "larson_kalja.h"

#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
//

static std::FILE* errFile = fopen("error_file", "w");
static std::FILE* tableFile = fopen("table_file", "w");

using Key      = size_t;
using Data     = size_t;
using PageEntry = LkPageEntry<Key, Data>;
using Verifier = std::unordered_map<Key,Data>;

/*
 * PageSize
 */
constexpr size_t 
PageSize(size_t entriesPerPage) 
{
  return sizeof(LkPageHeader) + entriesPerPage * sizeof(PageEntry);
}

size_t numInsertions = 5;

auto RandSize = std::bind(std::uniform_int_distribution<size_t>(), 
                          std::default_random_engine());
struct RandomPair {
  size_t key, pageId;

  RandomPair() : 
    key(RandSize()), pageId(RandSize()) {}

  RandomPair(size_t maxSize) : 
    key(RandSize() % maxSize), pageId(RandSize() % maxSize) {}
};

/*
 * Verify
 */
bool
Verify(const Verifier& cVerifier,
       const LkTable<Key, Data>&  lkTable)
{
  bool testResult = true;

  for (const auto& correctEntry : cVerifier) {

    auto searchResult = lkTable.Find(correctEntry.first);

    if (!searchResult.first)
    {
      fprintf(errFile, "Couldn't find: %zu\n", correctEntry.first);
      testResult = false;

    } else if (searchResult.second.key  != correctEntry.first ||
               searchResult.second.data != correctEntry.second)
    {
      fprintf(errFile, "wrong data: %s\n", 
                        searchResult.second.ToString().c_str());
      fprintf(errFile, "should be: key=%zu, data=%zu\n", 
                        correctEntry.first, correctEntry.second);
                        
      testResult = false;
    }
  }

  return testResult;
} //Verify

/*
 * VerifyInsert
 */
void VerifyInsert(const std::vector<RandomPair>& randPairVec,
                  Verifier& verifier,
                  LkTable<size_t, size_t> lkTable)
{
  size_t iCount = 0;

  for (auto rp : randPairVec) 
  {
    verifier[rp.key] = rp.pageId;
    lkTable.Insert(rp.key, rp.pageId);
    ++iCount;

    if (!Verify(verifier, lkTable)) 
    {
      fprintf(errFile, "Insertion error after: %zu, printing table\n", iCount);

      std::string table = lkTable.ToString();
      fwrite(table.c_str(), 1, table.size(), tableFile);
    }
  }
}


/*
 * VerifyErase
 */
void VerifyErase(Verifier& verifier,
                 LkTable<size_t, size_t> lkTable)
{
  size_t eCount = 0;

  for (auto v : verifier) {
    
    lkTable.Erase(v.first);
    ++eCount;

    if (!Verify(verifier, lkTable)) {

      fprintf(errFile, "Erase error after: %zu, printing table\n", eCount);

      std::string table = lkTable.ToString();
      fwrite(table.c_str(), 1, table.size(), tableFile);
    }
  }
}


class LkTableTest : public TestBase {
 public:
  LkTableTest(size_t entriesPerPage,
              size_t numPages,
              size_t numInsertions) : 
    TestBase("LkTableTest"),
    model_(PageSize(entriesPerPage)),
    numPages_(numPages),
    numInsertions_(numInsertions) {}

  /*
   * Signature
   */
  void Run() override
  {
    LkTable<size_t, size_t> lkTable(&model_, numPages_);

    std::vector<RandomPair> randPairVec(numInsertions_);

    printf("pages: %zu, insertions: %zu\n", numPages_, numInsertions_);
    printf("Inserting %zu entries\n", numInsertions_);

    VerifyInsert(randPairVec, verifier_, lkTable);
    printf("Erasing %zu entries\n", numInsertions_);
    VerifyErase(verifier_, lkTable);
  }

 private:
  unsafe_inmemory_storage model_;

  size_t   successes_;
  size_t   failures_;
  Verifier verifier_;
  size_t   numPages_;
  size_t   numInsertions_;
};

size_t maxPages = 0x40;
size_t maxEpp = 0x8;

/*
 * TestSequence
 */
void TestSequence(TestSuite& testSuite)
{
  for (size_t pages = 4; pages <= maxPages; pages *= 4) 
  {
    for (size_t epp = 8; epp <= 128; epp *= 4) //epp == entries per page
    { 
      testSuite.RegisterTest<LkTableTest>(epp, pages, epp*pages*9/10);
    }
  }
}

int main(int argc, char** argv) {
  if (argc > 1) maxPages = std::stoull(argv[1]);
  if (argc > 2) maxEpp   = std::stoull(argv[2]);
  TestSuite testSuite;

  TestSequence(testSuite);
  testSuite.Run();
}
