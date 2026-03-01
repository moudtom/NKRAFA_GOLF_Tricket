#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QLabel>
#include <QDateEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLineEdit>
#include <QTableWidget>

#include "pricing.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent=nullptr);

private:
    // pages
    QStackedWidget* stack_{};
    QWidget* pagePOS_{};
    QWidget* pageDash_{};

    // POS widgets
    QDateEdit* dateEdit_{};
    QLabel* dayTypeLabel_{};
    QComboBox* customerCombo_{};
    QRadioButton* rb18_{};
    QRadioButton* rb9_{};
    QCheckBox* cbCartSingle_{};
    QCheckBox* cbCartPair_{};
    QCheckBox* cbCaddy_{};
    QLabel* lblGreen_{};
    QLabel* lblCart_{};
    QLabel* lblCaddy_{};
    QLabel* lblTotal_{};
    QPushButton* btnConfirm_{};
    QPushButton* btnPrint_{};

    // Dashboard widgets
    QLabel* kpiRevenue_{};
    QLabel* kpiPlayers_{};
    QLabel* kpi18_{};
    QLabel* kpi9_{};
    QLabel* kpiCart_{};
    QLabel* kpiCaddy_{};
    QTableWidget* table_{};
    QLabel* lblDaily_{};
    QLabel* lblWeekly_{};
    QLabel* lblMonthly_{};
    QLabel* lblYearly_{};

    // History page
    QWidget* pageHistory_{};
    QDateEdit* dateFrom_{};
    QDateEdit* dateTo_{};
    QLineEdit* editOperator_{};
    QTableView* historyView_{};
    QSqlQueryModel* historyModel_{};
    QPushButton* btnSearch_{};
    QPushButton* btnExportCsv_{};

    // state
    int totalRevenue_ = 0;
    int totalPlayers_ = 0;
    int total18_ = 0;
    int total9_  = 0;
    int totalCart_ = 0;
    int totalCaddy_ = 0;

    // Pricing page
    QWidget* pagePricing_{};
    QTableWidget* pricingTable_{};
    QPushButton* btnPricingReload_{};
    QPushButton* btnPricingSave_{};

    QWidget* buildPricingPage();
    void loadPricingTable();
    bool savePricingTable();



    // builders & helpers
    void buildUi();
    QWidget* buildPosPage();
    QWidget* buildDashPage();
    QWidget* buildHistoryPage();
    void refreshHistory();

    void updateDayType();
    void updatePricePreview();
    void addTransactionToDashboard(const PriceBreakdown& b, DayType d, HoleType h, CustomerType c);
    void updateFinancialSummary();
    PriceBreakdown calcPriceDb(CustomerType c, DayType day, HoleType hole,
                               bool cartSingle, bool cartPair, bool caddy);

    enum class ThemeMode { NKRAFA_Light, NKRAFA_Dark };
    void applyTheme(ThemeMode mode);

private slots:
    void onAnyChanged();
    void onConfirm();
    void onPrintTicket();
    void onSearchHistory();
    void onExportCsv();
    void onPricingReload();
    void onPricingSave();

};
