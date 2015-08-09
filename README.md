# recurse
Qt based micro web framework with middleware design

# example

```
#include "../recurse.hpp"

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // Start middleware
    app.use([](auto &req, auto &res, auto next) {
        qDebug() << "received a new request";
        next();
    });

    // Second middleware, sets custom data
    app.use([](auto &req, auto &res, auto next) {
        qDebug() << "routed request" << req.header;

        // custom data to be passed around - qvariant types
        req.ctx.set("customdata", "value");

        next();
    });

    app.use([](auto &req, auto &res, auto next) {
        qDebug() << "last route" << req.header;

        // custom header
        res.header["x-foo-data"] = "tvmid";

        // these are already default values
        res.status(200).type("text/plain");

        res.send("hello world");
    });

    qDebug() << "listening on port 3000...";
    app.listen(3000);
}
```
