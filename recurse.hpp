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
using std::function;
using std::bind;
using std::ref;

struct Request {
    // tcp request data
    QString data;

    // higher level data
    QHash<QString, QString> header;
    QString body, method, proto, url;
};

struct Response {
    QString body;
    QHash<QString, QString> header;
    unsigned short int status;
    QString method, proto;
};

typedef function<void(Request &request, Response &response, function<void()> next)> next_f;

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
    void m_next(Request &request, Response &response);
    void http_parse(Request &request);
    void http_build_header(Response &response);
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
            Request request;
            Response response;

            request.data = client->readAll();
            QRegExp httpRx("^(?=[A-Z]).* \\/.* HTTP\\/[0-9]\\.[0-9]\\r\\n");
            bool isHttp = request.data.contains(httpRx);

            if (isHttp)
                http_parse(request);

            qDebug() << "client request: " << request.data;

            if (m_middleware.count() > 0)
                m_middleware[current_middleware](request, response, bind(&Recurse::m_next, this, ref(request), ref(response)));

            qDebug() << "middleware end; resp body:" << response.body;
            current_middleware = 0;

            if (isHttp)
                response.method = request.method;
                response.proto = request.proto;
                http_build_header(response);

            // send response to the client
            client->write(response.body.toStdString().c_str(), response.body.size());
        });
    });

    return app.exec();
};

//!
//! \brief Recurse::m_next
//! call next middleware
//!
void Recurse::m_next(Request &request, Response &response)
{
    qDebug() << "calling next:" << current_middleware << " num:" << m_middleware.size();

    if (++current_middleware >= m_middleware.size()) {
        return;
    };

    m_middleware[current_middleware](request, response, bind(&Recurse::m_next, this, ref(request), ref(response)));

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
//! \brief Recurse::http_parse
//! parse http data
//!
//! \param data reference to data received from the tcp connection
//!
void Recurse::http_parse(Request &request)
{
    QStringList data_list = request.data.split("\r\n");
    bool is_body = false;

    for (int i = 0; i < data_list.size(); ++i) {
        QStringList item_list = data_list.at(i).split(":");

        if (item_list.length() < 2 && item_list.at(0).size() < 1 && !is_body) {
            is_body = true;
            continue;
        }
        else if (i == 0 && item_list.length() < 2) {
            QStringList first_line = item_list.at(0).split(" ");
            request.method = first_line.at(0);
            request.url = first_line.at(1).trimmed();
            request.proto = first_line.at(2).trimmed();
        }
        else if (!is_body) {
            request.header[item_list.at(0).toLower()] = item_list.at(1).trimmed();
        }
        else {
            request.body.append(item_list.at(0));
        }
    }

    qDebug() << "request ctx ready: " << request.method << request.url << request.header << request.proto << request.body;
};

//!
//! \brief Recurse::http_build_header
//! build http header for response
//!
//! \param response reference to the Response instance
//!
void Recurse::http_build_header(Response &response)
{
    // qDebug() << "http_build_header:" << response.status;
    if (response.status == 0)
        response.status = 204;

    QString header = response.proto % " " % QString::number(response.status) % " OK\r\n";

    qDebug() << "header" << header;
}

#endif // RECURSE_HPP
