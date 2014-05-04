#include "remote-fs-iface.h"
#include "vtrc-client-base/vtrc-rpc-channel-c.h"
#include "vtrc-client-base/vtrc-client.h"
#include "protocol/remotefs.pb.h"

namespace {

    namespace gpb = google::protobuf;
    typedef vtrc_example::remote_fs_Stub stub_type;

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

        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_type                              stub_;
        vtrc_example::fs_iterator_info                 iter_;
        interfaces::fs_info                            info_;
        bool                                           has_info_;

        remote_fs_iterator_impl( vtrc::shared_ptr<gpb::RpcChannel> channel,
                                 const vtrc_example::fs_iterator_info &iter)
            :channel_(channel)
            ,stub_(channel_.get( ))
            ,iter_(iter)
            ,has_info_(false)
        {

        }

        ~remote_fs_iterator_impl( ) try
        {
            vtrc_example::fs_handle h;
            h.set_value( iter_.handle( ).value( ) );
            stub_.close( NULL, &h, &h, NULL );

        } catch( ... ) {
            ;;;
        }

        const interfaces::fs_info &info( )
        {
            if( !has_info_ ) {
                vtrc_example::fs_iterator_info ii;
                vtrc_example::fs_element_info  ei;
                ii.mutable_handle( )->CopyFrom( iter_.handle( ) );
                stub_.iter_info( NULL, &ii, &ei, NULL );
                info_ = fill_info( ei, iter_.path( ));
            }
            return info_;
        }

        void next( )
        {
            vtrc_example::fs_iterator_info ii;
            vtrc_example::fs_iterator_info ri;
            ii.mutable_handle( )->CopyFrom( iter_.handle( ) );
            stub_.iter_next( NULL, &ii, &ri, NULL );
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
            stub_.iter_clone( NULL, &ii, &ri, NULL );
            return vtrc::make_shared<remote_fs_iterator_impl>( channel_, ri );
        }
    };

    struct remote_file_impl: public interfaces::remote_file {
        typedef vtrc_example::remote_file_Stub file_stub_type;

        std::string                                    path_;
        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable file_stub_type                         stub_;
        vtrc_example::fs_handle                        fhdl_;

        remote_file_impl( const std::string &p,
                          vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                          const vtrc_example::fs_handle &fhdl)
            :path_(p)
            ,channel_(client->create_channel( ))
            ,stub_(channel_.get( ))
            ,fhdl_(fhdl)
        { }

        void close_impl( ) const
        {
            try {
                vtrc_example::fs_handle f;
                stub_.close( NULL, &fhdl_, &f, NULL );
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

        google::protobuf::uint64 tell( ) const
        {
            vtrc_example::file_position pos;
            stub_.tell( NULL, &fhdl_, &pos, NULL);
            return pos.position( );
        }

        void seek( google::protobuf::uint64 position, unsigned whence ) const
        {
            vtrc_example::file_set_position set_pos;
            vtrc_example::file_position pos;

            set_pos.mutable_hdl( )->CopyFrom( fhdl_ );
            set_pos.set_position( position );
            set_pos.set_whence( whence );
            stub_.seek( NULL, &set_pos, &pos, NULL);
        }

        void seek_begin( google::protobuf::uint64 pos ) const
        {
            seek( pos, vtrc_example::POS_SEEK_SET );
        }

        void seek_end( google::protobuf::uint64 pos )   const
        {
            seek( pos, vtrc_example::POS_SEEK_END );
        }

        size_t read( std::string &data, size_t max_len )  const
        {
            vtrc_example::file_data_block block;

            block.mutable_hdl( )->CopyFrom( fhdl_ );
            block.set_length( max_len );

            stub_.read( NULL, &block, &block, NULL );

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

            size_t len = (lenght > (44 * 1024))
                    ? (44 * 1024)
                    : lenght;

            block.mutable_hdl( )->CopyFrom( fhdl_ );
            block.set_data( data, len );

            stub_.write( NULL, &block, &block, NULL );
            return block.length( );
        }
    };

    struct remote_fs_impl: public interfaces::remote_fs {
        typedef remote_fs_impl this_type;

        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_type                              stub_;
        gpb::uint32                                    fs_handle_;

        ~remote_fs_impl( ) try
        {
            if( fs_handle_ ) {
                vtrc_example::fs_handle h;
                h.set_value( fs_handle_ );
                stub_.close( NULL, &h, &h, NULL );
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
            stub_.open( NULL, &hp, &hp, NULL );
            fs_handle_ = hp.handle( ).value( );
        }

        void cd( const std::string &new_path ) const
        {
            vtrc_example::fs_handle_path hp;
            hp.set_path( new_path );
            hp.mutable_handle( )->set_value( fs_handle_ );
            stub_.cd( NULL, &hp, &hp, NULL );
        }

        std::string pwd( ) const
        {
            vtrc_example::fs_handle_path hp;
            hp.mutable_handle( )->set_value( fs_handle_ );
            stub_.pwd( NULL, &hp, &hp, NULL );
            return hp.path( );
        }

        bool exists( std::string const &path ) const
        {
            vtrc_example::fs_handle_path hp;
            vtrc_example::fs_element_info info;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.exists( NULL, &hp, &info, NULL );
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
            stub_.info( NULL, &hp, &info, NULL );
            return fill_info( info );
        }

        interfaces::fs_info info( ) const
        {
            return info( "" );
        }

        vtrc::shared_ptr<interfaces::remote_fs_iterator>
                                  begin_iterator(const std::string &path) const
        {
            vtrc_example::fs_iterator_info ri;
            vtrc_example::fs_handle_path   hp;
            hp.mutable_handle( )->set_value( fs_handle_ );
            hp.set_path( path );
            stub_.iter_begin( NULL, &hp, &ri, NULL );
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
            std::string const &path )
    {

    }

}

