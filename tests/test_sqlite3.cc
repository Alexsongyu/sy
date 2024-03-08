#include "sy/db/sqlite3.h"
#include "sy/log.h"
#include "sy/util.h"

static sy::Logger::ptr g_logger = SY_LOG_ROOT();

void test_batch(sy::SQLite3::ptr db) {
    auto ts = sy::GetCurrentMS();
    int n = 1000000;
    sy::SQLite3Transaction trans(db);
    //db->execute("PRAGMA synchronous = OFF;");
    trans.begin();
    sy::SQLite3Stmt::ptr stmt = sy::SQLite3Stmt::Create(db,
                "insert into user(name, age) values(?, ?)");
    for(int i = 0; i < n; ++i) {
        stmt->reset();
        stmt->bind(1, "batch_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->step();
    }
    trans.commit();
    auto ts2 = sy::GetCurrentMS();

    SY_LOG_INFO(g_logger) << "used: " << (ts2 - ts) / 1000.0 << "s batch insert n=" << n;
}

int main(int argc, char** argv) {
    const std::string dbname = "test.db";
    auto db = sy::SQLite3::Create(dbname, sy::SQLite3::READWRITE);
    if(!db) {
        SY_LOG_INFO(g_logger) << "dbname=" << dbname << " not exists";
        db = sy::SQLite3::Create(dbname
                , sy::SQLite3::READWRITE | sy::SQLite3::CREATE);
        if(!db) {
            SY_LOG_INFO(g_logger) << "dbname=" << dbname << " create error";
            return 0;
        }

#define XX(...) #__VA_ARGS__
        int rt = db->execute(
XX(create table user (
        id integer primary key autoincrement,
        name varchar(50) not null default "",
        age int not null default 0,
        create_time datetime
        )));
#undef XX

        if(rt != SQLITE_OK) {
            SY_LOG_ERROR(g_logger) << "create table error "
                << db->getErrno() << " - " << db->getErrStr();
            return 0;
        }
    }

    for(int i = 0; i < 10; ++i) {
        if(db->execute("insert into user(name, age) values(\"name_%d\",%d)", i, i)
                != SQLITE_OK) {
            SY_LOG_ERROR(g_logger) << "insert into error " << i << " "
                << db->getErrno() << " - " << db->getErrStr();
        }
    }

    sy::SQLite3Stmt::ptr stmt = sy::SQLite3Stmt::Create(db,
                "insert into user(name, age, create_time) values(?, ?, ?)");
    if(!stmt) {
        SY_LOG_ERROR(g_logger) << "create statement error "
            << db->getErrno() << " - " << db->getErrStr();
        return 0;
    }

    int64_t now = time(0);
    for(int i = 0; i < 10; ++i) {
        stmt->bind(1, "stmt_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->bind(3, now + rand() % 100);
        //stmt->bind(3, sy::Time2Str(now + rand() % 100));
        //stmt->bind(3, "stmt_" + std::to_string(i + 1));
        //stmt->bind(4, i + 1);

        if(stmt->execute() != SQLITE_OK) {
            SY_LOG_ERROR(g_logger) << "execute statment error " << i << " "
                << db->getErrno() << " - " << db->getErrStr();
        }
        stmt->reset();
    }

    sy::SQLite3Stmt::ptr query = sy::SQLite3Stmt::Create(db,
            "select * from user");
    if(!query) {
        SY_LOG_ERROR(g_logger) << "create statement error "
            << db->getErrno() << " - " << db->getErrStr();
        return 0;
    }
    auto ds = query->query();
    if(!ds) {
        SY_LOG_ERROR(g_logger) << "query error "
            << db->getErrno() << " - " << db->getErrStr();
        return 0;
    }

    while(ds->next()) {
        //SY_LOG_INFO(g_logger) << "query ";
    };

    //const char v[] = "hello ' world";
    const std::string v = "hello ' world";
    db->execStmt("insert into user(name) values (?)", v);

    auto dd = (db->queryStmt("select * from user"));
    while(dd->next()) {
        SY_LOG_INFO(g_logger) << "ds.data_count=" << dd->getDataCount()
            << " ds.column_count=" << dd->getColumnCount()
            << " 0=" << dd->getInt32(0) << " 1=" << dd->getString(1)
            << " 2=" << dd->getString(2)
            << " 3=" << dd->getString(3);
    }

    test_batch(db);
    return 0;
}
