/*
*
* sqlite example usage
*/

#include "../../recurse.hpp"
#include <QtConcurrent/QtConcurrent>
#include <QtSql>

int main(int argc, char *argv[])
{
    Recurse::Application app(argc, argv);

    // prepare database data
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("sqlite.db");

    if (!db.open())
    {
        qDebug() << "opening sqlite database failed" << db.lastError();
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
        QString data;

        auto query = db.exec("SELECT info FROM lorem");

        while (query.next())
            data += query.value(0).toString() % "\n";

        ctx.response.send(data);
    });

    auto result = app.listen(3001);
    if (result.error())
    {
        qDebug() << "error upon listening:" << result.lastError();
    }
}
