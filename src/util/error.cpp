/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/error.hpp"

#include <cerrno>
#include <sstream>

namespace util {

error::error(error_code err, std::string err_msg)
  : err_(err), err_msg_(std::move(err_msg)) {
  // nop
}

error::error(error_code err) : err_(err) {
  // nop
}

// -- boolean operators ------------------------------------------------------

bool error::is_error() const {
  return err_ != no_error;
}

error::operator bool() const {
  return is_error();
}

bool error::operator==(const error& other) {
  return err_ == other.err_;
}

bool error::operator!=(const error& other) {
  return err_ != other.err_;
}

std::string to_string(const error& err) {
  std::stringstream ss;
  ss << err;
  return ss.str();
}

std::string last_error_as_string() {
  // This function is an abomination, I know. However, sterror is not
  // thread-safe and the alternatives strerrorlen_s/strerror_s are not available
  // -- Hence this is required to make the function thread-safe.
  // Maybe a better solution can be found in the future..
  switch (errno) {
    case EPERM:
      return "Operation not permitted";
    case ENOENT:
      return "No such file or directory";
    case ESRCH:
      return "No such process";
    case EINTR:
      return "Interrupted system call";
    case EIO:
      return "Input/output error";
    case ENXIO:
      return "No such device or address";
    case E2BIG:
      return "Argument list too long";
    case ENOEXEC:
      return "Exec format error";
    case EBADF:
      return "Bad file descriptor";
    case ECHILD:
      return "No child processes";
    case EAGAIN:
      return "Resource temporarily unavailable";
    case ENOMEM:
      return "Cannot allocate memory";
    case EACCES:
      return "Permission denied";
    case EFAULT:
      return "Bad address";
    case ENOTBLK:
      return "Block device required";
    case EBUSY:
      return "Device or resource busy";
    case EEXIST:
      return "File exists";
    case EXDEV:
      return "Invalid cross-device link";
    case ENODEV:
      return "No such device";
    case ENOTDIR:
      return "Not a directory";
    case EISDIR:
      return "Is a directory";
    case EINVAL:
      return "Invalid argument";
    case ENFILE:
      return "Too many open files in system";
    case EMFILE:
      return "Too many open files";
    case ENOTTY:
      return "Inappropriate ioctl for device";
    case ETXTBSY:
      return "Text file busy";
    case EFBIG:
      return "File too large";
    case ENOSPC:
      return "No space left on device";
    case ESPIPE:
      return "Illegal seek";
    case EROFS:
      return "Read-only file system";
    case EMLINK:
      return "Too many links";
    case EPIPE:
      return "Broken pipe";
    case EDOM:
      return "Numerical argument out of domain";
    case ERANGE:
      return "Numerical result out of range";
    case EDEADLK:
      return "Resource deadlock avoided";
    case ENAMETOOLONG:
      return "File name too long";
    case ENOLCK:
      return "No locks available";
    case ENOSYS:
      return "Function not implemented";
    case ENOTEMPTY:
      return "Directory not empty";
    case ELOOP:
      return "Too many levels of symbolic links";
    case ENOMSG:
      return "No message of desired type";
    case EIDRM:
      return "Identifier removed";
    case ENOSTR:
      return "Device not a stream";
    case ENODATA:
      return "No data available";
    case ETIME:
      return "Timer expired";
    case ENOSR:
      return "Out of streams resources";
    case EREMOTE:
      return "Object is remote";
    case ENOLINK:
      return "Link has been severed";
    case EPROTO:
      return "Protocol error";
    case EMULTIHOP:
      return "Multihop attempted";
    case EBADMSG:
      return "Bad message";
    case EOVERFLOW:
      return "Value too large for defined data type";
    case EILSEQ:
      return "Invalid or incomplete multibyte or wide character";
    case EUSERS:
      return "Too many users";
    case ENOTSOCK:
      return "Socket operation on non-socket";
    case EDESTADDRREQ:
      return "Destination address required";
    case EMSGSIZE:
      return "Message too long";
    case EPROTOTYPE:
      return "Protocol wrong type for socket";
    case ENOPROTOOPT:
      return "Protocol not available";
    case EPROTONOSUPPORT:
      return "Protocol not supported";
    case ESOCKTNOSUPPORT:
      return "Socket type not supported";
    case ENOTSUP:
      return "Operation not supported";
    case EPFNOSUPPORT:
      return "Protocol family not supported";
    case EAFNOSUPPORT:
      return "Address family not supported by protocol";
    case EADDRINUSE:
      return "Address already in use";
    case EADDRNOTAVAIL:
      return "Cannot assign requested address";
    case ENETDOWN:
      return "Network is down";
    case ENETUNREACH:
      return "Network is unreachable";
    case ENETRESET:
      return "Network dropped connection on reset";
    case ECONNABORTED:
      return "Software caused connection abort";
    case ECONNRESET:
      return "Connection reset by peer";
    case ENOBUFS:
      return "No buffer space available";
    case EISCONN:
      return "Transport endpoint is already connected";
    case ENOTCONN:
      return "Transport endpoint is not connected";
    case ESHUTDOWN:
      return "Cannot send after transport endpoint shutdown";
    case ETOOMANYREFS:
      return "Too many references: cannot splice";
    case ETIMEDOUT:
      return "Connection timed out";
    case ECONNREFUSED:
      return "Connection refused";
    case EHOSTDOWN:
      return "Host is down";
    case EHOSTUNREACH:
      return "No route to host";
    case EALREADY:
      return "Operation already in progress";
    case EINPROGRESS:
      return "Operation now in progress";
    case ESTALE:
      return "Stale file handle";
    case EDQUOT:
      return "Disk quota exceeded";
    case ECANCELED:
      return "Operation canceled";
    case EOWNERDEAD:
      return "Owner died";
    case ENOTRECOVERABLE:
      return "State not recoverable";
    default:
      return "unknown errno";
  }
}

} // namespace util
