#include "gui/PatientPanel.h"

#include "gui/GuiUtils.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "model/PatientFactory.h"
#include "utils/IDGenerator.h"
#include "utils/StringUtils.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using model::PatientCategory;

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    auto* layout = new QHBoxLayout(this); // table on the left, form on the right

    // ---------- left: filter + table ----------
    auto* left = new QVBoxLayout();
    filterEdit_ = new QLineEdit();
    filterEdit_->setPlaceholderText("Type to filter the table by name or ID...");
    filterEdit_->setToolTip("Filters the rows as you type (text field event)");
    filterEdit_->setClearButtonEnabled(true);
    left->addWidget(filterEdit_);

    table_ = new QTableWidget(0, 7);
    table_->setObjectName("patientTable");
    table_->setHorizontalHeaderLabels({"ID", "Name", "Age", "Contact", "Category", "Priority", "Details"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setToolTip("Click a row to load the patient into the form");
    left->addWidget(table_);

    auto* legend = new QLabel(
        "<span style='background:#ffcdd2;color:#b71c1c'>&nbsp;Emergency: High Priority&nbsp;</span> "
        "<span style='background:#dcedc8;color:#2e7d32'>&nbsp;General: Normal&nbsp;</span> "
        "<span style='background:#c5e1fa;color:#1565c0'>&nbsp;Follow-Up: Scheduled&nbsp;</span>");
    left->addWidget(legend);
    layout->addLayout(left, 3);

    // ---------- right: form (QFormLayout) + buttons (QGridLayout) ----------
    auto* formBox = new QGroupBox("Patient Details");
    auto* formBoxLayout = new QVBoxLayout(formBox);
    form_ = new QFormLayout();

    idEdit_ = new QLineEdit();
    idEdit_->setObjectName("idEdit");
    idEdit_->setToolTip("Leave blank to generate the next ID automatically (e.g. P0016)");
    typeCombo_ = new QComboBox();
    typeCombo_->setObjectName("typeCombo");
    typeCombo_->addItems({"General", "Emergency", "Follow-Up"});
    typeCombo_->setToolTip("Changing the type adds or removes the matching fields below");
    nameEdit_ = new QLineEdit();
    nameEdit_->setObjectName("nameEdit");
    nameEdit_->setPlaceholderText("e.g. Adaeze Okafor");
    ageEdit_ = new QLineEdit();
    ageEdit_->setObjectName("ageEdit");
    ageEdit_->setPlaceholderText("0 - 120");
    contactEdit_ = new QLineEdit();
    contactEdit_->setObjectName("contactEdit");
    contactEdit_->setPlaceholderText("Phone number or email");

    form_->addRow("Patient ID:", idEdit_);
    form_->addRow("Patient Type:", typeCombo_);
    form_->addRow("Full Name:", nameEdit_);
    form_->addRow("Age:", ageEdit_);
    form_->addRow("Contact:", contactEdit_);
    baseRowCount_ = form_->rowCount();
    formBoxLayout->addLayout(form_);

    ruleLabel_ = new QLabel();
    ruleLabel_->setWordWrap(true);
    ruleLabel_->setStyleSheet("color:#555; font-style:italic;");
    formBoxLayout->addWidget(ruleLabel_);

    auto* buttons = new QGridLayout();
    addButton_ = new QPushButton("Add Patient");
    addButton_->setObjectName("addPatientButton");
    addButton_->setToolTip("Register a new patient from the form");
    updateButton_ = new QPushButton("Update");
    updateButton_->setToolTip("Save changes to the selected patient (can be undone)");
    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setToolTip("Delete the selected patient and their appointments (can be undone)");
    clearButton_ = new QPushButton("Clear Form");
    clearButton_->setToolTip("Empty the form to register a new patient");
    queueButton_ = new QPushButton("Add to Waiting Queue");
    queueButton_->setToolTip("Put the selected patient at the back of the waiting queue");
    buttons->addWidget(addButton_, 0, 0);
    buttons->addWidget(updateButton_, 0, 1);
    buttons->addWidget(deleteButton_, 1, 0);
    buttons->addWidget(clearButton_, 1, 1);
    buttons->addWidget(queueButton_, 2, 0, 1, 2);
    formBoxLayout->addLayout(buttons);
    formBoxLayout->addStretch();
    layout->addWidget(formBox, 2);

    // ---------- events ----------
    connect(typeCombo_, &QComboBox::currentIndexChanged, this, &PatientPanel::onTypeChanged);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &PatientPanel::onSelectionChanged);
    connect(addButton_, &QPushButton::clicked, this, &PatientPanel::onAdd);
    connect(updateButton_, &QPushButton::clicked, this, &PatientPanel::onUpdate);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::onDelete);
    connect(clearButton_, &QPushButton::clicked, this, &PatientPanel::onClear);
    connect(queueButton_, &QPushButton::clicked, this, &PatientPanel::onAddToQueue);
    connect(filterEdit_, &QLineEdit::textChanged, this, &PatientPanel::onFilterChanged);
    connect(nameEdit_, &QLineEdit::textChanged, this, &PatientPanel::validateNameLive);
    connect(ageEdit_, &QLineEdit::textChanged, this, &PatientPanel::validateAgeLive);

    rebuildDynamicFields(PatientCategory::General);
    onClear();
    refresh();
}

void PatientPanel::refresh() {
    const QString keepId = toQ(selectedPatientId());
    table_->blockSignals(true);
    table_->setRowCount(0);
    for (const auto& patient : manager_.database().patients()) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QColor background = categoryBackground(patient->getCategory());
        table_->setItem(row, 0, makeItem(toQ(patient->getId()), background));
        table_->setItem(row, 1, makeItem(toQ(patient->getName()), background));
        table_->setItem(row, 2, makeItem(QString::number(patient->getAge()), background));
        table_->setItem(row, 3, makeItem(toQ(patient->getContact()), background));
        table_->setItem(row, 4, makeItem(toQ(patient->getCategoryName()), background));
        auto* priority = makeItem(toQ(patient->getPriorityLabel()), background);
        QFont bold = priority->font();
        bold.setBold(true);
        priority->setFont(bold);
        priority->setForeground(categoryAccent(patient->getCategory()));
        table_->setItem(row, 5, priority);
        table_->setItem(row, 6, makeItem(toQ(patient->getDetails()), background));
        if (toQ(patient->getId()) == keepId) {
            table_->selectRow(row);
        }
    }
    table_->blockSignals(false);
    onFilterChanged(filterEdit_->text());
    if (idEdit_->text().isEmpty() || selectedPatientId().empty()) {
        idEdit_->setPlaceholderText("Auto: " + toQ(utils::IDGenerator::peekNextPatientId()));
    }
}

// Dynamic components: remove the old type-specific rows and add new ones.
void PatientPanel::rebuildDynamicFields(PatientCategory category) {
    while (form_->rowCount() > baseRowCount_) {
        form_->removeRow(form_->rowCount() - 1); // deletes the label and the widget
    }
    symptomsEdit_ = nullptr;
    conditionEdit_ = nullptr;
    severitySpin_ = nullptr;
    diagnosisEdit_ = nullptr;
    lastVisitEdit_ = nullptr;

    switch (category) {
    case PatientCategory::General:
        symptomsEdit_ = new QLineEdit();
        symptomsEdit_->setObjectName("symptomsEdit");
        symptomsEdit_->setPlaceholderText("e.g. Fever and headache");
        form_->addRow("Symptoms:", symptomsEdit_);
        ruleLabel_->setText("General patients can book any future appointment slot.");
        break;
    case PatientCategory::Emergency:
        conditionEdit_ = new QLineEdit();
        conditionEdit_->setObjectName("conditionEdit");
        conditionEdit_->setPlaceholderText("e.g. Severe bleeding");
        severitySpin_ = new QSpinBox();
        severitySpin_->setRange(1, 5);
        severitySpin_->setValue(3);
        severitySpin_->setToolTip("1 = Minor, 5 = Critical");
        form_->addRow("Condition:", conditionEdit_);
        form_->addRow("Severity (1-5):", severitySpin_);
        ruleLabel_->setText("Emergency patients are seen through the waiting queue and cannot be booked.");
        break;
    case PatientCategory::FollowUp:
        diagnosisEdit_ = new QLineEdit();
        diagnosisEdit_->setPlaceholderText("e.g. Hypertension");
        lastVisitEdit_ = new QDateEdit(QDate::currentDate());
        lastVisitEdit_->setCalendarPopup(true);
        lastVisitEdit_->setDisplayFormat("yyyy-MM-dd");
        form_->addRow("Previous Diagnosis:", diagnosisEdit_);
        form_->addRow("Last Visit Date:", lastVisitEdit_);
        ruleLabel_->setText("Follow-up appointments must be dated after the last visit.");
        break;
    }
}

void PatientPanel::onTypeChanged(int index) {
    rebuildDynamicFields(static_cast<PatientCategory>(index));
}

std::string PatientPanel::selectedPatientId() const {
    const auto selected = table_->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return "";
    }
    return fromQ(table_->item(selected.first().row(), 0)->text());
}

void PatientPanel::onSelectionChanged() {
    const std::string id = selectedPatientId();
    if (id.empty()) {
        return;
    }
    model::PatientRecord* patient = manager_.getPatient(id); // goes through the cache
    if (patient == nullptr) {
        return;
    }
    idEdit_->setText(toQ(patient->getId()));
    idEdit_->setReadOnly(true);
    typeCombo_->setCurrentIndex(static_cast<int>(patient->getCategory())); // rebuilds fields
    nameEdit_->setText(toQ(patient->getName()));
    ageEdit_->setText(QString::number(patient->getAge()));
    contactEdit_->setText(toQ(patient->getContact()));

    // dynamic_cast picks the subclass to fill the type-specific fields.
    if (auto* general = dynamic_cast<model::GeneralPatient*>(patient)) {
        symptomsEdit_->setText(toQ(general->getSymptoms()));
        ruleLabel_->setText(toQ(general->bookingRule()));
    } else if (auto* emergency = dynamic_cast<model::EmergencyPatient*>(patient)) {
        conditionEdit_->setText(toQ(emergency->getCondition()));
        severitySpin_->setValue(emergency->getSeverity());
    } else if (auto* followUp = dynamic_cast<model::FollowUpPatient*>(patient)) {
        diagnosisEdit_->setText(toQ(followUp->getPreviousDiagnosis()));
        lastVisitEdit_->setDate(QDate::fromString(toQ(followUp->getLastVisitDate()), "yyyy-MM-dd"));
        ruleLabel_->setText(toQ(followUp->bookingRule()));
    }
}

std::unique_ptr<model::PatientRecord> PatientPanel::buildFromForm(const std::string& id) const {
    bool ok = false;
    const int age = ageEdit_->text().trimmed().toInt(&ok);
    if (!ok) {
        throw utils::ValidationException("Patient age must be a whole number between 0 and 120.");
    }
    const auto category = static_cast<PatientCategory>(typeCombo_->currentIndex());
    std::vector<std::string> extras;
    switch (category) {
    case PatientCategory::General:
        extras = {fromQ(symptomsEdit_->text())};
        break;
    case PatientCategory::Emergency:
        extras = {fromQ(conditionEdit_->text()), std::to_string(severitySpin_->value())};
        break;
    case PatientCategory::FollowUp:
        extras = {fromQ(diagnosisEdit_->text()), fromQ(lastVisitEdit_->date().toString("yyyy-MM-dd"))};
        break;
    }
    return model::createPatient(category, id, fromQ(nameEdit_->text()), age,
                                fromQ(contactEdit_->text()), extras);
}

void PatientPanel::onAdd() {
    try {
        std::string id = utils::toUpper(fromQ(idEdit_->text()));
        if (idEdit_->isReadOnly()) {
            id.clear(); // a patient is selected: Add creates a new record instead
        }
        if (id.empty()) {
            id = manager_.generatePatientId();
        }
        auto patient = buildFromForm(id); // throws ValidationException on bad input
        const QString name = toQ(patient->getName());
        manager_.registerPatient(std::move(patient)); // throws DuplicateIdException
        onClear();
        emit dataChanged(QString("Registered %1 (%2)").arg(name, toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void PatientPanel::onUpdate() {
    try {
        const std::string id = selectedPatientId();
        if (id.empty()) {
            throw utils::ValidationException("Select a patient in the table before updating.");
        }
        manager_.updatePatient(buildFromForm(id));
        emit dataChanged(QString("Updated patient %1").arg(toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void PatientPanel::onDelete() {
    try {
        const std::string id = selectedPatientId();
        if (id.empty()) {
            throw utils::ValidationException("Select a patient in the table before deleting.");
        }
        const auto answer = QMessageBox::question(
            this, "Delete Patient",
            QString("Delete patient %1 and all of their appointments?\nYou can undo this with Ctrl+Z.")
                .arg(toQ(id)));
        if (answer != QMessageBox::Yes) {
            return;
        }
        manager_.deletePatient(id);
        onClear();
        emit dataChanged(QString("Deleted patient %1").arg(toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void PatientPanel::onClear() {
    table_->clearSelection();
    idEdit_->clear();
    idEdit_->setReadOnly(false);
    idEdit_->setPlaceholderText("Auto: " + toQ(utils::IDGenerator::peekNextPatientId()));
    nameEdit_->clear();
    ageEdit_->clear();
    contactEdit_->clear();
    typeCombo_->setCurrentIndex(0);
    rebuildDynamicFields(PatientCategory::General);
    nameEdit_->setStyleSheet("");
    ageEdit_->setStyleSheet("");
}

void PatientPanel::onAddToQueue() {
    try {
        const std::string id = selectedPatientId();
        if (id.empty()) {
            throw utils::ValidationException("Select a patient in the table to add to the queue.");
        }
        manager_.enqueue(id);
        emit dataChanged(QString("Added %1 to the waiting queue").arg(toQ(id)));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void PatientPanel::onFilterChanged(const QString& text) {
    const QString term = text.trimmed();
    for (int row = 0; row < table_->rowCount(); ++row) {
        const bool match = term.isEmpty() ||
                           table_->item(row, 0)->text().contains(term, Qt::CaseInsensitive) ||
                           table_->item(row, 1)->text().contains(term, Qt::CaseInsensitive);
        table_->setRowHidden(row, !match);
    }
}

void PatientPanel::markField(QLineEdit* field, bool valid, const QString& tip) {
    field->setStyleSheet(valid ? "" : "border: 2px solid #c62828;");
    field->setToolTip(tip);
}

void PatientPanel::validateNameLive(const QString& text) {
    if (text.isEmpty()) {
        markField(nameEdit_, true, "");
        return;
    }
    try {
        model::PatientRecord::validateName(fromQ(text));
        markField(nameEdit_, true, "Name looks good");
    } catch (const utils::ValidationException& error) {
        markField(nameEdit_, false, error.what());
    }
}

void PatientPanel::validateAgeLive(const QString& text) {
    if (text.isEmpty()) {
        markField(ageEdit_, true, "");
        return;
    }
    bool ok = false;
    const int age = text.trimmed().toInt(&ok);
    const bool valid = ok && age >= 0 && age <= 120;
    markField(ageEdit_, valid, valid ? "Age looks good" : "Patient age must be between 0 and 120.");
}

} // namespace scapms::gui
