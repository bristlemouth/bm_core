# Bristlemouth Common Unit Tests

This directory contains unit tests for the Bristlemouth Common Library.
The unit tests are built using CMake and Google Test.

## Building and Running Unit Tests

To setup, build, and run all tests, use the following commands from the root directory of the project:

  ```bash
  cmake -S. -Bbuild -G "Unix Makefiles" -DENABLE_TESTING=ON
  cd build
  make -j
  ctest
  ```
  This will create a build directory, compile the tests, and then run them.
  If you want more verbose output, you can use the following command:

  ```bash
   ctest -C --output-on-failure --verbose
  ```

  If you want to re-run the CMake command from within the build directory, you can use the following command:

  ```bash
  cmake -S.. -G "Unix Makefiles" -DENABLE_TESTING=ON
  ```


## Adding Tests
Create a new test file in the `test/src` directory.
Name your test such that it is the file being tested with `_test` appended to the end. For example, if you are testing `my_file_to_test.c`, name your test file `my_file_to_test_test.cpp`.

Reference the Google Test documentation for how to write tests using Google Test or use the existing tests as examples.


Once you have created your new testing file you need to add it to the end of the `CMakeLists.txt` file in this directory.
First, list any of the source files and supporting files that the test will require like this:

  ```cmake
  set(MY_FILE_TO_TEST_SOURCES
    # This is the file that will be tested
    ${CMAKE_CURRENT_SOURCE_DIR}/my_file_to_test.c

    # These are the supporting files
    ${SUPPORT_SOURCE_DIR}/support_file.c
    ${STUB_SOURCE_DIR}/stub_file.c
  )
  ```

  Then to create the test you can use `create_gtest`
  where the first argument is the name of the test and
  the second argument is the list of source files.
  The name of the test should be the name of the file being tested
  without the file extension. This name will be used to automatically find the
  test file in the test/src directory. For example, if the test file is named
  `my_file_to_test_test.cpp`, the name of the test should be `my_file_to_test`.

  For example:
  ```cmake
  create_gtest("my_file_to_test" "${MY_FILE_TO_TEST_SOURCES}")
  ```

  In total it will look like:
  ```cmake
  set(MY_FILE_TO_TEST_SOURCES
    # This is the file that will be tested
    ${SOURCE_DIR}/my_file_to_test.c

    # These are the supporting files
    ${SUPPORT_SOURCE_DIR}/support_file.c
    ${STUB_SOURCE_DIR}/stub_file.c
  )
  create_gtest("my_file_to_test" "${MY_FILE_TO_TEST_SOURCES}")
  ```