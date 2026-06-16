#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <cctype> // The C++ version of <ctype.h>

// Prototypes
bool check_key(int argc, char* argv[]);
void print_cipher_text(std::string_view text, std::string_view cipher);

int main(int argc, char* argv[])
{
    // Checks whether key is valid
    if (check_key(argc, argv))
    {
        // Get plaintext from user
        std::print("plaintext:  ");
        std::string plaintext;
        std::getline(std::cin, plaintext);

        // Print ciphertext
        print_cipher_text(plaintext, argv[1]);

        return 0;
    }
    // Return error otherwise
    else
    {
        return 1;
    }
}

bool check_key(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::println("Usage: {} key", argv[0]);
        return false;
    }

    // Convert the C-style array to a modern string_view for easy length checking
    std::string_view key = argv[1];

    if (key.length() != 26)
    {
        std::println("Key must contain 26 characters.");
        return false;
    }

    for (size_t i = 0; i < key.length(); i++)
    {
        if (!std::isalpha(key[i]))
        {
            std::println("Key must contain only letters.");
            return false;
        }
        for (size_t j = 0; j < key.length(); j++)
        {
            if (i != j && std::tolower(key[i]) == std::tolower(key[j]))
            {
                std::println("Key must contain each letter exactly once.");
                return false;
            }
        }
    }
    return true;
}

void print_cipher_text(std::string_view text, std::string_view cipher)
{
    // Build the final string instead of printing character-by-character
    std::string result;
    result.reserve(text.length()); // Optimizes memory allocation

    // Range-based for loop: "For each character 'c' in 'text'"
    for (char c : text)
    {
        if (std::isalpha(c))
        {
            if (std::islower(c))
            {
                int index = c - 'a'; // 'a' is equivalent to 97, but more readable
                result += static_cast<char>(std::tolower(cipher[index]));
            }
            else
            {
                int index = c - 'A'; // 'A' is equivalent to 65
                result += static_cast<char>(std::toupper(cipher[index]));
            }
        }
        else
        {
            result += c;
        }
    }
    
    // Print the fully built string at once
    std::println("ciphertext: {}", result);
}
