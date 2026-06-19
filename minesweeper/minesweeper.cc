#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <algorithm>
#include <optional>
#include <string>
#include <ranges>
#include <format>
#include <print> // C++23 standard printing

// A struct to represent cell coordinates, acting as our tuples.
struct Cell {
    int i, j;
    
    // C++20/23: Auto-generates all comparison operators (==, <, >, etc.)
    // This is required to use Cell safely inside a std::set.
    auto operator<=>(const Cell&) const = default;
};

class Minesweeper {
public:
    int height;
    int width;
    std::set<Cell> mines;
    std::vector<std::vector<bool>> board;
    std::set<Cell> mines_found;

    Minesweeper(int height = 8, int width = 8, int num_mines = 8)
        : height(height), width(width) {
        
        // Initialize an empty field with no mines
        board.assign(height, std::vector<bool>(width, false));

        // Setup C++ random number generation
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist_h(0, height - 1);
        std::uniform_int_distribution<> dist_w(0, width - 1);

        // Add mines randomly
        while (mines.size() < static_cast<size_t>(num_mines)) {
            int i = dist_h(gen);
            int j = dist_w(gen);
            if (!board[i][j]) {
                mines.insert({i, j});
                board[i][j] = true;
            }
        }
    }

    void print() const {
        for (int i = 0; i < height; ++i) {
            std::println("{}-", std::string(width * 2, '-'));
            for (int j = 0; j < width; ++j) {
                if (board[i][j]) {
                    std::print("|X");
                } else {
                    std::print("| ");
                }
            }
            std::println("|");
        }
        std::println("{}-", std::string(width * 2, '-'));
    }

    bool is_mine(const Cell& cell) const {
        return board[cell.i][cell.j];
    }

    int nearby_mines(const Cell& cell) const {
        int count = 0;
        for (int i = cell.i - 1; i <= cell.i + 1; ++i) {
            for (int j = cell.j - 1; j <= cell.j + 1; ++j) {
                if (i == cell.i && j == cell.j) continue;

                if (i >= 0 && i < height && j >= 0 && j < width) {
                    if (board[i][j]) {
                        count++;
                    }
                }
            }
        }
        return count;
    }

    bool won() const {
        return mines_found == mines;
    }
};

class Sentence {
public:
    std::set<Cell> cells;
    int count;

    Sentence(std::set<Cell> cells, int count) 
        : cells(std::move(cells)), count(count) {}

    bool operator==(const Sentence& other) const {
        return cells == other.cells && count == other.count;
    }

    // Helper for debugging, mirrors Python's __str__
    std::string str() const {
        std::string s = "{";
        for (auto it = cells.begin(); it != cells.end(); ++it) {
            if (it != cells.begin()) s += ", ";
            s += std::format("({}, {})", it->i, it->j);
        }
        s += std::format("}} = {}", count);
        return s;
    }

    std::set<Cell> known_mines() const {
        if (cells.size() == static_cast<size_t>(count)) {
            return cells;
        }
        return {};
    }

    std::set<Cell> known_safes() const {
        if (count == 0) {
            return cells;
        }
        return {};
    }

    void mark_mine(const Cell& cell) {
        if (cells.erase(cell)) {
            count--;
        }
    }

    void mark_safe(const Cell& cell) {
        cells.erase(cell);
    }
};

class MinesweeperAI {
public:
    int height;
    int width;
    std::set<Cell> moves_made;
    std::set<Cell> mines;
    std::set<Cell> safes;
    std::vector<Sentence> knowledge;

    MinesweeperAI(int height = 8, int width = 8) 
        : height(height), width(width) {}

    void mark_mine(const Cell& cell) {
        mines.insert(cell);
        for (auto& sentence : knowledge) {
            sentence.mark_mine(cell);
        }
    }

    void mark_safe(const Cell& cell) {
        safes.insert(cell);
        for (auto& sentence : knowledge) {
            sentence.mark_safe(cell);
        }
    }

    void add_knowledge(const Cell& cell, int count) {
        moves_made.insert(cell);
        mark_safe(cell);

        std::set<Cell> nearby_cells;
        for (int i = cell.i - 1; i <= cell.i + 1; ++i) {
            for (int j = cell.j - 1; j <= cell.j + 1; ++j) {
                if (i >= 0 && i < height && j >= 0 && j < width && (i != cell.i || j != cell.j)) {
                    nearby_cells.insert({i, j});
                }
            }
        }

        // Note: Python allows mutating lists during iteration, which causes skipped items.
        // We fix that logical bug here by creating a cleaned set correctly.
        std::set<Cell> unresolved_cells;
        for (const auto& c : nearby_cells) {
            if (safes.contains(c)) {
                continue;
            }
            if (mines.contains(c)) {
                count--;
                continue;
            }
            unresolved_cells.insert(c);
        }

        knowledge.emplace_back(unresolved_cells, count);

        // Lambda to repeatedly mark safes and mines until no more updates can be made
        auto mark = [this]() {
            bool changed = true;
            while (changed) {
                changed = false;
                for (auto& sentence : knowledge) {
                    std::set<Cell> ks = sentence.known_safes();
                    for (const auto& c : ks) {
                        if (!safes.contains(c)) { 
                            mark_safe(c); 
                            changed = true; 
                        }
                    }
                    std::set<Cell> km = sentence.known_mines();
                    for (const auto& c : km) {
                        if (!mines.contains(c)) { 
                            mark_mine(c); 
                            changed = true; 
                        }
                    }
                }
            }
        };

        mark();

        // Infer new sentences using C++23 Ranges
        std::vector<Sentence> new_sentences;
        for (const auto& sentence1 : knowledge) {
            for (const auto& sentence2 : knowledge) {
                if (sentence1 == sentence2) continue;

                // Check for strict superset (std::set iterators are guaranteed sorted)
                if (sentence1.cells.size() > sentence2.cells.size() &&
                    std::ranges::includes(sentence1.cells, sentence2.cells)) {
                    
                    std::set<Cell> diff;
                    std::ranges::set_difference(sentence1.cells, sentence2.cells,
                                                std::inserter(diff, diff.begin()));
                    
                    Sentence newer_sentence(diff, sentence1.count - sentence2.count);
                    
                    if (newer_sentence.count >= 0) {
                        // Prevent storing endless duplicate sentences
                        if (std::ranges::find(knowledge, newer_sentence) == knowledge.end() &&
                            std::ranges::find(new_sentences, newer_sentence) == new_sentences.end()) {
                            new_sentences.push_back(newer_sentence);
                        }
                    }
                }
            }
        }
        
        // Append newly inferred knowledge
        knowledge.insert(knowledge.end(), new_sentences.begin(), new_sentences.end());

        mark();
    }

    // std::optional handles returning a valid cell or nothing (Python's None)
    std::optional<Cell> make_safe_move() const {
        for (const auto& cell : safes) {
            if (!moves_made.contains(cell)) {
                return cell;
            }
        }
        return std::nullopt; 
    }

    std::optional<Cell> make_random_move() const {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist_h(0, height - 1);
        std::uniform_int_distribution<> dist_w(0, width - 1);

        // Track attempts to avoid an infinite loop if the board is fully solved/full.
        int max_attempts = height * width * 2; 
        
        while (max_attempts-- > 0) {
            Cell random_move{dist_h(gen), dist_w(gen)};
            if (!moves_made.contains(random_move) && !mines.contains(random_move)) {
                return random_move;
            }
        }
        return std::nullopt;
    }
};
