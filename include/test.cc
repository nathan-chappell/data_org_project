//test.cc

#include "btree.h"
#include "btree_storage_model.h"
#include "fagin.h"
#include "hash_interface.h"
#include "header_array.h"
#include "larson_kalja.h"
#include "storage_model.h"
#include "universal_hash.h"

#include "test_unit.h"

#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
//

using namespace data_org_project_names;

static std::FILE* errFile = fopen("error_file", "w");
static std::FILE* tableFile = fopen("table_file", "w");

using Key  = size_t;
using Data = size_t;

using PageEntry = Entry<Key, Data>;
using Verifier = std::unordered_map<Key,Data>;

/*
 * PageSize
 */
template<typename Header>
constexpr size_t 
PageSize(size_t entriesPerPage) 
{
  return sizeof(Header) + entriesPerPage * sizeof(PageEntry);
}

size_t numInsertions = 5;

auto RandSize = std::bind(std::uniform_int_distribution<size_t>(), 
                          std::default_random_engine());

struct RandomPair {
  size_t key, pageId;

  RandomPair() : 
    key(RandSize()), pageId(RandSize()) {}

  RandomPair(size_t max_size) : 
    key(RandSize() % max_size), pageId(RandSize() % max_size) {}
};
using RInput = std::vector<RandomPair>;
/*
 * Verify
 */
template<typename Table>
bool Verify(const Verifier& verifier, const Table& table)
{
  bool testResult = true;

  for (const auto& correctEntry : verifier) {

    auto searchResult = table.find(correctEntry.first);

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
template<typename Table>
void VerifyInsert(Verifier& verifier, Table& table, const RInput& randPairVec)
{
  size_t iCount = 0;

  for (auto rp : randPairVec) 
  {
    verifier[rp.key] = rp.pageId;
    table.insert(rp.key, rp.pageId);
    ++iCount;

    if (!Verify(verifier, table)) 
    {
      fprintf(
          errFile, 
          "Insertion error after: %zu, printing table\n", 
          iCount
      );
    }
  }
}
/*
 * VerifyErase
 */
template<typename Table>
void VerifyErase(Verifier& verifier,  Table& table)
{
  size_t eCount = 0;

  for (auto v : verifier) {
    
    table.erase(v.first);
    ++eCount;

    if (!Verify(verifier, table)) {

      fprintf(
          errFile,
          "Erase error after: %zu, printing table\n",
          eCount
      );
    }
  }
}
template<typename Table>
class TableTest : public TestBase {
 public:
  TableTest(size_t entriesPerPage,
              size_t numPages,
              size_t numInsertions) : 
    TestBase("TableTest"),
    model_(PageSize<typename Table::Header>(entriesPerPage)),
    numPages_(numPages),
    numInsertions_(numInsertions) {}

  /*
   * Signature
   */
  void Run() override
  {
    Table table(&model_, numPages_);

    std::vector<RandomPair> randPairVec(numInsertions_);

    printf("pages: %zu, insertions: %zu\n", numPages_, numInsertions_);
    printf("Inserting %zu entries\n", numInsertions_);

    VerifyInsert(verifier_, table, randPairVec);
    printf("Erasing %zu entries\n", numInsertions_);
    VerifyErase(verifier_, table);
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
template<typename Table>
void TestSequence(TestSuite& testSuite)
{
  for (size_t pages = 4; pages <= maxPages; pages *= 4) 
  { 
    //epp == "entries per page"
    for (size_t epp = 8; epp <= 128; epp *= 4)
    { 
      testSuite.RegisterTest<TableTest<Table>>(epp, pages, epp*pages*9/10);
    }
  }
}
  
int main(int argc, char** argv) {
  if (argc > 1) maxPages = std::stoull(argv[1]);
  if (argc > 2) maxEpp   = std::stoull(argv[2]);

  TestSuite lkSuite;
  TestSequence<LkTable<Key, Data>>(lkSuite);

  TestSuite btreeSuite;
  TestSequence<Btree<Key, Data>>(btreeSuite);

  TestSuite faginSuite;
  TestSequence<FaginTable<Key, Data>>(faginSuite);

  //testSuite.Run();
}
