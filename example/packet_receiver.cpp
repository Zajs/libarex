//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING

#include <iostream>
#include <string>
#include <functional>
#include <stdexcept>
#include <arex.hpp>
#include <boost/asio.hpp>

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;
namespace arex = boost::asio::ip::arex;

class packet_receiver {
public:

    packet_receiver(boost::asio::io_service &io, std::string const &dev)
        : counter_(0), io_service_(io), socket_(io, arex::packet_p_all::v4())
    {
        socket_.bind(
            // The first argument is not used for receiving packets
            arex::packet_p_all::endpoint("FF:FF:FF:FF:FF:FF", dev)
        );
        start_receive();
    }

    static void print_ether_header(arex::ethernet_header const &eth)
    {
        cout << "Frame : " << (eth.is_802_3() ? "802.3" : "DIX") << endl;
        cout << "Source: " << eth.source() << endl;
        cout << "Dest  : " <<  eth.dest()  << endl;
        cout << "Type  : " << arex::ether_type_str(eth.eth_type()) << endl;
    }

    void recv_handler(
            boost::system::error_code const &ec,
            std::size_t size
        )
    {
        if ( ec )
            cerr << "[-] fatal: " << ec.message() << endl;
        else {
            recv_buffer_.commit(size);
            std::istream is(&recv_buffer_);
            arex::ethernet_header ethh;
            is >> ethh;

            cout << "---- ethernet header count=" << ++counter_ << " ----" << endl;
            print_ether_header(ethh);
            cout << endl;

            recv_buffer_.consume(recv_buffer_.size());
            start_receive();
        }
    }

    void start_receive()
    {
        socket_.async_receive_from(
            recv_buffer_.prepare(65536), 
            endpoint_from_,
            std::bind(&packet_receiver::recv_handler, this, _1, _2)
        );
    }

private:
    int counter_;
    boost::asio::io_service &io_service_;
    arex::packet_p_all::socket socket_;
    arex::packet_p_all::endpoint endpoint_from_;
    boost::asio::streambuf recv_buffer_;
};

int main(int argc, char **argv)
{
    try {
        
        if ( argc < 2 ) {
            cout << "Usage: ./a.out INTERFACE" << endl;
            return EXIT_FAILURE;
        }

        boost::asio::io_service io_service;
        packet_receiver recver(io_service, argv[1]);

        io_service.run();

    }
    catch ( std::exception &e ) {
        cerr << "[-] Exception: " << e.what() << endl;
    }
    return EXIT_SUCCESS;
}
