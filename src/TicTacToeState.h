#pragma once

#include "MctsState.h"

#include <array>

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
        int row;
        int col;
    };

    using Board = std::vector< std::vector< CellState > >;
    using Moves = std::vector< Cell >;

    TicTacToeState( Board&& board, bool my_turn, Cell&& last_move = {-1, -1} );

public:
    Result simulate( ) const override;
    ChildrenPtr get_children( MctsNodePtr parent ) const override;
    const Cell& get_last_move( ) const;

private:
    virtual std::unique_ptr< TicTacToeState > clone( ) const;
    virtual Result game_state( ) const;
    virtual Moves get_possible_moves( ) const;
    virtual void play_move( const Cell& cell );

protected:
    template < typename TBoard = Board,
               typename StateType = CellState,
               StateType Available = CellState::e_Cell_Available,
               StateType Mine = CellState::e_Cell_Mine,
               StateType Opponent = CellState::e_Cell_Opponent >
    static Result game_state( TBoard const& brd );
    static Moves get_possible_moves( Board const& brd );

protected:
    Board m_board;
    bool m_my_turn;
    Cell m_last_move;
};

template < typename TBoard,
           typename StateType,
           StateType Available,
           StateType Mine,
           StateType Opponent >
MctsState::Result
TicTacToeState::game_state( TBoard const& brd )
{
    auto const sz = brd.size( );

    auto cnt_available = 0;

    // Check rows and cols
    for ( size_t i = 0; i < sz; ++i )
    {
        auto cnt_row_mine = 0;
        auto cnt_row_opponent = 0;
        auto cnt_col_mine = 0;
        auto cnt_col_opponent = 0;
        for ( size_t j = 0; j < sz; ++j )
        {
            // Row
            switch ( brd[ i ][ j ] )
            {
            case Mine:
                ++cnt_row_mine;
                break;
            case Opponent:
                ++cnt_row_opponent;
                break;
            case Available:
                ++cnt_available;
                break;
            }
            // Col
            switch ( brd[ j ][ i ] )
            {
            case Mine:
                ++cnt_col_mine;
                break;
            case Opponent:
                ++cnt_col_opponent;
                break;
            }
        }

        if ( cnt_row_mine == sz || cnt_col_mine == sz )
        {
            return Result::e_Result_Hit;
        }

        if ( cnt_row_opponent == sz || cnt_col_opponent == sz )
        {
            return Result::e_Result_Miss;
        }
    }

    // Check diagonals
    auto cnt_diagonal1_mine = 0;
    auto cnt_diagonal1_opponent = 0;
    auto cnt_diagonal2_mine = 0;
    auto cnt_diagonal2_opponent = 0;

    for ( size_t i = 0; i < sz; ++i )
    {
        switch ( brd[ i ][ i ] )
        {
        case Mine:
            ++cnt_diagonal1_mine;
            break;
        case Opponent:
            ++cnt_diagonal1_opponent;
            break;
        }
        switch ( brd[ i ][ sz - i - 1 ] )
        {
        case Mine:
            ++cnt_diagonal2_mine;
            break;
        case Opponent:
            ++cnt_diagonal2_opponent;
            break;
        }
    }

    if ( cnt_diagonal1_mine == sz || cnt_diagonal2_mine == sz )
    {
        return Result::e_Result_Hit;
    }

    if ( cnt_diagonal1_opponent == sz || cnt_diagonal2_opponent == sz )
    {
        return Result::e_Result_Miss;
    }

    return cnt_available > 0 ? Result::e_Result_NotFinished : Result::e_Result_Draw;
}

}  // namespace mcts