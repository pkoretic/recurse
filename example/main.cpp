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

    // middleware1: logger
    app.use([](auto req, auto &res, auto next) {
        qDebug() << "incoming request:" << req.body;
        next();
        qDebug() << "outgoing response:" << res;
    });

    // middleware2: hello world
    app.use([](auto req, auto &res, auto next) {
       res = req.data.trimmed() +  " hello world\n";
    });

    app.listen(3000);
}
