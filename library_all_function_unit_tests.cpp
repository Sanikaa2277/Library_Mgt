/*
 * GoogleTest unit-test suite for the uploaded Library Management System.
 *
 * Run this test executable from a disposable copy of the project.
 * The fixture changes to a fresh temporary directory per test, so text-file
 * writes do not overwrite the project's real books.txt/members.txt/transactions.txt.
 *
 * Build C files as C object files, then link them with this C++ file and
 * GoogleTest. EXCLUDE main.c to avoid a duplicate main().
 *
 * Linux / GitHub Codespaces test harness (uses POSIX stdin redirection).
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

extern "C" {
#include "library.h"
}

static std::string g_temp_dir;
static char g_original_dir[PATH_MAX];

class LibraryTestEnvironment : public ::testing::Test {
protected:
    Library lib{};

    void SetUp() override {
        ASSERT_NE(getcwd(g_original_dir, sizeof(g_original_dir)), nullptr);
        char pattern[] = "/tmp/library_gtest_XXXXXX";
        char *dir = mkdtemp(pattern);
        ASSERT_NE(dir, nullptr);
        g_temp_dir = dir;
        ASSERT_EQ(chdir(g_temp_dir.c_str()), 0);
        std::memset(&lib, 0, sizeof(lib));
        std::strncpy(lib.username, "testuser", sizeof(lib.username) - 1);
        lib.role = ROLE_ADMIN;
    }

    void TearDown() override {
        free_library(&lib);
        (void)chdir(g_original_dir);
        if (!g_temp_dir.empty()) {
            std::string cmd = "rm -rf \"" + g_temp_dir + "\"";
            (void)std::system(cmd.c_str());
            g_temp_dir.clear();
        }
    }

    void SetInput(const std::string &input) {
        FILE *f = std::tmpfile();
        ASSERT_NE(f, nullptr);
        ASSERT_EQ(std::fwrite(input.data(), 1, input.size(), f), input.size());
        std::rewind(f);
        ASSERT_NE(dup2(fileno(f), STDIN_FILENO), -1);
        std::fclose(f);
        clearerr(stdin);
    }

    void RestoreInput() {
        // Most tests consume all supplied input; stdin is reset before each test
        // by the test runner process. Do not call further interactive functions
        // after the input stream has been exhausted.
    }

    void AddBook(int id = 101, const char *title = "Wings Of Fire",
                 int total = 2, int available = 2) {
        ASSERT_EQ(ensure_book_capacity(&lib, lib.book_count + 1), 1);
        Book &b = lib.books[lib.book_count++];
        std::memset(&b, 0, sizeof(b));
        b.id = id;
        std::strncpy(b.title, title, sizeof(b.title) - 1);
        std::strncpy(b.author, "A P J Abdul Kalam", sizeof(b.author) - 1);
        std::strncpy(b.category, "Biography", sizeof(b.category) - 1);
        b.total_copies = total;
        b.available_copies = available;
    }

    void AddMember(int id = 201) {
        ASSERT_EQ(ensure_member_capacity(&lib, lib.member_count + 1), 1);
        Member &m = lib.members[lib.member_count++];
        std::memset(&m, 0, sizeof(m));
        m.id = id;
        std::strncpy(m.name, "Rahul Patil", sizeof(m.name) - 1);
        std::strncpy(m.phone, "9876543210", sizeof(m.phone) - 1);
    }

    void AddTransaction(int id = 1, int book_id = 101, int member_id = 201,
                        int returned = 0) {
        ASSERT_EQ(ensure_transaction_capacity(&lib, lib.transaction_count + 1), 1);
        Transaction &t = lib.transactions[lib.transaction_count++];
        std::memset(&t, 0, sizeof(t));
        t.id = id;
        t.book_id = book_id;
        t.member_id = member_id;
        std::strcpy(t.issue_date, "2026-10-19");
        std::strcpy(t.due_date, "2026-10-29");
        std::strcpy(t.return_date, returned ? "2026-10-30" : "-");
        t.returned = returned;
        t.fine = returned ? 5.0 : 0.0;
    }
};

/* ------------------------------ util.c ------------------------------ */

TEST_F(LibraryTestEnvironment, OutputHelpersAcceptNormalOutput) {
    print_line("unit test output");
    print_format("value=%d\n", 42);
    EXPECT_EQ(output_failed(), 0);
}

TEST_F(LibraryTestEnvironment, ValidDateAcceptsCorrectDate) {
    EXPECT_EQ(valid_date("2026-11-02"), 1);
}
TEST_F(LibraryTestEnvironment, ValidDateRejectsBadFormat) {
    EXPECT_EQ(valid_date("2026-11-2"), 0);
    EXPECT_EQ(valid_date("20261102"), 0);
    EXPECT_EQ(valid_date(nullptr), 0);
}
TEST_F(LibraryTestEnvironment, ValidDateRejectsImpossibleDates) {
    EXPECT_EQ(valid_date("2026-02-30"), 0);
    EXPECT_EQ(valid_date("2026-13-01"), 0);
    EXPECT_EQ(valid_date("2026-00-01"), 0);
}
TEST_F(LibraryTestEnvironment, ValidDateLeapYears) {
    EXPECT_EQ(valid_date("2028-02-29"), 1);
    EXPECT_EQ(valid_date("2027-02-29"), 0);
}
TEST_F(LibraryTestEnvironment, TextFieldValidation) {
    EXPECT_EQ(text_field_valid("Wings Of Fire", 100), 1);
    EXPECT_EQ(text_field_valid("", 100), 0);
    EXPECT_EQ(text_field_valid(nullptr, 100), 0);
    EXPECT_EQ(text_field_valid("bad|field", 100), 0);
    EXPECT_EQ(text_field_valid("12345", 5), 0);
}
TEST_F(LibraryTestEnvironment, CaseInsensitiveSearch) {
    EXPECT_EQ(contains_ignore_case("Wings Of Fire", "wINGS"), 1);
    EXPECT_EQ(contains_ignore_case("Wings Of Fire", "Java"), 0);
    EXPECT_EQ(contains_ignore_case("anything", ""), 1);
    EXPECT_EQ(contains_ignore_case(nullptr, "x"), 0);
    EXPECT_EQ(contains_ignore_case("abc", nullptr), 0);
}
TEST_F(LibraryTestEnvironment, DateDifferenceAndOkFlag) {
    int ok = 0;
    EXPECT_EQ(days_between_dates("2026-10-29", "2026-11-02", &ok), 4);
    EXPECT_EQ(ok, 1);
    EXPECT_EQ(days_between_dates("2026-11-02", "2026-10-29", &ok), -4);
    EXPECT_EQ(ok, 1);
    EXPECT_EQ(days_between_dates("bad", "2026-11-02", &ok), 0);
    EXPECT_EQ(ok, 0);
    EXPECT_EQ(days_between_dates("2026-01-01", "2026-01-02", nullptr), 0);
}
TEST_F(LibraryTestEnvironment, TodayUsesIsoDateFormat) {
    char today[DATE_LEN] = {};
    get_today(today);
    ASSERT_EQ(std::strlen(today), 10U);
    EXPECT_EQ(today[4], '-');
    EXPECT_EQ(today[7], '-');
    EXPECT_EQ(valid_date(today), 1);
}
TEST_F(LibraryTestEnvironment, RoleNames) {
    EXPECT_STREQ(role_name(ROLE_ADMIN), "Administrator");
    EXPECT_STREQ(role_name(ROLE_LIBRARIAN), "Librarian");
    EXPECT_STREQ(role_name(ROLE_NONE), "Unauthenticated");
}
TEST_F(LibraryTestEnvironment, MemoryCapacityAndFree) {
    ASSERT_EQ(ensure_book_capacity(&lib, 1), 1);
    ASSERT_EQ(ensure_member_capacity(&lib, 1), 1);
    ASSERT_EQ(ensure_transaction_capacity(&lib, 1), 1);
    EXPECT_GE(lib.book_capacity, 1U);
    EXPECT_GE(lib.member_capacity, 1U);
    EXPECT_GE(lib.transaction_capacity, 1U);
    free_library(&lib);
    EXPECT_EQ(lib.books, nullptr);
    EXPECT_EQ(lib.members, nullptr);
    EXPECT_EQ(lib.transactions, nullptr);
    EXPECT_EQ(lib.book_count, 0U);
    EXPECT_EQ(lib.member_count, 0U);
    EXPECT_EQ(lib.transaction_count, 0U);
}
TEST_F(LibraryTestEnvironment, FreeLibraryAcceptsNull) {
    free_library(nullptr);
}
TEST_F(LibraryTestEnvironment, ConfirmActionYesAndNo) {
    SetInput("y\n");
    EXPECT_EQ(confirm_action("Confirm? "), 1);
    SetInput("n\n");
    EXPECT_EQ(confirm_action("Confirm? "), 0);
}
TEST_F(LibraryTestEnvironment, ReadLineReadsText) {
    char buffer[32] = {};
    SetInput("hello world\n");
    read_line("Text: ", buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "hello world");
}
TEST_F(LibraryTestEnvironment, ReadIntRetriesUntilValid) {
    SetInput("abc\n5\n");
    EXPECT_EQ(read_int("Number: ", 1, 10), 5);
}
TEST_F(LibraryTestEnvironment, LogEventCreatesLog) {
    log_event("tester", "test-event");
    std::ifstream in(LOG_FILE);
    ASSERT_TRUE(in.good());
    std::stringstream contents;
    contents << in.rdbuf();
    EXPECT_NE(contents.str().find("user=tester"), std::string::npos);
    EXPECT_NE(contents.str().find("event=test-event"), std::string::npos);
}

/* ----------------------------- auth.c -------------------------------- */

TEST_F(LibraryTestEnvironment, AdminAuthenticationSucceeds) {
    char username[32] = {};
    SetInput("admin\nAdmin@123\n");
    EXPECT_EQ(authenticate_user(username), ROLE_ADMIN);
    EXPECT_STREQ(username, "admin");
}
TEST_F(LibraryTestEnvironment, LibrarianAuthenticationSucceeds) {
    char username[32] = {};
    SetInput("librarian\nLibrarian@123\n");
    EXPECT_EQ(authenticate_user(username), ROLE_LIBRARIAN);
    EXPECT_STREQ(username, "librarian");
}
TEST_F(LibraryTestEnvironment, AuthenticationFailsAfterThreeAttempts) {
    char username[32] = {};
    SetInput("wrong\nbad\nwrong\nbad\nwrong\nbad\n");
    EXPECT_EQ(authenticate_user(username), ROLE_NONE);
}

/* ----------------------------- storage.c ----------------------------- */

TEST_F(LibraryTestEnvironment, SavesAndLoadsRecords) {
    AddBook();
    AddMember();
    AddTransaction();
    ASSERT_EQ(save_books(&lib), 1);
    ASSERT_EQ(save_members(&lib), 1);
    ASSERT_EQ(save_transactions(&lib), 1);

    Library loaded{};
    ASSERT_EQ(load_data(&loaded), 1);
    ASSERT_EQ(loaded.book_count, 1U);
    ASSERT_EQ(loaded.member_count, 1U);
    ASSERT_EQ(loaded.transaction_count, 1U);
    EXPECT_EQ(loaded.books[0].id, 101);
    EXPECT_EQ(loaded.members[0].id, 201);
    EXPECT_EQ(loaded.transactions[0].id, 1);
    free_library(&loaded);
}
TEST_F(LibraryTestEnvironment, MissingDataFilesLoadAsEmpty) {
    EXPECT_EQ(load_data(&lib), 1);
    EXPECT_EQ(lib.book_count, 0U);
    EXPECT_EQ(lib.member_count, 0U);
    EXPECT_EQ(lib.transaction_count, 0U);
}
TEST_F(LibraryTestEnvironment, MalformedBookFileIsRejected) {
    std::ofstream out(BOOKS_FILE);
    out << "bad record\n";
    out.close();
    EXPECT_EQ(load_data(&lib), 0);
}
TEST_F(LibraryTestEnvironment, MalformedMemberFileIsRejected) {
    std::ofstream out(MEMBERS_FILE);
    out << "not|enough\n";
    out.close();
    EXPECT_EQ(load_data(&lib), 0);
}
TEST_F(LibraryTestEnvironment, MalformedTransactionFileIsRejected) {
    std::ofstream out(TRANSACTIONS_FILE);
    out << "bad transaction\n";
    out.close();
    EXPECT_EQ(load_data(&lib), 0);
}
TEST_F(LibraryTestEnvironment, NullLibraryLoadFails) {
    EXPECT_EQ(load_data(nullptr), 0);
}

/* ------------------------------ books.c ------------------------------ */

TEST_F(LibraryTestEnvironment, AddBookStoresValidBook) {
    SetInput("102\nClean Code\nRobert Martin\nProgramming\n3\n");
    add_book(&lib);
    ASSERT_EQ(lib.book_count, 1U);
    EXPECT_EQ(lib.books[0].id, 102);
    EXPECT_STREQ(lib.books[0].title, "Clean Code");
    EXPECT_EQ(lib.books[0].total_copies, 3);
    EXPECT_EQ(lib.books[0].available_copies, 3);
}
TEST_F(LibraryTestEnvironment, AddBookRejectsDuplicateId) {
    AddBook(101);
    SetInput("101\n");
    add_book(&lib);
    EXPECT_EQ(lib.book_count, 1U);
}
TEST_F(LibraryTestEnvironment, AddBookRejectsInvalidField) {
    SetInput("102\nBad|Title\n");
    add_book(&lib);
    EXPECT_EQ(lib.book_count, 0U);
}
TEST_F(LibraryTestEnvironment, ListBooksHandlesEmptyAndPopulated) {
    list_books(&lib);
    AddBook();
    list_books(&lib);
}
TEST_F(LibraryTestEnvironment, SearchBooksHandlesEmptyFoundAndMissing) {
    search_books(&lib);
    AddBook();
    SetInput("wings\n");
    search_books(&lib);
    SetInput("missing title\n");
    search_books(&lib);
    SetInput("101\n");
    search_books(&lib);
}
TEST_F(LibraryTestEnvironment, UpdateBookHandlesMissingAndSuccess) {
    SetInput("999\n");
    update_book(&lib);
    AddBook();
    SetInput("101\nNew Title\nNew Author\nNew Category\n4\n");
    update_book(&lib);
    EXPECT_STREQ(lib.books[0].title, "New Title");
    EXPECT_EQ(lib.books[0].total_copies, 4);
}
TEST_F(LibraryTestEnvironment, UpdateBookRejectsTotalBelowIssuedCopies) {
    AddBook(101, "Book", 3, 1);
    SetInput("101\nNew Title\nNew Author\nNew Category\n1\n");
    update_book(&lib);
    EXPECT_EQ(lib.books[0].total_copies, 3);
}
TEST_F(LibraryTestEnvironment, DeleteBookHandlesMissingCancelAndSuccess) {
    SetInput("999\n");
    delete_book(&lib);
    AddBook();
    SetInput("101\nn\n");
    delete_book(&lib);
    EXPECT_EQ(lib.book_count, 1U);
    SetInput("101\ny\n");
    delete_book(&lib);
    EXPECT_EQ(lib.book_count, 0U);
}
TEST_F(LibraryTestEnvironment, DeleteBookRejectsActiveTransaction) {
    AddBook();
    AddMember();
    AddTransaction();
    SetInput("101\n");
    delete_book(&lib);
    EXPECT_EQ(lib.book_count, 1U);
}

/* ----------------------------- members.c ----------------------------- */

TEST_F(LibraryTestEnvironment, RegisterMemberStoresValidMember) {
    SetInput("202\nMaya Shah\n9876543210\n");
    register_member(&lib);
    ASSERT_EQ(lib.member_count, 1U);
    EXPECT_EQ(lib.members[0].id, 202);
    EXPECT_STREQ(lib.members[0].name, "Maya Shah");
}
TEST_F(LibraryTestEnvironment, RegisterMemberRejectsDuplicateId) {
    AddMember(201);
    SetInput("201\n");
    register_member(&lib);
    EXPECT_EQ(lib.member_count, 1U);
}
TEST_F(LibraryTestEnvironment, RegisterMemberRejectsInvalidField) {
    SetInput("202\nBad|Name\n");
    register_member(&lib);
    EXPECT_EQ(lib.member_count, 0U);
}
TEST_F(LibraryTestEnvironment, ListMembersHandlesEmptyAndPopulated) {
    list_members(&lib);
    AddMember();
    list_members(&lib);
}
TEST_F(LibraryTestEnvironment, SearchMembersHandlesEmptyFoundAndMissing) {
    search_members(&lib);
    AddMember();
    SetInput("201\n");
    search_members(&lib);
    SetInput("Rahul\n");
    search_members(&lib);
    SetInput("missing\n");
    search_members(&lib);
}
TEST_F(LibraryTestEnvironment, UpdateMemberHandlesMissingAndSuccess) {
    SetInput("999\n");
    update_member(&lib);
    AddMember();
    SetInput("201\nMaya Shah\n9123456789\n");
    update_member(&lib);
    EXPECT_STREQ(lib.members[0].name, "Maya Shah");
    EXPECT_STREQ(lib.members[0].phone, "9123456789");
}
TEST_F(LibraryTestEnvironment, DeleteMemberHandlesMissingCancelAndSuccess) {
    SetInput("999\n");
    delete_member(&lib);
    AddMember();
    SetInput("201\nn\n");
    delete_member(&lib);
    EXPECT_EQ(lib.member_count, 1U);
    SetInput("201\ny\n");
    delete_member(&lib);
    EXPECT_EQ(lib.member_count, 0U);
}
TEST_F(LibraryTestEnvironment, DeleteMemberRejectsActiveLoan) {
    AddBook();
    AddMember();
    AddTransaction();
    SetInput("201\n");
    delete_member(&lib);
    EXPECT_EQ(lib.member_count, 1U);
}

/* --------------------------- transactions.c --------------------------- */

TEST_F(LibraryTestEnvironment, IssueBookSuccess) {
    AddBook();
    AddMember();
    SetInput("101\n201\n2099-12-31\n");
    issue_book(&lib);
    ASSERT_EQ(lib.transaction_count, 1U);
    EXPECT_EQ(lib.books[0].available_copies, 1);
    EXPECT_EQ(lib.transactions[0].returned, 0);
}
TEST_F(LibraryTestEnvironment, IssueBookRejectsUnknownMember) {
    AddBook();
    SetInput("101\n999\n");
    issue_book(&lib);
    EXPECT_EQ(lib.transaction_count, 0U);
}
TEST_F(LibraryTestEnvironment, IssueBookRejectsUnknownBook) {
    AddMember();
    SetInput("999\n201\n");
    issue_book(&lib);
    EXPECT_EQ(lib.transaction_count, 0U);
}
TEST_F(LibraryTestEnvironment, IssueBookRejectsUnavailableCopy) {
    AddBook(101, "Book", 1, 0);
    AddMember();
    SetInput("101\n201\n");
    issue_book(&lib);
    EXPECT_EQ(lib.transaction_count, 0U);
}
TEST_F(LibraryTestEnvironment, IssueBookRejectsInvalidDueDate) {
    AddBook();
    AddMember();
    SetInput("101\n201\n2026-02-30\n");
    issue_book(&lib);
    EXPECT_EQ(lib.transaction_count, 0U);
}
TEST_F(LibraryTestEnvironment, ReturnBookCalculatesFine) {
    AddBook(101, "Book", 2, 1);
    AddMember();
    AddTransaction();
    SetInput("1\n2026-11-02\n");
    return_book(&lib);
    EXPECT_EQ(lib.transactions[0].returned, 1);
    EXPECT_DOUBLE_EQ(lib.transactions[0].fine, 20.0);
    EXPECT_EQ(lib.books[0].available_copies, 2);
}
TEST_F(LibraryTestEnvironment, ReturnBookRejectsUnknownTransaction) {
    AddBook();
    SetInput("999\n");
    return_book(&lib);
    EXPECT_EQ(lib.transaction_count, 0U);
}
TEST_F(LibraryTestEnvironment, ReturnBookRejectsAlreadyReturnedTransaction) {
    AddBook();
    AddTransaction(1, 101, 201, 1);
    SetInput("1\n");
    return_book(&lib);
    EXPECT_EQ(lib.transactions[0].returned, 1);
}
TEST_F(LibraryTestEnvironment, ReturnBookRejectsInvalidDate) {
    AddBook(101, "Book", 2, 1);
    AddTransaction();
    SetInput("1\n2026-02-30\n");
    return_book(&lib);
    EXPECT_EQ(lib.transactions[0].returned, 0);
}
TEST_F(LibraryTestEnvironment, ListTransactionsHandlesEmptyAndPopulated) {
    list_transactions(&lib);
    AddTransaction();
    list_transactions(&lib);
}
TEST_F(LibraryTestEnvironment, FineDetailsHandlesNoFineAndReturnedFine) {
    show_fine_details(&lib);
    AddTransaction(1, 101, 201, 1);
    show_fine_details(&lib);
}
