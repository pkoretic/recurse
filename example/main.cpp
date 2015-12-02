#include <../recurse.hpp>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // app.listen(3000);

    HttpServer srv;

    bool bound = srv.compose(3000);

    if (!bound) {
        qDebug() << "could not bind to port 3000";
        return 0;
    }

    app.listen(&srv);

    qDebug() << "ok";
};
