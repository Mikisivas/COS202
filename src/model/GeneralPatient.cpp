#include "model/GeneralPatient.h"

#include "model/Appointment.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

GeneralPatient::GeneralPatient(std::string id, std::string name, int age, std::string contact,
                               std::string symptoms)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {
    setSymptoms(symptoms);
}

void GeneralPatient::setSymptoms(const std::string& symptoms) {
    const std::string cleaned = utils::trim(symptoms);
    if (cleaned.empty()) {
        throw utils::ValidationException("Please describe the general patient's symptoms.");
    }
    symptoms_ = cleaned;
}

std::string GeneralPatient::getDetails() const {
    return "Symptoms: " + symptoms_;
}

std::unique_ptr<PatientRecord> GeneralPatient::clone() const {
    return std::make_unique<GeneralPatient>(*this);
}

void GeneralPatient::scheduleAppointment(Appointment& appointment) {
    if (appointment.getPatientId() != id_) {
        throw utils::AppointmentException("Appointment " + appointment.getId() +
                                          " does not belong to patient " + id_ + ".");
    }
    // General rule: if no reason was typed, the recorded symptoms become the reason.
    if (appointment.getReason().empty()) {
        appointment.setReason(symptoms_);
    }
    linkAppointment(appointment.getId());
}

void GeneralPatient::cancelAppointment(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) {
        throw utils::AppointmentException("Appointment " + appointmentId +
                                          " is not booked for " + name_ + ".");
    }
}

std::string GeneralPatient::bookingRule() const {
    return "General patients can book any future slot. Symptoms are used as the reason if none is given.";
}

} // namespace scapms::model
