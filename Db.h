#pragma once
#include <QtSql>
#include <QStandardPaths>
#include <QDir>

class Db {
public:
    static bool open()
    {
        // เก็บไฟล์ DB ใน AppData ของผู้ใช้ (ไม่ปนกับโฟลเดอร์ build)
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        const QString dbPath = dir + "/nkrfa_golf.sqlite";
        qDebug() << "DB Path =" << dbPath;

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(dbPath);
        if (!db.open()) return false;

        QSqlQuery q;
        q.exec(R"(
        CREATE TABLE IF NOT EXISTS pricing(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        customer_type TEXT NOT NULL,
        day_type TEXT NOT NULL,
        hole_type TEXT NOT NULL,
        green_fee INTEGER NOT NULL,
        cart_single INTEGER NOT NULL,
        cart_pair INTEGER NOT NULL,
        caddy INTEGER NOT NULL,
        UNIQUE(customer_type, day_type, hole_type)
        )
        )");
        q.exec("CREATE INDEX IF NOT EXISTS idx_transactions_date ON transactions(date)");

        QSqlQuery check;
        check.exec("SELECT COUNT(*) FROM pricing");
        check.next();
        if (check.value(0).toInt() == 0) {
            auto ins = [&](const QString& cust, const QString& day, const QString& hole,
                           int green, int cs, int cp, int caddy)
            {
                QSqlQuery q;
                q.prepare(R"(INSERT INTO pricing(customer_type,day_type,hole_type,green_fee,cart_single,cart_pair,caddy)
                     VALUES(?,?,?,?,?,?,?))");
                q.addBindValue(cust);
                q.addBindValue(day);
                q.addBindValue(hole);
                q.addBindValue(green);
                q.addBindValue(cs);
                q.addBindValue(cp);
                q.addBindValue(caddy);
                q.exec();
            };

            // --- Weekday ---
            ins("Member","Weekday","18 Holes",199,500,700,350);
            ins("Member","Weekday","9 Holes",120,300,500,200);

            ins("Thai","Weekday","18 Holes",300,500,700,350);
            ins("Thai","Weekday","9 Holes",200,300,500,200);

            ins("Foreigner","Weekday","18 Holes",400,500,700,350);
            ins("Foreigner","Weekday","9 Holes",300,300,500,200);

            // --- Weekend ---
            ins("Member","Weekend","18 Holes",250,500,700,350);
            ins("Member","Weekend","9 Holes",150,300,500,200);

            ins("Thai","Weekend","18 Holes",350,500,700,350);
            ins("Thai","Weekend","9 Holes",250,300,500,200);

            ins("Foreigner","Weekend","18 Holes",450,500,700,350);
            ins("Foreigner","Weekend","9 Holes",350,300,500,200);

            // --- Special groups (ตามภาพ: ไม่แยก weekday/weekend) ---
            // ใส่ทั้ง Weekday และ Weekend เพื่อ query ได้เสมอ
            for (auto day : {"Weekday","Weekend"}) {
                ins("Cadet (NKRAFA)",   day, "18 Holes",100,500,700,350);
                ins("Cadet (NKRAFA)",   day, "9 Holes", 50,300,500,200);

                ins("Air Force Officer",day, "18 Holes",150,500,700,350);
                ins("Air Force Officer",day, "9 Holes",100,300,500,200);

                ins("Military/Police",  day, "18 Holes",270,500,700,350);
                ins("Military/Police",  day, "9 Holes",170,300,500,200);
            }
        }
        return true;
    }

};
