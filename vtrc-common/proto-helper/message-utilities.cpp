#include <algorithm>

#include "message-utilities.h"

#include "boost/scoped_ptr.hpp"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace vtrc { namespace utilities {

    namespace gpb = google::protobuf;

    bool make_message_diff( gpb::Message const &templ,
                            gpb::Message const &src,
                            gpb::Message &result  );

    bool field_diff( gpb::Message const &templ,
                     gpb::Message &src,
                     gpb::FieldDescriptor const *fld )
    {
        typedef gpb::FieldDescriptor fld_type;

        gpb::Reflection const *treflect( templ.GetReflection( ) );
        gpb::Reflection const *sreflect(   src.GetReflection( ) );


#define VTRC_CAS_NOT_REP_EQUAL( Type, Getter )          \
            case fld_type::Type:                        \
            if( treflect->Getter( templ, fld ) !=       \
                sreflect->Getter (src, fld ) )          \
            return false;                               \
            break

#define VTRC_CAS_REP_EQUAL( Type, Getter, index )            \
            case fld_type::Type:                             \
            equal = treflect->Getter( templ, fld, index ) == \
                    sreflect->Getter( src, fld, index );     \
            break

        if( !fld->is_repeated( ) ) {
            switch( fld->cpp_type( ) ) {
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_BOOL,    GetBool  );
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_UINT64,  GetUInt64);
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_UINT32,  GetUInt32);
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_INT64,   GetInt64 );
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_INT32,   GetInt32 );
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_FLOAT,   GetFloat );
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_DOUBLE,  GetDouble);
                VTRC_CAS_NOT_REP_EQUAL( CPPTYPE_STRING,  GetString);
                default:
                    break;
            }
            sreflect->ClearField( &src, fld );
            return true;
        } else {

            int tsize( templ.GetReflection( )->FieldSize( templ, fld ) );
            int ssize( src.GetReflection( )->FieldSize( src, fld ) );

            if( tsize != ssize ) return false;

            for( int i(0); i<tsize; ++i ) {

                bool equal(false);

                switch( fld->cpp_type() ) {
                    VTRC_CAS_REP_EQUAL( CPPTYPE_BOOL,   GetRepeatedBool,   i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_UINT64, GetRepeatedUInt64, i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_UINT32, GetRepeatedUInt32, i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_INT64 , GetRepeatedInt64,  i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_INT32 , GetRepeatedInt32,  i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_FLOAT , GetRepeatedFloat,  i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_DOUBLE, GetRepeatedDouble, i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_STRING, GetRepeatedString, i );
                    VTRC_CAS_REP_EQUAL( CPPTYPE_ENUM,	GetRepeatedEnum,   i );
                    default:
                        break;
                }
                if( !equal ) return false;
            }
        }

#undef VTRC_CAS_REP_EQUAL
#undef VTRC_CAS_NOT_REP_EQUAL

        sreflect->ClearField( &src, fld );
        return true;
    }

    static bool field_is_message( const gpb::FieldDescriptor *fld )
    {
        return fld->type( ) == gpb::FieldDescriptor::TYPE_MESSAGE;
    }

    bool message_has_repeated( const gpb::FieldDescriptor *fld )
    {
        if( !field_is_message( fld ) ) {
            return false;
        }

        const gpb::Descriptor *mess(fld->message_type());
        for(int i(0); i!=mess->field_count(); ++i) {
            gpb::FieldDescriptor const *next(mess->field(i));
            if( next && next->is_repeated() ) {
                return true;
            }
        }
        return false;
    }

    /// true if equal
    bool messages_diff ( gpb::Message const &templ,
                         gpb::Message &src,
                         gpb::FieldDescriptor const *fld )
    {
        gpb::Reflection const *treflect( templ.GetReflection( ) );
        gpb::Reflection const *sreflect(   src.GetReflection( ) );

        if( message_has_repeated( fld ) ) {
            gpb::Message const &tmpl_next( treflect->GetMessage( templ, fld ) );
            gpb::Message     *src_next( sreflect->MutableMessage( &src, fld ) );

            std::string tmpls(tmpl_next.SerializeAsString());
            std::string srss(src_next->SerializeAsString());

            return tmpls == srss;
        }

        if( !fld->is_repeated(  ) ) {

            gpb::Message const &tmpl_next( treflect->GetMessage( templ, fld ) );
            gpb::Message     *src_next( sreflect->MutableMessage( &src, fld ) );

            bool equal = false;
            if( make_message_diff( tmpl_next, *src_next, *src_next ) ) {
                sreflect->ClearField( &src, fld );
                equal = true;
            }
            return equal;
        }

        int tsize( templ.GetReflection( )->FieldSize( templ, fld ) );
        int ssize( src.GetReflection( )->FieldSize( src, fld ) );

        if( tsize != ssize ) return false;

        for( int i(0); i<tsize; ++i ) {
            std::string tmess( treflect->GetRepeatedMessage( templ, fld, i )
                               .SerializeAsString( ) );
            std::string smess( sreflect->GetRepeatedMessage( src, fld, i )
                               .SerializeAsString( ) );
            if( tmess.compare(smess) != 0 ) return false;
        }
        sreflect->ClearField( &src, fld );
        return true;
    }

    /// true if messages are equal
    bool make_message_diff( gpb::Message const &templ,
                            gpb::Message const &src,
                            gpb::Message &result  )
    {
        boost::scoped_ptr<gpb::Message> new_result(src.New());
        new_result->CopyFrom( src );

        typedef gpb::FieldDescriptor field_desc;
        typedef std::vector<const field_desc *> field_list;
        field_list fields;

        new_result->GetReflection( )->ListFields(*new_result, &fields);

        size_t changes(0);

        for( field_list::const_iterator b(fields.begin()), e(fields.end());
                                                                b!=e; ++b )
        {
            field_desc const *next(*b);
            if( !next->is_repeated() &&
                !templ.GetReflection( )->HasField( templ, next ) )
            {
                ++changes;
                continue;
            }

            if( next->cpp_type( ) == field_desc::CPPTYPE_MESSAGE ) {
                changes += (!messages_diff( templ, *new_result, next ));
            } else {
                changes += (!field_diff( templ, *new_result, next ));
            }
        }

        result.GetReflection()->Swap( new_result.get(), &result );
        return changes == 0;
    }

    /// merges
    /// crutch for repeated fields
    void clear_repeated( gpb::Message &to, gpb::Message const &from )
    {
        gpb::Reflection const *refl( to.GetReflection() );

        typedef std::vector<gpb::FieldDescriptor const *> flds_vector;

        flds_vector fields;

        refl->ListFields( from, &fields );
        flds_vector::const_iterator b(fields.begin( ));
        flds_vector::const_iterator e(fields.end( ));

        for(; b!=e; ++b) {
            const gpb::FieldDescriptor *next(*b);
            if ( next->is_repeated( ) ) {
                refl->ClearField( &to, next );
            } else if( field_is_message( next ) ) {
                if( message_has_repeated( next ) ) {

                    refl->MutableMessage( &to, next )->Clear( );

                } else if( refl->HasField( to, next ) ) {
                    clear_repeated( *refl->MutableMessage( &to, next ),
                                     refl->GetMessage( from, next ));
                }
            }
        }
    }

    gpb::Message* find_same_message(gpb::Message &to, gpb::Message const &from)
    {
        if (to.GetDescriptor() == from.GetDescriptor())
            return &to;

        for (int i=0; i<to.GetDescriptor()->field_count(); i++) {
            const gpb::FieldDescriptor* fld = to.GetDescriptor()->field(i);
            if (fld->is_repeated())
                continue;

            if (fld->type() != gpb::FieldDescriptor::TYPE_MESSAGE)
                continue;

            bool has_already = to.GetReflection()->HasField(to, fld);
            gpb::Message* found_message =
                    find_same_message( *to.GetReflection()
                                        ->MutableMessage(&to, fld), from);
            if (found_message)
                return found_message;

            if( !has_already ) to.GetReflection()->ClearField(&to, fld);
        }

        return NULL;
    }

    void merge_messages( gpb::Message &to,
                         gpb::Message const &from )
    {
        gpb::Message* msg = find_same_message(to, from);
        msg = msg ? msg : &to;
        clear_repeated( *msg, from );
        msg->MergeFrom( from );
    }

}}

