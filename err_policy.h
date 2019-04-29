#ifndef ERRPOLICY_H_
#define ERRPOLICY_H_

//# include "trace.h" // ??? needed for log_trace but this may cause unwanted coupling

#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <memory>


namespace utils {


class ErrStream  : public std::runtime_error
{
public:
	ErrStream() : std::runtime_error("this should never happen") {}
	ErrStream(const ErrStream& other) :
		std::runtime_error(other.buf_ ? other.buf_->str() : other.what()) {}

    template <typename X>
    ErrStream& operator<<(const X& x) {
    	if (!buf_) buf_.reset(new std::ostringstream());
      *buf_ << x;
      return *this;
    }
private:
	std::unique_ptr<std::ostringstream> buf_;
};

} // namespace utils


#define signalError throw utils::ErrStream() << __FILE__<<"#"<<__LINE__<<":"<<__func__<<"> "
#define require(condition) if (!(condition)) throw utils::ErrStream() << __FILE__<<"#"<<__LINE__<<":"<<__func__<<">  [Condition '" #condition "' failed] "


#endif
