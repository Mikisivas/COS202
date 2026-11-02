#pragma once
// Waiting Queue tab: patients wait in a std::queue (first in, first out).
// A one-second timer updates waiting times and raises long-wait alerts.

#include "controller/ClinicManager.h"

#include <QWidget>

#include <set>
#include <string>

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimer;

namespace scapms::gui {

class QueuePanel : public QWidget {
    Q_OBJECT
public:
    explicit QueuePanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

signals:
    void dataChanged(const QString& message);
    void waitingAlert(const QString& message);

public slots:
    void refresh();

private slots:
    void onAddToQueue();
    void onCallNext();
    void updateWaitingTimes();

private:
    controller::ClinicManager& manager_;
    QComboBox* patientCombo_;
    QPushButton* addButton_;
    QPushButton* callButton_;
    QSpinBox* alertMinutes_;
    QLabel* servingLabel_;
    QLabel* countLabel_;
    QTableWidget* table_;
    QTimer* waitTimer_;
    std::set<std::string> alerted_;
};

} // namespace scapms::gui
