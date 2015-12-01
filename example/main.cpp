#include <../recurse.hpp>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    app.listen(3000);

    // HttpServer srv(2000);
};
