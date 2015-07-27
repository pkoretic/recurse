#ifndef RECURSE_RESPONSE_HPP
#define RECURSE_RESPONSE_HPP

struct Response {
    QString body;
    QHash<QString, QString> header;
    unsigned short int status;
    QString method, proto;
    QHash<unsigned short int, QString> http_codes;

    Response() {
        http_codes.insert(200, "OK");
        http_codes.insert(204, "No Content");
        http_codes.insert(404, "Not Found");
        http_codes.insert(500, "Internal Server Error");
    };
};

#endif
