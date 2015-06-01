//
// recurse example usage
//
// build as:
//
// qmake .
// make
//

#include <QCoreApplication>

#include "../recurse.hpp"
Recurse app;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // middleware1: logger
    app.use([](auto req, auto res, auto next) {

        qDebug() << req.body;

        next();

        qDebug() << "outgoing response:" << res;
    });

    // middleware2: hello world
    app.use([](auto req, auto &res, auto next) {
       res = req.data.trimmed() +  " hello world\n";
    });

    app.listen(3000);

    return a.exec();
}
