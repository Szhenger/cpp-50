#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>
#include <algorithm>
#include <optional>
#include <print>
#include <ranges>
#include <cctype>

using ID = std::string;

// Basic data structures to hold our IMDB entities
struct Person {
    std::string name;
    std::string birth;
    std::unordered_set<ID> movies;
};

struct Movie {
    std::string title;
    std::string year;
    std::unordered_set<ID> stars;
};

// Represents a node in the search tree
struct Node {
    ID state; // Person ID
    std::shared_ptr<Node> parent;
    ID action; // Movie ID

    Node(ID state, std::shared_ptr<Node> parent, ID action)
        : state(std::move(state)), parent(std::move(parent)), action(std::move(action)) {}
};

class DegreesOfSeparation {
private:
    std::unordered_map<std::string, std::unordered_set<ID>> names_map;
    std::unordered_map<ID, Person> people;
    std::unordered_map<ID, Movie> movies;

    // Helper method to parse an RFC 4180-style CSV line (handles quotes and commas within quotes)
    std::vector<std::string> parse_csv_line(const std::string& line) const {
        std::vector<std::string> result;
        std::string current;
        bool in_quotes = false;
        
        for (char c : line) {
            if (c == '\"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                result.push_back(current);
                current.clear();
            } else if (c != '\r') { // Ignore carriage return for Windows-generated files
                current += c;
            }
        }
        result.push_back(current);
        return result;
    }

    std::unordered_set<std::pair<ID, ID>, decltype([](const std::pair<ID, ID>& p) {
        return std::hash<ID>{}(p.first) ^ (std::hash<ID>{}(p.second) << 1);
    })> neighbors_for_person(const ID& person_id) const {
        decltype(neighbors_for_person("")) neighbors;
        
        auto p_it = people.find(person_id);
        if (p_it == people.end()) return neighbors;

        for (const auto& movie_id : p_it->second.movies) {
            auto m_it = movies.find(movie_id);
            if (m_it != movies.end()) {
                for (const auto& star_id : m_it->second.stars) {
                    neighbors.insert({movie_id, star_id});
                }
            }
        }
        return neighbors;
    }

public:
    void load_data(const std::string& directory) {
        auto read_csv = [&](const std::string& filename, auto row_handler) {
            std::ifstream file(directory + "/" + filename);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open " + directory + "/" + filename);
            }

            std::string line;
            if (!std::getline(file, line)) return;

            // Parse header to dynamically map columns
            std::vector<std::string> headers = parse_csv_line(line);
            std::unordered_map<std::string, size_t> header_map;
            for (size_t i = 0; i < headers.size(); ++i) {
                header_map[headers[i]] = i;
            }

            while (std::getline(file, line)) {
                if (line.empty()) continue;
                std::vector<std::string> row = parse_csv_line(line);
                
                auto get_col = [&](const std::string& col_name) -> std::string {
                    auto it = header_map.find(col_name);
                    if (it != header_map.end() && it->second < row.size()) {
                        return row[it->second];
                    }
                    return "";
                };
                row_handler(get_col);
            }
        };

        // 1. Load people
        read_csv("people.csv", [&](auto get_col) {
            ID id = get_col("id");
            std::string name = get_col("name");
            std::string birth = get_col("birth");

            people[id] = {name, birth, {}};
            
            std::string lower_name = name;
            std::ranges::transform(lower_name, lower_name.begin(), [](unsigned char c) { return std::tolower(c); });
            names_map[lower_name].insert(id);
        });

        // 2. Load movies
        read_csv("movies.csv", [&](auto get_col) {
            ID id = get_col("id");
            movies[id] = {get_col("title"), get_col("year"), {}};
        });

        // 3. Load stars
        read_csv("stars.csv", [&](auto get_col) {
            ID person_id = get_col("person_id");
            ID movie_id = get_col("movie_id");

            if (people.contains(person_id)) people[person_id].movies.insert(movie_id);
            if (movies.contains(movie_id)) movies[movie_id].stars.insert(person_id);
        });
    }

    std::optional<ID> person_id_for_name(const std::string& name) const {
        std::string lower_name = name;
        std::ranges::transform(lower_name, lower_name.begin(), [](unsigned char c) { return std::tolower(c); });

        auto it = names_map.find(lower_name);
        if (it == names_map.end() || it->second.empty()) {
            return std::nullopt;
        }

        const auto& person_ids = it->second;
        if (person_ids.size() == 1) {
            return *person_ids.begin();
        }

        std::println("Which '{}'?", name);
        for (const auto& person_id : person_ids) {
            const auto& person = people.at(person_id);
            std::println("ID: {}, Name: {}, Birth: {}", person_id, person.name, person.birth);
        }

        std::print("Intended Person ID: ");
        std::string intended_id;
        std::getline(std::cin, intended_id);

        if (person_ids.contains(intended_id)) {
            return intended_id;
        }

        return std::nullopt;
    }

    std::optional<std::vector<std::pair<ID, ID>>> shortest_path(const ID& source, const ID& target) const {
        if (source == target) {
            return std::vector<std::pair<ID, ID>>{};
        }

        std::queue<std::shared_ptr<Node>> frontier;
        std::unordered_set<ID> explored;
        
        // Frontier states optimization to check containments in O(1) avoiding Queue searches
        std::unordered_set<ID> frontier_states; 

        for (const auto& [movie_id, person_id] : neighbors_for_person(source)) {
            frontier.push(std::make_shared<Node>(person_id, nullptr, movie_id));
            frontier_states.insert(person_id);
        }

        while (!frontier.empty()) {
            auto node = frontier.front();
            frontier.pop();
            frontier_states.erase(node->state);
            explored.insert(node->state);

            if (node->state == target) {
                std::vector<std::pair<ID, ID>> path;
                auto curr = node;
                while (curr != nullptr) {
                    path.push_back({curr->action, curr->state});
                    curr = curr->parent;
                }
                std::ranges::reverse(path);
                return path;
            }

            for (const auto& [movie_id, person_id] : neighbors_for_person(node->state)) {
                if (!explored.contains(person_id) && !frontier_states.contains(person_id)) {
                    frontier.push(std::make_shared<Node>(person_id, node, movie_id));
                    frontier_states.insert(person_id);
                }
            }
        }

        return std::nullopt;
    }

    // Accessors for printing results
    const Person& get_person(const ID& id) const { return people.at(id); }
    const Movie& get_movie(const ID& id) const { return movies.at(id); }
};

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::println(std::cerr, "Usage: ./degrees [directory]");
        return 1;
    }

    std::string directory = (argc == 2) ? argv[1] : "large";
    DegreesOfSeparation ds;

    try {
        std::println("Loading data...");
        ds.load_data(directory);
        std::println("Data loaded.");
    } catch (const std::exception& e) {
        std::println(std::cerr, "Error: {}", e.what());
        return 1;
    }

    std::print("Name: ");
    std::string source_name;
    std::getline(std::cin, source_name);
    auto source = ds.person_id_for_name(source_name);
    if (!source) {
        std::println(std::cerr, "Person not found.");
        return 1;
    }

    std::print("Name: ");
    std::string target_name;
    std::getline(std::cin, target_name);
    auto target = ds.person_id_for_name(target_name);
    if (!target) {
        std::println(std::cerr, "Person not found.");
        return 1;
    }

    auto path_opt = ds.shortest_path(*source, *target);

    if (!path_opt) {
        std::println("Not connected.");
    } else {
        auto& path = *path_opt;
        int degrees = path.size();
        std::println("{} degrees of separation.", degrees);

        // Prepend source to match the exact Python loop indexing capability
        std::vector<std::pair<ID, ID>> full_path;
        full_path.push_back({"", *source});
        full_path.insert(full_path.end(), path.begin(), path.end());

        for (int i = 0; i < degrees; ++i) {
            std::string person1 = ds.get_person(full_path[i].second).name;
            std::string person2 = ds.get_person(full_path[i + 1].second).name;
            std::string movie = ds.get_movie(full_path[i + 1].first).title;
            
            std::println("{}: {} and {} starred in {}", i + 1, person1, person2, movie);
        }
    }

    return 0;
}
