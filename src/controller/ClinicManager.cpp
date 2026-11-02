#include "controller/ClinicManager.h"

#include "utils/Exceptions.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"

namespace scapms::controller {

using model::Appointment;
using model::PatientRecord;
using utils::ClinicException;
using utils::NotFoundException;

std::string triageLine(const PatientRecord& patient) {
    // Every call below is virtual: the actual subclass decides the output.
    return "[" + patient.getPriorityLabel() + "] " + patient.getId() + " " + patient.getName() +
           " - " + patient.getDetails();
}

ClinicManager::ClinicManager() {
    clearRecent();
}

// ===================== patients =====================

std::string ClinicManager::generatePatientId() const {
    std::string id;
    do {
        id = utils::IDGenerator::nextPatientId();
    } while (db_.findPatient(id) != nullptr);
    return id;
}

PatientRecord& ClinicManager::registerPatient(std::unique_ptr<PatientRecord> patient) {
    if (!patient) {
        throw utils::ValidationException("Cannot register an empty patient record.");
    }
    const std::string id = patient->getId();
    db_.addPatient(std::move(patient)); // throws DuplicateIdException
    utils::IDGenerator::observe(id);

    PatientRecord* added = db_.findPatient(id);
    touchRecent(added);

    AdminAction action;
    action.type = ActionType::AddPatient;
    action.targetId = id;
    action.description = "Registered patient " + id + " (" + added->getName() + ")";
    recordAction(std::move(action));
    return *added;
}

void ClinicManager::updatePatient(std::unique_ptr<PatientRecord> updated) {
    if (!updated) {
        throw utils::ValidationException("Cannot save an empty patient record.");
    }
    const std::string id = updated->getId();
    PatientRecord* existing = db_.findPatient(id);
    if (existing == nullptr) {
        throw NotFoundException("No patient with ID " + id + ". Select a patient before updating.");
    }

    AdminAction action;
    action.type = ActionType::ModifyPatient;
    action.targetId = id;
    action.description = "Modified patient " + id + " (" + existing->getName() + ")";
    action.patientSnapshot = existing->clone();

    updated->setAppointmentIds(existing->getAppointmentIds()); // keep the association
    evictRecent(existing); // old object is about to be destroyed
    db_.replacePatient(std::move(updated));
    touchRecent(db_.findPatient(id));
    recordAction(std::move(action));
}

void ClinicManager::deletePatient(const std::string& id) {
    PatientRecord* existing = db_.findPatient(id);
    if (existing == nullptr) {
        throw NotFoundException("No patient with ID " + id + ".");
    }
    AdminAction action;
    action.type = ActionType::DeletePatient;
    action.targetId = id;
    action.description = "Deleted patient " + id + " (" + existing->getName() + ")";
    action.patientSnapshot = existing->clone();
    action.appointmentSnapshots = db_.appointmentsFor(id);

    removePatientCompletely(id);
    recordAction(std::move(action));
}

void ClinicManager::removePatientCompletely(const std::string& id) {
    for (const Appointment& appointment : db_.appointmentsFor(id)) {
        db_.removeAppointment(appointment.getId());
    }
    evictRecent(db_.findPatient(id));
    removeFromQueue(id);
    db_.removePatient(id);
}

PatientRecord* ClinicManager::getPatient(const std::string& id) {
    // 1. Look in the small cache first (at most 10 checks).
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i]->getId() == id) {
            PatientRecord* found = recentPatients_[i];
            ++cacheHits_;
            touchRecent(found);
            return found;
        }
    }
    // 2. Fall back to the full database.
    ++cacheMisses_;
    PatientRecord* found = db_.findPatient(id);
    touchRecent(found);
    return found;
}

// ===================== waiting queue =====================

void ClinicManager::enqueue(const std::string& patientId) {
    PatientRecord* patient = getPatient(patientId);
    if (patient == nullptr) {
        throw NotFoundException("No patient with ID " + patientId + ".");
    }
    if (isQueued(patientId)) {
        throw utils::ValidationException(patient->getName() + " is already in the waiting queue.");
    }
    waitingQueue_.push({patientId, std::time(nullptr)});
}

QueueEntry ClinicManager::callNextPatient() {
    if (waitingQueue_.empty()) {
        throw ClinicException("The waiting queue is empty.");
    }
    QueueEntry next = waitingQueue_.front(); // first in ...
    waitingQueue_.pop();                     // ... first out
    return next;
}

std::vector<QueueEntry> ClinicManager::queueSnapshot() const {
    std::vector<QueueEntry> entries;
    std::queue<QueueEntry> copy = waitingQueue_; // std::queue has no iterators
    while (!copy.empty()) {
        entries.push_back(copy.front());
        copy.pop();
    }
    return entries;
}

bool ClinicManager::isQueued(const std::string& patientId) const {
    for (const QueueEntry& entry : queueSnapshot()) {
        if (entry.patientId == patientId) {
            return true;
        }
    }
    return false;
}

void ClinicManager::removeFromQueue(const std::string& patientId) {
    std::queue<QueueEntry> kept;
    while (!waitingQueue_.empty()) {
        if (waitingQueue_.front().patientId != patientId) {
            kept.push(waitingQueue_.front());
        }
        waitingQueue_.pop();
    }
    waitingQueue_ = kept;
}

// ===================== undo stack =====================

void ClinicManager::recordAction(AdminAction action) {
    action.performedAt = std::time(nullptr);
    undoStack_.push(std::move(action));
}

std::string ClinicManager::undoLastOperation() {
    if (undoStack_.empty()) {
        throw ClinicException("There is nothing to undo.");
    }
    AdminAction action = undoStack_.top(); // most recent operation
    undoStack_.pop();

    switch (action.type) {
    case ActionType::AddPatient:
        if (db_.findPatient(action.targetId) != nullptr) {
            removePatientCompletely(action.targetId);
        }
        break;

    case ActionType::DeletePatient: {
        db_.addPatient(action.patientSnapshot->clone());
        for (const Appointment& appointment : action.appointmentSnapshots) {
            db_.addAppointment(appointment);
        }
        touchRecent(db_.findPatient(action.targetId));
        break;
    }

    case ActionType::ModifyPatient: {
        PatientRecord* current = db_.findPatient(action.targetId);
        if (current == nullptr) {
            throw NotFoundException("Cannot undo: patient " + action.targetId + " no longer exists.");
        }
        std::unique_ptr<PatientRecord> restored = action.patientSnapshot->clone();
        restored->setAppointmentIds(current->getAppointmentIds());
        evictRecent(current);
        db_.replacePatient(std::move(restored));
        touchRecent(db_.findPatient(action.targetId));
        break;
    }

    case ActionType::RemoveAppointment: {
        const Appointment& appointment = action.appointmentSnapshots.front();
        db_.addAppointment(appointment);
        if (PatientRecord* patient = db_.findPatient(appointment.getPatientId())) {
            patient->linkAppointment(appointment.getId());
        }
        break;
    }

    case ActionType::CancelAppointment: {
        Appointment* current = db_.findAppointment(action.targetId);
        if (current == nullptr) {
            throw NotFoundException("Cannot undo: appointment " + action.targetId + " no longer exists.");
        }
        current->setStatus(action.appointmentSnapshots.front().getStatus());
        break;
    }
    }
    return "Undone: " + action.description;
}

std::vector<std::string> ClinicManager::undoHistory() const {
    std::vector<std::string> history;
    std::stack<AdminAction> copy = undoStack_;
    while (!copy.empty()) {
        history.push_back(copy.top().description);
        copy.pop();
    }
    return history;
}

// ===================== recent-patient cache =====================

void ClinicManager::touchRecent(PatientRecord* patient) {
    if (patient == nullptr) {
        return;
    }
    int position = -1;
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == patient) {
            position = i;
            break;
        }
    }
    if (position == -1) {
        // New entry: grow, or drop the least recently used one (last slot).
        position = recentCount_ < CACHE_SIZE ? recentCount_++ : CACHE_SIZE - 1;
    }
    for (int i = position; i > 0; --i) {
        recentPatients_[i] = recentPatients_[i - 1];
    }
    recentPatients_[0] = patient;
}

void ClinicManager::evictRecent(const PatientRecord* patient) {
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == patient) {
            for (int j = i; j < recentCount_ - 1; ++j) {
                recentPatients_[j] = recentPatients_[j + 1];
            }
            --recentCount_;
            recentPatients_[recentCount_] = nullptr;
            return;
        }
    }
}

void ClinicManager::clearRecent() {
    for (int i = 0; i < CACHE_SIZE; ++i) {
        recentPatients_[i] = nullptr;
    }
    recentCount_ = 0;
}

std::vector<PatientRecord*> ClinicManager::recentPatients() const {
    return std::vector<PatientRecord*>(recentPatients_, recentPatients_ + recentCount_);
}

// ===================== sorting =====================

SortStats ClinicManager::sortPatients(SortAlgorithm algorithm, model::SortField field) {
    const Comparator less = Sorter::comparatorFor(field, db_);
    SortStats stats = Sorter::sort(db_.patients(), algorithm, less);
    db_.setSortedBy(field);
    return stats;
}

// ===================== polymorphism =====================

std::vector<std::string> ClinicManager::triageOverview() const {
    // Collect base-class pointers, order by priority (insertion sort, stable).
    std::vector<const PatientRecord*> ordered;
    for (const auto& patient : db_.patients()) {
        ordered.push_back(patient.get());
    }
    for (std::size_t i = 1; i < ordered.size(); ++i) {
        const PatientRecord* key = ordered[i];
        std::size_t j = i;
        while (j > 0 && ordered[j - 1]->getPriorityLevel() > key->getPriorityLevel()) {
            ordered[j] = ordered[j - 1];
            --j;
        }
        ordered[j] = key;
    }
    std::vector<std::string> lines;
    for (const PatientRecord* patient : ordered) {
        lines.push_back(triageLine(*patient));
    }
    return lines;
}

// ===================== persistence =====================

void ClinicManager::saveToFile(const std::string& path) const {
    utils::FileHandler::save(path, db_);
}

void ClinicManager::loadFromFile(const std::string& path) {
    model::ClinicDatabase loaded;
    utils::FileHandler::load(path, loaded); // throws FileException; current data untouched
    reset();
    db_ = std::move(loaded);
    for (const auto& patient : db_.patients()) {
        utils::IDGenerator::observe(patient->getId());
    }
    for (const auto& appointment : db_.appointments()) {
        utils::IDGenerator::observe(appointment.getId());
    }
}

void ClinicManager::reset() {
    clearRecent();
    db_.clear();
    waitingQueue_ = std::queue<QueueEntry>();
    undoStack_ = std::stack<AdminAction>();
    cacheHits_ = 0;
    cacheMisses_ = 0;
    utils::IDGenerator::reset();
}

} // namespace scapms::controller
