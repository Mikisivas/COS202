#pragma once
// Generates sequential, zero-padded IDs such as P0001 and A0001.
// observe() keeps the counters ahead of IDs loaded from a file or typed by
// the user, so a generated ID never collides with an existing one.

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace scapms::utils {

class IDGenerator {
public:
    static std::string nextPatientId() { return format('P', ++patientCounter_); }
    static std::string nextAppointmentId() { return format('A', ++appointmentCounter_); }
    static std::string peekNextPatientId() { return format('P', patientCounter_ + 1); }

    static void observe(const std::string& id) {
        if (id.size() < 2) {
            return;
        }
        int number = 0;
        for (std::size_t i = 1; i < id.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(id[i]))) {
                return;
            }
            number = number * 10 + (id[i] - '0');
        }
        if (id[0] == 'P' && number > patientCounter_) {
            patientCounter_ = number;
        } else if (id[0] == 'A' && number > appointmentCounter_) {
            appointmentCounter_ = number;
        }
    }

    static void reset() {
        patientCounter_ = 0;
        appointmentCounter_ = 0;
    }

private:
    static std::string format(char prefix, int number) {
        std::ostringstream out;
        out << prefix << std::setw(4) << std::setfill('0') << number;
        return out.str();
    }

    static inline int patientCounter_ = 0;
    static inline int appointmentCounter_ = 0;
};

} // namespace scapms::utils
