#include "TicTacToeBigGameState.h"

namespace mcts
{
TicTacToeBigGameState::TicTacToeBigGameState( TicTacToeBigGameState::BigBoard&& board,
                                              bool my_turn,
                                              TicTacToeBigGameState::Cell&& last_move )
    : TicTacToeState( {}, my_turn, std::move( last_move ) )
    , m_board( std::move( board ) )
    , m_results( m_board.size( ),
                 std::vector< TicTacToeBigGameState::Result >(
                     m_board.size( ), TicTacToeBigGameState::Result::e_Result_NotFinished ) )
{
}

std::unique_ptr< TicTacToeState >
TicTacToeBigGameState::clone( ) const
{
    return std::unique_ptr< TicTacToeState >( new TicTacToeBigGameState( *this ) );
}

TicTacToeBigGameState::Result
TicTacToeBigGameState::game_state( ) const
{
    // FIXME: WHAT ABOUT DRAWS?
    return TicTacToeState::game_state< ResultBoard, Result, Result::e_Result_NotFinished,
                                       Result::e_Result_Hit, Result::e_Result_Miss >( m_results );
}

TicTacToeState::Moves
TicTacToeBigGameState::get_possible_moves( ) const
{
    auto last_board_coord = get_board_coord( m_last_move );

    if ( last_board_coord.row < 0
         || m_results[ last_board_coord.row ][ last_board_coord.col ]
                != Result::e_Result_NotFinished )
    {
        // all boards are possible
        Moves possible_moves;

        auto const big_sz = m_board.size( );

        for ( size_t i = 0; i < big_sz; ++i )
        {
            for ( size_t j = 0; j < big_sz; ++j )
            {
                if ( m_results[ i ][ j ] != Result::e_Result_NotFinished )
                {
                    append_board_moves( possible_moves, i, j );
                }
            }
        }

        return std::move( possible_moves );
    }
    else
    {
        return TicTacToeState::get_possible_moves(
            m_board[ last_board_coord.row ][ last_board_coord.col ] );
    }
}

void
TicTacToeBigGameState::play_move( const Cell& cell )
{
    auto const target_board_coord = get_board_coord( cell );
    auto& target_board = m_board[ target_board_coord.row ][ target_board_coord.col ];

    auto const big_sz = m_board.size( );
    auto const row_cell = cell.row - ( target_board_coord.row * big_sz );
    auto const col_cell = cell.col - ( target_board_coord.col * big_sz );
    auto& target_cell = target_board[ row_cell ][ col_cell ];

    target_cell = m_my_turn ? CellState::e_Cell_Mine : CellState::e_Cell_Opponent;
    m_results[ target_board_coord.row ][ target_board_coord.col ]
        = TicTacToeState::game_state( target_board );
    m_last_move = cell;
    m_my_turn = !m_my_turn;
}

TicTacToeBigGameState::Cell
TicTacToeBigGameState::get_board_coord( const Cell& cell ) const
{
    if ( cell.row < 0 )
    {
        return cell;
    }

    auto const big_sz = static_cast< int >( m_board.size( ) );
    return {cell.row / big_sz, cell.col / big_sz};
}

void
TicTacToeBigGameState::append_board_moves( Moves& moves, size_t row, size_t col ) const
{
    auto board_moves = TicTacToeState::get_possible_moves( m_board[ row ][ col ] );
    int const row_offset = m_board.size( ) * row;
    int const col_offset = m_board.size( ) * col;

    for ( auto const& mv : board_moves )
    {
        moves.push_back( {row_offset + mv.row, col_offset + mv.col} );
    }
}

}  // namespace mcts