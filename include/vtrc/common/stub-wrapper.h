#ifndef VTRC_STUB_WRAPPER_H
#define VTRC_STUB_WRAPPER_H

#include "vtrc-memory.h"

namespace google { namespace protobuf {
    class RpcChannel;
    class RpcController;
    class Closure;
    class Message;
}}

namespace vtrc { namespace common {

    template < typename StubType,
               typename ChannelType = google::protobuf::RpcChannel
             >
    class stub_wrapper {

        /// compiletime derived check;
        /// StubType must be derived from google::protobuf::RpcChannel
//        typedef google::protobuf::RpcChannel gpb_channel;
//        static const char tc__( gpb_channel * );
//        enum {
//            is_gpb_channel__ = sizeof( tc__( static_cast<ChannelType *>(0) ) )
//        };

    public:

        typedef StubType    stub_type;
        typedef ChannelType channel_type;

    private:

        typedef stub_wrapper<stub_type> this_type;

        vtrc::shared_ptr<channel_type>   channel_holder_;
        channel_type                    *channel_;
        stub_type                        stub_;

        template <typename ReqType, typename ResType>
        struct protobuf_call {
            typedef ReqType req_type;
            typedef ResType res_type;
            typedef void (stub_type::* type)
                (google::protobuf::RpcController *,
                 const req_type *, res_type *, google::protobuf::Closure *);
        };

        template <typename FuncType>
        struct call_args;

        template <typename RequestType, typename ResponseType>
        struct call_args <void (stub_type::*)
                               (google::protobuf::RpcController *,
                                const RequestType *, ResponseType *,
                                google::protobuf::Closure * )>
        {
            typedef RequestType  req_type;
            typedef ResponseType res_type;
        };

    public:

        channel_type *channel( )
        {
            return channel_;
        }

        const channel_type *channel( ) const
        {
            return channel_;
        }

        stub_wrapper(channel_type *channel, bool own_channel = false)
            :channel_holder_(own_channel ? channel : NULL)
            ,channel_(channel)
            ,stub_(channel_)
        { }

        stub_wrapper(vtrc::shared_ptr<channel_type> channel)
            :channel_holder_(channel)
            ,channel_(channel_holder_.get( ))
            ,stub_(channel_)
        { }

        /// call(controller, request, response, closure)
        template <typename StubFuncType, typename ReqType, typename ResType>
        void call( StubFuncType func,
                   google::protobuf::RpcController *controller,
                   const ReqType *request, ResType *response,
                   google::protobuf::Closure *closure )
        {
            (stub_.*func)(controller, request, response, closure);
        }

        /// call( request, response )
        template <typename StubFuncType, typename ReqType, typename ResType>
        void call( StubFuncType func, const ReqType *request, ResType *response)
        {
            (stub_.*func)(NULL, request, response, NULL);
        }

        /// call(  )
        template <typename StubFuncType>
        void call( StubFuncType func )
        {
            (stub_.*func)(NULL, NULL, NULL, NULL);
        }

        /// call( request ) send request. Don't need response
        template <typename StubFuncType, typename RequestType>
        void call_request( StubFuncType func, const RequestType *request)
        {
            (stub_.*func)(NULL, request, NULL, NULL);
        }

        /// call( response ) send empty request and get response
        template <typename StubFuncType, typename ResponseType>
        void call_response( StubFuncType func, ResponseType *response )
        {
            (stub_.*func)(NULL, NULL, response, NULL);
        }

    };

}}

#endif // VTRC_STUB_WRAPPER_H
