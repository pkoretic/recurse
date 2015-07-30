#ifndef RECURSE_HPP
#define RECURSE_HPP

#include <QCoreApplication>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHash>
#include <QStringBuilder>
#include <QVector>
#include "request.hpp"
#include "response.hpp"

#include <functional>
using std::function;
using std::bind;
using std::ref;

typedef function<void(Request &request, Response &response, function<void()> next)> next_f;

//!
//! \brief The Recurse class
//! main class of the app
//!
class Recurse : public QObject
{
public:

    Recurse(int & argc, char ** argv, QObject *parent = NULL);
    ~Recurse();

    bool listen(quint64 port, QHostAddress address = QHostAddress::Any);
    void use(next_f next);

private:
    QCoreApplication app;
    QTcpServer m_tcp_server;
    quint64 m_port;
    QVector<next_f> m_middleware;
    int current_middleware = 0;
    void m_next(Request &request, Response &response);
    void http_parse(Request &request);
    QString http_build_header(const Response &response);
};

Recurse::Recurse(int & argc, char ** argv, QObject *parent) : app(argc, argv)
{
    Q_UNUSED(parent);
};

Recurse::~Recurse()
{

};

//!
//! \brief Recurse::listen
//! listen for tcp requests
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
//! \return true on success
//!
bool Recurse::listen(quint64 port, QHostAddress address)
{
    m_port = port;
    int bound = m_tcp_server.listen(address, port);
    if (!bound)
        return false;

    connect(&m_tcp_server, &QTcpServer::newConnection, [this] {
        qDebug() << "client connected";
        QTcpSocket *client = m_tcp_server.nextPendingConnection();

        connect(client, &QTcpSocket::readyRead, [this, client] {
            Request request;
            Response response;

            request.data = client->readAll();
            http_parse(request);

            qDebug() << "client request: " << request.data;

            if (m_middleware.count() > 0)
                m_middleware[current_middleware](request, response, bind(&Recurse::m_next, this, ref(request), ref(response)));

            current_middleware = 0;
            QString header;

            response.method = request.method;
            response.proto = request.proto;

            if (response.status == 0)
                response.status = 200;

            header = http_build_header(response);
            QString response_data = header + response.body;

            // send response to the client
            qint64 check = client->write(response_data.toStdString().c_str(), response_data.size());

            qDebug() << "socket write debug:" << check;
            client->close();
        });
    });

    return app.exec();
};

//!
//! \brief Recurse::m_next
//! call next middleware
//!
void Recurse::m_next(Request &request, Response &response)
{
    qDebug() << "calling next:" << current_middleware << " num:" << m_middleware.size();

    if (++current_middleware >= m_middleware.size()) {
        return;
    };

    m_middleware[current_middleware](request, response, bind(&Recurse::m_next, this, ref(request), ref(response)));

};

//!
//! \brief Recurse::use
//! add new middleware
//!
//! \param f middleware function that will be called later
//!
void Recurse::use(next_f f)
{
    m_middleware.push_back(f);
};

//!
//! \brief Recurse::http_parse
//! parse http data
//!
//! \param data reference to data received from the tcp connection
//!
void Recurse::http_parse(Request &request)
{
    QStringList data_list = request.data.split("\r\n");
    bool is_body = false;

    for (int i = 0; i < data_list.size(); ++i) {
        if (is_body) {
            request.body.append(data_list.at(i));
            continue;
        }

        QStringList entity_item = data_list.at(i).split(":");

        if (entity_item.length() < 2 && entity_item.at(0).size() < 1 && !is_body) {
            is_body = true;
            continue;
        }
        else if (i == 0 && entity_item.length() < 2) {
            QStringList first_line = entity_item.at(0).split(" ");
            request.method = first_line.at(0);
            request.url = first_line.at(1).trimmed();
            request.proto = first_line.at(2).trimmed();
            continue;
        }

        request.header[entity_item.at(0).toLower()] = entity_item.at(1).trimmed();
    }

    qDebug() << "request object populated: " << request.method << request.url << request.header << request.proto << request.body;
};

//!
//! \brief Recurse::http_build_header
//! build http header for response
//!
//! \param response reference to the Response instance
//!
QString Recurse::http_build_header(const Response &response)
{
    QString header = response.proto % " " % QString::number(response.status) % " "
        % response.http_codes[response.status] % "\r\n";

    // set default header fields
    QHash<QString, QString>::const_iterator i;

    for (i = response.default_headers.constBegin(); i != response.default_headers.constEnd(); ++i) {
        if (i.key() == "content-length" && response.header[i.key()] == "")
            header += i.key() % ": " % QString::number(response.body.size()) % "\r\n";
        else if (response.header[i.key()] == "")
            header += i.key() % ": " % i.value() % "\r\n";
    }

    // set user-defined header fields
    QHash<QString, QString>::const_iterator j;

    for (j = response.header.constBegin(); j != response.header.constEnd(); ++j) {
        header += j.key() % ": " % j.value() % "\r\n";
    }

    qDebug() << "response header" << header;

    return header + "\r\n";
}

#endif // RECURSE_HPP
