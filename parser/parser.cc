#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <print>

const std::string TERMINALS = R"(
Adj -> "country" | "dreadful" | "enigmatical" | "little" | "moist" | "red"
Adv -> "down" | "here" | "never"
Conj -> "and" | "until"
Det -> "a" | "an" | "his" | "my" | "the"
N -> "armchair" | "companion" | "day" | "door" | "hand" | "he" | "himself"
N -> "holmes" | "home" | "i" | "mess" | "paint" | "palm" | "pipe" | "she"
N -> "smile" | "thursday" | "walk" | "we" | "word"
P -> "at" | "before" | "in" | "of" | "on" | "to"
V -> "arrived" | "came" | "chuckled" | "had" | "lit" | "said" | "sat"
V -> "smiled" | "tell" | "were"
)";

const std::string NONTERMINALS = R"(
S -> NP VP
NP -> N | Det N | Det Adj N | Det Adj Adj N | NP PP
VP -> V | VP Adv | Adv VP | VP Conj VP | VP NP | VP PP
PP -> P NP
)";

// --- Data Structures ---

struct Tree {
    std::string label;
    std::vector<Tree> children;
    std::string word;

    bool is_leaf() const {
        return children.empty() && !word.empty();
    }

    bool operator==(const Tree& other) const {
        return label == other.label && word == other.word && children == other.children;
    }
};

struct Rule {
    std::string lhs;
    std::vector<std::string> rhs;
    bool is_terminal;
};

// --- NLTK Helper Mimics ---

void pretty_print(const Tree& tree, int depth = 0) {
    std::string indent(depth * 4, ' ');
    if (tree.is_leaf()) {
        std::println("{}{}", indent, tree.word);
    } else {
        std::println("{}({}", indent, tree.label);
        for (const auto& child : tree.children) {
            pretty_print(child, depth + 1);
        }
        std::println("{})", indent);
    }
}

std::vector<std::string> flatten(const Tree& t) {
    std::vector<std::string> res;
    if (t.is_leaf()) {
        res.push_back(t.word);
    } else {
        for (const auto& c : t.children) {
            auto c_res = flatten(c);
            res.insert(res.end(), c_res.begin(), c_res.end());
        }
    }
    return res;
}

void get_subtrees(const Tree& t, std::vector<Tree>& result) {
    result.push_back(t);
    for (const auto& c : t.children) {
        get_subtrees(c, result);
    }
}

// --- Core Logic ---

std::vector<Rule> parse_cfg(const std::string& terminals, const std::string& nonterminals) {
    std::vector<Rule> rules;
    
    auto process_block = [&](const std::string& block, bool is_term) {
        std::istringstream iss(block);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty() || line.find("->") == std::string::npos) continue;
            
            size_t arrow = line.find("->");
            std::string lhs = line.substr(0, arrow);
            lhs.erase(0, lhs.find_first_not_of(" \t"));
            lhs.erase(lhs.find_last_not_of(" \t") + 1);

            std::istringstream rhs_iss(line.substr(arrow + 2));
            std::string option;
            while (std::getline(rhs_iss, option, '|')) {
                Rule r{lhs, {}, is_term};
                std::istringstream opt_iss(option);
                std::string token;
                while (opt_iss >> token) {
                    if (is_term) {
                        token.erase(std::remove(token.begin(), token.end(), '"'), token.end());
                    }
                    r.rhs.push_back(token);
                }
                if (!r.rhs.empty()) rules.push_back(r);
            }
        }
    };
    
    process_block(nonterminals, false);
    process_block(terminals, true);
    return rules;
}

std::vector<std::string> preprocess(const std::string& sentence) {
    std::vector<std::string> words;
    std::string padded;
    
    // NLTK tokenizes punctuation as separate words.
    for (char c : sentence) {
        if (std::ispunct(c)) {
            padded += " "; padded += c; padded += " ";
        } else {
            padded += c;
        }
    }
    
    std::istringstream iss(padded);
    std::string token;
    while (iss >> token) {
        std::string lower_word;
        bool has_alpha = false;
        for (char c : token) {
            lower_word += std::tolower(c);
            if (std::isalpha(c)) has_alpha = true;
        }
        if (has_alpha) {
            words.push_back(lower_word);
        }
    }
    return words;
}

std::vector<Tree> np_chunk(const Tree& tree) {
    std::vector<Tree> chunks;
    std::vector<Tree> all_subtrees;
    get_subtrees(tree, all_subtrees);

    for (const auto& parent : all_subtrees) {
        if (parent.label == "NP") {
            bool is_chunk = true;
            std::vector<Tree> child_subtrees;
            for (const auto& child : parent.children) {
                get_subtrees(child, child_subtrees);
            }
            
            for (const auto& desc : child_subtrees) {
                if (desc.label == "NP") {
                    is_chunk = false;
                    break;
                }
            }
            if (is_chunk) {
                chunks.push_back(parent);
            }
        }
    }
    return chunks;
}

// Emulates NLTK ChartParser algorithm (Dynamic Programming)
class ChartParser {
    std::vector<Rule> rules;

    void find_matches(const std::vector<std::vector<std::vector<Tree>>>& chart,
                      int i, int j, const std::vector<std::string>& rhs, int rhs_idx,
                      std::vector<Tree>& current_children, std::vector<std::vector<Tree>>& results) {
        if (rhs_idx == rhs.size()) {
            if (i == j) results.push_back(current_children);
            return;
        }
        if (i == j) return;

        for (int k = i + 1; k <= j; ++k) {
            for (const auto& t : chart[i][k]) {
                if (t.label == rhs[rhs_idx]) {
                    current_children.push_back(t);
                    find_matches(chart, k, j, rhs, rhs_idx + 1, current_children, results);
                    current_children.pop_back();
                }
            }
        }
    }

public:
    ChartParser(std::vector<Rule> r) : rules(std::move(r)) {}

    std::vector<Tree> parse(const std::vector<std::string>& words) {
        int n = words.size();
        if (n == 0) return {};

        // chart[i][j] stores all subtrees spanning from word index i to j
        std::vector<std::vector<std::vector<Tree>>> chart(n, std::vector<std::vector<Tree>>(n + 1));

        // 1. Initialize terminal leaves
        for (int i = 0; i < n; ++i) {
            Tree leaf{"", {}, words[i]};
            for (const auto& r : rules) {
                if (r.is_terminal && r.rhs.size() == 1 && r.rhs[0] == words[i]) {
                    chart[i][i+1].push_back(Tree{r.lhs, {leaf}, ""});
                }
            }
        }

        // 2. Bottom-up parsing spans
        for (int len = 1; len <= n; ++len) {
            for (int i = 0; i <= n - len; ++i) {
                int j = i + len;
                bool added = true;
                
                // Allow unary rules to exhaustively evaluate
                while (added) {
                    added = false;
                    for (const auto& r : rules) {
                        if (r.is_terminal) continue;

                        std::vector<std::vector<Tree>> results;
                        std::vector<Tree> curr;
                        find_matches(chart, i, j, r.rhs, 0, curr, results);

                        for (const auto& children : results) {
                            Tree new_tree{r.lhs, children, ""};
                            // Add if not a duplicate
                            if (std::find(chart[i][j].begin(), chart[i][j].end(), new_tree) == chart[i][j].end()) {
                                chart[i][j].push_back(new_tree);
                                added = true;
                            }
                        }
                    }
                }
            }
        }

        // Extract valid start symbols ("S") spanning the entire sentence
        std::vector<Tree> parsed_trees;
        for (const auto& t : chart[0][n]) {
            if (t.label == "S") {
                parsed_trees.push_back(t);
            }
        }
        return parsed_trees;
    }
};

// --- Execution ---

int main(int argc, char* argv[]) {
    std::string s;

    if (argc == 2) {
        std::ifstream f(argv[1]);
        if (f) {
            std::ostringstream ss;
            ss << f.rdbuf();
            s = ss.str();
        } else {
            std::println(stderr, "Could not open file.");
            return 1;
        }
    } else {
        std::print("Sentence: ");
        std::getline(std::cin, s);
    }

    auto words = preprocess(s);

    auto rules = parse_cfg(TERMINALS, NONTERMINALS);
    ChartParser parser(rules);

    auto trees = parser.parse(words);

    if (trees.empty()) {
        std::println("Could not parse sentence.");
        return 0;
    }

    for (const auto& tree : trees) {
        pretty_print(tree);

        std::println("\nNoun Phrase Chunks");
        for (const auto& np : np_chunk(tree)) {
            auto flat = flatten(np);
            std::string joined;
            for (size_t i = 0; i < flat.size(); ++i) {
                joined += flat[i];
                if (i + 1 < flat.size()) joined += " ";
            }
            std::println("{}", joined);
        }
        std::println(""); // Space out multiple parsed trees
    }

    return 0;
}
