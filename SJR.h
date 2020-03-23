#ifndef SJR_H
#define SJR_H

#include <vector>
#include <map>

#include <fstream>

#include <cstring>
#include <cmath>
#include <type_traits>
#include <variant>
#include <algorithm>


class SJR
{
    public :

        SJR() = default;

        // The same ordering, as in the std::variant<...> value.
        enum class Type
        {
            BOOL = 0,
            INT = 1,
            FLOAT = 2,
            STRING = 3,
            ARRAY = 4,
            OBJECT = 5,
        };

        [[nodiscard]]
        bool load(std::string_view filename);

        [[nodiscard]]
        bool save(std::string_view filename);

        template<class T>
        void setValue(T&& newValue);

        [[nodiscard]]
        Type getType() const;

        template<class T>
        [[nodiscard]]
        T getValue() const;

        [[nodiscard]]
        size_t getChildCount() const;
        [[nodiscard]]
        size_t getArraySize() const;

        SJR& operator[] (std::string_view nodeName);
        SJR& operator[] (size_t index);

    private :

        struct FormatGuard
        {
            FormatGuard(std::ofstream& file, std::ios_base::fmtflags flag);

            ~FormatGuard();

            private :

                std::ofstream& file;
                std::ios_base::fmtflags flag;
        };

        template<class T>
        static constexpr auto to_integral(T enumValue) -> std::underlying_type_t<T>
        {
            return static_cast<std::underlying_type_t<T>>(enumValue);
        }

        template<class Iterator>
        Iterator remove_multi_whitespaces(Iterator begin, Iterator end);

        enum class Indexing
        {
            //  Value
            BOOL = 0,
            INT = 1,
            FLOAT = 2,
            STRING = 3,

            //  Container
            MAP = 0,
            VECTOR = 1
        };

        using mapJson    = std::map<std::string, SJR>;
        using vectorJson = std::vector<SJR>;


        std::variant<mapJson, vectorJson> container = mapJson();
        std::variant<bool, int, float, std::string> value = false;

        Type type = Type::OBJECT;

        static void writeTabs(std::ofstream& file, size_t count);
        static void skipWhiteSpace(char*&file);

        void writeBool(std::ofstream& file);
        void writeInt(std::ofstream &file);
        void writeFloat(std::ofstream &file);
        void writeString(std::ofstream &file);
        void writeArray(std::ofstream &file);
        void writeObject(std::ofstream &file);

        void write(std::ofstream& file);

        [[nodiscard]]
        bool parseBool(char*& file);
        [[nodiscard]]
        bool parseNumber(char*&file);
        [[nodiscard]]
        bool parseString(char*& file);
        [[nodiscard]]
        bool parseArray(char*& file);
        [[nodiscard]]
        bool parseObject(char*& file);

        [[nodiscard]]
        bool parse(char*& file);
};


//      ====================      ====================
//      ====================PUBLIC====================
//      ====================      ====================


inline bool SJR::load(std::string_view filename)
{
    std::ifstream file(filename.data());
    std::string str;

    if (!file.is_open())
    {
        return false;
    }

    file.seekg(0, std::ios::end);
    str.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(file)),{});

    file.close();

    char* fileData = str.data();

    try
    {
        (void)parse(fileData);
    }
    catch(...)
    {
        return false;
    }
}


[[nodiscard]]
inline bool SJR::save(std::string_view filename)
{
    std::ofstream file(filename.data());

    if (!file.is_open())
    {
        return false;
    }

    write(file);

    file.close();

    return true;
}


template<class T>
void SJR::setValue(T&& newValue)
{
    if constexpr(std::is_same_v<T, bool>)
    {
        type = Type::BOOL;
    }

    if constexpr(std::is_same_v<T, int>)
    {
        type = Type::INT;
    }

    if constexpr(std::is_same_v<T, float>)
    {
        type = Type::FLOAT;
    }

    if constexpr (std::is_same_v<T, std::string>)
    {
        type = Type::STRING;
    }

    value = std::forward<T>(newValue);
}


[[nodiscard]]
inline SJR::Type SJR::getType() const
{
    return type;
}


template<class T>
[[nodiscard]]
T SJR::getValue() const
{
    if constexpr(std::is_same_v<T, bool>)
    {
        return std::get<to_integral(Type::BOOL)>(value);
    }

    if constexpr(std::is_same_v<T, int>)
    {
        return std::get<to_integral(Type::INT)>(value);
    }

    if constexpr(std::is_same_v<T, float>)
    {
        return std::get<to_integral(Type::FLOAT)>(value);
    }

    if constexpr(std::is_same_v<T, std::string>)
    {
        return std::get<to_integral(Type::STRING)>(value);
    }
}


[[nodiscard]]
inline size_t SJR::getChildCount() const
{
    return std::get<static_cast<int>(Indexing::MAP)>(container).size();
}


[[nodiscard]]
inline size_t SJR::getArraySize() const
{
    return std::get<static_cast<int>(Indexing::VECTOR)>(container).size();
}


[[nodiscard]]
inline SJR& SJR::operator[] (std::string_view nodeName)
{
    auto it = std::get<static_cast<int>(Indexing::MAP)>(container).find(nodeName.data());

    if (it != std::get<static_cast<int>(Indexing::MAP)>(container).end())
    {
        return it->second;
    }
    else
    {
        return std::get<static_cast<int>(Indexing::MAP)>(container)[nodeName.data()];
    }
}


//  Indexing starts with 0.
//
[[nodiscard]]
inline SJR& SJR::operator[] (size_t index)
{
    if (type != Type::ARRAY)
    {
        container = vectorJson();
        std::get<static_cast<int>(Indexing::VECTOR)>(container).resize(index);
        type = Type::ARRAY;
    }

    if (index >= std::get<static_cast<int>(Indexing::VECTOR)>(container).size())
    {
        std::get<static_cast<int>(Indexing::VECTOR)>(container).resize(index + 1);
    }

    return std::get<static_cast<int>(Indexing::VECTOR)>(container).at(index);
}


//      ====================       ====================
//      ====================PRIVATE====================
//      ====================       ====================


template<class Iterator>
Iterator SJR::remove_multi_whitespaces(Iterator begin, Iterator end)
{
    return std::unique(begin, end, [](const auto& a, const auto& b)
    {
        return isspace(a) && isspace(b);
    });
}


inline void SJR::writeTabs(std::ofstream& file, size_t count)
{
    for (size_t i = 0u; i < count; ++i)
    {
        file << '\t';
    }
}


inline void SJR::skipWhiteSpace(char*&file)
{
    while (isspace(*file))
    {
        ++file;
    }
}


inline void SJR::writeBool(std::ofstream& file)
{
    FormatGuard formatGuard(file, std::ios_base::boolalpha);
    file << static_cast<bool>(std::get<to_integral(Type::BOOL)>(value));
}


inline void SJR::writeInt(std::ofstream &file)
{
    file << std::get<to_integral(Type::INT)>(value);
}


inline void SJR::writeFloat(std::ofstream &file)
{
    FormatGuard formatGuard(file, std::ios_base::fixed);
    file << std::get<to_integral(Type::FLOAT)>(value);
}


inline void SJR::writeString(std::ofstream &file)
{
    file << "\"" << std::get<to_integral(Type::STRING)>(value) << "\"";
}


inline void SJR::writeArray(std::ofstream &file)
{
    file << '[';

    if (container.index() == static_cast<int>(Indexing::VECTOR))
    {
        auto vectorBegin = std::get<static_cast<int>(Indexing::VECTOR)>(container).begin();
        auto vectorEnd = std::get<static_cast<int>(Indexing::VECTOR)>(container).end();

        for (auto it = vectorBegin; it != vectorEnd; ++it)
        {
            it->write(file);

            if (it != std::prev(vectorEnd))
            {
                file << ',' << ' ';
            }
        }
    }

    file << ']';
}


inline void SJR::writeObject(std::ofstream &file)
{
    if (type != Type::ARRAY && type != Type::OBJECT)
    {
        file << "\"" << std::get<to_integral(Type::STRING)>(value) << "\"";
        file << ": ";
    }

    static int tabsCount;

    file << '\n';
    SJR::writeTabs(file, tabsCount);
    file << '{' << '\n';

    ++tabsCount;
    SJR::writeTabs(file, tabsCount);

    if (container.index() == static_cast<int>(Indexing::MAP))
    {

        auto mapBegin = std::get<static_cast<int>(Indexing::MAP)>(container).begin();
        auto mapEnd = std::get<static_cast<int>(Indexing::MAP)>(container).end();
        for (auto it = mapBegin; it != mapEnd; ++it)
        {
            file << "\"" << it->first << "\"";
            file << ": ";

            it->second.write(file);

            if (it != (std::prev(mapEnd)))
            {
                file << ", ";
                file << '\n';
                SJR::writeTabs(file, tabsCount);
            }
        }
    }

    --tabsCount;

    file << '\n';

    SJR::writeTabs(file, tabsCount);

    file << '}';

}


inline void SJR::write(std::ofstream& file)
{
    switch (type)
    {
        case Type::BOOL :
            writeBool(file);
            break;

        case Type::INT:
            writeInt(file);
            break;

        case Type::FLOAT :
            writeFloat(file);
            break;

        case Type::STRING:
            writeString(file);
            break;

        case Type::ARRAY:
            writeArray(file);
            break;

        case Type::OBJECT:
            writeObject(file);
            break;
    }


}


[[nodiscard]]
inline bool SJR::parseBool(char*& file)
{
    bool resultTrue = memcmp(file, "true", 4) == 0;
    bool resultFalse = memcmp(file, "false", 5) == 0;

    if (resultTrue || resultFalse)
    {
        value = resultTrue;
        file += resultTrue ? 4 : 5;
        type = Type::BOOL;
        return true;
    }

    return false;
}


[[nodiscard]]
inline bool SJR::parseNumber(char*& file)
{
    bool signNegative = *file == '-';
    bool signPositive = *file == '+';

    bool sign = signNegative | signPositive;

    int valueInt{0};
    float valueFloat{0.0f};

    if (sign || isdigit(*file))
    {
        if (sign)
        {
            ++file;
        }

        while (isdigit(*file))
        {
            valueInt *= 10;
            valueInt += *file - '0';
            ++file;
        }

        if (*file == '.')
        {
            ++file;

            type = Type::FLOAT;
            valueFloat = static_cast<float>(valueInt);

            int shift = 1;

            while(isdigit(*file))
            {
                valueFloat *= pow(10, shift);
                valueFloat += static_cast<float>(*file - '0');
                valueFloat /= pow(10, shift);

                ++shift;
                ++file;
            }

            value = static_cast<float>(valueFloat * (signNegative ? -1. : 1.));

            return true;
        }

        if (*file == 'e' || *file == 'E')
        {
            ++file;

            type = Type::FLOAT;
            valueFloat = static_cast<float>(valueInt);
            valueInt = 0;

            while (isdigit(*file))
            {
                valueInt *= 10;
                valueInt += *file - '0';
            }

            valueFloat *= static_cast<float>(pow(10, valueInt));

            value = static_cast<float>(valueFloat * (signNegative ? -1. : 1.));
            return true;
        }

        type = Type::INT;
        value = valueInt * (signNegative ? -1 : 1);
        return true;
    }

    return false;
}


[[nodiscard]]
inline bool SJR::parseString(char*& file)
{
    if (*file == '"')
    {
        ++file;

        type = Type::STRING;

        value = "";

        std::string currentWord;

        SJR::skipWhiteSpace(file);

        while (*file != '"')
        {
            currentWord += *file;

            ++file;
        }

        currentWord.erase(remove_multi_whitespaces(currentWord.begin(), currentWord.end()), currentWord.end());

        ++file;

        value = std::move(currentWord);

        return true;
    }

    return false;
}


[[nodiscard]]
inline bool SJR::parseArray(char*& file)
{
    if (*file == '[')
    {
        ++file;

        type = Type::ARRAY;
        container = vectorJson();

        while (*file != ']')
        {
            SJR::skipWhiteSpace(file);

            if (*file == ']')
            {
                ++file;

                return true;
            }

            SJR newSJR;

            if (!newSJR.parse(file))
            {
                return false;
            }

            std::get<to_integral(Indexing::VECTOR)>(container).push_back(newSJR);

            SJR::skipWhiteSpace(file);

            if (*file == ']')
            {
                ++file;
                type = Type::ARRAY;
                return true;
            }

            if (*file != ',')
            {
                return false;
            }

            ++file;
        }

        ++file;
        return true;
    }

    return false;
}


[[nodiscard]]
inline bool SJR::parseObject(char*& file)
{
    if (*file == '{')
    {
        ++file;
        type = Type::OBJECT;

        while (*file != '}')
        {
            SJR::skipWhiteSpace(file);

            if (*file == '}')
            {
                ++file;
                return true;
            }


            if (*file == '"')
            {
                ++file;
                std::string nodeName{};

                while (*file != '"')
                {
                    SJR::skipWhiteSpace(file);

                    if (*file == '"')
                    {
                        break;
                    }

                    nodeName += *file;

                    ++file;
                }

                if (*file != '"')
                {
                    return true;
                }

                ++file;
                SJR::skipWhiteSpace(file);


                if (*file != ':')
                {
                    return true;
                }

                ++file;
                SJR::skipWhiteSpace(file);

                SJR newNode;
                if (!newNode.parse(file)) {
                    return true;
                }
                type = Type::OBJECT;
                std::get<to_integral(Indexing::MAP)>(container)[nodeName] = newNode;
            }

            SJR::skipWhiteSpace(file);

            if (*file == '}')
            {
                ++file;
                return true;
            }

            if (*file != ',')
            {
                return false;
            }

            ++file;
        }

        ++file;
        return true;
    }

    return false;
}


//
//
[[nodiscard]]
inline bool SJR::parse(char*& file)
{
    skipWhiteSpace(file);

    if (parseString(file))
    {
        return true;
    }

    if (parseBool(file))
    {
        return true;
    }

    if (parseNumber(file))
    {
        return true;
    }

    if (parseArray(file))
    {
        return true;
    }

    if (parseObject(file))
    {
        return true;
    }

    return false;
}


//      ====================       ====================
//      ====================STRUCTURE====================
//      ====================       ====================


inline SJR::FormatGuard::FormatGuard(std::ofstream& file, std::ios_base::fmtflags flag) :
    file(file), flag(flag)
{
    file.setf(flag);
}


inline SJR::FormatGuard::~FormatGuard()
{
    file.unsetf(flag);
}


#endif // SJR_H
