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
#include <functional>

#include "request.hpp"
#include "response.hpp"
#include "context.hpp"

//!
//! \brief The Recurse class
//! main class of the app
//!
class Recurse : public QObject
{
    typedef std::function<void()> void_f;
    typedef std::function<void(Context &ctx, void_f next)> next_f;
    typedef std::function<void(Context &ctx)> final_f;

public:
    Recurse(int & argc, char ** argv, QObject *parent = NULL);
    ~Recurse();

    bool listen(quint64 port, QHostAddress address = QHostAddress::Any);
    void use(next_f next);
    void use(QVector<next_f> nexts);
    void use(final_f next);
    void end(Context *ctx);

private:
    QCoreApplication app;
    QTcpServer m_tcp_server;

    quint64 m_port;
    QVector<next_f> m_middleware;

    void m_next(Context *ctx, int current_middleware);
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

        auto ctx= new Context;
        ctx->request.socket = m_tcp_server.nextPendingConnection();

        connect(ctx->request.socket, &QTcpSocket::readyRead, [this, ctx] {

            QString data(ctx->request.socket->readAll());
            qDebug() << "ctx request: " << data;

            ctx->request.parse(data);

            if (ctx->request.length < ctx->request.header["content-length"].toLongLong())
                return;

            if (m_middleware.count() > 0) {
                ctx->response.end = std::bind(&Recurse::end, this, ctx);

                m_middleware[0]( *ctx, std::bind(&Recurse::m_next, this, ctx, 0) );
            }
        });
    });

    return app.exec();
};

//!
//! \brief Recurse::m_next
//! call next middleware
//!
void Recurse::m_next(Context *ctx, int current_middleware)
{
    qDebug() << "calling next:" << current_middleware << " num:" << m_middleware.size();

    if (++current_middleware >= m_middleware.size()) {
        return;
    };

    m_middleware[current_middleware]( *ctx, std::bind(&Recurse::m_next, this, ctx, current_middleware) );
};

//!
//! \brief Recurse::use
//! add new middleware
//!
//! \param f middleware function that will be called later
//!
//!
//!
void Recurse::use(next_f f)
{
    m_middleware.push_back(f);
};

//!
//! \brief Recurse::use
//! overloaded function,
//! add multiple middlewares
//! very useful for third party modules
//!
//! \param f vector of middlewares
//!
void Recurse::use(QVector<next_f> f)
{
    for(const auto &g : f)
        m_middleware.push_back(g);
};

//!
//! \brief Recurse::use
//! overloaded function,
//! final middleware that doesn't call next, used for returning response
//!
//! \param f final middleware function that will be called last
//!
void Recurse::use(final_f f)
{
    m_middleware.push_back([f](Context &ctx,  void_f /* next */) {
        f(ctx);
    });
};

//!
//! \brief Recurse::end
//! final function to be called for creating/sending response
//! \param request
//! \param response
//!
void Recurse::end(Context *ctx)
{
    Request &request = ctx->request;
    Response &response = ctx->response;

    qDebug() << "calling end with:" << response.body();

    response.method = request.method;
    response.protocol = request.protocol;

    QString reply = response.create_reply();

    // send response to the client
    qint64 check = request.socket->write(reply.toUtf8());

    qDebug() << "socket write debug:" << check;
    request.socket->close();

    delete ctx;
};


#endif // RECURSE_HPP
