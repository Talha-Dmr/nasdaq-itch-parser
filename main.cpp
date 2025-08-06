#include "itch_parser.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
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

// --- Main Function (Application Logic) ---
int main() {
  const char *MCAST_GROUP = "239.0.0.1";
  const int MCAST_PORT_A = 5007;
  const int MCAST_PORT_B = 5008;

  std::cout << "Launching listener threads..." << std::endl;
  std::thread feed_a_thread(listen_feed, MCAST_GROUP, MCAST_PORT_A, 'A');
  std::thread feed_b_thread(listen_feed, MCAST_GROUP, MCAST_PORT_B, 'B');

  uint64_t expected_tracking_number = 1;

  std::cout << "Starting processor loop with arbitration logic..." << std::endl;
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
          std::cerr << "\nWarning: Unknown message type '" << messageType
                    << "'." << std::endl;
          current_pos = end_pos;
          continue;
        }

        if (messageLength == 0) {
          break;
        }
        if (current_pos + messageLength > end_pos) {
          std::cerr << "Error: Incomplete message." << std::endl;
          break;
        }

        CommonHeader header;
        std::memcpy(&header, current_pos, sizeof(CommonHeader));
        uint16_t incoming_tracking_number = ntohs(header.trackingNumber);

        bool is_duplicate = false;
        if (incoming_tracking_number != 0) {
          if (incoming_tracking_number < expected_tracking_number) {
            is_duplicate = true;
            std::cout << "\n[Arbitrator] Duplicate message #"
                      << incoming_tracking_number << " received. Skipping."
                      << std::endl;
          } else if (incoming_tracking_number > expected_tracking_number) {
            std::cout << "\n[Arbitrator] GAP DETECTED! Expected: "
                      << expected_tracking_number
                      << ", but received: " << incoming_tracking_number
                      << std::endl;
            expected_tracking_number = incoming_tracking_number;
          }
          if (incoming_tracking_number >= expected_tracking_number) {
            expected_tracking_number++;
          }
        }

        if (!is_duplicate) {
          switch (messageType) {
          case 'S':
            parseSystemEventMessage(current_pos);
            break;
          case 'R':
            parseStockDirectoryMessage(current_pos);
            break;
          case 'A':
            parseAddOrderMessage(current_pos);
            break;
          case 'F':
            parseAddOrderWithMPIDMessage(current_pos);
            break;
          case 'E':
            parseOrderExecutedMessage(current_pos);
            break;
          case 'C':
            parseOrderExecutedWithPriceMessage(current_pos);
            break;
          case 'X':
            parseOrderCancelMessage(current_pos);
            break;
          case 'D':
            parseOrderDeleteMessage(current_pos);
            break;
          case 'U':
            parseOrderReplaceMessage(current_pos);
            break;
          }
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
