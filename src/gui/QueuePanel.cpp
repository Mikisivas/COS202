#include "gui/QueuePanel.h"

#include "gui/GuiUtils.h"

#include <QComboBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <ctime>

namespace scapms::gui {

namespace {
QString formatWait(std::time_t joinedAt) {
    const long seconds = static_cast<long>(std::difftime(std::time(nullptr), joinedAt));
    return QString("%1:%2").arg(seconds / 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
}
} // namespace

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    auto* layout = new QVBoxLayout(this);

    auto* controls = new QGroupBox("Queue Controls");
    auto* controlsLayout = new QHBoxLayout(controls);
    patientCombo_ = new QComboBox();
    patientCombo_->setObjectName("queuePatientCombo");
    patientCombo_->setMinimumWidth(280);
    patientCombo_->setToolTip("Choose a registered patient to join the queue");
    addButton_ = new QPushButton("Add to Queue");
    addButton_->setObjectName("queueAddButton");
    addButton_->setToolTip("Patient joins at the back of the queue");
    callButton_ = new QPushButton("Call Next Patient");
    callButton_->setObjectName("callNextButton");
    callButton_->setToolTip("Serve the patient at the front of the queue (first in, first out)");
    callButton_->setStyleSheet("font-weight:bold;");
    alertMinutes_ = new QSpinBox();
    alertMinutes_->setRange(1, 240);
    alertMinutes_->setValue(15);
    alertMinutes_->setSuffix(" min");
    alertMinutes_->setToolTip("Alert staff when a patient waits longer than this");
    controlsLayout->addWidget(new QLabel("Patient:"));
    controlsLayout->addWidget(patientCombo_, 1);
    controlsLayout->addWidget(addButton_);
    controlsLayout->addWidget(callButton_);
    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(new QLabel("Alert after:"));
    controlsLayout->addWidget(alertMinutes_);
    layout->addWidget(controls);

    servingLabel_ = new QLabel("Now serving: nobody");
    servingLabel_->setObjectName("servingLabel");
    servingLabel_->setStyleSheet("font-size:18px; font-weight:bold; color:#1b5e20; "
                                 "background:#e8f5e9; padding:10px; border-radius:4px;");
    layout->addWidget(servingLabel_);

    countLabel_ = new QLabel();
    layout->addWidget(countLabel_);

    table_ = new QTableWidget(0, 6);
    table_->setObjectName("queueTable");
    table_->setHorizontalHeaderLabels({"Position", "Patient ID", "Name", "Category", "Priority", "Waiting (mm:ss)"});
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table_, 1);

    connect(addButton_, &QPushButton::clicked, this, &QueuePanel::onAddToQueue);
    connect(callButton_, &QPushButton::clicked, this, &QueuePanel::onCallNext);

    // Timer event: update waiting times every second.
    waitTimer_ = new QTimer(this);
    connect(waitTimer_, &QTimer::timeout, this, &QueuePanel::updateWaitingTimes);
    waitTimer_->start(1000);

    refresh();
}

void QueuePanel::refresh() {
    const QString keep = patientCombo_->currentData().toString();
    patientCombo_->clear();
    for (const auto& patient : manager_.database().patients()) {
        patientCombo_->addItem(toQ(patient->getId() + " - " + patient->getName() + " (" +
                                   patient->getCategoryName() + ")"),
                               toQ(patient->getId()));
    }
    const int index = patientCombo_->findData(keep);
    if (index >= 0) {
        patientCombo_->setCurrentIndex(index);
    }

    const auto entries = manager_.queueSnapshot();
    table_->setRowCount(0);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const model::PatientRecord* patient = manager_.database().findPatient(entries[i].patientId);
        if (patient == nullptr) {
            continue;
        }
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QColor background = categoryBackground(patient->getCategory());
        table_->setItem(row, 0, makeItem(QString::number(i + 1), background));
        table_->setItem(row, 1, makeItem(toQ(patient->getId()), background));
        table_->setItem(row, 2, makeItem(toQ(patient->getName()), background));
        table_->setItem(row, 3, makeItem(toQ(patient->getCategoryName()), background));
        table_->setItem(row, 4, makeItem(toQ(patient->getPriorityLabel()), background));
        table_->setItem(row, 5, makeItem(formatWait(entries[i].joinedAt), background));
    }
    countLabel_->setText(QString("%1 patient(s) waiting. The first patient in the list is served next.")
                             .arg(entries.size()));
    callButton_->setEnabled(!entries.empty());
}

void QueuePanel::updateWaitingTimes() {
    const auto entries = manager_.queueSnapshot();
    if (static_cast<int>(entries.size()) != table_->rowCount()) {
        refresh();
        return;
    }
    const long limitSeconds = alertMinutes_->value() * 60L;
    for (int row = 0; row < table_->rowCount(); ++row) {
        const auto& entry = entries[static_cast<std::size_t>(row)];
        const long waited = static_cast<long>(std::difftime(std::time(nullptr), entry.joinedAt));
        QTableWidgetItem* waitItem = table_->item(row, 5);
        waitItem->setText(formatWait(entry.joinedAt));
        if (waited >= limitSeconds) {
            waitItem->setBackground(QColor(239, 83, 80));
            waitItem->setForeground(Qt::white);
            if (alerted_.insert(entry.patientId).second) {
                emit waitingAlert(QString("%1 has waited over %2 minutes")
                                      .arg(table_->item(row, 2)->text())
                                      .arg(alertMinutes_->value()));
            }
        }
    }
}

void QueuePanel::onAddToQueue() {
    try {
        if (patientCombo_->count() == 0) {
            throw utils::ValidationException("Register a patient before adding to the queue.");
        }
        const std::string id = fromQ(patientCombo_->currentData().toString());
        manager_.enqueue(id);
        emit dataChanged(QString("Added %1 to the waiting queue").arg(toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void QueuePanel::onCallNext() {
    try {
        const controller::QueueEntry next = manager_.callNextPatient();
        alerted_.erase(next.patientId);
        const model::PatientRecord* patient = manager_.getPatient(next.patientId);
        const QString name = patient ? toQ(patient->getName()) : toQ(next.patientId);
        servingLabel_->setText(QString("Now serving: %1 (%2), waited %3")
                                   .arg(name, toQ(next.patientId), formatWait(next.joinedAt)));
        emit dataChanged(QString("Called %1 for consultation").arg(name));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

} // namespace scapms::gui
