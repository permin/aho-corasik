#include <c++/bits/stl_queue.h>
#include <c++/bits/unordered_set.h>
#include <wsman.h>

// std::make_unique will be available since c++14
// Implementation was taken from http://herbsutter.com/gotw/_102/
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end):
            begin_(begin), end_(end) {}

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
    void BreadthFirstSearch(Vertex origin_vertex, const Graph& graph, Visitor visitor) {
        std::queue<Vertex> order (1, origin_vertex);
        std::unordered_set<Vertex> visited (1, origin_vertex);
        visitor.DiscoverVertex(origin_vertex);
        while(!order.empty()) {
            Vertex next = visited.pop();
            auto outgoingEdges = graph.OutgoingEdges(next);
            for (auto edge : outgoingEdges) {
                visitor.ExamineEdge(edge);
            }
            for (auto edge : outgoingEdges) {
                if (visited.find(edge.target) == std::unordered_set::end()) {
                    visitor.DiscoverVertex(edge.target);
                    visited.insert(edge.target);
                    order.push(edge.target);
                }
            }
        }
    }

/*
 * Для начала мы рекомендуем ознакомиться с общей
 * концепцией паттерна проектирования Visitor:
 * http://en.wikipedia.org/wiki/Visitor_pattern
 * Для применения Visitor'а к задаче обхода графа
 * можно ознакомиться с
 * http://www.boost.org/doc/libs/1_57_0/libs/graph/doc/BFSVisitor.html
 */
// See "Visitor Event Points" on
// http://www.boost.org/doc/libs/1_57_0/libs/graph/doc/breadth_first_search.html
    template<class Vertex, class Edge>
    class BfsVisitor {
    public:
        virtual void DiscoverVertex(Vertex /*vertex*/) {}
        virtual void ExamineEdge(const Edge& /*edge*/) {}
        virtual void ExamineVertex(Vertex /*vertex*/) {}
        virtual ~BfsVisitor() {}
    };

} // namespace traverses

namespace aho_corasick {

    struct AutomatonNode {
        AutomatonNode():
                suffix_link(nullptr),
                terminal_link(nullptr) {
        }

        std::vector<size_t> matched_string_ids;
        // Stores tree structure of nodes
        std::map<char, AutomatonNode> trie_transitions;
        /*
        * Обратите внимание, что std::set/std::map/std::list
        * при вставке и удалении неинвалидируют ссылки на
        * остальные элементы контейнера. Но стандартные контейнеры
        * std::vector/std::string/std::deque таких гарантий не
        * дают, поэтому хранение указателей на элементы этих
        * контейнеров крайне не рекомендуется.
        */
        // Stores pointers to the elements of trie_transitions
        std::map<char, AutomatonNode*> automaton_transitions;
        AutomatonNode* suffix_link;
        AutomatonNode* terminal_link;
    };

// Returns nullptr if there is no such transition
    AutomatonNode* GetTrieTransition(AutomatonNode* node, char character);

// Performs transition in automaton
    AutomatonNode* GetNextNode(AutomatonNode* node, AutomatonNode* root, char character);

    namespace internal {

        class AutomatonGraph;

        class AutomatonGraph {
        public:
            struct Edge {
                Edge(AutomatonNode* source,
                        AutomatonNode* target,
                        char character):
                        source(source),
                        target(target),
                        character(character) {
                }

                AutomatonNode* source;
                AutomatonNode* target;
                char character;
            };

            // Returns edges corresponding to all trie transitions from vertex
            std::vector<Edge> OutgoingEdges(AutomatonNode* vertex) const;
        };

        AutomatonNode* GetTarget(const AutomatonGraph::Edge& edge);

        class SuffixLinkCalculator:
                public traverses::BfsVisitor<AutomatonNode*, AutomatonGraph::Edge> {
        public:
            explicit SuffixLinkCalculator(AutomatonNode* root):
                    root_(root) {}

            void ExamineVertex(AutomatonNode* node) /*override*/;

            void ExamineEdge(const AutomatonGraph::Edge& edge) /*override*/;

        private:
            AutomatonNode* root_;
        };

        class TerminalLinkCalculator:
                public traverses::BfsVisitor<AutomatonNode*, AutomatonGraph::Edge> {
        public:
            explicit TerminalLinkCalculator(AutomatonNode* root):
                    root_(root) {}

            /*
            * Если вы не знакомы с этим ключевым словом,
            * то ознакомьтесь
            * http://en.wikipedia.org/wiki/C%2B%2B11#Explicit_overrides_and_final
            * override здесь и далее закомментирован, так
            * как у нас слишком старый компилятор с++0x в
            * контесте, который выдaет compilation error
            */
            void DiscoverVertex(AutomatonNode* node) /*override*/;

        private:
            AutomatonNode* root_;
        };

    } // namespace internal

/*
 * Объясним задачу, которую решает класс NodeReference.
 * Класс Automaton представляет из себя неизменяемый объект
 * (http://en.wikipedia.org/wiki/Immutable_object),
 * в данном случае, это означает, что единственное действие,
 * которое пользователь может совершать с готовым автоматом,
 * это обходить его разными способами. Это значит, что мы
 * должны предоставить пользователю способ получить вершину
 * автомата и дать возможность переходить между вершинами.
 * Одним из способов это сделать -- это предоставить
 * пользователю константный указатель на AutomatonNode,
 * а вместе с ним константый интерфейс AutomatonNode. Однако,
 * этот вариант ведет к некоторым проблемам.
 * Во-первых, этот же интерфейс AutomatonNode мы должны
 * использовать и для общения автомата с этим внутренним
 * представлением вершины. Так как константная версия
 * этого же интерфейса будет доступна пользователю, то мы
 * ограничены в добавлении функций в этот константный
 * интерфейс (не все функции, которые должны быть доступны
 * автомату должны быть доступны пользователю). Во-вторых,
 * так как мы используем кэширование при переходе по символу
 * в автомате, то условная функция getNextNode не может быть
 * константной (она заполняет кэш переходов). Это значит,
 * что мы лишены возможности добавить функцию "перехода
 * между вершинами" в константный интерфейс (то есть,
 * предоставить ее пользователю константного указателя на
 * AutomatonNode).
 * Во избежание этих проблем, мы создаем отдельный
 * класс, отвечающий ссылке на вершину, который предоставляет
 * пользователю только нужный интерфейс.
 */
    class NodeReference {
    public:
        typedef std::vector<size_t>::const_iterator MatchedStringIterator;
        typedef IteratorRange<MatchedStringIterator> MatchedStringIteratorRange;

        NodeReference():
                node_(nullptr),
                root_(nullptr) {
        }
        NodeReference(AutomatonNode* node, AutomatonNode* root):
                node_(node), root_(root) {
        }

        NodeReference Next(char character) const;

        NodeReference suffixLink() const;

        NodeReference terminalLink() const;

        MatchedStringIteratorRange matchedStringIds() const;

        explicit operator bool() const {
            return node_ != nullptr;
        }

        bool operator==(NodeReference other) const;

    private:
        AutomatonNode* node_;
        AutomatonNode* root_;
    };

    using std::rel_ops::operator !=;

    class AutomatonBuilder;

    class Automaton {
    public:
        /*
        * Чтобы ознакомиться с констркуцией =default, смотрите
        * http://en.wikipedia.org/wiki/C%2B%2B11#Explicitly_defaulted_and_deleted_special_member_functions
        */
        Automaton() = default;

        NodeReference Root() {
            return NodeReference(&root_, &root_);
        }

        /*
        * В этом случае есть два хороших способа получить
        * результат работы этой функции:
        * добавить параметр типа OutputIterator, который
        * последовательно записывает в него id найденных
        * строк, или же добавить параметр типа Callback,
        * который будет вызван для каждого такого id.
        * Чтобы ознакомиться с этими концепциями лучше,
        * смотрите ссылки:
        * http://en.cppreference.com/w/cpp/concept/OutputIterator и
        * http://en.wikipedia.org/wiki/Callback_(computer_programming).
        * По своей мощности эти способы эквивалентны. (см.
        * http://www.boost.org/doc/libs/release/libs/iterator/doc/function_output_iterator.html)
        * Так как в интерфейсе WildcardMatcher мы принимаем
        * Callback, то чтобы пользоваться одним и тем же средством
        * во всех интерфейсах, мы и здесь применяем Callback. Отметим,
        * что другие способы, как например, вернуть std::vector с
        * найденными id, не соответствуют той же степени гибкости, как
        * 2 предыдущие решения (чтобы в этом убедиться представьте
        * себе, как можно решить такую задачу: создать std::set
        * из найденных id).
        */
        // Calls on_match(string_id) for every string ending at
        // this node, i.e. collects all string ids reachable by
        // terminal links.
        template <class Callback>
        void GenerateMatches(NodeReference node, Callback on_match);

    private:
        AutomatonNode root_;

        Automaton(const Automaton&) = delete;
        Automaton& operator=(const Automaton&) = delete;

        friend class AutomatonBuilder;
    };

    class AutomatonBuilder {
    public:
        void Add(const std::string& string, size_t id) {
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
        static void BuildTrie(const std::vector<std::string>& words,
                const std::vector<size_t>& ids,
                Automaton* automaton) {
            for (size_t i = 0; i < words.size(); ++i) {
                AddString(&automaton->root_, ids[i], words[i]);
            }
        }

        static void AddString(AutomatonNode* root, size_t string_id, const std::string& string) {
            AutomatonNode current = &root;
            for (char ch : string) {
                if (current.trie_transitions.find(ch) == std::map::end()) {
                    current.trie_transitions.emplace(ch, AutomatonNode());
                }
                current = &current.trie_transitions.find(ch);
            }
            current.matched_string_ids.push_back(string_id);
        }

        static void BuildSuffixLinks(Automaton* automaton);

        static void BuildTerminalLinks(Automaton* automaton);

        std::vector<std::string> words_;
        std::vector<size_t> ids_;
    };

} // namespace aho_corasick