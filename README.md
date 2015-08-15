# recurse

Recurse is set to be modern micro web framework written in latest C++14 using
Qt library leveraging all the best features of both worlds.  We strongly
emphasize on writing a clean and easy to understand code and avoid using
templates to encourage contributions.

Recurse aims to be small with no middlewares bundled in the core. This should
allow it to be very robust for writing next generation web applications and
APIs.


[![GitHub license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://github.com/xwalk/recurse/blob/master/LICENSE)

# Example


```
#include "recurse.hpp"

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // logger
    app.use([](auto &ctx, auto next) {
        qDebug() << ctx.request.ip;
        next();
    });

    // hello world
    app.use([](auto &ctx) {
        ctx.response.send("Hello world");
    });


    app.listen(3000);
}

```
