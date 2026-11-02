#pragma once
// Hand-written searching algorithms: linear, binary and recursive.
// search() picks the right technique for the current state of the data.

#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

enum class SearchField { Id, Name, Category };
enum class SearchAlgorithm { Auto, Linear, Binary, Recursive };

struct SearchResult {
    std::vector<model::PatientRecord*> matches;
    std::string algorithmUsed;
    long comparisons = 0;
    std::string note;
};

class SearchEngine {
public:
    // Linear search by exact patient ID. Works on unsorted data. O(n).
    static int linearSearchById(const model::PatientList& list, const std::string& id,
                                long& comparisons);

    // Linear scan for names containing the term. O(n).
    static std::vector<model::PatientRecord*> linearSearchByName(const model::PatientList& list,
                                                                 const std::string& term,
                                                                 long& comparisons);

    // Binary search. The list MUST be sorted by the same field. O(log n).
    // field must be SortField::Id or SortField::Name.
    static int binarySearch(const model::PatientList& list, const std::string& key,
                            model::SortField field, long& comparisons);

    // Recursive divide-and-conquer search for names containing the term.
    // Splits the range in half until one record remains (recursion depth log n).
    static void recursiveSearchByName(const model::PatientList& list, const std::string& term,
                                      long low, long high,
                                      std::vector<model::PatientRecord*>& results,
                                      long& comparisons);

    static std::vector<model::PatientRecord*> searchByCategory(const model::PatientList& list,
                                                               model::PatientCategory category,
                                                               long& comparisons);

    // Validates the input, chooses an algorithm and returns the matches.
    static SearchResult search(const model::ClinicDatabase& db, SearchField field,
                               const std::string& term, SearchAlgorithm algorithm);

    static std::string name(SearchAlgorithm algorithm);
};

} // namespace scapms::controller
