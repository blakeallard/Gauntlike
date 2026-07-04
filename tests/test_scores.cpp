#include <catch2/catch_test_macros.hpp>

#include <unistd.h>

#include <cstdlib>
#include <filesystem>

#include "tae/scores.hpp"

using namespace tae;

namespace {
// Point XDG_DATA_HOME at a scratch dir so tests never touch real scores.
struct ScratchDataDir {
    std::filesystem::path dir;
    ScratchDataDir() {
        dir = std::filesystem::temp_directory_path() /
              ("tae-scores-test-" + std::to_string(::getpid()));
        std::filesystem::remove_all(dir);
        ::setenv("XDG_DATA_HOME", dir.c_str(), 1);
    }
    ~ScratchDataDir() {
        std::filesystem::remove_all(dir);
        ::unsetenv("XDG_DATA_HOME");
    }
};
}  // namespace

TEST_CASE("high scores persist across instances, sorted, capped") {
    ScratchDataDir scratch;

    {
        HighScores hs("testgame", 3);
        CHECK(hs.entries().empty());
        CHECK(hs.add("WARRIOR", 100) == 0);
        CHECK(hs.add("ELF", 300) == 0);      // new best
        CHECK(hs.add("WIZARD", 200) == 1);   // slots between
        CHECK(hs.add("VALKYRIE", 50) == -1); // table full, too low
        REQUIRE(hs.entries().size() == 3);
        CHECK(hs.entries()[0].score == 300);
        CHECK(hs.entries()[2].score == 100);
    }

    // Reload from disk.
    HighScores hs2("testgame", 3);
    REQUIRE(hs2.entries().size() == 3);
    CHECK(hs2.entries()[0].name == "ELF");
    CHECK(hs2.entries()[0].score == 300);
    CHECK(hs2.entries()[1].name == "WIZARD");

    // A qualifying score bumps the last one out.
    CHECK(hs2.add("VALKYRIE", 250) == 1);
    CHECK(hs2.entries().size() == 3);
    CHECK(hs2.entries()[2].score == 200);

    // Separate games get separate tables.
    HighScores other("othergame", 3);
    CHECK(other.entries().empty());
}
