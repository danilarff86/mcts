#pragma once

#include <memory>
#include <vector>

namespace mcts
{
struct MctsNode;
using MctsNodePtr = std::shared_ptr< MctsNode >;
using Children = std::vector< MctsNodePtr >;
using ChildrenPtr = std::unique_ptr< Children >;

struct MctsState
{
    enum class Result
    {
        e_Result_Draw = 0,
        e_Result_Miss,
        e_Result_Hit,
        e_Result_NotFinished,
    };

    virtual ~MctsState( )
    {
    }

    virtual Result simulate( ) const = 0;
    virtual ChildrenPtr get_children( MctsNodePtr parent ) const = 0;
};

}  // namespace mcts