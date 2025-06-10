#include "net_utils.h"

int ioctl_fd = -1;

static inline int fillRandom(void *buf, size_t len) { return getrandom(buf, len, 0) == (ssize_t)len ? 0 : -1; }

void formatMAC(const uint8_t m[6], char *out) { snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]); }
void status(const char *label, int ok) { printf("%-9s : %s\n", label, ok ? "SUCCESS" : "FAIL"); }

static FILE *popenChecked(const char *cmd, const char *mode)
{
	FILE *fp = popen(cmd, mode);
	if (!fp)
		die("popen");
		
	return fp;
}

void openSocket(void)
{
	if (ioctl_fd != -1) 
		return;
		
	ioctl_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (ioctl_fd == -1)
		die("socket");
}

void closeSocket(void)
{
	if (ioctl_fd != -1) 
	{
		close(ioctl_fd);
		ioctl_fd = -1;
	}
}

int getMAC(const char *iface, uint8_t mac[6])
{
	struct ifreq ifr = { 0 };
	strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
	
	if (ioctl(ioctl_fd, SIOCGIFHWADDR, &ifr) == -1)
		return -1;
		
	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
	return 0;
}

int setMAC(const char *iface, const uint8_t mac[6])
{
	struct ifreq ifr = {0};
	strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
	
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(ifr.ifr_hwaddr.sa_data, mac, 6);
	return ioctl(ioctl_fd, SIOCSIFHWADDR, &ifr);
}

int ifaceState(const char *iface, int up)
{
	struct ifreq ifr = {0};
	strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
	
	if (ioctl(ioctl_fd, SIOCGIFFLAGS, &ifr) == -1)
		return -1;
		
	if (up)
		ifr.ifr_flags |=  IFF_UP;
	else
		ifr.ifr_flags &= ~IFF_UP;
	
	return ioctl(ioctl_fd, SIOCSIFFLAGS, &ifr);
}

void genMAC(uint8_t mac[6])
{
	if (fillRandom(mac, 6) != 0) 
		die("getrandom");
	
	mac[0] &= 0xFE; // unicast
	mac[0] |= 0x02; // local
}

void genHEX(char *dst, size_t n)
{
	uint8_t tmp[n];
	
	if (fillRandom(tmp, n) != 0) die("getrandom");
	
	for (size_t i = 0; i < n; ++i)
		sprintf(dst + i * 2, "%02x", tmp[i]);
		
	dst[n * 2] = '\0';
}

void genHost(char *h)
{
	static const char alpha[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	
	uint8_t r[HOSTNAME_MAX_LEN];
	if (fillRandom(r, sizeof r) != 0) 
		die("getrandom");
		
	size_t len = 5 + (r[0] % (HOSTNAME_MAX_LEN - 4));
	
	for (size_t i = 0; i < len; ++i)
		h[i] = alpha[r[i + 1] % (arrayCount(alpha) - 1)];
		
	h[len] = '\0';
}

void msleep(unsigned ms)
{
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000000UL };
	nanosleep(&ts, NULL);
}

int runCMD(const char *cmd, char *buf, size_t len)
{
	FILE *fp = popenChecked(cmd, "r");
	
	if (!fgets(buf, (int)len, fp))
		buf[0] = '\0';
		
	size_t l = strnlen(buf, len);
	if (l && buf[l - 1] == '\n')
		buf[--l] = '\0';
		
	pclose(fp);
	return *buf ? 0 : 1;
}

int getiFace(const char *iface, char *conn, size_t len)
{
	FILE *fp = popenChecked("nmcli -t -f NAME,DEVICE c show --active", "r");
	
	while (fgets(conn, (int)len, fp)) 
	{
		char *sep = strchr(conn, ':');
		if (sep && strcmp(sep + 1, iface) == 0) 
		{
			*sep = '\0';
			break;
		}
		conn[0] = '\0';
	}
	pclose(fp);
	return *conn ? 0 : -1;
}

int getIPV4(const char *iface, char *buf, size_t len)
{
	char cmd[128];
	snprintf(cmd, sizeof cmd, "ip -4 -o addr show dev %s | awk '{print $4}' | cut -d/ -f1", iface);
	return runCMD(cmd, buf, len);
}

int getDHCPID(const char *iface, char *buf, size_t len)
{
	char conn[128] = "";
	if (getiFace(iface, conn, sizeof conn) != 0)
		return -1;
	
	char cmd[256];
	snprintf(cmd, sizeof cmd, "nmcli -g ipv4.dhcp-client-id connection show '%s'", conn);
	
	return runCMD(cmd, buf, len);
}

int renewDHCP(const char *iface, const char *dhcpHex)
{
	char conn[128] = "";
	if (getiFace(iface, conn, sizeof conn) != 0)
		return -1;
		
	char cmd[512];
	
	if (ENABLE_RANDOM_DHCP_ID && dhcpHex && *dhcpHex) 
	{
		snprintf(cmd, sizeof cmd, "nmcli connection modify '%s' ipv4.dhcp-client-id %s", conn, dhcpHex);
		system(cmd);
	}
	
	snprintf(cmd, sizeof cmd, "nmcli connection down '%s'", conn);
	system(cmd);
	
	snprintf(cmd, sizeof cmd, "nmcli connection up  '%s'", conn);
	system(cmd);
	
	return 0;
}
