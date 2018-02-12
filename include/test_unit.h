//test_unit.h
#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

using std::cout;
using std::endl;
using std::list;
using std::string;

/**This file defines some basic testing infrastructure.
 */

using PageId = size_t;

#define TEST(EXPRESSION) \
  if (EXPRESSION) { \
    ++successes_;\
  } else { \
    printf("%30s : %s\n", #EXPRESSION, "failure");\
    ++failures_;\
  }

#define PRINT(EXPRESSION) \
  cout << #EXPRESSION << ": " << EXPRESSION << endl;

auto RandInt = std::bind(std::uniform_int_distribution<int>(0,100),
                          std::default_random_engine());

std::vector<int> GetRandIntVec(size_t size) {
  std::vector<int> v;
  for (size_t i = 0; i < size; ++i) v.push_back(RandInt());
  return v;
}


/**TestBase is an abstract base for Test instances;
 */
class TestBase {
 public:
  TestBase(const string& testName) : testName_(testName) {}
  string testName() const { return testName_; }
  virtual void Run() = 0;
  virtual ~TestBase() {};

 private:
  string testName_;
};

/**Output some global test statistical information
 * time, count, whatever
 */
void GlobalBanner(TestBase* testPtr) {
  static size_t count = 0;
  printf("%8s: %s\n", "Test", testPtr->testName().c_str());
  //printf("%8s: %s\n", "Time", GetStrTime());
  printf("%8s: %zu\n", "Count", ++count);
}

class TestSuite {
 public:
  using TestPtr = std::unique_ptr<TestBase>;

  template<typename TestClass, class... Args>
  void RegisterTest(Args&&... args) { 
    testList_.emplace_back(new TestClass(args...)); 
  }

  void Run() {
    cout << "Test Suite: " << testList_.size() << " tests to run." << endl;
    for (auto testPtrIt = testList_.begin(); 
        testPtrIt != testList_.end(); ++testPtrIt) {
      GlobalBanner(testPtrIt->get());
      try {
        (*testPtrIt)->Run();
        printf("Test Complete.\n");
      }
      catch(...) {
        printf("Error thrown during test execution.\n");
      }
    }
    cout << "Test Suite Complete." << endl;
  }
 private:
  list<TestPtr> testList_;
};
