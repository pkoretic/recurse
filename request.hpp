#ifndef RECURSE_REQUEST_HPP
#define RECURSE_REQUEST_HPP

#include <QTcpSocket>
#include <QHash>

class Request {

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
    //! Shorthand for protocol == "HTTPS" to check if a requet was issued via TLS
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
    //! \return true on success, false otherwise, considered bad request
    //!
    bool parse(QString request);

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


inline bool Request::parse(QString request)
{
    // buffer all data
    this->data += request;

    // Save client ip address
    this->ip = this->socket->peerAddress();

    // if no header is present, just append all data to request.body
    if (!this->data.contains(httpRx)) {
        this->body.append(this->data);
        return true;
    }

    QStringList data_list = this->data.split("\r\n");
    bool is_body = false;

    for (int i = 0; i < data_list.size(); ++i) {
        if (is_body) {
            this->body.append(data_list.at(i));
            this->length += this->body.size();
            continue;
        }

        QStringList entity_item = data_list.at(i).split(":");

        if (entity_item.length() < 2 && entity_item.at(0).size() < 1 && !is_body) {
            is_body = true;
            continue;
        }
        else if (i == 0 && entity_item.length() < 2) {
            QStringList first_line = entity_item.at(0).split(" ");
            this->method = first_line.at(0);
            this->url = first_line.at(1).trimmed();
            this->protocol = first_line.at(2).trimmed();
            continue;
        }

        m_header[entity_item.at(0).toLower()] = entity_item.at(1).trimmed().toLower();
    }

    if (m_header.contains("host"))
        this->hostname = m_header["host"];

    qDebug() << "this->object populated: "
        << this->method << this->url << m_header << this->protocol << this->body
        << this->hostname << this->ip
        << this->length;

    // extract cookies
    // eg: USER_TOKEN=Yes;test=val
    if (m_header.contains("cookie")) {
        for(const QString &cookie : this->get("cookie").split(";")) {
            int split = cookie.trimmed().indexOf("=");
            if (split == -1)
                continue;

            QString key = cookie.left(split).trimmed();
            if (!key.size())
                continue;

            QString value = cookie.mid(split + 1).trimmed();

            this->cookies[key.toLower()] = value.toLower();
        }

        qDebug() << "cookies:\n" << this->cookies << "\n";
    }

    return true;
}

#endif
