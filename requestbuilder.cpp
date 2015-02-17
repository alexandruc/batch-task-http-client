#include "requestbuilder.h"

namespace http{

/* static */ const std::string RequestBuilder::Tokens::url_and = "&";
/* static */ const std::string RequestBuilder::Tokens::url_qmark = "?";
/* static */ const std::string RequestBuilder::Tokens::url_slash = "/";
/* static */ const std::string RequestBuilder::Tokens::url_equal = "=";

RequestBuilder::RequestBuilder()
{

}

RequestBuilder::~RequestBuilder()
{

}

void RequestBuilder::clear()
{
    m_paramMap.clear();
}

void RequestBuilder::addParam(const std::string& key, const std::string& value)
{
    m_paramMap.insert(std::pair<std::string, std::string>(key, value));
}

void RequestBuilder::removeParam(const std::string& key)
{
    m_paramMap.erase(key);
}

}//namespace http
