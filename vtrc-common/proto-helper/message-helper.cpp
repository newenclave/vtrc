#include <string>
#include <map>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "boost/lexical_cast.hpp"

#include "message-helper.h"

namespace vtrc { namespace helper {

    namespace gpb = google::protobuf;
    namespace {
        typedef gpb::FieldDescriptor field_desc_type;

        template<google::protobuf::FieldDescriptor::CppType>
        struct types_info;

#define VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(ValueName,RetType,SetType,TypeName) \
    template<>                                                                 \
    struct types_info<ValueName>                                               \
    {                                                                          \
        typedef RetType data_type;                                             \
        typedef std::vector<data_type> vector_type;                            \
        typedef RetType (google::protobuf::Reflection::* repeat_getter_type )  \
                        (const google::protobuf::Message &,                    \
                         const google::protobuf::FieldDescriptor *,int) const; \
        typedef RetType (google::protobuf::Reflection::* getter_type )         \
                        (const google::protobuf::Message &,                    \
                         const google::protobuf::FieldDescriptor *) const;     \
        typedef void    (google::protobuf::Reflection::* repeat_setter_type )  \
                        (google::protobuf::Message *,                          \
                         const google::protobuf::FieldDescriptor *,            \
                         int, SetType ) const;                                 \
        typedef void    (google::protobuf::Reflection::* setter_type )         \
                        (google::protobuf::Message *,                          \
                         const google::protobuf::FieldDescriptor *,            \
                         SetType ) const;                                      \
        typedef void    (google::protobuf::Reflection::* adder_type )          \
                        (google::protobuf::Message *,                          \
                         const google::protobuf::FieldDescriptor *,            \
                         SetType) const;                                       \
        static const repeat_getter_type get_repeated() {                       \
            return &::google::protobuf::Reflection::GetRepeated##TypeName; }   \
        static const getter_type        get() {                                \
            return &::google::protobuf::Reflection::Get##TypeName; }           \
        static const repeat_setter_type set_repeated() {                       \
            return &::google::protobuf::Reflection::SetRepeated##TypeName;}    \
        static const setter_type        set() {                                \
            return &::google::protobuf::Reflection::Set##TypeName; }           \
        static const adder_type         add() {                                \
            return &::google::protobuf::Reflection::Add##TypeName; }           \
    }

#define VTRC_STATIC_DEFINE_NAME_TYPE_CALLS_FOR_INTS( T )                       \
        VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(                                    \
            google::protobuf::FieldDescriptor::CPPTYPE_INT##T,                 \
            google::protobuf::int##T, google::protobuf::int##T, Int##T);       \
        VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(                                    \
            google::protobuf::FieldDescriptor::CPPTYPE_UINT##T,                \
            google::protobuf::uint##T, google::protobuf::uint##T, UInt##T);

VTRC_STATIC_DEFINE_NAME_TYPE_CALLS_FOR_INTS( 32 )
VTRC_STATIC_DEFINE_NAME_TYPE_CALLS_FOR_INTS( 64 )

#undef VTRC_STATIC_DEFINE_NAME_TYPE_CALLS_FOR_INTS

VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(
    google::protobuf::FieldDescriptor::CPPTYPE_FLOAT,
    float, float, Float);

VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(
    google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE,
    double, double, Double);

VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(
    google::protobuf::FieldDescriptor::CPPTYPE_STRING,
    std::string, const std::string &, String);

VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(
    google::protobuf::FieldDescriptor::CPPTYPE_BOOL,
    bool, bool, Bool);

VTRC_STATIC_DEFINE_NAME_TYPE_CALLS(
    google::protobuf::FieldDescriptor::CPPTYPE_ENUM,
    const google::protobuf::EnumValueDescriptor*,
    const google::protobuf::EnumValueDescriptor*, Enum);

#undef VTRC_STATIC_DEFINE_NAME_TYPE_CALLS

    }

    /// **** READER **** ///

    struct message_reader_impl {

        typedef gpb::FieldDescriptor field_desc_type;

        const gpb::Message *mess_;
        std::map<std::string,
                 const gpb::FieldDescriptor *> fields_map_;
        std::vector<std::string> name_seq_;

        const gpb::Reflection *reflection_;
        const gpb::Descriptor *descriptor_;

        typedef types_info<field_desc_type::CPPTYPE_INT32 > type_info_int32;
        typedef types_info<field_desc_type::CPPTYPE_UINT32> type_info_uint32;
        typedef types_info<field_desc_type::CPPTYPE_INT64 > type_info_int64;
        typedef types_info<field_desc_type::CPPTYPE_UINT64> type_info_uint64;

        typedef types_info<field_desc_type::CPPTYPE_BOOL  > type_info_bool;
        typedef types_info<field_desc_type::CPPTYPE_FLOAT > type_info_float;
        typedef types_info<field_desc_type::CPPTYPE_DOUBLE> type_info_double;

        typedef types_info<field_desc_type::CPPTYPE_STRING> type_info_string;
        typedef types_info<field_desc_type::CPPTYPE_ENUM  > type_info_enum;

    public:

        message_reader_impl( const gpb::Message *mess )
            :mess_(mess)
            ,reflection_(mess->GetReflection( ))
            ,descriptor_(mess->GetDescriptor( ))
        {
            name_seq_.reserve(field_count( ));
            for( int i=0, e=field_count( ); i<e; ++i ) {
                fields_map_.insert(std::make_pair(field(i)->name( ), field(i)));
                name_seq_.push_back( field(i)->name( ) );
            }
        }

        virtual ~message_reader_impl( ) /* throw( ) */ { }

        const gpb::Reflection *reflection( ) const
        {
            return reflection_;
        }

        const gpb::Descriptor *descriptor( ) const
        {
            return descriptor_;
        }

        const field_desc_type * field( int index ) const
        {
            return descriptor( )->field( index );
        }

        int field_count( ) const
        {
            return descriptor( )->field_count( );
        }

        void enum_set_fields( std::vector<
                                const field_desc_type *
                                > &result ) const
        {
            reflection( )->ListFields( *mess_, &result );
        }

        void enum_all_fields(std::vector<const field_desc_type *> &result) const
        {
            std::vector<const field_desc_type *> flds;
            int count = field_count( );
            flds.reserve( count );
            for( int i=0; i<count; ++i ) {
                flds.push_back( field( i ) );
            }
            result.swap( flds );
        }

        std::vector<std::string> const &names( ) const
        {
            return name_seq_;
        }

        const field_desc_type *get_field( int index ) const
        {
            if( index >= field_count( ) || index < 0 )
                throw std::runtime_error( "Bad field index" );
            return field( index );
        }

        const field_desc_type *get_field( const std::string &name ) const
        {
            std::map<std::string,
                     const field_desc_type *
                    >::const_iterator f(fields_map_.find( name ));
            if( f != fields_map_.end( ) ) return f->second;
            throw std::runtime_error( "Bad field name" );
        }

        const field_desc_type *field_number( int num ) const
        {
            return descriptor( )->FindFieldByNumber( num );
        }

        const field_desc_type *find_field( const std::string &name ) const
        {
            const field_desc_type *fld(descriptor()->FindFieldByName(name));
            if( fld ) return fld;
            fld = descriptor()->FindFieldByLowercaseName( name );
            if( fld ) return fld;
            fld = descriptor()->FindFieldByCamelcaseName( name );
            if( fld ) return fld;
            return NULL;
        }

        const field_desc_type *field_by_name( const std::string &name ) const
        {
            std::map<std::string,
                     const field_desc_type *
                    >::const_iterator f(fields_map_.find( name ));
            if( f != fields_map_.end( ) ) return f->second;
            return NULL;
        }

        bool is_set( const field_desc_type *fld ) const
        {
            return reflection( )->HasField( *mess_, fld );
        }


        const gpb::Message *message( ) const
        {
            return mess_;
        }

#define VTRC_REFLECT_HELPER_DEFINE_GETTER( TypeName )                          \
        type_info##TypeName::data_type                                         \
            get##TypeName(const field_desc_type *fld ) const                   \
        {                                                                      \
            return (reflection( )->*                                           \
                    ( type_info##TypeName::get( )))(*mess_, fld);              \
        }

#define VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( TypeName )                      \
        type_info##TypeName::data_type                                         \
            get_repeated##TypeName(const field_desc_type *fld,                 \
                                   int fld_num ) const                         \
        {                                                                      \
            return (reflection( )->*                                           \
                   (type_info##TypeName::get_repeated()))                      \
                        (*mess_, fld, fld_num);                                \
        }

#define VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( TypeName )                  \
        void get_repeated_vector##TypeName( const field_desc_type *fld,        \
                    std::vector<type_info##TypeName::data_type> &result ) const\
        {                                                                      \
            int count(get_repeated_size( fld ));                               \
            std::vector<type_info##TypeName::data_type> tmp;                   \
            tmp.reserve( count );                                              \
            for( int i=0; i<count; ++i ) {                                     \
                tmp.push_back( get_repeated##TypeName( fld, i ) );             \
            }                                                                  \
            result.swap( tmp );                                                \
        }

        VTRC_REFLECT_HELPER_DEFINE_GETTER( _int32  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _uint32 );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _int64  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _uint64 );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _bool   );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _float  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _double );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _string );
        VTRC_REFLECT_HELPER_DEFINE_GETTER( _enum   );

        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _int32  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _uint32 );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _int64  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _uint64 );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _bool   );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _float  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _double );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _string );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP( _enum   );

        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _int32  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _uint32 );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _int64  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _uint64 );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _bool   );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _float  );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _double );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _string );
        VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC( _enum );

#undef VTRC_REFLECT_HELPER_DEFINE_GETTER_REP_VEC
#undef VTRC_REFLECT_HELPER_DEFINE_GETTER
#undef VTRC_REFLECT_HELPER_DEFINE_GETTER_REP

        int get_repeated_size( const field_desc_type *fld ) const
        {
            return reflection( )->FieldSize( *mess_, fld );
        }

        template <typename T>
        T get( const field_desc_type *fld ) const
        {
            switch(fld->cpp_type( )) {
            case field_desc_type::CPPTYPE_INT32:
                return boost::lexical_cast<T>(get_int32(fld));
            case field_desc_type::CPPTYPE_INT64:
                return boost::lexical_cast<T>(get_int64(fld));
            case field_desc_type::CPPTYPE_UINT32:
                return boost::lexical_cast<T>(get_uint32(fld));
            case field_desc_type::CPPTYPE_UINT64:
                return boost::lexical_cast<T>(get_uint64(fld));
            case field_desc_type::CPPTYPE_FLOAT:
                return boost::lexical_cast<T>(get_float(fld));
            case field_desc_type::CPPTYPE_DOUBLE:
                return boost::lexical_cast<T>(get_double(fld));
            case field_desc_type::CPPTYPE_ENUM:
                return boost::lexical_cast<T>(get_enum(fld)->number());
            case field_desc_type::CPPTYPE_STRING:
                return boost::lexical_cast<T>(get_string(fld));
            case field_desc_type::CPPTYPE_BOOL:
                return boost::lexical_cast<T>(get_bool(fld));
            case field_desc_type::CPPTYPE_MESSAGE:
                throw std::bad_cast( );
            default:
                return T( );
            }
        }

        template <typename T>
        T get_repeated( const field_desc_type *fld, int index ) const
        {
            switch(fld->cpp_type( )) {
            case field_desc_type::CPPTYPE_INT32:
                return boost::lexical_cast<T>(get_repeated_int32(fld, index));
            case field_desc_type::CPPTYPE_INT64:
                return boost::lexical_cast<T>(get_repeated_int64(fld, index));
            case field_desc_type::CPPTYPE_UINT32:
                return boost::lexical_cast<T>(get_repeated_uint32(fld, index));
            case field_desc_type::CPPTYPE_UINT64:
                return boost::lexical_cast<T>(get_repeated_uint64(fld, index));
            case field_desc_type::CPPTYPE_FLOAT:
                return boost::lexical_cast<T>(get_repeated_float(fld, index));
            case field_desc_type::CPPTYPE_DOUBLE:
                return boost::lexical_cast<T>(get_repeated_double(fld, index));
            case field_desc_type::CPPTYPE_ENUM:
                return boost::lexical_cast<T>(
                        get_repeated_enum(fld, index)->number());
            case field_desc_type::CPPTYPE_STRING:
                return boost::lexical_cast<T>(get_repeated_string(fld, index));
            case field_desc_type::CPPTYPE_BOOL:
                return boost::lexical_cast<T>(get_repeated_bool(fld, index));
            case field_desc_type::CPPTYPE_MESSAGE:
                throw std::runtime_error( "Field is Message" );
            default:
                return T( );
            }
        }

private:
        template<typename F, typename T>
        void convert_vector(std::vector<F> const &src,
                            std::vector<T> &res) const
        {
            typedef std::vector<F> fv_type;
            std::vector<T> tmp;
            tmp.reserve( src.size( ) );
            for( typename fv_type::const_iterator b(src.begin()), e(src.end());
                                                                    b!=e; ++b )
            {
                tmp.push_back( boost::lexical_cast<T>(*b) );
            }
            res.swap( tmp );
        }

        template<typename T>
        void convert_vector(std::vector<type_info_enum::data_type> const &src,
                            std::vector<T> &res) const
        {
            typedef std::vector<type_info_enum::data_type> fv_type;
            std::vector<T> tmp;
            tmp.reserve( src.size( ) );
            for( typename fv_type::const_iterator b(src.begin()), e(src.end());
                                                                    b!=e; ++b )
            {
                tmp.push_back( boost::lexical_cast<T>((*b)->number()) );
            }
            res.swap( tmp );
        }

public:
        template <typename T>
        void get_repeated_vector( const field_desc_type *fld,
                            std::vector<T> &result ) const
        {

#           define VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CppType, TypeName ) \
                case field_desc_type::CppType: {                               \
                    typename type_info##TypeName::vector_type vec;             \
                    get_repeated_vector##TypeName( fld, vec );                 \
                    convert_vector( vec, result );                             \
                    break;                                                     \
                }

            switch(fld->cpp_type( )) {
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_INT32,  _int32  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_INT64,  _int64  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_UINT32, _uint64 );
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_UINT64, _uint64 );
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_FLOAT,  _float  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_DOUBLE, _double );
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_STRING, _string );
            VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC( CPPTYPE_ENUM,   _enum   );

            case field_desc_type::CPPTYPE_MESSAGE:
                throw std::bad_cast( );
            default:
                break;
            }
#           undef VTRC_REFLECT_HELPER_DEFINE_CASE_REP_VEC
        }
    };

    const gpb::Reflection *message_reader::reflection( ) const
    {
        return impl_->reflection( );
    }

    const gpb::Descriptor *message_reader::descriptor( ) const
    {
        return impl_->descriptor( );
    }

    const field_desc_type * message_reader::field( int index ) const
    {
        return impl_->field(index);
    }

    int message_reader::field_count( ) const
    {
        return impl_->field_count( );
    }
    void message_reader::enum_set_fields( std::vector<
        const gpb::FieldDescriptor *> &result ) const
    {
        impl_->enum_set_fields( result );
    }

    void message_reader::enum_all_fields(std::vector <
                                                const gpb::FieldDescriptor *
                                         > &result) const
    {
        impl_->enum_all_fields( result );
    }

    const gpb::Message *message_reader::message( ) const
    {
        return impl_->message( );
    }

    std::string message_reader::get_as_string(
                const gpb::FieldDescriptor *fld ) const
    {
        return impl_->get<std::string>( fld );
    }

    std::string message_reader::get_as_string(
                                     const std::string &field_name ) const
    {
        const field_desc_type *fld(impl_->find_field( field_name ));
        if( !fld ) throw std::runtime_error( "Bad field name" );
        return get_as_string( fld );
    }

    const gpb::Message *message_reader::get_as_message(
                                    const gpb::FieldDescriptor *fld ) const
    {
        return &(impl_->reflection( )->GetMessage( *impl_->message( ), fld ));
    }

    const gpb::Message *message_reader::get_as_message(
                                    const std::string &field_name ) const
    {
        const field_desc_type *fld(impl_->find_field( field_name ));
        if( !fld ) throw std::runtime_error( "Bad field name" );
        return get_as_message( fld );
    }

    message_reader::message_reader( const gpb::Message *mess )
        :impl_(new message_reader_impl(mess))
    {}

    message_reader::~message_reader( )
    {
        delete impl_;
    }

    /// *************** ///

    struct message_helper_impl: public message_reader_impl {

        typedef gpb::FieldDescriptor         field_desc_type;
        typedef gpb::EnumValueDescriptor     enum_value_type;
        gpb::Message *mess_;

        typedef types_info<field_desc_type::CPPTYPE_INT32 > type_info_int32;
        typedef types_info<field_desc_type::CPPTYPE_UINT32> type_info_uint32;
        typedef types_info<field_desc_type::CPPTYPE_INT64 > type_info_int64;
        typedef types_info<field_desc_type::CPPTYPE_UINT64> type_info_uint64;

        typedef types_info<field_desc_type::CPPTYPE_BOOL  > type_info_bool;
        typedef types_info<field_desc_type::CPPTYPE_FLOAT > type_info_float;
        typedef types_info<field_desc_type::CPPTYPE_DOUBLE> type_info_double;

        typedef types_info<field_desc_type::CPPTYPE_STRING> type_info_string;
        typedef types_info<field_desc_type::CPPTYPE_ENUM  > type_info_enum;

        message_helper_impl( gpb::Message *mess )
            :message_reader_impl(mess)
            ,mess_(mess)
        {}

        ~message_helper_impl( ) /*throw( )*/ { }

        void clear_field( const field_desc_type *fld )
        {
            reflection()->ClearField( mess_, fld );
        }

#define VTRC_REFLECT_HELPER_DEFINE_SETTER( TypeName )                          \
        void set##TypeName( const field_desc_type *fld,                        \
                            type_info##TypeName::data_type v )                 \
        {                                                                      \
            (reflection( )->*(type_info##TypeName::set( )))( mess_, fld, v );  \
        }

        VTRC_REFLECT_HELPER_DEFINE_SETTER( _int32  )
        VTRC_REFLECT_HELPER_DEFINE_SETTER( _int64  )
        VTRC_REFLECT_HELPER_DEFINE_SETTER( _uint32 )
        VTRC_REFLECT_HELPER_DEFINE_SETTER( _uint64 )
        VTRC_REFLECT_HELPER_DEFINE_SETTER( _float  )
        VTRC_REFLECT_HELPER_DEFINE_SETTER( _double )
        VTRC_REFLECT_HELPER_DEFINE_SETTER( _bool   )
        VTRC_REFLECT_HELPER_DEFINE_SETTER( _enum   )

        void set_string( const field_desc_type *fld, const std::string &v )
        {
            (reflection( )->*(type_info_string::set( )))( mess_, fld, v );
        }

        void set_enum( const field_desc_type *fld, int v )
        {
            const enum_value_type* ed(fld->enum_type( )->FindValueByNumber(v));
            if( ed )
                reflection()->SetEnum(mess_, fld, ed);
            else
                throw std::runtime_error( "Bad enum value number" );
        }

        void set_enum( const field_desc_type *fld, const std::string &v )
        {
            const enum_value_type* ed(fld->enum_type( )->FindValueByName(v));
            if( ed )
                reflection()->SetEnum(mess_, fld, ed);
            else
                throw std::runtime_error( "Bad enum value name" );
        }

#undef VTRC_REFLECT_HELPER_DEFINE_SETTER

        template <typename T>
        void set( const field_desc_type *fld, const T &v )
        {
#           define VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CppType, TypeName )    \
                case field_desc_type::CppType:                                 \
                    set##TypeName( fld,                                        \
                      boost::lexical_cast<type_info##TypeName::data_type>(v)); \
                    break;

            switch( fld->cpp_type( ) ) {
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_INT32,  _int32  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_INT64,  _int64  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_UINT32, _uint32 );
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_UINT64, _uint64 );
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_FLOAT,  _float  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_DOUBLE, _double );
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_STRING, _string );
            VTRC_REFLECT_HELPER_DEFINE_CASE_SET( CPPTYPE_BOOL,   _bool   );
            case field_desc_type::CPPTYPE_ENUM:
                set_enum( fld, boost::lexical_cast<int>(v));
                break;
            default:
                throw std::runtime_error( "Bad field type" );
            }
#            undef VTRC_REFLECT_HELPER_DEFINE_CASE_SET
        }

#define VTRC_REFLECT_HELPER_DEFINE_ADDER( TypeName )                           \
        void add##TypeName( const field_desc_type *fld,                        \
                            type_info##TypeName::data_type v )                 \
        {                                                                      \
            (reflection( )->*(type_info##TypeName::add()))( mess_, fld, v );   \
        }

        VTRC_REFLECT_HELPER_DEFINE_ADDER( _int32  )
        VTRC_REFLECT_HELPER_DEFINE_ADDER( _int64  )
        VTRC_REFLECT_HELPER_DEFINE_ADDER( _uint32 )
        VTRC_REFLECT_HELPER_DEFINE_ADDER( _uint64 )
        VTRC_REFLECT_HELPER_DEFINE_ADDER( _float  )
        VTRC_REFLECT_HELPER_DEFINE_ADDER( _double )
        VTRC_REFLECT_HELPER_DEFINE_ADDER( _bool   )
        VTRC_REFLECT_HELPER_DEFINE_ADDER( _enum   )

        void add_string( const field_desc_type *fld, const std::string &v )
        {
            (reflection( )->*(type_info_string::add()))( mess_, fld, v );
        }

        void add_enum( const field_desc_type *fld, const std::string &v )
        {
            const enum_value_type* ed(fld->enum_type( )->FindValueByName(v));
            if( ed )
                add_enum(fld, ed);
            else
                throw std::runtime_error( "Bad enum value name" );
        }

        void add_enum( const field_desc_type *fld, int v )
        {
            const enum_value_type* ed(fld->enum_type( )->FindValueByNumber(v));
            if( ed )
                add_enum(fld, ed);
            else
                throw std::runtime_error( "Bad enum value number" );
        }

#undef VTRC_REFLECT_HELPER_DEFINE_ADDER

        template <typename T>
        void add( const field_desc_type *fld, const T& v )
        {
#           define VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CppType, TypeName )    \
                case field_desc_type::CppType:                                 \
                    add##TypeName( fld,                                        \
                      boost::lexical_cast<type_info##TypeName::data_type>(v)); \
                    break;

            switch( fld->cpp_type( ) ) {
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_INT32,  _int32  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_INT64,  _int64  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_UINT32, _uint32 );
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_UINT64, _uint64 );
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_FLOAT,  _float  );
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_DOUBLE, _double );
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_STRING, _string );
            VTRC_REFLECT_HELPER_DEFINE_CASE_ADD( CPPTYPE_BOOL,   _bool   );
            case field_desc_type::CPPTYPE_ENUM:
                add_enum( fld, boost::lexical_cast<int>(v));
                break;
            default:
                throw std::runtime_error( "Bad field type" );
            }
#           undef VTRC_REFLECT_HELPER_DEFINE_CASE_SET
        }

    };

    void message_helper::set( const gpb::FieldDescriptor *fld,
                                                    std::string const &value )
    {
        impl_->set( fld, value );
    }

    void message_helper::set( const std::string &field_name,
                                                    std::string const &value )
    {
        const field_desc_type *fld(impl_->find_field( field_name ));
        if( !fld ) throw std::runtime_error( "Bad field name" );
        set( fld, value );
    }

    message_helper::message_helper(gpb::Message *mess )
        :message_reader(mess)
        ,impl_(new message_helper_impl(mess))
    {}

    message_helper::~message_helper( )
    {
        delete impl_;
    }

    void make_fields_map( const gpb::Descriptor *src, field_map_type &result,
                          const field_stack_type& current,
                          const std::string &name,
                          bool use_full_names, bool leafs_only )
    {
        for( int i=0, j=src->field_count(); i<j; ++i ) {

            field_stack_type current_stack(current);
            const gpb::FieldDescriptor *fld(src->field(i));
            const std::string &fld_name(fld->lowercase_name( ));

            std::string current_name = name.empty( )
                    ? fld_name
                    : name + "." + fld_name;

            current_stack.push_back( fld );

            bool for_add = true;
            if( fld->type() == gpb::FieldDescriptor::TYPE_MESSAGE ) {
                for_add = !leafs_only;
                make_fields_map( fld->message_type( ),
                    result, current_stack, current_name,
                    use_full_names, leafs_only);
            }
            if( for_add )
                result.insert(
                    std::make_pair(use_full_names ? current_name : fld_name,
                                                               current_stack) );
        }
    }

    void make_fields_map( const gpb::Descriptor *src, field_map_type &result,
                          bool use_full_names, bool leafs_only )
    {
        field_stack_type curr_stack;
        field_map_type tmp;
        std::string    name;
        make_fields_map( src, tmp, curr_stack,
                         name, use_full_names, leafs_only );
        result.swap( tmp );
    }

}}

