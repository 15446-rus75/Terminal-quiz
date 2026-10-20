#include "fake_curl.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <curl/curl.h>
#include <string>

namespace
{
  constexpr size_t CHUNK_SIZE = 16;

  static_assert(CURLE_OK == fake_curl::TRANSFER_OK);
  static_assert(CURLE_COULDNT_CONNECT == fake_curl::TRANSFER_COULDNT_CONNECT);
  static_assert(CURLE_OPERATION_TIMEDOUT == fake_curl::TRANSFER_TIMEOUT);

  struct FakeHandle
  {
    curl_write_callback write_function = nullptr;
    void *write_data = nullptr;
  };
}

fake_curl::State &fake_curl::state()
{
  static State instance{};
  return instance;
}

void fake_curl::reset()
{
  State &current = state();
  current.response_body = R"json({"response_code":0,"results":[]})json";
  current.http_status = 200;
  current.transfer_result = TRANSFER_OK;
  current.fail_init = false;
  current.fail_setopt_at = 0;
  current.url.clear();
  current.user_agent.clear();
  current.timeout = -1;
  current.no_signal = -1;
  current.handles_opened = 0;
  current.handles_closed = 0;
  current.setopt_calls = 0;
  current.performs = 0;
}

CURLcode curl_global_init(long int)
{
  ++fake_curl::state().global_init_calls;
  return CURLE_OK;
}

void curl_global_cleanup()
{
  ++fake_curl::state().global_cleanup_calls;
}

CURL *curl_easy_init()
{
  fake_curl::State &current = fake_curl::state();
  if (current.fail_init)
  {
    return nullptr;
  }
  ++current.handles_opened;
  return new FakeHandle();
}

void curl_easy_cleanup(CURL *handle)
{
  ++fake_curl::state().handles_closed;
  delete static_cast< FakeHandle * >(handle);
}

CURLcode curl_easy_setopt(CURL *handle, CURLoption option, ...)
{
  fake_curl::State &current = fake_curl::state();
  if (++current.setopt_calls == current.fail_setopt_at)
  {
    return CURLE_UNKNOWN_OPTION;
  }

  FakeHandle &fake = *static_cast< FakeHandle * >(handle);
  std::va_list arguments;
  va_start(arguments, option);
  switch (option)
  {
  case CURLOPT_URL:
    current.url = va_arg(arguments, const char *);
    break;
  case CURLOPT_USERAGENT:
    current.user_agent = va_arg(arguments, const char *);
    break;
  case CURLOPT_WRITEFUNCTION:
    fake.write_function = va_arg(arguments, curl_write_callback);
    break;
  case CURLOPT_WRITEDATA:
    fake.write_data = va_arg(arguments, void *);
    break;
  case CURLOPT_TIMEOUT:
    current.timeout = va_arg(arguments, long int);
    break;
  case CURLOPT_NOSIGNAL:
    current.no_signal = va_arg(arguments, long int);
    break;
  default:
    break;
  }
  va_end(arguments);
  return CURLE_OK;
}

CURLcode curl_easy_perform(CURL *handle)
{
  fake_curl::State &current = fake_curl::state();
  ++current.performs;
  if (current.transfer_result != fake_curl::TRANSFER_OK)
  {
    return static_cast< CURLcode >(current.transfer_result);
  }

  const FakeHandle &FAKE = *static_cast< FakeHandle * >(handle);
  const std::string &BODY = current.response_body;
  for (size_t offset = 0; offset < BODY.size(); offset += CHUNK_SIZE)
  {
    std::string chunk = BODY.substr(offset, CHUNK_SIZE);
    if (FAKE.write_function(chunk.data(), 1, chunk.size(), FAKE.write_data) != chunk.size())
    {
      return CURLE_WRITE_ERROR;
    }
  }
  return CURLE_OK;
}

CURLcode curl_easy_getinfo(CURL *, CURLINFO info, ...)
{
  std::va_list arguments;
  va_start(arguments, info);
  if (info == CURLINFO_RESPONSE_CODE)
  {
    *va_arg(arguments, long int *) = fake_curl::state().http_status;
  }
  va_end(arguments);
  return CURLE_OK;
}

const char *curl_easy_strerror(CURLcode code)
{
  switch (code)
  {
  case CURLE_COULDNT_CONNECT:
    return "Couldn't connect to server";
  case CURLE_OPERATION_TIMEDOUT:
    return "Timeout was reached";
  case CURLE_WRITE_ERROR:
    return "Failed writing received data to disk/application";
  default:
    return "Unknown error";
  }
}
