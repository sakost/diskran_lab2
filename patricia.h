//
// Created by sakost on 21.11.2020.
//

#ifndef LAB2_PATRICIA_H
#define LAB2_PATRICIA_H

#include <cinttypes>
#include <cstring>
#include <cassert>
#include <cmath>

#include <ostream>
#include <fstream>

const std::size_t KEY_LENGTH = 256;
using TKey = char;
using TValue = std::size_t;
const int INVALID_KEY = 1;


static inline bool CompareKeys(const TKey *lhs, const TKey *rhs) {
    if (lhs == nullptr || rhs == nullptr)
        return false;
    return strcmp(lhs, rhs) == 0;
}

TKey *Copy(TKey *key) {
    if (key == nullptr) {
        return nullptr;
    }
    char *copy = new TKey[KEY_LENGTH + 1];
    strcpy(copy, key);
    memset(copy + strlen(copy), '\0', KEY_LENGTH+1-strlen(copy));
    return copy;
}

static inline unsigned GetNBit(const TKey *key, int Bit) {
    if (Bit < 0)
        Bit = 0;
    unsigned int i = (((unsigned) key[(unsigned) Bit / 8u]) >> ((7u - ((unsigned) Bit % 8u)))) & 1u;
    return i;
}

static inline std::size_t FindDifferBit(const TKey *key1, const TKey *key2) {
    if (key1 == nullptr || key2 == nullptr) {
        return INVALID_KEY;
    }
    size_t len1 = strlen(key1);
    size_t len2 = strlen(key2);
    auto min_size = std::min(len1, len2);
    for (std::size_t i(0); i < min_size; i++) {
        if (key1[i] != key2[i]) {
            unsigned lmsb = sizeof(int) * 8 - __builtin_clz(((unsigned) key1[i] ^ (unsigned) key2[i]) | 1u);
            assert(lmsb < 8);
            return (i << 3u) + 8 - lmsb;
        }
    }
    if (len1 != len2) {
        return (min_size << 3u) + INVALID_KEY;
    }
    return INVALID_KEY;
}

class TNode {
public:
    using IdType = long long;
    using BitType = int;
    TNode() {
        Key = nullptr;
        Value = 0;
        Left = Right = this;
    }

    TNode(TKey *key, TValue value) {
        Key = Copy(key);
        Value = value;
        Left = Right = this;
    }

    ~TNode() {
        delete[]Key;
    }

    TNode(TNode &other) {
        if (strlen(this->Key) < strlen(other.Key)) {
            delete[] this->Key;
            this->Key = Copy(other.Key);
        }
        Left = Right = this;
        this->Value = other.Value;
    }

    TNode &operator=(const TNode &other) {
        if (this == &other) {
            return *this;
        }
        this->Value = other.Value;
        delete[] this->Key;
        this->Key = Copy(other.Key);
        return *this;
    }

    TNode *Left, *Right;
    BitType Bit = -1;
    TKey *Key = nullptr;
    TValue Value = 0;
    IdType id = -1;
};

bool IsBackref(TNode *from, TNode *to) {
    return to->Bit <= from->Bit;
}

bool IsNormal(TNode *from, TNode *to) {
    return !IsBackref(from, to);
}

static inline TNode *Choose(const TNode *cur, const TKey *key, bool reverse = false) {
    if (reverse)
        return GetNBit(key, cur->Bit) ? cur->Left : cur->Right;
    return GetNBit(key, cur->Bit) ? cur->Right : cur->Left;
}


struct TPatricia {
    TNode *Root = nullptr;
    std::size_t Size = 0;

    TPatricia() {
        Root = new TNode();
    }

    ~TPatricia() {
        Destruct(Root);
    }

    void Destruct(TNode *node) {
        if (IsNormal(node, node->Left)) {
            Destruct(node->Left);
        }
        if (IsNormal(node, node->Right)) {
            Destruct(node->Right);
        }
        delete node;
    }

    TNode *AddItem(TKey *key, TValue value) {
        TNode *cur = Get(key);
        if (CompareKeys(key, cur->Key)) {
            return nullptr;
        }
        int lBitPos = FindDifferBit(key, cur->Key);

        TNode *parent = Root;
        TNode *child = parent->Left;

        while (IsNormal(parent, child) && lBitPos > child->Bit) {
            parent = child;
            child = Choose(child, key);
        }

        auto *node = new TNode(key, value);
        Size++;

        node->Bit = lBitPos;
        node->Left = GetNBit(key, lBitPos) ? child : node;
        node->Right = GetNBit(key, lBitPos) ? node : child;
        if (GetNBit(key, parent->Bit)) {
            parent->Right = node;
        } else {
            parent->Left = node;
        }
        return node;
    }

    TNode *Find(const TKey *key) const {
        TNode *res = Get(key);
        if (CompareKeys(res->Key, key)) {
            return res;
        }
        return nullptr;
    }

    TNode *Get(const TKey *key) const {
        TNode *parent, *cur;
        parent = Root;
        cur = Root->Left;
        while (IsNormal(parent, cur)) {
            parent = cur;
            cur = Choose(cur, key);
        }

        return cur;
    }

    bool Erase(TKey *key) {
        TNode *parent, *cur, *grand_parent = nullptr;

        parent = Root;
        cur = parent->Left;

        while (parent->Bit < cur->Bit) {
            grand_parent = parent;
            parent = cur;
            cur = Choose(cur, key);
        }

        if (!CompareKeys(key, cur->Key))
            return false;

        if (parent != cur)
            *cur = *parent;

        TNode *child_backref_node, *backref_node;
        TKey *cache;

        if (parent != cur) {
            cache = parent->Key;

            backref_node = parent;
            child_backref_node = GetNBit(cache, parent->Bit) ? parent->Right : parent->Left;

            while (IsNormal(backref_node, child_backref_node)) {
                backref_node = child_backref_node;
                child_backref_node = Choose(child_backref_node, cache);
            }

            if (!CompareKeys(cache, child_backref_node->Key)) {
                return false;
            }

            if (GetNBit(cache, backref_node->Bit))
                backref_node->Right = cur;
            else
                backref_node->Left = cur;
        }

        TNode *to_replace = Choose(parent, key, true);
        if (GetNBit(key, grand_parent->Bit))
            grand_parent->Right = to_replace;
        else
            grand_parent->Left = to_replace;

        Size--;
        delete parent;

        return true;
    }

    void TreeDebugPrint(std::ostream &out, TNode *cur = nullptr, int tb = -1) const {
        if (cur == nullptr) {
            cur = Root;
        }
        if (IsNormal(cur, cur->Left)) {
            TreeDebugPrint(out, cur->Left, tb + 1);
        }
        if (cur != Root) {
            char *tbs = new char[tb + 1];
            memset(tbs, '\t', tb);
            tbs[tb] = '\0';
            out << tbs << "element: " << cur->Key << " " << cur->Value;
            out << " backref to ";
            if(cur->Left != Root){
                out << cur->Left->Key;
            } else{
                out << "Root";
            }
            out << " and to ";
            if(cur->Right != Root){
                out << cur->Right->Key;
            } else{
                out << "Root";
            }
            out << std::endl;
            delete[]tbs;
        }
        if (IsNormal(cur, cur->Right)) {
            TreeDebugPrint(out, cur->Right, tb + 1);
        }
    }

    void Serialize(std::ofstream &file) {
        file.write((const char *) &(Size), sizeof(decltype(Size)));


        if(Size == 0){
            return;
        }
        auto nodes = new TNode *[Size+1];
        int index = 0;
        GenerateIds(Root, nodes, index);

        TNode *node;

        for (std::size_t i = 0; i <= Size; ++i) {
            node = nodes[i];

            file.write((const char *) &(node->Value), sizeof(TValue));
            file.write((const char *) &(node->Bit), sizeof(TNode::BitType));
            std::size_t len = 0;
            if(node->Key != nullptr){
                len = strlen(node->Key);
            }
            file.write((const char *) &(len), sizeof(decltype(len)));
            file.write(node->Key, sizeof(TKey) * len);
            file.write((const char *) &(node->Left->id), sizeof(TNode::IdType));
            file.write((const char *) &(node->Right->id), sizeof(TNode::IdType));
        }

        delete[]nodes;
    }

    static void GenerateIds(TNode *nodeLoader, TNode **nodes, int &index) {
        nodeLoader->id = index;
        nodes[index++] = nodeLoader;
        if (IsNormal(nodeLoader, nodeLoader->Left)) {
            GenerateIds(nodeLoader->Left, nodes, index);
        }
        if (IsNormal(nodeLoader, nodeLoader->Right)) {
            GenerateIds(nodeLoader->Right, nodes, index);
        }
    }

    TPatricia* Deserialize(std::ifstream &file) {
        using size_type = decltype(Size);

        file.read((char *) &Size, sizeof(size_type));
        if (!Size) {
            return this;
        }

        auto **nodes = new TNode *[Size + 1];
        nodes[0] = Root;
        for (std::size_t i = 1; i < Size+1; ++i) {
            nodes[i] = new TNode;
        }

        TNode::BitType bit;
        TKey *key = nullptr;
        TValue value;
        std::size_t len;
        TNode::IdType left_id, right_id;

        for (std::size_t i = 0; i < Size + 1; ++i) {
            file.read((char *) &value, sizeof(TValue));
            file.read((char *) &bit, sizeof(decltype(bit)));
            file.read((char *) &len, sizeof(decltype(len)));
            if(len != 0) {
                key = new TKey[len + 1];
                key[len] = '\0';
                file.read(key, len);
            }
            file.read((char *) &left_id, sizeof(decltype(left_id)));
            file.read((char *) &right_id, sizeof(decltype(right_id)));
            nodes[i]->Bit = bit;
            nodes[i]->Key = key;
            key = nullptr;
            nodes[i]->Value = value;
            nodes[i]->id = i;
            nodes[i]->Left = nodes[left_id];
            nodes[i]->Right = nodes[right_id];
        }

        delete[]nodes;

        return this;
    }
};


#endif //LAB2_PATRICIA_H
