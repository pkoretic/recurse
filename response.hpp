#ifndef RECURSE_RESPONSE_HPP
#define RECURSE_RESPONSE_HPP

struct Response {
    QString body;
    QHash<QString, QString> header;
    unsigned short int status;
    QString method, proto;
    QHash<unsigned short int, QString> http_codes
    {
        {200, "OK"},
        {204, "No Content"},
        {404, "Not Found"},
        {500, "Internal Server Error"}
    };
    QHash<QString, QString> default_headers
    {
        {"content-type", "text/html; charset=utf-8"}
    };
};

#endif
