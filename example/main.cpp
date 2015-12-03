#include <../recurse.hpp>
#include <QHash>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    qDebug() << "start listening...";

    // default server setup
    // app.listen(3000);

    // manual http server setup
    /*
    HttpServer http;

    bool bound = http.compose(3000);

    if (!bound) {
        qDebug() << "could not bind to port 3000";
        return 0;
    }

    app.listen(&http);
    */

    // manual https server setup
    HttpsServer https;

    QHash<QString, QVariant> options;
    options["port"] = 3000;
    options["private_key"] = "./priv.pem";
    options["certificate"] = "./cert.pem";

    if (!https.compose(options)) {
        exit(1);
    }

    app.listen(&https);
};
