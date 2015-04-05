#include <algorithm>
#include <deque>
#include <iostream>
#include <queue>
#include <map>
#include <unordered_set>
#include <iterator>
#include <sstream>
#include <memory>
#include <vector>
#include <string>

// std::make_unique will be available since c++14
// Implementation was taken from http://herbsutter.com/gotw/_102/
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) :
            begin_(begin), end_(end) {
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

private:
    Iterator begin_, end_;
};

namespace traverses {

    template<class Visitor, class Graph, class Vertex>
    void BreadthFirstSearch(Vertex origin_vertex, const Graph &graph, Visitor visitor) {
        std::queue<Vertex> order;
        order.push(origin_vertex);
        std::unordered_set<Vertex> visited;
        visited.insert(origin_vertex);
        while (!order.empty()) {
            Vertex next = order.front();
            visitor.DiscoverVertex(next);
            order.pop();
            for (auto edge : graph.OutgoingEdges(next)) {
                visitor.ExamineEdge(edge);
                if (visited.find(edge.target) == visited.end()) {
                    visitor.ExamineVertex(edge.target);
                    visited.insert(edge.target);
                    order.push(edge.target);
                }
            }
        }
    }

    template<class Vertex, class Edge>
    class BfsVisitor {
    public:
        virtual void DiscoverVertex(Vertex /*vertex*/) {
        }

        virtual void ExamineEdge(const Edge & /*edge*/) {
        }

        virtual void ExamineVertex(Vertex /*vertex*/) {
        }

        virtual ~BfsVisitor() {
        }
    };

} // namespace traverses

namespace aho_corasick {

    struct AutomatonNode {
        AutomatonNode() :
                suffix_link(nullptr),
                terminal_link(nullptr) {
        }

        std::vector<size_t> matched_string_ids;
        // Stores tree structure of nodes
        std::map<char, AutomatonNode> trie_transitions;

        // Stores pointers to the elements of trie_transitions
        std::map<char, AutomatonNode *> automaton_transitions;
        AutomatonNode *suffix_link;
        AutomatonNode *terminal_link;
    };

// Returns nullptr if there is no such transition
    AutomatonNode *GetTrieTransition(AutomatonNode *node, char character) {
        auto transition = node->trie_transitions.find(character);
        if (transition == node->trie_transitions.end()) {
            return nullptr;
        } else {
            return &(*transition).second;
        }
    }

// Performs transition in automaton
    AutomatonNode *GetNextNode(AutomatonNode *node, AutomatonNode *root, char character) {
        if (node->automaton_transitions.find(character) == node->automaton_transitions.end()) {
            auto next = GetTrieTransition(node, character);
            if (next == nullptr) {
                if (node != root) {
                    next = GetNextNode(node->suffix_link, root, character);
                } else {
                    next = root;
                }
            }
            node->automaton_transitions.insert(std::pair<char, AutomatonNode *>(character, next));
            return next;
        } else {
            return node->automaton_transitions.find(character)->second;
        }
    }
    namespace internal {
        class AutomatonGraph;

        class AutomatonGraph {
        public:
            struct Edge {
                Edge(AutomatonNode *source,
                        AutomatonNode *target,
                        char character) :
                        source(source),
                        target(target),
                        character(character) {
                }

                AutomatonNode *source;
                AutomatonNode *target;
                char character;
            };

            // Returns edges corresponding to all trie transitions from vertex
            std::vector<Edge> OutgoingEdges(AutomatonNode *vertex) const {
                std::vector<Edge> result;
                for (auto edge : vertex->trie_transitions) {
                    result.push_back(Edge(vertex, GetTrieTransition(vertex, edge.first), edge
                            .first));
                }
                return result;
            }
        };

        AutomatonNode *GetTarget(const AutomatonGraph::Edge &edge) {
            return edge.target;
        }

        class SuffixLinkCalculator :
                public traverses::BfsVisitor<AutomatonNode *, AutomatonGraph::Edge> {
        public:
            explicit SuffixLinkCalculator(AutomatonNode *root) :
                    root_(root) {
            }

            void ExamineVertex() {

            }

            void ExamineEdge(const AutomatonGraph::Edge &edge) /*override*/ {
                if (edge.source == root_) {
                    edge.target->suffix_link = root_;
                    return;
                }
                AutomatonNode *next = edge.source->suffix_link;
                while (next != nullptr) {
                    if (next->trie_transitions.find(edge.character) != next->trie_transitions.end
                            ()) {
                        edge.target->suffix_link = &(*next->trie_transitions.find(edge.character)
                        ).second;
                        return;
                    }
                    if (next->suffix_link != nullptr) {
                        next = next->suffix_link;
                    } else {
                        break;
                    }
                }
                edge.target->suffix_link = next;
            }

        private:
            AutomatonNode *root_;
        };

        class TerminalLinkCalculator :
                public traverses::BfsVisitor<AutomatonNode *, AutomatonGraph::Edge> {
        public:
            explicit TerminalLinkCalculator(AutomatonNode *root) :
                    root_(root) {
            }

            void DiscoverVertex(AutomatonNode *node) /*override*/ {
                if (node == root_) {
                    node->terminal_link = nullptr;
                    return;
                }
                AutomatonNode *next = node->suffix_link;
                while (next->matched_string_ids.size() == 0 && next != root_) {
                    next = next->suffix_link;
                }
                node->terminal_link = next;
            }

        private:
            AutomatonNode *root_;
        };
} // namespace internal


    class NodeReference {
    public:
        typedef std::vector<size_t>::const_iterator MatchedStringIterator;
        typedef IteratorRange<MatchedStringIterator> MatchedStringIteratorRange;

        NodeReference() :
                node_(nullptr),
                root_(nullptr) {
        }

        NodeReference(AutomatonNode *node, AutomatonNode *root) :
                node_(node), root_(root) {
        }

        NodeReference Next(char character) const {
            return NodeReference(GetNextNode(node_, root_, character), root_);
        }

        NodeReference suffixLink() const {
            return NodeReference(node_->suffix_link, root_);
        }

        NodeReference terminalLink() const {
            return NodeReference(node_->terminal_link, root_);
        }

        MatchedStringIteratorRange matchedStringIds() const {
            return MatchedStringIteratorRange(node_->matched_string_ids.begin(),
                    node_->matched_string_ids.end());
        }

        explicit operator bool() const {
            return node_ != nullptr;
        }

        bool operator==(NodeReference other) const {
            return node_ == other.node_;
        }

    private:
        AutomatonNode *node_;
        AutomatonNode *root_;
    };

    using std::rel_ops::operator !=;

    class AutomatonBuilder;

    class Automaton {
    public:
        Automaton() = default;

        NodeReference Root() {
            return NodeReference(&root_, &root_);
        }

        // Calls on_match(string_id) for every string ending at
        // this node, i.e. collects all string ids reachable by
        // terminal links.
        template<class Callback>
        void GenerateMatches(NodeReference node, Callback on_match) {
            if (node != Root()) {
                for (auto string_id : node.matchedStringIds()) {
                    on_match(string_id);
                }
                GenerateMatches(node.terminalLink(), on_match);
            }
        }

    private:
        AutomatonNode root_;

        Automaton(const Automaton &) = delete;

        Automaton &operator=(const Automaton &) = delete;

        friend class AutomatonBuilder;
    };

    class AutomatonBuilder {
    public:
        void Add(const std::string &string, size_t id) {
            words_.push_back(string);
            ids_.push_back(id);
        }

        std::unique_ptr<Automaton> Build() {
            auto automaton = make_unique<Automaton>();
            BuildTrie(words_, ids_, automaton.get());
            BuildSuffixLinks(automaton.get());
            BuildTerminalLinks(automaton.get());
            return automaton;
        }

    private:
        static void BuildTrie(const std::vector<std::string> &words,
                const std::vector<size_t> &ids,
                Automaton *automaton) {
            for (size_t i = 0; i < words.size(); ++i) {
                AddString(&automaton->root_, ids[i], words[i]);
            }
        }

        static void AddString(AutomatonNode *root, size_t string_id, const std::string &string) {
            AutomatonNode *current = root;
            for (char ch : string) {
                if (current->trie_transitions.find(ch) == current->trie_transitions.end()) {
                    current->trie_transitions.insert(std::pair<char, AutomatonNode>(ch,
                            AutomatonNode()));
                }
                current = &(*current->trie_transitions.find(ch)).second;
            }
            current->matched_string_ids.push_back(string_id);
        }

        static void BuildSuffixLinks(Automaton *automaton) {
            internal::SuffixLinkCalculator visitor(&(automaton->root_));
            traverses::BreadthFirstSearch(&(automaton->root_),
                    internal::AutomatonGraph(), visitor);
        }

        static void BuildTerminalLinks(Automaton *automaton) {
            internal::TerminalLinkCalculator visitor(&(automaton->root_));
            traverses::BreadthFirstSearch(&(automaton->root_),
                    internal::AutomatonGraph(), visitor);
        }

        std::vector<std::string> words_;
        std::vector<size_t> ids_;
    };

} // namespace aho_corasick



template<class Predicate>
std::vector<std::string> Split(const std::string &string, Predicate is_delimiter) {
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

// Wildcard is a character that may be substituted
// for any of all possible characters
class WildcardMatcher {
public:
    WildcardMatcher() :
            number_of_words_(0),
            pattern_length_(0) {
    }

    void Init(const std::string &pattern, char wildcard) {
        aho_corasick::AutomatonBuilder builder;

        std::string substring;
        size_t position = 0;
        for (char character : pattern) {
            if (character != wildcard) {
                substring.push_back(character);
            } else if (!substring.empty()) {
                builder.Add(substring, position);
                substring.clear();
                number_of_words_++;
            }
            position++;
        }

        aho_corasick_automaton_ = builder.Build();
        state_ = aho_corasick_automaton_->Root();
        pattern_length_ = pattern.size();
    }

    // Resets matcher to start scanning new stream
    void Reset() {
        state_ = aho_corasick_automaton_->Root();
        words_occurrences_by_position_ = std::deque<size_t>();
    }

    // Scans new character and calls on_match() if
    // suffix of scanned characters matches pattern

    template<class Callback>
    void Scan(char character, Callback on_match) {
        if (words_occurrences_by_position_.size() == pattern_length_) {
            words_occurrences_by_position_.pop_front();
        }
        words_occurrences_by_position_.push_back(0);
        state_ = state_.Next(character);
        size_t currentLength = words_occurrences_by_position_.size();
        std::vector<size_t> matches;
        aho_corasick_automaton_->GenerateMatches(state_, [&matches](size_t string_id) {
            matches.push_back(string_id);
        });
        for (size_t string_id : matches) {
            size_t positionsBack = string_id - 1;
            if (positionsBack < currentLength) {
                ++words_occurrences_by_position_[currentLength - 1 - positionsBack];
            }
        }
        if (words_occurrences_by_position_.size() == pattern_length_ &&
                words_occurrences_by_position_[0] == number_of_words_) {
            on_match();
        }
    }

private:
    // Storing only O(|pattern|) elements allows us
    // to consume only O(|pattern|) memory for matcher
    std::deque<size_t> words_occurrences_by_position_;
    aho_corasick::NodeReference state_;
    size_t number_of_words_;
    size_t pattern_length_;
    std::unique_ptr<aho_corasick::Automaton> aho_corasick_automaton_;
};

void Print(const std::vector<size_t> &sequence) {
    std::cout << sequence.size() << "\n";
    for (auto element : sequence) {
        std::cout << element << " ";
    }
    std::cout << "\n";
}

std::string ReadString(std::istream &input_stream) {
    std::string result;
    input_stream >> result;
    return result;
}

// Returns positions of the first character of every match
std::vector<size_t> FindFuzzyMatches(const std::string &patternWithWildcards,
        const std::string &text,
        char wildcard) {
    WildcardMatcher matcher;
    matcher.Init(patternWithWildcards, wildcard);
    std::vector<size_t> occurrences;

    for (size_t offset = 0; offset < text.size(); ++offset) {
        matcher.Scan(text[offset], [&occurrences, offset, &patternWithWildcards] {
            occurrences.push_back(offset + 1 - patternWithWildcards.size());
        });
    }
    return occurrences;
}

template<typename T>
void Out(const std::vector<T> &vector) {
    for (auto element : vector) {
        std::cout << element << " ";
    }
    std::cout << "\n";
}

int main() {
    const char wildcard = '?';
    const std::string patternWithWildcards = ReadString(std::cin);
    const std::string text = ReadString(std::cin);
    Print(FindFuzzyMatches(patternWithWildcards, text, wildcard));
    return 0;
}
