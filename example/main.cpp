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
    app.use([](auto &ctx, auto next, auto prev) {

        qDebug() << "received a new request from:" << ctx.request.ip;
        int first = 123;

        next([=](){
            qDebug() << "last upstream mw:" << first;

            prev();
        });
    });

    // Second middleware, sets custom data
    app.use([](auto &ctx, auto next, auto prev) {
        qDebug() << "routed request:" << ctx.request.get("user-agent");

        QString test("a");
        // custom data to be passed around - qvariant types
        ctx.set("customdata", "value");

        // for any kind of data use
        // ctx.data["key"] = *void

        next([&ctx, test, prev](){
            qDebug() << "first upstream mw:" << test;

            // overrides response
            //ctx.response.body("10");

            prev();
        });
    });

    // Final middleware, does long running action concurrently and sends response as json
    app.use([](auto &ctx) {
        auto &res = ctx.response;

        // show our custom data
        qDebug() << "last route: " << ctx.get("customdata");

        // set custom header
        res.set("x-foo-data", "tvmid");

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
