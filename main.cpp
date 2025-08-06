#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

// --- Global Shared Data ---
// A thread-safe queue to hold incoming data packets.
std::queue<std::vector<char>> g_packet_queue;
// A mutex to protect access to the queue.
std::mutex g_queue_mutex;

// --- Struct Definitions ---
#pragma pack(push, 1)

struct CommonHeader {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
};

struct SystemEventMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  char eventCode;
};
struct StockDirectoryMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  char stockSymbol[8];
  char marketCategory;
  char financialStatusIndicator;
  uint32_t roundLotSize;
  char roundLotsOnly;
  char issueClassification;
  char issueSubType[2];
  char authenticity;
  char shortSaleThresholdIndicator;
  char ipoFlag;
  char luldReferencePriceTier;
  char etpFlag;
  uint32_t etpLeverageFactor;
  char inverseIndicator;
};
struct AddOrderMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t orderReferenceNumber;
  char buySellIndicator;
  uint32_t shares;
  char stockSymbol[8];
  uint32_t price;
};
struct AddOrderWithMPIDMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t orderReferenceNumber;
  char buySellIndicator;
  uint32_t shares;
  char stockSymbol[8];
  uint32_t price;
  char attribution[4];
};
struct OrderExecutedMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t orderReferenceNumber;
  uint32_t executedShares;
  uint64_t matchNumber;
};
struct OrderExecutedWithPriceMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t orderReferenceNumber;
  uint32_t executedShares;
  uint64_t matchNumber;
  char printable;
  uint32_t executionPrice;
};
struct OrderCancelMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t orderReferenceNumber;
  uint32_t canceledShares;
};
struct OrderDeleteMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t orderReferenceNumber;
};
struct OrderReplaceMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t originalOrderReferenceNumber;
  uint64_t newOrderReferenceNumber;
  uint32_t shares;
  uint32_t price;
};

#pragma pack(pop)

// --- Helper Functions ---
uint64_t reconstructTimestamp(const uint8_t a_timestamp[6]) {
  uint64_t timestamp = 0;
  for (int i = 0; i < 6; ++i) {
    timestamp = (timestamp << 8) | a_timestamp[i];
  }
  return timestamp;
}

// --- Message Parsers ---
void parseSystemEventMessage(const char *a_buffer) {
  SystemEventMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(SystemEventMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  std::cout << "\n--- Parsed System Event ('S') ---" << std::endl;
  std::cout << "Timestamp:  " << timestamp << std::endl;
  std::cout << "Event Code: " << msg.eventCode << std::endl;
}

void parseStockDirectoryMessage(const char *a_buffer) {
  StockDirectoryMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(StockDirectoryMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint32_t roundLotSize = ntohl(msg.roundLotSize);
  std::string stockSymbol(msg.stockSymbol, sizeof(msg.stockSymbol));
  std::cout << "\n--- Parsed Stock Directory ('R') ---" << std::endl;
  std::cout << "Timestamp:    " << timestamp << std::endl;
  std::cout << "Stock Symbol: " << stockSymbol << std::endl;
  std::cout << "Round Lot:    " << roundLotSize << std::endl;
}

void parseAddOrderMessage(const char *a_buffer) {
  AddOrderMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(AddOrderMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint64_t orderRef = __builtin_bswap64(msg.orderReferenceNumber);
  uint32_t shares = ntohl(msg.shares);
  double price = ntohl(msg.price) / 10000.0;
  std::string stockSymbol(msg.stockSymbol, sizeof(msg.stockSymbol));
  std::cout << "\n--- Parsed Add Order ('A') ---" << std::endl;
  std::cout << "Timestamp: " << timestamp << " | Order Ref: " << orderRef
            << std::endl;
  std::cout << "Side: " << msg.buySellIndicator << " | Shares: " << shares
            << " | Symbol: " << stockSymbol << " | Price: " << price
            << std::endl;
}

void parseAddOrderWithMPIDMessage(const char *a_buffer) {
  AddOrderWithMPIDMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(AddOrderWithMPIDMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint64_t orderRef = __builtin_bswap64(msg.orderReferenceNumber);
  uint32_t shares = ntohl(msg.shares);
  double price = ntohl(msg.price) / 10000.0;
  std::string stockSymbol(msg.stockSymbol, sizeof(msg.stockSymbol));
  std::string attribution(msg.attribution, sizeof(msg.attribution));
  std::cout << "\n--- Parsed Add Order w/ MPID ('F') ---" << std::endl;
  std::cout << "Timestamp: " << timestamp << " | Order Ref: " << orderRef
            << std::endl;
  std::cout << "Side: " << msg.buySellIndicator << " | Shares: " << shares
            << " | Symbol: " << stockSymbol << " | Price: " << price
            << " | MPID: " << attribution << std::endl;
}

void parseOrderExecutedMessage(const char *a_buffer) {
  OrderExecutedMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(OrderExecutedMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint64_t orderRef = __builtin_bswap64(msg.orderReferenceNumber);
  uint32_t executedShares = ntohl(msg.executedShares);
  uint64_t matchNumber = __builtin_bswap64(msg.matchNumber);
  std::cout << "\n--- Parsed Order Executed ('E') ---" << std::endl;
  std::cout << "Timestamp: " << timestamp << " | Order Ref: " << orderRef
            << std::endl;
  std::cout << "Executed Shares: " << executedShares
            << " | Match #: " << matchNumber << std::endl;
}

void parseOrderExecutedWithPriceMessage(const char *a_buffer) {
  OrderExecutedWithPriceMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(OrderExecutedWithPriceMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint64_t orderRef = __builtin_bswap64(msg.orderReferenceNumber);
  uint32_t executedShares = ntohl(msg.executedShares);
  uint64_t matchNumber = __builtin_bswap64(msg.matchNumber);
  double executionPrice = ntohl(msg.executionPrice) / 10000.0;
  std::cout << "\n--- Parsed Order Executed w/ Price ('C') ---" << std::endl;
  std::cout << "Timestamp: " << timestamp << " | Order Ref: " << orderRef
            << std::endl;
  std::cout << "Executed Shares: " << executedShares
            << " | Match #: " << matchNumber << " | Price: " << executionPrice
            << std::endl;
}

void parseOrderCancelMessage(const char *a_buffer) {
  OrderCancelMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(OrderCancelMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint64_t orderRef = __builtin_bswap64(msg.orderReferenceNumber);
  uint32_t canceledShares = ntohl(msg.canceledShares);
  std::cout << "\n--- Parsed Order Cancel ('X') ---" << std::endl;
  std::cout << "Timestamp: " << timestamp << " | Order Ref: " << orderRef
            << " | Canceled Shares: " << canceledShares << std::endl;
}

void parseOrderDeleteMessage(const char *a_buffer) {
  OrderDeleteMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(OrderDeleteMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint64_t orderRef = __builtin_bswap64(msg.orderReferenceNumber);
  std::cout << "\n--- Parsed Order Delete ('D') ---" << std::endl;
  std::cout << "Timestamp: " << timestamp << " | Order Ref: " << orderRef
            << std::endl;
}

void parseOrderReplaceMessage(const char *a_buffer) {
  OrderReplaceMessage msg;
  std::memcpy(&msg, a_buffer, sizeof(OrderReplaceMessage));
  uint64_t timestamp = reconstructTimestamp(msg.timestamp);
  uint64_t originalOrderRef =
      __builtin_bswap64(msg.originalOrderReferenceNumber);
  uint64_t newOrderRef = __builtin_bswap64(msg.newOrderReferenceNumber);
  uint32_t shares = ntohl(msg.shares);
  double price = ntohl(msg.price) / 10000.0;
  std::cout << "\n--- Parsed Order Replace ('U') ---" << std::endl;
  std::cout << "Timestamp: " << timestamp << " | Orig Ref: " << originalOrderRef
            << " -> New Ref: " << newOrderRef << std::endl;
  std::cout << "New Shares: " << shares << " | New Price: " << price
            << std::endl;
}

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

// --- Main Function ---
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
                    << "' at offset "
                    << (current_pos - packet_to_process.data())
                    << ". Stopping processing of this packet." << std::endl;
          current_pos = end_pos;
          continue;
        }

        if (messageLength == 0) {
          break;
        }

        if (current_pos + messageLength > end_pos) {
          std::cerr << "Error: Incomplete message of type '" << messageType
                    << "'. Aborting packet." << std::endl;
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
