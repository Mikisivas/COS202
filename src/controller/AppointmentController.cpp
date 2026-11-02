#include "controller/AppointmentController.h"

#include "model/Appointable.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/IDGenerator.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

using model::Appointment;
using model::AppointmentStatus;
using utils::AppointmentException;

AppointmentController::AppointmentController(ClinicManager& manager) : manager_(manager) {}

std::string AppointmentController::generateAppointmentId() const {
    std::string id;
    do {
        id = utils::IDGenerator::nextAppointmentId();
    } while (manager_.database().findAppointment(id) != nullptr);
    return id;
}

Appointment& AppointmentController::requireAppointment(const std::string& appointmentId) {
    Appointment* appointment = manager_.database().findAppointment(appointmentId);
    if (appointment == nullptr) {
        throw utils::NotFoundException("No appointment with ID " + appointmentId + ".");
    }
    return *appointment;
}

Appointment AppointmentController::schedule(const std::string& patientId, const std::string& date,
                                            const std::string& time, const std::string& doctor,
                                            const std::string& reason) {
    model::PatientRecord* patient = manager_.getPatient(patientId);
    if (patient == nullptr) {
        throw utils::NotFoundException("Please choose a registered patient for the appointment.");
    }
    // Only types that implement the Appointable interface can be booked.
    auto* appointable = dynamic_cast<model::Appointable*>(patient);
    if (appointable == nullptr) {
        throw AppointmentException(patient->getName() + " is an " + patient->getCategoryName() +
                                   " patient. Emergency patients are seen through the waiting "
                                   "queue and cannot be booked for appointments.");
    }
    if (!utils::isValidDate(date)) {
        throw utils::ValidationException("Appointment date must be a valid date (YYYY-MM-DD).");
    }
    if (!utils::isValidTime(time)) {
        throw utils::ValidationException("Appointment time must be a valid time (HH:MM).");
    }
    const std::string today = utils::today();
    if (date < today) {
        throw AppointmentException("Appointment date cannot be in the past.");
    }
    if (date == today && time < utils::currentTime()) {
        throw AppointmentException("That time has already passed today. Choose a later time.");
    }
    const std::string doctorName = utils::trim(doctor);
    if (doctorName.size() < 2) {
        throw utils::ValidationException("Please enter the doctor's name.");
    }
    for (const Appointment& existing : manager_.database().appointments()) {
        if (existing.getStatus() == AppointmentStatus::Pending && existing.getDate() == date &&
            existing.getTime() == time &&
            utils::toLower(existing.getDoctor()) == utils::toLower(doctorName)) {
            throw AppointmentException(doctorName + " already has an appointment on " + date +
                                       " at " + time + ".");
        }
    }

    Appointment appointment(generateAppointmentId(), patient->getId(), date, time, doctorName,
                            reason);
    appointable->scheduleAppointment(appointment); // subclass rules, may throw
    manager_.database().addAppointment(appointment);
    return appointment;
}

void AppointmentController::cancel(const std::string& appointmentId) {
    Appointment& appointment = requireAppointment(appointmentId);
    if (appointment.getStatus() != AppointmentStatus::Pending) {
        throw AppointmentException("Only pending appointments can be cancelled. This one is " +
                                   appointment.getStatusName() + ".");
    }
    model::PatientRecord* patient = manager_.database().findPatient(appointment.getPatientId());
    if (auto* appointable = dynamic_cast<model::Appointable*>(patient)) {
        appointable->cancelAppointment(appointmentId);
    }
    AdminAction action;
    action.type = ActionType::CancelAppointment;
    action.targetId = appointmentId;
    action.description = "Cancelled appointment " + appointmentId + " (" +
                         appointment.getDate() + " " + appointment.getTime() + ")";
    action.appointmentSnapshots.push_back(appointment);

    appointment.setStatus(AppointmentStatus::Cancelled);
    manager_.recordAction(std::move(action));
}

void AppointmentController::complete(const std::string& appointmentId) {
    Appointment& appointment = requireAppointment(appointmentId);
    if (appointment.getStatus() != AppointmentStatus::Pending) {
        throw AppointmentException("Only pending appointments can be marked as completed.");
    }
    appointment.setStatus(AppointmentStatus::Completed);
}

void AppointmentController::remove(const std::string& appointmentId) {
    const Appointment snapshot = requireAppointment(appointmentId);
    AdminAction action;
    action.type = ActionType::RemoveAppointment;
    action.targetId = appointmentId;
    action.description = "Removed appointment " + appointmentId + " for patient " +
                         snapshot.getPatientId();
    action.appointmentSnapshots.push_back(snapshot);

    if (model::PatientRecord* patient = manager_.database().findPatient(snapshot.getPatientId())) {
        patient->unlinkAppointment(appointmentId);
    }
    manager_.database().removeAppointment(appointmentId);
    manager_.recordAction(std::move(action));
}

std::vector<Appointment> AppointmentController::upcomingReminders(int minutesAhead) const {
    std::vector<Appointment> due;
    const std::string today = utils::today();
    const int now = utils::minutesOfDay(utils::currentTime());
    for (const Appointment& appointment : manager_.database().appointments()) {
        if (appointment.getStatus() != AppointmentStatus::Pending || appointment.getDate() != today) {
            continue;
        }
        const int start = utils::minutesOfDay(appointment.getTime());
        if (start >= now && start - now <= minutesAhead) {
            due.push_back(appointment);
        }
    }
    return due;
}

std::vector<Appointment> AppointmentController::appointmentsOn(const std::string& date) const {
    std::vector<Appointment> result;
    for (const Appointment& appointment : manager_.database().appointments()) {
        if (appointment.getDate() == date) {
            result.push_back(appointment);
        }
    }
    // Order by time with a small insertion sort.
    for (std::size_t i = 1; i < result.size(); ++i) {
        Appointment key = result[i];
        std::size_t j = i;
        while (j > 0 && result[j - 1].getTime() > key.getTime()) {
            result[j] = result[j - 1];
            --j;
        }
        result[j] = key;
    }
    return result;
}

std::string AppointmentController::nextAppointmentFor(const std::string& patientId) const {
    std::string best;
    for (const Appointment& appointment : manager_.database().appointments()) {
        if (appointment.getPatientId() == patientId &&
            appointment.getStatus() == AppointmentStatus::Pending &&
            (best.empty() || appointment.dateTimeKey() < best)) {
            best = appointment.dateTimeKey();
        }
    }
    return best;
}

} // namespace scapms::controller
