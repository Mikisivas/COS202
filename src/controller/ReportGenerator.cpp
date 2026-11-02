#include "controller/ReportGenerator.h"

#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

#include <sstream>

namespace scapms::controller {

using model::AppointmentStatus;

Report ReportGenerator::mostFrequentlyAttended(std::size_t limit) const {
    Report report;
    report.title = "Most Frequently Attended Patients";
    report.headers = {"Rank", "Patient ID", "Name", "Category", "Completed Visits"};

    struct Entry {
        const model::PatientRecord* patient;
        int completed;
    };
    std::vector<Entry> entries;
    for (const auto& patient : db_.patients()) {
        int completed = 0;
        for (const auto& appointment : db_.appointments()) {
            if (appointment.getPatientId() == patient->getId() &&
                appointment.getStatus() == AppointmentStatus::Completed) {
                ++completed;
            }
        }
        if (completed > 0) {
            entries.push_back({patient.get(), completed});
        }
    }
    // Insertion sort, highest count first (stable, small list).
    for (std::size_t i = 1; i < entries.size(); ++i) {
        Entry key = entries[i];
        std::size_t j = i;
        while (j > 0 && entries[j - 1].completed < key.completed) {
            entries[j] = entries[j - 1];
            --j;
        }
        entries[j] = key;
    }
    for (std::size_t i = 0; i < entries.size() && i < limit; ++i) {
        report.rows.push_back({std::to_string(i + 1), entries[i].patient->getId(),
                               entries[i].patient->getName(), entries[i].patient->getCategoryName(),
                               std::to_string(entries[i].completed)});
    }
    report.summary = entries.empty() ? "No completed appointments yet."
                                     : std::to_string(entries.size()) +
                                           " patient(s) have completed at least one appointment.";
    return report;
}

Report ReportGenerator::pendingAppointments() const {
    Report report;
    report.title = "Patients with Pending Appointments";
    report.headers = {"Appointment", "Patient ID", "Name", "Date", "Time", "Doctor", "Reason"};
    std::vector<const model::Appointment*> pending;
    for (const auto& appointment : db_.appointments()) {
        if (appointment.getStatus() == AppointmentStatus::Pending) {
            pending.push_back(&appointment);
        }
    }
    // Order by date and time (insertion sort).
    for (std::size_t i = 1; i < pending.size(); ++i) {
        const model::Appointment* key = pending[i];
        std::size_t j = i;
        while (j > 0 && pending[j - 1]->dateTimeKey() > key->dateTimeKey()) {
            pending[j] = pending[j - 1];
            --j;
        }
        pending[j] = key;
    }
    int distinctPatients = 0;
    std::vector<std::string> seen;
    for (const model::Appointment* appointment : pending) {
        const model::PatientRecord* patient = db_.findPatient(appointment->getPatientId());
        report.rows.push_back({appointment->getId(), appointment->getPatientId(),
                               patient ? patient->getName() : "(deleted)", appointment->getDate(),
                               appointment->getTime(), appointment->getDoctor(),
                               appointment->getReason()});
        bool already = false;
        for (const std::string& id : seen) {
            already = already || id == appointment->getPatientId();
        }
        if (!already) {
            seen.push_back(appointment->getPatientId());
            ++distinctPatients;
        }
    }
    report.summary = std::to_string(pending.size()) + " pending appointment(s) across " +
                     std::to_string(distinctPatients) + " patient(s).";
    return report;
}

Report ReportGenerator::categoryDistribution() const {
    Report report;
    report.title = "Patient Category Distribution";
    report.headers = {"Category", "Patients", "Share", "Chart"};
    const int total = static_cast<int>(db_.patientCount());
    const model::PatientCategory categories[] = {model::PatientCategory::General,
                                                 model::PatientCategory::Emergency,
                                                 model::PatientCategory::FollowUp};
    for (model::PatientCategory category : categories) {
        const int count = db_.countByCategory(category);
        const int percent = total == 0 ? 0 : (count * 100 + total / 2) / total;
        std::string bar;
        for (int i = 0; i < percent / 4; ++i) {
            bar += "#";
        }
        report.rows.push_back({model::categoryToString(category) + " Patients",
                               std::to_string(count), std::to_string(percent) + "%", bar});
    }
    report.summary = "Total registered patients: " + std::to_string(total);
    return report;
}

Report ReportGenerator::dailySummary(const std::string& date) const {
    if (!utils::isValidDate(date)) {
        throw utils::ValidationException("Report date must be a valid date (YYYY-MM-DD).");
    }
    Report report;
    report.title = "Daily Appointment Summary for " + date;
    report.headers = {"Measure", "Count"};
    int total = 0;
    int completed = 0;
    int pending = 0;
    int cancelled = 0;
    for (const auto& appointment : db_.appointments()) {
        if (appointment.getDate() != date) {
            continue;
        }
        ++total;
        switch (appointment.getStatus()) {
        case AppointmentStatus::Completed: ++completed; break;
        case AppointmentStatus::Pending:   ++pending; break;
        case AppointmentStatus::Cancelled: ++cancelled; break;
        }
    }
    report.rows = {{"Total Appointments", std::to_string(total)},
                   {"Completed", std::to_string(completed)},
                   {"Pending", std::to_string(pending)},
                   {"Cancelled", std::to_string(cancelled)}};
    report.summary = total == 0 ? "No appointments on this date."
                                : std::to_string(completed * 100 / total) + "% of the day's appointments are completed.";
    return report;
}

std::string ReportGenerator::toText(const Report& report) {
    // Work out column widths so the export lines up in a text editor.
    std::vector<std::size_t> widths(report.headers.size(), 0);
    for (std::size_t c = 0; c < report.headers.size(); ++c) {
        widths[c] = report.headers[c].size();
    }
    for (const auto& row : report.rows) {
        for (std::size_t c = 0; c < row.size() && c < widths.size(); ++c) {
            if (row[c].size() > widths[c]) {
                widths[c] = row[c].size();
            }
        }
    }
    std::ostringstream out;
    out << "SCAPMS Report: " << report.title << "\n";
    out << "Generated: " << utils::today() << " " << utils::currentTime() << "\n\n";
    const auto writeRow = [&](const std::vector<std::string>& cells) {
        for (std::size_t c = 0; c < cells.size() && c < widths.size(); ++c) {
            out << cells[c] << std::string(widths[c] - cells[c].size() + 2, ' ');
        }
        out << "\n";
    };
    writeRow(report.headers);
    std::size_t lineLength = 0;
    for (std::size_t width : widths) {
        lineLength += width + 2;
    }
    out << std::string(lineLength, '-') << "\n";
    for (const auto& row : report.rows) {
        writeRow(row);
    }
    out << "\n" << report.summary << "\n";
    return out.str();
}

} // namespace scapms::controller
