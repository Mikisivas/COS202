#pragma once
// A walk-in or routine patient. Can be booked for appointments.

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

class GeneralPatient : public PatientRecord, public Appointable {
public:
    GeneralPatient(std::string id, std::string name, int age, std::string contact,
                   std::string symptoms);

    const std::string& getSymptoms() const { return symptoms_; }
    void setSymptoms(const std::string& symptoms);

    // PatientRecord overrides
    PatientCategory getCategory() const override { return PatientCategory::General; }
    int getPriorityLevel() const override { return 2; }
    std::string getPriorityLabel() const override { return "Normal"; }
    std::string getDetails() const override;
    std::vector<std::string> getExtraFields() const override { return {symptoms_}; }
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable overrides
    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;
    std::string bookingRule() const override;

private:
    std::string symptoms_;
};

} // namespace scapms::model
