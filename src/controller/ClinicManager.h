#pragma once
// Main business-logic controller. Owns the ClinicDatabase and the three
// supporting data structures the brief asks for:
//   std::queue  -> patients waiting for consultation (first in, first out)
//   std::stack  -> undo history for administrative operations
//   fixed array -> cache of the 10 most recently accessed patient records

#include "controller/Sorter.h"
#include "model/ClinicDatabase.h"

#include <ctime>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace scapms::controller {

struct QueueEntry {
    std::string patientId;
    std::time_t joinedAt;
};

enum class ActionType { AddPatient, DeletePatient, ModifyPatient, RemoveAppointment, CancelAppointment };

// One entry on the undo stack: enough data to reverse the operation.
struct AdminAction {
    ActionType type;
    std::string targetId;
    std::string description;
    std::shared_ptr<const model::PatientRecord> patientSnapshot;
    std::vector<model::Appointment> appointmentSnapshots;
    std::time_t performedAt = 0;
};

class ClinicManager {
public:
    static constexpr int CACHE_SIZE = 10;

    ClinicManager();

    model::ClinicDatabase& database() { return db_; }
    const model::ClinicDatabase& database() const { return db_; }

    // ---- patient operations ----
    std::string generatePatientId() const;
    model::PatientRecord& registerPatient(std::unique_ptr<model::PatientRecord> patient);
    void updatePatient(std::unique_ptr<model::PatientRecord> updated);
    void deletePatient(const std::string& id);
    model::PatientRecord* getPatient(const std::string& id); // checks the cache first

    // ---- waiting queue (std::queue) ----
    void enqueue(const std::string& patientId);
    QueueEntry callNextPatient();
    std::vector<QueueEntry> queueSnapshot() const;
    bool isQueued(const std::string& patientId) const;
    std::size_t queueSize() const { return waitingQueue_.size(); }

    // ---- undo (std::stack) ----
    void recordAction(AdminAction action);
    bool canUndo() const { return !undoStack_.empty(); }
    std::string undoLastOperation();
    std::vector<std::string> undoHistory() const; // newest first

    // ---- recently accessed cache (fixed-size array) ----
    std::vector<model::PatientRecord*> recentPatients() const;
    int cacheHits() const { return cacheHits_; }
    int cacheMisses() const { return cacheMisses_; }

    // ---- sorting ----
    SortStats sortPatients(SortAlgorithm algorithm, model::SortField field);

    // ---- polymorphism demo ----
    std::vector<std::string> triageOverview() const;

    // ---- persistence ----
    void saveToFile(const std::string& path) const;
    void loadFromFile(const std::string& path);
    void reset();

private:
    void touchRecent(model::PatientRecord* patient);
    void evictRecent(const model::PatientRecord* patient);
    void clearRecent();
    void removeFromQueue(const std::string& patientId);
    void removePatientCompletely(const std::string& id);

    model::ClinicDatabase db_;
    std::queue<QueueEntry> waitingQueue_;
    std::stack<AdminAction> undoStack_;

    model::PatientRecord* recentPatients_[CACHE_SIZE]; // most recent at index 0
    int recentCount_ = 0;
    int cacheHits_ = 0;
    int cacheMisses_ = 0;
};

// Processes any patient through a base-class reference (runtime polymorphism).
std::string triageLine(const model::PatientRecord& patient);

} // namespace scapms::controller
