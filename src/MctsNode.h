#pragma once

#include "MctsState.h"

#include <memory>

namespace mcts
{
struct MctsNode : public std::enable_shared_from_this< MctsNode >
{
    MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent = nullptr );

private:
    std::unique_ptr< MctsState > m_state;
    std::weak_ptr< MctsNode > m_parent;
};

}  // namespace mcts