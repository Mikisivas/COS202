// Console tests for the SCAPMS core (no GUI). Run: ./scapms_tests
#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"
#include "controller/ReportGenerator.h"
#include "controller/SampleData.h"
#include "controller/SearchEngine.h"
#include "model/EmergencyPatient.h"
#include "model/GeneralPatient.h"
#include "model/PatientFactory.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

#include <cstdio>
#include <iostream>
#include <string>

using namespace scapms;

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++checks;                                                                \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            std::cerr << "FAIL line " << __LINE__ << ": " #cond << std::endl;    \
        }                                                                        \
    } while (0)

template <typename E, typename F>
bool throws(F f) {
    try {
        f();
    } catch (const E&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

static bool isSorted(const model::PatientList& list, const controller::Comparator& less) {
    for (std::size_t i = 1; i < list.size(); ++i) {
        if (less(*list[i], *list[i - 1])) {
            return false;
        }
    }
    return true;
}

int main() {
    controller::ClinicManager manager;
    controller::AppointmentController appointments(manager);
    controller::loadSampleData(manager);
    auto& db = manager.database();
    CHECK(db.patientCount() == 15);

    // Validation
    CHECK((throws<utils::ValidationException>([] {
        model::GeneralPatient("P0100", "Test Person", 130, "08012345678", "Cough");
    })));
    CHECK((throws<utils::ValidationException>([] {
        model::GeneralPatient("P0100", "", 30, "08012345678", "Cough");
    })));
    CHECK((throws<utils::DuplicateIdException>([&] {
        manager.registerPatient(std::make_unique<model::GeneralPatient>("P0001", "Dup Person", 30, "08012345678", "Cough"));
    })));

    // All four sorts, all four fields
    const controller::SortAlgorithm algorithms[] = {controller::SortAlgorithm::Insertion, controller::SortAlgorithm::Selection,
                                                    controller::SortAlgorithm::Merge, controller::SortAlgorithm::Quick};
    const model::SortField fields[] = {model::SortField::Name, model::SortField::Id, model::SortField::Age,
                                       model::SortField::AppointmentDate};
    for (auto algorithm : algorithms) {
        for (auto field : fields) {
            manager.sortPatients(algorithm, field);
            CHECK(isSorted(db.patients(), controller::Sorter::comparatorFor(field, db)));
            CHECK(db.patientCount() == 15);
        }
    }

    // Searches
    using controller::SearchAlgorithm;
    using controller::SearchEngine;
    using controller::SearchField;
    manager.sortPatients(controller::SortAlgorithm::Merge, model::SortField::Name);
    CHECK((throws<utils::SearchException>([&] { SearchEngine::search(db, SearchField::Id, "P0003", SearchAlgorithm::Binary); })));
    auto linear = SearchEngine::search(db, SearchField::Id, "p0003", SearchAlgorithm::Linear);
    CHECK(linear.matches.size() == 1 && linear.matches[0]->getName() == "Fatima Bello");
    auto byName = SearchEngine::search(db, SearchField::Name, "ngozi umeh", SearchAlgorithm::Binary);
    CHECK(byName.matches.size() == 1);
    auto partial = SearchEngine::search(db, SearchField::Name, "ad", SearchAlgorithm::Recursive);
    CHECK(partial.matches.size() == 3); // Adaeze, Adeyemi, Nnadi
    auto autoName = SearchEngine::search(db, SearchField::Name, "ad", SearchAlgorithm::Auto);
    CHECK(autoName.matches.size() == 3);
    manager.sortPatients(controller::SortAlgorithm::Quick, model::SortField::Id);
    auto binary = SearchEngine::search(db, SearchField::Id, "P0015", SearchAlgorithm::Auto);
    CHECK(binary.algorithmUsed == "Binary Search" && binary.matches.size() == 1);
    auto missing = SearchEngine::search(db, SearchField::Id, "P0999", SearchAlgorithm::Binary);
    CHECK(missing.matches.empty());
    auto emergencies = SearchEngine::search(db, SearchField::Category, "Emergency", SearchAlgorithm::Auto);
    CHECK(emergencies.matches.size() == 3);
    CHECK((throws<utils::SearchException>([&] { SearchEngine::search(db, SearchField::Name, "  ", SearchAlgorithm::Auto); })));

    // Appointments
    const std::string tomorrow = utils::addDays(utils::today(), 1);
    CHECK((throws<utils::AppointmentException>([&] { appointments.schedule("P0002", tomorrow, "10:00", "Dr. Test", "x"); })));
    auto booked = appointments.schedule("P0001", tomorrow, "15:00", "Dr. Test", "");
    CHECK(booked.getReason() == "Persistent headache");
    CHECK((throws<utils::AppointmentException>([&] { appointments.schedule("P0004", tomorrow, "15:00", "dr. test", "x"); })));
    CHECK((throws<utils::AppointmentException>([&] { appointments.schedule("P0003", utils::addDays(utils::today(), -1), "10:00", "Dr. Test", "x"); })));

    // Undo: cancel, remove, delete, modify, add
    appointments.cancel(booked.getId());
    CHECK(db.findAppointment(booked.getId())->getStatus() == model::AppointmentStatus::Cancelled);
    manager.undoLastOperation();
    CHECK(db.findAppointment(booked.getId())->getStatus() == model::AppointmentStatus::Pending);

    appointments.remove(booked.getId());
    CHECK(db.findAppointment(booked.getId()) == nullptr);
    manager.undoLastOperation();
    CHECK(db.findAppointment(booked.getId()) != nullptr);

    const std::size_t apptsBefore = db.appointments().size();
    manager.deletePatient("P0005");
    CHECK(db.findPatient("P0005") == nullptr && db.appointments().size() == apptsBefore - 3);
    manager.undoLastOperation();
    CHECK(db.findPatient("P0005") != nullptr && db.appointments().size() == apptsBefore);

    manager.updatePatient(model::createPatient(model::PatientCategory::Emergency, "P0001", "Adaeze Okafor", 21,
                                               "08031234567", {"Collapsed", "5"}));
    CHECK(db.findPatient("P0001")->getCategory() == model::PatientCategory::Emergency);
    manager.undoLastOperation();
    CHECK(db.findPatient("P0001")->getCategory() == model::PatientCategory::General);
    CHECK(db.findPatient("P0001")->getAppointmentIds().size() == 4);

    auto& added = manager.registerPatient(model::createPatient(model::PatientCategory::General, manager.generatePatientId(),
                                                               "New Person", 40, "08011112222", {"Cough"}));
    const std::string addedId = added.getId();
    CHECK(addedId == "P0016");
    manager.undoLastOperation();
    CHECK(db.findPatient(addedId) == nullptr);
    CHECK((throws<utils::ClinicException>([&] { while (true) manager.undoLastOperation(); })));

    // Queue is FIFO
    manager.enqueue("P0002");
    manager.enqueue("P0007");
    CHECK((throws<utils::ValidationException>([&] { manager.enqueue("P0002"); })));
    CHECK(manager.callNextPatient().patientId == "P0002");
    CHECK(manager.callNextPatient().patientId == "P0007");
    CHECK((throws<utils::ClinicException>([&] { manager.callNextPatient(); })));

    // Cache keeps at most 10 records, most recent first
    for (int i = 1; i <= 15; ++i) {
        char id[8];
        std::snprintf(id, sizeof(id), "P%04d", i);
        manager.getPatient(id);
    }
    CHECK(manager.recentPatients().size() == 10);
    CHECK(manager.recentPatients().front()->getId() == "P0015");

    // Polymorphism: emergency patients first
    auto triage = manager.triageOverview();
    CHECK(triage.size() == 15 && triage.front().find("[High Priority]") == 0);

    // Reports
    controller::ReportGenerator reports(db);
    CHECK(reports.categoryDistribution().rows.size() == 3);
    CHECK(reports.mostFrequentlyAttended().rows.front()[1] == "P0001");
    CHECK(!reports.pendingAppointments().rows.empty());
    CHECK(reports.dailySummary(utils::today()).rows[0][1] == "6");

    // Save and load round trip
    const std::string path = "scapms_test_data.csv";
    manager.saveToFile(path);
    controller::ClinicManager reloaded;
    reloaded.loadFromFile(path);
    CHECK(reloaded.database().patientCount() == db.patientCount());
    CHECK(reloaded.database().appointments().size() == db.appointments().size());
    CHECK((throws<utils::FileException>([&] { reloaded.loadFromFile("does_not_exist.csv"); })));
    CHECK(reloaded.database().patientCount() == db.patientCount()); // failed load keeps data
    std::remove(path.c_str());

    std::cout << (checks - failures) << "/" << checks << " checks passed" << std::endl;
    return failures == 0 ? 0 : 1;
}
