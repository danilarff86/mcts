#pragma once

#include <memory>
#include <vector>

namespace mcts
{
struct State;
struct MctsNode;

using StatePtr = std::shared_ptr< State >;
using MctsNodePtr = std::shared_ptr< MctsNode >;

using AvailableCells = std::vector< std::vector< bool > >;

struct TicTacToeGameAI
{
    struct Cell
    {
        size_t row;
        size_t col;
    };

    TicTacToeGameAI( const AvailableCells& available );

    void opponent_move( const Cell& cell );

    Cell get_my_move( ) const;

private:
    MctsNodePtr m_tree;
    std::weak_ptr< MctsNode > m_current_node;
};
}