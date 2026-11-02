#include "model/Appointment.h"

#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

using utils::ValidationException;

std::string statusToString(AppointmentStatus status) {
    switch (status) {
    case AppointmentStatus::Pending:   return "Pending";
    case AppointmentStatus::Completed: return "Completed";
    case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

AppointmentStatus statusFromString(const std::string& text) {
    const std::string key = utils::toLower(utils::trim(text));
    if (key == "pending") {
        return AppointmentStatus::Pending;
    }
    if (key == "completed") {
        return AppointmentStatus::Completed;
    }
    if (key == "cancelled" || key == "canceled") {
        return AppointmentStatus::Cancelled;
    }
    throw ValidationException("Unknown appointment status: " + text);
}

Appointment::Appointment(std::string id, std::string patientId, std::string date,
                         std::string time, std::string doctor, std::string reason,
                         AppointmentStatus status)
    : id_(utils::trim(id)),
      patientId_(utils::toUpper(utils::trim(patientId))),
      date_(utils::trim(date)),
      time_(utils::trim(time)),
      doctor_(utils::trim(doctor)),
      reason_(utils::trim(reason)),
      status_(status) {
    validate();
}

void Appointment::validate() const {
    if (id_.empty()) {
        throw ValidationException("Appointment ID is missing.");
    }
    if (patientId_.empty()) {
        throw ValidationException("An appointment must belong to a patient.");
    }
    if (!utils::isValidDate(date_)) {
        throw ValidationException("Invalid appointment date '" + date_ + "'. Use YYYY-MM-DD.");
    }
    if (!utils::isValidTime(time_)) {
        throw ValidationException("Invalid appointment time '" + time_ + "'. Use HH:MM (24-hour).");
    }
    if (doctor_.size() < 2) {
        throw ValidationException("Please enter the doctor's name for the appointment.");
    }
}

} // namespace scapms::model
