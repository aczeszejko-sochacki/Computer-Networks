// Aleksander Czeszejko-Sochacki 297438

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "packet_const.h"

/*************************
 * External functions
 * ***********************/

void send_packets(int sockfd, struct sockaddr_in recipient, int ttl, clock_t *start_times);

int receive_responses(int sockfd, int ttl, char *ip_dest, clock_t *start_times);

/**************************
 * Handling errors
 * ************************/

int socket_correctness(int sockfd)
{
  if (sockfd < 0) {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    return 0;
  }

  return 1;
}

int arg_correctness(int argc, char **argv)
{
  if (argc != 2) {
    puts("Traceroute needs exactly one argument");
    return 0;
  }

  char *ip_addr = argv[1];
  struct sockaddr_in sa;
  int is_ip = inet_pton(AF_INET, ip_addr, &(sa.sin_addr));

  if (!is_ip) {
    puts("Traceroute needs correct ip address as an argument");
    return 0;
  }

  return 1;
}

struct sockaddr_in configure_recipient(struct sockaddr_in *recipient, char *ip_addr)
{
  bzero(recipient, sizeof(recipient));
  recipient->sin_family = AF_INET;
  inet_pton(AF_INET, ip_addr, &recipient->sin_addr);
}

/***********************
 * Main
 * *********************/

int main(int argc, char **argv) {

  // Check args
  if (!arg_correctness(argc, argv)) return EXIT_FAILURE;

  // Extract ip
  char *ip_dest = argv[1];

  // Create a raw socket
  int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

  // Check if the socket is constructed correctly
  if (!socket_correctness(sockfd)) return EXIT_FAILURE;

  // Configure the recipient
  struct sockaddr_in recipient;
  configure_recipient(&recipient, ip_dest);

  // An array of start times
  clock_t start_times[(MAX_TTL+1) * (N_PACKETS+1)];

  // End as soon as dest_reached is 1
  int dest_reached = 0;

  for (int ttl = 1; ttl <= MAX_TTL && !dest_reached; ttl++) {

    printf("%d ", ttl);

    // Update TTL
    setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    // Send 3 packets, write their start times to start_times[ttl]
    send_packets(sockfd, recipient, ttl, start_times);

    // Receive at most 3 up-to-date packets
    dest_reached = receive_responses(sockfd, ttl, ip_dest, start_times);
  }

  return EXIT_SUCCESS;
}