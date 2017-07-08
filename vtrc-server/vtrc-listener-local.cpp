
#include "vtrc/server/listener-local.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners {

#ifdef  _WIN32
    namespace win_pipe {
        listener_sptr create( application &app, const std::string &name );
        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &name );
    }
    namespace local_namespace = win_pipe;
#else
    namespace unix_local {
        listener_sptr create( application &app, const std::string &name );
        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &name );
    }
    namespace local_namespace = unix_local;
#endif

    namespace local {
        listener_sptr create( application &app, const std::string &name )
        {
            return local_namespace::create( app, name );
        }

        listener_sptr create(application &app,
                             const rpc::session_options &opts,
                             const std::string &name )
        {
            return local_namespace::create( app, opts, name );
        }
    }

    }


}}
