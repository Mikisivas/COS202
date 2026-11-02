#include "model/EmergencyPatient.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

EmergencyPatient::EmergencyPatient(std::string id, std::string name, int age,
                                   std::string contact, std::string condition, int severity)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)), severity_(1) {
    setCondition(condition);
    setSeverity(severity);
}

void EmergencyPatient::setCondition(const std::string& condition) {
    const std::string cleaned = utils::trim(condition);
    if (cleaned.empty()) {
        throw utils::ValidationException("Please describe the emergency condition.");
    }
    condition_ = cleaned;
}

void EmergencyPatient::setSeverity(int severity) {
    if (severity < 1 || severity > 5) {
        throw utils::ValidationException("Emergency severity must be between 1 and 5.");
    }
    severity_ = severity;
}

std::string EmergencyPatient::getSeverityLabel() const {
    static const char* labels[] = {"Minor", "Moderate", "Serious", "Severe", "Critical"};
    return labels[severity_ - 1];
}

std::string EmergencyPatient::getDetails() const {
    return "Condition: " + condition_ + " | Severity " + std::to_string(severity_) + "/5 (" +
           getSeverityLabel() + ")";
}

std::vector<std::string> EmergencyPatient::getExtraFields() const {
    return {condition_, std::to_string(severity_)};
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const {
    return std::make_unique<EmergencyPatient>(*this);
}

} // namespace scapms::model
