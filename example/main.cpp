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
    app.use([](auto &ctx, auto next) {
        qDebug() << "received a new request from:" << ctx.request.ip;
        next();
    });

    // Second middleware, sets custom data
    app.use([](auto &ctx, auto next) {
        qDebug() << "routed request" << ctx.request.header;

        // custom data to be passed around - qvariant types
        ctx.set("customdata", "value");

        // for any kind of data use
        // ctx.data["key"] = *void

        next();
    });

    // Final middleware, does long running action concurrently and sends response as json
    app.use([](auto &ctx) {
        auto &res = ctx.response;
        auto &req = ctx.request;

        // show header and our custom data
        qDebug() << "last route" << req.header << " " << ctx.get("customdata");

        // set custom header
        res.header["x-foo-data"] = "tvmid";

        // these are already default values
        // text/plain will be overriden by send if json is wanted
        res.status(200).type("text/plain");

        // some long running action runs in thread pool
        auto future = QtConcurrent::run([]{
           qDebug() << "long running action...";

           // return our demo result
           return QJsonDocument::fromJson("{\"hello\" : \"world\"}");
        });

        // get results and send response to client
        auto watcher = new QFutureWatcher<QJsonDocument>;
        QObject::connect(watcher, &QFutureWatcher<QJsonDocument>::finished,[&res, future]() {
            qDebug() << "long running action done";

            // send our demo result
            res.send(future.result());
        });
        watcher->setFuture(future);

    });

    qDebug() << "starting on port 3000...";
    app.listen(3000);
}
