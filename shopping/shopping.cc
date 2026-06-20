#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <random>
#include <numeric>
#include <print>

const double TEST_SIZE = 0.4;

using Evidence = std::vector<double>;
using Labels = std::vector<int>;

// Custom structs to mirror scikit-learn's functionality
struct SplitData {
    std::vector<Evidence> X_train;
    std::vector<Evidence> X_test;
    Labels y_train;
    Labels y_test;
};

class KNeighborsClassifier {
    std::vector<Evidence> X_train;
    Labels y_train;

public:
    void fit(const std::vector<Evidence>& evidence, const Labels& labels) {
        X_train = evidence;
        y_train = labels;
    }

    Labels predict(const std::vector<Evidence>& X_test) {
        Labels predictions;
        predictions.reserve(X_test.size());

        for (const auto& test_point : X_test) {
            double min_dist = std::numeric_limits<double>::max();
            int best_label = 0;

            for (size_t i = 0; i < X_train.size(); ++i) {
                double dist = 0.0;
                // Calculate squared Euclidean distance
                for (size_t j = 0; j < test_point.size(); ++j) {
                    double diff = test_point[j] - X_train[i][j];
                    dist += diff * diff;
                }
                if (dist < min_dist) {
                    min_dist = dist;
                    best_label = y_train[i];
                }
            }
            predictions.push_back(best_label);
        }
        return predictions;
    }
};

// Function prototypes
std::pair<std::vector<Evidence>, Labels> load_data(const std::string& filename);
SplitData train_test_split(const std::vector<Evidence>& evidence, const Labels& labels, double test_size);
std::pair<double, double> evaluate(const Labels& labels, const Labels& predictions);

int main(int argc, char* argv[]) {
    // Check command-line arguments
    if (argc != 2) {
        std::println(stderr, "Usage: ./shopping data.csv");
        return 1;
    }

    try {
        // Load data from spreadsheet and split into train and test sets
        auto [evidence, labels] = load_data(argv[1]);
        auto split = train_test_split(evidence, labels, TEST_SIZE);

        // Train model and make predictions
        KNeighborsClassifier model;
        model.fit(split.X_train, split.y_train);
        Labels predictions = model.predict(split.X_test);
        
        auto [sensitivity, specificity] = evaluate(split.y_test, predictions);

        // Calculate corrects and incorrects
        int correct = 0, incorrect = 0;
        for (size_t i = 0; i < split.y_test.size(); ++i) {
            if (split.y_test[i] == predictions[i]) {
                correct++;
            } else {
                incorrect++;
            }
        }

        // Print results (Using C++23 std::println)
        std::println("Correct: {}", correct);
        std::println("Incorrect: {}", incorrect);
        std::println("True Positive Rate: {:.2f}%", 100.0 * sensitivity);
        std::println("True Negative Rate: {:.2f}%", 100.0 * specificity);

    } catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
        return 1;
    }

    return 0;
}

std::pair<std::vector<Evidence>, Labels> load_data(const std::string& filename) {
    /*
    Load shopping data from a CSV file `filename` and convert into a list of
    evidence lists and a list of labels. Return a pair (evidence, labels).
    */
    std::vector<Evidence> evidence;
    Labels labels;

    std::unordered_set<std::string> int_columns = {
        "Administrative", "Informational", "ProductRelated", "OperatingSystems", "Browser", "Region", "TrafficType"
    };
    std::unordered_set<std::string> string_column = {"VisitorType"};
    std::unordered_map<std::string, int> map_strings_to_numbers = {
        {"Returning_Visitor", 1}, {"New_Visitor", 0}, {"Other", 0}
    };
    std::unordered_set<std::string> bool_column = {"Weekend"};
    std::unordered_map<std::string, int> map_bools_to_numbers = {
        {"TRUE", 1}, {"FALSE", 0}, {"True", 1}, {"False", 0} 
    };
    std::unordered_set<std::string> month_column = {"Month"};
    std::unordered_map<std::string, int> map_months_to_numbers = {
        {"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3},
        {"May", 4}, {"June", 5}, {"Jul", 6}, {"Aug", 7},
        {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}
    };
    std::unordered_set<std::string> float_columns = {
        "Administrative_Duration", "Informational_Duration", "ProductRelated_Duration",
        "BounceRates", "ExitRates", "PageValues", "SpecialDay"
    };

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::string line;
    if (!std::getline(file, line)) return {evidence, labels}; // Empty file

    // Parse CSV header
    std::vector<std::string> headers;
    std::stringstream ss_header(line);
    std::string col;
    while (std::getline(ss_header, col, ',')) {
        if (!col.empty() && col.back() == '\r') col.pop_back(); // Handle Windows CRLF
        headers.push_back(col);
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss_row(line);
        std::vector<std::string> row;
        std::string val;
        while (std::getline(ss_row, val, ',')) {
            if (!val.empty() && val.back() == '\r') val.pop_back();
            row.push_back(val);
        }

        Evidence evidence_list;
        int label_int = 0;

        // Iterate through columns mirroring Python's `for key in row:` DictReader logic
        for (size_t i = 0; i < headers.size() && i < row.size(); ++i) {
            const auto& key = headers[i];
            const auto& value = row[i];

            if (int_columns.contains(key)) {
                evidence_list.push_back(std::stod(value)); // Stored as double for uniform vector
            } else if (string_column.contains(key)) {
                evidence_list.push_back(map_strings_to_numbers[value]);
            } else if (bool_column.contains(key)) {
                evidence_list.push_back(map_bools_to_numbers[value]);
            } else if (month_column.contains(key)) {
                evidence_list.push_back(map_months_to_numbers[value]);
            } else if (float_columns.contains(key)) {
                evidence_list.push_back(std::stod(value));
            } else {
                // If it falls into none of the sets, it's assumed to be the Label (Revenue)
                label_int = map_bools_to_numbers[value];
            }
        }
        evidence.push_back(evidence_list);
        labels.push_back(label_int);
    }

    return {evidence, labels};
}

SplitData train_test_split(const std::vector<Evidence>& evidence, const Labels& labels, double test_size) {
    // Generate an index array to shuffle
    std::vector<size_t> indices(labels.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Random shuffle using C++20 ranges
    std::random_device rd;
    std::mt19937 gen(rd());
    std::ranges::shuffle(indices, gen);

    size_t split_idx = static_cast<size_t>(labels.size() * (1.0 - test_size));

    SplitData split;
    split.X_train.reserve(split_idx);
    split.y_train.reserve(split_idx);
    split.X_test.reserve(labels.size() - split_idx);
    split.y_test.reserve(labels.size() - split_idx);

    // Distribute data based on the shuffled slice
    for (size_t i = 0; i < split_idx; ++i) {
        split.X_train.push_back(evidence[indices[i]]);
        split.y_train.push_back(labels[indices[i]]);
    }
    for (size_t i = split_idx; i < indices.size(); ++i) {
        split.X_test.push_back(evidence[indices[i]]);
        split.y_test.push_back(labels[indices[i]]);
    }

    return split;
}

std::pair<double, double> evaluate(const Labels& labels, const Labels& predictions) {
    /*
    Given a list of actual labels and a list of predicted labels,
    return a pair (sensitivity, specificity).
    */
    int positive = 0, negative = 0;
    int sensitivity_count = 0, specificity_count = 0;

    for (size_t i = 0; i < labels.size(); ++i) {
        if (labels[i] == 1) {
            positive++;
            if (predictions[i] == 1) {
                sensitivity_count++;
            }
        } else if (labels[i] == 0) {
            negative++;
            if (predictions[i] == 0) {
                specificity_count++;
            }
        }
    }

    double sensitivity = positive > 0 ? static_cast<double>(sensitivity_count) / positive : 0.0;
    double specificity = negative > 0 ? static_cast<double>(specificity_count) / negative : 0.0;

    return {sensitivity, specificity};
}
