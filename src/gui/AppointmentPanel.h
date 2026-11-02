#pragma once
// Appointments tab: schedule, cancel, complete and remove appointments.

#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"

#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimeEdit;

namespace scapms::gui {

class AppointmentPanel : public QWidget {
    Q_OBJECT
public:
    AppointmentPanel(controller::ClinicManager& manager,
                     controller::AppointmentController& appointments, QWidget* parent = nullptr);

signals:
    void dataChanged(const QString& message);

public slots:
    void refresh();

private slots:
    void onSchedule();
    void onCancel();
    void onComplete();
    void onRemove();
    void onPatientChanged(int index);
    void onFormEdited();

private:
    std::string selectedAppointmentId() const;
    void populatePatients();
    void populateTable();

    controller::ClinicManager& manager_;
    controller::AppointmentController& appointments_;

    QComboBox* patientCombo_;
    QDateEdit* dateEdit_;
    QTimeEdit* timeEdit_;
    QLineEdit* doctorEdit_;
    QLineEdit* reasonEdit_;
    QLabel* ruleLabel_;
    QPushButton* scheduleButton_;

    QComboBox* statusFilter_;
    QTableWidget* table_;
    QPushButton* cancelButton_;
    QPushButton* completeButton_;
    QPushButton* removeButton_;
};

} // namespace scapms::gui
