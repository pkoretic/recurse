#ifndef RECURSE_REQUEST_HPP
#define RECURSE_REQUEST_HPP

#include <QTcpSocket>
#include <QHash>
#include <QUrl>
#include <QUrlQuery>
#include "lib/http_parser_merged.h"

class Request
{

public:
    Request();

    //!
    //! \brief data
    //! client request buffer data
    //!
    QString data;

    //!
    //! \brief socket
    //! underlying client socket
    //!
    QTcpSocket *socket;

    //!
    //! \brief body_parsed
    //! Data to be filled by body parsing middleware
    //!
    QHash<QString, QVariant> body_parsed;

    //!
    //! \brief body
    //!
    QString body;

    //!
    //! \brief method
    //! HTTP method, eg: GET
    //!
    QString method;

    //!
    //! \brief protocol
    //! Request protocol, eg: HTTP
    //!
    QString protocol;

    //!
    //! \brief secure
    //! Shorthand for protocol == "HTTPS" to check if a request was issued via TLS
    //!
    bool secure = protocol == "HTTPS";

    //!
    //! \brief url
    //! HTTP request url, eg: /helloworld
    //!
    QUrl url;

    //!
    //! \brief query
    //! query strings
    //!
    QUrlQuery query;

    //!
    //! \brief params
    //!
    //! request parameters that can be filled by router middlewares
    //! it's easier to provide container here (which doesn't have to be used)
    QHash<QString, QString> params;

    //!
    //! \brief length
    //! HTTP request Content-Length
    //!
    qint64 length = 0;

    //!
    //! \brief ip
    //! Client ip address
    //!
    QHostAddress ip;

    //!
    //! \brief hostname
    //! HTTP hostname from "Host" HTTP header
    //!
    QString hostname;

    //!
    //! \brief getHeader
    //! return header value, keys are saved in lowercase
    //! \param key QString
    //! \return QString header value
    //!
    QString getHeader(const QString &key)
    {
        return m_headers[key];
    }

    //!
    //! \brief getRawHeader
    //! return original header name as sent by client
    //! \param key QString case-sensitive key of the header
    //! \return QString header value as sent by client
    //!
    QHash<QString, QString> getRawHeaders()
    {
        return m_headers;
    }

    //!
    //! \brief getCookie
    //! return cookie value with lowercase name
    //! \param key case-insensitive cookie name
    //! \return
    //!
    QString getCookie(const QString &key)
    {
        return m_cookies[key.toLower()];
    }

    //!
    //! \brief getRawCookie
    //! return cookie name as sent by client
    //! \param key case-sensitive cookie name
    //! \return
    //!
    QString getRawCookie(const QString &key)
    {
        return m_cookies[key];
    }

    //!
    //! \brief getParam
    //! return params value
    //! \param key of the param, eg: name
    //! \return value of the param, eg: johnny
    //!
    QString getParam(const QString &key)
    {
        return params.value(key);
    }

    //!
    //! \brief parse
    //! parse data from request
    //!
    //! \param QString request
    //! \return true on success, false otherwise, considered bad request
    //!
    bool parse(const char *data);

private:
    //!
    //! \brief header
    //! HTTP request headers, eg: header["content-type"] = "text/plain"
    //!
    QHash<QString, QString> m_headers;

    //!
    //! \brief cookies
    //! HTTP cookies in key/value form
    //!
    QHash<QString, QString> m_cookies;

    //!
    //! \brief httpRx
    //! match HTTP request line
    //!
    QRegExp httpRx = QRegExp("^(?=[A-Z]).* \\/.* HTTP\\/[0-9]\\.[0-9]\\r\\n");

    http_parser m_parser;
    QString m_current_header_field;
    QString m_current_header_value;

    http_parser_settings m_parser_settings{
	nullptr,
	on_url,
	on_status,
	on_header_field,
	on_header_value,
	on_headers_complete,
	on_body,
	on_message_complete,
	on_chunk_header,
	nullptr
    };

    static int on_url(http_parser *parser, const char *at, size_t length)
    {
        auto request = static_cast<Request *>(parser->data);

        request->url = QString::fromUtf8(at, length);
        request->query.setQuery(request->url.query());
        qDebug() << "url test" << request->url;

        return 0;
    };

    static int on_status(http_parser *parser, const char *at, size_t length)
    {
        auto request = static_cast<Request *>(parser->data);

        qDebug() << "on_status test" << QString::fromUtf8(at, length);
        return 0;
    };

    static int on_header_field(http_parser *parser, const char *at, size_t length)
    {
        auto request = static_cast<Request *>(parser->data);

        qDebug() << "header field test" << QString::fromUtf8(at, length);

        request->m_current_header_field = QString::fromLatin1(at, length);
        return 0;
    };

    static int on_header_value(http_parser *parser, const char *at, size_t length)
    {
        auto request = static_cast<Request *>(parser->data);

        qDebug() << "header value test" << QString::fromUtf8(at, length);
        return 0;
    };

    static int on_headers_complete(http_parser *parser)
    {
        auto request = static_cast<Request *>(parser->data);

        qDebug() << "all headers arrived";
        return 0;
    };

    static int on_body(http_parser *parser, const char *at, size_t length)
    {
        auto request = static_cast<Request *>(parser->data);

        qDebug() << "body:" << QString::fromUtf8(at, length);
        return 0;
    };

    static int on_chunk_header(http_parser *parser)
    {
        auto request = static_cast<Request *>(parser->data);

        qDebug() << "on_chunk_header" << parser->content_length;
        return 0;
    };
    static int on_message_complete(http_parser *parser)
    {
        auto request = static_cast<Request *>(parser->data);

        qDebug() << "message complete for url" << request->url;
        return 0;
    };
};

inline Request::Request()
{
    // initialize http-parser
    http_parser_init(&m_parser, HTTP_REQUEST);
    m_parser.data = this;

    qDebug() << "parser initialized";
}

inline bool Request::parse(const char *data)
{
    // Save client ip address
    this->ip = this->socket->peerAddress();

    qDebug() << "start exec\n";
    auto nparsed = http_parser_execute(&m_parser, &m_parser_settings, data, qstrlen(data));
    qDebug() << "end exec" << nparsed;

    /*
    // if no header is present, just append all data to request.body
    if (!this->data.contains(httpRx))
    {
        this->body.append(this->data);
        return true;
    }

    auto data_list = this->data.splitRef("\r\n");
    bool is_body = false;

    for (int i = 0; i < data_list.size(); ++i)
    {
        if (is_body)
        {
            this->body.append(data_list.at(i));
            this->length += this->body.size();
            continue;
        }

        auto entity_item = data_list.at(i).split(":");

        if (entity_item.length() < 2 && entity_item.at(0).size() < 1 && !is_body)
        {
            is_body = true;
            continue;
        }
        else if (i == 0 && entity_item.length() < 2)
        {
            auto first_line = entity_item.at(0).split(" ");
            this->method = first_line.at(0).toString();
            this->url = first_line.at(1).toString();
            this->query.setQuery(this->url.query());
            this->protocol = first_line.at(2).toString();
            continue;
        }

        m_headers[entity_item.at(0).toString().toLower()] = entity_item.at(1).toString();
    }

    if (m_headers.contains("host"))
        this->hostname = m_headers["host"];

    // extract cookies
    // eg: USER_TOKEN=Yes;test=val
    if (m_headers.contains("cookie"))
    {
        for (const auto &cookie : m_headers["cookie"].splitRef(";"))
        {
            int split = cookie.indexOf("=");
            if (split == -1)
                continue;

            auto key = cookie.left(split);
            if (!key.size())
                continue;

            auto value = cookie.mid(split + 1);

            m_cookies[key.toString().toLower()] = value.toString();
        }
    }
    */

    return true;
}

#endif
