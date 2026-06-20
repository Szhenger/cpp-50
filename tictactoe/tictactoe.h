#include <array>
#include <vector>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <ranges>

namespace tictactoe {

    // Define the Player states equivalent to Python's X, O, and EMPTY
    enum class Player {
        EMPTY,
        X,
        O
    };

    // Type aliases for easier reading
    using Board = std::array<std::array<Player, 3>, 3>;
    using Action = std::pair<int, int>;

    /**
     * Returns starting state of the board.
     */
    Board initial_state() {
        return {{{{Player::EMPTY, Player::EMPTY, Player::EMPTY},
                  {Player::EMPTY, Player::EMPTY, Player::EMPTY},
                  {Player::EMPTY, Player::EMPTY, Player::EMPTY}}}};
    }

    // Forward declaration needed for player()
    bool terminal(const Board& board);

    /**
     * Returns player who has the next turn on a board.
     * Returns std::nullopt if the board is terminal (game over).
     */
    std::optional<Player> player(const Board& board) {
        // Check whether board is in play
        if (terminal(board)) {
            return std::nullopt;
        }

        // Iterate over board to count number of X and O
        int x_size = 0;
        int o_size = 0;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == Player::X) {
                    x_size++;
                } else if (board[i][j] == Player::O) {
                    o_size++;
                }
            }
        }

        // Return player who has next turn
        if (x_size == o_size) {
            return Player::X;
        } else {
            return Player::O;
        }
    }

    /**
     * Returns vector of all possible actions (i, j) available on the board.
     * Replaces Python's set to maintain idiomatic, fast C++ performance.
     */
    std::vector<Action> actions(const Board& board) {
        // Check whether board is in play
        if (terminal(board)) {
            return {};
        }

        // Iterate over the board to find empty cells
        std::vector<Action> empty_cells;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == Player::EMPTY) {
                    empty_cells.emplace_back(i, j);
                }
            }
        }

        return empty_cells;
    }

    /**
     * Returns the board that results from making move (i, j) on the board.
     * Passing `board` by value acts as Python's `copy.deepcopy(board)`.
     */
    Board result(Board board, Action action) {
        // Ensure action is valid using C++20 ranges
        auto valid_actions = actions(board);
        if (std::ranges::find(valid_actions, action) == valid_actions.end()) {
            throw std::invalid_argument("Invalid action");
        }

        // Make action on the board
        std::optional<Player> current_player = player(board);
        if (current_player.has_value()) {
            board[action.first][action.second] = current_player.value();
        }

        // Return resulting board from action
        return board;
    }

    /**
     * Returns the winner of the game, if there is one.
     */
    std::optional<Player> winner(const Board& board) {
        // Check for row winner
        for (int i = 0; i < 3; ++i) {
            if (board[i][0] != Player::EMPTY && board[i][0] == board[i][1] && board[i][0] == board[i][2]) {
                return board[i][0];
            }
        }

        // Check for column winner
        for (int i = 0; i < 3; ++i) {
            if (board[0][i] != Player::EMPTY && board[0][i] == board[1][i] && board[0][i] == board[2][i]) {
                return board[0][i];
            }
        }

        // Check for diagonal winner
        if (board[2][2] != Player::EMPTY && board[0][0] == board[1][1] && board[0][0] == board[2][2]) {
            return board[2][2];
        }

        // Check for anti-diagonal winner
        if (board[2][0] != Player::EMPTY && board[0][2] == board[1][1] && board[0][2] == board[2][0]) {
            return board[0][2];
        }

        // No winner at present
        return std::nullopt;
    }

    /**
     * Returns true if game is over, false otherwise.
     */
    bool terminal(const Board& board) {
        // Check for winner
        if (winner(board).has_value()) {
            return true;
        }

        // Check for empty cells
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (board[i][j] == Player::EMPTY) {
                    return false;
                }
            }
        }

        // Else the game tied
        return true;
    }

    /**
     * Returns 1 if X has won the game, -1 if O has won, 0 otherwise.
     */
    int utility(const Board& board) {
        // Get winner of board
        std::optional<Player> champion = winner(board);

        // Find who is the winner
        if (champion == Player::X) {
            return 1;
        } else if (champion == Player::O) {
            return -1;
        } else {
            return 0;
        }
    }

    /**
     * Returns the optimal action for the current player on the board.
     */
    std::optional<Action> minimax(const Board& board) {
        // Check whether game ended
        if (terminal(board)) {
            return std::nullopt;
        }

        // Get utilities for all moves
        Player current_player = player(board).value();
        auto available_moves = actions(board);

        // Nested recursive lambda using C++23 'Deducing this'
        auto play = [&](this auto const& self, const Board& b) -> int {
            // Check whether game ended
            if (terminal(b)) {
                return utility(b);
            }

            // Get utilities for each move
            auto next_moves = actions(b);
            if (player(b) == current_player) {
                int max_val = -10000;
                for (const auto& move : next_moves) {
                    max_val = std::max(max_val, self(result(b, move)));
                }
                return max_val;
            } else {
                int min_val = 10000;
                for (const auto& move : next_moves) {
                    min_val = std::min(min_val, self(result(b, move)));
                }
                return min_val;
            }
        };

        // Get optimal action
        std::optional<Action> best_move = std::nullopt;

        if (current_player == Player::X) {
            int best_score = -10000;
            for (const auto& move : available_moves) {
                int score = play(result(board, move));
                if (score > best_score) {
                    best_score = score;
                    best_move = move;
                }
            }
        } else {
            int best_score = 10000;
            for (const auto& move : available_moves) {
                int score = play(result(board, move));
                if (score < best_score) {
                    best_score = score;
                    best_move = move;
                }
            }
        }

        return best_move;
    }

} // namespace tictactoe
