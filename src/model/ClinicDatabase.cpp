#include "model/ClinicDatabase.h"

#include "utils/Exceptions.h"

namespace scapms::model {

std::string sortFieldToString(SortField field) {
    switch (field) {
    case SortField::None:            return "Unsorted";
    case SortField::Name:            return "Patient Name";
    case SortField::Id:              return "Patient ID";
    case SortField::Age:             return "Age";
    case SortField::AppointmentDate: return "Next Appointment Date";
    }
    return "Unknown";
}

void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient) {
    if (!patient) {
        throw utils::ValidationException("Cannot add an empty patient record.");
    }
    if (findPatient(patient->getId()) != nullptr) {
        throw utils::DuplicateIdException(patient->getId());
    }
    patients_.push_back(std::move(patient));
    sortedBy_ = SortField::None; // appended at the end, so order is no longer guaranteed
}

std::unique_ptr<PatientRecord> ClinicDatabase::removePatient(const std::string& id) {
    for (auto it = patients_.begin(); it != patients_.end(); ++it) {
        if ((*it)->getId() == id) {
            std::unique_ptr<PatientRecord> removed = std::move(*it);
            patients_.erase(it); // erasing keeps the remaining order, so sortedBy_ stays valid
            return removed;
        }
    }
    throw utils::NotFoundException("No patient with ID " + id + ".");
}

void ClinicDatabase::replacePatient(std::unique_ptr<PatientRecord> patient) {
    for (auto& slot : patients_) {
        if (slot->getId() == patient->getId()) {
            slot = std::move(patient);
            sortedBy_ = SortField::None; // name or age may have changed
            return;
        }
    }
    throw utils::NotFoundException("No patient with ID " + patient->getId() + ".");
}

PatientRecord* ClinicDatabase::findPatient(const std::string& id) const {
    for (const auto& patient : patients_) {
        if (patient->getId() == id) {
            return patient.get();
        }
    }
    return nullptr;
}

int ClinicDatabase::countByCategory(PatientCategory category) const {
    int count = 0;
    for (const auto& patient : patients_) {
        if (patient->getCategory() == category) {
            ++count;
        }
    }
    return count;
}

void ClinicDatabase::addAppointment(const Appointment& appointment) {
    if (findAppointment(appointment.getId()) != nullptr) {
        throw utils::AppointmentException("Appointment ID already exists: " + appointment.getId());
    }
    appointments_.push_back(appointment);
}

Appointment ClinicDatabase::removeAppointment(const std::string& id) {
    for (auto it = appointments_.begin(); it != appointments_.end(); ++it) {
        if (it->getId() == id) {
            Appointment removed = *it;
            appointments_.erase(it);
            return removed;
        }
    }
    throw utils::NotFoundException("No appointment with ID " + id + ".");
}

Appointment* ClinicDatabase::findAppointment(const std::string& id) {
    for (auto& appointment : appointments_) {
        if (appointment.getId() == id) {
            return &appointment;
        }
    }
    return nullptr;
}

const Appointment* ClinicDatabase::findAppointment(const std::string& id) const {
    for (const auto& appointment : appointments_) {
        if (appointment.getId() == id) {
            return &appointment;
        }
    }
    return nullptr;
}

std::vector<Appointment> ClinicDatabase::appointmentsFor(const std::string& patientId) const {
    std::vector<Appointment> result;
    for (const auto& appointment : appointments_) {
        if (appointment.getPatientId() == patientId) {
            result.push_back(appointment);
        }
    }
    return result;
}

void ClinicDatabase::clear() {
    patients_.clear();
    appointments_.clear();
    sortedBy_ = SortField::None;
}

} // namespace scapms::model
