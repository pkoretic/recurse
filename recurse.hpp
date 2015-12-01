#include <QCoreApplication>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QStringBuilder>
#include <QVector>
#include <functional>

class Recurse : public QObject
{
    Q_OBJECT

public:
    Recurse(int & argc, char ** argv, QObject *parent = NULL);
    ~Recurse();

    bool listen();

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

inline bool Recurse::listen()
{
    return true;
};

class HttpServer : public QObject
{
    Q_OBJECT

public:
    HttpServer(quint64 port, QObject *parent = NULL);
    ~HttpServer();

private:
    QTcpServer m_tcp_server;
};

inline HttpServer::HttpServer(quint64 port, QObject *parent)
{
    Q_UNUSED(parent);

    qDebug() << "test" << port;
};

inline HttpServer::~HttpServer()
{

};
