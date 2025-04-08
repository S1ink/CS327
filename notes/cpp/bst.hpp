#pragma once

#include <type_traits>
#include <memory>


template<typename T>
class BST
{
    struct Node
    {
        Node *l{ nullptr }, *r{ nullptr };
        T value;

    public:
        inline Node() = default;
        inline Node(const T& v) : value{v} {}
        inline Node(T&& v) : value{ std::move(v) } {}
        inline ~Node()
        {
            if(this->l) delete this->l;
            if(this->r) delete this->r;
        }

    };

public:
    inline BST() = default;
    inline ~BST()
    {
        if(this->root) delete this->root;
    }

public:
    void insert(const T& v);
    bool contains(const T& v);

protected:
    Node* root{ nullptr };

};





template<typename T>
void BST<T>::insert(const T& v)
{
    Node* nn = new Node(v);

    if(!this->root)
    {
        this->root = nn;
        return;
    }

    Node *par, *tmp;
    for(tmp = this->root; tmp;)
    {
        par = tmp;
        if(v < tmp->value)
        {
            tmp = tmp->l;
        }
        else
        {
            tmp = tmp->r;
        }
    }

    if(v < par->value)
    {
        par->l = nn;
    }
    else
    {
        par->r = nn;
    }
}

template<typename T>
bool BST<T>::contains(const T& v)
{
    Node* tmp;

    for(tmp = this->root; (tmp && tmp->value != v);)
    {
        if(v < tmp->value)
        {
            tmp = tmp->l;
        }
        else
        {
            tmp = tmp->r;
        }
    }

    return static_cast<bool>(tmp);
}
