#include "model/PatientRecord.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

#include <cctype>

namespace scapms::model {

using utils::ValidationException;

std::string categoryToString(PatientCategory category) {
    switch (category) {
    case PatientCategory::General:   return "General";
    case PatientCategory::Emergency: return "Emergency";
    case PatientCategory::FollowUp:  return "Follow-Up";
    }
    return "Unknown";
}

PatientCategory categoryFromString(const std::string& text) {
    const std::string key = utils::toLower(utils::trim(text));
    if (key == "general") {
        return PatientCategory::General;
    }
    if (key == "emergency") {
        return PatientCategory::Emergency;
    }
    if (key == "follow-up" || key == "followup" || key == "follow up") {
        return PatientCategory::FollowUp;
    }
    throw ValidationException("Unknown patient category: " + text);
}

PatientRecord::PatientRecord(std::string id, std::string name, int age, std::string contact)
    : id_(utils::toUpper(utils::trim(id))),
      name_(utils::trim(name)),
      age_(age),
      contact_(utils::trim(contact)) {
    validateId(id_);
    validateName(name_);
    validateAge(age_);
    validateContact(contact_);
}

void PatientRecord::setName(const std::string& name) {
    const std::string cleaned = utils::trim(name);
    validateName(cleaned);
    name_ = cleaned;
}

void PatientRecord::setAge(int age) {
    validateAge(age);
    age_ = age;
}

void PatientRecord::setContact(const std::string& contact) {
    const std::string cleaned = utils::trim(contact);
    validateContact(cleaned);
    contact_ = cleaned;
}

std::string PatientRecord::describe() const {
    // Calls pure virtual functions, so the output depends on the real type.
    return id_ + " | " + name_ + " (" + std::to_string(age_) + ") | " + getCategoryName() +
           " | " + getPriorityLabel() + " | " + getDetails();
}

bool PatientRecord::hasAppointment(const std::string& appointmentId) const {
    for (const std::string& id : appointmentIds_) {
        if (id == appointmentId) {
            return true;
        }
    }
    return false;
}

void PatientRecord::linkAppointment(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) {
        appointmentIds_.push_back(appointmentId);
    }
}

void PatientRecord::unlinkAppointment(const std::string& appointmentId) {
    for (std::size_t i = 0; i < appointmentIds_.size(); ++i) {
        if (appointmentIds_[i] == appointmentId) {
            appointmentIds_.erase(appointmentIds_.begin() + static_cast<long>(i));
            return;
        }
    }
}

void PatientRecord::validateId(const std::string& id) {
    bool valid = id.size() >= 2 && id.size() <= 7 && id[0] == 'P';
    for (std::size_t i = 1; valid && i < id.size(); ++i) {
        valid = std::isdigit(static_cast<unsigned char>(id[i])) != 0;
    }
    if (!valid) {
        throw ValidationException("Patient ID must be the letter P followed by 1-6 digits (e.g. P0001).");
    }
}

void PatientRecord::validateName(const std::string& name) {
    if (name.size() < 2 || name.size() > 60) {
        throw ValidationException("Patient name must be between 2 and 60 characters.");
    }
    bool hasLetter = false;
    for (char c : name) {
        const auto uc = static_cast<unsigned char>(c);
        if (std::isalpha(uc)) {
            hasLetter = true;
        } else if (c != ' ' && c != '-' && c != '\'' && c != '.') {
            throw ValidationException("Patient name may only contain letters, spaces, hyphens, apostrophes and full stops.");
        }
    }
    if (!hasLetter) {
        throw ValidationException("Patient name must contain letters.");
    }
}

void PatientRecord::validateAge(int age) {
    if (age < 0 || age > 120) {
        throw ValidationException("Patient age must be between 0 and 120.");
    }
}

void PatientRecord::validateContact(const std::string& contact) {
    if (contact.empty()) {
        throw ValidationException("Contact information is required.");
    }
    const std::size_t at = contact.find('@');
    if (at != std::string::npos) {
        const bool hasSpace = contact.find(' ') != std::string::npos;
        const bool dotAfterAt = contact.find('.', at) != std::string::npos;
        if (at == 0 || hasSpace || !dotAfterAt || contact.back() == '.') {
            throw ValidationException("Contact email address is not valid.");
        }
        return;
    }
    int digits = 0;
    for (char c : contact) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            ++digits;
        } else if (c != '+' && c != ' ' && c != '-' && c != '(' && c != ')') {
            throw ValidationException("Contact must be a phone number (7-15 digits) or an email address.");
        }
    }
    if (digits < 7 || digits > 15) {
        throw ValidationException("Contact must be a phone number (7-15 digits) or an email address.");
    }
}

} // namespace scapms::model
