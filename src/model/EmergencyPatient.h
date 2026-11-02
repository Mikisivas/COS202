#pragma once
// An urgent patient. Not Appointable: emergencies are seen through the
// waiting queue instead of booked slots.

#include "model/PatientRecord.h"

namespace scapms::model {

class EmergencyPatient : public PatientRecord {
public:
    EmergencyPatient(std::string id, std::string name, int age, std::string contact,
                     std::string condition, int severity);

    const std::string& getCondition() const { return condition_; }
    int getSeverity() const { return severity_; }
    std::string getSeverityLabel() const;

    void setCondition(const std::string& condition);
    void setSeverity(int severity);

    PatientCategory getCategory() const override { return PatientCategory::Emergency; }
    int getPriorityLevel() const override { return 1; }
    std::string getPriorityLabel() const override { return "High Priority"; }
    std::string getDetails() const override;
    std::vector<std::string> getExtraFields() const override;
    std::unique_ptr<PatientRecord> clone() const override;

private:
    std::string condition_;
    int severity_; // 1 (minor) to 5 (critical)
};

} // namespace scapms::model
