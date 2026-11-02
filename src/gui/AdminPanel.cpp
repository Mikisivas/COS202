#include "gui/AdminPanel.h"

#include "gui/GuiUtils.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    auto* layout = new QVBoxLayout(this);

    auto* intro = new QLabel(
        "Administrative operations (register, modify, delete patient, cancel or remove appointment) "
        "are pushed onto a <b>std::stack</b>. Undo pops the most recent one and reverses it.");
    intro->setWordWrap(true);
    layout->addWidget(intro);

    topLabel_ = new QLabel();
    topLabel_->setStyleSheet("font-weight:bold; color:#6a1b9a;");
    layout->addWidget(topLabel_);

    auto* box = new QGroupBox("Undo Stack (top = next to undo)");
    auto* boxLayout = new QVBoxLayout(box);
    historyList_ = new QListWidget();
    historyList_->setObjectName("undoList");
    historyList_->setToolTip("Every entry here can be reversed, newest first");
    boxLayout->addWidget(historyList_);
    layout->addWidget(box, 1);

    auto* buttons = new QHBoxLayout();
    undoButton_ = new QPushButton("Undo Last Operation (Ctrl+Z)");
    undoButton_->setToolTip("Reverse the operation at the top of the stack");
    auto* resetButton = new QPushButton("Reset All Data...");
    resetButton->setToolTip("Clear every patient, appointment and queue entry (File > New)");
    buttons->addWidget(undoButton_);
    buttons->addStretch();
    buttons->addWidget(resetButton);
    layout->addLayout(buttons);

    connect(undoButton_, &QPushButton::clicked, this, &AdminPanel::undoRequested);
    connect(resetButton, &QPushButton::clicked, this, &AdminPanel::resetRequested);
    refresh();
}

void AdminPanel::refresh() {
    historyList_->clear();
    const auto history = manager_.undoHistory();
    for (std::size_t i = 0; i < history.size(); ++i) {
        auto* item = new QListWidgetItem(QString("%1. %2").arg(i + 1).arg(toQ(history[i])));
        if (i == 0) {
            QFont bold = item->font();
            bold.setBold(true);
            item->setFont(bold);
        }
        historyList_->addItem(item);
    }
    undoButton_->setEnabled(!history.empty());
    topLabel_->setText(history.empty() ? "Nothing to undo."
                                       : "Next undo: " + toQ(history.front()));
}

} // namespace scapms::gui
