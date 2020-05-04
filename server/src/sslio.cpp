#include "sslio.h"

#include <boost/bind.hpp>

#include "cppbase.h"

SslIO::SslIO(io_service_t& io_service, context_t& context)
      : socket_(io_service, context),
        busy(false)
{
}
SslIO::~SslIO(){}

// lowest_layer_type& SslIO::socket() {
//     return socket_.lowest_layer();
// }

//TODO: Support call back function to avoid copy when not busy
void SslIO::SendLater(void *data, size_t len) {
    if (busy) {
        size_t oldsz = sendbuf.size();
        sendbuf.resize(oldsz + len);
        memcpy(sendbuf.data() + oldsz, data, len);
    } else {
        sending.resize(len);
        memcpy(sending.data(), data, len);
        dbgcout << "sending " << len << " bytes\n";
        boost::asio::async_write(socket_,
            boost::asio::buffer(sending),
            boost::bind(&SslIO::handle_send, this, boost::asio::placeholders::error));
    }
}
void SslIO::StartSend() {
    swap(sendbuf, sending);
    dbgcout << "sending " << sending.size() << " bytes\n";
    boost::asio::async_write(socket_,
        boost::asio::buffer(sending),
        boost::bind(&SslIO::handle_send, this, boost::asio::placeholders::error));
}
void SslIO::handle_send(const boost::system::error_code& error) {
    if (error) {
        handle_sslio_error(error);
        return;
    }
    sending.clear();
    if (sendbuf.empty()) {
        busy = false;
    } else {
        StartSend();
    }
}
void SslIO::handle_sslio_error(const boost::system::error_code& error) {
    dbgcout << "handle_sslio_error: " << error.message() << '\n';
}