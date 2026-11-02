#include "model/FollowUpPatient.h"

#include "model/Appointment.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

FollowUpPatient::FollowUpPatient(std::string id, std::string name, int age, std::string contact,
                                 std::string previousDiagnosis, std::string lastVisitDate)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {
    setPreviousDiagnosis(previousDiagnosis);
    setLastVisitDate(lastVisitDate);
}

void FollowUpPatient::setPreviousDiagnosis(const std::string& diagnosis) {
    const std::string cleaned = utils::trim(diagnosis);
    if (cleaned.empty()) {
        throw utils::ValidationException("Please enter the previous diagnosis for the follow-up.");
    }
    previousDiagnosis_ = cleaned;
}

void FollowUpPatient::setLastVisitDate(const std::string& date) {
    const std::string cleaned = utils::trim(date);
    if (!utils::isValidDate(cleaned)) {
        throw utils::ValidationException("Last visit date must be a valid date (YYYY-MM-DD).");
    }
    lastVisitDate_ = cleaned;
}

std::string FollowUpPatient::getDetails() const {
    return "Follow-up for: " + previousDiagnosis_ + " | Last visit: " + lastVisitDate_;
}

std::vector<std::string> FollowUpPatient::getExtraFields() const {
    return {previousDiagnosis_, lastVisitDate_};
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const {
    return std::make_unique<FollowUpPatient>(*this);
}

void FollowUpPatient::scheduleAppointment(Appointment& appointment) {
    if (appointment.getPatientId() != id_) {
        throw utils::AppointmentException("Appointment " + appointment.getId() +
                                          " does not belong to patient " + id_ + ".");
    }
    // Follow-up rule: the check-up must come after the last visit.
    if (appointment.getDate() <= lastVisitDate_) {
        throw utils::AppointmentException("A follow-up appointment must be after the last visit (" +
                                          lastVisitDate_ + ").");
    }
    if (appointment.getReason().empty()) {
        appointment.setReason("Follow-up: " + previousDiagnosis_);
    }
    linkAppointment(appointment.getId());
}

void FollowUpPatient::cancelAppointment(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) {
        throw utils::AppointmentException("Appointment " + appointmentId +
                                          " is not booked for " + name_ + ".");
    }
}

std::string FollowUpPatient::bookingRule() const {
    return "Follow-up appointments must be dated after the last visit (" + lastVisitDate_ + ").";
}

} // namespace scapms::model
