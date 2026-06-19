#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <fstream>
#include <regex>
#include <random>
#include <cmath>
#include <print> // C++23 print formatting

namespace fs = std::filesystem;

// Type Aliases for readability mapping to Python's structures
using Corpus = std::map<std::string, std::set<std::string>>;
using ProbDist = std::map<std::string, double>;

// Constants
constexpr double DAMPING = 0.85;
constexpr int SAMPLES = 10000;

// Function Prototypes
Corpus crawl(const std::string& directory);
ProbDist transition_model(const Corpus& corpus, const std::string& page, double damping_factor);
ProbDist sample_pagerank(const Corpus& corpus, double damping_factor, int n);
ProbDist iterate_pagerank(const Corpus& corpus, double damping_factor);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::println(stderr, "Usage: ./pagerank corpus");
        return 1;
    }

    std::string directory = argv[1];
    Corpus corpus = crawl(directory);

    ProbDist sampled_ranks = sample_pagerank(corpus, DAMPING, SAMPLES);
    std::println("PageRank Results from Sampling (n = {})", SAMPLES);
    for (const auto& [page, rank] : sampled_ranks) {
        std::println("  {}: {:.4f}", page, rank);
    }

    ProbDist iterated_ranks = iterate_pagerank(corpus, DAMPING);
    std::println("PageRank Results from Iteration");
    for (const auto& [page, rank] : iterated_ranks) {
        std::println("  {}: {:.4f}", page, rank);
    }

    return 0;
}

Corpus crawl(const std::string& directory) {
    Corpus pages;

    // Extract all links from HTML files
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() != ".html") {
            continue;
        }

        std::string filename = entry.path().filename().string();
        std::ifstream file(entry.path());
        if (!file.is_open()) continue;

        // Read entire file into a string
        std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Regex to extract links
        std::regex link_re(R"(<a\s+(?:[^>]*?)href="([^"]*)")");
        auto words_begin = std::sregex_iterator(contents.begin(), contents.end(), link_re);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string link = match[1].str();
            if (link != filename) {
                pages[filename].insert(link);
            }
        }
    }

    // Only include links to other pages in the corpus
    for (auto& [filename, links] : pages) {
        for (auto it = links.begin(); it != links.end(); ) {
            if (!pages.contains(*it)) {
                it = links.erase(it);
            } else {
                ++it;
            }
        }
    }

    return pages;
}

ProbDist transition_model(const Corpus& corpus, const std::string& page, double damping_factor) {
    ProbDist prob_dist;
    
    // Initialize dictionary mapping pages in corpus to real 0
    for (const auto& [state, _] : corpus) {
        prob_dist[state] = 0.0;
    }

    // Check whether page in corpus
    if (corpus.contains(page)) {
        double num_pages = static_cast<double>(corpus.size());
        const auto& linked_pages = corpus.at(page);
        double num_links = static_cast<double>(linked_pages.size());

        if (num_links > 0) {
            // Get real probabilities to choose a link on page at random
            for (const auto& state : linked_pages) {
                prob_dist[state] += damping_factor / num_links;
            }
            // Get real probabilities to choose any page at random
            for (const auto& [state, _] : corpus) {
                prob_dist[state] += (1.0 - damping_factor) / num_pages;
            }
        } else {
            // Get real probabilities choosing randomly over all pages equally
            for (const auto& [state, _] : corpus) {
                prob_dist[state] += 1.0 / num_pages;
            }
        }
    }
    
    return prob_dist;
}

ProbDist sample_pagerank(const Corpus& corpus, double damping_factor, int n) {
    std::vector<std::string> pages;
    std::map<std::string, int> samples;

    for (const auto& [page, _] : corpus) {
        pages.push_back(page);
        samples[page] = 0;
    }

    // Randomly choose first sample of a page
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> initial_dist(0, pages.size() - 1);

    std::string sample = pages[initial_dist(gen)];
    samples[sample] += 1;

    // Randomly choose remaining samples based on transition model
    for (int i = 1; i < n; ++i) {
        ProbDist model = transition_model(corpus, sample, damping_factor);
        
        std::vector<std::string> keys;
        std::vector<double> weights;
        
        for (const auto& [key, weight] : model) {
            keys.push_back(key);
            weights.push_back(weight);
        }

        // Create a discrete distribution mapping to weights dynamically
        std::discrete_distribution<> custom_dist(weights.begin(), weights.end());
        int chosen_idx = custom_dist(gen);
        sample = keys[chosen_idx];
        samples[sample] += 1;
    }

    // Make page rank dictionary mapping each page to corresponding proportions
    ProbDist page_ranks;
    for (const auto& page : pages) {
        page_ranks[page] = static_cast<double>(samples[page]) / n;
    }

    return page_ranks;
}

ProbDist iterate_pagerank(const Corpus& corpus, double damping_factor) {
    ProbDist page_ranks;
    double num_pages = static_cast<double>(corpus.size());

    // Initialize page rank dictionary mapping each page to real probability 0
    for (const auto& [page, _] : corpus) {
        page_ranks[page] = 0.0;
    }

    if (num_pages > 0) {
        // Rank each page equally
        for (const auto& [page, _] : corpus) {
            page_ranks[page] = 1.0 / num_pages;
        }

        // Get number of links for each page has into a dictionary
        std::map<std::string, double> num_links;
        for (const auto& [page, links] : corpus) {
            num_links[page] = links.empty() ? num_pages : static_cast<double>(links.size());
        }

        // Iteratively rank every page until accuracy is less than or equal to 0.001
        bool iterate = true;
        while (iterate) {
            iterate = false;
            double first_condition = (1.0 - damping_factor) / num_pages;

            for (const auto& [page, _] : corpus) {
                double current_rank = page_ranks[page];
                double second_condition = 0.0;

                for (const auto& [linking_page, links] : corpus) {
                    if (links.contains(page) || links.empty()) {
                        second_condition += page_ranks[linking_page] / num_links[linking_page];
                    }
                }
                
                second_condition *= damping_factor;
                double new_rank = first_condition + second_condition;
                page_ranks[page] = new_rank;

                if (std::abs(new_rank - current_rank) > 0.001) {
                    iterate = true;
                }
            }
        }
    }

    return page_ranks;
}
