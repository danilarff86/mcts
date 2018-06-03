#include "MonteCarloTreeSearch.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iterator>
#include <memory>
#include <vector>
#include <functional>

namespace
{
double const SQRT_OF_TWO = sqrt( 2 );

// const bool srand_init = []( ) {
//     srand( static_cast< unsigned int >( time( NULL ) ) );
//     return true;
// }( );
}

namespace mcts
{
struct State;
struct MctsNode;

using StatePtr = std::shared_ptr< State >;
using StateConstPtr = std::shared_ptr< State const >;
using MctsNodePtr = std::shared_ptr< MctsNode >;

using Children = std::vector< MctsNodePtr >;
using ChildrenPtr = std::shared_ptr< Children >;

/**
 * In order to be as generic as possible,
 * I moved the current state of the board
 * to its own class.
 **/
struct State
{
    virtual ~State( )
    {
    }

    virtual ChildrenPtr getChildren( MctsNodePtr parent ) = 0;
    virtual int simulate( ) = 0;
};

struct TicTacToeState : public State
{
    enum class CellState : uint8_t
    {
        e_Cell_Available = 0,
        e_Cell_Mine,
        e_Cell_Opponent
    };

    enum class GameState : uint8_t
    {
        e_State_InProgress = 0,
        e_State_Draw,
        e_State_Hit,
        e_state_Miss
    };

    struct Cell
    {
        size_t row;
        size_t col;
    };

    using Board = std::vector< std::vector< CellState > >;
    using BoardPtr = std::shared_ptr< Board >;

    using Moves = std::vector< Cell >;

    TicTacToeState( BoardPtr board, bool turn, Cell last_move = {} )
        : m_board( board )
        , m_my_turn( turn )
        , m_last_move( last_move )
    {
    }

    const Cell&
    getLastMove( ) const
    {
        return m_last_move;
    }

    ChildrenPtr
    getChildren( MctsNodePtr parent ) override
    {
        auto temp_board = std::make_shared< Board >( *m_board );
        auto possible_moves = getPossibleMoves( *this );

        auto possible_children = std::make_shared< Children >( );
        possible_children->reserve( possible_moves.size( ) );

        for ( auto const& possible_move : possible_moves )
        {
            auto new_state
                = std::make_shared< TicTacToeState >( temp_board, m_my_turn, possible_move );
            playMove( *new_state, possible_move );
            possible_children->push_back( std::make_shared< MctsNode >( new_state, parent ) );

            temp_board = std::make_shared< Board >( *m_board );
        }

        return possible_children;

        /** In other words, return an array of all possible
         * children nodes from the given position.
         **/
    }

    int
    simulate( ) override
    {
        auto temp_state = std::make_shared< TicTacToeState >( std::make_shared< Board >( *m_board ),
                                                              m_my_turn, m_last_move );

        GameState game_state{};

        // Keep playing until game is over
        while ( /*!gameOver( *temp_state )*/ ( game_state = gameState( *this, *temp_state ) )
                == GameState::e_State_InProgress )
        {
            auto possible_moves = getPossibleMoves( *temp_state );

            // Play a random move from the possible moves
            // using the same playMove function as used in the
            // getChildren function.
            playMove( *temp_state, possible_moves[ rand( ) % possible_moves.size( ) ] );
        }
        // gameResult should take into account the original state (state)
        // and return a positive value if the person won, or a negative
        // value if they lost
        /*return gameResult( *this, *temp_state );*/
        switch ( game_state )
        {
        case GameState::e_State_Hit:
            return 1;
        case GameState::e_state_Miss:
            return -1;
        default:
            return 0;
        }
    }

private:
    static Moves
    getPossibleMoves( TicTacToeState const& state )
    {
        Moves possible_moves;

        // Assuming a 2d board

        for ( size_t i = 0; i < state.m_board->size( ); ++i )
        {
            for ( size_t j = 0; j < ( *state.m_board )[ i ].size( ); ++j )
            {
                // legalMove returns true if the move is legal
                // with the given state.
                if ( legalMove( state, i, j ) )
                {
                    possible_moves.push_back( {i, j} );
                }
            }
        }

        return possible_moves;
    }

    static bool
    legalMove( TicTacToeState const& state, size_t row, size_t col )
    {
        return ( *state.m_board )[ row ][ col ] == CellState::e_Cell_Available;
    }

    //     static bool
    //     gameOver( TicTacToeState const& state )
    //     {
    //         // TODO: Implement
    //     }
// 
//     static void
//     playMove( Board& board, bool turn, const Cell& cell )
//     {
//         board[ cell.row ][ cell.col ] = turn ? CellState::e_Cell_Mine : CellState::e_Cell_Opponent;
//     }

    static void
    playMove( TicTacToeState &state, const Cell& cell )
    {
        (*state.m_board)[ cell.row ][ cell.col ] = state.m_my_turn ? CellState::e_Cell_Mine : CellState::e_Cell_Opponent;
        // Toggle whose turn it is after each move
        state.m_my_turn = !state.m_my_turn;
    }

    //     static int
    //     gameResult( TicTacToeState const& state, TicTacToeState const& temp_state )
    //     {
    //         // TODO: Implement
    //     }

    static GameState
    gameState( TicTacToeState const& state, TicTacToeState const& temp_state )
    {
        auto const& brd = *temp_state.m_board;
        auto const sz = temp_state.m_board->size( );

        auto cnt_available = 0;

        // Check rows and cols
        for ( size_t i = 0; i < sz; ++i )
        {
            auto cnt_row_turn_true = 0;
            auto cnt_row_turn_false = 0;
            auto cnt_col_turn_true = 0;
            auto cnt_col_turn_false = 0;
            for ( size_t j = 0; j < sz; ++j )
            {
                // Row
                switch ( brd[ i ][ j ] )
                {
                case CellState::e_Cell_Mine:
                    ++cnt_row_turn_true;
                    break;
                case CellState::e_Cell_Opponent:
                    ++cnt_row_turn_false;
                    break;
                case CellState::e_Cell_Available:
                    ++cnt_available;
                    break;
                }
                // Col
                switch ( brd[ j ][ i ] )
                {
                case CellState::e_Cell_Mine:
                    ++cnt_col_turn_true;
                    break;
                case CellState::e_Cell_Opponent:
                    ++cnt_col_turn_false;
                    break;
                }
            }

            if ( cnt_row_turn_true == sz || cnt_col_turn_true == sz )
            {
                return !state.m_my_turn ? GameState::e_State_Hit : GameState::e_state_Miss;
            }

            if ( cnt_row_turn_false == sz || cnt_col_turn_false == sz )
            {
                return state.m_my_turn ? GameState::e_State_Hit : GameState::e_state_Miss;
            }
        }

        // Check diagonals
        auto cnt_diagonal1_turn_true = 0;
        auto cnt_diagonal1_turn_false = 0;
        auto cnt_diagonal2_turn_true = 0;
        auto cnt_diagonal2_turn_false = 0;

        for ( size_t i = 0; i < sz; ++i )
        {
            switch ( brd[ i ][ i ] )
            {
            case CellState::e_Cell_Mine:
                ++cnt_diagonal1_turn_true;
                break;
            case CellState::e_Cell_Opponent:
                ++cnt_diagonal1_turn_false;
                break;
            }
            switch ( brd[ i ][ sz - i - 1 ] )
            {
            case CellState::e_Cell_Mine:
                ++cnt_diagonal2_turn_true;
                break;
            case CellState::e_Cell_Opponent:
                ++cnt_diagonal2_turn_false;
                break;
            }
        }

        if ( cnt_diagonal1_turn_true == sz || cnt_diagonal2_turn_true == sz )
        {
            return /*!state.m_my_turn ? */GameState::e_State_Hit/* : GameState::e_state_Miss*/;
        }

        if ( cnt_diagonal1_turn_false == sz || cnt_diagonal2_turn_false == sz )
        {
            return /*state.m_my_turn ? */GameState::e_State_Hit/* : GameState::e_state_Miss*/;
        }

        return cnt_available > 0 ? GameState::e_State_InProgress : GameState::e_State_Draw;
    }

private:
    BoardPtr m_board;
    bool m_my_turn;
    Cell m_last_move;
};

struct MctsNode : public std::enable_shared_from_this< MctsNode >
{
    static MctsNodePtr
    createMctsRoot( StatePtr state )
    {
        return std::make_shared< MctsNode >( state, nullptr );
    }

    MctsNode( StatePtr state, MctsNodePtr parent )
        : m_state( state )  // The current state of the board
        , m_parent( parent )

        // As explained in the wiki, the hits, misses, and total tries
        // are used in the algorithm
        , m_hits( 0 )
        , m_misses( 0 )
        , m_total_trials( 0 )
    {
    }

    StateConstPtr
    getState( ) const
    {
        return m_state;
    }

    MctsNodePtr
    findChild( std::function< bool( StateConstPtr ) > predicate )
    {
        for (auto & child : *m_children)
        {
            if (predicate(child->m_state))
            {
                return child;
            }
        }

        return {};
    }

    /* additional functions go here */

    /** This function propagates to a leaf node and runs
     * a simulation at it.
     **/
    MctsNodePtr
    chooseChild( )
    {
        MctsNodePtr res;

        // If the node doesn't have children defined yet,
        // create them (the getChildren function will be
        // game specific.
        if ( !m_children )
        {
            m_children = m_state->getChildren( shared_from_this( ) );
        }

        // If the node is a leaf node, run a simulation
        // at it and back-propagate the results
        if ( m_children->empty( ) )
        {
            runSimulation( );
        }
        else
        {
            // Get all the unexplored children (if any)
            Children unexplored;
            std::copy_if( m_children->begin( ), m_children->end( ),
                          std::back_inserter( unexplored ),
                          []( MctsNodePtr node ) { return node->m_total_trials == 0; } );

            // If there are any unexplored children,
            // pick one at random and run with it.
            if ( !unexplored.empty( ) )
            {
                auto random_child = unexplored[ rand( ) % unexplored.size( ) ];
                random_child->runSimulation( );
                res = random_child;
            }
            else
            {
                // Find the best child (using the wiki-described
                // algorithm that I'll demo later) and recursively
                // call this function in it.
                MctsNodePtr best_child;
                auto best_potential = std::numeric_limits< double >::min( );
                for ( auto child : *m_children )
                {
                    auto const potential = childPotential( shared_from_this( ), child );

                    if ( potential > best_potential )
                    {
                        best_potential = potential;
                        best_child = child;
                    }
                }
                best_child->chooseChild( );
                res = best_child;
            }
        }

        return res;
    }

    /** This function returns the 'child potential'
     * for any given node, following the Wikipedia
     * "exploration and exploitation" principle.
     **/
    double
    childPotential( MctsNodePtr parent, MctsNodePtr child )
    {
        /** Assuming that the turn changes with every
         * move, the number of wins should be child.misses
         * (the number of times the opponent/next player
         * loses). However, I noticed that this works better
         * if also the wins are taken into account. For the
         * pure implementation, let w = child.misses;
         **/
        auto const w = child->m_misses - child->m_hits;
        auto const n = child->m_total_trials;
        auto const c = SQRT_OF_TWO;  // This constant is typically found empirically
        auto const t = parent->m_total_trials;
        return double( w ) / n + c * sqrt( log( double( t ) ) / n );
    }

    /** Run the simulation at the current board state,
     * and then back propagate the results to the
     * root of the tree.
     **/
    void
    runSimulation( )
    {
        backPropagate( m_state->simulate( ) );
    }

    void
    backPropagate( int simulation )
    {
        if ( simulation > 0 )
        {
            ++m_hits;
        }
        else if ( simulation < 0 )
        {
            ++m_misses;
        }
        ++m_total_trials;

        /** Assuming that the previous turn was by the opponent,
         * flip the signs of the simulation result while back
         * propagating. If it wasn't, then leave the sign of
         * the simulation the same.
         **/
        auto parent_strong_ref = m_parent.lock( );
        if ( parent_strong_ref )
        {
            parent_strong_ref->backPropagate( -simulation );
        }
    }

private:
    StatePtr m_state;
    std::weak_ptr< MctsNode > m_parent;

    int m_hits;
    int m_misses;
    int m_total_trials;

    ChildrenPtr m_children;
};

TicTacToeGameAI::TicTacToeGameAI( const AvailableCells& available )
{
    auto const sz = available.size( );
    auto board
        = std::make_shared< TicTacToeState::Board >( sz, TicTacToeState::Board::value_type( sz ) );
    for ( size_t i = 0; i < sz; ++i )
    {
        std::transform( available[ i ].begin( ), available[ i ].end( ), ( *board )[ i ].begin( ),
                        []( bool val ) {
                            return val ? TicTacToeState::CellState::e_Cell_Available
                                       : TicTacToeState::CellState::e_Cell_Opponent;
                        } );
    }

    auto state = std::make_shared< TicTacToeState >( board, true );
    m_tree = MctsNode::createMctsRoot( state );

    // Play Move
    // Save move position
    int iterations = 100;
    while (iterations-- > 0)
    {
        m_tree->chooseChild( );
    }
    m_current_node = m_tree->chooseChild( );
}

void
TicTacToeGameAI::opponent_move( const Cell& cell )
{
    auto current_node_strong_ref = m_current_node.lock( );
    TicTacToeState::Cell const state_cell{cell.row, cell.col};

    // Set evidence
    auto opponent_child = current_node_strong_ref->findChild( [&cell]( StateConstPtr state_ptr ) {
        auto ttt_state = std::dynamic_pointer_cast< TicTacToeState const >( state_ptr );
        auto const& last_move = ttt_state->getLastMove( );
        return last_move.row == cell.row && last_move.col == cell.col;
    } );

    // Play Move
    // Save move position
    int iterations = 100;
    while ( iterations-- > 0 )
    {
        opponent_child->chooseChild( );
    }
    m_current_node = opponent_child->chooseChild( );
}

mcts::TicTacToeGameAI::Cell
TicTacToeGameAI::get_my_move( ) const
{
    auto cell
        = std::dynamic_pointer_cast< TicTacToeState const >( m_current_node.lock( )->getState( ) )
              ->getLastMove( );

    return {cell.row, cell.col};
}

}
