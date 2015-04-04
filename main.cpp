#include <algorithm>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_set>
#include <iterator>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <utility>


template <class Predicate>
std::vector<std::string> Split(const std::string& string, Predicate is_delimiter) {
    std::vector<std::string> substrings;
    std::string substring;
    for (char character : string) {
        if (character != is_delimiter) {
            substring.push_back(character);
        } else if (!substring.empty()) {
            substrings.push_back(substring);
            substring.clear();
        }
    }
    return substrings;
}
/*
// Wildcard is a character that may be substituted
// for any of all possible characters
class WildcardMatcher {
public:
    WildcardMatcher():
            number_of_words_(0),
            pattern_length_(0) {
    }

    void Init(const std::string& pattern, char wildcard);

    // Resets matcher to start scanning new stream
    void Reset();

    /* ¬ данном случае Callback -- это функци€,
    * котора€ будет вызвана при наступлении
    * событи€ "суффикс совпал с шаблоном".
    * ѕочему мы выбрали именно этот способ сообщить
    * об этом событии? ћожно рассмотреть альтернативы:
    * вернуть bool из Scan, прин€ть итератор и записать
    * в него значение. ¬ первом случае, значение bool,
    * возвращенное из Scan, будет иметь непон€тный
    * смысл. True -- в смысле все считалось успешно?
    * True -- произошло совпадение? ¬ случае итератора,
    * совершенно не €сно, какое значение туда записывать
    * (подошедший суффикс, true, ...?). Ѕолее того, обычно,
    * если при сканировании потока мы наткнулись на
    * совпадение, то нам нужно как-то на это отреагировать,
    * это действие и есть наш Callback on_match.
    */
    // Scans new character and calls on_match() if
    // suffix of scanned characters matches pattern
/*
    template<class Callback>
    void Scan(char character, Callback on_match);

private:
    // Storing only O(|pattern|) elements allows us
    // to consume only O(|pattern|) memory for matcher
    std::deque<size_t> words_occurrences_by_position_;
    //TODO aho_corasick::NodeReference state_;
    size_t number_of_words_;
    size_t pattern_length_;
    //TODO std::unique_ptr<aho_corasick::Automaton> aho_corasick_automaton_;
};
*/
void Print(const std::vector<size_t>& sequence) {
    for (auto element : sequence) {
        std::cout << element << " ";
    }
    std::cout << "\n";
}

std::string ReadString(std::istream& input_stream) {
    std::string result;
    input_stream >> result;
    return result;
}

// Returns positions of the first character of every match
std::vector<size_t> FindFuzzyMatches(const std::string& patternWithWildcards,
        const std::string& text,
        char wildcard) {
    /*WildcardMatcher matcher;
    matcher.Init(patternWithWildcards, wildcard);*/
    std::vector<size_t> occurrences;
/*
    for (size_t offset = 0; offset < text.size(); ++offset) {
        matcher.Scan(text[offset], [&occurrences, offset, &patternWithWildcards] {
                occurrences.push_back(offset + 1 - patternWithWildcards.size());
                });
    }*/
    return occurrences;
}
template <typename T>
void Out(const std::vector<T>& vector) {
    for (auto element : vector) {
        std::cout << element << " ";
    }
    std::cout << "\n";
}

int main() {
    const char wildcard = '?';
    const std::string patternWithWildcards = ReadString(std::cin);
    Out(Split(patternWithWildcards, wildcard));
    //const std::string text = ReadString(std::cin);
    //Print(FindFuzzyMatches(patternWithWildcards, text, wildcard));
    return 0;
}