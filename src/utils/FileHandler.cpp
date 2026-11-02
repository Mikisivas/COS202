#include "utils/FileHandler.h"

#include "model/PatientFactory.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

#include <fstream>

namespace scapms::utils {

std::string FileHandler::escapeCsv(const std::string& field) {
    bool needsQuotes = false;
    for (char c : field) {
        needsQuotes = needsQuotes || c == ',' || c == '"' || c == '\n';
    }
    if (!needsQuotes) {
        return field;
    }
    std::string out = "\"";
    for (char c : field) {
        if (c == '"') {
            out += "\"\""; // CSV doubles quotes inside a quoted field
        } else if (c != '\n') {
            out += c;
        }
    }
    return out + "\"";
}

std::vector<std::string> FileHandler::parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                ++i;
            } else if (c == '"') {
                inQuotes = false;
            } else {
                current += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current += c;
        }
    }
    if (inQuotes) {
        throw FileException("Unclosed quote in line: " + line);
    }
    fields.push_back(current);
    return fields;
}

void FileHandler::save(const std::string& path, const model::ClinicDatabase& db) {
    std::ofstream out(path);
    if (!out) {
        throw FileException("Could not open '" + path + "' for writing.");
    }
    out << "# SCAPMS data file v1\n";
    for (const auto& patient : db.patients()) {
        out << "PATIENT," << escapeCsv(patient->getId()) << ','
            << escapeCsv(patient->getCategoryName()) << ',' << escapeCsv(patient->getName()) << ','
            << patient->getAge() << ',' << escapeCsv(patient->getContact());
        for (const std::string& extra : patient->getExtraFields()) {
            out << ',' << escapeCsv(extra);
        }
        out << '\n';
    }
    for (const auto& appointment : db.appointments()) {
        out << "APPT," << escapeCsv(appointment.getId()) << ','
            << escapeCsv(appointment.getPatientId()) << ',' << appointment.getDate() << ','
            << appointment.getTime() << ',' << escapeCsv(appointment.getDoctor()) << ','
            << escapeCsv(appointment.getReason()) << ',' << appointment.getStatusName() << '\n';
    }
    if (!out) {
        throw FileException("Writing to '" + path + "' failed. Check disk space and permissions.");
    }
}

void FileHandler::load(const std::string& path, model::ClinicDatabase& db) {
    std::ifstream in(path);
    if (!in) {
        throw FileException("Could not open '" + path + "'. Check the file exists and is readable.");
    }
    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        try {
            const std::vector<std::string> f = parseCsvLine(trimmed);
            if (f[0] == "PATIENT") {
                if (f.size() < 7) {
                    throw FileException("patient record has too few fields");
                }
                int age = 0;
                try {
                    age = std::stoi(f[4]);
                } catch (const std::exception&) {
                    throw ValidationException("age '" + f[4] + "' is not a number");
                }
                const std::vector<std::string> extras(f.begin() + 6, f.end());
                db.addPatient(model::createPatient(model::categoryFromString(f[2]), f[1], f[3],
                                                   age, f[5], extras));
            } else if (f[0] == "APPT") {
                if (f.size() < 8) {
                    throw FileException("appointment record has too few fields");
                }
                model::Appointment appointment(f[1], f[2], f[3], f[4], f[5], f[6],
                                               model::statusFromString(f[7]));
                model::PatientRecord* patient = db.findPatient(appointment.getPatientId());
                if (patient == nullptr) {
                    throw FileException("appointment refers to unknown patient " +
                                        appointment.getPatientId());
                }
                db.addAppointment(appointment);
                patient->linkAppointment(appointment.getId());
            } else {
                throw FileException("unknown record type '" + f[0] + "'");
            }
        } catch (const ClinicException& error) {
            throw FileException("Could not load '" + path + "'.\nLine " +
                                std::to_string(lineNumber) + ": " + error.what());
        }
    }
}

void FileHandler::writeText(const std::string& path, const std::string& text) {
    std::ofstream out(path);
    if (!out) {
        throw FileException("Could not open '" + path + "' for writing.");
    }
    out << text;
}

} // namespace scapms::utils
