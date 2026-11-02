#pragma once
// A returning patient who needs a check-up after an earlier visit.

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(std::string id, std::string name, int age, std::string contact,
                    std::string previousDiagnosis, std::string lastVisitDate);

    const std::string& getPreviousDiagnosis() const { return previousDiagnosis_; }
    const std::string& getLastVisitDate() const { return lastVisitDate_; }
    void setPreviousDiagnosis(const std::string& diagnosis);
    void setLastVisitDate(const std::string& date);

    PatientCategory getCategory() const override { return PatientCategory::FollowUp; }
    int getPriorityLevel() const override { return 3; }
    std::string getPriorityLabel() const override { return "Scheduled"; }
    std::string getDetails() const override;
    std::vector<std::string> getExtraFields() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;
    std::string bookingRule() const override;

private:
    std::string previousDiagnosis_;
    std::string lastVisitDate_; // YYYY-MM-DD
};

} // namespace scapms::model
