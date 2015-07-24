#ifndef RECURSE_HPP
#define RECURSE_HPP

#include <QCoreApplication>
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
    QString body;
};

typedef function<void(Request &request, QString &response,  function<void()> next)> next_f;

//!
//! \brief The Recurse class
//! main class of the app
//!
class Recurse : public QObject
{
public:

    Recurse(int & argc, char ** argv, QObject *parent = NULL);
    ~Recurse();

    bool listen(quint64 port, QHostAddress address = QHostAddress::Any);
    void use(next_f next);

private:
    QCoreApplication app;
    QTcpServer m_tcp_server;
    quint64 m_port;
    QVector<next_f> m_middleware;
    int current_middleware = 0;
    void m_next();
    void parse_http(QString &data);

    Request request;
    QString response;
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
        QTcpSocket *client = m_tcp_server.nextPendingConnection();

        connect(client, &QTcpSocket::readyRead, [this, client] {
            request.data = client->readAll();
            parse_http(request.data);

            qDebug() << "client request: " << request.data;

            if (m_middleware.count() > 0)
                m_middleware[current_middleware](request, response, bind(&Recurse::m_next, this));

            qDebug() << "middleware end";
            current_middleware = 0;

            // handle response
            client->write(response.toStdString().c_str(), response.size());
        });
    });

    return app.exec();
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

//!
//! \brief Recurse::parse_http
//! parse http data
//!
//! \param data reference to data received from the tcp connection
//!
void Recurse::parse_http(QString &data)
{
    qDebug() << "parser:" << data;
    QStringList data_list = data.split("\r\n");

    qDebug() << "parser after split:" << data_list;
    bool isBody = false;

    for (int i = 0; i < data_list.size(); ++i) {
        QStringList item_list = data_list.at(i).split(":");

        if (item_list.length() < 2 && item_list.at(0).size() < 1 && !isBody) {
            isBody = true;
            continue;
        }
        else if (i == 0 && item_list.length() < 2) {
            request.headers["method"] = item_list.at(0);
        }
        else if (!isBody) {
            qDebug() << "header: " << item_list.at(0);
            request.headers[item_list.at(0)] = item_list.at(1);
        }
        else {
            request.body.append(item_list.at(0));
        }

        qDebug() << item_list;
    }

    qDebug() << "headers ready: " << request.headers;
    qDebug() << "body ready: " << request.body;
};

#endif // RECURSE_HPP
