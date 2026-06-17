#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <ranges>

// ============================================================================
// Search Node
// ============================================================================

template <typename State, typename Action>
struct Node {
    State state;
    std::shared_ptr<Node> parent;
    
    // std::optional is used because the root/initial node has no action that led to it
    std::optional<Action> action;

    Node(State state, std::shared_ptr<Node> parent, std::optional<Action> action = std::nullopt)
        : state(std::move(state)), parent(std::move(parent)), action(std::move(action)) {}
};

// ============================================================================
// Stack Frontier (Depth-First Search)
// ============================================================================

template <typename State, typename Action>
class StackFrontier {
protected:
    // std::deque is preferred over std::vector here because it allows O(1) 
    // removal from both the front and the back.
    std::deque<std::shared_ptr<Node<State, Action>>> frontier;

public:
    virtual ~StackFrontier() = default;

    void add(std::shared_ptr<Node<State, Action>> node) {
        frontier.push_back(std::move(node));
    }

    bool contains_state(const State& state) const {
        return std::ranges::any_of(frontier, [&state](const auto& node) {
            return node->state == state;
        });
    }

    bool empty() const {
        return frontier.empty();
    }

    // Virtual to allow QueueFrontier to override the removal behavior
    virtual std::shared_ptr<Node<State, Action>> remove() {
        if (empty()) {
            throw std::runtime_error("empty frontier");
        }
        
        // Stack behavior: Last-In, First-Out (LIFO)
        auto node = frontier.back();
        frontier.pop_back();
        return node;
    }
};

// ============================================================================
// Queue Frontier (Breadth-First Search)
// ============================================================================

template <typename State, typename Action>
class QueueFrontier : public StackFrontier<State, Action> {
public:
    std::shared_ptr<Node<State, Action>> remove() override {
        if (this->empty()) {
            throw std::runtime_error("empty frontier");
        }
        
        // Queue behavior: First-In, First-Out (FIFO)
        auto node = this->frontier.front();
        this->frontier.pop_front();
        return node;
    }
};
