# Library Management System — C source + GoogleTest

This folder keeps the application source, data files, and GoogleTest unit-test source. Shell scripts and generated executables/reports are intentionally omitted; commands are documented below for manual execution.

## 1. Build the application (GCC)
Run these commands from this folder:

```bash
gcc -std=c11 -Wall -Wextra -Werror -pedantic -g \
  main.c auth.c books.c members.c storage.c transactions.c util.c \
  -o library
```

Run it with:

```bash
./library
```

## 2. Build and run GoogleTest
You need GCC, G++, and GoogleTest installed. The test suite uses POSIX functions and is intended for Linux-like environments.

Compile C modules as C object files. Exclude `main.c` to avoid linking the application's `main()` into the test executable:

```bash
mkdir -p build_gtest
```

Compile each C module to its own object file:

```bash
gcc -std=c11 -g -O0 --coverage -c auth.c -o build_gtest/auth.o
gcc -std=c11 -g -O0 --coverage -c books.c -o build_gtest/books.o
gcc -std=c11 -g -O0 --coverage -c members.c -o build_gtest/members.o
gcc -std=c11 -g -O0 --coverage -c storage.c -o build_gtest/storage.o
gcc -std=c11 -g -O0 --coverage -c transactions.c -o build_gtest/transactions.o
gcc -std=c11 -g -O0 --coverage -c util.c -o build_gtest/util.o

g++ -std=c++17 -g -O0 --coverage -I. \
  library_all_function_unit_tests.cpp build_gtest/*.o \
  -lgtest -lgtest_main -pthread -o build_gtest/library_tests

./build_gtest/library_tests
```

## 3. Measure coverage
After running the tests, use `gcovr` if installed:

```bash
gcovr --root . --filter '.*\\.c$' --print-summary
```

For a detailed HTML report:

```bash
gcovr --root . --filter '.*\\.c$' --html-details -o build_gtest/coverage.html
```

Coverage depends on the tests that pass and the lines/branches instrumented. The test file has 53 test declarations, but 90–100% coverage has not been measured or guaranteed. Review failed tests and add tests for uncovered lines and branches.

## 4. MISRA C review
Cppcheck's normal checks do not establish full MISRA C compliance. If Cppcheck and the approved MISRA addon/configuration are installed, run the command configured by your organization, for example:

```bash
cppcheck --enable=all --inconclusive --std=c11 --language=c \
  --addon=/path/to/approved/misra.json \
  main.c auth.c books.c members.c storage.c transactions.c util.c
```

Replace the addon path with the actual approved configuration. Review all findings against the MISRA C edition required by your organization and document approved deviations. This package is prepared for MISRA review; it is not certified as fully MISRA C compliant.

## Contents
- Application: `main.c`, `library.h`, `auth.c`, `books.c`, `members.c`, `storage.c`, `transactions.c`, `util.c`
- Data: `books.txt`, `members.txt`, `transactions.txt`
- Tests: `library_all_function_unit_tests.cpp`
