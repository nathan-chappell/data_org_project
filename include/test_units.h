//test_units.h
#pragma once

/*This file is intended to hold all the test-class implementations
 */

#include "test_unit.h"
#include "utility/storage_model.h"

/**HeaderArray
 * What to test:
 *
 * Optimal Alignment, max
 *
 * Array Operations:
 * Modifiers: insert, delete, size
 * Accessors: access, find
 */

const size_t kNumInts = 4;
const size_t kBlockSize = sizeof(Header) + kNumInts*sizeof(int) + 2;

/**Gets a block from the global storage manager and initializes it
 * like a header.  Useful for when only one node is needed.
 */
class TestBlock {
 public:
  TestBlock(storage_model* model) : model_(model) {
    Init();
  }

  void Init() {
    blockId_ = model_->create_block();
    block_ = model_->load_block(blockId_);
    InitializeHeader<int>((Header*)block_,kBlockSize,blockId_);
    PRINT((Header*)block_);
  } 

  PageId blockId() { return blockId_; }
  char* block() { return block_; }

  ~TestBlock() {
    model_->release_block(blockId_);
  }

 private:
  storage_model* model_;
  PageId blockId_;
  char* block_;
};

class HeaderArrayTest : public TestBase {
 public:
  HeaderArrayTest() : TestBase("HeaderArrayTest"), model_(kBlockSize),
                      testBlock_(&model_) {} 
  
  void Run() override {
    OptimalAlignment();
    Operations();
  }

  ~HeaderArrayTest() {}

 private:
  /**Check for alignment given by begin() and end(),
   * 
   * idea
   *  end() - begin() should be max()
   *  end() should line up at the end of the block as close as
   *  possible (with enough room for the past the end object)
   */
  void OptimalAlignment() {
    HeaderArray<int> intArray;
    intArray.header((Header*)testBlock_.block());
    cout << intArray.header()->ToString() << endl;
    char* blockEdge = testBlock_.block() + kBlockSize;

    PRINT(intArray.ArrayEnd()+1);
    PRINT(intArray.ArrayEnd()+2);
    PRINT((int*)blockEdge);
    TEST((char*)(intArray.end()+1) <= blockEdge &&
         blockEdge <= (char*)(intArray.end()+2));
  }

  /*
   *  insert back, front, middle (after a find), push_back
   */
  void Operations() {
    size_t curSize = 0;
    HeaderArray<int> intArray;
    intArray.header((Header*)testBlock_.block());
    std::vector<int> vInt = GetRandIntVec(kNumInts);
    std::sort(vInt.begin(), vInt.end());

    intArray.insert(intArray.end(), vInt[2]);

    TEST(intArray.size() == ++curSize);
    TEST(intArray[0] == vInt[2]);

    intArray.insert(intArray.begin(), vInt[0]);

    TEST(intArray[0] == vInt[0]);
    TEST(intArray[1] == vInt[2]);
    TEST(intArray.size() == ++curSize);

    auto rightPosition = intArray.find(
      [&vInt](const int& i) {
        return i >= vInt[2];
      });
    intArray.insert(rightPosition, vInt[1]);

    TEST(intArray[0] == vInt[0]);
    TEST(intArray[1] == vInt[1]);
    TEST(intArray[2] == vInt[2]);
    TEST(intArray.size() == ++curSize);

    intArray.push_back(vInt[3]);

    TEST(intArray[0] == vInt[0]);
    TEST(intArray[1] == vInt[1]);
    TEST(intArray[2] == vInt[2]);
    TEST(intArray[3] == vInt[3]);
    TEST(intArray.size() == ++curSize);

    auto cantFind = intArray.find([](const int& i) { return false; });
    intArray.erase(cantFind);

    TEST(intArray[0] == vInt[0]);
    TEST(intArray[1] == vInt[1]);
    TEST(intArray[2] == vInt[2]);
    TEST(intArray.size() == --curSize);

    intArray.erase(intArray.begin());

    TEST(intArray[0] == vInt[1]);
    TEST(intArray[1] == vInt[2]);
    TEST(intArray.size() == --curSize);

    //testing complete
  }

 private:
  unsafe_inmemory_storage model_;
  TestBlock testBlock_;
  //Statistics
  size_t successes_;
  size_t failures_;
};


