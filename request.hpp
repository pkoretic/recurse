#ifndef RECURSE_REQUEST_HPP
#define RECURSE_REQUEST_HPP

struct Request {
    // tcp request data
    QString data;

    // higher level data
    QHash<QString, QString> header;
    QString body, method, proto, url;
};

#endif
