#pragma once
// Builds the right PatientRecord subclass from plain values.
// Used by the GUI forms and by the file loader.

#include "model/PatientRecord.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

// extras holds the type-specific fields:
//   General   -> {symptoms}
//   Emergency -> {condition, severity}
//   Follow-Up -> {previous diagnosis, last visit date}
std::unique_ptr<PatientRecord> createPatient(PatientCategory category, const std::string& id,
                                             const std::string& name, int age,
                                             const std::string& contact,
                                             const std::vector<std::string>& extras);

} // namespace scapms::model
