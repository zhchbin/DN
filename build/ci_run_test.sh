#!/bin/bash

cd out/Debug

exit_code=0

RunTest() {
  $1
  unittest_ret=$?
  if [ $unittest_ret -ne 0 ]; then
    exit_code=$unittest_ret
  fi
}

RunTest ./lib_foo_unittest 

exit $exit_code
