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
//! \brief The RSslServer class
//! Recurse ssl server implementation used for Recurse::HttpsServer
//!
class RSslServer : public QTcpServer
{
    Q_OBJECT
    Q_DISABLE_COPY(RSslServer)

public:
    RSslServer(QObject *parent = NULL);
    ~RSslServer();

};

inline RSslServer::RSslServer(QObject *parent)
{
    Q_UNUSED(parent);
};

inline RSslServer::~RSslServer()
{

};

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

signals:
    void socketReady(QTcpSocket *socket);
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
        QTcpSocket *socket = m_tcp_server.nextPendingConnection();

        if (socket == 0) {
            qDebug() << "no connection?";
            delete socket;
            return;
        }

        emit socketReady(socket);
    });

    return true;
};

//!
//! \brief The HttpsServer class
//! Https (secure) server class
//!
class HttpsServer : public QObject
{
    Q_OBJECT

public:
    HttpsServer(QObject *parent = NULL);
    ~HttpsServer();

    bool compose(quint64 port, QHostAddress address = QHostAddress::Any);

private:
    RSslServer m_tcp_server;
    quint64 m_port;
    QHostAddress m_address;
    QObject *m_parent;

signals:
    void socketReady(QTcpSocket *socket);
};

inline HttpsServer::HttpsServer(QObject *parent)
{
    m_parent = parent;
};

inline HttpsServer::~HttpsServer()
{

};

inline bool HttpsServer::compose(quint64 port, QHostAddress address)
{
    m_port = port;
    m_address = address;

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
    bool listen(HttpsServer *server);

    void use(next_f next);
    void use(next_prev_f next);

    void use(QVector<next_f> nexts);
    void use(QVector<next_prev_f> nexts);

    void use(final_f next);

public slots:
    bool handleConnection(QTcpSocket *socket);

private:
    QCoreApplication app;
    HttpServer *http;
    HttpsServer *https;

    QVector<next_prev_f> m_middleware_next;

    void m_end(QVector<void_f> *middleware_prev);
    void m_send(Context *ctx);
    void m_next(void_f prev, Context *ctx, int current_middleware, QVector<void_f> *middleware_prev);
    bool startEventLoop();
};

inline Recurse::Recurse(int & argc, char ** argv, QObject *parent) : app(argc, argv)
{
    Q_UNUSED(parent);
};

inline Recurse::~Recurse()
{
    delete http;
    delete https;
};

//!
//! \brief Recurse::startEventLoop
//! starts a Qt build-in event loop
//!
//! \return true on success
//!
inline bool Recurse::startEventLoop()
{
    return app.exec();
};

//!
//! \brief Recurse::handleConnection
//! creates new recurse context for a tcp session
//!
//! \return true on success
//!
inline bool Recurse::handleConnection(QTcpSocket *socket)
{
    qDebug() << "handling new connection";

    auto middleware_prev = new QVector<void_f>;
    auto ctx = new Context;
    ctx->request.socket = socket;

    connect(socket, &QTcpSocket::readyRead, [this, ctx, middleware_prev] {
        QString data(ctx->request.socket->readAll());

        ctx->request.parse(data);

        if (ctx->request.length < ctx->request.get("content-length").toLongLong()) {
            return;
        }

        qDebug() << ctx->request.body;

        /*
        if (m_middleware_next.count() > 0) {
            ctx->response.end = std::bind(&Recurse::m_end, this, middleware_prev);

            m_middleware_next[0](
                *ctx,
                std::bind(&Recurse::m_next, this, std::placeholders::_1, ctx, 0, middleware_prev),
                std::bind(&Recurse::m_send, this, ctx));
        }
        */

    });

    return true;
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
    // set HttpServer instance, send reference to this object and prepare http connection
    http = new HttpServer(this);
    http->compose(port, address);

    // connect HttpServer signal 'socketReady' to this class' 'handleConnection' slot
    connect(http, &HttpServer::socketReady, this, &Recurse::handleConnection);
    return startEventLoop();
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

    // connect HttpServer signal 'socketReady' to this class' 'handleConnection' slot
    connect(http, &HttpServer::socketReady, this, &Recurse::handleConnection);
    return startEventLoop();
};

//!
//! \brief Recurse::listen
//! listen for tcp requests
//!
//! overloaded function
//!
//! \param server pointer to the HttpsServer instance
//!
//! \return true on success
//!
inline bool Recurse::listen(HttpsServer *server)
{
    https = server;

    // connect HttpServer signal 'socketReady' to this class' 'handleConnection' slot
    // connect(http, &HttpServer::socketReady, this, &Recurse::handleConnection);
    return startEventLoop();
};

