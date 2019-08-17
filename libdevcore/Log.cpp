#include "Log.h"

#ifdef __APPLE__
#include <pthread.h>
#endif

#include <boost/core/null_deleter.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/function.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/exception_handler.hpp>
#include <boost/log/utility/setup/file.hpp>

#if defined(NDEBUG)
#include <boost/log/sinks/async_frontend.hpp>
template <class T>
using log_sink = boost::log::sinks::asynchronous_sink<T>;
#else
#include <boost/log/sinks/sync_frontend.hpp>
template <class T>
using log_sink = boost::log::sinks::synchronous_sink<T>;
#endif
namespace keywords = boost::log::keywords;
namespace dev
{
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(context, "Context", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(threadName, "ThreadName", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

namespace
{
/// Associate a name with each thread for nice logging.
struct ThreadLocalLogName
{
    ThreadLocalLogName(std::string const& _name) { m_name.reset(new std::string(_name)); }
    boost::thread_specific_ptr<std::string> m_name;
};

ThreadLocalLogName g_logThreadName("main");

auto const g_timestampFormatter =
    (boost::log::expressions::stream
        << BrcViolet << boost::log::expressions::format_date_time(timestamp, "%m-%d %H:%M:%S-%f")
        << BrcReset " ");
auto const g_timestampFormatter_file =
    (boost::log::expressions::stream
        << boost::log::expressions::format_date_time(timestamp, "%m-%d %H:%M:%S-%f")
        << " ");

std::string verbosityToString(int _verbosity)
{
    switch (_verbosity)
    {
    case VerbosityError:
        return "ERROR";
    case VerbosityWarning:
        return "WARN";
    case VerbosityInfo:
        return "INFO";
    case VerbosityDebug:
        return "DEBUG";
    case VerbosityTrace:
        return "TRACE";
    }
    return {};
}

void formatter(boost::log::record_view const& _rec, boost::log::formatting_ostream& _strm)
{
    _strm << std::setw(5) << std::left << verbosityToString(_rec.attribute_values()[severity].get())
          << " ";

    g_timestampFormatter(_rec, _strm);

    _strm << BrcNavy << std::setw(4) << std::left << _rec[threadName] << BrcReset " ";
    _strm << std::setw(6) << std::left << _rec[channel] << " ";
    if (boost::log::expressions::has_attr(context)(_rec))
        _strm << BrcNavy << _rec[context] << BrcReset " ";

    _strm << _rec[boost::log::expressions::smessage];
}

void formatterFile(boost::log::record_view const& _rec, boost::log::formatting_ostream& _strm)
{
	_strm << std::setw(5) << std::left << verbosityToString(_rec.attribute_values()[severity].get())
		<< " ";
	g_timestampFormatter_file(_rec, _strm);
	_strm << std::setw(4) << std::left << _rec[threadName] <<" ";
	_strm << std::setw(6) << std::left << _rec[channel] << " ";
	if(boost::log::expressions::has_attr(context)(_rec))
		_strm << _rec[context]<< " ";
	_strm << _rec[boost::log::expressions::smessage];
}

}  // namespace

std::string getThreadName()
{
#if defined(__GLIBC__) || defined(__APPLE__)
    char buffer[128];
    pthread_getname_np(pthread_self(), buffer, 127);
    buffer[127] = 0;
    return buffer;
#else
    return g_logThreadName.m_name.get() ? *g_logThreadName.m_name.get() : "<unknown>";
#endif
}

void setThreadName(std::string const& _n)
{
#if defined(__GLIBC__)
    pthread_setname_np(pthread_self(), _n.c_str());
#elif defined(__APPLE__)
    pthread_setname_np(_n.c_str());
#else
    g_logThreadName.m_name.reset(new std::string(_n));
#endif
}

void setupLogging(LoggingOptions const& _options)
{
    auto sink = boost::make_shared<log_sink<boost::log::sinks::text_ostream_backend>>();

    boost::shared_ptr<std::ostream> stream{&std::cout, boost::null_deleter{}};
    sink->locked_backend()->add_stream(stream);
    sink->set_filter([_options](boost::log::attribute_value_set const& _set) {
        if (_set[severity] > _options.verbosity)
            return false;

        auto const messageChannel = _set[channel];
        return (_options.includeChannels.empty() ||
                   contains(_options.includeChannels, messageChannel)) &&
               !contains(_options.excludeChannels, messageChannel);
    });

    sink->set_formatter(&formatter);

	auto file_sink = boost::log::add_file_log(
		keywords::file_name = "%Y-%m-%d_%N.log",            //log fileName
		keywords::rotation_size = 200 * 1024 * 1024,         //a file size
		keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0)    //rebuild everyday    
	);
	file_sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector(
		keywords::target = "logs",                      //logs dir name 
		keywords::max_size = 500 * 1024 * 1024,          //dir max size
		keywords::min_free_space = 1000 * 1024 * 1024    //reserved dir size
	));
	file_sink->set_filter([_options](boost::log::attribute_value_set const& _set){
		if(_set[severity] > _options.verbosity)
			return false;
		auto const messageChannel = _set[channel];
		return (_options.includeChannels.empty() ||
				contains(_options.includeChannels, messageChannel)) &&
			!contains(_options.excludeChannels, messageChannel);
	});
	file_sink->set_formatter(&formatterFile);
	file_sink->locked_backend()->scan_for_files();
	file_sink->locked_backend()->auto_flush(true);
	//file_sink->imbue(std::locale("zh_CN.UTF-8"));
	//file_sink->locked_backend()->add_stream(stream)

	boost::log::core::get()->add_sink(sink);    
	boost::log::core::get()->add_sink(file_sink);

    boost::log::core::get()->add_global_attribute(
        "ThreadName", boost::log::attributes::make_function(&getThreadName));
    boost::log::core::get()->add_global_attribute(
        "TimeStamp", boost::log::attributes::local_clock());

    boost::log::core::get()->set_exception_handler(
        boost::log::make_exception_handler<std::exception>([](std::exception const& _ex) {
        std::cerr << "Exception from the logging library: " << _ex.what() << '\n';
    }));
}

}  // namespace dev
