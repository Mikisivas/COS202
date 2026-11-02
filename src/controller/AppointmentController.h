#pragma once
// Appointment business logic: booking, cancelling, completing, removing,
// and finding reminders. Uses ClinicManager for patients and the undo stack.

#include "controller/ClinicManager.h"
#include "model/Appointment.h"

#include <string>
#include <vector>

namespace scapms::controller {

class AppointmentController {
public:
    explicit AppointmentController(ClinicManager& manager);

    model::Appointment schedule(const std::string& patientId, const std::string& date,
                                const std::string& time, const std::string& doctor,
                                const std::string& reason);
    void cancel(const std::string& appointmentId);
    void complete(const std::string& appointmentId);
    void remove(const std::string& appointmentId);

    // Pending appointments today that start within the next `minutesAhead` minutes.
    std::vector<model::Appointment> upcomingReminders(int minutesAhead) const;
    std::vector<model::Appointment> appointmentsOn(const std::string& date) const;
    std::string nextAppointmentFor(const std::string& patientId) const;

private:
    model::Appointment& requireAppointment(const std::string& appointmentId);
    std::string generateAppointmentId() const;

    ClinicManager& manager_;
};

} // namespace scapms::controller
