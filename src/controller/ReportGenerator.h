#pragma once
// Builds the clinic reports as plain tables, so the GUI can show them and
// FileHandler can export them as text.

#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

struct Report {
    std::string title;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::string summary;
};

class ReportGenerator {
public:
    explicit ReportGenerator(const model::ClinicDatabase& db) : db_(db) {}

    Report mostFrequentlyAttended(std::size_t limit = 10) const;
    Report pendingAppointments() const;
    Report categoryDistribution() const;
    Report dailySummary(const std::string& date) const;

    static std::string toText(const Report& report);

private:
    const model::ClinicDatabase& db_;
};

} // namespace scapms::controller
