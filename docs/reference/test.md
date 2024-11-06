# Unit Testing

Unit tests have been created to test the modules within `bm_core`.
The unit test framework chosen is [GoogleTest](https://google.github.io/googletest/).
If contributing and creating a new module,
it is highly recommended to also create a unit test to go along with it.

## Requirements

In order to run the unit tests,
the host must have a C/C++ compiler (GCC, clang).

## Building and Running Unit Tests

To setup, build, and run all tests, use the following commands from the root directory of the project:

```
cmake -S. -Bbuild -G "Unix Makefiles" -DENABLE_TESTING=ON
cd build
make -j
ctest
```
This will create a build directory, compile the tests, and then run them.
It is imperative that the `-DENABLE_TESTING=ON` flag is called when initially invoking the initial CMake command,
as that is what allows the tests to be built.
If you want more verbose output, you can use the following command:

```
 ctest -C --output-on-failure --verbose
```

Individual module tests can be ran from the build directory by running the following command after compiling the tests (invoking the raw executable):

```
./test/{module_to_test}
```

If you want to re-run the CMake command from within the build directory, you can use the following command:

```
cmake -S.. -G "Unix Makefiles" -DENABLE_TESTING=ON
```

## Adding Tests
Create a new test file in the `test/src` directory.
Name your test such that it is the file being tested with `_test` appended to the end. For example, if you are testing `my_file_to_test.c`, name your test file `my_file_to_test_test.cpp`.

Reference the Google Test documentation for how to write tests using Google Test or use the existing tests as examples.
When writing tests in your `my_file_to_test_test.cpp` the chosen style is to use UpperCamelCase for the test suite name and lower_snake_case for the test name.

Example:

```
TEST_F(MyFileToTest, my_test_name) {
  // Test code here
}
```


Once you have created your new testing file you need to add it to the end of the `CMakeLists.txt` file in the `test` directory.
First, list any of the source files and supporting files that the test will require like this:

```
set(MY_FILE_TO_TEST_SOURCES
  # This is the file that will be tested
  ${CMAKE_CURRENT_SOURCE_DIR}/my_file_to_test.c

  # These are the supporting files
  ${SUPPORT_SOURCE_DIR}/support_file.c
  ${STUB_SOURCE_DIR}/stub_file.c
)
```

Then to create the test you can use `create_gtest` function,
where the first argument is the name of the test and
the second argument is the list of source files.
The name of the test should be the name of the file being tested without the file extension.
This name will be used to automatically find the
test file in the test/src directory.
For example, if the test file is named `my_file_to_test_test.cpp`,
the name of the test should be `my_file_to_test`.

For example:
```
create_gtest("my_file_to_test" "${MY_FILE_TO_TEST_SOURCES}")
```

In total it will look like:
```
set(MY_FILE_TO_TEST_SOURCES
  # This is the file that will be tested
  ${SOURCE_DIR}/my_file_to_test.c

  # These are the supporting files
  ${SUPPORT_SOURCE_DIR}/support_file.c
  ${STUB_SOURCE_DIR}/stub_file.c
)
create_gtest("my_file_to_test" "${MY_FILE_TO_TEST_SOURCES}")
```
