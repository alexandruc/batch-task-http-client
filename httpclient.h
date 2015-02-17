#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <ostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;

namespace http {


/**
 * @brief The contents of the http response
 */
struct ResponseObject {
    unsigned int status_code;
    std::string body;
};

class HttpClient
{
public:

    HttpClient(boost::asio::io_service& io_service,
               boost::asio::ssl::context& context,
               const std::string& server,
               const std::string& path,
               const std::string& protocol);

    ResponseObject getResponse();

private:
    void handle_resolve(const boost::system::error_code& err,
                        tcp::resolver::iterator endpoint_iterator);

    /* ssl handlers */
    bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);
    void handle_handshake(const boost::system::error_code& error);

    /* regular handlers */
    void handle_connect(const boost::system::error_code& err);
    void handle_write_request(const boost::system::error_code& err);
    void handle_read_status_line(const boost::system::error_code& err);

    void handle_read_headers(const boost::system::error_code& err);
    void handle_read_content(const boost::system::error_code& err);

    //data used for connections
    tcp::resolver m_resolver;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket;
    boost::asio::streambuf m_request;
    boost::asio::streambuf m_response;
    std::string m_protocol;
    //data for responses
    unsigned int m_status_code;
    std::stringstream m_response_stream;
};
} //namespace http

#endif // HTTPCLIENT_H
