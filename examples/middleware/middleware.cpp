/*
*
* this example shows how to compose multiple middlewares
*/

#include <recurse.hpp>
#include <QHash>

int main(int argc, char *argv[])
{
    Recurse::Application app(argc, argv);

    app.use([](auto &ctx, auto next)
    {
        qDebug() << "incoming request from: " << ctx.request.ip;
        next();
    });

    app.use([](auto &ctx, auto next, auto prev)
    {
        qDebug() << "second middleware downstream";

        // on can pass data through multiple middlewares downstream/upstream
        ctx.set("customdata", "value");

        next([=]()
        {
            qDebug() << "last middlware upstream:" << ctx.get("customdata");

            prev();
        });
    });

    app.use([](auto &ctx)
    {
        ctx.response.send("final response");
    });

    auto result = app.listen(3000);
    if (result.error())
    {
        qDebug() << "error upon listening:" << result.lastError();
    }
}
