//null_test.h
#pragma once

/**NullTest is a dummy test
 */
class NullTest : public TestBase {
 public:
  NullTest() : TestBase("NullTest") {}
  void Run() override {
    printf("%11s: %p\n", "Run()", this);
    return;
  }
  ~NullTest() {
    //printf("%11s: %p\n", "~NullTest()", this);
  }
};

