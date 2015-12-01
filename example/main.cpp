#include <../recurse.hpp>

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);
    HttpServer srv(2000);
};
