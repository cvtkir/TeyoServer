#pragma once
#include <memory>

class SessionManager;
class Session;

using SessionManagerPtr = std::shared_ptr<SessionManager>;
using SessionPtr = std::shared_ptr<Session>;