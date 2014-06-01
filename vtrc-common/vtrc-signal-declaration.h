#ifndef VTRC_SIGNAL_DECLARATION_H
#define VTRC_SIGNAL_DECLARATION_H

#include "boost/signals2/signal.hpp"
#include "boost/signals2/signal_type.hpp"
#include "boost/signals2/dummy_mutex.hpp"
#include "boost/preprocessor/cat.hpp"

#include "vtrc-mutex.h"

// ====== SAFE
#define VTRC_DECLARE_SIGNAL_SAFE( name, sig_type )                             \
    public:                                                                    \
        typedef boost::signals2::signal_type<                                  \
            sig_type,                                                          \
            boost::signals2::keywords::mutex_type<vtrc::mutex>                 \
        >::type BOOST_PP_CAT( name, _type );                                   \
        typedef BOOST_PP_CAT( name, _type )::slot_type                         \
                BOOST_PP_CAT( name, _slot_type );                              \
        typedef BOOST_PP_CAT( name, _type )::group_type                        \
                BOOST_PP_CAT( name, _group_type );                             \
        template <typename SlotType>                                           \
        boost::signals2::connection BOOST_PP_CAT(name, _connect)( SlotType s ) \
        { return BOOST_PP_CAT( name, _ ).connect( s ); }                       \
        void BOOST_PP_CAT( name, _disconnect_all_slots )( )                    \
        { BOOST_PP_CAT( name, _ ).disconnect_all_slots( ); }                   \
    protected:                                                                 \
        BOOST_PP_CAT( name, _type )&  BOOST_PP_CAT( get_, name )( )            \
        { return BOOST_PP_CAT( name, _ ); }                                    \
    private:                                                                   \
        BOOST_PP_CAT( name, _type ) BOOST_PP_CAT( name, _ )

// ====== UNSAFE

#define VTRC_DECLARE_SIGNAL_UNSAFE( name, sig_type )                           \
    public:                                                                    \
        typedef boost::signals2::signal_type<                                  \
            sig_type,                                                          \
            boost::signals2::keywords::mutex_type <                            \
                  boost::signals2::dummy_mutex                                 \
            >                                                                  \
        >::type BOOST_PP_CAT( name, _type );                                   \
        typedef BOOST_PP_CAT( name, _type )::slot_type                         \
                BOOST_PP_CAT( name, _slot_type );                              \
        typedef BOOST_PP_CAT( name, _type )::group_type                        \
                BOOST_PP_CAT( name, _group_type );                             \
        template <typename SlotType>                                           \
        boost::signals2::connection BOOST_PP_CAT(name, _connect)( SlotType s ) \
        { return BOOST_PP_CAT( name, _ ).connect( s ); }                       \
        void BOOST_PP_CAT( name, _disconnect_all_slots )( )                    \
        { BOOST_PP_CAT( name, _ ).disconnect_all_slots( ); }                   \
    protected:                                                                 \
        BOOST_PP_CAT( name, _type )&  BOOST_PP_CAT( get_, name )( )            \
        { return BOOST_PP_CAT( name, _ ); }                                    \
    private:                                                                   \
        BOOST_PP_CAT( name, _type ) BOOST_PP_CAT( name, _ )

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
        template <typename SlotType>
        boost::signals2::connection  on_event_connect( SlotType s )
        { return on_event_.connect( s ); }
        void on_event_disconnect_all_slots( )
        { on_event_.disconnect_all_slots( ); }
    protected:
        on_events_type &get_on_event( ) { return on_event_; }
    private:
        on_event_type on_event_;
}
==================

*/


#endif // VTRC_SIGNAL_DECLARATION_H
