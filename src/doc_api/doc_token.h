#ifndef DOC_TOKEN_H_
#define DOC_TOKEN_H_

#include "./doc_addr.h"

#include <filesystem>
#include <string>

enum class TextStyle
{
    Normal = 0,
    Bold = 1 << 0,
    Italic = 1 << 1,
    Monospace = 1 << 2,
};

inline TextStyle operator|(TextStyle a, TextStyle b)
{
    return static_cast<TextStyle>(static_cast<int>(a) | static_cast<int>(b));
}

inline TextStyle &operator|=(TextStyle &a, TextStyle b)
{
    a = a | b;
    return a;
}

inline bool has_style(TextStyle style, TextStyle flag)
{
    return (static_cast<int>(style) & static_cast<int>(flag)) != 0;
}

enum class TokenType
{
    Text,
    Header,
    Image,
    ListItem,
};

struct DocToken
{
    TokenType type;
    DocAddr address;

    DocToken(TokenType type, DocAddr address);
    virtual bool operator==(const DocToken &other) const;
    virtual std::string to_string() const = 0;

protected:
    std::string common_to_string(std::string data) const;
};

struct TextDocToken : public DocToken
{
    std::string text;
    TextStyle style;

    TextDocToken(DocAddr address, const std::string &text, TextStyle style = TextStyle::Normal);
    bool operator==(const DocToken &other) const override;
    std::string to_string() const override;
};

struct HeaderDocToken : public DocToken
{
    std::string text;
    TextStyle style;

    HeaderDocToken(DocAddr address, const std::string &text, TextStyle style = TextStyle::Bold);
    bool operator==(const DocToken &other) const override;
    std::string to_string() const override;
};

struct ImageDocToken : public DocToken
{
    std::filesystem::path path;

    ImageDocToken(DocAddr address, const std::filesystem::path &path);
    bool operator==(const DocToken &other) const override;
    std::string to_string() const override;
};

struct ListItemDocToken : public DocToken
{
    std::string text;
    TextStyle style;
    int nest_level;

    ListItemDocToken(DocAddr address, const std::string &text, int nest_level, TextStyle style = TextStyle::Normal);
    bool operator==(const DocToken &other) const override;
    std::string to_string() const override;
};

std::string to_string(TokenType type);

#endif
