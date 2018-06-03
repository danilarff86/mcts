#include "TicTacToeState.h"
#include "MctsNode.h"

namespace mcts
{
TicTacToeState::TicTacToeState( TicTacToeState::Board&& board,
                                bool my_turn,
                                TicTacToeState::Cell&& last_move /*= nullptr */ )
    : m_board( std::move( board ) )
    , m_my_turn( my_turn )
    , m_last_move( std::move( last_move ) )
{
}

MctsState::Result
TicTacToeState::simulate( ) const
{
    TicTacToeState temp_state = *this;

    Result result = Result::e_Result_NotFinished;

    while ( ( result = game_state( temp_state ) ) == Result::e_Result_NotFinished )
    {
        auto possible_moves = get_possible_moves( temp_state );

        play_move( temp_state, possible_moves[ rand( ) % possible_moves.size( ) ] );
    }

    return result;
}

ChildrenPtr
TicTacToeState::get_children( MctsNodePtr parent ) const
{
    auto possible_moves = get_possible_moves( *this );

    auto possible_children = ChildrenPtr( new Children );
    possible_children->reserve( possible_moves.size( ) );

    for ( auto const& possible_move : possible_moves )
    {
        std::unique_ptr< TicTacToeState > temp_state( new TicTacToeState( *this ) );
        play_move( *temp_state, possible_move );
        possible_children->push_back(
            std::make_shared< MctsNode >( std::move( temp_state ), parent ) );
    }

    return std::move( possible_children );
}

const TicTacToeState::Cell&
TicTacToeState::get_last_move( ) const
{
    return m_last_move;
}

MctsState::Result
TicTacToeState::game_state( TicTacToeState const& state ) const
{
    auto const& brd = state.m_board;
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
            case CellState::e_Cell_Mine:
                ++cnt_row_mine;
                break;
            case CellState::e_Cell_Opponent:
                ++cnt_row_opponent;
                break;
            case CellState::e_Cell_Available:
                ++cnt_available;
                break;
            }
            // Col
            switch ( brd[ j ][ i ] )
            {
            case CellState::e_Cell_Mine:
                ++cnt_col_mine;
                break;
            case CellState::e_Cell_Opponent:
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
        case CellState::e_Cell_Mine:
            ++cnt_diagonal1_mine;
            break;
        case CellState::e_Cell_Opponent:
            ++cnt_diagonal1_opponent;
            break;
        }
        switch ( brd[ i ][ sz - i - 1 ] )
        {
        case CellState::e_Cell_Mine:
            ++cnt_diagonal2_mine;
            break;
        case CellState::e_Cell_Opponent:
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

TicTacToeState::Moves
TicTacToeState::get_possible_moves( TicTacToeState const& state )
{
    Moves possible_moves;

    for ( size_t i = 0; i < state.m_board.size( ); ++i )
    {
        for ( size_t j = 0; j < state.m_board[ i ].size( ); ++j )
        {
            if ( state.m_board[ i ][ j ] == CellState::e_Cell_Available )
            {
                possible_moves.push_back( {i, j} );
            }
        }
    }

    return std::move( possible_moves );
}

void
TicTacToeState::play_move( TicTacToeState& state, const Cell& cell )
{
    state.m_board[ cell.row ][ cell.col ]
        = state.m_my_turn ? CellState::e_Cell_Mine : CellState::e_Cell_Opponent;
    state.m_my_turn = !state.m_my_turn;
}

}  // namespace mcts