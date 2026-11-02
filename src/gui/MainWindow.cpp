#include "gui/MainWindow.h"

#include "controller/SampleData.h"
#include "gui/AdminPanel.h"
#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/GuiUtils.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportsPanel.h"
#include "gui/SearchPanel.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>

namespace scapms::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), appointments_(manager_) {
    setWindowTitle("SCAPMS - Smart Clinic Appointment & Patient Management System");
    resize(1280, 780);

    tabs_ = new QTabWidget();
    tabs_->setObjectName("mainTabs");
    dashboard_ = new DashboardPanel(manager_, appointments_);
    patientPanel_ = new PatientPanel(manager_);
    appointmentPanel_ = new AppointmentPanel(manager_, appointments_);
    queuePanel_ = new QueuePanel(manager_);
    searchPanel_ = new SearchPanel(manager_);
    reportsPanel_ = new ReportsPanel(manager_);
    adminPanel_ = new AdminPanel(manager_);

    tabs_->addTab(dashboard_, "Dashboard");
    tabs_->addTab(patientPanel_, "Patient Records");
    tabs_->addTab(appointmentPanel_, "Appointments");
    tabs_->addTab(queuePanel_, "Waiting Queue");
    tabs_->addTab(searchPanel_, "Search && Sort");
    tabs_->addTab(reportsPanel_, "Reports");
    tabs_->addTab(adminPanel_, "Admin / Undo");
    setCentralWidget(tabs_);

    buildMenus();
    buildStatusBar();

    // Every panel reports changes through the same signal.
    connect(patientPanel_, &PatientPanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(appointmentPanel_, &AppointmentPanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(queuePanel_, &QueuePanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(searchPanel_, &SearchPanel::dataChanged, this, &MainWindow::onDataChanged);
    connect(reportsPanel_, &ReportsPanel::dataChanged, this,
            [this](const QString& message) { statusBar()->showMessage(message, 6000); });
    connect(queuePanel_, &QueuePanel::waitingAlert, this, [this](const QString& message) {
        statusBar()->showMessage("Waiting alert: " + message, 10000);
        dashboard_->showReminder("Waiting alert: " + message);
    });
    connect(adminPanel_, &AdminPanel::undoRequested, this, &MainWindow::onUndo);
    connect(adminPanel_, &AdminPanel::resetRequested, this, &MainWindow::onNew);
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int) { dashboard_->refresh(); });

    // Timer event: check for appointments starting within 30 minutes.
    reminderTimer_ = new QTimer(this);
    connect(reminderTimer_, &QTimer::timeout, this, &MainWindow::onReminderTimer);
    reminderTimer_->start(30000);
}

void MainWindow::buildMenus() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* newAction = fileMenu->addAction("&New", this, &MainWindow::onNew);
    newAction->setShortcut(QKeySequence::New);
    newAction->setToolTip("Start with an empty clinic");
    QAction* openAction = fileMenu->addAction("&Load...", this, &MainWindow::onOpen);
    openAction->setShortcut(QKeySequence::Open);
    QAction* saveAction = fileMenu->addAction("&Save", this, &MainWindow::onSave);
    saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    QAction* saveAsAction = fileMenu->addAction("Save &As...", this, &MainWindow::onSaveAs);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("E&xit", this, &QWidget::close);
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    QAction* undoAction = editMenu->addAction("&Undo Last Operation", this, &MainWindow::onUndo);
    undoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    QAction* findAction = editMenu->addAction("&Find Patient...", this, &MainWindow::onFind);
    findAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));

    QMenu* viewMenu = menuBar()->addMenu("&View");
    const QStringList tabNames = {"Dashboard", "Patient Records", "Appointments", "Waiting Queue",
                                  "Search && Sort", "Reports", "Admin / Undo"};
    for (int i = 0; i < tabNames.size(); ++i) {
        QAction* action = viewMenu->addAction(tabNames[i], this, [this, i] { tabs_->setCurrentIndex(i); });
        action->setShortcut(QKeySequence(Qt::ALT | static_cast<Qt::Key>(Qt::Key_1 + i)));
    }

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("Load &Demo Data", this, &MainWindow::onLoadSampleData);
    helpMenu->addAction("&About SCAPMS", this, &MainWindow::onAbout);
}

void MainWindow::buildStatusBar() {
    patientsStatus_ = new QLabel();
    queueStatus_ = new QLabel();
    fileStatus_ = new QLabel();
    statusBar()->addPermanentWidget(patientsStatus_);
    statusBar()->addPermanentWidget(queueStatus_);
    statusBar()->addPermanentWidget(fileStatus_);
    statusBar()->showMessage("Ready");
}

void MainWindow::loadStartupData() {
    // Look for data/clinic_data.csv next to the program, then in the working folder.
    const QStringList candidates = {
        QCoreApplication::applicationDirPath() + "/data/clinic_data.csv",
        QDir::currentPath() + "/data/clinic_data.csv"};
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            loadFile(path);
            return;
        }
    }
    controller::loadSampleData(manager_);
    setCurrentFile("");
    refreshAll();
    statusBar()->showMessage("No data file found, loaded demo data.", 8000);
}

void MainWindow::loadFile(const QString& path) {
    try {
        manager_.loadFromFile(fromQ(path));
        setCurrentFile(path);
        reminded_.clear();
        refreshAll();
        statusBar()->showMessage("Loaded " + QFileInfo(path).fileName(), 6000);
    } catch (const std::exception& error) {
        showError(this, error); // old data is kept when loading fails
    }
}

void MainWindow::setCurrentFile(const QString& path) {
    currentFile_ = path;
    dirty_ = false;
    fileStatus_->setText(path.isEmpty() ? "Unsaved data" : QFileInfo(path).fileName());
}

void MainWindow::refreshAll() {
    dashboard_->refresh();
    patientPanel_->refresh();
    appointmentPanel_->refresh();
    queuePanel_->refresh();
    searchPanel_->refresh();
    reportsPanel_->refresh();
    adminPanel_->refresh();
    patientsStatus_->setText(QString("Patients: %1").arg(manager_.database().patientCount()));
    queueStatus_->setText(QString("Queue: %1").arg(manager_.queueSize()));
    fileStatus_->setText((currentFile_.isEmpty() ? QString("Unsaved data") : QFileInfo(currentFile_).fileName()) +
                         (dirty_ ? " *" : ""));
}

void MainWindow::onDataChanged(const QString& message) {
    dirty_ = true;
    refreshAll();
    statusBar()->showMessage(message, 6000);
}

bool MainWindow::confirmDiscard() {
    if (!dirty_) {
        return true;
    }
    const auto answer = QMessageBox::question(this, "Unsaved Changes", "Save changes before continuing?",
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) {
        return onSave();
    }
    return answer == QMessageBox::Discard;
}

void MainWindow::onNew() {
    if (!confirmDiscard()) {
        return;
    }
    manager_.reset();
    setCurrentFile("");
    refreshAll();
    statusBar()->showMessage("Started a new, empty clinic.", 6000);
}

void MainWindow::onOpen() {
    if (!confirmDiscard()) {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, "Load Clinic Data", QDir::currentPath(),
                                                      "Clinic data (*.csv *.txt);;All files (*)");
    if (!path.isEmpty()) {
        loadFile(path);
    }
}

bool MainWindow::onSave() {
    if (currentFile_.isEmpty()) {
        return onSaveAs();
    }
    try {
        manager_.saveToFile(fromQ(currentFile_));
        setCurrentFile(currentFile_);
        refreshAll();
        statusBar()->showMessage("Saved to " + QFileInfo(currentFile_).fileName(), 6000);
        return true;
    } catch (const std::exception& error) {
        showError(this, error);
        return false;
    }
}

bool MainWindow::onSaveAs() {
    const QString path = QFileDialog::getSaveFileName(this, "Save Clinic Data", "clinic_data.csv",
                                                      "Clinic data (*.csv);;All files (*)");
    if (path.isEmpty()) {
        return false;
    }
    currentFile_ = path;
    return onSave();
}

void MainWindow::onUndo() {
    try {
        const std::string message = manager_.undoLastOperation();
        onDataChanged(toQ(message));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void MainWindow::onLoadSampleData() {
    if (!confirmDiscard()) {
        return;
    }
    controller::loadSampleData(manager_);
    setCurrentFile("");
    dirty_ = true;
    reminded_.clear();
    refreshAll();
    statusBar()->showMessage("Loaded demo data with dates relative to today.", 6000);
}

void MainWindow::onFind() {
    tabs_->setCurrentWidget(searchPanel_);
    searchPanel_->focusSearch();
}

void MainWindow::onReminderTimer() {
    QStringList messages;
    for (const auto& appointment : appointments_.upcomingReminders(30)) {
        if (!reminded_.insert(appointment.getId()).second) {
            continue; // remind once per appointment
        }
        const model::PatientRecord* patient = manager_.database().findPatient(appointment.getPatientId());
        messages << QString("%1 with %2 at %3")
                        .arg(patient ? toQ(patient->getName()) : toQ(appointment.getPatientId()),
                             toQ(appointment.getDoctor()), toQ(appointment.getTime()));
    }
    if (!messages.isEmpty()) {
        const QString text = "Reminder: upcoming appointment(s) - " + messages.join("; ");
        statusBar()->showMessage(text, 15000);
        dashboard_->showReminder(text);
    }
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About SCAPMS",
                       "<h3>Smart Clinic Appointment &amp; Patient Management System</h3>"
                       "<p>COS202 Programming II lab project, written in C++17 with Qt 6.</p>"
                       "<p>Shortcuts: Ctrl+S save, Ctrl+O load, Ctrl+F search, Ctrl+Z undo, Ctrl+Q exit.</p>");
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmDiscard()) {
        event->accept();
    } else {
        event->ignore();
    }
}

} // namespace scapms::gui
