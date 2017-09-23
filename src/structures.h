#define ETH_MAC_LEN 6

struct beaconinfo
{
  uint8_t *bssid;
  uint8_t ssid[33];
  int ssid_len;
  int channel;
};

struct clientinfo
{
  uint8_t *bssid;
  uint8_t *transmitter;
};

/* ==============================================
   Promiscous callback structures, see ESP manual
   ============================================== */
struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;
  unsigned legacy_length: 12;
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;
  unsigned CWB: 1;
  unsigned HT_length: 16;
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;
  unsigned SGI: 1;
  unsigned rxend_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;
  unsigned: 12;
};

struct LenSeq {
  uint16_t length;
  uint16_t seq;
  uint8_t  address3[6];
};

struct sniffer_buf {
  struct RxControl rx_ctrl;
  uint8_t buf[36];
  uint16_t cnt;
  struct LenSeq lenseq[1];
};

struct sniffer_buf2 {
  struct RxControl rx_ctrl;
  uint8_t buf[112];
  uint16_t cnt;
  uint16_t len;
};

struct clientinfo parse_data(uint8_t *frame)
{
  struct clientinfo ci;

  uint8_t ds = frame[1] & 3;    // get just the tods/fromds bits
  switch (ds) {
    // p[1] - xxxx xx00 => NoDS   p[4]-DST p[10]-SRC p[16]-BSS
    case 0:
      ci.bssid = frame + 16;
      ci.transmitter = frame + 10;
      break;
    // p[1] - xxxx xx01 => ToDS   p[4]-BSS p[10]-SRC p[16]-DST
    case 1:
      ci.bssid = frame + 4;
      ci.transmitter = frame + 10;
      break;
    // p[1] - xxxx xx10 => FromDS p[4]-DST p[10]-BSS p[16]-SRC
    case 2:
      ci.bssid = frame + 10;
      ci.transmitter = frame + 10;
      break;
    // p[1] - xxxx xx11 => WDS    p[4]-RCV p[10]-TRM p[16]-DST p[26]-SRC
    case 3:
      ci.bssid = frame + 10;
      ci.transmitter = frame + 4;
      break;
  }

  return ci;
}

struct beaconinfo parse_beacon(uint8_t *frame, uint16_t framelen)
{
  struct beaconinfo bi;
  bi.ssid_len = 0;
  bi.channel = 0;
  int pos = 36;

  if (frame[pos] == 0x00) {
    while (pos < framelen) {
      switch (frame[pos]) {
        case 0x00: //SSID
          bi.ssid_len = (int) frame[pos + 1];
          if (bi.ssid_len == 0) {
            memset(bi.ssid, '\x00', 33);
            break;
          }
          if (bi.ssid_len < 0) {
            break;
          }
          if (bi.ssid_len > 32) {
            break;
          }
          memset(bi.ssid, '\x00', 33);
          memcpy(bi.ssid, frame + pos + 2, bi.ssid_len);
          break;
        case 0x03: //Channel
          bi.channel = (int) frame[pos + 2];
          pos = -1;
          break;
        default:
          break;
      }
      if (pos < 0) break;
      pos += (int) frame[pos + 1] + 2;
    }
  }

  bi.bssid = frame + 10;
  return bi;
}

