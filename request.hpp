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
};

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

    for (int i = 0; i < ref_data.size(); ++i)
    {
        if (!this->body.isEmpty())
        {
            this->body.append(ref_data.at(i));
            this->length += this->body.size();
            continue;
        }

        // parse request-line
        // we use this->method's status as an indicator to determine if the
        // first header line (request-line) arrived
        if (this->method.isEmpty())
        {
            if (ref_data.at(i) == "")
            {
                continue;
            }
            else
            {
                // request-line arrived
                QVector<QStringRef> request_line = ref_data.at(i).split(" ");
                this->method = request_line.at(0).toString();
                this->url = request_line.at(1).toString();
                this->protocol = request_line.at(2).toString();

                // if request-line is invalid, return 400
                // if protocol is invalid, return 505

                continue;
            }
        }
        // when you get a newline, set this->body = ""; so isNull becomes false
    }

    return 0;
}

#endif
