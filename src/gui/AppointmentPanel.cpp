#include "gui/AppointmentPanel.h"

#include "gui/GuiUtils.h"
#include "model/Appointable.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace scapms::gui {

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager,
                                   controller::AppointmentController& appointments,
                                   QWidget* parent)
    : QWidget(parent), manager_(manager), appointments_(appointments) {
    auto* layout = new QHBoxLayout(this);

    // ---------- booking form (QFormLayout) ----------
    auto* formBox = new QGroupBox("Schedule Appointment");
    auto* formBoxLayout = new QVBoxLayout(formBox);
    auto* form = new QFormLayout();
    patientCombo_ = new QComboBox();
    patientCombo_->setObjectName("apptPatientCombo");
    patientCombo_->setToolTip("Only General and Follow-Up patients implement Appointable");
    dateEdit_ = new QDateEdit(QDate::currentDate().addDays(1));
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip("Appointments cannot be booked in the past");
    timeEdit_ = new QTimeEdit(QTime(9, 0));
    timeEdit_->setDisplayFormat("HH:mm");
    doctorEdit_ = new QLineEdit();
    doctorEdit_->setObjectName("doctorEdit");
    doctorEdit_->setPlaceholderText("e.g. Dr. Adebayo");
    reasonEdit_ = new QLineEdit();
    reasonEdit_->setPlaceholderText("Optional: defaults from the patient's record");
    form->addRow("Patient:", patientCombo_);
    form->addRow("Date:", dateEdit_);
    form->addRow("Time:", timeEdit_);
    form->addRow("Doctor:", doctorEdit_);
    form->addRow("Reason:", reasonEdit_);
    formBoxLayout->addLayout(form);

    ruleLabel_ = new QLabel();
    ruleLabel_->setWordWrap(true);
    ruleLabel_->setStyleSheet("color:#555; font-style:italic;");
    formBoxLayout->addWidget(ruleLabel_);

    scheduleButton_ = new QPushButton("Schedule Appointment");
    scheduleButton_->setObjectName("scheduleButton");
    scheduleButton_->setToolTip("Book the appointment (needs a patient and a doctor)");
    scheduleButton_->setEnabled(false);
    formBoxLayout->addWidget(scheduleButton_);
    formBoxLayout->addStretch();
    layout->addWidget(formBox, 2);

    // ---------- appointment list ----------
    auto* listBox = new QGroupBox("Appointments");
    auto* listLayout = new QVBoxLayout(listBox);
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Show:"));
    statusFilter_ = new QComboBox();
    statusFilter_->addItems({"All", "Pending", "Completed", "Cancelled"});
    statusFilter_->setToolTip("Filter the list by appointment status");
    filterRow->addWidget(statusFilter_);
    filterRow->addStretch();
    listLayout->addLayout(filterRow);

    table_ = new QTableWidget(0, 8);
    table_->setObjectName("appointmentTable");
    table_->setHorizontalHeaderLabels({"ID", "Patient ID", "Patient", "Date", "Time", "Doctor", "Reason", "Status"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);
    listLayout->addWidget(table_);

    auto* actions = new QHBoxLayout();
    cancelButton_ = new QPushButton("Cancel Appointment");
    cancelButton_->setObjectName("cancelApptButton");
    cancelButton_->setToolTip("Mark the selected pending appointment as cancelled (can be undone)");
    completeButton_ = new QPushButton("Mark Completed");
    completeButton_->setToolTip("Record that the patient attended");
    removeButton_ = new QPushButton("Remove (Admin)");
    removeButton_->setToolTip("Delete the appointment record completely (can be undone)");
    actions->addWidget(cancelButton_);
    actions->addWidget(completeButton_);
    actions->addWidget(removeButton_);
    listLayout->addLayout(actions);
    layout->addWidget(listBox, 5);

    connect(scheduleButton_, &QPushButton::clicked, this, &AppointmentPanel::onSchedule);
    connect(cancelButton_, &QPushButton::clicked, this, &AppointmentPanel::onCancel);
    connect(completeButton_, &QPushButton::clicked, this, &AppointmentPanel::onComplete);
    connect(removeButton_, &QPushButton::clicked, this, &AppointmentPanel::onRemove);
    connect(statusFilter_, &QComboBox::currentIndexChanged, this, &AppointmentPanel::populateTable);
    connect(patientCombo_, &QComboBox::currentIndexChanged, this, &AppointmentPanel::onPatientChanged);
    connect(doctorEdit_, &QLineEdit::textChanged, this, &AppointmentPanel::onFormEdited);

    refresh();
}

void AppointmentPanel::refresh() {
    populatePatients();
    populateTable();
}

void AppointmentPanel::populatePatients() {
    const QString keep = patientCombo_->currentData().toString();
    patientCombo_->blockSignals(true);
    patientCombo_->clear();
    for (const auto& patient : manager_.database().patients()) {
        if (dynamic_cast<const model::Appointable*>(patient.get()) == nullptr) {
            continue; // emergency patients cannot be booked
        }
        patientCombo_->addItem(toQ(patient->getId() + " - " + patient->getName() + " (" +
                                   patient->getCategoryName() + ")"),
                               toQ(patient->getId()));
    }
    const int index = patientCombo_->findData(keep);
    patientCombo_->setCurrentIndex(index >= 0 ? index : 0);
    patientCombo_->blockSignals(false);
    onPatientChanged(patientCombo_->currentIndex());
}

void AppointmentPanel::populateTable() {
    const QString filter = statusFilter_->currentText();
    const auto& db = manager_.database();
    table_->setRowCount(0);
    for (const auto& appointment : db.appointments()) {
        if (filter != "All" && toQ(appointment.getStatusName()) != filter) {
            continue;
        }
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QColor background = statusBackground(appointment.getStatus());
        const model::PatientRecord* patient = db.findPatient(appointment.getPatientId());
        const QStringList cells = {toQ(appointment.getId()), toQ(appointment.getPatientId()),
                                   patient ? toQ(patient->getName()) : "(deleted)",
                                   toQ(appointment.getDate()), toQ(appointment.getTime()),
                                   toQ(appointment.getDoctor()), toQ(appointment.getReason()),
                                   toQ(appointment.getStatusName())};
        for (int column = 0; column < cells.size(); ++column) {
            table_->setItem(row, column, makeItem(cells[column], background));
        }
    }
}

void AppointmentPanel::onPatientChanged(int) {
    const std::string id = fromQ(patientCombo_->currentData().toString());
    const model::PatientRecord* patient = manager_.database().findPatient(id);
    if (const auto* appointable = dynamic_cast<const model::Appointable*>(patient)) {
        ruleLabel_->setText(toQ(appointable->bookingRule()));
    } else {
        ruleLabel_->setText("Register a General or Follow-Up patient to book appointments.");
    }
    onFormEdited();
}

void AppointmentPanel::onFormEdited() {
    scheduleButton_->setEnabled(patientCombo_->count() > 0 && doctorEdit_->text().trimmed().size() >= 2);
}

std::string AppointmentPanel::selectedAppointmentId() const {
    const auto selected = table_->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        throw utils::ValidationException("Select an appointment in the table first.");
    }
    return fromQ(table_->item(selected.first().row(), 0)->text());
}

void AppointmentPanel::onSchedule() {
    try {
        const auto appointment = appointments_.schedule(
            fromQ(patientCombo_->currentData().toString()),
            fromQ(dateEdit_->date().toString("yyyy-MM-dd")),
            fromQ(timeEdit_->time().toString("HH:mm")), fromQ(doctorEdit_->text()),
            fromQ(reasonEdit_->text()));
        reasonEdit_->clear();
        emit dataChanged(QString("Scheduled %1 on %2 at %3")
                             .arg(toQ(appointment.getId()), toQ(appointment.getDate()),
                                  toQ(appointment.getTime())));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void AppointmentPanel::onCancel() {
    try {
        const std::string id = selectedAppointmentId();
        appointments_.cancel(id);
        emit dataChanged(QString("Cancelled appointment %1").arg(toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void AppointmentPanel::onComplete() {
    try {
        const std::string id = selectedAppointmentId();
        appointments_.complete(id);
        emit dataChanged(QString("Appointment %1 marked as completed").arg(toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void AppointmentPanel::onRemove() {
    try {
        const std::string id = selectedAppointmentId();
        if (QMessageBox::question(this, "Remove Appointment",
                                  QString("Remove appointment %1 permanently?\nYou can undo this with Ctrl+Z.")
                                      .arg(toQ(id))) != QMessageBox::Yes) {
            return;
        }
        appointments_.remove(id);
        emit dataChanged(QString("Removed appointment %1").arg(toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

} // namespace scapms::gui
