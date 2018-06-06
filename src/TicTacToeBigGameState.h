#pragma once

#include "TicTacToeState.h"

namespace mcts
{
struct TicTacToeBigGameState : public TicTacToeState
{
    using Result = TicTacToeState::Result;
    using Cell = TicTacToeState::Cell;
    using BigBoard = std::vector< std::vector< TicTacToeState::Board > >;
    using ResultBoard = std::vector< std::vector< Result > >;

    TicTacToeBigGameState( BigBoard&& board, bool my_turn, Cell&& last_move = {-1, -1} );

private:
    virtual std::unique_ptr< TicTacToeState > clone( ) const override;
    virtual Result game_state( ) const override;
    virtual Moves get_possible_moves( ) const override;
    virtual void play_move( const Cell& cell ) override;

private:
    Cell get_board_coord( const Cell& cell ) const;
    Cell get_cell_coord( const Cell& cell ) const;
    void append_board_moves( Moves& moves, size_t row, size_t col ) const;

private:
    BigBoard m_board;
    ResultBoard m_results;
};

}  // namespace mcts