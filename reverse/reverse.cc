// Reverses a WAV file's audio samples, writing a new WAV file.
// C++23 translation of reverse.c

#include "wav.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

// Returns true when the header's format field is "WAVE".
static bool check_format(const WAVHEADER& header)
{
    constexpr BYTE marker[4] = {'W', 'A', 'V', 'E'};
    for (int i = 0; i < 4; ++i)
    {
        if (header.format[i] != marker[i])
            return false;
    }
    return true;
}

// Returns the size in bytes of one audio block (one sample across all channels).
static int get_block_size(const WAVHEADER& header)
{
    return header.numChannels * header.bitsPerSample / 8;
}

int main(int argc, char* argv[])
{
    // Ensure proper usage
    if (argc != 3)
    {
        std::cerr << "Usage: ./reverse input.wav output.wav\n";
        return 1;
    }

    // Open input file for reading in binary mode
    // (the original used "r" — text mode — which is a bug on Windows;
    //  std::ios::binary is always correct for WAV files)
    std::ifstream input(argv[1], std::ios::binary);
    if (!input)
    {
        std::cerr << "Error: Cannot open file.\n";
        return 1;
    }

    // Read header
    WAVHEADER header{};
    input.read(reinterpret_cast<char*>(&header), sizeof(WAVHEADER));
    if (!input)
    {
        std::cerr << "Error: Could not read WAV header.\n";
        return 1;
    }

    // Validate WAV format
    if (!check_format(header))
    {
        std::cerr << "Error: Input is not a WAV file.\n";
        return 1;
    }

    // Open output file for writing in binary mode
    std::ofstream output(argv[2], std::ios::binary);
    if (!output)
    {
        std::cerr << "Error: Cannot open file.\n";
        return 1;
    }

    // Write header to output
    output.write(reinterpret_cast<const char*>(&header), sizeof(WAVHEADER));

    // Calculate block size (bytes per multi-channel sample)
    const int block_size = get_block_size(header);

    // Determine how many complete blocks are in the audio data
    input.seekg(0, std::ios::end);
    const long file_size  = static_cast<long>(input.tellg());
    const long audio_size = file_size - static_cast<long>(sizeof(WAVHEADER));
    const int  num_blocks = static_cast<int>(audio_size / block_size);

    // Buffer for one block — std::vector replaces the VLA in the original
    std::vector<char> buffer(static_cast<std::size_t>(block_size));

    // Write blocks in reverse order
    for (int i = 0; i < num_blocks; ++i)
    {
        const long offset = static_cast<long>(sizeof(WAVHEADER))
                          + static_cast<long>(num_blocks - (i + 1)) * block_size;
        input.seekg(offset, std::ios::beg);
        input.read(buffer.data(), block_size);
        output.write(buffer.data(), block_size);
    }

    // Files are closed automatically by their destructors (RAII)
    return 0;
}
