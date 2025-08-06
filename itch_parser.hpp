#ifndef ITCH_PARSER_HPP
#define ITCH_PARSER_HPP

#include <cstdint>
#include <string>
#include <vector>

// --- Struct Definitions ---
// Ensure structs are packed without padding to match the exact network message
// format.
#pragma pack(push, 1)

// A small helper struct to easily access the tracking number.
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

// --- Function Declarations ---
uint64_t reconstructTimestamp(const uint8_t a_timestamp[6]);
void parseSystemEventMessage(const char *a_buffer);
void parseStockDirectoryMessage(const char *a_buffer);
void parseAddOrderMessage(const char *a_buffer);
void parseAddOrderWithMPIDMessage(const char *a_buffer);
void parseOrderExecutedMessage(const char *a_buffer);
void parseOrderExecutedWithPriceMessage(const char *a_buffer);
void parseOrderCancelMessage(const char *a_buffer);
void parseOrderDeleteMessage(const char *a_buffer);
void parseOrderReplaceMessage(const char *a_buffer);

#endif // ITCH_PARSER_HPP
