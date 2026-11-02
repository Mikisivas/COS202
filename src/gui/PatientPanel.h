#pragma once
// Patient Records tab: view, add, edit and delete patients.
// The form adds and removes type-specific fields when the patient type changes.

#include "controller/ClinicManager.h"

#include <QWidget>

#include <memory>

class QComboBox;
class QDateEdit;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace scapms::gui {

class PatientPanel : public QWidget {
    Q_OBJECT
public:
    explicit PatientPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

signals:
    void dataChanged(const QString& message);

public slots:
    void refresh();

private slots:
    void onTypeChanged(int index);
    void onSelectionChanged();
    void onAdd();
    void onUpdate();
    void onDelete();
    void onClear();
    void onAddToQueue();
    void onFilterChanged(const QString& text);
    void validateNameLive(const QString& text);
    void validateAgeLive(const QString& text);

private:
    void rebuildDynamicFields(model::PatientCategory category);
    std::unique_ptr<model::PatientRecord> buildFromForm(const std::string& id) const;
    std::string selectedPatientId() const;
    void markField(QLineEdit* field, bool valid, const QString& tip);

    controller::ClinicManager& manager_;

    QLineEdit* filterEdit_;
    QTableWidget* table_;

    QFormLayout* form_;
    int baseRowCount_ = 0;
    QLineEdit* idEdit_;
    QComboBox* typeCombo_;
    QLineEdit* nameEdit_;
    QLineEdit* ageEdit_;
    QLineEdit* contactEdit_;
    QLabel* ruleLabel_;

    // Dynamic fields: created and destroyed by rebuildDynamicFields().
    QLineEdit* symptomsEdit_ = nullptr;
    QLineEdit* conditionEdit_ = nullptr;
    QSpinBox* severitySpin_ = nullptr;
    QLineEdit* diagnosisEdit_ = nullptr;
    QDateEdit* lastVisitEdit_ = nullptr;

    QPushButton* addButton_;
    QPushButton* updateButton_;
    QPushButton* deleteButton_;
    QPushButton* clearButton_;
    QPushButton* queueButton_;
};

} // namespace scapms::gui
