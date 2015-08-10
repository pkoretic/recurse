#ifndef RECURSE_REQUEST_HPP
#define RECURSE_REQUEST_HPP

#include <QTcpSocket>
#include <QHash>

#include "context.hpp"

struct Request {
    QString data;
    QTcpSocket *socket;

    //!
    //! \brief header
    //! Http request headers, eg: header["content-type"] = "text/plain"
    //!
    QHash<QString, QString> header;

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
    //! Http method, eg: GET
    //!
    QString method;

    //!
    //! \brief proto
    //! Http protocol, eg: HTTP
    //!
    QString proto;

    //!
    //! \brief url
    //! Http request url, eg: /helloworld
    //!
    QString url;

    //!
    //! \brief length
    //! Http request Content-Length
    //!
    qint64 length = 0;

    //!
    //! \brief ip
    //! Client ip address
    //!
    QHostAddress ip;

    //!
    //! \brief cookies
    //! Http cookies in key/value form
    //!
    QHash<QString, QString> cookies;

    //!
    //! \brief hostname
    //! Http hostname from "Host" http header
    //!
    QString hostname;

    //!
    //! \brief ctx
    //! Context that we can pass around and reuse
    //!
    Context ctx;

    //!
    //! \brief get
    //! Return http request header
    //!
    //! \param key of the header
    //! \return header value
    //!
    QString get(const QString &key) { return header[key]; };
};

#endif
