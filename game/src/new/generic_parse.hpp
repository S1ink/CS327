#pragma once

#include <unordered_map>
#include <type_traits>
#include <functional>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>


template<typename T>
class SequentialParser
{
    enum
    {
        MARKER_NONE = 0,
        MARKER_START_OBJECT,
        MARKER_END_OBJECT
    };

    using FunctorT = std::function<void(std::istream&, std::istream&, T&)>;

    template<typename M>
    using MemberAccessor = std::function<M&(T&)>;

public:
    inline SequentialParser() = default;
    inline ~SequentialParser() = default;

public:
    void addStartToken(const std::string& t);
    void addEndToken(const std::string& t);

    void addFunctorToken(const std::string& t, const FunctorT& f);
    template<typename F>
    inline void addFunctorToken(const std::string& t, F&& f)
    {
        this->addFunctorToken(t, std::function(std::forward<F>(f)));
    }

    void addStringToken(const std::string& t, const MemberAccessor<std::string>& a);
    void addParagraphToken(const std::string& t, const MemberAccessor<std::string>& a);
    template<typename P, typename E = P>
    void addPrimitiveToken(const std::string& t, const MemberAccessor<P>& a);
    // void addRollableToken(const std::string& t)
    // void addKeywordToken(const std::string& t)

    void parse(std::istream& in);

protected:
    static FunctorT makeStringExtractor(const MemberAccessor<std::string>& a);
    static FunctorT makeParagraphExtractor(const MemberAccessor<std::string>& a);
    template<typename P, typename E = P>
    static FunctorT makePrimitiveExtractor(const MemberAccessor<P>& a);

protected:
    struct TokenFunctor
    {
        FunctorT extractor{ nullptr };
        size_t idx{ 0 };
        uint8_t marker{ MARKER_NONE };
    };

protected:
    std::unordered_map<std::string, TokenFunctor> token_map;
    size_t next_token_idx{ 0 };

};





template<typename T>
void SequentialParser<T>::addStartToken(const std::string& t)
{
    this->token_map[t] =
        TokenFunctor
        {
            .marker = MARKER_START_OBJECT
        };
}

template<typename T>
void SequentialParser<T>::addEndToken(const std::string& t)
{
    this->token_map[t] =
        TokenFunctor
        {
            .marker = MARKER_END_OBJECT
        };
}

template<typename T>
void SequentialParser<T>::addFunctorToken(const std::string& t, const FunctorT& f)
{
    this->token_map[t] =
        TokenFunctor
        {
            .extractor = f,
            .idx = this->next_token_idx++
        };
}

template<typename T>
void SequentialParser<T>::addStringToken(
    const std::string& t,
    const MemberAccessor<std::string>& a )
{
    this->token_map[t] =
        TokenFunctor
        {
            .extractor = SequentialParser<T>::makeStringExtractor(a),
            .idx = this->next_token_idx++
        };
}

template<typename T>
void SequentialParser<T>::addParagraphToken(
    const std::string& t,
    const MemberAccessor<std::string>& a )
{
    this->token_map[t] =
        TokenFunctor
        {
            .extractor = SequentialParser<T>::makeParagraphExtractor(a),
            .idx = this->next_token_idx++
        };
}

template<typename T>
template<typename P, typename E>
void SequentialParser<T>::addPrimitiveToken(
    const std::string& t,
    const MemberAccessor<P>& a )
{
    this->token_map[t] =
        TokenFunctor
        {
            .extractor = SequentialParser<T>::makePrimitiveExtractor<P, E>(a),
            .idx = this->next_token_idx++
        };
}



template<typename T>
SequentialParser<T>::FunctorT SequentialParser<T>::makeStringExtractor(
    const MemberAccessor<std::string>& a )
{
    return
        [](std::istream& line, std::istream& stream, T& x) -> void
        {
            std::getline(line, a(x));
        };
}
template<typename T>
SequentialParser<T>::FunctorT SequentialParser<T>::makeParagraphExtractor(
    const MemberAccessor<std::string>& a )
{
    return
        [](std::istream& line, std::istream& stream, T& x) -> void
        {
            std::string s;
            for(std::getline(stream, s); !stream.eof() && s != "."; std::getline(stream, s))
            {
                a(x) += s += '\n';
            }
            a(x).pop_back();
        };
}
template<typename T>
template<typename P, typename E>
SequentialParser<T>::FunctorT SequentialParser<T>::makePrimitiveExtractor(
    const MemberAccessor<P>& a )
{
    return
        [](std::istream& line, std::istream& stream, T& x) -> void
        {
            if constexpr(std::is_same<P, E>::value)
            {
                line >> a(x);
            }
            else
            {
                E temp;
                line >> temp;
                a(x) = static_cast<P>(temp);
            }
        };
}
