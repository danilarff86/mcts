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

int
main( )
{
    std::vector< std::vector< bool > > available( 3, std::vector< bool >( 3, true ) );

    auto game = std::make_shared< mcts::TicTacToeGameAI >( available, 100 );

    bool cont = true;
    while ( cont )
    {
        auto ai_move = game->get_my_move( );
        std::cout << ai_move.row << " " << ai_move.col << std::endl;

        mcts::MovePosition pos;
        std::cin >> pos.row >> pos.col;
        game->opponent_move( pos );
    }
}
