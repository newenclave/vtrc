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
      typedef boost::signals2::signal_type<                                    \
            sig_type,                                                          \
            boost::signals2::keywords::mutex_type<vtrc::mutex>                 \
      >::type BOOST_PP_CAT( name, _type );                                     \
      typedef BOOST_PP_CAT( name, _type )::slot_type                           \
              BOOST_PP_CAT( name, _slot_type );                                \
      typedef BOOST_PP_CAT( name, _type )::group_type                          \
              BOOST_PP_CAT( name, _group_type );                               \
      BOOST_PP_CAT( name, _type )&  BOOST_PP_CAT( get_, name )( )              \
      { return BOOST_PP_CAT( name, _ ); }                                      \
    protected:                                                                 \
      BOOST_PP_CAT( name, _type ) BOOST_PP_CAT( name, _ )

// ====== UNSAFE
#define VTRC_DECLARE_SIGNAL_UNSAFE( name, sig_type )                           \
    public:                                                                    \
      typedef boost::signals2::signal_type<                                    \
            sig_type,                                                          \
            boost::signals2::keywords::mutex_type <                            \
                  boost::signals2::dummy_mutex                                 \
            >                                                                  \
      >::type BOOST_PP_CAT( name, _type );                                     \
      typedef BOOST_PP_CAT( name, _type )::slot_type                           \
              BOOST_PP_CAT( name, _slot_type );                                \
      typedef BOOST_PP_CAT( name, _type )::group_type                          \
              BOOST_PP_CAT( name, _group_type );                               \
      BOOST_PP_CAT( name, _type )&  BOOST_PP_CAT( get_, name )( )              \
      { return BOOST_PP_CAT( name, _ ); }                                      \
    protected:                                                                 \
      BOOST_PP_CAT( name, _type ) BOOST_PP_CAT( name, _ )

#define VTRC_DECLARE_SIGNAL VTRC_DECLARE_SIGNAL_SAFE


/*

Usage:
class test {
    VTRC_DECLARE_SIGNAL(on_event, void());
};

result:
==================
class test {
    public:
      typedef boost::signals2::signal_type <
                void(),
                boost::signals2::keywords::mutex_type<boost::mutex>
              >::type on_event_type;
      typedef on_event_type::slot_type on_event_slot_type;    // slot_type
      typedef on_event_type::group_type on_event_group_type;  // group_type
      on_events_type &get_on_event( ) { return on_event_; }
    protected:
      on_event_type on_event_;
}
==================

*/


#endif // VTRC_SIGNAL_DECLARATION_H
