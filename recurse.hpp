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
    HttpServer(quint64 port, QObject *parent = NULL);
    ~HttpServer();

private:
    QTcpServer m_tcp_server;
    quint64 m_port;
};

inline HttpServer::HttpServer(quint64 port, QObject *parent)
{
    Q_UNUSED(parent);

    m_port = port;
    qDebug() << "test" << m_port;
};

inline HttpServer::~HttpServer()
{

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

private:
    QCoreApplication app;
};

inline Recurse::Recurse(int & argc, char ** argv, QObject *parent) : app(argc, argv)
{
    Q_UNUSED(parent);
};

inline Recurse::~Recurse()
{

};

inline bool Recurse::listen(quint64 port, QHostAddress address)
{
    qDebug() << "listening on port" << port;

    HttpServer srv(2000);

    return true;
};

