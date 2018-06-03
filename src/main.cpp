#include "MonteCarloTreeSearch.h"

#include <iostream>
#include <vector>
#include <memory>


int main()
{
    std::vector< std::vector< bool > > available( 3, std::vector< bool >( 3, true ) );

    auto game = std::make_shared< mcts::TicTacToeGameAI >( available );
    auto ai_move = game->get_my_move( );

    mcts::TicTacToeGameAI::Cell cell{};
    bool cont = true;
    while (cont)
    {
        game->opponent_move( cell );
        ai_move = game->get_my_move( );
    }

    std::cout << "Test!" << std::endl;
}