#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// --- Struct Definitions ---
// Ensure structs are packed without padding to match the exact network message
// format.
#pragma pack(push, 1)

// From ITCH 5.0 specification, section 1.1
struct SystemEventMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  char eventCode;
};

// From ITCH 5.0 specification, section 1.2.1
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

// From ITCH 5.0 specification, section 1.3.1
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

// From ITCH 5.0 specification, section 1.3.2
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

// From ITCH 5.0 specification, section 1.4.1
struct OrderExecutedMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  uint64_t orderReferenceNumber;
  uint32_t executedShares;
  uint64_t matchNumber;
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

  // Alpha fields are not null-terminated, so create a std::string with explicit
  // length.
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
  uint64_t orderRef =
      __builtin_bswap64(msg.orderReferenceNumber); // 8-byte swap
  uint32_t shares = ntohl(msg.shares);
  double price = ntohl(msg.price) / 10000.0; // Price has 4 decimal places

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

// --- Main ---
int main() {
  std::ifstream file("../test_data.bin", std::ios::binary);
  if (!file) {
    std::cerr << "Error: Could not open test_data.bin!" << std::endl;
    return 1;
  }

  std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                           (std::istreambuf_iterator<char>()));
  file.close();

  // Loop through the buffer, processing one message at a time.
  const char *current_pos = buffer.data();
  const char *end_pos = buffer.data() + buffer.size();

  while (current_pos < end_pos) {
    char messageType = *current_pos;
    unsigned int messageLength = 0;

    // Determine message length based on type and dispatch to the correct
    // parser.
    switch (messageType) {
    case 'S':
      messageLength = sizeof(SystemEventMessage);
      parseSystemEventMessage(current_pos);
      break;
    case 'R':
      messageLength = sizeof(StockDirectoryMessage);
      parseStockDirectoryMessage(current_pos);
      break;
    case 'A':
      messageLength = sizeof(AddOrderMessage);
      parseAddOrderMessage(current_pos);
      break;
    case 'F':
      messageLength = sizeof(AddOrderWithMPIDMessage);
      parseAddOrderWithMPIDMessage(current_pos);
      break;
    case 'E':
      messageLength = sizeof(OrderExecutedMessage);
      parseOrderExecutedMessage(current_pos);
      break;
    default:
      std::cerr << "\nWarning: Unknown or unhandled message type: '"
                << messageType << "'" << std::endl;
      // To prevent an infinite loop on an unknown type, we must exit or
      // advance.
      return 1;
    }

    if (messageLength == 0) {
      std::cerr << "Error: Message with zero length. Aborting." << std::endl;
      return 1;
    }

    // Advance the pointer to the next message
    current_pos += messageLength;
  }

  return 0;
}
