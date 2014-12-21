#include <iostream>

#include "foo.h"

#define UNUSED(identifier)

int main(int UNUSED(argc), char** UNUSED(argv)) {
  std::cout << Return5() << std::endl;
  return 0;
}
