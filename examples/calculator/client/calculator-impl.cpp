#include "calculator-iface.h"
#include "vtrc-client-base/vtrc-rpc-channel-c.h"
#include "vtrc-client-base/vtrc-client.h"
#include "protocol/calculator.pb.h"

namespace {
    struct calculator_impl: public interfaces::calculator {

        typedef vtrc_example::calculator_Stub stub_type;
        typedef vtrc_example::number number_type;
        typedef vtrc_example::number_pair number_pair_type;


        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_type                              stub_;
        mutable unsigned                               calls_count_;

        calculator_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> client )
            :channel_(client->create_channel( ))
            ,stub_(channel_.get( ))
            ,calls_count_(0)
        { }

        number_pair_type make_pair( double l, double r ) const
        {
            ++calls_count_;
            number_pair_type result;
            result.mutable_first( )->set_value( l );
            result.mutable_second( )->set_value( r );
            return result;
        }

        number_pair_type make_pair( std::string const &l, double r ) const
        {
            ++calls_count_;
            number_pair_type result;
            result.mutable_first( )->set_name( l );
            result.mutable_second( )->set_value( r );
            return result;
        }

        number_pair_type make_pair( std::string const &l,
                                    std::string const &r ) const
        {
            ++calls_count_;
            number_pair_type result;
            result.mutable_first( )->set_name( l );
            result.mutable_second( )->set_name( r );
            return result;
        }

        number_pair_type make_pair( double l, std::string const &r ) const
        {
            ++calls_count_;
            number_pair_type result;
            result.mutable_first( )->set_value( l );
            result.mutable_second( )->set_name( r );
            return result;
        }

        double sum( double l, double r )             const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.sum( NULL, &p, &n, NULL );
            return n.value( );
        }

        double sum( std::string const &l, double r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.sum( NULL, &p, &n, NULL );
            return n.value( );
        }

        double sum( double l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.sum( NULL, &p, &n, NULL );
            return n.value( );
        }

        double sum( std::string const &l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.sum( NULL, &p, &n, NULL );
            return n.value( );
        }


        double mul( double l, double r )             const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.mul( NULL, &p, &n, NULL );
            return n.value( );

        }

        double mul( std::string const &l, double r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.mul( NULL, &p, &n, NULL );
            return n.value( );

        }

        double mul( double l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.mul( NULL, &p, &n, NULL );
            return n.value( );

        }
        double mul( std::string const &l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.mul( NULL, &p, &n, NULL );
            return n.value( );

        }

        double div( double l, double r )             const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.div( NULL, &p, &n, NULL );
            return n.value( );

        }

        double div( std::string const &l, double r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.div( NULL, &p, &n, NULL );
            return n.value( );

        }

        double div( double l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.div( NULL, &p, &n, NULL );
            return n.value( );

        }

        double div( std::string const &l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.div( NULL, &p, &n, NULL );
            return n.value( );

        }

        double pow( double l, double r )             const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.pow( NULL, &p, &n, NULL );
            return n.value( );

        }

        double pow( std::string const &l, double r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.pow( NULL, &p, &n, NULL );
            return n.value( );

        }

        double pow( double l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.pow( NULL, &p, &n, NULL );
            return n.value( );
        }

        double pow( std::string const &l, std::string const &r ) const
        {
            number_pair_type p = make_pair( l, r );
            number_type      n;
            stub_.pow( NULL, &p, &n, NULL );
            return n.value( );
        }

        unsigned calls_count( ) const
        {
            return calls_count_;
        }

    };
}

namespace interfaces {

    calculator *create_calculator( vtrc::shared_ptr<vtrc::client::vtrc_client> client )
    {
        return new calculator_impl(client);
    }

}
