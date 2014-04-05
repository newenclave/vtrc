#ifndef VTRC_SIGNAL_DECLARATION_H
#define VTRC_SIGNAL_DECLARATION_H

#include <boost/signals2/signal.hpp>
#include <boost/signals2/signal_type.hpp>
#include <boost/signals2/dummy_mutex.hpp>

// ====== SAFE
#define VTRC_DECLARE_SIGNAL_SAFE( name, sig_type )                              \
    public:                                                                    \
      typedef boost::signals2::signal_type<                                    \
            sig_type,                                                          \
            boost::signals2::keywords::mutex_type<boost::mutex>                \
      >::type BOOST_PP_CAT( name, _type );                                     \
      typedef BOOST_PP_CAT( name, _type )::slot_type                           \
              BOOST_PP_CAT( name, _slot_type );                                \
      typedef BOOST_PP_CAT( name, _type )::group_type                          \
              BOOST_PP_CAT( name, _group_type );                               \
      BOOST_PP_CAT( name, _type )&  BOOST_PP_CAT( get_, name )()               \
      { return BOOST_PP_CAT( name, _ ); }                                      \
    private:                                                                   \
      BOOST_PP_CAT( name, _type ) BOOST_PP_CAT( name, _ );

// ====== UNSAFE
#define VTRC_DECLARE_SIGNAL_UNSAFE( name, sig_type )                            \
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
      BOOST_PP_CAT( name, _type )&  BOOST_PP_CAT( get_, name )()               \
      { return BOOST_PP_CAT( name, _ ); }                                      \
    private:                                                                   \
      BOOST_PP_CAT( name, _type ) BOOST_PP_CAT( name, _ );

#define VTRC_DECLARE_SIGNAL VTRC_DECLARE_SIGNAL_SAFE

#endif // VTRC_SIGNAL_DECLARATION_H
