#pragma once
// Interface-like abstract class: only pure virtual functions, no data.
// Patient types that can be booked (General, Follow-Up) inherit from it.
// EmergencyPatient does not, because emergencies go through the waiting queue.

#include <string>

namespace scapms::model {

class Appointment;

class Appointable {
public:
    virtual ~Appointable() = default;

    // Applies type-specific booking rules, then links the appointment.
    // Throws AppointmentException when a rule is broken.
    virtual void scheduleAppointment(Appointment& appointment) = 0;

    // Confirms the appointment belongs to this patient before it is cancelled.
    virtual void cancelAppointment(const std::string& appointmentId) = 0;

    // Short text shown in the GUI explaining this type's booking rule.
    virtual std::string bookingRule() const = 0;
};

} // namespace scapms::model
