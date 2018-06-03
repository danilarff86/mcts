#pragma once

#include "MctsState.h"

namespace mcts
{

struct TicTacToeState : public MctsState
{

    enum class CellState
    {
        e_Cell_Available = 0,
        e_Cell_Mine,
        e_Cell_Opponent
    };

    struct Cell
    {
        size_t row;
        size_t col;
    };

    using Board = std::vector< std::vector< CellState > >;
    using Moves = std::vector< Cell >;

    TicTacToeState( Board&& board, bool my_turn, Cell&& last_move = {} );

    Result simulate( ) const override;
    ChildrenPtr get_children( MctsNodePtr parent ) const override;
    const Cell& get_last_move( ) const;

private:
    Result game_state( TicTacToeState const& state ) const;
    static Moves get_possible_moves( TicTacToeState const& state );
    static void play_move( TicTacToeState& state, const Cell& cell );

private:
    Board m_board;
    bool m_my_turn;
    Cell m_last_move;
};

}