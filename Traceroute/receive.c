// Aleksander Czeszejko-Sochacki 297438

#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <netinet/ip_icmp.h>
#include "packet_const.h"

/******************************
 * Configuration and correctess
 * ****************************/

int packet_recv_correctness(ssize_t packet_len)
{
  if (packet_len < 0) {
    fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
    return 0;
  }

  return 1;
}

fd_set configure_descriptors(int sockfd)
{
  fd_set descriptors;
  FD_ZERO(&descriptors);
  FD_SET(sockfd, &descriptors);

  return descriptors;
}

/*******************************
 * Header informations selectors
 * *****************************/

void extract_ip_from_hdr(struct sockaddr_in sender, u_int8_t *buffer, ssize_t packet_len, char *sender_ip_str)
{
  inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, IP_LENGTH * sizeof(char));
}

void extract_sender_id_and_seq(uint8_t *buffer, int *sender_seq, int *sender_id) {

  // Extract icmp header
  struct iphdr *ip_header = (struct iphdr *) buffer;
  u_int8_t *icmp_packet = buffer + 4 * ip_header->ihl;
  struct icmphdr *icmp_header = (struct icmphdr *) icmp_packet;

  struct icmphdr *icmphdr_echoreply;

  // Below representations differs from each other, need to handle both of them
  if (icmp_header->type == ICMP_ECHOREPLY) {
    icmphdr_echoreply = icmp_header;
  }
  if (icmp_header->type == ICMP_TIME_EXCEEDED) {
    struct iphdr *orig_iphdr = (struct iphdr *)((uint8_t *)icmp_header + 8);
    icmphdr_echoreply = (struct icmphdr *)((uint8_t *)orig_iphdr + 4 * orig_iphdr->ihl);
  }

  *sender_seq = icmphdr_echoreply->un.echo.sequence;
  *sender_id = icmphdr_echoreply->un.echo.id;
}

/********************************
 * Response processing functions
 * ******************************/

int print_unique_correct_ips(char correct_resp_ips[][20]) {
  char tmp[20];

  if (strlen(correct_resp_ips[0]) == 0 &&
      strlen(correct_resp_ips[1]) == 0 &&
    	strlen(correct_resp_ips[2]) == 0) return 0;

  printf("%s ", correct_resp_ips[0]);
  if (!strcmp(correct_resp_ips[0], correct_resp_ips[1]) == 0) printf("%s ", correct_resp_ips[1]);
  if (!strcmp(correct_resp_ips[0], correct_resp_ips[2]) == 0 && 
      !strcmp(correct_resp_ips[1], correct_resp_ips[2]) == 0) printf("%s ", correct_resp_ips[2]);

  return 1;
}

int print_average_time(clock_t *start_times, clock_t *receive_times, int *seqs) {
  double sum_of_times = 0.0;

  for (int i = 0; i < N_PACKETS; i++) {

    // Check if all of the packets with a current ttl are received
    if (!receive_times[i]) return 0;

    sum_of_times += (receive_times[i] - start_times[seqs[i]]);
  }

  int average = (int)(sum_of_times / CLOCKS_PER_SEC * 100000) / 3;

  printf("%d ms\n", average);

  return 1;
}

/***********************
 * Core
 * *********************/

int receive_responses(int sockfd, int ttl, char *ip_dest, clock_t *start_times)
{
  fd_set descriptors = configure_descriptors(sockfd);
  struct sockaddr_in sender;
  socklen_t sender_len = sizeof(sender);
  uint8_t buffer[IP_MAXPACKET];

  // Set waiting time to MAX_WAIT
  struct timeval tv = { .tv_sec = MAX_WAIT, tv.tv_usec = 0 };

  // IPs, times, and seqs of the received packets
  char correct_resp_ips[N_PACKETS][IP_LENGTH];
  clock_t response_times[N_PACKETS] = {0};
  int seqs[3];
  
  // For strlen to return 0 in case of no overwriting
  for (int i = 0; i < N_PACKETS; i++) correct_resp_ips[i][0] = 0;

  // We want to receive 3 good responses
  int good_responses = 0;

  while (good_responses < N_PACKETS) {
    int ready = select(sockfd + 1, &descriptors, NULL, NULL, &tv);
    
    if (ready == 0) {
      // Timeout
      break;
    }
    if (ready < 0) {
      // Select error
      return EXIT_FAILURE;
    }

    // Everything ok
    ssize_t packet_len = recvfrom(
      sockfd,
      buffer,
      IP_MAXPACKET,
      MSG_DONTWAIT,
      (struct sockaddr *)&sender,
      &sender_len
    );
      
    // Stop measuring the time
    clock_t response_time = clock();

    // Check if the received packet is correct
    if (!packet_recv_correctness(packet_len)) return EXIT_FAILURE;

    char sender_ip_str[IP_LENGTH];

    // Extract ip to sender_ip_str
    extract_ip_from_hdr(sender, buffer, packet_len, sender_ip_str);

    int sender_seq, sender_id;

    // Extract sender id and seq
    extract_sender_id_and_seq(buffer, &sender_seq, &sender_id);

    // Check if the received packet is up-to-date and from the same process
    if (sender_seq / 3 == ttl && sender_id == getpid()) {
      strcpy(correct_resp_ips[good_responses], sender_ip_str);
      response_times[good_responses] = response_time;
      seqs[good_responses++] = sender_seq;
    }
  }

  // Print correct unique ips with the current ttl
  if (!print_unique_correct_ips(correct_resp_ips)) {
    puts("*");
    return 0;
  }

  // Print average time or '???' if received less than 3 packets
  if (!print_average_time(start_times, response_times, seqs)) puts("???");

  // Success if received a package from ip_dest
  for (int i = 0; i < N_PACKETS; i++) {
    if (!strcmp(correct_resp_ips[i], ip_dest)) return 1;
  }

  return 0;
}
