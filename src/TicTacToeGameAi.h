#pragma once

#include "TicTacToeBigGameState.h"

#include <memory>
#include <vector>

namespace mcts
{
struct MctsNode;
using MctsNodePtr = std::shared_ptr< MctsNode >;

struct MovePosition
{
    int row;
    int col;
};

struct TicTacToeGameAI
{
    static const size_t SMALL_BOARD_SIZE = 3;
    static const size_t BIG_BOARD_SIZE = SMALL_BOARD_SIZE * SMALL_BOARD_SIZE;

    using AvailableCells = std::vector< std::vector< bool > >;

    TicTacToeGameAI( const AvailableCells& available, size_t iterations = 0 );

    void opponent_move( const MovePosition& position );
    MovePosition get_my_move( ) const;

private:
    static TicTacToeState::Board
        create_small_board(const AvailableCells& available);

    static TicTacToeBigGameState::BigBoard
        create_big_board(const AvailableCells& available);

private:
    const size_t m_iterations;
    MctsNodePtr m_tree;
    std::weak_ptr< MctsNode > m_current_node;
};
}  // namespace mcts
