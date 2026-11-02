#include "gui/ReportsPanel.h"

#include "gui/GuiUtils.h"
#include "utils/FileHandler.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

ReportsPanel::ReportsPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    auto* layout = new QVBoxLayout(this);

    auto* controls = new QHBoxLayout();
    reportCombo_ = new QComboBox();
    reportCombo_->setObjectName("reportCombo");
    reportCombo_->addItems({"Most Frequently Attended Patients", "Patients with Pending Appointments",
                            "Patient Category Distribution", "Daily Appointment Summary"});
    reportCombo_->setToolTip("Choose which report to generate");
    dateLabel_ = new QLabel("Date:");
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip("Day to summarise");
    generateButton_ = new QPushButton("Generate Report");
    generateButton_->setToolTip("Build the selected report from the current data");
    exportButton_ = new QPushButton("Export to File...");
    exportButton_->setToolTip("Save the report as a text file (file chooser)");
    controls->addWidget(new QLabel("Report:"));
    controls->addWidget(reportCombo_, 1);
    controls->addWidget(dateLabel_);
    controls->addWidget(dateEdit_);
    controls->addWidget(generateButton_);
    controls->addWidget(exportButton_);
    layout->addLayout(controls);

    titleLabel_ = new QLabel();
    titleLabel_->setStyleSheet("font-size:16px; font-weight:bold;");
    layout->addWidget(titleLabel_);

    table_ = new QTableWidget();
    table_->setObjectName("reportTable");
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(table_, 1);

    summaryLabel_ = new QLabel();
    summaryLabel_->setStyleSheet("color:#1565c0; font-weight:bold;");
    layout->addWidget(summaryLabel_);

    connect(generateButton_, &QPushButton::clicked, this, &ReportsPanel::onGenerate);
    connect(exportButton_, &QPushButton::clicked, this, &ReportsPanel::onExport);
    connect(reportCombo_, &QComboBox::currentIndexChanged, this, &ReportsPanel::onReportTypeChanged);
    connect(dateEdit_, &QDateEdit::dateChanged, this, &ReportsPanel::onGenerate);

    onReportTypeChanged(0);
}

void ReportsPanel::refresh() {
    onGenerate();
}

void ReportsPanel::onReportTypeChanged(int index) {
    const bool daily = index == 3;
    dateLabel_->setVisible(daily);
    dateEdit_->setVisible(daily);
    onGenerate();
}

controller::Report ReportsPanel::buildReport() const {
    controller::ReportGenerator generator(manager_.database());
    switch (reportCombo_->currentIndex()) {
    case 0: return generator.mostFrequentlyAttended();
    case 1: return generator.pendingAppointments();
    case 2: return generator.categoryDistribution();
    default: return generator.dailySummary(fromQ(dateEdit_->date().toString("yyyy-MM-dd")));
    }
}

void ReportsPanel::onGenerate() {
    try {
        const controller::Report report = buildReport();
        titleLabel_->setText(toQ(report.title));
        summaryLabel_->setText(toQ(report.summary));
        table_->clear();
        table_->setColumnCount(static_cast<int>(report.headers.size()));
        table_->setRowCount(static_cast<int>(report.rows.size()));
        QStringList headers;
        for (const std::string& header : report.headers) {
            headers << toQ(header);
        }
        table_->setHorizontalHeaderLabels(headers);
        for (int row = 0; row < static_cast<int>(report.rows.size()); ++row) {
            const auto& cells = report.rows[static_cast<std::size_t>(row)];
            for (int column = 0; column < static_cast<int>(cells.size()); ++column) {
                QString text = toQ(cells[static_cast<std::size_t>(column)]);
                if (report.headers[static_cast<std::size_t>(column)] == "Chart") {
                    text.replace('#', QChar(0x2588)); // draw a solid bar
                }
                QColor background = Qt::white;
                if (reportCombo_->currentIndex() == 2) {
                    background = categoryBackground(static_cast<model::PatientCategory>(row));
                }
                table_->setItem(row, column, makeItem(text, background));
            }
        }
        table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table_->horizontalHeader()->setStretchLastSection(true);
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void ReportsPanel::onExport() {
    try {
        const controller::Report report = buildReport();
        const QString path = QFileDialog::getSaveFileName(this, "Export Report", "report.txt",
                                                          "Text files (*.txt);;All files (*)");
        if (path.isEmpty()) {
            return; // user cancelled the dialog
        }
        utils::FileHandler::writeText(fromQ(path), controller::ReportGenerator::toText(report));
        emit dataChanged("Exported report to " + path);
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

} // namespace scapms::gui
