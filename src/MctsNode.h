#pragma once

#include "MctsState.h"

#include <functional>
#include <memory>

namespace mcts
{
struct MctsNode : public std::enable_shared_from_this< MctsNode >
{
    explicit MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent = nullptr );

    MctsNodePtr choose_child( );
    MctsState const& get_state( ) const;
    MctsNodePtr find_child( std::function< bool( const MctsState& ) > predicate ) const;

private:
    void run_simulation( );
    MctsNodePtr explore_and_exploit( );
    void back_propagate( MctsState::Result result );
    double child_potential( const MctsNode& child ) const;

private:
    std::unique_ptr< MctsState > m_state;
    std::weak_ptr< MctsNode > m_parent;

    int m_hits;
    int m_total_trials;

    ChildrenPtr m_children;
};

}  // namespace mcts