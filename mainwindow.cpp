#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QToolBar>
#include <QAction>
#include <QDateTime>
#include <QHeaderView>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QTextDocument>
#include <QTextOption>
#include <QPageSize>
#include <QPageLayout>
#include <QtSql>
#include <QtSql>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QCoreApplication>


static QLabel* makeKpi(const QString& title)
{
    auto* w = new QLabel;
    w->setText(title + "\n—");
    w->setMinimumHeight(82);
    w->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // ให้ stylesheet global จับได้
    w->setProperty("kpiCard", true);

    return w;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    buildUi();
    onAnyChanged();
}

void MainWindow::buildUi()
{
    setWindowTitle("NKRAFA Golf Club – Ticketing (Template)");
    resize(753, 606);

    // Toolbar navigation
    auto* tb = addToolBar("Nav");
    tb->setMovable(false);

    // ✅ เปลี่ยนสีแถบ + สีปุ่ม + hover
    tb->setStyleSheet(R"(
QToolBar{
    background:#1E3A2F;     /* << สีแถบด้านบน */
    spacing:8px;
    padding:6px;
}
QToolButton{
    color:white;
    background:transparent;
    border:1px solid rgba(255,255,255,0.25);
    border-radius:8px;
    padding:6px 12px;
    font-weight:600;
}
QToolButton:hover{
    background:rgba(255,255,255,0.12);
}
QToolButton:pressed{
    background:rgba(255,255,255,0.20);
}
)");

    QAction* actPOS  = tb->addAction("Position");
    QAction* actDash = tb->addAction("Dashboard");
    QAction* actHistory = tb->addAction("History");
    QAction* actPricing = tb->addAction("Pricing");

    stack_ = new QStackedWidget(this);
    pagePOS_  = buildPosPage();
    pageDash_ = buildDashPage();
    pageHistory_ = buildHistoryPage();
    pagePricing_ = buildPricingPage();

    stack_->addWidget(pagePOS_);
    stack_->addWidget(pageDash_);
    stack_->addWidget(pageHistory_);
    stack_->addWidget(pagePricing_);
    setCentralWidget(stack_);

    QLabel* logo = new QLabel;
    QPixmap pix(":/nkrafalogo_eng.png");
    logo->setPixmap(pix.scaledToHeight(36, Qt::SmoothTransformation));
    tb->addWidget(logo);
    tb->addSeparator();
    tb->addSeparator();
    QAction* actLight = tb->addAction("Light");
    QAction* actDark  = tb->addAction("Dark");

    connect(actLight, &QAction::triggered, this, [this]{ applyTheme(ThemeMode::NKRAFA_Light); });
    connect(actDark,  &QAction::triggered, this, [this]{ applyTheme(ThemeMode::NKRAFA_Dark);  });

    connect(actPOS,  &QAction::triggered, this, [this]{ stack_->setCurrentWidget(pagePOS_); });
    connect(actDash, &QAction::triggered, this, [this]{ stack_->setCurrentWidget(pageDash_); });
    connect(btnPrint_, &QPushButton::clicked, this, &MainWindow::onPrintTicket);
    connect(actHistory, &QAction::triggered, this, [this]{
        stack_->setCurrentWidget(pageHistory_);
        refreshHistory(); // เปิดมาก็โหลดข้อมูลช่วงวันล่าสุดทันที
    });
    connect(actPricing, &QAction::triggered, this, [this]{
        stack_->setCurrentWidget(pagePricing_);
        loadPricingTable();
    });

    applyTheme(ThemeMode::NKRAFA_Light);
}

QWidget* MainWindow::buildPosPage()
{
    auto* root = new QWidget;
    auto* main = new QHBoxLayout(root);
    main->setContentsMargins(16,16,16,16);
    main->setSpacing(16);

    // Left: inputs
    auto* left = new QVBoxLayout;
    left->setSpacing(12);

    auto* boxA = new QGroupBox("Sale Setup");
    boxA->setStyleSheet("QGroupBox{font-weight:700;}");
    auto* g = new QGridLayout(boxA);

    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);

    dayTypeLabel_ = new QLabel;
    dayTypeLabel_->setStyleSheet("QLabel{font-weight:700;}");

    customerCombo_ = new QComboBox;
    customerCombo_->addItem("Member",          (int)CustomerType::Member);
    customerCombo_->addItem("Thai (General)",  (int)CustomerType::Thai);
    customerCombo_->addItem("Foreigner",       (int)CustomerType::Foreigner);
    customerCombo_->addItem("Cadet (NKRAFA)",  (int)CustomerType::Cadet_NKRAFA);
    customerCombo_->addItem("Air Force Officer",(int)CustomerType::AirForceOfficer);
    customerCombo_->addItem("Military/Police", (int)CustomerType::MilitaryPolice);

    rb18_ = new QRadioButton("18 Holes");
    rb9_  = new QRadioButton("9 Holes");
    rb18_->setChecked(true);

    cbCartSingle_ = new QCheckBox("Cart – Single");
    cbCartPair_   = new QCheckBox("Cart – Pair");
    cbCaddy_      = new QCheckBox("Caddy");

    g->addWidget(new QLabel("Date"), 0,0);        g->addWidget(dateEdit_, 0,1);
    g->addWidget(new QLabel("Day Type"), 1,0);    g->addWidget(dayTypeLabel_, 1,1);
    g->addWidget(new QLabel("Customer"), 2,0);    g->addWidget(customerCombo_, 2,1);
    g->addWidget(new QLabel("Holes"), 3,0);
    auto* holesRow = new QHBoxLayout; holesRow->addWidget(rb18_); holesRow->addWidget(rb9_);
    auto* holesW = new QWidget; holesW->setLayout(holesRow);
    g->addWidget(holesW, 3,1);

    g->addWidget(new QLabel("Add-ons"), 4,0);
    auto* addRow = new QVBoxLayout; addRow->addWidget(cbCartSingle_); addRow->addWidget(cbCartPair_); addRow->addWidget(cbCaddy_);
    auto* addW = new QWidget; addW->setLayout(addRow);
    g->addWidget(addW, 4,1);

    left->addWidget(boxA);

    // Right: price preview (still on left column)
    auto* boxB = new QGroupBox("Price Preview");
    boxB->setStyleSheet("QGroupBox{font-weight:700;}");
    auto* pv = new QGridLayout(boxB);

    lblGreen_ = new QLabel("-");
    lblCart_  = new QLabel("-");
    lblCaddy_ = new QLabel("-");
    lblTotal_ = new QLabel("0");
    lblTotal_->setStyleSheet("QLabel{font-size:28px;font-weight:900;}");

    pv->addWidget(new QLabel("Green Fee"), 0,0); pv->addWidget(lblGreen_, 0,1);
    pv->addWidget(new QLabel("Cart"),      1,0); pv->addWidget(lblCart_,  1,1);
    pv->addWidget(new QLabel("Caddy"),     2,0); pv->addWidget(lblCaddy_, 2,1);
    pv->addWidget(new QLabel("TOTAL"),     3,0); pv->addWidget(lblTotal_, 3,1);

    btnConfirm_ = new QPushButton("Confirm / Add Transaction");
    btnConfirm_->setMinimumHeight(44);

    btnPrint_ = new QPushButton("Print Ticket");
    btnPrint_->setMinimumHeight(44);
    btnPrint_->setEnabled(true);

    auto* rowBtn = new QHBoxLayout;
    rowBtn->addWidget(btnConfirm_);
    rowBtn->addWidget(btnPrint_);
    auto* rowBtnW = new QWidget;
    rowBtnW->setLayout(rowBtn);

    left->addWidget(boxB);
    left->addWidget(rowBtnW);
    left->addStretch(1);

    // Right: helper panel (optional)
    auto* right = new QVBoxLayout;
    auto* note = new QLabel(
        "ระบบจำหน่ายบัตรสนามกอล์ฟ NKRAFA\n"
        "ธุรกรรมทั้งหมดถูกบันทึกและสามารถตรวจสอบย้อนหลังได้"
        );
    note->setStyleSheet(
        "QLabel{background:#ffffff;border:1px solid #ddd;"
        "border-radius:12px;padding:12px;font-size:16px;}"
        );

    // 🔽 สร้าง QLabel สำหรับ Logo
    auto* logoLabel = new QLabel;
    QPixmap logoPix(":/nkrafalogo_eng.png");   // ใช้ resource path เดิม
    logoLabel->setPixmap(logoPix.scaledToHeight(230, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);

    // เพิ่มเข้า layout ด้านขวา
    right->addWidget(note);
    right->addWidget(logoLabel);
    right->addStretch(1);
    main->addLayout(left, 2);
    main->addLayout(right, 1);

    // signals
    connect(dateEdit_, &QDateEdit::dateChanged, this, &MainWindow::onAnyChanged);
    connect(customerCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::onAnyChanged);
    connect(rb18_, &QRadioButton::toggled, this, &MainWindow::onAnyChanged);
    connect(rb9_,  &QRadioButton::toggled, this, &MainWindow::onAnyChanged);
    connect(btnPrint_, &QPushButton::clicked, this, &MainWindow::onPrintTicket);

    // cart single/pair mutually exclusive
    connect(cbCartSingle_, &QCheckBox::toggled, this, [this](bool on){
        if (on) cbCartPair_->setChecked(false);
        onAnyChanged();
    });
    connect(cbCartPair_, &QCheckBox::toggled, this, [this](bool on){
        if (on) cbCartSingle_->setChecked(false);
        onAnyChanged();
    });
    connect(cbCaddy_, &QCheckBox::toggled, this, &MainWindow::onAnyChanged);

    connect(btnConfirm_, &QPushButton::clicked, this, &MainWindow::onConfirm);

    return root;
}

QWidget* MainWindow::buildDashPage()
{
    auto* root = new QWidget;
    auto* v = new QVBoxLayout(root);
    v->setContentsMargins(16,16,16,16);
    v->setSpacing(12);


    // KPI row
    auto* k = new QHBoxLayout;
    k->setSpacing(12);

    kpiRevenue_ = makeKpi("Revenue (Today)"); // เขียว
    kpiPlayers_ = makeKpi("Players (Today)"); // น้ำเงิน
    kpi18_      = makeKpi("18 Holes"); // ม่วง
    kpi9_       = makeKpi("9 Holes"); // ส้ม
    kpiCart_    = makeKpi("Cart Revenue"); // ฟ้า
    kpiCaddy_   = makeKpi("Caddy Revenue"); // แดง

    k->addWidget(kpiRevenue_);
    k->addWidget(kpiPlayers_);
    k->addWidget(kpi18_);
    k->addWidget(kpi9_);
    k->addWidget(kpiCart_);
    k->addWidget(kpiCaddy_);
    v->addLayout(k);

    // Table
    table_ = new QTableWidget(0, 9);
    table_->setHorizontalHeaderLabels(
        {"Time","Customer","Day","Holes","Green Fee","Cart","Caddy","Total","Operator"});
    table_->setStyleSheet("QTableWidget{background:#ffffff;border:1px solid #ddd;border-radius:12px;}");
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* summaryLayout = new QHBoxLayout;

    lblDaily_   = makeKpi("Today");
    lblWeekly_  = makeKpi("This Week");
    lblMonthly_ = makeKpi("This Month");
    lblYearly_  = makeKpi("This Year");

    summaryLayout->addWidget(lblDaily_);
    summaryLayout->addWidget(lblWeekly_);
    summaryLayout->addWidget(lblMonthly_);
    summaryLayout->addWidget(lblYearly_);

    v->addLayout(summaryLayout);

    // ✅ เรียกหลังจากสร้าง lbl... แล้วเท่านั้น
    updateFinancialSummary();

    v->addWidget(table_, 1);


    return root;
}



void MainWindow::onAnyChanged()
{
    updateDayType();
    updatePricePreview();
}

void MainWindow::updateDayType()
{
    DayType d = detectDayType(dateEdit_->date());
    dayTypeLabel_->setText(toString(d));
}

void MainWindow::updatePricePreview()
{
    DayType d = detectDayType(dateEdit_->date());
    HoleType h = rb18_->isChecked() ? HoleType::H18 : HoleType::H9;
    CustomerType c = (CustomerType)customerCombo_->currentData().toInt();

    bool cartSingle = cbCartSingle_->isChecked();
    bool cartPair   = cbCartPair_->isChecked();
    bool caddy      = cbCaddy_->isChecked();

    PriceBreakdown b = calcPriceDb(c, d, h, cartSingle, cartPair, caddy);

    lblGreen_->setText(QString("฿ %1").arg(b.greenFee));
    lblCart_->setText(QString("฿ %1").arg(b.cartFee));
    lblCaddy_->setText(QString("฿ %1").arg(b.caddyFee));
    lblTotal_->setText(QString("฿ %1").arg(b.total()));
}

void MainWindow::addTransactionToDashboard(const PriceBreakdown& b, DayType d, HoleType h, CustomerType c)
{
    const int row = table_->rowCount();
    table_->insertRow(row);

    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString oper = "Counter-1"; // TODO: login user

    auto set = [&](int col, const QString& s){
        table_->setItem(row, col, new QTableWidgetItem(s));
    };

    set(0, time);
    set(1, toString(c));
    set(2, toString(d));
    set(3, toString(h));
    set(4, QString::number(b.greenFee));
    set(5, QString::number(b.cartFee));
    set(6, QString::number(b.caddyFee));
    set(7, QString::number(b.total()));
    set(8, oper);

    // update KPIs
    totalRevenue_ += b.total();
    totalPlayers_ += 1;
    if (h == HoleType::H18) total18_++; else total9_++;
    totalCart_  += b.cartFee;
    totalCaddy_ += b.caddyFee;

    kpiRevenue_->setText(QString("Revenue (Today)<br><span style='font-size:26px;font-weight:900;'>฿ %1</span>").arg(totalRevenue_));
    kpiPlayers_->setText(QString("Players (Today)\n%1").arg(totalPlayers_));
    kpi18_->setText(QString("18 Holes\n%1").arg(total18_));
    kpi9_->setText(QString("9 Holes\n%1").arg(total9_));
    kpiCart_->setText(QString("Cart Revenue\n฿ %1").arg(totalCart_));
    kpiCaddy_->setText(QString("Caddy Revenue\n฿ %1").arg(totalCaddy_));
}

#include <QtSql>

void MainWindow::onConfirm()
{
    DayType d = detectDayType(dateEdit_->date());
    HoleType h = rb18_->isChecked() ? HoleType::H18 : HoleType::H9;
    CustomerType c = (CustomerType)customerCombo_->currentData().toInt();

    PriceBreakdown b = calcPriceDb(c, d, h,
                                   cbCartSingle_->isChecked(),
                                   cbCartPair_->isChecked(),
                                   cbCaddy_->isChecked());

    // ✅ บันทึกลง DB
    QSqlQuery q;
    q.prepare(R"(
      INSERT INTO transactions(ts,date,customer_type,day_type,hole_type,green_fee,cart_fee,caddy_fee,total,operator)
      VALUES(?,?,?,?,?,?,?,?,?,?)
    )");
    const QDateTime now = QDateTime::currentDateTime();
    q.addBindValue(now.toString("yyyy-MM-dd HH:mm:ss"));
    q.addBindValue(now.date().toString("yyyy-MM-dd"));
    q.addBindValue(toString(c));
    q.addBindValue(toString(d));
    q.addBindValue(toString(h));
    q.addBindValue(b.greenFee);
    q.addBindValue(b.cartFee);
    q.addBindValue(b.caddyFee);
    q.addBindValue(b.total());
    q.addBindValue("Counter-1"); // TODO: login user



    if (!q.exec()) {
        QMessageBox::warning(this, "DB", "Insert failed:\n" + q.lastError().text());
        return;
    }
     updateFinancialSummary();
    // โชว์บน dashboard (เหมือนเดิม)
    addTransactionToDashboard(b, d, h, c);
    stack_->setCurrentWidget(pageDash_);
}

void MainWindow::onPrintTicket()
{
    DayType d = detectDayType(dateEdit_->date());
    HoleType h = rb18_->isChecked() ? HoleType::H18 : HoleType::H9;
    CustomerType c = (CustomerType)customerCombo_->currentData().toInt();

    PriceBreakdown b = calcPriceDb(c, d, h,
                                   cbCartSingle_->isChecked(),
                                   cbCartPair_->isChecked(),
                                   cbCaddy_->isChecked());

    QPrinter printer(QPrinter::HighResolution);

    // ✅ ขนาด A5 (ปรับได้)
    printer.setPageSize(QPageSize(QSizeF(80, 200), QPageSize::Millimeter));
    printer.setPageOrientation(QPageLayout::Portrait);

    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle("Print Ticket");
    if (dlg.exec() != QDialog::Accepted) return;

    // --- HTML Ticket ---
    const QString html = QString(R"(
    <div style="font-family:'Segoe UI'; font-size:11pt;">
      <div style="font-size:18pt; font-weight:700;">NKRAFA GOLF CLUB</div>
      <div style="font-size:12pt; margin-top:4px;">Ticket / Receipt</div>
      <hr/>
      <table style="width:100%%; border-collapse:collapse;">
        <tr><td>Date</td><td style="text-align:right;">%1</td></tr>
        <tr><td>Day</td><td style="text-align:right;">%2</td></tr>
        <tr><td>Customer</td><td style="text-align:right;">%3</td></tr>
        <tr><td>Holes</td><td style="text-align:right;">%4</td></tr>
      </table>
      <hr/>
      <table style="width:100%%; border-collapse:collapse;">
        <tr><td>Green Fee</td><td style="text-align:right;">฿ %5</td></tr>
        <tr><td>Cart</td><td style="text-align:right;">฿ %6</td></tr>
        <tr><td>Caddy</td><td style="text-align:right;">฿ %7</td></tr>
      </table>
      <hr/>
      <div style="font-size:16pt; font-weight:800; text-align:right;">
        TOTAL: ฿ %8
      </div>
      <div style="margin-top:10px; font-size:10pt;">Thank you.</div>
    </div>
    )")
                             .arg(dateEdit_->date().toString("dd/MM/yyyy"))
                             .arg(toString(d))
                             .arg(toString(c))
                             .arg(toString(h))
                             .arg(b.greenFee)
                             .arg(b.cartFee)
                             .arg(b.caddyFee)
                             .arg(b.total());

    QTextDocument doc;
    doc.setHtml(html);

    // ✅ ทำให้ doc รู้ขนาดหน้ากระดาษของ printer (สำคัญมาก)
    const QRectF pageRect = printer.pageRect(QPrinter::Point);
    doc.setPageSize(pageRect.size());

    doc.print(&printer);
}

QWidget* MainWindow::buildHistoryPage()
{
    auto* root = new QWidget;
    auto* v = new QVBoxLayout(root);
    v->setContentsMargins(16,16,16,16);
    v->setSpacing(12);

    // Top filter bar
    auto* bar = new QHBoxLayout;
    bar->setSpacing(10);

    dateFrom_ = new QDateEdit(QDate::currentDate().addDays(-7));
    dateFrom_->setCalendarPopup(true);

    dateTo_ = new QDateEdit(QDate::currentDate());
    dateTo_->setCalendarPopup(true);

    editOperator_ = new QLineEdit;
    editOperator_->setPlaceholderText("Operator (optional) e.g., Counter-1");

    btnSearch_ = new QPushButton("Search");
    btnExportCsv_ = new QPushButton("Export CSV");

    bar->addWidget(new QLabel("From"));
    bar->addWidget(dateFrom_);
    bar->addWidget(new QLabel("To"));
    bar->addWidget(dateTo_);
    bar->addWidget(editOperator_, 1);
    bar->addWidget(btnSearch_);
    bar->addWidget(btnExportCsv_);

    v->addLayout(bar);

    // Table
    historyView_ = new QTableView;
    historyView_->setStyleSheet("QTableView{background:#ffffff;border:1px solid #ddd;border-radius:12px;}");
    historyView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyView_->setSelectionMode(QAbstractItemView::SingleSelection);
    historyView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyView_->horizontalHeader()->setStretchLastSection(true);
    historyView_->verticalHeader()->setVisible(false);

    historyModel_ = new QSqlQueryModel(this);
    historyView_->setModel(historyModel_);

    v->addWidget(historyView_, 1);

    connect(btnSearch_, &QPushButton::clicked, this, &MainWindow::onSearchHistory);
    connect(btnExportCsv_, &QPushButton::clicked, this, &MainWindow::onExportCsv);

    // เปลี่ยนวันแล้ว search ได้เลย
    connect(dateFrom_, &QDateEdit::dateChanged, this, &MainWindow::onSearchHistory);
    connect(dateTo_, &QDateEdit::dateChanged, this, &MainWindow::onSearchHistory);

    return root;
}

void MainWindow::refreshHistory()
{
    // normalize: from <= to
    QDate from = dateFrom_->date();
    QDate to   = dateTo_->date();
    if (from > to) std::swap(from, to);

    QString fromS = from.toString("yyyy-MM-dd");
    QString toS   = to.toString("yyyy-MM-dd");

    QString sql = R"(
      SELECT
        id,
        ts,
        customer_type,
        day_type,
        hole_type,
        green_fee,
        cart_fee,
        caddy_fee,
        total,
        operator
      FROM transactions
      WHERE date >= :from AND date <= :to
    )";

    const QString op = editOperator_->text().trimmed();
    if (!op.isEmpty()) {
        sql += " AND operator LIKE :op";
    }

    sql += " ORDER BY ts DESC";

    QSqlQuery q;
    q.prepare(sql);
    q.bindValue(":from", fromS);
    q.bindValue(":to", toS);
    if (!op.isEmpty()) q.bindValue(":op", "%" + op + "%");

    q.exec();
    historyModel_->setQuery(q);

    // ตั้งชื่อหัวคอลัมน์ให้อ่านง่าย
    historyModel_->setHeaderData(0, Qt::Horizontal, "ID");
    historyModel_->setHeaderData(1, Qt::Horizontal, "Time");
    historyModel_->setHeaderData(2, Qt::Horizontal, "Customer");
    historyModel_->setHeaderData(3, Qt::Horizontal, "Day");
    historyModel_->setHeaderData(4, Qt::Horizontal, "Holes");
    historyModel_->setHeaderData(5, Qt::Horizontal, "Green");
    historyModel_->setHeaderData(6, Qt::Horizontal, "Cart");
    historyModel_->setHeaderData(7, Qt::Horizontal, "Caddy");
    historyModel_->setHeaderData(8, Qt::Horizontal, "Total");
    historyModel_->setHeaderData(9, Qt::Horizontal, "Operator");
}

void MainWindow::onSearchHistory()
{
    refreshHistory();
}

static QString csvEscape(QString s)
{
    // escape quotes and wrap with quotes if needed
    s.replace("\"", "\"\"");
    if (s.contains(",") || s.contains("\n") || s.contains("\""))
        s = "\"" + s + "\"";
    return s;
}

void MainWindow::onExportCsv()
{
    if (!historyModel_ || historyModel_->rowCount() == 0) {
        QMessageBox::information(this, "Export CSV", "No data to export for selected range.");
        return;
    }

    const QString defaultName =
        QString("NKRAFA_GOLF_History_%1_to_%2.csv")
            .arg(dateFrom_->date().toString("yyyyMMdd"))
            .arg(dateTo_->date().toString("yyyyMMdd"));

    const QString path = QFileDialog::getSaveFileName(
        this, "Save CSV", defaultName, "CSV Files (*.csv)");

    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, "Export CSV", "Cannot write file:\n" + path);
        return;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8); // ✅ กันภาษาไทยเพี้ยนใน Excel/Notepad

    const int cols = historyModel_->columnCount();
    const int rows = historyModel_->rowCount();

    // header
    for (int c=0; c<cols; ++c) {
        out << csvEscape(historyModel_->headerData(c, Qt::Horizontal).toString());
        if (c < cols-1) out << ",";
    }
    out << "\n";

    // rows
    for (int r=0; r<rows; ++r) {
        for (int c=0; c<cols; ++c) {
            out << csvEscape(historyModel_->data(historyModel_->index(r,c)).toString());
            if (c < cols-1) out << ",";
        }
        out << "\n";
    }

    f.close();
    QMessageBox::information(this, "Export CSV", "Exported successfully:\n" + path);
}

void MainWindow::updateFinancialSummary()
{

    if (!lblDaily_ || !lblWeekly_ || !lblMonthly_ || !lblYearly_) return;

    QSqlQuery q;

    auto getSum = [&](const QString& sql)->int{
        if (!q.exec(sql) || !q.next()) return 0;
        return q.value(0).toInt();
    };

    int daily   = getSum("SELECT IFNULL(SUM(total),0) FROM transactions WHERE date = DATE('now','localtime')");
    int weekly  = getSum("SELECT IFNULL(SUM(total),0) FROM transactions WHERE date >= DATE('now','localtime','weekday 1','-7 days')");
    int monthly = getSum("SELECT IFNULL(SUM(total),0) FROM transactions WHERE strftime('%Y-%m', date) = strftime('%Y-%m','now','localtime')");
    int yearly  = getSum("SELECT IFNULL(SUM(total),0) FROM transactions WHERE strftime('%Y', date) = strftime('%Y','now','localtime')");

    lblDaily_->setText(QString(
                           "Today<br><span style='font-size:24px;font-weight:900;'>฿ %1</span>"
                           ).arg(daily));
    lblWeekly_->setText(QString("This Week\n฿ %1").arg(weekly));
    lblMonthly_->setText(QString("This Month\n฿ %1").arg(monthly));
    lblYearly_->setText(QString("This Year\n฿ %1").arg(yearly));
}

PriceBreakdown MainWindow::calcPriceDb(CustomerType c, DayType day, HoleType hole,
                                       bool cartSingle, bool cartPair, bool caddy)
{
    PriceBreakdown b;

    QSqlQuery q;
    q.prepare(R"(
        SELECT green_fee, cart_single, cart_pair, caddy
        FROM pricing
        WHERE customer_type=? AND day_type=? AND hole_type=?
    )");
    q.addBindValue(toString(c));
    q.addBindValue(toString(day));
    q.addBindValue(toString(hole));

    if (!q.exec() || !q.next()) {
        // ถ้าไม่เจอราคา ให้เป็น 0 และแจ้งเตือน (ตอน debug)
        qDebug() << "Pricing not found for" << toString(c) << toString(day) << toString(hole)
                 << "err=" << q.lastError().text();
        return b;
    }

    b.greenFee = q.value(0).toInt();
    int cs = q.value(1).toInt();
    int cp = q.value(2).toInt();
    int cd = q.value(3).toInt();

    b.cartFee  = cartSingle ? cs : (cartPair ? cp : 0);
    b.caddyFee = caddy ? cd : 0;
    return b;
}

QWidget* MainWindow::buildPricingPage()
{
    auto* root = new QWidget;
    auto* v = new QVBoxLayout(root);
    v->setContentsMargins(16,16,16,16);
    v->setSpacing(12);

    // Header row: title + buttons
    auto* top = new QHBoxLayout;
    auto* title = new QLabel("Pricing Setup");
    title->setStyleSheet("QLabel{font-size:20px;font-weight:900;}");
    top->addWidget(title);
    top->addStretch(1);

    btnPricingReload_ = new QPushButton("Reload");
    btnPricingSave_   = new QPushButton("Save");
    btnPricingReload_->setMinimumHeight(36);
    btnPricingSave_->setMinimumHeight(36);

    top->addWidget(btnPricingReload_);
    top->addWidget(btnPricingSave_);
    v->addLayout(top);

    // Table
    pricingTable_ = new QTableWidget;
    pricingTable_->setStyleSheet("QTableWidget{background:#ffffff;border:1px solid #ddd;border-radius:12px;}");
    pricingTable_->setColumnCount(8);
    pricingTable_->setHorizontalHeaderLabels({
        "ID","Customer","Day","Hole","Green Fee","Cart Single","Cart Pair","Caddy"
    });
    pricingTable_->horizontalHeader()->setStretchLastSection(true);
    pricingTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    pricingTable_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    // ID column read-only
    pricingTable_->setColumnHidden(0, true);

    v->addWidget(pricingTable_, 1);

    connect(btnPricingReload_, &QPushButton::clicked, this, &MainWindow::onPricingReload);
    connect(btnPricingSave_,   &QPushButton::clicked, this, &MainWindow::onPricingSave);

    return root;
}

void MainWindow::onPricingReload()
{
    loadPricingTable();
}

void MainWindow::loadPricingTable()
{
    if (!pricingTable_) return;

    pricingTable_->setRowCount(0);

    QSqlQuery q;
    if (!q.exec(R"(
        SELECT id, customer_type, day_type, hole_type,
               green_fee, cart_single, cart_pair, caddy
        FROM pricing
        ORDER BY customer_type, day_type, hole_type
    )")) {
        QMessageBox::warning(this, "Pricing", "Load pricing failed:\n" + q.lastError().text());
        return;
    }

    while (q.next()) {
        const int row = pricingTable_->rowCount();
        pricingTable_->insertRow(row);

        auto setText = [&](int col, const QString& s, bool editable){
            auto* it = new QTableWidgetItem(s);
            if (!editable) it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            pricingTable_->setItem(row, col, it);
        };

        setText(0, q.value(0).toString(), false); // id hidden
        setText(1, q.value(1).toString(), false); // customer
        setText(2, q.value(2).toString(), false); // day
        setText(3, q.value(3).toString(), false); // hole

        // editable numeric fields
        setText(4, q.value(4).toString(), true);
        setText(5, q.value(5).toString(), true);
        setText(6, q.value(6).toString(), true);
        setText(7, q.value(7).toString(), true);
    }

    // UI polish
    pricingTable_->resizeColumnsToContents();
}

static bool parseIntCell(QTableWidget* t, int row, int col, int& out)
{
    auto* it = t->item(row, col);
    if (!it) return false;
    bool ok = false;
    int v = it->text().trimmed().toInt(&ok);
    if (!ok || v < 0) return false;
    out = v;
    return true;
}

bool MainWindow::savePricingTable()
{
    if (!pricingTable_) return false;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        QMessageBox::warning(this, "Pricing", "Database is not open.");
        return false;
    }

    db.transaction();

    for (int r = 0; r < pricingTable_->rowCount(); ++r) {
        const int id = pricingTable_->item(r, 0)->text().toInt();

        int green=0, cs=0, cp=0, caddy=0;
        if (!parseIntCell(pricingTable_, r, 4, green) ||
            !parseIntCell(pricingTable_, r, 5, cs) ||
            !parseIntCell(pricingTable_, r, 6, cp) ||
            !parseIntCell(pricingTable_, r, 7, caddy)) {

            db.rollback();
            QMessageBox::warning(this, "Pricing",
                                 QString("Invalid number at row %1 (must be integer >= 0).").arg(r+1));
            return false;
        }

        QSqlQuery q;
        q.prepare(R"(
            UPDATE pricing
            SET green_fee=?, cart_single=?, cart_pair=?, caddy=?
            WHERE id=?
        )");
        q.addBindValue(green);
        q.addBindValue(cs);
        q.addBindValue(cp);
        q.addBindValue(caddy);
        q.addBindValue(id);

        if (!q.exec()) {
            db.rollback();
            QMessageBox::warning(this, "Pricing", "Save failed:\n" + q.lastError().text());
            return false;
        }
    }

    db.commit();
    return true;
}

void MainWindow::onPricingSave()
{
    if (!savePricingTable()) return;

    QMessageBox::information(this, "Pricing", "Pricing updated successfully.");

    // Optional: refresh preview + dashboard summary after price update
    onAnyChanged();
}

void MainWindow::applyTheme(ThemeMode mode)
{
    // ===== NKRAFA Core Colors =====
    const QString nkGreen = (mode == ThemeMode::NKRAFA_Dark) ? "#0B1A14" : "#0F2A20";
    const QString nkGreen2= (mode == ThemeMode::NKRAFA_Dark) ? "#10261E" : "#15382B";
    const QString nkGold  = "#C8A24A";
    const QString bg      = (mode == ThemeMode::NKRAFA_Dark) ? "#0A0F0D" : "#F4F6F5";
    const QString card    = (mode == ThemeMode::NKRAFA_Dark) ? "#121A17" : "#FFFFFF";
    const QString text    = (mode == ThemeMode::NKRAFA_Dark) ? "#E7ECEA" : "#10201A";
    const QString border  = (mode == ThemeMode::NKRAFA_Dark) ? "rgba(255,255,255,0.10)" : "rgba(0,0,0,0.08)";
    const QString hover   = (mode == ThemeMode::NKRAFA_Dark) ? "rgba(200,162,74,0.18)" : "rgba(200,162,74,0.14)";

    // ✅ เอา QApplication instance มาใช้ (แก้ error setPalette / setStyleSheet)
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) {
        // กรณีแปลก ๆ (ไม่ควรเกิดใน Qt Widgets) ก็ไม่ทำอะไรเพื่อไม่ให้ crash
        return;
    }

    // ===== Palette =====
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(bg));
    pal.setColor(QPalette::WindowText, QColor(text));
    pal.setColor(QPalette::Base, QColor(card));
    pal.setColor(QPalette::AlternateBase, QColor(mode == ThemeMode::NKRAFA_Dark ? "#0F1513" : "#F0F3F2"));
    pal.setColor(QPalette::Text, QColor(text));
    pal.setColor(QPalette::Button, QColor(card));
    pal.setColor(QPalette::ButtonText, QColor(text));
    pal.setColor(QPalette::Highlight, QColor(nkGold));
    pal.setColor(QPalette::HighlightedText, QColor(mode == ThemeMode::NKRAFA_Dark ? "#0A0F0D" : "#0F2A20"));

    app->setPalette(pal);

    // ===== Global StyleSheet =====
    const QString css = QString(R"(
/* App base */
QWidget{
    font-family:"Segoe UI";
    color:%1;
}

/* Toolbar */
QToolBar{
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %2, stop:1 %3);
    spacing:8px;
    padding:6px;
    border:0px;
}
QToolButton{
    color:#ffffff;
    background:transparent;
    border:1px solid rgba(255,255,255,0.22);
    border-radius:10px;
    padding:7px 14px;
    font-weight:700;
}
QToolButton:hover{ background:rgba(255,255,255,0.12); }
QToolButton:pressed{ background:rgba(255,255,255,0.18); }

/* GroupBox */
QGroupBox{
    background:%4;
    border:1px solid %5;
    border-radius:16px;
    margin-top:10px;
    font-weight:800;
}
QGroupBox::title{
    subcontrol-origin: margin;
    left:12px;
    padding:0 6px;
    color:%1;
}

/* Inputs */
QLineEdit, QComboBox, QDateEdit, QSpinBox{
    background:%4;
    border:1px solid %5;
    border-radius:10px;
    padding:6px 10px;
}
QComboBox::drop-down{ border:0px; }

/* Buttons */
QPushButton{
    background:%4;
    border:1px solid %5;
    border-radius:12px;
    padding:8px 12px;
    font-weight:800;
}
QPushButton:hover{
    border:1px solid %6;
    background:%7;
}
QPushButton:pressed{
    background:rgba(200,162,74,0.22);
}

/* KPI Cards */
QLabel[kpiCard="true"]{
    background:%4;
    border:1px solid %5;
    border-left:8px solid %6;
    border-radius:18px;
    padding:14px;
}

/* Tables */
QTableWidget, QTableView{
    background:%4;
    border:1px solid %5;
    border-radius:16px;
    gridline-color:%5;
}
QHeaderView::section{
    background:%4;
    border:0px;
    border-bottom:1px solid %5;
    padding:8px 10px;
    font-weight:900;
}
QTableWidget::item:selected, QTableView::item:selected{
    background:%7;
}
)")
                            .arg(text, nkGreen, nkGreen2, card, border, nkGold, hover);

    app->setStyleSheet(css);

    // ===== Optional: ปรับขนาดฟอนต์ทั้งระบบ =====
    QFont f = app->font();
    f.setPointSize(12);         // << ปรับเลขนี้ได้ตามต้องการ (เช่น 13/14)
    app->setFont(f);
}
