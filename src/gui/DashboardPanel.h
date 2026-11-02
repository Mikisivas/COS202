#pragma once
// Home tab: live statistics, reminders, recently accessed patients (cache)
// and a triage list built through base-class pointers.

#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"

#include <QWidget>

class QGridLayout;
class QLabel;
class QListWidget;
class QTimer;

namespace scapms::gui {

class DashboardPanel : public QWidget {
    Q_OBJECT
public:
    DashboardPanel(controller::ClinicManager& manager,
                   controller::AppointmentController& appointments, QWidget* parent = nullptr);

public slots:
    void refresh();
    void showReminder(const QString& message);

private:
    QLabel* addCard(QGridLayout* grid, int row, int column, const QString& title,
                    const QString& colour, const QString& tooltip);

    controller::ClinicManager& manager_;
    controller::AppointmentController& appointments_;

    QLabel* totalLabel_;
    QLabel* emergencyLabel_;
    QLabel* queueLabel_;
    QLabel* todayLabel_;
    QLabel* pendingLabel_;
    QLabel* completedLabel_;
    QLabel* reminderLabel_;
    QLabel* refreshedLabel_;
    QListWidget* todayList_;
    QListWidget* recentList_;
    QListWidget* triageList_;
    QTimer* refreshTimer_;
};

} // namespace scapms::gui
