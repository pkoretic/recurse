/*
*
* this is minimal hello world example
*/

#include "router/router.hpp"

int main(int argc, char *argv[])
{
    Recurse::Application app(argc, argv);

    Module::Router router;

    //    router.GET("/hello", [](auto &ctx, auto /* next */)
    //    {
    //        ctx.response.send("hello world");
    //    });

    router.GET("/hello", [](auto &ctx, auto /* next */)
    {
        ctx.response.send("hello world");
    });

//    router.GET("/foo/:id", [](auto &ctx, auto /* next */)
//    {
//        ctx.response.send("id only" + ctx.request.params["id"]);
//    });

    //    router.GET("*", [](auto &ctx, auto)
    //    {
    //        ctx.response.send("other");
    //    });

//    router.GET("/users/:id", [](auto &ctx, auto)
//    {
//        ctx.response.send("Hello World");
//    });

    //    router.ALL("*", [](auto &ctx, auto)
    //    {
    //        ctx.response.send("matches any");
    //    });

    //    router.GET("/hello/foo/:id/:name", [](auto &ctx, auto /* next */)
    //    {
    //        ctx.response.send("hello world foo" + ctx.request.params["id"]);
    //    });

    //    router.GET("/hello/foo/:id", [](auto &ctx, auto /* next */)
    //    {
    //        ctx.response.send("hello world foo");
    //    });

    //    router.GET("*", [](auto &ctx, auto /* next */)
    //    {
    //        ctx.response.send("hello world all");
    //    });

    //    router.GET("/hello/bar/:id", [](auto &ctx, auto /* next */)
    //    {
    //        auto query = ctx.request.query;
    //        ctx.response.send("hello world bar" + query.queryItemValue("test"));
    //    });

//    app.use(router.routes());
    app.use([](auto &ctx)
    {
       ctx.response.send("Hello World");
    });

    auto result = app.listen(3001);
    if (result.error())
    {
        qDebug() << "error upon listening:" << result.lastError();
    }
}
