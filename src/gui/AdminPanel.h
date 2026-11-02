#pragma once
// Admin tab: shows the undo stack (newest on top) and the Undo button.

#include "controller/ClinicManager.h"

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::gui {

class AdminPanel : public QWidget {
    Q_OBJECT
public:
    explicit AdminPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

signals:
    void undoRequested();
    void resetRequested();

public slots:
    void refresh();

private:
    controller::ClinicManager& manager_;
    QListWidget* historyList_;
    QPushButton* undoButton_;
    QLabel* topLabel_;
};

} // namespace scapms::gui
