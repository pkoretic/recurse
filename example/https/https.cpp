/*
*
* this example shows how to have both http and https at the same time
* one can of course use only http or https
*/

#include <recurse.hpp>
#include <QHash>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

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

    app.use([](auto &ctx, auto next) {
        qDebug("incoming request over %s", ctx.request.secure ? "HTTPS" : "HTTP");
        next();
    });

    app.use([](auto &ctx) {
        QString response = QString("Hello world from: %1").arg(ctx.request.secure ? "HTTPS" : "HTTP");
        ctx.response.send(response);
    });

    qDebug() << "start listening...";

    auto ret = app.listen();
    if (ret.error()) {
        qDebug() << "error upon listening:" << ret.lastError();
    }
}
