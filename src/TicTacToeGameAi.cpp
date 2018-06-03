#include "TicTacToeGameAi.h"
#include "MctsNode.h"
#include "TicTacToeState.h"

#include <algorithm>

namespace mcts
{
TicTacToeGameAI::TicTacToeGameAI( const AvailableCells& available, size_t iterations )
    : m_iterations( iterations )
{
    auto const sz = available.size( );
    TicTacToeState::Board board( sz, TicTacToeState::Board::value_type( sz ) );
    for ( size_t i = 0; i < sz; ++i )
    {
        std::transform( available[ i ].begin( ), available[ i ].end( ), board[ i ].begin( ),
                        []( bool val ) {
                            return val ? TicTacToeState::CellState::e_Cell_Available
                                       : TicTacToeState::CellState::e_Cell_Opponent;
                        } );
    }

    std::unique_ptr< TicTacToeState > state( new TicTacToeState( std::move( board ), true ) );
    m_tree = std::make_shared< MctsNode >( std::move( state ) );

    // Play Move
    // Save move position
    int cnt = m_iterations;
    while ( cnt-- > 0 )
    {
        m_tree->choose_child( );
    }
    m_current_node = m_tree->choose_child( );
}

void
TicTacToeGameAI::opponent_move( const MovePosition& position )
{
    auto current_node_strong_ref = m_current_node.lock( );
    TicTacToeState::Cell const state_cell{position.row, position.col};

    // Ensure current node has children
    current_node_strong_ref->choose_child( );

    // Get opponent child
    auto opponent_child
        = current_node_strong_ref->find_child( [&position]( const MctsState& state ) {
              auto const& tic_tac_toe_state = dynamic_cast< TicTacToeState const& >( state );
              auto const& last_move = tic_tac_toe_state.get_last_move( );
              return last_move.row == position.row && last_move.col == position.col;
          } );

    // Play Move
    // Save move position
    int cnt = m_iterations;
    while ( cnt-- > 0 )
    {
        opponent_child->choose_child( );
    }
    m_current_node = opponent_child->choose_child( );
}

MovePosition
TicTacToeGameAI::get_my_move( ) const
{
    auto move_pos = dynamic_cast< TicTacToeState const& >( m_current_node.lock( )->get_state( ) )
                        .get_last_move( );

    return {move_pos.row, move_pos.col};
}

}  // namespace mcts