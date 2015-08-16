#ifndef RECURSE_RESPONSE_HPP
#define RECURSE_RESPONSE_HPP

#include <QHash>
#include <QJsonDocument>
#include <functional>

class Response {

public:

    QString get(const QString &key) { return header[key]; };
    Response &set(const QString &key, const QString &value) { header[key] = value; return *this; };

    quint16 status() const { return m_status; };
    Response &status(quint16 status) { m_status = status; return *this; };

    QString type() const { return header["content-type"]; };
    Response &type(const QString &type) { header["content-type"] = type; return *this; };

    QString body() const { return m_body;};
    Response &body(const QString &body) { m_body = body; return *this; };

    Response &write(const QString &msg) { m_body += msg; return *this; };

    void send(const QString &body = "") {
        if (body.size())
            m_body = body;

        end();
    };

    void send(const QJsonDocument &body) {
        type("application/json");
        qDebug() << " body:" << body;
        m_body = body.toJson(QJsonDocument::Compact);

        end();
    }

    // final function set and called from recurse
    std::function<void()> end;

    QHash<QString, QString> header;
    QString method, protocol;

    QHash<quint16, QString> http_codes
    {
        {100, "Continue"},
        {101, "Switching Protocols"},
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {205, "Reset Content"},
        {206, "Partial Content"},
        {300, "Multiple Choices"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {305, "Use Proxy"},
        {307, "Temporary Redirect"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Time-out"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Request Entity Too Large"},
        {414, "Request-URI Too Large"},
        {415, "Unsupported Media Type"},
        {416, "Requested range not satisfiable"},
        {417, "Expectation Failed"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Time-out"},
        {505, "HTTP Version not supported"}
    };

    //!
    //! \brief create_reply
    //! create reply for sending to client
    //!
    //! \return QString reply to be sent
    //!
    QString create_reply();

private:
    quint16 m_status = 200;
    QString m_body;
};

// https://tools.ietf.org/html/rfc7230#page-19
QString Response::create_reply()
{
    qDebug() << __FUNCTION__ << this->body();

    qDebug() << "response header:" << this->header;

    QString reply = this->protocol % " " % QString::number(this->status()) % " "
        % this->http_codes[this->status()] % "\r\n";

    // set content length
    this->header["content-length"] = QString::number(this->body().size());

    // set content type if not set
    if (!this->header.contains("content-type"))
        this->header["content-type"] = "text/plain";

    // set custom header fields
    for (auto i = this->header.constBegin(); i != this->header.constEnd(); ++i)
        reply = reply % i.key() % ": " % i.value() % "\r\n";

    reply += "\r\n";

    if (this->body().size())
        reply += this->body();

    return reply;
};

#endif
