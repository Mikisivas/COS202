#include "controller/SearchEngine.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

using model::PatientList;
using model::PatientRecord;
using model::SortField;
using utils::SearchException;

int SearchEngine::linearSearchById(const PatientList& list, const std::string& id,
                                   long& comparisons) {
    for (std::size_t i = 0; i < list.size(); ++i) {
        ++comparisons;
        if (list[i]->getId() == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<PatientRecord*> SearchEngine::linearSearchByName(const PatientList& list,
                                                             const std::string& term,
                                                             long& comparisons) {
    std::vector<PatientRecord*> results;
    for (const auto& patient : list) {
        ++comparisons;
        if (utils::containsIgnoreCase(patient->getName(), term)) {
            results.push_back(patient.get());
        }
    }
    return results;
}

namespace {
std::string keyOf(const PatientRecord& patient, SortField field) {
    return field == SortField::Name ? utils::toLower(patient.getName()) : patient.getId();
}
} // namespace

int SearchEngine::binarySearch(const PatientList& list, const std::string& key, SortField field,
                               long& comparisons) {
    const std::string target = field == SortField::Name ? utils::toLower(key) : key;
    long low = 0;
    long high = static_cast<long>(list.size()) - 1;
    while (low <= high) {
        const long mid = low + (high - low) / 2; // avoids overflow of (low + high)
        const std::string value = keyOf(*list[static_cast<std::size_t>(mid)], field);
        ++comparisons;
        if (value == target) {
            return static_cast<int>(mid);
        }
        if (value < target) {
            low = mid + 1;  // target is in the right half
        } else {
            high = mid - 1; // target is in the left half
        }
    }
    return -1;
}

void SearchEngine::recursiveSearchByName(const PatientList& list, const std::string& term,
                                         long low, long high,
                                         std::vector<PatientRecord*>& results,
                                         long& comparisons) {
    if (low > high) {
        return; // base case 1: empty range
    }
    if (low == high) {
        // base case 2: a single record, check it
        ++comparisons;
        PatientRecord* patient = list[static_cast<std::size_t>(low)].get();
        if (utils::containsIgnoreCase(patient->getName(), term)) {
            results.push_back(patient);
        }
        return;
    }
    const long mid = low + (high - low) / 2;
    recursiveSearchByName(list, term, low, mid, results, comparisons);      // left half
    recursiveSearchByName(list, term, mid + 1, high, results, comparisons); // right half
}

std::vector<PatientRecord*> SearchEngine::searchByCategory(const PatientList& list,
                                                           model::PatientCategory category,
                                                           long& comparisons) {
    std::vector<PatientRecord*> results;
    for (const auto& patient : list) {
        ++comparisons;
        if (patient->getCategory() == category) {
            results.push_back(patient.get());
        }
    }
    return results;
}

SearchResult SearchEngine::search(const model::ClinicDatabase& db, SearchField field,
                                  const std::string& rawTerm, SearchAlgorithm algorithm) {
    const std::string term = utils::trim(rawTerm);
    if (term.empty()) {
        throw SearchException("Please enter a search keyword.");
    }
    const PatientList& list = db.patients();
    SearchResult result;

    if (field == SearchField::Id) {
        const std::string id = utils::toUpper(term);
        try {
            PatientRecord::validateId(id);
        } catch (const utils::ValidationException&) {
            throw SearchException("'" + term + "' is not a valid patient ID. IDs look like P0001.");
        }
        if (algorithm == SearchAlgorithm::Recursive) {
            throw SearchException("Recursive search works on patient names. Choose 'Name' as the search field.");
        }
        const bool sortedById = db.sortedBy() == SortField::Id;
        if (algorithm == SearchAlgorithm::Binary && !sortedById) {
            throw SearchException("Binary search needs the records sorted by Patient ID. "
                                  "Sort by Patient ID first, or choose Linear or Auto.");
        }
        int index = -1;
        if (algorithm == SearchAlgorithm::Binary ||
            (algorithm == SearchAlgorithm::Auto && sortedById)) {
            index = binarySearch(list, id, SortField::Id, result.comparisons);
            result.algorithmUsed = "Binary Search";
            result.note = "Records are sorted by ID, so binary search halves the range each step.";
        } else {
            index = linearSearchById(list, id, result.comparisons);
            result.algorithmUsed = "Linear Search";
            result.note = "Records are not sorted by ID, so every record is checked in turn.";
        }
        if (index >= 0) {
            result.matches.push_back(list[static_cast<std::size_t>(index)].get());
        }
        return result;
    }

    if (field == SearchField::Name) {
        const bool sortedByName = db.sortedBy() == SortField::Name;
        if (algorithm == SearchAlgorithm::Binary && !sortedByName) {
            throw SearchException("Binary search needs the records sorted by Patient Name. "
                                  "Sort by Patient Name first, or choose Linear, Recursive or Auto.");
        }
        if (algorithm == SearchAlgorithm::Binary ||
            (algorithm == SearchAlgorithm::Auto && sortedByName)) {
            const int index = binarySearch(list, term, SortField::Name, result.comparisons);
            result.algorithmUsed = "Binary Search (exact name)";
            if (index >= 0) {
                // Equal names sit next to each other in sorted data: collect neighbours.
                const std::string target = utils::toLower(term);
                long start = index;
                while (start > 0 && utils::toLower(list[static_cast<std::size_t>(start - 1)]->getName()) == target) {
                    --start;
                    ++result.comparisons;
                }
                for (std::size_t i = static_cast<std::size_t>(start); i < list.size(); ++i) {
                    ++result.comparisons;
                    if (utils::toLower(list[i]->getName()) != target) {
                        break;
                    }
                    result.matches.push_back(list[i].get());
                }
                result.note = "Records are sorted by name, so binary search found the exact name.";
                return result;
            }
            if (algorithm == SearchAlgorithm::Binary) {
                result.note = "Binary search matches full names only. Try Recursive for partial names.";
                return result;
            }
            // Auto: no exact match, fall back to a partial-name recursive search.
            result.note = "No exact name match, so Auto switched to recursive partial matching.";
        }
        if (algorithm == SearchAlgorithm::Linear) {
            result.matches = linearSearchByName(list, term, result.comparisons);
            result.algorithmUsed = "Linear Search (partial name)";
            result.note = "Checks every record for names containing the keyword.";
            return result;
        }
        recursiveSearchByName(list, term, 0, static_cast<long>(list.size()) - 1, result.matches,
                              result.comparisons);
        result.algorithmUsed = "Recursive Search (partial name)";
        if (result.note.empty()) {
            result.note = "Splits the list in half recursively and checks each record for the keyword.";
        }
        return result;
    }

    // Category search
    if (algorithm == SearchAlgorithm::Binary || algorithm == SearchAlgorithm::Recursive) {
        throw SearchException("Category search uses linear search. Choose Linear or Auto.");
    }
    model::PatientCategory category;
    try {
        category = model::categoryFromString(term);
    } catch (const utils::ValidationException&) {
        throw SearchException("Category must be General, Emergency or Follow-Up.");
    }
    result.matches = searchByCategory(list, category, result.comparisons);
    result.algorithmUsed = "Linear Search (category)";
    result.note = "Many patients share a category, so every record is checked.";
    return result;
}

std::string SearchEngine::name(SearchAlgorithm algorithm) {
    switch (algorithm) {
    case SearchAlgorithm::Auto:      return "Auto (choose for me)";
    case SearchAlgorithm::Linear:    return "Linear Search";
    case SearchAlgorithm::Binary:    return "Binary Search";
    case SearchAlgorithm::Recursive: return "Recursive Search";
    }
    return "Unknown";
}

} // namespace scapms::controller
