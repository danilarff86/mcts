#pragma once

#include <memory>
#include <vector>

namespace mcts
{
struct MctsNode;
using MctsNodePtr = std::shared_ptr< MctsNode >;
using Children = std::vector< MctsNodePtr >;

struct MctsState
{
    enum class Result
    {
        e_Result_Draw = 0,
        e_Result_Miss,
        e_Result_Hit
    };

    virtual ~MctsState( )
    {
    }

    virtual Result simulate( ) const = 0;
    virtual Children get_children( MctsNodePtr parent ) const = 0;
};

}  // namespace mcts