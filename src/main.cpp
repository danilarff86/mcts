#include <algorithm>
#include <array>
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
#include <stack>
#include <stdexcept>
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
using WeightType = double;

WeightType const SQRT_OF_TWO = sqrt( 2 );

static const uint32_t SZ_SIDE_SMALL = 3;
static const uint32_t SZ_CELLS_SMALL = SZ_SIDE_SMALL * SZ_SIDE_SMALL;
static const uint32_t SZ_SIDE_BIG = SZ_SIDE_SMALL * SZ_SIDE_SMALL;
static const uint32_t SZ_CELLS_BIG = SZ_SIDE_BIG * SZ_SIDE_BIG;
static const uint32_t SZ_NUM_BOARDS_BIG = SZ_SIDE_SMALL * SZ_SIDE_SMALL;

using GameBoardArrangement = uint32_t;
using SmallBoardArrangement = GameBoardArrangement;
using BigBoardArrangement = GameBoardArrangement;
using GameBoard = std::array< SmallBoardArrangement, SZ_NUM_BOARDS_BIG >;

struct Cell
{
    int row;
    int col;
};

static uint8_t cell2index[ SZ_SIDE_BIG ][ SZ_SIDE_BIG ]
    = {{0, 1, 2, 9, 10, 11, 18, 19, 20},     {3, 4, 5, 12, 13, 14, 21, 22, 23},
       {6, 7, 8, 15, 16, 17, 24, 25, 26},    {27, 28, 29, 36, 37, 38, 45, 46, 47},
       {30, 31, 32, 39, 40, 41, 48, 49, 50}, {33, 34, 35, 42, 43, 44, 51, 52, 53},
       {54, 55, 56, 63, 64, 65, 72, 73, 74}, {57, 58, 59, 66, 67, 68, 75, 76, 77},
       {60, 61, 62, 69, 70, 71, 78, 79, 80}};

static Cell index2cell[ SZ_CELLS_BIG ]
    = {{0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2},

       {0, 3}, {0, 4}, {0, 5}, {1, 3}, {1, 4}, {1, 5}, {2, 3}, {2, 4}, {2, 5},

       {0, 6}, {0, 7}, {0, 8}, {1, 6}, {1, 7}, {1, 8}, {2, 6}, {2, 7}, {2, 8},

       {3, 0}, {3, 1}, {3, 2}, {4, 0}, {4, 1}, {4, 2}, {5, 0}, {5, 1}, {5, 2},

       {3, 3}, {3, 4}, {3, 5}, {4, 3}, {4, 4}, {4, 5}, {5, 3}, {5, 4}, {5, 5},

       {3, 6}, {3, 7}, {3, 8}, {4, 6}, {4, 7}, {4, 8}, {5, 6}, {5, 7}, {5, 8},

       {6, 0}, {6, 1}, {6, 2}, {7, 0}, {7, 1}, {7, 2}, {8, 0}, {8, 1}, {8, 2},

       {6, 3}, {6, 4}, {6, 5}, {7, 3}, {7, 4}, {7, 5}, {8, 3}, {8, 4}, {8, 5},

       {6, 6}, {6, 7}, {6, 8}, {7, 6}, {7, 7}, {7, 8}, {8, 6}, {8, 7}, {8, 8}};

enum CellState : uint8_t
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
    e_Result_Hit = 2,
    e_Result_Draw = 3
};

struct GameBoardArrangements
{
    struct BoardResults
    {
        Result small_board;
        Result big_board;
    };

    GameBoardArrangements( )
    {
        for ( uint32_t i = 0; i < ALL_POSSIBLE_ARRANGEMENTS; ++i )
        {
            m_all_arrangemets[ i ] = calc_game_state( i );
        }
    }

    inline BoardResults const& operator[]( const GameBoardArrangement arrangement ) const
    {
        return m_all_arrangemets[ arrangement ];
    }

private:
    BoardResults
    calc_game_state( const GameBoardArrangement board )
    {
        // Check rows and cols
        static const uint32_t ROW_MASK = 0x3F;
        static const uint32_t COL_MASK = 0x30C3;
        static const uint32_t DIAGONAL1_MASK = 0x30303;
        static const uint32_t DIAGONAL2_MASK = 0x3330;
        for ( size_t i = 0; i < SZ_SIDE_SMALL; ++i )
        {
            // Row
            switch ( ( board >> ( i * 6 ) ) & ROW_MASK )
            {
            case 0x15:
                return {Result::e_Result_Miss, Result::e_Result_Miss};
            case 0x2A:
                return {Result::e_Result_Hit, Result::e_Result_Hit};
            }

            // Col
            switch ( ( board << ( i * 2 ) ) & COL_MASK )
            {
            case 0x1041:
                return {Result::e_Result_Miss, Result::e_Result_Miss};
            case 0x2082:
                return {Result::e_Result_Hit, Result::e_Result_Hit};
            }
        }

        // Diagonal 1
        switch ( board & DIAGONAL1_MASK )
        {
        case 0x10101:
            return {Result::e_Result_Miss, Result::e_Result_Miss};
        case 0x20202:
            return {Result::e_Result_Hit, Result::e_Result_Hit};
        }
        switch ( board & DIAGONAL2_MASK )
        {
        case 0x1110:
            return {Result::e_Result_Miss, Result::e_Result_Miss};
        case 0x2220:
            return {Result::e_Result_Hit, Result::e_Result_Hit};
        }

        size_t cnt_available = 0;
        size_t cnt_all_opponent = 0;
        size_t cnt_all_mine = 0;
        auto temp_board = board;
        for ( size_t i = 0; i < SZ_CELLS_SMALL; ++i )
        {
            switch ( temp_board & 0x3 )
            {
            case 0:
                ++cnt_available;
                break;
            case 1:
                ++cnt_all_opponent;
                break;
            case 2:
                ++cnt_all_mine;
                break;
            }
            temp_board >>= 2;
        }

        if ( cnt_available == 0 )
        {
            return {Result::e_Result_Draw, cnt_all_mine > cnt_all_opponent
                                               ? Result::e_Result_Hit
                                               : cnt_all_mine < cnt_all_opponent
                                                     ? Result::e_Result_Miss
                                                     : Result::e_Result_Draw};
        }

        return {Result::e_Result_NotFinished, Result::e_Result_NotFinished};
    }

private:
    static const size_t ALL_POSSIBLE_ARRANGEMENTS = 262144;
    BoardResults m_all_arrangemets[ ALL_POSSIBLE_ARRANGEMENTS ];
};

static const GameBoardArrangements arrangements;

struct MctsNode;
using MctsNodePtr = std::shared_ptr< MctsNode >;
using Children = std::vector< MctsNodePtr >;

struct MctsState
{
    MctsState( GameBoard board, uint8_t turn, uint8_t last_move = INVALID_MOVE )
        : m_board( board )
        , m_small_boards_state( 0 )
        , m_turn( turn )
        , m_last_move( last_move )
    {
    }

    inline void
    get_children( MctsNodePtr parent, Children& children ) const
    {
        uint8_t possible_moves[ SZ_CELLS_BIG ];
        uint8_t possible_moves_count = 0;
        get_possible_moves( possible_moves, possible_moves_count );

        children.reserve( possible_moves_count );
        for ( uint8_t i = 0; i < possible_moves_count; ++i )
        {
            auto temp_state = std::unique_ptr< MctsState >( new MctsState( *this ) );
            temp_state->play_move( possible_moves[ i ] );
            children.push_back( std::make_shared< MctsNode >( std::move( temp_state ), parent ) );
        }
    }

    inline Result
    simulate( ) const
    {
        auto temp_state = *this;

        Result result = Result::e_Result_NotFinished;
        uint8_t possible_moves[ SZ_CELLS_BIG ];
        uint8_t possible_moves_count = 0;

        while ( ( result = arrangements[ temp_state.m_small_boards_state ].big_board )
                == Result::e_Result_NotFinished )
        {
            temp_state.get_possible_moves( possible_moves, possible_moves_count );

            temp_state.play_move( possible_moves[ rand( ) % possible_moves_count ] );
        }

        return result;
    }

    inline const uint8_t
    get_last_move( ) const
    {
        return m_last_move;
    }

private:
    inline void
    play_move( const uint8_t move )
    {
        uint8_t const target_board_index = move / SZ_NUM_BOARDS_BIG;
        uint8_t const target_cell_index = move - ( target_board_index * SZ_NUM_BOARDS_BIG );
        auto& target_board = m_board[ target_board_index ];

        auto cell_state = m_turn ? CellState::e_Mine : CellState::e_Opponent;
        target_board |= cell_state << ( target_cell_index * 2 );
        auto const target_board_result = arrangements[ target_board ].small_board;
        if ( target_board_result != Result::e_Result_NotFinished )
        {
            auto const target_board_state_for_big_board
                = target_board_result == Result::e_Result_Draw ? CellState::e_Common : cell_state;
            m_small_boards_state |= target_board_state_for_big_board << ( target_board_index * 2 );
        }
        m_last_move = move;
        m_turn ^= 1;
    }

    inline void
    get_possible_moves( uint8_t* out_moves, uint8_t& out_count ) const
    {
        out_count = 0;

        // Last cell position on the small board = next board
        uint8_t const target_board = m_last_move % SZ_NUM_BOARDS_BIG;

        if ( m_last_move == INVALID_MOVE
             || get_small_brd_state( target_board ) != Result::e_Result_NotFinished )
        {
            for ( uint8_t i = 0; i < SZ_NUM_BOARDS_BIG; ++i )
            {
                if ( get_small_brd_state( i ) == Result::e_Result_NotFinished )
                {
                    append_board_moves( i, out_moves, out_count );
                }
            }
        }
        else
        {
            append_board_moves( target_board, out_moves, out_count );
        }
    }

    inline void
    append_board_moves( const uint8_t board_num, uint8_t* out_moves, uint8_t& out_count ) const
    {
        uint8_t const board_offset = SZ_NUM_BOARDS_BIG * board_num;

        auto temp_board = m_board[ board_num ];
        for ( uint8_t i = 0; i < SZ_CELLS_SMALL; ++i )
        {
            if ( ( temp_board & 0x3 ) == 0 )
            {
                out_moves[ out_count++ ] = board_offset + i;
            }
            temp_board >>= 2;
        }
    }

    inline Result
    get_small_brd_state( uint8_t board_num ) const
    {
        return static_cast< Result >( ( m_small_boards_state >> ( board_num * 2 ) ) & 0x3 );
    }

private:
    static const uint8_t INVALID_MOVE = 0xFF;

    GameBoard m_board;
    BigBoardArrangement m_small_boards_state;
    uint8_t m_turn;
    uint8_t m_last_move;
};

struct MctsNode : std::enable_shared_from_this< MctsNode >
{
    explicit MctsNode( std::unique_ptr< MctsState > state, MctsNodePtr parent = nullptr )
        : m_state( std::move( state ) )
        , m_parent( parent )
        , m_hits( 0 )
        , m_misses( 0 )
        , m_total_trials( 0 )
        , m_children_received( false )
    {
    }

    void
    print_details( MctsNode const& node_selected ) const
    {
        auto& out = std::cerr;

        auto print_top_moves = [&out]( MctsNode const& node, bool opponent,
                                       size_t max_children = 5 ) {
            struct Statistics
            {
                Cell cell;
                int hits;
                int total;
                double win_rate;

                bool
                operator<( const Statistics& rhs )
                {
                    return win_rate > rhs.win_rate;
                }
            };

            std::vector< Statistics > stat( node.m_children.size( ) );
            for ( size_t i = 0; i < stat.size( ); ++i )
            {
                auto const& child = *( node.m_children[ i ] );
                stat[ i ].cell = index2cell[ child.get_state( ).get_last_move( ) ];
                stat[ i ].hits = opponent ? child.m_misses : child.m_hits;
                stat[ i ].total = child.m_total_trials;
                stat[ i ].win_rate = double( stat[ i ].hits ) * 100. / double( stat[ i ].total );
            }

            std::sort( stat.begin( ), stat.end( ) );

            for ( size_t i = 0; i < max_children && i < stat.size( ); ++i )
            {
                auto const& entry = stat[ i ];
                out << "'" << entry.cell.row << " " << entry.cell.col
                    << "' - win rate: " << entry.win_rate << "%, hits/total: " << entry.hits << "/"
                    << entry.total << std::endl;
            }
        };

        out << "Simulations hits/total: " << m_hits << "/" << m_total_trials << std::endl;
        out << "-------------------------" << std::endl;
        out << "Potential moves:" << std::endl;
        print_top_moves( *this, false );
        out << "-------------------------" << std::endl;
        out << "Potential opponent moves:" << std::endl;
        print_top_moves( node_selected, true );
        out << "-------------------------" << std::endl;
    }

    inline void
    remove_parent_link( )
    {
        m_parent = std::shared_ptr< MctsNode >( );
    }

    inline MctsState const&
    get_state( ) const
    {
        return *m_state;
    }

    inline MctsNodePtr
    find_child( std::function< bool( const MctsState& ) > predicate ) const
    {
        if ( !m_children.empty( ) )
        {
            for ( auto& child : m_children )
            {
                if ( predicate( *( child->m_state ) ) )
                {
                    return child;
                }
            }
        }

        std::cerr << "MctsNode::find_child(): return nullptr" << std::endl;
        return nullptr;
    }

    inline MctsNodePtr
    choose_child( )
    {
        if ( !m_children_received )
        {
            m_state->get_children( shared_from_this( ), m_children );
            m_children_received = true;
        }

        if ( m_children.empty( ) )
        {
            run_simulation( );
        }
        else
        {
            return explore_and_exploit( );
        }

        return nullptr;
    }

private:
    inline MctsNodePtr
    explore_and_exploit( )
    {
        Children unexplored;
        std::copy_if( m_children.begin( ), m_children.end( ), std::back_inserter( unexplored ),
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
            auto best_potential = std::numeric_limits< WeightType >::lowest( );
            for ( auto const& child : m_children )
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

    inline void
    run_simulation( )
    {
        // back_propagate( m_state->simulate( ) );
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

    inline void
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

    inline WeightType
    child_potential( MctsNode& child ) const
    {
        WeightType const w = child.m_hits;
        // float const w = child.m_hits - child.m_misses;
        WeightType const n = child.m_total_trials;
        static WeightType const c = SQRT_OF_TWO;
        WeightType const t = m_total_trials;
        return w / n + c * sqrt( log( t ) / n );
    }

private:
    std::unique_ptr< MctsState > m_state;
    std::weak_ptr< MctsNode > m_parent;

    std::atomic_int m_hits;
    std::atomic_int m_misses;
    std::atomic_int m_total_trials;

    Children m_children;
    bool m_children_received;
};

struct TicTacToeGameAI
{
    explicit TicTacToeGameAI( const Cell& opponent_move = {-1, -1} )
    {
        GameBoard board{};

        auto move_index = 0xFF;

        if ( opponent_move.row >= 0 )
        {
            move_index = cell2index[ opponent_move.row ][ opponent_move.col ];
            uint8_t const target_board_index = move_index / SZ_NUM_BOARDS_BIG;
            uint8_t const target_cell_index
                = move_index - ( target_board_index * SZ_NUM_BOARDS_BIG );
            board[ target_board_index ] |= CellState::e_Opponent << ( target_cell_index * 2 );
        }

        std::unique_ptr< MctsState > state( new MctsState( board, 1, move_index ) );
        m_tree = std::make_shared< MctsNode >( std::move( state ) );

        auto const end_time = std::chrono::duration_cast< std::chrono::milliseconds >(
                                  std::chrono::system_clock::now( ).time_since_epoch( ) )
                              + std::chrono::milliseconds( 995 );

        make_move( m_tree, end_time );
    }

    void
    opponent_move( const Cell& position )
    {
        auto const end_time = std::chrono::duration_cast< std::chrono::milliseconds >(
                                  std::chrono::system_clock::now( ).time_since_epoch( ) )
                              + std::chrono::milliseconds( 100 );

        auto current_node_strong_ref = m_current_node.lock( );

        // Ensure current node has children
        current_node_strong_ref->choose_child( );

        auto move_index = cell2index[ position.row ][ position.col ];

        // Get opponent child
        auto opponent_child
            = current_node_strong_ref->find_child( [move_index]( const MctsState& state ) {
                  return state.get_last_move( ) == move_index;
              } );

        make_move( opponent_child, end_time );
    }

    Cell
    get_ai_move( ) const
    {
        return index2cell[ m_current_node.lock( )->get_state( ).get_last_move( ) ];
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

        node->print_details( *( m_current_node.lock( ) ) );
    }

private:
    MctsNodePtr m_tree;
    std::weak_ptr< MctsNode > m_current_node;
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

const size_t SMALL_BRD_SZ = game::SZ_SIDE_SMALL;
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

    game::Cell pos = {4, 4};
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
