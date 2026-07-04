#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace tae {

struct ScoreEntry {
    std::string name;
    long score = 0;
};

// Per-game persistent high-score table, stored as one file per game under
// $XDG_DATA_HOME/terminal-arcade (default ~/.local/share/terminal-arcade).
class HighScores {
public:
    explicit HighScores(const std::string& game, std::size_t max_entries = 10);

    const std::vector<ScoreEntry>& entries() const { return entries_; }

    // Insert if it qualifies, persist, and return the 0-based rank — or -1
    // if the score didn't make the table.
    int add(const std::string& name, long score);

    static std::filesystem::path dir();

private:
    void load();
    void save() const;

    std::filesystem::path path_;
    std::size_t max_;
    std::vector<ScoreEntry> entries_;
};

}  // namespace tae
