#ifndef MESSAGE_HELPER_H
#define MESSAGE_HELPER_H

#include <vector>
#include <string>
#include <map>
#include <deque>

namespace google { namespace protobuf {
    class Message;
    class FieldDescriptor;
    class Reflection;
    class Descriptor;
}}

namespace vtrc { namespace helper {

    typedef std::deque <
            const google::protobuf::FieldDescriptor *
    >  field_stack_type;

    typedef std::map<std::string, field_stack_type> field_map_type;

    struct message_reader_impl;
    class message_reader  {

        message_reader_impl *impl_;

        message_reader & operator = ( const message_reader &other );
        message_reader( const message_reader &other );

    public:
        message_reader( const google::protobuf::Message *mess );
        virtual ~message_reader( );
    public:

        const google::protobuf::Reflection *reflection( ) const;
        const google::protobuf::Descriptor *descriptor( ) const;

        const google::protobuf::FieldDescriptor * field( int index ) const;
        int field_count( ) const;

        void enum_set_fields( std::vector <
                                    const google::protobuf::FieldDescriptor *
                              > &result ) const;
        void enum_all_fields(std::vector <
                                const google::protobuf::FieldDescriptor *
                             > &result) const;

        const google::protobuf::Message *message( ) const;

        std::string get_as_string(
                    const google::protobuf::FieldDescriptor *fld ) const;
        std::string get_as_string( const std::string &field_name ) const;

        const google::protobuf::Message *get_as_message(
                          const google::protobuf::FieldDescriptor *fld ) const;

        const google::protobuf::Message *get_as_message(
                          const std::string &field_name ) const;

    };

    struct message_helper_impl;

    class message_helper: public  message_reader {

        message_helper_impl *impl_;
        message_helper & operator = ( const message_helper &other );
        message_helper( const message_helper &other );

    public:

        message_helper(google::protobuf::Message *mess );
        virtual ~message_helper( );

        void set( const google::protobuf::FieldDescriptor *fld,
                                                 std::string const &value );
        void set( const std::string &field_name, std::string const &value );
    };

    void make_fields_map( const google::protobuf::Descriptor *src,
                          field_map_type &result,
                          bool use_full_names,
                          bool leafs_only );

}}

#endif
