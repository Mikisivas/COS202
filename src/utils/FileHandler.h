#pragma once
// Saves and loads all clinic data in one CSV file.
//
//   # SCAPMS data file v1
//   PATIENT,<id>,<category>,<name>,<age>,<contact>,<extra1>,<extra2>
//   APPT,<id>,<patientId>,<date>,<time>,<doctor>,<reason>,<status>
//
// Fields containing commas or quotes are wrapped in double quotes.

#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::utils {

class FileHandler {
public:
    static void save(const std::string& path, const model::ClinicDatabase& db);
    static void load(const std::string& path, model::ClinicDatabase& db);
    static void writeText(const std::string& path, const std::string& text);

    static std::string escapeCsv(const std::string& field);
    static std::vector<std::string> parseCsvLine(const std::string& line);
};

} // namespace scapms::utils
