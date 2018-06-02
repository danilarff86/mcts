#include "MonteCarloTreeSearch.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iterator>
#include <memory>
#include <vector>

namespace
{
double const SQRT_OF_TWO = sqrt( 2 );

const bool srand_init = []( ) {
    srand( static_cast< unsigned int >( time( NULL ) ) );
    return true;
}( );
}

namespace mcts
{

struct State;
struct MctsNode;

using StatePtr = std::shared_ptr< State >;
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
        e_Cell_Turn_True,
        e_Cell_Turn_False
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
        , m_turn( turn )
        , m_last_move( last_move )
    {
    }

    ChildrenPtr getChildren( MctsNodePtr parent ) override
    {
        auto temp_board = std::make_shared<Board>(*m_board);
        auto possible_moves = getPossibleMoves( *this );

        auto possible_children = std::make_shared<Children>();
        possible_children->reserve( possible_moves.size( ) );

        for ( auto const &possible_move : possible_moves )
        {
            playMove( *temp_board, m_turn, possible_move );
            auto new_state = std::make_shared< TicTacToeState >( temp_board, !m_turn, possible_move );
            possible_children->push_back( std::make_shared< MctsNode >( new_state, parent ) );

            temp_board = std::make_shared< Board >( *temp_board );
        }

        return possible_children;

        /** In other words, return an array of all possible
         * children nodes from the given position.
         **/
    }

    int simulate() override
    {
        auto temp_state = std::make_shared< TicTacToeState >( *this );

        GameState game_state{};

        // Keep playing until game is over
        while ( /*!gameOver( *temp_state )*/ ( game_state = gameState( *this, *temp_state ) )
                == GameState::e_State_InProgress )
        {
            auto possible_moves = getPossibleMoves( *temp_state );

            // Play a random move from the possible moves
            // using the same playMove function as used in the
            // getChildren function.
            playMove( *( temp_state->m_board ), temp_state->m_turn,
                      possible_moves[ rand( ) % possible_moves.size( ) ] );

            // Toggle whose turn it is after each move
            temp_state->m_turn = !temp_state->m_turn;
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

private :
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

    static void
    playMove( Board& board, bool turn, const Cell& cell )
    {
        board[ cell.row ][ cell.col ]
            = turn ? CellState::e_Cell_Turn_True : CellState::e_Cell_Turn_False;
    }

//     static int
//     gameResult( TicTacToeState const& state, TicTacToeState const& temp_state )
//     {
//         // TODO: Implement
//     }

    static GameState
    gameState( TicTacToeState const& state, TicTacToeState const& temp_state )
    {
        auto const& brd = *state.m_board;
        auto const sz = state.m_board->size( );

        auto cnt_available = 0;

        // Check rows and cols
        for (size_t i = 0; i < sz; ++i)
        {
            auto cnt_row_turn_true = 0;
            auto cnt_row_turn_false = 0;
            auto cnt_col_turn_true = 0;
            auto cnt_col_turn_false = 0;
            for ( size_t j = 0; j < sz; ++j )
            {
                // Row
                switch (brd[i][j])
                {
                case CellState::e_Cell_Turn_True:
                    ++cnt_row_turn_true;
                    break;
                case CellState::e_Cell_Turn_False:
                    ++cnt_row_turn_false;
                    break;
                case CellState::e_Cell_Available:
                    ++cnt_available;
                    break;
                }
                //Col
                switch ( brd[ j ][ i ] )
                {
                case CellState::e_Cell_Turn_True:
                    ++cnt_col_turn_true;
                    break;
                case CellState::e_Cell_Turn_False:
                    ++cnt_col_turn_false;
                    break;
                }
            }

            if ( cnt_row_turn_true == sz || cnt_col_turn_true == sz )
            {
                return state.m_turn ? GameState::e_State_Hit : GameState::e_state_Miss;
            }

            if ( cnt_row_turn_false == sz || cnt_col_turn_false == sz )
            {
                return !state.m_turn ? GameState::e_State_Hit : GameState::e_state_Miss;
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
            case CellState::e_Cell_Turn_True:
                ++cnt_diagonal1_turn_true;
                break;
            case CellState::e_Cell_Turn_False:
                ++cnt_diagonal1_turn_false;
                break;
            }
            switch ( brd[ i ][ sz - i - 1 ] )
            {
            case CellState::e_Cell_Turn_True:
                ++cnt_diagonal2_turn_true;
                break;
            case CellState::e_Cell_Turn_False:
                ++cnt_diagonal2_turn_false;
                break;
            }
        }

        if ( cnt_diagonal1_turn_true == sz || cnt_diagonal2_turn_true == sz )
        {
            return state.m_turn ? GameState::e_State_Hit : GameState::e_state_Miss;
        }

        if ( cnt_diagonal1_turn_false == sz || cnt_diagonal2_turn_false == sz )
        {
            return !state.m_turn ? GameState::e_State_Hit : GameState::e_state_Miss;
        }

        return cnt_available > 0 ? GameState::e_State_InProgress : GameState::e_State_Draw;
    }

private:
    BoardPtr m_board;
    bool m_turn;
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

    /* additional functions go here */

    /** This function propagates to a leaf node and runs
     * a simulation at it.
     **/
    void
    chooseChild( )
    {
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
                unexplored[ rand( ) % unexplored.size( ) ]->runSimulation( );
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
            }
        }
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
        if ( m_parent )
        {
            m_parent->backPropagate( -simulation );
        }
    }

private:
    StatePtr m_state;
    MctsNodePtr m_parent;

    int m_hits;
    int m_misses;
    int m_total_trials;

    ChildrenPtr m_children;
};
}
