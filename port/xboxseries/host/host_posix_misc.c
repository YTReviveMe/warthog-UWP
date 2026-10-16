#include "posix.h"
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>

void posix_describe_address(void *address, char *buffer, posix_ulong size) {
    if (size)
        snprintf(buffer, size, "%p", address);
}

int posix_upnp_forward_udp(unsigned short port, unsigned short preferred_port, posix_ulong *address,
                           unsigned short *external_port, char *error, int error_size) {
    (void)port;
    (void)preferred_port;
    (void)address;
    (void)external_port;
    if (error_size)
        snprintf(error, error_size, "UPnP unavailable on Xbox UWP");
    return 0;
}
void posix_upnp_stop_forwarding_udp(unsigned short port) { (void)port; }
int posix_command_line_argument(int index, char *buffer, posix_ulong size) {
    if (index || !size)
        return 0;
    snprintf(buffer, size, "halo");
    return 1;
}
posix_ulong posix_process_id(void) { return GetCurrentProcessId(); }
int posix_register_url_scheme(const char *scheme, const char *description) {
    (void)scheme;
    (void)description;
    return 0;
}
int posix_user_secret(unsigned char *secret, int size) {
    return BCryptGenRandom(NULL, secret, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
}
int posix_discord_connect(void) { return -1; }
int posix_discord_write(int handle, const void *buffer, int length) {
    (void)handle;
    (void)buffer;
    (void)length;
    return -1;
}
int posix_discord_read(int handle, void *buffer, int length) {
    (void)handle;
    (void)buffer;
    (void)length;
    return -1;
}
void posix_discord_close(int handle) { (void)handle; }
