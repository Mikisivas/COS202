#pragma once
// Hand-written sorting algorithms (no std::sort).
// They sort the database's vector of unique_ptr<PatientRecord> in place,
// moving pointers only, so the patient objects themselves never move.

#include "model/ClinicDatabase.h"

#include <functional>
#include <string>

namespace scapms::controller {

enum class SortAlgorithm { Insertion, Selection, Merge, Quick };

// Returns true when a should come before b.
using Comparator = std::function<bool(const model::PatientRecord& a, const model::PatientRecord& b)>;

struct SortStats {
    long long comparisons = 0;
    long long moves = 0;
    double milliseconds = 0.0;
};

class Sorter {
public:
    static SortStats sort(model::PatientList& list, SortAlgorithm algorithm,
                          const Comparator& less);

    static void insertionSort(model::PatientList& list, const Comparator& less, SortStats& stats);
    static void selectionSort(model::PatientList& list, const Comparator& less, SortStats& stats);
    static void mergeSort(model::PatientList& list, const Comparator& less, SortStats& stats);
    static void quickSort(model::PatientList& list, const Comparator& less, SortStats& stats);

    static Comparator comparatorFor(model::SortField field, const model::ClinicDatabase& db);

    static std::string name(SortAlgorithm algorithm);
    static std::string complexity(SortAlgorithm algorithm);
    static std::string explain(SortAlgorithm algorithm);

private:
    static void mergeSortRange(model::PatientList& list, model::PatientList& buffer,
                               std::size_t low, std::size_t high, const Comparator& less,
                               SortStats& stats);
    static void quickSortRange(model::PatientList& list, long low, long high,
                               const Comparator& less, SortStats& stats);
};

} // namespace scapms::controller
