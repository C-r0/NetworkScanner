#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <string.h>

// VARIABLES
#define MAX_PORT 65535
#define THREAD_COUNT 1000

char ip[INET_ADDRSTRLEN];
int next_port = 1;
pthread_mutex_t port_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int port;
    const char *service;
} ServiceMap;

ServiceMap common_services[] = {
    {21, "FTP"},
    {22, "SSH"},
    {23, "Telnet"},
    {25, "SMTP"},
    {53, "DNS"},
    {80, "HTTP"},
    {443, "HTTPS"},
    {3306, "MySQL"},
    {4444, "Reverse Shell"},
    {5000, "Flask Python"},
    {5432, "PostgreSQL"},
    {7656, "I2P NODE"},
    {8080, "HTTP-Proxy"},
    {8118, "Privoxy"},
    {9050, "TOR"}
};

const char* get_service_name(int port) {
    int size = sizeof(common_services) / sizeof(ServiceMap);
    for (int i = 0; i < size; i++) {
        if (common_services[i].port == port) {
            return common_services[i].service;
        }
    }
    return "unknown";
}

void* connectTCP(void* arg) {
	int port;
	
	while (1) {
		pthread_mutex_lock(&port_lock);
		port = next_port;
	    next_port++;
	    pthread_mutex_unlock(&port_lock);
	
	    if (port > MAX_PORT) break;
	
	    int sock = socket(AF_INET, SOCK_STREAM, 0);
	    if (sock < 0) continue;
	
	    struct timeval timeout;
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 200000;
	    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	
	    struct sockaddr_in addr;
	    addr.sin_family = AF_INET;
	    addr.sin_port = htons(port);
	    addr.sin_addr.s_addr = inet_addr(ip);
	
	    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
	        const char *payload = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
	        send(sock, payload, strlen(payload), 0);
	        char buffer[256];
	        memset(buffer, 0, sizeof(buffer));
	        pthread_mutex_lock(&print_lock);
	        printf("[+] %d \n", port);
	        printf("- Possible Type: %s\n", get_service_name(port));
	        int received = recv(sock, buffer, sizeof(buffer), 0);
			if (received > 0) {
				if (strncmp(buffer, payload, strlen(payload)) == 0) {
					printf("- Banner: [Echo Detected]\n");
				} else {
					printf("- Banner:\n%s\n", buffer);
				}
			}
	        printf("----------------------\n");
	        pthread_mutex_unlock(&print_lock);
	    }
	
	    close(sock);
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Invalid Number of Arguments!\n");
		return 0;
	}
	
	printf("Network Scan Starting...\n");
	
	struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    printf("Resolving target: %s...\n", argv[1]);
    
    int status = getaddrinfo(argv[1], NULL, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "Error resolving target: %s\n", gai_strerror(status));
        return 1;
    }

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ip, sizeof(ip));
    
    freeaddrinfo(res);

	pthread_t threads[THREAD_COUNT];

	printf("IP: %s\n", ip);

	// CONECTING TCP
	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_create(&threads[i], NULL, connectTCP, NULL);
	}
	
	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}
	
	return 0;
}
