#include "rfs-iface.h"
#include "vtrc/client/rpc-channel-c.h"
#include "vtrc/client/client.h"

#include "vtrc/common/stub-wrapper.h"

#include "protocol/remotefs.pb.h"

namespace {

    namespace gpb = google::protobuf;
    typedef vtrc_example::remote_fs_Stub stub_type;
    typedef vtrc::common::stub_wrapper<stub_type> stub_wrapper_type;

    const size_t max_block_length = 65000 ;

    interfaces::fs_info fill_info( vtrc_example::fs_element_info const &info,
                                   std::string const &path)
    {
        interfaces::fs_info res;
        res.path_           = path;
        res.is_exist_       = info.is_exist( );
        res.is_directory_   = info.is_directory( );
        res.is_empty_       = info.is_empty( );
        res.is_regular_     = info.is_regular( );
        res.is_symlink_     = info.is_symlink( );
        return res;
    }

    interfaces::fs_info fill_info( vtrc_example::fs_element_info const &info )
    {
        return fill_info( info, "" );
    }

    struct remote_fs_iterator_impl: public interfaces::remote_fs_iterator {

        vtrc::shared_ptr<gpb::RpcChannel>              channel_;
        mutable stub_wrapper_type                      stub_;
        vtrc_example::fs_iterator_info                 iter_;
        interfaces::fs_info                            info_;
        bool                                           has_info_;

        remote_fs_iterator_impl( vtrc::shared_ptr<gpb::RpcChannel> channel,
                                 const vtrc_example::fs_iterator_info &iter)
            :channel_(channel)
            ,stub_(channel)
            ,iter_(iter)
            ,has_info_(false)
        {

        }

        ~remote_fs_iterator_impl( ) try
        {
            vtrc_example::fs_handle h;
            h.set_value( iter_.handle( ).value( ) );
            stub_.call_request( &stub_type::close, &h );

        } catch( ... ) {
            ;;;
        }

        const interfaces::fs_info &info( )
        {
            if( !has_info_ ) {
                vtrc_example::fs_iterator_info ii;
                vtrc_example::fs_element_info  ei;
                ii.mutable_handle( )->CopyFrom( iter_.handle( ) );
                stub_.call( &stub_type::iter_info, &ii, &ei );
                info_ = fill_info( ei, iter_.path( ));
            }
            return info_;
        }

        void next( )
        {
            vtrc_example::fs_iterator_info ii;
            vtrc_example::fs_iterator_info ri;
            ii.mutable_handle( )->CopyFrom( iter_.handle( ) );
            stub_.call( &stub_type::iter_next, &ii, &ri );
            iter_.CopyFrom( ri );
        }

        bool end( ) const
        {
            return iter_.end( );
        }

        vtrc::shared_ptr<interfaces::remote_fs_iterator> clone( ) const
        {
            vtrc_example::fs_iterator_info ii;
            vtrc_example::fs_iterator_info ri;
            ii.mutable_handle( )->CopyFrom( iter_.handle( ) );
            stub_.call( &stub_type::iter_clone, &ii, &ri );
            return vtrc::make_shared<remote_fs_iterator_impl>( channel_, ri );
        }
    };

    struct remote_file_impl: public interfaces::remote_file {

        typedef vtrc_example::remote_file_Stub         stub_type;
        typedef vtrc::common::stub_wrapper<stub_type>  stub_wrapper_type;

        std::string                                    path_;
        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_wrapper_type                      stub_;
        vtrc_example::fs_handle                        fhdl_;

        void open( std::string const &mode )
        {
            vtrc_example::file_open_req req;
            req.set_path( path_ );
            req.set_mode( mode );
            stub_.call( &stub_type::open, &req, &fhdl_ );
        }

        remote_file_impl( const std::string &p, const std::string &mode,
                          vtrc::shared_ptr<vtrc::client::vtrc_client> client)
            :path_(p)
            ,channel_(client->create_channel( ))
            ,stub_(channel_.get( ))
        {
            open( mode );
        }

        void close_impl( ) const
        {
            try {
                stub_.call_request( &stub_type::close, &fhdl_ );
            } catch( ... ) {
                ;;;
            }
        }

        ~remote_file_impl( )
        {
            close_impl( );
        }

        const std::string &path( ) const
        {
            return path_;
        }

        void close( ) const
        {
            close_impl( );
        }

        gpb::uint64 tell( ) const
        {
            vtrc_example::file_position pos;
            stub_.call( &stub_type::tell, &fhdl_, &pos );
            return pos.position( );
        }

        gpb::uint64 seek( google::protobuf::uint64 position,
                          unsigned whence ) const
        {
            vtrc_example::file_set_position set_pos;
            vtrc_example::file_position pos;

            set_pos.mutable_hdl( )->CopyFrom( fhdl_ );
            set_pos.set_position( position );
            set_pos.set_whence( whence );
            stub_.call( &stub_type::seek, &set_pos, &pos );
            return pos.position( );
        }

        gpb::uint64 seek_begin( google::protobuf::uint64 pos ) const
        {
            return seek( pos, vtrc_example::POS_SEEK_SET );
        }

        gpb::uint64 seek_end( google::protobuf::uint64 pos )   const
        {
            return seek( pos, vtrc_example::POS_SEEK_END );
        }

        size_t read( std::string &data, size_t max_len )  const
        {
            vtrc_example::file_data_block block;

            block.mutable_hdl( )->CopyFrom( fhdl_ );
            block.set_length( max_len );

            stub_.call( &stub_type::read, &block, &block );

            data.swap( *(block.mutable_data( )) );
            return data.size( );
        }

        size_t write( std::string &data ) const
        {
            return write( data.c_str( ), data.size( ) );
        }

        size_t write( const char *data, size_t lenght ) const
        {
            vtrc_example::file_data_block block;

            size_t len = (lenght > max_block_length)
                    ? max_block_length
                    : lenght;

            block.mutable_hdl( )->CopyFrom( fhdl_ );
            block.set_data( data, len );

            stub_.call( &stub_type::write, &block, &block );
            return block.length( );
        }

        void flush( ) const
        {
            stub_.call_request( &stub_type::flush, &fhdl_ );
        }

    };

    struct remote_fs_impl: public interfaces::remote_fs {

        typedef remote_fs_impl this_type;

        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_wrapper_type                      stub_;
        gpb::uint32                                    fs_handle_;

        ~remote_fs_impl( ) try
        {
            if( fs_handle_ ) {
                vtrc_example::fs_handle h;
                h.set_value( fs_handle_ );
                stub_.call_request( &stub_type::close, &h );
            }

        } catch ( ... ) {
            ;;;
        }

        remote_fs_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                        std::string const &path)
            :channel_(client->create_channel( ))
            ,stub_(channel_.get( ))
            ,fs_handle_(0)
        {
            vtrc_example::fs_handle_path hp;
            hp.set_path( path );
            stub_.call( &stub_type::open, &hp, &hp );
            fs_handle_ = hp.handle( ).value( );
        }

        void cd( const std::string &new_path ) const
        {
            vtrc_example::fs_handle_path hp;
            hp.set_path( new_path );
            hp.mutable_handle( )->set_value( fs_handle_ );
            stub_.call_request( &stub_type::cd, &hp );
        }

        std::string pwd( ) const
        {
            vtrc_example::fs_handle_path hp;
            hp.mutable_handle( )->set_value( fs_handle_ );
            stub_.call( &stub_type::pwd, &hp, &hp );
            return hp.path( );
        }

        bool exists( std::string const &path ) const
        {
            vtrc_example::fs_handle_path hp;
            vtrc_example::fs_element_info info;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.call( &stub_type::exists, &hp, &info );
            return info.is_exist( );
        }

        bool exists( ) const
        {
            return exists( "" );
        }

        interfaces::fs_info info( std::string const &path ) const
        {
            vtrc_example::fs_handle_path hp;
            vtrc_example::fs_element_info info;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.call( &stub_type::info, &hp, &info );
            return fill_info( info );
        }

        interfaces::fs_info info( ) const
        {
            return info( "" );
        }

        gpb::uint64 file_size( std::string const &path ) const
        {
            vtrc_example::fs_handle_path hp;
            vtrc_example::file_position  pos;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.call( &stub_type::file_size, &hp, &pos );
            return pos.position( );
        }

        gpb::uint64 file_size( ) const
        {
            return file_size( "" );
        }

        void mkdir( std::string const &path ) const
        {
            vtrc_example::fs_handle_path hp;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.call_request( &stub_type::mkdir, &hp );
        }

        void mkdir( ) const
        {
            mkdir( "" );
        }

        void del( std::string const &path ) const
        {
            vtrc_example::fs_handle_path hp;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.call_request( &stub_type::del, &hp );
        }

        void del( ) const
        {
            del( "" );
        }

        vtrc::shared_ptr<interfaces::remote_fs_iterator>
                                  begin_iterator(const std::string &path) const
        {
            vtrc_example::fs_iterator_info ri;
            vtrc_example::fs_handle_path   hp;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.call( &stub_type::iter_begin, &hp, &ri );
            return vtrc::make_shared<remote_fs_iterator_impl>( channel_, ri );
        }

        vtrc::shared_ptr<interfaces::remote_fs_iterator> begin_iterator( ) const
        {
            return begin_iterator( "" );
        }

        unsigned get_handle( ) const
        {
            return fs_handle_;
        }
    };

}

namespace interfaces {

    remote_fs *create_remote_fs(
                vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                std::string const &path )
    {
        return new remote_fs_impl( client, path );
    }

    remote_file *create_remote_file(
            vtrc::shared_ptr<vtrc::client::vtrc_client> client,
            std::string const &path , const std::string &mode)
    {
        return new remote_file_impl( path, mode, client );
    }

}

