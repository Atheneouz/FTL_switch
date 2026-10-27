#pragma once

#include "android/HyperspaceHost.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Punto de entrada para Nintendo Switch */
int hs_switch_init(const hs_host_info *info, char *error, size_t error_size);

/* Gestión de errores del host */
void PAL_HostShowError(const char *title, const char *message);

#ifdef __cplusplus
}
#endif
