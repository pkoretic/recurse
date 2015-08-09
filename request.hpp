#ifndef RECURSE_REQUEST_HPP
#define RECURSE_REQUEST_HPP

#include <QTcpSocket>
#include <QHash>

#include "context.hpp"

struct Request {
    // tcp request data
    QString data;
    QTcpSocket *socket;

    // higher level data
    QHash<QString, QString> header;
    QHash<QString, QVariant> body_parsed;
    QString body, method, proto, url;
    qint64 body_length = 0;

    Context ctx;
};

#endif
