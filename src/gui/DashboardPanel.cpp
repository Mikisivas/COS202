#include "gui/DashboardPanel.h"

#include "gui/GuiUtils.h"
#include "utils/DateUtils.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace scapms::gui {

DashboardPanel::DashboardPanel(controller::ClinicManager& manager,
                               controller::AppointmentController& appointments, QWidget* parent)
    : QWidget(parent), manager_(manager), appointments_(appointments) {
    auto* layout = new QVBoxLayout(this);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel("<h2 style='margin:0'>Clinic Dashboard</h2>");
    refreshedLabel_ = new QLabel();
    refreshedLabel_->setStyleSheet("color: #666;");
    header->addWidget(title);
    header->addStretch();
    header->addWidget(refreshedLabel_);
    layout->addLayout(header);

    // Statistic cards in a grid (QGridLayout).
    auto* grid = new QGridLayout();
    grid->setSpacing(12);
    totalLabel_ = addCard(grid, 0, 0, "Registered Patients", "#37474f", "All patients in the database");
    emergencyLabel_ = addCard(grid, 0, 1, "Emergency Patients", "#b71c1c", "Patients with High Priority");
    queueLabel_ = addCard(grid, 0, 2, "Waiting in Queue", "#e65100", "Patients waiting for consultation");
    todayLabel_ = addCard(grid, 1, 0, "Appointments Today", "#1565c0", "All appointments dated today");
    pendingLabel_ = addCard(grid, 1, 1, "Pending (All Days)", "#f9a825", "Appointments not yet completed");
    completedLabel_ = addCard(grid, 1, 2, "Completed Today", "#2e7d32", "Appointments completed today");
    layout->addLayout(grid);

    reminderLabel_ = new QLabel("No upcoming appointment reminders.");
    reminderLabel_->setObjectName("reminderLabel");
    reminderLabel_->setWordWrap(true);
    reminderLabel_->setStyleSheet("background:#e3f2fd; color:#0d47a1; padding:8px; border-radius:4px;");
    layout->addWidget(reminderLabel_);

    // Three lists side by side (QHBoxLayout).
    auto* lists = new QHBoxLayout();
    const auto makeList = [&lists](const QString& heading, const QString& tooltip) {
        auto* box = new QGroupBox(heading);
        auto* boxLayout = new QVBoxLayout(box);
        auto* list = new QListWidget();
        list->setToolTip(tooltip);
        boxLayout->addWidget(list);
        lists->addWidget(box);
        return list;
    };
    todayList_ = makeList("Today's Schedule", "Appointments for today, ordered by time");
    recentList_ = makeList("Recently Accessed (cache of 10)",
                           "Fixed-size array cache: the most recently opened patient is on top");
    triageList_ = makeList("Triage Overview (polymorphism)",
                           "Built by calling virtual functions through PatientRecord pointers");
    layout->addLayout(lists, 1);

    // Timer event: refresh the statistics every 5 seconds.
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &DashboardPanel::refresh);
    refreshTimer_->start(5000);

    refresh();
}

QLabel* DashboardPanel::addCard(QGridLayout* grid, int row, int column, const QString& title,
                                const QString& colour, const QString& tooltip) {
    auto* card = new QFrame();
    card->setToolTip(tooltip);
    card->setStyleSheet(QString("QFrame { background:%1; border-radius:6px; }"
                                "QLabel { color:white; background:transparent; }").arg(colour));
    auto* cardLayout = new QVBoxLayout(card);
    auto* value = new QLabel("0");
    value->setStyleSheet("font-size:28px; font-weight:bold;");
    auto* caption = new QLabel(title);
    cardLayout->addWidget(value);
    cardLayout->addWidget(caption);
    grid->addWidget(card, row, column);
    return value;
}

void DashboardPanel::refresh() {
    const auto& db = manager_.database();
    const std::string today = utils::today();

    int todayCount = 0;
    int pending = 0;
    int completedToday = 0;
    for (const auto& appointment : db.appointments()) {
        const bool isToday = appointment.getDate() == today;
        todayCount += isToday ? 1 : 0;
        pending += appointment.getStatus() == model::AppointmentStatus::Pending ? 1 : 0;
        completedToday += isToday && appointment.getStatus() == model::AppointmentStatus::Completed ? 1 : 0;
    }
    totalLabel_->setText(QString::number(db.patientCount()));
    emergencyLabel_->setText(QString::number(db.countByCategory(model::PatientCategory::Emergency)));
    queueLabel_->setText(QString::number(manager_.queueSize()));
    todayLabel_->setText(QString::number(todayCount));
    pendingLabel_->setText(QString::number(pending));
    completedLabel_->setText(QString::number(completedToday));

    todayList_->clear();
    for (const auto& appointment : appointments_.appointmentsOn(today)) {
        const model::PatientRecord* patient = db.findPatient(appointment.getPatientId());
        auto* item = new QListWidgetItem(
            QString("%1  %2 with %3  [%4]")
                .arg(toQ(appointment.getTime()),
                     patient ? toQ(patient->getName()) : toQ(appointment.getPatientId()),
                     toQ(appointment.getDoctor()), toQ(appointment.getStatusName())));
        item->setBackground(statusBackground(appointment.getStatus()));
        item->setForeground(QColor(33, 33, 33));
        todayList_->addItem(item);
    }
    if (todayList_->count() == 0) {
        todayList_->addItem("No appointments today.");
    }

    recentList_->clear();
    for (const model::PatientRecord* patient : manager_.recentPatients()) {
        recentList_->addItem(toQ(patient->getId() + "  " + patient->getName() + "  (" +
                                 patient->getCategoryName() + ")"));
    }
    recentList_->addItem(QString("Cache hits: %1, misses: %2")
                             .arg(manager_.cacheHits())
                             .arg(manager_.cacheMisses()));

    triageList_->clear();
    for (const std::string& line : manager_.triageOverview()) {
        auto* item = new QListWidgetItem(toQ(line));
        if (line.rfind("[High Priority]", 0) == 0) {
            item->setForeground(QColor(183, 28, 28));
        }
        triageList_->addItem(item);
    }

    refreshedLabel_->setText("Last refreshed " + QDateTime::currentDateTime().toString("hh:mm:ss"));
}

void DashboardPanel::showReminder(const QString& message) {
    reminderLabel_->setText(message);
}

} // namespace scapms::gui
