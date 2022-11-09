#ifndef PLUGIN_HELPER_H
#define PLUGIN_HELPER_H

#include <nvtx3/nvToolsExt.h>

#define FUNC_ENTER(X)                                                                                        \
    do {                                                                                                     \
      nvtxRangePush(__FUNCTION__);                 \
    } while (0)

#define FUNC_LEAVE(ret_value)                                                                                \
    do {                                                                                                     \
      nvtxRangePop();                              \
        return (ret_value);                                                                                  \
    } while (0)

#define FUNC_LEAVE_VOID                                                                                      \
    do {                                                                                                     \
      nvtxRangePop();                              \
        return;                                                                                              \
    } while (0)

#endif /* PLUGIN_HELPER_H */

