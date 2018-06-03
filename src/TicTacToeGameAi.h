#pragma once

#include <memory>
#include <vector>

namespace mcts
{
struct MctsNode;
using MctsNodePtr = std::shared_ptr< MctsNode >;

struct MovePosition
{
    size_t row;
    size_t col;
};

struct TicTacToeGameAI
{
    using AvailableCells = std::vector< std::vector< bool > >;

    TicTacToeGameAI( const AvailableCells& available, size_t iterations = 0 );

    void opponent_move( const MovePosition& position );
    MovePosition get_my_move( ) const;

private:
    const size_t m_iterations;
    MctsNodePtr m_tree;
    std::weak_ptr< MctsNode > m_current_node;
};
}  // namespace mcts
