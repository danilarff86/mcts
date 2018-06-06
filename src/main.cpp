#include "TicTacToeGameAi.h"

#include <ctime>
#include <iostream>

namespace
{
const bool srand_init = []( ) {
    srand( static_cast< unsigned int >( time( NULL ) ) );
    return true;
}( );
}

using Brd = std::vector< std::vector< char > >;

const size_t SMALL_BOARD_SIZE = mcts::TicTacToeGameAI::SMALL_BOARD_SIZE;
const size_t BIG_BOARD_SIZE = mcts::TicTacToeGameAI::BIG_BOARD_SIZE;

void
print_board( const Brd& brd )
{
    auto const big = brd.size( ) > SMALL_BOARD_SIZE;
    auto const header_footer = big ? "#############" : "#####";

    std::cout << header_footer << std::endl;
    for ( size_t i = 0; i < ( big ? BIG_BOARD_SIZE : SMALL_BOARD_SIZE ); ++i )
    {
        std::cout << "#";
        for ( size_t j = 0; j < ( big ? BIG_BOARD_SIZE : SMALL_BOARD_SIZE ); ++j )
        {
            std::cout << brd[ i ][ j ];
            if ( ( j + 1 ) % SMALL_BOARD_SIZE == 0 )
            {
                std::cout << '#';
            }
        }
        if ( ( i + 1 ) % SMALL_BOARD_SIZE == 0 )
        {
            std::cout << "\n" << header_footer;
        }
        std::cout << std::endl;
    }
}

int
main( )
{
    auto const board_size = BIG_BOARD_SIZE;

    Brd visualization( board_size, std::vector< char >( board_size, '-' ) );

    std::vector< std::vector< bool > > available( board_size,
                                                  std::vector< bool >( board_size, true ) );

    auto game = std::make_shared< mcts::TicTacToeGameAI >( available, 100 );

    bool cont = true;
    while ( cont )
    {
        auto ai_move = game->get_my_move( );
        visualization[ ai_move.row ][ ai_move.col ] = 'X';
        print_board( visualization );

        mcts::MovePosition pos;
        std::cin >> pos.row >> pos.col;
        visualization[ pos.row ][ pos.col ] = '0';
        game->opponent_move( pos );
    }
}
