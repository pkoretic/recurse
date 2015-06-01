# recurse
Qt based micro web framework with middleware design

# example

```
#include "../recurse.hpp"

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

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
}
```
