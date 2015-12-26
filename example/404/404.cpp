/*
*
* this is simple example on how to handle 'not found' pages
*
* middlewares are positioned one after another and data is processed in same
* fashion
* to handle 404 simply write last middleware that will handle it
* that middleware won't be called if somebody responded before hand, but only if
* nobody made any response
*/

#include <recurse.hpp>
#include <QHash>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // example other middleware
    // if we send data from here, last 404 middleware won't get called
    app.use([](auto /* &ctx */, auto next) {
        next();
    });

    // if any middleware before this responds this won't get called
    app.use([](auto &ctx) {
        ctx.response.status(404).send("Not found");
    });

    auto result = app.listen(3000);
    if(result.error()) {
        qDebug() << "error upon listening:" << result.lastError();
    }
}
