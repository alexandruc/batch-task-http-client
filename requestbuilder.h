#ifndef REQUESTBUILDER_H
#define REQUESTBUILDER_H

#include <string>
#include <map>

namespace http {

typedef std::map<std::string, std::string> ParamMap;

/**
 * @brief Base class for http requests
 */
class RequestBuilder
{
public:

    /* Tokens to build the request url */
    struct Tokens {
        static const std::string url_and;
        static const std::string url_qmark;
        static const std::string url_slash;
        static const std::string url_equal;
    };

    RequestBuilder();
    virtual ~RequestBuilder();

    /* clears all parameters from the request */
    void clear();
    /* adds a key/value parameter to the request parameter list */
    void addParam(const std::string& key, const std::string& value);
    /* removes a parameter by key */
    void removeParam(const std::string& key);
    /* needs to be implemented by the derived classes in order to customize the request string */
    virtual std::string buildRequest() const = 0;

protected:
    ParamMap m_paramMap;
};


}//namespace http
#endif // REQUESTBUILDER_H
