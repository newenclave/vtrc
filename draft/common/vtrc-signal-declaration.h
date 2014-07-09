#ifndef VTRC_SIGNAL_DECLARATION_H
#define VTRC_SIGNAL_DECLARATION_H

#include "boost/signals2/signal.hpp"
#include "boost/signals2/signal_type.hpp"
#include "boost/signals2/dummy_mutex.hpp"
#include "boost/preprocessor/cat.hpp"
#include "boost/thread/mutex.hpp"

//#define VTRC_PP_CONCAT( a, b ) a ## b

#define VTRC_DECLARE_SIGNAL_COMMON( Access, Name, SigType, MutexType )         \
    public:                                                                    \
        typedef boost::signals2::signal_type <                                 \
            SigType,                                                           \
            boost::signals2::keywords::mutex_type<MutexType>                   \
        >::type BOOST_PP_CAT( Name, _type );                                   \
                                                                               \
        typedef BOOST_PP_CAT( Name, _type )::slot_type                         \
                BOOST_PP_CAT( Name, _slot_type );                              \
        typedef BOOST_PP_CAT( Name, _type )::group_type                        \
                BOOST_PP_CAT( Name, _group_type );                             \
                                                                               \
        template <typename SlotType>                                           \
        boost::signals2::connection                                            \
        BOOST_PP_CAT( Name, _connect )                                         \
             ( SlotType s,                                                     \
             boost::signals2::connect_position pos = boost::signals2::at_back )\
        {                                                                      \
            return BOOST_PP_CAT( Name, _ ).connect( s, pos );                  \
        }                                                                      \
                                                                               \
        void BOOST_PP_CAT( Name, _disconnect_all_slots )( )                    \
        {                                                                      \
            BOOST_PP_CAT( Name, _ ).disconnect_all_slots( );                   \
        }                                                                      \
    Access:                                                                    \
        BOOST_PP_CAT( Name, _type )&  BOOST_PP_CAT( get_, Name )( )            \
        {                                                                      \
            return BOOST_PP_CAT( Name, _ );                                    \
        }                                                                      \
    private:                                                                   \
        BOOST_PP_CAT( Name, _type ) BOOST_PP_CAT( Name, _ )


// ====== SAFE

#define VTRC_DECLARE_SIGNAL_SAFE(   Name, SigType )             \
        VTRC_DECLARE_SIGNAL_COMMON( protected, Name, SigType,   \
                                    boost::mutex )

// ====== UNSAFE
#define VTRC_DECLARE_SIGNAL_UNSAFE( Name, SigType )             \
        VTRC_DECLARE_SIGNAL_COMMON( protected, Name, SigType,   \
                                    boost::signals2::dummy_mutex)

#define VTRC_DECLARE_SIGNAL VTRC_DECLARE_SIGNAL_SAFE

/*

Usage:
class test {
    VTRC_DECLARE_SIGNAL(on_event, void( ));
};

result:
==================
class test {
    public:
        typedef boost::signals2::signal_type <
                void( ),
                boost::signals2::keywords::mutex_type<boost::mutex>
              >::type on_event_type;

        typedef on_event_type::slot_type on_event_slot_type;    // slot_type

        typedef on_event_type::group_type on_event_group_type;  // group_type

        /// connect to signal
        template <typename SlotType>
        boost::signals2::connection on_event_connect
            ( SlotType s,
              boost::signals2::connect_position pos = boost::signals2::at_back)
        {
            return on_event_.connect( s, pos );
        }

        /// disconnect all slots
        void on_event_disconnect_all_slots( )
        {
            on_event_.disconnect_all_slots( );
        }

    protected:
        on_events_type &get_on_event( ) { return on_event_; }
    private:
        on_event_type on_event_;
}
==================

*/


#endif // VTRC_SIGNAL_DECLARATION_H
