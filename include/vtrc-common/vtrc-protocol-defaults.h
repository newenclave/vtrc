#ifndef VTRC_PROTOCOL_DEFAULTS_H
#define VTRC_PROTOCOL_DEFAULTS_H


namespace vtrc { namespace rpc {
    class session_options;
    class unit_options;
    class options;
}}


namespace vtrc { namespace common { namespace defaults {

    vtrc::rpc::session_options session_options( );
    vtrc::rpc::unit_options    unit_options( );
    vtrc::rpc::options         method_options( );

}}}

#endif // VTRC_PROTOCOL_DEFAULTS_H
