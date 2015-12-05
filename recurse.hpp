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
    };

private:
    QSslConfiguration m_ssl_configuration;
};

inline SslTcpServer::SslTcpServer(QObject *parent)
{
    Q_UNUSED(parent);
};

inline SslTcpServer::~SslTcpServer()
{

};

//!
//! \brief SslTcpServer::setSslConfiguration
//! set ssl socket configuration
//!
//! \param sslConfiguration ssl socket configuration
//!
inline void SslTcpServer::setSslConfiguration(const QSslConfiguration &sslConfiguration)
{
    m_ssl_configuration = sslConfiguration;
};

inline QSslSocket *SslTcpServer::nextPendingConnection()
{
    return static_cast<QSslSocket *>(QTcpServer::nextPendingConnection());
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

    bool compose(quint16 port, QHostAddress address = QHostAddress::Any);

private:
    QTcpServer m_tcp_server;
    quint16 m_port;
    QHostAddress m_address;
    QObject *m_parent;

signals:
    void socketReady(QTcpSocket *socket);
};

inline HttpServer::HttpServer(QObject *parent)
{
    // FIXME: is this necessary? it's unused
    m_parent = parent;
};

inline HttpServer::~HttpServer()
{

};

//!
//! \brief HttpServer::compose
//! prepare http server for request forwarding
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
//! \return true on success
//!
inline bool HttpServer::compose(quint16 port, QHostAddress address)
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

    bool compose(quint16 port, QHostAddress address = QHostAddress::Any);
    bool compose(const QHash<QString, QVariant> &options);

private:
    SslTcpServer m_tcp_server;
    quint16 m_port;
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
    delete m_parent;
};

//!
//! \brief HttpsServer::compose
//! prepare https server for request forwarding
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
//! \return true on success
//!
inline bool HttpsServer::compose(quint16 port, QHostAddress address)
{
    m_port = port;
    m_address = address;

    if (!m_tcp_server.listen(address, port)) {
        qDebug() << "failed to start listening";
        return false;
    }

    connect(&m_tcp_server, &SslTcpServer::connectionEncrypted, [this] {
        qDebug() << "secured connection ready";

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
//! \brief HttpsServer::compose
//! overloaded function,
//! prepare https server for request forwarding
//!
//! \param options QHash options of <QString, QVariant>
//!
//! \return true on success
//!
inline bool HttpsServer::compose(const QHash<QString, QVariant> &options)
{
    QByteArray priv_key;
    QFile priv_key_file(options.value("private_key").toString());

    if (priv_key_file.open(QIODevice::ReadOnly)) {
        priv_key = priv_key_file.readAll();
        priv_key_file.close();
    }

    QSslKey ssl_key(priv_key, QSsl::Rsa);

    QByteArray cert_key;
    QFile cert_key_file(options.value("certificate").toString());

    if (cert_key_file.open(QIODevice::ReadOnly)) {
        cert_key = cert_key_file.readAll();
        cert_key_file.close();
    }

    QSslCertificate ssl_cert(cert_key);

    QSslConfiguration ssl_configuration;
    ssl_configuration.setPrivateKey(ssl_key);
    ssl_configuration.setLocalCertificate(ssl_cert);

    m_tcp_server.setSslConfiguration(ssl_configuration);

    // TODO: run server and set slots

    if (!options.contains("port")) {
        // FIXME: return something else?
        return false;
    }

    m_port = options.value("port").toUInt();

    if (!options.contains("host")) {
        m_address = QHostAddress::LocalHost;
    } else {
        m_address = QHostAddress(options.value("host").toString());
    }

    return compose(m_port, m_address);
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

    bool http_server(quint16 port, QHostAddress address = QHostAddress::Any);
    bool http_server(const QHash<QString, QVariant> &options);
    bool https_server(const QHash<QString, QVariant> &options);

    bool listen(quint16 port, QHostAddress address = QHostAddress::Any);
    bool listen();

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
    bool m_http_set = false;
    bool m_https_set = false;
    quint16 m_http_port;
    QHostAddress m_http_address;
    const QHash<QString, QVariant> *m_https_options;

    void m_end(QVector<void_f> *middleware_prev);
    void m_send(Context *ctx);
    void m_next(void_f prev, Context *ctx, int current_middleware, QVector<void_f> *middleware_prev);
};

inline Recurse::Recurse(int & argc, char ** argv, QObject *parent) : app(argc, argv)
{
    Q_UNUSED(parent);
};

inline Recurse::~Recurse()
{
    delete http;
    delete https;
    delete m_https_options;
};

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
    auto ctx = new Context;
    ctx->request.socket = socket;

    connect(socket, &QTcpSocket::readyRead, [this, ctx, middleware_prev] {
        QString data(ctx->request.socket->readAll());

        ctx->request.parse(data);

        if (ctx->request.length < ctx->request.get("content-length").toLongLong()) {
            return;
        }

        qDebug() << ctx->request.body;

        // TODO: copy middleware implementation from master branch
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
//! \brief Recurse::http_server
//! http server initialization
//!
//! \param port tcp server port
//! \param address tcp server listening address
//!
//! \return true on success
//!
inline bool Recurse::http_server(quint16 port, QHostAddress address)
{
    http = new HttpServer(this);

    m_http_port = port;
    m_http_address = address;
    m_http_set = true;

    return true;
};

//!
//! \brief Recurse::http_server
//! overloaded function,
//! http server initialization
//!
//! \param options QHash options of <QString, QVariant>
//!
//! \return true on success
//!
inline bool Recurse::http_server(const QHash<QString, QVariant> &options)
{
    if (!options.contains("port")) {
        // FIXME: return an appropriate error
        return false;
    }

    if (options.contains("host")) {
        return http_server(
            options.value("port").toUInt(),
            QHostAddress(options.value("host").toString()));
    }

    return http_server(options.value("port").toUInt());
};

//!
//! \brief Recurse::https_server
//! https (secure) server initialization
//!
//! \param options QHash options of <QString, QVariant>
//!
//! \return true on success
//!
inline bool Recurse::https_server(const QHash<QString, QVariant> &options)
{
    if (!options.contains("port")) {
        // FIXME: return an appropriate error
        return false;
    }

    https = new HttpsServer(this);

    m_https_options = &options;
    m_https_set = true;

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
inline bool Recurse::listen(quint16 port, QHostAddress address)
{
    // if this function is called and m_http_set is true, ignore port
    if (m_http_set) {
        port = m_http_port
    }

    // if this function is called and m_http_set is false
    // set HttpServer instance, send reference to this object and prepare http connection
    http = new HttpServer(this);
    http->compose(port, address);

    // connect HttpServer signal 'socketReady' to this class' 'handleConnection' slot
    connect(http, &HttpServer::socketReady, this, &Recurse::handleConnection);

    return app.exec();
};

//!
//! \brief Recurse::listen
//! overloaded function,
//! listen for tcp requests
//!
//! \return true on success
//!
inline bool Recurse::listen()
{
    if (m_http_set) {
        http->compose(m_http_port, m_http_address);
        connect(http, &HttpServer::socketReady, this, &Recurse::handleConnection);
    }

    if (m_https_set) {
        https->compose(*m_https_options);
        connect(https, &HttpsServer::socketReady, this, &Recurse::handleConnection);
    }

    if (!m_http_set && !m_https_set) {
        return listen(0);
    }

    return app.exec();
};

