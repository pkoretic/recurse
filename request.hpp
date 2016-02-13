#ifndef RECURSE_REQUEST_HPP
#define RECURSE_REQUEST_HPP

#include <QTcpSocket>
#include <QHash>

class Request
{

public:
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
    QString url;

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
    //! \brief cookies
    //! HTTP cookies in key/value form
    //!
    QHash<QString, QString> cookies;

    //!
    //! \brief hostname
    //! HTTP hostname from "Host" HTTP header
    //!
    QString hostname;

    //!
    //! \brief get
    //! Return HTTP request header specified by the key
    //!
    //! \param QString case-insensitive key of the header
    //! \return QString header value
    //!
    QString get(const QString &key)
    {
        return m_header[key.toLower()];
    }

    //!
    //! \brief parse
    //! parse data from request
    //!
    //! \param QString request
    //! \return HTTP error code
    //!
    quint16 parse(QString request);

private:
    //!
    //! \brief header
    //! HTTP request headers, eg: header["content-type"] = "text/plain"
    //!
    QHash<QString, QString> m_header;

    //!
    //! \brief httpRx
    //! match HTTP request line
    //!
    QRegExp httpRx = QRegExp("^(?=[A-Z]).* \\/.* HTTP\\/[0-9]\\.[0-9]\\r\\n");

    const QList<QString> HttpMethods{
        "GET",
        "POST",
        "PUT",
        "DELETE",
        "OPTIONS",
        "CONNECT"
    };

    void parse_request_line(const QStringRef &req_line);
    quint16 validate_request_line();
};

inline void Request::parse_request_line(const QStringRef &req_line)
{
    for (int ch = 0; ch < req_line.size(); ++ch)
    {
        if (req_line.at(ch).isSpace())
        {
            continue;
        }

        // TODO: validate after each char
        // TODO: benchmark char loop vs validation regex
        if (this->method.length() > ch - 1)
        {
            this->method += req_line.at(ch);
            continue;
        }
        else if (this->method.length() + this->url.length() > ch - 2)
        {
            this->url += req_line.at(ch);
            continue;
        }
        else if (this->method.length() + this->url.length() + this->protocol.length() > ch - 3)
        {
            this->protocol += req_line.at(ch);
            continue;
        }
    }
}

inline quint16 Request::validate_request_line()
{
    // check method
    if (this->method.size() > 7 || !HttpMethods.contains(this->method))
        return 501;

    // TODO: check request-line entries
    // if request-line is invalid, return 400
    // check uri
    // (origin and absolute forms are used for all except OPTIONS and CONNECT methods)
    // (authority form is only used for a CONNECT method)
    // (asterisk form is only used for an OPTIONS method)
    //
    // if protocol is invalid, return 505

    return 0;
}

inline quint16 Request::parse(QString request)
{
    // if http request-line is not present, just append all data to request.body
    if (!request.contains(httpRx))
    {
        this->body.append(request);
        return true;
    }

    this->ip = this->socket->peerAddress();
    QVector<QStringRef> ref_data = request.splitRef("\r\n");

    for (int ln = 0; ln < ref_data.size(); ++ln)
    {
        if (!this->body.isNull())
        {
            this->body.append(ref_data.at(ln));
            this->length += this->body.size();
            continue;
        }

        // we use this->method's status as an indicator to determine if the
        // first header line (request-line) arrived
        if (this->method.isEmpty())
        {
            if (ref_data.at(ln) == "")
            {
                continue;
            }

            parse_request_line(ref_data.at(ln));
            auto code = validate_request_line();

            if (code != 0)
                return code;

            continue;
        }

        // got an empty line, so all subsequent lines in ref_data are body
        if (ref_data.at(ln).isEmpty())
        {
            this->body = "";
            continue;
        }

        // fill request headers
        qDebug() << "line" << ref_data.at(ln);
        // m_header[ref_data.at(ln).split(":").at(0).toString().trimmed()] = ref_data.at(ln).split(":").at(1).toString().trimmed();

        /*
        for (int ch = 0; ch < ref_data.at(ln).size(); ++ch)
        {
        }
        */
    }

    qDebug() << "headers:" << m_header;
    return 0;
}

#endif
