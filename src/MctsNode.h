#pragma once

#include "MctsState.h"

#include <memory>

namespace mcts
{
struct MctsNode : public std::enable_shared_from_this< MctsNode >
{
    MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent = nullptr );

    MctsNodePtr choose_child( );
    MctsState const& get_state( ) const;

private:
    std::unique_ptr< MctsState > m_state;
    std::weak_ptr< MctsNode > m_parent;

    int m_hits;
    int m_misses;
    int m_total_trials;

    std::unique_ptr< Children > m_children;
};

}  // namespace mcts