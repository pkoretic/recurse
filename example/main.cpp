//
// recurse example usage
//
// build as:
//
// qmake .
// make
//

#include "../recurse.hpp"

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // Start middleware
    app.use([](auto &req, auto &res, auto next) {
        qDebug() << "received a new request";
        next();
    });

    app.use([](auto &req, auto &res, auto next) {
        qDebug() << "routed request" << req.header;
        res.header["content-type"] = "application/json";
        next();
    });

    app.use([](auto &req, auto &res, auto next) {
        qDebug() << "last route" << req.header;
        res.header["x-foo-data"] = "tvmid";
        res.status = 200;
        res.body = "hello";
    });

    qDebug() << "listening on port 3000...";
    app.listen(3000);
}
