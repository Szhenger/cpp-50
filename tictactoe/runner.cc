#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <array>
#include <vector>
#include <thread>
#include <chrono>
#include <optional>
#include <format>
#include <stdexcept>

// ---------------------------------------------------------
// Tic-Tac-Toe Game Logic (Equivalent to tictactoe.py)
// ---------------------------------------------------------
namespace ttt {
    enum Player { EMPTY = 0, X = 1, O = 2 };
    using Board = std::array<std::array<Player, 3>, 3>;
    using Move = std::pair<int, int>;

    Board initial_state() {
        return {{{{EMPTY, EMPTY, EMPTY},
                  {EMPTY, EMPTY, EMPTY},
                  {EMPTY, EMPTY, EMPTY}}}};
    }

    Player player(const Board& board) {
        int x_count = 0, o_count = 0;
        for (const auto& row : board) {
            for (auto p : row) {
                if (p == X) x_count++;
                else if (p == O) o_count++;
            }
        }
        return (x_count <= o_count) ? X : O;
    }

    Player winner(const Board& board) {
        for (int i = 0; i < 3; ++i) {
            // Check rows and columns
            if (board[i][0] != EMPTY && board[i][0] == board[i][1] && board[i][1] == board[i][2]) return board[i][0];
            if (board[0][i] != EMPTY && board[0][i] == board[1][i] && board[1][i] == board[2][i]) return board[0][i];
        }
        // Check diagonals
        if (board[0][0] != EMPTY && board[0][0] == board[1][1] && board[1][1] == board[2][2]) return board[0][0];
        if (board[0][2] != EMPTY && board[0][2] == board[1][1] && board[1][1] == board[2][0]) return board[0][2];
        return EMPTY;
    }

    bool terminal(const Board& board) {
        if (winner(board) != EMPTY) return true;
        for (const auto& row : board) {
            for (auto p : row) {
                if (p == EMPTY) return false;
            }
        }
        return true;
    }

    int utility(const Board& board) {
        Player w = winner(board);
        if (w == X) return 1;
        if (w == O) return -1;
        return 0;
    }

    Board result(Board board, Move move) {
        if (board[move.first][move.second] != EMPTY) {
            throw std::invalid_argument("Invalid move");
        }
        board[move.first][move.second] = player(board);
        return board;
    }

    int minimax_helper(const Board& board, bool is_maximizing) {
        if (terminal(board)) return utility(board);

        int best = is_maximizing ? -1000 : 1000;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == EMPTY) {
                    Board new_board = result(board, {i, j});
                    if (is_maximizing) {
                        best = std::max(best, minimax_helper(new_board, false));
                    } else {
                        best = std::min(best, minimax_helper(new_board, true));
                    }
                }
            }
        }
        return best;
    }

    Move minimax(const Board& board) {
        if (terminal(board)) return {-1, -1};
        
        Player curr_player = player(board);
        int best_val = (curr_player == X) ? -1000 : 1000;
        Move best_move = {-1, -1};

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == EMPTY) {
                    Board new_board = result(board, {i, j});
                    int val = minimax_helper(new_board, curr_player == O); 
                    
                    if (curr_player == X && val > best_val) {
                        best_val = val;
                        best_move = {i, j};
                    } else if (curr_player == O && val < best_val) {
                        best_val = val;
                        best_move = {i, j};
                    }
                }
            }
        }
        return best_move;
    }

    std::string to_string(Player p) {
        if (p == X) return "X";
        if (p == O) return "O";
        return "";
    }
}

// ---------------------------------------------------------
// GUI Helper Functions
// ---------------------------------------------------------
void centerTextInRect(sf::Text& text, const sf::FloatRect& rect) {
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setOrigin(textBounds.left + textBounds.width / 2.0f,
                   textBounds.top + textBounds.height / 2.0f);
    text.setPosition(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
}

void centerTextAtPoint(sf::Text& text, float x, float y) {
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setOrigin(textBounds.left + textBounds.width / 2.0f,
                   textBounds.top + textBounds.height / 2.0f);
    text.setPosition(x, y);
}

// ---------------------------------------------------------
// Main GUI Application (Equivalent to Pygame logic)
// ---------------------------------------------------------
int main() {
    const int width = 600;
    const int height = 400;

    sf::RenderWindow window(sf::VideoMode(width, height), "Tic-Tac-Toe");

    // Colors
    sf::Color black(0, 0, 0);
    sf::Color white(255, 255, 255);

    // Fonts
    sf::Font font;
    // Note: Make sure "OpenSans-Regular.ttf" is in the exact same directory as your executable
    if (!font.loadFromFile("OpenSans-Regular.ttf")) {
        std::cerr << "Error loading OpenSans-Regular.ttf. Using default system behavior.\n";
        return 1;
    }

    std::optional<ttt::Player> user = std::nullopt;
    ttt::Board board = ttt::initial_state();
    bool ai_turn = false;

    // Pre-allocate shapes and texts
    sf::Text titleText("", font, 40);
    titleText.setFillColor(white);

    sf::RectangleShape playXButton(sf::Vector2f(width / 4.0f, 50.0f));
    playXButton.setPosition(width / 8.0f, height / 2.0f);
    playXButton.setFillColor(white);

    sf::Text playXText("Play as X", font, 28);
    playXText.setFillColor(black);
    centerTextInRect(playXText, playXButton.getGlobalBounds());

    sf::RectangleShape playOButton(sf::Vector2f(width / 4.0f, 50.0f));
    playOButton.setPosition(5.0f * (width / 8.0f), height / 2.0f);
    playOButton.setFillColor(white);

    sf::Text playOText("Play as O", font, 28);
    playOText.setFillColor(black);
    centerTextInRect(playOText, playOButton.getGlobalBounds());

    sf::RectangleShape againButton(sf::Vector2f(width / 3.0f, 50.0f));
    againButton.setPosition(width / 3.0f, height - 65.0f);
    againButton.setFillColor(white);

    sf::Text againText("Play Again", font, 28);
    againText.setFillColor(black);
    centerTextInRect(againText, againButton.getGlobalBounds());

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(black);

        // Let user choose a player
        if (!user.has_value()) {
            titleText.setString("Play Tic-Tac-Toe");
            centerTextAtPoint(titleText, width / 2.0f, 50.0f);
            window.draw(titleText);

            window.draw(playXButton);
            window.draw(playXText);

            window.draw(playOButton);
            window.draw(playOText);

            // Check if button is clicked
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (playXButton.getGlobalBounds().contains(mousePos)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    user = ttt::X;
                } else if (playOButton.getGlobalBounds().contains(mousePos)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    user = ttt::O;
                }
            }
        } else {
            // Draw game board
            float tile_size = 80.0f;
            sf::Vector2f tile_origin(width / 2.0f - (1.5f * tile_size),
                                     height / 2.0f - (1.5f * tile_size));
            
            std::array<std::array<sf::FloatRect, 3>, 3> tiles;

            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    sf::RectangleShape rect(sf::Vector2f(tile_size, tile_size));
                    rect.setPosition(tile_origin.x + j * tile_size, tile_origin.y + i * tile_size);
                    rect.setFillColor(black);
                    rect.setOutlineColor(white);
                    rect.setOutlineThickness(3.0f);
                    window.draw(rect);

                    tiles[i][j] = rect.getGlobalBounds();

                    if (board[i][j] != ttt::EMPTY) {
                        sf::Text moveText(ttt::to_string(board[i][j]), font, 60);
                        moveText.setFillColor(white);
                        centerTextInRect(moveText, tiles[i][j]);
                        window.draw(moveText);
                    }
                }
            }

            bool game_over = ttt::terminal(board);
            ttt::Player curr_player = ttt::player(board);

            // Show title
            if (game_over) {
                ttt::Player winner = ttt::winner(board);
                if (winner == ttt::EMPTY) {
                    titleText.setString("Game Over: Tie.");
                } else {
                    titleText.setString(std::format("Game Over: {} wins.", ttt::to_string(winner)));
                }
            } else if (user.value() == curr_player) {
                titleText.setString(std::format("Play as {}", ttt::to_string(user.value())));
            } else {
                titleText.setString("Computer thinking...");
            }
            centerTextAtPoint(titleText, width / 2.0f, 30.0f);
            window.draw(titleText);

            // Check for AI move
            if (user.value() != curr_player && !game_over) {
                if (ai_turn) {
                    // Update display before thread sleep so "Computer thinking..." renders
                    window.display(); 
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    ttt::Move move = ttt::minimax(board);
                    board = ttt::result(board, move);
                    ai_turn = false;
                    continue; // Skip flip to prevent flickering after computation
                } else {
                    ai_turn = true;
                }
            }

            // Check for a user move
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && user.value() == curr_player && !game_over) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        if (board[i][j] == ttt::EMPTY && tiles[i][j].contains(mousePos)) {
                            board = ttt::result(board, {i, j});
                        }
                    }
                }
            }

            // Game over screen buttons
            if (game_over) {
                window.draw(againButton);
                window.draw(againText);

                if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    if (againButton.getGlobalBounds().contains(mousePos)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        user = std::nullopt;
                        board = ttt::initial_state();
                        ai_turn = false;
                    }
                }
            }
        }

        window.display();
    }

    return 0;
}
