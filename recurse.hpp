#include <QCoreApplication>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QStringBuilder>
#include <QVector>
#include <functional>

#include "request.hpp"
#include "response.hpp"
#include "context.hpp"

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
    QObject *m_parent;
};

inline HttpServer::HttpServer(QObject *parent)
{
    m_parent = parent;
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
        qDebug() << "client connected";

        // TODO: create ctx (need parent's ref here?)
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

    using void_f = std::function<void()>;
    using void_ff = std::function<void(void_f prev)>;
    using next_prev_f = std::function<void(Context &ctx, void_ff next, void_f prev)>;
    using next_f = std::function<void(Context &ctx, void_f next)>;
    using final_f =  std::function<void(Context &ctx)>;

public:
    Recurse(int & argc, char ** argv, QObject *parent = NULL);
    ~Recurse();

    bool listen(quint64 port, QHostAddress address = QHostAddress::Any);
    bool listen(HttpServer *server);
    void tester();

private:
    QCoreApplication app;
    HttpServer *http;
};

inline Recurse::Recurse(int & argc, char ** argv, QObject *parent) : app(argc, argv)
{
    Q_UNUSED(parent);
};

inline Recurse::~Recurse()
{
    delete http;
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
    http = new HttpServer(this);
    http->compose(port, address);

    return app.exec();
};

//!
//! \brief Recurse::listen
//! listen for tcp requests
//!
//! overloaded function
//!
//! \param server pointer to the HttpServer instance
//!
//! \return true on success
//!
inline bool Recurse::listen(HttpServer *server)
{
    http = server;

    return app.exec();
};


inline void Recurse::tester()
{
    qDebug() << "test function";
};
