#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <cctype> // C++ equivalent of <ctype.h>
#include <cmath>  // C++ equivalent of <math.h>

// Prototypes using std::string_view for lightweight string passing
int count_letters(std::string_view s);
int count_words(std::string_view s);
int count_sentences(std::string_view s);
int compute_index(int letter_c, int word_c, int sentence_c);

int main()
{
    // Get text from user
    std::print("Text: ");
    std::string text;
    std::getline(std::cin, text);

    // Count letters in text
    int letter_count = count_letters(text);

    // Count words in text
    int word_count = count_words(text);

    // Count sentences in text
    int sentence_count = count_sentences(text);

    // Compute Coleman-Liau index for text
    int grade = compute_index(letter_count, word_count, sentence_count);

    // Print grade level for text
    if (grade < 1)
    {
        std::println("Before Grade 1");
    }
    else if (grade >= 16)
    {
        std::println("Grade 16+");
    }
    else
    {
        std::println("Grade {}", grade);
    }
}

int count_letters(std::string_view s)
{
    int count = 0;
    // Range-based for loop makes character traversal much cleaner
    for (char c : s)
    {
        if (std::isalpha(c))
        {
            count++;
        }
    }
    return count;
}

int count_words(std::string_view s)
{
    if (s.empty())
    {
        return 0;
    }

    // Since words are separated by spaces, there is always 
    // 1 more word than there are spaces.
    int count = 1; 
    for (char c : s)
    {
        if (c == ' ')
        {
            count++;
        }
    }
    return count;
}

int count_sentences(std::string_view s)
{
    int count = 0;
    for (char c : s)
    {
        if (c == '.' || c == '!' || c == '?')
        {
            count++;
        }
    }
    return count;
}

int compute_index(int letter_c, int word_c, int sentence_c)
{
    // Use static_cast instead of C-style casting (float)
    float L = static_cast<float>(letter_c) / static_cast<float>(word_c) * 100.0f;
    float S = static_cast<float>(sentence_c) / static_cast<float>(word_c) * 100.0f;
    
    // The 'f' suffix ensures these literals are treated strictly as floats
    float index = 0.0588f * L - 0.296f * S - 15.8f;
    
    // std::round returns a float/double, so we cast it back to an int
    return static_cast<int>(std::round(index));
}
