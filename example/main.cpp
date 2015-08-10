//
// recurse example usage
//
// build as:
//
// qmake .
// make
//

#include "../recurse.hpp"

#include <QtConcurrent/QtConcurrent>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // Start middleware, logger
    app.use([](auto &req, auto /* &res */, auto next) {
        qDebug() << "received a new request from:" << req.ip;
        next();
    });

    // Second middleware, sets custom data
    app.use([](auto &req, auto /* &res */, auto next) {
        qDebug() << "routed request" << req.header;

        // custom data to be passed around - qvariant types
        req.ctx.set("customdata", "value");

        // for any kind of data use
        // req.ctx.data["key"] = *void

        next();
    });

    // Final middleware, does long running action concurrently and sends reponse as json
    app.use([](auto &req, auto &res) {
        // show header and our custom data
        qDebug() << "last route" << req.header << " " << req.ctx.get("customdata");

        // set custom header
        res.header["x-foo-data"] = "tvmid";

        // these are already default values
        // text/plain will be overriden by send if json is wanted
        res.status(200).type("text/plain");

        // some long running action in new thread pool
        auto future = QtConcurrent::run([]{
           qDebug() << "long running action...";

           // return our demo result
           return QJsonDocument::fromJson("{\"hello\" : \"world\"}");
        });

        auto watcher = new QFutureWatcher<QJsonDocument>;
        QObject::connect(watcher, &QFutureWatcher<QJsonDocument>::finished,[&res, future]() {
            qDebug() << "long running action done";

            // send our demo result
            res.send(future.result());
        });
        watcher->setFuture(future);

    });

    qDebug() << "listening on port 3000...";
    app.listen(3000);
}
