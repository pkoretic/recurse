# [<img title="Recurse" src="http://i.imgur.com/HJ1oUqY.png" width="810px" alt="Recurse logo"/>](https://github.com/qaap/recurse.git)

[![License MIT](https://cdn.rawgit.com/qaap/recurse/badges/license.svg)](https://github.com/qaap/recurse/blob/master/LICENSE)
[![Language (C++)](https://cdn.rawgit.com/qaap/recurse/badges/powered_by-C%2B%2B-blue.svg)](http://en.cppreference.com/w/cpp/language)

Recurse is set to be a modern web micro framework written in latest C++ (14) using
Qt library leveraging all the best features of both worlds.  We strongly
emphasize on writing a clean and easy to understand code and avoid using
templates to encourage contributions.

Recurse aims to be small with no middlewares bundled in the core. This should
allow it to be very robust for writing next generation web applications and
APIs.

It is inspired by [Node.js](https://nodejs.org/en) [koa](http://koajs.com) and [Express](http://expressjs.com) micro frameworks.

## Example


```
#include "recurse.hpp"

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // logger
    app.use([](auto &ctx, auto next)
    {
        qDebug() << ctx.request.ip;
        next();
    });

    // hello world
    app.use([](auto &ctx)
    {
        ctx.response.send("Hello world");
    });

    app.listen(3000);
};
```

## Installation

This is a header-only library. To use, just include `recurse.hpp` inside your project. See
[examples](examples) for more information.

**`NOTE`** you also need `context.hpp`, `request.hpp`, `response.hpp` as `recurse.hpp` depends on
them.

## Middlewares

There is no middleware bundled in the core.
For example, for routing, one can use [Router](https://github.com/qaap/recurse-router)

```
#include "router.hpp"

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    Module::Router router;

    router.GET("/hello/:user", [](auto &ctx, auto /* next */)
    {
        ctx.response.send("Hello World " + ctx.request.params["user"]);
    });

    app.listen();
}
```

## 404 - Not Found

By default, if no middleware responds, **Recurse** will respond with `Not Found`
message, and `404` HTTP error code.

To make your own response, simply add new middleware at the **end** of the list
```
// if any middleware before this responds this won't get called
app.use([](auto &ctx)
{
    ctx.response.status(404).send("Custom Not Found");
});
```
For a complete example see [404 example](https://github.com/qaap/recurse/tree/master/examples/404)

You can also have it as a **first** middleware (if you already have some first
middleware that does your logging or similar)

```
app.use([](auto &ctx, auto next, auto prev)
{
    next([&ctx, prev]
    {
        // this is last code to be called before sending response to client
        if(ctx.response.status() == 404)
            ctx.response.body("Custom Not Found");

        prev();
    });
});
```

## Styling

When writing code, please use the provided [.clang-format](https://github.com/qaap/recurse/blob/master/.clang-format) file.
There is a nice [vim-clang-format](https://github.com/rhysd/vim-clang-format) plugin that you can use in vim.

You can also call it manually

```
clang-format -i source.hpp

# to format all files
find . -name "*.hpp" -or -name "*.cpp" | xargs clang-format -i

```

And you can also use shortcut command
```
clang-format -i -style="{BasedOnStyle: WebKit, PointerAlignment: Right, Standard: Cpp11, TabWidth: 4, UseTab: Never, BreakBeforeBraces: Allman, AllowShortFunctionsOnASingleLine: false, ContinuationIndentWidth: 0, MaxEmptyLinesToKeep: 1, NamespaceIndentation: All, AccessModifierOffset: 0}" source.hpp

```
