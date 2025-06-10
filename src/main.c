#include "util/net_utils.h"

int main(void)
{
	if (geteuid() != 0) 
	{
		char exe[PATH_MAX];
		if (!realpath("/proc/self/exe", exe))
			die("realpath");
			
		execlp("sudo", "sudo", exe, NULL);
		die("sudo");
	}
	
	openSocket();
	
	const char *iFace = FALLBACK_IFACE;
		
	uint8_t oldMAC[6];
	char oldHost[HOST_NAME_MAX + 1];
	char oldIP[64] = "";
	char oldDHCP[DHCP_ID_BYTES * 2 + 1] = "";
	
	getMAC(iFace, oldMAC);
	gethostname(oldHost, sizeof oldHost);
	getIPV4(iFace, oldIP, sizeof oldIP);
	getDHCPID(iFace, oldDHCP, sizeof oldDHCP);
	
	uint8_t newMAC[6];
	char newHost[HOSTNAME_MAX_LEN + 1];
	char DHCPHEX[DHCP_ID_BYTES * 2 + 1] = "";
	
	genMAC(newMAC);
	genHost(newHost);
	
	if (ENABLE_RANDOM_DHCP_ID)
		genHEX(DHCPHEX, DHCP_ID_BYTES);
		
	char macStr[18];
	
	printf("Interface : %s\n", iFace);
	
	formatMAC(oldMAC, macStr);
	
	printf("Old MAC : %s\n", macStr);
	printf("Old Host : %s\nOld IPv4 : %s\nOld DHCP : %s\n\n",
		*oldHost ? oldHost : "(none)",
		*oldIP ? oldIP : "(none)",
		*oldDHCP ? oldDHCP : "(none)");
		
	formatMAC(newMAC, macStr);
	
	printf("New MAC : %s\n", macStr);
	printf("New Host : %s\n", newHost);
	
	if (ENABLE_RANDOM_DHCP_ID)
		printf("New DHCP : %s\n", DHCPHEX);
		
	printf("\nProceed (Y/N)? ");
	
	int ch = getchar();
	if (ch != 'y' && ch != 'Y') 
	{
		closeSocket();
		return 0;
	}
	
	iFaceState(iFace, 0);
	if (setMAC(iFace, newMAC) != 0) 
		die("setMacAddress");
	iFaceState(iFace, 1);
	
	if (sethostname(newHost, strlen(newHost)) != 0)
		perror("sethostname");
		
	char cmd[256];
	snprintf(cmd, sizeof cmd, "hostnamectl set-hostname %s", newHost);
	system(cmd);
	
	if (RESET_MACHINE_ID)
		system("rm -f /etc/machine-id && systemd-machine-id-setup >/dev/null");
		
	renewDHCP(iFace, DHCPHEX);
	
	uint8_t curMAC[6];
	char curHost[HOST_NAME_MAX + 1];
	char curIP[64] = "";
	char curDHCP[DHCP_ID_BYTES * 2 + 1] = "";
	
	unsigned waited = 0;
	while (waited < DHCP_TIMEOUT_SEC * 1000)
	{
		if (getIPV4(iFace, curIP, sizeof curIP) == 0 && strcmp(curIP, oldIP) != 0)
			break;
			
		msleep(POLL_INTERVAL_MS);
		waited += POLL_INTERVAL_MS;
	}
	
	getMAC(iFace, curMAC);
	gethostname(curHost, sizeof curHost); // unistd posix call
	getDHCPID(iFace, curDHCP, sizeof curDHCP);
	
	puts("\nVerification Results");
	status("MAC", !memcmp(curMAC, newMAC, 6));
	status("Host", !strcmp(curHost, newHost));
	status("IPv4", strcmp(curIP, oldIP) != 0 && *curIP);
	status("DHCP", !*oldDHCP && !*curDHCP ? 1 : ENABLE_RANDOM_DHCP_ID ? !strcmp(curDHCP, DHCPHEX) : !strcmp(curDHCP, oldDHCP));
	
	printf("Current IPv4 : %s\n", *curIP ? curIP : "(none)");
	printf("Current DHCP : %s\n", *curDHCP? curDHCP: "(none)");
	
	memset(newMAC,  0, sizeof newMAC);
	memset(DHCPHEX, 0, sizeof DHCPHEX);
	
	closeSocket();
	return EXIT_SUCCESS;
}
