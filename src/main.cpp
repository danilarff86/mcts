#include "MonteCarloTreeSearch.h"

#include <iostream>
#include <vector>
#include <memory>

int main()
{
    std::vector< std::vector< bool > > available( 3, std::vector< bool >( 3, true ) );

    auto game = std::make_shared< mcts::TicTacToeGameAI >( available );

	std::cout << "Test!" << std::endl;
}