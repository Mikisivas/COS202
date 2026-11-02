#pragma once
// One clinic appointment. Linked to a patient through patientId.

#include <string>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

std::string statusToString(AppointmentStatus status);
AppointmentStatus statusFromString(const std::string& text);

class Appointment {
public:
    Appointment(std::string id, std::string patientId, std::string date, std::string time,
                std::string doctor, std::string reason,
                AppointmentStatus status = AppointmentStatus::Pending);

    const std::string& getId() const { return id_; }
    const std::string& getPatientId() const { return patientId_; }
    const std::string& getDate() const { return date_; }
    const std::string& getTime() const { return time_; }
    const std::string& getDoctor() const { return doctor_; }
    const std::string& getReason() const { return reason_; }
    AppointmentStatus getStatus() const { return status_; }
    std::string getStatusName() const { return statusToString(status_); }

    // "YYYY-MM-DD HH:MM" sorts chronologically as a plain string.
    std::string dateTimeKey() const { return date_ + " " + time_; }

    void setReason(const std::string& reason) { reason_ = reason; }
    void setStatus(AppointmentStatus status) { status_ = status; }

private:
    void validate() const;

    std::string id_;
    std::string patientId_;
    std::string date_;
    std::string time_;
    std::string doctor_;
    std::string reason_;
    AppointmentStatus status_;
};

} // namespace scapms::model
