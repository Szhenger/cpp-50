#include "nim.h"

int main() {
    // Train AI for 10000 games
    NimAI ai = train(10000);

    // Play human game against the AI
    play(ai);

    return 0;
}
