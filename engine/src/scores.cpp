#include "tae/scores.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace tae {

std::filesystem::path HighScores::dir() {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) {
        return std::filesystem::path(xdg) / "terminal-arcade";
    }
    if (const char* home = std::getenv("HOME"); home && *home) {
        return std::filesystem::path(home) / ".local" / "share" / "terminal-arcade";
    }
    return std::filesystem::path(".") / ".terminal-arcade";
}

HighScores::HighScores(const std::string& game, std::size_t max_entries)
    : path_(dir() / (game + ".scores")), max_(max_entries) {
    load();
}

void HighScores::load() {
    entries_.clear();
    std::ifstream in(path_);
    long score;
    std::string name;
    while (in >> score) {
        in.ignore(1, '\t');
        std::getline(in, name);
        entries_.push_back({name, score});
    }
    std::sort(entries_.begin(), entries_.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > max_) entries_.resize(max_);
}

void HighScores::save() const {
    std::error_code ec;
    std::filesystem::create_directories(path_.parent_path(), ec);
    std::ofstream out(path_, std::ios::trunc);
    for (const ScoreEntry& e : entries_) {
        out << e.score << '\t' << e.name << '\n';
    }
}

int HighScores::add(const std::string& name, long score) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const ScoreEntry& e) { return score > e.score; });
    if (it == entries_.end() && entries_.size() >= max_) return -1;
    int rank = static_cast<int>(it - entries_.begin());
    entries_.insert(it, {name, score});
    if (entries_.size() > max_) entries_.resize(max_);
    save();
    return rank;
}

}  // namespace tae
