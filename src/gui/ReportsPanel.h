#pragma once
// Reports tab: four clinic reports, shown as tables and exportable to text.

#include "controller/ClinicManager.h"
#include "controller/ReportGenerator.h"

#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

class ReportsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ReportsPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

signals:
    void dataChanged(const QString& message);

public slots:
    void refresh();

private slots:
    void onGenerate();
    void onExport();
    void onReportTypeChanged(int index);

private:
    controller::Report buildReport() const;

    controller::ClinicManager& manager_;
    QComboBox* reportCombo_;
    QDateEdit* dateEdit_;
    QLabel* dateLabel_;
    QPushButton* generateButton_;
    QPushButton* exportButton_;
    QLabel* titleLabel_;
    QLabel* summaryLabel_;
    QTableWidget* table_;
};

} // namespace scapms::gui
