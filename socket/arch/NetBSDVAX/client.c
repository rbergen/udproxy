/*
 * client.c - Socket client for NetBSD VAX load testing
 * Sends panel data frames to server 30 times per second
 * NetBSD VAX specific implementation - reads from kernel panel symbol
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

/* External getopt declarations for compatibility */
extern char *optarg;
extern int optind, optopt;

/* Include common functions */
#define SERVER_PORT 4002
#include "../../common.c"

/* Timing method selection - define USE_USLEEP to use usleep() instead of select() */
#define USE_USLEEP 1

/* Panel notification mechanism */
#define PANEL_UPDATE_IDENT 0x12345  /* Unique identifier for our panel event */

/* Include panel state definitions */
#include "panel_state.h"

/* Fallback for systems without uintptr_t */
#ifndef uintptr_t
#define uintptr_t unsigned long
#endif

/* Function prototypes */
int open_kmem_and_find_panel(void **panel_addr);
int read_panel_from_kmem(int kmem_fd, void *panel_addr, struct vax_panel_state *panel);
void send_frames(int sockfd, struct sockaddr_in *server_addr, int kmem_fd, void *panel_addr);
void send_frames_with_notification(int sockfd, struct sockaddr_in *server_addr, int kmem_fd, void *panel_addr);
int setup_panel_notification(void);
void register_with_kernel(void);
void cleanup_panel_notification(void);

int main(int argc, char *argv[])
{
    char *server_ip = "127.0.0.1";
    int sockfd;
    int c;
    struct sockaddr_in server_addr;
    void *panel_addr;
    int kmem_fd;
    
    /* Parse command line arguments */
    while ((c = getopt(argc, argv, "s:h")) != -1) {
        switch (c) {
        case 's':
            server_ip = optarg;
            break;
        case 'h':
        case '?':
            usage(argv[0]);
            exit(1);
        }
    }
    
    printf("NetBSD VAX Panel Client with Kernel Notification Support\n");
    printf("Connecting to server at %s:%d via UDP\n", server_ip, SERVER_PORT);
    
    /* Set up kernel notification mechanism first */
    if (setup_panel_notification() < 0) {
        printf("Warning: Kernel notification setup failed, falling back to polling mode\n");
    }
    
    /* Register our process with the kernel for panel notifications */
    register_with_kernel();
    
    /* Open /dev/kmem and find panel symbol */
    kmem_fd = open_kmem_and_find_panel(&panel_addr);
    if (kmem_fd < 0) {
        fprintf(stderr, "Failed to open /dev/kmem or find panel symbol\n");
        cleanup_panel_notification();
        exit(1);
    }
    
    printf("Panel symbol found at address %p\n", panel_addr);
    
    /* Create UDP socket and set up server address */
    sockfd = create_udp_socket(server_ip, &server_addr);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create UDP socket\n");
        close(kmem_fd);
        exit(1);
    }
    
    /* Configure socket for immediate transmission - disable batching */
    /* Disable socket send buffering for immediate transmission */
    int sndbuf = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
        /* If we can't disable buffering completely, try minimal buffer */
        sndbuf = 1024;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    }
    
    /* Set low latency socket option if available */
    int lowdelay = 1;
    setsockopt(sockfd, IPPROTO_IP, IP_TOS, &lowdelay, sizeof(lowdelay));
    
    // Verbose output removed for background operation
    
    /* Send frames continuously with kernel notification */
    send_frames_with_notification(sockfd, &server_addr, kmem_fd, panel_addr);
    
    close(sockfd);
    close(kmem_fd);
    cleanup_panel_notification();
    return 0;
}

int open_kmem_and_find_panel(void **panel_addr)
{
    FILE *fp;
    char line[256];
    char symbol[64];
    uintptr_t addr;
    char type;
    int kmem_fd;
    int found_panel_symbols = 0;
    int found_any_symbols = 0;
    
    /* First, test if nm works at all */
    printf("Testing nm command on /netbsd...\n");
    printf("Kernel info: ");
    fflush(stdout);
    system("uname -a");
    printf("Kernel file: ");
    fflush(stdout);
    system("ls -la /netbsd");
    
    fp = popen("nm /netbsd | head -5", "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot run nm on /netbsd\n");
        return -1;
    }
    
    printf("First 5 symbols from nm:\n");
    while (fgets(line, sizeof(line), fp) && found_any_symbols < 5) {
        printf("  %s", line);
        found_any_symbols++;
    }
    pclose(fp);
    
    if (found_any_symbols == 0) {
        fprintf(stderr, "nm command returned no output - kernel symbol table may not be available\n");
        return -1;
    }
    
    /* Now search specifically for panel */
    printf("\nSearching for panel symbol...\n");
    fp = popen("nm /netbsd | grep panel", "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot run nm on /netbsd to find panel symbol\n");
        return -1;
    }
    
    *panel_addr = NULL;
    while (fgets(line, sizeof(line), fp)) {
        found_panel_symbols++;
        printf("Found panel-related symbol: %s", line);
        
        /* Try both octal and hex formats */
        if (sscanf(line, "%lo %c %s", &addr, &type, symbol) == 3 ||
            sscanf(line, "%lx %c %s", &addr, &type, symbol) == 3) {
            printf("  -> Parsed: addr=0x%lx, type='%c', symbol='%s'\n", 
                   (unsigned long)addr, type, symbol);
            
            if (strcmp(symbol, "panel") == 0 || strcmp(symbol, "_panel") == 0) {
                printf("  -> MATCH! Using symbol '%s' at address 0x%lx\n", 
                       symbol, (unsigned long)addr);
                *panel_addr = (void*)addr;
                break;
            }
        } else {
            printf("  -> Failed to parse line\n");
        }
    }
    pclose(fp);
    
    printf("Total panel-related symbols found: %d\n", found_panel_symbols);
    
    if (*panel_addr == NULL) {
        if (found_panel_symbols == 0) {
            fprintf(stderr, "No panel symbols found in kernel - the kernel may not have panel support compiled in\n");
            fprintf(stderr, "Try checking: nm /netbsd | grep panel\n");
        } else {
            fprintf(stderr, "Panel symbol not found in kernel symbol table\n");
            fprintf(stderr, "Looking specifically for symbol named 'panel' or '_panel'\n");
        }
        return -1;
    }
    
    /* Open /dev/kmem */
    kmem_fd = open("/dev/kmem", O_RDONLY);
    if (kmem_fd < 0) {
        perror("open /dev/kmem");
        /* On NetBSD, try /dev/mem as fallback */
        kmem_fd = open("/dev/mem", O_RDONLY);
        if (kmem_fd < 0) {
            perror("open /dev/mem");
            return -1;
        }
        printf("Using /dev/mem instead of /dev/kmem\n");
    }
    
    return kmem_fd;
}

int read_panel_from_kmem(int kmem_fd, void *panel_addr, struct vax_panel_state *panel)
{
    off_t offset = (off_t)(uintptr_t)panel_addr;
    unsigned char raw_data[128];  /* Buffer for raw kernel panel data */
    size_t bytes_to_read = sizeof(raw_data);
    struct timeval tv;
    
    /* Seek to panel address in kernel memory */
    if (lseek(kmem_fd, offset, SEEK_SET) < 0) {
        perror("lseek");
        return -1;
    }
    
    /* Read raw panel data from kernel */
    if (read(kmem_fd, raw_data, bytes_to_read) < 0) {
        perror("read");
        return -1;
    }

    
    /* Get current time for dynamic data */
    gettimeofday(&tv, NULL);
    
    /* Convert raw kernel data to VAX panel format */
    /* Since we're on the same architecture, we can directly extract values */
    
    /* Extract address and data directly from raw memory */
    panel->ps_address = *(uint32_t*)&raw_data[0];
    panel->ps_data = *(uint32_t*)&raw_data[4];
    
    return 0;
}

void send_frames_with_notification(int sockfd, struct sockaddr_in *server_addr, int kmem_fd, void *panel_addr)
{
    struct vax_panel_state panel;
    struct vax_panel_packet packet;
    int frame_count = 0;
    struct timeval start_time, current_time;
    double elapsed_seconds, actual_fps;
    
    /* Get start time for FPS measurement */
    gettimeofday(&start_time, NULL);
    
    printf("Starting PURE EVENT-DRIVEN panel monitoring (no polling)...\n");
    printf("Waiting for kernel signals at system interrupt rate (typically 100 Hz on NetBSD VAX)...\n");
    
    while (1) {
        /* PURE EVENT-DRIVEN: Only wait for kernel notification, no fallback */
        if (wait_for_panel_notification() <= 0) {
            /* If we get here, something is wrong with the event system */
            /* Just continue waiting - NO POLLING */
            continue;
        }
        
        /* Kernel signaled us - send frame immediately */
        
        /* Read panel structure from kernel memory */
        if (read_panel_from_kmem(kmem_fd, panel_addr, &panel) < 0) {
            fprintf(stderr, "Failed to read panel data from kernel\n");
            break;
        }
        
        /* Populate packet structure */
        packet.header.pp_byte_count = sizeof(struct vax_panel_state);
        packet.header.pp_byte_flags = PANEL_VAX;  /* Set panel type flag */
        packet.panel_state = panel;
        
        /* Send panel packet via UDP with immediate transmission */
        if (sendto(sockfd, &packet, sizeof(packet), MSG_DONTWAIT,
                   (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
            fprintf(stderr, "sendto failed after %d packets: ", frame_count);
            perror("");
            if (errno == ENETUNREACH) {
                fprintf(stderr, "Network unreachable - check server IP address\n");
            } else if (errno == EHOSTUNREACH) {
                fprintf(stderr, "Host unreachable - server may be down\n");
            } else if (errno == ECONNREFUSED) {
                fprintf(stderr, "Connection refused - server not listening on port %d\n", SERVER_PORT);
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                /* Non-blocking send would block, continue anyway */
                fprintf(stderr, "Warning: UDP send would block (frame %d)\n", frame_count);
            }
            else {
                break;
            }
        }
        
        frame_count++;
        
        /* Measure and report actual FPS every 60 frames */
        /* FPS reporting removed for background operation */
        
        /* First packet success notification removed for background operation */
    }
}

void send_frames(int sockfd, struct sockaddr_in *server_addr, int kmem_fd, void *panel_addr)
{
    struct vax_panel_state panel;
    struct vax_panel_packet packet;
    int frame_count = 0;
    struct timeval start_time, current_time;
    double elapsed_seconds, actual_fps;
    
    /* Get start time for FPS measurement */
    gettimeofday(&start_time, NULL);
    
    while (1) {
        /* Read panel structure from kernel memory */
        if (read_panel_from_kmem(kmem_fd, panel_addr, &panel) < 0) {
            fprintf(stderr, "Failed to read panel data from kernel\n");
            break;
        }
        
        /* Populate packet structure */
        packet.header.pp_byte_count = sizeof(struct vax_panel_state);
        packet.header.pp_byte_flags = PANEL_VAX;  /* Set panel type flag */
        packet.panel_state = panel;
        
        /* Send panel packet via UDP with immediate transmission */
        if (sendto(sockfd, &packet, sizeof(packet), MSG_DONTWAIT,
                   (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
            fprintf(stderr, "sendto failed after %d packets: ", frame_count);
            perror("");
            if (errno == ENETUNREACH) {
                fprintf(stderr, "Network unreachable - check server IP address\n");
            } else if (errno == EHOSTUNREACH) {
                fprintf(stderr, "Host unreachable - server may be down\n");
            } else if (errno == ECONNREFUSED) {
                fprintf(stderr, "Connection refused - server not listening on port %d\n", SERVER_PORT);
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                /* Non-blocking send would block, continue anyway */
                fprintf(stderr, "Warning: UDP send would block (frame %d)\n", frame_count);
            }
            else {
                break;
            }
        }
        
        frame_count++;
        
        /* FPS reporting removed for background operation */
        
        /* First packet success notification removed for background operation */
        
        /* Wait for next frame time */
        /* Testing different timing methods to resolve 2x timing issue on NetBSD VAX */
#ifdef USE_USLEEP
        usleep(USEC_PER_FRAME);
#else
        precise_delay(USEC_PER_FRAME);
#endif
    }
}

/* Kernel notification system implementation */
static int kq = -1;
static int notification_available = 0;

int setup_panel_notification(void)
{
    /* Create kqueue for event handling */
    kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        return -1;
    }
    
    /* Set up EVFILT_USER filter for panel notifications */
    struct kevent kev[2];
    EV_SET(&kev[0], PANEL_UPDATE_IDENT, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, NULL);
    
    /* Also set up EVFILT_SIGNAL for SIGUSR1 as fallback */
    signal(SIGUSR1, SIG_IGN);  /* We'll handle it via kqueue */
    EV_SET(&kev[1], SIGUSR1, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
    
    if (kevent(kq, kev, 2, NULL, 0, NULL) == -1) {
        perror("kevent setup");
        close(kq);
        kq = -1;
        return -1;
    }
    
    notification_available = 1;
    printf("Kernel notification system initialized with kqueue and signal fallback\n");
    return 0;
}

void register_with_kernel(void)
{
    /* Register our PID with the kernel so it knows to send us notifications */
    pid_t our_pid = getpid();
    char sysctl_cmd[256];
    
    printf("Panel client PID %d registering for kernel notifications\n", our_pid);
    
    /* Use sysctl to register our PID with the kernel */
    snprintf(sysctl_cmd, sizeof(sysctl_cmd), "sysctl -w hw.panel.client_pid=%d", our_pid);
    int result = system(sysctl_cmd);
    if (result == 0) {
        printf("Successfully registered with kernel notification system\n");
    } else {
        printf("Warning: Could not register with kernel (sysctl failed)\n");
        printf("Panel notifications may not work - falling back to polling\n");
    }
    
    /* Also enable panel notifications */
    result = system("sysctl -w hw.panel.enabled=1");
    if (result != 0) {
        printf("Warning: Could not enable panel notifications\n");
    }
}

int wait_for_panel_notification(void)
{
    if (!notification_available || kq == -1) {
        printf("ERROR: Notification system not available, cannot proceed\n");
        exit(1);  /* Pure event-driven mode requires working notifications */
    }
    
    struct kevent kev;
    /* NO TIMEOUT - block indefinitely until kernel sends signal */
    int n = kevent(kq, NULL, 0, &kev, 1, NULL);
    
    if (n == -1) {
        if (errno == EINTR) {
            /* Interrupted by signal - this might be our SIGUSR1 */
            return 1;
        }
        perror("kevent wait failed");
        exit(1);  /* Pure event-driven mode cannot tolerate kevent failures */
    }
    
    if (n == 0) {
        /* This should never happen with NULL timeout */
        printf("ERROR: kevent returned 0 with NULL timeout\n");
        return 0;
    }
    
    if (kev.filter == EVFILT_SIGNAL && kev.ident == SIGUSR1) {
        /* Received SIGUSR1 signal from kernel - this is what we want */
        return 1;
    } else if (kev.filter == EVFILT_USER && kev.ident == PANEL_UPDATE_IDENT) {
        /* Received EVFILT_USER notification from kernel */
        return 1;
    }
    
    printf("DEBUG: Received unexpected event: filter=%d, ident=%lu\n", kev.filter, kev.ident);
    return 0;  /* Wait for next event */
}

void cleanup_panel_notification(void)
{
    if (kq != -1) {
        close(kq);
        kq = -1;
    }
    notification_available = 0;
    printf("Kernel notification system cleaned up\n");
}
