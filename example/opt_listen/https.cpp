/*
*
* this example shows how to use your own QCoreApplication instance with Recurse
* you should call QCoreApplication's exec() method when the Recurse setup is
* done
*/

#include <recurse.hpp>
#include <QHash>

int main(int argc, char *argv[])
{
    QCoreApplication core(argc, argv);
    Recurse app(&core);

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

    auto ret = app.listen();
    if (ret.error()) {
        qDebug() << "error upon listening:" << ret.lastError();
    }

    qDebug() << "start listening...";
    core.exec();
}
