#ifndef FAKE_CURL_HPP
#define FAKE_CURL_HPP

#include <string>

namespace fake_curl
{
  constexpr int TRANSFER_OK = 0;
  constexpr int TRANSFER_COULDNT_CONNECT = 7;
  constexpr int TRANSFER_TIMEOUT = 28;

  struct State
  {
    std::string response_body;
    long int http_status;
    int transfer_result;
    bool fail_init;
    int fail_setopt_at;

    std::string url;
    std::string user_agent;
    long int timeout;
    long int no_signal;
    int handles_opened;
    int handles_closed;
    int setopt_calls;
    int performs;
    int global_init_calls;
    int global_cleanup_calls;
  };

  State &state();
  void reset();
}

#endif
