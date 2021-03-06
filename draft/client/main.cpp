#include <iostream>
#include <fcntl.h>
#include <vector>

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/eventfd.h>

#include <errno.h>

namespace ba = boost::asio;
std::vector<char> data(4096);

void start_read( ba::posix::stream_descriptor &desc );

void read_handler( boost::system::error_code const &e,
                   size_t bytes,
                   ba::posix::stream_descriptor &desc)
{
    if( !e ) {
        std::cout << "read " << bytes << "\n";
    } else {
        std::cout << "read error: " << e.message( ) << "\n";
        boost::this_thread::sleep_for( boost::chrono::milliseconds(500) );
        //throw std::runtime_error( e.message( ) );
    }
    start_read( desc );
}

void start_read( ba::posix::stream_descriptor &desc )
{
    desc.async_read_some( ba::buffer(data),
                          boost::bind( &read_handler,
                                       ba::placeholders::error,
                                       ba::placeholders::bytes_transferred,
                                       boost::ref(desc)) );
}

struct add_del_struct {
    int      fd_;
    uint32_t flags_;
};

//eventfd

void fd_cb( int fd, int add, int stop )
{
    char bl = 0;
    lseek( fd, 0, SEEK_SET );
    read( fd, &bl, 1 );
    std::cout << "Read 1 byte from " << fd
              << " result: '" << bl << "'\n";
}

int add_fd_to_epoll( int ep, int ev, uint32_t flags )
{
    struct epoll_event epv;

    epv.events   = flags;
    epv.data.fd  = ev;

    return epoll_ctl( ep, EPOLL_CTL_ADD, ev, &epv );
}

int add_event_to_epoll( int ep, int ev )
{
    return add_fd_to_epoll( ep, ev, EPOLLIN | EPOLLET );
}

int del_fd_from_epoll( int ep, int ev )
{
    struct epoll_event epv = { 0 };
    epv.data.fd  = ev;
    return epoll_ctl( ep, EPOLL_CTL_DEL, ev, &epv );
}

void add_fd( int add_event, int fd )
{
    add_del_struct *new_fd = (add_del_struct *)malloc( sizeof(*new_fd) );

    std::cout << "Create new data: "
              << std::hex << new_fd << std::dec
              << "\n";

    new_fd->fd_     = fd;
    new_fd->flags_  = EPOLLIN | EPOLLET | EPOLLPRI;

    eventfd_write( add_event, (eventfd_t)(new_fd) );
}

void poll_thread( int add_event,
                  int del_event,
                  int stop_event,
                  boost::function<void (int)> cb,
                  ba::io_service &disp )
{
    int epfd = epoll_create( 1024 );
    int working = 1;

    std::cout << "Epoll: " << epfd << "\n";

    if( epfd > 0 ) {

        int ar = 0;

        ar = add_event_to_epoll( epfd, add_event );
        std::cout << "Epoll add: " << ar << " " << add_event << "\n";

        ar = add_event_to_epoll( epfd, del_event );
        std::cout << "Epoll del: " << ar << " " << del_event << "\n";

        ar = add_event_to_epoll( epfd, stop_event );
        std::cout << "Epoll stop: " << ar << " " << stop_event << "\n";

        struct epoll_event rcvd[1] = {0};

        while( working ) {

            std::cout << " >> Start reading event\n";

            int count = epoll_wait( epfd, rcvd, 1, -1);

            std::cout << " << Got " << count << "\n";

            if( -1 == count ) {
                std::cerr << "epoll_wait failed: " << errno << "\n";
                working = 0;
            } else {

                std::cout << "Got "
                          << count << " event"
                          << " " << rcvd[0].data.fd
                          << "\n";


                add_del_struct *data = 0;

                if( rcvd[0].data.fd == add_event ) {

                    int res = eventfd_read( add_event, (eventfd_t *)(&data) );

                    std::cout << "Read ptr data (add) "
                              << " res = " << res
                              << " errno " << errno
                              << " 0x"
                              << std::hex << data << std::dec
                              << "\n";

                    res = add_fd_to_epoll( epfd, data->fd_, data->flags_ );

                    std::cout << "Add fd " << data->fd_
                              << " res = " << res
                              << "\n";

                    free( data );

                } else if( rcvd[0].data.fd == del_event ) {

                    int res = eventfd_read( del_event, (eventfd_t *)(&data) );

                    std::cout << "Read ptr data (del) 0x"
                              << std::hex << data << std::dec
                              << "\n";

                    res = del_fd_from_epoll( epfd, data->fd_ );

                    std::cout << "Del fd " << data->fd_
                              << " res = " << res
                              << "\n";

                    free( data );

                } else if( rcvd[0].data.fd == stop_event ) {
                    working = 0;
                    std::cout << "Stop rcved!\n";
                } else {
                    std::cout << "Cb rcved for " << rcvd[0].data.fd << "\n";
                    cb( rcvd[0].data.fd );
                }
            }
        }

        close( add_event );
        close( del_event );
        close( stop_event );
    }
}

int main( int argc, const char *argv[ ] ) try
{
    ba::io_service ios;
    ba::io_service::work w(ios);

    ba::posix::stream_descriptor sd(ios);

    int add  = eventfd( 0, 0 );
    int del  = eventfd( 0, 0 );
    int stop = eventfd( 0, 0 );

    boost::function<void (int)> fcb(boost::bind( fd_cb, _1, add, stop ));

    boost::thread t( boost::bind( poll_thread, add, del, stop,
                                  fcb, boost::ref(ios)) );

    int fd = open( argv[1], O_RDWR | O_NONBLOCK );

    std::cout << "Fd: " << fd << "\n";

    add_fd( add, fd );

    //write( add, &new_fd, sizeof(new_fd) );

//    while( 1 ) {
//        ios.run_one( );
//    }

    t.join( );

    std::cout << "Stop!\n";

    return 0;
} catch( const std::exception &ex ) {
    std::cerr << ex.what( ) << "\n";
    return 1;
}

