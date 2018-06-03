#include "TicTacToeGameAi.h"

#include <iostream>

int
main( )
{
    std::vector< std::vector< bool > > available( 3, std::vector< bool >( 3, true ) );

    auto game = std::make_shared< mcts::TicTacToeGameAI >( available );

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
