#pragma once
// Shared GUI helpers: string conversion, error dialogs and colour coding.

#include "model/Appointment.h"
#include "model/PatientRecord.h"
#include "utils/Exceptions.h"

#include <QColor>
#include <QMessageBox>
#include <QString>
#include <QTableWidgetItem>
#include <QWidget>

#include <exception>
#include <string>

namespace scapms::gui {

inline QString toQ(const std::string& text) { return QString::fromStdString(text); }
inline std::string fromQ(const QString& text) { return text.trimmed().toStdString(); }

// Shows a warning dialog. ClinicException subclasses supply their own title.
inline void showError(QWidget* parent, const std::exception& error) {
    QString title = "Error";
    if (const auto* clinicError = dynamic_cast<const utils::ClinicException*>(&error)) {
        title = toQ(clinicError->title());
    }
    QMessageBox::warning(parent, title, "Error: " + QString::fromStdString(error.what()));
}

inline QColor categoryBackground(model::PatientCategory category) {
    switch (category) {
    case model::PatientCategory::Emergency: return QColor(255, 205, 210);
    case model::PatientCategory::General:   return QColor(220, 237, 200);
    case model::PatientCategory::FollowUp:  return QColor(197, 225, 250);
    }
    return Qt::white;
}

inline QColor categoryAccent(model::PatientCategory category) {
    switch (category) {
    case model::PatientCategory::Emergency: return QColor(183, 28, 28);
    case model::PatientCategory::General:   return QColor(46, 125, 50);
    case model::PatientCategory::FollowUp:  return QColor(21, 101, 192);
    }
    return Qt::black;
}

inline QColor statusBackground(model::AppointmentStatus status) {
    switch (status) {
    case model::AppointmentStatus::Pending:   return QColor(255, 243, 205);
    case model::AppointmentStatus::Completed: return QColor(220, 237, 200);
    case model::AppointmentStatus::Cancelled: return QColor(230, 230, 230);
    }
    return Qt::white;
}

// Read-only table cell with fixed colours so text stays readable in any theme.
inline QTableWidgetItem* makeItem(const QString& text, const QColor& background = Qt::white) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackground(background);
    item->setForeground(QColor(33, 33, 33));
    return item;
}

} // namespace scapms::gui
