#include <iostream>
#include <exception>
#include <initializer_list>
#include <vector>

template<typename T, typename Alloc = std::allocator<T>>
class Forward_list {
private:
    struct BaseNode {
        BaseNode* next;

        BaseNode(): next(nullptr) {}

        virtual ~BaseNode() = default; 
    };
    struct Node: BaseNode {
        T value;

        template<typename... Args>
        Node(Args&&... args): BaseNode(), value(std::forward<Args>(args)...) {}
    };

    using rebind = typename std::allocator_traits<Alloc>::rebind_alloc<Node>;
    using AllocTraits = std::allocator_traits<rebind>;
    
    BaseNode fakeNode;
    std::size_t sz;
    rebind alloc;
public:
    class iterator;
    
    Forward_list(const Alloc& alloc = Alloc()) noexcept : fakeNode(), sz(0), alloc(alloc) {}

    template<typename InputIter>
    Forward_list(InputIter begin, InputIter end, const Alloc& a = Alloc()): fakeNode(), sz(0), alloc(a) {
        auto thisIter = &fakeNode;
        auto otherIter = begin;
        while (otherIter != end) {
            try {
                thisIter->next = AllocTraits::allocate(alloc, 1);
            } catch (std::bad_alloc& ba) {
                thisIter->next = nullptr;
                throw;
            }
            
            try {
                AllocTraits::construct(alloc, static_cast<Node*>(thisIter->next), *otherIter);
            } catch (std::exception& ex) {
                AllocTraits::deallocate(alloc, static_cast<Node*>(thisIter->next), 1);
                thisIter->next = nullptr;
                throw;
            }

            thisIter = thisIter->next;
            ++otherIter;
            ++sz;
        }

        thisIter->next = nullptr;
    }

    Forward_list(std::initializer_list<T> ilist, const Alloc& a = Alloc()): fakeNode(), sz(0), alloc(a) {
        BaseNode* iter = &fakeNode;
        for (const auto& elem: ilist) {
            try {
                iter->next = AllocTraits::allocate(alloc, 1);
            } catch (std::bad_alloc& ba) {
                iter->next = nullptr;
                throw;
            }

            try {
                AllocTraits::construct(alloc, static_cast<Node*>(iter->next), elem);
            } catch (std::exception& ex) {
                AllocTraits::deallocate(alloc, static_cast<Node*>(iter->next), 1);
                iter->next = nullptr;
                throw;
            }

            iter = iter->next;
            ++sz;
        }

        iter->next = nullptr;
    }

    Forward_list(const Forward_list& other): Forward_list() {
        BaseNode* thisIter  = &fakeNode;
        const BaseNode* otherIter = &(other.fakeNode);
        while (otherIter->next != nullptr) {
            try {
                thisIter->next = AllocTraits::allocate(alloc, 1);
            } catch (std::bad_alloc& ba) {
                thisIter->next = nullptr;
                throw;
            }
            
            try {
                AllocTraits::construct(alloc, static_cast<Node*>(thisIter->next), static_cast<const Node*>(otherIter->next)->value);
            } catch (std::exception& ex) {
                AllocTraits::deallocate(alloc, static_cast<Node*>(thisIter->next), 1);
                thisIter->next = nullptr;
                throw;
            }
            
            otherIter = otherIter->next;
            thisIter  = thisIter->next;
            ++sz;
        }
        
        thisIter->next = nullptr;
    }

    Forward_list& operator=(const Forward_list& other) {
        Forward_list copy = other;
        std::swap(fakeNode, copy.fakeNode);
        std::swap(sz, copy.sz);
        std::swap(alloc, copy.alloc);
        return *this;
    }

    Forward_list(Forward_list&& other) noexcept: Forward_list() {
        std::swap(sz, other.sz);
        std::swap(fakeNode, other.fakeNode);
        std::swap(alloc, other.alloc);
    }

    Forward_list& operator=(Forward_list&& other) noexcept {
        Forward_list copy = std::move(other);
        std::swap(sz, copy.sz);
        std::swap(fakeNode, copy.fakeNode);
        std::swap(alloc, copy.alloc);
        return *this;
    }

    rebind get_allocator() {
        return alloc;
    }
    
    iterator before_begin() {
        return iterator(&fakeNode);
    }

    iterator begin() {
        return iterator(fakeNode.next);
    }

    iterator end() {
        return iterator(nullptr);
    }

    T& front() {
        return fakeNode->next->value;
    }

    const T& front() const {
        return fakeNode->next->value;
    }

    bool empty() const noexcept{
        return sz == 0;
    }
    
    void clear() noexcept(noexcept(~T())) {
        if (sz != 0) {
            auto currentNode = fakeNode.next;
            auto nextNode    = currentNode->next;

            while (nextNode != nullptr) {
                AllocTraits::destroy(alloc, static_cast<Node*>(currentNode));
                AllocTraits::deallocate(alloc, static_cast<Node*>(currentNode), 1);
                currentNode = nextNode;
                nextNode = nextNode->next;
            }

            AllocTraits::destroy(alloc, static_cast<Node*>(currentNode));
            AllocTraits::deallocate(alloc, static_cast<Node*>(currentNode), 1);

            sz = 0;
        }
    }
    
    template<typename... Args>
    iterator insert_after(iterator position, Args&&... args) {
        auto current = position.get_pointer();
        auto next    = current->next;

        //в случае исключения список останется в валидном состоянии
        current->next = AllocTraits::allocate(alloc, 1);

        try {
            AllocTraits::construct(alloc, static_cast<Node*>(current->next), std::forward<Args>(args)...);
        } catch (std::exception& ex) {
            AllocTraits::deallocate(alloc, static_cast<Node*>(current->next), 1);
            current->next = next;
            throw;
        }

        ++sz;
        current = current->next;
        current->next = next;
        return iterator(current);
    }

    iterator erase_after(iterator position) {
        auto current = position.get_pointer();
        if (current == nullptr || current->next == nullptr) 
            return end();

        auto newNext = current->next->next;
        AllocTraits::destroy(alloc, static_cast<Node*>(current->next));
        AllocTraits::deallocate(alloc, static_cast<Node*>(current->next), 1);
        --sz;
        current->next = newNext;
        return iterator(newNext);
    }

    template<typename... Args>
    void push_front(Args&&... args) {
        insert_after(before_begin(), std::forward<Args>(args)...);
    }

    void pop_front() {
        erase_after(before_begin());
    }
   
    ~Forward_list() {
        clear();
    }

    void print() {
        auto iter = fakeNode.next;
        std::cout << "size = " << sz << "{";
        while (iter != nullptr) {
            std::cout << static_cast<Node*>(iter)->value << " ";
            iter = iter->next;
        }
        std::cout << "}" << std::endl;
    }
};

template<typename T, typename Alloc>
class Forward_list<T, Alloc>::iterator {
private:
    BaseNode* ptr;
public:
    using value_type = T;

    iterator(BaseNode* ptr): ptr(ptr) {}

    iterator(const iterator& iter) = default;

    iterator& operator=(const iterator& iter) = default;

    const T& operator*() const {
        Node* node = dynamic_cast<Node*>(ptr);
        if (node == nullptr)
            throw std::runtime_error{"Attempt to dename fakeNode"};
            
        return node->value;
    }

    const T* operator&() const {
        Node* node = dynamic_cast<Node*>(ptr);
        if (node == nullptr)
            throw std::runtime_error{"Attempt to dename fakeNode"};

        return &(node->value);
    }

    iterator operator++(int) {
        if (ptr == nullptr) {
            return iterator(nullptr);
        }
        iterator copy(ptr);
        ptr = ptr->next;
        return copy;
    }

    iterator& operator++() {
        if (ptr != nullptr) {
            ptr = ptr->next;
        }
        return *this;
    }

    bool operator==(const iterator& other) const {
        return ptr == other.ptr;
    }

    bool operator!=(const iterator& other) const {
        return ptr != other.ptr;
    }

    BaseNode* get_pointer() {
        return ptr;
    }

    ~iterator() = default;
};
    


