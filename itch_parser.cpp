#include "itch_parser.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

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
