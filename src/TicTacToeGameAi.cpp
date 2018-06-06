#include "TicTacToeGameAi.h"
#include "MctsNode.h"

#include <algorithm>
#include <cmath>

namespace mcts
{

TicTacToeState::Board
TicTacToeGameAI::create_small_board( const TicTacToeGameAI::AvailableCells& available )
{
    TicTacToeState::Board board( SMALL_BOARD_SIZE,
                                 TicTacToeState::Board::value_type( SMALL_BOARD_SIZE ) );
    for ( size_t i = 0; i < SMALL_BOARD_SIZE; ++i )
    {
        std::transform( available[ i ].begin( ), available[ i ].end( ), board[ i ].begin( ),
                        []( bool val ) {
                            return val ? TicTacToeState::CellState::e_Cell_Available
                                       : TicTacToeState::CellState::e_Cell_Opponent;
                        } );
    }
    return board;
}

TicTacToeBigGameState::BigBoard
TicTacToeGameAI::create_big_board( const TicTacToeGameAI::AvailableCells& available )
{
    TicTacToeBigGameState::BigBoard big_board(
        SMALL_BOARD_SIZE,
        std::vector< TicTacToeState::Board >(
            SMALL_BOARD_SIZE,
            TicTacToeState::Board( SMALL_BOARD_SIZE, std::vector< TicTacToeState::CellState >(
                                                         SMALL_BOARD_SIZE ) ) ) );
    for ( size_t i = 0; i < SMALL_BOARD_SIZE; ++i )
    {
        for ( size_t j = 0; j < SMALL_BOARD_SIZE; ++j )
        {
            auto& brd = big_board[ i ][ j ];
            for ( size_t i_cell = 0; i_cell < SMALL_BOARD_SIZE; ++i_cell )
            {
                for ( size_t j_cell = 0; j_cell < SMALL_BOARD_SIZE; ++j_cell )
                {
                    brd[ i_cell ][ j_cell ] = available[ i * SMALL_BOARD_SIZE + i_cell ]
                                                       [ j * SMALL_BOARD_SIZE + j_cell ]
                                                  ? TicTacToeState::CellState::e_Cell_Available
                                                  : TicTacToeState::CellState::e_Cell_Opponent;
                }
            }
        }
    }
    return big_board;
}

void TicTacToeGameAI::make_move(MctsNodePtr node)
{
    int cnt = std::max( m_iterations, BIG_BOARD_SIZE * BIG_BOARD_SIZE );
    while ( cnt-- > 0 )
    {
        node->choose_child( );
    }
    m_current_node = node->choose_child( );
}

TicTacToeGameAI::TicTacToeGameAI( const AvailableCells& available, size_t iterations )
    : m_iterations( iterations )
{
    std::unique_ptr< MctsState > state(
        available.size( ) > 3 ? new TicTacToeBigGameState( create_big_board( available ), true )
                              : new TicTacToeState( create_small_board( available ), true ) );
    m_tree = std::make_shared< MctsNode >( std::move( state ) );

    make_move( m_tree );
}

void
TicTacToeGameAI::opponent_move( const MovePosition& position )
{
    auto current_node_strong_ref = m_current_node.lock( );
    TicTacToeState::Cell const state_cell{position.row, position.col};

    // Ensure current node has children
    make_move( current_node_strong_ref );

    // Get opponent child
    auto opponent_child
        = current_node_strong_ref->find_child( [&position]( const MctsState& state ) {
              auto const& tic_tac_toe_state = dynamic_cast< TicTacToeState const& >( state );
              auto const& last_move = tic_tac_toe_state.get_last_move( );
              return last_move.row == position.row && last_move.col == position.col;
          } );

    make_move( opponent_child );
}

MovePosition
TicTacToeGameAI::get_ai_move( ) const
{
    auto move_pos = dynamic_cast< TicTacToeState const& >( m_current_node.lock( )->get_state( ) )
                        .get_last_move( );

    return {move_pos.row, move_pos.col};
}

}  // namespace mcts