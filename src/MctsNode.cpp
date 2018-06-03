#include "MctsNode.h"

namespace mcts
{
MctsNode::MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent /*= nullptr */ )
    : m_state( std::move( state ) )
    , m_parent( parent )
{
}

mcts::MctsNodePtr
MctsNode::choose_child( )
{
    if (!m_children)
    {
        /*m_children = std::unique_ptr< Children >( m_state->get_children( shared_from_this( ) ) );*/
    }
    // TODO: Implement

    return nullptr;
}

mcts::MctsState const&
MctsNode::get_state( ) const
{
    return *m_state;
}

}  // namespace mcts