#include "httpclient.h"
#include <iostream>

#define FLOG(x) {std::cout << __FILE__ << ":" << __FUNCTION__ << "(" << __LINE__ << "): " << x << std::endl;}

namespace http {

HttpClient::HttpClient(boost::asio::io_service& io_service,
                       boost::asio::ssl::context& context,
                       const std::string& server,
                       const std::string& path,
                       const std::string& protocol)
    : m_resolver(io_service),
      m_socket(io_service, context),
      m_protocol(protocol)
{
    FLOG("START");

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&m_request);
    request_stream << "GET " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    tcp::resolver::query query(server, m_protocol.c_str());
    m_resolver.async_resolve(query,
                            boost::bind(&HttpClient::handle_resolve, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::iterator));
    FLOG("END");
}

void HttpClient::handle_resolve(const boost::system::error_code& err,
                    tcp::resolver::iterator endpoint_iterator)
{
    if (!err)
    {
        FLOG("Resolve OK");
        m_socket.set_verify_mode(boost::asio::ssl::verify_peer);
        m_socket.set_verify_callback(
                    boost::bind(&HttpClient::verify_certificate, this, _1, _2));

        // Attempt a connection to each endpoint in the list until we
        // successfully establish a connection.
        boost::asio::async_connect(m_socket.lowest_layer(), endpoint_iterator,
                                   boost::bind(&HttpClient::handle_connect, this,
                                               boost::asio::placeholders::error));
    }
    else
    {
        FLOG("Error: " << err.message());
    }
}

bool HttpClient::verify_certificate(bool preverified,
                                    boost::asio::ssl::verify_context& ctx)
{
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    FLOG("Verifying " << subject_name);

    return preverified;
}

void HttpClient::handle_connect(const boost::system::error_code& err)
{
    if (!err)
    {
        FLOG("Connect OK ");
        m_socket.async_handshake(boost::asio::ssl::stream_base::client,
                                boost::bind(&HttpClient::handle_handshake, this,
                                            boost::asio::placeholders::error));

    }
    else
    {
        FLOG("Error: " << err.message());
    }
}

void HttpClient::handle_handshake(const boost::system::error_code& err)
{
    if (!err)
    {
        FLOG("Handshake OK ");
        const char* header=boost::asio::buffer_cast<const char*>(m_request.data());
        FLOG("Request: \n" << header);

        // The handshake was successful. Send the request.
        boost::asio::async_write(m_socket, m_request,
                                 boost::bind(&HttpClient::handle_write_request, this,
                                             boost::asio::placeholders::error));
    }
    else
    {
        FLOG("Handshake failed: " << err.message());
    }
}


void HttpClient::handle_write_request(const boost::system::error_code& err)
{
    if (!err)
    {
        // Read the response status line. The response_ streambuf will
        // automatically grow to accommodate the entire line. The growth may be
        // limited by passing a maximum size to the streambuf constructor.
        boost::asio::async_read_until(m_socket, m_response, "\r\n",
                                      boost::bind(&HttpClient::handle_read_status_line, this,
                                                  boost::asio::placeholders::error));
    }
    else
    {
        FLOG("Error: " << err.message());
    }
}

void HttpClient::handle_read_status_line(const boost::system::error_code& err)
{
    if (!err)
    {
        // Check that response is OK.
        std::istream response_stream(&m_response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);

        FLOG("status code: " << status_code);
        m_status_code = status_code;

        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            FLOG("Invalid response");
            return;
        }
        if (status_code != 200)
        {
            FLOG("Response returned with status code:" << status_code);
            return;
        }
        FLOG("Status code: " << status_code);

        // Read the response headers, which are terminated by a blank line.
        boost::asio::async_read_until(m_socket, m_response, "\r\n\r\n",
                                      boost::bind(&HttpClient::handle_read_headers, this,
                                                  boost::asio::placeholders::error));
    }
    else
    {
        FLOG("Error: " << err.message());
    }
}

void HttpClient::handle_read_headers(const boost::system::error_code& err)
{
    if (!err)
    {
        // Process the response headers.
        std::istream response_stream(&m_response);
        std::string header;
        std::stringstream ss_header;
        while (std::getline(response_stream, header) && header != "\r"){
            ss_header << header << "\n";
        }
        FLOG("Header: \n" << ss_header.str());

        // Write whatever content we already have to response stream
        if (m_response.size() > 0){
            m_response_stream << &m_response;
        }

        // Start reading remaining data until EOF.
        boost::asio::async_read(m_socket, m_response,
                                boost::asio::transfer_at_least(1),
                                boost::bind(&HttpClient::handle_read_content, this,
                                            boost::asio::placeholders::error));
    }
    else
    {
        FLOG("Error: " << err.message());
    }
}

void HttpClient::handle_read_content(const boost::system::error_code& err)
{
    if (!err)
    {
        // Write all of the data that has been read so far.
        m_response_stream << &m_response;

        // Continue reading remaining data until EOF.
        boost::asio::async_read(m_socket, m_response,
                                boost::asio::transfer_at_least(1),
                                boost::bind(&HttpClient::handle_read_content, this,
                                            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
        FLOG("Error: " << err.message());
    }
}

ResponseObject  HttpClient::getResponse()
{
    ResponseObject response;
    response.body = m_response_stream.str();
    response.status_code = m_status_code;
    return response;
}

}//namespace http
