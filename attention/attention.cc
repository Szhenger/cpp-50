#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <format>
#include <cmath>
#include <stdexcept>

// OpenCV for image generation
#include <opencv2/opencv.hpp>
// LibTorch for Deep Learning Inference
#include <torch/torch.h>
#include <torch/script.h> 

// ---------------------------------------------------------
// Hyperparameters & Constants
// ---------------------------------------------------------
constexpr int K = 3;
constexpr int GRID_SIZE = 40;
constexpr int PIXELS_PER_WORD = 200;
const int MASK_TOKEN_ID = 103; // Default [MASK] id for bert-base-uncased

// ---------------------------------------------------------
// Mock Tokenizer (Represents tokenizers-cpp)
// ---------------------------------------------------------
// In a real C++ application, you would link against HuggingFace's tokenizers-cpp.
// For translation completeness, we stub the interface here.
struct Tokenizer {
    int mask_token_id = MASK_TOKEN_ID;
    std::string mask_token = "[MASK]";

    std::vector<int> encode(const std::string& text) {
        // Mock tokenization logic
        return {101, 2023, 2003, 103, 1012, 102}; // [CLS] this is [MASK] . [SEP]
    }

    std::vector<std::string> get_tokens(const std::string& text) {
        return {"[CLS]", "this", "is", "[MASK]", ".", "[SEP]"};
    }

    std::string decode(int token_id) {
        return "example"; // Mock decoded token
    }
};

// ---------------------------------------------------------
// Helper Functions
// ---------------------------------------------------------
std::optional<size_t> get_mask_token_index(int mask_token_id, const std::vector<int>& input_ids) {
    auto it = std::find(input_ids.begin(), input_ids.end(), mask_token_id);
    if (it != input_ids.end()) {
        return std::distance(input_ids.begin(), it);
    }
    return std::nullopt;
}

cv::Scalar get_color_for_attention_score(float attention_score) {
    // Return a grayscale color [0, 255]. OpenCV uses BGR format, so all 3 channels get the same value.
    int color_score = static_cast<int>(std::round(255.0f * attention_score));
    return cv::Scalar(color_score, color_score, color_score);
}

void replace_substring(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos != std::string::npos) {
        str.replace(start_pos, from.length(), to);
    }
}

// ---------------------------------------------------------
// Attention Visualization Logic
// ---------------------------------------------------------
void generate_diagram(int layer_number, int head_number, const std::vector<std::string>& tokens, torch::Tensor attention_weights) {
    int image_size = GRID_SIZE * tokens.size() + PIXELS_PER_WORD;
    
    // Create new black image (CV_8UC3 represents 8-bit unsigned, 3 channels)
    cv::Mat img = cv::Mat::zeros(image_size, image_size, CV_8UC3);
    
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.8;
    int thickness = 1;
    cv::Scalar white(255, 255, 255);

    // Draw each token onto the image
    for (size_t i = 0; i < tokens.size(); ++i) {
        // Draw token columns (Rotated 90 degrees)
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(tokens[i], fontFace, fontScale, thickness, &baseline);
        
        cv::Mat textImg = cv::Mat::zeros(textSize.width, textSize.height + baseline, CV_8UC3);
        cv::Point textOrg(0, textSize.height); // Standard horizontal draw
        
        // Draw text horizontally on temp buffer, then rotate
        cv::Mat tempTextImg = cv::Mat::zeros(textSize.height + baseline, textSize.width, CV_8UC3);
        cv::putText(tempTextImg, tokens[i], textOrg, fontFace, fontScale, white, thickness);
        cv::rotate(tempTextImg, textImg, cv::ROTATE_90_COUNTERCLOCKWISE);

        // Calculate ROI and paste
        int col_x = image_size - PIXELS_PER_WORD;
        int col_y = PIXELS_PER_WORD + i * GRID_SIZE;
        cv::Rect col_roi(col_x, col_y, textImg.cols, textImg.rows);
        if ((col_roi.x + col_roi.width <= img.cols) && (col_roi.y + col_roi.height <= img.rows)) {
            textImg.copyTo(img(col_roi));
        }

        // Draw token rows
        int row_x = PIXELS_PER_WORD - textSize.width - 10; // Padding
        int row_y = PIXELS_PER_WORD + i * GRID_SIZE + (GRID_SIZE + textSize.height) / 2;
        cv::putText(img, tokens[i], cv::Point(row_x, row_y), fontFace, fontScale, white, thickness);
    }

    // Access raw tensor data for fast plotting
    auto attentions_accessor = attention_weights.accessor<float, 2>();

    // Draw each word's attention grid
    for (size_t i = 0; i < tokens.size(); ++i) {
        int y = PIXELS_PER_WORD + i * GRID_SIZE;
        for (size_t j = 0; j < tokens.size(); ++j) {
            int x = PIXELS_PER_WORD + j * GRID_SIZE;
            
            float weight = attentions_accessor[i][j];
            cv::Scalar color = get_color_for_attention_score(weight);
            
            // Draw filled rectangle
            cv::Rect cell(x, y, GRID_SIZE, GRID_SIZE);
            cv::rectangle(img, cell, color, cv::FILLED);
        }
    }

    // Save image
    std::string filename = std::format("Attention_Layer{}_Head{}.png", layer_number + 1, head_number + 1);
    cv::imwrite(filename, img);
}

void visualize_attentions(const std::vector<std::string>& tokens, torch::Tensor attentions) {
    // Assuming attentions shape is [Num_Layers, Batch_Size, Num_Heads, Seq_Len, Seq_Len]
    int num_layers = attentions.size(0);
    int num_heads = attentions.size(2);

    for (int i = 0; i < num_layers; ++i) {
        for (int j = 0; j < num_heads; ++j) {
            // Extract the 2D tensor for a specific layer and head (Batch 0)
            torch::Tensor head_attention = attentions[i][0][j].cpu();
            generate_diagram(i, j, tokens, head_attention);
        }
    }
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main() {
    std::string text;
    std::cout << "Text: ";
    std::getline(std::cin, text);

    Tokenizer tokenizer;
    
    // Tokenize input
    std::vector<int> input_ids_vec = tokenizer.encode(text);
    std::vector<std::string> tokens = tokenizer.get_tokens(text);
    
    auto mask_token_index = get_mask_token_index(tokenizer.mask_token_id, input_ids_vec);
    if (!mask_token_index.has_value()) {
        std::cerr << std::format("Input must include mask token {}.\n", tokenizer.mask_token);
        return EXIT_FAILURE;
    }

    // Convert to LibTorch Tensors
    auto input_tensor = torch::tensor(input_ids_vec, torch::kInt64).unsqueeze(0); // Add batch dimension
    
    // Load pre-trained and traced model
    torch::jit::script::Module model;
    try {
        // You must pre-export your BERT model from Python using torch.jit.trace
        model = torch::jit::load("bert_masked_lm.pt");
    } catch (const c10::Error& e) {
        std::cerr << "Error loading the model. Ensure 'bert_masked_lm.pt' exists in the directory.\n";
        return EXIT_FAILURE;
    }

    // Run model inference
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input_tensor);
    
    // Assuming the traced model returns a tuple: (logits, attentions)
    auto output = model.forward(inputs).toTuple();
    torch::Tensor logits = output->elements()[0].toTensor();     // Shape: [1, Seq_Len, Vocab_Size]
    torch::Tensor attentions = output->elements()[1].toTensor(); // Shape: [Layers, 1, Heads, Seq, Seq]

    // Generate predictions
    torch::Tensor mask_token_logits = logits[0][mask_token_index.value()];
    
    // Get Top K tokens
    auto top_k_result = torch::topk(mask_token_logits, K);
    torch::Tensor top_tokens_tensor = std::get<1>(top_k_result); // Get indices
    
    // Print predictions
    auto top_tokens_accessor = top_tokens_tensor.accessor<int64_t, 1>();
    for (int i = 0; i < K; ++i) {
        std::string decoded_token = tokenizer.decode(top_tokens_accessor[i]);
        std::string predicted_text = text;
        replace_substring(predicted_text, tokenizer.mask_token, decoded_token);
        std::cout << predicted_text << "\n";
    }

    // Visualize attentions
    visualize_attentions(tokens, attentions);

    return EXIT_SUCCESS;
}
