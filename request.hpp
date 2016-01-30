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
        "OPTIONS"
    };
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

    for (int ln = 0; ln < ref_data.size(); ++ln)
    {
        if (!this->body.isNull())
        {
            this->body.append(ref_data.at(ln));
            this->length += this->body.size();
            continue;
        }

        // parse request-line
        // we use this->method's status as an indicator to determine if the
        // first header line (request-line) arrived
        if (this->method.isEmpty())
        {
            if (ref_data.at(ln) == "")
            {
                continue;
            }

            // request-line arrived
            for (int ch = 0; ch < ref_data.at(ln).size(); ++ch)
            {
                if (ref_data.at(ln).at(ch).isSpace())
                {
                    continue;
                }

                if (this->method.length() > ch - 1)
                {
                    this->method += ref_data.at(ln).at(ch);
                    continue;
                }
                else if (this->method.length() + this->url.length() > ch - 2)
                {
                    this->url += ref_data.at(ln).at(ch);
                    continue;
                }
                else if (this->method.length() + this->url.length() + this->protocol.length() > ch - 3)
                {
                    this->protocol += ref_data.at(ln).at(ch);
                    continue;
                }
            }

            qDebug() << this->method << this->url << this->protocol;

            // longest supported method name is OPTIONS
            if (this->method.size() > 7 || !HttpMethods.contains(this->method))
                return 501;

            // TODO: check request-line entries
            // if request-line is invalid, return 400
            // if protocol is invalid, return 505
            continue;
        }

        // got an empty line, so all subsequent lines in ref_data are body
        if (ref_data.at(ln).isEmpty())
        {
            this->body = "";
            continue;
        }

        // fill request headers
        for (int ch = 0; ch < ref_data.at(ln).size(); ++ch)
        {
        }

        // m_header[
    }

    return 0;
}

#endif
