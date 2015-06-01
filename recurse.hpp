#ifndef RECURSE_HPP
#define RECURSE_HPP

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHash>
#include <QVector>

#include <functional>
using std::function;
using std::bind;

struct Request {
    // tpc request data
    QString data;

    // higher level data
    QHash<QString, QString> headers;
    QHash<QString, QString> body;
};

typedef function<void(Request &request, QString &response,  function<void()> next)> next_f;

//!
//! \brief The Recurse class
//! main class of the app
//!
class Recurse : public QObject
{
public:

    Recurse(QObject *parent = NULL);
    ~Recurse();

    bool listen(quint64 port, QHostAddress address = QHostAddress::Any);
    void use(next_f next);

private:
    QTcpServer m_tcp_server;
    quint64 m_port;
    QVector<next_f> m_middleware;
    int current_middleware = 0;
    void m_next();

    Request request;
    QString response;
};

Recurse::Recurse(QObject *parent)
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
            request.data = client->readAll();

            qDebug() << "client request: " << request.body["data"];

            if (m_middleware.count() > 0)
                m_middleware[current_middleware](request, response, bind(&Recurse::m_next, this));

            qDebug() << "middleware end";
            current_middleware = 0;

            // handle response
            client->write(response.toStdString().c_str(), response.size());
        });
    });

    return true;
};

//!
//! \brief Recurse::m_next
//! call next middleware
//!
void Recurse::m_next()
{
    qDebug() << "calling next:" << current_middleware << " num:" << m_middleware.size();

    if (++current_middleware >= m_middleware.size()) {
        return;
    };

    m_middleware[current_middleware](request, response, bind(&Recurse::m_next, this));

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

#endif // RECURSE_HPP
