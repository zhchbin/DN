#include "net_errors.h"

#include "base/basictypes.h"
#include "base/metrics/histogram.h"
#include "base/strings/stringize_macros.h"


namespace net {

const char kErrorDomain[] = "net";

std::string ErrorToString(int error) {
  return "net::" + ErrorToShortString(error);
}

std::string ErrorToShortString(int error) {
  if (error == 0)
    return "OK";

  const char* error_string;
  switch (error) {
#define NET_ERROR(label, value) \
  case ERR_ ## label: \
    error_string = # label; \
    break;
#include "net_error_list.h"
#undef NET_ERROR
  default:
    NOTREACHED();
    error_string = "<unknown>";
  }
  return std::string("ERR_") + error_string;
}

Error FileErrorToNetError(base::File::Error file_error) {
  switch (file_error) {
    case base::File::FILE_OK:
      return net::OK;
    case base::File::FILE_ERROR_ACCESS_DENIED:
      return net::ERR_ACCESS_DENIED;
    case base::File::FILE_ERROR_INVALID_URL:
      return net::ERR_INVALID_URL;
    case base::File::FILE_ERROR_NOT_FOUND:
      return net::ERR_FILE_NOT_FOUND;
    default:
      return net::ERR_FAILED;
  }
}

}  // namespace net
