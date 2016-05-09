#ifndef RECURSE_REQUEST_HPP
#define RECURSE_REQUEST_HPP

#include "lib/http_parser_merged.h"
#include <QHash>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

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
    //! it's easier to provide a container here (which doesn't have to be used)
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
    QString protocol = "HTTP";

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
    //! it's easier to provide a container here (which doesn't have to be used)
    //!
    QHash<QString, QString> params;

    //!
    //! \brief length
    //! HTTP request Content-Length
    //!
    quint64 length = 0;

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
    QByteArray m_current_url;
    QByteArray m_current_body;

    http_parser_settings m_parser_settings{ on_message_begin, on_url, nullptr, on_header_field,
        on_header_value, on_headers_complete, on_body, on_message_complete, on_chunk_header,
        on_chunk_complete };

    static int on_message_begin(http_parser *parser)
    {
        auto connection = static_cast<Request *>(parser->data);

        connection->m_headers.clear();
        connection->url.clear();
        return 0;
    };

    // TODO: on_url+, on_status, on_header_field+, on_header_value+ and
    // on_body can arrive chunked, so append their data

    static int on_url(http_parser *parser, const char *at, size_t length)
    {
        auto connection = static_cast<Request *>(parser->data);

        connection->m_current_url.append(at, length);
        return 0;
    };

    static int on_header_field(http_parser *parser, const char *at, size_t length)
    {
        auto connection = static_cast<Request *>(parser->data);

        qDebug() << "header field" << QString::fromUtf8(at, length);

        if (!(connection->m_current_header_field.isEmpty())
            && !(connection->m_current_header_value.isEmpty()))
        {
            connection->m_headers[connection->m_current_header_field.toLower()]
                = connection->m_current_header_value;

            // clear headers to simply append to them later
            connection->m_current_header_field = QString();
            connection->m_current_header_value = QString();
        }

        connection->m_current_header_field += QString::fromLatin1(at, length);
        return 0;
    };

    static int on_header_value(http_parser *parser, const char *at, size_t length)
    {
        auto connection = static_cast<Request *>(parser->data);

        qDebug() << "header value" << QString::fromUtf8(at, length);

        connection->m_current_header_value += QString::fromLatin1(at, length);
        return 0;
    };

    static int on_headers_complete(http_parser *parser)
    {
        auto connection = static_cast<Request *>(parser->data);

        // add last header
        connection->m_headers[connection->m_current_header_field.toLower()]
            = connection->m_current_header_value;

        // TODO: if 0, respond with the "Connection: close" header and close the connection
        qDebug() << http_should_keep_alive(parser);

        // set url and query parameters
        connection->url = QString(connection->m_current_url);
        connection->query.setQuery(connection->url.query());

        if (connection->m_headers.contains("host"))
            connection->hostname = connection->m_headers["host"];

        if (connection->m_headers.contains("content-length"))
            connection->length = connection->m_headers["content-length"].toULongLong();

        // extract cookies
        // eg: USER_TOKEN=Yes;test=val
        if (connection->m_headers.contains("cookie"))
        {
            for (const auto &cookie : connection->m_headers["cookie"].splitRef(";"))
            {
                int split = cookie.indexOf("=");

                if (split == -1)
                    continue;

                auto key = cookie.left(split);

                if (!key.size())
                    continue;

                auto value = cookie.mid(split + 1);

                connection->m_cookies[key.toString().toLower()] = value.toString();
            }
        }

        // set user's remote ip address
        connection->ip = connection->socket->peerAddress();

        qDebug() << "all headers arrived" << connection->length;

        connection->method = http_method_str(static_cast<http_method>(parser->method));
        return 0;
    };

    static int on_body(http_parser *parser, const char *at, size_t length)
    {
        auto connection = static_cast<Request *>(parser->data);

        connection->m_current_body.append(at, length);
        return 0;
    };

    static int on_chunk_header(http_parser *parser)
    {
        auto connection = static_cast<Request *>(parser->data);

        qDebug() << "on_chunk_header" << parser->content_length;
        return 0;
    };

    static int on_message_complete(http_parser *parser)
    {
        auto connection = static_cast<Request *>(parser->data);

        // TODO: if 0, respond with the "Connection: close" header and close the connection
        qDebug() << http_should_keep_alive(parser);

        // set body
        connection->body = QString(connection->m_current_body);

        qDebug() << "message complete for url" << connection->url;
        qDebug() << "body" << connection->body;
        return 0;
    };


    static int on_chunk_complete(http_parser *parser)
    {
        auto connection = static_cast<Request *>(parser->data);

        // TODO: if chunked, readyRead from recurse.hpp gets fired multiple times!
        qDebug() << "on_chunk_complete";
        return 0;
    }
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
    qDebug() << "start exec\n";
    auto nparsed = http_parser_execute(&m_parser, &m_parser_settings, data, qstrlen(data));
    qDebug() << "end exec" << nparsed;

    return true;
}

#endif
