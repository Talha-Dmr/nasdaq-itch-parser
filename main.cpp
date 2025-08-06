#include "itch_parser.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

// --- Global Shared Data ---
std::queue<std::vector<char>> g_packet_queue;
std::mutex g_queue_mutex;

// --- Network Listener Function ---
void listen_feed(const char *group, int port, char feed_id) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket");
    return;
  }
  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                 sizeof(reuse)) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    close(sock);
    return;
  }
  struct sockaddr_in local_addr;
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
    perror("bind");
    close(sock);
    return;
  }
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(group);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) <
      0) {
    perror("setsockopt(IP_ADD_MEMBERSHIP)");
    close(sock);
    return;
  }
  std::cout << "Thread for Feed '" << feed_id << "' is listening on port "
            << port << std::endl;
  char recv_buffer[4096];
  while (true) {
    ssize_t nbytes =
        recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0, NULL, 0);
    if (nbytes > 0) {
      std::vector<char> packet(recv_buffer, recv_buffer + nbytes);
      std::lock_guard<std::mutex> lock(g_queue_mutex);
      g_packet_queue.push(packet);
    }
  }
  close(sock);
}

// --- Helper function to dispatch parsing ---
void dispatch_parse_call(const char *buffer) {
  char messageType = buffer[0];
  switch (messageType) {
  case 'S':
    parseSystemEventMessage(buffer);
    break;
  case 'R':
    parseStockDirectoryMessage(buffer);
    break;
  case 'A':
    parseAddOrderMessage(buffer);
    break;
  case 'F':
    parseAddOrderWithMPIDMessage(buffer);
    break;
  case 'E':
    parseOrderExecutedMessage(buffer);
    break;
  case 'C':
    parseOrderExecutedWithPriceMessage(buffer);
    break;
  case 'X':
    parseOrderCancelMessage(buffer);
    break;
  case 'D':
    parseOrderDeleteMessage(buffer);
    break;
  case 'U':
    parseOrderReplaceMessage(buffer);
    break;
  }
}

// --- Main Function (Updated with Advanced Gap Handling) ---
int main() {
  const char *MCAST_GROUP = "239.0.0.1";
  const int MCAST_PORT_A = 5007;
  const int MCAST_PORT_B = 5008;
  std::thread feed_a_thread(listen_feed, MCAST_GROUP, MCAST_PORT_A, 'A');
  std::thread feed_b_thread(listen_feed, MCAST_GROUP, MCAST_PORT_B, 'B');

  uint64_t expected_tracking_number = 1;
  std::map<uint64_t, std::vector<char>> gap_buffer;

  std::cout << "Starting processor loop with ADVANCED arbitration logic..."
            << std::endl;
  while (true) {
    std::vector<char> packet_to_process;
    {
      std::lock_guard<std::mutex> lock(g_queue_mutex);
      if (!g_packet_queue.empty()) {
        packet_to_process = g_packet_queue.front();
        g_packet_queue.pop();
      }
    }

    if (!packet_to_process.empty()) {
      const char *current_pos = packet_to_process.data();
      const char *end_pos = packet_to_process.data() + packet_to_process.size();

      while (current_pos < end_pos) {
        char messageType = *current_pos;
        unsigned int messageLength = 0;
        switch (messageType) {
        case 'S':
          messageLength = sizeof(SystemEventMessage);
          break;
        case 'R':
          messageLength = sizeof(StockDirectoryMessage);
          break;
        case 'A':
          messageLength = sizeof(AddOrderMessage);
          break;
        case 'F':
          messageLength = sizeof(AddOrderWithMPIDMessage);
          break;
        case 'E':
          messageLength = sizeof(OrderExecutedMessage);
          break;
        case 'C':
          messageLength = sizeof(OrderExecutedWithPriceMessage);
          break;
        case 'X':
          messageLength = sizeof(OrderCancelMessage);
          break;
        case 'D':
          messageLength = sizeof(OrderDeleteMessage);
          break;
        case 'U':
          messageLength = sizeof(OrderReplaceMessage);
          break;
        default:
          current_pos = end_pos;
          continue;
        }
        if (messageLength == 0 || (current_pos + messageLength > end_pos))
          break;

        CommonHeader header;
        std::memcpy(&header, current_pos, sizeof(CommonHeader));
        uint16_t incoming_tracking_number = ntohs(header.trackingNumber);

        if (incoming_tracking_number != 0) {
          if (incoming_tracking_number < expected_tracking_number) {
            // It's a duplicate, do nothing.
          } else if (incoming_tracking_number > expected_tracking_number) {
            if (gap_buffer.find(incoming_tracking_number) == gap_buffer.end()) {
              std::cout
                  << "\n[Arbitrator] Gap detected. Buffering future message #"
                  << incoming_tracking_number << std::endl;
              std::vector<char> msg_copy(current_pos,
                                         current_pos + messageLength);
              gap_buffer[incoming_tracking_number] = msg_copy;
            }
          } else {
            dispatch_parse_call(current_pos);
            expected_tracking_number++;
          }
        } else {
          dispatch_parse_call(current_pos);
        }

        while (gap_buffer.count(expected_tracking_number)) {
          std::cout << "[Arbitrator] Processing buffered message #"
                    << expected_tracking_number << " from gap." << std::endl;
          const std::vector<char> &buffered_message_vec =
              gap_buffer.at(expected_tracking_number);
          dispatch_parse_call(buffered_message_vec.data());
          gap_buffer.erase(expected_tracking_number);
          expected_tracking_number++;
        }
        current_pos += messageLength;
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  feed_a_thread.join();
  feed_b_thread.join();
  return 0;
}
