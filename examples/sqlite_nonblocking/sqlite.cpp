/*
*
* sqlite example using QtConcurrent to prevent bloking event loop by using thread pool
* default sqlite mode is 'serialized' which supports usage by multiple threads
* with no restriction https://www.sqlite.org/threadsafe.html
*/

#include "../../recurse.hpp"
#include <QtConcurrent/QtConcurrent>
#include <QtSql>

int main(int argc, char *argv[])
{
    Recurse::Application app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("sqlite.db");

    if (!db.open())
    {
        qDebug() << db.lastError();
        return 1;
    }

    db.exec("DROP TABLE if exists lorem");
    db.exec("CREATE TABLE lorem (info TEXT)");

    db.transaction();

    for (int i = 0; i < 10; i++)
        db.exec(QString("INSERT INTO lorem VALUES('ipsum %1')").arg(i));

    db.commit();

    app.use([&db](auto &ctx)
    {
        // fetch result from database in thread pool so we don't block clients
        // there is hardly any benefit for this simple example
        // but if here was some complex operation here then benefit would be shown (try QThread::sleep(1))
        auto future = QtConcurrent::run([&db]
        {
            QString data;

            auto query = db.exec("SELECT info FROM lorem");

            while (query.next())
                data += query.value(0).toString() % "\n";

            return data;
        });

        // get result from thread and send it to client
        auto watcher = new QFutureWatcher<QString>;
        QObject::connect(watcher, &QFutureWatcher<QString>::finished, [&ctx, future]
        {
            ctx.response.send(future.result());
        });

        watcher->setFuture(future);

    });

    auto result = app.listen(3001);
    if (result.error())
    {
        qDebug() << "error upon listening:" << result.lastError();
    }
}
