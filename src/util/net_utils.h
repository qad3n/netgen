#ifndef NET_UTILS_H
#define NET_UTILS_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/random.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>

#define HOSTNAME_MAX_LEN 25u
#define DHCP_ID_BYTES 6u
#define POLL_INTERVAL_MS 200u
#define DHCP_TIMEOUT_SEC 10u
#define FALLBACK_IFACE "wlan0"

#define RESET_MACHINE_ID 0
#define ENABLE_RANDOM_DHCP_ID 1

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define arrayCount(a) (sizeof(a) / sizeof *(a))

extern int ioctl_fd;

void closeSocket(void);
void openSocket(void);

int getMAC (const char *iface, uint8_t mac[6]);
int setMAC (const char *iface, const uint8_t mac[6]);
int ifaceState(const char *iface, int up);

void genMAC (uint8_t mac[6]);
void genHEX (char *dst, size_t n);
void genHost (char *dst);

void msleep(unsigned ms);

int runCMD (const char *cmd, char *buf, size_t len);
int getiFace(const char *iface, char *conn, size_t len);
int getIPV4 (const char *iface, char *buf, size_t len);
int getDHCPID (const char *iface, char *buf, size_t len);
int renewDHCP (const char *iface, const char *dhcpHex);

void formatMAC (const uint8_t m[6], char *out);
void status (const char *label, int ok);
#endif
