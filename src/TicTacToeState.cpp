#include "TicTacToeState.h"
#include "MctsNode.h"

namespace mcts
{
TicTacToeState::TicTacToeState( TicTacToeState::Board&& board,
                                bool my_turn,
                                TicTacToeState::Cell&& last_move )
    : m_board( std::move( board ) )
    , m_my_turn( my_turn )
    , m_last_move( std::move( last_move ) )
{
}

MctsState::Result
TicTacToeState::simulate( ) const
{
    auto temp_state = clone( );

    Result result = Result::e_Result_NotFinished;

    while ( ( result = temp_state->game_state( ) ) == Result::e_Result_NotFinished )
    {
        auto const possible_moves = temp_state->get_possible_moves( );

        temp_state->play_move( possible_moves[ rand( ) % possible_moves.size( ) ] );
    }

    return result;
}

ChildrenPtr
TicTacToeState::get_children( MctsNodePtr parent ) const
{
    auto possible_moves = get_possible_moves( );

    auto possible_children = ChildrenPtr( new Children );
    possible_children->reserve( possible_moves.size( ) );

    for ( auto const& possible_move : possible_moves )
    {
        auto temp_state = clone( );
        temp_state->play_move( possible_move );
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

std::unique_ptr< TicTacToeState >
TicTacToeState::clone( ) const
{
    return std::unique_ptr< TicTacToeState >( new TicTacToeState( *this ) );
}

mcts::MctsState::Result
TicTacToeState::game_state( ) const
{
    return game_state( m_board );
}

TicTacToeState::Moves
TicTacToeState::get_possible_moves( ) const
{
    return get_possible_moves( m_board );
}

void
TicTacToeState::play_move( const Cell& cell )
{
    m_board[cell.row][cell.col]
        = m_my_turn ? CellState::e_Cell_Mine : CellState::e_Cell_Opponent;
    m_last_move = cell;
    m_my_turn = !m_my_turn;
}

TicTacToeState::Moves
TicTacToeState::get_possible_moves( Board const& brd )
{
    Moves possible_moves;

    for ( int i = 0; i < static_cast< int >( brd.size( ) ); ++i )
    {
        for ( int j = 0; j < static_cast< int >( brd[ i ].size( ) ); ++j )
        {
            if ( brd[ i ][ j ] == CellState::e_Cell_Available )
            {
                possible_moves.push_back( {i, j} );
            }
        }
    }

    return std::move( possible_moves );
}

}  // namespace mcts