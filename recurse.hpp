#include <QCoreApplication>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QStringBuilder>
#include <QVector>
#include <functional>

//!
//! \brief The HttpServer class
//! Http (unsecure) server class
//!
class HttpServer : public QObject
{
    Q_OBJECT

public:
    HttpServer(QObject *parent = NULL);
    ~HttpServer();

    bool compose(quint64 port, QHostAddress address = QHostAddress::Any);

private:
    QTcpServer m_tcp_server;
    quint64 m_port;
    QHostAddress m_address;
};

inline HttpServer::HttpServer(QObject *parent)
{
    Q_UNUSED(parent);
};

inline HttpServer::~HttpServer()
{

};

inline bool HttpServer::compose(quint64 port, QHostAddress address)
{
    m_port = port;
    m_address = address;

    if (!m_tcp_server.listen(address, port)) {
        qDebug() << "failed to start listening";
        return false;
    }

    connect(&m_tcp_server, &QTcpServer::newConnection, [this] {
        qDebug() << "connection!";
    });

    return true;
};

//!
//! \brief The Recurse class
//! main class of the app
//!
class Recurse : public QObject
{
    Q_OBJECT

public:
    Recurse(int & argc, char ** argv, QObject *parent = NULL);
    ~Recurse();

    bool listen(quint64 port, QHostAddress address = QHostAddress::Any);
    bool listen(HttpServer *server);

private:
    QCoreApplication app;
    HttpServer *srv;
};

inline Recurse::Recurse(int & argc, char ** argv, QObject *parent) : app(argc, argv)
{
    Q_UNUSED(parent);
};

inline Recurse::~Recurse()
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
inline bool Recurse::listen(quint64 port, QHostAddress address)
{
    srv->compose(port, address);

    return app.exec();
};

//!
//! \brief Recurse::listen
//! listen for tcp requests
//!
//! Overloaded function
//!
//! \param server pointer to the HttpServer instance
//!
//! \return true on success
//!
inline bool Recurse::listen(HttpServer *server)
{
    srv = server;
    return true;
};

