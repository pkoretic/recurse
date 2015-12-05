#include <../recurse.hpp>
#include <QHash>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    qDebug() << "start listening...";

    // default server setup
    // app.listen(3000);

    // manual http server setup *OLD BEHAVIOUR* - info will be removed
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
    /*
    HttpsServer https;

    QHash<QString, QVariant> options;
    options["port"] = 3000;
    options["private_key"] = "./priv.pem";
    options["certificate"] = "./cert.pem";

    if (!https.compose(options)) {
        exit(1);
    }
    */

    // http options
    QHash<QString, QVariant> http_options;
    http_options["port"] = 3000;

    // https options
    QHash<QString, QVariant> https_options;
    https_options["port"] = 3020;
    https_options["private_key"] = "./priv.pem";
    https_options["certificate"] = "./cert.pem";

    app.http_server(http_options);
    app.https_server(https_options);

    app.listen();
};
