#include "StarSignalHandler.hpp"

namespace Star {

struct SignalHandlerImpl {};

SignalHandlerImplUPtr SignalHandler::s_singleton = nullptr;

SignalHandler::SignalHandler() = default;

SignalHandler::~SignalHandler() = default;

void SignalHandler::setHandleFatal(bool) {}

bool SignalHandler::handlingFatal() const {
  return false;
}

void SignalHandler::setHandleInterrupt(bool) {}

bool SignalHandler::handlingInterrupt() const {
  return false;
}

bool SignalHandler::interruptCaught() const {
  return false;
}

}
