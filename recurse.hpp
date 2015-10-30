#ifndef RECURSE_HPP
#define RECURSE_HPP

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
//! \brief The Recurse class
//! main class of the app
//!
class Recurse : public QObject
{
    using void_f = std::function<void()>;
    using void_ff = std::function<void(void_f prev)>;
    using next_prev_f = std::function<void(Context &ctx, void_ff next, void_f prev)>;
    using next_f = std::function<void(Context &ctx, void_f next)>;
    using final_f =  std::function<void(Context &ctx)>;

public:
    Recurse(int & argc, char ** argv, QObject *parent = NULL);
    ~Recurse();

    bool listen(quint64 port, QHostAddress address = QHostAddress::Any);
    void use(next_f next);
    void use(next_prev_f next);

    void use(QVector<next_f> nexts);
    void use(QVector<next_prev_f> nexts);

    void use(final_f next);

private:
    QCoreApplication app;
    QTcpServer m_tcp_server;

    quint64 m_port;
    QVector<next_prev_f> m_middleware_next;

    void m_end(QVector<void_f> *middleware_prev);
    void m_send(Context *ctx);
    void m_next(void_f prev, Context *ctx, int current_middleware, QVector<void_f> *middleware_prev);
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

        auto middleware_prev = new QVector<void_f>;
        auto ctx = new Context;
        ctx->request.socket = m_tcp_server.nextPendingConnection();

        connect(ctx->request.socket, &QTcpSocket::readyRead, [this, ctx, middleware_prev] {

            QString data(ctx->request.socket->readAll());
            qDebug() << "ctx request: " << data;

            ctx->request.parse(data);

            if (ctx->request.length < ctx->request.get("content-length").toLongLong())
                return;

            if (m_middleware_next.count() > 0) {
                ctx->response.end = std::bind(&Recurse::m_end, this, middleware_prev);

                m_middleware_next[0](*ctx, std::bind(&Recurse::m_next, this, std::placeholders::_1, ctx, 0, middleware_prev), std::bind(&Recurse::m_send, this, ctx));
            }
        });

        connect(ctx->request.socket, &QTcpSocket::disconnected, [this, ctx, middleware_prev]{
            ctx->request.socket->deleteLater();
            delete ctx;
            delete middleware_prev;
        });
    });

    return app.exec();
};

//!
//! \brief Recurse::m_next
//! call next middleware
//!
void Recurse::m_next(void_f prev, Context *ctx, int current_middleware, QVector<void_f> *middleware_prev)
{
    qDebug() << "calling next:" << current_middleware << " num:" << m_middleware_next.size();

    if (++current_middleware >= m_middleware_next.size()) {
        return;
    };

    // save prev function
    if (prev)
        middleware_prev->push_back(prev);

    // call next function with current prev
    m_middleware_next[current_middleware]( *ctx, std::bind(&Recurse::m_next, this,  std::placeholders::_1, ctx, current_middleware, middleware_prev), prev );
};

//!
//! \brief Recurse::use
//! add new middleware
//!
//! \param f middleware function that will be called later
//!
//!
//!
void Recurse::use(next_prev_f f)
{
    m_middleware_next.push_back(f);
};

//!
//! \brief Recurse::use
//! overload function, next middleware only, no upstream
//!
//! \param f
//!
void Recurse::use(next_f f)
{
    m_middleware_next.push_back([f](Context &ctx,  void_ff next, void_f prev) {
        f(ctx, [next, prev]() {
            next([prev](){
                prev();
            });
        });
    });
};

//!
//! \brief Recurse::use
//! overloaded function,
//! add multiple middlewares
//! very useful for third party modules
//!
//! \param f vector of middlewares
//!
void Recurse::use(QVector<next_prev_f> f)
{
    for(const auto &g : f)
        m_middleware_next.push_back(g);
};

//!
//! \brief Recurse::use
//! overloaded function,
//! add multiple middlewares, no upstream
//! \param f
//!
void Recurse::use(QVector<next_f> f)
{
    for(const auto &g : f)
        m_middleware_next.push_back([g](Context &ctx,  void_ff next, void_f prev) {
            g(ctx, [next, prev]() {
                next([prev](){
                    prev();
                });
            });
        });
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
    m_middleware_next.push_back([f](Context &ctx,  void_ff /* next */, void_f /* prev */) {
        f(ctx);
    });
};

//!
//! \brief Recurse::end
//! final function to be called for creating/sending response
//! \param request
//! \param response
//!
void Recurse::m_end(QVector<void_f> *middleware_prev)
{
    qDebug() << "start upstream";

    void_f prev_f = middleware_prev->at(middleware_prev->size()-1);
    prev_f();
};

//!
//! \brief Recurse::m_send
//! used as last middleware (upstream) to be called
//! sends response to client
//! \param ctx
//!
void Recurse::m_send(Context *ctx)
{
    qDebug() << "end upstream";

    auto request = ctx->request;
    auto response = ctx->response;

    response.method = request.method;
    response.protocol = request.protocol;

    QString reply = response.create_reply();

    // send response to the client
    request.socket->write(reply.toUtf8());

    request.socket->disconnectFromHost();
};

#endif // RECURSE_HPP
