#include "model/PatientFactory.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"

namespace scapms::model {

namespace {
const std::string& extraAt(const std::vector<std::string>& extras, std::size_t index,
                           const std::string& what) {
    if (index >= extras.size()) {
        throw utils::ValidationException("Missing field: " + what + ".");
    }
    return extras[index];
}
} // namespace

std::unique_ptr<PatientRecord> createPatient(PatientCategory category, const std::string& id,
                                             const std::string& name, int age,
                                             const std::string& contact,
                                             const std::vector<std::string>& extras) {
    switch (category) {
    case PatientCategory::General:
        return std::make_unique<GeneralPatient>(id, name, age, contact,
                                                extraAt(extras, 0, "symptoms"));
    case PatientCategory::Emergency: {
        int severity = 0;
        try {
            severity = std::stoi(extraAt(extras, 1, "severity"));
        } catch (const std::invalid_argument&) {
            throw utils::ValidationException("Emergency severity must be a number from 1 to 5.");
        } catch (const std::out_of_range&) {
            throw utils::ValidationException("Emergency severity must be a number from 1 to 5.");
        }
        return std::make_unique<EmergencyPatient>(id, name, age, contact,
                                                  extraAt(extras, 0, "condition"), severity);
    }
    case PatientCategory::FollowUp:
        return std::make_unique<FollowUpPatient>(id, name, age, contact,
                                                 extraAt(extras, 0, "previous diagnosis"),
                                                 extraAt(extras, 1, "last visit date"));
    }
    throw utils::ValidationException("Unknown patient category.");
}

} // namespace scapms::model
