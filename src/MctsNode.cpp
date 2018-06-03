#include "MctsNode.h"

#include <algorithm>
#include <cmath>

namespace mcts
{
namespace
{
double const SQRT_OF_TWO = sqrt( 2 );

}

MctsNode::MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent /*= nullptr */ )
    : m_state( std::move( state ) )
    , m_parent( parent )
    , m_hits( 0 )
    , m_total_trials( 0 )
{
}

MctsNodePtr
MctsNode::choose_child( )
{
    if ( !m_children )
    {
        m_children = m_state->get_children( shared_from_this( ) );
    }

    if ( m_children->empty( ) )
    {
        run_simulation( );
    }
    else
    {
        return explore_and_exploit( );
    }

    return nullptr;
}

MctsState const&
MctsNode::get_state( ) const
{
    return *m_state;
}

MctsNodePtr
MctsNode::find_child( std::function< bool( const MctsState& ) > predicate ) const
{
    if ( m_children )
    {
        for ( auto& child : *m_children )
        {
            if ( predicate( *( child->m_state ) ) )
            {
                return child;
            }
        }
    }

    return nullptr;
}

void
MctsNode::run_simulation( )
{
    back_propagate( m_state->simulate( ) );
}

MctsNodePtr
MctsNode::explore_and_exploit( )
{
    Children unexplored;
    std::copy_if( m_children->begin( ), m_children->end( ), std::back_inserter( unexplored ),
                  []( MctsNodePtr node ) { return node->m_total_trials == 0; } );

    if ( !unexplored.empty( ) )
    {
        auto random_child = unexplored[ rand( ) % unexplored.size( ) ];
        random_child->run_simulation( );
        return random_child;
    }
    else
    {
        MctsNodePtr best_child;
        auto best_potential = std::numeric_limits< double >::min( );
        for ( auto child : *m_children )
        {
            auto const potential = child_potential( *child );
            if ( potential > best_potential )
            {
                best_potential = potential;
                best_child = child;
            }
        }
        best_child->choose_child( );
        return best_child;
    }
}

void
MctsNode::back_propagate( MctsState::Result result )
{
    if ( result == MctsState::Result::e_Result_Hit )
    {
        ++m_hits;
    }
    ++m_total_trials;

    auto parent_strong_ref = m_parent.lock( );
    if ( parent_strong_ref )
    {
        parent_strong_ref->back_propagate( result );
    }
}

double
MctsNode::child_potential( const MctsNode& child ) const
{
    auto const w = child.m_hits;
    auto const n = child.m_total_trials;
    auto const c = SQRT_OF_TWO;
    auto const t = m_total_trials;
    return double( w ) / n + c * sqrt( log( double( t ) ) / n );
}

}  // namespace mcts