#pragma once
// Main application window: menus, tabs, status bar, shortcuts and the
// appointment reminder timer. It wires the panels to the controllers.

#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"

#include <QMainWindow>

#include <set>
#include <string>

class QLabel;
class QTabWidget;
class QTimer;

namespace scapms::gui {

class DashboardPanel;
class PatientPanel;
class AppointmentPanel;
class QueuePanel;
class SearchPanel;
class ReportsPanel;
class AdminPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void loadStartupData();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNew();
    void onOpen();
    bool onSave();
    bool onSaveAs();
    void onUndo();
    void onLoadSampleData();
    void onAbout();
    void onFind();
    void onDataChanged(const QString& message);
    void onReminderTimer();
    void refreshAll();

private:
    void buildMenus();
    void buildStatusBar();
    bool confirmDiscard();
    void loadFile(const QString& path);
    void setCurrentFile(const QString& path);

    controller::ClinicManager manager_;
    controller::AppointmentController appointments_;

    QTabWidget* tabs_;
    DashboardPanel* dashboard_;
    PatientPanel* patientPanel_;
    AppointmentPanel* appointmentPanel_;
    QueuePanel* queuePanel_;
    SearchPanel* searchPanel_;
    ReportsPanel* reportsPanel_;
    AdminPanel* adminPanel_;

    QLabel* patientsStatus_;
    QLabel* queueStatus_;
    QLabel* fileStatus_;
    QTimer* reminderTimer_;

    QString currentFile_;
    bool dirty_ = false;
    std::set<std::string> reminded_;
};

} // namespace scapms::gui
