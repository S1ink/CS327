#pragma once

#include <initializer_list>
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <utility>
#include <string>
#include <vector>
#include <cmath>

#include "items.hpp"


template<typename T>
class SequentialParser
{
    using FunctorT = std::function<void(std::istream&, std::istream&, T&)>;

    template<typename M>
    using MemberAccessor = std::function<M&(T&)>;
    template<typename A>
    using KeywordToken = std::pair<std::string, A>;
    template<typename A>
    using AttributeMap = std::unordered_map<std::string, A>;

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
    void addRollableToken(const std::string& t, const MemberAccessor<RollNum>& a);

    template<typename A>
    inline void addAttributeToken(
        const std::string& t,
        const MemberAccessor<A>& a,
        std::initializer_list<KeywordToken<A>> attributes )
    {
        this->addAttributeToken(t, a, std::unordered_map<std::string, A>{ attributes });
    }
    template<typename A>
    void addAttributeToken(
        const std::string& t,
        const MemberAccessor<A>& a,
        AttributeMap<A>&& m );

    void parse(std::istream& in, std::vector<T>& out);

protected:
    static FunctorT makeStringExtractor(const MemberAccessor<std::string>& a);
    static FunctorT makeParagraphExtractor(const MemberAccessor<std::string>& a);
    template<typename P, typename E = P>
    static FunctorT makePrimitiveExtractor(const MemberAccessor<P>& a);
    static FunctorT makeRollableExtractor(const MemberAccessor<RollNum>& a);
    template<typename A>
    static FunctorT makeAttributeExtractor(
        const MemberAccessor<A>& a,
        const AttributeMap<A>& m );

protected:
    struct TokenFunctor
    {
        FunctorT extractor{ nullptr };
        size_t idx{ 0 };
    };

protected:
    std::unordered_map<std::string, TokenFunctor> token_map;
    std::string start_object_token, end_object_token;
    size_t next_token_idx{ 0 };

};





template<typename T>
void SequentialParser<T>::addStartToken(const std::string& t)
{
    this->start_object_token = t;
}

template<typename T>
void SequentialParser<T>::addEndToken(const std::string& t)
{
    this->end_object_token = t;
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
void SequentialParser<T>::addRollableToken(
    const std::string& t,
    const MemberAccessor<RollNum>& a )
{
    this->token_map[t] =
        TokenFunctor
        {
            .extractor = SequentialParser<T>::makeRollableExtractor(a),
            .idx = this->next_token_idx++;
        };
}

template<typename T>
template<typename A>
void SequentialParser<T>::addAttributeToken(
    const std::string& t,
    const MemberAccessor<A>& a,
    AttributeMap<A>&& m )
{
    this->token_map[t] =
        TokenFunctor
        {
            .extractor = SequentialParser<T>::makeAttributeExtractor(a, m),
            .idx = this->next_token_idx++
        };
}



template<typename T>
typename SequentialParser<T>::FunctorT SequentialParser<T>::makeStringExtractor(
    const MemberAccessor<std::string>& a )
{
    return
        [a](std::istream& line, std::istream& stream, T& x) -> void
        {
            std::getline(line, a(x));
        };
}

template<typename T>
typename SequentialParser<T>::FunctorT SequentialParser<T>::makeParagraphExtractor(
    const MemberAccessor<std::string>& a )
{
    return
        [a](std::istream& line, std::istream& stream, T& x) -> void
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
typename SequentialParser<T>::FunctorT SequentialParser<T>::makePrimitiveExtractor(
    const MemberAccessor<P>& a )
{
    return
        [a](std::istream& line, std::istream& stream, T& x) -> void
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

template<typename T>
typename SequentialParser<T>::FunctorT SequentialParser<T>::makeRollableExtractor(
    const MemberAccessor<RollNum>& a )
{
    return
        [a](std::istream& line, std::istream& stream, T& x) -> void
        {
            std::string seg;

            std::getline(line, seg, '+');
            a(x).base = static_cast<uint32_t>( atoi(seg.c_str()) );

            std::getline(line, seg, 'd');
            a(x).rolls = static_cast<uint16_t>( atoi(seg.c_str()) );

            line >> a(x).sides;
        };
}

template<typename T>
template<typename A>
typename SequentialParser<T>::FunctorT SequentialParser<T>::makeAttributeExtractor(
    const MemberAccessor<A>& a,
    const AttributeMap<A>& m )
{
    return
        [a](std::istream& line, std::istream& stream, T& x) -> void
        {
            std::string seg;
            do
            {
                std::getline(line, seg, ' ');
                auto search = m.find(seg);
                if(search != m.end())
                {
                    a(x) |= search->second;
                }
            }
            while(!line.eof());
        };
}





template<typename T>
void SequentialParser<T>::parse(std::istream& in, std::vector<T>& out)
{
    out.clear();

    std::string line;
    bool in_object = false;
    std::vector<bool> token_mask;

    for(std::getline(in, line); !in.eof(); std::getline(in, line))
    {
        if( !in_object &&
            !this->start_object_token.compare(0, this->start_object_token.length(), line) )
        {
            if(!in_object)
            {
                out.emplace_back();
                in_object = true;
                token_mask.assign(this->next_token_idx, false);
            }
            continue;
        }
        else
        if( in_object &&
            !this->end_object_token.compare(0, this->end_object_token.length(), line) )
        {
            in_object = false;
            continue;
        }

        std::stringstream ss{ line };
        std::getline(ss, line, ' ');

        auto search = this->token_map.find(line);
        if(search == this->token_map.end()) continue;

        const TokenFunctor& tk = search->second;
        if(in_object && (token_mask[tk.idx] = !token_mask[tk.idx]))
        {
            tk.extractor(ss, in, out.back());
        }
        else
        {
            out.pop_back();
            in_object = false;
        }
    }
}
