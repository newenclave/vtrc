#ifndef VTRC_ENDPOINT_TCP_H
#define VTRC_ENDPOINT_TCP_H

namespace vtrc { namespace server {

    struct endpoint_iface;
    struct application_iface;

    namespace endpoints { namespace tcp {
        endpoint_iface *create( application_iface &app,
                                const std::string &address,
                                unsigned short service );
    }}

}}

#endif // VTRCENDPOINTTCP_H
