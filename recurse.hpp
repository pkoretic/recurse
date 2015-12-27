#include <QCoreApplication>
#include <QFile>
#include <QHostAddress>
#include <QObject>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QStringBuilder>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <functional>

#include "request.hpp"
#include "response.hpp"
#include "context.hpp"

//!
//! \brief The Returns class
//! Generic exit code and response value returning class
//!
class Returns
{
private:
    quint16 m_last_error = 0;
    QString m_result;

    QHash<quint16, QString> codes
    {
        {100, "Failed to start listening on port"},
        {101, "No pending connections available"},
        {200, "Generic app.exec() error"},
        {201, "Another generic app.exec() error"},
        {301, "SSL private key open error"},
        {302, "SSL certificate open error"}
    };

public:
    QString lastError()
    {
        if (m_last_error == 0) {
            return "No error";
        } else {
            return codes[m_last_error];
        }
    }

    void setErrorCode(quint16 error_code)
    {
        m_last_error = error_code;
    }

    quint16 errorCode()
    {
        return m_last_error;
    }

    bool error()
    {
        if (m_last_error == 0) {
            return false;
        } else {
            return true;
        }
    }
};

//!
//! \brief The SslTcpServer class
//! Recurse ssl server implementation used for Recurse::HttpsServer
//!
class SslTcpServer : public QTcpServer
{
    Q_OBJECT
    Q_DISABLE_COPY(SslTcpServer)

    typedef void (QSslSocket::* RSslErrors)(const QList<QSslError> &);

public:
    SslTcpServer(QObject *parent = NULL);
    ~SslTcpServer();

    QSslSocket *nextPendingConnection();
    void setSslConfiguration(const QSslConfiguration &sslConfiguration);

Q_SIGNALS:
    void connectionEncrypted();
    void sslErrors(const QList<QSslError> &errors);
    void peerVerifyError(const QSslError &error);

protected:
    //!
    //! \brief overridden incomingConnection from QTcpServer
    //!
    virtual void incomingConnection(qintptr socket_descriptor)
    {
        QSslSocket *socket = new QSslSocket();

        socket->setSslConfiguration(m_ssl_configuration);
        socket->setSocketDescriptor(socket_descriptor);

        connect(socket, &QSslSocket::encrypted, this, &SslTcpServer::connectionEncrypted);
        connect(socket, static_cast<RSslErrors>(&QSslSocket::sslErrors), this, &SslTcpServer::sslErrors);
        connect(socket, &QSslSocket::peerVerifyError, this, &SslTcpServer::peerVerifyError);

        addPendingConnection(socket);
        socket->startServerEncryption();
    }

private:
    QSslConfiguration m_ssl_configuration;
};

inline SslTcpServer::SslTcpServer(QObject *parent)
{
    Q_UNUSED(parent);
}

inline SslTcpServer::~SslTcpServer()
{

}

//!
//! \brief SslTcpServer::setSslConfiguration
//! set ssl socket configuration
//!
//! \param sslConfiguration ssl socket configuration
//!
inline void SslTcpServer::setSslConfiguration(const QSslConfiguration &sslConfiguration)
{
    m_ssl_configuration = sslConfiguration;
}

inline QSslSocket *SslTcpServer::nextPendingConnection()
{
    return static_cast<QSslSocket *>(QTcpServer::nextPendingConnection());
}

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

    Returns compose(quint16 port, QHostAddress address = QHostAddress::Any);

private:
    QTcpServer m_tcp_server;
    quint16 m_port;
    QHostAddress m_address;
    QObject *m_parent;
    Returns ret;

signals:
    void socketReady(QTcpSocket *socket);
};

inline HttpServer::HttpServer(QObject *parent)
{
    // FIXME: is this necessary? it's unused
    m_parent = parent;
}

inline HttpServer::~HttpServer()
{

}

//!
//! \brief HttpServer::compose
//! prepare http server for request forwarding
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
inline Returns HttpServer::compose(quint16 port, QHostAddress address)
{
    m_port = port;
    m_address = address;

    if (!m_tcp_server.listen(address, port)) {
        qDebug() << "failed to start listening on port";
        ret.setErrorCode(100);
        return ret;
    }

    qDebug() << "(http) started listening on host"
        <<  m_tcp_server.serverAddress()
        << "port" << m_tcp_server.serverPort();

    connect(&m_tcp_server, &QTcpServer::newConnection, [this] {
        qDebug() << "client connected";
        QTcpSocket *socket = m_tcp_server.nextPendingConnection();

        if (socket == 0) {
            qDebug() << "no connection?";
            delete socket;
            // FIXME: send signal instead of only setting an error and
            // erroneously (?) returning
            ret.setErrorCode(101);
            return ret;
        }

        emit socketReady(socket);

        ret.setErrorCode(0);
        return ret;
    });

    ret.setErrorCode(0);
    return ret;
}

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

    Returns compose(quint16 port, QHostAddress address = QHostAddress::Any);
    Returns compose(const QHash<QString, QVariant> &options);

private:
    SslTcpServer m_tcp_server;
    quint16 m_port;
    QHostAddress m_address;
    QObject *m_parent;
    Returns ret;

signals:
    void socketReady(QTcpSocket *socket);
};

inline HttpsServer::HttpsServer(QObject *parent)
{
    m_parent = parent;
}

inline HttpsServer::~HttpsServer()
{
    delete m_parent;
}

//!
//! \brief HttpsServer::compose
//! prepare https server for request forwarding
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
//! \return true on success
//!
inline Returns HttpsServer::compose(quint16 port, QHostAddress address)
{
    m_port = port;
    m_address = address;

    if (!m_tcp_server.listen(address, port)) {
        qDebug() << "failed to start listening on port";
        ret.setErrorCode(100);
        return ret;
    }

    qDebug() << "(https) started listening on host"
        <<  m_tcp_server.serverAddress()
        << "port" << m_tcp_server.serverPort();

    connect(&m_tcp_server, &SslTcpServer::connectionEncrypted, [this] {
        qDebug() << "secured connection ready";

        QTcpSocket *socket = m_tcp_server.nextPendingConnection();

        if (socket == 0) {
            qDebug() << "no pending connections available";
            delete socket;
            // FIXME: send signal instead of throwing
            ret.setErrorCode(101);
            return ret;
        }

        emit socketReady(socket);

        ret.setErrorCode(0);
        return ret;
    });

    ret.setErrorCode(0);
    return ret;
}

//!
//! \brief HttpsServer::compose
//! overloaded function,
//! prepare https server for request forwarding
//!
//! \param options QHash options of <QString, QVariant>
//!
//! \return true on success
//!
inline Returns HttpsServer::compose(const QHash<QString, QVariant> &options)
{
    QByteArray priv_key;
    QFile priv_key_file(options.value("private_key").toString());

    if (!priv_key_file.open(QIODevice::ReadOnly)) {
        qDebug() << "private_key open error:" << priv_key_file.errorString();
        ret.setErrorCode(301);
        return ret;
    }

    priv_key = priv_key_file.readAll();
    priv_key_file.close();

    if (priv_key.isEmpty()) {
        ret.setErrorCode(301);
        return ret;
    }

    QSslKey ssl_key(priv_key, QSsl::Rsa);

    QByteArray cert_key;
    QFile cert_key_file(options.value("certificate").toString());

    if (!cert_key_file.open(QIODevice::ReadOnly)) {
        qDebug() << "certificate open error:" << cert_key_file.errorString();
        ret.setErrorCode(302);
        return ret;
    }

    cert_key = cert_key_file.readAll();
    cert_key_file.close();

    if (cert_key.isEmpty()) {
        ret.setErrorCode(302);
        return ret;
    }

    QSslCertificate ssl_cert(cert_key);

    QSslConfiguration ssl_configuration;
    ssl_configuration.setPrivateKey(ssl_key);
    ssl_configuration.setLocalCertificate(ssl_cert);

    m_tcp_server.setSslConfiguration(ssl_configuration);

    if (!options.contains("port")) {
        m_port = 0;
    } else {
        m_port = options.value("port").toUInt();
    }

    if (!options.contains("host")) {
        m_address = QHostAddress::LocalHost;
    } else {
        m_address = QHostAddress(options.value("host").toString());
    }

    auto r = compose(m_port, m_address);
    if (r.error()) {
        ret.setErrorCode(r.errorCode());
        return ret;
    }

    ret.setErrorCode(0);
    return ret;
}

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

    void http_server(quint16 port, QHostAddress address = QHostAddress::Any);
    void http_server(const QHash<QString, QVariant> &options);
    void https_server(const QHash<QString, QVariant> &options);
    Returns listen(quint16 port, QHostAddress address = QHostAddress::Any);
    Returns listen();

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
    Returns ret;

    QVector<next_prev_f> m_middleware_next;
    bool m_http_set = false;
    bool m_https_set = false;
    quint16 m_http_port;
    QHostAddress m_http_address;
    const QHash<QString, QVariant> *m_https_options;

    void m_start_upstream(Context *ctx, QVector<void_f> *middleware_prev);
    void m_send_response(Context *ctx);
    void m_call_next(void_f prev, Context *ctx, int current_middleware, QVector<void_f> *middleware_prev);

    quint16 appExitHandler(quint16 code);
};

inline Recurse::Recurse(int & argc, char ** argv, QObject *parent) : app(argc, argv)
{
    Q_UNUSED(parent);
}

inline Recurse::~Recurse()
{
    delete http;
    delete https;
    delete m_https_options;
}

//!
//! \brief Recurse::end
//! final function to be called for creating/sending response
//! \param request
//! \param response
//!
inline void Recurse::m_start_upstream(Context *ctx, QVector<void_f> *middleware_prev)
{
    qDebug() << "start upstream:" << middleware_prev->size();

    // if there are no upstream middlewares send response directly
    if(!middleware_prev->size())
        m_send_response(ctx);
    else
        middleware_prev->at(middleware_prev->size()-1)();
}

//!
//! \brief Recurse::m_send_response
//! used as last middleware (upstream) to be called
//! sends response to client
//! \param ctx
//!
inline void Recurse::m_send_response(Context *ctx)
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
}

//!
//! \brief Recurse::m_call_next
//! call next middleware
//!
inline void Recurse::m_call_next(void_f prev, Context *ctx, int current_middleware, QVector<void_f> *middleware_prev)
{
    qDebug() << "calling next:" << current_middleware << " num:" << m_middleware_next.size();

    if(++current_middleware >= m_middleware_next.size()) {
        return;
    }

    // save previous middleware function
    if(prev) {
        middleware_prev->push_back(prev);
    }

    // call next function with current prev
    m_middleware_next[current_middleware]( *ctx, std::bind(&Recurse::m_call_next, this,  std::placeholders::_1, ctx, current_middleware, middleware_prev), prev );
}

//!
//! \brief Recurse::use
//! add new middleware
//!
//! \param f middleware function that will be called later
//!
//!
//!
inline void Recurse::use(next_prev_f f)
{
    m_middleware_next.push_back(f);
}

//!
//! \brief Recurse::use
//! overload function, next middleware only, no upstream
//!
//! \param f
//!
inline void Recurse::use(next_f f)
{
    m_middleware_next.push_back([f](Context &ctx,  void_ff next, void_f prev) {
        f(ctx, [next, prev]() {
            next([prev](){
                prev();
            });
        });
    });
}

//!
//! \brief Recurse::use
//! overloaded function,
//! add multiple middlewares
//! very useful for third party modules
//!
//! \param f vector of middlewares
//!
inline void Recurse::use(QVector<next_prev_f> f)
{
    for(const auto &g : f)
        m_middleware_next.push_back(g);
}

//!
//! \brief Recurse::use
//! overloaded function,
//! add multiple middlewares, no upstream
//! \param f
//!
inline void Recurse::use(QVector<next_f> f)
{
    for(const auto &g : f)
        m_middleware_next.push_back([g](Context &ctx,  void_ff next, void_f prev) {
            g(ctx, [next, prev]() {
                next([prev](){
                    prev();
                });
            });
        });
}

//!
//! \brief Recurse::use
//! overloaded function,
//! final middleware that doesn't call next, used for returning response
//!
//! \param f final middleware function that will be called last
//!
inline void Recurse::use(final_f f)
{
    m_middleware_next.push_back([f](Context &ctx,  void_ff /* next */, void_f /* prev */) {
        f(ctx);
    });
}

//!
//! \brief Recurse::handleConnection
//! creates new recurse context for a tcp session
//!
//! \param pointer to the socket sent from http/https server
//!
//! \return true on success
//!
inline bool Recurse::handleConnection(QTcpSocket *socket)
{
    qDebug() << "handling new connection";

    auto middleware_prev = new QVector<void_f>;
    middleware_prev->reserve(m_middleware_next.count());

    auto ctx = new Context;
    ctx->request.socket = socket;

    connect(socket, &QTcpSocket::readyRead, [this, ctx, middleware_prev] {
        QString data(ctx->request.socket->readAll());

        ctx->request.parse(data);

        if (ctx->request.length < ctx->request.get("content-length").toLongLong()) {
            return;
        }

        if (m_middleware_next.count() > 0) {
            ctx->response.end = std::bind(&Recurse::m_start_upstream, this, ctx, middleware_prev);

            m_middleware_next[0](
                *ctx,
                std::bind(&Recurse::m_call_next, this, std::placeholders::_1, ctx, 0, middleware_prev),
                std::bind(&Recurse::m_send_response, this, ctx));
        }
        else {
        }
    });

    connect(ctx->request.socket, &QTcpSocket::disconnected, [this, ctx, middleware_prev] {
        ctx->request.socket->deleteLater();
        delete ctx;
        delete middleware_prev;
    });

    return true;
}

//!
//! \brief Recurse::appExitHandler
//! acts according to the provided application event loop exit code
//!
//! \param code app.exec()'s exit code
//!
//! \return quint16 error code
//!
inline quint16 Recurse::appExitHandler(quint16 code)
{
    if (code == 1) {
        return 201;
    }

    return 200;
}

//!
//! \brief Recurse::http_server
//! http server initialization
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
inline void Recurse::http_server(quint16 port, QHostAddress address)
{
    http = new HttpServer(this);

    m_http_port = port;
    m_http_address = address;
    m_http_set = true;
}

//!
//! \brief Recurse::http_server
//! overloaded function,
//! http server initialization
//!
//! \param options QHash options of <QString, QVariant>
//!
inline void Recurse::http_server(const QHash<QString, QVariant> &options)
{
    http = new HttpServer(this);

    if (!options.contains("port")) {
        m_http_port = 0;
    } else {
        m_http_port = options.value("port").toUInt();
    }

    if (!options.contains("host")) {
        m_http_address = QHostAddress::Any;
    } else {
        m_http_address = QHostAddress(options.value("host").toString());
    }

    m_http_set = true;
}

//!
//! \brief Recurse::https_server
//! https (secure) server initialization
//!
//! \param options QHash options of <QString, QVariant>
//!
inline void Recurse::https_server(const QHash<QString, QVariant> &options)
{
    https = new HttpsServer(this);

    m_https_options = &options;
    m_https_set = true;
}

//!
//! \brief Recurse::listen
//! listen for tcp requests
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
//! \return true on success
//!
inline Returns Recurse::listen(quint16 port, QHostAddress address)
{
    // if this function is called and m_http_set is true, ignore new values
    if (m_http_set) {
        return listen();
    }

    // if this function is called and m_http_set is false
    // set HttpServer instance, send reference to this object and prepare http connection
    http = new HttpServer(this);
    auto r = http->compose(port, address);

    if (r.error()) {
        ret.setErrorCode(r.errorCode());
        app.exit(1);
        return ret;
    }

    // connect HttpServer signal 'socketReady' to this class' 'handleConnection' slot
    connect(http, &HttpServer::socketReady, this, &Recurse::handleConnection);

    auto ok = app.exec();

    if (!ok) {
        ret.setErrorCode(200);
        app.exit(1);
        return ret;
    }

    ret.setErrorCode(0);
    return ret;
}

//!
//! \brief Recurse::listen
//! overloaded function,
//! listen for tcp requests
//!
//! \return true on success
//!
inline Returns Recurse::listen()
{
    if (m_http_set) {
        auto r = http->compose(m_http_port, m_http_address);
        if (r.error()) {
            ret.setErrorCode(r.errorCode());
            app.exit(1);
            return ret;
        }

        connect(http, &HttpServer::socketReady, this, &Recurse::handleConnection);
    }

    if (m_https_set) {
        auto r = https->compose(*m_https_options);
        if (r.error()) {
            ret.setErrorCode(r.errorCode());
            app.exit(1);
            return ret;
        }

        connect(https, &HttpsServer::socketReady, this, &Recurse::handleConnection);
    }

    if (!m_http_set && !m_https_set) {
        return listen(0);
    }

    auto exit_code = app.exec();

    if (exit_code != 0) {
        // TODO: set error code according to app.quit() or app.exit() method's code
        ret.setErrorCode(appExitHandler(exit_code));
        return ret;
    }

    ret.setErrorCode(0);
    return ret;
}

