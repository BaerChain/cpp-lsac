#pragma once

#include "CommonIO.h"
#include "FixedHash.h"
#include "Terminal.h"
#include <string>
#include <vector>
#include <libdevcore/Exceptions.h>

#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

namespace dev
{
void setThreadName(std::string const& _n);

/// Set the current thread's log name.
std::string getThreadName();


#define FILE_AND_LINE   path_to_file(__FILE__) << " " << __LINE__ 
#define FORMAT_FILE      "[" << std::setw(10) << std::left   << FILE_AND_LINE  << "]  "


#define LOG(X) BOOST_LOG(X) << FORMAT_FILE

enum Verbosity
{
    VerbositySilent = -1,
    VerbosityError = 0,
    VerbosityWarning = 1,
    VerbosityInfo = 2,
    VerbosityDebug = 3,
    VerbosityTrace = 4,
};


inline std::string path_to_file(const std::string &file){
    auto pos = file.find_last_of('/', file.size());
    if(pos != std::string::npos){
        return file.substr(pos + 1, file.size());
    }
    return file;
}





// Simple cout-like stream objects for accessing common log channels.
// Thread-safe
BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_errorLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = VerbosityError)(boost::log::keywords::channel = "error"))
#define cerror LOG(dev::g_errorLogger::get())
//#define cerror ERROR_LOG

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_warnLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = VerbosityWarning)(boost::log::keywords::channel = "warn"))
#define cwarn LOG(dev::g_warnLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_noteLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = VerbosityInfo)(boost::log::keywords::channel = "info"))
#define cnote LOG(dev::g_noteLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_debugLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = VerbosityDebug)(boost::log::keywords::channel = "debug"))
#define cdebug LOG(dev::g_debugLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_traceLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = VerbosityTrace)(boost::log::keywords::channel = "trace"))
#define ctrace LOG(dev::g_traceLogger::get())

// Simple macro to log to any channel a message without creating a logger object
// e.g. clog(VerbosityInfo, "channel") << "message";
// Thread-safe
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    g_clogLogger, boost::log::sources::severity_channel_logger_mt<>);
#define clog(SEVERITY, CHANNEL)                            \
    BOOST_LOG_STREAM_WITH_PARAMS(dev::g_clogLogger::get(), \
        (boost::log::keywords::severity = SEVERITY)(boost::log::keywords::channel = CHANNEL)) << FORMAT_FILE

#define testlog LOG(dev::g_warnLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_lateLogger,
     boost::log::sources::severity_channel_logger_mt<>,
     (boost::log::keywords::severity = VerbosityDebug)(boost::log::keywords::channel = "latecy"))
#define CLATE_LOG LOG(dev::g_lateLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_p2pTestLogger,
     boost::log::sources::severity_channel_logger_mt<>,
     (boost::log::keywords::severity = VerbosityDebug)(boost::log::keywords::channel = "p2ptest"))
#define CP2P_LOG  LOG(dev::g_p2pTestLogger::get())


BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_FeeLogger,
                                             boost::log::sources::severity_channel_logger_mt<>,
                                             (boost::log::keywords::severity = VerbosityDebug)(boost::log::keywords::channel = "fee"))
#define CFEE_LOG LOG(dev::g_FeeLogger::get())

struct LoggingOptions
{
    int verbosity = VerbosityInfo;
    strings includeChannels;
    strings excludeChannels;
};

// Should be called in every executable
void setupLogging(LoggingOptions const& _options);

// Simple non-thread-safe logger with fixed severity and channel for each message
// For better formatting it is recommended to limit channel name to max 6 characters.
using Logger = boost::log::sources::severity_channel_logger<>;
inline Logger createLogger(int _severity, std::string const& _channel)
{
    return Logger(
        boost::log::keywords::severity = _severity, boost::log::keywords::channel = _channel);
}

// Adds the context string to all log messages in the scope
#define LOG_SCOPED_CONTEXT(context) \
    BOOST_LOG_SCOPED_THREAD_ATTR("Context", boost::log::attributes::constant<std::string>(context));


// Below overloads for both const and non-const references are needed, because without overload for
// non-const reference generic operator<<(formatting_ostream& _strm, T& _value) will be preferred by
// overload resolution.
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bigint const& _value)
{
    _strm.stream() << BrcNavy << _value << BrcReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bigint& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u256 const& _value)
{
    _strm.stream() << BrcNavy << _value << BrcReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u256& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u160 const& _value)
{
    _strm.stream() << BrcNavy << _value << BrcReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u160& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <unsigned N>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, FixedHash<N> const& _value)
{
    _strm.stream() << BrcTeal "#" << _value.abridged() << BrcReset;
    return _strm;
}
template <unsigned N>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, FixedHash<N>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h160 const& _value)
{
    _strm.stream() << BrcRed "@" << _value.abridged() << BrcReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h160& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h256 const& _value)
{
    _strm.stream() << BrcCyan "#" << _value.abridged() << BrcReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h256& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h512 const& _value)
{
    _strm.stream() << BrcTeal "##" << _value.abridged() << BrcReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h512& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bytesConstRef _value)
{
    _strm.stream() << BrcYellow "%" << toHex(_value) << BrcReset;
    return _strm;
}

//inline boost::log::formatting_ostream& operator<<(
//        boost::log::formatting_ostream& _strm, Exception const&_value)
//{
//    _strm << _value.what();
//    return _strm;
//}
//
//
//inline boost::log::formatting_ostream& operator<<(
//        boost::log::formatting_ostream& _strm, boost::exception const&_value)
//{
//    _strm << boost::diagnostic_information_what(_value);
//    return _strm;
//}


}  // namespace dev

// Overloads for types of std namespace can't be defined in dev namespace, because they won't be
// found due to Argument-Dependent Lookup Placing anything into std is not allowed, but we can put
// them into boost::log
namespace boost
{
namespace log
{
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, dev::bytes const& _value)
{
    _strm.stream() << BrcYellow "%" << dev::toHex(_value) << BrcReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, dev::bytes& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::vector<T> const& _value)
{
    _strm.stream() << BrcWhite "[" BrcReset;
    int n = 0;
    for (T const& i : _value)
    {
        _strm.stream() << (n++ ? BrcWhite ", " BrcReset : "");
        _strm << i;
    }
    _strm.stream() << BrcWhite "]" BrcReset;
    return _strm;
}
template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::vector<T>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::set<T> const& _value)
{
    _strm.stream() << BrcYellow "{" BrcReset;
    int n = 0;
    for (T const& i : _value)
    {
        _strm.stream() << (n++ ? BrcYellow ", " BrcReset : "");
        _strm << i;
    }
    _strm.stream() << BrcYellow "}" BrcReset;
    return _strm;
}
template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::set<T>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_set<T> const& _value)
{
    _strm.stream() << BrcYellow "{" BrcReset;
    int n = 0;
    for (T const& i : _value)
    {
        _strm.stream() << (n++ ? BrcYellow ", " BrcReset : "");
        _strm << i;
    }
    _strm.stream() << BrcYellow "}" BrcReset;
    return _strm;
}
template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_set<T>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::map<T, U> const& _value)
{
    _strm.stream() << BrcLime "{" BrcReset;
    int n = 0;
    for (auto const& i : _value)
    {
        _strm << (n++ ? BrcLime ", " BrcReset : "");
        _strm << i.first;
        _strm << (n++ ? BrcLime ": " BrcReset : "");
        _strm << i.second;
    }
    _strm.stream() << BrcLime "}" BrcReset;
    return _strm;
}
template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::map<T, U>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_map<T, U> const& _value)
{
    _strm << BrcLime "{" BrcReset;
    int n = 0;
    for (auto const& i : _value)
    {
        _strm.stream() << (n++ ? BrcLime ", " BrcReset : "");
        _strm << i.first;
        _strm.stream() << (n++ ? BrcLime ": " BrcReset : "");
        _strm << i.second;
    }
    _strm << BrcLime "}" BrcReset;
    return _strm;
}
template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_map<T, U>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::pair<T, U> const& _value)
{
    _strm.stream() << BrcPurple "(" BrcReset;
    _strm << _value.first;
    _strm.stream() << BrcPurple ", " BrcReset;
    _strm << _value.second;
    _strm.stream() << BrcPurple ")" BrcReset;
    return _strm;
}
template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::pair<T, U>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}
}
}  // namespace boost