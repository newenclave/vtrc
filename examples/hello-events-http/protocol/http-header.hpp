#ifndef HTTP_HEADER_HPP
#define HTTP_HEADER_HPP

#include <map>
#include <string>
#include <deque>
#include <sstream>

#include "boost/lexical_cast.hpp"
#include "vtrc-common/protocol/vtrc-rpc-lowlevel.pb.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace http {

    typedef std::pair<std::string, std::string> string_pair;

    static void replace_path( std::string &path, char f, char t )
    {
        typedef std::string::iterator itr;
        if( *path.rbegin( ) == f ) {
            path.resize( path.size( ) - 1 );
        }
        for( itr b(path.begin( )), e(path.end( )); b!=e; ++b ) {
            if( *b == f ) {
                *b = t;
            }
        }
    }

    static string_pair path_to_service( std::string &path )
    {
        typedef std::string::iterator itr;
        itr iter;
        for( itr b(path.begin( )), e(path.end( )); b!=e; ++b ) {
            if( *b == '.' ) {
                iter = b;
            }
        }
        return std::make_pair( std::string( path.begin( ) + 1, iter ),
                               std::string( iter + 1, path.end( ) ) );
    }

    class header_parser
    {
        std::map<std::string, std::string> cont_;
        std::deque<std::string>            method_;
        std::string                        current_;
        bool                               header_ready_;
        bool                               ready_;

        static bool is_space( char p )
        {
            return (p == ' ') || (p == '\t');
        }

        std::pair<std::string, std::string> split_header( const std::string &v )
        {
            std::string::size_type p = v.find( ':' );
            if( p != std::string::npos ) {
                std::string name( v.begin( ), v.begin( ) + p++ );
                while( is_space( v[p] ) ) ++p;
                std::string value( v.begin( ) + p, v.end( ) );
                return std::make_pair( name, value );
            } else {
                return std::make_pair( v, std::string( ) );
            }
        }

        void set_method( const std::string &v )
        {
            typedef std::string::const_iterator itr;
            itr last = v.begin( );
            for( itr b(v.begin( )), e(v.end( )); b!=e; ++b ) {
                if( is_space( *b ) ) {
                    method_.push_back( std::string(last, b) );
                    last = b + 1;
                }
            }
            if( last != v.end( ) ) {
                method_.push_back( std::string(last, v.end( )) );
            }
        }

        template <typename Iter>
        Iter push_header( Iter b, Iter e )
        {
            for( ; b!=e && !header_ready_; ++b ) {
                switch (*b) {
                case '\r':
                    break;
                case '\n':
                    if( current_.empty( ) ) {
                        header_ready_ = true;
                    } else if( method_.empty( ) ) {
                        set_method( current_ );
                    } else {
                        cont_.insert( split_header( current_ ) );
                    }
                    current_.clear( );
                    break;
                default:
                    current_.push_back( *b );
                    break;
                }
            }
            return b;
        }

        template <typename Iter>
        Iter push_body( Iter b, Iter e )
        {
            size_t len = content_length( );
            if( len == 0 ) {
                ready_ = true;
                return b;
            }
            size_t cur_len = current_.size( );
            size_t pos = std::min( len - cur_len, size_t(e - b) );
            current_.append( b, b + pos );
            ready_ = (current_.size( ) == len);
            return b + pos;
        }

    public:

        header_parser( )
            :header_ready_(false)
            ,ready_(false)
        { }

        template <typename Cont>
        size_t push( const Cont &c )
        {
            return push( c.begin( ), c.end( ) );
        }

        template <typename Iter>
        size_t push( Iter b, Iter e )
        {
            Iter bb = b;

            if( !header_ready_ ) {
                b = push_header( b, e );
            }
            if( header_ready_ && !ready_ ) {
                b = push_body( b, e );
            }

            return (b - bb);
        }

        std::map<std::string, std::string> &container( )
        {
            return cont_;
        }

        const std::deque<std::string> &method( ) const
        {
            return method_;
        }

        const std::string &content( ) const
        {
            return current_;
        }

        bool ready( ) const
        {
            return ready_;
        }

        bool header_ready( ) const
        {
            return header_ready_;
        }

        bool has_content_length( ) const
        {
            return has_field( "Content-Length" );
        }

        bool has_field( const std::string &fld ) const
        {
            return cont_.find( fld ) != cont_.end( );
        }

        template <typename T>
        T get( const std::string &fld, const T &def = T( ) ) const
        {
            typedef std::map<std::string, std::string>::const_iterator itr;

            itr f = cont_.find( fld );
            if( f != cont_.end( ) ) {
                return boost::lexical_cast<T>(f->second);
            }
            return def;
        }

        const std::string &field( const std::string &fld ) const
        {
            typedef std::map<std::string, std::string>::const_iterator itr;
            static const std::string empty;

            itr f = cont_.find( fld );
            if( f != cont_.end( ) ) {
                return f->second;
            }
            return empty;
        }

        size_t content_length( ) const
        {
            typedef std::map<std::string, std::string>::const_iterator itr;
            itr f = cont_.find( "Content-Length" );
            if( f != cont_.end( ) ) {
                return boost::lexical_cast<size_t>( f->second.c_str( ) );
            }
            return 0;
        }

        void clear( )
        {
            cont_.clear( );
            method_.clear( );
            current_.clear( );
            header_ready_ = ready_ = false;
        }

    };

    static
    std::string bin2hex( const std::string &str )
    {

        typedef std::string::const_iterator itr;
        static unsigned char _hexes[] = {  '0', '1', '2', '3', '4',
                                           '5', '6', '7', '8', '9',
                                           'A', 'B', 'C', 'D', 'E', 'F' };
        struct hi_lo_t {
            unsigned char h :4;
            unsigned char l :4;
            char line[3];
            hi_lo_t(){ line[2] = 0; }
            const char * operator( )(const char _in) {
                h = (_in >> 4);
                l = (_in & 0xF);
                line[0] = _hexes[h];
                line[1] = _hexes[l];
                return line;
            }
        } int_to_hex;

        std::string tmp;

        tmp.reserve( str.size( ) * 2 + 1);

        for( itr b(str.begin( )), e(str.end( )); b!=e; ++b ) {
            tmp.append( int_to_hex( *b ) );
        }

        return tmp;
    }

    static
    std::string hex2bin( const std::string &str )
    {
        struct {
            unsigned char operator[](const char _in) {
                switch(_in) {
                case '0':   case '1':
                case '2':   case '3':
                case '4':   case '5':
                case '6':   case '7':
                case '8':   case '9':
                    return _in - '0';
                case 'a':   case 'b':
                case 'c':   case 'd':
                case 'e':   case 'f':
                    return _in - 'a' + 10;
                case 'A':   case 'B':
                case 'C':   case 'D':
                case 'E':   case 'F':
                    return _in - 'A' + 10;
                default:
                    return 0xff; // fail
                }
            }
        } static hex_to_int;

        if( str.size( ) % 2 ) { // invalid size -> invalid string
            return std::string( );
        }

        std::string tmp;
        tmp.reserve( str.size( ) >> 1 );

        for(size_t i=0; i<str.size( ); i+=2) {

            unsigned char next;
            unsigned char hight = hex_to_int[str[i]];
            unsigned char low   = hex_to_int[str[i+1]];

            if( ( hight | low ) == 0xff ) { // invalid character
                return std::string( );
            }
            next = ( hight << 4) | low;
            tmp.push_back( next);
        }
        return tmp;
    }

    enum request_option {
        REQ_WAIT        = 1,
        REQ_ACCEPT_RES  = 1 << 1,
        REQ_ACCEPT_CBS  = 1 << 2,
    };

    unsigned opt2flags( const vtrc::rpc::unit_options &opts )
    {
        unsigned res = 0;

        res |= ( opts.wait( )             ? REQ_WAIT       : 0);
        res |= ( opts.accept_callbacks( ) ? REQ_ACCEPT_CBS : 0);
        res |= ( opts.accept_response( )  ? REQ_ACCEPT_RES : 0);

        return res;
    }

    void flags2opt( unsigned flags, vtrc::rpc::unit_options *opts )
    {
        opts->set_wait            ( flags & REQ_WAIT       );
        opts->set_accept_response ( flags & REQ_ACCEPT_RES );
        opts->set_accept_callbacks( flags & REQ_ACCEPT_CBS );
    }

    static
    bool http2lowlevel( header_parser &pars, vtrc::rpc::lowlevel_unit &llu )
    {

        bool is_request = (pars.method( ).begin( )->compare( "GET" ) == 0);

        llu.mutable_info( )
                ->set_message_type( pars.get<unsigned>("X-VTRC-Type", 1) );

        llu.set_id( pars.get<unsigned>("X-VTRC-Id") );
        llu.set_target_id( pars.get<unsigned>("X-VTRC-Tid") );

        flags2opt( pars.get<unsigned>("X-VTRC-Options", 3),
                   llu.mutable_opt( ) );

        if( pars.has_field( "X-VTRC-Error" ) ) {
            llu.mutable_error( )
                    ->set_code( pars.get<unsigned>("X-VTRC-Error") );
            llu.mutable_error( )
                    ->set_category( pars.get<unsigned>("X-VTRC-Category"));
            llu.mutable_error( )
                    ->set_additional( pars.field( "X-VTRC-Message" ) );
        } else {
            if( is_request ) {
                if( pars.method( ).size( ) > 1 ) {
                    std::string path = *(++pars.method( ).begin( ));
                    replace_path( path, '/', '.' );
                    string_pair sp = path_to_service( path );
                    llu.mutable_call( )->set_service_id( sp.first );
                    llu.mutable_call( )->set_method_id( sp.second );
                }
                llu.set_request( hex2bin( pars.content( ) ) );
            } else {
                llu.set_response( hex2bin( pars.content( ) ) );
            }
        }
        return true;
    }

    static std::string lowlevel2http( const vtrc::rpc::lowlevel_unit &llu )
    {
        using namespace vtrc::common;
        using namespace vtrc::rpc;
        std::ostringstream oss;

        bool is_request = llu.has_call( );

        if( is_request && !llu.has_error( ) ) {
            std::string ser("/");
            ser.append(llu.call( ).service_id( ));

            replace_path( ser, '.', '/' );

            ser.append("/");
            ser.append(llu.call( ).method_id( ));
            oss << "GET " << ser << " HTTP/1.1\r\n";
        } else {
            oss << "HTTP/1.1 200 OK\r\n"; //
        }

        oss << "Connection: keep-alive\r\n";
        oss << "X-VTRC-Type: "     << llu.info( ).message_type( ) << "\r\n";
        oss << "X-VTRC-Options: "  << opt2flags( llu.opt( ) ) << "\r\n";

        if( llu.has_error( ) ) {
            oss << "X-VTRC-Error: "    << llu.error( ).code( )       << "\r\n";
            oss << "X-VTRC-Category: " << llu.error( ).category( )   << "\r\n";
            oss << "X-VTRC-Message: "  << llu.error( ).additional( ) << "\r\n";
            oss << "Content-Length: "  << 0                          << "\r\n";
        } else {

            std::string content(bin2hex( is_request ? llu.request( )
                                                    : llu.response( ) ) );

            oss << "X-VTRC-Id: "      << llu.id( )                  << "\r\n";
            oss << "X-VTRC-Tid: "     << llu.target_id( )           << "\r\n";
            oss << "Content-Length: " << content.size( )            << "\r\n";
            oss << "\r\n" << content;
        }

//        std::cout << (is_request ? "REQ << \n" : "RES >> \n" )
//                  << oss.str( ) << "\n-----\n";

        return oss.str( );
    }
}

#endif // HTTP_HEADER_HPP

