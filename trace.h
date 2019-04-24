/**
 * \file
 * Trace and log functions and macros
 */

#ifndef TRACE_H_
#define TRACE_H_

#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace utils {

struct tr_stream_helper {
  std::ostream& get() {
    return std::cout;
  }
  ~tr_stream_helper() {
    std::cout << std::endl;
  }
};

struct err_stream_helper {
  std::ostream& get() {
    return std::cerr;
  }
  ~err_stream_helper() {
    std::cerr << std::endl;
  }
};


/*!
 *  NullStream is an analogue of "null device"; use to disable output.
 * \code
 *    #define     tr_stream       utils::NullStream()
 * \endcode
 */
class NullStream {
public:
  template <class T> NullStream& operator<<(const T& ) {
    return *this;
  }
};

inline std::string hexdump(const void* const buf, size_t len)
{
	std::ostringstream os;
	for (const uint8_t *p = (const uint8_t*)buf, *e = p + len; p != e; ++p) {
		os << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)*p << ' ';
	}
	return os.str();
}

} // namespace utils

// Defines are out of namespace anyway so use qualified typenames here
//#ifdef NDEBUG
//#define tr_stream utils::NullStream()
//#define err_stream  utils::err_stream_helper().get()
//#else
#define tr_stream   utils::tr_stream_helper().get() << __func__ << "> "
#define err_stream  utils::tr_stream_helper().get() << "[ERROR] "<<__FILE__<<"#"<<__LINE__<<":"<<__func__<<"> "
//#endif // NDEBUG

namespace LOG_LEVEL {
enum LOG_LEVEL {trace, debug, info, warn, error, fatal};
}

// Compilation: CXXFLAGS=-DDEF_LOG_LEVEL=2 make
//#define DEF_LOG_LEVEL gLogLevel;  // define this to have runtime option
#ifndef DEF_LOG_LEVEL
#define DEF_LOG_LEVEL (LOG_LEVEL::trace)
#endif
extern LOG_LEVEL::LOG_LEVEL gLogLevel;

#define log_trace   (DEF_LOG_LEVEL <= LOG_LEVEL::trace) && tr_stream
#define log_debug   (DEF_LOG_LEVEL <= LOG_LEVEL::debug) && tr_stream
#define log_info    (DEF_LOG_LEVEL <= LOG_LEVEL::info) && tr_stream
#define log_warn    (DEF_LOG_LEVEL <= LOG_LEVEL::warn) && tr_stream << "[WARN] "
#define log_error   (DEF_LOG_LEVEL <= LOG_LEVEL::error) && err_stream
#define log_fatal   err_stream << "[FATAL] "


#define TraceF      log_trace
#define TraceX(a)   log_trace << #a << " = " << (a)
#define Trace2(a,b)   log_trace << #a << " = " << (a) <<"; " << #b << " = " << (b)
#define Trace3(a,b,c)   log_trace << #a << " = " << (a) <<"; " << #b << " = " << (b) <<"; " << #c << " = " << (c)


#define NLOG4CXX
#ifndef NLOG4CXX
#include <log4cxx/logger.h>
class log_helper
{
public:
  explicit log_helper(log4cxx::LoggerPtr& l, const log4cxx::LevelPtr& lev)
    : logger(l), level(lev) {}
  ~log_helper() {
    logger->forcedLog(level, oss_.str(oss_), LOG4CXX_LOCATION);
  }

  std::ostream& get() {
    return oss_;
  }
private:
  ::log4cxx::helpers::MessageBuffer oss_;
  ::log4cxx::LoggerPtr logger;
  ::log4cxx::LevelPtr level;
  log_helper(const log_helper&);
};

extern log4cxx::LoggerPtr logger;

#undef tr_stream
#undef err_stream
#undef log_trace
#undef log_debug
#undef log_info
#undef log_warn
#undef log_error

//extern log4cxx::LoggerPtr logger;  // commented out. Instead, declare extern in the file where it needed as it may conflict with ather static declarations
#define tr_stream log_helper(logger, ::log4cxx::Level::getInfo()).get() << __func__ << "> "
#define err_stream log_helper(logger, ::log4cxx::Level::getError()).get() << __func__ << "> "
#define log_trace log_helper(logger, ::log4cxx::Level::getTrace()).get() << __func__ << "> "
#define log_debug log_helper(logger, ::log4cxx::Level::getDebug()).get() << __func__ << "> "
#define log_info log_helper(logger, ::log4cxx::Level::getInfo()).get() << __func__ << "> "
#define log_warn log_helper(logger, ::log4cxx::Level::getWarn()).get() << __func__ << "> "
#define log_error log_helper(logger, ::log4cxx::Level::getError()).get() << __func__ << "> "

#endif


#endif
