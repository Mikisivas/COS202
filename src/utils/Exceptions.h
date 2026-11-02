#pragma once
// Custom exception hierarchy for SCAPMS.
// Every error the program reports derives from ClinicException, so the GUI
// can catch one base type and still show a specific dialog title.

#include <stdexcept>
#include <string>

namespace scapms::utils {

class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message)
        : std::runtime_error(message) {}
    virtual std::string title() const { return "Clinic Error"; }
};

// Bad user input: empty name, age outside 0-120, malformed date, etc.
class ValidationException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Invalid Input"; }
};

// A patient ID that is already registered.
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(const std::string& id)
        : ClinicException("Patient ID already exists: " + id) {}
    std::string title() const override { return "Duplicate Patient ID"; }
};

// A patient or appointment ID that does not exist.
class NotFoundException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Record Not Found"; }
};

// Rule violations when booking or changing appointments.
class AppointmentException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Appointment Error"; }
};

// Problems opening, reading, parsing or writing data files.
class FileException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "File Error"; }
};

// Empty keywords or a search algorithm used on data in the wrong state.
class SearchException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Search Error"; }
};

} // namespace scapms::utils
