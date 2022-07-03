#ifdef UVCLS_AS_LIB
#    include "emitter.h"
#endif

#include "config.h"

namespace uvcls {

UVCLS_INLINE int ErrorEvent::translate(int sys) noexcept {
    return uv_translate_sys_error(sys);
}

UVCLS_INLINE const char *ErrorEvent::what() const noexcept {
    return uv_strerror(ec);
}

UVCLS_INLINE const char *ErrorEvent::name() const noexcept {
    return uv_err_name(ec);
}

UVCLS_INLINE int ErrorEvent::code() const noexcept {
    return ec;
}

UVCLS_INLINE ErrorEvent::operator bool() const noexcept {
    return ec < 0;
}

} // namespace uvw
