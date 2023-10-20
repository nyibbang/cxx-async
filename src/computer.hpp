#include <stdexcept>
#include <variant>
#include <string_view>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/outcome.hpp>

using BOOST_OUTCOME_V2_NAMESPACE::result;

/// ==============================================================================================
/// Error handling types.
/// ==============================================================================================
enum class ComputeError {
    NotACommand,
    NotAnAnswer,
    MissingValue,
    InvalidValue,
    ValueOutOfRange,
    DivisionByZero,
};

struct ComputeErrorCategory : std::error_category
{
    inline virtual
    const char* name() const noexcept override final {
        return "Compute Error";
    }

    inline virtual
    std::string message(int c) const override final {
        switch (static_cast<ComputeError>(c))
        {
            case ComputeError::NotACommand:
                return "not a command";
            case ComputeError::NotAnAnswer:
                return "not an answer";
            case ComputeError::MissingValue:
                return "value is missing";
            case ComputeError::InvalidValue:
                return "value is invalid";
            case ComputeError::ValueOutOfRange:
                return "value is out of range";
            case ComputeError::DivisionByZero:
                return "division by zero";
            default:
                return "unknown";
        }
    }

    inline virtual
    std::error_condition default_error_condition(int c) const noexcept override final {
        switch (static_cast<ComputeError>(c))
        {
            case ComputeError::NotACommand:
            case ComputeError::NotAnAnswer:
            case ComputeError::MissingValue:
            case ComputeError::InvalidValue:
            case ComputeError::DivisionByZero:
                return make_error_condition(std::errc::invalid_argument);
            case ComputeError::ValueOutOfRange:
                return make_error_condition(std::errc::result_out_of_range);
            default:
                return std::error_condition(c, *this);
        }
    }
};

inline const ComputeErrorCategory& compute_error_category()
{
  static const ComputeErrorCategory c;
  return c;
}

inline std::error_code make_error_code(ComputeError e)
{
  return {static_cast<int>(e), compute_error_category()};
}

namespace std {
    template<>
    struct is_error_code_enum<ComputeError> : true_type {};
}

/// ==============================================================================================
/// A command is a sum of all these types.
/// ==============================================================================================
struct Init { int value; };
struct Add { int value; };
struct Mul { int value; };
struct Div { int value; };
struct Compute {};

struct Command : std::variant<Init, Add, Mul, Div, Compute>
{
    using Variant = std::variant<Init, Add, Mul, Div, Compute>;
    using Variant::variant;
    using Variant::operator=;

    inline static
    result<Command> from_string(std::string_view s)
    {
        std::vector<std::string> parts;
        boost::algorithm::split(
            parts,
            s,
            [](char c) { return std::isspace(c); },
            boost::algorithm::token_compress_on
        );
        auto parts_it = parts.begin();
        auto parts_end = parts.end();

        if (parts_it == parts_end) {
            return ComputeError::NotACommand;
        }

        auto name = *parts_it++;

        if (name == "compute") {
            return Command(Compute {});
        }

        if (parts_it == parts_end) {
            return ComputeError::MissingValue;
        }

        auto value_str = *parts_it;
        int value = 0;
        try {
            value = std::stoi(value_str);
        }
        catch (const std::invalid_argument&) {
            return ComputeError::InvalidValue;
        }
        catch (const std::out_of_range&) {
            return ComputeError::ValueOutOfRange;
        }

        if (name == "init") {
            return Command(Init{ value });
        }
        else if (name == "add") {
            return Command(Add{ value });
        }
        else if (name == "mul") {
            return Command(Mul{ value });
        }
        else if (name == "div") {
            if (value == 0) {
                return ComputeError::DivisionByZero;
            }
            return Command(Div{ value });
        }
        return ComputeError::NotACommand;
    }

    inline
    std::optional<int> run(int& value) const
    {
        return std::visit(
            [&]<typename T>(const T& command) -> std::optional<int> {
                if constexpr (std::is_same_v<T, Init>) {
                    value = command.value;
                    return std::nullopt;
                }
                else if constexpr (std::is_same_v<T, Add>) {
                    value += command.value;
                    return std::nullopt;
                }
                else if constexpr (std::is_same_v<T, Mul>) {
                    value *= command.value;
                    return std::nullopt;
                }
                else if constexpr (std::is_same_v<T, Div>) {
                    value /= command.value;
                    return std::nullopt;
                }
                else if constexpr (std::is_same_v<T, Compute>) {
                    return value;
                }
                else {
                    static_assert(std::is_void_v<std::void_t<T>>, "non-exhaustive visitor!");
                }
            },
            *this
        );
    }
};

inline
std::ostream& operator<<(std::ostream& os, const Command& command) {
    return std::visit(
        [&]<typename T>(const T& command) -> std::ostream& {
            if constexpr (std::is_same_v<T, Init>) {
                return os << "init(" << command.value <<  ")";
            }
            else if constexpr (std::is_same_v<T, Add>) {
                return os << "add(" << command.value <<  ")";
            }
            else if constexpr (std::is_same_v<T, Mul>) {
                return os << "mul(" << command.value <<  ")";
            }
            else if constexpr (std::is_same_v<T, Div>) {
                return os << "div(" << command.value <<  ")";
            }
            else if constexpr (std::is_same_v<T, Compute>) {
                return os << "compute";
            }
            else {
                static_assert(std::is_void_v<std::void_t<T>>, "non-exhaustive visitor!");
            }
        },
        command
    );
}

namespace std {
    template<>
    struct variant_size<Command>
        : variant_size<Command::variant> {};

    template<std::size_t I>
    struct variant_alternative<I, Command>
        : variant_alternative<I, Command::variant> {};
}

inline
std::string to_string(const Command& command)
{
    return std::visit(
        []<typename T>(const T& command) {
            std::ostringstream oss;
            if constexpr (std::is_same_v<T, Init>) {
                oss << "init " << command.value;
            }
            else if constexpr (std::is_same_v<T, Add>) {
                oss << "add " << command.value;
            }
            else if constexpr (std::is_same_v<T, Mul>) {
                oss << "mul " << command.value;
            }
            else if constexpr (std::is_same_v<T, Div>) {
                oss << "div " << command.value;
            }
            else if constexpr (std::is_same_v<T, Compute>) {
                oss << "compute";
            }
            else {
                static_assert(std::is_void_v<std::void_t<T>>, "non-exhaustive visitor!");
            }
            return oss.str();
        },
        command
    );
}

/// ==============================================================================================
/// A simple answer, with its value.
/// ==============================================================================================
struct Answer {
    int value;

    inline static
    result<Answer> from_string(std::string_view s)
    {
        std::vector<std::string> parts;
        boost::algorithm::split(
            parts,
            s,
            [](char c) { return std::isspace(c); },
            boost::algorithm::token_compress_on
        );
        auto parts_it = parts.begin();
        auto parts_end = parts.end();

        if (parts_it == parts_end) {
            return ComputeError::NotACommand;
        }

        auto name = *parts_it++;

        if (name != "answer") {
            return ComputeError::NotAnAnswer;
        }

        if (parts_it == parts_end) {
            return ComputeError::MissingValue;
        }

        auto value_str = *parts_it;
        int value = 0;
        try {
            value = std::stoi(value_str);
        }
        catch (const std::invalid_argument&) {
            return ComputeError::InvalidValue;
        }
        catch (const std::out_of_range&) {
            return ComputeError::ValueOutOfRange;
        }
        return Answer{value };
    }
};

inline
std::string to_string(const Answer& answer)
{
    return "answer " + std::to_string(answer.value);
}

inline
std::ostream& operator<<(std::ostream& os, const Answer& answer) {
    return os << "answer(" << answer.value << ")";
}
