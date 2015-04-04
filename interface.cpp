/*
 * Интерфейс прокомментирован с целью объяснить,
 * почему он написан так, а не иначе. В реальной жизни
 * так никто никогда не делает. Комментарии к коду,
 * которые остались бы в его рабочем варианте, заданы
 * с помощью команды однострочного комментария // и написаны
 * на английском языке, как рекомендуется.
 * Остальные комментарии здесь исключительно в учебных целях.
 */

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



/*
 * Часто в c++ приходится иметь дело с парой итераторов,
 * которые представляют из себя полуинтервал. Например,
 * функция std:sort принимает пару итераторов, участок
 * между которыми нужно отсортировать. В с++11 появился
 * удобный range-based for, который позволяет итерироваться
 * по объекту, для которого определены функции std::begin
 * и std::end (например, это объекты: массив фиксированного
 * размера, любой объект, у которого определены методы
 * begin() и end()). То есть удобный способ итерироваться
 * по std::vector такой:
 * for (const std::string& string: words).
 * Однако, для некоторых объектов существует не один способ
 * итерироваться по ним. Например std::map: мы можем
 * итерироваться по парам объект-значение (как это сделает
 * for(...: map)), или мы можем итерироваться по ключам.
 * Для этого мы можем сделать функцию:
 * IteratorRange<...> keys(const std::map& map),
 * которой можно удобно воспользоваться:
 * for(const std::string& key: keys(dictionary)).
 */




// Consecutive delimiters are not grouped together and are deemed
// to delimit empty strings





void testAll();



// ===== TESTING ZONE =====

template<class T>
std::ostream& operator << (std::ostream& os, const std::vector<T>& vector) {
    std::copy(
            vector.begin(),
            vector.end(),
            std::ostream_iterator<T>(os, " ")
    );
    return os;
}

class TestNotPassedException : public std::runtime_error {
public:
    explicit TestNotPassedException(const char* what) :
            std::runtime_error(what)
    {}

    explicit TestNotPassedException(const std::string& what) :
            std::runtime_error(what.c_str())
    {}
};

#define REQUIRE_EQUAL(first, second) do { \
 auto firstValue = (first); \
 auto secondValue = (second); \
 if (firstValue != secondValue) { \
 std::ostringstream oss; \
 oss << "Require equal failed: " << # first << " != " << # second \
 << " (" << firstValue << " != " << secondValue << ")\n"; \
 throw TestNotPassedException(oss.str()); \
 } \
 } while (false)

void testAll() {
    {
        aho_corasick::AutomatonBuilder builder;
        builder.Add("suffix", 1);
        builder.Add("ffix", 2);
        builder.Add("ix", 3);
        builder.Add("abba", 4);

        std::unique_ptr<aho_corasick::Automaton> automaton =
                builder.Build();

        const std::string text = "let us find some suffix";
        aho_corasick::NodeReference node = automaton->Root();
        for (char ch: text) {
            node = node.Next(ch);
        }
        std::vector<size_t> string_ids;

        automaton->GenerateMatches(node, [&string_ids](size_t string_id) {
            string_ids.push_back(string_id);
        });
        std::sort(string_ids.begin(), string_ids.end());

        REQUIRE_EQUAL(string_ids, std::vector<size_t>({1, 2, 3}));
    }
    {
        WildcardMatcher matcher;
        matcher.Init("a?c?", '?');

        {
            std::vector<size_t> occurrences;
            // 012345678901234
            const std::string first_text = "abcaaccxaxcxacc";
            for (size_t i = 0; i < first_text.size(); ++i) {
                matcher.Scan(first_text[i], [&occurrences, i] () {
                    occurrences.push_back(i);
                });
            }

            REQUIRE_EQUAL(occurrences, std::vector<size_t>({3, 6, 7, 11}));
        }
        {
            matcher.Reset();
            std::vector<size_t> occurrences;
            const std::string second_text = "xyzadcc";
            for (size_t i = 0; i < second_text.size(); ++i) {
                matcher.Scan(second_text[i], [&occurrences, i] () {
                    occurrences.push_back(i);
                });
            }

            REQUIRE_EQUAL(occurrences, std::vector<size_t>({6}));
        }
    }
}
