#include "MctsNode.h"

namespace mcts
{
MctsNode::MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent /*= nullptr */ )
    : m_state( std::move( state ) )
    , m_parent( parent )
{
}

}  // namespace mcts