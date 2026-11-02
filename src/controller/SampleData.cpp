#include "controller/SampleData.h"

#include "model/PatientFactory.h"
#include "utils/DateUtils.h"
#include "utils/IDGenerator.h"

namespace scapms::controller {

using model::AppointmentStatus;
using model::PatientCategory;

void loadSampleData(ClinicManager& manager) {
    manager.reset();
    model::ClinicDatabase& db = manager.database();
    const std::string today = utils::today();

    struct PatientSeed {
        PatientCategory category;
        const char* name;
        int age;
        const char* contact;
        std::string extra1;
        std::string extra2;
    };
    const PatientSeed patients[] = {
        {PatientCategory::General, "Adaeze Okafor", 21, "08031234567", "Persistent headache", ""},
        {PatientCategory::Emergency, "Chinedu Eze", 34, "08052223344", "Deep cut on left arm", "4"},
        {PatientCategory::FollowUp, "Fatima Bello", 45, "fatima.bello@mail.com", "Hypertension", utils::addDays(today, -14)},
        {PatientCategory::General, "Tunde Adeyemi", 19, "07061112233", "Fever and chills", ""},
        {PatientCategory::FollowUp, "Ngozi Umeh", 52, "08099887766", "Type 2 diabetes", utils::addDays(today, -30)},
        {PatientCategory::General, "Ibrahim Musa", 23, "09012345678", "Sore throat", ""},
        {PatientCategory::Emergency, "Blessing Ojo", 27, "08134567890", "Severe asthma attack", "5"},
        {PatientCategory::General, "Emeka Nwosu", 20, "emeka.nwosu@student.edu.ng", "Stomach pain", ""},
        {PatientCategory::FollowUp, "Aisha Abdullahi", 38, "08167778899", "Malaria treatment review", utils::addDays(today, -7)},
        {PatientCategory::General, "Kelechi Obi", 22, "07035556677", "Skin rash", ""},
        {PatientCategory::FollowUp, "Funmilayo Akande", 60, "08023334455", "Knee surgery recovery", utils::addDays(today, -21)},
        {PatientCategory::Emergency, "Yusuf Danjuma", 41, "08176665544", "Suspected fracture", "3"},
        {PatientCategory::General, "Chiamaka Nnadi", 18, "09098765432", "Eye irritation", ""},
        {PatientCategory::General, "Segun Bakare", 25, "08045671234", "Back pain", ""},
        {PatientCategory::FollowUp, "Halima Garba", 33, "halima.garba@mail.com", "Antenatal check", utils::addDays(today, -10)},
    };

    for (const PatientSeed& seed : patients) {
        std::vector<std::string> extras = {seed.extra1};
        if (seed.category != PatientCategory::General) {
            extras.push_back(seed.extra2);
        }
        db.addPatient(model::createPatient(seed.category, manager.generatePatientId(), seed.name,
                                           seed.age, seed.contact, extras));
    }

    struct AppointmentSeed {
        int patientIndex;
        int dayOffset;
        const char* time;
        const char* doctor;
        const char* reason;
        AppointmentStatus status;
    };
    const AppointmentSeed appointments[] = {
        {0, -10, "09:00", "Dr. Adebayo", "Headache review", AppointmentStatus::Completed},
        {0, -3, "10:30", "Dr. Adebayo", "Headache follow-up", AppointmentStatus::Completed},
        {0, 2, "11:00", "Dr. Adebayo", "Results discussion", AppointmentStatus::Pending},
        {2, -14, "08:30", "Dr. Okonkwo", "Blood pressure check", AppointmentStatus::Completed},
        {2, 0, "14:00", "Dr. Okonkwo", "Blood pressure review", AppointmentStatus::Pending},
        {3, -5, "13:00", "Dr. Yusuf", "Malaria test", AppointmentStatus::Completed},
        {3, 0, "09:30", "Dr. Yusuf", "Fever review", AppointmentStatus::Completed},
        {4, -30, "10:00", "Dr. Okonkwo", "Diabetes check", AppointmentStatus::Completed},
        {4, -15, "10:00", "Dr. Okonkwo", "Blood sugar test", AppointmentStatus::Completed},
        {4, 0, "15:30", "Dr. Okonkwo", "Diabetes review", AppointmentStatus::Pending},
        {5, 0, "11:30", "Dr. Adebayo", "Throat examination", AppointmentStatus::Cancelled},
        {7, 1, "09:00", "Dr. Yusuf", "Stomach pain review", AppointmentStatus::Pending},
        {8, -7, "12:00", "Dr. Yusuf", "Malaria treatment", AppointmentStatus::Completed},
        {8, 3, "12:00", "Dr. Yusuf", "Treatment review", AppointmentStatus::Pending},
        {9, 0, "10:00", "Dr. Adebayo", "Rash check", AppointmentStatus::Completed},
        {10, -21, "08:00", "Dr. Okonkwo", "Post-surgery check", AppointmentStatus::Completed},
        {10, 5, "08:00", "Dr. Okonkwo", "Physiotherapy plan", AppointmentStatus::Pending},
        {12, 0, "16:30", "Dr. Yusuf", "Eye examination", AppointmentStatus::Pending},
        {13, -2, "14:00", "Dr. Adebayo", "Back pain", AppointmentStatus::Cancelled},
        {13, 4, "14:00", "Dr. Adebayo", "Back pain review", AppointmentStatus::Pending},
        {14, 7, "09:30", "Dr. Okonkwo", "Antenatal visit", AppointmentStatus::Pending},
    };
    for (const AppointmentSeed& seed : appointments) {
        model::PatientRecord* patient = db.patients()[static_cast<std::size_t>(seed.patientIndex)].get();
        model::Appointment appointment(utils::IDGenerator::nextAppointmentId(), patient->getId(),
                                       utils::addDays(today, seed.dayOffset), seed.time,
                                       seed.doctor, seed.reason, seed.status);
        db.addAppointment(appointment);
        patient->linkAppointment(appointment.getId());
    }
}

} // namespace scapms::controller
