#include "controller/Sorter.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

#include <chrono>
#include <map>
#include <utility>

namespace scapms::controller {

using model::PatientList;
using model::PatientRecord;

SortStats Sorter::sort(PatientList& list, SortAlgorithm algorithm, const Comparator& less) {
    SortStats stats;
    const auto start = std::chrono::steady_clock::now();
    switch (algorithm) {
    case SortAlgorithm::Insertion: insertionSort(list, less, stats); break;
    case SortAlgorithm::Selection: selectionSort(list, less, stats); break;
    case SortAlgorithm::Merge:     mergeSort(list, less, stats); break;
    case SortAlgorithm::Quick:     quickSort(list, less, stats); break;
    }
    const auto end = std::chrono::steady_clock::now();
    stats.milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
    return stats;
}

// ---------------------------------------------------------------------------
// Insertion sort: take each record and slide it left until it is in place.
// O(n) on nearly sorted data, O(n^2) worst case, stable, in place.
// ---------------------------------------------------------------------------
void Sorter::insertionSort(PatientList& list, const Comparator& less, SortStats& stats) {
    for (std::size_t i = 1; i < list.size(); ++i) {
        std::unique_ptr<PatientRecord> key = std::move(list[i]);
        ++stats.moves;
        std::size_t j = i;
        while (j > 0) {
            ++stats.comparisons;
            if (!less(*key, *list[j - 1])) {
                break;
            }
            list[j] = std::move(list[j - 1]);
            ++stats.moves;
            --j;
        }
        list[j] = std::move(key);
        ++stats.moves;
    }
}

// ---------------------------------------------------------------------------
// Selection sort: find the smallest remaining record and swap it forward.
// Always O(n^2) comparisons but at most n-1 swaps. Not stable.
// ---------------------------------------------------------------------------
void Sorter::selectionSort(PatientList& list, const Comparator& less, SortStats& stats) {
    for (std::size_t i = 0; i + 1 < list.size(); ++i) {
        std::size_t smallest = i;
        for (std::size_t j = i + 1; j < list.size(); ++j) {
            ++stats.comparisons;
            if (less(*list[j], *list[smallest])) {
                smallest = j;
            }
        }
        if (smallest != i) {
            std::swap(list[i], list[smallest]);
            stats.moves += 3;
        }
    }
}

// ---------------------------------------------------------------------------
// Merge sort (recursive): split in half, sort each half, merge them.
// O(n log n) in every case, stable, needs an O(n) buffer.
// ---------------------------------------------------------------------------
void Sorter::mergeSort(PatientList& list, const Comparator& less, SortStats& stats) {
    PatientList buffer(list.size());
    mergeSortRange(list, buffer, 0, list.size(), less, stats);
}

void Sorter::mergeSortRange(PatientList& list, PatientList& buffer, std::size_t low,
                            std::size_t high, const Comparator& less, SortStats& stats) {
    if (high - low < 2) {
        return; // base case: 0 or 1 record is already sorted
    }
    const std::size_t mid = low + (high - low) / 2;
    mergeSortRange(list, buffer, low, mid, less, stats);   // recursive call: left half
    mergeSortRange(list, buffer, mid, high, less, stats);  // recursive call: right half

    std::size_t left = low;
    std::size_t right = mid;
    std::size_t out = low;
    while (left < mid && right < high) {
        ++stats.comparisons;
        // "<" on the right element keeps equal keys in original order (stable).
        if (less(*list[right], *list[left])) {
            buffer[out++] = std::move(list[right++]);
        } else {
            buffer[out++] = std::move(list[left++]);
        }
        ++stats.moves;
    }
    while (left < mid) {
        buffer[out++] = std::move(list[left++]);
        ++stats.moves;
    }
    while (right < high) {
        buffer[out++] = std::move(list[right++]);
        ++stats.moves;
    }
    for (std::size_t k = low; k < high; ++k) {
        list[k] = std::move(buffer[k]);
        ++stats.moves;
    }
}

// ---------------------------------------------------------------------------
// Quick sort (recursive): partition around a pivot, then sort each side.
// Middle element as pivot avoids the O(n^2) case on already sorted lists.
// ---------------------------------------------------------------------------
void Sorter::quickSort(PatientList& list, const Comparator& less, SortStats& stats) {
    if (!list.empty()) {
        quickSortRange(list, 0, static_cast<long>(list.size()) - 1, less, stats);
    }
}

void Sorter::quickSortRange(PatientList& list, long low, long high, const Comparator& less,
                            SortStats& stats) {
    if (low >= high) {
        return;
    }
    const long mid = low + (high - low) / 2;
    std::swap(list[mid], list[high]); // move pivot to the end
    stats.moves += 3;
    const PatientRecord& pivot = *list[high];

    long boundary = low;
    for (long j = low; j < high; ++j) {
        ++stats.comparisons;
        if (less(*list[j], pivot)) {
            if (boundary != j) {
                std::swap(list[boundary], list[j]);
                stats.moves += 3;
            }
            ++boundary;
        }
    }
    std::swap(list[boundary], list[high]); // pivot into its final place
    stats.moves += 3;

    quickSortRange(list, low, boundary - 1, less, stats);
    quickSortRange(list, boundary + 1, high, less, stats);
}

// ---------------------------------------------------------------------------
// Comparators. Ties fall back to the patient ID so every algorithm gives the
// same final order.
// ---------------------------------------------------------------------------
Comparator Sorter::comparatorFor(model::SortField field, const model::ClinicDatabase& db) {
    switch (field) {
    case model::SortField::Name:
        return [](const PatientRecord& a, const PatientRecord& b) {
            const std::string nameA = utils::toLower(a.getName());
            const std::string nameB = utils::toLower(b.getName());
            if (nameA != nameB) {
                return nameA < nameB;
            }
            return a.getId() < b.getId();
        };
    case model::SortField::Id:
        return [](const PatientRecord& a, const PatientRecord& b) {
            return a.getId() < b.getId();
        };
    case model::SortField::Age:
        return [](const PatientRecord& a, const PatientRecord& b) {
            if (a.getAge() != b.getAge()) {
                return a.getAge() < b.getAge();
            }
            return a.getId() < b.getId();
        };
    case model::SortField::AppointmentDate: {
        // Pre-compute each patient's next pending appointment once.
        std::map<std::string, std::string> nextAppointment;
        for (const auto& appointment : db.appointments()) {
            if (appointment.getStatus() != model::AppointmentStatus::Pending) {
                continue;
            }
            const std::string key = appointment.dateTimeKey();
            auto found = nextAppointment.find(appointment.getPatientId());
            if (found == nextAppointment.end() || key < found->second) {
                nextAppointment[appointment.getPatientId()] = key;
            }
        }
        return [nextAppointment](const PatientRecord& a, const PatientRecord& b) {
            const auto lookup = [&nextAppointment](const std::string& id) {
                auto found = nextAppointment.find(id);
                return found == nextAppointment.end() ? std::string("9999") : found->second;
            };
            const std::string keyA = lookup(a.getId());
            const std::string keyB = lookup(b.getId());
            if (keyA != keyB) {
                return keyA < keyB;
            }
            return a.getId() < b.getId();
        };
    }
    case model::SortField::None:
        break;
    }
    throw utils::ValidationException("Please choose a field to sort by.");
}

std::string Sorter::name(SortAlgorithm algorithm) {
    switch (algorithm) {
    case SortAlgorithm::Insertion: return "Insertion Sort";
    case SortAlgorithm::Selection: return "Selection Sort";
    case SortAlgorithm::Merge:     return "Merge Sort";
    case SortAlgorithm::Quick:     return "Quick Sort";
    }
    return "Unknown";
}

std::string Sorter::complexity(SortAlgorithm algorithm) {
    switch (algorithm) {
    case SortAlgorithm::Insertion: return "Best O(n), worst O(n^2), stable, in place";
    case SortAlgorithm::Selection: return "Always O(n^2), at most n-1 swaps, not stable";
    case SortAlgorithm::Merge:     return "Always O(n log n), stable, O(n) extra memory";
    case SortAlgorithm::Quick:     return "Average O(n log n), worst O(n^2), in place, not stable";
    }
    return "";
}

std::string Sorter::explain(SortAlgorithm algorithm) {
    switch (algorithm) {
    case SortAlgorithm::Insertion:
        return "Good for the clinic's day-to-day list. Staff add patients one at a time, so the "
               "list is usually almost sorted and insertion sort finishes in close to one pass.";
    case SortAlgorithm::Selection:
        return "Makes the fewest swaps (at most n-1). Simple and predictable, but it always "
               "compares every pair, so keep it for small lists.";
    case SortAlgorithm::Merge:
        return "Best for large record sets. It stays O(n log n) whatever the input order and is "
               "stable, so two patients with the same name keep their registration order.";
    case SortAlgorithm::Quick:
        return "Usually the fastest on large, randomly ordered lists and needs no extra buffer. "
               "The middle pivot protects it from already sorted input.";
    }
    return "";
}

} // namespace scapms::controller
