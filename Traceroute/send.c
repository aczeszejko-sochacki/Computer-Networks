// Aleksander Czeszejko-Sochacki 297438

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/ip_icmp.h>
#include <assert.h>
#include "packet_const.h"

/**********************
 * Icmphdr computations
 * ********************/

u_int16_t compute_icmp_checksum(const void *buff, int length)
{
  u_int32_t sum;
  const u_int16_t *ptr = buff;
  assert(length % 2 == 0);
  for (sum = 0; length > 0; length -= 2)
    sum += *ptr++;
  sum = (sum >> 16) + (sum & 0xffff);
  return (u_int16_t)(~(sum + (sum >> 16)));
}

struct icmphdr init_icmphdr(int seq)
{
  struct icmphdr icmp_header;

  icmp_header.type = ICMP_ECHO;
  icmp_header.code = 0;
  icmp_header.un.echo.id = getpid();
  icmp_header.un.echo.sequence = seq;
  icmp_header.checksum = 0;
  icmp_header.checksum = compute_icmp_checksum(
    (u_int16_t *)&icmp_header,
    sizeof(icmp_header)
  );

  return icmp_header;
}

/***************************
 * Sending ICMP ECHO message
 * *************************/

int send_successful(ssize_t bytes_sent)
{
  if (bytes_sent < 0) {
    return 0;
  }

  return 1;
}

int send_packets(int sockfd, struct sockaddr_in recipient, int ttl, clock_t *start_times)
{

  // Try to send exactly 3 packets
  for (int i = 0; i < N_PACKETS; i++) {

    // Unique seq number associated with ttl
    int seq = N_PACKETS*ttl + i;

    // Configure sending
    struct icmphdr icmp_header = init_icmphdr(seq);

    // Init the clock
    start_times[seq] = clock();

    // Send an echo request
    ssize_t bytes_sent = sendto(
        sockfd,
        &icmp_header,
        sizeof(icmp_header),
        0,
        (struct sockaddr *)&recipient,
        sizeof(recipient)
    );

    if (!send_successful(bytes_sent)) return EXIT_FAILURE;
  }
}