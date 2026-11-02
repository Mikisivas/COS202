#pragma once
// Composition: the database owns every patient record and appointment.
// Patients live in a std::vector of unique_ptr<PatientRecord>, so one
// container holds all three subclasses through the base-class pointer.

#include "model/Appointment.h"
#include "model/PatientRecord.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

using PatientList = std::vector<std::unique_ptr<PatientRecord>>;

// Field the patient list is currently sorted by. Binary search checks this.
enum class SortField { None, Name, Id, Age, AppointmentDate };

std::string sortFieldToString(SortField field);

class ClinicDatabase {
public:
    ClinicDatabase() = default;
    ClinicDatabase(const ClinicDatabase&) = delete;
    ClinicDatabase& operator=(const ClinicDatabase&) = delete;
    ClinicDatabase(ClinicDatabase&&) = default;
    ClinicDatabase& operator=(ClinicDatabase&&) = default;

    // ---- patients ----
    void addPatient(std::unique_ptr<PatientRecord> patient);
    std::unique_ptr<PatientRecord> removePatient(const std::string& id);
    void replacePatient(std::unique_ptr<PatientRecord> patient);
    PatientRecord* findPatient(const std::string& id) const;
    PatientList& patients() { return patients_; }
    const PatientList& patients() const { return patients_; }
    std::size_t patientCount() const { return patients_.size(); }
    int countByCategory(PatientCategory category) const;

    // ---- appointments ----
    void addAppointment(const Appointment& appointment);
    Appointment removeAppointment(const std::string& id);
    Appointment* findAppointment(const std::string& id);
    const Appointment* findAppointment(const std::string& id) const;
    std::vector<Appointment>& appointments() { return appointments_; }
    const std::vector<Appointment>& appointments() const { return appointments_; }
    std::vector<Appointment> appointmentsFor(const std::string& patientId) const;

    // ---- sort state ----
    SortField sortedBy() const { return sortedBy_; }
    void setSortedBy(SortField field) { sortedBy_ = field; }

    void clear();

private:
    PatientList patients_;
    std::vector<Appointment> appointments_;
    SortField sortedBy_ = SortField::None;
};

} // namespace scapms::model
