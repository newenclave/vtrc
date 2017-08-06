#ifndef MESSAGE_UTILITIES_H
#define MESSAGE_UTILITIES_H

namespace google { namespace protobuf {
    class Message;
}}

namespace vtrc { namespace utilities {

    bool make_message_diff( google::protobuf::Message const &templ,
                            google::protobuf::Message const &src,
                            google::protobuf::Message &result  );

    void merge_messages( google::protobuf::Message &to,
                         google::protobuf::Message const &from );

}}

#endif // MESSAGEUTILITIES_H
