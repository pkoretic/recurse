/*
*
* this is minimal hello world example
*/

#include <recurse.hpp>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    app.use([](auto &ctx) {
        ctx.response.send("Hello world");
    });

    auto result = app.listen(3000);
    if(result.error()) {
        qDebug() << "error upon listening:" << result.lastError();
    }
}
