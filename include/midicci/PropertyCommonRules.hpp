#pragma once

#include <string>

namespace midicci::commonproperties {

struct PropertyCommonHeaderKeys {
    static constexpr const char* RESOURCE = "resource";
    static constexpr const char* RES_ID = "resId";
    static constexpr const char* MUTUAL_ENCODING = "mutualEncoding";
    static constexpr const char* STATUS = "status";
    static constexpr const char* MESSAGE = "message";
    static constexpr const char* CACHE_TIME = "cacheTime";
    static constexpr const char* MEDIA_TYPE = "mediaType";
    static constexpr const char* OFFSET = "offset";
    static constexpr const char* LIMIT = "limit";
    static constexpr const char* TOTAL_COUNT = "totalCount";
    static constexpr const char* SET_PARTIAL = "setPartial";
    static constexpr const char* COMMAND = "command";
    static constexpr const char* SUBSCRIBE_ID = "subscribeId";
};

struct CommonRulesKnownMimeTypes {
    static constexpr const char* APPLICATION_JSON = "application/json";
};

struct PropertyExchangeStatus {
    static constexpr int OK = 200;
    static constexpr int ACCEPTED = 202;
    static constexpr int RESOURCE_UNAVAILABLE_OR_ERROR = 341;
    static constexpr int BAD_DATA = 342;
    static constexpr int TOO_MANY_REQUESTS = 343;
    static constexpr int BAD_REQUEST = 400;
    static constexpr int UNAUTHORIZED = 403;
    static constexpr int NOT_FOUND = 404;
    static constexpr int NOT_ALLOWED = 405;
    static constexpr int PAYLOAD_TOO_LARGE = 413;
    static constexpr int UNSUPPORTED_MEDIA_TYPE = 415;
    static constexpr int INVALID_DATA_VERSION = 445;
    static constexpr int INTERNAL_ERROR = 500;
};

struct PropertyDataEncoding {
    static constexpr const char* ASCII = "ASCII";
    static constexpr const char* MCODED7 = "Mcoded7";
    static constexpr const char* ZLIB_MCODED7 = "zlib+Mcoded7";
};

struct PropertyResourceNames {
    static constexpr const char* RESOURCE_LIST = "ResourceList";
    static constexpr const char* DEVICE_INFO = "DeviceInfo";
    static constexpr const char* CHANNEL_LIST = "ChannelList";
    static constexpr const char* JSON_SCHEMA = "JSONSchema";
    static constexpr const char* MODE_LIST = "ModeList";
    static constexpr const char* CURRENT_MODE = "CurrentMode";
    static constexpr const char* CHANNEL_MODE = "ChannelMode";
    static constexpr const char* BASIC_CHANNEL_RX = "BasicChannelRx";
    static constexpr const char* BASIC_CHANNEL_TX = "BasicChannelTx";
    static constexpr const char* LOCAL_ON = "LocalOn";
    static constexpr const char* EXTERNAL_SYNC = "ExternalSync";
};

struct DeviceInfoPropertyNames {
    static constexpr const char* MANUFACTURER_ID = "manufacturerId";
    static constexpr const char* FAMILY_ID = "familyId";
    static constexpr const char* MODEL_ID = "modelId";
    static constexpr const char* VERSION_ID = "versionId";
    static constexpr const char* MANUFACTURER = "manufacturer";
    static constexpr const char* FAMILY = "family";
    static constexpr const char* MODEL = "model";
    static constexpr const char* VERSION = "version";
    static constexpr const char* SERIAL_NUMBER = "serialNumber";
};

struct ChannelInfoPropertyNames {
    static constexpr const char* TITLE = "title";
    static constexpr const char* CHANNEL = "channel";
    static constexpr const char* PROGRAM_TITLE = "programTitle";
    static constexpr const char* BANK_PC = "bankPC";
    static constexpr const char* CLUSTER_CHANNEL_START = "clusterChannelStart";
    static constexpr const char* CLUSTER_LENGTH = "clusterLength";
    static constexpr const char* CLUSTER_MIDI_MODE = "clusterMidiMode";
    static constexpr const char* CLUSTER_TYPE = "clusterType";
};

struct PropertyResourceFields {
    static constexpr const char* RESOURCE = "resource";
    static constexpr const char* CAN_GET = "canGet";
    static constexpr const char* CAN_SET = "canSet";
    static constexpr const char* CAN_SUBSCRIBE = "canSubscribe";
    static constexpr const char* REQUIRE_RES_ID = "requireResId";
    static constexpr const char* MEDIA_TYPE = "mediaTypes";
    static constexpr const char* ENCODINGS = "encodings";
    static constexpr const char* SCHEMA = "schema";
    static constexpr const char* CAN_PAGINATE = "canPaginate";
    static constexpr const char* COLUMNS = "columns";
};

struct PropertySetAccess {
    static constexpr const char* NONE = "none";
    static constexpr const char* FULL = "full";
    static constexpr const char* PARTIAL = "partial";
};

struct MidiCISubscriptionCommand {
    static constexpr const char* START = "start";
    static constexpr const char* END = "end";
    static constexpr const char* NOTIFY = "notify";
    static constexpr const char* FULL = "full";
    static constexpr const char* PARTIAL = "partial";
};

} // namespace
