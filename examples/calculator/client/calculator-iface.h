#ifndef CALCULATORIFACE_H
#define CALCULATORIFACE_H

#include <string>
#include "vtrc-memory.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace interfaces {

    struct calculator {

        virtual ~calculator( ) { }

        virtual double sum( double l, double r )             const = 0;
        virtual double sum( std::string const &l, double r ) const = 0;
        virtual double sum( double l, std::string const &r ) const = 0;
        virtual double sum( std::string const &l,
                            std::string const &r ) const = 0;

        virtual double mul( double l, double r )             const = 0;
        virtual double mul( std::string const &l, double r ) const = 0;
        virtual double mul( double l, std::string const &r ) const = 0;
        virtual double mul( std::string const &l,
                            std::string const &r ) const = 0;

        virtual double div( double l, double r )             const = 0;
        virtual double div( std::string const &l, double r ) const = 0;
        virtual double div( double l, std::string const &r ) const = 0;
        virtual double div( std::string const &l,
                            std::string const &r ) const = 0;

        virtual double pow( double l, double r )             const = 0;
        virtual double pow( std::string const &l, double r ) const = 0;
        virtual double pow( double l, std::string const &r ) const = 0;
        virtual double pow( std::string const &l,
                            std::string const &r ) const = 0;

        virtual unsigned calls_count( ) const = 0;

    };

    calculator *create_calculator( vtrc::shared_ptr<vtrc::client::vtrc_client> client );

}

#endif // CALCULATORIFACE_H
