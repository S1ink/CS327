#include "bst.hpp"

#include <iostream>
#include <string>


class FOO
{
public:
    bool operator<(const FOO&) const { return false; }
    bool operator!=(const FOO&) const { return true; }
};

int main(int argc, char** argv)
{
    BST<int> itree;
    BST<std::string> stree;

    itree.insert(5);
    itree.insert(7);
    itree.insert(5);
    itree.insert(8);
    itree.insert(2);
    itree.insert(1);

    std::cout << itree.contains(6) << std::endl;
    std::cout << itree.contains(1) << std::endl;
    std::cout << itree.contains(5) << std::endl;

    stree.insert("rob");
    stree.insert("susie");
    stree.insert("alex");
    stree.insert("chet");
    stree.insert("steve");
    stree.insert("michelle");
    stree.insert("jeremy");
    stree.insert("joe");

    std::cout << stree.contains("jeremy") << std::endl;
    std::cout << stree.contains("susie") << std::endl;
    std::cout << stree.contains("flint") << std::endl;

    BST<FOO> ftree;

    ftree.insert({});
    ftree.insert({});

    std::cout << ftree.contains({}) << std::endl;

    return 0;
}
