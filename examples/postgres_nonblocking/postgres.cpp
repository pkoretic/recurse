/*
*
* postgres example using QtConcurrent to prevent bloking event loop by using thread pool
*/

#include "../../recurse.hpp"
#include <QtConcurrent/QtConcurrent>
#include <QtSql>

int main(int argc, char *argv[])
{
    Recurse::Application app(argc, argv);

    QMutex mutex;
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("127.0.0.1");
    db.setDatabaseName("postgsres");
    db.setUserName("postgress");
    db.setPassword("");

    if (!db.open())
    {
        qDebug() << db.lastError();
        return 1;
    }

    QSqlQuery query;
    query.exec("DROP table lorem");
    query.exec("CREATE TABLE lorem (info TEXT)");

    db.transaction();

    for (int i = 0; i < 10; i++)
        query.exec(QString("INSERT INTO lorem VALUES('ipsum %1')").arg(i));

    db.commit();

    app.use([&db, &mutex](auto &ctx)
    {
        // we must fetch result from database in thread pool so we don't block clients
        // there is hardly any benefit for this simple example
        // but if this was slow connection to database or there was some complex operation here
        // then benefit would be shown (try QThread::sleep(1))
        auto future = QtConcurrent::run([&db, &mutex]
        {
            QString data;

            // we can only query from one thread at the time
            QMutexLocker locker(&mutex);
            QSqlQuery query("SELECT info FROM lorem");

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
