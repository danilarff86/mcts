#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <ctime>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

namespace threadpool
{
class ThreadPool
{
public:
    ThreadPool( size_t );
    template < class F, class... Args >
    auto enqueue( F&& f, Args&&... args )
        -> std::future< typename std::result_of< F( Args... ) >::type >;
    ~ThreadPool( );

private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function< void( ) > > tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool( size_t threads )
    : stop( false )
{
    for ( size_t i = 0; i < threads; ++i )
        workers.emplace_back( [this] {
            for ( ;; )
            {
                std::function< void( ) > task;

                {
                    std::unique_lock< std::mutex > lock( this->queue_mutex );
                    this->condition.wait( lock,
                                          [this] { return this->stop || !this->tasks.empty( ); } );
                    if ( this->stop && this->tasks.empty( ) )
                        return;
                    task = std::move( this->tasks.front( ) );
                    this->tasks.pop( );
                }

                task( );
            }
        } );
}

// add new work item to the pool
template < class F, class... Args >
auto
ThreadPool::enqueue( F&& f, Args&&... args )
    -> std::future< typename std::result_of< F( Args... ) >::type >
{
    using return_type = typename std::result_of< F( Args... ) >::type;

    auto task = std::make_shared< std::packaged_task< return_type( ) > >(
        std::bind( std::forward< F >( f ), std::forward< Args >( args )... ) );

    std::future< return_type > res = task->get_future( );
    {
        std::unique_lock< std::mutex > lock( queue_mutex );

        // don't allow enqueueing after stopping the pool
        if ( stop )
            throw std::runtime_error( "enqueue on stopped ThreadPool" );

        tasks.emplace( [task]( ) { ( *task )( ); } );
    }
    condition.notify_one( );
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool( )
{
    {
        std::unique_lock< std::mutex > lock( queue_mutex );
        stop = true;
    }
    condition.notify_all( );
    for ( std::thread& worker : workers )
        worker.join( );
}

static ThreadPool pool( std::thread::hardware_concurrency( ) );

struct AsyncWorker
{
    void
    thread_fn( )
    {
        std::unique_lock< std::mutex > lock( m_mutex );

        while ( true )
        {
            {
                std::unique_lock< std::mutex > lock( m_mutex_started_waiting );
                m_started_waiting = true;
                m_cond_var_started_waiting.notify_all( );
            }

            // Wait for a notification.
            m_cond_var.wait( lock, [&]( ) { return m_processing; } );
            lock.unlock( );
            while ( true )
            {
                m_fn( );
                lock.lock( );
                if ( !m_processing )
                {
                    break;
                }
                lock.unlock( );
            }
            if ( m_exit )
            {
                break;
            }
        }
    }

    AsyncWorker( std::function< void( ) > fn )
        : m_fn( fn )
        , m_started_waiting( false )
        , m_processing( false )
        , m_exit( false )
    {
        m_thread = std::thread( &AsyncWorker::thread_fn, this );
        std::unique_lock< std::mutex > lock( m_mutex_started_waiting );
        m_cond_var_started_waiting.wait( lock, [&]( ) { return m_started_waiting; } );
    }

    ~AsyncWorker( )
    {
        {
            std::unique_lock< std::mutex > lock( m_mutex );
            m_processing = false;
            m_exit = true;
            m_cond_var.notify_all( );
        }
        m_thread.join( );
    }

    void
    processing( )
    {
        std::unique_lock< std::mutex > lock( m_mutex );
        if ( !m_processing )
        {
            m_processing = true;
            m_cond_var.notify_all( );
        }
    }
    void
    idle( )
    {
        std::unique_lock< std::mutex > lock( m_mutex );
        if ( m_processing )
        {
            m_processing = false;
            m_cond_var.notify_all( );
        }
    }

private:
    std::function< void( ) > m_fn;

    std::thread m_thread;

    std::mutex m_mutex;
    std::condition_variable m_cond_var;

    std::mutex m_mutex_started_waiting;
    std::condition_variable m_cond_var_started_waiting;

    bool m_started_waiting;
    bool m_processing;
    bool m_exit;
};

}  // namespace threadpool

namespace game
{
static const int SMALL_BRD_SZ = 3;

float const SQRT_OF_TWO = sqrt( 2 );

using AvailableCells = std::vector< std::vector< bool > >;

struct MctsNode;
using MctsNodePtr = std::shared_ptr< MctsNode >;
using Children = std::vector< MctsNodePtr >;
using ChildrenPtr = std::unique_ptr< Children >;

template < typename T >
using Board = std::vector< std::vector< T > >;

struct Cell
{
    int row;
    int col;
};

bool
operator==( const Cell& rhs, const Cell& lhs )
{
    return ( rhs.row == lhs.row ) && ( rhs.col == lhs.col );
}

using Moves = std::vector< Cell >;

enum class CellState : uint8_t
{
    e_Available = 0,
    e_Opponent = 1,
    e_Mine = 2,
    e_Common = 3
};

enum class Result : uint8_t
{
    e_Result_NotFinished = 0,
    e_Result_Miss = 1,
    e_Result_Draw = 2,
    e_Result_Hit = 3,
};

Result
game_state( Board< CellState > const& brd, bool big_board = false )
{
    auto const sz = brd.size( );
    assert( sz == 3 );
    assert( brd[ 0 ].size( ) == 3 );

    auto cnt_available = 0;
    auto cnt_all_mine = 0;
    auto cnt_all_opponent = 0;

    // Check rows and cols
    for ( size_t i = 0; i < sz; ++i )
    {
        auto cnt_row_mine = 0;
        auto cnt_row_opponent = 0;
        auto cnt_col_mine = 0;
        auto cnt_col_opponent = 0;
        for ( size_t j = 0; j < sz; ++j )
        {
            // Row
            switch ( brd[ i ][ j ] )
            {
            case CellState::e_Mine:
                ++cnt_row_mine;
                break;
            case CellState::e_Opponent:
                ++cnt_row_opponent;
                break;
            case CellState::e_Available:
                ++cnt_available;
                break;
            }
            // Col
            switch ( brd[ j ][ i ] )
            {
            case CellState::e_Mine:
                ++cnt_col_mine;
                break;
            case CellState::e_Opponent:
                ++cnt_col_opponent;
                break;
            }
        }

        cnt_all_mine += cnt_row_mine;
        cnt_all_opponent += cnt_row_opponent;

        if ( cnt_row_mine == sz || cnt_col_mine == sz )
        {
            return Result::e_Result_Hit;
        }

        if ( cnt_row_opponent == sz || cnt_col_opponent == sz )
        {
            return Result::e_Result_Miss;
        }
    }

    // Check diagonals
    auto cnt_diagonal1_mine = 0;
    auto cnt_diagonal1_opponent = 0;
    auto cnt_diagonal2_mine = 0;
    auto cnt_diagonal2_opponent = 0;

    for ( size_t i = 0; i < sz; ++i )
    {
        switch ( brd[ i ][ i ] )
        {
        case CellState::e_Mine:
            ++cnt_diagonal1_mine;
            break;
        case CellState::e_Opponent:
            ++cnt_diagonal1_opponent;
            break;
        }
        switch ( brd[ i ][ sz - i - 1 ] )
        {
        case CellState::e_Mine:
            ++cnt_diagonal2_mine;
            break;
        case CellState::e_Opponent:
            ++cnt_diagonal2_opponent;
            break;
        }
    }

    if ( cnt_diagonal1_mine == sz || cnt_diagonal2_mine == sz )
    {
        return Result::e_Result_Hit;
    }

    if ( cnt_diagonal1_opponent == sz || cnt_diagonal2_opponent == sz )
    {
        return Result::e_Result_Miss;
    }

    if ( cnt_available == 0 )
    {
        if ( big_board )
        {
            return cnt_all_mine > cnt_all_opponent
                       ? Result::e_Result_Hit
                       : cnt_all_mine < cnt_all_opponent ? Result::e_Result_Miss
                                                         : Result::e_Result_Draw;
        }

        return Result::e_Result_Draw;
    }

    return Result::e_Result_NotFinished;
}

Cell
get_board_coord( const Cell& cell )
{
    return {cell.row / SMALL_BRD_SZ, cell.col / SMALL_BRD_SZ};
}

Cell
get_cell_coord( const Cell& cell )
{
    return {cell.row % SMALL_BRD_SZ, cell.col % SMALL_BRD_SZ};
}

struct MctsState
{
    MctsState( Board< Board< CellState > > board, bool turn, Cell last_move = {-1, -1} )
        : m_board( board )
        , m_big_board_states( SMALL_BRD_SZ,
                              std::vector< CellState >( SMALL_BRD_SZ, CellState::e_Available ) )
        , m_turn( turn )
        , m_last_move( last_move )
    {
    }

    Result
    simulate( ) const
    {
        auto temp_state = std::unique_ptr< MctsState >( new MctsState( *this ) );

        Result result = Result::e_Result_NotFinished;

        while ( ( result = game::game_state( temp_state->m_big_board_states, true ) )
                == Result::e_Result_NotFinished )
        {
            auto const possible_moves = temp_state->get_possible_moves( );

            temp_state->play_move( possible_moves[ rand( ) % possible_moves.size( ) ] );
        }

        return result;
    }

    ChildrenPtr
    get_children( MctsNodePtr parent ) const
    {
        auto possible_moves = get_possible_moves( );

        auto possible_children = ChildrenPtr( new Children( ) );
        possible_children->reserve( possible_moves.size( ) );

        for ( auto const& possible_move : possible_moves )
        {
            auto temp_state = std::unique_ptr< MctsState >( new MctsState( *this ) );
            temp_state->play_move( possible_move );
            possible_children->push_back(
                std::make_shared< MctsNode >( std::move( temp_state ), parent ) );
        }

        return std::move( possible_children );
    }

    const Cell&
    get_last_move( ) const
    {
        return m_last_move;
    }

private:
    Moves
    get_possible_moves( ) const
    {
        // Last cell position on the small board = next board
        auto const target_board_coord
            = m_last_move.row < 0 ? m_last_move : get_cell_coord( m_last_move );

        Moves possible_moves;

        if ( target_board_coord.row < 0
             || m_big_board_states[ target_board_coord.row ][ target_board_coord.col ]
                    != CellState::e_Available )
        {
            // all boards are possible
            for ( size_t i = 0; i < SMALL_BRD_SZ; ++i )
            {
                for ( size_t j = 0; j < SMALL_BRD_SZ; ++j )
                {
                    if ( m_big_board_states[ i ][ j ] == CellState::e_Available )
                    {
                        append_board_moves( possible_moves, i, j );
                    }
                }
            }
        }
        else
        {
            append_board_moves( possible_moves, target_board_coord.row, target_board_coord.col );
        }

        return possible_moves;
    }

    void
    play_move( const Cell& cell )
    {
        auto const target_board_coord = get_board_coord( cell );
        auto& target_board = m_board[ target_board_coord.row ][ target_board_coord.col ];

        auto const row_cell = cell.row - ( target_board_coord.row * SMALL_BRD_SZ );
        auto const col_cell = cell.col - ( target_board_coord.col * SMALL_BRD_SZ );
        auto& target_cell = target_board[ row_cell ][ col_cell ];

        target_cell = m_turn ? CellState::e_Mine : CellState::e_Opponent;
        auto target_board_result = game::game_state( target_board, false );
        if ( target_board_result != Result::e_Result_NotFinished )
        {
            m_big_board_states[ target_board_coord.row ][ target_board_coord.col ]
                = target_board_result == Result::e_Result_Draw ? CellState::e_Common : target_cell;
        }
        m_last_move = cell;
        m_turn = !m_turn;
    }

    void
    append_board_moves( Moves& moves, size_t row, size_t col ) const
    {
        auto const& brd = m_board[ row ][ col ];
        int const row_offset = SMALL_BRD_SZ * row;
        int const col_offset = SMALL_BRD_SZ * col;

        for ( int i = 0; i < SMALL_BRD_SZ; ++i )
        {
            for ( int j = 0; j < SMALL_BRD_SZ; ++j )
            {
                if ( brd[ i ][ j ] == CellState::e_Available )
                {
                    moves.push_back( {row_offset + i, col_offset + j} );
                }
            }
        }
    }

private:
    Board< Board< CellState > > m_board;
    Board< CellState > m_big_board_states;
    bool m_turn;
    Cell m_last_move;
};

struct MctsNode : public std::enable_shared_from_this< MctsNode >
{
    explicit MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent = nullptr )
        : m_state( std::move( state ) )
        , m_parent( parent )
        , m_hits( 0 )
        , m_misses( 0 )
        , m_total_trials( 0 )
    {
    }

    MctsNodePtr
    choose_child( )
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
    get_state( ) const
    {
        return *m_state;
    }

    MctsNodePtr
    find_child( std::function< bool( const MctsState& ) > predicate ) const
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

        assert( false );
        std::cerr << "MctsNode::find_child(): return nullptr" << std::endl;
        return nullptr;
    }

private:
    void
    run_simulation( )
    {
        static const size_t BATCH_SIZE = 8;
        std::vector< std::future< void > > results;

        for ( size_t i = 0; i < BATCH_SIZE; ++i )
        {
            results.emplace_back(
                threadpool::pool.enqueue( [this] { back_propagate( m_state->simulate( ) ); } ) );
        }

        for ( auto& result : results )
        {
            result.get( );
        }
    }

    MctsNodePtr
    explore_and_exploit( )
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
            auto best_potential = std::numeric_limits< float >::lowest( );
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
    back_propagate( Result result )
    {
        if ( result == Result::e_Result_Hit )
        {
            ++m_hits;
        }
        else if ( result == Result::e_Result_Miss )
        {
            ++m_misses;
        }
        ++m_total_trials;

        auto parent_strong_ref = m_parent.lock( );
        if ( parent_strong_ref )
        {
            parent_strong_ref->back_propagate( result );
        }
    }

    double
    child_potential( const MctsNode& child ) const
    {
        float const w = child.m_total_trials - child.m_misses - child.m_misses;
        // float const w = child.m_hits - child.m_misses;
        float const n = child.m_total_trials;
        static float const c = SQRT_OF_TWO - 1.2;
        float const t = m_total_trials;
        return w / n + c * sqrt( log( t ) / n );
    }

private:
    std::unique_ptr< MctsState > m_state;
    std::weak_ptr< MctsNode > m_parent;

    std::atomic_int m_hits;
    std::atomic_int m_misses;
    std::atomic_int m_total_trials;

    ChildrenPtr m_children;
};

struct TicTacToeGameAI
{
    explicit TicTacToeGameAI( const Cell& opponent_move = {-1, -1} )
    //: m_worker( [this]( ) { m_current_node.lock( )->choose_child( ); } )
    {
        Board< Board< CellState > > board(
            SMALL_BRD_SZ,
            std::vector< Board< CellState > >(
                SMALL_BRD_SZ,
                Board< CellState >( SMALL_BRD_SZ, std::vector< CellState >(
                                                      SMALL_BRD_SZ, CellState::e_Available ) ) ) );

        if ( opponent_move.row >= 0 )
        {
            auto const board_coord = game::get_board_coord( opponent_move );
            auto const cell_coord = game::get_cell_coord( opponent_move );
            board[ board_coord.row ][ board_coord.col ][ cell_coord.row ][ cell_coord.col ]
                = CellState::e_Opponent;
        }

        std::unique_ptr< MctsState > state(
            new MctsState( std::move( board ), true, opponent_move ) );
        m_tree = std::make_shared< MctsNode >( std::move( state ) );

        auto const end_time = std::chrono::duration_cast< std::chrono::milliseconds >(
                                  std::chrono::system_clock::now( ).time_since_epoch( ) )
                              + std::chrono::milliseconds( 995 );

        make_move( m_tree, end_time );

        // m_worker.processing( );
    }

    void
    opponent_move( const Cell& position )
    {
        auto const end_time = std::chrono::duration_cast< std::chrono::milliseconds >(
                                  std::chrono::system_clock::now( ).time_since_epoch( ) )
                              + std::chrono::milliseconds( 120 );

        // m_worker.idle( );

        auto current_node_strong_ref = m_current_node.lock( );

        // Ensure current node has children
        current_node_strong_ref->choose_child( );

        // Get opponent child
        auto opponent_child = current_node_strong_ref->find_child(
            [&position]( const MctsState& state ) { return state.get_last_move( ) == position; } );

        make_move( opponent_child, end_time );

        // m_worker.processing( );
    }

    Cell
    get_ai_move( ) const
    {
        return m_current_node.lock( )->get_state( ).get_last_move( );
    }

private:
    void
    make_move( MctsNodePtr node, std::chrono::milliseconds end_time )
    {
        while ( std::chrono::duration_cast< std::chrono::milliseconds >(
                    std::chrono::system_clock::now( ).time_since_epoch( ) )
                < end_time )
        {
            m_current_node = node->choose_child( );
        }
    }

private:
    MctsNodePtr m_tree;
    std::weak_ptr< MctsNode > m_current_node;
    // threadpool::AsyncWorker m_worker;
};
}  // namespace game

namespace
{
const bool srand_init = []( ) {
    srand( static_cast< unsigned int >( time( NULL ) ) );
    return true;
}( );
}

using Brd = std::vector< std::vector< char > >;

const size_t SMALL_BRD_SZ = game::SMALL_BRD_SZ;
const size_t BIG_BRD_SIZE = SMALL_BRD_SZ * SMALL_BRD_SZ;

void
print_board( const Brd& brd )
{
    auto const big = brd.size( ) > SMALL_BRD_SZ;
    auto const header_footer = big ? "#############" : "#####";

    std::cout << header_footer << std::endl;
    for ( size_t i = 0; i < ( big ? BIG_BRD_SIZE : SMALL_BRD_SZ ); ++i )
    {
        std::cout << "#";
        for ( size_t j = 0; j < ( big ? BIG_BRD_SIZE : SMALL_BRD_SZ ); ++j )
        {
            std::cout << brd[ i ][ j ];
            if ( ( j + 1 ) % SMALL_BRD_SZ == 0 )
            {
                std::cout << '#';
            }
        }
        if ( ( i + 1 ) % SMALL_BRD_SZ == 0 )
        {
            std::cout << "\n" << header_footer;
        }
        std::cout << std::endl;
    }
}

int
main( )
{
    Brd visualization( BIG_BRD_SIZE, std::vector< char >( BIG_BRD_SIZE, '-' ) );

    game::Cell pos = {1, 1};
    if ( pos.row >= 0 )
    {
        visualization[ pos.row ][ pos.col ] = '0';
    }

    auto game = std::make_shared< game::TicTacToeGameAI >( pos );

    bool cont = true;
    while ( cont )
    {
        auto ai_move = game->get_ai_move( );
        visualization[ ai_move.row ][ ai_move.col ] = 'X';
        print_board( visualization );

        std::cin >> pos.row >> pos.col;
        visualization[ pos.row ][ pos.col ] = '0';
        game->opponent_move( pos );
    }
}
