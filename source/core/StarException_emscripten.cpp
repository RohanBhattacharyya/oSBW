#include "StarException.hpp"
#include "StarCasting.hpp"

namespace Star {

StarException::StarException() noexcept
  : StarException(std::string("StarException")) {}

StarException::~StarException() noexcept {}

StarException::StarException(std::string message, bool) noexcept
  : StarException("StarException", std::move(message), false) {}

StarException::StarException(std::exception const& cause) noexcept
  : StarException("StarException", std::string(), cause) {}

StarException::StarException(std::string message, std::exception const& cause) noexcept
  : StarException("StarException", std::move(message), cause) {}

const char* StarException::what() const throw() {
  if (m_whatBuffer.empty()) {
    std::ostringstream os;
    m_printException(os, false);
    m_whatBuffer = os.str();
  }
  return m_whatBuffer.c_str();
}

StarException::StarException(char const* type, std::string message, bool) noexcept {
  auto printException = [](std::ostream& os, bool, char const* type, std::string message) {
    os << "(" << type << ")";
    if (!message.empty())
      os << " " << message;
  };
  m_printException = bind(printException, _1, _2, type, std::move(message));
}

StarException::StarException(char const* type, std::string message, std::exception const& cause) noexcept
  : StarException(type, std::move(message), false) {
  auto printException = [](std::ostream& os, bool fullStacktrace, function<void(std::ostream&, bool)> self, function<void(std::ostream&, bool)> cause) {
    self(os, fullStacktrace);
    os << std::endl << "Caused by: ";
    cause(os, fullStacktrace);
  };

  std::function<void(std::ostream&, bool)> printCause;
  if (auto starException = as<StarException>(&cause)) {
    printCause = bind(starException->m_printException, _1, _2);
  } else {
    printCause = bind([](std::ostream& os, bool, std::string causeWhat) {
      os << "std::exception: " << causeWhat;
    }, _1, _2, std::string(cause.what()));
  }

  m_printException = bind(printException, _1, _2, m_printException, printCause);
}

void printStack(char const* message) {
  _unused(message);
}

}
