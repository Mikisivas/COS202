#pragma once
// Abstract base class for every patient in the clinic.
// Holds the data all patients share and declares the behaviour each
// patient type must define (pure virtual functions).

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

enum class PatientCategory { General, Emergency, FollowUp };

std::string categoryToString(PatientCategory category);
PatientCategory categoryFromString(const std::string& text);

class PatientRecord {
public:
    PatientRecord(std::string id, std::string name, int age, std::string contact);
    virtual ~PatientRecord() = default;

    // Encapsulated data: read through getters, changed through validating setters.
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int getAge() const { return age_; }
    const std::string& getContact() const { return contact_; }

    void setName(const std::string& name);
    void setAge(int age);
    void setContact(const std::string& contact);

    // ---- Pure virtual functions: every subclass must implement these ----
    virtual PatientCategory getCategory() const = 0;
    virtual int getPriorityLevel() const = 0;           // 1 = most urgent
    virtual std::string getPriorityLabel() const = 0;   // e.g. "High Priority"
    virtual std::string getDetails() const = 0;         // type-specific summary
    virtual std::vector<std::string> getExtraFields() const = 0; // for saving
    virtual std::unique_ptr<PatientRecord> clone() const = 0;    // for undo snapshots

    // Non-pure virtual built from the pure virtual calls above.
    virtual std::string describe() const;

    std::string getCategoryName() const { return categoryToString(getCategory()); }

    // Association with appointments (a patient can have many).
    const std::vector<std::string>& getAppointmentIds() const { return appointmentIds_; }
    void setAppointmentIds(const std::vector<std::string>& ids) { appointmentIds_ = ids; }
    bool hasAppointment(const std::string& appointmentId) const;
    void linkAppointment(const std::string& appointmentId);
    void unlinkAppointment(const std::string& appointmentId);

    // Shared validation rules; each throws ValidationException on failure.
    static void validateId(const std::string& id);
    static void validateName(const std::string& name);
    static void validateAge(int age);
    static void validateContact(const std::string& contact);

protected:
    std::string id_;
    std::string name_;
    int age_;
    std::string contact_;
    std::vector<std::string> appointmentIds_;
};

} // namespace scapms::model
